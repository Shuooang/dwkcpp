#include "pch.h"
#include "UcBaseTools.h"

//namespace Uc {

#pragma message("__cplusplus=" DWSTR1(__cplusplus))

//__cplusplus=199711L
#ifdef _MSVC_LANG
#pragma message("_MSVC_LANG=" DWSTR(_MSVC_LANG))
#endif
#if CPP17_OR_LATER
DWKWARN("C++17 is supported. in "DWSTR(PROJECT_NAME))
#else
DWKBLD("C++14 is supported. (Not C++17)")
#endif

// 멀티바이트/유니코드 빌드 확인
#ifdef _UNICODE
DWKBLD("_UNICODE defined.")
#else
DWKBLD("_MBCS MULTIBYTE supported: TCHAR = char (Not _UNICODE)")
#endif

#ifdef _MBCS
//DWKBLD("dwk: _MBCS defined.")
#endif





// C++14 호환성을 위한 static 멤버 정의
#if CPP_BEFORE_17
// UcBaseTools.h의 static 멤버들
std::unordered_map<void*, std::weak_ptr<void>> s_sharedInstances;
std::mutex s_saredTempMtx;
#endif


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
//};// namespace Uc
#ifdef _DEBUG

class MyObj4 {
public:
	MyObj4(int i) : x(i) {}
	int x{ 0 };
	int y{ 0 };
};
class MyObj5 : public CObject {
public:
	DECLARE_SERIAL(MyObj5);
	MyObj5() = default;
	MyObj5(int i) : x(i) {}
	int x{ 0 };
	int y{ 0 };
	virtual void Serialize(CArchive& ar) {

	}
	BOOL operator==(const MyObj5& a) const { return x == a.x && y == a.y; }
	BOOL operator!=(const MyObj5& a) const { return !operator==(a); }

};
IMPLEMENT_SERIAL(MyObj5, CObject, 1)
// 🔹 MyObj5를 명시적으로 인스턴스화 (해당 `.cpp`에서 반드시 추가!)
template class KList<shared_ptr<MyObj5>>;//template class를 type을 적용한 것을 선언만 하는 듯.
/// <summary>
/// 아래 3개의 객체 빌드 테스트 하려면 사용 코드가 있어야 템플릿 객체 빌드 시도 한다.
/// </summary>
void SampleList() {//dwk: 2025-02-24 17:27 KList 테스트 
	KList<MyObj4> l1 = { MyObj4(1),MyObj4(2),MyObj4(3), };
	l1.AddHead(MyObj4(10));
	auto pos1 = l1.GetHeadPosition();
	auto pos1n = l1.GetNext(pos1);
	auto pos1t = l1.GetTailPosition();
	auto pos1p = l1.GetPrev(pos1t);
	KPtrList<MyObj4> l2 = { new MyObj4(1), new MyObj4(2), new MyObj4(3), };
	l2.AddHead(new MyObj4(10));
	auto pos2 = l2.GetHeadPosition();
	auto pos2n = l2.GetNext(pos2);
	auto pos2t = l2.GetTailPosition();
	auto pos2p = l2.GetPrev(pos2t);
	KList<SHP<MyObj5>> l3 = { NEWSHP(MyObj5, 1), NEWSHP(MyObj5, 2), NEWSHP(MyObj5, 3), };
	//KSharedPtrList<MyObj5> l3 = { NEWSH<MyObj5>(1), NEWSH<MyObj5>(2), NEWSH<MyObj5>(3), };
	l3.AddHead(NEWSHP(MyObj5, 1));
	auto pos3 = l3.GetHeadPosition();
	auto pos3n = l3.GetNext(pos3);
}
#endif // _DEBUG
#if CPP_BEFORE_17

std::filesystem::path std::filesystem::path::parent_path() const
{
	size_t pos = m_path.find_last_of("/\\");
	return pos != std::string::npos ? path(m_path.substr(0, pos)) : path();
}

std::filesystem::path std::filesystem::path::filename() const
{
	size_t pos = m_path.find_last_of("/\\");
	return pos != std::string::npos ? path(m_path.substr(pos + 1)) : path(m_path);
}


bool std::filesystem::exists(const path& p)
{
	// Windows API 사용
	return GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool std::filesystem::create_directories(const path& p)
{
	// Windows API 사용
	return CreateDirectoryA(p.c_str(), NULL) != 0;
}
#endif // CPP_BEFORE_17
