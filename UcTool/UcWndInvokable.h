#pragma once

#define UCWNDINVOKABLE
#include <afxdialogex.h>

#include <functional> // std::function<>

#include "UcBaseTools.h"
#include "UcTool.h"
#include "UcWindow.h"
#include "UcTimeTools.h"
#include "UcThreadTool.h"

#define _UseOnWndMsg
#define WM_USER_INVOKE (WM_USER+0x4140)
#ifndef WM_USER_BEGININVOKE
#define WM_USER_BEGININVOKE WM_USER_INVOKE
#endif

/// PostMessage뿐만 아니라 SendMessage에도 쓰인다.
class KBeginInvoke
{
public:
	//HWND m_hwnd; , m_hwnd(hwnd)
#ifdef _UseSharedFunc_
	shared_ptr<std::function<void(LPVOID)>> m_lambda;
	shared_ptr<std::function<void(LPVOID)>> m_lambdaFinish;
	shared_ptr<std::function<LRESULT(LPVOID)>> m_pLambdaSend;
#else
	function<void(LPVOID)> m_lambda;
	function<void(LPVOID)> m_lambdaFinish;
	function<LRESULT(LPVOID)> m_pLambdaSend;
#endif // _UseSharedFunc_
	bool _bCalled{ false };
	WPARAM _srl{ 0 };
	CStringA _note;
	CStringA m_fnc;
	int m_line{ 0 };
	ULONGLONG _tik{ 0 };
	HWND _hwnd{ NULL };
	//bool _bSend{ false };

	KBeginInvoke(std::function<void(LPVOID)> lambda, LPCSTR fnc = NULL, int line = 0, function<void(LPVOID)> lmdaFinish = NULL);
	KBeginInvoke(std::function<LRESULT(LPVOID)> lambda, LPCSTR fnc = NULL, int line = 0);
	~KBeginInvoke()
	{
		// 		if(m_lambda) delete m_lambda;
		// 		else if(m_pLambdaSend) delete m_pLambdaSend;
	}
	static WORD s_srl;
	/// BeginInvoke가 동시에 여러 스레드에서 되는건 gabage free에 위험 하다.
	static CUcCriticalSection _csGabage;//트랜잭션 있는 함수 연거푸 불려 질때 트랜잭션이 중첩 된다.
	static CUcCriticalSection _csSendMsg;//트랜잭션 있는 함수 연거푸 불려 질때 트랜잭션이 중첩 된다.

	//static CUcCriticalSection* GetCSXX(int mode = 0)
	//{
	//	if (mode == 0)
	//		return &_csGabage;
	//	else if (mode == 1)
	//		return &_csSendMsg;
	//	ASSERT(0);
	//}
	///?변경
	/// Invoke때 할당한 게 Post 받아서 읿을  수도 있으니, gabage를 프리 하는 방법을 쓴다.
	/// static auto free array를 쓴다.
	static void setInvokeFree(KBeginInvoke* pbi);
	static void freeInvokeFree();
	static KPtrList<KBeginInvoke>* getGabage()
	{
		static KPtrList<KBeginInvoke> s_list;
		return &s_list;
	}

	// 	template<typename Func>
	// 	static void Begin(CWnd* pWnd, Func lmda)
	// 	{
	// 		// lmda가 로컬 람다 변수 이므로 힙에 복제 해서 전달 해야 한다. 비동기 이므로 스택에서 사라 진다.
	// 		KBeginInvoke* pbi = new KBeginInvoke()
	// 			std::function<void(LPVOID)> *pLambda = new std::function<void(LPVOID)>(lmda);
	// 		::PostMessage(pWnd->GetSafeHwnd(), WM_USER_INVOKE, 0, (LPARAM)pLambda);
	// 	}
};

// 2024-03-16 (*pbi->m_lambda)() 에서 '*' 제거
#define OnBeginInvoke_DefineEx(clss, bVisible) \
LRESULT clss::OnBeginInvoke(WPARAM wParam, LPARAM lParam)\
{	\
	LRESULT lr = 0;\
	if (GetSafeHwnd()) {\
		KBeginInvoke* pbi = (KBeginInvoke*)lParam;\
		pbi->_bCalled = true; \
		if(IsRealWindow(this, bVisible)){\
			if(pbi->m_lambda) (pbi->m_lambda)(pbi);\
			else         lr = (pbi->m_pLambdaSend)(pbi);\
			if(pbi->m_lambdaFinish) pbi->m_lambdaFinish(pbi);\
	}	}\
	KBeginInvoke::freeInvokeFree();\
	return lr;\
}
#define OnBeginInvoke_Define(clss) OnBeginInvoke_DefineEx(clss, FALSE)
//auto pLambda = (std::function<void()LPVOID>*)lParam; delete pbi;

///?주의: 람다식에서 [&] 는 괜찮은데, [&,val] 처럼 더 들어 가는 경우 람다식 전체를 가로로 더 묶어 줘야 한다.
///		이렇게 매크로 사용하면, 람다식 내부에서 line trace debug가 안된다.
// #define KwBeginInvoke(wnd, lmda) PostMainTask((wnd), (lmda), __FUNCTION__, __LINE__)
// #define KwSendInvoke(wnd, lmda) SendMainTask((wnd), (lmda), __FUNCTION__, __LINE__)
// #define KwBeginInvokeNt(wnd, lmda, note) PostMainTask((wnd), (lmda), __FUNCTION__, __LINE__, note)
// #define KwSendInvokeNt(wnd, lmda, note) SendMainTask((wnd), (lmda), __FUNCTION__, __LINE__, note)

/// @deprecated 람다 매크로는 디버깅이 어렵습니다. PostMainTask / SendMainTask 를 직접 호출하세요.
///   PostMainTask(wnd, [captures]() { ... }, __FUNCTION__, __LINE__);
///   SendMainTask(wnd, [captures](LPVOID) -> LRESULT { ... return 0; }, __FUNCTION__, __LINE__);
#if defined(_MSC_VER)
#define UC_KW_INVOKE_DEPRECATE_MSG \
	__pragma(message(__FILE__ ": warning: Kw*Invoke macro is deprecated. Use PostMainTask/SendMainTask directly."))
#else
#define UC_KW_INVOKE_DEPRECATE_MSG
#endif

#define KwBeginInvoke(wnd, lmda) \
	UC_KW_INVOKE_DEPRECATE_MSG \
	PostMainTask((wnd), (lmda), __FUNCTION__, __LINE__)

#define KwSendInvoke(wnd, lmda) \
	UC_KW_INVOKE_DEPRECATE_MSG \
	SendMainTask((wnd), (lmda), __FUNCTION__, __LINE__)

#define KwBeginInvokeNt(wnd, lmda, note) \
	UC_KW_INVOKE_DEPRECATE_MSG \
	PostMainTask((wnd), (lmda), __FUNCTION__, __LINE__)

#define KwSendInvokeNt(wnd, lmda, note) \
	UC_KW_INVOKE_DEPRECATE_MSG \
	SendMainTask((wnd), (lmda), __FUNCTION__, __LINE__)

/*
* this: message날릴 윈도우 CWnd*
* i,sAstrR : 비동기라 스택에서 사라질 변수를 복사해 전달 한다.
	PostMainTask(this, [i, sAstrR]() {
		UiForAsync(i, sAstrR);
	}, __FUNCTION__, __LINE__);
*/

class UCTOOLDYNAMIC CSizeAdjustable
{
public:
	/// OnSize에서 늘릴때 따라 늘어 나는거 할떄 이전 크기 저장 해두는 거
	enum EOnSize {
		eWidth = 1 << 0,
		eHeight = 1 << 1, // size
		eSzBoth = (eWidth | eHeight),
		eHorz = 1 << 2, // move x
		eVert = 1 << 3, // move y
		eMvBoth = (eHorz | eVert),
		eMaxX = 1 << 4,
		eMaxY = 1 << 5,
		eTop = 1 << 6,
		eLeft = 1 << 7,
		eBottom = 1 << 8,
		eRight = 1 << 9,
		eMaxWidth = (eWidth | eMaxX),
		eMaxHeight = (eHeight | eMaxY),
		eMaxBoth = (eMaxWidth | eMaxHeight),
	};
	CSize m_sz{ 0, 0 };
	ULONGLONG _tikStart{ 0 };
	ULONGLONG _tikPrev{ 0 };
	virtual BOOL OnSizeAdjust(UINT nType, int cx, int cy, vector<vector<int>> artp);
};


template<typename TWND>
void UcDebugWindowInfo(TWND* pWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	if (message == WM_CREATE) // 마우스 가운데 버튼이 눌렸을 때
	{
		TCHAR className[256] = { 0 };
		::GetClassName(pWnd->m_hWnd, className, sizeof(className) / sizeof(TCHAR));
		CWnd* pw = static_cast<CWnd*>(pWnd);
		CString sText;
		pw->GetWindowText(sText);
		CString tp(typeid(*pWnd).name());
		if (tp.Left(6) == _T("class "))
			tp = tp.Mid(6);

		DWKFUNCV(L"WM_CREATE : %s - %s [%s]", tp, className, sText);
	}
	else if (message == WM_MBUTTONDOWN || message == WM_NCMBUTTONDOWN) // 마우스 가운데 버튼이 눌렸을 때
	{
		// Ctrl와 Alt 키의 상태를 확인
		if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000)) // Ctrl + Alt
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			::ClientToScreen(pWnd->m_hWnd, &pt); // 클라이언트 좌표를 스크린 좌표로 변환

			// 클릭된 위치의 윈도우 핸들 찾기
			HWND hClickedWnd = WindowFromPoint(pt);

			if (hClickedWnd)
			{
				TCHAR className[256] = { 0 };
				::GetClassName(hClickedWnd, className, sizeof(className) / sizeof(TCHAR));
				const CRuntimeClass* pClass = pWnd->GetRuntimeClass();
				if (pClass != nullptr)
				{
					CString sMessage;
					CString tp(typeid(*pWnd).name());
					if (tp.Left(6) == _T("class "))
						tp = tp.Mid(6);
					else if (tp.Left(7) == _T("struct "))
						tp = tp.Mid(7);
					sMessage.Format(_T("%s"), tp);
					UcCopyTextClipboad(CStringW(sMessage), NULL);
					//message.Format(_T("%s - %s - %s"), tp, pClass->m_lpszClassName, className);
					//auto sr = UcPrintStack();
					AfxMessageBox(sMessage);			// 메시지 박스로 클래스 이름 출력
				}
			}
		}
	}
#endif // _DEBUG
}


#ifdef _UseOnWndMsg

// KException으로 모두 처리 함
#define OWNDMSG(baseWType, WType) \
BOOL WType::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) {\
	BOOL bRv = TRUE;\
	if (UcOrAny(message, { WM_COMMAND, WM_CREATE }))\
	{	try {\
			UcDebugWindowInfo(this, message, wParam, lParam);\
			bRv = baseWType::OnWndMsg(message, wParam, lParam, pResult);\
		} catch (CException* e){\
		VERIFY(dynamic_cast<KException*>(e));}\
		catch (const std::exception& e) {	VERIFY(e.what() == nullptr); }\
		catch (...)	{ASSERT("..." == nullptr);}\
	} else {\
		bRv = baseWType::OnWndMsg(message, wParam, lParam, pResult);\
	}\
	return bRv;\
}
#ifdef _RefNote____ //참고
CFileException::ThrowOsError((LONG)::GetLastError(), L"TEST File Exception.exp");
throw std::exception("An TEST error has occurred");
throwLINE;
#endif // _DEBUG

#endif // _UseOnWndMsg



#ifdef _UseBaseClassInpokable_


/// 삭제 금지: 테스트 프로젝트에서 사용
//#ifdef _KWLIB_INVOKABLE_EX
class UCTOOLDYNAMIC CFormInvokable
	: public CFormView
	, public KLambdaTimer//?LbTimer 1
	, public CSizeAdjustable
{
protected: // serialization에서만 만들어집니다.
	//CFormInvokable() noexcept;
	CFormInvokable(UINT nIDTemplate);
	DECLARE_DYNAMIC(CFormInvokable)

public:

	// 특성입니다.
public:
	// 작업입니다.
public:
	virtual CDocument* GetDoc()
	{
		return NULL;
	}
	// 재정의입니다.
public:
protected:
	//필요 하면 넣고, 자식의 것에서 아버지를 제대로 명칭 해야
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	//virtual void OnInitialUpdate(); // 생성 후 처음 호출되었습니다.

	// 구현입니다.
public:
	virtual ~CFormInvokable();
#ifdef _DEBUG
	//virtual void AssertValid() const;
	//virtual void Dump(CDumpContext& dc) const;
#endif
#ifdef _UseOnWndMsg
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
#endif // _UseOnWndMsg

	BOOL OnSizeDefault(UINT nType, int cx, int cy, int nCtrl, int arIdc[]);

protected:

	// 생성된 메시지 맵 함수
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnBeginInvoke(WPARAM wParam, LPARAM lParam);//?beginInvoke 1
	afx_msg void OnTimer(UINT_PTR nIDEvent);//?LbTimer 3

};

/* CFormInvokable 계승 하는 법 4군데
* CFormView 를 CFormInvokable 로만 바꾸면 됨.
* 헤더에서 1
class CPostTestView : public CFormInvokable
* cpp에서 2
IMPLEMENT_DYNCREATE(CPostTestView, CFormInvokable)
* cpp에서 3
BEGIN_MESSAGE_MAP(CPostTestView, CFormInvokable)
* cpp에서 4
CPostTestView::CPostTestView() noexcept
	: CFormInvokable(IDD_MFCAPPPOSTTEST1_FORM) //할아버지 객체 것을 바로 부를수 없어서 아버지꺼 부름.
*/

/// for CMainFrame
class UCTOOLDYNAMIC CMDIFrameWndInvokable : public CMDIFrameWnd
	, public KLambdaTimer//?LbTimer 1
{
	DECLARE_DYNAMIC(CMDIFrameWndInvokable)
public:
	CMDIFrameWndInvokable() noexcept;

	// 구현입니다.
public:
	virtual ~CMDIFrameWndInvokable()
	{
	}
#ifdef _UseOnWndMsg
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
#endif // _UseOnWndMsg

	// 생성된 메시지 맵 함수
protected:
	afx_msg LRESULT OnBeginInvoke(WPARAM wParam, LPARAM lParam);//?beginInvoke 1
	afx_msg void OnTimer(UINT_PTR nIDEvent);//?LbTimer 3
	DECLARE_MESSAGE_MAP()
};

class UCTOOLDYNAMIC CDlgInvokable : public CDialog
	, public KLambdaTimer//?LbTimer 1
	, public CSizeAdjustable
{
public: // serialization에서만 만들어집니다.
	//CFormInvokable() noexcept;
	CDlgInvokable();
	CDlgInvokable(UINT nIDTemplate, CWnd* pParent = NULL);
	CDlgInvokable(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
#ifdef _UseOnWndMsg
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
#endif // _UseOnWndMsg

	DECLARE_DYNAMIC(CDlgInvokable)

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnBeginInvoke(WPARAM wParam, LPARAM lParam);//?beginInvoke 1
	afx_msg void OnTimer(UINT_PTR nIDEvent);//?LbTimer 3
};

class UCTOOLDYNAMIC CDlgExInvokable : public CDialogEx
	, public KLambdaTimer//?LbTimer 1
	, public CSizeAdjustable
{
public: // serialization에서만 만들어집니다.
	//CFormInvokable() noexcept;
	CDlgExInvokable();
	CDlgExInvokable(UINT nIDTemplate, CWnd* pParent = NULL);
	CDlgExInvokable(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);

	DECLARE_DYNAMIC(CDlgExInvokable)
	//virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	/// TRUE리턴 하면 처리 됨.
#ifdef _UseOnWndMsg
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
#endif // _UseOnWndMsg

protected:
	DECLARE_MESSAGE_MAP()
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	afx_msg LRESULT OnBeginInvoke(WPARAM wParam, LPARAM lParam);//?beginInvoke 1
	afx_msg void OnTimer(UINT_PTR nIDEvent);//?LbTimer 3
};

class UCTOOLDYNAMIC CWndInvokable : public CWnd
	, public KLambdaTimer//?LbTimer 1
{
public:
	DECLARE_DYNAMIC(CWndInvokable)
	explicit CWndInvokable() noexcept;
public:
#ifdef _UseOnWndMsg
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
#endif // _UseOnWndMsg
protected:
	afx_msg LRESULT OnBeginInvoke(WPARAM wParam, LPARAM lParam);//?beginInvoke 1
	afx_msg void OnTimer(UINT_PTR nIDEvent);//?LbTimer 3
	DECLARE_MESSAGE_MAP()
};

#endif // _UseBaseClassInpokable_


// ========================================
// PostMainTask 기능을 CWnd 상속 없이 사용하는 최소한의 코드
// ========================================
#define _UseInvoke_Helper_
#ifdef _UseInvoke_Helper_

INLINE_STATIC constexpr LPCWSTR kPostMainTaskPropKey = L"NGS_PostMainTask_Enabled";

inline void UcMarkPostMainTaskWindow(HWND hwnd)
{
	if (!::IsWindow(hwnd))
		return;
	if (!::GetPropW(hwnd, kPostMainTaskPropKey))
		::SetPropW(hwnd, kPostMainTaskPropKey, (HANDLE)1);
}

inline bool UcIsPostMainTaskWindow(HWND hwnd)
{
	return ::IsWindow(hwnd) && (::GetPropW(hwnd, kPostMainTaskPropKey) != nullptr);
}

inline void UcUnmarkPostMainTaskWindow(HWND hwnd)
{
	if (::IsWindow(hwnd))
		::RemovePropW(hwnd, kPostMainTaskPropKey);
}

/// PostMainTask 기능을 CWnd 상속 없이 사용할 수 있는 헬퍼 클래스
class CPostMainTaskHelper
{
public:
	struct LambdaData
	{
		std::function<void(LPVOID)> lambda;
		std::function<LRESULT(LPVOID)> lambdaSend;
		bool bCalled = false;
		CStringA fnc;
		int line = 0;
		ULONGLONG tik = 0;
		HWND hwnd = NULL;

		LambdaData(std::function<void(LPVOID)> l, LPCSTR f, int ln, HWND h)
			: lambda(l), fnc(f), line(ln), tik(GetTickCount64()), hwnd(h) {
		}

		LambdaData(std::function<LRESULT(LPVOID)> l, LPCSTR f, int ln, HWND h)
			: lambdaSend(l), fnc(f), line(ln), tik(GetTickCount64()), hwnd(h) {
		}
	};

	// 가비지 컬렉션을 위한 정적 멤버들
	INLINE_STATIC CUcCriticalSection s_csGarbage;
	INLINE_STATIC KPtrList<LambdaData>* s_pGarbageList;
	INLINE_STATIC WORD s_srl;//{0 };

	// 비동기 실행
	static void PostMainTask(HWND hwnd, std::function<void(LPVOID)> lambda, LPCSTR fnc = nullptr, int line = -1)
	{
		if (!::IsWindow(hwnd))
		{
			TRACE("PostMainTask: Window is gone.\n");
			return;
		}
		if (!UcIsPostMainTaskWindow(hwnd))
		{
			TRACE("PostMainTask: target window is not configured for invoke. Add OVERRIDE_WINDOWPROC_FOR_ALL/POSTMAINTASK.\n");
			ASSERT(!"PostMainTask target has no OVERRIDE_WINDOWPROC_FOR_ALL/POSTMAINTASK");
			return;
		}

		LambdaData* pData = new LambdaData(lambda, fnc, line, hwnd);
		s_srl++;
		setInvokeFree(pData);
		::PostMessage(hwnd, WM_USER_INVOKE, s_srl, (LPARAM)pData);
	}

	// 동기 실행
	static LRESULT SendMainTask(HWND hwnd, std::function<LRESULT(LPVOID)> lambda, LPCSTR fnc = nullptr, int line = -1)
	{
		if (!::IsWindow(hwnd))
		{
			TRACE("SendMainTask: Window is gone.\n");
			return 0;
		}
		if (!UcIsPostMainTaskWindow(hwnd))
		{
			TRACE("SendMainTask: target window is not configured for invoke. Add OVERRIDE_WINDOWPROC_FOR_ALL/POSTMAINTASK.\n");
			ASSERT(!"SendMainTask target has no OVERRIDE_WINDOWPROC_FOR_ALL/POSTMAINTASK");
			return 0;
		}

		LambdaData* pData = new LambdaData(lambda, fnc, line, hwnd);
		s_srl++;
		setInvokeFree(pData);
		return ::SendMessage(hwnd, WM_USER_INVOKE, s_srl, (LPARAM)pData);
	}

	// 메시지 처리 함수
	static LRESULT HandleInvokeMessage(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		LambdaData* pData = (LambdaData*)lParam;
		if (!pData) return 0;

		// 메시지 처리 시점에 윈도우가 유효한지 체크
		if (!::IsWindow(hwnd))
		{
			TRACE("HandleInvokeMessage: Window is gone during processing.\n");
			pData->bCalled = true; // 호출된 것으로 표시하여 가비지 컬렉션에서 삭제되도록
			freeInvokeFree();
			return 0;
		}

		pData->bCalled = true;
		LRESULT result = 0;

		try
		{
			if (pData->lambda)
				pData->lambda((LPVOID)pData);
			else if (pData->lambdaSend)
				result = pData->lambdaSend((LPVOID)pData);
		}
		catch (...) { TRACE("Exception in PostMainTask lambda\n"); }

		freeInvokeFree();
		return result;
	}

private:
	// 가비지 컬렉션 함수들
	static void setInvokeFree(LambdaData* pData)
	{
		CSyncAutoLock __lock(&s_csGarbage, TRUE, __FUNCTION__, __LINE__, "s_csGarbage");
		if (!s_pGarbageList)
			s_pGarbageList = new KPtrList<LambdaData>();
		s_pGarbageList->push_back(pData);
	}

	static void freeInvokeFree()
	{
		CSyncAutoLock __lock(&s_csGarbage, TRUE, __FUNCTION__, __LINE__, "s_csGarbage");
		if (!s_pGarbageList) return;

		auto sz = s_pGarbageList->size();
		for (ULONGLONG i = 0; i < sz; i++)
		{
			LambdaData* pData = (LambdaData*)s_pGarbageList->front();
			if (pData)
			{
				if (pData->bCalled)
				{
					if (i < (sz - 2000)) // 2000개는 항상 남겨두자
					{
						delete pData;
						s_pGarbageList->pop_front();
					}
				}
				else
				{
					// 아직 호출되지 않은 경우, 윈도우가 유효한지 체크
					if (!::IsWindow(pData->hwnd))
					{
						// 윈도우가 닫혔으면 강제로 삭제
						delete pData;
						s_pGarbageList->pop_front();
					}
					else
					{
						break; // 아직 유효한 윈도우가 있으면 중단
					}
				}
			}
		}
	}
};

inline BOOL IsPostableWindow(CWnd* pw, BOOL bVisible = FALSE) {
	return pw && pw->GetSafeHwnd() && ::IsWindow(pw->GetSafeHwnd()) && (!bVisible || pw->IsWindowVisible());
}
inline BOOL IsPostableWindow(HWND hWnd, BOOL bVisible = FALSE) {
	return hWnd && ::IsWindow(hWnd) && (!bVisible || ::IsWindowVisible(hWnd));
}

// 편의 함수들 (디버깅 가능)
inline void PostMainTaskSimple(HWND hwnd, std::function<void(LPVOID)> lambda, LPCSTR fnc = nullptr, int line = -1)
{
	if (IsPostableWindow(hwnd)) {
		ASSERT(UcIsPostMainTaskWindow(hwnd));
		CPostMainTaskHelper::PostMainTask(hwnd, lambda, fnc, line);
	}
}

inline LRESULT SendMainTaskSimple(HWND hwnd, std::function<LRESULT(LPVOID)> lambda, LPCSTR fnc = nullptr, int line = -1)
{
	if (IsPostableWindow(hwnd)) {
		ASSERT(UcIsPostMainTaskWindow(hwnd));
		return CPostMainTaskHelper::SendMainTask(hwnd, lambda, fnc, line);
	}
	return {};
}

// 단순한 편의 함수들 (func, line 파라미터 없음)
inline void PostMainTask(HWND hwnd, std::function<void(LPVOID)> lambda, LPCSTR fnc = NULL, int line = -1)
{
	if (IsPostableWindow(hwnd)) {
		ASSERT(UcIsPostMainTaskWindow(hwnd));
		CPostMainTaskHelper::PostMainTask(hwnd, lambda, fnc, line);
	}
}


inline void PostMainTask(CWnd* pw, std::function<void(LPVOID)> lambda, LPCSTR fnc = NULL, int line = -1)
{
	//ASSERT(IsPostableWindow(pw)); 앱을 닫을때 온다.
	if(IsPostableWindow(pw)) {
		ASSERT(UcIsPostMainTaskWindow(pw->GetSafeHwnd()));
		CPostMainTaskHelper::PostMainTask(pw->GetSafeHwnd(), lambda, fnc, line);
	}
}

inline void PostMainTask(CWnd* pw, std::function<void()> lambda, LPCSTR fnc = NULL, int line = -1)
{
	PostMainTask(pw, [lambda](LPVOID) { lambda(); }, fnc, line);
}

inline LRESULT SendMainTask(HWND hwnd, std::function<LRESULT(LPVOID)> lambda, LPCSTR fnc = nullptr, int line = -1)
{
	if (IsPostableWindow(hwnd)) {
		ASSERT(UcIsPostMainTaskWindow(hwnd));
		return CPostMainTaskHelper::SendMainTask(hwnd, lambda, fnc, line);
	}
	return {};
}

inline LRESULT SendMainTask(CWnd* pw, std::function<LRESULT(LPVOID)> lambda, LPCSTR fnc = nullptr, int line = -1)
{
	if (IsPostableWindow(pw)) {
		ASSERT(UcIsPostMainTaskWindow(pw->GetSafeHwnd()));
		return CPostMainTaskHelper::SendMainTask(pw->GetSafeHwnd(), lambda, fnc, line);
	}
	return {};
}

inline LRESULT SendMainTask(CWnd* pw, std::function<LRESULT()> lambda, LPCSTR fnc = nullptr, int line = -1)
{
	return SendMainTask(pw, [lambda](LPVOID) -> LRESULT { return lambda(); }, fnc, line);
}

// this를 명시적으로 받는 버전 (일반 클래스에서 사용)
template<typename T>
inline void PostMainTaskSelf(T* pThis, std::function<void(LPVOID)> lambda)
{
	static_assert(std::is_base_of_v<CWnd, T>, "T must be derived from CWnd");
	if (IsPostableWindow(pThis))
		CPostMainTaskHelper::PostMainTask(pThis->GetSafeHwnd(), lambda, nullptr, -1);
}

template<typename T>
inline LRESULT SendMainTaskSelf(T* pThis, std::function<LRESULT(LPVOID)> lambda)
{
	static_assert(std::is_base_of_v<CWnd, T>, "T must be derived from CWnd");
	return CPostMainTaskHelper::SendMainTask(pThis->GetSafeHwnd(), lambda, nullptr, -1);
}

// CWnd 포인터를 직접 받는 버전 (윈도우 선택 가능)
//inline void PostMainTaskSelf(CWnd* pWnd, std::function<void(LPVOID)> lambda)
//{
//	if (IsPostableWindow(pWnd))
//		CPostMainTaskHelper::PostMainTask(pWnd->GetSafeHwnd(), lambda, nullptr, -1);
//}

inline LRESULT SendMainTaskSelf(CWnd* pWnd, std::function<LRESULT(LPVOID)> lambda)
{
	if (IsPostableWindow(pWnd)) {
		ASSERT(UcIsPostMainTaskWindow(pWnd->GetSafeHwnd()));
		return CPostMainTaskHelper::SendMainTask(pWnd->GetSafeHwnd(), lambda, nullptr, -1);
	}
	return 0;
}
#endif // _UseInvoke_Helper_

// 편의 매크로들 (디버깅이 어려우므로 함수 사용 권장)
//#define POST_MAIN_TASK(hwnd, lambda) \
//    PostMainTaskSimple(hwnd, lambda, __FUNCTION__, __LINE__)
//
//#define SEND_MAIN_TASK(hwnd, lambda) \
//    SendMainTaskSimple(hwnd, lambda, __FUNCTION__, __LINE__)
//
//#define POST_MAIN_TASK_SELF(lambda) \
//    PostMainTaskSelfSimple(lambda, __FUNCTION__, __LINE__)
//
//#define SEND_MAIN_TASK_SELF(lambda) \
//    SendMainTaskSelfSimple(lambda, __FUNCTION__, __LINE__)

/// WindowProc 가 없는 class 인 경우 WindowProc override를 위한 매크로들
#define OVERRIDE_WINDOWPROC_FOR_POSTMAINTASK(BaseClass) \
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override \
	{ \
		UcMarkPostMainTaskWindow(GetSafeHwnd()); \
		if (message == WM_NCDESTROY) UcUnmarkPostMainTaskWindow(GetSafeHwnd()); \
		if (message == WM_USER_INVOKE) \
			return CPostMainTaskHelper::HandleInvokeMessage(GetSafeHwnd(), wParam, lParam); \
		if (message == WM_TIMER) \
			if (auto* timerTool = dynamic_cast<ITimerTaskTool*>(this))\
					return timerTool->DoTimerTask((UINT)wParam); \
		return BaseClass::WindowProc(message, wParam, lParam); \
	}

/// 기존 WindowProc override에 메시지 처리만 윗 부분에 삽입하는 매크로 (함수처럼 사용)
/// message 추가 //dwk: 2025-10-22 10:55:00
#define ADD_POSTMAINTASK_TO_WINDOWPROC(message) \
	UcMarkPostMainTaskWindow(GetSafeHwnd()); \
	if (message == WM_NCDESTROY) UcUnmarkPostMainTaskWindow(GetSafeHwnd()); \
	if (message == WM_USER_INVOKE) \
		return CPostMainTaskHelper::HandleInvokeMessage(GetSafeHwnd(), wParam, lParam); \
	if (message == WM_TIMER)\
		if (auto* timerTool = dynamic_cast<ITimerTaskTool*>(this)) \
			return timerTool->DoTimerTask((UINT)wParam);

// ========================================
// KLambdaTimer 기능을 CWnd 상속 없이 사용하는 최소한의 코드
// ========================================

/// KLambdaTimer 기능을 CWnd 상속 없이 사용할 수 있는 헬퍼 클래스
class CKLambdaTimerHelper
{
public:
	// OnTimer 처리 함수
	static void HandleTimerMessage(CWnd* pWnd, UINT_PTR nIDEvent)
	{
		if (!pWnd || !::IsWindow(pWnd->GetSafeHwnd()))
			return;

		// timer id = _idx(_id*1000) + _idTm → 맵 키로 직접 조회 (dangling 순회 방지)
		auto& mapId2This = KLambdaTimer::GetMapId2This();
		const INT_PTR idx = static_cast<INT_PTR>((nIDEvent / 1000) * 1000);
		if (!mapId2This.Has(idx))
			return;

		KLambdaTimer* pTimer = mapId2This[idx];
		if (!pTimer || !AfxIsValidAddress(pTimer, sizeof(KLambdaTimer), TRUE))
		{
			mapId2This.RemoveKey(idx);
			return;
		}
		if (pTimer->_wnd != pWnd)
			return;

		pTimer->DoTimerTask(nIDEvent);
	}

	//// 한 번에 타이머 생성 및 설정하는 편의 함수들
	//static KLambdaTimer* SetTimerLambda(CWnd* pWnd, LPCSTR sid, UINT elapsed,
	//	std::function<void(LPVOID)> lmda, int maxCount = 0,
	//	std::function<void(LPVOID)> lmdaFinish = nullptr, LPCSTR fnc = nullptr, int line = 0)
	//{
	//	if (!pWnd || !::IsWindow(pWnd->GetSafeHwnd()))
	//		return nullptr;

	//	KLambdaTimer* pTimer = new KLambdaTimer(pWnd);
	//	pTimer->SetTimerLambda(sid, elapsed, lmda, maxCount, lmdaFinish, fnc, line);
	//	return pTimer;
	//}

	//static KLambdaTimer* SetTimerLambda(HWND hwnd, LPCSTR sid, UINT elapsed,
	//	std::function<void(LPVOID)> lmda, int maxCount = 0,
	//	std::function<void(LPVOID)> lmdaFinish = nullptr, LPCSTR fnc = nullptr, int line = 0)
	//{
	//	if (!::IsWindow(hwnd))
	//		return nullptr;

	//	CWnd* pWnd = CWnd::FromHandle(hwnd);
	//	return SetTimerLambda(pWnd, sid, elapsed, lmda, maxCount, lmdaFinish, fnc, line);
	//}



};

// WindowProc override에 타이머 처리 추가하는 매크로
#define OVERRIDE_WINDOWPROC_FOR_LAMBDATIMER(BaseClass) \
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override \
    { \
		UcMarkPostMainTaskWindow(GetSafeHwnd()); \
		if (message == WM_NCDESTROY) UcUnmarkPostMainTaskWindow(GetSafeHwnd()); \
        if (message == WM_USER_INVOKE) \
            return CPostMainTaskHelper::HandleInvokeMessage(GetSafeHwnd(), wParam, lParam); \
        if (message == WM_TIMER) \
            CKLambdaTimerHelper::HandleTimerMessage(this, wParam); \
        return BaseClass::WindowProc(message, wParam, lParam); \
    }

// 기존 WindowProc override에 타이머 처리만 추가하는 매크로 (함수처럼 사용)
#define ADD_LAMBDATIMER_TO_WINDOWPROC() \
	UcMarkPostMainTaskWindow(GetSafeHwnd()); \
	if (message == WM_NCDESTROY) UcUnmarkPostMainTaskWindow(GetSafeHwnd()); \
    if (message == WM_TIMER) \
        CKLambdaTimerHelper::HandleTimerMessage(this, wParam);

// PostMainTask와 LambdaTimer를 모두 처리하는 통합 매크로
#define OVERRIDE_WINDOWPROC_FOR_ALL(BaseClass) \
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override \
    { \
		UcMarkPostMainTaskWindow(GetSafeHwnd()); \
		if (message == WM_NCDESTROY) UcUnmarkPostMainTaskWindow(GetSafeHwnd()); \
        if (message == WM_USER_INVOKE) \
            return CPostMainTaskHelper::HandleInvokeMessage(GetSafeHwnd(), wParam, lParam); \
        if (message == WM_TIMER) \
            CKLambdaTimerHelper::HandleTimerMessage(this, wParam); \
        return BaseClass::WindowProc(message, wParam, lParam); \
    }

// 기존 WindowProc override에 모든 처리 추가하는 매크로 (함수처럼 사용)
#define ADD_ALL_TO_WINDOWPROC() \
	UcMarkPostMainTaskWindow(GetSafeHwnd()); \
	if (message == WM_NCDESTROY) UcUnmarkPostMainTaskWindow(GetSafeHwnd()); \
    if (message == WM_USER_INVOKE) \
        return CPostMainTaskHelper::HandleInvokeMessage(GetSafeHwnd(), wParam, lParam); \
    if (message == WM_TIMER) \
        CKLambdaTimerHelper::HandleTimerMessage(this, wParam);

// ========================================
// 편의 함수들 - 한 번에 타이머 생성 및 설정
// ========================================

///// func, line 파라미터를 넣어주려면 CKLambdaTimerHelper::SetTimerLambda 를 직접 부른다.
//inline KLambdaTimer* SetTimerLambda(CWnd* pWnd, LPCSTR sid, UINT elapsed,
//	std::function<void(LPVOID)> lmda, int maxCount = 0,
//	std::function<void(LPVOID)> lmdaFinish = nullptr)
//{
//	return CKLambdaTimerHelper::SetTimerLambda(pWnd, sid, elapsed, lmda, maxCount, lmdaFinish);
//}
//
//// HWND 버전
//inline KLambdaTimer* SetTimerLambda(HWND hwnd, LPCSTR sid, UINT elapsed,
//	std::function<void(LPVOID)> lmda, int maxCount = 0,
//	std::function<void(LPVOID)> lmdaFinish = nullptr)
//{
//	return CKLambdaTimerHelper::SetTimerLambda(hwnd, sid, elapsed, lmda, maxCount, lmdaFinish);
//}
//
//// AfxGetMainWnd() 버전 (가장 편리) - 윈도우 정보를 먼저 명시
////inline KLambdaTimer* SetTimerLambda(CWnd* pWnd, LPCSTR sid, UINT elapsed, 
////    std::function<void(LPVOID)> lmda, int maxCount = 0, 
////    std::function<void(LPVOID)> lmdaFinish = nullptr)
////{
////    return CKLambdaTimerHelper::SetTimerLambda(pWnd, sid, elapsed, lmda, maxCount, lmdaFinish);
////}
//
//// AfxGetMainWnd() 자동 지정 버전 (편의용)
//inline KLambdaTimer* SetTimerLambda(LPCSTR sid, UINT elapsed,
//	std::function<void(LPVOID)> lmda, int maxCount = 0,
//	std::function<void(LPVOID)> lmdaFinish = nullptr)
//{
//	return CKLambdaTimerHelper::SetTimerLambda(AfxGetMainWnd(), sid, elapsed, lmda, maxCount, lmdaFinish);
//}
//inline void DelayAndRunOnceLambda(LPCSTR sid, UINT elapsed, function<void(LPVOID)> lmda)
//{
//	if (!IsTimerExists(sid))// 처음 불려 질때
//		SetTimerLambda(sid, elapsed, lmda, 1, nullptr);
//	else// 이미 타이머 작동중인데 또다시 불려졌을때
//		RestartTimer(sid);
//}

UCTOOLDYNAMIC
long UcGetCtrlRect(CWnd* pParent, int idc, LPRECT lpRect);
UCTOOLDYNAMIC
long UcGetCtrlRect(CWnd* pParent, CWnd* pCtrl, LPRECT lpRect);
UCTOOLDYNAMIC
void UcMoveCtrl(CWnd* wparent, CWnd* ctrl, int cx, int cy);
UCTOOLDYNAMIC
void UcMoveCtrl(CWnd* wparent, UINT idc, int cx, int cy);





/// 주어진 CSingleLock이 변수가 사라질때 Unlock()한다.
/// shared_ptr로 넘기면 백그라운나 포그라운드로 람다 함수로 전달 하여 Unlock()할수 있다.
/// 이걸 다중 상속 받은경우만 쓸수 있다.
class KLongTaskDoing
{
public:
	///           fnc       key       
	typedef tuple<string, string, SHP<CCriticalSection>, SHP<CSingleLock>> TPLife;

	class KLife
	{
	public:
		KLife(KLongTaskDoing* loading, TPLife info)
			: _loading(loading)
			, _info(info)
		{
		}

		~KLife()
		{
			Finished();
		}

		KLongTaskDoing* _loading{ nullptr };
		TPLife _info;
		/// 여러개 로드할때 마지막 이미지 인지 체크후 이것을 직접 부를수 있다.
		void Finished()
		{
			//auto lock = std::get<3>(_info);
			//if(lock->IsLocked())///어차피 다 free되면서 Unlock되지만 CCriticalSection이 먼저 없어질수 있으니 Unlock을 부른다.
			//	lock->Unlock();
			/// _mapLongTaskingLock에서 제거 해야 하고 중복코드 피하기 위해 아래를 호출 한다.
			auto key = std::get<1>(_info);
			if (_loading)
				_loading->LongTaskFinished(key);//타이머도 호출해야 하므로 필요하다.
		}
		//BOOL IsDownloading() {
		//	auto key = std::get<1>(_info);
		//	auto bLod = _loading->IsLongTaskDoing(key.c_str());
		//	return bLod;
		//}
	};

	/// 이미지 전체 다운로딩에 필요한 락 // dwkang 2022-12-26 14:15
	std::map<string, TPLife> _mapLongTaskingLock;

	/// 특히 백그라운드에서 이미지 다운로드 하기 직전 이것으로 시작 한다.
	/// bSafe: 다운로딩중 에러가 나서 락이 안풀리는걸 방지. 15초 후에 자동으로 풀린다.
	/// 리턴 받은 'loading'을 반드시 람다 캡쳐 파라미터에 넘겨 줘야 그 람다 안에서 lock이 해제된다.
	/// 안 넘겨 주면 ~KLife 되는 위치에서 LongTaskFinished(UnLock)되어 버린다.
	/// shared_ptr<> 는 람다로 넘겨서 현재 로컬을 끝나도, 비동기 람다 함수 안에까지 수명이 연장 되는 점을 이용하여 destructor
	SHP<KLife> LongTaskStartedRaw(LPCSTR fnc, LPCSTR key, BOOL bSafe = TRUE, BOOL bGuid = TRUE);
	SHP<KLife> LongTaskStartedRaw(LPCSTR fnc, int ikey, BOOL bSafe = TRUE)
	{
		CStringA key; key.Format("[%d]", ikey);
		return LongTaskStartedRaw(fnc, key);
	}

	/// 이미지 다운로드가 끝나고 또는 출력(foreground)까지 마친경우 모든 이미지 처리 까지 끝난 경우 부른다.
	void LongTaskFinished(string key, BOOL bTimeout = FALSE);

	/// virtual 은 어차피 dynamic_cast<> 하려면 다형식이어야 하므로 적어도 하나는 virtual 해줘야 한다.
	/// OnClose()에서 아직 이미지 다운로딩중인지 체크 하는 함수. TRUE이면 Close 하면 안된다.
	virtual BOOL IsLongTaskDoing(LPCSTR key = NULL);

	/// 알수 없는 사태 일때 
	/// 15초가 지나도록 다운로드가 안끝났다면, 강제로 Unlock을 해야지. 무슨 문제가 있을지 모르니.
	void SafeDownloading(string key);
#ifdef _sample__
	// MFC 방식에서 thread 사용	예제. 
	// CMainPool은 MFC에서 만든 thread pool 클래스이다. CIconTaxiControlApp은 MFC에서 만든 App 클래스이다.
	void Samples()
	{
		/// ex
		FOREGROUND();
		auto pool = (CMainPool*)((CIconTaxiControlApp*)AfxGetApp())->m_pool;
		auto loading = LongTaskStarted();//?async image download 'loading'을 반드시 람다 캡쳐에 넘겨 줘야 lock이 풀린다.
		CStringW sImgServerNM = pDriv->GetImgServerNM();
		pool->ThreadTask([this, sImgServerNM, loading]() {
			BACKGROUND(1);
			CStringW strImgDownPath;
			if (DownLoadImgFromSvr(sImgServerNM, IMAGE_TYPE::IMG_DRIVER, strImgDownPath))
			{
				PostMainTask(this, [this, strImgDownPath, loading]() {
					FOREGROUND();
					m_strImgDownPath = strImgDownPath;
					_reDrawDrivImage();
					});
			}
			});
	}

	/// <summary>
	/// 가장 좋음
	/// thread를 스택변수로 하여 람다 함수로 캡쳐하여 join 하는 예제. 
	/// (이 경우 람다 안에서 join해야 하므로 t를 멤버변수로 하여 람다 안에서 this->t.join() 하는 방법 밖에 몰랐다.)
	///	thread가 스택변수이므로 람다로 캡쳐하면 스택에서 사라지므로
	///	반드시 move한 후 mutable 람다로 캡쳐해야 한다. 
	///	move 하지 않으면 람다 안에서 join이 안된다.
	/// </summary>
	void Samples2() {
		std::thread t([this] {
			BACKGROUND(1);
			DoWork();
			});
		std::thread joiner([this, t = std::move(t)]() mutable {
			t.join();
			PostMainTask([this] {
				FOREGROUND();
				OnWorkDone(); // UI에서 바로 실행 
				});
			});
		joiner.detach();
	}

/// 기존 방식에서는 thread가 멤버변수이므로 람다로 캡쳐하면 this->t.join() 하면 되지만, 
/// 스택변수로 할 경우에는 move한 후 mutable 람다로 캡쳐해야 한다.
class CMyWnd : public CWnd
{
public:
	std::thread m_t;
	void Samples2()
	{
		m_t = std::thread([this] {
			BACKGROUND(1);
			DoWork();
			});
		std::thread joiner([this]() {
			ASSERT(joinable());
			m_t.join();
			ASSERT(joinable() == false);
			PostMainTask([this] {
				FOREGROUND();
				OnWorkDone(); // UI에서 실행
				});
			});
		joiner.detach();
	}
};
#endif // _sample__
};

/// 현재창에서 이미지 하나 다운로드할 경우 호출하는 함수명을 키로 쓴다.
#define LongTaskStarted(...) LongTaskStartedRaw(__FUNCTION__, __FUNCTION__, ##__VA_ARGS__)
#define LongTaskStartedByKey(key, ...) LongTaskStartedRaw(__FUNCTION__, key, TRUE, FALSE, ##__VA_ARGS__)
/// 여러개 인경우 인덱스 번호를 서로 유니크하게 주면 된다. 순서나 증가량은 무관.
#define LongTaskStartedIndex(idx, ...) LongTaskStartedRaw(__FUNCTION__, idx, ##__VA_ARGS__)


UCTOOLDYNAMIC
void UcPostMessageBoxError(LPCWSTR fmt, ...);


#ifdef _timer_sample_ //#KLambdaTimer 1,2,3
SHP<KLambdaTimer> _timer;

_timer = NEWSHP(KLambdaTimer, this);

int count = 6;// count == 0 이면 계속 반복
_timer->SetTimerLambda("reg", 500, [this, doc](auto ato) {
	auto pr = (KTimerParam*)ato;
	SetDlgItemText(IDC_STATIC_insert, (pr->_i % 2) == 0 ? L"" : L"사용자 등록 되었습니다.");
	}, count, [](auto) {}, __FILE__, __LINE__);
#endif // _timer_sample_ //#KLambdaTimer 1,2,3



#ifdef _sample__ /// 이미지 여러개 반복하여 다운로딩 예제

/// 1. 일단 KLongTaskDoing을 상속한다. CWnd 아니어도 된다.
class CDlgSample : public CBaseDialog
	, public KLongTaskDoing//?async image download
{
public:
	void _loadImg();
	void loadImgAsync(int iImg, TmpImg tmpImg, shared_ptr<KLife> loading);
};

void CDlgSample::_loadImg()
{
	FOREGROUND();
	for (int i = 0; i < _countof(_tmpArr); i++)
	{
		TmpImg tmpImg = _tmpArr[i];
		auto loading = LongTaskStartedIndex(i);//?async image download 'loading'을 반드시 람다 캡쳐에 넘겨 줘야 lock이 풀린다.
		loadImgAsync(i, tmpImg, loading);//여기서 CSingleLock 을 뺀다.
	}
}

void CDlgSample::loadImgAsync(int iImg, TmpImg tmpImg, shared_ptr<KLife> loading)
{
	KDEBUGSTACKEXP("2022-12-22 13:14:38");
	TRACE("\n");
	auto pool = (CMainPool*)((CIconTaxiControlApp*)AfxGetApp())->m_pool;
	pool->ThreadTask([this, iImg, tmpImg, loading]() -> void {
		TmpImg* tmp = (TmpImg*)&tmpImg;//람다로 캡쳐된 변수는 const 이므로
		BACKGROUND(1);
		if (DownLoadImgFromSvr(*tmp->imgNM, tmp->imgType, sLocal))
		{
			PostMainTask(this, [this, pCtrl, loading]()-> void {
				FOREGROUND();
				CRect rc;
				UcGetCtrlRect(this, pCtrl, rc);
				rc.DeflateRect(2, 2);
				InvalidateRect(rc, 1);
				});//PostMainTask
		}
		});//pool->ThreadTask
}
void CDlgJoinUs::OnClose()
{
	/// 아직 다운로딩 중이면 메시지 없이 안닫힌다.
	if (IsLongTaskDoing())//?async image download
		return;
	CICDialog::OnClose();
}

#endif // _sample__
