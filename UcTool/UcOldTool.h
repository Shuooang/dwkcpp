#pragma once

/// Memo 레거시 Kw19 컨트롤/다이얼로그 + KwLib32 호환 (구 KwLib32/Kw19/Kdb 의존 제거용).
#include "UcExport.inl"
#include "UcWndInvokable.h"
#include "OldTool/Ctrlext.h"
#include "OldTool/DrListView.h"
#include "OldTool/DrTreeView.h"
#include "OldTool/KDialog32.h"
#include "OldTool/ATMSGBOX.H"

// --- KwLib32 호환 (Memo 전용, 점진 제거) ---

class KFirst
{
public:
	KFirst() { m_b = TRUE; }
	BOOL IsFirst() { BOOL b = m_b; m_b = FALSE; return b; }
	BOOL m_b;
};

inline BOOL KwIsHangul(WCHAR ch) { return (0xAC00 <= ch && ch <= 0xD7A3); }
#define ISWORD(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z') || ('0' <= (c) && (c) <= '9') || (c) == '_' || KwIsHangul(c))

template<typename TStr, typename TCH>
inline int tstristrIndex(TStr& str, int start, const TCH* pKey, BOOL bWord)
{
	if (!*pKey)
		return -1;

	TStr key = pKey;
	int len = key.GetLength();
	int lenMatch = 0;
	const int gab = 'a' - 'A';
	int iFind = -1;

	for (int i = start; i < str.GetLength(); i++)
	{
		TCH c = str.GetAt(i);
		TCH k = key[lenMatch];
		if (c == k)
			lenMatch++;
		else if (ISWORD(c))
		{
			if (c >= 'a' && c <= 'z')
				c = (TCH)(c + gab);
			if (c == k)
				lenMatch++;
			else
				lenMatch = 0;
		}
		else
			lenMatch = 0;

		if (lenMatch == 1)
		{
			if (bWord && !(i == start || !ISWORD(str.GetAt(i - 1))))
				lenMatch = 0;
			else
				iFind = i;
		}
		if (lenMatch == len)
		{
			if (bWord && !(i == len - 1 || !ISWORD(str.GetAt(i + 1))))
				lenMatch = 0;
			else
				return iFind;
		}
	}
	return -1;
}

template<typename T>
inline int PtrCompareValT(T ps1, T ps2, T)
{
	return ((T)ps1 == (T)ps2) ? 0 : ((T)ps1 < (T)ps2) ? -1 : 1;
}
#define PtrCompareVAL(p1, p2, TYPE) PtrCompareValT((*(TYPE*)p1), (*(TYPE*)p2), (TYPE)0)

inline int PtrCompare_int(const void* p1, const void* p2) { return PtrCompareVAL(p1, p2, int); }
inline int PtrCompare_char(const void* p1, const void* p2) { return PtrCompareVAL(p1, p2, char); }
inline int PtrCompare_WCHAR(const void* p1, const void* p2) { return PtrCompareVAL(p1, p2, WCHAR); }

#define QuickSORT(arr, len, TYPE) qsort((void*)(arr), (len), sizeof(TYPE), PtrCompare_##TYPE)
#define BinSearch(key, arr, len, TYPE) (TYPE*)bsearch((const void*)(&(key)), (const void*)(arr), (len), sizeof(TYPE), PtrCompare_##TYPE)

UCTOOLDYNAMIC void DeleteMe(void* p);

UCTOOLDYNAMIC int KwMessageBox(LPCTSTR lpFormat, ...);
UCTOOLDYNAMIC LPCSTR KwReadSmallTextFileA(LPCTSTR fileName, CStringA& str);

UCTOOLDYNAMIC HWND KwFindWindow(HWND hParent, LPCTSTR lpClassName, LPCTSTR lpWinText);
UCTOOLDYNAMIC HWND KwFindWindow(HWND hParent, CString className, LPCTSTR lpWinText);
UCTOOLDYNAMIC HWND KwFindWindow(HWND hParent, CString className, int count);

inline void UcMyTrace(LPCTSTR fmt, ...)
{
	CString buf;
	va_list args;
	va_start(args, fmt);
	buf.FormatV(fmt, args);
	va_end(args);
	buf += _T("\n");
	::OutputDebugString(buf);
}
#ifndef MYTRACE
#define MYTRACE UcMyTrace
#endif

/// Memo 종료 시 정리용 스텁 (ThreadPool 미사용)
class UCTOOLDYNAMIC CPushWorker
{
public:
	struct ThrdLockMap { void DeleteAll() {} };
	static ThrdLockMap* s_thrdLock;
};
