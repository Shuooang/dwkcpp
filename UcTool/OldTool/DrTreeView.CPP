// MyTreeCtrl.cpp : implementation file
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

#include "pch.h"
#include "Ctrlext.h"
#include "DrTreeView.h"
//#include "treecpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define TC ((CTreeCtrl&)GetTreeCtrl())
#define TC ((KTreeCtrlEx&)GetTreeCtrl())

/////////////////////////////////////////////////////////////////////////////
// KDrTreeView
IMPLEMENT_DYNCREATE(KDrTreeView,CTreeView)

KDrTreeView::KDrTreeView()
{
	m_bDragging = FALSE;
	m_pimagelist = NULL;
}

KDrTreeView::~KDrTreeView()
{
}


BEGIN_MESSAGE_MAP(KDrTreeView, CTreeView)
	//{{AFX_MSG_MAP(KDrTreeView)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINRDRAG, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_USER_BEGININVOKE, KDrTreeView::OnBeginInvoke)//?beginInvoke 2
END_MESSAGE_MAP()


OnBeginInvoke_Define(KDrTreeView)//?beginInvoke 3

/////////////////////////////////////////////////////////////////////////////
// KDrTreeView message handlers
void KDrTreeView::OnDestroy()
{
	CImageList	*pimagelist;

	pimagelist = TC.GetImageList(TVSIL_NORMAL);
	if(pimagelist)
	{
		pimagelist->DeleteImageList();
//		delete pimagelist;
	}
}

void KDrTreeView::SetNewStyle(long lStyleMask, BOOL bSetBits)
{
	long		lStyleOld;

	lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyleOld &= ~lStyleMask;
	if (bSetBits)
		lStyleOld |= lStyleMask;

	SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

void KDrTreeView::OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult)
{
	TV_DISPINFO		*ptvinfo;

//	((CTreeViewPage *)GetParent())->ShowNotification(pnmhdr, pLResult);
	ptvinfo = (TV_DISPINFO *)pnmhdr;
	if (ptvinfo->item.pszText != NULL)
	{
		ptvinfo->item.mask = TVIF_TEXT;
		TC.SetItem(&ptvinfo->item);
	}
	*pLResult = TRUE;
}

void KDrTreeView::OnMouseMove(UINT nFlags, CPoint point)
{
	HTREEITEM			hitem;
	UINT				flags;

	if (m_bDragging)
	{
		m_pimagelist->DragMove(point);
		if ((hitem = TC.HitTest(point, &flags)) != NULL)
		{
			m_pimagelist->DragLeave(this);
			TC.SelectDropTarget(hitem);
			m_hitemDrop = hitem;
			m_pimagelist->DragEnter(this, point);
			CString sDataDrop = TC.GetItemText(m_hitemDrop);
			TRACE(L"(%4d,%4d) %s/\n",point.x,point.y,(PS)sDataDrop);
		}
	}

	CTreeView::OnMouseMove(nFlags, point);
}

BOOL KDrTreeView::IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent)
{
	do
	{
		if (hitemChild == hitemSuspectedParent)
			break;
	}
	while ((hitemChild = TC.GetParentItem(hitemChild)) != NULL);

	return (hitemChild != NULL);
}


BOOL KDrTreeView::TransferItem(HTREEITEM hitemDrag, HTREEITEM hitemDrop)
{
	TV_INSERTSTRUCT		tvstruct;
	TCHAR				sztBuffer[50];
	HTREEITEM			hNewItem, hFirstChild;

		// avoid an infinite recursion situation
	tvstruct.item.hItem = hitemDrag;
	tvstruct.item.cchTextMax = 49;
	tvstruct.item.pszText = sztBuffer;
	tvstruct.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	TC.GetItem(&tvstruct.item);  // get information of the dragged element
	tvstruct.hParent = hitemDrop;
	tvstruct.hInsertAfter = TVI_SORT;
	tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	hNewItem = TC.InsertItem(&tvstruct);

	while ((hFirstChild = TC.GetChildItem(hitemDrag)) != NULL)
	{
		TransferItem(hFirstChild, hNewItem);  // recursively transfer all the items
		TC.DeleteItem(hFirstChild);		// delete the first child and all its children
	}
	return TRUE;
}

void KDrTreeView::OnButtonUp()
{
	if (m_bDragging)
	{
		m_pimagelist->DragLeave(this);
		m_pimagelist->EndDrag();
		if (m_hitemDrag != m_hitemDrop && 
			!IsChildNodeOf(m_hitemDrop, m_hitemDrag) && 
			TC.GetParentItem(m_hitemDrag) != m_hitemDrop)
		{
			TransferItem(m_hitemDrag, m_hitemDrop);
			CString sDrag=TC.GetFullItemText(m_hitemDrag);
			CString sDrop=TC.GetFullItemText(m_hitemDrop);
			TRACE("%s -> %s\n",(const char*)m_hitemDrag,(const char*)m_hitemDrop);
			TC.DeleteItem(m_hitemDrag);
		}
		else
			MessageBeep(0);

		ReleaseCapture();
		m_bDragging = FALSE;
		TC.SelectDropTarget(NULL);
	}
}

void KDrTreeView::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp();
	CTreeView::OnLButtonUp(nFlags, point);
}

void KDrTreeView::OnRButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp();
	CTreeView::OnRButtonUp(nFlags, point);
}

void KDrTreeView::OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pLResult)
{
	CPoint		ptAction;
	UINT		nFlags;

	GetCursorPos(&ptAction);
	ScreenToClient(&ptAction);
//	((CTreeViewPage *)GetParent())->ShowNotification(pnmhdr, pLResult);
	ASSERT(!m_bDragging);
	m_bDragging = TRUE;
	m_hitemDrag = TC.HitTest(ptAction, &nFlags);
	m_hitemDrop = NULL;
	m_pimagelist = TC.CreateDragImage(m_hitemDrag);  // get the image list for dragging
	m_pimagelist->DragShowNolock(TRUE);
	m_pimagelist->SetDragCursorImage(0, CPoint(0, 0));
	m_pimagelist->BeginDrag(0, CPoint(0,0));
	m_pimagelist->DragMove(ptAction);
	m_pimagelist->DragEnter(this, ptAction);
	SetCapture();
}








/////////////////////////////////////////////////////////////////////////














/////////////////////////////////////////////////////////////////////////////
// KDrTreeCtrl

KDrTreeCtrl::KDrTreeCtrl()
{
	m_bDragging = FALSE;
	m_pimagelist = NULL;
}

KDrTreeCtrl::~KDrTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(KDrTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(KDrTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINRDRAG, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// KDrTreeCtrl message handlers
void KDrTreeCtrl::OnDestroy()
{
	CImageList	*pimagelist;

	pimagelist = GetImageList(TVSIL_NORMAL);
	if(pimagelist)
	{
		pimagelist->DeleteImageList();
//		delete pimagelist;
	}
}

void KDrTreeCtrl::SetNewStyle(long lStyleMask, BOOL bSetBits)
{
	long		lStyleOld;

	lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyleOld &= ~lStyleMask;
	if (bSetBits)
		lStyleOld |= lStyleMask;

	SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

void KDrTreeCtrl::OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult)
{
	TV_DISPINFO		*ptvinfo;

//	((CTreeCtrlPage *)GetParent())->ShowNotification(pnmhdr, pLResult);
	ptvinfo = (TV_DISPINFO *)pnmhdr;
	if (ptvinfo->item.pszText != NULL)
	{
		ptvinfo->item.mask = TVIF_TEXT;
		SetItem(&ptvinfo->item);
	}
	*pLResult = TRUE;
}

void KDrTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	HTREEITEM			hitem;
	UINT				flags;

	if (m_bDragging)
	{
		m_pimagelist->DragMove(point);
		if ((hitem = HitTest(point, &flags)) != NULL)
		{
			m_pimagelist->DragLeave(this);
			SelectDropTarget(hitem);
			m_hitemDrop = hitem;
			m_pimagelist->DragEnter(this, point);
		}
	}

	CTreeCtrl::OnMouseMove(nFlags, point);
}

BOOL KDrTreeCtrl::IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent)
{
	do
	{
		if (hitemChild == hitemSuspectedParent)
			break;
	}
	while ((hitemChild = GetParentItem(hitemChild)) != NULL);

	return (hitemChild != NULL);
}


BOOL KDrTreeCtrl::TransferItem(HTREEITEM hitemDrag, HTREEITEM hitemDrop)
{
	TV_INSERTSTRUCT		tvstruct;
	TCHAR				sztBuffer[50];
	HTREEITEM			hNewItem, hFirstChild;

		// avoid an infinite recursion situation
	tvstruct.item.hItem = hitemDrag;
	tvstruct.item.cchTextMax = 49;
	tvstruct.item.pszText = sztBuffer;
	tvstruct.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	GetItem(&tvstruct.item);  // get information of the dragged element
	tvstruct.hParent = hitemDrop;
	tvstruct.hInsertAfter = TVI_SORT;
	tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	hNewItem = InsertItem(&tvstruct);

	while ((hFirstChild = GetChildItem(hitemDrag)) != NULL)
	{
		TransferItem(hFirstChild, hNewItem);  // recursively transfer all the items
		DeleteItem(hFirstChild);		// delete the first child and all its children
	}
	return TRUE;
}

void KDrTreeCtrl::OnButtonUp()
{
	if (m_bDragging)
	{
		m_pimagelist->DragLeave(this);
		m_pimagelist->EndDrag();
		if (m_hitemDrag != m_hitemDrop && !IsChildNodeOf(m_hitemDrop, m_hitemDrag) && 
															GetParentItem(m_hitemDrag) != m_hitemDrop)
		{
			TransferItem(m_hitemDrag, m_hitemDrop);
			DeleteItem(m_hitemDrag);
		}
		else
			MessageBeep(0);

		ReleaseCapture();
		m_bDragging = FALSE;
		SelectDropTarget(NULL);
	}
}

void KDrTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp();
	CTreeCtrl::OnLButtonUp(nFlags, point);
}

void KDrTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp();
	CTreeCtrl::OnRButtonUp(nFlags, point);
}

void KDrTreeCtrl::OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pLResult)
{
	CPoint		ptAction;
	UINT		nFlags;

	GetCursorPos(&ptAction);
	ScreenToClient(&ptAction);
//	((CTreeCtrlPage *)GetParent())->ShowNotification(pnmhdr, pLResult);
	ASSERT(!m_bDragging);
	m_bDragging = TRUE;
	m_hitemDrag = HitTest(ptAction, &nFlags);
	m_hitemDrop = NULL;
	m_pimagelist = CreateDragImage(m_hitemDrag);  // get the image list for dragging
	m_pimagelist->DragShowNolock(TRUE);
	m_pimagelist->SetDragCursorImage(0, CPoint(0, 0));
	m_pimagelist->BeginDrag(0, CPoint(0,0));
	m_pimagelist->DragMove(ptAction);
	m_pimagelist->DragEnter(this, ptAction);
	SetCapture();
}
