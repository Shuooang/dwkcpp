/************** <1> sub class방식 ****************

    <하나>. CDialog에서 계승 받은 class의 base class명을 CDialog -> KDialog로 모두 바꾼다.
    
    class KDlg : public KDialog             /// 1 ///
    {
    // Construction
    
    
    <둘>. 적당한 위치 에 header file을 포함 시킨다.
    
    #include "kdialog32.h"                            /// 2 ///
    
    
    <셋>. Control 갯수 만큼 resource의 string table에서 각 control id와 연관 시켜 추가 해준다.
    BOOL KDlg::OnInitDialog()
    {
        KDialog::OnInitDialog();
    
        m_mapIDC_IDS.Add(MAKELONG((UINT)IDC_EDIT1,(UINT)IDC_EDIT1));
        
        // macro를 이용 하면 편하다.
        POPUPID(IDC_EDIT1,  IDS_EDIT1);             /// 3 ///
        POPUPID(IDC_COMBO1, IDS_COMBO1);
        POPUPID(IDC_LIST1,  IDS_LIST1);
        POPUPID(IDOK,       IDS_OK);
        POPUPID(IDCANCEL,   IDS_CANCEL);
            :
            :
    
        return TRUE;  // return TRUE  unless you set the focus to a control
    }
    
    <넷>. 마지막으로 \kinclude\kdialog.cpp를 함께 링크 시켜 주면 된다.
    




    *************** <2> 소스삽입 하는 방식  ****************************
    *   derive 받지 않고 CDialog에서 계승 받아서 그냥 쓰는 경우
    
    1. Resource의 control id와 string id를 저장할 배열을 선언 한다.(아래 /// 1 ///)
    2. Popup Help가 필요한 Dialog class에서 
        ClassWizard로 WM_SETCURSOR message를 추가 한다. (아래 /// 2 ///)
    
        
    public:     //꼭 public 변수 이어야 한다.
        CDWordArray m_mapIDC_IDS;                           /// 1 ///
        
        //{{AFX_MSG(KNewDlg)
          ....
    
        afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);  /// 2 ///
    
          ....
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
    
    3. Dialog 본체 파일에 header file을 포함 시킨다.
    
    #include "kdialog32.h"                            /// 3 ///
    
    
    2. Message Map 에 WM_SETCURSOR가 포함된다.
    
    BEGIN_MESSAGE_MAP(KNewDlg, CDialog)
        //{{AFX_MSG_MAP(KNewDlg)
        ON_WM_SETCURSOR()                           /// 2 ///
        //}}AFX_MSG_MAP
    END_MESSAGE_MAP()
    
    
    4. Control 갯수 만큼 resource의 string table에서 각 control id와 연관 시켜 추가 해준다.
    BOOL KDlg::OnInitDialog()
    {
        CDialog::OnInitDialog();
    
        m_mapIDC_IDS.Add(MAKELONG((UINT)IDC_EDIT1,(UINT)IDC_EDIT1));
            
        // macro를 이용 하면 편하다.
        POPUPID(IDC_EDIT1,  IDS_EDIT1);             /// 4 ///
        POPUPID(IDC_COMBO1, IDS_COMBO1);
        POPUPID(IDC_LIST1,  IDS_LIST1);
        POPUPID(IDOK,       IDS_OK);
        POPUPID(IDCANCEL,   IDS_CANCEL);
            :
            :
    
        return TRUE;  // return TRUE  unless you set the focus to a control
    }
    
    5. WM_SETCURSOR 메세지 함수 에 메인 작업을 추가 한다.
    BOOL KDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
    {
        READY_POPUPHELP();                  /// 5 ///
    
        return CDialog::OnSetCursor(pWnd, nHitTest, message);
    }
    
    
    6. 마지막으로 kdialog.cpp를 함께 링크 시켜 주면 된다.       /// 6 ///

*/




#ifndef _POPUPHELP_H_//[
#define _POPUPHELP_H_

#include "../UcExport.inl"

// kdialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// KDialog dialog

class UCTOOLDYNAMIC KDialog : public CDialog
{
// Construction
public:
    KDialog(CWnd* pParent = NULL);  // standard constructor
    KDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
    CDWordArray m_mapIDC_IDS;
	void AddHelpItem(UINT idc,UINT ids);
// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(KDialog)
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
 	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDestroy();
	afx_msg void OnKillFocus(CWnd* pNewWnd);	// 970611
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
   //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


#ifndef _DEBUG//[
inline void KDialog::AddHelpItem(UINT idc,UINT ids)
{
	m_mapIDC_IDS.Add((unsigned long)MAKELONG((UINT)idc,(UINT)ids));
}
#endif//]









// PopupHelp.h : header file
//
#define POPUPID(idc,ids)    AddHelpItem(idc,ids)
//    m_mapIDC_IDS.Add((unsigned long)MAKELONG((UINT)idc,(UINT)ids));

#define READY_POPUPHELP()   \
    static int s_lid=-1;\
    int id = pWnd->GetDlgCtrlID();\
    static CPoint s_pt;\
    POINT pt;\
    ::GetCursorPos(&pt);\
    pt.x += 20;\
    pt.y += 20;\
    if(KPopupHelp::s_point != pt)\
    {   KillTimer(KPopupHelp::s_idTimer);\
        KPopupHelp::s_idTimer=0;\
        KPopupHelp::ClosePopupHelp();\
        CWnd *pMainWnd = AfxGetMainWnd();\
        CWnd *pActive=GetActiveWindow();\
        CWnd *pFocus=GetFocus();\
        CWnd *pDeskWnd = GetDesktopWindow();\
        BOOL bOK=FALSE;\
        for(CWnd *pW=pWnd;pW && pW->m_hWnd != pDeskWnd->m_hWnd;pW = pW->GetParent())\
        {   if((pFocus &&pW->m_hWnd == pFocus->m_hWnd) || (pActive && pW->m_hWnd == pActive->m_hWnd))\
            {   bOK=TRUE;\
                break;\
            }\
        }\
        if(id > 0 && bOK)\
        {   KPopupHelp::ReadyPopupHelp(id,this);\
            KPopupHelp::s_idTimer = SetTimer(4000,1000,KPopupHelp::PopupHelpProc);\
        }\
        s_lid = id;\
        KPopupHelp::s_point = pt;\
    }

/////////////////////////////////////////////////////////////////////////////
//   PopupHelp Screen class

class UCTOOLDYNAMIC KPopupHelp : public CWnd
{
// Construction
protected:
    KPopupHelp();
                                     
// Attributes:
public:
    CBitmap m_bitmap;

    CPtrArray m_arSize;
    INT_PTR m_cbLine;
    CSize m_size; // 전체 크기
    CSize m_maxSize;    // 가장 큰 문장
    
    static CRect s_rc;
    static CPoint s_point;  // window position
    static CString s_msg;
    static CWnd *s_pParent;
    static int s_id;

    CStringArray m_arMsg;
    COLORREF m_bkColor;
    int m_fontHeight;
    CString m_fontName;
    BOOL m_bBmp;
    

// Operations
public:
    void CalcRect();

    static void Balloon();
    static void Balloon(UINT idc,UINT ids,CWnd* pParentWnd = NULL);
    static void CALLBACK EXPORT PopupHelpProc(HWND hWnd,UINT nMsg,UINT_PTR nIDEvent,DWORD dwTime);
    static void ReadyPopupHelp(UINT idc,CWnd* pParentWnd = NULL);
    static BOOL ClosePopupHelp();

    static UINT_PTR s_idTimer;
    static UINT_PTR s_idTimerClose;




// Overrides
//  virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(KPopupHelp)
    //}}AFX_VIRTUAL

// Implementation
public:
    ~KPopupHelp();
    virtual void PostNcDestroy();

protected:
    BOOL Create(CWnd* pParentWnd = NULL);
//  void HideBalloonScreen();
//  static BOOL c_bShowBalloonWnd;
    static KPopupHelp* c_pBalloonWnd;

// Generated message map functions
protected:
    //{{AFX_MSG(KPopupHelp)
//  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point) ;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
  //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



inline void KPopupHelp::ReadyPopupHelp(UINT idc,CWnd* pParentWnd)
{
    s_id=idc;
    s_pParent = pParentWnd;
}



class UCTOOLDYNAMIC KEditBox : public CFrameWnd
{
	DECLARE_DYNCREATE(KEditBox)
protected:
public:
	KEditBox();			// protected constructor used by dynamic creation

	void SetCtrl(CWnd *ctrl){m_Ctrl = ctrl;}
	CWnd *m_Ctrl;

	// Attributes
public:
	CRect m_rcOrg;
	UINT m_id;
	CWnd *m_parent;
	// Operations
public:

	// Implementation
	virtual ~KEditBox();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(KEditBox)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMove(int x, int y);
	//afx_msg BOOL OnCommandx(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	afx_msg void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};





class UCTOOLDYNAMIC KEdit : public CEdit
{
	DECLARE_DYNCREATE(KEdit)
public:
	KEdit()
	{
		m_pWndFocus=m_pFrame=NULL;
		m_oldpt = CPoint(32000,32000);
		m_bBoot = FALSE;
	};
	virtual ~KEdit(){};

	KEditBox *m_pFrame;
	CPoint m_oldpt;
	CRect m_rcClient;
	CWnd *m_pWndFocus;

	void PutItOnDlg()
	{
		if(m_pFrame && m_pFrame->m_parent && m_bBoot)
		{	ShowWindow(SW_HIDE);
			SetParent(m_pFrame->m_parent);
			m_bBoot=FALSE;
		}
	}
	void BootItUpDlg()
	{
		if(m_pFrame && m_pFrame->m_parent && !m_bBoot)
		{	SetParent(m_pFrame);
			ShowWindow(SW_SHOW);
			m_bBoot=TRUE;
		}
	}
	void FrameEdit(BOOL bFrame=TRUE,LPCSTR title=NULL)                ;
	void FollowMove(CPoint pt)        ;
	void FollowMove(int offx,int offy) ;
	void SetCaption(LPCSTR title);
	CString m_caption;
	
	
	BOOL m_bBoot;
	void ShowFrameSheet(BOOL bShow=TRUE)
	{
		if(m_pFrame && m_bBoot)
			m_pFrame->ShowWindow(bShow?SW_SHOW:SW_HIDE);
	}
	void CheckKey(LPVOID data)
	{
		if(m_pFrame && m_pFrame->m_parent && m_bBoot)
		{	//int KeyCode, Shift;
//			KeyCode = AFX_NUM_EVENTPARAMINDEX (int, data, 1);
//			Shift = AFX_NUM_EVENTPARAMINDEX (int, data, 0);
//			if((KeyCode==18 && Shift==4) || (KeyCode==9 && Shift==0))
//				if(m_pWndFocus)
//					m_pWndFocus->SetFocus();
		}
	}
	//{{AFX_MSG(KEditBox)
	afx_msg void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};






#endif//]
