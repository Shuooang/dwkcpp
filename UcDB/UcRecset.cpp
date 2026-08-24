#include "pch.h"

#include <vector>
#include <cstring>

#include <sql.h>
#include <sqlext.h>
#include <atldbcli.h>

#include "UcRecset.h" // UcDb/UcRecset.h
#include "UcDbCol.h"
#include "UcTool/UcTool.h"
#include "UcTool/UcJson.h"

namespace {

constexpr SQLULEN kSqlLongTextColumnSize = 16777215; // MEDIUMTEXT upper bound

CStringW SqlTypeToJsonType(SWORD sqlType)
{
	switch (sqlType)
	{
	case SQL_TINYINT:
	case SQL_SMALLINT:
	case SQL_INTEGER:
		return L"int";
	case SQL_BIGINT:
		return L"int";
	case SQL_FLOAT:
	case SQL_REAL:
	case SQL_DOUBLE:
		return L"double";
	case SQL_BIT:
		return L"bool";
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_WCHAR:
	case SQL_WVARCHAR:
	case SQL_WLONGVARCHAR:
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP:
		return L"string";
	default:
		return L"string";
	}
}

/// 동적 SQL용 최소 `CRecordset` — 컬럼은 `GetFieldValue` / `GetODBCFieldInfo` 로 처리.
class CUcDynamicRecordset : public CRecordset
{
public:
	explicit CUcDynamicRecordset(CDatabase* pdb)
		: CRecordset(pdb)
	{
	}

	CString GetDefaultConnect() override { return _T(""); }
	CString GetDefaultSQL() override { return _T(""); }

	void DoFieldExchange(CFieldExchange* pFX) override
	{
		(void)pFX;
	}
};

CStringW OdbcDiagMessage(SQLSMALLINT handleType, SQLHANDLE handle, SQLRETURN rc)
{
	CStringW msg;
	msg.Format(L"ODBC error %d", (int)rc);
	if (!handle)
		return msg;
	SQLWCHAR state[6]{};
	SQLWCHAR text[1024]{};
	SQLINTEGER native = 0;
	SQLSMALLINT textLen = 0;
	const SQLRETURN dr = SQLGetDiagRecW(handleType, handle, 1, state, &native, text, (SQLSMALLINT)(std::size(text)), &textLen);
	if (SQL_SUCCEEDED(dr) && textLen > 0)
		msg.Format(L"[%s] %s (native %d)", (LPCWSTR)state, (LPCWSTR)text, (int)native);
	return msg;
}

SQLRETURN BindSqlParam(HSTMT hstmt, SQLUSMALLINT paramIndex, const UcSqlParam& p, SQLLEN& strLenOrInd)
{
	const SQLUSMALLINT col = paramIndex;
	strLenOrInd = 0;

	switch (p.kind)
	{
	case UcSQL_NULL:
		strLenOrInd = SQL_NULL_DATA;
		return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
			0, 0, (SQLPOINTER)nullptr, 0, &strLenOrInd);

	case UcSQL_STRING:
	{
		auto* ws = static_cast<LPCWSTR>(p.data);
		if (!ws || !*ws) {
			strLenOrInd = SQL_NULL_DATA;
			return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WLONGVARCHAR,
				0, 0, (SQLPOINTER)nullptr, 0, &strLenOrInd);
		}
		strLenOrInd = SQL_NTS;
		return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WLONGVARCHAR,
			kSqlLongTextColumnSize, 0, (SQLPOINTER)ws, 0, &strLenOrInd);
	}

	case UcSQL_UTF8:
	{
		auto* u8 = static_cast<LPCSTR>(p.data);
		if (!u8 || !*u8) {
			strLenOrInd = SQL_NULL_DATA;
			return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
				0, 0, (SQLPOINTER)nullptr, 0, &strLenOrInd);
		}
		strLenOrInd = SQL_NTS;
		return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
			kSqlLongTextColumnSize, 0, (SQLPOINTER)u8, 0, &strLenOrInd);
	}

	case UcSQL_INT32:
	{
		auto* pv = static_cast<const int*>(p.data);
		if (!pv) {
			strLenOrInd = SQL_NULL_DATA;
			return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
				0, 0, (SQLPOINTER)nullptr, 0, &strLenOrInd);
		}
		return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
			0, 0, (SQLPOINTER)pv, 0, nullptr);
	}

	case UcSQL_INT64:
	{
		auto* pv = static_cast<const LONGLONG*>(p.data);
		if (!pv) {
			strLenOrInd = SQL_NULL_DATA;
			return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT,
				0, 0, (SQLPOINTER)nullptr, 0, &strLenOrInd);
		}
		return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT,
			0, 0, (SQLPOINTER)pv, 0, nullptr);
	}
#pragma region MemoApp//[
	case UcSQL_BINARY:
	{
		auto* pb = static_cast<const UcSqlParamBinary*>(p.data);
		if (!pb || !pb->data || pb->cbLen <= 0) {
			strLenOrInd = SQL_NULL_DATA;
			return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, pb ? pb->cType : SQL_C_BINARY,
				pb ? pb->sqlType : SQL_LONGVARBINARY, 0, 0, (SQLPOINTER)nullptr, 0, &strLenOrInd);
		}
		strLenOrInd = pb->cbLen;
		return SQLBindParameter(hstmt, col, SQL_PARAM_INPUT, pb->cType, pb->sqlType,
			pb->cbLen, 0, (SQLPOINTER)pb->data, 0, &strLenOrInd);
	}
#pragma endregion//]
	default:
		return SQL_ERROR;
	}
}

} // namespace

CUcRecset::CUcRecset() = default;

CUcRecset::~CUcRecset()
{
	Close();
}

void CUcRecset::NoteSql(LPCWSTR psql)
{
	if (!psql || !*psql)
		return;
	CStringW sql(psql);
	sql.Trim();
	if (sql.IsEmpty())
		return;
	_lstSql.push_back(sql.GetString());
	if (_lstSql.size() > 10)
		_lstSql.pop_front();
}

void CUcRecset::RecordSqlLog(LPCWSTR psql)
{
	if (_sqlLogSuppress > 0 || !psql || !*psql)
		return;
	CStringW sql(psql);
	sql.Trim();
	if (sql.IsEmpty())
		return;
	LPCWSTR err = m_lastError.IsEmpty() ? nullptr : (LPCWSTR)m_lastError;
	InsertSqlLog(sql, err);
}

void CUcRecset::InsertSqlLog(LPCWSTR /*sql*/, LPCWSTR /*err*/)
{
}

BOOL CUcRecset::IsOpen() const
{
	return m_db.IsOpen();
}

BOOL CUcRecset::Open()
{
	DWKFUNC;
	if (m_db.IsOpen())
		return TRUE;

	CStringW conn;
	if (!m_connOverride.IsEmpty())
		conn = m_connOverride;
	else
	{
		// ODBC 등록된 DSN 없고 override도 없다면: MariaDB 직통 문자열(윈도우 DSN 테스트의 Sample)
		conn.Format(L"DRIVER={MariaDB ODBC 3.2 Driver};TCPIP=1;"
			L"SERVER=%s;DATABASE=%s;"
			//L"DSN=%s;"
			L"UID=%s;PWD={%s};" /// 특수 문자 있는 경우 {}로 싼다.
			L"PORT=3306;NULLISCURRENT=1;PSCACHESIZE=250;MAXCACHEKEY=2112;PCALLBACK=1;"
			, m_server.GetString(), m_database.GetString(), //m_dsn.GetString(), 
			m_uid.GetString(), m_pwd.GetString());
	}
	//	conn.Format(L"DSN=%s;UID=%s;PWD=%s", (LPCWSTR)m_dsn, (LPCWSTR)m_uid, (LPCWSTR)m_pwd);

	/// ODBC DSN이 시스템 ODBC에 정상 등록된 경우라면 DSN 이름만 주면 충분합니다.
	// 그러나 DSN이 레지스트리에만 있거나, 사용자가 DSN-less(DSN 없이) 연결을 하고 싶을 경우,
	// 즉, 시스템 ODBC에 등록되어 있지 않은 환경에서는 아래처럼 전체 연결 문자열(드라이버, 서버, DB, 아이디, 암호 등)을 모두 지정해야 합니다.
	// 예:
	//   Driver={SQL Server};Server=서버IP또는이름;Database=DB명;UID=사용자;PWD=비밀번호;
	// 위 내용을 app 설정 또는 레지스트리에서 읽어서 m_connOverride 등에 세팅해 두면 됩니다.
	// 1. DSN이 시스템 ODBC에 등록되어 있는지(odbcad32.exe 등에서 확인)

	//if (m_dsn.IsEmpty() && m_connOverride.IsEmpty())
	//DRIVER={MariaDB ODBC 3.2 Driver};TCPIP=1;SERVER=localhost;UID=root;PWD=*******};              PORT=3306;NULLISCURRENT=1;PSCACHESIZE=250;MAXCACHEKEY=2112;PCALLBACK=1
	//DRIVER={MariaDB ODBC 3.2 Driver};TCPIP=1;SERVER=localhost;UID=ngsadmin;PWD=***};DATABASE=ngsx;PORT=3306;NULLISCURRENT=1;PSCACHESIZE=250;MAXCACHEKEY=2112;PCALLBACK=1
	//---------------------------
	//Server Information: MariaDB 12.02.000002

	DWKTRACE(L"%s", UcMaskConnPwd(conn));
	BOOL rb{ FALSE };
	try
	{
		rb = m_db.OpenEx(conn, CDatabase::noOdbcDialog);
		m_lastError.Empty();
		return TRUE;
	}
	catch (CDBException* e)///ODBC connector 를 설치해야 한다.
	{
		m_lastError = e->m_strError;/// https://mariadb.com/downloads/connectors/connectors-data-access/odbc-connector/
		DWKTRACE(L"DB Open Exception: %s (user: %s, server: %s, db: %s)", m_lastError.GetString(), m_uid.GetString(), m_server.GetString(), m_database.GetString());

		// 만약 에러 문자열이 'Access denied for user ... (using password: YES)' 형태라면,
		// 이는 입력한 계정(ngsadmin)/비밀번호 혹은 해당 계정의 DB 접근 권한 문제입니다.
		// (1) 아이디 또는 비밀번호 오타
		// (2) DB서버의 사용자 권한 미부여/설정 문제 (ex. localhost에서의 접근권한 또는 비밀번호 불일치)
		// (3) MariaDB/MySQL 계정이 'localhost'에서의 접속을 허용하지 않음
		// (4) MariaDB 10.4 이상에서는 비밀번호 인증 방식(caching_sha2 등)이 맞지 않을 수도 있음
		// (5) 방화벽/접속 제어 정책 등도 확인 필요
		// 해결: DB에서 GRANT 권한, 비밀번호, 사용자/호스트 허용 범위, 인증방식을 확인해 주세요.
		no_throw_str(m_lastError);
		e->Delete();
		return FALSE;
	}
}

void CUcRecset::Close()
{
	if (m_db.IsOpen())
		m_db.Close();
}

BOOL CUcRecset::EnsureOpen()
{
	if (m_db.IsOpen())
		return TRUE;
	return Open();
}

BOOL CUcRecset::TryReopen()
{
	Close();
	return Open();
}

bool CUcRecset::IsConnectionError() const
{
	if (m_lastError.IsEmpty())
		return !m_db.IsOpen();
	CStringW e(m_lastError);
	e.MakeLower();
	return e.Find(L"연결") >= 0
		|| e.Find(L"connection") >= 0
		|| e.Find(L"communication link") >= 0
		|| e.Find(L"disconnected") >= 0
		|| e.Find(L"not connected") >= 0
		|| e.Find(L"열려 있지") >= 0
		|| e.Find(L"invalid handle") >= 0
		|| e.Find(L"08s01") >= 0
		|| e.Find(L"08003") >= 0
		|| e.Find(L"08001") >= 0
		|| e.Find(L"im002") >= 0; // driver not found / data source name not found sometimes after drop
}

//----------------------------------------------------------------------
// SELECT 절의 필드명을 추출한다.
// - alias.fName
// - fName
// - alias.fName AS xxx
// - alias.fName xxx
// 모두 지원
//
// 규칙:
//   실제 DB 필드명은 반드시 'f' 로 시작해야 한다.
//   아니면 ASSERT(0)
//
// 예:
//   SELECT a.fName, b.fAge AS Age, COUNT(*) cnt
//   -> fName, fAge
//
// 주의:
//   함수/COUNT(*)/CASE 등은 무시한다.
//----------------------------------------------------------------------

#include <regex>
#include <set>
inline std::vector<CStringW> ExtractAllFieldsFromSql(const CStringW& sql)
{
	std::vector<CStringW> fields;
	std::set<CStringW> uniqueSet;

	CStringW work;
	work.Preallocate(sql.GetLength());
	bool inString = false;

	for (int i = 0; i < sql.GetLength(); ++i)
	{
		const wchar_t ch = sql[i];
		if (ch == L'\''){
			inString = !inString;
			work += L' ';
			continue;
		}
		work += inString ? L' ' : ch;
	}

	std::wregex re(LR"((?:\b[A-Za-z_][A-Za-z0-9_]*\s*\.\s*)?(f[A-Za-z0-9_]+))");
	// 일부러 icase 안 씀. FROM 잡히면 안 됨.

	const wchar_t* first = work.GetString();
	const wchar_t* last = first + work.GetLength();

	std::wcregex_iterator it(first, last, re);
	std::wcregex_iterator end;

	for (; it != end; ++it)
	{
		CStringW fld = (*it)[1].str().c_str();
		fld.Trim();
		if (fld.IsEmpty())
			continue;
		if (fld[0] != L'f'){
			ASSERT(0);
			continue;
		}
		if (uniqueSet.insert(fld).second){
			fields.push_back(fld);
			//UcJObj::FieldCheckAgainstLoadedSqlBackupFields(fld);
		}
	}
	return fields;
}

SHP<UcJTable> CUcRecset::QueryToTableJson(LPCWSTR psql)// , LPCWSTR tableKey, UcJObj& outRoot1)
{
	DWKFUNC;
	CStringW sql(psql);
	sql.Trim();
	NoteSql(sql);
	KDefer deferSqlLog([&]() { RecordSqlLog(sql); });
#ifdef _DEBUG
	std::vector<CStringW> selectFields = ExtractAllFieldsFromSql(sql);
	for (auto& fld : selectFields) {
		// CUcRecset이 UcJObj::FieldCheckAgainstLoadedSqlBackupFields 를 호출할 수 있다고 가정
		jstring jfld(fld.GetString());
		UcJObj::FieldCheckAgainstLoadedSqlBackupFields(jfld); // 외부 static 또는 글로벌 함수여야 함
	}
#endif // _DEBUG

#ifdef _DEBUGx
	// 쿼리문에서 select된 필드명 추출 (단순 파싱, 서브쿼리/함수 호출 등 복잡한 경우는 제한됨)
	{
		CStringW upperSql = sql;
		upperSql.MakeLower(); // 소문자로 통일 (키워드 감지 목적)

		int selectPos = upperSql.Find(L"select");
		int fromPos = upperSql.Find(L"from");
		
		if (selectPos >= 0 && fromPos > selectPos) {
			CStringW fieldStr = sql.Mid(selectPos + 6, fromPos - (selectPos + 6));
			// 콤마(,)로 필드 분리, 트림 작업
			std::vector<CStringW> selectFields;
			int cur = 0;
			while (true) {
				int commaPos = fieldStr.Find(L',', cur);
				CStringW one;
				if (commaPos < 0) {
					one = fieldStr.Mid(cur);
				} else {
					one = fieldStr.Mid(cur, commaPos - cur);
				}
				one.Trim();
				if (!one.IsEmpty()) {
					// alias가 있을 경우 공백 뒤에 별칭이 올 수 있음: "name as username"
					int asPos = one.Find(L" as ");
					if (asPos >= 0) {
						one = one.Mid(asPos + 4);
						one.Trim();
					} else {
						// 끝 쪽 별칭 지원 (공백 기준, ex: "name username")
						int lastSpace = one.ReverseFind(L' ');
						if (lastSpace >= 0) {
							CStringW possibleAlias = one.Mid(lastSpace + 1);
							if (!possibleAlias.IsEmpty() && possibleAlias.CompareNoCase(L"asc") != 0 && possibleAlias.CompareNoCase(L"desc") != 0) {
								one = possibleAlias;
							}
						}
					}
					selectFields.push_back(one);
				}
				if (commaPos < 0)
					break;
				cur = commaPos + 1;
			}
			// 모든 필드명에 대해 체크
			for (auto& fld : selectFields) {
				// CUcRecset이 UcJObj::FieldCheckAgainstLoadedSqlBackupFields 를 호출할 수 있다고 가정
				jstring jfld(fld.GetString());
				UcJObj::FieldCheckAgainstLoadedSqlBackupFields(jfld); // 외부 static 또는 글로벌 함수여야 함
			}
		}
	}
#endif

	m_lastError.Empty();//|| !tableKey || !*tableKey
	if (!sql || !*sql) {// NULL 또는 길이 0인 SQL 문자열 체크.
		SetLastError(L"QueryToTableJson: sql 또는 tableKey 가 비었습니다.");
		return {};
	}
	if (!EnsureOpen()) {
		if (m_lastError.IsEmpty())
			SetLastError(L"QueryToTableJson: DB 가 열려 있지 않습니다.");
		return {};
	}

	thread_local int s_qryRetryDepth = 0;

	CUcDynamicRecordset rs(&m_db);
	try {
		///주의: sql 앞에 공백문자 있으면 걍 죽어 버린다. 예외도 아니고.
		///		alias 에 s.Hst 점이 들어 가면 오류
		DWKTRACE(L"sql: %v", sql);
		rs.Open(CRecordset::forwardOnly, sql, CRecordset::readOnly);
	}
	catch (CDBException* e)
	{
		m_lastError = e->m_strError;
		e->Delete();
		DWKTRACE(L"CDBException:%v", m_lastError);
		if (s_qryRetryDepth == 0 && IsConnectionError() && TryReopen()) {
			++s_qryRetryDepth;
			auto ret = QueryToTableJson(psql);
			--s_qryRetryDepth;
			return ret;
		}
		return {};
	}
	catch (CException* e)
	{
		DWKTRACE(L"CException"); e;
		return {};
	}
	catch (...) {
		DWKTRACE(L"exception...");
	}

	ShJVal shTbl2 = make_shared<UcJObj>(); //outRoot1.O(tableKey, true);
	//if (!shTbl2 || !shTbl2->IsDic())
	//{
	//	rs.Close();
	//	SetLastError(L"QueryToTableJson: UcJObj 블록을 만들 수 없습니다.");
	//	return FALSE;
	//}

	UcJObj* pTbl3 = shTbl2->Dic();
	pTbl3->Format(L"type", L"%s", L"table");
	UcJArr& fields4 = pTbl3->SetArray(L"fields");
	UcJArr& rows4 = pTbl3->SetArray(L"rows");

	std::vector<CStringW> colNames;
	short nFields = rs.m_nResultCols; // m_nFields;는 항상 0이다. 뭐지 이 쓰레기는?
	colNames.reserve((size_t)nFields);

	short maxTryColumns = 64; // 적당히 안전하게 한도

	// m_nFields가 0이어도, CRecordset 내의 m_mapFieldIndex나 GetODBCFieldInfo, GetFieldValue 등에서 뭔가 직접 쥘 수 있는지 확인한다.
	// 먼저 GetODBCFieldInfo를 사용해보면, nFields==0에서 실패하고 예외가 나지만,
	// .GetFieldValue는 0-based 인덱스로 컬럼 값을 읽을 수 있고, CRecordset 내부의 m_mapFieldIndex도 있을 수 있다.
	if (!rs.IsEOF()) {
		// 컬럼 인덱스 0, 1, 2, ...로 시도해서 값을 읽어보고, 내부적으로 필드 이름 정보를 얻는 방법을 찾는다.
		// MFC CRecordset 내부의 m_mapFieldIndex/m_mapFieldInfo가 populated 되어 있는지 확인:
		maxTryColumns = rs.m_nResultCols;

		/// rs.m_nFields는 제대로 설정이 안 되어 있을 수 있고, 
		//nFields = rs.m_nResultCols;
		// GetFieldValue는 실제 데이터 읽을 때 한 번만 쓰고,
		// 컬럼 이름이나 타입 추출은 GetODBCFieldInfo로 한 번만 돌리면 된다.
		for (short tryCol = 0; tryCol < maxTryColumns; ++tryCol) 
		{
			// CDBVariant value;
			// rs.GetFieldValue(tryCol, value); //+m_pstring	L"svr_0022"	
			CStringW colName;
			CStringW jsType;
			try {
				CODBCFieldInfo fi;
				rs.GetODBCFieldInfo(tryCol, fi);
				// 컬럼 인덱스가 유효 범위를 넘으면, GetODBCFieldInfo가 예외를 던지는 대신 죽어버릴 수 있으므로,
				colName = fi.m_strName; // L"fSidServerID", table alaias가 있는데 's.'이 빠지고 뒤 fHstHost 만 나오네
				// s.fHstHost s_Hst 하면 뒤  s_Hst 가 나온다.
				jsType = SqlTypeToJsonType(fi.m_nSQLType);
				//DWKTRACE(L"col %v. %v, %v:%v", tryCol, colName, fi.m_nSQLType, jsType);
			}
			catch (CDBException* e) {
				// 이름 못 얻어도 그냥 진행
				colName.Format(L"col%d", tryCol+1);
				e->Delete();
			}
			catch(CException* e){
			//SELECT a.fCidClientID, c.fIpAddress, c.fMacAddr, c.fPcName
			//from tauthclient a 
			//	join tclients c ON a.fCidClientID = c.fCidClientID
			//		WHERE c.fIpAddress = '192.168.0.157' and c.fMacAddr = '0A-00-27-00-00-05'
				_break; e;
			}
			catch(...){
				_break;
			}

			// GetFieldValue를 호출하지 않으므로 value.m_dwType이 의미 없음.
			// 대신 fi.m_nSQLType을 이용해서 타입을 판별한다.
			if (!colName.IsEmpty()) {// && gotValue
				UcJObj& fo = fields4.AddObj();
				fo("name") = colName;
				fo("type") = jsType;
			}
		}
		// 실제로 시도해서 찾아낸 컬럼 갯수를 nFields로 대입해서, 아래의 row 처리 루프에서도 활용할 수 있게 한다.
		nFields = (short)fields4.size();
	}

	DWKTRACE(L"----------------------------------------------");
	m_resultTruncated = false;
	m_fetchedRows = 0;
	size_t totalBytes = 0;
	int row = 0;
	while (!rs.IsEOF())
	{
		if (m_maxRow > 0 && row >= m_maxRow) {
			m_resultTruncated = true;
			SetLastError(L"QueryToTableJson: 행 수 상한 초과 (LIMIT로 줄이세요)");
			break;
		}
		if (m_maxResultBytes > 0 && totalBytes >= m_maxResultBytes) {
			m_resultTruncated = true;
			SetLastError(L"QueryToTableJson: 결과 크기 상한 초과 (LIMIT로 줄이세요)");
			break;
		}

		KDefer d_row([&row]() {row++; });
		auto shRow5 = NEWSHP(UcJArr);

		for (short c = 0; c < nFields; ++c)
		{
			CString textValue;
			try {
				rs.GetFieldValue((short)c, textValue);
			//   void GetFieldValue(short nIndex, long& lValue); 다른 타입으로 가져올수도 있지만 내부 변수가 wstring이므로
			}
			catch (CDBException* e)
			{
				m_lastError = e->m_strError;
				e->Delete();
				rs.Close();
				return {};
			}
			const CStringW cellW(textValue);
			totalBytes += (size_t)cellW.GetLength() * sizeof(wchar_t);
			shRow5->Add(cellW);
			//DWKTRACE(L"[%v,%v] %v", row, c, textValue);
		}
		if (m_maxResultBytes > 0 && totalBytes > m_maxResultBytes) {
			m_resultTruncated = true;
			SetLastError(L"QueryToTableJson: 결과 크기 상한 초과 (LIMIT로 줄이세요)");
			break;
		}
		rows4.Add(NEWSHP(JVal, shRow5, false), false);
		m_fetchedRows = row + 1;
		rs.MoveNext();
	}

	rs.Close();
	// 상한 초과 시에도 이미 가져온 행은 유지한다.
	// (예전: return {} → DoQuery 실패 → 목록 DeleteAllItems 후 빈 화면)
	if (m_resultTruncated && m_fetchedRows <= 0)
		return {};
	if (shTbl2 && shTbl2->IsDic())
		return UcJObj::Table(shTbl2);
		//return shTbl2->Dic()->Table();
	return {};
}

BOOL CUcRecset::ExecuteCommand(LPCWSTR pSql)//, LPCWSTR logicalKey, LPCWSTR cmdType, UcJObj& outRoot)
{
	DWKFUNC;
	CStringW sql(pSql);
	sql.Trim();
	m_lastError.Empty();
	if (sql.IsEmpty())//!sql || !*sql)// || !logicalKey || !*logicalKey || !cmdType || !*cmdType)
	{
		SetLastError(L"ExecuteCommand: SQL 문이 비었습니다.");
		return FALSE;
	}
	if (!EnsureOpen())
	{
		if (m_lastError.IsEmpty())
			SetLastError(L"ExecuteCommand: DB 가 열려 있지 않습니다.");
		return FALSE;
	}

	NoteSql(sql);
	KDefer deferSqlLog([&]() { RecordSqlLog(sql); });

	for (int attempt = 0; attempt < 2; ++attempt)
	{
		try
		{
			m_db.ExecuteSQL(sql);
			return TRUE;
		}
		catch (CDBException* e)
		{
			SetLastError(e->m_strError);
			e->Delete();
			if (attempt == 0 && IsConnectionError() && TryReopen())
				continue;
			return FALSE;
		}
	}
	return FALSE;
}

BOOL CUcRecset::BeginTransaction()
{
	m_lastError.Empty();
	if (!m_db.IsOpen()) {
		SetLastError(L"BeginTransaction: DB 가 열려 있지 않습니다.");
		return FALSE;
	}
	try {
		if (!m_db.BeginTrans()) {
			SetLastError(L"BeginTransaction failed");
			return FALSE;
		}
	}
	catch (CDBException* e) {
		SetLastError(e->m_strError);
		e->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CUcRecset::CommitTransaction()
{
	m_lastError.Empty();
	if (!m_db.IsOpen()) {
		SetLastError(L"CommitTransaction: DB 가 열려 있지 않습니다.");
		return FALSE;
	}
	try {
		if (!m_db.CommitTrans()) {
			SetLastError(L"CommitTransaction failed");
			return FALSE;
		}
	}
	catch (CDBException* e) {
		SetLastError(e->m_strError);
		e->Delete();
		return FALSE;
	}
	return TRUE;
}

void CUcRecset::RollbackTransaction()
{
	if (!m_db.IsOpen())
		return;
	try {
		m_db.Rollback();
	}
	catch (CDBException* e) {
		SetLastError(e->m_strError);
		e->Delete();
	}
}

BOOL CUcRecset::ExecuteCommandWithParams(LPCWSTR sql, std::initializer_list<UcSqlParam> params)
{
	return ExecuteCommandWithParamsImpl(sql, params.begin(), params.size());
}

BOOL CUcRecset::ExecuteCommandWithParams(LPCWSTR sql, const std::vector<UcSqlParam>& params)
{
	return ExecuteCommandWithParamsImpl(sql, params.data(), params.size());
}

BOOL CUcRecset::ExecuteCommandWithParamsImpl(LPCWSTR sql, const UcSqlParam* params, size_t paramCount)
{
	CStringW sqlW(sql ? sql : L"");
	sqlW.Trim();
	m_lastError.Empty();
	if (sqlW.IsEmpty()) {
		SetLastError(L"ExecuteCommandWithParams: SQL 문이 비었습니다.");
		return FALSE;
	}
	if (!EnsureOpen()) {
		if (m_lastError.IsEmpty())
			SetLastError(L"ExecuteCommandWithParams: DB 가 열려 있지 않습니다.");
		return FALSE;
	}

	HDBC hdbc = m_db.GetHdbc();
	if (!hdbc) {
		if (TryReopen())
			hdbc = m_db.GetHdbc();
		if (!hdbc) {
			SetLastError(L"ExecuteCommandWithParams: ODBC 연결 핸들이 없습니다.");
			return FALSE;
		}
	}

	NoteSql(sqlW);
	KDefer deferSqlLog([&]() { RecordSqlLog(sqlW); });

	HSTMT hstmt = SQL_NULL_HSTMT;
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (!SQL_SUCCEEDED(ret) || !hstmt) {
		SetLastError(OdbcDiagMessage(SQL_HANDLE_DBC, hdbc, ret));
		return FALSE;
	}

	struct StmtGuard {
		HSTMT h{ SQL_NULL_HSTMT };
		~StmtGuard() {
			if (h)
				SQLFreeHandle(SQL_HANDLE_STMT, h);
		}
	} guard;
	guard.h = hstmt;

	ret = SQLPrepareW(hstmt, (SQLWCHAR*)sqlW.GetString(), SQL_NTS);
	if (!SQL_SUCCEEDED(ret)) {
		SetLastError(OdbcDiagMessage(SQL_HANDLE_STMT, hstmt, ret));
		return FALSE;
	}

	std::vector<SQLLEN> ind(paramCount, 0);
	for (size_t i = 0; i < paramCount; ++i) {
		ret = BindSqlParam(hstmt, (SQLUSMALLINT)(i + 1), params[i], ind[i]);
		if (!SQL_SUCCEEDED(ret)) {
			SetLastError(OdbcDiagMessage(SQL_HANDLE_STMT, hstmt, ret));
			return FALSE;
		}
	}

	ret = SQLExecute(hstmt);
	if (!SQL_SUCCEEDED(ret)) {
		SetLastError(OdbcDiagMessage(SQL_HANDLE_STMT, hstmt, ret));
		return FALSE;
	}

	return TRUE;
}


void CUcRecset::SetLastError(LPCWSTR msg)
{
	m_lastError = msg ? msg : L"";
	DWKFUNCV(L"Error: %v", m_lastError);
}

#ifdef _DEBUGxx
UcJObj out;
rs.ExecuteCommand(
    L"UPDATE tsessions SET fState='offline' WHERE fEidSessionID='abc'",
    L"cmd_close_session",   // logicalKey
    L"update",              // cmdType
    out);
//성공 후 out은 대략 이렇게 됩니다 (실제로는 type만 있음):


{
  "cmd_close_session": {
    "type": "update"
  }
}
#endif // _DEBUGxx


CStringW CUcRecset::EscSQL(const CStringW& in)
{
	CStringW out(in);
	out.Replace(L"'", L"''");
	return out;
}
CStringW CUcRecset::SqlQuoted(const CStringW& v)
{
	return L"'" + EscSQL(v) + L"'";
}

BOOL CUcRecset::OpenConnect(LPCTSTR connectString)
{
	if (!connectString || !*connectString)
		return FALSE;

	CString conn(connectString);
	std::vector<CString> ar;
	UcCutByToken(conn.GetString(), _T(";"), ar);
	for (size_t i = 0; i < ar.size(); ++i)
	{
		std::vector<CString> ar1;
		UcCutByToken(ar[i].GetString(), _T("="), ar1);
		if (ar1.size() < 2)
			continue;
		CString key = ar1[0];
		key.Trim();
		CString val = ar1[1];
		val.Trim();
		if (key.CompareNoCase(_T("DSN")) == 0)
			m_dsn = val;
		else if (key.CompareNoCase(_T("UID")) == 0)
			m_uid = val;
		else if (key.CompareNoCase(_T("PWD")) == 0)
			m_pwd = val;
		else if (key.CompareNoCase(_T("SERVER")) == 0)
			m_server = val;
		else if (key.CompareNoCase(_T("DATABASE")) == 0 || key.CompareNoCase(_T("DBQ")) == 0)
			m_database = val;
	}

	if (conn.Left(5).CompareNoCase(_T("ODBC;")) == 0 || conn.Find(_T("DRIVER=")) >= 0)
		SetConnectionString(conn);
	else
	{
		CStringW odbc;
		odbc.Format(L"ODBC;DSN=%s;UID=%s;PWD=%s", (LPCWSTR)m_dsn, (LPCWSTR)m_uid, (LPCWSTR)m_pwd);
		SetConnectionString(odbc);
	}
	return Open();
}

void CUcRecset::ClearResult()
{
	m_grid.reset();
	m_colNames.clear();
	m_rows.clear();
	m_binCells.clear();
}

BOOL CUcRecset::LoadGridFromTable(std::shared_ptr<UcJTable> tb)
{
	ClearResult();
	if (!tb)
		return FALSE;
	m_grid = tb;
	const size_t nCols = tb->ColSize();
	const size_t nRows = tb->RowSize();
	if (m_maxRow > 0 && (int)nRows > m_maxRow)
	{
		// truncate — not implemented for UcJTable slice; use as-is
	}
	m_colNames.resize(nCols);
	m_rows.resize(nRows);
	m_binCells.resize(nRows);
	for (size_t c = 0; c < nCols; ++c)
	{
		try {
			auto shFo = tb->_fields->Arr()->GetAt((int)c);
			if (shFo && shFo->IsDic())
				m_colNames[c] = shFo->Dic()->S(L"name");
			else
				m_colNames[c].Format(L"col%d", (int)c + 1);
		}
		catch (...) {
			m_colNames[c].Format(L"col%d", (int)c + 1);
		}
	}
	for (size_t r = 0; r < nRows; ++r)
	{
		m_rows[r].resize(nCols);
		m_binCells[r].resize(nCols);
		for (size_t c = 0; c < nCols; ++c)
		{
			try {
				m_rows[r][c] = tb->CellS(c, r);
			}
			catch (...) {
				m_rows[r][c].Empty();
			}
		}
	}
	return TRUE;
}

BOOL CUcRecset::DoQuery(LPCWSTR sql)
{
	auto tb = QueryToTableJson(sql);
	if (!tb)
		return FALSE;
	return LoadGridFromTable(tb);
}

BOOL CUcRecset::DoSql(LPCWSTR sql)
{
	if (!sql || !*sql)
		return FALSE;
	CStringW s(sql);
	s.TrimLeft();
	if (s.Left(6).CompareNoCase(L"select") == 0 || s.Left(4).CompareNoCase(L"with") == 0)
		return DoQuery(sql);
	ClearResult();
	return ExecuteCommand(sql);
}

BOOL CUcRecset::DoSqlFmt(LPCTSTR fmt, ...)
{
	CString buf;
	va_list args;
	va_start(args, fmt);
	buf.FormatV(fmt, args);
	va_end(args);
	return DoSql(buf);
}

BOOL CUcRecset::DoSqlVarInsert(LPCWSTR sql, LPCVOID data, SQLLEN cbLen,
	SQLSMALLINT cType, SQLSMALLINT sqlType)
{
	UcSqlParamBinary bin;
	bin.data = data;
	bin.cbLen = cbLen;
	bin.cType = cType;
	bin.sqlType = sqlType;
	return ExecuteCommandWithParams(sql, { { UcSQL_BINARY, &bin } });
}

int CUcRecset::GetColCount() const
{
	return (int)m_colNames.size();
}

SDWORD CUcRecset::GetRowCount() const
{
	return (SDWORD)m_rows.size();
}

LPCTSTR CUcRecset::GetCellStr(int col, int row) const
{
	if (col < 0 || row < 0 || row >= (int)m_rows.size())
		return _T("");
	if (col >= (int)m_rows[row].size())
		return _T("");
	return m_rows[row][col];
}

LPCSTR CUcRecset::GetCellPtr(int col, int row) const
{
	m_cellPtrScratch = CW2A(GetCellStr(col, row), CP_UTF8);
	return m_cellPtrScratch;
}

int CUcRecset::GetCellInt(int col, int row) const
{
	return _wtoi(GetCellStr(col, row));
}

LONG CUcRecset::GetCellLong(int col, int row) const
{
	return (LONG)_wtol(GetCellStr(col, row));
}

double CUcRecset::GetCellDouble(int col, int row) const
{
	return _wtof(GetCellStr(col, row));
}

CUcDbCol CUcRecset::GetCol(int col) const
{
	CUcDbCol c;
	const int nRows = (int)m_rows.size();
	for (int r = 0; r < nRows; ++r)
	{
		if (col >= 0 && col < (int)m_rows[r].size())
		{
			if (r < (int)m_binCells.size() && col < (int)m_binCells[r].size()
				&& !m_binCells[r][col].empty())
				c.SetRowBin(r, m_binCells[r][col].data(), (int)m_binCells[r][col].size());
			else
				c.SetRow(r, m_rows[r][col]);
		}
	}
	return c;
}

void CUcRecset::TableDump() const
{
	TRACE(_T("--- CUcRecset TableDump %d cols x %d rows ---\n"), GetColCount(), GetRowCount());
	for (int c = 0; c < GetColCount(); ++c)
		TRACE(_T("[%d] %s\t"), c, (LPCTSTR)m_colNames[c]);
	TRACE(_T("\n"));
	for (int r = 0; r < (int)GetRowCount(); ++r)
	{
		for (int c = 0; c < GetColCount(); ++c)
			TRACE(_T("%s\t"), GetCellStr(c, r));
		TRACE(_T("\n"));
	}
}

BOOL CUcRecset::vDoSQLwithBinary(LPCTSTR sql)
{
	return DoSql(CStringW(sql));
}

BOOL CUcRecset::vDoSQL(LPCTSTR sql)
{
	return DoSql(CStringW(sql));
}

BOOL CUcRecset::vDoSQLVar(LPCTSTR fmt, ...)
{
	CString buf;
	va_list args;
	va_start(args, fmt);
	buf.FormatV(fmt, args);
	va_end(args);
	return DoSql(buf);
}

BOOL CUcRecset::vDoSQLVarInsert(LPCTSTR sql, LPCVOID data, SQLLEN cbLen,
	SQLSMALLINT cType, SQLSMALLINT sqlType)
{
	return DoSqlVarInsert(CStringW(sql), data, cbLen, cType, sqlType);
}

BOOL CUcRecsetCmd::vDoSQL(LPCTSTR sql)
{
	if (!m_pDb)
		return FALSE;
	CStringW w(sql);
	w.TrimLeft();
	if (w.Left(6).CompareNoCase(L"select") == 0 || w.Left(4).CompareNoCase(L"with") == 0)
		return m_pDb->DoQuery(w);
	return m_pDb->ExecuteCommand(w);
}

BOOL CUcRecsetCmd::vDoSQLwithBinary(LPCTSTR sql)
{
	return vDoSQL(sql);
}

BOOL CUcRecsetCmd::vDoSQLVarInsert(LPCTSTR sql, LPCVOID data, SQLLEN cbLen,
	SQLSMALLINT cType, SQLSMALLINT sqlType)
{
	if (!m_pDb)
		return FALSE;
	return m_pDb->DoSqlVarInsert(CStringW(sql), data, cbLen, cType, sqlType);
}
