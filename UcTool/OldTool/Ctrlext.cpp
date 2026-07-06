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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#define _AFXCTL_INLINE inline


/////////////////////////////////////////////////////////////////////////////
// CTreeCtrlNode


const KTreeCursor& KTreeCursor::operator =(const KTreeCursor& posSrc)
{
	if(&posSrc != this){
		m_hTreeItem = posSrc.m_hTreeItem;
		m_pTree = posSrc.m_pTree;
	}
	return *this;
}


KTreeCursor KTreeCursor::_Insert(LPCTSTR strItem,int nImageIndex,HTREEITEM hAfter)
{
	TV_INSERTSTRUCT ins;
	ins.hParent = m_hTreeItem;
	ins.hInsertAfter = hAfter;
	ins.item.mask = TVIF_TEXT;
	ins.item.pszText = (LPTSTR) strItem;
	if(nImageIndex != -1){
		ins.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		ins.item.iImage = nImageIndex;
		ins.item.iSelectedImage = nImageIndex+1;
	}
	const TCHAR* tvi[4] = {_T("TVI_ROOT"),_T("TVI_FIRST"),_T("TVI_LAST"),_T("TVI_SORT")};
	HTREEITEM hti[4] = {TVI_ROOT,TVI_FIRST,TVI_LAST,TVI_SORT,};
	const TCHAR* p = nullptr;
	for(int i=0;i<4;i++)
		if(hAfter == hti[i])
			p = tvi[i];
//	MYTRACE("_Insert(LPCTSTR strItem(%s),int nImageIndex(%d),HTREEITEM hAfter(%s))",
//		strItem,nImageIndex,p);
//	TRACE("_Insert(LPCTSTR strItem(%s),int nImageIndex(%d),HTREEITEM hAfter(%s))\n",
//		strItem,nImageIndex,p);
	return KTreeCursor(m_pTree->InsertItem(&ins), m_pTree);
}

int KTreeCursor::GetImageID()
{
	TV_ITEM item;
	item.mask = TVIF_HANDLE | TVIF_IMAGE;
	item.hItem = m_hTreeItem;
	m_pTree->GetItem(&item);
	return item.iImage;
}

/////////////////////////////////////////////////////////////////////////////
// KTreeCtrlEx

// All handled by inlines

/////////////////////////////////////////////////////////////////////////////
// KListCtrlEx

BOOL KListCtrlEx::AddColumn(LPCTSTR strItem,int nItem,int nSubItem,int nMask,int nFmt)
{
	LV_COLUMN lvc;
	lvc.mask = nMask;
	lvc.fmt = nFmt;
	lvc.pszText = (LPTSTR) strItem;
	lvc.cx = GetStringWidth(lvc.pszText) + 15;
	if(nMask & LVCF_SUBITEM){
		if(nSubItem != -1)
			lvc.iSubItem = nSubItem;
		else
			lvc.iSubItem = nItem;
	}
	//MYTRACE("AddColumn(LPCTSTR strItem(%s),int nItem(%d),int nSubItem(%d),int nMask(%d),int nFmt(%d))",
	//			strItem,nItem,nSubItem,nMask,nFmt);
//	TRACE("AddColumn(LPCTSTR strItem(%s),int nItem(%d),int nSubItem(%d),int nMask(%d),int nFmt(%d))\n",
//				strItem,nItem,nSubItem,nMask,nFmt);
	return InsertColumn(nItem,&lvc);
}

BOOL KListCtrlEx::AddItem(int nItem,int nSubItem,LPCTSTR strItem,int nImageIndex)
{
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = nSubItem;
	lvItem.pszText = (LPTSTR) strItem;
	if(nImageIndex != -1){
		lvItem.mask |= LVIF_IMAGE;
		lvItem.iImage =nImageIndex;//|= LVIF_IMAGE;
	}
	if(nSubItem == 0)
		InsertItem(&lvItem);
	//MYTRACE("AddItem(int nItem(%d),int nSubItem(%d),LPCTSTR strItem(%s),int nImageIndex(%d))",
	//			nItem,nSubItem,strItem,nImageIndex);
//	if(nSubItem == 0)
//		TRACE("AddItem(int nItem(%d),int nSubItem(%d),LPCTSTR strItem(%s),int nImageIndex(%d))\n",
//				nItem,nSubItem,strItem,nImageIndex);
	return SetItem(&lvItem);
}

void KListCtrlEx::SelectItem(int nItem)
{	int i=nItem;
//	EnsureVisible(i,TRUE);
	LV_ITEM lvitem;
	lvitem.iItem = i;
	lvitem.iSubItem = 0;
	lvitem.mask = LVIF_STATE;
	lvitem.state = LVIS_SELECTED;
	lvitem.stateMask = LVIS_SELECTED;  // highlight the drop target
	SetItem(&lvitem);
//	CRect rc;
//	GetItemRect(i,rc,LVIR_LABEL);
//	CPoint point = rc.TopLeft();
//	SendMessage(WM_LBUTTONDOWN,0,MAKELONG(point.x,point.y));
}




int	KListCtrlEx::GetSeletedItem()
{	int cb = GetSelectedCount();
	if(cb==1)
		return GetNextItem(-1,LVNI_SELECTED);
	else
		return -1;
}












// ex result : aaa/asdfa/asdfad/  return 2;
int KTreeCtrlEx::GetItemDepth(HTREEITEM hTiNode)
{
	if(hTiNode==NULL) return -1;
	KTreeCursor hTi;
	hTi = KTreeCursor(hTiNode,this);
	int i = 0;
	for(i=0;;i++)
	{
		hTi=GetNextItem(hTi,TVGN_PARENT);
		if(hTi == NULL)
			break;
	}		
	return i;
}







/////////////////////////////////////////////////////////////////////////////
//
// ���̸� return�Ѵ�. root�� 0���� �ؼ� 1,2,.. 
// start�� full path���� root�� path�� ���� �ϱ� ���ؼ� �̴�. 0:full path 1:root ����.
// ���� start���� root�� ������ �����ٸ� ���� ���� str�� �����Ѵ�.
// ���Լ��� �θ� �Ŀ� ���ϰ��� start���� �������� Ȯ���� ������ ���ϴ� ������ ���� �����̴�.
int KTreeCtrlEx::GetFullItemText(HTREEITEM hTiNode,CString &str,TCHAR sep/* = '/' */,int start/*=0*/)
{
	KTreeCursor hTi,hTiSel;
	hTiSel = hTi = KTreeCursor(hTiNode,this);

	CString sTmp=_T("");
	str = _T("");
	sTmp = str;
	str = GetItemText(hTi);
	str += sep;
	str += sTmp;	
	int i = 0;
	for(i=0;;)
	{
		hTi=GetNextItem(hTi,TVGN_PARENT);
		if(hTi == NULL)
			break;
		i++;
	}		// ex result : aaa/asdfa/asdfad/
	int total=i;
	int count = total - start;
	if(count < 0)
	{	//str = "";
		str = _T("");
		return total;
	}
	hTi = hTiSel;
	for(i=0;i<count;i++)
	{
		hTi=GetNextItem(hTi,TVGN_PARENT);
		if(hTi == NULL)
			break;
		sTmp = str;
		str = GetItemText(hTi);
		str += sep;
		str += sTmp;
	}		// ex result : aaa/asdfa/asdfad/
	return total;
}
KTreeCursor KTreeCtrlEx::FindBranchByText(const TCHAR* key)
{
	CStringArray ar;
	TCHAR sep[2];
	sep[1]='\0';
	sep[0]='/';//m_separator;
	KwCutByToken(key,sep,ar);
	int depth = 0;//ar.GetUpperBound()+1;

	KTreeCursor hTi = GetRootItem();	
	return FindBranchNextByText(hTi,depth,ar);

/*	KTreeCursor hTi=TC.GetNextItem(tiRoot,TVGN_CARET);
	hTi=GetNextItem(tiRoot,TVGN_CARET);
	DWORD dw=GetItemData(hTi);
	int nImage,  nSelectedImage;
	BOOL b=GetItemImage(hTi, nImage, nSelectedImage ); 
	return KTreeCursor(NULL,this);
*/
}

// ��Ȯ�� �ִ°�츸 ���� �ϰ� ������ NULL�̴�.
KTreeCursor KTreeCtrlEx::FindBranchNextByText(HTREEITEM tiParent,int depth,CStringArray &ar)
{
	if(ar.GetUpperBound() < depth)
		return KTreeCursor(NULL,this);
	KTreeCursor tiChild=GetNextItem(tiParent,TVGN_CHILD);
	if(tiChild == NULL)
		return KTreeCursor(NULL,this);
#ifdef _DEBUG
	CString Text=GetItemText(tiChild);
	CString sDepth = ar.GetAt(depth);
	TRACE("branch --- %s -- %s\n",(LPCTSTR)Text,(LPCTSTR)sDepth);
#endif

	if(GetItemText(tiChild) == ar.GetAt(depth))	// text�� ����.
	{
		if(ar.GetUpperBound() == depth)		// ������ depth�̴� == ������ ã�Ҵ�.
			return tiChild;					// ���� �۾� ��.
		else
			return FindBranchNextByText(tiChild,depth+1,ar); // ���� child�� ���ұ�.
	}
	for(;;)
	{
		tiChild=GetNextItem(tiChild,TVGN_NEXT);
		if(tiChild == NULL)	// ���̻� ����. �׷���..
			return KTreeCursor(NULL,this); //tiParent;
#ifdef _DEBUG
		CString Text=GetItemText(tiChild);
		CString sDepth = ar.GetAt(depth);
		TRACE("branch --- %s -- %s\n",(LPCTSTR)Text,(LPCTSTR)sDepth);
#endif
		if(GetItemText(tiChild) == ar.GetAt(depth))
		{
			if(ar.GetUpperBound() == depth)
				return tiChild;
			else
				return FindBranchNextByText(tiChild,depth+1,ar);
		}
	}
	return KTreeCursor(NULL,this);
}
/*	hTi=GetNextItem(tiRoot,TVGN_CHILD);
	hTi=GetNextItem(tiRoot,TVGN_DROPHILITE);
	hTi=GetNextItem(tiRoot,TVGN_FIRSTVISIBLE);
	hTi=GetNextItem(tiRoot,TVGN_NEXT);
	hTi=GetNextItem(tiRoot,TVGN_NEXTVISIBLE);
	hTi=GetNextItem(tiRoot,TVGN_CARET);
	hTi=GetNextItem(tiRoot,TVGN_PARENT);
	hTi=GetNextItem(tiRoot,TVGN_PREVIOUS);
	hTi=GetNextItem(tiRoot,TVGN_PREVIOUSVISIBLE);
	hTi=GetNextItem(tiRoot,TVGN_ROOT);
*/

////////////////////////////////////////////////////////////////////////
// full path�� �ָ� �־��� parent���� ���� branch�� ����� ����.
// parent �� NULL�̸� root�� ���δ�.
void KTreeCtrlEx::AddBranch(const TCHAR* str,HTREEITEM parent/*=NULL*/,TCHAR sepChar/*='/'*/,int iImage/*=-1*/)
{
	CStringArray ar;
	TCHAR sep[2];
	sep[1]='\0';
	sep[0]=sepChar;//'/';//m_separator;
	KwCutByToken(str,sep,ar);
	
	KTreeCursor tiTmp,tiParent;
	if(parent == NULL)
		tiParent = GetRootItem();
	else
		tiParent = KTreeCursor(parent,this);

	CString arStr;
	CString itemText;
	for(int dep=0;dep<=ar.GetUpperBound();dep++)
	{
		tiTmp=GetNextItem(tiParent,TVGN_CHILD);
		
		if((HTREEITEM)tiTmp)	// child �߰�.
		{	arStr=ar.GetAt(dep);
			itemText=GetItemText(tiTmp);
			if(GetItemText(tiTmp) == arStr)	// ù���� child�� ���׿�..
			{	tiParent = tiTmp;
				continue;
			}
			else	// child�� 1�� �̻� ������ ù��° ���� ������ �ʱ���
			{
				while(1) // ��� ������ ����.
				{
					tiTmp=GetNextItem(tiTmp,TVGN_NEXT);

					if((HTREEITEM)tiTmp) 
					{
						itemText=GetItemText(tiTmp);
						if(GetItemText(tiTmp) == ar.GetAt(dep))// �̹� depth�� �ִ�. ���� depth�� ����.
						{	tiParent = tiTmp;
							break;
						}
					//	else // ���� ������
					//	���� while loop;
					}
					else	// ���̻� node�� ����. ã���� ����. ������ �ڴ�.
					{	//int imageIndex = (iImage > -1)?iImage:IMAGE_INDEX(dep);
						tiParent = tiParent.AddTail(ar.GetAt(dep),
									(iImage > -1)?iImage:IMAGE_INDEX(dep));
						break;
					}
				}
			}
		}
		else // child �ϳ��� ����..
		{	tiParent = tiParent.AddTail(ar.GetAt(dep),(iImage > -1)?iImage:IMAGE_INDEX(dep));
		}
	}
}
/*
void KTreeCtrlEx::SetSeparator(TCHAR sep)
{ m_separator = sep; 
}
*/