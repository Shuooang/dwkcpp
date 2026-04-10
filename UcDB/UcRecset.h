#pragma once

/// UcRecset.h — UcDB: ODBC(MFC CRecordset) ↔ UcJObj(JSON 응답용)
/// 링크: MFC 공유 DLL, ODBC, UcTool(UcJson)

#include <afxdb.h>

#include "UcDBExport.inl"

class UcJObj;

/// DSN/연결 문자열 구성, 연결·해제, SELECT 결과를 `UcJObj` 테이블 규약(`13.2`)으로 채우고,
/// INSERT/UPDATE/DELETE/프로시저 등은 `type` 만 있는 블록으로 채운다.
class UCDBDYNAMIC CUcRecset
{
public:
	CUcRecset();
	~CUcRecset();

	/// DSN 이름( ODBC 에 등록된 이름 ). `Open` 시 `DSN=...;UID=...;PWD=...` 로 연결한다.
	void SetDsn(const CStringW& dsn);
	void SetUid(const CStringW& uid);
	void SetPwd(const CStringW& pwd);

	/// 한 번에 연결 문자열을 쓰고 싶을 때(위 Set* 보다 우선).
	void SetConnectionString(const CStringW& conn);

	CDatabase& Database() { return m_db; }
	const CDatabase& Database() const { return m_db; }

	BOOL IsOpen() const;

	/// ODBC 연결. 실패 시 FALSE, `m_lastError` 참고.
	BOOL Open();
	void Close();

	/// SELECT: `outRoot[tableKey]` 에 `type: table`, `fields`, `rows` 채움 (`13.2`).
	BOOL QueryToTableJson(LPCWSTR sql, LPCWSTR tableKey, UcJObj& outRoot);

	/// INSERT/UPDATE/DELETE/프로시저 등 — `outRoot[logicalKey]` 에 `type` 만 설정 (`fields`/`rows` 없음).
	/// `cmdType`: L"insert" | L"update" | L"delete" | L"procedure" 등.
	BOOL ExecuteCommand(LPCWSTR sql, LPCWSTR logicalKey, LPCWSTR cmdType, UcJObj& outRoot);

	const CStringW& LastError() const { return m_lastError; }

private:
	CDatabase m_db;
	CStringW m_dsn;
	CStringW m_uid;
	CStringW m_pwd;
	CStringW m_connOverride;
	CStringW m_lastError;

	void SetLastError(LPCWSTR msg);
};
