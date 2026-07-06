// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __TREECTLX_H__
#define __TREECTLX_H__

#ifdef _AFX_NO_AFXCMN_SUPPORT
	#error Windows Common Control classes not supported in this library variant.
#endif

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#include "../UcExport.inl"


/////////////////////////////////////////////////////////////////////////////
// TREECTLX - MFC Tree Control Helper Classes

class KTreeCursor;
class KTreeCtrlEx;
#define IMAGE_INDEX(dep) ((dep==-1)?10:((dep%5)*2))

/////////////////////////////////////////////////////////////////////////////
// KTreeCursor

class UCTOOLDYNAMIC KTreeCursor
{
	// Attributes
protected:
	HTREEITEM	m_hTreeItem;
	KTreeCtrlEx	*m_pTree;

	// Implementation
protected:
	KTreeCursor _Insert(LPCTSTR strItem,int nImageIndex,HTREEITEM hAfter);

	// Operation
public:
	KTreeCursor();
	KTreeCursor(HTREEITEM hTreeItem, KTreeCtrlEx* pTree);
	KTreeCursor(const KTreeCursor& posSrc);
	~KTreeCursor();
	const KTreeCursor& operator =(const KTreeCursor& posSrc);
	operator HTREEITEM();

	KTreeCursor InsertAfter(LPCTSTR strItem,HTREEITEM hAfter,int nImageIndex = -1);
	KTreeCursor AddHead(LPCTSTR strItem,int nImageIndex = -1);
	KTreeCursor AddTail(LPCTSTR strItem,int nImageIndex = -1);

	int GetImageID();

	BOOL GetRect(LPRECT lpRect, BOOL bTextOnly);
	KTreeCursor GetNext(UINT nCode);
	KTreeCursor GetChild();
	KTreeCursor GetNextSibling();
	KTreeCursor GetPrevSibling();
	KTreeCursor GetParent();
	KTreeCursor GetFirstVisible();
	KTreeCursor GetNextVisible();
	KTreeCursor GetPrevVisible();
	KTreeCursor GetSelected();
	KTreeCursor GetDropHilight();
	KTreeCursor GetRoot();
	CString GetText();
	BOOL GetImage(int& nImage, int& nSelectedImage);
	UINT GetState(UINT nStateMask);
	DWORD_PTR GetData();
	BOOL Set(UINT nMask, LPCTSTR lpszItem, int nImage, 
		int nSelectedImage,	UINT nState, UINT nStateMask, LPARAM lParam);
	BOOL SetText(LPCTSTR lpszItem);
	BOOL SetImage(int nImage, int nSelectedImage);
	BOOL SetState(UINT nState, UINT nStateMask);
	BOOL SetData(DWORD dwData);
	BOOL HasChildren();
// Operations
	BOOL Delete();

	BOOL Expand(UINT nCode = TVE_EXPAND);
/*	KTreeCursor Select(UINT nCode);
	KTreeCursor Select();
	KTreeCursor SelectDropTarget();
	KTreeCursor SelectSetFirstVisible();
*/
	CEdit* EditLabel();
	CImageList* CreateDragImage();
	BOOL SortChildren();
	BOOL EnsureVisible();
};


/////////////////////////////////////////////////////////////////////////////
// KTreeCtrlEx

class UCTOOLDYNAMIC KTreeCtrlEx : public CTreeCtrl
{
	// Attributes
protected:
	//TCHAR m_separator;	//ex: '\\' , '/', ',' ...
	// Operation
public:	// kdw
	CString GetSelectedFullItemText(TCHAR sep = '/' ,int start=0);			// inline
	CString GetFullItemText(HTREEITEM hTiNode,TCHAR sep = '/' ,int start=0);// inline
	int GetSelectedFullItemText(CString &str,TCHAR sep = '/' ,int start=0);// inline
	int GetFullItemText(HTREEITEM hTiNode,CString &str,TCHAR sep = '/' ,int start=0);
	int GetItemDepth(HTREEITEM hTiNode);

	
	
	//void SetSeparator(TCHAR sep);
	KTreeCursor FindBranchByText(const TCHAR* key);
	KTreeCursor FindBranchNextByText(HTREEITEM tiParent,int depth,CStringArray &ar);
	void AddBranch(const TCHAR* str,HTREEITEM parent=NULL,TCHAR sep='/',int iImage=-1);

public:
	KTreeCtrlEx();
	~KTreeCtrlEx();
	CImageList* SetImageList(CImageList* pImageList, int nImageListType = TVSIL_NORMAL);

	KTreeCursor GetNextItem(HTREEITEM hItem, UINT nCode);
	KTreeCursor GetChildItem(HTREEITEM hItem);
	KTreeCursor GetNextSiblingItem(HTREEITEM hItem);
	KTreeCursor GetPrevSiblingItem(HTREEITEM hItem);
	KTreeCursor GetParentItem(HTREEITEM hItem);
	KTreeCursor GetFirstVisibleItem();
	KTreeCursor GetNextVisibleItem(HTREEITEM hItem);
	KTreeCursor GetPrevVisibleItem(HTREEITEM hItem);
	KTreeCursor GetSelectedItem();
	KTreeCursor GetDropHilightItem();
	KTreeCursor GetRootItem();
	KTreeCursor InsertItem(LPTV_INSERTSTRUCT lpInsertStruct);
	KTreeCursor InsertItem(UINT nMask, LPCTSTR lpszItem, int nImage, 
		int nSelectedImage,	UINT nState, UINT nStateMask, LPARAM lParam, 
		HTREEITEM hParent, HTREEITEM hInsertAfter);
	KTreeCursor InsertItem(LPCTSTR lpszItem, HTREEITEM hParent = TVI_ROOT, 
		HTREEITEM hInsertAfter = TVI_LAST);
	KTreeCursor InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage,
		HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
//	KTreeCursor Select(HTREEITEM hItem, UINT nCode);
//	KTreeCursor SelectItem(HTREEITEM hItem);
//	KTreeCursor SelectDropTarget(HTREEITEM hItem);
//	KTreeCursor SelectSetFirstVisible(HTREEITEM hItem);
	KTreeCursor HitTest(CPoint pt, UINT* pFlags = NULL);
	KTreeCursor HitTest(TV_HITTESTINFO* pHitTestInfo);
};

/////////////////////////////////////////////////////////////////////////////
// KListCtrlEx

class UCTOOLDYNAMIC KListCtrlEx : public CListCtrl
{
	// Attributes
protected:

	// Operation
public:
	KListCtrlEx();
	~KListCtrlEx();
	CImageList* SetImageList(CImageList* pImageList, int nImageListType = TVSIL_NORMAL);
	BOOL AddColumn(
		LPCTSTR strItem,int nItem,int nSubItem = -1,
		int nMask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
		int nFmt = LVCFMT_LEFT);
	BOOL AddItem(int nItem,int nSubItem,LPCTSTR strItem,int nImageIndex = -1);
	void SelectItem(int nItem);
	int	GetSeletedItem();
};


/////////////////////////////////////////////////////////////////////////////

#include "Ctrlext.inl"

#endif //__TREECTLX_H__
