#pragma once

/// Memo 마이그레이션: KwLib32 / Kdb 매크로 → UcTool (점진 제거용).
#include "UcBaseTools.h"

#ifndef PS
typedef LPCTSTR PS;
#endif

#ifndef NTRACE
#define NTRACE(...) ((void)0)
#endif

#define KW_BLACK RGB(0,0,0)
#define KW_GRAY RGB(192,192,192)
#define KW_DARKGRAY RGB(128,128,128)
#define KW_WHITE RGB(255,255,255)
#define KW_RED RGB(255,0,0)
#define KW_GREEN RGB(0,255,0)
#define KW_BLUE RGB(0,0,255)

#define KwCheckMessage UcCheckMessage
#define KwReady UcReady
#define KwGetFileName UcGetFileName
#define KwCutByToken UcCutByToken
#define KwCutByTokenInt UcCutByTokenInt
#define KwPasteTextClipboard UcPasteTextClipboard
#define KwCopyTextClipboad UcCopyTextClipboad
#define KwSeparatePathFile UcSeparatePathFile
#define KwSetWindowStyle UcSetWindowStyle
#define KwSavePosition UcSavePosition
#define KwGetCurrentTimeFullString UcGetCurrentTimeString
#define KwCenterDialog UcCenterDialog
#define KwGetCenterRect UcGetCenterRect
#define KwRectangle UcRectangle

template<typename T0, typename TA0 = const T0&>
using CKArray = KArray<T0>;

template<typename T0, typename TA0 = const T0&>
using CKList = KList<T0>;

inline int UcUstrlen(CString& str) { return str.GetLength(); }
#define KwUstrlen UcUstrlen

inline LPCTSTR UcItoa(int iv, LPTSTR pbuf = nullptr)
{
	static thread_local CString s_buf;
	if (pbuf)
	{
		_itow_s(iv, pbuf, 32, 10);
		return pbuf;
	}
	s_buf.Format(_T("%d"), iv);
	return s_buf;
}
#define KwItoa UcItoa

inline void UcCStringSprintf(CString& str, LPCTSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	str.FormatV(fmt, args);
	va_end(args);
}
#define CSTRING_SPRINTF(lpFormat, str) \
	do { \
		va_list _uc_args; \
		va_start(_uc_args, lpFormat); \
		(str).FormatV(lpFormat, _uc_args); \
		va_end(_uc_args); \
	} while (0)

inline CRect UcGetCenterRect(CRect rcBig, CRect rc)
{
	CSize wh = rc.Size();
	return CRect(CPoint(rcBig.left + (rcBig.Width() / 2) - (wh.cx / 2),
		rcBig.top + (rcBig.Height() / 2) - (wh.cy / 2)), wh);
}

inline void UcCenterDialog(CWnd* pWnd, int cx = 0, int cy = 0)
{
	CRect rc, drc;
	CWnd* deskTop = pWnd->GetDesktopWindow();
	deskTop->GetWindowRect(drc);
	if (cx == 0 || cy == 0)
		pWnd->GetWindowRect(rc);
	else
		rc.SetRect(0, 0, cx, cy);
	pWnd->MoveWindow(UcGetCenterRect(drc, rc));
}

inline void UcRectangle(CDC& dc, LOGPEN& logPen, LOGBRUSH& logBrush, CRect rc)
{
	CBrush brush;
	CPen pen;
	if (brush.CreateBrushIndirect(&logBrush) && pen.CreatePenIndirect(&logPen))
	{
		CBrush* pOldBrush = dc.SelectObject(&brush);
		CPen* pOldPen = dc.SelectObject(&pen);
		dc.Rectangle(rc);
		if (pOldBrush)
			dc.SelectObject(pOldBrush);
		if (pOldPen)
			dc.SelectObject(pOldPen);
	}
}
