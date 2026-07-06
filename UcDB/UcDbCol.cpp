#include "pch.h"
#include "UcDbCol.h"

void CUcDbCol::Clear()
{
	m_text.clear();
	m_bin.clear();
	m_binAnsiScratch.Empty();
}

LPCTSTR CUcDbCol::GetRowStr(int row) const
{
	if (row < 0 || row >= (int)m_text.size())
		return _T("");
	return m_text[row];
}

LONG CUcDbCol::GetRowLong(int row) const
{
	return (LONG)_wtol(GetRowStr(row));
}

int CUcDbCol::GetRowInt(int row) const
{
	return _wtoi(GetRowStr(row));
}

void CUcDbCol::SetRow(int row, LPCWSTR text)
{
	if (row >= (int)m_text.size())
		m_text.resize(row + 1);
	m_text[row] = text ? text : L"";
	if (row >= (int)m_bin.size())
		m_bin.resize(row + 1);
	m_bin[row].clear();
}

void CUcDbCol::SetRowBin(int row, const void* data, int cb)
{
	if (row >= (int)m_text.size())
		m_text.resize(row + 1);
	if (row >= (int)m_bin.size())
		m_bin.resize(row + 1);
	m_text[row].Empty();
	m_bin[row].assign((const BYTE*)data, (const BYTE*)data + cb);
}

CHAR* CUcDbCol::GetRowBin(int row, long* pcb)
{
	if (!pcb)
		return nullptr;
	if (row < 0 || row >= (int)m_text.size())
	{
		*pcb = 0;
		return nullptr;
	}
	if (row < (int)m_bin.size() && !m_bin[row].empty())
	{
		*pcb = (long)m_bin[row].size();
		return (CHAR*)m_bin[row].data();
	}
	m_binAnsiScratch = CW2A(m_text[row], CP_UTF8);
	*pcb = m_binAnsiScratch.GetLength();
	return m_binAnsiScratch.GetBuffer();
}

LPCTSTR CUcDbCol::GetBinaryRow(int row, long* pcb)
{
	if (row < 0 || row >= (int)m_text.size())
	{
		if (pcb) *pcb = 0;
		return _T("");
	}
	if (pcb)
	{
		if (row < (int)m_bin.size() && !m_bin[row].empty())
			*pcb = (long)m_bin[row].size();
		else
			*pcb = m_text[row].GetLength() * sizeof(WCHAR);
	}
	return m_text[row];
}
