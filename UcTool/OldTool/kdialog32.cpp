// kdlg.cpp : implementation file
//
#include "pch.h"
#include "KDialog32.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// KDialog dialog

KDialog::KDialog(CWnd* pParent )
{
    //{{AFX_DATA_INIT(KDialog)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


KDialog::KDialog(UINT nIDTemplate, CWnd* pParentWnd)
:    CDialog(nIDTemplate, pParentWnd)
{
}

BEGIN_MESSAGE_MAP(KDialog, CDialog)
    //{{AFX_MSG_MAP(KDialog)
    ON_WM_SETCURSOR()
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_KILLFOCUS()
	ON_WM_WINDOWPOSCHANGING()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// KDialog message handlers
void KDialog::OnDestroy() 
{
	CDialog::OnDestroy();
	KPopupHelp::ClosePopupHelp();
}

void KDialog::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	if(!bShow)
		KPopupHelp::ClosePopupHelp();
}

void KDialog::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
	KPopupHelp::ClosePopupHelp();
}


void KDialog::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	CDialog::OnWindowPosChanging(lpwndpos);
//	TRACE("OnWindowPosChanging(lpwndpos);\n");
//	KPopupHelp::ClosePopupHelp();
	
}


BOOL KDialog::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
//  READY_POPUPHELP();                  ///
    static int s_lid=-1;
    int id = pWnd->GetDlgCtrlID( );  
//	TRACE("pWnd->m_hWnd(%p->%d)  Control IDC(%d)\n",pWnd,pWnd->m_hWnd,id);
    static CPoint s_pt;
    POINT pt;
    ::GetCursorPos(&pt);          
    pt.x += 20;
    pt.y += 20;
    
    if(KPopupHelp::s_point != pt)
    {   KillTimer(KPopupHelp::s_idTimer);
//      TRACE("----------------------------KillTimer(\n");
        KPopupHelp::s_idTimer=0;
        KPopupHelp::ClosePopupHelp();
        
                                              
        CWnd *pMainWnd = AfxGetMainWnd();
        CWnd *pActive=GetActiveWindow();
        CWnd *pFocus=GetFocus();
                                  
        CWnd *pDeskWnd = GetDesktopWindow();
        BOOL bOK=FALSE;
        for(CWnd *pW=pWnd;pW && pW->m_hWnd != pDeskWnd->m_hWnd;pW = pW->GetParent())
        {           
            if((pFocus &&pW->m_hWnd == pFocus->m_hWnd) || (pActive && pW->m_hWnd == pActive->m_hWnd))
            {   bOK=TRUE;
//              TRACE("!!!!!!!! SAME WINDOW !!!!!!!\n");
                break;             
            }
        }
        
        if(id > 0 && bOK)
        {   
			KPopupHelp::ReadyPopupHelp(id,this);      
            KPopupHelp::s_idTimer = SetTimer(4000,1000,KPopupHelp::PopupHelpProc);
        }
        s_lid = id;
        KPopupHelp::s_point = pt;
    }

    return CDialog::OnSetCursor(pWnd, nHitTest, message);
}






#ifdef _DEBUG
void KDialog::AddHelpItem(UINT idc,UINT ids)
{
	m_mapIDC_IDS.Add((unsigned long)MAKELONG((UINT)idc,(UINT)ids));
	TRACE("IDC(%d) - IDS(%d)\n",idc,ids);
}
#endif












/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//   Balloon Screen class

CRect KPopupHelp::s_rc;
CPoint KPopupHelp::s_point;
KPopupHelp* KPopupHelp::c_pBalloonWnd;
CString KPopupHelp::s_msg = L"죄송 합니다.\r\n도움말이 없습니다.";
int KPopupHelp::s_id = -1;
CWnd *KPopupHelp::s_pParent = NULL;
UINT_PTR KPopupHelp::s_idTimer=0;
UINT_PTR KPopupHelp::s_idTimerClose=0;	// 자기 자신의 timer id

KPopupHelp::KPopupHelp()
{
    m_cbLine = 0;
    m_bkColor = RGB(255,255,212);
    m_fontHeight = 12;
    m_fontName = _T("굴림체");
//  m_bBmp = TRUE;
}

KPopupHelp::~KPopupHelp()
{
    // Clear the static window pointer.
    ASSERT(c_pBalloonWnd == this);
    c_pBalloonWnd = NULL;
}

BEGIN_MESSAGE_MAP(KPopupHelp, CWnd)
    //{{AFX_MSG_MAP(KPopupHelp)
//  ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void KPopupHelp::OnMouseMove(UINT nFlags, CPoint point) 
{
	CWnd::OnMouseMove(nFlags, point);
	KPopupHelp::ClosePopupHelp();
}

BOOL KPopupHelp::Create(CWnd* pParentWnd )
{
//  m_bBmp = m_bitmap.LoadBitmap(IDB_BALLOON);

//  int w=0,h=0;
//  if(m_bBmp)
//  {   BITMAP bm;
//      m_bitmap.GetBitmap(&bm);
//      w = bm.bmWidth;
//      h = bm.bmHeight;
//  }
    HBRUSH hb = (HBRUSH)GetStockObject(NULL_BRUSH);
    return CreateEx(0,//"AfxControlBar", 
        AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW),hb),
        NULL, WS_POPUP, 0, 0, 0,0, pParentWnd->GetSafeHwnd(), NULL);

//  return CreateEx(0, "AfxControlBar", NULL, WS_POPUP,
//      0, 0, 0, 0, pParentWnd->GetSafeHwnd(), NULL, NULL);
}
void KPopupHelp::PostNcDestroy()
{
    // Free the C++ class.
    for(int i=0;i<m_cbLine;i++)
        if(m_arSize[i])
            delete m_arSize[i];
    delete this;                 
    c_pBalloonWnd=NULL;
}
void KPopupHelp::OnPaint()
{
    CPaintDC dc(this);                   
    CRect rc = CRect(CPoint(0,0), CSize(m_size.cx-5,m_size.cy-5));
    CBrush* pOldBrush=NULL;
    CPen* pOldPen=NULL;

    CWnd *pWndDesk = GetDesktopWindow();
    CDC *pDeskDC = pWndDesk->GetWindowDC();
    
        
    dc.BitBlt(0,0,m_size.cx,m_size.cy, pDeskDC, s_rc.left,s_rc.top,SRCCOPY);

    LOGPEN logPen = {PS_SOLID, {0,0}, RGB(0,0,0)};
    LOGBRUSH logBrush = {BS_SOLID,m_bkColor, 0};

    int oldBkMode = dc.SetBkMode(TRANSPARENT);     //KDW 항상 이다.

    logBrush.lbStyle = BS_SOLID;
    logBrush.lbColor = KW_RED; //WHITE;
    logPen.lopnColor = KW_WHITE;
    logPen.lopnStyle = PS_DOT;
    CBitmap bmShadow;

    short int azz[8] = {0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,};
    bmShadow.CreateBitmap(8,8,1,1,azz); 
    
//////////////////////////////////////////////////////////////////
//  custom pattern bitmap created OK then apply it to shdow
    logBrush.lbStyle = BS_PATTERN;
    logBrush.lbHatch = (ULONG_PTR)bmShadow.m_hObject;//HS_DIAGCROSS;
    logPen.lopnColor = logBrush.lbColor = KW_BLACK;//RGB(129,129,129);
    logPen.lopnStyle = PS_NULL;

    CBrush brush;
    CPen pen;
    if(brush.CreateBrushIndirect(&logBrush) && 
        pen.CreatePenIndirect(&logPen))
    {
        pOldBrush = dc.SelectObject(&brush);
        pOldPen = dc.SelectObject(&pen);
                
        CSize sz=m_size;
    
        CBitmap* pOldBitmap = dc.SelectObject(&bmShadow);
    
        dc.BitBlt(5,5,rc.Width(),rc.Height(), &dc, 5, 5, MERGECOPY);
    
        dc.SelectObject(pOldBitmap);
    //////////////////////////////////////////////////////////////////
        bmShadow.DeleteObject();
                
        if(pOldBrush) dc.SelectObject(pOldBrush);
        if(pOldPen) dc.SelectObject(pOldPen);
        brush.DeleteObject();
        pen.DeleteObject();
    }

    static LOGPEN logPen1 = {PS_SOLID, {0,0}, RGB(0,0,0)};
    static LOGBRUSH logBrush1 = {BS_SOLID,m_bkColor, 0};
    KwRectangle(dc,logPen1,logBrush1,rc);


////////////// TEXT /////////
    COLORREF oldTextColor = dc.GetTextColor();
    dc.SetTextColor(0L);
                    
    LOGFONT logFont;
    memset(&logFont, 0, sizeof(logFont));
    logFont.lfHeight = m_fontHeight;
    lstrcpy(logFont.lfFaceName, (const TCHAR*)m_fontName);
    logFont.lfCharSet = DEFAULT_CHARSET;

    CFont font, *pOldFont=NULL;
    if(!font.CreateFontIndirect(&logFont))
        TRACE("Could Not create font\n");
    else
        pOldFont = dc.SelectObject(&font);
    CPoint point(4,5);  // marginWidth marginHeight
    rc = CRect(point,m_maxSize);
    for(int i=0;i<m_cbLine;i++)
    {
        CString str = m_arMsg[i];
        dc.DrawText( (LPCTSTR)str,str.GetLength(), rc, DT_SINGLELINE|DT_LEFT|DT_VCENTER);
        rc += CPoint(0,m_maxSize.cy+3);
    }
    dc.SetTextColor(oldTextColor);
    dc.SetBkMode(oldBkMode);     //KDW
    if(pOldFont) dc.SelectObject(pOldFont);
    font.DeleteObject();
}

void KPopupHelp::CalcRect()
{                        
    for(int i=0;i<m_cbLine;i++)
        if(m_arSize[i])
            delete m_arSize[i];
    m_arSize.RemoveAll();

    KwCutByToken(s_msg,L"\r\n",m_arMsg);
    m_cbLine= m_arMsg.GetUpperBound()+1;
	 INT_PTR maxHeight=0;
	 INT_PTR maxWidth=0;

    CDC *pDC = GetDC();

// 크기를 알기 위해 폰트를 임시로 설정 한다.
    LOGFONT logFont;
    memset(&logFont, 0, sizeof(logFont));
    logFont.lfHeight = m_fontHeight;;
    logFont.lfWeight = FW_NORMAL;
    logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    lstrcpy(logFont.lfFaceName, (const TCHAR*)m_fontName);
    logFont.lfCharSet = DEFAULT_CHARSET;
    CFont font, *pOldFont=NULL;
    
    if(!font.CreateFontIndirect(&logFont))
        TRACE("Could Not create font\n");
    else
        pOldFont = pDC->SelectObject(&font);

    if(pDC == NULL)
        return;             
    for(int i=0;i<m_cbLine;i++)
    {
        CSize size = pDC->GetTextExtent( (LPCTSTR)m_arMsg[i], m_arMsg[i].GetLength());
        if(maxHeight < size.cy)
            maxHeight = size.cy;
        if(maxWidth < size.cx)
            maxWidth = size.cx;
        CSize *pS = new CSize;
        *pS = size;
        m_arSize.Add(pS);
    }

    m_maxSize = CSize((int)maxWidth, (int)maxHeight);// 제일 큰 라인..
    maxWidth += ((2*4) + 5); // ?bmWidth? + (2*margin) + shadow
    maxHeight = (maxHeight+3)*m_cbLine + (2*4) + 5; //(m_cbLine+line gab)*m_cbLine + (2*margin) + shadow)
    m_size = CSize((int)maxWidth, (int)maxHeight); // 전체.. 크기.
    
// 폰트를 해체 한다.
    if(pOldFont) pDC->SelectObject(pOldFont);
    font.DeleteObject();

    ReleaseDC(pDC);

    s_rc = CRect(s_point,m_size);
//  TRACE("1MoveWindow(rc)(%d,%d,%d,%d)\n",rc.left,rc.top,rc.right,rc.bottom);
    CWnd *pDwin = CWnd::GetDesktopWindow();    
    CRect drc;
    CPoint offset(0,0);
    pDwin->GetWindowRect(drc);
    if(drc.right < s_rc.right)
        offset.x = drc.right - s_rc.right;
    if(drc.bottom < s_rc.bottom)
        offset.y = drc.bottom - s_rc.bottom;
//  TRACE("rc(%d,%d,%d,%d) drc(%d,%d,%d,%d) offset(%d,%d)\n",
//      rc.left,rc.top,rc.right,rc.bottom,
//      drc.left,drc.top,drc.right,drc.bottom,offset.x,offset.y);
    s_rc += offset;
//  TRACE("3MoveWindow(rc)(%d,%d,%d,%d)\n",rc.left,rc.top,rc.right,rc.bottom);
	s_idTimerClose = SetTimer(4000,6000,NULL);
}


////////// WARNING ////////
// 이 함수는 부모 쪽에서 아무곳도 호출 되면 안된다.
// 자기 자신을 죽이지 말기..
BOOL KPopupHelp::ClosePopupHelp()
{                            
    if(c_pBalloonWnd)
	{	BOOL br = c_pBalloonWnd->DestroyWindow();
		delete c_pBalloonWnd;			// 970611
		c_pBalloonWnd = NULL;
		s_pParent = NULL;
		return br;
	}
    else 
        return FALSE;
}

void CALLBACK EXPORT KPopupHelp::PopupHelpProc(
            HWND hWnd,      //handle of CWnd that called SetTimer
            UINT nMsg,      //WM_TIMER
            UINT_PTR nIDEvent,  //timer identification
            DWORD dwTime    //system time
            )
{
//  TRACE("PopupHelpProc\n"); //(s_pParent(%u)==hWnd(%u) nIdEvent(%u)\n",(UINT)s_pParent->m_hWnd,(UINT)hWnd,nIDEvent);
    if(s_pParent)
        if(s_pParent->m_hWnd==hWnd)
        {
            if(nIDEvent != 4000) return;
            ::KillTimer(s_pParent->m_hWnd,s_idTimer);
            s_idTimer = 0;
            KPopupHelp::Balloon();
        }
}
void KPopupHelp::Balloon()
{   
//  TRACE("m_mapIDC_IDS.GetSize() == %d\n",((KDialog*)s_pParent)->m_mapIDC_IDS.GetSize());
    if(((KDialog*)s_pParent)->m_mapIDC_IDS.GetSize() == 0) 
        return;
    int idc,ids;
    for(int i=0;i<((KDialog*)s_pParent)->m_mapIDC_IDS.GetSize();i++)
    {
        ids = HIWORD(((KDialog*)s_pParent)->m_mapIDC_IDS.GetAt(i));
        idc = LOWORD(((KDialog*)s_pParent)->m_mapIDC_IDS.GetAt(i));
//		TRACE("i(%d) : idc(%u==%u)-ids(%u)\n",i,idc,KPopupHelp::s_id,ids);
        if(idc == KPopupHelp::s_id) // 최근에 다룬 ctrl의 ID
        {
			TRACE("Found s_id(%u) idc(%u)-ids(%u)\n",s_id,idc,ids);
            KPopupHelp::s_id = -1;
            Balloon(idc,ids,s_pParent);
            return;
        }
    }
}

void KPopupHelp::Balloon(UINT idc,UINT ids,CWnd* pParentWnd /*= NULL*/)
{
    if (c_pBalloonWnd != NULL)
        return;

    CString msg;
    msg.LoadString(ids);
    KPopupHelp::s_msg = msg;
    
    c_pBalloonWnd = new KPopupHelp;              
    if (!c_pBalloonWnd->Create(pParentWnd))
    {   delete c_pBalloonWnd;
        c_pBalloonWnd = NULL;
    }
    else
    {
        c_pBalloonWnd->CalcRect();
		s_pParent = pParentWnd;
//      c_pBalloonWnd->ShowWindow(SW_SHOWNA);
        POINT pt;
        ::GetCursorPos(&pt);
        pt.x += 20;
        pt.y += 20;
        if(s_point == CPoint(pt))
        {   
            c_pBalloonWnd->MoveWindow(s_rc);
            c_pBalloonWnd->ShowWindow(SW_SHOWNA);
        }
        else
            ClosePopupHelp();
    }
}

void KPopupHelp::OnLButtonDown(UINT nFlags, CPoint point)
{

//  CWnd::OnLButtonDown(nFlags, point);
    ClosePopupHelp();   
}
void KPopupHelp::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
//  CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
    ClosePopupHelp();   
}


void KPopupHelp::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent==4000 && s_idTimerClose)
	{	KillTimer(s_idTimerClose);
		s_idTimerClose = 0;
		CWnd::OnTimer(nIDEvent);
		ClosePopupHelp();
	}
	else
		CWnd::OnTimer(nIDEvent);
}




void KPopupHelp::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	CWnd::OnWindowPosChanging(lpwndpos);
	TRACE("OnWindowPosChanging(lpwndpos);\n");
//	KPopupHelp::ClosePopupHelp();
	
}









IMPLEMENT_DYNCREATE(KEditBox, CFrameWnd)

BEGIN_MESSAGE_MAP(KEditBox, CFrameWnd)
	//{{AFX_MSG_MAP(KEditBox)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_MOVE()
	//ON_MESSAGE(WM_COMMAND, &KEditBox::OnCommand)
	ON_WM_SYSCOMMAND()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

KEditBox::KEditBox()
{
	m_Ctrl = NULL;
	m_parent = NULL;
}

KEditBox::~KEditBox()
{
}
void KEditBox::DoDataExchange(CDataExchange* pDX)
{
	CFrameWnd::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(KEditBox)
	//}}AFX_DATA_MAP
	//if(m_parent)m_parent->DoDataExchange(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// KEditBox message handlers

BOOL KEditBox::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style &= ~(LONG)(FWS_ADDTOTITLE|WS_SYSMENU);
	cs.style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	return CFrameWnd::PreCreateWindow(cs);
}

void KEditBox::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	CRect wrc;
	if(m_Ctrl)
	{
		CRect rc2;
		if(nType==SIZE_RESTORED || nType==SIZE_MAXIMIZED)
		{	rc2 = CRect(CPoint(0,0),CSize(cx,cy));
			m_Ctrl->MoveWindow(rc2);
		}
		else if(nType == SIZE_MINIMIZED)
		{
		}
	}
}

int KEditBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}

void KEditBox::OnMove(int x, int y)
{
	CFrameWnd::OnMove(x, y);
//	CRect rc;
//	GetClientRect(rc);
//	OnSize(	SIZE_RESTORED,rc.Width(),rc.Height());
}
BOOL KEditBox::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(m_parent)
		return m_parent->SendMessage(WM_COMMAND,wParam, lParam) == 1L;
	else return FALSE;
}

  
void KEditBox::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch(nID)
	{
		case SC_MINIMIZE:
		{	TRACE("SC_MINIMIZE\n");
			CRect rcDlg;
			if(m_Ctrl)
			{
				m_parent->GetWindowRect(rcDlg);
				CRect rcf = CRect(CPoint(rcDlg.left+m_rcOrg.left,rcDlg.top+m_rcOrg.top),CSize(m_rcOrg.Width(),m_rcOrg.Height()));
				ShowWindow(SW_RESTORE);
				MoveWindow(m_rcOrg+rcDlg.TopLeft());
		
				CRect rc2;
				GetClientRect(rc2);
				m_Ctrl->MoveWindow(rc2);
			}
			return;
		}
		case SC_MAXIMIZE:
			TRACE("SC_MAXIMIZE\n");
			break;
	}
	
	CFrameWnd::OnSysCommand(nID, lParam);
}
void KEditBox::OnSetFocus(CWnd* pOldWnd)
{
	CFrameWnd::OnSetFocus(pOldWnd);
	
	if(m_Ctrl)
		m_Ctrl->SetFocus();	
}

void KEditBox::OnClose()
{
	return;
	
	CFrameWnd::OnClose();
}



void KEditBox::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if(nChar == VK_ESCAPE)
	{	((KEdit*)m_Ctrl)->FrameEdit(0);
	}
	CFrameWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}






















IMPLEMENT_DYNCREATE(KEdit, CEdit)

BEGIN_MESSAGE_MAP(KEdit, CEdit)
	//{{AFX_MSG_MAP(KNewDlg)
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void KEdit::SetCaption(LPCSTR title)
{
	m_caption = title;
	if(m_pFrame)
		m_pFrame->SetWindowText(m_caption);
}



void KEdit::FrameEdit(BOOL bBoot/*=TRUE*/,LPCSTR title)
{
	if(bBoot)
	{

		if(m_bBoot)
		{	m_pFrame->ShowWindow(SW_RESTORE);
			return;
		}
		CRect rc,crc;
		GetWindowRect(rc);
		
		CWnd *pDlg = GetParent();
		BOOL b;
		if(m_pFrame == NULL)
		{	m_pFrame = new KEditBox();
			CRect rcDlg;
			pDlg->GetWindowRect(rcDlg);
			CPoint ptOff(rc.left-rcDlg.left,rc.top-rcDlg.top);
			
			m_pFrame->m_rcOrg = CRect(ptOff,CSize(rc.Width(),rc.Height()));
			m_rcClient = rc;
			pDlg->ScreenToClient(m_rcClient);
			
			if (title) m_caption = title;
			else m_caption = "Sheet";
			ASSERT(pDlg);
			b = m_pFrame->Create(NULL,
						(LPCTSTR)m_caption,
						WS_OVERLAPPEDWINDOW,//|WS_VISIBLE,
						rc,//+rcDlg.TopLeft(),
						pDlg);
			m_pFrame->m_parent = pDlg;
			ASSERT(b);
		}
		if(b && m_pFrame) 
		{
			m_pWndFocus = CWnd::GetFocus();
			if(m_pWndFocus)
			{
				if(m_pWndFocus->m_hWnd == m_hWnd)
					m_pWndFocus = pDlg->GetNextDlgTabItem(this);
			}
			SetParent(m_pFrame);
			m_pFrame->ShowWindow(SW_RESTORE);
			m_pFrame->m_id = GetDlgCtrlID();
			m_pFrame->m_Ctrl = this;
			SetFocus();
			m_pFrame->GetClientRect(rc);
			MoveWindow(CRect(CPoint(0,0),CSize(rc.Width(),rc.Height())));
			m_bBoot = TRUE;
		}
	}
    else
    {
		if(m_pFrame && m_pFrame->m_parent)
		{
			m_pFrame->ShowWindow(SW_HIDE);
			SetParent(m_pFrame->m_parent);
			MoveWindow(m_rcClient);//rc);
			m_bBoot = FALSE;
		}
	}
}

void KEdit::FollowMove(CPoint pt)
{
	CRect rcDlg,rcFrame;
	CPoint offset;
	if(m_pFrame)
	{
		offset = pt - CSize(m_oldpt);
		m_pFrame->GetWindowRect(rcFrame);
		rcFrame += offset;
		m_pFrame->MoveWindow(rcFrame);
	}
	m_oldpt = pt;
}

void KEdit::FollowMove(int offx,int offy)
{	FollowMove(CPoint(offx,offy));
}


void KEdit::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if(nChar == VK_ESCAPE)
	{	FrameEdit(0);
		//m_pFrame->m_parent->SetFocus();
	}
	CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
}






