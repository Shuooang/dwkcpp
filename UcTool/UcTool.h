#pragma once
#include <string>
#include <set>
#include <shlobj.h>//REFKNOWNFOLDERID
#include <atlbase.h>//CRegKey

#include "UcBaseTools.h"
#if CPP17_OR_LATER //UcBaseTools.h에 선언 되어 있슴.
#include <optional>
#endif
#include "UcDebug.h"
#include <type_traits>


extern const WORD s_wfeff;// = 0xfeff;


//C:\srce\cad4\IntelliCAD\Source\CADian\UpdateCourier\UpdateCourierCheck\HiddenWindow.cpp(1185) : atlTraceGeneral - xxxx
/// ToStrSTEPNUM: 스텝 번호와 함께 디버그 문자열을 생성 (출력X, 문자열만 생성)
/// 형식: "파일(라인) : RTRACE - >>> 스텝번호.스레드타입 함수명(라인) : 메시지"
/// 스레드타입: F=Frontend(메인), B=Background, N=None
#define ToStrSTEPNUM(str, num, fmt, ...) {CStringW xtr2 = UcShortLambdaName(__FUNCTIONW__);\
	CStringW xtr3;xtr3.Format(L"%s(%d) : RTRACE - >>> %4d.%c %s(%d) : " fmt L"\n", __FILEW__, __LINE__, ++num, \
		m_hWnd ? ::GetWindowThreadProcessId(m_hWnd, NULL) == ::GetCurrentThreadId() ? 'F' : 'B' : 'N', xtr2.GetString(), __LINE__, ##__VA_ARGS__);\
		str = xtr3;}

/// ToStrNoSTEP: 스텝 번호 없이 디버그 문자열을 생성 (출력X, 문자열만 생성)
/// 형식: "파일(라인) : RTRACE - [스레드타입]        함수명(라인) : 메시지"
/// 스레드타입: F=Frontend(메인), B=Background, N=None
#define ToStrNoSTEP(str, fmt, ...) {CStringW xtr2 = UcShortLambdaName(__FUNCTIONW__);\
	CStringW xtr3;xtr3.Format(L"%s(%d) : RTRACE - [%c]        %s(%d) : " fmt L"\n", __FILEW__, __LINE__, \
		m_hWnd ? ::GetWindowThreadProcessId(m_hWnd, NULL) == ::GetCurrentThreadId() ? 'F' : 'B' : 'N', xtr2.GetString(), __LINE__, ##__VA_ARGS__);\
		str = xtr3;}


/// RTRACE: 파일(라인) + function name까지 출력한다.
#define RTRACE(fmt, ...) UcTrace(__FILEW__, __LINE__, L"RTRACE", L"%s # " fmt, UcShortLambdaName(__FUNCTIONW__).GetString(), ##__VA_ARGS__)






#define ITOABUFSIZE 128



#define KNVALTOSTRT(FNCNTOS2, TChr, cfnc, NTYPE)	\
inline TCString<TChr> FNCNTOS2(TCString<TChr>& sbuf, NTYPE lv, const TChr* fmt = NULL)	{	\
	if(fmt) sbuf.Format(fmt, lv);	\
	else { auto* buf = sbuf.GetBuffer(ITOABUFSIZE); \
		cfnc(lv, buf, ITOABUFSIZE, 10);	\
		sbuf.ReleaseBuffer();	\
	}	return sbuf;	\
}\
inline TCString<TChr> FNCNTOS2(NTYPE lv, const TChr* fmt = NULL)	{\
	TCString<TChr> sbuf;	\
	return FNCNTOS2(sbuf, lv, fmt);\
}

/// int, long vs CStringW, CStringA 4가지 교차 해서 만든다.
/// ex: CStringW sValue = KItoW(iValue);
KNVALTOSTRT(KLtoW, wchar_t, _ltow_s, long)
KNVALTOSTRT(KItoW, wchar_t, _itow_s, int)
KNVALTOSTRT(KLLtoW, wchar_t, _i64tow_s, INT64)
KNVALTOSTRT(KLtoA, char, _ltoa_s, long)
KNVALTOSTRT(KItoA, char, _itoa_s, int)
KNVALTOSTRT(KLLtoA, char, _i64toa_s, INT64)

/// 아래는 문자를 숫자로 바꾸는 거다. 
/// 숫자를 문자로 바꾸는 것은 문자버퍼로 복잡하여 kw_tool.h 에 있다.
template< class Tchar >
inline __int64 UcAtoi64(const Tchar* sdv)
{
	Tchar p[127] = { '\0', };
	tchcpynum(p, sdv);
	if (sizeof(Tchar) == 1)
		return _atoi64(reinterpret_cast<char*>(p));
	else
		return _wtoi64(reinterpret_cast<wchar_t*>(p));
	//	return _ttoi64(sdv);
}
template< class Tchar >
inline int UcAtoi(const Tchar* sdv)
{
	Tchar p[127] = { '\0', };
	tchcpynum(p, sdv);
	if (sizeof(Tchar) == 1)
		return atoi(reinterpret_cast<char*>(p));
	else
		return _wtoi(reinterpret_cast<wchar_t*>(p));
}
template< class Tchar >
inline int UcAtoU(const Tchar* sdv)
{
	Tchar p[127] = { '\0', };
	tchcpynum(p, sdv);
	if (sizeof(Tchar) == 1)
		return static_cast<unsigned int>(std::stoul(p));
	//return atoi(reinterpret_cast<char*>(p));
	else
		return static_cast<unsigned int>(wcstoul(p, nullptr, 10));
	//return _wtoi(reinterpret_cast<wchar_t*>(p));
}

template< class Tchar >
inline double UcAtof(const Tchar* sdv)
{
	Tchar p[127] = { '\0', };
	tchcpynum(p, sdv);
	if (sizeof(Tchar) == 1)
		return atof(reinterpret_cast<char*>(p));
	else
		return _wtof(reinterpret_cast<wchar_t*>(p));
}

#ifdef _Samples__
void Samples()
{
	auto sl = KLtoW2(1234, L"(%ld)");
	CStringW sbuf;
	auto& sl2 = KLtoW2(sbuf, 12346, L"[%ld])");
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	char buf[100];
	int decimal, sign;//
	_fcvt_s(buf, -123.432101234, 5, &decimal, &sign);// 소수 아래 5자리까지 나온다.
	// +		buf	0x006fec1c "123432"	char[100] 숫자만 나온다. '-'와 '.'는 빠진다. 이래서 사람들이 안쓰는 군.
	// 		decimal	3	int
	// 		sign	0	int 0:+, 1:-
	TRACE(L"%s\n", sl);
}
#endif // _Samples__

/// UINT64 문자열 변환함수는 MS에 없으므로 std:: 를 사용한다.
inline CStringW KULLtoW(CStringW& sbuf, UINT64 lv, LPCWSTR fmt = NULL)
{
	if (fmt)
		sbuf.Format(fmt, lv);
	else
		sbuf = std::to_wstring(lv).c_str();
	return sbuf;
}
inline CStringW KULLtoW(UINT64 lv, LPCWSTR fmt = NULL)
{
	CStringW sbuf;// = KwAutoBuf(CStringW(), (LPCWSTR)NULL);
	return KULLtoW(sbuf, lv, fmt);
}
inline CStringA KULLtoA(CStringA& sbuf, UINT64 lv, LPCSTR fmt = NULL)
{
	if (fmt)
		sbuf.Format(fmt, lv);
	else
		sbuf = std::to_string(lv).c_str();//여기가 위 Wide 와 다르다
	return sbuf;
}
inline CStringA KULLtoA(UINT64 lv, LPCSTR fmt = NULL)
{
	CStringA sbuf;// = KwAutoBuf(CStringA(), (LPCSTR)NULL);
	return KULLtoA(sbuf, lv, fmt);
}

//class KVal
//{
//public:
//	VType t{ eNul }; //입력 당시의 type
//
//	/// string type은 wchar만 지원 char인 경우믄 변형해야 한다.
//	std::wstring s;
//
//	union Nval {
//		Nval() {}
//		/// signed
//		Nval(int v): i((INT64)v) {}
//		Nval(INT64 v): i(v) {}
//		/// unsigned
//		Nval(UINT v): u((UINT64)v) {}
//		Nval(UINT64 v): u(v) {}
//		/// float
//		Nval(double v): d(v) {}
//
//		/// 3가지 type 8byte
//		INT64 i{ 0 };
//		UINT64 u;//error C2836: 'KVal::Nval::u': 공용 구조체에서 한 개의 비정적 데이터 멤버만 in-class initializer를 가질 수 있습니다. {0};
//		double d;
//	} n;
//
//public:
//	KVal(): n(0) {}
//	KVal(const KVal& v): t(v.t), s(v.s), n(v.n) {}
//
//	KVal(LPCWSTR v): n(0), s(v), t(eStr) {}
//	KVal(const wstring& v): n(0), s(v.c_str()), t(eStr) {}
//	KVal(LPCSTR v): n(0), s((LPCWSTR)CStringW(v)), t(eStr) {}
//
//	KVal(int v): n(v), t(eInt) {}
//	KVal(long v): n((int)v), t(eInt) {}
//
//	KVal(UINT v): n(v), t(eUnt) {}
//	KVal(unsigned long v): n((UINT)v), t(eUnt) {}
//
//	//__int64	8	long long, signed long long	-9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
//	KVal(INT64 v): n(v), t(eI64) {}
//	KVal(UINT64 v): n(v), t(eU64) {}
//
//	KVal(double v): n(v), t(eFlt) {}
//	KVal(float v): n((double)v), t(eFlt)
//	{
//
////n((double)v), 해줘도 뒤에 쓰레기 들어간다.  
////+		kp1	{k="xxxxxf" s=L"" n=123.40000152587891 ...}	KPara 그래서 한번 받아서
//#ifndef _WIN64 //x86 32비트로 빌드할때
//		auto d = (INT64)(v * 100000.);// (double) cast해도 소수 7자리 부터 쓰레기 붙어서 고스란히 double에 들어 가므로 잘라낸다.
//		n.d = d / 100000.;//이렇게 해야 double 넣을때와 float 넣을때 내부값이 동일 하다.
//#else
//		n.d = v;
//#endif
//		// 		float	4	none	3.4E +/- 38 (7 digits)
//		// 		double	8	none	1.7E +/- 308 (15 digits)
//	}
//	KVal(CTime v): n(v.GetTime()), t(eTme) {}
//	KVal(COleDateTime v): n(0), t(eTme)
//	{
//		CTime ct = OletTmToCtm(v);
//		n.i = ct.GetTime();
//	}
//	void Init()
//	{
//		n = Nval(0);
//		s.clear();
//	}
//	// tool
//	CTime OletTmToCtm(COleDateTime& ot)
//	{
//		if (ot.GetStatus() == COleDateTime::valid)
//		{
//			SYSTEMTIME sm ={ 0 };
//			ot.GetAsSystemTime(sm);
//			if (sm.wYear >= 1900)
//			{
//				CTime ct(sm.wYear, sm.wMonth, sm.wDay, sm.wHour, sm.wMinute, sm.wSecond);
//				return ct;
//			}
//		}
//		return CTime();
//	}
//
//public:
//	bool IsNan() const
//	{
//		if (t == eInt || t == eI64 || t == eUnt || t == eU64)
//			return false;
//		else if (t & eFlt)
//			return isnan(n.d) || isinf(n.d) || isfinite(n.d);
//		else
//			return false;
//	}
//	void operator=(LPCWSTR v) { s = v; t = eStr; }
//	void operator=(LPCSTR v) { s = (LPCWSTR)CStringW(v); t = eStr; }
//	void operator=(INT64 v)
//	{
//		s.clear();
//		n = v; t = eI64;
//		//ASSERT(v <= MAXINTFLOAT);//내부적으로 15digit double 형으로 저장 되므로 숫자의 한계를 둔다.
//	}
//	void operator=(UINT64 v)
//	{
//		s.clear();
//		n = v; t = eI64;
//	}
//	void operator=(int v)
//	{
//		s.clear();
//		n = v; t = eInt;
//	}
//	void operator=(UINT v)
//	{
//		s.clear();
//		n.u = v; t = eInt;
//	}
//	void operator=(COleDateTime v)
//	{
//		s.clear();
//		CTime ct = OletTmToCtm(v);
//		n = (INT64)ct.GetTime();
//		t = eTme;
//	}
//	void operator=(CTime v)
//	{
//		s.clear();
//		n = v.GetTime();
//		t = eTme;
//	}
//	void operator=(double v)
//	{
//		s.clear();
//		n = v; t = eFlt;
//	}
//
//	class VNULL { char* v{ nullptr }; };
//	void operator=(VNULL& v) { n = 0; s.clear(); t = eNul; }
//
//#define COMPAREFNC(sig) \
//	BOOL operator##sig(const KVal& v) \
//	{	if(t == v.t)\
//		{	switch(t)\
//			{\
//			case eInt:	case eI64:	case eI16:\
//				return n.i sig v.n.i;\
//			case eUnt:		case eU64:\
//				return n.u sig v.n.u;\
//			case eFlt:\
//				return n.d sig v.n.d;\
//				break;\
//			case eStr:\
//				return s sig v.s;\
//				break;\
//			default:\
//				ASSERT(0);\
//				break;\
//			}\
//		}\
//		ASSERT(0);\
//		return FALSE;\
//	}
//	COMPAREFNC(<);
//	COMPAREFNC(==);
//	COMPAREFNC(>);
//
//	/// ///////////////////// GET //////////////////////////
//public:
//	int I()
//	{
//		if (t & (eInt | eI64))		return (int)n.i;
//		else if (t & (eUnt | eU64))	return (int)n.u;
//		else if (t & eFlt)			return (int)n.d;
//		else if (t & eStr)			return _wtoi(s.c_str());
//		ASSERT(0);//아직 return 안했다면~ 이상하지.
//		return 0;
//	}
//	UINT U()
//	{
//		if (t & (eInt | eI64))		return (UINT)n.i;
//		else if (t & (eUnt | eU64))	return (UINT)n.u;
//		else if (t & eFlt)			return (UINT)n.d;
//		else if (t & eStr)			return (UINT)_wtoi64(s.c_str()); //_wtoll
//		ASSERT(0);//아직 return 안했다면~ 이상하지.
//		return 0;
//	}
//	double D()
//	{
//		if (t & (eInt | eI64))		return (double)n.i;
//		else if (t & (eUnt | eU64))	return (double)n.u;
//		else if (t & eFlt)			return n.d;
//		else if (t & eStr)			return (double)_wtof(s.c_str());
//		ASSERT(0);//아직 return 안했다면~ 이상하지.
//		return 0;
//	}
//	float F() { return (float)D(); }
//	INT64 I64()
//	{
//		if (t & (eInt | eI64))		return n.i;
//		else if (t & (eUnt | eU64))	return (INT64)n.u;
//		else if (t & eFlt)			return (INT64)n.d;
//		else if (t & eStr)			return _wtoi64(s.c_str());
//		else if (t & eTme)			return (INT64)n.i;
//		ASSERT(0);//아직 return 안했다면~ 이상하지.
//		return 0;
//	};
//	__time64_t Tm()
//	{
//		ASSERT(t == eI64);
//		return I64();
//	}
//	UINT64 U64()
//	{
//		if (t & (eInt | eI64))		return (UINT64)n.i;
//		else if (t & (eUnt | eU64))	return n.u;
//		else if (t & eFlt)			return (UINT64)n.d;
//		else if (t & eStr)			return (UINT64)_wtoi64(s.c_str()); //_wtoll
//		ASSERT(0);//아직 return 안했다면~ 이상하지.
//		return 0;
//	}
//	/// 값이 보존되는 문자열포인터. 받은 값을 바로 사용해야 함. 
//	operator LPCWSTR() const { return s.c_str(); }
//	operator int() { return I(); }
//	operator double() { return D(); }
//	operator INT64() { return I64(); }
//	operator float() { return F(); }
//	operator UINT() { return U(); }
//	operator UINT64() { return U64(); }
//
//#define NDPOINT 2
//	/// fmt는 값이 숫자나 시간인 경우 형식을 지정한다.
//	// ex num: "%02d", "%.3f%", "%Y-%m-%d %H:%M:%S"
//	// nRound : 반올림한 후 소숫점 자리. 2이면 3번째 자리가 반올림 한후 없어진다. 123.3457 -> 123.35
//	CStringW S(LPCWSTR fmt = NULL, int nRound = NDPOINT)
//	{
//		if (t & (eInt | eI64))
//			return KLLtoW((INT64)n.i, fmt);
//		else if (t & (eUnt | eU64))
//			return KULLtoW(n.u, fmt);
//		else if (t & eFlt)
//		{
//			auto tn = pow(10, nRound);//100.
//			auto d1 = n.d * tn;
//			auto d2 = round(d1); //소숫점 nRound자리 밑에서 반올림
//			auto d = d2 / tn; //소숫점 nRound자리 밑에서 반올림
//			CStringW sfmt;
//			if (fmt == NULL)//올림/반올림/내림을 위한 ceil, round, floor
//				sfmt.Format(L"%%.%df", nRound);//	sfmt = L"%.2f";//보통 UI에 표시할때 소숫점 2자리 까지 표현
//			else
//				sfmt = fmt;
//			CStringW sw; sw.Format((LPCWSTR)sfmt, d);
//			return sw;
//// 			auto u92 = kar1.S(NULL, 3);//반올림하여 소수3자리까지 구한다.
//// 			auto u93 = kar1.S(L"%.3f%%", 3);//반올림하여 소수3자리까지 구한 후 문자포맷에 보낸다.
//		}
//		else if (t & eTme)
//		{
//			CStringA fmta(fmt);
//			CTime tm = T();
//			return tm.Format(fmta);
//		}
//		else if (t & eStr)
//		{
//			if (fmt)
//			{
//				CStringW sw; sw.Format(fmt, s.c_str());
//				return sw;
//			}
//			else
//				return CStringW(s.c_str());
//		}
//		ASSERT(0);
//		return L"";
//	}
//
//	wstring wstr(LPCWSTR fmt = NULL, int nRound = NDPOINT)
//	{
//		if (t == eStr)
//		{
//			if (fmt)
//				return (LPCWSTR)S(fmt, nRound);
//			else
//				return s;
//		}
//		else
//		{
//			wstring ws = (LPCWSTR)S(fmt, nRound);
//			return ws;
//		}
//	}
//	CStringA SA(LPCSTR fmtA = NULL, int nRound = NDPOINT)
//	{
//		CStringW fmt(fmtA);
//		return CStringA(S(fmt, nRound));
//	}
//	string str(LPCSTR fmtA = NULL, int nRound = NDPOINT)
//	{
//		return string((LPCSTR)SA(fmtA, nRound));
//	}
//
//	time_t GetTmt() { return I64(); }
//	CTime T() { return CTime(GetTmt()); }
//	COleDateTime OT() { return COleDateTime(GetTmt()); }
//	DBTIMESTAMP DT()
//	{
//		COleDateTime ot(GetTmt());
//		DBTIMESTAMP dt ={ 0, };
//		bool b = ot.GetAsDBTIMESTAMP(dt);	ASSERT(b);
//		return dt;
//	}
//
//	/// 숫자타입인 경우 값을 증감 시킨다.
//	void Inc(int inc = 1)
//	{
//		if (t & (eInt | eI64))
//			n.i += inc;
//		else if (t & (eUnt | eU64))
//		{
//			n.u += inc;
//		}
//		else if (t & eFlt)
//			n.d += (double)inc;
//		else
//			ASSERT(0);//아직 return 안했다면~ 이상하지.
//	}
//	void Trim()
//	{
//		ASSERT(t == eStr);
//		if ((t == eStr))
//			wstrltrim(s);
//	}
//
//};



/// CStringW gagabe collection
/// 담을 통을 주지 않고, 문자열 포인터를 리턴 받고 싶으면 이걸 쓴다. 힘수 사용의 편리성 도모.
/// 내부적으로 15000개 까지 저장 되는데, 그이상 쓰는 대량의 리스트 에서는 쓰면 안되고, 
/// 굳이 쓰려면 리턴한 포인터를 CStringW으로 받아야 한다.
//template<typename TYPE, typename ARG_TYPE = const TYPE&>
//class CStrBufferT
//{
//public:
//	CStrBufferT(int nmax = 15000)
//		: m_nMax(nmax)
//	{
//	}
//	int m_nMax;
//	void SetMaxBufCount(int nMax)
//	{
//		m_nMax = nMax;
//	}
//
//	CUcCriticalSection m_csBuf;//UcAUTOLOCK(m_csBuf);
//	CList<TYPE, ARG_TYPE> m_arBuf;
//
//	TYPE& GetBuf(LPCSTR fnc = NULL, int line = 0)
//	{
//		CSingleLock __synchthis(&m_csBuf, TRUE);
//		//UcAUTOLOCK(m_csBuf);
//		INT_PTR n = m_arBuf.GetCount();
//		if (n >= m_nMax)
//			m_arBuf.RemoveHead();
//
//		POSITION pos = m_arBuf.AddTail(TYPE());// (ARG_TYPE)L"");
//		return m_arBuf.GetAt(pos);
//	}
//	void ReleaseBuf()
//	{
//		m_arBuf.RemoveAll();
//	}
//	static CStrBufferT<TYPE, ARG_TYPE>* GetStrBufT()
//	{
//		/// s_buf는 사실 A, W 두개가 생긴다.
//		static CStrBufferT<TYPE, ARG_TYPE>* s_buf = nullptr;
//		if (s_buf == nullptr)
//			s_buf = new CStrBufferT<TYPE, ARG_TYPE>();
//		return s_buf;
//	}
//};



#define CASE_STR0(ev)	case ev: psErr = _T(#ev); break;
#define CASE_TSTR(ev)	case ev: return _T(#ev)
#define CASE_ASTR(ev)	case ev: return #ev
#define CASE_WSTR(ev)	case ev: return L#ev
#define CASE_ABRK(ev)	case ev: str = #ev; break
#define CASE_WBRK(ev)	case ev: str = L#ev; break
#define CASE_TBRK(ev)	case ev: str = _T(#ev); break


template<typename TYPE>
inline int UcZeroMemory(TYPE& sbuf)
{
	int len = sizeof(TYPE);//return 할려고
	memset(reinterpret_cast<void*>(&sbuf), 0, len);
	return len;
}






UCTOOLDYNAMIC
BOOL UcIfFileExistEx(LPCTSTR filePath, BOOL* pbDir = NULL);

UCTOOLDYNAMIC
CStringW UcUTF8ToWchar(LPCSTR sUtf8);

inline CStringW UcUTF8ToWchar(std::string& sUtf8) {
	return UcUTF8ToWchar(sUtf8.c_str());
}

UCTOOLDYNAMIC
LPCWSTR UcUTF8ToWchar(LPCSTR sUtf8, CStringW& sWstr);

UCTOOLDYNAMIC
CStringA UcWcharToUTF8(LPCWSTR sWstr);

UCTOOLDYNAMIC
LPCSTR UcWcharToUTF8(LPCWSTR sWstr, CStringA& sUtf8);

class KBinary;
UCTOOLDYNAMIC
LPCSTR UcWcharToUTF8(LPCWSTR sWstr, KBinary& sUtf8);

template <typename TCH>
void UcCutByToken(const TCH* psSrc, const TCH* seps, CArray<CString, LPCTSTR>& ars, bool bTrim = false)
{
	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
		ars.Add(str);
	}, bTrim);
}


CString UcGetFormattedGuid(bool bHipn = true);
//CStringA UcGetFormattedGuidA(bool bHipn = true);


//void UcJsonToData(UcJObj& jDocData, ShJObj& sjobj, bool bToJson);

//int UcJsonSerialize(UcJObj& jDocData, CArchive & ar);
inline bool UcIsDirectory(LPCTSTR pDir)
{
	auto dw = ::GetFileAttributes(pDir);
	bool bDir = dw != INVALID_FILE_ATTRIBUTES && (dw & FILE_ATTRIBUTE_DIRECTORY);
	return bDir;
}

/// <summary>
/// folder check or create
/// </summary>
/// <param name="sFull"></param>
/// <param name="bCreate">TRUE:폴더 없으면 생성</param>
/// <param name="bToEnd">FALSE:마지막 이름은 파일명이니 체크 중 제외</param>
/// <param name="iStart"></param>
/// <returns>0:exists, other:error</returns>
UCTOOLDYNAMIC
int UcCheckTargetDir(LPCTSTR sFull, BOOL bCreate = TRUE, BOOL bToEnd = TRUE, int iStart = 0);

UCTOOLDYNAMIC
bool UcIsDirExists(LPCTSTR sDir);

//CStringW& UcMoneyToStr(CStringW& sm, int nUnderDot = -1, bool bComma = true, bool bTruncate = true);
CStringW UcMoneyToStr(double dv, int nUnderDot = -1, bool bComma = true, bool bTruncate = true);


/// <summary>
///람다 함수와 함께 리커시브하게 디렉토리 스캔할때.
///람다에 파일삭제 또는 파일 다루는 어떤 일도 할수 있다.
///ex: 아래 KwDeleteTheFile(...)
/// </summary>
/// <param name="dir"></param>
/// <param name="stExt"></param>
/// <param name="fFile">파일 발견할 때 마다 불리는 람다함수</param>
/// <param name="fDir">폴더 발견할 때 마다 불리는 람다함수</param>
/// <param name="bSubDir">서브폴더 까지 뒤진다.</param>
/// <param name="bExitOnError"></param>
/// <param name="nLevel"></param>
/// <returns></returns>
int UcRecursiveDirLambda(CString dir, std::set<CString> stExt,
	std::function<int(CString, const WIN32_FIND_DATA&)> fFile,
	std::function<int(CString, int)> fDir = NULL,
	BOOL bSubDir = 1, BOOL bExitOnError = 0, int nLevel = 0);

DWORD UcRemoveDir(LPCWSTR pDir, function<int(PWS sm, LPCWSTR sf)> cbDel = NULL);


CStringW UcMakeLongNameWorks(PWS lpszFileName, int extLen = 0);


void UcSetFileStatus(PWS lpszFileName, const CFileStatus& status);

BOOL UcGetFileStatus(PWS lpszFileName, CFileStatus& rStatus);


class UcSafeCopy
{
public:
	HANDLE _hTransaction{ NULL };
	BOOL _bSuccess{ FALSE };
	KStdMap<CStringW, BOOL> _fails;

	explicit UcSafeCopy(BOOL bAutoBegin = FALSE)
	{
		if (bAutoBegin)
		{
			VERIFY(Begin());
		}
	}
	~UcSafeCopy()
	{
		if (_hTransaction)
		{
			if (!_bCommited && _fails.size() == 0)
				Commit();
			else if (!_bRollbacked)
				Rollback();
		}
	}
	void Close()
	{
		CloseHandle(_hTransaction);
		_hTransaction = NULL;
	}

	BOOL Begin()
	{
		_hTransaction = CreateTransaction(NULL, 0, 0, 0, 0, 0, NULL);
		return !(_hTransaction == INVALID_HANDLE_VALUE);
	}

	BOOL _bCommited{ FALSE };
	BOOL _bRollbacked{ FALSE };
	BOOL Commit()
	{
		ASSERT(_hTransaction);
		BOOL rv = CommitTransaction(_hTransaction);
		ASSERT(rv);
		_bCommited = TRUE;
		Close();
		return rv;
	}

	void Rollback()
	{
		ASSERT(_hTransaction);
		RollbackTransaction(_hTransaction);
		_bRollbacked = TRUE;
		Close();
	}
	BOOL IsAllSuccess()
	{
		return _fails.size() == 0;
	}
	BOOL SafeCopyFile(LPCTSTR sSrcFile, LPCTSTR sTarFile, BOOL bOverWrite = FALSE, BOOL bCheckFolder = TRUE);
};

UCTOOLDYNAMIC
BOOL UcSafeCopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bOverWrite = FALSE, BOOL bCheckFolder = TRUE);
UCTOOLDYNAMIC
BOOL UcSafeCopyFile(std::vector<std::pair<std::wstring, std::wstring>>& filesToCopy, BOOL bOverWrite = FALSE, BOOL bCheckFolder = TRUE
	, function<int(wstring, wstring)> cbBefore = NULL
	, function<void(wstring, wstring)> cbAfter = NULL
	, function<void(wstring, wstring, int, BOOL)> cbError = NULL);

enum {
	eScpOK = 0,
	eScpTransactionError = -50,
	eScpDontCopy = -100,
	eScpAbort = -200,
	eScpCpError = -1000,
};
int UcSafeCopyFileEx(std::vector<std::pair<std::wstring, std::wstring>>& filesToCopy, BOOL bOverWrite = FALSE, BOOL bCheckFolder = TRUE
	, function<int(wstring, wstring)> cbBefore = NULL
	, function<void(wstring, wstring)> cbAfter = NULL
	, function<void(wstring, wstring, int, BOOL)> cbError = NULL);

UCTOOLDYNAMIC
long UcWriteSmallTextFileA(LPCTSTR filename, CStringA& text, BOOL bOverwrite = TRUE);

UCTOOLDYNAMIC
long UcWriteSmallTextFileW(LPCWSTR filename, CStringW& text);

CStringA UcGetErrorMsg(UINT err = 0xffffffff);
UCTOOLDYNAMIC
CStringW UcErrorToStrW(UINT err = 0xffffffff);

void UcShowWindow(CWnd* pw, int idc, int eShow);

void UcShowWindow(CWnd* pw, std::initializer_list<int> arIdc, int eShow);

PAS UcShowValueToStr(UINT sv);


PWS UcUTF8ToHtmlUrl(CStringA& sUtf8, CStringW& sWstr);


PWS UcWcharToUTF8ToHtmlUrl(CStringW& sWchar, CStringW& sWUrl);


// CTime tExp = UcParseTimeStrA4(sExpire);auto tNow = UcGetCurrentTime();
/// macro test
//PAS sExpire = "2024-03-31 15:34";
//CTime tExp = UcParseTimeStrA4(sExpire);
//auto tNow = UcGetCurrentTime();
//CStringW sm;sm.Format(L"test UcMessageTempBox");
//CStringW s;s.Format(L"%s\n- - - - - - - - - - - - - - - - - - \nMessage displays until [%s]", sm, CStringW(sExpire));
//if (tNow <= tExp)
//UcMessageBoxGeneral(MB_OK, s);
// ex: 
// UcMessageTempBox("2024-03-18 15:35", L"test UcMessageTempBox 2");

/// <summary>
/// 2024-02-13 10:38:55
/// </summary>
/// <param name="fid"></param>
/// <returns></returns>
CStringW UcGetShellFolder(REFKNOWNFOLDERID fid = FOLDERID_LocalAppData);
bool UcCompareLastMultiSegments(CStringW path1, CStringW path2, int nSegment = 3);
bool UcCompareFolderWithUrlDir(CStringW sLocalFolder, CStringW sUrlDir);
CStringW UcMakeBackupFileName(CStringW ufj, bool bBuFolder = true, int nLength = 0);
CStringW UcMakeBackupFileNameGeneral(CStringW ufj, bool bBuFolder = true, int nLength = 0, WCHAR slash = '\\');

int UcBackupFile(CString ufj, int nDayExpire = 0);
bool UcCompressFile(const wchar_t* sourcePath, const wchar_t* destPath);
bool UcDecompressFile(const wchar_t* sourcePath, const wchar_t* destPath);
CStringW UcGetProductName();
//class UcJObj;
//class JBase;/// 이게 error C3240: 'IsDic': 'JBase'의 오버로드되지 않은 추상 멤버 함수여야 합니다.
/// moved to UcJson.h
//void UcJsonToData(UcJObj& jDocData, SHP<JBase>& sjobj, bool bToJson);
//void UcJsonSave(UcJObj& jDocData, CFile & oFile, function<int(LPCWSTR, int)> cbChk = NULL, int preety = 3);
////void UcJsonSave(UcJObj& jDocData, CStringW sFile, BOOL bBackup = FALSE);
//int UcJsonLoad(SHP<JBase>& jDocData, CFile & oFile, function<int(int, int, LPCWSTR)> cb = nullptr);
//int UcJsonLoad(SHP<JBase>& jDocData, CString sFile);
//int UcJsonLoad(SHP<JBase>& jDocData, LPCSTR psUtf8, DWORD len, function<int(int, int, LPCWSTR)> cb = nullptr);
//int UcJsonLoad(SHP<JBase>& jDocData, LPCWSTR sWstr, DWORD len, function<int(int, int, LPCWSTR)> cb = nullptr);
//int UcJsonLoad(UcJObj& jDocData, CFile& oFile, function<int(int, int, LPCWSTR)> cb = nullptr);
//
//void UcJsonSave(UcJObj& jDocData, CString sPath, BOOL bBackup = FALSE, int nDayExpire = 0, int preety = 3);
//void UcJsonSave(SHP<JBase> jDocData, CString sPath, BOOL bBackup = FALSE, int nDayExpire = 0, int preety = 3);
//
////int UcJsonSerialize(UcJObj& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb = nullptr, function<int(LPCWSTR, int)> cbChk = NULL);
//
//int UcJsonSerialize(UcJObj& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb = nullptr, function<int(LPCWSTR, int)> cbChk = NULL);
//int UcJsonSerialize(SHP<JBase>& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb = nullptr, function<int(LPCWSTR, int)> cbChk = NULL);


/// <summary>
/// 서버에서 주로 발생 하는 exception을 처리 하는 객체.
/// 
/// </summary>

class UCTOOLDYNAMIC KException : public CException
{
	DECLARE_DYNAMIC(KException)
public:
	// s) {KException("throw_gen",       errn,         0,         NULL,              s, __FUNCTIONW__, __LINE__, __FILE__, NULL, FALSE); throw_common; }
	KException(LPCSTR sExcept, DWORD error, int rcde, LPCWSTR sErr, LPCWSTR sState,
		LPCWSTR funcW = NULL, int line = 0, LPCSTR file = NULL,
		CRuntimeClass* pRc = NULL, int iOp = 0, LPCTSTR sLastErr = NULL, CException* ec = NULL)
	{
		Init(sExcept, error, rcde, sErr, sState, funcW, line, file, pRc, iOp, sLastErr, ec);
	}
	virtual ~KException()
	{
		// m_bAutoDelete 가 참이라 안오네
		//FinishException();//왜 안불리지?
	}
	enum// EOutput
	{
		eShowMsgBox = 0b0001,
		eNoDebugOutput = 0b0010,
		eNotError = 0b0100,
	};

	void Init(LPCSTR sExcept, DWORD error, int rcde, LPCWSTR sErr, LPCWSTR sState, LPCWSTR func = NULL, int line = 0, LPCSTR file = NULL, CRuntimeClass* pRc = NULL, int iOp = 0, LPCTSTR sLastErr = NULL, CException* ec = NULL)
	{
		_idThread = ::GetCurrentThreadId();
		_tmOccur = CTime::GetCurrentTime();

		if (sExcept || error || rcde || sErr || sLastErr)
			_break;//진짜 에러
		else
			_break;//로그쌓는게 목적인 가짜에러
		_sExcept = (sExcept);
		if (_sExcept == L"int" || _sExcept == L"long")// throw __LINE__ 이런거 처리 하기 위해
			_num = rcde;
		m_nRetCode = (rcde); // client에서 이값에 따라 무슨 짓을 구분 하기 위함
		_error = (error); ////GetLastError() trow_response(err, "sErr") 시스템에서 얻은 값
		//this->GetErrorMessage(_lastError.GetBuffer(1024), 1024);_lastError.ReleaseBuffer();
		if (tchlen(sLastErr))
			_lastError = sLastErr;
		if (sErr)
			m_strError = sErr;
		m_strStateNativeOrigin = (sState);
		_func = (func);
		_fileTar = (file);
		_line = (line);
		if (pRc)
			_class = pRc->m_lpszClassName;

		m_bAutoDelete = TRUE; // CException 변수: 이게 TRUE여야 ->Delete() 가 먹는다. FALSE로 줘도  destrictor가 안불려 진다
		_iOp = iOp;
		//_bBox = (iOp & eShowMsgBox) == eShowMsgBox;//bBox;
		/// UCCATCH_ALL을 하지 않은 경우 FinishException가 안불릴수 있으니. 생성 하자 마자 일단 부른다.
		FinishException();//왜 안불리지?
	}

	// 통합 복사/이동 함수
	//template<typename T>
	//void CopyOrMoveFrom(T&& other, bool bMove = false)
	void CopyOrMoveFrom(const KException& other, bool bMove = false)
	{
		_idThread              = other._idThread    ;
		m_nRetCode             = other.m_nRetCode   ;
		_error                 = other._error       ;
		_line                  = other._line        ;
		_num                   = other._num         ;
		_iOp                   = other._iOp         ;
		_status                = other._status      ;
		_bFinished             = other._bFinished   ;
		m_bAutoDelete          = other.m_bAutoDelete;
		
		_sExcept               = bMove ? std::move(other._sExcept              ): other._sExcept              ;
		_lastError             = bMove ? std::move(other._lastError            ): other._lastError            ;
		m_strError             = bMove ? std::move(other.m_strError            ): other.m_strError            ;
		_func                  = bMove ? std::move(other._func                 ): other._func                 ;
		_fileTar               = bMove ? std::move(other._fileTar              ): other._fileTar              ;
		_class                 = bMove ? std::move(other._class                ): other._class                ;
		m_strStateNativeOrigin = bMove ? std::move(other.m_strStateNativeOrigin): other.m_strStateNativeOrigin;
	}
	// 복사 생성자
	KException(const KException& other) {
		CopyOrMoveFrom(other, false);
	}
	// 이동 생성자
	KException(KException&& other) noexcept {
		CopyOrMoveFrom(std::move(other), true);
	}
	// 복사 대입 연산자
	KException& operator=(const KException& other) {
		if (this != &other) {
			CopyOrMoveFrom(other, false);
		}
		return *this;
	}
	// 이동 대입 연산자
	KException& operator=(KException&& other) noexcept {
		if (this != &other) {
			CopyOrMoveFrom(std::move(other), true);
		}
		return *this;
	}

	/// HTTP Request 일때 사용
	int _status{ 200 };// OK HTTP status 400(Bad Request)

	int m_nRetCode{ -1 };//return -1; 이 앱애서 의미있는 구분 - 이면 오류. + 이면 상황
	DWORD _error{ 0 };//GetLastError()시스템에서 얻은 값
	CStringW _lastError;//e->GetErrorMessage
	CStringW m_strError;//
	CStringW m_strStateNativeOrigin;//ODBC에러는 추가 오류 문자열이 있다.
	CStringW _sExcept;//CException GetRuntimeClass();의 CRuntimeClass::m_lpszClassName

	DWORD _idThread{};
	CTime _tmOccur;
	CStringW _stack;//dwk: 2025-08-12 16:18  FinishException

	CStringW _func;
	CStringW _fileTar;
	CStringW _class;
	int _line{ 0 };
	int _num{ 0 };
	int _iOp{ 0 };
	CStringW GetErrStr() {
		CStringW s;
		if (m_strError.IsEmpty()) {
			if (m_strStateNativeOrigin.IsEmpty())
				s = L"Unkown error.";
			else
				s = m_strStateNativeOrigin;
		}
		else
			s.Format(L"%s : %s", m_strError.GetString(), m_strStateNativeOrigin.GetString());
		return s;
	}
	//BOOL _bBox{ FALSE };
	// Implementation (use AfxThrowDBException to create)
	BOOL _bFinished{ FALSE };
	void FinishException();//int iOp = 0);
public:
	/// <summary>
	/// UCCATCH_DONE 에서 잡힌 예외를 KException 으로 바꾼 후 처리 한다.
	/// 여기서 보통 디버그 출력이나 로그를 쌓는다. DWKFUNCV 가 로그 까지 셋팅이 된 경우 DWKFUNCV 만 해도 기본적으로 할건 다 한거다.
	/// </summary>
	INLINE_STATIC std::function<void(KException*)> s_fncExceptionDealer;

	/// 이건 커스텀 CException derived 처리 해주는 람다함수. 예:CMyException
	INLINE_STATIC std::function<bool(CException*, KException*)> s_fncCExceptionChecker;

	/// 이건 커스텀 std::exception derived 처리 해주는 람다함수. 예:FMPDFException
	INLINE_STATIC std::function<bool(std::exception*, KException*)> s_fncStdExceptionChecker;
	///	Custom Exception Checker 사용 예제:
	///	1. std::exception 계열 custom 처리:
	///	KException::s_fncStdExceptionChecker = [](std::exception* ex, KException* ke) -> bool {
	///		if (auto* pdfEx = dynamic_cast<FMPDFException*>(ex)) {
	///			ke->_sExcept = "FMPDFException";
	///			ke->m_strError = pdfEx->GetErrorMessage();
	///			return true; // custom 처리 완료
	///		}
	///		return false; // 기본 처리 사용
	///	};
	///
	///	2. CException 계열 custom 처리:
	///	KException::s_fncCExceptionChecker = [](CException* ex, KException* ke) -> bool {
	///		if (auto* fileEx = dynamic_cast<CFileException*>(ex)) {
	///			ke->_sExcept = "CFileException";
	///			ke->m_strError = L"파일 오류: " + CStringW(fileEx->m_cause);
	///			return true; // custom 처리 완료
	///		}
	///		return false; // 기본 처리 사용
	///	};


private:
	INLINE_STATIC std::map<CStringW, std::shared_ptr<KException>> s_exceptionMap;
	INLINE_STATIC std::mutex s_mutex;
public:
	// 토큰 생성 함수와 스레드+추가토큰으로 키를 만드니, 같은 함수의 위치에서는 map 특정상 덮어 쓰니 이전께 중복 될 수는 없겠군.
	static CStringW GenerateKey(LPCWSTR functionName, const CStringW& token, DWORD threadId = 0) {
		if (threadId == 0)
			threadId = GetCurrentThreadId();
		CStringW key;
		key.Format(L"%08x_%s_%s", threadId, functionName, token.GetString());// GetTickCount());
		return key;
	}

	// 예외 저장. UCCATCH_DONE 에서 호출 한다.
	static void StoreEx(LPCWSTR functionName, const CStringW& token, KException* exception) {
		std::lock_guard<std::mutex> lock(s_mutex);
		// 기존 예외가 있으면 제거
		auto key = GenerateKey(functionName, token);
		auto it = s_exceptionMap.find(key);
		if (it != s_exceptionMap.end())
			s_exceptionMap.erase(it);
		// 새 예외 저장: make_shared  안하고 이미 new 로 온 포인터를 shared_ptr 로 싸서 넣는댜.
		//s_exceptionMap[key] = std::shared_ptr<KException>(exception);
		// 새 예외 저장 (커스텀 삭제자로 Delete() 호출)
		exception->m_bAutoDelete = TRUE; /// ~CException 은 기본적으로 셀프 삭제를 한다. CException::operator delete. 그래서 안되게.
		s_exceptionMap[key] = std::shared_ptr<KException>(exception,
			[](KException* p) {
			if (p)
				p->Delete();
		});
		if (s_exceptionMap.size() > 100) {
			// 100개 초과시 오래된 것부터 제거 (가장 오래된 20개 제거)
			std::vector<std::pair<CStringW, CTime>> temp;
#if CPP17_OR_LATER
			for (const auto&[k, v] : s_exceptionMap)
				if (v)
					temp.emplace_back(k, v->_tmOccur);
#else
			for (const auto& pair : s_exceptionMap)
				if (pair.second)
					temp.emplace_back(pair.first, pair.second->_tmOccur);
#endif
			// 시간순으로 vector 정렬 (오래된 것부터) exception이니 많지는 않겠지.
			std::sort(temp.begin(), temp.end(),
				[](const auto& a, const auto& b) { return a.second < b.second; });

#ifndef UcMin // min은 매크로로 std::min 에서 문제가 생기니 따로 만들어 쓴다.
#define UcMin(a,b) (a <= b ? a : b) //이거 하나 때문에 UcBaseTools.h 를 include하기는 싫다.
#endif // UcMin
			int toRemove = UcMin(20, (int)temp.size());// 가장 오래된 20개 제거
			for (int i = 0; i < toRemove; ++i)
				s_exceptionMap.erase(temp[i].first);
		}

	}//C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\atlmfc\src\mfc\except.cpp

	static void StoreEx(LPCWSTR functionName, const CStringA& token, KException* exception) {
		StoreEx(functionName, CStringW(token), exception);
	}

	// 예외 조회
	// 이 함수는 호출한 스레드에서 그 함수에서 발생한 exception만 리턴 한다. 그래서 함수명	과 스레드 아이디로 구분 한다.
	// ex:
	//try {
	//} UCCATCH_DONE;
	//if (auto ex__ = UcGetLastException()) {
	//}
	static std::shared_ptr<KException> GetLastException(LPCWSTR functionName, const CStringW& token = L"") {
		std::lock_guard<std::mutex> lock(s_mutex);
		auto key = GenerateKey(functionName, token);//key = L"00008218_FMRptClearDBPathName_"
		auto it = s_exceptionMap.find(key);
		if (it != s_exceptionMap.end()) {
			auto exception = it->second;
			TRACE(L"KException: %s, %s\n", exception->m_strError, exception->m_strStateNativeOrigin);
			//s_exceptionMap.erase(it);  // 어차피 함수+스레드 키로 map에 덮어 쓰니, 굳이 제거 하니 않아도 됨. 자동 삭제는 StoreEx에서 하게 됨. 너무 많이 쌓이면 메모리 문제가 생길수 있으니.
			return exception;
		}
		return nullptr;//현 스레드 그 함수에서 발생한 exception이 없다.
	}

	// 정리 KException._tmOccur 로 너무 오래 된거 부터 제거
	static void CleanupExceptions() {
		std::lock_guard<std::mutex> lock(s_mutex);
		s_exceptionMap.clear();
	}

};
#include <deque>
#include <mutex>

class GCStringBuffer {
private:
	INLINE_STATIC std::deque<CStringW> s_stringBuffer;
	INLINE_STATIC std::mutex s_bufferMutex;
	static const size_t MAX_BUFFER_SIZE = 100;
public:
	static LPCWSTR StoreString(const CStringW& str) {
		std::lock_guard<std::mutex> lock(s_bufferMutex);
		// 버퍼가 가득 찬 경우 맨 앞에서 제거
		if (s_stringBuffer.size() >= MAX_BUFFER_SIZE) {
			s_stringBuffer.pop_front();
		}
		// 새 문자열 추가
		s_stringBuffer.push_back(str);
		// 맨 뒤 문자열의 포인터 반환
		return s_stringBuffer.back().GetString();
	}

	static void ClearBuffer() {
		std::lock_guard<std::mutex> lock(s_bufferMutex);
		s_stringBuffer.clear();
	}
};


#define UcGetLastKExceptionEx(token) KException::GetLastException(__FUNCTIONW__, token)
#define UcGetLastException() UcGetLastKExceptionEx(L"")
// ex:
//try {
//} UCCATCH_DONE;
//if (auto ex__ = UcGetLastException()) {
//}

//[[deprecated]]
//#define throw_common_gen(_ke) TRACE("<< Click here to go to Source line\t#### EXCEPTION #### %s: %u, %d, %s, %s, - %s(%d)\n\n",\
//	_ke->_sExcept, _ke->_error, _ke->m_nRetCode, CStringA(_ke->m_strError), \
//		CStringA(_ke->m_strStateNativeOrigin), _ke->_func, _ke->_line ); throw _ke

//#define throw_common throw _ke// 결국 두번 나오니 여기서 하는건 뺀다.

//GetLastError() 값을 직접 줄때
#define throwLINE                                  {auto _ke = new KException("throwLINE",    0, 0,           (PWS)NULL, (PWS)NULL, __FUNCTIONW__, __LINE__, __FILE__, NULL, 0); throw _ke; }
#define throw_err(errn)                            {auto _ke = new KException("throw_gen", errn, 0, UcErrorToStrW(errn), (PWS)NULL, __FUNCTIONW__, __LINE__, __FILE__, NULL, 0); throw _ke; }
#define    throw_gen(errn, s)                      {auto _ke = new KException("throw_gen", errn, 0, (PWS)NULL          , (PWS)s   , __FUNCTIONW__, __LINE__, __FILE__, NULL, 0); throw _ke; }
#define no_throw_gen(errn, s)                      {           KException _ke(NULL       , errn, 0, (PWS)NULL          , (PWS)s   , __FUNCTIONW__, __LINE__, __FILE__, NULL, KException::eNotError);}
#define no_throw_genFL(errn, s, fname, line, file) {           KException _ke(NULL       , errn, 0, (PWS)NULL          , (PWS)s   , fname        , line    , file    , NULL, KException::eNotError);}

/// 주의: 매크로 함수 가로는 매크로명에 바짝 붙여야 한다. C7515 에러를 피하기 위해
#define     throw_str(fmt, ...)                    {CStringW VAL_LINE(s, __LINE__)(DwkFormat(fmt, ##__VA_ARGS__)); throw_gen(GetLastError(), VAL_LINE(s, __LINE__));}

/// no_throw계열은 throw는 하지 않고, 그냥 출력 또는 기록만 하게 하는 것이다.
#define  no_throw_str(fmt, ...)                    {CStringW VAL_LINE(s, __LINE__)(DwkFormat(fmt, ##__VA_ARGS__)); no_throw_gen(0, VAL_LINE(s, __LINE__));}
#define no_throw_str1(fmt)                         {CStringW VAL_LINE(s, __LINE__)(fmt); no_throw_gen(0, VAL_LINE(s, __LINE__));}
#define no_throw_strFL(fname, line, file, fmt, ...){CStringW VAL_LINE(s, __LINE__)(DwkFormat(fmt, ##__VA_ARGS__)); no_throw_genFL(GetLastError(), VAL_LINE(s, __LINE__), fname, line, file);}

/// <summary>
/// `현재파일(라인):somestr- `와 뒤에 문자열 까지 만들어 그 문자열을 리턴 한다.
/// 출력창에 뿌리지는 않는다.
/// </summary>
/// <returns>그 문자열을 리턴</returns>
CStringW GetFileLineW(LPCWSTR f, int l, LPCWSTR sTrace, LPCWSTR fmt, ...);//dwk: 2025-08-05 15:52 
#define UcFileLine(fmt, ...) GetFileLineW( __FILEW__, __LINE__, L"UcTRACE", fmt, ##__VA_ARGS__)

#define UcMessageBoxErrorLog(fmt, ...) no_throw_str((PWS)UcMessageBoxGeneralStr(MB_OK|MB_ICONSTOP, fmt, ##__VA_ARGS__))
#define UcMessageBoxErrorLog1(fmt)     no_throw_str1((PWS)UcMessageBoxGeneralStr(MB_OK|MB_ICONSTOP, fmt))

int UcMessageBoxGeneral(UINT nType, LPCWSTR fmt, ...);
UCTOOLDYNAMIC
CStringW UcMessageBoxGeneralStr(UINT nType, LPCWSTR fmt, ...);
int UcMessageBoxLastError();
int UcMessageBoxException(CException* e);
int UcFileComp(CString sf1, CString sf2);
//#define UcMessageRelese(fmt, ...)         UcMessageBoxGeneralStr(MB_OK|MB_ICONINFORMATION, fmt, ##__VA_ARGS__)
#define UcMessageBox(fmt, ...)            UcMessageBoxGeneralStr(MB_OK         |MB_ICONINFORMATION, fmt, ##__VA_ARGS__)
#define UcMessageBoxWaring(fmt, ...)      UcMessageBoxGeneralStr(MB_OK         |MB_ICONWARNING    , fmt, ##__VA_ARGS__)
#define UcMessageBoxError(fmt, ...)       UcMessageBoxGeneralStr(MB_OK         |MB_ICONSTOP       , fmt, ##__VA_ARGS__)
//#define UcMessageBoxErrorNoLog(fmt, ...)  UcMessageBoxGeneralStr(MB_OK         |MB_ICONSTOP       , fmt, ##__VA_ARGS__)
#define UcMessageBoxYesNo(fmt, ...)       UcMessageBoxGeneral   (MB_YESNO      |MB_ICONQUESTION   , fmt, ##__VA_ARGS__)
#define UcMessageBoxYesNoCancel(fmt, ...) UcMessageBoxGeneral   (MB_YESNOCANCEL|MB_ICONQUESTION   , fmt, ##__VA_ARGS__)
#define UcMessageBoxOkCancel(fmt, ...)    UcMessageBoxGeneral   (MB_OKCANCEL   |MB_ICONEXCLAMATION, fmt, ##__VA_ARGS__)
//#define UcMessageBoxError(fmt, ...) UcMessageBoxGeneral(MB_OK|MB_ICONEXCLAMATION, fmt, ##__VA_ARGS__)
#define UcMessageTempBox(sExpire, fmt, ...) {\
	auto rex = UcQaTestExpired(sExpire);\
	CStringW sm;sm.Format(fmt, ##__VA_ARGS__);\
	no_throw_str(sm);\
	CStringW s;s.Format(L"%s\n- - - - - - - - - - - - - - - - - - \nMessage displays until [%s]\n%s(%d)", sm.GetString(), CStringW(sExpire).GetString(), __FILEW__, __LINE__);\
	if (!rex) UcMessageBoxGeneral(MB_OK, s);}

class UcMessageBoxStatic
{
public:
	static CStringW s_title;
	static HWND s_hWndParent;// { NULL };static은 여기서 초기화 안되
	static void SetMessageBoxBase(CStringW title, HWND hWnd)
	{
		s_title = title;
		s_hWndParent = hWnd;
	}
};

BOOL UcAssertFailedLine(LPCWSTR sFunc, LPCSTR sFile, int nLine, LPCSTR sFlag, LPCWSTR smsg = NULL, bool bMDbgBox = true);

//#include <corecrt.h>//__FILEW__
#ifdef _DEBUG
#define UCASSERT(f)          ((void) ((f) || !UcAssertFailedLine(__FUNCTIONW__, __FILE__, __LINE__, #f   ) || (AfxDebugBreak(), 0)))
#define UCASSERT2(f,s)       ((void) ((f) || !UcAssertFailedLine(__FUNCTIONW__, __FILE__, __LINE__, #f, s) || (AfxDebugBreak(), 0)))
#else
#define UCASSERT(f)          ((void) ((f) || !UcAssertFailedLine(__FUNCTIONW__, __FILE__, __LINE__, #f   ) ))
#define UCASSERT2(f,s)       ((void) ((f) || !UcAssertFailedLine(__FUNCTIONW__, __FILE__, __LINE__, #f, s) ))
#endif // _DEBUG


#ifdef _RefNote____ //참고
CFileException::ThrowOsError((LONG)::GetLastError(), L"TEST File Exception.exp");
throw std::exception("An TEST error has occurred");
throwLINE;
#endif // _DEBUG

#define _FWLNFL_ __FUNCTIONW__, __LINE__, __FILE__

#define UCCATCH_ALLEX2(token, rethr) \
catch (KException* ke)    {ke->FinishException(); if(rethr) throw ke;}\
catch (CException* ec)    {auto rc = ec->GetRuntimeClass(); std::vector<TCHAR> buf(1024);ec->GetErrorMessage(buf.data(), 1023);\
                           auto ke0 = new KException(rc ? rc->m_lpszClassName : "CException", GetLastError(), 0, (LPCWSTR)NULL, (PWS)0, _FWLNFL_, rc, 0, (LPCTSTR)buf.data(), ec);\
	if(KException::s_fncCExceptionChecker)   KException::s_fncCExceptionChecker(ec, ke0);                               KException::StoreEx(__FUNCTIONW__, token, ke0);if(rethr)throw ke0;}\
catch (std::exception& es){auto ke1 = new KException("std::exception", -1,  0, CStringW(es.what()), (PWS)0, _FWLNFL_);\
	if(KException::s_fncStdExceptionChecker) KException::s_fncStdExceptionChecker(&es, ke1);                            KException::StoreEx(__FUNCTIONW__, token, ke1);if(rethr)throw ke1;}\
catch (PWS   &ew)         {auto ke2 = new KException("LPCWSTR", GetLastError(),       0, CStringW(ew)  , (PWS)0, _FWLNFL_); KException::StoreEx(__FUNCTIONW__, token, ke2);if(rethr)throw ke2;}\
catch (PAS   &ea)         {auto ke3 = new KException("LPCSTR" , GetLastError(),       0, CStringW(ea)  , (PWS)0, _FWLNFL_); KException::StoreEx(__FUNCTIONW__, token, ke3);if(rethr)throw ke3;}\
catch (int   &ln)         {auto ke4 = new KException("int"    , GetLastError(),      ln, L"catch(int)" , (PWS)0, _FWLNFL_); KException::StoreEx(__FUNCTIONW__, token, ke4);if(rethr)throw ke4;}\
catch (long  &ln)         {auto ke5 = new KException("long"   , GetLastError(), (int)ln, L"catch(long)", (PWS)0, _FWLNFL_); KException::StoreEx(__FUNCTIONW__, token, ke5);if(rethr)throw ke5;}\
catch (...)               {auto ke6 = new KException("Unknown", GetLastError(),       0, L"catch(...)" , (PWS)0, _FWLNFL_); KException::StoreEx(__FUNCTIONW__, token, ke6);if(rethr)throw ke6;}

//#define UCCATCH_ALLGEN(excepFnc) UCCATCH_ALLEX(excepFnc, L"")

/// 3. rethrow 하므로 최 말단 에서 한번더 try{}catch(...){} 하면 된다.
/// 1. 어디선가 UCCATCH_ALL을 하지 않아도 throwLINE throw_str 를 부르면 로그가 쌓인다.
/// 2. 일반적인 exception은 반드시 UCCATCH_ALL 해야 한다. 안하려면 반드시 throwLINE이라도 해 줘야 한다.
#define UCCATCH_ALL UCCATCH_ALLEX2("", true)
#define UCCATCH_RETHROW UCCATCH_ALL

/// 이건 모든 exception을 처리해 버린다. 로그 처리 같은 걸로 떼웠으니 안해도 된다는 거다.
/// teken 은 한 함수에 try catch가 하나 뿐이면 UCCATCH_DONE을 쓴다.
/// 두개 이상일 때는 token을 다르게 주면 된다. "A", "B"... 
#define UCCATCH_DONEX(token) UCCATCH_ALLEX2(token, false)

/// teken 은 한 함수에 try catch가 하나 뿐일 때 쓴다.
#define UCCATCH_DONE UCCATCH_DONEX("")

#define UC_RETURNX(tkn) \
	auto VAL_LINE(ex__,__LINE__) = UcGetLastKExceptionEx(tkn);\
	return VAL_LINE(ex__,__LINE__) ? GCStringBuffer::StoreString(VAL_LINE(ex__,__LINE__)->m_strError) : nullptr

#define UC_RETURN UC_RETURNX(L"")

/// catch 후 에러 리턴 : 에러 없으면 nullptr 리턴
/// StoreString는 ex__가 날라 가더라도 ex__->m_strError 가 제대로 전달 되도록 임시 거쳐에 복사 되어 잠시 머물수 있게 한다.
#define UCCATCH_RETURN UCCATCH_DONE; UC_RETURN

//
#define UCCATCH_RETURNX(tkn) UCCATCH_DONEX(tkn); UC_RETURNX(tkn);

///	sample 직접 에러를 리턴 : 권장
//		try {
//			theApp.m_pFMReportCaseInfo->Clear();
//		} UCCATCH_DONE;
//		auto ex = UcGetLastException();
//		return ex ? ex->m_strError : nullptr;

///	sampe 매크로 리턴 : 아주 단순 함수인 경우 코드 줄임
//		try {
//			theApp.m_pFMReportCaseInfo->AddDBPathName(nProductType, nOrganizationType, lpszPathName, pfmvDump/*, bBookmarkInclude*/);
//		} UCCATCH_RETURN;



CString UcGetFileVersionRunning(LPCTSTR filePath = NULL);

CString UcGetFileVersion(CString filePath, BOOL bProductVersion = FALSE);
CString UcGetProductVersion(LPCTSTR filePath);

std::set<CString> UcEnumerateSubKeys(CString sRootKey);

float UcGetCPUUsage();

void UcEvaluateSystemState();

#define DISK_COUNTER L"\\LogicalDisk(C:)\\Disk Bytes/sec"
#define NETWORK_COUNTER L"\\Network Interface(*)\\Bytes Total/sec"

double UcGetPerformanceCounterValue(LPCTSTR counterName);

bool UcRegOpen(CRegKey& reg, CString sKey, HKEY kUp = HKEY_CURRENT_USER, DWORD iOp = KEY_READ | KEY_WRITE);

CStringW UcHKeyToStr(HKEY hk);

CRegKey* UcParseAndSetRegistryKey(CString sRegKey, CRegKey& key, HKEY kUp = HKEY_CURRENT_USER);

void UcParseAndSetRegistryValue(CString regContent);

CString UcGetExeFilePath();

CStringW UcGetProductVersion();

CStringW UcShortLambdaName(CStringW sFnc);

void UcFileLineTrace(const CStringW& sFile, int nLine, const CStringW& sOwner, const CStringW& sTxt);

void UcTrace(const CStringW& sFile, int nLine, const CStringW& sOwner, LPCWSTR fmt, ...);

int UcGetRandomNumber(int minNum, int maxNum);

BOOL UcCopyTextClipboad(LPCWSTR text, HWND hwnd);

int UcIsProcessRunningEx(LPCTSTR lpszProcessName, std::vector<tuple<DWORD, CString, CString>>* dwProcessId = NULL);

size_t UcKillProcessEx(LPCTSTR sExe, LPCTSTR pskill = NULL);

CStringW UcGetShallExcuteErrorStr(HINSTANCE hInstance);

SHP<KStdMap<UINT, CStringW>> UcGetLastErros(int toFind = -1);
bool UcErrorFound(int toFind);
HANDLE UcCheckMutex(CString mutexName, BOOL bMsg = FALSE, BOOL bOwner = TRUE);

BOOL UcRegisterAppForAutoStart(LPCTSTR pszAppName, LPCTSTR pszAppPath, bool bSetOrDelete = true);

class UcMutex {
public:
	explicit UcMutex(LPCWSTR sAppName = NULL)
	{
		if (sAppName)
			_sAppName = sAppName;
	}
	CString _sAppName;
	HANDLE _hMutex{ NULL };
	~UcMutex()
	{
		CloseMutex();
	}
	void CloseMutex()
	{
		if (_hMutex)
		{
			CloseHandle(_hMutex);
			_hMutex = NULL;
		}
	}
	BOOL OpenNamed(LPCWSTR sAppName = NULL, BOOL bMsg = FALSE, BOOL bOwner = TRUE)
	{
		if (sAppName)
			_sAppName = sAppName;
		ASSERT(_hMutex == NULL);
		_hMutex = UcCheckMutex(_sAppName, bOwner);
		auto err = GetLastError();
		return _hMutex && err != ERROR_ALREADY_EXISTS;
	}
};

bool UcWriteFileFromResource(HMODULE hModule, LPCTSTR lpRscName, LPCTSTR lpFile);

std::map<int, CString> UcLoadAllResourceStringsMap(HMODULE hModule, LANGID langID);

int UcFindMostSimilarSentence(CStringW target, int cnt, function<CStringW& (int)> getStr);

CStringW UcSetVerTitle(CWnd* wnd, LPCWSTR head = nullptr, LPCWSTR tail = nullptr);

shared_ptr<char> UcSharedBuffer(ULONGLONG len);

CStringW UcBitToStr(int val, std::map<int, PWS> mapBitStr);

CStringW UcEnumToStr(int val, std::map<int, PWS> mapBitStr);


std::pair<CStringW, CStringW> UcCutToFolderAndFile(CStringW full, WCHAR cut = '\\');

std::tuple<CStringW, CStringW, CStringW> UcCutToTwoFolderAndFile(CStringW full, WCHAR cut = '\\');

std::vector<CStringW> UcCutPath(CStringW full, int n = 0, WCHAR cut = '\\');
std::tuple<CStringW, CStringW, CStringW> UcCutFile(CStringW full, WCHAR cut = '\\');

CString UcGetModulePath(BOOL bPathOnly = FALSE);

#include <sstream>
#include <vector>
class UcVersion {
public:
	int _major{ 0 };
	int _minor{ 0 };
	int _build{ 0 };
	int _revision{ 0 };

	// 문자열로부터 버전을 파싱하여 객체 생성
	explicit UcVersion(LPCWSTR versionStr) {
		std::wstringstream ss(versionStr);
		wchar_t dot;
		ss >> _major >> dot >> _minor >> dot >> _build >> dot >> _revision;
	}

	// 두 버전을 비교하는 함수
	bool operator<(const UcVersion& other) const {
		if (_major != other._major) return _major < other._major;
		if (_minor != other._minor) return _minor < other._minor;
		if (_build != other._build) return _build < other._build;
		return _revision < other._revision;
	}
	bool operator>= (const UcVersion& other) const {
		return !operator<(other);
	}
	bool operator==(const UcVersion& other) const {
		return _major == other._major && _minor == other._minor && _build == other._build && _revision == other._revision;
	}
	bool operator<= (const UcVersion& other) const {
		return operator<(other) || operator==(other);
	}
	bool operator> (const UcVersion& other) const {
		return !operator<=(other);
	}

};


#ifdef KGLOBALVAR
#error "KGlobalVar is already defined."
#else
#define KGLOBALVAR
/// <summary>
/// KGlobalVar is moved to UcTool.h
/// UCDBG__.h 와 DWK__.h 에 중복선언됨.
/// </summary>
/// <typeparam name="TVAL"></typeparam>
template<typename TVAL>
class KGlobalVar {
public:
	HANDLE _hFileMappingCreated = 0;
	HANDLE _hFileMapping = 0;
	CStringW _key;
	explicit KGlobalVar(LPCWSTR key, std::function<void(PVOID)> fncInit = NULL) : _key(key)
	{
		for (int i = 0; i < 2; i++)
		{
			if (_hFileMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, _key))
			{
				if (PVOID pShared = (PVOID)MapViewOfFile(_hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0))
					UnmapViewOfFile(pShared);
				CloseHandle(_hFileMapping);
				break;
			}
			else
			{
				if (_hFileMappingCreated
					= CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TVAL), _key))
				{
					if (PVOID pShared = (PVOID)MapViewOfFile(_hFileMappingCreated, FILE_MAP_ALL_ACCESS, 0, 0, 0))
					{
						TVAL* pSharedVal = reinterpret_cast<TVAL*>(pShared);
						if (fncInit)
							fncInit(pShared);
						else
							*pSharedVal = (TVAL)0;
						break;
					}
				}
			}
		}// while
	}
	TVAL ModifyVal(std::function<void(PVOID)> fnc)
	{
		TVAL val;
		if (_hFileMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, _key))
		{
			if (PVOID pShared = (PVOID)MapViewOfFile(_hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0))
			{
				if (fnc)
					fnc(pShared);
				val = *(reinterpret_cast<TVAL*>(pShared));
				UnmapViewOfFile(pShared);
			}
			CloseHandle(_hFileMapping);
		}
		return val;
	}
protected:
	TVAL getVal() {
		return ModifyVal([](PVOID pval) {});
	}
};

template<typename TNum>
class KGlobalNum : public KGlobalVar<TNum>
{
public:
	explicit KGlobalNum(LPCWSTR key)
		: KGlobalVar<TNum>(key, [](PVOID pv) {
#ifdef _DEBUG
		TNum* pvi = reinterpret_cast<TNum*>(pv);
#endif // _DEBUG
		* (reinterpret_cast<TNum*>(pv)) = 0;
	})
	{
	}
	TNum incVal()
	{
		return ModifyVal([](PVOID pval) {
#ifdef _DEBUG
			auto& i = *(reinterpret_cast<TNum*>(pval));
#endif // _DEBUG
			(*(reinterpret_cast<TNum*>(pval)))++;
		});
	}
	TNum decVal()
	{
		return ModifyVal([](PVOID pval) {
#ifdef _DEBUG
			auto& i = *(reinterpret_cast<TNum*>(pval));
#endif // _DEBUG
			(*(reinterpret_cast<TNum*>(pval)))--;
		});
	}
	TNum get()
	{
		return (TNum)this->getVal();
		////return (TNum)getVal(); // 이건 왜 빌드에러나지? error C3861: 'getVal': identifier not found
	}
	void set(TNum nv)
	{
		ModifyVal([nv](PVOID pval) { (*(reinterpret_cast<TNum*>(pval))) = nv; });
	}
};
class KGlobalInt64 : public KGlobalNum<__int64>
{
public:
	explicit KGlobalInt64(LPCWSTR key) : KGlobalNum<__int64>(key) {}
#ifdef _sample__
	KGlobalInt64 shSrl(L"UCD__Serial");
	shSrl.incVal();
#endif // _sample__

};
class KGlobalDword : public KGlobalNum<DWORD>
{
public:
	explicit KGlobalDword(LPCWSTR key) : KGlobalNum<DWORD>(key) {}
#ifdef _sample__
	KGlobalDword UCD__MainThread(L"UCD__MainThread");
	auto idth = UCD__MainThread.get();
#endif // _sample__
};
#endif


class UcCpuBusy
{
public:
	ULARGE_INTEGER lastIdleTime{ 0 }, lastKernelTime{ 0 }, lastUserTime{ 0 };
	bool _bInit{ false };
	void initSystemCPUUsage();
	double getSystemCPUUsage();
};

std::shared_ptr<std::tuple<CString, CString, CString>> UcGetMyComputerInfo(bool bIP = true, bool bDefault = false);

template <typename TCH>
int UcCutStrByChar(TCH c, TCString<TCH> s, std::function<void(const TCH*)> cb, bool bIgnorFirst = false, bool bIgnorLast = false)
{
	if (s.IsEmpty())
		return 0;

	int i0 = 0, i1 = -1;
	int n = 0;
	for (int i = 0;; i++)
	{
		i1 = s.Find(c, i0);
		if (i1 >= 0)
		{
			if (i1 == 0)
			{
				if (!bIgnorFirst)
				{
					cb(s.Mid(0, 0));
					n++;
				}
			}
			else
			{
				cb(s.Mid(i0, i1 - i0));
				n++;
			}
			i0 = i1 + 1;
		}
		else
		{
			if (i0 < s.GetLength())
			{
				if (!bIgnorLast)
				{
					cb(s.Mid(i0));
					n++;
				}
			}
			break;
		}
	}
	return n;
}

template <typename TCH>
int UcCutStrByChar(TCH c, TCString<TCH> s, std::vector<CString>& ar, bool bIgnorFirst = false, bool bIgnorLast = false)
{
	return UcCutStrByChar(c, s, [&ar](const TCH* str) {
		ar.push_back(str);
	}, bIgnorFirst, bIgnorLast);
}


CString UcCutStrByChar(CString src, TCHAR cCut, int nStr);


//void UcCutByToken(LPCTSTR psSrc, LPCTSTR seps, std::function<void(LPCTSTR)> cb, bool bTrim = false);
template <typename TCH>
void UcCutByToken(const TCH* psSrc, const TCH* seps, std::vector<CString>& ar, bool bTrim = false)
{
	UcCutByTokenT(psSrc, seps, [&ar](auto str) {
		ar.push_back(str);
	}, bTrim);
}

template <typename TCH>
void UcCutByToken(const TCH* psSrc, const TCH* seps, std::vector<int>& ar, bool bTrim = false)
{
	UcCutByTokenT(psSrc, seps, [&ar](auto str) {
		try {
			int i = std::stoi(str);
			ar.push_back(i);
		}
		catch (const std::invalid_argument&) {
			// 잘못된 문자가 포함된 경우 무시
		}
		catch (const std::out_of_range&) {
			// 범위를 벗어난 값인 경우 무시
		}
	}, bTrim);
}

UCTOOLDYNAMIC
void UcCutByTokenW(LPCWSTR psSrc, LPCWSTR seps, std::vector<wstring>& ars, bool bTrim = false);

template <typename TCH>
std::vector<CString> UcCutByToken(const TCH* psSrc, const TCH* seps, bool bTrim = false)
{
	std::vector<CString> ar;
	UcCutByToken(psSrc, seps, ar, bTrim);
	return ar;
}

//void UcCutByTokenA(LPCSTR psSrc, LPCSTR seps, std::function<void(LPCSTR)> cb, bool bTrim = false);
UCTOOLDYNAMIC
void UcCutByTokenA(LPCSTR src, LPCSTR dl, std::vector<std::string>& ar, bool bTrim = false);

//void UcCutByToken(LPCWSTR psSrc, LPCWSTR seps, CStringArray& ars, bool bTrim = false);
//void UcCutByToken(LPCWSTR psSrc, LPCWSTR seps, std::vector<std::wstring>& ar, bool bTrim = false);

//inline int UcCutByTokenA(LPCSTR psrc, std::string dl, std::vector<std::string>& ar, int offset = 0) {
//	//std::string src = psrc;
//	return UcCutByTokenA(string(psrc), dl, ar, offset);
//}

//dwk: 2025-02-24 15:10 svnLog 949 code test 
// 1. 원본 템플릿 선언 (몸체 없음)
template <typename TCH>
const TCH* UcFindChar(const TCH* str, TCH ch);
// 아래 두타입에 대해서만 특수화 함
template <>
inline const char* UcFindChar(const char* str, char ch) {
	return strchr(str, ch);
}
template <>
inline const wchar_t* UcFindChar(const wchar_t* str, wchar_t ch) {
	return wcschr(str, ch);
}

template <typename TCH, typename Callback>
void UcCutByTokenT(const TCH* pSrc, const TCH* seps, Callback cb, bool bTrim = false)
{
	if (!pSrc || !seps)
		return;

	const TCH* p = pSrc;
	while (*p) {
		const TCH* start = p;
		while (*p && !UcFindChar(seps, *p))
			++p;// 구분자 아닐 때까지 진행
		if (start != p) {
			TCString<TCH> token(start, (int)(p - start));
			if (bTrim)
				token.Trim();
			cb(token.GetString());
		}
		while (*p && UcFindChar(seps, *p))
			++p;// 구분자 스킵
	}
}

//dwk: 2025-08-19 11:26 
//std::vector<int> UcCutByTokenInt(LPCTSTR psSrc, LPCTSTR seps, bool bTrim = false);
//std::vector<double> UcCutByTokenDouble(LPCTSTR psSrc, LPCTSTR seps, bool bTrim = false);
template <typename TCH>
std::vector<int> UcCutByTokenInt(const TCH* psSrc, const TCH* seps, bool bTrim)
{
	std::vector<int> ars;
	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
		ars.push_back(UcAtoi(str));//token.GetString()
	}, bTrim);
	return ars;
}

template <typename TCH>
std::vector<double> UcCutByTokenDouble(const TCH* psSrc, const TCH* seps, bool bTrim)
{
	std::vector<double> ars;
	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
		ars.push_back(UcAtof(str));//token.GetString()
	}, bTrim);
	return ars;
}


bool UcIsProcessWithArgsRunning(const std::wstring processName, const std::wstring args);

LONG UcDeleteKeyWithSubkeys(HKEY hKeyRoot, CString sSubKey);


template <typename TFNC>
HANDLE UcCreateThread(TFNC fnc, DWORD attr = 0)
{
	// std::function을 저장할 포인터
	auto pFunc = new std::function<int()>(fnc);
	// 스레드 시작 루틴
	LPTHREAD_START_ROUTINE pSR = [](LPVOID lpParam) -> DWORD {
		auto pFunc2 = reinterpret_cast<std::function<int()>*>(lpParam);
		KAtEnd _delFunc([pFunc2]() {delete pFunc2; });// 사용 후 메모리 해제 예약
		return (*pFunc2)();//함수실행
	};
	// 스레드 생성
	return CreateThread(NULL, 0, pSR, pFunc, attr, NULL);
}


class FFmt {
public:
	FFmt(double dv, int point)
		: _dv(dv), _point(point)
	{
	}
	double _dv;
	int _point;
};

class CUcTrace
{
	// #include <tuple>
	// #include <functional>
	// #include <string>
	// #include <sstream>
	// #include <memory>
public:

	CUcTrace(std::function<void(LPCWSTR)> fnc = nullptr, bool bEndCrlf = false)//, CStringW sToOutput = L"\0")
		: _fncTrace(fnc), _bEndCrlf(bEndCrlf)//, _sToOutput(sToOutput)
	{
	}
	std::wstringstream _s;

	std::wstring str()
	{
		return _s.str();
	}
	CStringW _sToOutput{ L"\r\n" };//이 문자가 오면 끝낸다.

	bool _bEndCrlf{ false };//참이면 끝날때 CRLF 를 붙인다.
	std::function<void(LPCWSTR str)> _fncTrace;

	//void SetTrace(std::function<void(std::wstring)> fnc)//?ExTrace 5 CUcTrace::fnc 에 저장
	//{
	//	if (!_fncTrace)
	//		_fncTrace = fnc;
	//}

	CCriticalSection _csTrace;
	void Output(const wchar_t* txt)
	{
		CSingleLock _lock__(&_csTrace, TRUE);
		if (tchcmp(txt, (PWS)_sToOutput) == 0)
		{
			if (_bEndCrlf)
				_s << L"\r\n";// txt;// L"\r\n" 를 뒤에

			OutputEnd();
		}
		else // 계속 덛붙이기만 한다.
			_s << txt;
	}
	void OutputEnd()
	{
		if (_fncTrace)
			_fncTrace(_s.str().c_str());
		else
			OutputDebugStringW(_s.str().c_str());
		_s.str(L"");// = "";//.clear(); 이게 말을 안듣네.
	}

	CUcTrace& operator<<(const char* ctr)
	{
		Output(CStringW(ctr));
		return *this;
	}
	CUcTrace& operator<<(const wchar_t* ctr)
	{
		Output((ctr));
		return *this;
	}
	CUcTrace& operator<<(const CStringA& ctr)
	{
		CStringW ctra(ctr);
		Output((LPCWSTR)ctra);
		return *this;
	}
	CUcTrace& operator<<(const CStringW& ctr)
	{
		//CStringA ctra(ctr);
		Output((LPCWSTR)ctr);
		return *this;
	}
	CUcTrace& operator<<(const std::wstring& ctr)
	{
		//CStringA ctra(ctr.c_str());
		Output(ctr.c_str());
		return *this;
	}
	CUcTrace& operator<<(const std::string& ctr) {
		CStringW ctra(ctr.c_str());
		Output((PWS)ctra);
		return *this;
	}
	CUcTrace& operator<<(const std::stringstream& ctr) {
		CStringW ctra(ctr.str().c_str());
		Output(ctra);
		return *this;
	}
	CUcTrace& operator<<(const std::wstringstream& ctr) {
		Output(ctr.str().c_str());
		return *this;
	}
	/// int도 되는데, 왜 막아두었지? 아~ 아래 template
	CUcTrace& operator<<(int ctr) {
		wchar_t buf[100];
		_itow_s(ctr, buf, 10);//10은 10진수다. 버퍼크기가 아니다.
		Output(buf);
		return *this;
	}
	CUcTrace& operator<<(__int64 ctr) {
		auto str = std::to_wstring(ctr);
		Output(str.c_str());
		return *this;
	}
	CUcTrace& operator<<(unsigned long long ctr) {
		auto str = std::to_wstring(ctr);
		Output(str.c_str());
		return *this;
	}
	CUcTrace& operator<<(float ctr) {
		auto str = std::to_wstring(ctr);
		Output(str.c_str());
		return *this;
	}
	CUcTrace& operator<<(HANDLE ctr) {
		CStringW f; f.Format(L"%llX", (INT64)ctr);
		Output(f);
		return *this;
	}

	/// 소수점 아래 수지정은 double형에만 적용 된다. 
	/// float는 기본 6자리 까지 나온다.
	/// 기본적으로 반올림 된다.
	/// 개별적으로 소수점 자리를 지정 하려면 << tuple<double,int>(dval, 5)를 사용 한다. 5는 decimal pointer 값이다.
	int _decimalPoint{ 6 };

	///ex: << tuple<double,int>(3.123456789, 4) <<
	///ex: << {3.123456789, 4} <<
	///output: 3.1235 // 반올림이 자동으로 되네
	//CUcTrace& operator<<(std::tuple<double, int> ctr) {
	CUcTrace& operator<<(std::initializer_list<double> ctr) {
		//auto& [dv, point] = ctr;
		vector<double> vctr = ctr;
		auto dv = vctr[0];
		auto point = (int)vctr[1];
		CStringW fmt; fmt.Format(L"%%.%df", point);
		wchar_t buf[100];
		//wsprintf_s(buf, (LPCSTR)fmt, dv);
		swprintf_s(buf, sizeof(buf) / sizeof(wchar_t), (LPCWSTR)fmt, dv);

		Output(buf);
		return *this;
	}
	CUcTrace& operator<<(FFmt ctr) {
		auto dv = ctr._dv;
		auto point = ctr._point;
		CStringW fmt; fmt.Format(L"%%.%df", point);
		wchar_t buf[100];
		swprintf_s(buf, sizeof(buf) / sizeof(wchar_t), (LPCWSTR)fmt, dv);
		Output(buf);
		return *this;
	}
	/// 기본으로 실수는 소숫점 6자리까지 된다.
	// template 로 되지만 _decimalPoint 때문에 만들어 준다.
	CUcTrace& operator<<(double ctr) {
		return operator<<(FFmt{ ctr, _decimalPoint });// ,6
		//return operator<<({ ctr, (double)_decimalPoint });// ,6
		//return operator<<(tuple<double, int>(ctr, _decimalPoint));// ,6
	}

	/// 마지막으로 std::endl 가 오면 이게 불려진다.
	CUcTrace& operator<<(std::wostream& (*manip)(std::wostream&)) {
		//manip(std::wcout);
		Output(_sToOutput);// L"\r\n");// str.c_str());
		return *this;
	}
};

#ifdef _samples__
CUcTrace std_cerr1([](auto s) { TRACE(s); }, true);
std_cerr1 << L"this is test " << 123 << " asicii " << 0.234 << L" 한글 " << FFmt{ 0.234567, 4 } << std::endl;
#endif // _samples__

int UcRegistryRecursiveGeneral2Level(HKEY hKeyParent, LPCTSTR lpszKeyName
	, function<void(CRegKey&)> cbRoot
	, function<int(CRegKey&, CString)> cbSub, REGSAM sam = KEY_READ | KEY_WRITE);

/// 특정 레지스트리 키아래 값을 루프로 돌면서 람다 함수를 부른다.
void UcSaveRegistryKeyValuesLoop(CRegKey& regKey, function<int(int, LPCTSTR, BYTE*, DWORD)> cb);


bool UcIsValidGUID(LPCWSTR guid);


/// <summary>
/// 파일에서 읽은 데이타가 UTF16 Big Endian 인지 Little Endian인지 확인 한다.
/// </summary>
/// <param name="pData"></param>
/// <param name="bBE">TRUE:Big Endian, FALSE:Little Endian</param>
/// <returns>TRUE:UTF-16(UNICODE)</returns>
/// Byte Order Mark (BOM) 확인: UTF-16 LE(code page 12000) 파일은 보통 파일 시작 부분에 FF FE의 BOM을 가집니다.
/// Byte Order Mark (BOM) 확인: UTF-16 BE(code page 12001) 파일은 보통 파일 시작 부분에 FE FF의 BOM을 가집니다.
inline BOOL UcIsUicodeFileData(const char* p, int& bLE)
{
	if (p == NULL)
		return FALSE;
	BOOL bUni = FALSE;
	if (p[0] != '\0' && p[1] != '\0')
	{
		int bLE1 = (p[0] == '\xff' && p[1] == '\xfe') ? 1 : 0;
		bUni = bLE1 || (p[0] == '\xfe' && p[1] == '\xff');//0xfe 는 int형, 캐스트(char)완전 중요
		if (bUni)
			bLE = bLE1;
	}
	return bUni;
}
void UcSwapBytes(char* p, int length);

std::map<string, int> UcContainsHowMany(const char* pA, const std::vector<std::string>& strings, UINT_PTR len);

//#define _Use_WSH__

#ifdef _Use_WSH__
CStringW UcExecuteScript(const wchar_t* script, const wchar_t* language = L"JScript");
#endif // _Use_WSH__

#define CStringFORMAT(buffer, fmt) CStringW buffer;\
do {ATLASSERT(AtlIsValidString(fmt));\
	va_list args;\
	va_start(args, fmt);\
	buffer.FormatV(fmt, args);\
	va_end(args);}while(0)

//template<typename TCString>
template<typename TChar>
void UcCutPathEnd(TCString<TChar>& sPath)
{
	if (sPath.GetAt(sPath.GetLength() - 1) == '\\')
		sPath.Truncate(sPath.GetLength() - 1);
	//sPath = sPath.Left(sPath.GetLength() - 1);
}
template<typename TChar>
void UcCutPathEnd(tstring<TChar>& sPath)
{
	if (sPath[sPath.length() - 1] == '\\')
		sPath.erase(sPath.size() - 1);
}



#ifdef _dwk_try4__

/// <summary>
/// CComQIPtr의 템플릿 인자는 하나만 사용해도 충분히 동작합니다.
/// 두 번째 템플릿 인자는 기본값으로& __uuidof(Interface)가 설정되어 있으며, 생략 가능하게 설계되어 있습니다.
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class UcComQIPtr : public CComQIPtr<T> {
public:
	HRESULT _hr{ E_POINTER }; // HRESULT를 저장

	UcComQIPtr(IUnknown* p) {
		if (p) {
			_hr = p->QueryInterface(__uuidof(T), (void**)&this->p);
			if (FAILED(_hr)) {
				this->p = nullptr; // 실패 시 nullptr로 초기화
			}
		}
	}
	// CComPtr 또는 IUnknown* 할당
	UcComQIPtr& operator=(IUnknown* p) {
		this->Release(); // 기존 객체 해제
		_hr = E_POINTER;

		if (p) {
			_hr = p->QueryInterface(__uuidof(T), (void**)&this->p);
			if (FAILED(_hr)) {
				this->p = nullptr;
			}
		}
		return *this;
	}

	// 기존 CComQIPtr 또는 CComQIPtrWithHRESULT와 호환
	template <typename Q>
	UcComQIPtr& operator=(const CComQIPtr<Q>& other) {
		return operator=(other.p);
	}
	template <typename Q>
	UcComQIPtr& operator=(const CComPtr<Q>& other) {
		return operator=(other.p);
	}
	HRESULT GetHRESULT() const {
		return _hr;
	}
	//bool Failed() {	return FAILED(_hr);}
	//bool Succed() {	return SUCCEEDED(_hr);}
};

#ifdef _Sample__
// 사용 예제
CComPtr<IActiveScript> spActiveScript = ...;
UcComQIPtr<IActiveScriptParse> spActiveScriptParse = spActiveScript;

if (spActiveScriptParse) {
	// 성공
	spActiveScriptParse->ParseScriptText(...);
}
else {
	// 실패 시 HRESULT 확인
	HRESULT hr = spActiveScriptParse.GetHRESULT();
	if (FAILED(hr)) {
		// 실패 원인 처리
	}
}
#endif // _Sample__
#endif // _dwk_try4__


std::tuple<CString, CString, CString, CString> UcSplitPath(LPCTSTR sFull);

bool UcIsValidFileName(const TCHAR * filename, bool bSpaseALlso = false);

int UcRunBatchFileAndWait(LPCTSTR batchFilePath, std::function<void()> onSuccess);//dwk: 2025-01-24 09:26  


#include <map>
#include <functional>
#include <typeinfo>
#include <stdexcept>
#include "UcDebug.h"
#include <memory>//singletone 0 include
#include <mutex>

#define GSINGLETON

/// ///////////////////  첫번째 방법 ///////////////////////

/// 1. template 함수
/// 2. 부르는 순간 singleton이 된다.
/// 3. 반드시 한 class에 앱 전체에 하나만 가진다.
/// 사용자 정의 class와 typedef 된 객체인 경우 전역 템플릿 방식으로(template이므로 inline 할 필요 없다.)
template<typename TCL, typename... Args>
shared_ptr<TCL> GetSimpleInstance(Args&&... args) {
#ifdef _DEBUG
	/// singleton 하나 생성 중에 동일 singleton을 또 호출하는 것을 잡아내기 위해 넣은 코드
	std::string tName = typeid(TCL).name();
	static std::set<std::string> setGlobal;
	auto its = setGlobal.find(tName);
	if (its == setGlobal.end()) {
#if CPP17_OR_LATER
		auto[it1, bOk] = setGlobal.insert(tName);
#else
		auto insert_result = setGlobal.insert(tName);
		auto it1 = insert_result.first;
		bool bOk = insert_result.second;
#endif
		its = it1;
	}
	else {
		ASSERT(0);
		throw std::exception("recursive call");
	}
#endif
	static shared_ptr<TCL> instance_;
	static std::once_flag initFlag_;
	std::call_once(initFlag_, []() {
		instance_ = std::make_shared<TCL>(std::forward<Args>(args)...);
	});
#ifdef _DEBUG
	TRACE("GetSimpleInstance(%s)\n", tName.c_str());
	setGlobal.erase(its);
#endif
	return instance_;
}
///주의: 참조형 리턴을 따로 만들면, 각각 instance가 생겨 버린다.
//사용예 1: GetSimpleInstance<CGlobalTest>()
//사용예 2: 참조로 쓸경우   (*GetSimpleInstance<CGlobalTest>())
//사용예 3: 매크로로 쓸경우  #define gfxTest (*GetSimpleInstance<CGlobalTest>())
template<typename TCL, typename... Args>
shared_ptr<TCL> GetSimpleInstanceClean(Args&&... args) {
	static shared_ptr<TCL> instance_;
	static std::once_flag initFlag_;
	std::call_once(initFlag_, [&]() {
		instance_ = std::make_shared<TCL>(std::forward<Args>(args)...);
	});
	return instance_;
}


/// ///////////////////  두번째 방법 ///////////////////////
/// GSingletonUniqu<MyCla1>::GetInstance() 처럼 그냥 소비하면 만들어 진다.
/// class type 당 하나만 가능 하다. std::map 를 typedef한 객체 같은 경우는 안된다.
/// deprecated : 대신에 GSingleton 를 사용한다.
template<typename TCL>
class GSingletonUniqu {
public:
	static shared_ptr<TCL> GetInstance() {
#ifdef _DEBUG
		std::string tName = typeid(TCL).name();
		static std::set<std::string> setGlobal;
		auto its = setGlobal.find(tName);
		if (its == setGlobal.end()) {
#if CPP17_OR_LATER
			auto[it1, bOk] = setGlobal.insert(tName);
#else
			auto insert_result = setGlobal.insert(tName);
			auto it1 = insert_result.first;
			bool bOk = insert_result.second;
#endif
			its = it1;
		}
		else {
			ASSERT(0);
			throw std::exception("recursive call");
		}

		std::call_once(*initFlag_, [&tName]() {// std::call_once로 초기화를 보장
			DWKFUNCV(L"<%s>", tName);
#else
		std::call_once(*initFlag_, []() {// std::call_once로 초기화를 보장
#endif
			instance_ = std::make_shared<TCL>();
		});
#ifdef _DEBUG
		setGlobal.erase(its);
#endif
		return instance_;
		}
	static void ChangeInstance(shared_ptr<TCL> newInstance) {
		std::lock_guard<std::mutex> lock(mutex_);
		instance_ = std::move(newInstance);
	}
	static void ChangeInstance(TCL * newPtr, std::function<void(shared_ptr<TCL>)> cbAfter = {}) {
		std::lock_guard<std::mutex> lock(mutex_);
		instance_.reset(newPtr);
		if (cbAfter)
			cbAfter(instance_);
	}
	static void DeleteInstance() {
		std::lock_guard<std::mutex> lock(mutex_);
		instance_.reset(); // 인스턴스 해제
		initFlag_ = std::make_unique<std::once_flag>(); // 새로운 once_flag 생성
	}
private:
	INLINE_STATIC shared_ptr<TCL> instance_;
	INLINE_STATIC std::unique_ptr<std::once_flag> initFlag_;
	INLINE_STATIC std::mutex mutex_;

	// C++14 호환성을 위한 초기화
#if CPP_BEFORE_17
	static void initStaticMembers() {
		static bool initialized = false;
		if (!initialized) {
			initFlag_ = std::make_unique<std::once_flag>();
			initialized = true;
		}
	}
#endif
	};




class MyCla1 {
public:
	int _n{};
	void Test() {
		TRACE(L"dwk: %s(%d)\n", __FUNCTIONW__, _n);
	}
};
class MyCla2 {
public:
	void Test() {
		TRACE(L"+++: %s\n", __FUNCTIONW__);
	}
};
#ifdef _Samples__
GSingletonUniqu<MyCla1>::GetInstance()->Test();//MyCla1::Test(0)

auto newSh = std::make_shared<MyCla1>();
newSh->_n = 1;
GSingletonUniqu<MyCla1>::ChangeInstance(newSh);
GSingletonUniqu<MyCla1>::GetInstance()->Test();//MyCla1::Test(1)

auto newPtr = new MyCla1();
newPtr->_n = 2;
GSingletonUniqu<MyCla1>::ChangeInstance(newPtr);
GSingletonUniqu<MyCla1>::GetInstance()->Test();//MyCla1::Test(2)

GSingletonUniqu<MyCla1>::DeleteInstance();
GSingletonUniqu<MyCla1>::GetInstance()->Test();//MyCla1::Test(0)


auto sh2 = GSingletonUniqu<MyCla2>::GetInstance();
sh2->Test();
#endif // _Samples__



/// ///////////////////  세번째 방법 ///////////////////////
/// GSingleton<MyCla1>::GetInstance() 처럼 그냥 소비하면 만들어 진다.
/// class type 당 하나만 가능 하다. std::map 를 typedef한 객체 같은 경우는 안된다.
/// deprecated : 대신에 GSingleton 를 사용한다.
template<typename TCL>
class GSingleton {
public:

	static shared_ptr<TCL> GetInstance(const std::string& key = "") {//LPCSTR key = "default") {//
		std::lock_guard<std::mutex> lock(mutex_);
#ifdef _DEBUG
		std::string tName = typeid(TCL).name() + (key);
		DbgCString::ReplaceCString(tName);
#endif
		auto itf = initFlags_.find(key);
		if (itf == initFlags_.end()) {
			initFlags_[key] = std::make_unique<std::once_flag>();
		}
#ifdef _DEBUG
		static std::set<std::string> setGlobal;
		auto its = setGlobal.find(tName);
		if (its == setGlobal.end()) {
			//auto [it1, bOk] = setGlobal.insert(tName);
#if CPP17_OR_LATER
			auto[it1, bOk] = setGlobal.insert(tName);
#else
			auto insert_result = setGlobal.insert(tName);
			auto it1 = insert_result.first;
			bool bOk = insert_result.second;
#endif
			its = it1;
		}
		else {
			ASSERT(0);
			throw std::exception("recursive call");
		}
		std::call_once(*initFlags_[key], [&key, &tName]() { // std::call_once로 초기화를 보장
			DWKFUNCV(L"<%s>", tName);
#else
		std::call_once(*initFlags_[key], [&key]() { // std::call_once로 초기화를 보장
#endif
			instances_[key] = std::make_shared<TCL>();
		});
#ifdef _DEBUG
		setGlobal.erase(its);
#endif
		return instances_[key];
		}

	static void ChangeInstance(shared_ptr<TCL> newInst, std::function<void(shared_ptr<TCL>)> cbAfter = {}, const std::string & key = "") {
		std::lock_guard<std::mutex> lock(mutex_);
		instances_[key] = std::move(newInst);
#ifdef _DEBUG
		std::string tName = typeid(TCL).name() + (key);
		DbgCString::ReplaceCString(tName);
		DWKFUNCV(L"+++ GSingleton::ChangeInstance(%s) shared", tName);
#endif
		if (cbAfter)
			cbAfter(instances_[key]);
	}

	/// <summary>
	/// 주의: GetInstance 할 때 준 key 동일한 키를 주어야 한다.
	///		GetInstance_Global(TCL) 을 썼다면 키는 class명과 같다.
	/// </summary>
	/// <param name="newPtr"></param>
	/// <param name="cbAfter">인스턴스 교체 후 할일을 람다함수에 담아 보낸다.</param>
	/// <param name="key"></param>
	static void ChangeInstance(TCL * newPtr, std::function<void(shared_ptr<TCL>)> cbAfter = {}, const std::string & key = "") {
		std::lock_guard<std::mutex> lock(mutex_);
		instances_[key].reset(newPtr);
#ifdef _DEBUG
		std::string tName = typeid(TCL).name() + (key);
		DbgCString::ReplaceCString(tName);
		DWKFUNCV(L"+++ GSingleton::ChangeInstance(%s) ptr", tName);
#endif
		if (cbAfter)
			cbAfter(instances_[key]);
	}

	static void DeleteInstance(const std::string & key = "") {
		std::lock_guard<std::mutex> lock(mutex_);
		instances_.erase(key);
		initFlags_.erase(key); // 새로운 초기화를 위해 삭제
#ifdef _DEBUG
		std::string tName = typeid(TCL).name() + (key);
		DbgCString::ReplaceCString(tName);
		DWKFUNCV(L"+++ GSingleton::DeleteInstance(%s) ptr", tName);
#endif
	}

private:
	INLINE_STATIC std::map<std::string, shared_ptr<TCL>> instances_; // 키별로 인스턴스 관리
	INLINE_STATIC std::map<std::string, std::unique_ptr<std::once_flag>> initFlags_; // 키별로 초기화 플래그 관리
	//#pragma message(FILINDWK("INLINE_STATIC std::mutex GSingleton<TCL>::mutex_ declared."))
	INLINE_STATIC std::mutex mutex_; // 멀티스레드 안전성 보장

	// C++14 호환성을 위한 초기화
#if CPP_BEFORE_17
	static void initStaticMembers() {
		static bool initialized = false;
		if (!initialized) {
			// static 멤버들은 자동으로 초기화됨
			initialized = true;
		}
	}
#endif
	};

#if CPP_BEFORE_17
// GSingleton 템플릿 클래스의 static 멤버 정의 (C++14용)
template<typename TCL>
std::map<std::string, std::shared_ptr<TCL>> GSingleton<TCL>::instances_;

template<typename TCL>
std::map<std::string, std::unique_ptr<std::once_flag>> GSingleton<TCL>::initFlags_;

//#pragma message(FILINDWK("GSingleton<TCL>::mutex_ defined."))
template<typename TCL>
std::mutex GSingleton<TCL>::mutex_;
#else
// C++17에서는 헤더에서 직접 정의 (템플릿은 기본적으로 inline)
//template<typename TCL>
//std::map<std::string, std::shared_ptr<TCL>> GSingleton<TCL>::instances_{};
//
//template<typename TCL>
//std::map<std::string, std::unique_ptr<std::once_flag>> GSingleton<TCL>::initFlags_{};
//
//template<typename TCL>
//std::mutex GSingleton<TCL>::mutex_{};
#endif //CPP_BEFORE_17
#ifdef _Samples__
GSingleton<MyCla1>::GetInstance()->Test();//MyCla1::Test(0)

auto newSh = std::make_shared<MyCla1>();
newSh->_n = 1;
GSingleton<MyCla1>::ChangeInstance(newSh);
GSingleton<MyCla1>::GetInstance()->Test();//MyCla1::Test(1)

auto newPtr = new MyCla1();
newPtr->_n = 2;
GSingleton<MyCla1>::ChangeInstance(newPtr);
GSingleton<MyCla1>::GetInstance("myClass")->Test();//MyCla1::Test(2)

GSingleton<MyCla1>::DeleteInstance("myClass");
GSingleton<MyCla1>::GetInstance("myClass")->Test();//MyCla1::Test(0)
GSingleton<MyCla1>::GetInstance("myKey")->Test();//MyCla1::Test(0)


auto sh2 = GSingleton<MyCla2>::GetInstance();
sh2->Test();
#endif // _Samples__

/// 2. 기존에 이미 ClassName::GetInstance() 방식인 경우 기존코드 호환을 위해 static member 함수로 유지
// shared_ptr로 리턴(포인터처럼 사용)
#define GetInstance_Global(TCL) static shared_ptr<TCL> GetInstance()\
									{ return GSingleton<TCL>::GetInstance(#TCL);}
#define GetInstance_Global_NoKey(TCL) static shared_ptr<TCL> GetInstance()\
									{ return GSingleton<TCL>::GetInstance();}

// 참조:static local변수는 한번만 초기화 됨
#define SetSINGLETON(TCL) static shared_ptr<TCL> GetInstance()\
									{ static auto shp = std::make_shared<TCL>(); return shp;}





#ifdef _DEBUGx

#include <map>
#include <memory>
#include <mutex>
#include <cassert>
//#include <iostream>

//[[deprecated]]
class UcParamMap {
private:
	std::map < LPVOID, shared_ptr<std_any> _registry;
	std::mutex _mtx;
	INT_PTR _currentKey = 0;
	const std::size_t _maxSize = 5; // 최대 크기 제한

public:
	// 키 생성 및 포인터 등록
	template <typename T>
	LPVOID RegisterParam(shared_ptr<T> ptr) {
		std::lock_guard<std::mutex> lock(_mtx);
		LPVOID key = (LPVOID)(++_currentKey);
		registry[key] = std::static_pointer_cast<void>(ptr);
		ASSERT(registry.size() <= maxSize);
		//"Warning: PointerRegistry size exceeds maximum limit: "	<< registry.size()
		return key;
	}

	// 키로 포인터 가져오고 제거
	template <typename T>
	shared_ptr<T> Take(LPVOID key) {
		std::lock_guard<std::mutex> lock(mtx);
		auto it = registry.find(key);
		if (it != registry.end()) {
			auto ptr = std::static_pointer_cast<T>(it->second);
			registry.erase(it);
			return ptr;
		}
		return nullptr;
	}

	//std::size_t Size() const {
	//	std::lock_guard<std::mutex> lock(mtx);
	//	return registry.size();
	//}
};
#define ParamByShObj(shpr) GSingleton<UcParamMap>::GetInstance()->RegisterParam(shpr)
#define ShObjByData(TYPE, lprm) GSingleton<UcParamMap>::GetInstance()->Take<TYPE>((LPVOID)lprm)
#endif // _DEBUGx


#include <map>
#include <memory>
#include <mutex>
#include <cassert>
//#include <iostream>
class UcSharedPtrTool {
private:
	std::map<INT_PTR, shared_ptr<std_any>> _sharedPtrMap;
	std::mutex _mapMutex;
	INT_PTR _nextKey = 0;

	INT_PTR generateKey() {
		return ++_nextKey;
	}

public:
	template<typename T>
	INT_PTR StoreShared(const shared_ptr<T>& shPtr) {
		std::lock_guard<std::mutex> lock(_mapMutex);
		INT_PTR key = generateKey();
		_sharedPtrMap[key] = std::make_shared<std_any>(shPtr);
		/// ptr > shPtr > any > shAny
		return key;/// shPtr도 shared_ptr인데, 이걸 또 any 싸고, 또 shared_ptr로 또 싼다.
	}

	template<typename T>
	shared_ptr<T> RetrieveAndRemove(INT_PTR key) {
		std::lock_guard<std::mutex> lock(_mapMutex);
		auto it = _sharedPtrMap.find(key);
		if (it != _sharedPtrMap.end()) {
			shared_ptr<T> ptr = std_any_cast<shared_ptr<T>>(*(it->second));
			_sharedPtrMap.erase(it);
			return ptr;
		}
		return nullptr;
	}

	void clear() {
		std::lock_guard<std::mutex> lock(_mapMutex);
		_sharedPtrMap.clear();
	}
};

#define ParamByShObj(shpr) (LPVOID)GSingleton<UcSharedPtrTool>::GetInstance()->StoreShared(shpr)
///주의: TYPE 안에 ','가 있으면 안된다. 매개변수 구분으로 인식해 버린다. using으로 단일하게 써야
/// 리턴: shared_ptr<TYPE>
/// ex: ShObjByData(MyObj2, lparam) , Bad ex: ParamOfKey(share_ptr<MyObj2>, lparam)
#define ShObjByData(TYPE, lparam) GSingleton<UcSharedPtrTool>::GetInstance()->RetrieveAndRemove<TYPE>((INT_PTR)(lparam))

#ifdef _Sample__
class MyObj2 {
public:
	int x{ 1 };
	int y{ 2 };
};
DWORD WINAPI MyThread1(LPVOID lparam) {
	auto sh = ShObjByData(MyObj2, lparam);///주의 shared_ptr<MyObj2> 하지 않는다.
	TRACE(L"%d %d\n", sh->x, sh->y);
	return 0;
}
void NvBuilder::Test1()
{
	DWORD dwThreadId{};
	HANDLE hThread{};
	auto shrPoint = make_shared<MyObj2>();
	shrPoint->x = 10;
	shrPoint->y = 20;
	auto key = ParamByShObj(shrPoint);
	hThread = CreateThread(NULL, NULL, MyThread1, key, 0, &dwThreadId);
	// 비동기 함수에 객체 전달을 위해 등록 한 후, 비동기 함수에 전달된 키로 다시 빼내온다.
	// 반드시 shared_ptr 로 싸서 보내고, 한번 빼내면 다시 못빼니 반드시 변수로 받아서 쓴다.
	// 빼내온 객체의 메모리 해제는 신경쓰지 않아도 된다.
	auto key2 = ParamByShObj(shrPoint);
	std::thread([key2]() {
		MyThread1(key2);
	}).detach();
}
#endif // _Sample__

void UcTest1();
void UcTest2();
void UcTest3();
void UcTest4();

UCTOOLDYNAMIC
bool UcIsUTF8String(const char* str, size_t length);

CStringA UcReadSmallTextFile(PWS fileName);
CStringW UcReadSmallTextFileW(PWS fileName);

UCTOOLDYNAMIC
CStringW UcReadTextFileAnyEncoding(PWS fileName);

UCTOOLDYNAMIC
wstring UcHinstanceErrStr(HINSTANCE hInst);

//void UcAppendTextToFile(const CStringW filePath, const CStringW textToAppend);

UCTOOLDYNAMIC
void UcAppendTextToFile_UTF16_BOM(const CString& filePath, const CStringW& textToAppend);

UCTOOLDYNAMIC
void UcAppendTextToFile_UTF8_BOM(const CString& filePath, const CStringW& textToAppend);


/// #define _UseLegacyThread_ //dwk: 2025-01-17 09:55 
inline CStringW UcHError2Str(HRESULT hr) {//dwk
	CStringW msg;// [MAX_PATH * 5] ;
	FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetScode(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		msg.GetBuffer(MAX_PATH), MAX_PATH, NULL);
	msg.ReleaseBuffer();
	return std::move(msg);
}

inline std::tuple<CString, CString, CString, CString> UcSplitpath(LPCTSTR fullPath)//, wchar_t* _Drive, wchar_t* _Dir, wchar_t* _Filename, wchar_t* _Ext)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	//void __cdecl _wsplitpath(wchar_t const* _FullPath, wchar_t* _Drive, wchar_t* _Dir, wchar_t* _Filename, wchar_t* _Ext)
	_tsplitpath_s(fullPath, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	// 	_tsplitpath_s(filePath, drive, dir, fname, ext);
	return make_tuple(drive, dir, fname, ext);
}

#define _TSPLITPATH(pathname) _tsplitpath_s(pathname, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT)

inline void UcSplitpath(LPCTSTR fullPath, CString& drive, CString& dir, CString& fname, CString& ext)
{
	_tsplitpath_s(fullPath, drive.GetBuffer(_MAX_DRIVE), _MAX_DRIVE, dir.GetBuffer(_MAX_DIR), _MAX_DIR, fname.GetBuffer(_MAX_FNAME), _MAX_FNAME, ext.GetBuffer(_MAX_EXT), _MAX_EXT);
	drive.ReleaseBuffer();
	dir.ReleaseBuffer();
	fname.ReleaseBuffer();
	ext.ReleaseBuffer();
}

inline CString _TMAKEPATH(LPCTSTR drive, LPCTSTR dir, LPCTSTR fname, LPCTSTR ext)
{
	CString sr;
	_tmakepath_s(sr.GetBuffer(_MAX_PATH), _MAX_PATH, drive, dir, fname, ext);
	sr.ReleaseBuffer();
	return sr;//dwk: 2025-01-31 09:26 C++17 이상에서는 RVO(Return Value Optimization)가 강제 적용됨 
}

#include <VersionHelpers.h>
inline BOOL UcIsNT() {
	return IsWindowsXPOrGreater();
}
#ifdef _Sample__
#ifdef _UseHideOldWindowsVersion__

#else
if (UcIsNT())
#endif // _UseHideOldWindowsVersion__

if (!UcIsNT())
return FALSE;
#endif // _Sample__


inline TCHAR* UcSTRTOK_S(TCHAR* strToken, const TCHAR* strDelimit, LPTSTR* context)
{
	return _tcstok_s(strToken, strDelimit, context);
}

template <typename TCH>
DWORD UcTokenizeString(const TCH* sStr, const TCH delimiters[], function<void(int, TCH*)> cbGet)
{
	TCH* str = const_cast<TCH*>(sStr);  // 나눌 문자열
	TCH* context = NULL;  // 각 스레드가 쓸 "자기만의 접시"
	int i = 0;
	TCH* token = UcSTRTOK_S(str, delimiters, &context);
	if (cbGet)
		cbGet(i, token);
	i++;
	while (token != NULL) {
		// 다음 토큰 가져오기
		if (token = UcSTRTOK_S(NULL, delimiters, &context)) {
			if (cbGet)
				cbGet(i, token);
			i++;
		}
		else
			break;
	}
	return i;
}


inline void UcFormat(CString& str, LPCTSTR lpszFormat, ...) {
	va_list args;
	va_start(args, lpszFormat);
	str.FormatV(lpszFormat, args);
	va_end(args);
}

inline CString UcItoS(int value, int radix = 10) {
	CString s;
	auto buffer = s.GetBuffer(512);
	_itot_s(value, buffer, 512, radix);
	s.ReleaseBuffer();
	return s;
}



//#PtrList SerializeListLambda
/// std::enable_if를 사용하여 CObject 기반 클래스만 직렬화 가능하도록 제한
/// T 가 CObject 기반 이거나 primary type 이어야 << 가 된다.
template <typename T, typename SaveFunc, typename LoadFunc,
	typename std::enable_if<std::is_base_of<CObject, T>::value>::type* = nullptr>
	void TSerializeListLambda(CArchive& ar, std::list<std::shared_ptr<T>>& list, SaveFunc cbSave, LoadFunc cbLoad)
{
	if (ar.IsStoring()) // 저장
	{
		//ar << static_cast<int>(list.size()); // 리스트 크기 저장
		ar.WriteCount(static_cast<int>(list.size()));
		for (const auto& ptr : list)
			cbSave(*ptr);//ar << *ptr; // 객체 내용 직렬화
	}
	else // 로드
	{
		list.clear(); // 기존 데이터 삭제
		//int count = 0;
		//ar >> count; // 리스트 크기 읽기
		auto count = ar.ReadCount();
		for (int i = 0; i < (int)count; ++i)
		{
			auto ptr = std::make_shared<T>(); // 객체 생성
			cbLoad(*ptr);//ar >> *ptr; // 객체 로드
			list.push_back(ptr); // 리스트에 추가
		}
	}
}

//#PtrList SerializeList
template <typename T, typename std::enable_if<std::is_base_of<CObject, T>::value>::type* = nullptr>
void TSerializeList(CArchive& ar, std::list<std::shared_ptr<T>>& list)
{
	TSerializeListLambda(ar, list,
		[&ar](T& obj) { obj.Serialize(ar); },  // `<<` 대신 명시적 `Serialize()` 호출
		[&ar](T& obj) { obj.Serialize(ar); }
	);
}
#ifdef _Sample__
std::list<shared_ptr<CFOBaseProperties>> m_propList;
virtual void Serialize(CArchive& ar)
{
	CObject::Serialize(ar);
	SerializeList(ar, m_propList); // 공통 템플릿 함수 사용!
}
#endif // _Sample__

// _UseBaseOne_
//template<typename T>
//inline void KSharedPtrList<T>::Serialize(CArchive& ar) {
//	TSerializeList(ar, *this);
//}



//#PtrList KSharedObjList
//[[deprecated]]
template <typename T, typename std::enable_if<std::is_base_of<CObject, T>::value>::type* = nullptr>
class KSharedObjList : public KList<shared_ptr<T>>, public CObject
	//class KSharedObjList : public KSharedPtrList<T>, public CObject
{
public:
	//DECLARE_SERIAL(KSharedObjList) 계승 받은 객체에 해줘야 함.
	/// IMPLEMENT_SERIAL는 T가 정해 졌을때 해준다.
	/// 예: class CParamObjList
#ifdef _Sample__
	class KMyDataList : public KSharedObjList<CMyData> {
	public:
		DECLARE_SERIAL(KMyDataList) // MFC 직렬화를 위한 매크로 계승 받은 객체에 해줘야 함.
		KMyDataList() = default;
		virtual ~KMyDataList() {}
		virtual void Serialize(CArchive& ar) override {
			KSharedObjList<CMyData>::Serialize(ar);  // 🔥 부모 클래스의 직렬화 호출
		}
	};
	IMPLEMENT_SERIAL(KMyDataList, CObject, 1) // 이건 cpp파일에 링크되어야 함.

		KMyDataList m_propList;
#endif // _Sample__

	KSharedObjList() = default;
	virtual ~KSharedObjList() {}
	void AddListHead(const KSharedObjList<T>& lstItem) {
		for (const auto& item : lstItem)
			this->push_front(item); // shared_ptr이므로 자동 참조 증가 (reference count 증가)
	}
	void AddListTail(const KSharedObjList<T>& lstItem) {
		for (const auto& item : lstItem)
			this->push_back(item); // shared_ptr이므로 자동 참조 증가 (reference count 증가)
	}

	virtual void Serialize(CArchive& ar) {
		CObject::Serialize(ar); // CObject 기본 직렬화 수행
		::TSerializeList(ar, *this); // 리스트 직렬화
	}
};

//IMPLEMENT_SERIAL(KSharedObjList<CObject>, CObject, 1)
	// Obtain the list of tags
#ifdef _UseOld_ParamList
#define POS_EXISTS(obj,pos) pos != NULL
#else
#define POS_EXISTS(obj,pos) (pos) != (obj).end()
#endif // _UseOld_ParamList #PtrList


#define MPAIR(var) std::make_pair(std::wstring(L#var), std_any(var))
	// 가변 인자들을 받아서 std::map<wstring, any>로 변환하는 함수
template <typename... Args>
std::map<std::wstring, std_any> UcMakeAnyMap(Args&&... args) {
	return{ std::forward<Args>(args)... };  // 전달된 pair들을 map에 삽입
}


#pragma region [var to map<key,any>
/// <summary>
/// 어떤 변수를 any로 변형하여 리턴, DwktoAny응용 하여 만든 함수
/// </summary>
/// throw //dwk: 2025-02-11 13:19  
/// <typeparam name="T"></typeparam>
/// <param name="arg"></param>
/// <returns></returns>
template<typename T>
std_any VarToAny(T&& arg)
{
	try {
#if CPP17_OR_LATER
		if constexpr (std::is_enum_v<std::decay_t<T>>) {
			return std_any(static_cast<int>(std::forward<T>(arg)));//primary 1 : int, CStringW
		}
#else
		if (std::is_enum<std::decay_t<T>>::value) {
			return std_any(static_cast<int>(std::forward<T>(arg)));//primary 1 : int, CStringW
		}
#endif
		if (&arg != nullptr) //return std::move(aa);내부적으로 move해주므로 굳이 해줄 필요 없다.
			return std_any(std::forward<T>(arg));//primary 1 : int, CStringW
		else
			return std_any(static_cast<char*>(nullptr));//return std::move(aa);
	}
	catch (const std::exception& e) {
		char* pfs = __FUNCSIG__;//현재함수 타입을 찍어보면, T 가 뭔지도 알수 있다.
		auto pType = typeid(arg).name();//enum EEEE
		//auto aa = std::make_any<LPCSTR>(pType);
		TRACE("%s std::exception : (%s) %s\n", pfs, pType, e.what());
		//return (aa);
		throw e;
	}
	catch (...) {
		auto pType = typeid(arg).name();//enum EEEE
		//auto aa = std::make_any<LPCSTR>(pType);
		TRACE("VarToAny ...(%s)\n", pType);
		//return (aa);
		throw; // ✅ 현재 예외를 다시 던짐
	}
}

/// <summary>
/// 각 변수들을 vector<any> 로 리턴
/// </summary>
/// <typeparam name="...Args"></typeparam>
/// <param name="...args"></param>
/// <returns></returns>
template<typename... Args>
std::vector<std_any> VarsToAnys(Args&&... args) {
	return{ VarToAny(std::forward<Args>(args))... };
}

/// <summary>
/// 변수들 이름을 각각 분리 하여 진짜 변수들과 함께 map 으로 만들어 리턴
/// </summary>
/// <typeparam name="...Args">진짜 각 변수들 타입</typeparam>
/// <param name="names">"nVersion, nCount, strPassword, strProjectName, m_bWithShadow" 처럼 키들이 모여있는 문자열</param>
/// <param name="...args">진짜 각 변수들</param>
/// <returns></returns>
template<typename... Args>
void AddVarPairsToMap(std::map<std::wstring, std_any>& mtokens, std::wstring names, Args&&... args)
{
	std::vector<std_any> arAny = VarsToAnys(std::forward<Args>(args)...);
	//std::map<std::wstring, std::any> mtokens;
	size_t start = 0, end = 0;
	const std::wstring delimiter = L", ";
	int i = 0;
	for (; (end = names.find(delimiter, start)) != std::wstring::npos; i++) {
		mtokens.insert(make_pair(names.substr(start, end - start), arAny[i]));
		start = end + delimiter.length();
	}
	mtokens.insert(make_pair(names.substr(start), arAny[i])); // 마지막 토큰 추가
}

template<typename... Args>
std::map<std::wstring, std_any> VarPairsToMap(std::wstring names, Args&&... args)
{
	std::map<std::wstring, std_any> mtokens;
	AddVarPairsToMap(mtokens, names, args...);/// `&&... args` 를 전달 하려면 `args...` 해 줘야지.
	return mtokens;//c++17 부터는 이런 경우 std::move 하지 않아도 복사되지 않고, 리턴 값이 호출한 곳에서 생성 전달 된다.
}

#define VAR_NAME(var) std::wstring(L#var)
/// 다양한 타입의 변수들을 키와 any 의 std::map<std::wstring, std::any> 로 만들어 준다.
#define VarsToJMap(...) VarPairsToMap(std::wstring{ VAR_NAME(__VA_ARGS__) },##__VA_ARGS__)
//#define VarToJMap(map1, ...) VarPairsToMap(map1, std::wstring{ VAR_NAME(__VA_ARGS__) },##__VA_ARGS__)
#pragma endregion ]var to map<key,any>




#include <atlbase.h>
#include <atlstr.h>

inline CStringW UcCLSIDToCString(const CLSID& clsid) {
	LPOLESTR wszCLSID = nullptr;
	if (SUCCEEDED(StringFromCLSID(clsid, &wszCLSID))) {
		CStringW strCLSID(wszCLSID);
		CoTaskMemFree(wszCLSID);
		return strCLSID;
	}
	return L"";
}

inline bool UcCStringToCLSID(const CStringW& strCLSID, CLSID& clsid) {
	if (strCLSID.GetLength() > 0)
		return SUCCEEDED(CLSIDFromString(strCLSID, &clsid));
	return false;
}

inline void UcBackSlash(CStringW& sFolder, BOOL bAdd = TRUE)
{
	if (bAdd)
	{
		if (sFolder.Right(1) != L"\\")
			sFolder += L"\\";
	}
	else
	{
		if (sFolder.Right(1) == L"\\")
			sFolder = sFolder.Left(sFolder.GetLength() - 1);
	}
}
template<typename T>
bool UcIsClass() {
#if CPP17_OR_LATER
	if constexpr (std::is_class_v<T>) {
		//std::cout << "T는 struct 또는 class입니다.\n";
		return true;
	}
	else {
		//std::cout << "T는 struct/class가 아닙니다.\n";
		return false;
	}
#else
	if (std::is_class<T>::value) {
		//std::cout << "T는 struct 또는 class입니다.\n";
		return true;
	}
	else {
		//std::cout << "T는 struct/class가 아닙니다.\n";
		return false;
	}
#endif
}
#ifdef _Sample__
main() {
	UcIsClass<MyStruct>(); // ✅ "T는 struct 또는 class입니다."
	UcIsClass<MyClass>();  // ✅ "T는 struct 또는 class입니다."
	UcIsClass<MyUnion>();  // ❌ "T는 struct/class가 아닙니다."
	UcIsClass<MyEnum>();   // ❌ "T는 struct/class가 아닙니다."
	UcIsClass<int>();      // ❌ "T는 struct/class가 아닙니다."
}
#endif // _Sample__



//#include <iostream>
//#include <type_traits>
//
////class CArchive {};  // 가상의 CArchive 클래스
//
//// 특정한 함수 시그니처 (virtual void Serialize(CArchive&))가 존재하는지 확인하는 trait
//template<typename T, typename = void>
//struct has_serialize
//	: std::false_type
//{
//};  // 기본적으로 false
//
//template<typename T>
//struct has_serialize<T, std::void_t<decltype(std::declval<T>().Serialize(std::declval<CArchive&>()))>>
//	: std::true_type
//{
//};
//
//template<typename T>
//constexpr bool has_serialize_v = has_serialize<T>::value; //dwk: 2025-03-17 09:29 
//

#include <type_traits>

// ✅ 특정 클래스가 `Serialize(CArchive& ar)`를 포함하는지 확인하는 템플릿
template <typename, typename = std::void_t<>>
struct has_serialize : std::false_type {};  // 기본적으로 false

// ✅ `Serialize`를 포함하는 클래스의 경우 true로 설정
template <typename T>
struct has_serialize<T, std::void_t<decltype(std::declval<T>().Serialize(std::declval<CArchive&>()))>>
	: std::true_type {
};

// ✅ `has_serialize_v` 단축 표현
template <typename T>
constexpr bool has_serialize_v = has_serialize<T>::value;

// ✅ 객체를 받아서 `Serialize` 함수 여부를 검사하는 함수
template <typename T>
bool UcCheckIfHasSerialize(const T& obj) {
	return has_serialize_v<T>;
}

#ifdef _Sample__
// 테스트용 클래스
struct WithSerialize {
	virtual void Serialize(CArchive& ar) {}  // 올바른 시그니처
};

struct WithoutSerialize {};  // Serialize() 없음

struct WrongSerialize {
	void Serialize(int) {}  // 매개변수가 다름
};

int mainx() {
	std::cout << std::boolalpha;
	std::cout << "WithSerialize has Serialize(CArchive&): " << has_serialize_v<WithSerialize> << "\n";  // true
	std::cout << "WithoutSerialize has Serialize(CArchive&): " << has_serialize_v<WithoutSerialize> << "\n";  // false
	std::cout << "WrongSerialize has Serialize(CArchive&): " << has_serialize_v<WrongSerialize> << "\n";  // false
}
#endif // _Sample__

// C++17 버전
#if CPP17_OR_LATER
template <typename Parent, typename T>
bool UcCheckIfDerived(const T& obj) {
	using DerivedType = std::decay_t<T>; // `const`, `&` 제거

	if constexpr (std::is_base_of<Parent, DerivedType>::value || std::is_same<Parent, DerivedType>::value) {
		return true;
	}
	else {
		return false;
	}
}

template <typename T, typename... TParents>
bool UcCheckMultipleBases(const T& obj, const std::tuple<TParents...>&) {
	// 타입 정보만 사용하므로 복사 불필요. fold expression으로 타입 체크
	return (... || UcCheckIfDerived<TParents>(obj));
}

#else
// C++14 버전
template <typename Parent, typename T>
typename std::enable_if<std::is_base_of<Parent, std::decay_t<T>>::value || std::is_same<Parent, std::decay_t<T>>::value, bool>::type
UcCheckIfDerived(const T& obj) {
	return true;
}

template <typename Parent, typename T>
typename std::enable_if<!std::is_base_of<Parent, std::decay_t<T>>::value && !std::is_same<Parent, std::decay_t<T>>::value, bool>::type
UcCheckIfDerived(const T& obj) {
	return false;
}

#pragma warning(push)
#pragma warning(disable: 4503)
// C++14에서 fold expression 대신 재귀적 템플릿 사용
template <typename T, typename Tuple, std::size_t... Is>
bool UcCheckMultipleBasesImpl(const T& obj, const Tuple&, std::index_sequence<Is...>) {
	// 타입 정보만 사용하므로 tuple 객체를 사용하지 않음
	bool results[] = { UcCheckIfDerived<std::tuple_element_t<Is, Tuple>>(obj)... };
	for (bool result : results) {
		if (result) return true;
	}
	return false;
}

//dwk: 2025-11-05 17:28 warning C4503: '__LINE__Var': 데코레이팅된 이름 길이를 초과했으므로 이름이 잘립니다.
template <typename T, typename... TParents>
bool UcCheckMultipleBases(const T& obj, const std::tuple<TParents...>& tuple) {
	// 타입 정보만 사용하므로 복사 불필요. index_sequence로 타입 체크
	// tuple을 참조로 전달하지만 실제로는 사용하지 않고 타입 정보만 사용
	return UcCheckMultipleBasesImpl(obj, tuple,
		std::index_sequence_for<TParents...>{});
}
#pragma warning(pop)

#endif

#ifdef _Sample__
bool bVtS = checkIfDerived<std::vector<wstring>>(_vtStr);  //  원래 벡터도 검사 가능
bool bMapS = checkIfDerived<std::map<wstring, wstring>>(_mapStr);  //  원래 벡터도 검사 가능
UcCheckMultipleBases(vecStrObj, std::tuple<std::vector<std::wstring>, std::vector<int>>{});  //  여러 벡터 타입 검사
#endif // _Sample__
template <typename T>
bool UcIsClass(const T& var) {
	const char* sTp = typeid(var).name();
	return tchstr(sTp, "class ") != nullptr;
	//Tas ss;
	//ss << "Type: " << typeid(var).name() << " | ";
	//ss << "Is class? " << std::is_class<decltype(var)>::value << std::endl;
}
template <typename T>
bool UcIsStruct(const T& var) {
	const char* sTp = typeid(var).name();
	return tchstr(sTp, "struct ") != nullptr;
}
template <typename T>
bool UcIsObject(const T& var) {
	return UcIsClass(var) || UcIsStruct(var);
}


UCTOOLDYNAMIC
bool UcIsCppSourceFile(LPCTSTR fullPath);


inline bool UcIsValidBase64(const std::string& s)
{
	for (char c : s) {
		if (!(isalnum((unsigned char)c) || c == '+' || c == '/' || c == '=')) {// 문제 있는 문자 발견 시 출력
			//TRACE("Invalid Base64 character: 0x%02X (%c)\n", (unsigned char)c, isprint((unsigned char)c) ? c : '.');
			return false;
		}
	}
	return true;
}
// P:QRCon1.cpp|I:0|H:fd78a911|D:77u/Ly8gUVJDb24xLmNwcCA6IFRoaXMgZmlsZSBjb250YWlucyB0aGUgJ21haW4nIGZ1bmN0aW9uLiBQcm9ncmFtIGV4ZWN1dGlvbiBiZWdpbnMgYW5kIGVuZHMgdGhlcmUuDQ==


int UcCutFileToHalf(CString sFile, ULONGLONG MAX_LOG_SIZE = 1024ULL * 1024 * 1024);


inline int UcGetBlockCount(ULONGLONG fileSize, int chunkSize)
{
	return (int)((fileSize + chunkSize - 1) / chunkSize);
}

//auto [h, m, s, n] = UcParseMilliseconds(tk.GetElapsed());
#include <cmath>  // std::floor
std::tuple<int, int, int, int> UcParseMilliseconds(double milliseconds);

std::tuple<bool, std::string> UcGetLineReverse(std::fstream& file, LONGLONG& pos, bool bEmptyLineAlso = true);


CStringW UcCleanFuncName(const char* func_name);
CString UcGetAbsolutePath(LPCTSTR szRelative);

//#include <afxstr.h> // CStringA, CStringW
inline CStringA operator"" _cs(const char* str, size_t len) {
	return CStringA(str, static_cast<int>(len));
}
//사용자 정의 리터럴의 이름은 반드시 접미사 앞에 _를 붙여야 합니다.
//이것은 C++ 표준에서 강제하는 규칙입니다.
inline CStringW operator"" _cs(const wchar_t* str, size_t len) {
	return CStringW(str, static_cast<int>(len));
}
//CStringA a = "hello"cs;  // 여기서는 그냥 `cs` 사용할 때는 접미사에서 _ 없이 사용
//CStringW w = L"안녕"cs;

#ifdef _DEBUG_try_ //dwk:2025-11-18 11:11
///이걸 DLL 프로젝트에 넣어야 extern으로 유일하게 쓸수 있다.
//#define DWK_JXDOC() 
std::map<std::string, function<void*()>> _mapFactory;
#endif // _DEBUG_try_ //dwk:2025-11-18 11:11


/// @brief  CArray가 MFC 이므로 UcBasetools.h 에서 옮겨옴.
template<typename T>
static void UcCopyCArray(CArray<T>& dst, const CArray<T>& src)//dwk: 2025-12-01 17:34 
{
	dst.RemoveAll();
	int n = src.GetSize();
	dst.SetSize(n);
	for (int i = 0; i < n; i++)
		dst[i] = src[i];   // deep copy (CString, double 모두 OK)
}



template<typename ShTVal, typename TObj, typename TVal>
bool UcCloneObjectT(TObj* source, TObj& tar, bool bClone)
{
	ASSERT(source);// bClone과 상관 없이 source가 있어야 한다.
	if (!source)
		return false;
#if STRUCTURED_BINDING_SUPPORTED
	for (const auto& [k, sjv] : *source)
	{
#else
	for (const auto& pair : *source)
	{
		const auto& k = pair.first;
		const auto& sjv = pair.second;
#endif
		ShTVal sjn;
		if (auto pval = sjv->Val())
			sjn = make_shared<TVal>(*sjv->Val(), bClone);
		else
			sjn = make_shared<TVal>();
		tar.SetAt(k, sjn);
	}
	return true;
	}


template<typename TObj, typename TArr, typename TVal>
void UcCloneArrayT(TArr & src, TArr & tar, bool bClone)
{
	if (bClone)
	{
		tar.clear();
		for (auto& sjv1 : src)
		{
			auto bv = sjv1->IsVal();
			auto ba = sjv1->IsArr();
			auto bd = sjv1->IsDic();
			if (ba || bd)
			{
				if (bClone)
					_break;
			}
			auto shVal = make_shared<TVal>(sjv1, bClone);
			tar.push_back(shVal);
		}
	}
	else
		tar = src;
}

#pragma region [CATCH
template<typename TCL>
inline CRuntimeClass* GetRTClass(TCL* th)
{
	CRuntimeClass* rc = NULL;
	// dynamic_cast는 다형 타입에만 허용. 비다형(순수 유틸 클래스 등)은 RTTI 없음 → NULL
	if constexpr (std::is_polymorphic_v<TCL>)
	{
		CObject* obj = dynamic_cast<CObject*>(th);
		if (obj)
			rc = obj->GetRuntimeClass();
	}
	else
		(void)th;
	return rc;
}

#define CATCH_CEXEPT(e) \
	{ CRuntimeClass* rc = GetRTClass(this); \
		auto buf = new WCHAR(1024);	KAtEnd d_buf([&]() { delete buf;});\
		e->GetErrorMessage(buf, 1000); CStringW sw(wcslen(buf) ? buf : L"No GetErrorMessage");\
		throw new KException(dynamic_cast<CException*>(e) ? "CException" : "UnkownException", GetLastError(), 0, sw, NULL, __FUNCTION__, __LINE__, __FILE__, rc);}

#define RETHROWCEXPT(e) \
	if(dynamic_cast<KException*>(e)) throw e;\
	if(dynamic_cast<CException*>(e)){ CATCH_CEXEPT(e);}\
	else {throw e;}


/// for pure class => CRuntimeClass is NULL
#define CATCH_CEXEPTNORC(e) \
	{	auto buf = new WCHAR(1024);	KAtEnd d_buf([&]() { delete buf;});\
		e->GetErrorMessage(buf, 1000); CStringW sw(wcslen(buf) ? buf : L"No GetErrorMessage");\
		throw new KException(dynamic_cast<CException*>(e) ? "CException" : "UnkownException", GetLastError(), 0, sw, NULL, __FUNCTION__, __LINE__, __FILE__, NULL);}

#define RETHROWCEXPTSTAIC(e) \
	if(dynamic_cast<KException*>(e)) throw e;\
	if(dynamic_cast<CException*>(e)){ CATCH_CEXEPTNORC(e);}\
	else {throw e;}

#define CATCH_ICEXEPTRC(rc) \
	catch(KException* e)\
	{	throw e;\
	} catch(CException* e)\
	{	auto buf = new WCHAR(1024);	KAtEnd d_buf([&]() { delete buf;});\
		e->GetErrorMessage(buf, 1000);\
		throw new KException("CException", GetLastError(), 0, buf, NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(std::exception &e)\
	{	throw new KException("std::exception", -1, 0, CStringW(e.what()), NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(LPCWSTR e)\
	{	throw new KException("LPCWSTR", GetLastError(), 0, e, NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(CStringW e)\
	{	throw new KException("CStringW", GetLastError(), 0, e, NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(CStringA e)\
	{	throw new KException("CStringA", GetLastError(), 0, CStringW(e), NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(LPCSTR e)\
	{	throw new KException("LPCSTR", GetLastError(), 0, CStringW(e), NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(int e)\
	{	throw new KException("int", 0, e, L"", NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(long e)\
	{	throw new KException("long", 0, e, L"", NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	} catch(...)\
	{	throw new KException("Unknown", GetLastError(), 0, L"Unknown catch(...) Error.", NULL, __FUNCTIONW__, __LINE__, __FILE__, rc);\
	}

#define CATCH_ICEXEPT CATCH_ICEXEPTRC(GetRTClass(this)) //dynamic_cast<CObject*>(this) ? this->GetRuntimeClass() : NULL)

#define CATCH_ICEXEPTRCLOG(rc) \
	catch(KException* e)\
	{	_log_.Error(e); throw e; \
	}\
	catch(CException* e)\
	{	auto buf = new WCHAR(1024);	KAtEnd d_buf([&]() { delete buf; }); \
		e->GetErrorMessage(buf, 1000); \
		_log_.Error(e); throw new KException("CException", GetLastError(), 0, buf, NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(std::exception &e)\
	{	CStringW ws(e.what()); _log_.Error(ws); throw new KException("std::exception", -1, 0, CStringW(e.what()), NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(LPCWSTR e)\
	{	_log_.Error(e); throw new KException("LPCWSTR", GetLastError(), 0, e, NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(CStringW e)\
	{	_log_.Error(e); throw new KException("CStringW", GetLastError(), 0, e, NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(CStringA e)\
	{	CStringW ws(e); _log_.Error(ws); throw new KException("CStringA", GetLastError(), 0, CStringW(e), NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(LPCSTR e)\
	{	CStringW ws(e); _log_.Error(ws); throw new KException("LPCSTR", GetLastError(), 0, CStringW(e), NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(int e)\
	{	CStringW ws; ws.Format(L"%d", e); _log_.Error(ws); throw new KException("int", 0, e, L"", NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(long e)\
	{	CStringW ws; ws.Format(L"%d", e); _log_.Error(ws); throw new KException("long", 0, e, L"", NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}\
	catch(...)\
	{	_log_.Error(L"..."); throw new KException("Unknown", GetLastError(), 0, L"Unknown catch(...) Error.", NULL, __FUNCTIONW__, __LINE__, __FILE__, rc); \
	}

/// 관련 로그는 KException 안에서 처리 하는 법
#define CATCH_ICEXEPTLOG CATCH_ICEXEPTRCLOG(GetRTClass(this)) //dynamic_cast<CObject*>(this) ? this->GetRuntimeClass() : NULL)

/// 이건 그냥 TRACE만 하는 군.
#define CATCH_TEXEPT catch(CException* e)\
	{	auto buf = new WCHAR(1024);\
		e->GetErrorMessage(buf, 1000);\
		TRACE(L"CException %u, %s\n", GetLastError(), buf);\
		delete buf;\
	} catch(std::exception &e)\
	{	TRACE(L"std::exception %s", CStringW(e.what()));\
	} catch(LPCWSTR e)\
	{	TRACE(L"LPCWSTR %u, %s",       GetLastError());\
	} catch(CStringW e)\
	{	TRACE(L"CStringW %u, %s",      GetLastError());\
	} catch(CStringA e)\
	{	TRACE(L"CStringA %u, %s",  GetLastError(), CStringW(e));\
	} catch(LPCSTR e)\
	{	TRACE(L"LPCSTR %u, %s",    GetLastError(), CStringW(e));\
	} catch(int e)\
	{	TRACE(L"int %d", e);\
	} catch(long e)\
	{	TRACE(L"long %ld", e);\
	} catch(...)\
	{	TRACE(L"Unknown", GetLastError());\
	}


#pragma endregion ]CATCH


//extern std::map<std::wstring, function<void* ()>> _mapFactory;//이거 싱글톤으로 해야 하고
//inline void UcSetupFactory(std::wstring sClassName, function<void* ()> fncCreate) {
//		_mapFactory[sClassName] = fncCreate;
//}
//dwk: 2025-12-04 13:23 
//dwk: 2025-12-08 09:59 
//dwk: 2026-01-22 15:24 UcClon Copy 함수들 CArray 때문에 UcTool.h 로 옮김
