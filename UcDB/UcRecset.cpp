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

	DWKTRACE(L"conn: %s", conn.GetString());
	BOOL rb{ FALSE };
	try
	{
		rb = m_db.OpenEx(conn, CDatabase::noOdbcDialog);
		m_lastError.Empty();
		return TRUE;
	}
	catch (CDBException* e)
	{
		m_lastError = e->m_strError;
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


SHP<UcJTable> CUcRecset::QueryToTableJson(LPCWSTR psql)// , LPCWSTR tableKey, UcJObj& outRoot1)
{
	DWKFUNC;
	CStringW sql(psql);
	sql.Trim();

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
			DWKTRACE(L"col:%v,row:%v %v", c, row, textValue);
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
