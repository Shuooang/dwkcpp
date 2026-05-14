#include "pch.h"

#include <vector>

#include <sql.h>
#include <sqlext.h>
#include <atldbcli.h>

#include "UcRecset.h" // UcDb/UcRecset.h
#include "UcTool/UcJson.h"

namespace {

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

} // namespace

CUcRecset::CUcRecset() = default;

CUcRecset::~CUcRecset()
{
	Close();
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

	DWKTRACE(L"sql:%s", conn.GetString());
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
	if (!m_db.IsOpen()) {
		SetLastError(L"QueryToTableJson: DB 가 열려 있지 않습니다.");
		return {};
	}

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
		//e->Delete();
		DWKTRACE(L"CDBException");
		return {};
	}
	catch (CException* e)
	{
		DWKTRACE(L"CException");
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
				DWKTRACE(L"col %v. %v, %v:%v", tryCol, colName, fi.m_nSQLType, jsType);
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
				_break;
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
	int row = 0;
	while (!rs.IsEOF())
	{
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
			shRow5->Add(CStringW(textValue));
			DWKTRACE(L"[%v,%v] %v", row, c, textValue);
		}
		rows4.Add(NEWSHP(JVal, shRow5, false), false);
		rs.MoveNext();
	}

	rs.Close();
	if (shTbl2 && shTbl2->IsDic())
		return shTbl2->Dic()->Table();
	return {};
}

BOOL CUcRecset::ExecuteCommand(LPCWSTR sql, LPCWSTR logicalKey, LPCWSTR cmdType, UcJObj& outRoot)
{
	m_lastError.Empty();
	if (!sql || !*sql || !logicalKey || !*logicalKey || !cmdType || !*cmdType)
	{
		SetLastError(L"ExecuteCommand: 인자가 비었습니다.");
		return FALSE;
	}
	if (!m_db.IsOpen())
	{
		SetLastError(L"ExecuteCommand: DB 가 열려 있지 않습니다.");
		return FALSE;
	}

	try
	{
		m_db.ExecuteSQL(sql);
	}
	catch (CDBException* e)
	{
		m_lastError = e->m_strError;
		e->Delete();
		return FALSE;
	}

	ShJVal sh = outRoot.O(logicalKey, true);
	if (!sh || !sh->IsDic())
	{
		SetLastError(L"ExecuteCommand: UcJObj 블록을 만들 수 없습니다.");
		return FALSE;
	}
	UcJObj* p = sh->Dic();
	p->Format(L"type", L"%s", cmdType);
	return TRUE;
}
