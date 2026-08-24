#include "pch.h"
#include "UcIconButton.h"
#include "UcImageScale.h"
#include <algorithm>

#pragma comment(lib, "msimg32.lib")

namespace {

void DrawPressedOverlay(HDC hdc, const RECT& rc, BYTE alpha = 56)
{
	const int w = rc.right - rc.left;
	const int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0)
		return;
	HDC hdcMem = ::CreateCompatibleDC(hdc);
	HBITMAP hBmp = ::CreateCompatibleBitmap(hdc, w, h);
	HGDIOBJ hOld = ::SelectObject(hdcMem, hBmp);
	::PatBlt(hdcMem, 0, 0, w, h, BLACKNESS);
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, 0 };
	::AlphaBlend(hdc, rc.left, rc.top, w, h, hdcMem, 0, 0, w, h, bf);
	::SelectObject(hdcMem, hOld);
	::DeleteObject(hBmp);
	::DeleteDC(hdcMem);
}

} // namespace

CUcIconButton::~CUcIconButton()
{
	m_bg.Destroy();
	m_icon.Destroy();
	m_pParentBg = nullptr;
}

void CUcIconButton::PremultiplyAlpha(CImage& img)
{
	if (img.IsNull() || img.GetBPP() != 32)
		return;

	img.SetHasAlphaChannel(true);

	const int w = img.GetWidth();
	const int h = img.GetHeight();
	for (int y = 0; y < h; ++y) {
		BYTE* row = static_cast<BYTE*>(img.GetPixelAddress(0, y));
		if (!row)
			continue;
		for (int x = 0; x < w; ++x) {
			BYTE* p = row + x * 4; // BGRA
			const unsigned a = p[3];
			p[0] = static_cast<BYTE>((p[0] * a) / 255);
			p[1] = static_cast<BYTE>((p[1] * a) / 255);
			p[2] = static_cast<BYTE>((p[2] * a) / 255);
		}
	}
}

void CUcIconButton::DrawPngAlpha(CDC& dc, CImage& img,
	int dx, int dy, int dw, int dh,
	int sx, int sy, int sw, int sh)
{
	if (img.IsNull() || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
		return;

	if (img.GetBPP() == 32) {
		img.AlphaBlend(dc.GetSafeHdc(), dx, dy, dw, dh, sx, sy, sw, sh, 255);
		return;
	}
	::SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
	img.Draw(dc.GetSafeHdc(), dx, dy, dw, dh, sx, sy, sw, sh);
}

BOOL CUcIconButton::LoadPngResource(CImage& img, UINT nId, bool premultiplyAlpha, HINSTANCE hInst)
{
	img.Destroy();
	if (!hInst)
		hInst = AfxGetResourceHandle();
	HRSRC hRes = ::FindResource(hInst, MAKEINTRESOURCE(nId), L"PNG");
	if (!hRes)
		return FALSE;
	const DWORD size = ::SizeofResource(hInst, hRes);
	HGLOBAL hData = ::LoadResource(hInst, hRes);
	const void* pData = hData ? ::LockResource(hData) : nullptr;
	if (!pData || size == 0)
		return FALSE;

	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hMem)
		return FALSE;
	void* pMem = ::GlobalLock(hMem);
	if (!pMem) {
		::GlobalFree(hMem);
		return FALSE;
	}
	memcpy(pMem, pData, size);
	::GlobalUnlock(hMem);

	IStream* pStream = nullptr;
	if (FAILED(::CreateStreamOnHGlobal(hMem, TRUE, &pStream)) || !pStream) {
		::GlobalFree(hMem);
		return FALSE;
	}
	const HRESULT hr = img.Load(pStream);
	pStream->Release();
	if (FAILED(hr))
		return FALSE;

	if (premultiplyAlpha)
		PremultiplyAlpha(img);
	else if (img.GetBPP() == 32)
		img.SetHasAlphaChannel(true);
	return TRUE;
}

BOOL CUcIconButton::SetImages(UINT idBgPng, UINT idIconPng, HINSTANCE hInst)
{
	m_hResInst = hInst ? hInst : AfxGetResourceHandle();
	const BOOL okBg = LoadPngResource(m_bg, idBgPng, true, m_hResInst);
	const BOOL okIcon = LoadPngResource(m_icon, idIconPng, false, m_hResInst);
	SyncIconScaleSize();
	InvalidateIfAlive();
	return okBg && okIcon;
}

void CUcIconButton::SetIconDrawScale(float scale)
{
	if (scale <= 0.f)
		scale = 1.f;
	m_iconDrawSpec = IconDrawSpec::ByScale;
	m_iconDrawScale = scale;
	SyncIconScaleSize();
	InvalidateIfAlive();
}

void CUcIconButton::SetIconDrawSize(int cx, int cy)
{
	if (cx < 1) cx = 1;
	if (cy < 1) cy = 1;
	m_iconDrawSpec = IconDrawSpec::BySize;
	m_iconDrawSize = CSize(cx, cy);
	SyncIconScaleSize();
	InvalidateIfAlive();
}

void CUcIconButton::SyncIconScaleSize()
{
	if (m_icon.IsNull()) {
		if (m_iconDrawSpec == IconDrawSpec::ByScale)
			m_iconDrawSize = CSize(0, 0);
		return;
	}

	const int iw = m_icon.GetWidth();
	const int ih = m_icon.GetHeight();
	if (iw <= 0 || ih <= 0)
		return;

	if (m_iconDrawSpec == IconDrawSpec::ByScale) {
		m_iconDrawSize.cx = (std::max)(1, (int)(iw * m_iconDrawScale + 0.5f));
		m_iconDrawSize.cy = (std::max)(1, (int)(ih * m_iconDrawScale + 0.5f));
	}
	else {
		const float sx = (float)m_iconDrawSize.cx / (float)iw;
		const float sy = (float)m_iconDrawSize.cy / (float)ih;
		m_iconDrawScale = (sx < sy) ? sx : sy;
		m_iconDrawSize.cx = (std::max)(1, (int)(iw * m_iconDrawScale + 0.5f));
		m_iconDrawSize.cy = (std::max)(1, (int)(ih * m_iconDrawScale + 0.5f));
	}
}

CSize CUcIconButton::GetBgPixelSize() const
{
	if (m_bg.IsNull())
		return CSize(0, 0);
	return CSize(m_bg.GetWidth(), m_bg.GetHeight());
}

void CUcIconButton::SetTexts(LPCWSTR t1, LPCWSTR t2, LPCWSTR t3)
{
	m_text[0] = t1 ? t1 : L"";
	m_text[1] = t2 ? t2 : L"";
	m_text[2] = t3 ? t3 : L"";
	InvalidateIfAlive();
}

void CUcIconButton::EnsureOwnerDraw()
{
	if (!GetSafeHwnd())
		return;
	ModifyStyle(BS_TYPEMASK | WS_BORDER, BS_OWNERDRAW);
	ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME, 0);
}

void CUcIconButton::InvalidateIfAlive()
{
	if (GetSafeHwnd())
		Invalidate(FALSE);
}

void CUcIconButton::ApplyToLines(int line, const std::function<void(LineFont&)>& fn)
{
	if (line == kLineAll) {
		for (int i = 0; i < 3; ++i)
			fn(m_lineFont[i]);
		return;
	}
	if (line >= 0 && line < 3)
		fn(m_lineFont[line]);
}

void CUcIconButton::SetFontSize(int line, int pointSize)
{
	if (pointSize < 1)
		pointSize = 1;
	ApplyToLines(line, [pointSize](LineFont& f) { f.pointSize = pointSize; });
	InvalidateIfAlive();
}

void CUcIconButton::SetFontBold(int line, bool bold)
{
	ApplyToLines(line, [bold](LineFont& f) { f.bold = bold; });
	InvalidateIfAlive();
}

void CUcIconButton::SetFontName(int line, LPCWSTR faceName)
{
	const CString name = (faceName && *faceName) ? faceName : L"맑은 고딕";
	ApplyToLines(line, [&name](LineFont& f) { f.faceName = name; });
	InvalidateIfAlive();
}

void CUcIconButton::SetFontColor(int line, COLORREF color)
{
	ApplyToLines(line, [color](LineFont& f) { f.color = color; });
	InvalidateIfAlive();
}

void CUcIconButton::SetTextMargin(int left, int top, int right, int bottom)
{
	m_textPadL = left;
	m_textPadT = top;
	m_textPadR = right;
	m_textPadB = bottom;
	InvalidateIfAlive();
}

void CUcIconButton::SetTextOffset(int dx, int dy)
{
	m_textOffX = dx;
	m_textOffY = dy;
	InvalidateIfAlive();
}

void CUcIconButton::DrawFont(CDC& dc, CRect rc, LPCWSTR text,
	int pointSize, bool bold, COLORREF color, UINT format, LPCWSTR faceName)
{
	if (!text || !*text || rc.IsRectEmpty())
		return;

	LOGFONTW lf{};
	lf.lfHeight = -MulDiv(pointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
	lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = CLEARTYPE_QUALITY;
	wcsncpy_s(lf.lfFaceName, faceName ? faceName : L"맑은 고딕", _TRUNCATE);

	CFont font;
	if (!font.CreateFontIndirect(&lf))
		return;

	CFont* pOld = dc.SelectObject(&font);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(color);
	dc.DrawText(text, -1, &rc, format);
	dc.SelectObject(pOld);
}

void CUcIconButton::DrawParentBackdrop(CDC& dc, const CRect& rc)
{
	if (!m_pParentBg || m_pParentBg->IsNull()) {
		dc.FillSolidRect(rc, RGB(255, 255, 255));
		return;
	}

	CPoint pt(0, 0);
	MapWindowPoints(GetParent(), &pt, 1);

	const int iw = m_pParentBg->GetWidth();
	const int ih = m_pParentBg->GetHeight();
	int sx = pt.x;
	int sy = pt.y;
	int dw = rc.Width();
	int dh = rc.Height();
	int dx = rc.left;
	int dy = rc.top;

	if (sx < 0) { dw += sx; dx -= sx; sx = 0; }
	if (sy < 0) { dh += sy; dy -= sy; sy = 0; }
	if (sx + dw > iw) dw = iw - sx;
	if (sy + dh > ih) dh = ih - sy;
	if (dw <= 0 || dh <= 0) {
		dc.FillSolidRect(rc, RGB(255, 255, 255));
		return;
	}

	::SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
	m_pParentBg->Draw(dc.GetSafeHdc(), dx, dy, dw, dh, sx, sy, dw, dh);
}

void CUcIconButton::DrawBackground(CDC& dc, const CRect& rc)
{
	if (m_bg.IsNull())
		return;

	const int iw = m_bg.GetWidth();
	const int ih = m_bg.GetHeight();
	DrawPngAlpha(dc, m_bg,
		rc.left, rc.top, rc.Width(), rc.Height(),
		0, 0, iw, ih);
}

void CUcIconButton::DrawIcon(CDC& dc, const CRect& rcIcon)
{
	if (m_icon.IsNull() || rcIcon.IsRectEmpty())
		return;

	if (m_iconDrawSize.cx > 0 && m_iconDrawSize.cy > 0) {
		UcImageScale::DrawCenteredSize(
			dc.GetSafeHdc(), m_icon, rcIcon,
			m_iconDrawSize.cx, m_iconDrawSize.cy,
			UcImageScale::Quality::Bicubic,
			false);
		return;
	}

	UcImageScale::DrawCentered(
		dc.GetSafeHdc(), m_icon, rcIcon,
		m_iconDrawScale,
		UcImageScale::Quality::Bicubic,
		false);
}

void CUcIconButton::DrawTexts(CDC& dc, const CRect& rcText, bool pressed)
{
	CRect rc = rcText;
	rc.OffsetRect(m_textOffX, m_textOffY);
	if (pressed)
		rc.OffsetRect(1, 1);
	if (rc.IsRectEmpty())
		return;

	int used[3]{};
	int nUsed = 0;
	for (int i = 0; i < 3; ++i) {
		if (!m_text[i].IsEmpty())
			used[nUsed++] = i;
	}
	if (nUsed == 0)
		return;

	int maxPt = 9;
	for (int i = 0; i < nUsed; ++i)
		maxPt = (std::max)(maxPt, m_lineFont[used[i]].pointSize);
	const int lineH = (std::max)(MulDiv(maxPt, dc.GetDeviceCaps(LOGPIXELSY), 72) + 6, rc.Height() / (std::max)(nUsed, 1));
	const int blockH = lineH * nUsed;
	int y0 = rc.top;
	if (m_textVCenter && blockH < rc.Height())
		y0 = rc.top + (rc.Height() - blockH) / 2;

	for (int i = 0; i < nUsed; ++i) {
		const int li = used[i];
		const LineFont& f = m_lineFont[li];
		CRect r(rc.left, y0 + i * lineH, rc.right, y0 + (i + 1) * lineH);
		DrawFont(dc, r, m_text[li], f.pointSize, f.bold, f.color,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS,
			f.faceName);
	}
}

void CUcIconButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	if (!lpDIS)
		return;

	const CRect rc(lpDIS->rcItem);
	const bool pressed = (lpDIS->itemState & ODS_SELECTED) != 0;

	CDC dc;
	dc.Attach(lpDIS->hDC);

	DrawParentBackdrop(dc, rc);
	DrawBackground(dc, rc);

	const int iconW = (int)(rc.Width() * m_iconAreaRatio + 0.5f);
	CRect rcIcon(rc.left, rc.top, rc.left + iconW, rc.bottom);
	CRect rcText(
		rc.left + iconW + m_textPadL,
		rc.top + m_textPadT,
		rc.right - m_textPadR,
		rc.bottom - m_textPadB);
	if (pressed)
		rcIcon.OffsetRect(1, 1);

	DrawIcon(dc, rcIcon);
	DrawTexts(dc, rcText, pressed);

	if (pressed)
		DrawPressedOverlay(dc.GetSafeHdc(), rc, 48);

	dc.Detach();
}

BEGIN_MESSAGE_MAP(CUcIconButton, CButton)
	ON_WM_SETCURSOR()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CUcIconButton::OnSetCursor(CWnd* /*pWnd*/, UINT /*nHitTest*/, UINT /*message*/)
{
	::SetCursor(::LoadCursor(nullptr, IDC_HAND));
	return TRUE;
}

BOOL CUcIconButton::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CUcIconButton::OnDestroy()
{
	CButton::OnDestroy();
}
