#include "pch.h"

#include "UcTool.h"
#include "UcWndInvokable.h"
#include "UcWindow.h"




CUcCriticalSection KBeginInvoke::_csGabage;
CUcCriticalSection KBeginInvoke::_csSendMsg;

// CPostMainTaskHelper 정적 멤버 초기화
//CUcCriticalSection CPostMainTaskHelper::s_csGarbage;
//KPtrList<CPostMainTaskHelper::LambdaData>* CPostMainTaskHelper::s_pGarbageList = nullptr;
//WORD CPostMainTaskHelper::s_srl = 0;



///?변경
/// Invoke때 할당한 게 Post 받아서 읿을  수도 있으니, gabage를 프리 하는 방법을 쓴다.
/// static auto free array를 쓴다.
WORD KBeginInvoke::s_srl = 0;

KBeginInvoke::KBeginInvoke(std::function<void(LPVOID)> pLambda, LPCSTR fnc, int line, function<void(LPVOID)> pLambdaFinish)
	: m_fnc(fnc)
	, m_line(line)
{
#ifdef _UseSharedFunc_
	m_lambda = make_shared<std::function<void()>>(pLambda);
	m_lambdaFinish = make_shared<std::function<void()>>(pLambdaFinish);
#else
	m_lambda = pLambda;
	m_lambdaFinish = pLambdaFinish;
#endif // _DEBUG
	s_srl++;
	_srl = s_srl;
	_tik = GetTickCount64();
	//auto pbi = this;
	//TRACE("!!! Created, %ld, %s (%d)\n", pbi->_srl, pbi->m_fnc, pbi->m_line);
}

KBeginInvoke::KBeginInvoke(std::function<LRESULT(LPVOID)> pLambda, LPCSTR fnc, int line)
	: m_fnc(fnc)
	, m_line(line)
{
#ifdef _UseSharedFunc_
	m_pLambdaSend = make_shared<std::function<LRESULT(LPVOID)>>(pLambda);
#else
	m_pLambdaSend = pLambda;
#endif // _UseSharedFunc_
	s_srl++;
	_srl = s_srl;
	_tik = GetTickCount64();
}

void KBeginInvoke::setInvokeFree(KBeginInvoke* pbi)
{
	CSyncAutoLock __lock(&_csGabage, TRUE, __FUNCTION__, __LINE__, "_csGabage");
	KPtrList<KBeginInvoke>* pl = getGabage();
	pl->push_back(pbi);
}

void KBeginInvoke::freeInvokeFree()
{
	CSyncAutoLock __lock(&_csGabage, TRUE, __FUNCTION__, __LINE__, "_csGabage");
	KPtrList<KBeginInvoke>* pl = getGabage();
	auto sz = pl->size();
	int nDeleted = 0;
	int nNotYet = 0;
	BOOL bRapped = FALSE;
	for (ULONGLONG i = 0; i < sz; i++)
	{
		KBeginInvoke* pbi = (KBeginInvoke*)pl->front();
		if (pbi)
		{
			if (pbi->_bCalled)// 수행하기 전에 free된 경우도 있드라.
			{
				if (i < (sz - 2000))// 2000개는 항상 남겨 두자.
				{
					//TRACE("@@@ CALLED delete, %ld, %s (%d)\n", pbi->_srl, pbi->m_fnc, pbi->m_line);
					delete pbi;
					pl->pop_front();
					nDeleted++;
				}
			}
			else
			{
				//TRACE("### NOT CALLED YET, %ld, %s (%d)\n", pbi->_srl, pbi->m_fnc, pbi->m_line);
				bRapped = TRUE;
				nNotYet++;
				break;
			};
		}
	}
}

/// 이 함수는 background thread 에서 UI쪽 변수를 접근 하거나 UI API를 비동기적으로 수행하고자 할때 사용한다.
/// 언제 수행하는지는 UI의 메시지큐에 Post되므로 UI 작업 뒤로 순서를 기다리다 실행 된다.
/// 이렇게 하면, UI와 background 작업이 부르럽게 진행 된다.
void DebugShowWindow(CWnd * pw, int nCmdShow)
{
#ifdef _DEBUG
	///return;//필요시 제거.

	if (pw)
	{
		if (IsRealWindow(pw))//::IsWindow(pw->GetSafeHwnd()))
		{
			CString wcl;
			GetClassName(pw->GetSafeHwnd(), wcl.GetBuffer(512), 512);
			wcl.ReleaseBuffer();
			auto* rc = pw->GetRuntimeClass();
			if (rc)
			{
				//BOOL bShow = FALSE;
				//switch(nCmdShow)
				//{
				//case SW_SHOWNORMAL:
				//case SW_SHOWMAXIMIZED:
				//case SW_SHOWNOACTIVATE:
				//case SW_SHOW:
				//case SW_SHOWNA:
				//case SW_RESTORE:
				//case SW_SHOWDEFAULT:
				//	bShow = TRUE;
				//	break;
				//default:
				//	_break;
				//	break;
				//}
				CStringA cl(rc->m_lpszClassName);
				CStringA sa;
				auto bvis = pw->IsWindowVisible();
				CStringA sShow = UcShowValueToStr(nCmdShow);
				sa.Format("%s::ShowWindow(%s) \n", cl.GetString(), sShow.GetString());// bShow ? "SW_SHOW" : "SW_HIDE");
				//if(cl == "CDlgJisaCallSum")
				//	_break;
				//if(bShow)
				//	_break;
				//if((bShow && !bvis) || (!bShow && bvis))
				TRACE(sa);//OutputDebugStringA(sa);
				if (cl == "CDlgMapPlace" && sShow.Left(7) == "SW_SHOW")
					_break;
			}
		}
		else
			TRACE("################# window is NOT window.\n");
	}
	else
		TRACE("################# window is NULL.\n");
#endif // _DEBUG
}


long UcGetCtrlRect(CWnd * pParent, int idc, LPRECT lpRect)
{
	return UcGetCtrlRect(pParent, pParent->GetDlgItem(idc), lpRect);
}
long UcGetCtrlRect(CWnd * pParent, CWnd * pCtrl, LPRECT lpRect)
{
	if (pParent == NULL || pCtrl == NULL || !::IsWindow(pCtrl->GetSafeHwnd())) // 이게 release 에서 디버그없이 실행 
		return -1;
	pCtrl->GetWindowRect(lpRect);
#ifdef _DEBUG
	CWnd* ppr = pCtrl->GetParent();
	if (ppr != pParent)/// 바로 위 parent가 아닐 수 있다. 조상 몇대 고조 윈도우 일수 있다.
		_break;
#endif // _DEBUG
	pParent->ScreenToClient(lpRect);
	long lStyleOld = GetWindowLong(pParent->GetSafeHwnd(), GWL_STYLE);
	return lStyleOld;
}


void UcMoveCtrl(CWnd * wparent, CWnd * ctrl, int cx, int cy)
{
	CRect rcw, rcc;
	UcGetCtrlRect(wparent, ctrl, rcc);
	rcc.left += cx;
	rcc.right += cx;
	rcc.top += cy;
	rcc.bottom += cy;
	ctrl->MoveWindow(rcc);
	// 	wparent->GetWindowRect(rcw);
	// 	wparent->GetClientRect(rcc);

}
void UcMoveCtrl(CWnd * wparent, UINT idc, int cx, int cy)
{
	CWnd* ctrl = wparent->GetDlgItem(idc);
	UcMoveCtrl(wparent, ctrl, cx, cy);
}





#ifdef _UseBaseClassInpokable_
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////

//#ifdef _KWLIB_INVOKABLE_EX/// 삭제 금지: 테스트 프로젝트에서 사용

IMPLEMENT_DYNAMIC(CFormInvokable, CFormView)

BEGIN_MESSAGE_MAP(CFormInvokable, CFormView)
	ON_MESSAGE(WM_USER_INVOKE, &CFormInvokable::OnBeginInvoke)//?beginInvoke 2
	ON_WM_TIMER() //?LbTimer 4
END_MESSAGE_MAP()

//#ifdef _DEBUG
LRESULT CFormInvokable::OnBeginInvoke(WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = 0;
	KBeginInvoke* pbi = (KBeginInvoke*)lParam;
	if (GetSafeHwnd() && pbi) {
		pbi->_bCalled = true;
		if (IsRealWindow(this)) {
			if (pbi->m_lambda)
				(pbi->m_lambda)(pbi);
			else
				lr = (pbi->m_pLambdaSend)(pbi);
			if (IsRealWindow(this)) {
				if (pbi->m_lambdaFinish)
					pbi->m_lambdaFinish(pbi);
			}
		}
		else
			_break;
	}
	else
		_break;
	KBeginInvoke::freeInvokeFree();
	return lr;
}

//LRESULT CFormInvokable::OnBeginInvoke(WPARAM wParam, LPARAM lParam)
//{
//	KBeginInvoke* pbi = (KBeginInvoke*)lParam;
//	pbi->_bCalled = true;
//	LRESULT lr = 0;
//	if (IsRealWindow(this))
//	{
//		if (pbi->m_lambda)
//			(pbi->m_lambda)(pbi);
//		else
//			lr = (pbi->m_pLambdaSend)(pbi);
//		if(pbi->m_lambdaFinish)
//			pbi->m_lambdaFinish(pbi);
//	}
//	else
//		_break;
//	KBeginInvoke::freeInvokeFree();//Lock
//	return lr;
//}
//#else
//OnBeginInvoke_Define(CFormInvokable)//?beginInvoke 3
//#endif

CFormInvokable::CFormInvokable(UINT nIDTemplate)
	: CFormView(nIDTemplate)//아들이 할아버지 객체 것을 바로 부를수 없어서 아버지가 전달 해 줌.
	, KLambdaTimer(this) //?LbTimer 2
{
}
CFormInvokable::~CFormInvokable()
{
}
void CFormInvokable::OnTimer(UINT_PTR nIDEvent)//?LbTimer 5
{
	DoTimerTask(nIDEvent);

	CFormView::OnTimer(nIDEvent);
}
#endif // _UseBaseClassInpokable_

#ifdef _DEBUG_example
void CThreadCopy3View::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	vector<tuple<int, EOnSize, int, int>> artp = {
		{ IDC_ErrFiles, eMaxWidth, 4, 4 },//가로 간격 4에 최대로
		{ IDC_WrittenFiles, eWidth, 0, 0 },//커진 cx 만큼 w가 커짐(결과적으로 eMaxWidth와 같은 경우)
		{ IDC__Memo, eMaxBoth, 4, 4 },//가로세로 간격 4에 최대로
		//    { idc_control    , 작동옵션  , 마진x,마진y},
		// 마진은 0이면 벽에 닫고, 벽과 간격. -이면 넘어서 끝이 가려진다.
	};
	OnSizeAdjust(nType, cx, cy, artp);
}
#endif // _DEBUG_example

BOOL CSizeAdjustable::OnSizeAdjust(UINT nType, int cx, int cy, vector<vector<int>> artp)
{
	CWnd* thsWnd = dynamic_cast<CWnd*>(this);
	if (!thsWnd || !::IsWindow(thsWnd->GetSafeHwnd()))
		return FALSE;

	if (_tikStart == 0)
		_tikStart = GetTickCount64();
	auto tik = GetTickCount64();

	CRect rcc;
	thsWnd->GetClientRect(rcc);
	CSize wh = rcc.Size();
	if (cx == 0 || cy == 0)
	{
		//m_sz = CSize(0, 0);//초기화
		return FALSE;
	}
	else if (m_sz == CSize(0, 0))
	{
		m_sz = wh;//처음 이면, 원래 크기로
		return FALSE;
	}

	if ((tik - _tikStart) < 2000)
		return FALSE;

	KAtEnd defer([this, wh, tik]() {
		m_sz = wh;
		//TRACE("save - m_sz(%4d,%4d) <- wh\n", m_sz.cx, m_sz.cy);
		_tikPrev = tik;
		});
	BOOL bPrev = m_sz.cx > 0 && m_sz.cy > 0;
	if (SIZE_MINIMIZED != nType)// && m_sz.cx > 0 && m_sz.cy > 0)
	{
		CSize ds = CSize(cx - m_sz.cx, cy - m_sz.cy);//현재 크기에서 이전 크기를 빼서 변화한 크기 도출
		//TRACE("m_sz(%4d,%4d) ds: %4d,%4d\n", m_sz.cx, m_sz.cy, ds.cx, ds.cy);

		auto nCtrl = artp.size();
		for (int i = 0; i < (int)nCtrl; i++)
		{
			int idc = artp[i][0];
#ifdef _DEBUG
			if (idc == 1014)
			{
				//TRACE("1001 h:%d\n", rc.Height());
				_break;
			}
#endif // _DEBUG
			CWnd* pCtrl = thsWnd->GetDlgItem(idc);
			if (!IsRealWindow(pCtrl))
				continue;

			EOnSize op = (EOnSize)artp[i][1];
			CSize ms(artp[i][2], artp[i][3]); // margin

			CRect rc;
			UcGetCtrlRect(thsWnd, idc, rc);//현재 컨트롤 크기 rc
#ifdef _DEBUG
			if (idc == 1014 && rc.Height() <= 1)
			{
				//TRACE("1001 h:%d\n", rc.Height());
				_break;
			}
#endif // _DEBUG
			CRect rcbu(rc);
			//TRACE(">> %u: %4d,%4d, %4d,%4d\n", idc, rc.left, rc.top, rc.Width(), rc.Height());
			if (op & eWidth)
			{
				if (op & eMaxX)
					rc.right = rcc.right - ms.cx;
				else if (op & eLeft)// 아래쪽으로 커진 만큼 이동
				{
					ASSERT(artp[i].size() >= 5);
					auto idDn = artp[i][4]; // idDn 의 윗쪽 
					auto spc = artp[i][5]; // 간격
					CRect rcd;
					UcGetCtrlRect(thsWnd, idDn, rcd);//현재 컨트롤 크기 rc
					rc.right = rcd.left + spc;
				}
				else
					if (bPrev)
						rc.right += ds.cx;//우측 끝에서 8마진으로
			}
			else // no eWidth이면, 크기는 유지
			{
				if (op & eMaxX)// 우측끝으로 이동
				{
					rc.right = rcc.right - ms.cx;
					rc.left = rc.right - rcbu.Width();
				}
				else if (op & eHorz)// 아래쪽으로 커진 만큼 이동
				{
					rc.left += ds.cx;
					rc.right += ds.cx;
				}
			}

			if (op & eHeight)
			{
				if (op & eMaxY)
				{
					rc.bottom = rcc.bottom - ms.cy;
				}
				else if (op & eTop)// 아래쪽으로 커진 만큼 이동
				{
					ASSERT(artp[i].size() >= 5);
					auto idDn = artp[i][4]; // idDn 의 윗쪽 
					auto spc = artp[i][5]; // 간격
					CRect rcd;
					UcGetCtrlRect(thsWnd, idDn, rcd);//현재 컨트롤 크기 rc
					//rc.top += ds.cy;
					rc.bottom = rcd.top + spc;
				}
				else if (op & eBottom)// 아래쪽으로 커진 만큼 이동
				{
					ASSERT(artp[i].size() >= 5);
					auto idDn = artp[i][4]; // idDn 의 윗쪽 
					auto spc = artp[i][5]; // 간격
					CRect rcd;
					UcGetCtrlRect(thsWnd, idDn, rcd);//현재 컨트롤 크기 rc
					//rc.top += ds.cy;
					rc.bottom = rcd.bottom + spc;
				}
				else if (bPrev)
				{
					if (idc == 1001)
						TRACE(L"IDC_FileList: TB(%3d, %3d), h(%3d) dsy(%3d)\n", rc.top, rc.bottom, rc.Height(), ds.cy);

					rc.bottom += ds.cy;
				}
				//rc.bottom += ms.cy;
			}
			else // no eWidth이면, 크기는 유지
			{
				if (op & eMaxY)// 아래끝으로 이동
				{
					rc.bottom = rcc.bottom - ms.cy;
					rc.top = rc.bottom - rcbu.Height();
				}
				else if (op & eVert)// 아래쪽으로 커진 만큼 이동
				{
					rc.top += ds.cy;
					rc.bottom += ds.cy;
				}
			}

			if (rc.Width() > 0 && rc.Height() > 0)
				thsWnd->GetDlgItem(idc)->MoveWindow(rc);
			//TRACE("<< %u: %4d,%4d, %4d,%4d\n", idc, rc.left, rc.top, rc.Width(), rc.Height());
		}
	}

	if (SIZE_MINIMIZED != nType)
	{
		m_sz.cx = cx;
		m_sz.cy = cy;
	}
	return TRUE;
}












#ifdef _UseBaseClassInpokable_

/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////

//#ifdef _KWLIB_INVOKABLE_EX/// 삭제 금지: 테스트 프로젝트에서 사용

IMPLEMENT_DYNAMIC(CMDIFrameWndInvokable, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMDIFrameWndInvokable, CMDIFrameWnd)
	ON_MESSAGE(WM_USER_INVOKE, &CMDIFrameWndInvokable::OnBeginInvoke)//?beginInvoke 2
	ON_WM_TIMER() //?LbTimer 4
END_MESSAGE_MAP()


OnBeginInvoke_Define(CMDIFrameWndInvokable)//?beginInvoke 3

CMDIFrameWndInvokable::CMDIFrameWndInvokable() noexcept
	: KLambdaTimer(this) //?LbTimer 2
{
}
void CMDIFrameWndInvokable::OnTimer(UINT_PTR nIDEvent)//?LbTimer 5
{
	DoTimerTask(nIDEvent);

	CMDIFrameWnd::OnTimer(nIDEvent);
}

/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////

//#ifdef _KWLIB_INVOKABLE_EX/// 삭제 금지: 테스트 프로젝트에서 사용

IMPLEMENT_DYNAMIC(CWndInvokable, CWnd)

BEGIN_MESSAGE_MAP(CWndInvokable, CWnd)
	ON_MESSAGE(WM_USER_INVOKE, &CWndInvokable::OnBeginInvoke)//?beginInvoke 2
	ON_WM_TIMER() //?LbTimer 4
END_MESSAGE_MAP()


OnBeginInvoke_Define(CWndInvokable)//?beginInvoke 3

CWndInvokable::CWndInvokable() noexcept
	: KLambdaTimer(this) //?LbTimer 2
{
}
void CWndInvokable::OnTimer(UINT_PTR nIDEvent)//?LbTimer 5
{
	DoTimerTask(nIDEvent);

	CWnd::OnTimer(nIDEvent);
}


/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
CDlgInvokable::CDlgInvokable()
	: CDialog()
	, KLambdaTimer(this)//?LbTimer 2
{
}
CDlgInvokable::CDlgInvokable(UINT nIDTemplate, CWnd * pParent)
	: CDialog(nIDTemplate, pParent), KLambdaTimer(this)
{
}
CDlgInvokable::CDlgInvokable(LPCTSTR lpszTemplateName, CWnd * pParentWnd)
	: CDialog(lpszTemplateName, pParentWnd), KLambdaTimer(this)
{
}

IMPLEMENT_DYNAMIC(CDlgInvokable, CDialog)

BEGIN_MESSAGE_MAP(CDlgInvokable, CDialog)
	ON_MESSAGE(WM_USER_INVOKE, &CDlgInvokable::OnBeginInvoke)//?beginInvoke 2
	ON_WM_TIMER() //?LbTimer 4
END_MESSAGE_MAP()

//#ifdef _DEBUG
LRESULT CDlgInvokable::OnBeginInvoke(WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = 0;
	KBeginInvoke* pbi = (KBeginInvoke*)lParam;
	if (GetSafeHwnd() && pbi) {
		pbi->_bCalled = true;
		if (IsRealWindow(this)) {
			if (pbi->m_lambda)
				(pbi->m_lambda)(pbi);
			else
				lr = (pbi->m_pLambdaSend)(pbi);
			if (IsRealWindow(this)) {
				if (pbi->m_lambdaFinish)
					pbi->m_lambdaFinish(pbi);
			}
		}
		else
			_break;
	}
	else
		_break;
	KBeginInvoke::freeInvokeFree();
	return lr;
}
//#else
//OnBeginInvoke_Define(CDlgInvokable)//?beginInvoke 3
//#endif // _DEBUG



void CDlgInvokable::OnTimer(UINT_PTR nIDEvent)//?LbTimer 5
{
	DoTimerTask(nIDEvent);

	CDialog::OnTimer(nIDEvent);
}

/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////
CDlgExInvokable::CDlgExInvokable()
	: CDialogEx()
	, KLambdaTimer(this)//?LbTimer 2
{
}
CDlgExInvokable::CDlgExInvokable(UINT nIDTemplate, CWnd * pParent)
	: CDialogEx(nIDTemplate, pParent), KLambdaTimer(this)
{
}
CDlgExInvokable::CDlgExInvokable(LPCTSTR lpszTemplateName, CWnd * pParentWnd)
	: CDialogEx(lpszTemplateName, pParentWnd), KLambdaTimer(this)
{
}

IMPLEMENT_DYNAMIC(CDlgExInvokable, CDialogEx)

BEGIN_MESSAGE_MAP(CDlgExInvokable, CDialogEx)
	ON_MESSAGE(WM_USER_INVOKE, &CDlgInvokable::OnBeginInvoke)//?beginInvoke 2
	ON_WM_TIMER() //?LbTimer 4
END_MESSAGE_MAP()

#ifdef _DEBUG
LRESULT CDlgExInvokable::OnBeginInvoke(WPARAM wParam, LPARAM lParam)
{
	KBeginInvoke* pbi = (KBeginInvoke*)lParam;
	pbi->_bCalled = true;
	LRESULT lr = 0;
	if (IsRealWindow(this))
	{
		if (pbi->m_lambda)
			(pbi->m_lambda)(pbi);
		else
			lr = (pbi->m_pLambdaSend)(pbi);
		if (pbi->m_lambdaFinish)
			pbi->m_lambdaFinish(pbi);
	}
	else
		_break;
	KBeginInvoke::freeInvokeFree();
	return lr;
}
#else
OnBeginInvoke_Define(CDlgExInvokable)//?beginInvoke 3
#endif // _DEBUG



void CDlgExInvokable::OnTimer(UINT_PTR nIDEvent)//?LbTimer 5
{
	DoTimerTask(nIDEvent);

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT CDlgExInvokable::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = CDialogEx::WindowProc(message, wParam, lParam);
	return lResult;
}

#endif // _UseBaseClassInpokable_


/// 특히 백그라운드에서 이미지 다운로드 하기 직전 이것으로 시작 한다.
/// bSafe: 다운로딩중 에러가 나서 락이 안풀리는걸 방지. 15초 후에 자동으로 풀린다.
/// 리턴 받은 'shLock'을 반드시 람다 캡쳐 파라미터에 넘겨 줘야 그 람다 안에서 lock이 풀린다.
/// 안 넘겨 주면 ~KLife 되는 위치에서 LongTaskFinished(UnLock)되어 버린다.
/// shared_ptr<> 는 람다로 넘겨서 현재 로컬을 끝나도, 비동기 람다 함수 안에까지 수명이 연장 되는 점을 이용하여 destructor

SHP<KLongTaskDoing::KLife> KLongTaskDoing::LongTaskStartedRaw(LPCSTR fnc, LPCSTR key0, BOOL bSafe, BOOL bGuid)
{
	//_lockImgDowning.Lock();
	auto shCS = make_shared<CCriticalSection>();// _csImgDowning;//
	auto shLock = make_shared<CSingleLock>(shCS.get(), FALSE);
	shLock->Lock();// 20000);//ASSERT(dwTimeout == INFINITE); 로 timeout을 줄수 없다. 왜그러지?

	if (key0 == NULL)
		key0 = fnc;// fnc에 키만 달랑 올때
	CStringA keyA;
	if (bGuid)//tchsame(fnc, key0))//같으면 __FUNCTION__ 을 키로 쓰므로 뒤에 GUID를 붙인다.
	{
		CStringA sguid((LPCTSTR)UcGetFormattedGuid());
		keyA.Format("%s__%s", key0, sguid.GetString());
	}
	else
		keyA = key0;//직접 키를 준 경우. 단독으로 락을 체크 하고 싶을때.
	string key = (LPCSTR)keyA;
	//	TRACE("\t\t########### LongTaskStartedRaw. %s <= %s\n", key.c_str(), fnc);

	if (bSafe)//위에서 timeout을 20초 줄수 있었다면 굳이 타이머로 unlock 할필요 있나
		SafeDownloading(key);/// timeout되면 자동 unlock

#ifdef _DEBUG
	if (_mapLongTaskingLock.find(key) != _mapLongTaskingLock.end())
		TRACE("\t\t\t########### LongTaskStartedRaw KEY EXISTS. %s <= %s\n", key.c_str(), fnc);
#endif // _DEBUG
	auto tpInfo = make_tuple(fnc, key, shCS, shLock);
	_mapLongTaskingLock[key] = tpInfo;//단지 보관을 위해

	auto shLife = make_shared<KLongTaskDoing::KLife>(this, tpInfo);// fnc, key, shCS, shSLock);// _lockImgDowning);
	return shLife;
}

/// 이미지 다운로드가 끝나고 또는 출력(foreground)까지 마친경우 모든 이미지 처리 까지 끝난 경우 부른다.

void KLongTaskDoing::LongTaskFinished(string key, BOOL bTimeout)
{
	TPLife shSLock;
	if (MapLookup(_mapLongTaskingLock, key, shSLock))
	{
		//		TRACE("\t\t########### LongTaskFinished. %s %s\n", key.c_str(), bTimeout ? "TIMEOUT" : "");
		auto lock = std::get<3>(shSLock);
		if (lock->IsLocked())
		{
			lock->Unlock();
			_mapLongTaskingLock.erase(key);
		}
	}
	else
	{
		if (!bTimeout)
			TRACE("\t\t\t########### LongTaskFinished. %s %s\n", "KEY NOT FOUND", key.c_str());
	}
}

BOOL KLongTaskDoing::IsLongTaskDoing(LPCSTR pkey)
{
	string key;
	if (pkey) // 중복 콜 방지로 쓸때
	{
		TPLife shSLock;
		if (MapLookup(_mapLongTaskingLock, string(pkey), shSLock))
		{
#if CPP17_OR_LATER
			auto& [fnc, key, csection, lock] = shSLock;
#else
			auto& fnc = std::get<0>(shSLock);
			auto& key = std::get<1>(shSLock);
			auto& csection = std::get<2>(shSLock);
			auto& lock = std::get<3>(shSLock);
#endif
			auto bLod = lock->IsLocked();
			if (bLod)
				TRACE("image(%s) is still loading from %s\n", pkey, fnc.c_str());
			return bLod;
		}
	}
	else // 다운로딩중인것이 하나라도 있으면 TRUE
	{
		for (auto& it : _mapLongTaskingLock)
		{
			auto shSLock = it.second;
#if CPP17_OR_LATER
			auto& [fnc, key, csection, lock] = shSLock;
#else
			auto& fnc = std::get<0>(shSLock);
			auto& key = std::get<1>(shSLock);
			auto& csection = std::get<2>(shSLock);
			auto& lock = std::get<3>(shSLock);
#endif
			auto bLod = lock->IsLocked();
			if (bLod)//아직 다운로딩 중인거 처음 발견하자마자
			{
				TRACE("image(%s) is still loading from %s\n", key.c_str(), fnc.c_str());
				return TRUE;
			}
		}
	}
	return FALSE;
	/* ex:
	void CDlgJoinUs::OnClose()	{
		if(!IsLongTaskDoing())
			CICDialog::OnClose();
	}
	*/
}

/// 알수 없는 사태 일때 , CSingleLock이 timeout을 지정할수 없으므로
/// 15초가 지나도록 다운로드가 안끝났다면, 강제로 Unlock을 해야지. 무슨 문제가 있을지 모르니.
void KLongTaskDoing::SafeDownloading(string key)
{
	auto* tmr = dynamic_cast<KLambdaTimer*>(this); //ASSERT(tmr);
	if (tmr)
	{
		string key1 = key + "_image_downloading";
		tmr->SetTimerLambda(key1.c_str(), 15000, [this, key](auto)-> void
			{
				//				TRACE("\t\t\t########### image loading timeout. %s\n", key.c_str());
				LongTaskFinished(key, TRUE);//?async image download
			}, 1);
	}
}
/// <summary>
/// UcTool.cpp에서 옮겨옴
/// </summary>
UCTOOLDYNAMIC
void UcPostMessageBoxError(LPCWSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CStringW sMsg = UcFormatStringFromArgs(fmt, args);
	va_end(args);
	PostMainTaskSelf(UcGetMainCWnd(), [sMsg](auto) {
		UcMessageBoxError(sMsg);
		});
}
#ifdef _UseBaseClassInpokable_
#ifdef _UseOnWndMsg
#ifdef _DEBUG
BOOL CDlgInvokable::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT * pResult)
{
	BOOL bRv = TRUE;
	try {
		UcDebugWindowInfo(this, message, wParam, lParam);
		bRv = CDialog::OnWndMsg(message, wParam, lParam, pResult);
	}
	catch (CException* e)
	{
		auto rc = e->GetRuntimeClass(); CStringW sc(rc->m_lpszClassName);
		auto ke = dynamic_cast<KException*>(e);//s_fncExceptionDealer >> UcWriteLog >> UcWriteException
		if (ke == NULL)
		{
			TRACE(L"Uncaught CException:%s", sc.GetString());
			ASSERT("Uncaught CException" == nullptr);
		}
		else
			_break;	// KException으로 모두 처리 함
	}
	catch (const std::exception& e) {
		//exception 도 모두 KException으로 바뀌니 문제가 없다.
		VERIFY(e.what() == nullptr);
	}
	catch (...)
	{
		ASSERT("..." == nullptr);
	}
	return bRv;
}
BOOL CDlgExInvokable::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT * pResult)
{
	BOOL bRv = TRUE;
	try {
		UcDebugWindowInfo(this, message, wParam, lParam);
		bRv = CDialogEx::OnWndMsg(message, wParam, lParam, pResult);
	}
	catch (CException* e)
	{
		/// CGlobalSettings::SaveGlobalSettings에서 테스트
		auto rc = e->GetRuntimeClass(); CStringW sc(rc->m_lpszClassName);
		auto ke = dynamic_cast<KException*>(e);//s_fncExceptionDealer >> UcWriteLog >> UcWriteException
		if (ke == NULL)
		{
			TRACE(L"Uncaught CException:%s", sc.GetString());
			ASSERT("Uncaught CException" == nullptr);
		}
		else
			_break;	// KException으로 모두 처리 함
	}
	catch (const std::exception& e) {
		//exception 도 모두 KException으로 바뀌니 문제가 없다.
		VERIFY(e.what() == nullptr);
	}
	catch (...)
	{
		std::exception_ptr p = std::current_exception();
		ASSERT(p);
		ASSERT("..." == nullptr);
	}
	return bRv;
}
BOOL CFormInvokable::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT * pResult)
{
	BOOL bRv = TRUE;
	try {
		UcDebugWindowInfo(this, message, wParam, lParam);
		bRv = CFormView::OnWndMsg(message, wParam, lParam, pResult);
	}
	catch (CException* e)
	{
		//CFileException::ThrowOsError((LONG)::GetLastError(), sFull);
		auto rc = e->GetRuntimeClass(); CStringW sc(rc->m_lpszClassName);
		auto ke = dynamic_cast<KException*>(e);//s_fncExceptionDealer >> UcWriteLog >> UcWriteException
		if (ke == NULL)
		{
			TRACE(L"Uncaught CException:%s", sc.GetString());
			ASSERT("Uncaught CException" == nullptr);
		}
		else
			_break;	// KException으로 모두 처리 함
	}
	catch (const std::exception& e) {
		//exception 도 모두 KException으로 바뀌니 문제가 없다.
		VERIFY(e.what() == nullptr);
	}
	catch (...)
	{
		ASSERT("..." == nullptr);
	}
	return bRv;
}
#else
OWNDMSG(CDialog, CDlgInvokable);
OWNDMSG(CDialogEx, CDlgExInvokable);
OWNDMSG(CFormView, CFormInvokable);
#endif // _DEBUG
OWNDMSG(CMDIFrameWnd, CMDIFrameWndInvokable);
OWNDMSG(CWnd, CWndInvokable);

#endif // _DEBUG
#endif // _UseBaseClassInpokable_

#if CPP_BEFORE_17
// UcWndInvokable.h의 INLINE_STATIC 멤버들 정의
CUcCriticalSection CPostMainTaskHelper::s_csGarbage;
KPtrList<CPostMainTaskHelper::LambdaData>* CPostMainTaskHelper::s_pGarbageList = nullptr;
WORD CPostMainTaskHelper::s_srl = 0;
#endif

#ifndef REMIND_PostMainTask_
#define REMIND_PostMainTask_
DWKREMINDER("여기 PostMainTaskSelf 관련 샘플")
#endif

#ifdef _Usage_PostMainTask_LambdaTimer
/// 사용법: C:\Dropbox\Proj\CmnJ\UcTool\UcWndInvokable.h  UcTool은 링크
***********************************************************************
1. 헤더 파일에 #include "UcWndInvokable.h" 추가
#include "UcTool/UcWndInvokable.h"

2 - A.WindowProc을 override하지 않은 경우 :
class CMyDialog : public CDialog
{
public:
	/// 이걸 해줌 으로써 WindowProc에서 WM_USER_INVOKE 메시지를 받을수 있게 됩니다. 
	/// 1. PostMainTaskSelf([this]() { ... }); 를 이용할 수 있게 됩니다.
	/// 
	/// 이걸 해줌 으로써 WindowProc에서 WM_TIMER 메시지를 받을수 있게 됩니다. 
	/// 2. SetTimerLambda(HWND...) 를 이용할 수 있게 됩니다.
	OVERRIDE_WINDOWPROC_FOR_ALL(CDialog)  // 매크로 한 줄로 추가!

		void DoWork()
	{
		// 함수 사용 (디버깅 가능)
		PostMainTaskSelfSimple([this]() {
			SetDlgItemText(IDC_STATUS, L"작업 완료!");
			});

		// 또는 매크로 사용 (디버깅 어려움)
		// POST_MAIN_TASK_SELF([this]() {
		//     SetDlgItemText(IDC_STATUS, L"작업 완료!");
		// });
	}
};


2 - B.이미 WindowProc을 override하고 있는 경우 :
class CMyDialog : public CDialog
{
public:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override
	{
		ADD_POSTMAINTASK_TO_WINDOWPROC();  /// 기존 코드에 한 줄만 추가!
		// 기존 메시지 처리 코드들...
		switch (message)
		{
		case WM_CREATE:	// 기존 처리
			break;
		case WM_DESTROY:// 기존 처리
			break;
		}
		return CDialog::WindowProc(message, wParam, lParam);
	}
};

3. 사용 예제 : 사용법
// CWnd 계열 클래스에서 사용 (this 자동)
PostMainTaskSelf([this](auto) {
	SetDlgItemText(IDC_STATUS, L"작업 완료!");
	});
// 일반 클래스에서 사용 (원하는 윈도우 선택)
PostMainTaskSelf(AfxGetMainWnd(), [this]() {
	// 메인 윈도우에 PostMessage
	});
PostMainTaskSelf(m_pDialog, [this](auto) {
	// 특정 다이얼로그에 PostMessage
	});

// 복잡한 람다 (디버깅 필요시)
PostMainTaskSelf([this](auto) {
	// 복잡한 로직...
	SetDlgItemText(IDC_STATUS, L"작업 완료!");
	});

// 동기 실행 : 리턴값이 필요한 경우
LRESULT result = SendMainTaskSelf([this](auto) -> LRESULT {
	return MessageBox(L"계속하시겠습니까?", L"확인", MB_YESNO);
	});

/// 매크로 사용도 가능하지만 디버깅이 어려움
// POST_MAIN_TASK_SELF([this]() { ... });
// SEND_MAIN_TASK_SELF([this]() -> LRESULT { ... });

최소한의 추가 코드 :
-CMainFrame에만 : OVERRIDE_WINDOWPROC_FOR_ALL(CFrameWnd) 매크로 한 줄
- CMainFrame에 하면, 다른 모든 클래스에서 WindowProc override 할필요 없다.

사용법 :
	///	PostMainTask: 
	PostMainTaskSelf(AfxGetMainWnd(), [this]() { ... });
/// LambdaTimer (권장): 타이머처리 윈도우는 MainFrame에서 한다.

#ifdef _TimerSamples_ //dwk: 2026-04-20 17:38 
class KLambdaTimer;// foward declaration

SHP<KLambdaTimer> _timer;//멤버 pointer 선언

_timer = make_shared<KLambdaTimer>(this);//이건 윈도우 초기 단계에서 OnCreate, OnInitDialog 같은데서 만들어 진다고 가정.

_timer->SetTimerLambda("_progr", 250, [&](auto) {
	// do something repeatedly every 250ms
	});

_timer->DelayAndRunOnce(__FUNCTION__, 10_sec,
	[this, sFileOnly, sAction, sExt](auto) {
		SomeAutoCommitAtBuildDelay(sFileOnly, sAction, sExt);
	});
#endif // _TimerSamples_

//SetTimerLambda(AfxGetMainWnd(), "test", 1000, [this](auto) { ... });
///// LambdaTimer (편의): 
//SetTimerLambda("test", 1000, [this](auto) { ... });
///// LambdaTimer (기존): KLambdaTimer timer(AfxGetMainWnd()); timer.SetTimerLambda("test", 1000, [this](auto) { ... });
///// 특정 윈도우: 
//	PostMainTaskSelf(m_pDialog, [this](auto) { ... });
///// HWND 직접: 
//	PostMainTask(hwnd, [this](auto) { ... });

장점:
-CFormInvokable, CDlgInvokable 등 특별한 클래스 상속 불필요 : [[deprecated]]
- 메인 윈도우에만 한 번만 설정하면 어디서든 사용 가능
- PostMainTask와 LambdaTimer 모두 지원
- 원하는 윈도우를 선택해서 PostMessage 가능
- 기존 PostMainTask와 동일한 사용법
- 자동 메모리 관리(메모리 누수 방지)
- 스레드 안전
- 디버깅 지원(함수 사용 시)

기존 방식과 비교 :
기존: class CMyDialog : public CDlgInvokable  // 각 클래스마다 상속 필요
새로운 : CMainFrame에만 한 번 설정 → 어디서든 PostMainTaskSelf(AfxGetMainWnd(), ...) 사용!
#endif // _Usage_PostMainTask_LambdaTimer

