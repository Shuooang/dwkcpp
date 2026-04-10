//#include "stdafx.h" //byte 충돌 때문에 결국 제외
//#include <windows.h>//afx에서 포함 하지 말라고 에러난다.
#include <wtypes.h>
#include <gdiplus.h>

//#include "UcBasetools.h"//DWKWARN
//DWKWARN("This file does not include the precompiled header to avoid 'byte' name conflicts in GDI+.(c++17)")
/// #pragma push_macro("byte") 도 써버고 stdafx.h 맨위에 별짓을 다해도 이미 SDK 에서 byte가 쓰여 충돌이 나서

#include "UcGdiplus.h"

Gdiplus::RectF FromGRectF(const GRectF& grcf)//#gdiplus_error
{
	Gdiplus::RectF rcf;
	rcf.X = grcf.X;
	rcf.Y = grcf.Y;
	rcf.Width = grcf.Width;
	rcf.Height = grcf.Height;
	return rcf;
}

Gdiplus::Rect FromGRect(const GRect& grcf)
{
	Gdiplus::Rect rcf;
	rcf.X = grcf.X;
	rcf.Y = grcf.Y;
	rcf.Width = grcf.Width;
	rcf.Height = grcf.Height;
	return rcf;
}

GRectF ToGRectF(const Gdiplus::RectF& rcf)//#gdiplus_error
{
	GRectF grcf;
	grcf.X = rcf.X;
	grcf.Y = rcf.Y;
	grcf.Width = rcf.Width;
	grcf.Height = rcf.Height;
	return grcf;
}

GRect ToGRect(const Gdiplus::Rect& rcf)
{
	GRect grcf;
	grcf.X = rcf.X;
	grcf.Y = rcf.Y;
	grcf.Width = rcf.Width;
	grcf.Height = rcf.Height;
	return grcf;
}
