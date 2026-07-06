#pragma once

#include "UcDBExport.inl"
#include <vector>

/// SELECT 결과의 한 컬럼 캐시 (Memo ListView 등).
class UCDBDYNAMIC CUcDbCol
{
public:
	CUcDbCol() = default;
	CUcDbCol(const CUcDbCol&) = default;
	CUcDbCol(CUcDbCol&&) = default;
	CUcDbCol& operator=(const CUcDbCol&) = default;
	CUcDbCol& operator=(CUcDbCol&&) = default;

	void Clear();
	void RemoveData() { Clear(); }

	int GetCount() const { return (int)m_text.size(); }
	//SDWORD GetRowCount() const { return (SDWORD)m_text.size(); }

	LPCTSTR GetRowStr(int row) const;
	LONG GetRowLong(int row) const;
	int GetRowInt(int row) const;
	LPCTSTR GetRowPtr(int row) const { return GetRowStr(row); }

	/// 바이너리 셀 — 없으면 텍스트를 ANSI 바이트로 반환.
	CHAR* GetRowBin(int row, long* pcb);
	LPCTSTR GetBinaryRow(int row, long* pcb);

	void SetRow(int row, LPCWSTR text);
	void SetRowBin(int row, const void* data, int cb);

private:
	std::vector<CStringW> m_text;
	std::vector<std::vector<BYTE>> m_bin;
	mutable CStringA m_binAnsiScratch;
};
