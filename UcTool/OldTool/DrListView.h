// MyListCtrl.h : header file
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// KDrListView window
#ifndef INC_MYLISTCTRL_H
#define INC_MYLISTCTRL_H

#include <afxcview.h>
#include "../UcExport.inl"

class UCTOOLDYNAMIC KDrListView : public CListView
{
// Construction
public:
	KDrListView();
	DECLARE_DYNCREATE(KDrListView)

// Attributes
public:

	BOOL			m_bDragging;
	int				m_iItemDrag;
	int				m_iItemDrop;
	CPoint			m_ptHotSpot;
	CPoint			m_ptOrigin;
	CSize			m_sizeDelta;
	CImageList		*m_pimagelist;

	// multi selection
	CObArray m_arImageList;
	CWordArray m_arSelItem;

// Operations
public:
//	BOOL m_bDownMove;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(KDrListView)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~KDrListView();
	void	OnButtonUp(CPoint point);

	// Generated message map functions
protected:
	//{{AFX_MSG(KDrListView)
	afx_msg void OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pResult);
	afx_msg void OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};




















/////////////////////////////////////////////////////////////////////////////
#endif

