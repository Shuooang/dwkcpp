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

// MyListCtrl.cpp : implementation file
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
#include "DrListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LC ((KListCtrlEx&)GetListCtrl())

/////////////////////////////////////////////////////////////////////////////
// KDrListView
IMPLEMENT_DYNCREATE(KDrListView,CListView)
KDrListView::KDrListView()
{
	m_bDragging = FALSE;
	m_pimagelist = NULL;
//	m_bDownMove=FALSE;
}

KDrListView::~KDrListView()
{
	if(m_pimagelist)
	{
		delete m_pimagelist;
		m_pimagelist = NULL;
	}
}


BEGIN_MESSAGE_MAP(KDrListView, CListView)
	//{{AFX_MSG_MAP(KDrListView)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(LVN_BEGINRDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// KDrListView message handlers

void KDrListView::OnMouseMove(UINT nFlags, CPoint point)
{
	long		lStyle;
	int			iItem;
	LV_ITEM		lvitem;

	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= LVS_TYPEMASK;  // drag will do different things in list and report mode
	if (m_bDragging)
	{
		m_pimagelist->DragMove(point - m_sizeDelta);  // move the image
		if ((iItem = LC.HitTest(point)) != -1)
		{
			m_iItemDrop = iItem;
			m_pimagelist->DragLeave(this); // unlock the window and hide drag image
			if (lStyle == LVS_REPORT || lStyle == LVS_LIST)
			{
				lvitem.iItem = iItem;
				lvitem.iSubItem = 0;
				lvitem.mask = LVIF_STATE;
				lvitem.stateMask = LVIS_DROPHILITED;  // highlight the drop target
				LC.SetItem(&lvitem);
			}

			point -= m_sizeDelta;
			m_pimagelist->DragEnter(this, point);  // lock updates and show drag image
		}
	}

	CListView::OnMouseMove(nFlags, point);
}
/*
void KDrListView::OnMouseMove(UINT nFlags, CPoint point)
{
	long		lStyle;
	int			iItem;
	LV_ITEM		lvitem;

	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= LVS_TYPEMASK;  // drag will do different things in list and report mode
	if (m_bDragging)
	{
		for(int i=0;i<=m_arImageList.GetUpperBound();i++)
			((CImageList*)m_arImageList[i])->DragMove(point - m_sizeDelta);
//		m_pimagelist->DragMove(point - m_sizeDelta);  // move the image
		if ((iItem = LC.HitTest(point)) != -1)
		{
			m_iItemDrop = iItem;
			for(i=0;i<=m_arImageList.GetUpperBound();i++)
				((CImageList*)m_arImageList[i])->DragLeave(this);
//			m_pimagelist->DragLeave(this); // unlock the window and hide drag image
			if (lStyle == LVS_REPORT || lStyle == LVS_LIST)
			{
				lvitem.iItem = iItem;
				lvitem.iSubItem = 0;
				lvitem.mask = LVIF_STATE;
				lvitem.stateMask = LVIS_DROPHILITED;  // highlight the drop target
				LC.SetItem(&lvitem);
			}

			point -= m_sizeDelta;
			for(int i=0;i<=m_arImageList.GetUpperBound();i++)
				((CImageList*)m_arImageList[i])->DragEnter(this, point);
			//m_pimagelist->DragEnter(this, point);  // lock updates and show drag image
		}
	}

	CListView::OnMouseMove(nFlags, point);
}
*/

void KDrListView::OnButtonUp(CPoint point)
{
	if (m_bDragging)  // end of the drag operation
	{
		long		lStyle;
		CString		cstr;

		lStyle = GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK; 

		m_bDragging = FALSE;
		
		//[ set of end of drag
		m_pimagelist->DragLeave(this);
		m_pimagelist->EndDrag();
		//]
		
		if (lStyle == LVS_REPORT && m_iItemDrop != m_iItemDrag)  
		{
			cstr = LC.GetItemText(m_iItemDrag, 0);
			LC.SetItemText(m_iItemDrop, 1, cstr);  // drop subitem text is dragged main item text
		}
		
		if (lStyle == LVS_LIST && m_iItemDrop != m_iItemDrag)  //add ** to the drop item text
		{
			cstr = LC.GetItemText(m_iItemDrop, 0);
			cstr += _T("**");
			LC.SetItemText(m_iItemDrop, 0, cstr);
		}
	
		if (lStyle == LVS_ICON || lStyle == LVS_SMALLICON)  // move the icon
		{
			point -= m_ptHotSpot;  // the icon should be drawn exactly where the image is
			//point += m_ptOrigin;
			LC.SetItemPosition(m_iItemDrag, point);  // just move the dragged item
		}

		//::ReleaseCapture();
	}
}

void KDrListView::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp(point);
	CListView::OnLButtonUp(nFlags, point);
}

void KDrListView::OnRButtonUp(UINT nFlags, CPoint point)
{
	//OnButtonUp(point);
	CListView::OnRButtonUp(nFlags, point);
}

void KDrListView::OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult)
{
	LV_DISPINFO  *plvDispInfo = (LV_DISPINFO *)pnmhdr;
 	LV_ITEM		 *plvItem = &plvDispInfo->item;

	//((CListCtrlPage *)GetParent())->ShowNotification(pnmhdr, pLResult);
	if (plvItem->pszText != NULL)
		LC.SetItemText(plvItem->iItem, plvItem->iSubItem, plvItem->pszText);
}

void KDrListView::OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pResult)
{
	CPoint	ptItem,		// used GetItemPosition
				ptAction,	// current cursor point 
				ptImage;		// used when creating image

	NM_LISTVIEW		*pnmListView = (NM_LISTVIEW *)pnmhdr;

	//((CListCtrlPage *)GetParent())->ShowNotification(pnmhdr, pResult);
	//ASSERT(!m_bDragging);
	//if(!m_bDragging) return;;
	
	if(m_pimagelist)
	{
		delete m_pimagelist;
		m_pimagelist = NULL;
	}

	m_bDragging = TRUE;
	m_iItemDrag = pnmListView->iItem;

	ptAction = pnmListView->ptAction;
	
	LC.GetItemPosition(m_iItemDrag, &ptItem);  // ptItem is relative to (0,0) and not the view origin
//	LC.GetOrigin(&m_ptOrigin);		// returned gabage so invalide value
	int cb = LC.GetSelectedCount();
	TCHAR buf[127];
	CString sOrg;
	if(cb > 1)
	{	wsprintf(buf, L"[[[ %d items selected.. ]]]",cb);
		sOrg = LC.GetItemText(m_iItemDrag,0);
		LC.SetItemText(m_iItemDrag,0,buf);
	}
	
	// ptImage will be filled by the initial location of upper-left corner of the image
	m_pimagelist = LC.CreateDragImage(m_iItemDrag, &ptImage);
	
	m_sizeDelta = ptAction - ptImage;   // difference between cursor pos and image pos
	m_ptHotSpot = ptAction - ptItem ;//+ m_ptOrigin;  // trye 1 , calculate hotspot for the cursor
	m_pimagelist->DragShowNolock(TRUE);  // lock updates and show drag image

	m_pimagelist->SetDragCursorImage(0, CPoint(0, 0));  // define the hot spot for the new cursor image

	CPoint ptUpLeft = ptAction - m_sizeDelta;	// OnMove

	//[ begin draging set
	m_pimagelist->BeginDrag(0, CPoint(0, 0));
	m_pimagelist->DragMove(ptUpLeft);  // move image to overlap original icon
	m_pimagelist->DragEnter(this, ptUpLeft);
	//]

/*
	m_pimagelist->SetDragCursorImage(0, CPoint(0,0));  // define the hot spot for the new cursor image
	m_pimagelist->BeginDrag(0, CPoint(0, 0));

	CPoint ptUpLeft = ptAction - m_sizeDelta;	// OnMove
	m_pimagelist->DragMove(ptUpLeft);  // move image to overlap original icon
	m_pimagelist->DragEnter(this, ptUpLeft);
*/
	if(cb > 1)
		LC.SetItemText(m_iItemDrag,0,(PS)sOrg);
	SetCapture();
}





/*
void KDrListView::OnBeginDrag(LPNMHDR pnmhdr, LRESULT *pResult)
{
	CPoint			ptItem(0,0), ptAction(0,0), ptImage(0,0);
	NM_LISTVIEW		*pnmListView = (NM_LISTVIEW *)pnmhdr;
	ASSERT(!m_bDragging);
		  

	for(int i=0;i<=m_arImageList.GetUpperBound();i++)
	{	CImageList *pil = (CImageList*)m_arImageList[i];
		if(pil)		delete pil;
	}
	m_arImageList.RemoveAll();
	m_arSelItem.RemoveAll();
  
	CWordArray arI;
	int cur;
	int cb = LC.GetSelectedCount();
	for(i=0,cur=-1; i<cb; i++)
	{	cur = LC.GetNextItem(cur,LVNI_ALL|LVNI_SELECTED);
		m_arSelItem.Add(cur);
	}

	m_bDragging = TRUE;
	m_iItemDrag = pnmListView->iItem;
	ptAction = pnmListView->ptAction;
	LC.GetItemPosition(m_iItemDrag, &ptItem);  // ptItem is relative to (0,0) and not the view origin
	CRect rc;
	LC.GetItemRect( m_iItemDrag, &rc, LVIR_LABEL );
	int h=rc.Height();  

	LC.GetOrigin(&m_ptOrigin);
	for(i=0;i<cb;i++)
	{
		CImageList *pimagelist = new CImageList;
		int iItem = m_arSelItem[i];
		pimagelist = LC.CreateDragImage(iItem, &ptImage);
		m_arImageList.Add((CObject*)pimagelist);
		m_sizeDelta = ptAction - ptImage;                  // difference between cursor pos and image pos
		m_ptHotSpot = ptAction - ptItem + m_ptOrigin;      // calculate hotspot for the cursor
		pimagelist->DragShowNolock(TRUE);					// lock updates and show drag image
		pimagelist->SetDragCursorImage(0, m_ptHotSpot);		// define the hot spot for the new cursor image
		pimagelist->BeginDrag(0, CPoint(0, 0));
		ptAction -= m_sizeDelta;
		CPoint lstPt = ptAction + CPoint(0,h*(i-m_iItemDrag));
		pimagelist->DragEnter(this, lstPt);
		pimagelist->DragMove(lstPt);//ptAction);						// move image to overlap original icon
	}
	SetCapture();
}
//void KDrListView::AddDragImage(CObArray &ar,
*/
