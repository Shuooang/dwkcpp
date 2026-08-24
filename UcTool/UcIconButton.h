#pragma once
#include <afxwin.h>
#include <atlimage.h>
#include <functional>
#include "UcExport.inl"

/// 공용 아이콘 버튼 — 배경 PNG + 왼쪽 아이콘 PNG + 텍스트 최대 3줄 (owner-draw)
/// 투명 PNG는 A 채널(AlphaBlend / GDI+ Bicubic)로 합성. 투명 영역은 부모 배경이 비침.
class UCTOOLDYNAMIC CUcIconButton : public CButton
{
public:
	/// 텍스트 줄 인덱스: 0..2, -1=전체 줄
	static constexpr int kLineAll = -1;

	CUcIconButton() = default;
	~CUcIconButton() override;

	/// PNG 리소스(type "PNG"). hInst==nullptr 이면 AfxGetResourceHandle().
	BOOL SetImages(UINT idBgPng, UINT idIconPng, HINSTANCE hInst = nullptr);
	void SetTexts(LPCWSTR t1, LPCWSTR t2, LPCWSTR t3);
	CSize GetBgPixelSize() const;

	/// 투명 가장자리에 비칠 부모 배경. 수명은 호출측 관리.
	void SetParentBackdrop(CImage* pBg) { m_pParentBg = pBg; }

	CString GetText1() const { return m_text[0]; }
	CString GetText2() const { return m_text[1]; }
	CString GetText3() const { return m_text[2]; }

	void EnsureOwnerDraw();

	void SetFontSize(int line, int pointSize);
	void SetFontBold(int line, bool bold);
	void SetFontName(int line, LPCWSTR faceName);
	void SetFontColor(int line, COLORREF color);

	void SetTextMargin(int left, int top, int right, int bottom);
	void SetTextOffset(int dx, int dy);
	void SetTextVCenter(bool vcenter) { m_textVCenter = vcenter; InvalidateIfAlive(); }

	void DrawFont(CDC& dc, CRect rc, LPCWSTR text,
		int pointSize, bool bold, COLORREF color,
		UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
		LPCWSTR faceName = L"맑은 고딕");

	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;

	void SetIconAreaRatio(float r) { m_iconAreaRatio = r; }

	void SetIconDrawScale(float scale);
	void SetIconDrawSize(int cx, int cy);
	void SetIconDrawSize(CSize size) { SetIconDrawSize(size.cx, size.cy); }

	float GetIconDrawScale() const { return m_iconDrawScale; }
	CSize GetIconDrawSize() const { return m_iconDrawSize; }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();

	struct LineFont {
		int pointSize{ 9 };
		bool bold{ false };
		CString faceName{ L"맑은 고딕" };
		COLORREF color{ RGB(100, 116, 139) };
	};

	static BOOL LoadPngResource(CImage& img, UINT nId, bool premultiplyAlpha, HINSTANCE hInst);
	static void PremultiplyAlpha(CImage& img);
	static void DrawPngAlpha(CDC& dc, CImage& img,
		int dx, int dy, int dw, int dh,
		int sx, int sy, int sw, int sh);

	void InvalidateIfAlive();
	void ApplyToLines(int line, const std::function<void(LineFont&)>& fn);
	void SyncIconScaleSize();
	void DrawParentBackdrop(CDC& dc, const CRect& rc);
	void DrawBackground(CDC& dc, const CRect& rc);
	void DrawIcon(CDC& dc, const CRect& rcIcon);
	void DrawTexts(CDC& dc, const CRect& rcText, bool pressed);

	enum class IconDrawSpec { ByScale, BySize };

	CImage m_bg;
	CImage m_icon;
	CImage* m_pParentBg{ nullptr };
	CString m_text[3];
	LineFont m_lineFont[3]{
		{ 9, false, L"맑은 고딕", RGB(100, 116, 139) },
		{ 12, false, L"맑은 고딕", RGB(30, 41, 59) },
		{ 9, false, L"맑은 고딕", RGB(100, 116, 139) },
	};

	float m_iconAreaRatio{ 0.30f };
	float m_iconDrawScale{ 1.0f };
	CSize m_iconDrawSize{ 0, 0 };
	IconDrawSpec m_iconDrawSpec{ IconDrawSpec::ByScale };

	int m_textPadL{ 10 };
	int m_textPadT{ 12 };
	int m_textPadR{ 14 };
	int m_textPadB{ 12 };
	int m_textOffX{ 0 };
	int m_textOffY{ 0 };
	bool m_textVCenter{ true };
	HINSTANCE m_hResInst{ nullptr };
};
