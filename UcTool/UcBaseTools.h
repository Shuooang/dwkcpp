#pragma once
#include <wtypes.h>

#include "UcExport.inl"//UCTOOLDYNAMIC

#define UCBASETOOLS



#define DWSTR1(x) #x
#define DWSTR(x) DWSTR1(x)

#include "UcPragma.inl"


#if defined(_MSVC_LANG)
#define UC_LANG _MSVC_LANG
#else
#define UC_LANG __cplusplus
#endif
inline LPCWSTR GetDevIDE()
{
#if _MSC_VER >= 1940
	return L"VS2025";
#elif _MSC_VER >= 1930
	return L"VS2022";
#elif _MSC_VER >= 1920
	return L"VS2019";
#else
	return L"VS(unknown)";
#endif
}
#if UC_LANG >= 201703L
//DWKWARN("C++17 is supported. in "DWSTR(PROJECT_NAME))
#define CPP17_OR_LATER 1
#define CPP_BEFORE_17 0 
#define INLINE_STATIC inline static
#define EXTERN_STATIC inline static
#else
DWKWARN("C++14 is supported. (Not C++17) in "DWSTR(PROJECT_NAME))
#define CPP17_OR_LATER 0 ///?주의: #ifdef를 쓰면 안됨. 반드시 #if 를 써야함.
#define CPP_BEFORE_17 1 //C++14 
#define INLINE_STATIC static
#define EXTERN_STATIC extern
#endif
///props파일에 이게 추가 되어 있군. props파일이 없으면, 프로젝트마다 속성에	추가 해야 함.
//  <ItemDefinitionGroup>
//   <ClCompile>
//    <PreprocessorDefinitions>
//      PROJECT_NAME="$(ProjectName)";
//      %(PreprocessorDefinitions)
//    </PreprocessorDefinitions>
//   </ClCompile>
//  </ItemDefinitionGroup>` 


//
// MSVC의 표준 버전 매크로
//기본 설정에선 cplusplus 값이 199711L로 고정됩니다. 실제 표준 버전은 MSVC_LANG를 쓰세요.
// /Zc:cplusplus 컴파일 옵션을 켜면 _cplusplus가 올바른 값(201402L, 201703L 등)을 가집니다.
//PCH(미리 컴파일 헤더) 영향
// C++14와 C++17 호환성을 위한 매크로
#if CPP17_OR_LATER
//#define UC_ANY14_ 0
#define STRUCTURED_BINDING_SUPPORTED 1
#define IF_WITH_INIT_SUPPORTED 1
#define STD_ANY_SUPPORTED 1
#define STD_OPTIONAL_SUPPORTED 1
#define STD_STRING_VIEW_SUPPORTED 1
#else
//#define UC_ANY14_ 1 // C++14용 std_any 구현 사용
#define STRUCTURED_BINDING_SUPPORTED 0
#define IF_WITH_INIT_SUPPORTED 0
#define STD_ANY_SUPPORTED 0
#define STD_OPTIONAL_SUPPORTED 0
#define STD_STRING_VIEW_SUPPORTED 0
#endif

#include <functional>
#include <string>
#if CPP17_OR_LATER
#include <string_view>
#endif
#include <sstream>
#include <set>
#if CPP17_OR_LATER
#include <any>
#endif
#include <map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>



//-----------------------------------------------------------------------------
// ?? fold expression 대체
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
template<typename... Args>
constexpr auto fold_sum(Args... args) -> decltype(int(), void(), 0) {
	using swallow = int[];
	int result = 0;
	(void)swallow {
		(result += args, 0)...
	};
	return result;
}
#else
template<typename... Args>
constexpr auto fold_sum(Args... args) {
	return (... + args);
}
#endif

//-----------------------------------------------------------------------------
// ?? std::clamp / std::size / std::data / std::empty 백포트
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17x
namespace std {
	template<typename T>
	constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
		return (v < lo) ? lo : (hi < v) ? hi : v;
	}

	template<typename C>
	constexpr auto UcSize(const C& c) -> decltype(c.size()) { return c.size(); }

	template<typename T, size_t N>
	constexpr size_t UcSize(const T(&)[N]) noexcept { return N; }

	template<typename C>
	constexpr auto UcData(C& c) -> decltype(c.data()) { return c.data(); }

	template<typename T, size_t N>
	constexpr T* UcData(T(&arr)[N]) noexcept { return arr; }

	template<typename C>
	constexpr bool UcEmpty(const C& c) -> decltype(c.empty()) { return c.empty(); }

	template<typename T, size_t N>
	constexpr bool UcEmpty(const T(&)[N]) noexcept { return false; }
}
#endif

//-----------------------------------------------------------------------------
// ?? std::byte 대체
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
//namespace std {
//	enum class byte : unsigned char {};
//	template <class IntType>
//	constexpr byte operator<<(byte b, IntType shift) noexcept {
//		return static_cast<byte>(static_cast<unsigned char>(b) << shift);
//	}
//}
#endif
#if CPP17_OR_LATER
#define SET_ENUM_MINMAX(EnumName, min1, max2) \
namespace magic_enum::customize {\
template <> struct enum_range<EnumName> {\
	static constexpr int min = min1;	static constexpr int max = max2;\
};}
#else
// C++14에서는 magic_enum을 사용할 수 없으므로 빈 매크로로 정의
#define SET_ENUM_MINMAX(EnumName, min1, max2)
#endif


// C++14용 std_any 구현
#if CPP_BEFORE_17
#include "UcAny14.h"
#endif // CPP_BEFORE_17

// C++14/17 구분하여 std_any 별칭 정의
#if CPP17_OR_LATER
using std_any = std::any;
template<typename T>
auto std_any_cast(const std_any& operand) -> decltype(std::any_cast<T>(operand)) {
	return std::any_cast<T>(operand);
}
#else
using std_any = std14::any;
// 전제: using std_any = std14::any;

template<typename T>
inline T std_any_cast(const std_any& operand) {
	using U = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	return static_cast<T>(std14::any_cast<const U&>(operand));
}

template<typename T>
inline T std_any_cast(std_any& operand) {
	using U = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	return static_cast<T>(std14::any_cast<U&>(operand));
}

template<typename T>
inline T std_any_cast(std_any&& operand) {
	using U = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	return static_cast<T>(std14::any_cast<U&&>(std::move(operand)));
}

// 포인터 형태도 필요하면 함께 제공
template<typename T>
inline const T* std_any_cast(const std_any* p) noexcept { return std14::any_cast<T>(p); }

template<typename T>
inline T* std_any_cast(std_any* p) noexcept { return std14::any_cast<T>(p); }
#endif

//-----------------------------------------------------------------------------
// ?? std::optional / variant / any 대체 (Boost or custom fallback)
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
/// 일단 optional 을 쓰던 곳을 다 제거 했다.
// Boost가 없는 경우 간단한 대체 구현
namespace std {
	template<typename T>
	class optional {
	private:
		bool m_has_value;
		T m_value;
	public:
		optional() : m_has_value(false) {}
		optional(const T& value) : m_has_value(true), m_value(value) {}
		bool has_value() const { return m_has_value; }
		const T& value() const { return m_value; }
		T& value() { return m_value; }
		const T& operator*() const { return m_value; }
		T& operator*() { return m_value; }
	};

	struct nullopt_t {};
	static constexpr nullopt_t nullopt;
}
#endif

//-----------------------------------------------------------------------------
// ?? std::filesystem 대체
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
// Boost가 설치되어 있는 경우에만 포함
//#ifdef BOOST_VERSION
//#include <boost/filesystem.hpp>
//namespace std {
//	namespace filesystem = boost::filesystem;
//}
//#else
// Boost가 없는 경우 간단한 대체 구현 (필요한 기능만)
namespace std {
	namespace filesystem {
		class path {
		private:
			std::string m_path;
		public:
			path() = default;
			path(const std::string& p) : m_path(p) {}
			path(const char* p) : m_path(p) {}

			const std::string& string() const { return m_path; }
			const char* c_str() const { return m_path.c_str(); }

			path parent_path() const;

			path filename() const;

			bool empty() const { return m_path.empty(); }
		};

		// 간단한 파일 존재 확인
		bool exists(const path& p);

		// 간단한 디렉토리 생성
		bool create_directories(const path& p);
	}
}
//#endif boost
#endif

//-----------------------------------------------------------------------------
// ?? std::invoke / apply / make_from_tuple 대체
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
namespace std {
	template <typename F, typename Tuple, size_t... I>
	constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
		return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
	}

	template <typename F, typename Tuple>
	constexpr decltype(auto) apply(F&& f, Tuple&& t) {
		return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
	}
}
#endif

//-----------------------------------------------------------------------------
// ?? std::uncaught_exceptions() 대체
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
namespace std {
	inline int uncaught_exceptions() noexcept { return std::uncaught_exception() ? 1 : 0; }
}
#endif

//-----------------------------------------------------------------------------
// ?? std::shared_mutex 대체
//-----------------------------------------------------------------------------
#if CPP_BEFORE_17
// Boost가 설치되어 있는 경우에만 포함
//#ifdef BOOST_VERSION
//#include <boost/thread/shared_mutex.hpp>
//namespace std {
//	using shared_mutex = boost::shared_mutex;
//	using shared_lock = boost::shared_lock<boost::shared_mutex>;
//}
//#else
// Boost가 없는 경우 간단한 대체 구현 (Windows API 사용)
// SRWLOCK 타입은 이미 위에서 #include <wtypes.h>로 포함됨
#include <synchapi.h> // InitializeSRWLock, AcquireSRWLockExclusive, ReleaseSRWLockExclusive, AcquireSRWLockShared, ReleaseSRWLockShared
namespace std {
	class shared_mutex {
	private:
		SRWLOCK m_lock;
	public:
		shared_mutex() { InitializeSRWLock(&m_lock); }

		void lock() { AcquireSRWLockExclusive(&m_lock); }
		void unlock() { ReleaseSRWLockExclusive(&m_lock); }
		void lock_shared() { AcquireSRWLockShared(&m_lock); }
		void unlock_shared() { ReleaseSRWLockShared(&m_lock); }
	};

	template<typename Mutex>
	class shared_lock {
	private:
		Mutex* m_mutex;
	public:
		shared_lock(Mutex& m) : m_mutex(&m) { m_mutex->lock_shared(); }
		~shared_lock() { if (m_mutex) m_mutex->unlock_shared(); }
	};
}
//#endif boost
#endif

#define __STD_FUNCTIONW__ std::wstring(__FUNCTIONW__)




/// 가상 함수 바인딩을 피하고 현재 클래스의 함수를 직접 호출
/// THISTYPE::DocSerialize(ar)는 MyClass::DocSerialize(ar)처럼 현재 클래스의 함수를 호출
#define THISTYPE std::remove_reference_t<decltype(*this)>











#include <afxmt.h> //CCriticalSection

#define _UcTool_
//using namespace std; 대신 일부믄 사용 하기 위해. 이전 코드 호환을 위해.
//namespace Uc {
using std::string;
using std::wstring;
using std::shared_ptr;
using std::pair;
using std::tuple;
using std::make_shared;
using std::make_tuple;
using std::function;
using std::vector;
using std::wstringstream;
using std::stringstream;
using std::initializer_list;
using std::dynamic_pointer_cast;

#if CPP17_OR_LATER
using std::wstring_view;// c++17
using std::string_view;
using std::any_cast;
#endif

template<typename T>
using SHP = std::shared_ptr<T>;

enum VType {
	eMpt = VT_EMPTY,
	eNul = VT_NULL,
	eStr = VT_LPWSTR,
	eInt = VT_INT,
	eFlt = VT_R8,
	eTme = VT_FILETIME, // see VT_DATE. 
	//ref: JVal에서 CTime이나 COleDateTime은 UcCTimeToString 로 문자열로 들어간다.
	eI64 = VT_I8,
	eI16 = VT_I2,
	eUnt = VT_UINT,
	eU64 = VT_UI8,
	eBol = VT_BOOL,
	ePtr = VT_PTR,
	eStA = VT_LPSTR,
	eBin = VT_BLOB,
	eArr = VT_ARRAY,
	eObj = VT_BLOB_OBJECT, // see VT_STREAMED_OBJECT VT_STORED_OBJECT
};

#define TO_STR(sm) #sm
#define ENUM2STR(e) {e, #e}
#define ENUM2STRW(e) {e, L#e}

//allocator를 직접 신경 쓸 일이 드문 이유
//std::vector, std::map 같은 컨테이너들은 이미 내부적으로 std::allocator를 기본값으로 씁니다.
//즉, std::vector<int> == std::vector<int, std::allocator<int>>
//우리가 따로 지정하지 않아도 표준 allocator 자동 사용.
//template <typename T>
//using SHPAlloc = std::allocator<SHP<T>>;

///ex: auto pEntry = DSHCAST<FOPColorEntry>(shPtr);
template <typename T, typename U>
inline std::shared_ptr<T> SSHCAST(const std::shared_ptr<U>& ptr) {
	return std::static_pointer_cast<T>(ptr);
}
template <typename T, typename U>
inline std::shared_ptr<T> DSHCAST(const std::shared_ptr<U>& ptr) {
	return std::dynamic_pointer_cast<T>(ptr);
}
#define DSHCAST1(T, P) std::dynamic_pointer_cast<T>((P))
//template<typename T>
//using MSHP = std::make_shared<T>; 안된다.

/// not used yet
//#define NEWSH std::make_shared
#define NEWSHP(TYPE, ...) std::make_shared<TYPE>(__VA_ARGS__)
typedef std::wstringstream Tss;
typedef std::stringstream Tas;
//#define std_endl std::endl

//#define MKTP std::make_tuple
#define MKTPL(...) std::make_tuple(__VA_ARGS__)

#ifndef VAL_LINE
#define CONCAT_IMPL(x, y) x##y
#define VAL_LINE(x, y) CONCAT_IMPL(x, y)
#endif // VAL_LINE

///deprecated PRGMSG 대신 FILINDWK 써라.
#ifndef PRGMSG 
#define PRGMSG(msg) __FILE__ "(" _CRT_STRINGIZE(__LINE__) "): dwk - " msg
#endif 
// #pragma message(PRGMSG("메시지 다블클릭하면 코드로 이동"))


//?waring default_delete 는 반드시 넣어 줘야 한다.
#define SharedBuf(len) std::shared_ptr<char>(new char[len]{'\0'}, std::default_delete<char[]>()) // 할당한 전체가 0으로 초기화

// 매크로 안에서 임시로 쓰는 버퍼
//#define SharedBufP(len) VAL_LINE(pbuf,__LINE__) = std::shared_ptr<char>(new char[len]{'\0'}, std::default_delete<char[]>());

#ifndef PWS
typedef LPCWSTR PWS;
typedef LPCTSTR PS;
typedef LPCSTR PAS;
#endif

#ifdef _ATL_VER
#if defined(_AFXDLL)
#define TStrTraitMFC StrTraitMFC_DLL
#else
#define TStrTraitMFC StrTraitMFC
#endif

template<typename TChar>
using TCString = ATL::CStringT<TChar, TStrTraitMFC<TChar>>;
//using
//template<typename TChar>
//void TFunc(const TCString<TChar>& str) {
//	TRACE(str);
//}
#endif
template<typename TChar>
using tstring = std::basic_string<TChar, std::char_traits<TChar>, std::allocator<TChar>>;
//using
//template<typename TChar>
//void TFunc(const tstring<TChar>& str) {
//	TRACE(str);
//}

#define _break do {int ____i = 0;} while(0)

#define MAKE_ULONGLONG(high, low) ((ULONGLONG)(high) << 32 | (ULONGLONG)(low))


/// int 와 long을 혼동 해서 안씀
//template<typename T>
//const T& UcMin(const T& a, const T& b){
//	return (a < b) ? a : b;
//}
//
//template<typename T>
//const T& UcMax(const T& a, const T& b){
//	return (a > b) ? a : b;
//}
#define UcMin(a, b) ((a) < (b) ? (a) : (b))
#define UcMax(a, b) ((a) > (b) ? (a) : (b))






/// UDL은 반드시 네임스페이스 using으로 가져와야만 쓸 수 있어요.
#include <chrono>
//#include <iostream>
using namespace std::chrono_literals;// 1나노초가 단위
//	auto a = 1h;       // 1시간
//	auto b = 30min;    // 30분
//	auto c = 45s;      // 45초
//	auto d = 500ms;    // 500밀리초
//	auto e = 200us;    // 200마이크로초
//	auto f = 100ns;    // 100나노초
//auto d1 = 3s;       // std::chrono::seconds
//auto d2 = 1500ms;   // std::chrono::milliseconds
//long long v1 = d1.count(); // 1us는 1'000, 1ms는 1'000'000 를 리턴
/// <chrono> 안에 이미 정의된 UDL들은 모두 std::chrono::duration 계열 타입을 리턴합니다.
/// long long으로 바꾸려면 .count() 멤버 함수를 쓰면 됩니다.
/// 





/// 단위는 반드시 소문자여야 한다. _MS, _Ms 등은 안된다.
/// 아래 세개는 단위가 밀리초일 사용
constexpr long long operator"" _sec(unsigned long long nSec) { return nSec * 1000; }
constexpr long long operator"" _min(unsigned long long nMinute) { return nMinute * 60 * 1000; }
constexpr long long operator"" _hour(unsigned long long nHour) { return nHour * 60 * 60 * 1000; }

/// <summary>
/// 주어진 각도를 라디안으로 변환하는 사용자 정의 리터럴 연산자입니다.
/// ex: auto angleInRadians = 90.0_degree; // 90도를 라디안으로 변환
/// </summary>
/// <param name="dRadian">변환할 각도(도 단위) 값입니다.</param>
/// <returns>도 단위 입력값을 라디안으로 변환한 결과(long double 타입)입니다.</returns>
constexpr long double operator"" _degree(long double dRadian) {
	return dRadian * 3.14159265358979323846 / 180.0;  // 각도를 라디안으로 변환
}

/// 특정 비트 검사
inline bool UcAttr(int val, int attr) {
	return (val & attr) == attr;
}

#define for_each0(n)  for(int _i=0;_i<(n);_i++)

template<typename TOBJ>
inline void DeleteMeSafe(TOBJ*& p)
{
	if (p)
	{
		//TOBJ::operator delete(p);
		delete p; //TOBJ 는 반드시 정의된 clas 형태이어야 한다.
		p = NULL;
	}
}

#ifndef ORANY_FNC
template <typename TVAL>
inline BOOL UcOrAny(TVAL v, std::set<TVAL> ari)
{
	return ari.find(v) != ari.end();
}
template<typename T, typename... Args>
bool IsOneOf(T ch, Args... args) {
#if CPP17_OR_LATER
	return ((ch == args) || ...); // fold expression (C++17)
#else
	// C++14 버전: 재귀적 템플릿 사용
	bool results[] = { (ch == args)... };
	for (bool result : results) {
		if (result) return true;
	}
	return false;
#endif
}
//#define IsOneOf(ch, ...) IsOneOf(ch, __VA_ARGS__)
#endif // ORANY_FNC

// trim from end of string (right)
inline std::string& strrtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}
// trim from beginning of string (left)
inline std::string& strltrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}
// trim from both ends of string (right then left)
inline std::string& strtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	return strltrim(strrtrim(s, t), t);
}
// trim from end of string (right)
inline std::wstring& wstrrtrim(std::wstring& s, const wchar_t* t = L" \t\n\r\f\v")
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}
// trim from beginning of string (left)
inline std::wstring& wstrltrim(std::wstring& s, const wchar_t* t = L" \t\n\r\f\v")
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}
// trim from both ends of string (right then left)
inline std::wstring& wstrtrim(std::wstring& s, const wchar_t* t = L" \t\n\r\f\v")
{
	return wstrltrim(wstrrtrim(s, t), t);
}

// 이 객체는 C++에서 스택에서 벗어 날때, 객체가 소멸 할때, 미리 할일을 예약 한다.
/// RAII(Resource Acquisition Is Initialization) 패턴의 일종으로 보입니다.
/// 이 패턴은 자원의 수명을 객체의 수명과 결합시켜 자동으로 자원을 관리하는 방법입니다.
class KAtEnd
{
public:
	std::function<void()> m_lambda;
	bool _bFinished{ false };

	explicit KAtEnd(std::function<void()> lambda)
		: m_lambda(std::move(lambda))	{
	}
	KAtEnd(KAtEnd&& other) noexcept
		: m_lambda(std::move(other.m_lambda)), _bFinished(other._bFinished) {
		other._bFinished = false;
	}
	~KAtEnd() { DoNow(); }
	/// 끝낸다. Done을 끝에 다다르지 전에 미리 부를 수 있게 하기 위해 만든다.
	void DoNow()	{
		if (!_bFinished && m_lambda)		{
			m_lambda();
			_bFinished = true;
		}
	}
	///에약된 람다 함수를 끝에가서 실행하는 것을 취소 하도록 하기 위해 만든다.
	void Abort()	{
		_bFinished = true;
	}
	int _touched{ 0 };
	void Touch()	{
		++_touched;
	}
	/* a sample:
		IStream* stream = (IStream*)responseVariant.punkVal;
		KAtEnd d_stream([&]() {
				if(stream)
					stream->Release();
		});
		CHAR *szBuff = new CHAR[csz];
		//CAutoFreePtr<CHAR> au(szBuff);
		KAtEnd d_szBuff([szBuff]() {
				delete szBuff;
		});

	*/
};

//[[deprecated("KDefer is deprecated, instead use KAtEnd.")]]  // 경고 메시지 추가
/// KAtEnd의 간단한 버전
class KDefer {
public:
	std::function<void(void)> _fnc;
	KDefer() = delete;
	KDefer(std::function<void(void)> fnc) { _fnc = fnc; }
	~KDefer() { if (_fnc) _fnc(); }
	void Abort() { _fnc = nullptr; }
};

//dwk: 2025-04-18 11:20  
#ifndef throw_std_out_of_range
#define throw_std_out_of_range(msg) while(1){char __buf__[1024]{};\
	sprintf_s(__buf__, "%s(%d):std_info-\t%s",__FILE__,__LINE__,(msg));\
	throw std::out_of_range(__buf__); break;}
#endif

template<typename TKey, typename TVal>
class KStdMap
	: public std::map<TKey, TVal>
{
public:
	/// 키를 이 안에서만 쓰는것을 모아둔다.
	std::set<TKey>* _staticKeys{ NULL };
	void SetKeys(std::set<TKey>& staticKeys)
	{
		_staticKeys = &staticKeys;
	}
	void CheckKey(TKey key) const
	{
		auto keys = _staticKeys;//(std::set<TKey>*)
		if (keys && keys->size() > 0 && keys->find(key) == keys->end())
		{
			throw_std_out_of_range("The Key can't be used.");
		}
	}

	/// staticKeys: 로컬변수를 쓰면 큰일남. static set<>이거나, 수명긴 멤버변수 이어야 한다.
	explicit KStdMap(std::set<TKey>& staticKeys) : _staticKeys(&staticKeys) {}

	KStdMap() {}//다른 constructor이 있을때는 기본생성자를 만들어줘야한다.

	///KStdMap<int, char> m = {{1, 'a'}, {3, 'b'}, {5, 'c'}, {7, 'd'}}; 이렇게 초기화 하려면
	KStdMap(std::initializer_list<std::pair<TKey, TVal>> ar)//& ar 도 실패
	{
		for (const auto& a : ar)
			this->insert(a);
	}
	virtual BOOL Lookup(TKey k, TVal& v)
	{
		if (this == nullptr)
			return FALSE;
		//throw "KStdMap::Lookup() this is null.";
		if (this->size() == 0)
			return FALSE;
#ifdef _DEBUG
		CheckKey(k);
#endif // _DEBUG
		auto it = this->find(k);
		if (it != this->end())
		{
			v = it->second;
			return TRUE;
		}
		return FALSE;
	}

	TVal* Lookup(TKey k)
	{
		if (this == nullptr)
		{
			TRACE(L"Warning: KStdMap::Lookup() this is null.\n");
			return NULL;
		}
		//throw "KStdMap::Lookup() this is null.";
		if (this->size() == 0)
			return NULL;
#ifdef _DEBUG
		CheckKey(k);
#endif // _DEBUG
		auto it = this->find(k);
		return it != this->end() ? &it->second : NULL;
#ifdef _sample__
		// ------------- for string -------------------
		string spt1;
		if (auto pVal = s_placeType.Lookup(spt))
			spt1 = *pVal;

		// ------------- or for CStringW -------------------
		CStringA spt2;
		if (auto pVal = s_placeType.Lookup((PWS)spt))
			spt2 = pVal->c_str();
#endif // _sample__
	}

	TVal& S(TKey k) {
		auto pv = Lookup(k);
		if (pv) {
			return *pv;
		}
		throw_std_out_of_range("KStdMap::S() key not found");
	}

	bool Has(TKey k)
	{
		if (this == nullptr)
			return false;
		//throw "KStdMap::Has() this is null.";
		if (this->size() == 0)
			return false;
		auto it = this->find(k);
		return (it != this->end());
	}
	void SetAt(const TKey& k, const TVal& v)
	{
		if (this == nullptr)
			throw "KStdMap::SetAt() this is null.";
		(*this)[k] = v;
		///		toString(); 안에 디버그 작업 하고 싶으면 한다.
	}
	//KStdMap<TKey, TVal>* operator[](TKey k) { return (*m_pJobj)[k]; }
	//void operator[](TKey k) {  }

	typename std::map<TKey, TVal>::iterator Find(TKey k)
	{
		if (this == nullptr)
			throw "KStdMap::Find() this is null.";
		auto it = this->find(k);
		return it;
	}
	/// 그냥 for(auto& [k,v] : *this) 쓰면 되는데 굳이 쓴다면 reverse용으로
	virtual void toString()
	{
	}
	TVal Get(const TKey& key)
	{
		TVal v;
		if (Lookup(key, v))
			return v;
		return {};
	}

	size_t GetCount(){//dwk: 2025-12-24 13:39 
		return this->size();
	}
	
	void RemoveAll()
	{
		this->clear();
	}
	BOOL RemoveKey(TKey k)
	{
		auto it = this->find(k);
		if (it != this->end())
		{
			this->erase(it);
			return TRUE;
		}
		return FALSE;
	}
	//C++17
	using std::map<TKey, TVal>::insert; // node_handle 등 기본 오버로드 노출
	void ChangeKey(const TKey& k1, const TKey& k2)
	{
		auto e = this->extract(k1);
		e.key() = k2;
		std::map<TKey, TVal>::insert(std::move(e));
	}

	std::pair<typename std::map<TKey, TVal>::iterator, bool> insert(const std::pair<const TKey, TVal>& value)
	{
		return std::map<TKey, TVal>::insert(value);
	}
	TVal& operator[](const TKey& key)
	{
#ifdef _DEBUG
		CheckKey(key);
#endif // _DEBUG
		if (this->find(key) == this->end())
		{
			insert(pair<TKey, TVal>(key, TVal()));
		}
		return std::map<TKey, TVal>::operator[](key);
	}
	const TVal& operator[](const TKey& key) const
	{
#ifdef _DEBUG
		CheckKey(key);
#endif // _DEBUG
		if (this->find(key) == this->end())
		{
			throw_std_out_of_range("Key not found in KStdMap");
		}
		auto it = this->find(key);//+	it	("k2", 10)	std::_Tree_const_iterator<std::_Tree_val<Tree<pair<string<char> const ,int> > > >
		//TRACE("KStdMap: %d\n", it->second);// (CONST int&)mtsc["k2"]);
		return it->second;// std::map<TKey, TVal>::operator[](key);
	}
	///const 멤버 함수를 정의하는 이유는, const 객체에서도 operator[]를 호출할 수 있도록 하기 위해서입니다. 

	/// Ex 붙은 것은 리턴값에 따라 break 여부가 정해진다.
	template<typename FNC>
	bool for_loopEx(FNC lambda)
	{
		// cppcheck consider
		return !std::any_of(this->begin(), this->end(),
			[&lambda](const auto& x) { return !lambda(x.first, x.second); });
		///Id: useStlAlgorithm	CWE : 398	Consider using std::any_of algorithm instead of a raw loop.
		//for (auto const& x : *this)
		//	if (!lambda(x.first, x.second))
		//		break;
		// c++17
// 		for (auto&[k, v] : *this)
// 			if (!lambda(k, v))
// 				break;
	}
	template<typename FNC>
	void for_RLoopEx(FNC lambda)
	{
		for (auto it = this->rbegin(); it != this->rend(); it++)
		{
			if (!lambda(it->first, it->second))
				break;
		}
	}

	//sample:
	//	_mapCnt.for_loop([&](int st, int cnt) -> void {
	// 		_mapCnt[st] = 0;
	// 		});
	template<typename FNC>
	void for_loop(FNC lambda)
	{
		for (auto const& x : *this)
			lambda(x.first, x.second);
		// c++17
// 		for (auto&[k, v] : *this)
// 			lambda(k, v);
	}
	template<typename FNC>
	void for_RLoop(FNC lambda)
	{
		for (auto it = this->rbegin(); it != this->rend(); it++)
			lambda(it->first, it->second);
	}
};

#include <unordered_map>
template<typename TKey, typename TVal>
class KStdHashMap
	: public std::unordered_map<TKey, TVal>
{
public:
	KStdHashMap() {}//다른 constructor이 있을때는 기본생성자를 만들어줘야한다.

	///KStdMap<int, char> m = {{1, 'a'}, {3, 'b'}, {5, 'c'}, {7, 'd'}}; 이렇게 초기화 하려면
	KStdHashMap(std::initializer_list<std::pair<TKey, TVal>> ar)//& ar 도 실패
		//: std::map<TKey, TVal>(ar)/// 이거만 실패 이군
	{
		for (auto& a : ar)
			this->insert(a);
	}
	virtual BOOL Lookup(TKey k, TVal& v)
	{
		if (this == nullptr)
			throw "KStdHashMap::Lookup() this is null.";
		if (this->size() == 0)
			return FALSE;
		auto it = this->find(k);
		if (it != this->end())
		{
			v = it->second;
			return TRUE;
		}
		return FALSE;
	}
	bool Has(TKey k)
	{
		if (this == nullptr)
			throw "KStdHashMap::Has() this is null.";
		if (this->size() == 0)
			return false;
		auto it = this->find(k);
		return (it != this->end());
	}
	void SetAt(TKey k, TVal v)
	{
		if (this == nullptr)
			throw "KStdHashMap::SetAt() this is null.";
		(*this)[k] = v;
		///		toString(); 안에 디버그 작업 하고 싶으면 한다.
	}
	//KStdMap<TKey, TVal>* operator[](TKey k) { return (*m_pJobj)[k]; }
	//void operator[](TKey k) {  }

	typename std::unordered_map<TKey, TVal>::iterator Find(TKey k)
	{
		if (this == nullptr)
			throw "KStdHashMap::Find() this is null.";
		auto it = this->find(k);
		return it;
	}
	/// 그냥 for(auto& [k,v] : *this) 쓰면 되는데 굳이 쓴다면 reverse용으로
	virtual void toString()
	{
	}
	TVal Get(TKey key)
	{
		TVal v;
		Lookup(key, v);
		return v;
	}
	BOOL RemoveKey(TKey k)
	{
		auto it = this->find(k);
		if (it != this->end())
		{
			erase(it);
			return TRUE;
		}
		return FALSE;
	}


	/// Ex 붙은 것은 리턴값에 따라 break 여부가 정해진다.
	template<typename FNC>
	bool for_loopEx(FNC lambda)
	{
		// cppcheck consider
		return !std::any_of(this->begin(), this->end(),
			[&lambda](const auto& x) { return !lambda(x.first, x.second); });
		///Id: useStlAlgorithm	CWE : 398	Consider using std::any_of algorithm instead of a raw loop.
		//for (const auto& x : *this)
		//	if (!lambda(x.first, x.second))
		//		break;
		/// c++17
		// 	for (auto&[k, v] : *this)
		// 		if (!lambda(k, v))
		// 			break;
	}
	template<typename FNC>
	void for_RLoopEx(FNC lambda)
	{
		for (auto it = this->rbegin(); it != this->rend(); it++)
		{
			if (!lambda(it->first, it->second))
				break;
		}
	}

	//sample:
	//	_mapCnt.for_loop([&](int st, int cnt) -> void {
	// 		_mapCnt[st] = 0;
	// 		});
	template<typename FNC>
	void for_loop(FNC lambda)
	{
		for (auto const& x : *this)
			lambda(x.first, x.second);
		// c++17
// 		for (auto&[k, v] : *this)
// 			lambda(k, v);
	}
	template<typename FNC>
	void for_RLoop(FNC lambda)
	{
		for (auto it = this->rbegin(); it != this->rend(); it++)
			lambda(it->first, it->second);
	}
};



// 내부 데이터를 디버깅 중에 볼수 있어서 앞으로 std 쪽을 써야 겠다.
/// TKey는 std::string 또는 std::wstring 을 쓴다.
/// TObj는 포인터로 *는 생략 한다.
/// m_bAutoFree를 건드리지 않는 한, 자체 free된다.
/// Linmk error
//[[deprecated]]
template<typename TKey, typename TObj>
class KStdMapPtr
	: public KStdMap<TKey, TObj*>
{
public:

	KStdMapPtr() {
	}
	~KStdMapPtr() {
		DeleteAll();
	}

	int _token{ 0 };

	BOOL DeleteKey(TKey k)
	{
		auto it = this->find(k);
		if (it != this->end())
		{
			TObj* v = it->second;
			DeleteMeSafe(v);
			this->erase(it);
			return TRUE;
		}
		return FALSE;
	}

	BOOL m_bAutoFree{ true };

	void DeleteAll()
	{
		if (m_bAutoFree)
		{
			for (auto it = this->begin(); it != this->end(); it++)
			{
				auto v = it->second;
				DeleteMeSafe(v);
			}
		}
		this->clear();
	}
	TObj* get(TKey k)
	{
		auto it = this->find(k);
		if (it != this->end())
			return it->second;
		return nullptr;
	}

};



template<typename T>
class KArray : public std::vector<T>
{
public:
	//CCriticalSection _cs;
	KArray() {}
	///KArray<int> sic = {1,3,5,7}; 이렇게 초기화 가능
	KArray(std::initializer_list<T> ar)
		: std::vector<T>(ar)//this->insert(ar);
	{
	}
	KArray(int n, T v) {
		for (int i = 0; i < n; i++)
			Add(v);
	}
	void Add(T v) {
		this->push_back(v);
	}
	const T& GetAt(int i) const {
		return this->at(i);
		//return (*this)[i]; error C2440: 'return': cannot convert from 'const _Ty' to 'T &'
	}
	const T& GetObject(int i) const {//dwk: 2025-03-28 11:09 호환 함수를 위해
		return GetAt(i);
	}

	void Insert(const T& value, int index)//dwk: 2025-03-28 13:30  
	{
		if (index < 0 || static_cast<size_t>(index) >= this->size()) {
			this->push_back(value);  // 맨 뒤에 붙이기
		}
		else {
			this->insert(this->begin() + index, value);  // 중간에 삽입
		}
	}
	/// std::vector::insert(iterator, value) 호환 — insert(iter, value) 호출 시 이 오버로드 사용 (C2664 방지)
	typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator position, const T& value) {
		return std::vector<T>::insert(position, value);
	}

	BOOL Found(const T& value) const {
		return Find(value) >= 0;
	}
	int Find(const T& value) const
	{
		auto it = std::find(this->begin(), this->end(), value);
		if (it != this->end()) {
			return static_cast<int>(std::distance(this->begin(), it));
		}
		return -1;  // 못 찾았을 때
	}
	//void SetAt(int i, T v) {
	//	(*this)[i] = v;
	//}
	void SetAt(size_t index, const T& value) {
		if (index >= this->size()) {
			this->resize(index + 1); // ✅ 크기 자동 확장
		}
		(*this)[index] = value;
	}

	int GetCount() const { return (int)this->size(); }
	int GetSize() const { return (int)this->size(); }
	int Count() const { return (int)this->size(); }
	/// CArray 호환: 최대 유효 인덱스 (빈 배열이면 -1)
	int GetUpperBound() const { return GetCount() - 1; }

	int Last()	const {
		return GetCount() - 1;
	}
	void RemoveAt(int i) {
		this->erase(this->begin() + i);
	}
	/// CArray 호환: index부터 count개 요소 제거
	void RemoveAt(int index, int count) {
		if (count <= 0 || index < 0) 
			return;
		size_t uStart = (size_t)index;
		if (uStart >= this->size()) 
			return;
		size_t uEnd = (std::min)(uStart + (size_t)count, this->size());
		this->erase(this->begin() + uStart, this->begin() + uEnd);
	}
	void RemoveAll() {
		this->clear();
	}
	void Clear() {//dwk: 2025-03-28 11:09 호환 함수를 위해 
		this->RemoveAll();
	}
	bool IsEmpty() const {
		return this->empty();
	}
	/// loop
	// 	for(int& el : *this)
	// 		el += i++;
	// 	for(int& el : reverse(*this))
	// 		el += i++;
//<algorithm> c++98
	T& Min() { return *std::min_element(this->begin(), this->end()); }
	T& Max() { return *std::max_element(this->begin(), this->end()); }
	int MinX() { return (int)(std::min_element(this->begin(), this->end()) - this->begin()); }
	int MaxX() { return (int)(std::max_element(this->begin(), this->end()) - this->begin()); }

	void Sort() { sort(this->begin(), this->end()); }
	void Reverse() { std::reverse(this->begin(), this->end()); }

protected:
	size_t partition(size_t left, size_t right, std::function<int(T&, T&)> fcmp)
	{
		auto& v = *this;
		T& pivot = v[right];
		size_t pivot_index = right;

		right--;

		while (1)
		{

			while (fcmp(v[left], pivot) < 0)
				left++;

			while (fcmp(v[right], pivot) > 0)
			{
				if (right == 0)
					break;
				right--;
			}

			if (left >= right)
				break;
			else
			{
				std::swap(v[left], v[right]);
				left++;
			}
		}
		std::swap(v[left], v[pivot_index]);

		return left;
	}
	void quick_sort(size_t left, size_t right, std::function<int(T&, T&)> fcmp)
	{
		if (left >= right)
			return;
		size_t pivot = partition(left, right, fcmp);
		if (pivot != 0)
			quick_sort(left, pivot - 1, fcmp);
		quick_sort(pivot + 1, right, fcmp);
	}

public:
	// T 타입이 primary type이 아니더라도 비교 람다 함수를 제공할 수 있어 어떤 타입도 정렬 가능.
	void QSort(std::function<int(T&, T&)> fcmp = nullptr)
	{
		std::function<int(T&, T&)> cmp;
		if (fcmp)
			cmp = fcmp;
		else
		{
			cmp = [](const T& l, const T& r) -> int {
				return l < r ? -1 : l > r ? 1 : 0;
				};
		}
		quick_sort(0, this->size() - 1, cmp);
	}
#ifdef _DEBUG
	static void Sample()
	{
		KArray<int> karr2 = { 64, 34, 25, 12, 2, 11, 90, 45, 25, 33, 94, 75, 17, 39, 83, 43, 78, 19, 24, 53, 71, -1 };
		karr2.QSort([](int& l, int& r) -> int {
			return l < r ? -1 : l > r ? 1 : 0;
			});

		KArray<string> karr3 = { "64", "34", "25", "12", "2", "11", "90", "45", "25", "33", "94", "75", "17", "39", "83", "43", "78", "19", "24", "53", "71", "-1" };
		karr3.QSort([](string& l, string& r) -> int {
			return l < r ? -1 : l > r ? 1 : 0;
			});
		TRACE("\n");
		//		[ 0]	"-1" string 
		//		[ 1]	"11" string 
		//		[ 2]	"12" string 
		//		[ 3]	"17" string 
		//		[ 4]	"19" string 
		//		[ 5]	"2"  string 
		//		[ 6]	"24" string 
		//		[ 7]	"25" string 
		//		[ 8]	"25" string 
		//		[ 9]	"33" string 
		//		[10]	"34" string 
		//		[11]	"39" string 
		//		[12]	"43" string 
		//		[13]	"45" string 
		//		[14]	"53" string 
		//		[15]	"64" string 
		//		[16]	"71" string 
		//		[17]	"75" string 
		//		[18]	"78" string 
		//		[19]	"83" string 
		//		[20]	"90" string 
		//		[21]	"94" string 
	}//Sample()
#endif // _DEBUG

	void ForEach(std::function<void(T&)> func) {
		for (auto& item : *this)
			func((T&)(item));
	}

	// 정수 인덱스를 기반으로 insert 수행
	void insert(int index, const T& value) {
		if (index < 0 || index > this->size()) {
			//std::cerr << "Index out of range: " << index << "\n";
			ASSERT(0 == "Index out of range");
			return;
		}
		this->std::vector<T>::insert(this->begin() + index, value);//dwk: 2025-03-18 10:42  
	}

	// ✅ `T`가 `shared_ptr<U>`인 경우에만 `FindObj` 실행
	template<typename U = T, typename std::enable_if_t<
		std::is_same_v<U, std::shared_ptr<typename U::element_type>>, int> = 0>
	std::shared_ptr<typename U::element_type> FindObj(const U& target) {
		auto it = std::find_if(this->begin(), this->end(),
			[&target](const U& obj) { return obj == target; });

		return (it != this->end()) ? *it : nullptr;  // 찾으면 `shared_ptr<U>` 반환, 없으면 `nullptr`
	}

};

//template<typename TBJ>
//class KShpArray : public KArray<SHP<TBJ>>
//{
//public:
//	SHP<TBJ> Find(SHP<TBJ> shbj)
//	{
//		auto it = std::find(this->begin(), this->end(), shbj);
//		if (it != this->end())
//			return *it;
//		return nullptr;
//	}
//};


template<typename T>
class KList : public std::list<T>
{
public:
	//다른 constructor이 있을때는 기본생성자를 만들어줘야한다.
	KList() {}
	///KStdSet<int> sic = {1,3,5,7}; 이렇게 초기화 가능
	KList(std::initializer_list<T> ar)
		: std::list<T>(ar)//this->insert(ar);
	{
	}
	//CCriticalSection _cs;
	/// loop
// 	for(auto&& el : reverse(*this))
// 		cout << el << ',';

	int GetCount() const { return static_cast<int>(this->size()); }
	int Count() const { return GetCount(); }

	bool IsEmpty() const { return this->empty(); }

	void AddHead(T pItem) { this->push_front(pItem); }

	void AddTail(T pItem) { this->push_back(pItem); }

	void AddListHead(const KList<T>& lstItem) {
		for (const auto& item : lstItem)
			this->push_front(item); // shared_ptr이므로 자동 참조 증가 (reference count 증가)
	}
	void AddListTail(const KList<T>& lstItem) {
		for (const auto& item : lstItem)
			this->push_back(item); // shared_ptr이므로 자동 참조 증가 (reference count 증가)
	}
	// POSITION 개념을 대체할 iterator 반환
	//using iterator = typename std::list<T>::iterator;
	using TIT = typename std::list<T>::iterator;
	using CTIT = typename std::list<T>::const_iterator;
	//using const_iterator = typename std::list<T>::const_iterator;
	//template<typename TIT>
	TIT GetHeadPosition() { return this->begin(); }
	//template<typename TCIT>
	CTIT GetHeadPosition() const { return this->begin(); }
	template<typename TIT>
	T GetNext(TIT& pos) {
		//if (pos == this->end()) 
		//	return {};
		T pItem = *pos;
		++pos; // 다음 요소로 이동
		return pItem;
	}

	// GetNext() (const 버전)
	template<typename TCIT>
	const T GetNext(TCIT& pos) const {
		if (pos == this->end()) return nullptr;
		const T pItem = *pos;
		++pos; // 다음 요소로 이동
		return pItem;
	}

public:
	using reverse_iterator = typename std::list<T>::reverse_iterator;
	TIT GetTailPosition() {
		return this->empty() ? this->end() : --this->end();
	}
	// GetTailPosition(): 리스트의 마지막 요소를 가리키는 iterator 반환
	using const_reverse_iterator = typename std::list<T>::const_reverse_iterator;


	T GetPrev(TIT& pos) {
		//if (pos == this->rend()) //'return': cannot convert from 'std::_List_iterator<std::_List_val<std::_List_simple_types<_Ty>>>' to 'std::reverse_iterator<std::_List_const_iterator<std::_List_val<std::_List_simple_types<_Ty>>>>'
		//	return {};//
		T pItem = *pos;
		++pos; // 이전 요소로 이동
		return pItem;
	}
	T GetPrev(const_reverse_iterator& pos) const {
		auto temp = pos; // 복사본 사용
		++temp; // 이전 요소로 이동
		return *temp; // 이전 요소 반환
	}
	virtual void RemoveAll() {
		this->clear();
	}


	// operator==을 호출
	//std::find가 std::shared_ptr<T> 객체를 비교할 때, 내부적으로 포인터 주소를 비교
	//template< class T, class U >
	//bool operator==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept;
	TIT Find(const T& value) {//typename std::list<T>::
		auto& lst = *this;
		return std::find(lst.begin(), lst.end(), value);
	}
	bool FindCheck(const T& value) {//typename std::list<T>::
		return Find(value) != this->end();
	}

	template<typename U = T, typename std::enable_if_t<std::is_same_v<U, std::shared_ptr<typename U::element_type>>, int> = 0>
	std::shared_ptr<typename U::element_type> FindObj(const U& target) {
		auto it = std::find_if(this->begin(), this->end(),
			[&target](const U& obj) { return obj == target; });

		return (it != this->end()) ? *it : nullptr;  // 찾으면 `shared_ptr<U>` 반환, 없으면 `nullptr`
	}
	template<typename U = T, typename std::enable_if_t<
		std::is_same_v<U, std::shared_ptr<typename U::element_type>>, int> = 0>
	std::shared_ptr<typename U::element_type> FindObj1(const U& target) {
		auto it = std::find(this->begin(), this->end(), target);
		return (it != this->end()) ? *it : nullptr;
	}
	T& GetAt(TIT it) {//std::list<T>& lst, 
		auto& lst = *this;
		if (it != lst.end()) {
			return *it;
		}
		throw_std_out_of_range("Invalid iterator");
	}
	const T& GetAt(CTIT it) const {//const std::list<T>& lst, 
		const auto& lst = *this;
		if (it != lst.end()) {
			return *it;
		}
		throw_std_out_of_range("Invalid iterator");
	}
	T At(size_t index) const {
		if (index >= this->size())
			return T();
		auto it = this->begin();
		std::advance(it, index);
		return *it;
	}

	TIT RemoveAt(TIT it) {//std::list<T>& lst, 
		auto& lst = *this;
		if (it != lst.end()) {
			return lst.erase(it);
		}
		return {};//false;
	}
	T First() const {
		if (this->empty())
			//throw std::out_of_range("KList::First() - list is empty");
			return T();  // 또는 throw 예외
		return *(this->begin());
	}
	T Last() const {
		if (this->empty())
			//throw std::out_of_range("KList::First() - list is empty");
			return T();
		return *(--this->end());
	}
	//const T& Last() const {
	//	if (this->empty())
	//		throw std::out_of_range("empty");
	//	return *(--this->end());
	//}
	T PopFirst() {
		if (this->empty())
			throw_std_out_of_range("empty");
		T val = this->front();
		this->pop_front();
		return val;
	}
	// 조건에 맞는 요소 찾기
	template<typename Pred>
	T FindIf(Pred pred) const {
		auto it = std::find_if(this->begin(), this->end(), pred);
		if (it != this->end())
			return *it;
		return T();
	}

	// 조건에 맞는 요소를 제거
	template<typename Pred>
	void RemoveIf(Pred pred) {
		this->remove_if(pred);
	}

	// 특정 값의 인덱스를 반환 (없으면 -1)
	int IndexOf(const T& val2) const {
		int i = 0;
		for (auto& val : *this) {
			if (val == val2)
				return i;
			++i;
		}
		return -1;
	}
	void ForEach(function<void(T&)> func) {
		for (auto& val : *this)
			func((T&)(val));//dwk (T&) 을 넣으니 에러 잡힌다.
	}
	void ForEach(function<void(T&)> func) const {
		for (const auto& val : *this)
			func((T&)(val));//dwk 에러나면 (T&) 해뫄라.
	}
	/// 특정 조건으로 찾아서 할일 까지 람다 함수로 받아서 실행
	/// 람다에서 TRUE 리턴 하면 loop 탈출(return TRUE)
	/// 아래 ForEachBool후 리턴 값 if(rv) {}로 해도 된다.
	/// deprecated 너무 복잡하다. 직관적으로 아래 함수 쓰면 된다.
	//BOOL ForEachBool(function<BOOL(T&)> func, function<void(iterator& item)> todoOnTrue) {
	BOOL ForEachBool(function<BOOL(T&)> cbCompare, function<void(T&)> OnTrue = nullptr)
	{
		BOOL rv{ FALSE };
		for (auto& val : *this) {
			if (cbCompare((T&)val)) {// 람다 함수가 리턴 하면 바로 리턴 TRUE
				if (OnTrue)
					OnTrue((T&)val);// item은 iterator이다. 위 조건에 맞는 경우 실행 후 참 리턴
				return TRUE;
			}
		}
		return FALSE;
	}
	void ForEachRemove(function<BOOL(T&)> OnCompare, function<void(T&)> OnTrueBeforeRemove) {
		for (auto it = this->begin(); it != this->end(); ) {
			if (OnCompare(*it)) { // 짝수 삭제 *it % 2 == 0
				OnTrueBeforeRemove(*it);// item은 iterator이다. 위 조건에 맞는 경우 //이거 실행후 내부에서 삭제 한다.
				it = this->erase(it); // ✅ erase 후 반복자가 자동으로 다음 요소를 가리킴
			}
			else
				++it; // ✅ 삭제되지 않은 경우만 다음 요소로 이동
		}
	}
#ifdef _Sample__
	void Sample() {
		m_objects.ForEachBool(
			[this](auto& pObjTemp) -> BOOL {
				if (pObjTemp.get() == pObj.get()) {
					// do something
					//m_objects.RemoveAt(item);
					//m_bTagModified = TRUE;
					return TRUE;
				}
				return FALSE;
			});
	}
#endif // _Sample__


	/// loop로 각 항목 실행 하나, toBreak 조건에 맞으면 loop반복을 끝낸다.
	/// <returns>toBreak가 참이면 그값을, 아니면 func가 리턴한 값을 리턴</returns>
#ifdef _Sample__
	int ForEachRtn(function<int(TIT&)> func, function<bool(int)> toBreak = [](int r)->bool { return r == -1; })
	{
		int rv{ 0 };//0은 의미 없슴. 매번 갱신
		for (auto& item : *this) {
			rv = func(item); // 람다 함수 실행
			if (toBreak(rv))
				break;
		}
		return rv;
	}
	auto todo = [this, &outFile](auto& pObjTemp) {
		DoSomethingWithObj(pObjTemp);
		});
	myList.ForEach([this, &todo](auto& pObjTemp) {
		todo(pObjTemp);
		});
	auto rv = myList.ForEachRtn([this, nTagNumber](SHP<CParamObj>& pObj) -> int {
		return (pObj->m_nUniqueID == nTagNumber) ? -1 : 0;//-1;//일때 루프 중간에 탈출
		});//, [](int rv) -> bool { return rv == -1;});
	return rv == -1;//발견되었으면 TRUE를 리턴
#endif // _Sample__

	//template<typename TShbj, typename Func>
	//TShbj FindObj(Func cbCmp)
	//T FindObj(Func cbCmp)
	T FindObj(function<bool(T&)> cbCmp)
	{
		T shFnd;
		this->ForEachBool([this, cbCmp, &shFnd](T& vbj) -> BOOL {
			if (cbCmp(vbj)) { //-1;//일때 루프 중간에 탈출
				shFnd = vbj;
				return TRUE;
			}
			return FALSE;
			});
		return shFnd;
	}
#ifdef _Sample__
	return rObj.FindObj([nObjId](auto& pShape) {
		return (pShape->GetType() == nObjId);
		});
	SHP<CFODrawShape> CFOPChartShape::GetObjectWithId(const long nId, const long nCol, const long nRow) {
		return m_GroupList->FindObj([&](auto& pShape) -> bool {
			return (pShape->GetType() == (UINT)nId);
			});
	}
#endif // _Sample__

	//template<typename TShbj>
	//void RemoveObj(SHP<TShbj> pObj)//dwk: 2025-02-14 16:21  
	void RemoveObj(T pObj)//dwk: 2025-02-14 16:21  
	{
		this->remove(pObj);//dwk shared_ptr은 == 하면, 내부 포인터로 비교 하므로 
		//auto b = this->ForEachBool(//dwk: 2025-02-14 15:04  
		//	[this, pObj](auto& shItem) -> BOOL {
		//		if (shItem.get() == pObj.get()) {//-1;//일때 루프 중간에 탈출
		//			this->RemoveAt(item);
		//			return TRUE;
		//		}
		//		return FALSE;
		//	});
		//return b;
	}
	// T 자체가 shared_ptr<TObj>일 때, 내부 객체 값 비교
	static bool EqualByValue(const KList<T>& a, const KList<T>& b)
	{
		static_assert(std::is_class_v<typename std::remove_pointer_t<decltype(a.front().get())>>,
			"EqualByValue는 shared_ptr<T>에만 사용 가능합니다");
		if (a.size() != b.size())
			return false;
		auto it1 = a.begin();
		auto it2 = b.begin();
		for (; it1 != a.end(); ++it1, ++it2) {
			if (!*it1 || !*it2)
				return false; // 널 포인터는 false 처리
			if (**it1 != **it2)  // ⭐ shared_ptr을 역참조해서 실제 값 비교
				return false;
			//BOOL operator==(const MyObj5 & a) const { return x == a.x && y == a.y; }
			//BOOL operator!=(const MyObj5 & a) const { return !operator==(a); }
		}
		return true;
	}
}; // class KList



///  /////////////////////////////////////////////////////////////////////////////

template<typename TBJ>
bool EqualByValueT(const KList<SHP<TBJ>>& a, const KList<SHP<TBJ>>& b)
{
	static_assert(std::is_class_v<typename std::remove_pointer_t<decltype(a.front().get())>>,
		"EqualByValue는 shared_ptr<T>에만 사용 가능합니다");
	if (a.size() != b.size())
		return false;
	auto it1 = a.begin();
	auto it2 = b.begin();
	for (; it1 != a.end(); ++it1, ++it2) {
		if (!*it1 || !*it2)
			return false; // 널 포인터는 false 처리
		if (**it1 != **it2)  // ⭐ shared_ptr을 역참조해서 실제 값 비교
			return false;
		//BOOL operator==(const MyObj5 & a) const { return x == a.x && y == a.y; }
		//BOOL operator!=(const MyObj5 & a) const { return !operator==(a); }
	}
	return true;
}

template<typename TBJ>
class KShpList : public KList<SHP<TBJ>>
{
public:
	SHP<TBJ> Find(SHP<TBJ> shbj)
	{
		auto it = std::find(this->begin(), this->end(), shbj);
		if (it != this->end())
			return *it;
		return nullptr;
	}
};

template<typename TBJ>
class KPtrList : public KList<TBJ*>
{
public:
	using PTBJ = TBJ*;

	BOOL m_bAutoFree{ true };
	KPtrList(BOOL bAutoFree = true)
		: m_bAutoFree(bAutoFree)// = default;
	{
	}
	KPtrList(std::initializer_list<PTBJ> ar)
		: KList<PTBJ>(ar)//this->insert(ar);
	{
	}

	~KPtrList() {
		if (m_bAutoFree)
			void FreeAll();
	}
	void FreeAll() {
		for (auto& v : *this)
			DeleteMeSafe(v);
	}
	void DeleteAll() {
		if (m_bAutoFree)
			FreeAll();
		this->clear();
	}
	//int GetCount() const { return static_cast<int>(this->size()); }

	// IsEmpty(): 리스트가 비어있는지 확인
	//bool IsEmpty() const { return this->empty(); }

	// AddTail(): 리스트 끝에 추가
	void AddTail(PTBJ pItem) { this->push_back(pItem); }
	// 🔥 AddTail(KSharedPtrList<T> lstItem): 다른 리스트의 모든 요소를 현재 리스트 끝에 추가
	void AddTail(const KPtrList<PTBJ>& lstItem) {
		for (const auto& item : lstItem)
			this->push_back(item); // shared_ptr이므로 자동 참조 증가 (reference count 증가)
	}

	// RemoveAll(): 모든 요소 삭제 및 메모리 해제
	void RemoveAll() override
	{
		ASSERT(0 == "안에 포인터는 어쩌고 RemoveAll 을 불러? DeleteAll을 불러야지.");
		//for (auto ptr : *this)
		//	delete ptr;
		//this->clear();
	}
};



///  KSharedObjList는 UcTool.h 에 정의. MFC를 쓰므로



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



//template<typename TCH>
//TCH  tchlower(const TCH c) { return ((c)-'A' + 'a'); }

template<typename TCH>
TCH  tchlower(const TCH c) {
	ASSERT('a' > 'A');
	ASSERT(('a' - 'A') == 32);
	ASSERT('A' <= c && c <= 'Z');
	return (c + 32);// ('a' - 'A'));
}

template<typename TCH>
TCH  tchupper(const TCH c) {
	//const int gab = 'a' - 'A';//32
	ASSERT('a' <= c && c <= 'z');
	return (c - 32);
}


template<typename TCH>
BOOL  tchiscap(const TCH c) { return ('A' <= c && c <= 'Z'); }
template<typename TCH>
TCH  tchicase(const TCH c) { return tchiscap(c) ? tchlower(c) : c; }

template<typename TCH>
TCH tchtolower(TCH uc)
{
	const int gab = 'a' - 'A';
	if ((TCH)'A' <= uc && uc <= (TCH)'Z')
		return (TCH)(uc + gab);
	else
		return uc;
}
template<typename TCH>
TCH tchtoupper(TCH uc)
{
	const int gab = 'a' - 'A';
	if ((TCH)'a' <= uc && uc <= (TCH)'z')
		return (TCH)(uc - gab);
	else
		return uc;
}

template<typename TCH>
int tchcmpchi(const TCH c1, const TCH c2)
{
	return tchicase(c1) - tchicase(c2);
}


// size_t  가 아니고 int 를 쓰는 이유는, 어차피 2기가 최대 메모리 할당 이므로
// int 0x7fffffff 2giga 이므로 CString 의 인덱스가 대부분 int 이므로 매번 cast하기 번거로워서 int로 한다.
/// wcslen은 NULL이 오면, 죽지만 이것은 0을 리턴한다.
template<typename TCH>
int tchlen(const TCH* wcs)
{
	if (wcs == NULL)
		return 0;
	auto eos = wcs;  // 캐스트 제거
	while (*eos++);
	return static_cast<int>(eos - wcs - 1);  // static_cast 사용
}

template<typename TCH>
TCH* tchstr(const TCH* wcs1, const TCH* wcs2)
{
	TCH* cp = (TCH*)wcs1;
	TCH* s1, * s2;
	if (!*wcs2)
		return (TCH*)wcs1;
	if (cp == NULL)
		return NULL;
	while (*cp)
	{
		s1 = cp;
		s2 = (TCH*)wcs2;
		while (*s1 && *s2 && !(*s1 - *s2))
			s1++, s2++;
		if (!*s2)
			return cp;

		cp++;
	}
	return NULL;
}

template<typename TCH>
TCH* tchchr(const TCH* wcs, int ch)
{
	ASSERT(ch);
	while (*wcs && *wcs != (TCH)ch)
		wcs++;
	if (*wcs == (TCH)ch)
		return (TCH*)wcs;
	return NULL;
}

template<typename TCH>
const TCH* tchcat(const TCH* dst, const TCH* src)
{
	auto cp = const_cast<TCH*>(dst);  // Use const_cast to explicitly remove const
	//auto cp = dst;
	while (*cp)
		cp++;                   /* find end of dst  */
	while (*cp++ = *src++);       /* Copy src to end of dst */
	return(dst);                  /* return dst */
}

template<typename TCH>
const TCH* tchcpy(const TCH* dst, const TCH* src)
{
	auto cp = const_cast<TCH*>(dst);  // Use const_cast to explicitly remove const
	//auto cp = (TCH * )dst; cppcheck
	while (*cp++ = *src++);
	return(dst);
}

template<typename TCH>
const TCH* tchncpy(TCH* dest0, const TCH* source, size_t count, TCH chFillEnd = 0)
{
	TCH* dest = (TCH*)dest0;
	while (count && (*dest++ = *source++))    /* copy string */
		count--;
	if (count)                              /* pad out with zeroes */
		while (--count)
			*dest++ = chFillEnd;//L'\0';
	//?주의 count가 '\0'을 포함한 갯수 인데 source가 '\0'을 포함 하지 않았다면 부른 후 반드시 뒤에 '\0'을 붙여 줘야 핟다.	
	return(dest0);
}


template<typename TCH>
int tchncmp(const TCH* src, const TCH* dst, size_t count)
{
	ASSERT(src && dst);
	if (!count)
		return(0);
	while (--count && *src && *src == *dst)
	{
		src++;
		dst++;
	}
	//	int ret = (*(TCH*)src - *(TCH*)dst); cppcheck
	int ret = (*src - *dst);
	if (ret > 1) ret = 1;
	else if (ret < -1) ret = -1;
	return ret;
}

template<typename TCH>
bool tchcmpLeft(const TCH* src, const TCH* target, size_t n)
{
	int ri = tchncmp(src, target, n);
	return ri == 0;
}

template<typename TCH>
bool tchcmpRight(const TCH* src, const TCH* target, size_t n)
{
	if (src == nullptr && target == nullptr) // both null -> true
		return true;
	if (src == nullptr || target == nullptr) // only one null -> false
		return false;

	size_t srcLen = tchlen(src);
	size_t targetLen = tchlen(target);

	if (n > srcLen || n > targetLen)
		return false;

	return tchncmp(src + srcLen - n, target + targetLen - n, n) == 0;
}


template<typename TCH>
int tchicmp(const TCH* src, const TCH* dst)
{
	ASSERT(src && dst);
	int ret = 0;

	while (!(ret = (int)tchcmpchi(*src, *dst)) && *dst)
		++src, ++dst;

	if (ret < 0)
		ret = -1;
	else if (ret > 0)
		ret = 1;

	return(ret);
}




template<typename TCH>
int tchnicmp(const TCH* dst, const TCH* src, size_t count)
{
	ASSERT(src && dst);
	TCH f, l;

	if (count)
	{
		do
		{
			f = tchtolower(*dst);
			l = tchtolower(*src);
			dst++;
			src++;
		} while ((--count) && (f) && (f == l));
	}
	return (int)(f - l);
}




template<typename TCH>
int tchcmp(const TCH* src, const TCH* dst)
{
	int ret = 0;
	if (src == dst)
		return 0;
	if (!src && dst)
		return -1;
	if (src && !dst)
		return 1;

	if (src && dst)
	{
		while (!(ret = (int)(*src - *dst)) && *dst)
			++src, ++dst;

		if (ret < 0)
			ret = -1;
		else if (ret > 0)
			ret = 1;
	}
	else
	{
		if (src)
			ret = 1;
		else
			ret = -1;
	}

	return(ret);
}

template<typename TCH>
bool tchsame(const TCH* src, const TCH* dst)
{
	return tchcmp(src, dst) == 0;
}

/// tstring is wstring or string
template <typename tstring, typename TPS>
int tstrfind(tstring str, TPS searchStr)
{
	size_t found = str.find(searchStr);
	if (found != tstring::npos)
		return (int)found;
	return -1;
}

inline bool UcIsAlpha(TCHAR c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}
inline bool UcIsDigit(TCHAR c)
{
	return ('0' <= c && c <= '9');
}

/// 숫자와 +-. 까지 측정
inline bool UcIsFloat(TCHAR c)
{
	return UcIsDigit(c) || c == '-' || c == '.' || c == '+';// || c == 'e' || c == 'E';
}
inline bool UcIsInt(TCHAR c)
{
	return UcIsDigit(c) || c == '-' || c == '+';
}

inline bool UcIsDigitStr(const TCHAR* c)
{
	LPTSTR p = (LPTSTR)c;
	if (*p == '\0')
		return false;
	for (; *p; p++)
	{
		if (!UcIsDigit(*p))
			return false;
	}
	return true;
}
/// 숫자와 +-. 까지 측정
inline bool UcIsFloatStr(const TCHAR* c)
{
	LPTSTR p = (LPTSTR)c;
	for (; *p; p++)
	{
		if (!UcIsFloat(*p))
			return false;
	}
	return true;
}

inline bool UcIsIntStr(const TCHAR* c)
{
	LPTSTR p = (LPTSTR)c;
	for (; *p; p++)
	{
		if (!UcIsInt(*p))
			return false;
	}
	return true;
}


// UNICODE전용
inline bool UcIsSpace(TCHAR c)
{
	return tchchr(_T(" \t\r\n"), c) != NULL;
}



inline bool UcIsAscii(TCHAR c)
{
	return (' ' <= c && c <= '~');// 0x20 ~ 0x7e
}
inline bool UcIsAlNum(TCHAR c)
{
	return UcIsAlpha(c) || UcIsDigit(c);
}

inline bool UcIsCharInStrOf(const TCHAR* c, LPCTSTR strIn = _T(" \t\r\n"))
{
	LPTSTR p = (LPTSTR)c;
	for (; *p; p++)
	{
		if (!tchchr(strIn, *p))
			return false;
	}
	return true;
}




enum { eCpnInt, eCpnFloat, eCpnCutZero, eCpnNormal };
//template<typename TCH>
//const TCH* tchcpynum(const TCH* dst, const TCH* src, int iOp = 3)
//{
//	// if iOp==1 이면 double 로 소숫점까지 복사, 0이면 '.'에서끝낸다.
//	// 2: 뒤에 0 잘라 내기
//	auto cp = const_cast<TCH*>(dst);  // Use const_cast to explicitly remove const
//	//auto cp = (TCH*)dst;
//	int j = 0;
//	BOOL bDot = 0;
//	BOOL bOver = 0;
//	for (int i = 0; i < 1024; i++)//숫자가 가장 긴것을 1024길이로 본거지.
//	{
//		TCH c = src[i];
//		if (((TCH)'0' <= c && c <= (TCH)'9') || c == (TCH)'\0' /*|| c == (TCH)'.'*/ || c == (TCH)'-')
//			cp[j++] = c;
//		else if (c == (TCH)'.')
//		{
//			bDot = 1;
//			//if (iOp == eCpnInt || bDot)// . 이 나왔는데 정수? 이미 나왔어? 끝내자. cppcheck 때문에 제거
//			//{
//			cp[j++] = (TCH)'\0';
//			break;
//			//}
//			//else
//			//	cp[j++] = c;
//		}
//		//if(iOp == eCpnInt && c == (TCH)'.')
//		//	break;
//		if (c == (TCH)'\0')
//			break;
//		if (i >= 126)
//		{
//			cp[j++] = (TCH)'\0';
//			bOver = 1;
//			break;
//		}
//	}
//	if ((iOp & eCpnNormal) == eCpnNormal && bDot && j > 1)
//	{
//		for (int r = j - 2; r > 0; r--)
//		{
//			TCH c = cp[r];
//			if (c == (TCH)'.')
//			{
//				cp[r] = (TCH)'\0';
//				break;
//			}
//			else if (c == (TCH)'0')// 뒤에 0 계속 제거
//				cp[r] = (TCH)'\0';
//			else
//				break;
//		}
//	}
//	return bOver ? NULL : dst;
//}

template<typename TCH>
const TCH* tchcpynum(const TCH* dst, const TCH* src, int iOp = eCpnNormal)
{
	auto cp = const_cast<TCH*>(dst);
	int j = 0;
	BOOL bDot = FALSE;
	BOOL bOver = FALSE;

	for (int i = 0; i < 1024; i++)
	{
		TCH c = src[i];

		if (((TCH)'0' <= c && c <= (TCH)'9') || c == (TCH)'-')
		{
			cp[j++] = c;
		}
		else if (c == (TCH)'.')
		{
			if (iOp == eCpnInt) // 정수 모드면 소수점에서 종료
			{
				cp[j++] = (TCH)'\0';
				break;
			}
			else
			{
				bDot = TRUE;
				cp[j++] = c; // ✅ 소수점 포함
			}
		}
		else if (c == (TCH)'\0')
		{
			cp[j++] = (TCH)'\0';
			break;
		}
		else
		{
			cp[j++] = (TCH)'\0';
			break;
		}

		if (i >= 126)
		{
			cp[j++] = (TCH)'\0';
			bOver = TRUE;
			break;
		}
	}

	// 소수점 이하 0 제거 (eCpnCutZero, eCpnNormal)
	if ((iOp == eCpnCutZero || iOp == eCpnNormal) && bDot && j > 1)
	{
		for (int r = j - 1; r > 0; r--)
		{
			TCH c = cp[r];
			if (c == (TCH)'.')
			{
				cp[r] = (TCH)'\0';
				break;
			}
			else if (c == (TCH)'0')
			{
				cp[r] = (TCH)'\0';
			}
			else
			{
				break;
			}
		}
	}

	return bOver ? NULL : dst;
}


/// <summary>
/// shared_ptr에서 맨나중에 없어질떄도 내부 포인터를 delete하지 않으려 할떄 이걸 쓴다.
/// fnc에 delete를 직접 해주거나 다른 일을 한다.
/// 여기서는 default로 delete를 해주지 않는다.
/// </summary>
struct TNotFree
{
	std::function<void(void*)> _fncToDelete;
	explicit TNotFree(std::function<void(const void*)> fnc = nullptr) : _fncToDelete(fnc)
	{
	}
	template<typename T>
	void operator()(T* p)
	{
		if (_fncToDelete)
			_fncToDelete(reinterpret_cast<void*>(p));
		//delete p; // 배열은 delete [] p;  //TRACE("Do not free.\n");
	}
	/// JVal::ShareObj(UcJObj& obj1) 에서 	shared_ptr 인데도 자동 삭제 하지 않으려 할때 쓴다.
};


/// std string에서 찾아서 발견 되었나?
template<typename TStr>
BOOL StrFind(TStr& str, TStr key)
{
	return str.find(key) != TStr::npos;
}

/// map에서 key를 찾아 값이 있으면 TRUE
template<typename TKey, typename TVal>
BOOL MapLookup(std::map<TKey, TVal>& mp, TVal key)
{
	return mp.find(key) != mp.end();
}
template<typename TKey, typename TVal>
TVal* MapFind(std::map<TKey, TVal>& mp, TVal key)
{
	auto it = mp.find(key);
	return it != mp.end() ? &it->second : NULL;
}

/// 맵에서 찾아서 v에 담아서 보낸다.
template<typename TKey, typename TVal>
BOOL MapLookup(std::map<TKey, TVal>& mp, TKey k, TVal& v)
{
	auto it = mp.find(k);
	if (it != mp.end())
	{
		v = it->second;
		return TRUE;
	}
	return FALSE;
}

/// value가 객체인경우 value 의 포인터만 가져 오면 내부를 건들일 수 있다.
template<typename TKey, typename TVal>
BOOL MapLookupRef(std::map<TKey, TVal>& mp, TKey k, TVal*& v)
{
	auto it = mp.find(k);
	if (it != mp.end())
	{
		v = &it->second;
		return TRUE;
	}
	return FALSE;
}


template<typename TItem>
void UcVectorErase(vector<TItem> rows, function<BOOL(TItem& item)> condition)
{
	//auto condition = [](auto jGuid) -> BOOL {
	//	auto bDel = jGuid->Dic()->Len("_updated");
	//	return bDel;
	//	};
	rows.erase(std::remove_if(rows.begin(), rows.end(), condition), rows.end());
}


template <typename CharT>
CharT toLowerChar(CharT c)
{
#if CPP17_OR_LATER
	if constexpr (std::is_same_v<CharT, char>) {
#else
	if (std::is_same<CharT, char>::value) {
#endif
		return ::tolower(static_cast<unsigned char>(c));
	}
#if CPP17_OR_LATER
	else if constexpr (std::is_same_v<CharT, wchar_t>) {
#else
	else if (std::is_same<CharT, wchar_t>::value) {
#endif
		return ::towlower(c);
	}
	}

// 문자열을 소문자로 변환하는 템플릿 함수
template <typename StringT>
StringT toLowerString(const StringT & str)
{
	StringT lowerStr;
	std::transform(str.begin(), str.end(), std::back_inserter(lowerStr),
		[](auto c) { return toLowerChar(c); });
	return lowerStr;
}

#include <algorithm>
#include <type_traits>
#include <cctype>
#include <cwctype>

namespace ucstd
{
#if CPP17_OR_LATER
template <typename CharT>
CharT StdToLower(CharT ch)
{
	if constexpr (std::is_same_v<CharT, char>)
		return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	else if constexpr (std::is_same_v<CharT, wchar_t>)
		return static_cast<wchar_t>(std::towlower(ch));
	else
		static_assert(sizeof(CharT) == 0, "Unsupported character type");
}

template <typename CharT>
CharT StdToUpper(CharT ch)
{
	if constexpr (std::is_same_v<CharT, char>)
		return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
	else if constexpr (std::is_same_v<CharT, wchar_t>)
		return static_cast<wchar_t>(std::towupper(ch));
	else
		static_assert(sizeof(CharT) == 0, "Unsupported character type");
}
#else
	 // char
inline char ToLowerChar(char c){
	return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
inline char ToUpperChar(char c){
	return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}
// wchar_t
inline wchar_t ToLowerChar(wchar_t c){
	return static_cast<wchar_t>(std::towlower(c));
}
inline wchar_t ToUpperChar(wchar_t c){
	return static_cast<wchar_t>(std::towupper(c));
}
///#ifdef _UNICODE 이렇게 쓸수도 있지만 그러면 이건 template아니고 MS 빌드 모드에 따라 대응 하는 코드가 된다.
//	auto toLower = [](TCHAR c) { return static_cast<TCHAR>(std::towlower(static_cast<std::wint_t>(c))); };
//#else
//	auto toLower = [](TCHAR c) { return static_cast<TCHAR>(std::tolower(static_cast<unsigned char>(c))); };
//#endif

#endif // CPP17_OR_LATER

#if CPP17_OR_LATER
template <typename StringT>
void MakeStrLower(StringT& s)
{
	using CharT = typename StringT::value_type;
	std::transform(s.begin(), s.end(), s.begin(), [](CharT c) { return StdToLower(c); });
}

template <typename StringT>
void MakeStrUpper(StringT& s)
{
	using CharT = typename StringT::value_type;
	std::transform(s.begin(), s.end(), s.begin(), [](CharT c) { return StdToUpper(c); });
}
#else
template<typename TString>
void MakeStrLower(TString& s)
{
	using CharT = typename TString::value_type;

	std::transform(
		 s.begin(), s.end(), s.begin(),
		 [](CharT c) { return ToLowerChar(c); }
	);
}
template<typename TString>
void MakeStrUpper(TString& s)
{
	using CharT = typename TString::value_type;

	std::transform(
		 s.begin(), s.end(), s.begin(),
		 [](CharT c) { return ToUpperChar(c); }
	);
}
#endif // CPP17_OR_LATER

#ifdef _Useses__
std::set<std::string, CaseInsensitiveCompare<std::string>> stringSet;
#endif // _Useses
}//ucstd


//#include <locale>//std::locale()
//// 그냥 wstring 용 deprecated Use below template KwompIns
//struct UcCaseInsensitiveCompareWS {
//	bool operator()(const std::wstring& a, const std::wstring& b) const
//	{
//		return std::lexicographical_compare(
//			a.begin(), a.end(), b.begin(), b.end(),
//			[](wchar_t a, wchar_t b) {
//				return std::tolower(a, std::locale()) < std::tolower(b, std::locale());
//			}
//		);
//	}
//};
/// 대소문자를 구분하지 않는 비교를 위한 템플릿 구조체
template <typename StringT>
struct UcCompIns { //Case Insensitive Compare
	bool operator()(const StringT& a, const StringT& b) const
	{
		return toLowerString(a) < toLowerString(b);
	}
};


/// 객체가 아닌 메모리를 shared_ptr로 할당 하는 법: template방법 try2
/// new char[]로 하여, delete 람다함수 제공
/// len은 바이트 단위인데 VARTTYPE의 크기와 맞추어야 한다.
/// VARTYPE은 char, wchar_t, BYTE, int, MyStruct 등
template<typename VARTYPE>
std::shared_ptr<VARTYPE> MakeSharedBuf(size_t len) {
	return std::shared_ptr<VARTYPE>(
		reinterpret_cast<VARTYPE*>(new char[len]),
		[](VARTYPE* ptr) { delete[] reinterpret_cast<char*>(ptr); }
	);
}

#ifdef _sample__
//  예제: HTTP_REQUEST 구조체 크기 만큼의 버퍼를 shared_ptr로 할당
size_t len = sizeof(HTTP_REQUEST);
auto reqBuf = MakeSharedBuf<HTTP_REQUEST>(len);
#endif // _sample__



//#include <iostream>
#include <memory>
#include <unordered_map>
#include <mutex>

//  전역 객체 맵 (모든 클래스에서 공유)
EXTERN_STATIC std::unordered_map<void*, std::weak_ptr<void>> s_sharedInstances;
EXTERN_STATIC std::mutex s_saredTempMtx;

///  범용 safe_shared_from_this() 함수
template <typename T>
std::shared_ptr<T> UcSharedThis(T * obj)
{
	std::lock_guard<std::mutex> lock(s_saredTempMtx);

	//  기존 shared_ptr이 있으면 반환
	auto it = s_sharedInstances.find(obj);
	if (it != s_sharedInstances.end()) {
		auto sp = std::static_pointer_cast<T>(it->second.lock());
		if (sp) return sp;
	}

	//  새로운 shared_ptr 생성 + `s_sharedInstances`에서 자동 제거
	std::shared_ptr<T> sp(obj, [](T* ptr) {
		std::lock_guard<std::mutex> lock(s_saredTempMtx);
		s_sharedInstances.erase(ptr);  //  해제 시 `s_sharedInstances`에서 삭제
		TRACE(L"Custom deleter: Object removed from sharedInstances!\n");
		});
	s_sharedInstances[obj] = sp;  //  맵에 저장
	return sp;
}
#ifdef _Sample__
//  클래스 정의 (enable_shared_from_this 없이 사용 가능)
class MyClass2;
class MyClass {
public:
	void show() {
		TRACE(L"MyClass instance at %x\n", this);// << std::endl;
	}
	void useShared(MyClass2* mc2);
};

class MyClass2 {
public:
	SHP<MyClass> _sp;
};

int Testmain2(MyClass2 & mc2) {

	MyClass obj;  // X shared_ptr<MyClass> 없이 생성됨
	obj.useShared(&mc2);  //  안전하게 shared_ptr 반환

	return 0;
}
/// 여기서 std::enable_shared_from_this<MyClass>를 상속받지 않았기 때문에
/// 표준의 shared_from_this()는 당연히 쓸 수 없습니다.
void MyClass::useShared(MyClass2 * mc2) {
	auto sp = safe_shared_from_this(this);  //  안전한 shared_ptr 반환
	sp->show();
	mc2->_sp = sp;
}
#endif // _Sample__


template<typename TStr>
void StdReplaceAll(TStr & str, const TStr & from, const TStr & to)
{
	if (from.empty())
		return;
	typename TStr::size_type pos = 0;
	while ((pos = str.find(from, pos)) != TStr::npos) {
		str.replace(pos, from.length(), to);
		pos += to.length(); // 중복 방지
	}
}
template<typename TStr>
void StdRemoveChar(TStr & str, typename TStr::value_type ch)
{
	str.erase(std::remove(str.begin(), str.end(), ch), str.end());
}

//template<typename TStr>
//void StdRemoveChars(TStr& str, const TStr& charsToRemove)
//{
//	str.erase(std::remove_if(str.begin(), str.end(),
//		[&](typename TStr::value_type ch) {
//			return charsToRemove.find(ch) != TStr::npos;
//		}), str.end());
//}
template<typename TStr>
void UcStdRemoveChars(TStr & str, const typename TStr::value_type * chars)
{
	TStr charsToRemove(chars); // const char* → std::string, const wchar_t* → std::wstring
	str.erase(std::remove_if(str.begin(), str.end(),
		[&](typename TStr::value_type ch) {
			return charsToRemove.find(ch) != TStr::npos;
		}), str.end());
}
//DWKREMINDER("typename TStr::value_type")


/// <summary>
///  UcJson.h 에서 옮겨옴
/// 어떤 문자열형식 이든 std::wstring 으로 변환
/// </summary>
/// <param name="k"></param>
/// <returns></returns>
#if CPP_BEFORE_17
inline std::wstring PTstr(LPCSTR k) {
	CStringW kw(k);// CStringW(lp)
	return std::wstring(kw);//
}
inline std::wstring PTstr(int kw) {
	return std::to_wstring(kw);// jstring(kw);
}
inline std::wstring PTstr(LPCWSTR kw) {
	return std::wstring(kw);
}
inline std::wstring& PTstr(std::wstring& kw) {
	return kw;
}
inline std::wstring PTstr(CStringA& ka) {
	CStringW kw(ka);
	return std::wstring((LPCWSTR)kw);
}
inline std::wstring PTstr(CStringW& kw) {
	return std::wstring((LPCWSTR)kw);
}
inline std::wstring PTstr(const std::string& ks) {
	CStringW kw(ks.c_str());
	return std::wstring((LPCWSTR)kw);
}
#else
template<typename>
struct UcPstrAlwaysFalse : std::false_type {};

template<typename T>
inline std::wstring PTstr(T&& k)
{
	using U = std::decay_t<T>;

	if constexpr (std::is_same_v<U, std::wstring>) {
		return std::forward<T>(k);
	}
	else if constexpr (std::is_same_v<U, std::wstring_view>) {
		return std::wstring(k.begin(), k.end());
	}
	else if constexpr (std::is_same_v<U, const wchar_t*> || std::is_same_v<U, wchar_t*>) {
		return k ? std::wstring(k) : std::wstring();
	}
	else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
		CStringW kw(k);
		return std::wstring((LPCWSTR)kw);
	}
	else if constexpr (std::is_same_v<U, CStringW>) {
		return std::wstring((LPCWSTR)k);
	}
	else if constexpr (std::is_same_v<U, CStringA>) {
		CStringW kw(k);
		return std::wstring((LPCWSTR)kw);
	}
	else if constexpr (std::is_same_v<U, std::string>) {
		CStringW kw(k.c_str());
		return std::wstring((LPCWSTR)kw);
	}
	else if constexpr (std::is_integral_v<U>) {
		return std::to_wstring(static_cast<long long>(k));
	}
	else {
		static_assert(UcPstrAlwaysFalse<U>::value, "PTstr: unsupported type");
	}
}
#endif

/// Simple function to check a string 's' has at least 'n' characters
inline bool HasMinLengthW(LPCWSTR s, size_t n)
{
	if (s) {
		while (n-- > 0) {
			if (*(s++) == 0)
				return false;
		}
		return true;
	}
	return false;
}

//};// namespace Uc

class ITimerTaskTool {
public:
	virtual int DoTimerTask(UINT_PTR nIDEvent) = NULL;
};




//dwk: 2025-12-01 17:53 
//dwk: 2025-12-10 13:10 UcJXBase.h 제거
//dwk: 2026-01-22 15:24 UcClon Copy 함수들 CArray 때문에 UcTool.h 로 옮김
