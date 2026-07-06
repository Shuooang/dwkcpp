// MyTreeCtrl.h : header file
//

#ifndef INC_DRTREECTRL_H
#define INC_DRTREECTRL_H


#include "Ctrlext.h"
#include "../UcExport.inl"
/////////////////////////////////////////////////////////////////////////////
// KDrTreeView window

class UCTOOLDYNAMIC KDrTreeView : public CTreeView
{
// Construction
public:
	KDrTreeView();
	DECLARE_DYNCREATE(KDrTreeView)
// Attributes
public:
	BOOL		m_bDragging;
	HTREEITEM	m_hitemDrag;
	HTREEITEM	m_hitemDrop;
	CImageList	*m_pimagelist;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(KDrTreeView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~KDrTreeView();
	void	SetNewStyle(long lStyleMask, BOOL bSetBits);
	BOOL	TransferItem(HTREEITEM hitem, HTREEITEM hNewParent);
	void	OnButtonUp(void);
	BOOL	IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent);
	int	GetSeletedItem();


	// Generated message map functions
protected:
	//{{AFX_MSG(KDrTreeView)
	afx_msg void OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LRESULT OnBeginInvoke(WPARAM wParam, LPARAM lParam);//?beginInvoke 1

	void OnButtonUp(CPoint point);

	DECLARE_MESSAGE_MAP()
};







/////////////////////////////////////////////////////////////////////////////
// KDrTreeCtrl window

class UCTOOLDYNAMIC KDrTreeCtrl : public KTreeCtrlEx
{
// Construction
public:
	KDrTreeCtrl();

// Attributes
public:
	BOOL		m_bDragging;
	HTREEITEM	m_hitemDrag;
	HTREEITEM	m_hitemDrop;
	CImageList	*m_pimagelist;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(KDrTreeCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~KDrTreeCtrl();
	void	SetNewStyle(long lStyleMask, BOOL bSetBits);
	BOOL	TransferItem(HTREEITEM hitem, HTREEITEM hNewParent);
	void	OnButtonUp(void);
	BOOL	IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent);

	// Generated message map functions
protected:
	//{{AFX_MSG(KDrTreeCtrl)
	afx_msg void OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	void OnButtonUp(CPoint point);

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////

#endif
