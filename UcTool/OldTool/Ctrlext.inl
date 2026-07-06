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
// KTreeCursor CTLEXT_INLINE functions

#define CTLEXT_INLINE inline

CTLEXT_INLINE KTreeCursor::KTreeCursor() : m_hTreeItem(NULL),	m_pTree(NULL)
	{ }
CTLEXT_INLINE KTreeCursor::KTreeCursor(HTREEITEM hTreeItem,KTreeCtrlEx* pTree) :	m_hTreeItem(hTreeItem),	m_pTree(pTree)
	{ }
CTLEXT_INLINE KTreeCursor::KTreeCursor(const KTreeCursor& posSrc)
	{ *this = posSrc; }
CTLEXT_INLINE KTreeCursor::~KTreeCursor()
	{ }
CTLEXT_INLINE KTreeCursor::operator HTREEITEM()
	{ return m_hTreeItem; }
CTLEXT_INLINE KTreeCursor KTreeCursor::InsertAfter(LPCTSTR strItem,HTREEITEM hAfter,int nImageIndex)
	{ return _Insert(strItem,nImageIndex,hAfter); }
CTLEXT_INLINE KTreeCursor KTreeCursor::AddHead(LPCTSTR strItem,int nImageIndex)
	{ return _Insert(strItem,nImageIndex,TVI_FIRST); }
CTLEXT_INLINE KTreeCursor KTreeCursor::AddTail(LPCTSTR strItem,int nImageIndex)
	{ return _Insert(strItem,nImageIndex,TVI_LAST); }
CTLEXT_INLINE BOOL KTreeCursor::GetRect(LPRECT lpRect, BOOL bTextOnly)
	{ return m_pTree->GetItemRect(m_hTreeItem,lpRect,bTextOnly); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetNext(UINT nCode)
	{ return m_pTree->GetNextItem(m_hTreeItem,nCode); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetChild()
	{ return m_pTree->GetChildItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetNextSibling()
	{ return m_pTree->GetNextSiblingItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetPrevSibling()
	{ return m_pTree->GetPrevSiblingItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetParent()
	{ return m_pTree->GetParentItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetFirstVisible()
	{ return m_pTree->GetFirstVisibleItem(); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetNextVisible()
	{ return m_pTree->GetNextVisibleItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetPrevVisible()
	{ return m_pTree->GetPrevVisibleItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetSelected()
	{ return m_pTree->GetSelectedItem(); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetDropHilight()
	{ return m_pTree->GetDropHilightItem(); }
CTLEXT_INLINE KTreeCursor KTreeCursor::GetRoot()
	{ return m_pTree->GetRootItem(); }
CTLEXT_INLINE CString KTreeCursor::GetText()
	{ return m_pTree->GetItemText(m_hTreeItem); }
CTLEXT_INLINE BOOL KTreeCursor::GetImage(int& nImage, int& nSelectedImage)
	{ return m_pTree->GetItemImage(m_hTreeItem,nImage,nSelectedImage); }
CTLEXT_INLINE UINT KTreeCursor::GetState(UINT nStateMask)
	{ return m_pTree->GetItemState(m_hTreeItem,nStateMask); }
CTLEXT_INLINE DWORD_PTR KTreeCursor::GetData()
	{ return m_pTree->GetItemData(m_hTreeItem); }
//CTLEXT_INLINE BOOL SetItem(UINT nMask, LPCTSTR lpszItem, int nImage, 
//	int nSelectedImage,	UINT nState,nStateMask,lParam);
CTLEXT_INLINE BOOL KTreeCursor::SetText(LPCTSTR lpszItem)
	{ return m_pTree->SetItemText(m_hTreeItem,lpszItem); }
CTLEXT_INLINE BOOL KTreeCursor::SetImage(int nImage, int nSelectedImage)
	{ return m_pTree->SetItemImage(m_hTreeItem,nImage,nSelectedImage); }
CTLEXT_INLINE BOOL KTreeCursor::SetState(UINT nState, UINT nStateMask)
	{ return m_pTree->SetItemState(m_hTreeItem,nState,nStateMask); }
CTLEXT_INLINE BOOL KTreeCursor::SetData(DWORD dwData)
	{ return m_pTree->SetItemData(m_hTreeItem,dwData); }
CTLEXT_INLINE BOOL KTreeCursor::HasChildren()
	{ return m_pTree->ItemHasChildren(m_hTreeItem); }
// Operations
CTLEXT_INLINE BOOL KTreeCursor::Delete()
	{ return m_pTree->DeleteItem(m_hTreeItem); }
CTLEXT_INLINE BOOL KTreeCursor::Expand(UINT nCode)
	{ return m_pTree->Expand(m_hTreeItem,nCode); }
/*
CTLEXT_INLINE KTreeCursor KTreeCursor::Select(UINT nCode)
	{ return m_pTree->Select(m_hTreeItem,nCode); }
CTLEXT_INLINE KTreeCursor KTreeCursor::Select()
	{ return m_pTree->SelectItem(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::SelectDropTarget()
	{ return m_pTree->SelectDropTarget(m_hTreeItem); }
CTLEXT_INLINE KTreeCursor KTreeCursor::SelectSetFirstVisible()
	{ return m_pTree->SelectSetFirstVisible(m_hTreeItem); }
*/
CTLEXT_INLINE CEdit* KTreeCursor::EditLabel()
	{ return m_pTree->EditLabel(m_hTreeItem); }
CTLEXT_INLINE CImageList* KTreeCursor::CreateDragImage()
	{ return m_pTree->CreateDragImage(m_hTreeItem); }
CTLEXT_INLINE BOOL KTreeCursor::SortChildren()
	{ return m_pTree->SortChildren(m_hTreeItem); }
CTLEXT_INLINE BOOL KTreeCursor::EnsureVisible()
	{ return m_pTree->EnsureVisible(m_hTreeItem); }





inline CString KTreeCtrlEx::GetSelectedFullItemText(TCHAR sep/* = '/' */,int start/*=0*/)
{
	KTreeCursor hTi = GetSelectedItem();
	CString str;
	GetFullItemText(hTi,str,sep,start);
	return str;
}
inline CString KTreeCtrlEx::GetFullItemText(HTREEITEM hTiNode,TCHAR sep/* = '/' */,int start/*=0*/)
{	CString str;
	GetFullItemText(hTiNode,str,sep,start);
	return str;
}

inline int KTreeCtrlEx::GetSelectedFullItemText(CString &str,TCHAR sep/* = '/' */,int start/*=0*/)
{
	KTreeCursor hTi = GetSelectedItem();
	return GetFullItemText(hTi,str,sep,start);
}



/////////////////////////////////////////////////////////////////////////////
// KTreeCtrlEx CTLEXT_INLINE functions

CTLEXT_INLINE KTreeCtrlEx::KTreeCtrlEx() : CTreeCtrl()
	{ }//m_separator='\\';}
CTLEXT_INLINE KTreeCtrlEx::~KTreeCtrlEx()
	{ }
CTLEXT_INLINE CImageList* KTreeCtrlEx::SetImageList(CImageList* pImageList, int nImageListType)
	{ return CTreeCtrl::SetImageList(pImageList,nImageListType); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetNextItem(HTREEITEM hItem, UINT nCode)
	{ return KTreeCursor(CTreeCtrl::GetNextItem(hItem,nCode),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetChildItem(HTREEITEM hItem)
	{ return KTreeCursor(CTreeCtrl::GetChildItem(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetNextSiblingItem(HTREEITEM hItem)
	{ return KTreeCursor(CTreeCtrl::GetNextSiblingItem(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetPrevSiblingItem(HTREEITEM hItem)
	{ return KTreeCursor(CTreeCtrl::GetPrevSiblingItem(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetParentItem(HTREEITEM hItem)
	{ return KTreeCursor(CTreeCtrl::GetParentItem(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetFirstVisibleItem()
	{ return KTreeCursor(CTreeCtrl::GetFirstVisibleItem(),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetNextVisibleItem(HTREEITEM hItem)
	{ return KTreeCursor(CTreeCtrl::GetNextVisibleItem(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetPrevVisibleItem(HTREEITEM hItem)
	{ return KTreeCursor(CTreeCtrl::GetPrevVisibleItem(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetSelectedItem()
	{ return KTreeCursor(CTreeCtrl::GetSelectedItem(),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetDropHilightItem()
	{ return KTreeCursor(CTreeCtrl::GetDropHilightItem(),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::GetRootItem()
	{ return KTreeCursor(CTreeCtrl::GetRootItem(),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::InsertItem(LPTV_INSERTSTRUCT lpInsertStruct)
	{ return KTreeCursor(CTreeCtrl::InsertItem(lpInsertStruct),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::InsertItem(UINT nMask, LPCTSTR lpszItem, int nImage, 
	int nSelectedImage,	UINT nState, UINT nStateMask, LPARAM lParam, 
	HTREEITEM hParent, HTREEITEM hInsertAfter)
	{ return KTreeCursor(CTreeCtrl::InsertItem(nMask, lpszItem, nImage, 
		nSelectedImage, nState, nStateMask, lParam, hParent, hInsertAfter),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::InsertItem(LPCTSTR lpszItem, HTREEITEM hParent,HTREEITEM hInsertAfter)
	{ return KTreeCursor(CTreeCtrl::InsertItem(lpszItem, hParent, hInsertAfter),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage,
		HTREEITEM hParent, HTREEITEM hInsertAfter)
	{ return KTreeCursor(CTreeCtrl::InsertItem(lpszItem, nImage, nSelectedImage,
		hParent, hInsertAfter),this); }
//CTLEXT_INLINE KTreeCursor KTreeCtrlEx::Select(HTREEITEM hItem, UINT nCode)
//	{ return KTreeCursor(CTreeCtrl::Select(hItem,nCode),this); }
//CTLEXT_INLINE KTreeCursor KTreeCtrlEx::SelectItem(HTREEITEM hItem)
//	{ return KTreeCursor(CTreeCtrl::SelectItem(hItem),this); }
//CTLEXT_INLINE KTreeCursor KTreeCtrlEx::SelectDropTarget(HTREEITEM hItem)
//	{ return KTreeCursor(CTreeCtrl::SelectDropTarget(hItem),this); }
//CTLEXT_INLINE KTreeCursor KTreeCtrlEx::SelectSetFirstVisible(HTREEITEM hItem)
//	{ return KTreeCursor(CTreeCtrl::SelectSetFirstVisible(hItem),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::HitTest(CPoint pt, UINT* pFlags)
	{ return KTreeCursor(CTreeCtrl::HitTest(pt,pFlags),this); }
CTLEXT_INLINE KTreeCursor KTreeCtrlEx::HitTest(TV_HITTESTINFO* pHitTestInfo)
	{ return KTreeCursor(CTreeCtrl::HitTest(pHitTestInfo),this); }

/////////////////////////////////////////////////////////////////////////////
// KListCtrlEx CTLEXT_INLINE functions

CTLEXT_INLINE KListCtrlEx::KListCtrlEx() : CListCtrl()
 { }
CTLEXT_INLINE KListCtrlEx::~KListCtrlEx()
 { }
CTLEXT_INLINE CImageList* KListCtrlEx::SetImageList(CImageList* pImageList, int nImageListType)
 { return CListCtrl::SetImageList(pImageList,nImageListType); }
