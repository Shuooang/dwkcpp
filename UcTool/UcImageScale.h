#pragma once
#include <atlimage.h>
#include "UcExport.inl"

/// 이미지 확대/축소 품질 유틸 (doc/VC++_이미지_확대_품질_개선.md)
/// - Halftone: GDI SetStretchBltMode(HALFTONE)
/// - Bicubic: GDI+ InterpolationModeHighQualityBicubic + 알파 합성
namespace UcImageScale {

enum class Quality {
	Halftone,
	Bicubic,
};

/// GdiplusStartup 참조 카운트. InitInstance에서 Startup, ExitInstance에서 Shutdown.
UCTOOLDYNAMIC bool Startup();
UCTOOLDYNAMIC void Shutdown();

UCTOOLDYNAMIC bool Draw(HDC hdc, CImage& img, const RECT& dest,
	Quality quality = Quality::Bicubic,
	bool premultipliedAlpha = false);

UCTOOLDYNAMIC bool Draw(HDC hdc, CImage& img,
	int dx, int dy, int dw, int dh,
	int sx, int sy, int sw, int sh,
	Quality quality = Quality::Bicubic,
	bool premultipliedAlpha = false);

UCTOOLDYNAMIC CRect DrawCentered(HDC hdc, CImage& img, const CRect& destRect,
	float scale = 1.f,
	Quality quality = Quality::Bicubic,
	bool premultipliedAlpha = false);

UCTOOLDYNAMIC CRect DrawCenteredSize(HDC hdc, CImage& img, const CRect& destRect,
	int boxW, int boxH,
	Quality quality = Quality::Bicubic,
	bool premultipliedAlpha = false);

} // namespace UcImageScale
