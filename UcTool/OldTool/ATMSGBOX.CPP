#include "pch.h"
#include "ATMSGBOX.H"
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(KAutoMsgBox,CFrameWnd)//CWnd)

BEGIN_MESSAGE_MAP(KAutoMsgBox, CFrameWnd)//CWnd)
	//{{AFX_MSG_MAP(CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

KAutoMsgBox::KAutoMsgBox()
{
	memset(&m_font, 0, sizeof(m_font));
	// Since design guide says toolbars are fixed height so is the font.
	m_font.lfHeight = 20;
	m_font.lfWeight = FW_NORMAL;
	m_font.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	lstrcpy(m_font.lfFaceName, L"�ü�ü");
	m_timerID=-1;
	m_pParent = NULL;
}
KAutoMsgBox::~KAutoMsgBox()
{
}
BOOL KAutoMsgBox::PreCreateWindow(CREATESTRUCT &cs)
{
//	HBRUSH hb = (HBRUSH)GetStockObject(NULL_BRUSH);
//	cs.lpszClass = AfxRegisterWndClass(NULL,NULL, hb, NULL);
	return TRUE;
}
BOOL KAutoMsgBox::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}
BOOL KAutoMsgBox::Create(CWnd* pParentWnd)
{
	LPCTSTR lpszClass =
		AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
							LoadCursor(NULL, IDC_UPARROW),
							(HBRUSH)(COLOR_WINDOW+1),NULL);
	BOOL b = CFrameWnd::Create(lpszClass, NULL, WS_POPUP|WS_DLGFRAME, 
						CRect(0,0,0,0), pParentWnd);
	NTRACE(L"KLogoWnd CWnd::Create(... %s\n", b ? L"Success":L"Fail");
	return b;

/*	BOOL rv =  CreateEx(0, NULL, NULL, WS_POPUP,//|WS_BORDER,
//	BOOL rv =  CreateEx(0, "AfxControlBar", NULL, WS_POPUP,//|WS_BORDER,
		m_rc.left,m_rc.top,m_rc.Width(),m_rc.Height(), 
		pParentWnd->GetSafeHwnd(), NULL, NULL);
		
	
	return rv;
*/
}



void KAutoMsgBox::OnPaint()
{
	static BOOL s_B=0;
	if(s_B)
		return;
	CPaintDC dc(this);


//////////////////// RECT ///////////////////   

	CBrush* pOldBrush=NULL;
	CPen* pOldPen=NULL;
	static LOGPEN logPen[4] = {{PS_SOLID, {1,1}, KW_WHITE},
						{PS_SOLID, {1,1}, KW_BLACK},
						{PS_SOLID, {1,1}, KW_DARKGRAY},
						{PS_SOLID, {0,0}, KW_GRAY},
						};
	LOGBRUSH logBrush = {BS_SOLID,KW_GRAY, 0};
	CBrush brush;
	CPen pen[4];
	if (!brush.CreateBrushIndirect(&logBrush))
		return;
	BOOL b = pen[0].CreatePenIndirect(&logPen[0]);
	b |= pen[1].CreatePenIndirect(&logPen[1]);
	b |= pen[2].CreatePenIndirect(&logPen[2]);
	b |= pen[3].CreatePenIndirect(&logPen[3]);
	

	if(b)
	{	CBrush* pOldBrush=NULL;
		CPen* pOldPen=NULL;
		
		pOldBrush = dc.SelectObject(&brush);
		pOldPen = dc.SelectObject(&pen[3]);
		
		CRect crc(0,0,m_rc.Width(),m_rc.Height());
		
		dc.Rectangle(crc);

		pOldPen = dc.SelectObject(&pen[0]);
		
		dc.MoveTo(crc.left+1,crc.top+1);
		dc.LineTo(crc.right-1,crc.top+1);
		dc.MoveTo(crc.left+1,crc.top+1);
		dc.LineTo(crc.left+1,crc.bottom-1);

		dc.SelectObject(&pen[2]);
		
		dc.MoveTo(crc.right-2,crc.bottom-2);
		dc.LineTo(crc.right-2,crc.top+1);
		dc.MoveTo(crc.right-2,crc.bottom-2);
		dc.LineTo(crc.left+1,crc.bottom-2);
		
		dc.SelectObject(&pen[1]);

		dc.MoveTo(crc.right-1,crc.bottom-1);
		dc.LineTo(crc.right-1,crc.top+1);
		dc.MoveTo(crc.right-1,crc.bottom-1);
		dc.LineTo(crc.left+1,crc.bottom-1);
		
		if(pOldBrush) dc.SelectObject(pOldBrush);
		if(pOldPen) dc.SelectObject(pOldPen);
		brush.DeleteObject();

		pen[0].DeleteObject();
		pen[1].DeleteObject();
		pen[2].DeleteObject();
		pen[3].DeleteObject();
	}
	else
	{
		brush.DeleteObject();
	}
////////////// TEXT /////////

    COLORREF oldTextColor = dc.GetTextColor();
	int oldBkMode = dc.SetBkMode(TRANSPARENT);     //KDW �׻� �̴�.
	dc.SetBkColor(KW_GRAY);

    CFont font, *pOldFont=NULL;
    if(!font.CreateFontIndirect(&m_font))
		TRACE("Could Not create font\n");
	else
		pOldFont = dc.SelectObject(&font);

//	dc.SetTextColor(KW_DARKGRAY);
//	dc.DrawText( (LPCSTR)m_msg,m_msg.GetLength(), m_rcTxt-CPoint(1,1), DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_WORDBREAK);

	dc.SetTextColor(KW_WHITE);
	dc.DrawText( (LPCTSTR)m_msg,m_msg.GetLength(), m_rcTxt+CPoint(1,1), DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_WORDBREAK);

	dc.SetTextColor(KW_BLACK);
	dc.DrawText( (LPCTSTR)m_msg,m_msg.GetLength(), m_rcTxt, DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_WORDBREAK);

	dc.SetTextColor(oldTextColor);
	dc.SetBkMode(oldBkMode);	 //KDW
	if(pOldFont) dc.SelectObject(pOldFont);
	font.DeleteObject();
//////////////// END TEXT ///////////////////
	if(m_timerID==-1)
		m_timerID = SetTimer(1,m_period,NULL);



}
void KAutoMsgBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	OnTimer(1);	
	//CDialog::OnLButtonDown(nFlags, point);
}

int KAutoMsgBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	CRect rc(10,10,32+10,32+10);
	m_static.Create(L"",WS_CHILD|SS_ICON|WS_VISIBLE,rc,this);
	LPSTR lpRsName = NULL;
	switch(m_styleIcon)
	{	
		case MB_ICONSTOP:	lpRsName=(LPSTR)IDI_HAND; break;
		case MB_ICONQUESTION:	lpRsName=(LPSTR)IDI_QUESTION; break;
		case MB_ICONASTERISK:	lpRsName=(LPSTR)IDI_ASTERISK; break;
		case MB_ICONEXCLAMATION:	lpRsName=(LPSTR)IDI_EXCLAMATION; break;
	}
	
	HICON hIcon = AfxGetApp()->LoadStandardIcon((PS)lpRsName);
	m_static.SetIcon(hIcon);	

	SetWindowPos((const CWnd*)&CWnd::wndTopMost,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
	MoveWindow(rc);
	KwCenterDialog(this);
//	ShowWindow(SW_SHOW);
	//SetTimer(100,m_sec*1000,NULL);
	KwReady(500);



	return 0;
}

void KAutoMsgBox::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	
}

void KAutoMsgBox::OnTimer(UINT_PTR nIDEvent)
{
	m_timerID=-1;
	CFrameWnd::OnTimer(nIDEvent);
//	KillTimer(m_timerID);
	DestroyWindow();
}
void KwAutoMessageBox(CWnd *pParent,CString str,int period,UINT styleIcon,int x,int y)
{
//	static KAutoMsgBox dlgMsg;
	KAutoMsgBox *pdlg= new KAutoMsgBox;
	KAutoMsgBox &dlgMsg = *pdlg;
//	if(dlgMsg.IsWindowEnabled()) return;
	dlgMsg.m_msg = str;
	dlgMsg.m_styleIcon = styleIcon;
	dlgMsg.m_rc.left = x;
	dlgMsg.m_rc.top = y;
	dlgMsg.m_period = period;
	dlgMsg.m_pParent = pParent;
	dlgMsg.Create(dlgMsg.m_pParent ? dlgMsg.m_pParent : AfxGetMainWnd());

	CDC * pDC = dlgMsg.GetDC();
    CFont font, *pOldFont=NULL;
    if(!font.CreateFontIndirect(&dlgMsg.m_font))
		TRACE("Could Not create font\n");
	else
		pOldFont = pDC->SelectObject(&font);
	// width�� �˾� ����.
	CSize szTx = pDC->GetTextExtent( (PS)dlgMsg.m_msg, dlgMsg.m_msg.GetLength());
	if(pOldFont) pDC->SelectObject(pOldFont);
	font.DeleteObject();
	dlgMsg.ReleaseDC(pDC);
	const int iconDim=10+32+10;
	dlgMsg.m_rc.right = dlgMsg.m_rc.left + iconDim + szTx.cx + 10;
	dlgMsg.m_rc.bottom = dlgMsg.m_rc.top + 10 + szTx.cy + 10;
	if(dlgMsg.m_rc.Height() < iconDim)
		dlgMsg.m_rc.bottom = dlgMsg.m_rc.top + iconDim;
	
	dlgMsg.m_rcTxt = CRect(iconDim,10,iconDim + szTx.cx,10 + szTx.cy);

	CSize szCl(dlgMsg.m_rc.Width(),dlgMsg.m_rc.Height());

	CRect rc,drc;
	CWnd *deskTop = dlgMsg.GetDesktopWindow();
	deskTop->GetWindowRect((LPRECT)drc);
	rc.left = (dlgMsg.m_rc.left==-1)? (drc.right / 2) - (szCl.cx / 2) : dlgMsg.m_rc.left;
	rc.right = rc.left + szCl.cx;
	rc.top = (dlgMsg.m_rc.top==-1)? (drc.bottom / 2) - (szCl.cy /2) : dlgMsg.m_rc.top;
	rc.bottom = rc.top + szCl.cy;
	dlgMsg.m_rc = rc;


	dlgMsg.MoveWindow(dlgMsg.m_rc,FALSE);
	dlgMsg.ShowWindow(SW_SHOW);	
	KwReady(500);
}
void KwAtMsgPrintf(CWnd *pParent, LPCTSTR lpFormat, ... )
{
	CString buf;
	CSTRING_SPRINTF(lpFormat,buf)	;
	KwAutoMessageBox(pParent,buf);
}
