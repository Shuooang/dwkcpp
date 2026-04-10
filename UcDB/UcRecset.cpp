#include "pch.h"
#include "UcRecset.h"

#include <vector>

#include <sql.h>
#include <sqlext.h>

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

void CUcRecset::SetDsn(const CStringW& dsn)
{
	m_dsn = dsn;
}

void CUcRecset::SetUid(const CStringW& uid)
{
	m_uid = uid;
}

void CUcRecset::SetPwd(const CStringW& pwd)
{
	m_pwd = pwd;
}

void CUcRecset::SetConnectionString(const CStringW& conn)
{
	m_connOverride = conn;
}

BOOL CUcRecset::IsOpen() const
{
	return m_db.IsOpen();
}

void CUcRecset::SetLastError(LPCWSTR msg)
{
	m_lastError = msg ? msg : L"";
}

BOOL CUcRecset::Open()
{
	if (m_db.IsOpen())
		return TRUE;

	CStringW conn;
	if (!m_connOverride.IsEmpty())
		conn = m_connOverride;
	else
		conn.Format(L"DSN=%s;UID=%s;PWD=%s", (LPCWSTR)m_dsn, (LPCWSTR)m_uid, (LPCWSTR)m_pwd);

	try
	{
		m_db.OpenEx(conn, CDatabase::noOdbcDialog);
		m_lastError.Empty();
		return TRUE;
	}
	catch (CDBException* e)
	{
		m_lastError = e->m_strError;
		e->Delete();
		return FALSE;
	}
}

void CUcRecset::Close()
{
	if (m_db.IsOpen())
		m_db.Close();
}

BOOL CUcRecset::QueryToTableJson(LPCWSTR sql, LPCWSTR tableKey, UcJObj& outRoot)
{
	m_lastError.Empty();
	if (!sql || !*sql || !tableKey || !*tableKey)
	{
		SetLastError(L"QueryToTableJson: sql 또는 tableKey 가 비었습니다.");
		return FALSE;
	}
	if (!m_db.IsOpen())
	{
		SetLastError(L"QueryToTableJson: DB 가 열려 있지 않습니다.");
		return FALSE;
	}

	CUcDynamicRecordset rs(&m_db);
	try
	{
		rs.Open(CRecordset::forwardOnly, sql, CRecordset::readOnly);
	}
	catch (CDBException* e)
	{
		m_lastError = e->m_strError;
		e->Delete();
		return FALSE;
	}

	ShJVal shTbl = outRoot.O(tableKey, true);
	if (!shTbl || !shTbl->IsDic())
	{
		rs.Close();
		SetLastError(L"QueryToTableJson: UcJObj 블록을 만들 수 없습니다.");
		return FALSE;
	}

	UcJObj* pTbl = shTbl->Dic();
	pTbl->Format(L"type", L"%s", L"table");
	UcJArr& fields = pTbl->SetArray(L"fields");
	UcJArr& rows = pTbl->SetArray(L"rows");

	std::vector<CStringW> colNames;
	const short nFields = rs.m_nFields;
	colNames.reserve((size_t)nFields);

	for (short i = 0; i < nFields; ++i)
	{
		CODBCFieldInfo fi = {};
		rs.GetODBCFieldInfo(i + 1, fi);
		CStringW name(fi.m_strName);
		colNames.push_back(name);

		UcJObj& fo = fields.AddObj();
		fo.Format(L"name", L"%s", name.GetString());
		fo.Format(L"type", L"%s", SqlTypeToJsonType(fi.m_nSQLType).GetString());
	}

	while (!rs.IsEOF())
	{
		auto shRow = std::make_shared<UcJArr>();
		for (short c = 0; c < nFields; ++c)
		{
			CString textValue;
			try
			{
				// CRecordset 오버로드 호환을 위해 컬럼 인덱스(short) + CString 버전을 사용한다.
				rs.GetFieldValue((short)(c + 1), textValue);
			}
			catch (CDBException* e)
			{
				m_lastError = e->m_strError;
				e->Delete();
				rs.Close();
				return FALSE;
			}
			shRow->Add(CStringW(textValue));
		}
		rows.Add(std::make_shared<JVal>(*shRow, true), false);
		rs.MoveNext();
	}

	rs.Close();
	return TRUE;
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
