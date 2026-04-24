#pragma once

/// UcRecset.h — UcDB: ODBC(MFC CRecordset) ↔ UcJObj(JSON 응답용)
/// 링크: MFC 공유 DLL, ODBC, UcTool(UcJson)

#include <afxdb.h>

#include "UcDBExport.inl"

class UcJObj;
class UcJTable;

/// DSN/연결 문자열 구성, 연결·해제, SELECT 결과를 `UcJObj` 테이블 규약(`13.2`)으로 채우고,
/// INSERT/UPDATE/DELETE/프로시저 등은 `type` 만 있는 블록으로 채운다.
class UCDBDYNAMIC CUcRecset
{
public:
	CUcRecset();
	~CUcRecset();


	CDatabase& Database() { return m_db; }
	const CDatabase& Database() const { return m_db; }

	BOOL IsOpen() const;

	/// ODBC 연결. 실패 시 FALSE, `m_lastError` 참고.
	BOOL Open();
	void Close();

	/// SELECT: `outRoot[tableKey]` 에 `type: table`, `fields`, `rows` 채움 (`13.2`).
	std::shared_ptr<UcJTable> QueryToTableJson(LPCWSTR sql);// , LPCWSTR tableKey, UcJObj& outRoot);

	/// INSERT/UPDATE/DELETE/프로시저 등 — `outRoot[logicalKey]` 에 `type` 만 설정 (`fields`/`rows` 없음).
	/// `cmdType`: L"insert" | L"update" | L"delete" | L"procedure" 등.
	BOOL ExecuteCommand(LPCWSTR sql, LPCWSTR logicalKey, LPCWSTR cmdType, UcJObj& outRoot);

	const CStringW& LastError() const { return m_lastError; }

private:
	CDatabase m_db;
	CStringW m_dsn     ;//{ L"NewNGS" };
	CStringW m_server  ;//{ L"localhost" };
	CStringW m_database;//{ L"ngsx" };
	CStringW m_uid;//{ L"ngsadmin" };
	CStringW m_pwd     ;//{ L"*******" };
	CStringW m_connOverride;// `SetConnectionString` 으로 설정된 연결 문자열이 있으면, `Open` 시 이 문자열을 사용한다.
	CStringW m_lastError;
public:
	void SetLastError(LPCWSTR msg) {
		m_lastError = msg ? msg : L"";
	}
	/// DSN 이름( ODBC 에 등록된 이름 ). `Open` 시 `DSN=...;UID=...;PWD=...` 로 연결한다.
	void SetDsn(const CStringW& dsn) { m_dsn = dsn; }
	void SetUid(const CStringW& uid) { m_uid = uid; }
	void SetPwd(const CStringW& pwd) { m_pwd = pwd; }
	void SetServer(const CStringW& svr) { m_server = svr; }
	void SetDatabase(const CStringW& dbs) { m_database = dbs; }

	/// 한 번에 연결 문자열을 쓰고 싶을 때(위 Set* 보다 우선).
	void SetConnectionString(const CStringW& conn) { m_connOverride = conn; }
};
