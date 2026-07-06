#pragma once



#include <functional> // std::function<>
#include <atomic> // std::function<>

#include <atlutil.h> // CThreadPool
#include "UcTool.h"


#define AUTOLOCKN(sobj, n) CSingleLock __lock##n(&(sobj), TRUE)
#define UcAUTOLOCK(sobj) AUTOLOCKN((sobj), 1) // << 주로 쓴다.


class CTaskBase
{
public:
	CTaskBase() {}

	LPARAM m_lp{ NULL };//추가 parameter를 Queue 에서 알아서 넣어 주도롤 하기 위해
	bool m_bAutoFree{ true };
	BOOL m_bKeepRunning{ FALSE };
	CStringW m_sFunc;
	CStringW m_sExtra;
	int m_line{ -1 };

	virtual void DeleteMe()
	{
		if (m_bAutoFree)
			delete this;
	}
	CStringW m_sClass;

	virtual void DoTask(void* pvParam, OVERLAPPED* pOverlapped) = NULL;
};







class CTaskLamdaEx : public CTaskBase
{
public:
	static std::function<void(KException*)> s_fncOnException;

	std::function<void()> m_lambda{ nullptr };
	std::function<void()> m_lambdaFinish{ nullptr };

	explicit CTaskLamdaEx(std::function<void()> lambda, LPCSTR fnc = NULL, int line = 0, function<void()> lmdaFinish = NULL
	)
		: m_lambda(lambda)
		, m_lambdaFinish(lmdaFinish)
	{
		m_sFunc = fnc;
		m_line = line;
	}
	void DoTask(void* pvParam, OVERLAPPED* pOverlapped) override;
};



class CThreadWorker
{
public:
	typedef DWORD_PTR RequestType;
	virtual BOOL Initialize(void* pvParam)
	{
		return TRUE;
	}
	virtual void Terminate(void* pvParam)
	{
	}
	// CTaskBase* pTask -> dw -> CTaskBase* pTask 로 전달 된다.
	void Execute(RequestType dw, void* pvParam, OVERLAPPED* pOverlapped) throw()
	{
		auto pTask = reinterpret_cast<CTaskBase*>(dw);
		pTask->DoTask(pvParam, pOverlapped);
		pTask->DeleteMe();
	}
};


class CMainPool : public CThreadPool<CThreadWorker>
{
public:
	explicit CMainPool(int nThread = 4)
		: _nThread(nThread)
	{
	}
	int _nThread{ 4 };
	BOOL _bInitialized{ FALSE };

	void InitThreadPool(int nThread = 4)
	{
		auto app = dynamic_cast<CWinApp*>(this);
		auto hr = Initialize(app, nThread);// 4 개 병렬로 쓸때를 대비해서
		ASSERT(hr == S_OK);
	}
	void SetThreadCount(int nThread)
	{
		ASSERT(_bInitialized);
		if (nThread != _nThread)
		{
			SetThreadCount(nThread);
			_nThread = nThread;
		}
	}

	BOOL Queue(CTaskBase* pTask)
	{
		return this->QueueRequest((CThreadWorker::RequestType)pTask);
	}

	template<typename TFNC>
	void ThreadTask(TFNC rmda, LPCSTR fnc = NULL, int line = -1, BOOL bAsync = TRUE, function<void()> lmdaFinish = NULL)
	{
		if (!_bInitialized)
		{
			InitThreadPool(_nThread);
			_bInitialized = TRUE;
		}
		CTaskLamdaEx* pTask = new CTaskLamdaEx(rmda, fnc, line, lmdaFinish);
		if (bAsync)
			this->Queue(pTask);
		else
		{
			pTask->DoTask(NULL, NULL);
			pTask->DeleteMe();
		}
	}
#ifdef __Sample__
#include "UcTool/UcThreadTool.h"
	class CThreadCopy3App : public CWinApp
		, public CMainPool//?thread_pool 1
	{
		CThreadCopy3App() : CMainPool(6) {//?thread_pool 2 안해도 기본값 4로 들어간다.
		}
	}
#define APP  ((CThreadCopy3App*)AfxGetApp())//?thread_pool 3

	BOOL CKDiff1App::InitInstance() {
		//?deprecated	InitThreadPool();// 4 개 병렬로 쓸때를 대비해서
	}

	APP->ThreadTask([this]() {//?thread_pool 4
		BACKGROUND(0);
		system(R"(C:\src\svn_update_cadnow.bat)");//이 폴더는 SVN Server와 계속 동기화 시킨다. 10분에 한번씩 Update 한다.
		ToolMsg(L"cadnow가 SVN update 되었습니다!");
		});
#endif // __Sample__
};


// 이걸 쓸려면 App시작 InitInstance() 에서 
// m_pool = new CMainPool();
// HRESULT hr = m_pool->Initialize((void*)this, 4);
inline BOOL IsBackground()
{
	return AfxGetApp()->m_nThreadID != ::GetCurrentThreadId();
}
#define FOREGROUND()    ASSERT(!IsBackground()) // ::IsGUIThread(FALSE));
// nth는 그저 참조. 몇번째 백그라운드인가? 2이상 이면 백그라은드에서 다시 또 스레드큐에 넣음.
#define BACKGROUND(nth) ASSERT(IsBackground()) //ASSERT(AfxGetApp()->m_hThread !::IsGUIThread(FALSE));



template<typename TVAL>
void UcSyncInc(TVAL& value)
{
#if CPP17_OR_LATER
	if constexpr (sizeof(TVAL) == sizeof(LONG))
		InterlockedIncrement(&value);
	else if constexpr (sizeof(TVAL) == sizeof(LONG64))
		InterlockedIncrement64(&value);
	else
		static_assert(sizeof(TVAL) == sizeof(LONG) || sizeof(TVAL) == sizeof(LONG64), "TVAL must be a LONG or LONG64 compatible type.");
#else
	if (sizeof(TVAL) == sizeof(LONG))
		InterlockedIncrement(&value);
	else if (sizeof(TVAL) == sizeof(LONG64))
		InterlockedIncrement64(&value);
	else
		static_assert(sizeof(TVAL) == sizeof(LONG) || sizeof(TVAL) == sizeof(LONG64), "TVAL must be a LONG or LONG64 compatible type.");
#endif
}
template<typename TVAL>
void UcSyncSet(TVAL& value, TVAL newValue)
{
#if CPP17_OR_LATER
	if constexpr (sizeof(TVAL) == sizeof(LONG))
		InterlockedExchange(&value, newValue);
	else if constexpr (sizeof(TVAL) == sizeof(LONG64))
		InterlockedExchange64(&value, newValue);
	else
		static_assert(sizeof(TVAL) == sizeof(LONG) || sizeof(TVAL) == sizeof(LONG64), "TVAL must be a LONG or LONG64 compatible type.");
#else
	if (sizeof(TVAL) == sizeof(LONG))
		InterlockedExchange(&value, newValue);
	else if (sizeof(TVAL) == sizeof(LONG64))
		InterlockedExchange64(&value, newValue);
	else
		static_assert(sizeof(TVAL) == sizeof(LONG) || sizeof(TVAL) == sizeof(LONG64), "TVAL must be a LONG or LONG64 compatible type.");
#endif
}

template<typename TVAL>
TVAL UcSyncGet(TVAL& value)
{
#if CPP17_OR_LATER
	if constexpr (sizeof(TVAL) == sizeof(LONG))
		return static_cast<TVAL>(InterlockedCompareExchange(reinterpret_cast<LONG*>(&value), value, value));
	else if constexpr (sizeof(TVAL) == sizeof(LONG64))
		return static_cast<TVAL>(InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&value), value, value));
	else
		static_assert(sizeof(TVAL) == sizeof(LONG) || sizeof(TVAL) == sizeof(LONG64), "TVAL must be a LONG or LONG64 compatible type.");
#else
	if (sizeof(TVAL) == sizeof(LONG))
		return static_cast<TVAL>(InterlockedCompareExchange(reinterpret_cast<LONG*>(&value), value, value));
	else if (sizeof(TVAL) == sizeof(LONG64))
		return static_cast<TVAL>(InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&value), value, value));
	else
		static_assert(sizeof(TVAL) == sizeof(LONG) || sizeof(TVAL) == sizeof(LONG64), "TVAL must be a LONG or LONG64 compatible type.");
#endif
}

template<typename TVAL>
INT64 UcSyncComp(TVAL& value, TVAL valComp)
{
	TVAL originalValue = UcSyncGet(value);
	return static_cast<INT64>(originalValue) - static_cast<INT64>(valComp);
}

template<typename TVAL> bool UcSyncIsZero(TVAL& value) { return UcSyncComp(value, (TVAL)0) == 0; }
template<typename TVAL> bool UcSyncIsPlus(TVAL& value) { return UcSyncComp(value, (TVAL)0) > 0; }
template<typename TVAL> bool UcSyncIsMinus(TVAL& value) { return UcSyncComp(value, (TVAL)0) < 0; }
template<typename TVAL> bool UcSyncIsBiggerThan(TVAL& value, TVAL valComp) { return UcSyncComp(value, valComp) > 0; }

class UcMutexForProfile
{
public:
	std::shared_ptr<CMutex> _mutex;
	DWORD _dwResult{ 0 };
	UcMutexForProfile(CString sMutex)
	{
		try
		{
			sMutex.Replace(_T("\\"), _T("/")); // do not use '\\'
			_mutex = std::make_shared<CMutex>(FALSE, sMutex);
			if (_mutex->m_hObject != NULL)
				_dwResult = _mutex->Lock(); //WAIT_OBJECT_0
		}
		catch (CResourceException* e)
		{
			CString buf; // if '\\' used : A required resource was unavailable.
			e->GetErrorMessage(buf.GetBuffer(1024), 1024);
			TRACE(_T("Mutex error: %s\n"), (LPCTSTR)buf);
		}
	}
	~UcMutexForProfile()
	{
		if (_mutex->m_hObject != NULL)
			_mutex->Unlock();
	}
};

class UcAbortableSleep {
public:
	UcAbortableSleep() : stopFlag(false), isSleepingFlag(false) {}

	// 지정된 시간 동안 대기하며, 중단 요청이 있으면 즉시 중단
	bool Sleep(int durationMillis, int checkInterval = 1000) {
		isSleepingFlag.store(true);  // 대기 시작을 표시
		//int checkInterval = 1000;  // 100ms마다 중단 요청 확인
		int elapsed = 0;

		while (elapsed < durationMillis) {
			if (stopFlag.load()) {
				TRACE(_T("xxx 중단 요청이 감지되어 대기를 종료합니다.\n"));
				Reset();  // 루프 탈출 후 플래그 초기화
				return false;  // 중단된 경우
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));  // 100ms 대기
			//TRACE(L"elapsed: %d\n", elapsed);
			elapsed += checkInterval;
		}
		return true;  // 정상적으로 대기 완료
	}

	// 다른 스레드에서 호출하여 중단 요청을 설정
	void Abort() {
		stopFlag.store(true);
	}

	// 현재 대기 중인지 확인하는 함수
	bool IsSleeping() {
		return isSleepingFlag.load();
	}

private:
	// stopFlag와 isSleepingFlag를 초기화
	void Reset() {
		stopFlag.store(false);
		isSleepingFlag.store(false);  // 대기 종료를 표시
	}

	std::atomic<bool> stopFlag;        // 중단 요청 플래그
	std::atomic<bool> isSleepingFlag;  // 현재 대기 중인지 나타내는 플래그


#ifdef _Sample__
	UcAbortableSleep _sleep;
	SHP<std::thread> sleepThread;

	void CEnglishRscView::OnBnClickedAbortablesleep()
	{
		// 대기를 수행하는 스레드
		sleepThread = std::make_shared<std::thread>([&]() {
			TRACE(L"MySleep start 60sec.\n");
			bool result = _abSleep.Sleep(60'000, 1'000);  // 60초 대기
			if (result) {
				TRACE(L"대기 완료.\n");
			}
			else {
				TRACE(L"zzz 대기가 중단되었습니다.\n");
			}
			});

		sleepThread->detach();//이걸 해줘야 재 사용 가능. 기다리려면 join()
	}

	void CEnglishRscView::OnBnClickedStopsleep()
	{
		_sleep.Abort();  // 다른 스레드에서 대기 중단 요청
	}
#endif // _Sample__

};

/// <summary>
/// HANDLE CreateThread 를 std::thread 방식으로 간단히 마이그레이션 해주는 함수
/// HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThdAttr, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, _In_opt_ __drv_aliasesMem LPVOID lpParameter, _In_ DWORD dwCreationFlags, LPDWORD lpThreadId )
/// 보류
/// </summary>
//inline HANDLE UcCreateThread() {}//dwk: 2025-08-14 12:22 


#include <atlbase.h>

class UcComSingleLock {
public:
	UcComSingleLock(CComCriticalSection* pcs, BOOL initialLock = TRUE)
		: m_cs(*pcs), m_isLocked(false) {
		if (initialLock) {
			Lock();
		}
	}

	~UcComSingleLock() {
		if (m_isLocked) {
			Unlock();
		}
	}

	void Lock() {
		if (!m_isLocked) {
			m_cs.Lock();
			m_isLocked = true;
		}
	}

	void Unlock() {
		if (m_isLocked) {
			_Analysis_assume_lock_held_(m_cs.m_sec);
			m_cs.Unlock();
			m_isLocked = false;
		}
	}

	bool IsLocked() const {
		return m_isLocked;
	}

private:
	CComCriticalSection& m_cs;
	bool m_isLocked;
};


#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <tuple>
#include <future> // std::future, std::promise

/// <summary>
/// CMainPool 다음 세대 스레드 풀
/// </summary>
class UCTOOLDYNAMIC UcStdThreadPool {
public:
	UcStdThreadPool(size_t minThreads = 0, size_t maxThreads = 0);
	~UcStdThreadPool();

	void AddThreadToPool();

	void RemoveThreadFromPool();

	/// <summary>
	/// 외부에서 큐에 넣는 함수는 리턴 int이고 EnqueueTask 리턴된 future.wait() 후 future.get()으로 받는다.
	/// </summary>
	/// <param name="fncTask"></param>
	/// <returns></returns>
	std::future<int> EnqueueTask(std::function<void()> fncIn, LPCWSTR sFile = nullptr, int nLine = 0) {//std::function<TRTN()>
		auto promise = std::make_shared<std::promise<int>>();
		EnqueueTaskItem(fncIn, promise, sFile, nLine);
		return promise->get_future();
	}

	using TplFLSI = std::tuple<function<void()>, LONGLONG, std::wstring, int>;

	void EnqueueTaskItem(std::function<void()> fncIn, std::shared_ptr<std::promise<int>> promise, LPCWSTR sFile, int nLine);
	void MonitorAndAddThreads(size_t delayMs);

	void StopPool() {
		std::unique_lock<std::mutex> lock(_queueMutex);
		_stop = true;// 앱 종료시 한번 하면 다시 못쓴다.
	}
private:
	size_t _minThreads{ 0 }, _maxThreads{ 0 }, _numThreads{ 0 }, _numCores{ 0 };
	std::vector<std::thread> _workers;

	/// promise->set_value(rv) 까지 해주는 바깥쪽 람다함수의 리턴 은 void
	std::queue<TplFLSI> _tasks;

	std::mutex _queueMutex;
	std::condition_variable _condition;
	//bool _stop{ false }; //외부에서 바꾸면 스레드 안정성 없어서
	std::atomic<bool> _stop{ false }; // std::atomic으로 선언
	// 의례된 람다함수를 백그라운드 풀에서 실행하다가 exceptions이 발생 하면 람다함수를 등록할 수 있다.
	std::function<void(LPCTSTR sMode, CString sErrorMsg)> _OnError;
};

#define DEFAULT_UTHREAD nullptr,__FILE__,__LINE__

class UCTOOLDYNAMIC UcThread {
public:
	std::thread _th;
	//int _error{ 0 };

	/// Detach 일경우 this가 std::thread 내부 에서 this가 스택변수라 없어질수 있으므로 멤버 쓰지 말아야
	//std::function<void(UcThread*)> _onThreadError;
	//std::wstring _tag;
	//CStringA _sFile;
	//int _nLine{ 0 };

	UcThread() {
	}
	template<typename Func>
	UcThread(Func&& fn, LPCSTR tag = nullptr, std::function<void(UcThread*)> onThError = nullptr, LPCSTR sFile = nullptr, int nLine = 0) {
		// 람다(특히 중첩·mutable 캡처)를 그대로 std::thread에 넘기면 MSVC가 void(*)(void) 쪽으로
		//std::function<void()> job = std::forward<Func>(fn);
		StartThread(fn, tag, onThError, sFile, nLine);
	}

	void StartThread(std::function<void()> job, LPCSTR tag = nullptr, std::function<void(UcThread*)> onThError = nullptr, LPCSTR sFile = nullptr, int nLine = 0);

	//[[deprecated]]
	//template<typename Func>
	//UcThread& operator=(Func&& fn) {
	//	ASSERT(0);//[[deprecated]]
	//	StartThread(fn);
	//	return *this;
	//}

	bool joinable() const noexcept { return _th.joinable(); }
	void join() { if (_th.joinable()) _th.join(); }

	/// <summary>
	/// 주의: Detatch 하면, this가 사라 져서 _onThreadError 같은 멤버가 없을 수 있다.
	/// </summary>
	void Detach() {
		if (_th.joinable()) 
			_th.detach(); 
	}

private:
};


//[ UcBaseTools.h 에서 옮겨옴
class CUcCriticalSection : public CCriticalSection
{
public:
	CUcCriticalSection()
		: m_nLocked(0)
	{
	}
	LONG m_nLocked;
	void Increase() { InterlockedIncrement(&m_nLocked); }
	void Decrease() { InterlockedDecrement(&m_nLocked); }

	///?주의: 아래걸 안쓰면 CCriticalSection::Lock() 안에서는 
	/// virtual 이 아니므로 내부 걸 써버려서 overrided Lock을 못 부른다.
	virtual BOOL Lock(DWORD dwTimeout)
	{
		ASSERT(dwTimeout == INFINITE);
		(void)dwTimeout;
		return Lock();
	}

	/// 이걸 모든 객체가 override해야 어디서 스레드 lock 걸린줄 알수 있다.
	virtual BOOL Lock()
	{
		::EnterCriticalSection(&m_sect);
		return TRUE;
	}
};

class UCTOOLDYNAMIC CSyncAutoLock : public CSingleLock
{
public:
	// CSyncObject 는 CCriticalSection CKCriticalSection이 대중
	explicit CSyncAutoLock(CSyncObject* pObject, BOOL bInitialLock = TRUE, LPCSTR sFile = NULL, int iLine = 0, LPCSTR sobj = NULL);
	virtual ~CSyncAutoLock();

	UINT64 m_token{ 0 };
	ULONGLONG m_ull;
	CTime m_tLocked;
	CStringW m_sObj;
	CStringA m_sFile;
	int m_iLine;

	BOOL Lock(DWORD dwTimeOut = INFINITE);//constructor 에서 부르므로 virtual 제거. cppcheck
	virtual BOOL Unlock();
	virtual BOOL Unlock(LONG lCount, LPLONG lPrevCount = NULL);
	virtual BOOL IsLocked();
};
//Id: virtualCallInConstructor
//Virtual function 'Lock' is called from constructor 'CSyncAutoLock(CSyncObject*pObject,BOOL bInitialLock=TRUE,LPCSTR sFile=NULL,int iLine=0,LPCSTR sobj=NULL)' at line 315. Dynamic binding is not used.

inline BOOL CSyncAutoLock::IsLocked()
{
	return m_bAcquired;
}

//] UcBaseTools.h 에서 옮겨옴