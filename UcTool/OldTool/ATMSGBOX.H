#ifndef _ATMSGBOX_H
#define _ATMSGBOX_H

#include "../UcExport.inl"

class KAutoMsgBox : public CFrameWnd
{
	DECLARE_DYNAMIC(KAutoMsgBox)
public:
	KAutoMsgBox();
	~KAutoMsgBox();    
	CStatic m_static;
	CRect m_rc,m_rcTxt;
	LOGBRUSH m_brush;
	LOGFONT  m_font;
	CString GetCurText() { return m_msg;}
	
	CString m_msg;
	UINT m_styleIcon;
	UINT_PTR m_timerID;
	int m_period;
	CWnd *m_pParent;	

	BOOL Create(CWnd* pParentWnd=NULL);
	//{{AFX_VIRTUAL(KLogoWindow)
	protected:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
// Generated message map functions
protected:
	//{{AFX_MSG(KAutoMsgBox)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
		
};
/*
class KAutoMsgBox : public CToolBar
{
	DECLARE_DYNAMIC(KAutoMsgBox)
public:
	KAutoMsgBox();
	~KAutoMsgBox();    
	CStatic m_static;
	CRect m_rc,m_rcTxt;
	LOGBRUSH m_brush;
	LOGFONT  m_font;
	CString GetCurText() { return m_msg;}
	
	CString m_msg;
	UINT m_styleIcon;
	UINT m_timerID;
	int m_period;
	CWnd *m_pParent;	
	virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
	virtual BOOL Create(CWnd* pParentWnd);
// Generated message map functions
protected:
	//{{AFX_MSG(KAutoMsgBox)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
		
};
*/

UCTOOLDYNAMIC void KwAutoMessageBox(CWnd *pParent,CString str,int time=1500,UINT styleIcon=MB_ICONINFORMATION,int x=-1,int y=-1);
UCTOOLDYNAMIC void KwAtMsgPrintf(CWnd *pParent, LPCTSTR lpFormat, ... );



#endif
