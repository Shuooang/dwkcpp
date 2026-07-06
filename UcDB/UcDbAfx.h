#pragma once

/// Memo 앱용 UcDB 진입점 — Kdb 레거시 없음.
#include "UcRecset.h"
#include "UcDbCol.h"

/// printf 스타일 SQL 문자열 (VDOSQL/FMTS 용).
inline LPCTSTR UcFmtS(LPCTSTR fmt, ...)
{
	static thread_local CString s_buf;
	va_list args;
	va_start(args, fmt);
	s_buf.FormatV(fmt, args);
	va_end(args);
	return s_buf;
}
#define FMTS UcFmtS

inline LPCTSTR UcSqlTr(const CString& s)
{
	static thread_local CString s_buf;
	s_buf = CUcRecset::EscSQL(s);
	return s_buf;
}
#define SQLTR(s) UcSqlTr(CString(s))

/// Memo 소스 점진 이전용 별칭 (신규 코드는 CUcRecset / CUcDbCol / CUcRecsetCmd 사용).
using KDbSrc = CUcRecset;
using KCol = CUcDbCol;
using KRecset = CUcRecsetCmd;
