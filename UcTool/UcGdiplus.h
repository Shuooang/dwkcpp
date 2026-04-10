#pragma once
/// #gdiplus_error: Gdiplus 충돌 오류 방지용 헤더
/// 주의: 이파일은 head file에만 include 되어야 함
/// cpp 파일에서는 #include <gdiplus.h> 하고 using namespace Gdiplus; 해도 된다.
/// 이파일은 gdiplus.h를 포함하지 않고 Gdiplus의 PointF, RectF를 대신할 구조체와
/// 다른 헤더때문에 gdiplus 충동 오류가 난 경우 (c++17)를 피하기 위한 변환 함수 선언을 포함함


/// @brief Gdiplus::PointF가 헤더에서 변수로 쓰인 경우 대신 사용할 객체
/// 헤더에서 #include <gdiplus.h>를 할 수 없는 경우가 있어 별도 정의
/// Gdiplus::PointF를 쓰는 함수중 헤더에 몸체가 있는 경우 cpp로 옮겨야 함
/// 예: Contour.h 의 CContour 클래스
/// 
#ifdef _Sample_
#include "UcTool/UcGdiplus.h"
class AFX_EXT_CLASS ContourRect
{
public:
	GRectF _rect;//#gdiplus_error
};

#define _HAS_STD_BYTE 0
#include <gdiplus.h>//#gdiplus_error
using namespace Gdiplus;//#gdiplus_error
#endif // _Sample_

struct GPointF
{
	float X{0.f};
	float Y{0.f};

	GPointF() = default;
	GPointF(float x, float y) : X(x), Y(y) {}
};


class GRectF
{
public:
	float X{0.f};
	float Y{0.f};
	float Width{0.f};
	float Height{0.f};

	GRectF()
	{
		X = Y = Width = Height = 0.0f;
	}

	GRectF(IN float x,
			IN float y,
			IN float width,
			IN float height)
	{
		X = x;
		Y = y;
		Width = width;
		Height = height;
	}
	float GetLeft() const	{	return X;	}
	float GetTop() const	{		return Y;	}
	float GetRight() const	{	return X + Width;	}
	float GetBottom() const	{		return Y + Height;	}
};

class GRect
{
public:

	GRect() {
		X = Y = Width = Height = 0;
	}

	GRect(IN int x, IN int y, IN int width,IN int height)	{
		X = x;
		Y = y;
		Width = width;
		Height = height;
	}

	int GetLeft() const	{		return X;	}
	int GetTop() const	{		return Y;	}
	int GetRight() const	{		return X + Width;	}
	int GetBottom() const	{		return Y + Height;	}

public:
	INT X;
	INT Y;
	INT Width;
	INT Height;
};

class GColor
{
public:
	GColor()
	{
	}
	GColor(IN unsigned long argb)
	{
		Argb = argb;
	}
public:
	//enum Black = 0xFF000000
	unsigned long Argb{0xFF000000};
};


namespace Gdiplus {
class Graphics;
class Bitmap;

class Color;

class PointF;
class RectF;
class Rect;
}

Gdiplus::RectF FromGRectF(const GRectF& grcf);
Gdiplus::Rect FromGRect(const GRect& grcf);
//tipRc = FromGRectF(_rect);//#gdiplus_error

GRectF ToGRectF(const Gdiplus::RectF& rcf);
GRect ToGRect(const Gdiplus::Rect& rcf);
