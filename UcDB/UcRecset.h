#pragma once

/// UcRecset.h — UcDB: ODBC(MFC CRecordset) ↔ UcJObj(JSON 응답용)
/// 링크: MFC 공유 DLL, ODBC, UcTool(UcJson)

#include <initializer_list>
#include <vector>
#include <string>

#include <afxdb.h>

#include "UcDBExport.inl"

class UcJObj;
class UcJTable;
class CUcDbCol;

/// `ExecuteCommandWithParams` 바인딩 종류 (`{ SQL_STRING, ptr }` 형태).
enum UcSqlParamKind : int
{
	UcSQL_NULL = 0,
	UcSQL_STRING,   ///< `LPCWSTR` (UTF-16) → `SQL_WLONGVARCHAR`
	UcSQL_UTF8,     ///< `LPCSTR` (UTF-8 바이트) → `SQL_LONGVARCHAR`
	UcSQL_BINARY,   ///< `UcSqlParamBinary*` — 임의 바이트
	UcSQL_INT32,    ///< `const int*`
	UcSQL_INT64,    ///< `const __int64*` / `const long long*`
};

struct UcSqlParamBinary
{
	const void* data{ nullptr };
	SQLLEN cbLen{ 0 };
	SQLSMALLINT cType{ SQL_C_BINARY };
	SQLSMALLINT sqlType{ SQL_LONGVARBINARY };
};

inline constexpr UcSqlParamKind SQL_STRING = UcSQL_STRING;
inline constexpr UcSqlParamKind SQL_UTF8 = UcSQL_UTF8;
inline constexpr UcSqlParamKind SQL_INT32 = UcSQL_INT32;
inline constexpr UcSqlParamKind SQL_INT64 = UcSQL_INT64;

struct UcSqlParam
{
	UcSqlParamKind kind{ UcSQL_NULL };
	const void* data{ nullptr };

	constexpr UcSqlParam() = default;
	constexpr UcSqlParam(UcSqlParamKind k, const void* p) : kind(k), data(p) {}
};

/// DSN/연결 문자열 구성, 연결·해제, SELECT 결과를 `UcJObj` 테이블 규약(`13.2`)으로 채우고,
/// INSERT/UPDATE/DELETE/프로시저 등은 `type` 만 있는 블록으로 채운다.
/// 사용법
// class SshRecset : public CUcRecset
//{
//public:
//	SshRecset();
//};
// 
//SshRecset::SshRecset()
//{
//	SetDsn(L"NewNGS");
//	SetUid(L"ngsadmin");//root");// 
//	SetPwd(LR"(hkjl;')");
//	SetServer(L"localhost");
//	SetDatabase(L"ngsx");
//	try {
// 	BOOL	rb = Open();
//	}
//	catch (CDBException* e) {	}
//	catch (...) {	}
//}
// 
//bool CSshTool::GetSomeData() {
//	SshRecset rs;
//	if (SHP<UcJTable> tb = rs.QueryToTableJson(LR"(
//		SELECT * FROM tSmtSomeTable")) {
//		for (int r = 0; r < tb->RowSize(); ++r) {
//			ShJBase robj = tb->RowObj(r);
//			if (robj->IsDic()) {
//				auto row = robj->Dic();
//				auto fKey = row->I("fKey");
//				_mapSome[fKey] = robj;
//			}
//		}
//		return true;
//	}
//	return false;
//}
class UCDBDYNAMIC CUcRecsetDatabase : public CDatabase
{
public:
	HDBC GetHdbc() const { return m_hdbc; }
};

class UCDBDYNAMIC CUcRecset
{
public:
	CUcRecset();
	virtual ~CUcRecset();


	CDatabase& Database() { return m_db; }
	const CDatabase& Database() const { return m_db; }

	BOOL IsOpen() const;

	/// ODBC 연결. 실패 시 FALSE, `m_lastError` 참고.
	BOOL Open();
	void Close();

	/// 닫혀 있으면 Open 재시도. 이미 열려 있으면 TRUE.
	BOOL EnsureOpen();
	/// Close 후 연결 문자열로 다시 Open (끊김 복구용).
	BOOL TryReopen();
	/// `m_lastError` 가 연결 끊김/핸들 무효 계열인지.
	bool IsConnectionError() const;

	/// SELECT: `outRoot[tableKey]` 에 `type: table`, `fields`, `rows` 채움 (`13.2`).
	std::shared_ptr<UcJTable> QueryToTableJson(LPCWSTR sql);// , LPCWSTR tableKey, UcJObj& outRoot);
	std::list<std::wstring> _lstSql;
	std::wstring LastSql(){
		if(_lstSql.size() > 0)
			return _lstSql.back();
		return {};
	}
	/// INSERT/UPDATE/DELETE/프로시저 등 — `outRoot[logicalKey]` 에 `type` 만 설정 (`fields`/`rows` 없음).
	/// `cmdType`: L"insert" | L"update" | L"delete" | L"procedure" 등.
	BOOL ExecuteCommand(LPCWSTR sql);// , LPCWSTR logicalKey, LPCWSTR cmdType, UcJObj& outRoot);

	BOOL BeginTransaction();
	BOOL CommitTransaction();
	void RollbackTransaction();

	/// `?` 플레이스홀더 + ODBC 파라미터 바인딩. 예:
	/// `ExecuteCommandWithParams(L"INSERT INTO t (a,b) VALUES (?,?)", { { SQL_STRING, sA.GetString() }, { SQL_UTF8, sB.GetString() } });`
	BOOL ExecuteCommandWithParams(LPCWSTR sql, std::initializer_list<UcSqlParam> params);
	BOOL ExecuteCommandWithParams(LPCWSTR sql, const std::vector<UcSqlParam>& params);

	const CStringW& LastError() const { return m_lastError; }

protected:
	/// `_lstSql` 최근 SQL 목록 (실행 직전).
	void NoteSql(LPCWSTR sql);
	/// 실행 후 `tsqllog` 기록 (`m_lastError` 있으면 `fError`).
	void RecordSqlLog(LPCWSTR sql);
	/// NgsServer `SshRecset`: `tsqllog` INSERT 등. 기본 구현은 no-op.
	virtual void InsertSqlLog(LPCWSTR sql, LPCWSTR err = nullptr);
	/// `InsertSqlLog` 내부 `ExecuteCommand` 재진입 시 `InsertSqlLog` 생략.
	int _sqlLogSuppress{ 0 };

private:
	BOOL ExecuteCommandWithParamsImpl(LPCWSTR sql, const UcSqlParam* params, size_t paramCount);

	CUcRecsetDatabase m_db;
	CStringW m_dsn     ;//{ L"NewNGS" };
	CStringW m_server  ;//{ L"localhost" };
	CStringW m_database;//{ L"ngsx" };
	CStringW m_uid;//{ L"ngsadmin" };
	CStringW m_pwd     ;//{ L"*******" };
	CStringW m_connOverride;// `SetConnectionString` 으로 설정된 연결 문자열이 있으면, `Open` 시 이 문자열을 사용한다.

public:
	CStringW m_lastError;
public:
	void SetLastError(LPCWSTR msg);
	/// DSN 이름( ODBC 에 등록된 이름 ). `Open` 시 `DSN=...;UID=...;PWD=...` 로 연결한다.
	void SetDsn(const CStringW& dsn) { m_dsn = dsn; }
	void SetUid(const CStringW& uid) { m_uid = uid; }
	void SetPwd(const CStringW& pwd) { m_pwd = pwd; }
	void SetServer(const CStringW& svr) { m_server = svr; }
	void SetDatabase(const CStringW& dbs) { m_database = dbs; }

	/// 한 번에 연결 문자열을 쓰고 싶을 때(위 Set* 보다 우선).
	void SetConnectionString(const CStringW& conn) { m_connOverride = conn; }
	CStringW GetConnectionString() { return m_connOverride; }


	static CStringW EscSQL(const CStringW& in);

	static CStringW SqlQuoted(const CStringW& v);

	// --- Memo / 그리드 스타일 API (Kdb 대체) ---

	/// `DSN=...;UID=...;PWD=...` 또는 전체 ODBC 연결 문자열.
	BOOL OpenConnect(LPCTSTR connectString);

	BOOL DoQuery(LPCWSTR sql);
	BOOL DoQueryBinary(LPCWSTR sql) { return DoQuery(sql); }

	/// SELECT 이면 DoQuery, 그 외 ExecuteCommand.
	BOOL DoSql(LPCWSTR sql);
	BOOL DoSqlFmt(LPCTSTR fmt, ...);

	BOOL DoSqlVarInsert(LPCWSTR sql, LPCVOID data, SQLLEN cbLen,
		SQLSMALLINT cType = SQL_C_BINARY, SQLSMALLINT sqlType = SQL_LONGVARBINARY);

	void ClearResult();
	void SetMaxRow(int maxRow) { m_maxRow = maxRow; }
	/// QueryToTableJson 시 셀 텍스트 합산 바이트 상한 (0=무제한). 초과 시 조회 중단.
	void SetMaxResultBytes(size_t maxBytes) { m_maxResultBytes = maxBytes; }
	bool WasResultTruncated() const { return m_resultTruncated; }
	int GetFetchedRowCount() const { return m_fetchedRows; }

	int GetColCount() const;
	SDWORD GetRowCount() const;
	int GetCount() const { return (int)GetRowCount(); }

	LPCTSTR GetCellStr(int col, int row) const;
	LPCSTR GetCellPtr(int col, int row) const;
	int GetCellInt(int col, int row) const;
	LONG GetCellLong(int col, int row) const;
	double GetCellDouble(int col, int row) const;

	CUcDbCol GetCol(int col) const;
	void TableDump() const;

	void TimeoutOn() {}
	void TimeoutOff() {}

	// Memo KDbSrc 호환 (이름만 유지, 구현은 CUcRecset)
	BOOL odbcConnect(LPCTSTR connectString) { return OpenConnect(connectString); }
	void odbcDisconnect() { Close(); }
	int InitDbSrc0(CWnd*, LPCTSTR connectString) { return OpenConnect(connectString) ? 0 : -1; }

	BOOL vDoSQLwithBinary(LPCTSTR sql);
	BOOL vDoSQL(LPCTSTR sql);
	BOOL vDoSQLVar(LPCTSTR fmt, ...);
	BOOL vDoSQLVarInsert(LPCTSTR sql, LPCVOID data, SQLLEN cbLen,
		SQLSMALLINT cType = SQL_C_BINARY, SQLSMALLINT sqlType = SQL_LONGVARBINARY);
	BOOL vDoBinaryInsert(LPCTSTR sql, LPCVOID data, SQLLEN cbLen,
		SQLSMALLINT cType = SQL_C_BINARY, SQLSMALLINT sqlType = SQL_LONGVARBINARY)
	{
		return vDoSQLVarInsert(sql, data, cbLen, cType, sqlType);
	}

private:
	BOOL LoadGridFromTable(std::shared_ptr<UcJTable> tb);

	std::shared_ptr<UcJTable> m_grid;
	std::vector<CStringW> m_colNames;
	std::vector<std::vector<CStringW>> m_rows;
	std::vector<std::vector<std::vector<BYTE>>> m_binCells;
	int m_maxRow{ 0 };
	size_t m_maxResultBytes{ 0 };
	bool m_resultTruncated{ false };
	int m_fetchedRows{ 0 };
	mutable CStringA m_cellPtrScratch;
};

/// 부모 `CUcRecset` ODBC 연결을 공유해, 메인 결과 그리드를 건드리지 않고 SQL 실행.
class UCDBDYNAMIC CUcRecsetCmd
{
public:
	explicit CUcRecsetCmd(CUcRecset* pDb) : m_pDb(pDb) {}

	BOOL vDoSQL(LPCTSTR sql);
	BOOL vDoSQLwithBinary(LPCTSTR sql);
	BOOL vDoSQLVarInsert(LPCTSTR sql, LPCVOID data, SQLLEN cbLen,
		SQLSMALLINT cType = SQL_C_BINARY, SQLSMALLINT sqlType = SQL_LONGVARBINARY);

private:
	CUcRecset* m_pDb{ nullptr };
};
