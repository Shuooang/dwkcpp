#include "pch.h"
#include "UcImageScale.h"

#include <memory>

#define _HAS_STD_BYTE 0
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

namespace {

ULONG_PTR g_gdiplusToken = 0;
LONG g_gdiplusRefs = 0;

bool EnsureGdiplus()
{
	return UcImageScale::Startup();
}

void CopyRowToArgb(BYTE* dst, const BYTE* src, int width, bool premultiplied)
{
	for (int x = 0; x < width; ++x) {
		BYTE b = src[0], g = src[1], r = src[2], a = src[3];
		if (premultiplied && a > 0 && a < 255) {
			r = static_cast<BYTE>((r * 255) / a);
			g = static_cast<BYTE>((g * 255) / a);
			b = static_cast<BYTE>((b * 255) / a);
		}
		else if (premultiplied && a == 0) {
			r = g = b = 0;
		}
		dst[0] = b;
		dst[1] = g;
		dst[2] = r;
		dst[3] = a;
		src += 4;
		dst += 4;
	}
}

bool DrawHalftone(HDC hdc, CImage& img,
	int dx, int dy, int dw, int dh,
	int sx, int sy, int sw, int sh)
{
	if (img.IsNull() || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
		return false;

	const int oldMode = ::SetStretchBltMode(hdc, HALFTONE);
	POINT oldOrg{};
	::SetBrushOrgEx(hdc, 0, 0, &oldOrg);

	const BOOL ok = img.StretchBlt(hdc, dx, dy, dw, dh, sx, sy, sw, sh, SRCCOPY);

	::SetBrushOrgEx(hdc, oldOrg.x, oldOrg.y, nullptr);
	::SetStretchBltMode(hdc, oldMode);
	return ok != FALSE;
}

std::unique_ptr<Gdiplus::Bitmap> CImageToGdiplusBitmap(CImage& img, bool premultipliedAlpha)
{
	if (img.IsNull())
		return nullptr;

	const int w = img.GetWidth();
	const int h = img.GetHeight();
	if (w <= 0 || h <= 0)
		return nullptr;

	auto bmp = std::make_unique<Gdiplus::Bitmap>(w, h, PixelFormat32bppARGB);
	if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok)
		return nullptr;

	Gdiplus::BitmapData bd{};
	const Gdiplus::Rect lockRc(0, 0, w, h);
	if (bmp->LockBits(&lockRc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bd) != Gdiplus::Ok)
		return nullptr;

	const int bpp = img.GetBPP();
	for (int y = 0; y < h; ++y) {
		BYTE* dst = static_cast<BYTE*>(bd.Scan0) + static_cast<size_t>(y) * bd.Stride;
		BYTE* src = static_cast<BYTE*>(img.GetPixelAddress(0, y));
		if (!src) {
			memset(dst, 0, static_cast<size_t>(w) * 4);
			continue;
		}
		if (bpp == 32) {
			CopyRowToArgb(dst, src, w, premultipliedAlpha);
		}
		else if (bpp == 24) {
			for (int x = 0; x < w; ++x) {
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				dst[3] = 255;
				src += 3;
				dst += 4;
			}
		}
		else {
			memset(dst, 0, static_cast<size_t>(w) * 4);
		}
	}

	bmp->UnlockBits(&bd);
	return bmp;
}

bool DrawBicubic(HDC hdc, CImage& img,
	int dx, int dy, int dw, int dh,
	int sx, int sy, int sw, int sh,
	bool premultipliedAlpha)
{
	if (!EnsureGdiplus())
		return DrawHalftone(hdc, img, dx, dy, dw, dh, sx, sy, sw, sh);

	auto bmp = CImageToGdiplusBitmap(img, premultipliedAlpha);
	if (!bmp)
		return DrawHalftone(hdc, img, dx, dy, dw, dh, sx, sy, sw, sh);

	Gdiplus::Graphics graphics(hdc);
	graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
	graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

	const Gdiplus::Status st = graphics.DrawImage(
		bmp.get(),
		Gdiplus::Rect(dx, dy, dw, dh),
		sx, sy, sw, sh,
		Gdiplus::UnitPixel);
	return st == Gdiplus::Ok;
}

} // namespace

namespace UcImageScale {

bool Startup()
{
	if (::InterlockedIncrement(&g_gdiplusRefs) == 1) {
		Gdiplus::GdiplusStartupInput input;
		if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, nullptr) != Gdiplus::Ok) {
			g_gdiplusToken = 0;
			::InterlockedDecrement(&g_gdiplusRefs);
			return false;
		}
	}
	return g_gdiplusToken != 0;
}

void Shutdown()
{
	if (::InterlockedDecrement(&g_gdiplusRefs) == 0 && g_gdiplusToken != 0) {
		Gdiplus::GdiplusShutdown(g_gdiplusToken);
		g_gdiplusToken = 0;
	}
}

bool Draw(HDC hdc, CImage& img, const RECT& dest,
	Quality quality, bool premultipliedAlpha)
{
	if (img.IsNull())
		return false;
	return Draw(hdc, img,
		dest.left, dest.top,
		dest.right - dest.left, dest.bottom - dest.top,
		0, 0, img.GetWidth(), img.GetHeight(),
		quality, premultipliedAlpha);
}

bool Draw(HDC hdc, CImage& img,
	int dx, int dy, int dw, int dh,
	int sx, int sy, int sw, int sh,
	Quality quality, bool premultipliedAlpha)
{
	if (img.IsNull() || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
		return false;

	if (quality == Quality::Halftone)
		return DrawHalftone(hdc, img, dx, dy, dw, dh, sx, sy, sw, sh);

	return DrawBicubic(hdc, img, dx, dy, dw, dh, sx, sy, sw, sh, premultipliedAlpha);
}

CRect DrawCentered(HDC hdc, CImage& img, const CRect& destRect,
	float scale, Quality quality, bool premultipliedAlpha)
{
	CRect drawn(0, 0, 0, 0);
	if (img.IsNull() || destRect.IsRectEmpty() || scale <= 0.f)
		return drawn;

	const int iw = img.GetWidth();
	const int ih = img.GetHeight();
	if (iw <= 0 || ih <= 0)
		return drawn;

	double fit = 1.0;
	const double sx = (double)destRect.Width() / iw;
	const double sy = (double)destRect.Height() / ih;
	fit = (sx < sy) ? sx : sy;
	if (fit > 1.0)
		fit = 1.0;

	const double s = fit * (double)scale;
	int dw = (int)(iw * s + 0.5);
	int dh = (int)(ih * s + 0.5);
	if (dw < 1) dw = 1;
	if (dh < 1) dh = 1;

	const int x = destRect.left + (destRect.Width() - dw) / 2;
	const int y = destRect.top + (destRect.Height() - dh) / 2;
	drawn.SetRect(x, y, x + dw, y + dh);

	if (!Draw(hdc, img, x, y, dw, dh, 0, 0, iw, ih, quality, premultipliedAlpha))
		drawn.SetRectEmpty();
	return drawn;
}

CRect DrawCenteredSize(HDC hdc, CImage& img, const CRect& destRect,
	int boxW, int boxH, Quality quality, bool premultipliedAlpha)
{
	CRect drawn(0, 0, 0, 0);
	if (img.IsNull() || destRect.IsRectEmpty() || boxW < 1 || boxH < 1)
		return drawn;

	const int iw = img.GetWidth();
	const int ih = img.GetHeight();
	if (iw <= 0 || ih <= 0)
		return drawn;

	// 吏??box ?덉뿚 醫낇슒鍮??좎?
	double sx = (double)boxW / iw;
	double sy = (double)boxH / ih;
	double s = (sx < sy) ? sx : sy;
	int dw = (int)(iw * s + 0.5);
	int dh = (int)(ih * s + 0.5);
	if (dw < 1) dw = 1;
	if (dh < 1) dh = 1;

	// 踰꾪듉 ?꾩씠肄??앹뿭???섏쑝硫???踰???clamp
	if (dw > destRect.Width() || dh > destRect.Height()) {
		const double fx = (double)destRect.Width() / dw;
		const double fy = (double)destRect.Height() / dh;
		const double f = (fx < fy) ? fx : fy;
		dw = (int)(dw * f + 0.5);
		dh = (int)(dh * f + 0.5);
		if (dw < 1) dw = 1;
		if (dh < 1) dh = 1;
	}

	const int x = destRect.left + (destRect.Width() - dw) / 2;
	const int y = destRect.top + (destRect.Height() - dh) / 2;
	drawn.SetRect(x, y, x + dw, y + dh);

	if (!Draw(hdc, img, x, y, dw, dh, 0, 0, iw, ih, quality, premultipliedAlpha))
		drawn.SetRectEmpty();
	return drawn;
}

} // namespace UcImageScale
