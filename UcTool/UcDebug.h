#pragma once

/// Release에서도 덤프 하고 싶으면 _DWKTRACE_TRUE 를 define 해야 한다.
/// 덤프 자체를 하지 않으려면 NoDwkTrace 를 define 해야 한다.

#include <map>
#include <mutex>
#include <memory> // call_once
#include <stack>
#include <unordered_map>
#include <typeindex>
#include <tuple>
#include <functional>
#include <iomanip> // setw

#include "UcBaseTools.h"
#if CPP17_OR_LATER
#include "magic_enum/magic_enum.hpp"//dwk: 2025-02-27 11:58  
#endif  


// move to UcBaseTools.h
//#ifndef FILINDWK //이름 바꾸자.//dwk: 2025-07-15 11:33 
//#define FILINDWK(msg) __FILE__ "(" _CRT_STRINGIZE(__LINE__) "): dwk - " msg
//#endif // #pragma message(FILINDWK("메시지 다블클릭하면 코드로 이동"))
////ex 매크로없이 쓰려면: #pragma message(__FILE__ "(" _CRT_STRINGIZE(__LINE__) "): dwk - " "여기부터 메시지 이러쿵저러쿵.")


// [1] `enum_cast()`를 사용하여 특정 `enum`에 속하는지 확인
//template <typename T>
//bool UcIsEnumType(int value) {
//	return magic_enum::enum_cast<T>(value).has_value();
//}

/// error C2059: syntax error: 'constant'
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


#pragma region	[DWK_FORMAT


/// <summary>
///  주의: c++17 전역 변수 이면 inline 뒤에 static을 붙이지 않는다. class의 멤버 static에만 붙인다.
/// </summary>
#if CPP_BEFORE_17
EXTERN_STATIC std::map<std::string, std::function<std::wstring(int)>> DWK_mapEnum;
#else
EXTERN_STATIC std::map<std::string, std::function<std::wstring(int)>> DWK_mapEnum = {};
#endif // _DEBUG

inline std::wstring DwkDefaultEnum(const char* pType, int nId) {
	CStringW s; s.Format(L"%s(%d)", (PWS)CStringW(pType), (int)nId);
	return (PWS)s;//s = L"enum IcEdSplinEditDragPointJig::PointType(0)"
}//s = L"enum `protected: virtual void __cdecl CUcView::OnInitialUpdate(void) __ptr64'::`2'::ENum(0)" 로컬 enum변수 경우

inline bool DWK__isdigitOr(wchar_t ch)
{
	return ('0' <= ch && ch <= '9') || ch == '-';
}

inline void DWK__parseFormat(const CStringW& format, int& width, int& precision) {
	auto percentPos = format.Find(L'%');
	auto dotPos = format.Find(L'.');

	width = 0;
	precision = -1;

	if (percentPos != -1) {
		auto endPos = dotPos != std::wstring::npos ? dotPos : format.GetLength();
		if (endPos > percentPos + 1) {
			auto sff = format.Mid(percentPos + 1, endPos - percentPos - 1);
			sff.TrimLeft(L"0");
			if (sff.GetLength() > 0 && DWK__isdigitOr(sff.GetAt(0)))
				width = std::stoi((PWS)sff);
		}
	}

	if (dotPos != -1) {
		auto endPos = format.Find(L'v', dotPos);
		if (endPos != std::wstring::npos && endPos > dotPos + 1) {
			precision = std::stoi((PWS)format.Mid(dotPos + 1, endPos - dotPos - 1));
		}
	}
}

#pragma region	type mapping[

#define VAR_PARAMS std_any& operand, std::wstringstream& wss, const CStringW& format, int& pr

// static 람다 초기화에서는 캡쳐를 한번만 하니, 파라미터로 넘겨 줘야 한다. 올드: [&wss, &format] -> []
#define PrimierVar_Arg(FTYPE, wss, operand, format) \
	auto ff = format.Mid(1);\
	FTYPE value1 = std_any_cast<FTYPE>(operand);\
	if (ff.GetLength() >= 1)\
		wss << std::setfill(ff.Left(1) == L"0" ? L'0' : L' ');\
	wss << value1;

#define PrimierRealArg(FTYPE, wss, operand, format) \
	wss << std::setprecision(pr+1); \
	wss << std_any_cast<FTYPE>(operand);


UCTOOLDYNAMIC extern std::mutex mutexHandler_;

UCTOOLDYNAMIC extern std::unordered_map<std::type_index, std::function<void(std_any&, std::wstringstream&, const CStringW&, int&)>> dwk_handlers_;

#pragma endregion	type mapping]





inline CStringA UcShortType(std::type_index tp)
{
	static std::map<string, string> s_shortType = {
		{ "class ATL::CStringT<wchar_t,class StrTraitMFC<wchar_t,class ATL::ChTraitsCRT<wchar_t> > >", "CStringW"},
		{ "class std::basic_string<wchar_t,struct std::char_traits<wchar_t>,class std::allocator<wchar_t> >", "wstring"},
		{ "class ATL::CStringT<char,class StrTraitMFC<char,class ATL::ChTraitsCRT<char> > >", "CStringA"},
		{ "class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >", "string"},
		//{ ",std::allocator<wstring >", ""},
		//{ ",std::allocator<string >", ""},
	};
	CStringA sName = tp.name();//class std::shared_ptr<class CStringArray>
#if CPP17_OR_LATER
	for (auto& [sl, sSrt] : s_shortType) {
		sName.Replace(sl.c_str(), sSrt.c_str());
	}
#else
	for (auto& pair : s_shortType) {
		sName.Replace(pair.first.c_str(), pair.second.c_str());
	}
#endif//class std::vector<wstring,class std::allocator<wstring > >
	//순서 때문에 여기서 해야 map은 ordered sort 되어 버린다.
	sName.Replace("class ", "");
	sName.Replace(",std::allocator<wstring >", "");
	sName.Replace(",std::allocator<string >", "");
	sName.Replace(" * __ptr64", "*");
	sName.Replace(" >", ">");

	return sName;
}

template< typename VType>
CStringA UcShortTypeT(VType& aval)
{
	auto& theType = typeid(aval);
	return UcShortType(theType);
}


//#define _UseDWK2CPP__ 
//수정할때 마다 너무 많이 컴파일 해서 cpp로 옮김. 원본은 cpp것.  여기는 복사본
#ifdef _UseDWK2CPP__
std::wstring DWK__anyToStringEx(const std_any& operand, const CStringW& format);
#else
inline std::wstring DWK__anyToStringEx(const std_any& operand, const CStringW& format)
{
	std::wstringstream wss;
	int w = 0;
	int pr = -1;
	DWK__parseFormat(format, w, pr);//format = L"%04v"(4,-1:"0042") L"%6.2v"(6,2:"   3.1") L"%-20v"(-20,-1:) L"%4v"(4,-1:"  42")
	auto& tp = operand.type();
	//tp = {_Data={_UndecoratedName="std::nullptr_t" _DecoratedName=".$$T" } }
#pragma region	[align
	if (w < 0) {
		wss << std::left;
		w = -w;
	}
	else
		wss << std::right;
	wss << std::setw(w);
#pragma endregion	align]

	std::lock_guard<std::mutex> lock(mutexHandler_);

	bool bDone = false;
	const char* ptp1(tp.name());//ptp = "struct HKEY__ * __ptr64" ptp1 = "class CJsonFieldsSave * __ptr64"
	auto ptp = UcShortType(tp);
	auto it = dwk_handlers_.find(operand.type());
	if (it != dwk_handlers_.end()) {
		it->second((std_any&)operand, wss, format, pr);
		bDone = true;
	}
	else {
#ifdef _DEBUGx
		auto ptp11 = UcShortType(tp);
		auto ptp12 = UcShortType(tp);
		auto ptp13 = UcShortType(tp);
		//for (auto& [ktp, hand] : dwk_handlers_) {
		//	bool bSameType = ktp == tp;
		//	TRACE("%s =? %s - bSame:%d\n", ptp1, ktp.name(), bSameType);
		//}
#endif // _DEBUG
#ifdef _OldTypeStr_
		static std::unordered_map<std::string, std::function<void(std_any&, std::wstringstream&, const CStringW&, int&)>> handlersStr_ = {
			{	"std::vector<", [](VAR_PARAMS) { wss << L"vector<>";	}},
			{	"std::map<", [](VAR_PARAMS) { wss << L"map<>";	}},
			{	"std::list<", [](VAR_PARAMS) { wss << L"list<>";	}},
			{	"KArray<", [](VAR_PARAMS) { wss << L"KArray<>";	}},
			{	"KStdMap<", [](VAR_PARAMS) { wss << L"KStdMap<>";	}},
			{	"KList<", [](VAR_PARAMS) { wss << L"KList<>";	}},
		};
		static vector<std::string> stdvml = {
			"std::vector<",		//class 
			"std::map<",			//class 
			"std::list<",			//class 
			"KArray<",				//class 
			"KStdMap<",				//class 
			"KList<",				//class 
		};
		std::string key2;
		for (auto& [vml, _0] : handlersStr_) {
			//for (auto& vml : stdvml) {
			if (strstr(ptp.GetString(), vml.c_str())) {
				key2 = vml;
				break;
			}
		}
		if (key2.length() > 0) {
			auto it1 = handlersStr_.find(key2);
			if (it1 != handlersStr_.end()) {
				it1->second((std_any&)operand, wss, format, pr);
				bDone = true;
			}
		}
#else
		auto ptpw = CStringW(ptp);
		wss << ptpw.GetString();
		bDone = true;
#endif // _OldTypeStr_
	}

	if (!bDone) {
		//ASSERT(strncmp(ptp, "enum", 4) == 0);//swtp.Left(4) == L"enum")
		// 이름없는 enum은 이미 처리 되었고, 이름 있는 경우는 tuple로 래핑 해서 여기 올리가 없다.
		OutputDebugStringA(ptp.GetString());
		OutputDebugStringA(" @@@@@@@@@@@@@@ format type DWK__anyToStringEx @@@@@@@@@@@@@@@\r\n");
		wss << L"<type:" << (PWS)CStringW(ptp) << L">";
	}
	return wss.str();
}
#endif // _UseDWK2CPP__

inline bool DWK__isFormatChar(wchar_t cf)
{
	static auto fch = L"vsdfrtxX";// 원래는 '%v' 만 쓰는데, 이전꺼와 호환을 위해 t:는 wstring r:CStringW
	auto b = wcschr(fch, cf) != nullptr;
	return b;
}
// MyFormat 함수 정의
inline CStringW DwkFormatStr(const wchar_t* fmt, const std::vector<std_any>& arAny)
{
	std::wstringstream result;
	const wchar_t* p = fmt;
	size_t argIndex = 0;
	int ipc = 0;

	while (*p != L'\0')
	{
		if (*p == L'%' && *(p + 1) != L'%') {
			++ipc; // 몇 번째 %
			const wchar_t* start = p;
			p++; // Skip '%'

			CStringW format = L"%";
			bool hasHash = false;

			// 파싱: '#', '0-9', 'l', 'I64', 'd', 'u', 'x', 'X'
			while (*p != L'\0' && !DWK__isFormatChar(*p))
			{
				if (*p == L'#') {
					hasHash = true;
					format += *p++;
				}
				else if (iswdigit(*p) || *p == L'l' || *p == L'I' || *p == L'.' || *p == L'-')
					format += *p++;
				else
					break;
			}
			if (*p != L'\0' && DWK__isFormatChar(*p)) {
				format += *p++;
			}
			if (argIndex < arAny.size())
			{
				auto& operand = arAny[argIndex];
				auto chLast = format.GetLength() > 0 ? format[format.GetLength() - 1] : L'\0';
				if (operand.type() == typeid(void*)) {// void* == HANDLE
					void* handle = std_any_cast<void*>(operand);
					int64_t handleInt = reinterpret_cast<int64_t>(handle);

					if (!format.IsEmpty() && (chLast == L'x' || chLast == L'X')) {
						if (hasHash)
							result << (chLast == L'X' ? L"0X" : L"0x");
						result << std::hex;
						if (chLast == L'X')
							result << std::uppercase;
						result << handleInt;
					}
					else
						result << handleInt;
				}
				else {
					auto formattedString = DWK__anyToStringEx(operand, format);
					if (hasHash && !format.IsEmpty() && (chLast == L'x' || chLast == L'X')) {
						result << L"0" << chLast;
					}
					result << formattedString;
				}
				argIndex++;
			}
			else
				result.write(start, p - start); // Incomplete format specification
		}
		else
			result << *p++;
	}
	CStringW sw(result.str().c_str());
	return std::move(sw);
}

inline CStringW DwkGetErrStr(int nErr)
{
	LPWSTR mbuf = nullptr;
	auto lm = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, nErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&mbuf, 0, NULL);

	CStringW sErr;
	sErr.Format(L"%d: %s", nErr, mbuf);
	sErr.TrimRight();
	LocalFree(mbuf);
	return sErr;
}
#ifndef MAPTOS // see KPrW(v)
#define MAPTOS(v) {v,L#v}
#endif

//using EnumPairMap = std::pair<std::type_index, std::vector<std::pair<int, std::wstring>> >;
template<typename TENUM>
CStringW UcEnumToStr(TENUM e) {
#if CPP17_OR_LATER
	auto sm = magic_enum::enum_name(e);//sm = "FMPDFE_INVALID_PARAM"
	return CStringW(sm.data());//string_view 타입 이므로 data()
#else
	// C++14에서는 magic_enum을 사용할 수 없으므로 기본값 반환
	return CStringW(_T("Unknown"));
#endif
}
/// <summary>
/// 각종 타이ㅂ을 std_any타입으로 변환해 준다.
/// 살짝 사용자 의견이 개입한다. 특히 DEBUG용으로 쓰므로 enum인 경우 enum 정보도 출력 되게 조절한다.
/// </summary>
/// <typeparam name="T">각종 타입</typeparam>
/// <param name="arg">각종 타입</param>
/// <returns>std_any</returns>
// CArray는 복사 생성자가 삭제되어 있어서 특수화 필요
//template<typename TArrayElem>
//std_any DwktoAny(CArray<TArrayElem, TArrayElem>& arg)
//{
//	CStringA sArray;
//	sArray.Format("CArray[%d]", arg.GetCount());
//	return std_any(sArray);
//}

#if CPP17_OR_LATER
#else
// C++14: 포인터일 때만 null 비교 인스턴스화 (비포인터 T에서 static_cast<T>(0) 시 C2440 방지)//dwk: 2026-02-25 12:28 
template<typename T>
typename std::enable_if<std::is_pointer<std::decay_t<T>>::value, std_any>::type
DwkToAnyPointerImpl(T&& arg, const CStringA& sNull) {
	if (arg == static_cast<std::decay_t<T>>(0)) return std_any(sNull);
	return std_any(std::forward<T>(arg));
}
template<typename T>
typename std::enable_if<!std::is_pointer<std::decay_t<T>>::value, std_any>::type
DwkToAnyPointerImpl(T&& arg, const CStringA& sNull) {
	(void)sNull;
	return std_any(std::forward<T>(arg));
}
#endif

template<typename T>
std_any DwktoAny(T&& arg)
{
	auto pType = UcShortTypeT(arg);// nullptr은 "std::nullptr_t" 로 인식 한다. 하지만 std::nullptr_t 를 쓰면 에러 난다.
	//auto pType = typeid(arg).name();//enum IcEdSplinEditDragPointJig::PointType
	auto FncNullObj = [pType]() -> CStringA {
		CStringA sa; sa.Format("(%s)NULL", pType.GetString());// (CSomeClass*)NULL
		return std::move(sa);
		};//enum `2'::<unnamed-enum-eTest1>

	// CString 타입을 먼저 체크 (enum으로 잘못 인식되는 것을 방지)
	if (std::is_same<std::decay_t<T>, CString>::value ||
		std::is_same<std::decay_t<T>, CStringW>::value ||
		std::is_same<std::decay_t<T>, CStringA>::value) {
		return std_any(std::forward<T>(arg));
	}

	/// enum은 가급적 enum명으로 출력 된다. 정수로 바로 출력 되면 뭔지 아나?
#if CPP17_OR_LATER
	if constexpr (std::is_enum_v<std::decay_t<T>>) {
#else
	if (std::is_enum<std::decay_t<T>>::value) {
#endif
		/// unnamed enum은 바로 값의 심볼을 뽑아 낼수 있다.
		const char* pfs = __FUNCSIG__;
		auto lenfs = strlen(pfs);
		bool bUnamedEnum = false;
		size_t iFound = -1;
		char* pFound = NULL;

		//sm1 = "eValue1"		pType = "enum MyEnum"						enum class MyEnum { eValue1, eValue2};		enum MyEnum::eValue1{0}
		//sm2	= "edValue2"	pType = "enum DEnum"							enum DEnum { edValue1, edValue2};			enum DEnum::edValue2{1}
		//sm3 = "enValue3"	pType = "enum <unnamed-enum-enValue1>"	enum { enValue1, enValue2, enValue3 };		enum::enValue3{2}
#if CPP17_OR_LATER
		auto sm = magic_enum::enum_name(arg);//sm = "FMPDFE_INVALID_PARAM"
		bUnamedEnum = strstr(pType, "<unnamed-") != nullptr;
		string sType(pType);
		if (!bUnamedEnum)//if(sType.substr(0, 5) == "enum ")
			sType = sType.substr(5);//"enum "을 뺀 
		else
			sType = sType.substr(0, 4);//"enum"만

		CStringA sEnum;//MyEnum::eValue1{0}, DEnum::edValue2{1}, enum::enValue3{2}
		sEnum.Format("%s::%s{%d}", sType.c_str(), sm.data(), (int)arg);//속하지 않은데 억지로 캐스팅 한 경우 "DEnum::(null){3}"
#else
		// C++14에서는 enum 값을 직접 문자열로 변환
		std::string sm = "enum";// std::to_string(static_cast<int>(arg));
		bUnamedEnum = strstr(pType, "<unnamed-") != nullptr;
		string sType(pType);
		CStringA sEnum;//MyEnum::eValue1{0}, DEnum::edValue2{1}, enum::enValue3{2}
		sEnum.Format("%s::%s{--}", sType.c_str(), sm.data());//속하지 않은데 억지로 캐스팅 한 경우 "DEnum::(null){3}"
#endif
		return std_any(sEnum);
	}
	else {// Enum 타입이 아니면 그대로 any로 저장
		try {
#if CPP17_OR_LATER
			if constexpr (std::is_pointer_v<std::decay_t<T>>) {/// 여긴 컴파일 타임 이라 아예 F9 break point 도 안걸리네.
				if (arg == nullptr) {/// 캐스트가 있어 null type이 확실한 경우 여기로 들어간다. arg = 0x0000000000000000 <NULL>//dwk: 2025-08-07 17:10 
					// (LPCWSTR)NULL 포인터 체크 추가: 주의: 그냥 NULL은 아래 int&& 로 인식
					auto sNull = FncNullObj();///sNull = "NULL wchar_t const*" 일때는 그냥 NULL 과는 완전히 다르게 인식 하네.
					return std_any(sNull);
				}// 그냥 NULL은 0으로 인식 하여 아예 이안으로 컴파일 당시 constexpr 부터 제외 되었다.
			}
#else
			// C++14: 포인터일 때만 static_cast 사용(헬퍼에서). CString 등 비포인터 T에서 C2440 방지//dwk: 2026-02-25 12:28 
			return DwkToAnyPointerImpl(std::forward<T>(arg), FncNullObj());
			//if (arg == nullptr) {//dwk: 2026-02-25 12:03 
			//	auto sNull = FncNullObj();
			//	return std_any(sNull);
			//}
			//return std_any(std::forward<T>(arg)); // nullptr이 아니면 포인터 값을 그대로 저장 (C++17 분기와 동일)

			//if (std::is_pointer<std::decay_t<T>>::value) {/// 여긴 컴파일 타임 이라 아예 F9 break point 도 안걸리네.
			//	auto sNull = FncNullObj(); // (CSomeClass*)NULL
			//	return std_any(sNull);//return std::move(aa);
			//}
#endif
			/// 그냥 NULL일때는 int&& : 0 값으로 인식 해버리네//dwk: 2025-08-08 10:14 
			if (&arg != nullptr) {///그냥 캐스트 없이 nullptr 일 떄와 NULL일때 이 안으로 들어간다. 그리고 최종적으로 (null) 만 찍힌다.
				return std_any(std::forward<T>(arg));//pType = "std::nullptr_t"
				/// 여기서 에러나면: DWKTRACE에 any 로 바꿀수 없는 객체를 붙였다. CStringArray같은 거다.
			}
			else {// &arg == nullptr 일수가 있나?
				auto sNull = FncNullObj(); // (CSomeClass*)NULL
				return std_any(sNull);//return std::move(aa);
			}
		}
		catch (const std::exception& e) {
			auto aa = std_any(pType);
			TRACE("DwktoAny std::exception : %s\n", e.what());
			return (aa);
		}
		catch (...) {
			auto aa = std_any(pType);
			TRACE("DwktoAny ...\n");
			return (aa);
		}
	}
	}
//template<>
//inline std_any DwktoAny<CString>(CString value){
//	return std_any(std::wstring((LPCWSTR)value));
//}
//template<>
//inline std_any DwktoAny<CStringW>(CStringW value){
//	return std_any(std::wstring((LPCWSTR)value));
//}
//template<>
//inline std_any DwktoAny<CStringA>(CStringA value){
//	return std_any(std::string((LPCSTR)value));
//}

template<typename... Args>
std::vector<std_any> prepareAnyArgs(Args&&... args) {
	return { DwktoAny(std::forward<Args>(args))... };
}

/// rvalue는 forward 하고, lvalue는 복사하여 any를 만든다. 결국 any를 만드는 수고가 있으니, 성능은 포인터만 쓰는 CStringW 보다 떨어질수 밖에.
/// 가급적 포인터가 가도록 .c_str()을 붙여서 가변인수에 넣어 주면 효율적.
template<typename... Args>
CStringW DwkFormat(const wchar_t* fmt, Args&&... args) {
	//	std::vector<std_any> arAny = {              std::forward<Args>(args)...};//모두 any로 변환 할 경우
	std::vector<std_any> arAny = prepareAnyArgs(std::forward<Args>(args)...);
	return std::move(DwkFormatStr(fmt, arAny));
}
template<typename... Args>
CStringW DwkFormat(const char* fmtA, Args&&... args) {
	//	std::vector<std_any> arAny = {              std::forward<Args>(args)...};//모두 any로 변환 할 경우
	std::vector<std_any> arAny = prepareAnyArgs(std::forward<Args>(args)...);
	CStringW fmt(fmtA);
	return std::move(DwkFormatStr(fmt, arAny));
}


#pragma endregion	]DWK_FORMAT


/// dll에서 공유하는 스레드별 스택 맵을 얻는다.
/// DLL 또는 EXE 중 한군데서만 정의 해야 한다.

UCTOOLDYNAMIC std::shared_ptr<std::map<DWORD, int>> GetMapStack();
//AFX_EXT_CLASS dll인 경우
UCTOOLDYNAMIC std::shared_ptr<std::map<DWORD, std::stack<LPCWSTR>>> GetMapStackFunc();
#ifndef _PUT_THIS_TO_DLL_OR_EXE_
//AFX_EXT_CLASS AFX_EXT_API
#define _PUT_THIS_TO_DLL_OR_EXE_ \
UCTOOLDYNAMIC std::shared_ptr<std::map<DWORD, int>>  GetMapStack() {\
	static std::shared_ptr<std::map<DWORD, int>> _mapStack;\
	if (!_mapStack)\
		_mapStack = std::make_shared<std::map<DWORD, int>>();\
	return _mapStack;\
}\
UCTOOLDYNAMIC std::shared_ptr<std::map<DWORD, std::stack<LPCWSTR>>> GetMapStackFunc() {\
	static std::shared_ptr<std::map<DWORD, std::stack<LPCWSTR>>> _mapStkFunc;\
	if (!_mapStkFunc)\
		_mapStkFunc = std::make_shared<std::map<DWORD, std::stack<LPCWSTR>>>();\
	return _mapStkFunc;\
}

/// sample:
//#include "UcTool/UcDebug.h"
//_PUT_THIS_TO_DLL_OR_EXE_;
#endif // _PUT_THIS_TO_DLL_OR_EXE_

/// static global 변수들은 초기화 순서에 따라 오류가 날수 있으니
/// static 전용 객체 하나로 합친다.
class UCTOOLDYNAMIC KTrace
{
public:
	//DLL에서 static 멤버변수를 전체 공유 하려면, 레거시 방식으로 cpp에서 정의 해야 한다.(inline static 은 안된다.)
	//static std::shared_ptr<KTrace> instance_;
	//static std::once_flag initFlag_;//std::call_once에서 사용

	static int wHd_;//dwk: 2025-08-07 17:11  함수 덤프할 때 함수명 출력 폭. 0 이면 폭 없이 출력 한다. 20 정도면 적당하다. 너무 크면 오히려 가독성이 떨어진다.
public:
	/// GSingleton<KTrace>::GetInstance() 을 쓰려면 UcTool.h를 include해야 해서, 단독 singlton 을 쓴다.KTrace 마찬가지
	
	//static 함수는 인라인으로 하면 모듈만 중복될 뿐 작동에는 문제 없다. 변수만 단일 하게 하면 된다.
	static std::shared_ptr<KTrace> Instance();
	static void setWidth(int whd) { wHd_ = whd; }
	 
	std::mutex             _csMapStack;
	// 스레드별로 스택 깊이를 매핑 해 둔다. 스레드별로 커졌다 작아졌다 한다.
	/// GetMapStack()으로 대체
	///std::map<DWORD, int> _mapStack;
	// 건너띌 함수덤프는 아래 처럼 추가 해주면 된다.

	enum { eDump, eNotDump };
	std::map<std::wstring, int> _funcNoOutput = {
		{L"main", 0}, //예: 함수 main은 0으로서 덤프하지 않는다.
		{L"CVmsCCTV::cbNotify", eNotDump},
		{L"CVmsCCTV::DoNotify", eNotDump},
	};
	//std::map<DWORD, std::stack<LPCWSTR>> _mapStkFunk;
	//std::stack<LPCWSTR> _stkFunk;
	// 백그라운드지 메인 스레드인지 체크하기 위해 메인스레드 ID를 보관해 둔다.
	DWORD         _idMainThread{ 0 };
	// 함수덤프별 걸린 시간을 제기 위핸 틱 카운트 보관
	LONG64        _lastTik{ 0 };
	LARGE_INTEGER _lastQTik{ 0 };
	LARGE_INTEGER _frequency{ 0 };

	//[ 빠르게 반복되는 대표 함수(DWKFUNC 쪽)를 감지/요약하기 위한 정보
	struct RepeatInfo {
		CStringW   funcName;   // 대표 함수 이름
		ULONGLONG  lastTick{}; // 마지막 호출 시각 (GetTickCount64 기준)
		int        count{};    // 같은 대표 함수가 연속으로 호출된 횟수
		bool       squashing{};// 6번째 이상부터 요약 모드(출력 대신 '.' 만 찍는 상태)
	};

	std::mutex _csRepeat;                     // 반복 정보 보호용
	std::map<DWORD, RepeatInfo> _mapRepeat;   // 스레드별 대표 함수 반복 정보
	//] 빠르게 반복되는 대표 함수(DWKFUNC 쪽)를 감지/요약하기 위한 정보

	CStringW Trace(CStringW&& str, LPCWSTR sFile, int nLine, LPCWSTR pFunc, void* pDummy = NULL);
	CStringW KFormat(LPCWSTR fmt, ...);

	function<void(LPCWSTR)> _cbTraceLog;//dwk: 2025-05-15 16:12 이게 Release에서 하려면 _DWKTRACE_TRUE 를 프로젝트에 정의해줘야
	void OutputDwkDebugString(LPCWSTR str, bool bOutput = true, bool bLog = true);
	void GeneralLog(CString sFile, LPCWSTR str);
	/// 예: "C:\Outbin\QrEndec\x64\Debug\QrOut.log"
	/// CStringW sFile = UcGetModulePath(1) + L"\\QrOut.log";  
	/// auto ktr = KTrace::Instance();
	/// ktr->_cbTraceLog = [ktr, sFile](LPCWSTR sLog){
	///	ktr->GeneralLog(sFile, sLog);
	/// };
};


/// <summary>
/// thread에 따라 스택 레벨과 함수명을 증/감 시키는 장치
/// </summary>
class UCTOOLDYNAMIC KStackDump
{
public:
	KStackDump(LPCWSTR pFunc = nullptr)
	{
		auto ktr = KTrace::Instance();
		std::lock_guard<std::mutex> lck(ktr->_csMapStack);
		_idThread = ::GetCurrentThreadId();
		auto& mp = *GetMapStack();// ktr->_mapStack;
		auto fl = mp.find(_idThread);//현 스레드로 찾는다. static 으로 map을 사용시 로드 순서에 따라 여기서 죽을 수 있다.
		if (fl == mp.end()) {
			mp[_idThread] = 0;// 이 스레드가 처음 불려 지면 0 부터 시작 한다.
			fl = mp.find(_idThread);//다시 찾는다.
		}
		++fl->second;//함수가 불렸으니 스택을 증가 시킨다.

		auto& mp1 = *GetMapStackFunc();// ktr->_mapStkFunk;/// 함수명 스택을 스레드별로 각각 관리 한다.
		auto fl1 = mp1.find(_idThread);//static 으로 map을 사용시 로드 순서에 따라 여기서 죽을 수 있다.
		if (fl1 == mp1.end()) {
			std::stack<LPCWSTR> stkFunkNew;
			mp1[_idThread] = std::move(stkFunkNew);
			fl1 = mp1.find(_idThread);
		}
		auto& stkFunk = fl1->second;
		//stkFunk.push(pFunc);
		stkFunk.push(pFunc ? pFunc : L"(null)");// 함수명을 추가로 쌓는다.
		//ASSERT(fl->second == stkFunk.size());
	}
	~KStackDump()
	{
		auto ktr = KTrace::Instance();
		std::lock_guard<std::mutex> lck(ktr->_csMapStack);
		auto& mp = *GetMapStack();//ktr->_mapStack;
		auto fl = mp.find(_idThread);
		--fl->second;// 스택 레벨을 감소 시킨다.
		auto& mp1 = *GetMapStackFunc();//ktr->_mapStkFunk;
		auto fl1 = mp1.find(_idThread);//static 으로 map을 사용시 로드 순서에 따라 여기서 죽을 수 있다.
		ASSERT(fl1 != mp1.end());
		auto& stkFunk = fl1->second;
		stkFunk.pop(); // 스택 함수명 하나를 버린다.
		ASSERT(fl->second == stkFunk.size());
	}
	//이번 스레드를 알아야 스택에 증가 했다 감소 하는데 키로 쓰인다.
	DWORD _idThread{ 0 };
};


#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

#ifdef _DWKTRACE_
#define _DWKTRACE_TRUE // <= _DWKTRACE_ 를 프로젝트 셋팅에 정의해 줘야 한다.
#endif

#pragma region	[KTRACE
/// _DWKTACE_ 이전에 _DEBUG 였는데, _DEBUG 때 모두 되면 문제니, 환경변수 PRJVAL == 3929 


class DWKSETTINGS {
public:
#if CPP17_OR_LATER
	INLINE_STATIC std::map<string, int> g_map = {
		{"do", 1},
	};
#else
	INLINE_STATIC std::map<string, int> g_map;
#endif
	//static bool get(const std_variant<int, std::string>& param)
	static bool get(const std::string& param)
	{
		return g_map[param] != 0;
	}

	static bool get(int param)
	{
		CStringA str; str.Format("%02d", param);
		return g_map[str.GetString()] != 0;
	}
};


#ifdef _DWKTRACE_TRUE // DWKTRACE <= _DWKTRACE_ 를 프로젝트 셋팅에 정의해 줘야 한다.

//#define DWKFORMAT DwkFormat

/// 함수 맨위에 넣으면 함수 이름이 덤프 된다.
#define DWKSTACDUMP KStackDump _no_DWKFUNC___(__FUNCTIONW__)
#define DWKKTRACE KTrace::Instance()->Trace(L"", __FILEW__, __LINE__, __FUNCTIONW__, nullptr)
// VS2015 호환: 빈 인수일 때 콤마 문제 해결
// VS2015에서는 ##__VA_ARGS__가 빈 값일 때 콤마를 제거하지 못하므로, _0 버전에서 콤마를 허용하도록 수정
#define DWKKTRACEVTOP_0(fmt, ...) KTrace::Instance()->Trace(DwkFormat(fmt), __FILEW__, __LINE__, __FUNCTIONW__)
#define DWKKTRACEVTOP_1(fmt, ...) KTrace::Instance()->Trace(DwkFormat(fmt, __VA_ARGS__), __FILEW__, __LINE__, __FUNCTIONW__)
// GET_MACRO로 빈/비빈 인수 구분 시도 (VS2015에서는 완벽하지 않을 수 있음)
#define DWKKTRACEVTOP_GET_MACRO(_1, NAME, ...) NAME
#define DWKKTRACEVTOP(fmt, ...) DWKKTRACEVTOP_GET_MACRO(__VA_ARGS__, DWKKTRACEVTOP_1, DWKKTRACEVTOP_0)(fmt, ##__VA_ARGS__)

#define DWKKTRACE0(fmt, ...) KTrace::Instance()->Trace(DwkFormat(fmt), __FILEW__, __LINE__, __FUNCTIONW__, &_no_DWKFUNC___)
#define DWKKTRACEV_1(fmt, ...) KTrace::Instance()->Trace(DwkFormat(fmt, __VA_ARGS__), __FILEW__, __LINE__, __FUNCTIONW__, &_no_DWKFUNC___)
//#define DWKKTRACEV_GET_MACRO(_1, _2, _3, NAME, ...) NAME
//#define DWKKTRACEV(fmt, ...) DWKKTRACEV_GET_MACRO(__VA_ARGS__, dummy, dummy, DWKKTRACE0, DWKKTRACEV_1)(fmt, ##__VA_ARGS__)

//dwk: 2025-12-10 09:44 
#define DWKKTRACEV(fmt, ...) KTrace::Instance()->Trace(DwkFormat(fmt, ##__VA_ARGS__), __FILEW__, __LINE__, __FUNCTIONW__, &_no_DWKFUNC___)
#define DWKTRACE(fmt, ...) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKUSETRACE DWKSTACDUMP
#define DWKFUNC DWKSTACDUMP; DWKKTRACE
//#define DWKFUNC KStackDump _no_DWKFUNC___(__FUNCTIONW__); KTrace::Instance()->Trace(L"", __FILEW__, __LINE__, __FUNCTIONW__, nullptr)

/// DWKFUNC와 비슷 하며 함수 맨위에 넣으면서 특정 메시지를 서식 문자열로 조합 하여 출력할 수 있다.
#define DWKFUNCV(fmt, ...) DWKSTACDUMP; DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
//#define DWKFUNCV(fmt, ...) KStackDump _no_DWKFUNC___(__FUNCTIONW__);\
//	KTrace::Instance()->Trace(DwkFormat(fmt, ##__VA_ARGS__), __FILEW__, __LINE__, __FUNCTIONW__)

/// 함수내 일반 덤프로서 그 함수시작부분에 반드시 DWKFUNC 또는 DWKFUNCV가 있어야 한다. (없는 경우 에러:_no_DWKFUNC___가 없다고 나온다.)
// VS2015 호환: DWKKTRACEV와 동일한 방식으로 처리
//#define DWKTRACE(fmt, ...) DWKKTRACEV(fmt, ##__VA_ARGS__)
//#define DWKTRACE(fmt, ...) KTrace::Instance()->Trace(DwkFormat(fmt, ##__VA_ARGS__), __FILEW__, __LINE__, __FUNCTIONW__, &_no_DWKFUNC___)

#define DWKNONE {}



EXTERN_STATIC DWKSETTINGS g_DWK;
//extern DWKSETTINGS g_DWK;

#define IFTK(tk) if(g_DWK.get(#tk)) //이거 굳이 할필요있나?
#define DWKFUNC_pr DWKSTACDUMP; IFTK(pr) DWKKTRACE 
#define DWKFUNCV_pr(fmt, ...) DWKSTACDUMP; IFTK(pr) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_pr(fmt, ...) IFTK(pr) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_ac DWKSTACDUMP; if(g_DWK.get("ac")) DWKKTRACE 
#define DWKFUNCV_ac(fmt, ...) DWKSTACDUMP; if(g_DWK.get("ac")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_ac(fmt, ...) if(g_DWK.get("ac")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_sh DWKSTACDUMP; if(g_DWK.get("sh")) DWKKTRACE
#define DWKFUNCV_sh(fmt, ...) DWKSTACDUMP; if(g_DWK.get("sh")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_sh(fmt, ...) if(g_DWK.get("sh")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_sl DWKSTACDUMP; if(g_DWK.get("sl")) DWKKTRACE 
#define DWKFUNCV_sl(fmt, ...) DWKSTACDUMP; if(g_DWK.get("sl")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_sl(fmt, ...) if(g_DWK.get("sl")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_do DWKSTACDUMP; if(g_DWK.get("do")) DWKKTRACE
#define DWKFUNCV_do(fmt, ...) DWKSTACDUMP; if(g_DWK.get("do")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_do(fmt, ...) if(g_DWK.get("do")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_wm DWKSTACDUMP; if(g_DWK.get("wm")) DWKKTRACE
#define DWKFUNCV_wm(fmt, ...) DWKSTACDUMP; if(g_DWK.get("wm")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_wm(fmt, ...) if(g_DWK.get("wm")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_ms DWKSTACDUMP; if(g_DWK.get("ms")) DWKKTRACE
#define DWKFUNCV_ms(fmt, ...) DWKSTACDUMP; if(g_DWK.get("ms")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_ms(fmt, ...) if(g_DWK.get("ms")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_55 DWKSTACDUMP; if(g_DWK.get("05")) DWKKTRACE
#define DWKFUNCV_55(fmt, ...) DWKSTACDUMP; if(g_DWK.get("05")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_55(fmt, ...) if(g_DWK.get("05")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_66 DWKSTACDUMP; if(g_DWK.get("06")) DWKKTRACE
#define DWKFUNCV_66(fmt, ...) DWKSTACDUMP; if(g_DWK.get("06")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_66(fmt, ...) if(g_DWK.get("06")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_77 DWKSTACDUMP; if(g_DWK.get("07")) DWKKTRACE
#define DWKFUNCV_77(fmt, ...) DWKSTACDUMP; if(g_DWK.get("07")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_77(fmt, ...) if(g_DWK.get("07")) DWKKTRACEV(fmt, ##__VA_ARGS__)

#define DWKFUNC_88 DWKSTACDUMP; if(g_DWK.get("08")) DWKKTRACE
#define DWKFUNCV_88(fmt, ...) DWKSTACDUMP; if(g_DWK.get("08")) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_88(fmt, ...) if(g_DWK.get("08")) DWKKTRACEV(fmt, ##__VA_ARGS__)


#define DWKFUNC_K(k) DWKSTACDUMP; if(g_DWK.get((k))) DWKKTRACE
#define DWKFUNCV_K(k, fmt, ...) DWKSTACDUMP; if(g_DWK.get((k))) DWKKTRACEVTOP(fmt, ##__VA_ARGS__)
#define DWKTRACE_K(k, fmt, ...) if(g_DWK.get((k))) DWKKTRACEV(fmt, ##__VA_ARGS__)
// ex: DWKFUNC_K("ex")
// ex: DWKFUNCV_K("ex", L"%v", val)

#define DWKREADINI(g_vv) {\
	g_DWK.g_vv = GetPrivateProfileIntW(L"DWKSETTINGS", L#g_vv, g_DWK.g_vv, LR"(.\DwkSettings.ini)");\
}

inline std::map<std::string, int> LoadSectionInts(const std::string & section, const std::string & iniPath)
{
	std::map<std::string, int> result;
	const DWORD bufferSize = 4096;
	char buffer[bufferSize] = { 0 };
	DWORD charsRead = GetPrivateProfileSectionA(section.c_str(), buffer, bufferSize, iniPath.c_str());
	if (charsRead == 0)
		return result;//std::wcout << L"Section not found or empty." << std::endl;

	char* line = buffer;
	while (*line) {
		std::string keyval = line;  // "key=value"
		size_t eqPos = keyval.find(L'=');
		if (eqPos != std::string::npos) {
			std::string key = keyval.substr(0, eqPos);
			std::string valStr = keyval.substr(eqPos + 1);
			int val = std::stoi(valStr);
			result[key] = val;
		}
		line += strlen(line) + 1;  // 다음 null-terminated 문자열로 이동
	}
	return result;
}
inline void DwkReadIniSettings() //dwk: 2025-03-21 10:07  
{
	g_DWK.g_map = LoadSectionInts("DWKSETTINGS", R"(.\DwkSettings.ini)");
}


#else //_DWKTRACE_TRUE // DWKTRACE
//#define VOID_VARARGS do { (void)(__VA_ARGS__); } while(0)
/// 앞에 ... 가 없는데, __VA_ARGS__ 가 있으면 에러 난다. 그래서 아래처럼 한다.
#define DWKSTACDUMP {}
#define DWKUSETRACE DWKSTACDUMP
#define DWKFUNC {}
#define DWKFUNCV(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
//#define DWKTRACE(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE(fmt, ...) {}
#define DWKKTRACE0(fmt) {}
#define DWKFUNC0(fmt) {}

#define DWKFUNC_pr {}
#define DWKFUNC_ac {}
#define DWKFUNC_sh {}
#define DWKFUNC_sl {}
#define DWKFUNC_do {}
#define DWKFUNC_wm {}
#define DWKFUNC_ms {}
#define DWKFUNC_55 {}
#define DWKFUNC_66 {}
#define DWKFUNC_77 {}
#define DWKFUNC_88 {}

#define DWKFUNCV_pr(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_ac(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_sh(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_sl(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_do(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_wm(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_ms(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_55(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_66(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_77(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKFUNCV_88(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_pr(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_ac(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_sh(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_sl(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_do(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_wm(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_ms(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_55(fmt, ...) {}//do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_66(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_77(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#define DWKTRACE_88(fmt, ...) do{(void)(__VA_ARGS__);}while(0)
#endif //_DWKTRACE_TRUE <= _DWKTRACE_ 를 프로젝트 셋팅에 정의해 줘야 한다.

#pragma endregion	]KTRACE


class DbgCString {
public:
#if CPP17_OR_LATER
	INLINE_STATIC std::string to_findW = "class ATL::CStringT<wchar_t,class StrTraitMFC<wchar_t,class ATL::ChTraitsCRT<wchar_t> > >";
	INLINE_STATIC std::string to_replaceW = "CStringW";
	INLINE_STATIC std::string to_findA = "class ATL::CStringT<char,class StrTraitMFC<char,class ATL::ChTraitsCRT<char> > >";
	INLINE_STATIC std::string  to_replaceA = "CStringA";

	INLINE_STATIC LPCWSTR to_findWW = L"class ATL::CStringT<wchar_t,class StrTraitMFC<wchar_t,class ATL::ChTraitsCRT<wchar_t> > >";
	INLINE_STATIC LPCWSTR to_replaceWW = L"CStringW";
	INLINE_STATIC LPCWSTR to_findWA = L"class ATL::CStringT<char,class StrTraitMFC<char,class ATL::ChTraitsCRT<char> > >";
	INLINE_STATIC LPCWSTR to_replaceWA = L"CStringA";
#else
	INLINE_STATIC std::string to_findW;
	INLINE_STATIC std::string to_replaceW;
	INLINE_STATIC std::string to_findA;
	INLINE_STATIC std::string  to_replaceA;

	INLINE_STATIC LPCWSTR to_findWW;
	INLINE_STATIC LPCWSTR to_replaceWW;
	INLINE_STATIC LPCWSTR to_findWA;
	INLINE_STATIC LPCWSTR to_replaceWA;
#endif

	static void ReplaceCStringW(std::string& str) {
		size_t pos = 0;
		while ((pos = str.find(to_findW, pos)) != std::string::npos) {
			str.replace(pos, to_findW.length(), to_replaceW);
			pos += to_replaceW.length(); // 대체 후 위치 조정
		}
	}

	static void ReplaceCStringA(std::string& str) {
		size_t pos = 0;
		while ((pos = str.find(to_findA, pos)) != std::string::npos) {
			str.replace(pos, to_findA.length(), to_replaceA);
			pos += to_replaceA.length(); // 대체 후 위치 조정
		}
	}

	// CStringW를 매개변수로 받는 문자열 치환 함수
	static void ReplaceCStringW(CStringW& str) {
		str.Replace(to_findWW, to_replaceWW);
	}

	// CStringA를 매개변수로 받는 문자열 치환 함수
	static void ReplaceCStringA(CStringW& str) {
		str.Replace(to_findWA, to_replaceWA);
	}

	template<typename TSTR>
	static void ReplaceCString(TSTR& str) {
		ReplaceCStringW(str);
		ReplaceCStringA(str);
	}
};

#include <regex>
inline std::string ShortenLambdaName(const std::string & input)
{
	std::regex lambdaRegex(R"(lambda_([0-9a-fA-F]{4})[0-9a-fA-F]*)");
	return std::regex_replace(input, lambdaRegex, "lambda_$1");
}
//template<typename CharT>
//std::basic_string<CharT> ShortenLambdaName(const std::basic_string<CharT>& input) {
//	using StringT = std::basic_string<CharT>;
//	using RegexT = std::basic_regex<CharT>;
//	LPCSTR sptn = R"(lambda_([0-9a-fA-F]{4})[0-9a-fA-F]*)";
//	ASSERT(strlen(sptn) == 35);
//	CharT pbuf[35+1]{};
//	for (int i = 0; i < 36; ++i)
//		pbuf[i] = sptn[i];
//	//const CharT prefix[] = { 'l','a','m','b','d','a','_', '\0' };
//	//const CharT pattern[] = { '(','l','a','m','b','d','a','_',    '[','0','-','9','a','-','f','A','-','F','{','4','}',')','[','0','-','9','a','-','f','A','-','F','*',']', 0 };
//	const CharT pattern[] = { '(','l','a','m','b','d','a','_','(','[','0','-','9','a','-','f','A','-','F',']','{','4','}',')','[','0','-','9','a','-','f','A','-','F',']','*',')', '\0' };
//	const CharT replaceFmt[] = { 'l','a','m','b','d','a','_','$','1', '\0' };
//
//	RegexT lambdaRegex(pattern);
//	return std::regex_replace(input, lambdaRegex, replaceFmt);
//}
#ifdef __MOVE_TO_HD_
CStringW UcPrintStack(int ncall = 2, PAS sDelimter = " << ");
#else
//#include <ImageHlp.h>C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\
//		ImageHlp.h(93,16): error C2011: '_LOADED_IMAGE': 'struct' type redefinition
#ifndef _IMAGEHLP64 //이걸 해주니 컴파일 되제//dwk: 2025-08-04 10:23 
#include <ImageHlp.h>//DbgHelp.h
#endif //_IMAGEHLP64
#pragma comment(lib, "Dbghelp.lib")

// stack 2개 함수를 받아 온다.
// 이 함수와 KwPrintStack를 부른 함수는 자동 제외
inline CStringW UcPrintStack(int ncall = 2, PAS sDelimter = " << ")
{
	static std::mutex s_mtx;
	std::lock_guard<std::mutex> lck(s_mtx);

	unsigned int   i{};
	void* stack[100];
	unsigned short frames{};
	SYMBOL_INFO* symbol{};
	CStringW stk;
	//static CKCriticalSection cs_;
	HANDLE  process{};		//static 
	BOOL s_bInit{ FALSE };	//static 
	if (!s_bInit)
	{
		process = GetCurrentProcess();//SymCleanup(process);를 아래에 매번 해주는거 봐봐.
		s_bInit = SymInitialize(process, NULL, TRUE);
	}
	if (s_bInit)
	{
		frames = CaptureStackBackTrace(0, 100, stack, NULL);
		ASSERT(frames > 2);
		symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 1024 * sizeof(char), 1);
		KAtEnd _free_symbol([&symbol]() {		free(symbol);		});
		if (symbol == nullptr)
			return {};
		symbol->MaxNameLen = 1024;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		int ncall2 = (ncall + 2); // 이함수와 콜러 제외
		stringstream ss;
		int nfnc = 0;
		bool bFncLast = false;
		int nStd = 0;
		for (i = 0; i < frames; i++)
		{
			BOOL bCopy = SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
			//TRACE("%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
			if (i >= 2) //skip 2: excluding this and caller function
			{
				static std::set<std::string> s_prfx = {//34길이 문자열
					"std::invoke<void (__cdecl*&)(void ",//*,_st_table_title_info_ *)",//,FDSelectedEvid
					"std::_Invoker_ret<std::_Unforced>:",//:_Call<void (__cdecl*&)",//(void *,_st_table_
					"std::_Call_binder<std::_Unforced,0",//,1,void (__cdecl*)",//(void *,_st_table_title
					"std::_Binder<std::_Unforced,void (",//__cdecl&)",//(void *,_st_table_title_info_ *)
					"std::invoke<std::_Binder<std::_Unf",//orced,void (__cdecl&)",//(void *,_st_table_ti
					"std::_Func_impl_no_alloc<std::_Bin",//der<std::_Unforced,void (__cdecl&)",//(void *
					"std::_Func_class<void>::operator()",//",//() Line 926	C++	Non-user code. Symbols l
					"std::_Packaged_state<void __cdecl(",//void)>::_Call_immediate()",// Line 533	C++	
					"std::packaged_task<void __cdecl(vo",//id)>::operator()",//() Line 1253	C++	Non-user
					"std::invoke<void <lambda>(void) &>",//",//(FDRe
					"std::_Func_impl_no_alloc<void <lam",//bda>",//(
				};
				static auto s_prlen = (*s_prfx.begin()).length();
				if (tchlen(symbol->Name) > 0)
				{
					const CHAR* prefix = "std::";// invoke<void(__cdecl*&)(";
					bool bFnc = false;
					auto len = strlen((LPCSTR)symbol->Name);
					if (len >= s_prlen)
					{
						std::string spr((LPCSTR)symbol->Name, s_prlen);
						auto it = s_prfx.find(spr);
						if (it != s_prfx.end())
							bFnc = true;
					}
					if (bFnc) {//람다함수나 function 을 직접 부른 경우 중지
						if (nStd == 0)
							ss << "std";
						nStd++;
					}
					else {
						if (bFncLast) {
							CStringA sSt; sSt.Format("(%d)%s", nStd, sDelimter);
							ss << (PAS)sSt;// sDelimter;
						}//std 마무리 후...
						std::string result = ShortenLambdaName(std::string(symbol->Name));
						ss << result << sDelimter;
						nfnc++;
						if (nfnc >= 3)
							break;
						nStd = 0;
					}
					bFncLast = bFnc;
				}
			}
		}//for
		stk = CStringW(ss.str().c_str());
	}
	SymCleanup(process);// 이거 해도 SymInitialize 를 한번만 해도 되는 군.
	return stk;
}
#endif // __MOVE_TO_HD_

CStringW UcWaitStr(UINT nId);

bool UcIsBuildOverdue(int daysThreshold = 100);


inline CStringA DWK__EMapA(LPCSTR ars, std::initializer_list<INT64> ari, INT64 ekey, int ltrim)
{
	std::map<INT64, std::string> vt;
	std::string s;
	std::stringstream ss(ars);
	for (int i = 0; std::getline(ss, s, ','); i++)
	{
		auto itk = ari.begin() + i;
		size_t first = s.find_first_not_of(' ');
		size_t last = s.find_last_not_of(' ');
		if (first != std::string::npos || last == std::string::npos)
			s = s.substr(first, last - first + 1);
		vt[*itk] = s;
	}
	CStringA rs;
	auto it = vt.find(ekey);
	if (it != vt.end())
		rs.Format("%s(%lld)", it->second.c_str(), ekey);
	else
		rs.Format("[none](%lld)", ekey);
	if (ltrim > 0)
		rs = rs.Mid(ltrim);
	return rs;
}
inline CStringW DWK__EMap(LPCSTR ars, std::initializer_list<INT64> ari, INT64 ekey, int ltrim)
{
	return CStringW(DWK__EMapA(ars, ari, ekey, ltrim));
}
#define DWK__EMAPA(k, ...) DWK__EMapA({#__VA_ARGS__}, {(INT64)__VA_ARGS__}, ((INT64)k), 0)	 //ltrim, 
#define DWK__EMAP(k,  ...) DWK__EMap( {#__VA_ARGS__}, {(INT64)__VA_ARGS__}, ((INT64)k), 0)	 //ltrim, 
#ifdef _Samples__
TRACE(L"%d:%s\n", k, DWK__EMAP(k, SW_RESTORE, SW_MAXIMIZE, SW_MINIMIZE)); // prijnt enum string
TRACE("%d:%s\n", k, DWK__EMAPA(k, SW_RESTORE, SW_MAXIMIZE, SW_MINIMIZE));
#endif //_Samples__




#pragma region	[KTRACE

class DwkOutputFilter {
public:
	//static std::set<std::wstring> s_setFilterShow;/// 스레드 생성 단계 아이디: 요거만 보여
	//inline std::set<std::wstring> s_setFilterShow = {/// 스레드 생성 단계 아이디: 요거만 보여
	//	L"CFMReportApp::InitInstance",//DLL UI
	//	L"FMRptCreateReport",//API called from C#
	//	L"FMReportEvidenceInfoFF::ThreadProc_CreateReport",//thread 1
	//	L"FMReportEvidenceInfoBase::ThreadProc_CreateIndividualImagePdf",//thread 2
	//};
	///#define DWKAddFuncShow s_setFilterShow.insert(__FUNCTIONW__)
	//= ::GetCurrentThreadId()

	//cpp로 옮기니, FMParser에서 에러남
#if CPP17_OR_LATER
	INLINE_STATIC std::set<std::wstring> s_setFilterHide = {///예외로 숨겨
		L"FMReportCallback::SendProgressMsg",
		L"FMReportCallback::SendLogMsg",
		L"FMReportCallback::SendNotifyMsg",
		//L"FMReportEvidenceInfoBase::ThreadProc_CreateIndividualImageExcel",
		//L"FMFReportEvidenceDeailedList_FMF_SPO::ThreadProc_ExcelReport",
		//L"FMFReportEvidenceDeailedList_FMF_SPO::ThreadProc_DBReport",
	};


	/// <summary>
	///  1.스레드 시작 점에서 한번만 여기에 등록 하면 같은 스레드 함수는 다 출력 된다.
	///  2. 그 중 숨기고 싶은 것만 s_setFilterHide 에 등록 하면 숨긴다.
	/// </summary>
	INLINE_STATIC std::map<DWORD, std::wstring> s_setFilterShowID = {
		{0, L""},
	};
#else
	INLINE_STATIC std::set<std::wstring> s_setFilterHide;
	INLINE_STATIC std::map<DWORD, std::wstring> s_setFilterShowID;
#endif
	static void RegisterToShow(LPCWSTR swFunc) {
		DwkOutputFilter::s_setFilterShowID[GetCurrentThreadId()] = swFunc;
		TRACE(L"DWKTRACE Enabled: %#x : %s\n", GetCurrentThreadId(), swFunc);
	}
};

#ifdef _DWKTRACE_TRUE // DWKSetThread DwkThreadName OutputDwkDebugString
//#if (defined(_DEBUG) || defined(DwkReleaseDump)) && (!defined(NoDwkTrace))

inline CStringW KTrace::KFormat(LPCWSTR fmt, ...)
{
	CStringW sFmt;
	va_list args;
	va_start(args, fmt);
	sFmt.FormatV(fmt, args);
	va_end(args);
	return sFmt;
}


/// <summary>
/// thread 별로 stack 깊이를 보유 한다.
/// </summary>
//std::map<DWORD, int> KStackNum::mapStack_;
//std::mutex KStackNum::csMapStack_;

/// <summary>
/// 스레드별 이름을 주어서 DWK덤프 필터에 이용 한다.
/// 전체 솔루션에서 하나의 인스턴스만 존재하도록 Singleton 패턴 사용
/// </summary>

#ifdef _old_debug_filter__
class DwkThreadName {
private:
	std::map<DWORD, std::string> m_mapThrdName;
	std::mutex m_mutex;

	DwkThreadName() = default;
	DwkThreadName(const DwkThreadName&) = delete;
	DwkThreadName& operator=(const DwkThreadName&) = delete;

public:
	/// GSingleton<DwkThreadName>::GetInstance() 을 쓰려면 UcTool.h를 include해야 해서, 단독 singlton 을 쓴다.KTrace 마찬가지
	static DwkThreadName& Instance() {
		static DwkThreadName instance;
		return instance;
	}

	void SetThreadName(DWORD threadId, const std::string& name) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_mapThrdName[threadId] = name;
	}

	[[deprecated]]
	std::string GetThreadName(DWORD threadId) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_mapThrdName.find(threadId);
		return (it != m_mapThrdName.end()) ? it->second : "";
	}

	std::map<DWORD, std::string>& GetMap() { return m_mapThrdName; }
};



/// 백그라운드 스레드로 시작한 함수와 시작한 thread ID를 저장 한다.
inline void DwkSetThread(const string & func, const string & name) {
	if (!name.empty())
		DwkThreadName::Instance().SetThreadName(GetCurrentThreadId(), name);
	else
		DwkThreadName::Instance().SetThreadName(GetCurrentThreadId(), func);
}
/// __FUNCTION__ 로 저장 할수도 있고, name을 줄수도 있다. 
///	DWKSetThread() 또는 DWKSetThread("name") 매크로 함수도 가변으로 부를수 있다.
inline void DwkSetThread(const string & func) {
	DwkSetThread(func, "");
}
#define DWKSetThread(...) do { DwkSetThread(__FUNCTION__, ##__VA_ARGS__); } while(0)
#else
#define DWKSetThread(...) //오류 안나게. 찾아서 다 제거 하거 이것도 없애야
#endif //_old_debug_filter__


///스레드 시작 점에서 한번만 여기에 등록, 또는 DLL API 첫 시작점에서
#define DWKShowThreadID() do{DwkOutputFilter::RegisterToShow(__FUNCTIONW__);}while(0)

/// <summary>
/// 함수 안에서, 현재 스레드의 스택 만큼 마진을 주고 TRACE처럼 툴력 한다. KTrace::Instance()->Trace
/// </summary>
/// <param name="sMsg"></param>
inline CStringW KTrace::Trace(CStringW && sMsg, LPCWSTR sFile, int nLine, LPCWSTR pFunc, void* pDummy)
{
	auto ktr = KTrace::Instance();
	CStringW sFunc;
	auto idThrd = ::GetCurrentThreadId();
	bool bFunc = pDummy == nullptr;
	if (pFunc) {
		sFunc = pFunc;
		auto ifn = _funcNoOutput.find(pFunc);
		if (ifn != _funcNoOutput.end()) {
			if (ifn->second != eDump)
				return {};
		}
		//sFunc = L"CCenterView::StartServer::<lambda_0f42662d052511f3a0ffe00f70938767>::operator ()"
		auto il = sFunc.Find(L"<lambda_");
		if (il >= 0) {//ShortenLambdaName 참조. 동일 기능-여긴 CStringW, 함수는 string
			//sFunc = ShortenLambdaName(std::wstring(sFunc.GetString())).c_str();
			auto ilc = sFunc.Find(L"::", il);
			if (il < ilc) {
				CStringW tmp1 = sFunc.Left(il + 8);//sFunc.Mid(il, 8); //
				CStringW tmp2; tmp2.Format(L"%d", nLine);// = sFunc.Mid(il + 8, 4);// ilc - (il + 12));
				CStringW tmp3 = L">"; // L"...>";
				sFunc = tmp1 + tmp2 + tmp3;//sFunc = L"FMReportCaseInfo::AddDBPathName::<lambda_4f41>"
			}
		}
		else {
			auto lins = sFunc.Find(L"::InitInstance");
			if (lins > 0 || ktr->_idMainThread == 0) {
				ktr->_idMainThread = idThrd;
			}
		}
		DbgCString::ReplaceCString(sFunc);
	}

#pragma region 	//[ ---- 대표 함수 빠른 반복 처리 끝 ----
	// ---- 대표 함수(DWKFUNC) 빠른 반복 감지/요약 처리 ----
	// bFunc == true 인 경우가 함수 진입(DWKFUNC/DWKFUNCV) 이므로,
	// 이 호출을 "대표"로 보고 반복 여부를 체크한다.
	if (bFunc && !sFunc.IsEmpty())
	{
		ULONGLONG now = GetTickCount64();

		std::unique_lock<std::mutex> lk(ktr->_csRepeat);
		auto& info = ktr->_mapRepeat[idThrd]; // 없으면 default 생성

		bool isSameFunc = (info.funcName == sFunc);
		bool isLast = (info.lastTick != 0);
		bool isFast = ((now - info.lastTick) <= 5000ULL);
		bool isFastSeq = isSameFunc && isLast && isFast;
		const int nMaxRepeat_ = 10;
		if (isFastSeq) {
			// 같은 스레드, 같은 대표 함수가 5초 안에 다시 들어온 경우
			info.lastTick = now;
			info.count++;

			if (info.count > nMaxRepeat_) {
				// nMaxRepeat_번째 이상부터는 디테일 출력 대신 '.'만 찍는다.
				info.squashing = true;
				lk.unlock();
				// 너무 많이 찍히면 우측으로 길어지니, 100개마다 줄바꿈을 넣어 준다.
				if ((info.count - nMaxRepeat_) % 100 == 0)
					ktr->OutputDwkDebugString(L".\n", true);
				else
					ktr->OutputDwkDebugString(L".", true);
				return {};
			}
			// 1~nMaxRepeat_번째까지는 그냥 아래 원래 Trace 로직을 타게 둔다.
		}
		else {
			// 이전에 다른 함수이거나(또는 같은 함수라도 5초 넘게 쉬었다가) 들어온 경우
			// 직전 묶음이 "압축 모드" 였다면 요약 한 줄을 먼저 출력해 준다.
			if (!info.funcName.IsEmpty() && info.squashing && info.count > nMaxRepeat_) {
				CStringW sum;
				sum.Format(L"[FAST] %s : %d회 반복 (≤5초 간격)\n",
					info.funcName.GetString(), info.count);
				lk.unlock();
				ktr->OutputDwkDebugString(sum, true);
				lk.lock();
			}

			// 새 묶음 시작: 현재 함수를 대표 함수로 등록
			info.funcName = sFunc;
			info.lastTick = now;
			info.count = 1;
			info.squashing = false;
		}
		// info는 그대로 보존된 상태에서 아래 기존 Trace 포맷팅/출력 로직으로 진입
	}
#pragma endregion	//] ---- 대표 함수 빠른 반복 처리 끝 ----

	auto iDwk = sMsg.Find(L"dwk");
	CStringW fmt1;
	CStringW kTrace = iDwk >= 0 ? L"dwk" : L"info";
	fmt1.Format(L"%s(%d):%s-", sFile, nLine, (PWS)kTrace);
	//"{fullpath}(line):-" 딱 이 규칙만 지키면 더블클릭할 때 소스로 간다.
	int nStack = 0;
	{
		std::lock_guard<std::mutex> lck(ktr->_csMapStack);
		auto& mp = *GetMapStack();//ktr->_mapStack;///스레드별 스택 정보가 들어 있다.
		auto fl = mp.find(idThrd);
		if (fl != mp.end())
			nStack = fl->second; // (pFunc ? 0 : 1); // 일반 TRACE의 경우 한 번만으로
		//else{ // KStackDump::KStackDump 에서 한다.
		//	mp[idThrd] = 0;
		//	fl = mp.find(idThrd);
		//}
	}
	//ASSERT(nStack > 0);FOIUnknown::AddRef() 에서 0이다. 왜지?
	//CStringW sTab('\t', nStack - 1);
	wstringstream sTab;
	for (int i = 0; i < nStack + (bFunc ? 0 : 1); ++i)
		sTab << ((i % 3) == 0 ? L"  !" : (i % 3) == 1 ? L"  ." : L"  :");
	//sTab.Replace(L"\t", L"  :");
	auto btn = bFunc ? L"  +" : L"  -";

	LARGE_INTEGER frequency;
	LARGE_INTEGER qtik;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&qtik);
	double elapsed = _lastQTik.QuadPart == 0 ? 0. : static_cast<double>(qtik.QuadPart - _lastQTik.QuadPart) / frequency.QuadPart;

	SYSTEMTIME st;
	GetLocalTime(&st);
	CStringW sElp;
	sElp.Format(elapsed < 10 ? L"%5.3f" : elapsed < 100 ? L"%5.2f" : L"%5.1f", elapsed);
	bool bTime{false};
	bool bElapsed{false};
	CStringW stm; 
	if(bTime && bElapsed)
		stm.Format(L"%02d:%02d:%02d[%s]", st.wHour, st.wMinute, st.wSecond, sElp.GetString());

	//KTrace::wHd_ = 100;
	if (fmt1.GetLength() > KTrace::wHd_)//앞에 수직 폭은 자동으로 커진다.
	{
		auto mk = fmt1.GetLength() % 6;
		auto mk2 = 6 - mk;
		setWidth(fmt1.GetLength() + mk2);
	}
	CStringW ffmt;
	CStringW fmt2;
	int iThrd = 0;
	auto bFG = ktr->_idMainThread == idThrd;

	auto IsShowThread = [](DWORD idth) -> bool {
		auto itf = DwkOutputFilter::s_setFilterShowID.find(idth);//보이는 필터에 thread ID 검색
		return (itf != DwkOutputFilter::s_setFilterShowID.end());
		};

	//auto IsShowFunc = [](const CStringW& sFunc) -> bool {
	//	auto itf = s_setFilterShow.find(sFunc.GetString());
	//	return (itf != s_setFilterShow.end());
	//	};
	bool bOutput = true;/// bFG;//debug output 창 : initinstance 스레드면 show
	if (!bFG) {//백그라운드 경우만 체크
		bOutput = true;//일단 다 보이고 IsShowThread(idThrd);
		if (bOutput){/// 굳이 숨길 것만 숨겨: 너무 자주 뜨는거
			if(pFunc) {
				auto itf = DwkOutputFilter::s_setFilterHide.find(pFunc);//함수 이름 가져 와서 그걸로 hide 있나 검색?
				if (itf != DwkOutputFilter::s_setFilterHide.end())
					bOutput = false;
			}
		}
	}
	ffmt.Format(L"%%-%ds %%s %%5X:%%c%%s%%s", KTrace::wHd_);
	fmt2.Format((PWS)ffmt, fmt1, stm, idThrd, bFG ? L'F' : L'B', /*iThrd,*/ sTab.str().c_str(), btn);
	//fmt2.Format(L"%-100s %s %5X:%c%s%s", fmt1, stm, idThrd, bFG ? L'F' : L'B', sTab.str().c_str(), btn);

	CStringW fmt;
	if (bFunc)
		fmt.Format(L"%s %s # %s\n", fmt2.GetString(), sFunc.GetString(), sMsg.GetString());
	else
		fmt.Format(L"%s %s\n", fmt2.GetString(), sMsg.GetString());

	auto tik = GetTickCount64();
	auto elap = tik - _lastTik;
	if (elap > 500) {//dwk: 2025-07-31 10:20 0.5초 지난 것 찍는다.
		CStringW sl; sl.Format(L"%s --------- %.3f초--------------------------------------------------\n"
			, CStringW('+', 100).GetString(), _lastTik == 0 ? 0 : elap / 1'000.);
		OutputDwkDebugString(sl);//bOutput 적용 않마. ----는 항상 출력
	}

	_lastTik = tik;
	_lastQTik = qtik;
	frequency = frequency;
	OutputDwkDebugString(fmt, bOutput);
	return fmt;
}
inline void KTrace::OutputDwkDebugString(LPCWSTR str, bool bOutput/* = true*/, bool bLog/* = true*/)
{
	if (bOutput)
		OutputDebugStringW(str);
	else
		_break;
	if (bLog) {
		if (_cbTraceLog)
			_cbTraceLog(str);//dwk: 2025-05-15 16:12 이게 Release에서 하려면 DwkReleaseDump 를 프로젝트에 정의해줘야
	}
}

#else // _DWKTRACE_TRUE DWKSetThread
#define DWKSetThread(...) 
#define DWKShowThreadID()
//do { DwkSetThread(__FUNCTION__, ##__VA_ARGS__); } while(0)
#endif //_DWKTRACE_TRUE DWKSetThread
#pragma endregion	]KTRACE

//C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\atlmfc\include\afx.h
//inline CStringA UcClassHierarchy(CObject* th)
//{
//	CRuntimeClass* pClass = th->GetRuntimeClass();
//	//std::cout << "Class Hierarchy:" << std::endl;
//	CStringA scl;
//	while (pClass) {
//		scl += pClass->m_lpszClassName;
//		scl += " : ";
//		pClass = pClass->m_pBaseClass;//
//	}
//	return scl;
//}
//inline CStringA UcClassHierarchy(CObject* pObj)
//{
//	CStringA scl;
//#ifndef _AFXDLL
//	if (!pObj) return scl;
//
//	CRuntimeClass* pClass = pObj->GetRuntimeClass();
//	while (pClass) {
//		scl += pClass->m_lpszClassName;
//		pClass = pClass->m_pBaseClass;
//		if (pClass)
//			scl += " : ";
//	}
//#else
//	scl = typeid(*pObj).name(); // fallback: RTTI 이름 출력
//#endif
//	return scl;
//}
inline CStringA UcClassHierarchy(CObject * th)
{
	if (!th) return "(null)";
	CStringA scl;
	CRuntimeClass* pClass = th->GetRuntimeClass();

	while (pClass) {
		scl += pClass->m_lpszClassName;

#ifdef _AFXDLL
		if (pClass->m_pfnGetBaseClass)
			pClass = pClass->m_pfnGetBaseClass();
		else
			break;
#else
		pClass = pClass->m_pBaseClass;
#endif
		if (pClass) scl += " : ";
	}
	return scl;
}

#include <tuple>
#include <map>
class UcIdStrMap {
public:
	std::vector<std::map<int, std::tuple<LPCSTR, int, DWORD>>*> _arMapIds;

	void AddIdStrMap(std::map<int, std::tuple<LPCSTR, int, DWORD>>* pAppMapIds) {
		_arMapIds.push_back(pAppMapIds);
	}

	std::tuple<LPCSTR, int, DWORD>* GetIdStrInMap(std::map<int, std::tuple<LPCSTR, int, DWORD>>* pMapIds, int nID, int nCode = 0);

	std::tuple<LPCSTR, int, DWORD> GetIdStr(int nID, int nCode = 0);

	CStringA IdStr(int nID, int nCode = 0);
};

#ifdef _Sample
BOOL CHMIBuilderApp::InitInstance()
auto gidmap = GSingleton<UcIdStrMap>::GetInstance();
gidmap->AddIdStrMap(GSingleton<NvIdStrMap>::GetInstance()->GetIdMap());//dwk: 2025-03-26 09:32  
gidmap->AddIdStrMap(GetIdMap());//dwk: 2025-03-26 09:32  
}
void Usage()
{
#if CPP17_OR_LATER
	auto [sid, op, psd] = GSingleton<UcIdStrMap>::GetInstance()->GetIdStr(nID, nCode);
	//auto [sid, op, psd] = UcIdStrMap::GetIdStr(nID, nCode);
#else
	auto tuple_result = GSingleton<UcIdStrMap>::GetInstance()->GetIdStr(nID, nCode);
	auto sid = std::get<0>(tuple_result);
	auto op = std::get<1>(tuple_result);
	auto psd = std::get<2>(tuple_result);
	//auto tuple_result = UcIdStrMap::GetIdStr(nID, nCode);
	//auto sid = std::get<0>(tuple_result);
	//auto op = std::get<1>(tuple_result);
	//auto psd = std::get<2>(tuple_result);
#endif
	auto sid = GSingleton<UcIdStrMap>::GetInstance()->IdStr(nID);
}
#endif // _Sample



#include <sstream>
#define KTRACEOUTPUT

class CKTrace
{
public:
	CKTrace(bool bDebug = false)
		: _debug(bDebug)
	{
	}

	bool _debug;

	std::wstringstream _s;

	std::wstring str() {
		return _s.str();
	}

	std::shared_ptr<std::function<void(std::wstring)>> _fncTrace;

	void SetTrace(std::shared_ptr<std::function<void(std::wstring)>> fnc)//?ExTrace 5 CUcTrace::fnc 에 저장
	{
		ASSERT(!_fncTrace);
		_fncTrace = fnc;
	}

	void Output(const WCHAR* txt)
	{
		if (wcscmp(txt, L"\r\n") == 0) {
			if (_fncTrace) {
				if (!_debug)
					(*_fncTrace)(_s.str());//?ExTrace 6 Output에서 실행. 추가 출력은 줄바꿈을 안 넣고 내 보낸다.
			}
			_s << txt;
			OutputDebugStringW(_s.str().c_str());
			_s.str(L"");// = "";//.clear(); 이게 말을 안듣네.
		}
		else
			_s << txt;
	}

	CKTrace& operator<<(const WCHAR* ctr) {
		Output(ctr);
		return *this;
	}
	CKTrace& operator<<(const CHAR* ctr) {
		CStringW wstr(ctr);
		return operator<<(wstr);
	}
	CKTrace& operator<<(CHAR* ctr) {
		return operator<<((PAS)ctr);
	}
	CKTrace& operator<<(const std::string& ctr) {
		CStringW wstr(ctr.c_str());
		return operator<<(wstr);
	}
	CKTrace& operator<<(const std::wstring& wstr) {
		return operator<<(wstr.c_str());
	}

	CKTrace& operator<<(const std::stringstream& ctr) {
		CStringW wstr(ctr.str().c_str());
		return operator<<(wstr);
	}
	CKTrace& operator<<(const std::wstringstream& ctr) {
		return operator<<(ctr.str().c_str());
	}
	CKTrace& operator<<(const int ctr) {
		return operator<<(std::to_string(ctr));
	}
	CKTrace& operator<<(const DWORD ctr) {
		return operator<<(std::to_string(ctr));
	}
	CKTrace& operator<<(const long ctr) {
		return operator<<(std::to_string(ctr));
	}
	CKTrace& operator<<(const int64_t ctr) {
		return operator<<(std::to_string(ctr));
	}

	//template<typename TNUM>
	//CKTrace& operator<<(TNUM ctr){
	//	std::wstring str = std::to_string(ctr);
	//	Output(str);
	//	return *this;
	//}
	INLINE_STATIC std::shared_ptr<CKTrace> pstd_cout;
	static CKTrace& getInst() {
		if (!pstd_cout)
			pstd_cout = std::make_shared<CKTrace>();
		return *pstd_cout;
	}
};
#define std_endl "\r\n" //std::endl
#define ENDL "\r\n" //std::endl

//inline static CKTrace std_cout;
//extern CKTrace std_cout; 위에 정의 만으로 충분
#define KTRACESTREAM    CKTrace::getInst()
#define STREAM_OUT(tag) KTRACESTREAM << __FILE__ << "(" << std::to_string(__LINE__) << "):" #tag "- "
#define STCOUT          STREAM_OUT(STCOUT)
#define STCERR          STREAM_OUT(STCERR)			//error 때문에 출력창에 빨간색으로
#define STCEXCEP        STREAM_OUT(STCEXCEP)


class KNanoTik
{
public:
	LARGE_INTEGER t1{}, t2{}, freq{};
	double _elapsed;
	KNanoTik() {
		QueryPerformanceFrequency(&freq);  // 초당 카운트 수 (보통 수백만 단위)
		QueryPerformanceCounter(&t1);
	}
	~KNanoTik() {
		//double delap = GetElapsed();
	}
	/// <summary>
	/// _elapsed에 담기는 단위는 **밀리초 (milliseconds)**입니다.
	/// </summary>
	/// <returns></returns>
	double GetElapsed() {
		QueryPerformanceCounter(&t2);
		_elapsed = (double)(t2.QuadPart - t1.QuadPart) * 1000.0 / freq.QuadPart;
		return _elapsed;
	}
};

class KElapsed {
public:
	KElapsed(std::function<void(double)> fn) : _fn(fn) {}
	std::function<void(double)> _fn;
	KNanoTik _nt;
	~KElapsed() {
		if (_fn)
			_fn(_nt.GetElapsed());
	}
};

#define DWKELAPSED KElapsed elp__([](double elp) {DWKFUNCV(L"Elapesed: %.5v msec", elp);});

inline bool IsDwkang() {
	CString sUser;
	DWORD username_len = 1024;// UNLEN + 1;

	// 사용자 이름을 UPN(User Principal Name) 형식으로 가져오기
	//if (GetUserNameExW(NameSamCompatible, username, &username_len)) {
	bool bMe = false;
	if (GetEnvironmentVariable(_T("USERNAME"), sUser.GetBuffer(username_len), username_len)) {
		TRACE(_T("%s\n"), sUser);
		bMe = sUser == L"dwkang";
	}
	return bMe;
}

inline bool UcIsDebugExpired() {//dwk: 2025-08-20 13:35 
	return (CTime::GetCurrentTime() < CTime(2025, 8, 21, 18, 0, 0));
}
//dwk: 2025-12-08 10:01 
//dwk: 2025-12-10 13:10 UcJXBase.h 제거
//dwk: 2026-01-02 12:01 std::map<DWORD, int> _mapStack; -> GetMapStack()
//dwk: 2026-01-02 13:09 std::map<DWORD, std::stack<LPCWSTR>> _mapStkFunk; -> GetMapStackFunc()
