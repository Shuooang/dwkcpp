#include "pch.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include <array>
#include "UcTimeTools.h"
#include "UcThreadTool.h"

#include "UcTool.h"
#include "UcDebug.h"
//#pragma message(FILINDWK("주의: 반드시 DwkData::g_mapEnumStr 인스턴스를 넣어야 한다."))
//#pragma message("std::map<std::type_index, std::map<int, std::wstring>> DwkData::g_mapEnumStr;")

// C++14 호환성을 위한 EXTERN_STATIC 변수 정의

/// DLL 로  UcTool.dll로 된 경우, GetMapStatck() 함수가 export 가 자동으로 된다.
/// UcTool이 static lib일때는, 어디선가 GetMapStack() 함수가 정의되어야 한다. (예: App InitInstance가 있는 파일에 주로)
#ifdef UCTOOL_EXPORTS
_PUT_THIS_TO_DLL_OR_EXE_
#else
DWKREMINDER("_PUT_THIS_TO_DLL_OR_EXE_ : GetMapStack() to EXE or DLL Project.")
#endif // UCTOOL_EXPORTS

#pragma region thread tag mapping
#ifdef _DWKTRACE_TRUE
namespace {
//std::mutex g_ucThreadTagMtx;
//std::unordered_map<DWORD, std::wstring> g_ucThreadTagMap;
}
#endif

//void UcThreadTag_Register(DWORD threadId, LPCSTR tag)
//{
//#ifdef _DWKTRACE_TRUE
//	if (!tag || !tag[0])
//		return;
//	std::lock_guard<std::mutex> lk(KException::mtxTag_);
//	KException::mapTags_[threadId] = CStringW(tag).GetString();
//#else
//	(void)threadId;
//	(void)tag;
//#endif
//}
//
//void UcThreadTag_Unregister(DWORD threadId)
//{
//#ifdef _DWKTRACE_TRUE
//	std::lock_guard<std::mutex> lk(KException::mtxTag_);
//	KException::mapTags_.erase(threadId);
//#else
//	(void)threadId;
//#endif
//}

#ifdef _DWKTRACE_TRUE
CStringW UcThreadTag_FormatFixed(DWORD threadId)
{
	std::wstring raw;
	{
		std::lock_guard<std::mutex> lk(KException::mtxTag_);
		const auto it = KException::mapTags_.find(threadId);
		if (it != KException::mapTags_.end())
			raw = it->second;
	}
	CStringW sraw(raw.c_str());
	const int w = UcThreadTag_kTraceColWidth;
	if (sraw.GetLength() > w)
		sraw = sraw.Left(w);
	//return sraw;
	CStringW out;
	//out.Format(L"%-*s", w, sraw.GetString());
	// 오른쪽 정렬을 위해서 '-'(왼쪽 정렬) 대신 아무 플래그도 없는 %*s 포맷을 사용해야 하고, 
	// 태그는 반드시 스레드 번호 "앞"에, 고정폭(동적X, 항상 UcThreadTag_kTraceColWidth만큼)으로 출력해야 합니다.
	// 동적으로 하면 줄맞춤이 무너지므로, 아래와 같이 오른쪽 정렬(고정폭)로 바꿔야 한다.
	if(sraw.GetLength() > 0){
		out.Format(L"[%s]", sraw.GetString());
		return out;
	}
	return {};
}
#endif
#pragma endregion thread tag mapping
#if CPP17_OR_LATER
/// static 멤버 변수는 클래스 정의에서 선언만 하고, cpp 파일에서 정의해야 합니다. (초기화 포함)
//std::shared_ptr<KTrace> KTrace::instance_;
UCTOOLDYNAMIC int KTrace::wHd_ = 80;


//UCTOOLDYNAMIC
//std::shared_ptr<KTrace> KTrace_Instance(){
//	return GSingleton<KTrace>::GetInstance();
//}


//GlobalInstance(KTrace);

std::shared_ptr<KTrace> KTrace::Instance()
{
	//return GlobalInstance(KTrace);
	return GSingleton<KTrace>::GetInstance();
	// 
	//std::call_once(initFlag_, []() {//프로세스 전역이 아니라 “모듈 단위”
	//	instance_ = std::make_shared<KTrace>();
	//	});
	//return instance_;
}
//std::once_flag KTrace::initFlag_;

/// extern 변수는 헤더에 선언만 하고, cpp에서 정의해야 합니다. (초기화 포함)
UCTOOLDYNAMIC std::mutex mutexHandler_; //14

/// extern 변수 초기화
UCTOOLDYNAMIC std::unordered_map<std::type_index, std::function<void(std_any&, std::wstringstream&, const CStringW&, int&)>> dwk_handlers_ = {
#ifdef _DEBUG_tst
	{	typeid(int), [](std_any& operand, std::wstringstream& wss, const CStringW& format, int& pr) {
		auto ff = format.Mid(1);
		auto value1 = std_any_cast<int>(operand);
		if (ff.GetLength() >= 1)
			wss << std::setfill(ff.Left(1) == L"0" ? L'0' : L' ');
		wss << value1;
	}},
#else
	{ typeid(int), [](VAR_PARAMS) {auto ff = format.Mid(1); int value1 = std_any_cast<int>(operand); if (ff.GetLength() >= 1) wss << std::setfill(ff.Left(1) == L"0" ? L'0' : L' '); wss << value1;;	} },
#endif // _DEBUG
	{	typeid(short int), [](VAR_PARAMS) {PrimierVar_Arg(short int         , wss, operand, format);	}},
	{	typeid(short unsigned int), [](VAR_PARAMS) {PrimierVar_Arg(short unsigned int, wss, operand, format);	}},
	{	typeid(unsigned int), [](VAR_PARAMS) {PrimierVar_Arg(unsigned int      , wss, operand, format);	}},
	{	typeid(long), [](VAR_PARAMS) {PrimierVar_Arg(long              , wss, operand, format);	}},
	{	typeid(unsigned long), [](VAR_PARAMS) {PrimierVar_Arg(unsigned long     , wss, operand, format);	}},
	{	typeid(__int64), [](VAR_PARAMS) {PrimierVar_Arg(__int64           , wss, operand, format);	}},
	{	typeid(unsigned __int64), [](VAR_PARAMS) {PrimierVar_Arg(unsigned __int64  , wss, operand, format);	}},
	{	typeid(float), [](VAR_PARAMS) {PrimierRealArg(float             , wss, operand, format);	}},//format = L"%6.2v"		   
	{	typeid(double), [](VAR_PARAMS) {PrimierRealArg(double            , wss, operand, format);	}},
	{	typeid(std::wstring), [](VAR_PARAMS) { wss << std_any_cast<const std::wstring&>(operand); }},
	{	typeid(std::string), [](VAR_PARAMS) { wss << CStringW(std_any_cast<const std::string&>(operand).c_str()).GetString(); }},
	/// nullterminated 라고 보장한 경우 data()를 쑬수 있다.
	{	typeid(std::wstring_view), [](VAR_PARAMS) { wss << std_any_cast<const std::wstring_view&>(operand).data(); }},
	{	typeid(std::string_view), [](VAR_PARAMS) { wss << CStringW(std_any_cast<const std::string_view&>(operand).data()).GetString(); }},
	{	typeid(std::wstringstream), [](VAR_PARAMS) { ASSERT(0 == "DwktoAny error"); wss << std_any_cast<const std::wstringstream&>(operand).str(); }},
	{	typeid(std::stringstream), [](VAR_PARAMS) { ASSERT(0 == "DwktoAny error"); wss << CStringW(std_any_cast<const std::stringstream&>(operand).str().c_str()).GetString(); }},
	{	typeid(CStringW), [](VAR_PARAMS) { wss << std_any_cast<const CStringW&>(operand).GetString(); }},
	/// &참조를 쓸때는 반드시 const를 붙여야, 호출한 곳에서 const가 붙었더라도 여기서 붙일수 있다.
	{	typeid(CStringA), [](VAR_PARAMS) { wss << CStringW(std_any_cast<const CStringA&>(operand)).GetString(); }},
	{	typeid(const wchar_t*), [](VAR_PARAMS) {
		wss << std_any_cast<const wchar_t*>(operand); //NULL인 경우
	}},
	{	typeid(wchar_t*), [](VAR_PARAMS) { wss << (LPCWSTR)std_any_cast<wchar_t*>(operand); }},/// const 붙이고 cast하면 안된다. 
	{	typeid(const char*), [](VAR_PARAMS) { wss << CStringW((LPCSTR)std_any_cast<const char*>(operand)).GetString(); }},
	{	typeid(char*), [](VAR_PARAMS) { wss << CStringW((LPCSTR)std_any_cast<char*>(operand)).GetString(); }},
	{	typeid(bool), [](VAR_PARAMS) { wss << (std_any_cast<bool>(operand) ? L"true" : L"false"); }},//wss << (b ? L"1" : L"0");
	{	typeid(std::nullptr_t), [](VAR_PARAMS) { wss << L"(null)"; }},
	{	typeid(void*), [](VAR_PARAMS) { void* ptr = std_any_cast<void*>(operand);
		wss << L"0x" << std::hex << std::setw(sizeof(ptr) * 2) << std::setfill(L'0') << (uintptr_t)ptr;
	}},//0x00001234//wss << reinterpret_cast<int64_t>(ptr);
	{	typeid(std::tuple<LPCSTR, int>), [](VAR_PARAMS) {// 예전에 enum 사용자 문자열 정의때 쓰던 tuple타입을 임시로 생성 된다.
		auto [pType, iValue] = std_any_cast<std::tuple<LPCSTR, int>>(operand);
		//auto tuple_val = std_any_cast<std::tuple<LPCSTR, int>>(operand);
		//LPCSTR pType = std::get<0>(tuple_val);
		//int iValue = std::get<1>(tuple_val);
		std::wstring value;
		auto it = DWK_mapEnum.find(pType);// enum type인 경우만 사용
		if (it != DWK_mapEnum.end())
			value = it->second(iValue);
		else
		{// "enum IcEdSplinEditDragPointJig::PointType", 0
			value = DwkDefaultEnum(pType, (int)iValue);//value = L"enum `protected: virtual void __cdecl CUcView::OnInitialUpdate(void) __ptr64'::`2'::ENum(0)"
			size_t pos = value.rfind(':');
			if (pos != std::wstring::npos)
				value = value.substr(pos + 1);//value = L"ENum(0)"   L"<unnamed-enum-eTest1>(0)"
		}
		wss << value;//value = L"@@IcEdSplinEditDragPointJig::kFitPoint(0)"
	}},
	{	typeid(HANDLE), [](VAR_PARAMS) {
		auto ptr = std_any_cast<HANDLE>(operand);
		wss << L"0x" << std::hex << std::setw(sizeof(ptr) * 2) << std::setfill(L'0') << (uintptr_t)ptr;
	}},//0x00001234//wss << reinterpret_cast<int64_t>(ptr);
	{	typeid(HKEY), [](VAR_PARAMS) {
		auto ptr = std_any_cast<HKEY>(operand);
		wss << L"0x" << std::hex << std::setw(sizeof(ptr) * 2) << std::setfill(L'0') << (uintptr_t)ptr;
	}},//0x00001234//wss << reinterpret_cast<int64_t>(ptr);
};

#else//CPP_BEFORE_17

// KTrace 클래스의 INLINE_STATIC 멤버들 정의


// DbgCString 클래스의 INLINE_STATIC 멤버들 정의
std::string DbgCString::to_findW = "class ATL::CStringT<wchar_t,class StrTraitMFC<wchar_t,class ATL::ChTraitsCRT<wchar_t> > >";
std::string DbgCString::to_replaceW = "CStringW";
std::string DbgCString::to_findA = "class ATL::CStringT<char,class StrTraitMFC<char,class ATL::ChTraitsCRT<char> > >";
std::string DbgCString::to_replaceA = "CStringA";

LPCWSTR DbgCString::to_findWW = L"class ATL::CStringT<wchar_t,class StrTraitMFC<wchar_t,class ATL::ChTraitsCRT<wchar_t> > >";
LPCWSTR DbgCString::to_replaceWW = L"CStringW";
LPCWSTR DbgCString::to_findWA = L"class ATL::CStringT<char,class StrTraitMFC<char,class ATL::ChTraitsCRT<char> > >";
LPCWSTR DbgCString::to_replaceWA = L"CStringA";

//#ifdef _DWKTRACE_TRUE // DWKTRACE <= _DWKTRACE_ 를 프로젝트 셋팅에 정의해 줘야 한다.
// DWKSETTINGS 클래스의 INLINE_STATIC 멤버들 정의
std::map<std::string, int> DWKSETTINGS::g_map = {
	{"do", 1},
};


// DwkOutputFilter 클래스의 INLINE_STATIC 멤버들 정의
// 헤더 파일의 #else 블록(C++14 이하)에서 선언만 있으므로 여기서 정의
std::set<std::wstring> DwkOutputFilter::s_setFilterHide = {
	L"FMReportCallback::SendProgressMsg",
	L"FMReportCallback::SendLogMsg",
	L"FMReportCallback::SendNotifyMsg",
	//L"FMReportEvidenceInfoBase::ThreadProc_CreateIndividualImageExcel",
	//L"FMFReportEvidenceDeailedList_FMF_SPO::ThreadProc_ExcelReport",
	//L"FMFReportEvidenceDeailedList_FMF_SPO::ThreadProc_DBReport",
};

std::map<unsigned long, std::wstring> DwkOutputFilter::s_setFilterShowID = {
	{0, L""},
};

std::shared_ptr<CKTrace> CKTrace::pstd_cout;

std::map<std::string, std::function<std::wstring(int)>> DWK_mapEnum = {};


#endif //_DWKTRACE_TRUE

//#endif//_DWKTRACE_TRUE

#ifdef _UseDWK2CPP__
std::wstring DWK__anyToStringEx(const std_any& operand, const CStringW& format)
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

#pragma region	[type mapping
	static std::unordered_map<std::type_index, std::function<void(std_any&, std::wstringstream&, const CStringW&, int&)>> handlers_ = {
#ifdef _DEBUG_tst
		{	typeid(int), [](std_any& operand, std::wstringstream& wss, const CStringW& format, int& pr) {
			auto ff = format.Mid(1);
			auto value1 = std_any_cast<int>(operand);
			if (ff.GetLength() >= 1)
				wss << std::setfill(ff.Left(1) == L"0" ? L'0' : L' ');
			wss << value1;
		}},
#else
#endif // _DEBUG
		
		{ typeid(int    ), [] (VAR_PARAMS) { PrimierVar_Arg(int    , wss, operand, format); }},
		{ typeid(INT16  ), [] (VAR_PARAMS) { PrimierVar_Arg(INT16  , wss, operand, format); }},
		{ typeid(UINT16 ), [] (VAR_PARAMS) { PrimierVar_Arg(UINT16 , wss, operand, format); }},
		{ typeid(UINT32 ), [] (VAR_PARAMS) { PrimierVar_Arg(UINT32 , wss, operand, format); }},
		{ typeid(long   ), [] (VAR_PARAMS) { PrimierVar_Arg(long   , wss, operand, format); }},
		{ typeid(ULONG  ), [] (VAR_PARAMS) { PrimierVar_Arg(ULONG  , wss, operand, format); }},
		{ typeid(__int64), [] (VAR_PARAMS) { PrimierVar_Arg(__int64, wss, operand, format); }},
		{ typeid(UINT64 ), [] (VAR_PARAMS) { PrimierVar_Arg(UINT64 , wss, operand, format); }},
		{ typeid(float  ), [] (VAR_PARAMS) { PrimierRealArg(float  , wss, operand, format); }}, //format = L"%6.2v"
		{ typeid(double ), [] (VAR_PARAMS) { PrimierRealArg(double , wss, operand, format); }},
		{ typeid(std::wstring      ), [] (VAR_PARAMS) { wss <<          std_any_cast<const std::wstring       &>(operand);}},
		{ typeid(std::wstring_view ), [] (VAR_PARAMS) { wss <<          std_any_cast<const std::wstring_view  &>(operand).data();}},
		{ typeid(std::wstringstream), [] (VAR_PARAMS) { wss <<          std_any_cast<const std::wstringstream &>(operand).str();}},
		{ typeid(CStringW          ), [] (VAR_PARAMS) { wss <<          std_any_cast<const CStringW           &>(operand).GetString();}},
		{ typeid(std::string       ), [] (VAR_PARAMS) { wss << CStringW(std_any_cast<const std::string        &>(operand).c_str()).GetString();}},
		{ typeid(std::string_view  ), [] (VAR_PARAMS) { wss << CStringW(std_any_cast<const std::string_view   &>(operand).data()).GetString();}},
		{ typeid(std::stringstream ), [] (VAR_PARAMS) { wss << CStringW(std_any_cast<const std::stringstream  &>(operand).str().c_str()).GetString();}},
		{ typeid(CStringA          ), [] (VAR_PARAMS) { wss << CStringW(std_any_cast<const CStringA           &>(operand)).GetString();}},
		/// nullterminated 라고 보장한 경우 data()를 쑬수 있다.
		/// &참조를 쓸때는 반드시 const를 붙여야, 호출한 곳에서 const가 붙었더라도 여기서 붙일수 있다.
//#define NULLOPERAND(TT) (operand.has_value() ? std_any_cast<TT>(operand) : L"(null)")
//#define NULLOPERAND_(TT) (operand.has_value() ? std_any_cast<TT>(operand) : L"")
		{	typeid(const wchar_t*), [](VAR_PARAMS) {
			ASSERT(operand.has_value());//널이어도 true
			auto pstr = std_any_cast<const wchar_t*>(operand);
			if (pstr)
				wss << pstr;// std_any_cast<const wchar_t*>(operand);
			else
				wss << L"(null)";
	}},
{	typeid(wchar_t*), [](VAR_PARAMS) { //wss <<
				auto pstr = std_any_cast<wchar_t*>(operand);
				if (pstr)
					wss << (LPCWSTR)pstr;
				else
					wss << L"(null)";
			}},/// const 붙이고 cast하면 안된다. 
			{	typeid(const char*), [](VAR_PARAMS) { wss <<
				CStringW((LPCSTR)std_any_cast<const char*>(operand)).GetString();
}},
{	typeid(char*), [](VAR_PARAMS) { wss <<
	CStringW((LPCSTR)std_any_cast<char*>(operand)).GetString();
}},
{	typeid(bool), [](VAR_PARAMS) { wss << (std_any_cast<bool>(operand) ? L"true" : L"false"); }},//wss << (b ? L"1" : L"0");
{	typeid(std::nullptr_t), [](VAR_PARAMS) { wss << L"(nullptr)"; }},
{	typeid(void*), [](VAR_PARAMS) { void* ptr = std_any_cast<void*>(operand);
	wss << L"0x" << std::hex << std::setw(sizeof(ptr) * 2) << std::setfill(L'0') << (uintptr_t)ptr;
}},//0x00001234//wss << reinterpret_cast<int64_t>(ptr);
{	typeid(std::tuple<LPCSTR, int>), [](VAR_PARAMS) {// 예전에 enum 사용자 문자열 정의때 쓰던 tuple타입을 임시로 생성 된다.
#if CPP17_OR_LATER
			auto [pType, iValue] = std_any_cast<std::tuple<LPCSTR, int>>(operand);
#else
			auto tuple_val = std_any_cast<std::tuple<LPCSTR, int>>(operand);
			LPCSTR pType = std::get<0>(tuple_val);
			int iValue = std::get<1>(tuple_val);
#endif
			std::wstring value;
			auto it = DWK_mapEnum.find(pType);// enum type인 경우만 사용
			if (it != DWK_mapEnum.end())
				value = it->second(iValue);
			else
			{// "enum IcEdSplinEditDragPointJig::PointType", 0
				value = DwkDefaultEnum(pType, (int)iValue);//value = L"enum `protected: virtual void __cdecl CUcView::OnInitialUpdate(void) __ptr64'::`2'::ENum(0)"
				size_t pos = value.rfind(':');
				if (pos != std::wstring::npos)
					value = value.substr(pos + 1);//value = L"ENum(0)"   L"<unnamed-enum-eTest1>(0)"
			}
			wss << value;//value = L"@@IcEdSplinEditDragPointJig::kFitPoint(0)"
		}},
		{	typeid(HANDLE), [](VAR_PARAMS) {
			auto ptr = std_any_cast<HANDLE>(operand);
			wss << L"HANDLE" << std::hex << std::setw(sizeof(ptr) * 2) << std::setfill(L'0') << (uintptr_t)ptr;
		}},//0x00001234//wss << reinterpret_cast<int64_t>(ptr);
		{	typeid(HKEY), [](VAR_PARAMS) {
			auto ptr = std_any_cast<HKEY>(operand);
			wss << L"HKEY:" << std::hex << std::setw(sizeof(ptr) * 2) << std::setfill(L'0') << (uintptr_t)ptr;
		}},//0x00001234//wss << reinterpret_cast<int64_t>(ptr);
	};
#pragma endregion	type mapping]

	std::lock_guard<std::mutex> lock(mutexHandler_);

	auto it = handlers_.find(operand.type());
	if (it != handlers_.end())
		it->second((std_any&)operand, wss, format, pr);
	else {
		bool bDone = false;
		if (!bDone)
		{
			//const char* ptp(tp.name());//ptp = "struct HKEY__ * __ptr64"
			auto ptp = UcShortType(tp);

			ASSERT(strncmp(ptp, "enum", 4) == 0);//swtp.Left(4) == L"enum")
			// 이름없는 enum은 이미 처리 되었고, 이름 있는 경우는 tuple로 래핑 해서 여기 올리가 없다.
			OutputDebugStringA(tp.name());
			OutputDebugStringA(" @@@@@@@@@@@@@@ format type DWK__anyToStringEx @@@@@@@@@@@@@@@\r\n");
			wss << L"<type:" << (PWS)CStringW(ptp) << L">";
		}
	}
	return wss.str();
}
#endif // _UseDWK2CPP__


#ifdef __MOVE_TO_HD_
#include <ImageHlp.h>
#pragma comment(lib, "Dbghelp.lib")

// stack 2개 함수를 받아 온다.
// 이 함수와 KwPrintStack를 부른 함수는 자동 제외
CStringW UcPrintStack(int ncall/* = 2*/, PWS sDelimter/* = L" << "*/)
{
	unsigned int   i{};
	void* stack[100];
	unsigned short frames{};
	SYMBOL_INFO* symbol{};
	CStringW stk;
	//static CKCriticalSection cs_;
	HANDLE  process{};
	static BOOL s_bInit{ FALSE };
	if (!s_bInit)
	{
		process = GetCurrentProcess();//SymCleanup(process);를 아래에 매번 해주는거 봐봐.
		s_bInit = SymInitialize(process, NULL, TRUE);
	}
	if (process == nullptr)
		return {};
	if (!s_bInit)
	{
		_break;
	}
	if (s_bInit)
	{
		//UcAUTOLOCK(cs_);
		frames = CaptureStackBackTrace(0, 100, stack, NULL);
		ASSERT(frames > 2);
		symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 1024 * sizeof(char), 1);
		KAtEnd _free_symbol([&symbol]() {		free(symbol);		});
		if (symbol == nullptr)
			return {};
		symbol->MaxNameLen = 1024;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		int ncall2 = (ncall + 2); // 이함수와 콜러 제외
		wstringstream ss;
		int nfnc = 0;
		for (i = 0; i < frames; i++)
		{
			BOOL bCopy = SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
			//TRACE("%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
			if (i >= 2) //skip 2: excluding this and caller function
			{
				if (nfnc < ncall2) // 2 ~ 4 까지 2(ncall)개 "fnc1 << fnc2 <<" 형식으로 
				{
					if (tchlen(symbol->Name) > 0)
					{
						ss << symbol->Name << sDelimter;
						stk += CStringW(symbol->Name);
						stk += sDelimter;//L" << ";
						nfnc++;
					}
				}
				else
					break;
			}
		}
		stk = ss.str().c_str();
	}
	SymCleanup(process);// 이거 해도 SymInitialize 를 한번만 해도 되는 군.
	return stk;
}
#endif // __MOVE_TO_HD_

#define IDTOSTR_1(id) {id, #id}

CStringW UcWaitStr(UINT nId)
{
	static std::map<UINT, std::string> s_mapWait =
	{
		IDTOSTR_1(WAIT_OBJECT_0),
		IDTOSTR_1(WAIT_TIMEOUT),
		IDTOSTR_1(WAIT_ABANDONED),
		IDTOSTR_1(WAIT_FAILED),
		IDTOSTR_1(WAIT_IO_COMPLETION),
	};
	auto itis = s_mapWait.find(nId);
	CStringW sid = itis != s_mapWait.end() ? CStringW(itis->second.c_str()) : CStringW(L"Unknown");
	CStringW sw; sw.Format(L"%s(%u)", sid.GetString(), nId);
	return sw;
}
#undef IDTOSTR_1


std::tuple<LPCSTR, int, ULONGLONG>* UcIdStrMap::GetIdStrInMap(std::map<int, std::tuple<LPCSTR, int, ULONGLONG>>* pMapIds, int id, int nCode)
{
	auto it = pMapIds->find(id);
	if (it != pMapIds->end()) {
		return &it->second;
	}
	return nullptr;
}

std::tuple<LPCSTR, int, ULONGLONG> UcIdStrMap::GetIdStr(int id, int nCode)
{
	if (id == -1)
		return {};
	//if (nCode == CN_COMMAND)
	if (nCode == CN_UPDATE_COMMAND_UI)
		return {};
	//if (id == ID_FO_DRAW_RECTANGLE)
	//	_break;
	ASSERT(_arMapIds.size() > 0);
	for (auto* pMap : _arMapIds)
	{
		auto pTp = GetIdStrInMap(pMap, id, nCode);//FORes_KOKR.rc에 있는지 찾아본다.
		if (pTp)
		{
#if CPP17_OR_LATER
			auto& [sid, op, tk] = *pTp;
#else
			auto& tuple_ref = *pTp;
			auto& sid = std::get<0>(tuple_ref);
			auto& op = std::get<1>(tuple_ref);
			auto& tk = std::get<2>(tuple_ref);
#endif
			auto tk0 = tk;
			auto tk1 = GetTickCount64();
			tk = tk1;///dwk: 2025-03-26 09:50 원본 바뀜

			auto psd = tk1 - tk0;
			if (op == 0)
				return make_tuple(sid, op, psd);// {};
			//if (psd < 3'000)//이전 실행과 3초 이상 지난 거만
			//	return {};이거는 부른 쪽에서 판단 한다.
			//sa.Format("%s(%d)", sid, id);
			return make_tuple(sid, op, psd);///dwk: 2025-03-26 09:50 tk를 넘기지 않고 이전 시각과 현재 시각 차이 psd를 넘긴다.
		}
	}
	return {};// id가 없으면 빈 tuple을 리턴한다.
}

CStringA UcIdStrMap::IdStr(int nID, int nCode)
{
#if CPP17_OR_LATER
	auto [sid, op, psd] = GetIdStr(nID, nCode);
#else
	auto tuple_result = GetIdStr(nID, nCode);
	auto sid = std::get<0>(tuple_result);
	auto op = std::get<1>(tuple_result);
	auto psd = std::get<2>(tuple_result);
#endif
	if (sid)
		return CStringA(sid);
	return {};
}

//#ifdef _DWKTRACE_TRUE //(defined(_DEBUG) || defined(DwkReleaseDump)) && (!defined(NoDwkTrace))
//#include "UcTool.h"
/// <summary>
///  DWK 계열 출력을 출력창에만 찍는게 아니라  파일에 쓰고 싶으면 
/// _cbTraceLog 에 함수를 넣어 주면 된다.
/// 거기에 파일명과 문자열만 주면 써주는 범용 함수 이다.
/// </summary>
/// <param name="sFile"></param>
/// <param name="str"></param>
/// 예:
/// CString sFile = LR"(c:\temp\log\dwk.log)";
/// auto ktr = KTrace::Instance();
/// ktr->_cbTraceLog = [ktr, sFile](LPCWSTR sLog){
///	ktr->GeneralLog(sFile, sLog);
/// };
/// 
void KTrace::GeneralLog(CString sFile, LPCWSTR str)
{
	//static std::mutex s_mtx;
//std::lock_guard<std::mutex> lock(s_mtx);

///괜히 죽어서 일단 막음, FMReport만 리빌드 하고 아래 제거 하니 된다. 어떤거 때문에 된거야? 그럼 아래 다시 살려 볼까?
//CString mutexName = _T("Global\\FileMutex_") + sFile;
//CString mutexName = _T("FileMutex_") + sFile;
//CMutex fileMutex(FALSE, mutexName);
//CSingleLock lock2(&fileMutex, TRUE);  // TRUE = 무한 대기 //// Mutex 획득 여기서 무조건 리소스 못 구한다고 죽는다.
	UcMutexForProfile mtx(sFile);
	//try {
	//}
	//catch (...) {
	//	TRACE(L"CSingleLock error : %s [%s]", sFile.GetString(), str);
	//}

	UcCutFileToHalf(sFile);//파일이 무한정 커지는거 방지 하기 위해 자른다.

	int mode = CFile::modeWrite | CFile::typeBinary;
	static bool s_bFolder = false;
	if (!UcIfFileExistEx(sFile)) {
		mode |= CFile::modeCreate;
		if (!s_bFolder) {
			UcCheckTargetDir(sFile, 1, 0);
			s_bFolder = true;
		}
	}
	// CreateFile로 HANDLE 생성
	HANDLE hFile = CreateFile(sFile, GENERIC_WRITE,
		FILE_SHARE_READ,  // 다른 프로세스가 읽기 허용
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
	);
	if (hFile == INVALID_HANDLE_VALUE) {
		CStringW sbuf(UcGetErrorMsg(GetLastError()));
		TRACE(L"fail to CreateFile() : %s - %s [%s]\n", sbuf.GetString(), sFile.GetString(), str);
		return;
	}

	// HANDLE을 CFile로 변환
	//CFile file(hFile);
	//file.Attach(hFile);
	CFile flog(hFile);
	try {
		//if (flog.Open(sFile, mode))
		{
			if (mode & CFile::modeCreate)
				flog.Write(&s_wfeff, sizeof(s_wfeff));//s, s.GetLength());
			else
				flog.SeekToEnd();
			CString stm = UcGetCurrentTimeString();
			stm += _T(" ");
			flog.Write(stm, stm.GetLength() * sizeof(WCHAR));
			flog.Write(str, tchlen(str) * sizeof(WCHAR));
			//flog.Write(L"\r\n", sizeof(WCHAR) * 2);
			flog.Close();
		}
		//else {
		//	CStringW sbuf(UcGetErrorMsg(GetLastError()));
		//	TRACE(L"fail to CFile::Open () : %s - %s [%s]\n", sbuf.GetString(), sFile.GetString(), str);
		//}
	}
	catch (CFileException* e) {
		CString sbuf;
		e->GetErrorMessage(sbuf.GetBuffer(1024), 1024);
		sbuf.ReleaseBuffer();
		TRACE(L"flog CFileException : %s - %s [%s]\n", sbuf.GetString(), sFile.GetString(), str);
	}
	catch (CException*) {
		CStringW sbuf(UcGetErrorMsg(GetLastError()));
		TRACE(L"flog CException : %s - %s [%s]\n", sbuf.GetString(), sFile.GetString(), str);
	}
	catch (...) {
		TRACE(L"flog error : %s [%s]", sFile.GetString(), str);
	}
}
//#endif //_DWKTRACE_TRUE


//#include <iostream>
/// <summary>
/// 빌드한지 100일 지났나? 특정 기간만 보안 풀고 하는 코드를 넣을 때 사용
/// </summary>
/// <param name="daysThreshold"></param>
/// <returns></returns>
bool UcIsBuildOverdue(int daysThreshold)// = 100
{
	// __DATE__ = "Jul  1 2025"
	// __TIME__ = "10:42:35"
	std::string buildDateStr = std::string(__DATE__) + " " + std::string(__TIME__);

	// strptime이 없으므로 직접 처리
	std::tm tmBuild = {};
	std::istringstream ss(buildDateStr);
	ss >> std::get_time(&tmBuild, "%b %d %Y %H:%M:%S");

	if (ss.fail()) {
		//std::cerr << "Failed to parse build date/time\n";
		return false;
	}

	std::time_t tBuild = std::mktime(&tmBuild);
	std::time_t tNow = std::time(nullptr);

	double daysDiff = std::difftime(tNow, tBuild) / (60 * 60 * 24);
	return daysDiff > daysThreshold;
}

bool UcEHaTest()
{
	DWKFUNC;
	char buf[11]{};
	int ari[11]{};

	char* pbuf = buf;
	char* pbuf2 = new char[12];
	int ln = __LINE__;
	CStringW sw; CStringA sa;
	//__try {
	try {
		ln = __LINE__;
		pbuf[10] = 100;//에러안남
		ln = __LINE__;
		pbuf2[11] = 'x';// 여기서 안나지만, delete할때 죽기 때문에 어디서 잘못한지 알수 없다.
		ln = __LINE__;
		delete pbuf2;//주요: 윗줄 범위 넘어간 침범 있을 경우 access violation 나지만, 무시 하면 넘어간다.
		ln = __LINE__;
		pbuf2[10] = 'x';// __except  catch (...)에 잡힌다.
		ln = __LINE__;
		/// UcTool이 /EHsc 이고 /EHa 가 아니면 여기서 안잡히고 이걸 호출한 쪽에서 잡힌다.
		/// 따라서 lib 쪽에서 가급적 `/EHa + catch(...)` 를 하되, lib쪽이 /EHa를 못하면 호출한 쪽에서 라도 잡아야 한다.
		/// 
		// pbuf[10] = 100;	❌ 없음	스택 overflow, 운 좋으면 그냥 넘어감
		//pbuf2[11] = 'x';	❌ 없음	heap overflow지만, 인접 메모리가 사용 중 아님
		//delete pbuf2; 후 pbuf2[10] = 'x';	✅ 발생 가능	해제된 영역 접근(use - after - free), 시스템이 막음
	}
	//__except (EXCEPTION_EXECUTE_HANDLER) {
	catch (...) {// /EHa 이어야만 잡힌다. 만약 /EHsc로 하여 안집힌다면 호출한쪽에서 /EHa 하고 catch(...)로 잡아야 한다.
		void* stack[1];
		USHORT frames = RtlCaptureStackBackTrace(0, 1, stack, NULL);

		DWORD64 addr = (DWORD64)stack[0];

		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);

		DWORD displacement;
		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(line);
		if (SymGetLineFromAddr64(process, addr, &displacement, &line)) {
			TRACE("at %s:%lu\n", line.FileName, line.LineNumber);// line.LineNumber 는 RtlCaptureStackBackTrace 위치이다. 예외 발생한 위치가 아니다.
		}
		//AfxMessageBox(L"SEH 예외 발생 (예: Access Violation)");
		sw.Format(L"UcEHScTest2 알 수 없는 예외 발생: %d", ln);
		//AfxMessageBox(s);
		sa = CStringA(sw);
		throw std::exception((PAS)sa);// "UcEHScTest2");
	}
	return true;
}

bool UcEHScTest()
{
	DWKFUNC;
	// 오류 발생
	char buf[11]{};
	int ari[11]{};

	char* pbuf = buf;
	try
	{
		//pbuf[10] = 100; exe:/EHa, lib:/EHsc : 안죽는다. 심지어 조용히 지나 간다.

		char* pbuf2 = new char[12];
		char* pbuf3 = new char[12];
		pbuf2[11] = 100;//exe:/EHa, lib:/EHsc : 안죽는다. 심지어 조용히 지나 간다.
		delete pbuf2;//주요: 윗줄 범위 넘어간 침범 있을 경우  delete 할때 죽는다.
		pbuf2[10] = 'x';//
		//*(nullptr) = 100;

		std::array<int, 5> arr = { 1, 2, 3, 4, 5 };
		// 메모리 침범 (오버플로우)
		for (int i = 0; i < 10; i++) {
			arr.at(i) = i * 10;  // 위험한 접근!
		}
	}
	catch (const std::exception& e) {
		AfxMessageBox(CString(_T("예외 발생: ")) + CString(e.what()));
		TRACE("%s %s\n", __FUNCTION__, e.what());
		throw e;
	}
	catch (...) {
		AfxMessageBox(_T("UcEHScTest 알 수 없는 예외 발생"));
	}

	return true;
}

//dwk: 2025-12-08 15:05 
//dwk: 2025-12-08 10:01 
//dwk: 2025-12-10 13:10 UcJXBase.h 제거
