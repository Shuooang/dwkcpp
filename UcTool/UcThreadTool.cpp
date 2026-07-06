#include "pch.h"
#include "UcThreadTool.h"
#include "UcTool.h"

/// <summary>
/// 기본 콜백인 KException::s_fncExceptionDealer 대신에 쓰레드에서는 별도로 처리 하고자 할 때 람다함수를 별도로 준다.
/// </summary>
/// <param name="pvParam"></param>
/// <param name="pOverlapped"></param>
std::function<void(KException*)> CTaskLamdaEx::s_fncOnException;

//#define UCCATCH_THREAD UCCATCH_ALLGEN((CTaskLamdaEx::s_fncOnException ? CTaskLamdaEx::s_fncOnException : KException::s_fncExceptionDealer))

void CTaskLamdaEx::DoTask(void* pvParam, OVERLAPPED* pOverlapped)
{
	try {
		ASSERT(m_lambda);
		m_lambda();
		if (m_lambdaFinish)
			m_lambdaFinish();
	}
	UCCATCH_ALL;///KException.s_fncExceptionDealer 를 채워야 로그 처리가 된다.
}


UcStdThreadPool::~UcStdThreadPool()
{
	StopPool();

	_condition.notify_all();

	for (std::thread& worker : _workers)
		worker.join();
}
/// <summary>
/// std를 활용하여 스레드 풀을 셋팅한다.
/// </summary>
/// <param name="numThreads">스레드 갯수를 정한다. 기본값은 8</param>
UcStdThreadPool::UcStdThreadPool(size_t minThreads, size_t maxThreads)
{
	ASSERT(minThreads <= maxThreads);
	_numCores = std::thread::hardware_concurrency(); // 시스템의 논리 프로세서 개수를 가져옴
	_minThreads = minThreads == 0 ? _numCores : minThreads;
	_maxThreads = maxThreads == 0 ? _minThreads : maxThreads;

	for (size_t i = 0; i < _minThreads; ++i)
		AddThreadToPool();
}

/// <summary>
/// thread pool 갯수 만큼 std::thread를 _workers에 쳐 넣는다.
///dwk: 2025-01-21 09:33
/// </summary>
void UcStdThreadPool::AddThreadToPool()
{
	// 각 thread는 loop안에서 기다리고 있다가, _tasks에 일감이 들어오면 헤치운다.
	{
		std::lock_guard<std::mutex> lock(_queueMutex);

		// 현재 _stop 상태인지 확인
		if (_stop) {
			ASSERT("Cannot add threads to a stopped ThreadPool." == nullptr);
			return;
		}

		// 최대 스레드 제한 확인
		if (_numThreads >= _maxThreads) {
			ASSERT("Maximum thread limit reached. Cannot add more threads." == nullptr);
			return;
		}
	}

	_workers.emplace_back([this]
		{//각 스레드 람다: 
			for (;;)
			{
				/// EnqueueTaskItem에서 promise->set_value() 포함하여 새로 만든 람다함수
				std::function<void()> fncInside;
				{
					std::unique_lock<std::mutex> lock(_queueMutex);
					_condition.wait(lock, [this] {
						return _stop || !_tasks.empty();//이 조건으로 기다리다
						});
					//AI:  _stop = true가 설정되더라도 스레드 풀 안에 남아 있는 작업(_tasks)은 모두 실행됩니다.
					//AI: wait은 스레드 효율성을 위해, if는 정확한 종료를 보장하기 위해 각각 사용됩니다.
					if (_stop && _tasks.empty())
						return;//위에 wait 조건 인데 반복하여 한번더 체크

#if CPP17_OR_LATER
					auto [fnc, tikEnque, sFile, nLine] = std::move(_tasks.front());//처리할 람다 함수를 끄집어 내서
#else
					auto tuple_val = std::move(_tasks.front());
					auto fnc = std::get<0>(tuple_val);
					auto tikEnque = std::get<1>(tuple_val);
					auto sFile = std::get<2>(tuple_val);
					auto nLine = std::get<3>(tuple_val);
#endif
					fncInside = fnc;// std::move(_tasks.front());//처리할 람다 함수를 끄집어 내서
					_tasks.pop();//하나 제거 하고
				}
				/// 임시 테스크에서 다 catch한다.
				fncInside();//그걸 백그라운드 thread 에서 실행 한다.
			}
		});
	_numThreads++;

}
/// <summary>
/// 맨 뒤의 스레드를 제거. 가동중인지 체크해야 해서 좀 복잡
///dwk: 2025-01-21 09:33
/// </summary>
void UcStdThreadPool::RemoveThreadFromPool()
{
	if (_numThreads <= _minThreads)
		return;// 최소 스레드 제한: 제거 불가

	// 종료 신호를 전달하기 위해 _stop 신호를 임시적으로 설정
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		_stop = true; // 해당 스레드가 종료 조건을 만족하도록 유도
	}
	_condition.notify_one(); // 스레드를 깨움

	// 맨 뒤의 스레드 join 후 제거
	if (!_workers.empty()) {
		std::thread& threadToRemove = _workers.back();
		if (threadToRemove.joinable()) {
			threadToRemove.join(); // 스레드가 종료될 때까지 기다림
		}
		_workers.pop_back(); // 스레드 벡터에서 제거
		_numThreads--;
	}
	// `_stop` 플래그 복원 (다른 스레드가 영향을 받지 않도록)
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		_stop = false;
	}
}


void UcStdThreadPool::EnqueueTaskItem(std::function<void()> fncIn, std::shared_ptr<std::promise<int>> promise, LPCWSTR sFile, int nLine)
{
	auto enqueueTime = GetTickCount64();// std::chrono::steady_clock::now(); // Enqueue 시각 계산 : chrono는 너무 복잡하군
	std::unique_lock<std::mutex> lock(_queueMutex);
	if (_stop)//StopPool() 하고서 EnqueueTask 하면 안되
		throw std::runtime_error("StopPool() 하고서 EnqueueTask 하면 안되.");
	//_tasks.emplace(std::move(fncIn));
	///람다함수를 부가 작업"을 붙이기 위해 포장한다.즉, 뒤에 종료 신호를 보낸다.
	auto fncIn2 = [this, fncIn, promise, enqueueTime]() -> void {
		auto delayMs = GetTickCount64() - enqueueTime;
		//std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - enqueueTime);
		if (delayMs > 500) { // 지연이 500ms 초과 delayMs.count()
			DWKFUNCV(L"Task delay: %lld ms", delayMs);
		}
		try {
			fncIn(); /// void 람다 함수 안에서 에러 처리 까지 다 함.
			if (promise)
				promise->set_value(0); // 작업 완료 
		}
		catch (CException* e) {
			CString sbuf;
			e->GetErrorMessage(sbuf.GetBuffer(1024), 1024); sbuf.ReleaseBuffer();
			TRACE(_T("CException(%s)\n"), sbuf.GetString());
			if (_OnError)
				_OnError(_T("CException"), std::move(sbuf));
			ASSERT(0);
			if (promise)
				promise->set_value(-1);
		}
		catch (std::exception e) {
			TRACE("std::exception(%s)\n", e.what());
			CString sbuf(e.what());
			if (_OnError)
				_OnError(_T("std::exception"), std::move(sbuf));
			ASSERT(0);
			if (promise)
				promise->set_value(-2);
		}
		catch (...) {
			TRACE("catch ...\n");
			if (_OnError)
				_OnError(_T("..."), _T("(unknown)"));
			ASSERT(0);
			if (promise)
				promise->set_value(-9);
		}
		//catch (...) {
		/// 위에 방법은 future.get()으로 예외까지 - 값으로 받으려 할 때
		/// promise->set_exception(std::current_exception()); 하면 future.get()으로 받을 때, 예외가 다시 발생
		//}
		MonitorAndAddThreads((size_t)delayMs);// .count());
		};
	auto stk = UcPrintStack();
	TplFLSI tpl = std::make_tuple(fncIn2, enqueueTime, sFile, nLine);//TplFnc을 주니 뒤에 <> 리스트를 줄 필요 없다.
	//auto tpl = make_tuple<function<void()>, LONGLONG, std::wstring, int>(funcIn2, enqueueTime, sFile, nLine);
	_tasks.emplace(tpl);
	_condition.notify_one();
}

/// <summary>
/// delayMs는 큐에 넣은 후 실행 되기 까지 걸린 시간인데,, 
/// 이거만 검사 하는게 아니고 _tasks에 밀려 있는 갯수가 스레드 할당의 두배가 넘게 적체 된 경우까지 검사 하는거 맞지? 
/// </summary>
/// <param name="delayMs"></param>
void UcStdThreadPool::MonitorAndAddThreads(size_t delayMs)
{
	DWKFUNC;
	std::unique_lock<std::mutex> lock(_queueMutex);
	// 지연 시간 및 큐 크기 확인
	if ((delayMs > 500 || _tasks.size() > _numThreads * 2) && _numThreads < _maxThreads) {
		DWKTRACE(L"Adding thread: delay = %lld ms, task queue size = %lld", delayMs, _tasks.size());
		AddThreadToPool();
	}
}
#ifdef _Sample__
int main() {
	UcStdThreadPool pool(4);
	auto future = APP->EnqueueTask([]() {
		DWKFUNC;
		Sleep(2000);
		DWKTRACE(L"BG 작업: 스레드 풀에서\n");
		}, __FUNCTIONW__, __LINE__);

	// 태스크를 Enqueue하고 결과를 기다림
	auto future1 = pool.EnqueueTask([]() {
		std::cout << "Task 1 is running...\n";
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::cout << "Task 1 is done.\n";
		});

	std::cout << "Waiting for Task 1...\n";
	future1.wait(); // Task 1의 완료를 기다림
	std::cout << "Task 1 completed.\n";

	// 블럭 되지 않게 wait()대신 get()을 쓰면서 이중 스레드 방식
	std::thread([]() {
		std::string result = pool.EnqueueTask([] {

			// do something...
			//std::this_thread::sleep_for(std::chrono::seconds(2)); // 메인 BG작업

			return "Data fetched";
			}).get(); // 작업 결과를 기다림

		// do with result of BG
		//std::cout << "Result: " << result << std::endl;//BG작업 리턴값 처리

		}).detach();

	return 0;
}
#endif // _Sample__

void UcThread::StartThread(std::function<void()> job, LPCSTR tag /*= nullptr*/, std::function<void(UcThread*)> onThError /*= nullptr*/, LPCSTR sFile /*= nullptr*/, int nLine /*= 0*/)
{
	//if (tag)
	//	_tag = CStringW(tag).GetString();
	//if (_tag.empty())
	//	_break;
	//DWKFUNCV(L"BEGIN [[[ tag:%v", _tag);
	ASSERT(!_th.joinable());
	//if (onThError)
	//	_onThreadError = onThError;
	//if (sFile)
	//	_sFile = sFile;
	//if (nLine)
	//	_nLine = nLine; 
	
	///?주의: this가 스택변수로, Detatch 하여, 사라졌을 수도 있음. 필요한건 모두 캡쳐해서 넘겨야.
	_th = std::thread([this, job = std::move(job), onThError = std::move(onThError), tag1 = CStringW(tag), sFile = CStringA(sFile), nLine = nLine]() mutable {
		// KException이 발생 한 경우 스레드 키로 KException에 보관 해두었다가, 나갈때 제거 한다.
		// 같은 스레드 니까 여기 블럭에서 쓰거나(예외발생) 제거된다.
		/// 현재 스레드의 태그를 보관 해두고 KException이 발생 하면 그 태그를 로그에 찍을 수 있게 한다.
		KException::KExTag d_tag(tag1.GetString());
		DWKFUNCV(L"dwk:[[[%v", tag1);
		try {
			try {
				job();//job = nullptr;   // 이 경우 mutable 이 필요
			}
			UCCATCH_ALL;/// 이것은 rethrow 하기 때문에 다시 try{}catch{}해야 한다.
			///KException.s_fncExceptionDealer를 채워야 로그 처리가 된다.
		}
		catch (...) {
			//auto stk = UcPrintStack();//이걸로 추적이 안된다.
			DWKTRACE(L"ERROR ###tag:%v, %v(%v)", tag1, sFile, nLine);
			if (onThError)
				onThError(this);
			_break;
		}
		DWKTRACE(L"dwk:]]]%v", tag1);
		});
	ASSERT(_th.joinable());
}

CSyncAutoLock::CSyncAutoLock(CSyncObject* pObject, BOOL bInitialLock,
	LPCSTR sFile, int iLine, LPCSTR sobj)
	: CSingleLock(pObject, FALSE)//FALSE 이므로 여기서 Lock안한다.
	, m_sFile(sFile)
	, m_iLine(iLine)
	, m_sObj(sobj)
{

	auto pCS = static_cast<CUcCriticalSection*>(m_pObject);
	const CRITICAL_SECTION& sect = pCS->m_sect;
#ifdef _DEBUG
	long lc = sect.LockCount;// -1 => lock -2
	long rc = sect.RecursionCount;// 0,1,2 계속 증가 한다.
#endif // _DEBUG
	HANDLE pth = sect.OwningThread;// null -> lock thread id
	if (bInitialLock)
		Lock();////constructor 에서 부르므로 virtual 제거. cppcheck
	m_ull = GetTickCount64();

}

CSyncAutoLock::~CSyncAutoLock()
{

	ULONGLONG ull = GetTickCount64();
	DWORD usp = (DWORD)(ull - m_ull);
	if (usp > 100)
	{
		// 		KTrace(L"Unlock\t%u\t%s %s(%d)\n", usp, m_sObj, m_sFile, m_iLine);
	}
	auto pObject = static_cast<CUcCriticalSection*>(m_pObject);
	pObject->Decrease();//m_nLocked--;
}


BOOL CSyncAutoLock::Lock(DWORD dwTimeOut /*= INFINITE*/)
{
	auto pCS = static_cast<CUcCriticalSection*>(m_pObject);
	const CRITICAL_SECTION& sect = pCS->m_sect;
#ifdef _DEBUG
	long lc = sect.LockCount;// -1 => lock -2
	long rc = sect.RecursionCount;// 0,1,2 계속 증가 한다.
	//void* pth = sect.OwningThread;// null -> lock thread id
#endif // _DEBUG
	pCS->Increase(); //m_nLocked++;
	// 	BOOL b = __super::Lock(dwTimeOut);
	// 	return b;

	ASSERT(m_pObject != NULL || m_hObject != NULL);
	ASSERT(!m_bAcquired);

	//	m_bAcquired = m_pObject->Lock(dwTimeOut);
	///	m_bAcquired = ((CCriticalSection*)pCS)->Lock(dwTimeOut); 이렇게 하면 CCriticalSection::Lock()에서 overrided Lock을 못 부른다.
	m_bAcquired = pCS->Lock(dwTimeOut);
	return m_bAcquired;
}


BOOL CSyncAutoLock::Unlock()
{
	ASSERT(m_pObject != NULL);
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock();

	// successfully unlocking means it isn't acquired
	return !m_bAcquired;
}

BOOL CSyncAutoLock::Unlock(LONG lCount, LPLONG lpPrevCount /* = NULL */)
{
	ASSERT(m_pObject != NULL);
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock(lCount, lpPrevCount);

	// successfully unlocking means it isn't acquired
	return !m_bAcquired;
}
