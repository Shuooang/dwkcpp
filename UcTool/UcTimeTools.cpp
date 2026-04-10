#include "UcTimeTools.h"
#include "UcTimeTools.h"
#include "pch.h"
#include <ctime>
#include <iomanip>//std::get_time
#include <sstream>//istringstream

#include "UcBaseTools.h"
#include "UcTool.h"
#include "UcTimeTools.h"


//static std::map<wstring, int> g_wkko ={ { L"일", 0 }, { L"월", 1 }, { L"화", 2 }, { L"수", 3 }, { L"목", 4 }, { L"금", 5 }, { L"토", 6 }, };
static std::map<wstring, int> g_wkko = { { L"\uc77c", 0 }, { L"\uc6d4", 1 }, { L"\ud654", 2 }, { L"\uc218", 3 }, { L"\ubaa9", 4 }, { L"\ua208", 5 }, { L"\ud1a0", 6 }, };

int IsDateHan(WCHAR cd)
{
	//"수요일, 17 10 2007 16:34:17 +0900"
	//static std::set<wstring> s_datex ={ L"금",     L"목",     L"수",      L"월",     L"일",     L"토",     L"화", };//정렬됨
	//static std::set<wstring> s_date ={ L"\ua208", L"\ubaa9", L"\uc218", L"\uc6d4", L"\uc77c", L"\ud1a0", L"\ud654", };//정렬됨
	wstring wcd(cd, 1);
	auto it = g_wkko.find(wcd);
	return it != g_wkko.end();
}
int IsDateEng(LPCTSTR cd)
{
	static std::set<CString> s_dateE = { _T("Fri"), _T("Mon"), _T("Sat"), _T("Sun"), _T("Thu"), _T("Tue"), _T("Wed"), };//정렬됨
	auto it = s_dateE.find(cd);
	return it != s_dateE.end();
}

// 결국 문자열 -> CTime 은 모두 여기를 통과 하게 된다.
// CTime 을 쓰지 않기 때문에 1900- 이 초기값이다.
LPSYSTEMTIME UcCStringAllDigitTo_SYSTEMTIME(LPCTSTR strData, LPSYSTEMTIME pSt, bool bDateOnly)
{
	pSt->wYear = NULLYEAR;
	pSt->wMonth = NULLMONTH;
	pSt->wDay = NULLDAY;
	int len = (int)_tcslen(strData);
	if (len == 4)
	{
		//_stscanf
		_tscanf_s(strData, _T("%04hu"), &pSt->wYear);//swscanf_s
	}
	else if (len == 6)
	{
		_tscanf_s(strData, _T("%02hu%02hu%02hu"), &pSt->wYear, &pSt->wMonth, &pSt->wDay);
		if (0 <= pSt->wYear && pSt->wYear < 40)//20?? 에서 앞에 세기 생략된 경우
			pSt->wYear += 2000;
		else if (80 <= pSt->wYear && pSt->wYear < 100)//1980 에서 19 생략된 경우
			pSt->wYear += 1900;
		ASSERT(pSt->wYear >= 0);
	}
	else if (len == 8)
		_tscanf_s(strData, _T("%04hu%02hu%02hu"), &pSt->wYear, &pSt->wMonth, &pSt->wDay);
	else
	{
		if (bDateOnly)
		{
			if (len == 12 || len == 14 || len == 17)
				_tscanf_s(strData, _T("%04hu%02hu%02hu"), &pSt->wYear, &pSt->wMonth, &pSt->wDay);
			else
				return NULL;
		}
		else
		{
			if (len == 12)
				_tscanf_s(strData, _T("%04hu%02hu%02hu%02hu%02hu"), &pSt->wYear, &pSt->wMonth, &pSt->wDay, &pSt->wHour, &pSt->wMinute);
			else if (len == 14)
				_tscanf_s(strData, _T("%04hu%02hu%02hu%02hu%02hu%02hu"), &pSt->wYear, &pSt->wMonth, &pSt->wDay, &pSt->wHour, &pSt->wMinute, &pSt->wSecond);
			else if (len == 17)
				_tscanf_s(strData, _T("%04hu%02hu%02hu%02hu%02hu%02hu%03hu"), &pSt->wYear, &pSt->wMonth, &pSt->wDay, &pSt->wHour, &pSt->wMinute, &pSt->wSecond, &pSt->wMilliseconds);
			else
				return NULL;
		}
	}

	return pSt;
}


/// COleDateTime.ParseDataTime이 숫자로만 된 경우 invalid 되기 때문에 여기서 한다.2022-10-28 16:35:08
/// flag: 0 full, 1 VAR_TIMEVALUEONLY, 2 VAR_DATEVALUEONLY
COleDateTime UcAlldigitToOleTime(LPCTSTR strData, int flag)
{
	SYSTEMTIME st{ 0, };
	LPSYSTEMTIME pSt = &st;

	COleDateTime ot;
	auto rp = UcCStringAllDigitTo_SYSTEMTIME(strData, pSt, false);
	if (rp)
	{
		pSt->wYear = flag == 1 ? 1899 : pSt->wYear;// 1899,12,30 는 date 가 null것으로 간주한다.
		pSt->wMonth = flag == 1 ? 12 : pSt->wMonth;
		pSt->wDay = flag == 1 ? 30 : pSt->wDay;
		pSt->wHour = flag == 2 ? 0 : pSt->wHour;
		pSt->wMinute = flag == 2 ? 0 : pSt->wMinute;
		pSt->wSecond = flag == 2 ? 0 : pSt->wSecond;
		pSt->wMilliseconds = flag == 2 ? 0 : pSt->wMilliseconds;

		ot = st;
	}
	else
	{
		ot.SetStatus(COleDateTime::invalid);
	}
	return ot;
}

LPCTSTR UcTimeToString(CString& sTime, bool bSpace, TCHAR cSpDay, TCHAR cSpTime, int y, int m, int d, int hr, int mn, int sc)
{
	if (hr >= 0 && mn >= 0 && sc >= 0)
	{
		if (bSpace)
			sTime.Format(_T("%04d%c%02d%c%02d %02d%c%02d%c%02d"), y, cSpDay, m, cSpDay, d, hr, cSpTime, mn, cSpTime, sc);
		else
			sTime.Format(_T("%04d%02d%02d%02d%02d%02d"), y, m, d, hr, mn, sc);
	}
	else
	{
		if (bSpace)
			sTime.Format(_T("%04d%c%02d%c%02d"), y, cSpDay, m, cSpDay, d);
		else
			sTime.Format(_T("%04d%02d%02d"), y, m, d);
	}
	return sTime;
}
//LPCTSTR UcCTimeToString(CTime t, CString& sTime, bool bSpace, TCHAR cSpDay, TCHAR cSpTime)
//{
//	return UcTimeToString(sTime, bSpace, cSpDay, cSpTime, t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());
//}
CString UcCTimeToString(CTime t, bool bSpace, TCHAR cSpDay, TCHAR cSpTime)
{
	CString sTime;
	UcTimeToString(sTime, bSpace, cSpDay, cSpTime, t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());
	return sTime;
}

CTime UcOTimeToCTime(COleDateTime t)
{
	if (t.GetStatus() == COleDateTime::valid)
		return CTime(t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());
	else
		return{};
}
LPCTSTR UcOTimeToString(COleDateTime t, CString& sTime, bool bSpace, TCHAR cSpDay, TCHAR cSpTime)
{
	if (t.GetStatus() == COleDateTime::valid)
		return UcTimeToString(sTime, bSpace, cSpDay, cSpTime, t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());
	else
	{	/// valid 아닐때 하면 "-001--1--1" 이런값을 만들어 버린다.
		//sTime.Empty();
		return NULL;
	}
}

__time64_t UcSystimeToCtime(SYSTEMTIME& st)
{
	if (!(
		(st.wYear >= 1900) &&
		(st.wMonth >= 1 && st.wMonth <= 12) &&
		(st.wDay >= 1 && st.wDay <= 31) &&
		(st.wHour >= 0 && st.wHour <= 23) &&
		(st.wMinute >= 0 && st.wMinute <= 59) &&
		(st.wSecond >= 0 && st.wSecond <= 59)))
		return 0;
	CTime t(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	return t.GetTime();
}


bool UcIsPmHour(LPCTSTR pstr, int& pm)
{
	if (tchcmp(pstr, _T("오후")) == 0 || tchicmp(pstr, _T("PM")) == 0)
	{
		pm = 12;
		return true;
	}
	else if (tchcmp(pstr, _T("오전")) == 0 || tchicmp(pstr, _T("AM")) == 0)//if(ps.ExistInI(_T("오전"), _T("AM")))
	{
		pm = 0;
		return true;
	}
	return false;
}

int UcMonthFromStr10(LPCTSTR sMonth)
{
	static std::map<CString, int> s_sm =
	{
		{ _T("Apr"), 4, },
		{ _T("Aug"), 8, },
		{ _T("Dec"), 12, },
		{ _T("Feb"), 2, },
		{ _T("Jan"), 1, },
		{ _T("Jul"), 7, },
		{ _T("Jun"), 6, },
		{ _T("Mar"), 3, },
		{ _T("May"), 5, },
		{ _T("Nov"), 11, },
		{ _T("Oct"), 10, },
		{ _T("Sep"), 9, },
	};

	auto it = s_sm.find(sMonth);
	if (it != s_sm.end())
		return it->second;
	else
		return -1;
}

#define for_each0(n)  for(int _i=0;_i<(n);_i++)

//구명: KwParseTimeStr10 +		psEngTime	L"Wed May 31 14:44:16 2023"	__TIMESTAMP__
SYSTEMTIME* UcParseTimeStr_SYSTEMTIME(LPCTSTR psEngTime, SYSTEMTIME* pSyt, bool bDateOnly)
{
	ASSERT(psEngTime != NULL);
	// caller가 알아서 좋은 변수 보내도록 조치.

	SYSTEMTIME syt = { 0, 0, 0, 0, 0, 0, 0, 0 };
	//if(!UcIsAlpha(psEngTime[0]) && IsDateHan(psEngTime[0]) < 0)

	if (UcIsDigit(psEngTime[0]))
	{
		// 		if(psEngTime[10] == 'T')
		// 			UcParseTimeStr5(psEngTime, &syt);
		// //		<dc:date>2007-07-04T11:54:22+09:00</dc:date> 
		// 		else
		UcCStringToTime10(psEngTime, &syt, bDateOnly); // 숫자로만 된것은 맨 처음이 숫자 이다. 순서는 책임 못진다.
	}
	else
	{
		bool bAmPm = false;
		int pm = 0; // "오후" 또는 "PM" 이면 12가 된다.
		CArray<CString, LPCTSTR> ar;
		UcCutByToken(psEngTime, _T(", "), ar);

		__time64_t tsp = 0; // 시차
		syt.wMilliseconds = 0;
		for_each0(ar.GetCount())
		{
			CString& ps = ar[_i];
			if (IsDateHan(ps[0]) >= 0)
			{	// 한글은 요일로 간주
				//static std::map<wstring, int>& wkko = g_wkko;//{ { L"일", 0 }, { L"월", 1 }, { L"화", 2 }, { L"수", 3 }, { L"목", 4 }, { L"금", 5 }, { L"토", 6 }, };
				//if(wkko.find((PWS)ps[0]) != wkko.end())
			}
			else if (IsDateEng(ps) >= 0)
			{	// 한글은 요일로 간주
				//static std::map<wstring, int> wken ={ { L"Sun", 0 }, { L"Mon", 1 }, { L"Tue", 2 }, { L"Wed", 3 }, { L"Thr", 4 }, { L"Fri", 5 }, { L"Sat", 6 }, };
			}
			else if (UcIsPmHour(ps, pm)) //pm 은 오후 12:?? 인 경우 를 제외 하고 12를 더하면 된다.
			{
				//pm이 이미 바뀜
				bAmPm = true; // 오전/오후,am,pm 등의 글씨가 있었다.
			}
			else if (UcIsAlpha(ps[0]))
			{
				int iMon = UcMonthFromStr10(ps);
				if (iMon >= 1)// year
				{
					syt.wMonth = iMon;
				}
				// else : it's date
			}
			else if (ps == _T("GMT") || ps[0] == '+' || ps[0] == '-') //시차
			{
				//CString& sdf = ar[_i];
				//tsp = KwParseTimeDiff(ps);
			}
			else if (ps.GetLength() > 2 && ps[2] == ':' || ps[1] == ':') // hh:mm:ss
			{
				CArray<CString, LPCTSTR> art;
				UcCutByToken((LPCTSTR)ps, _T(":"), art);

				syt.wHour = _ttoi(art[0]);

				if (bAmPm)
				{
					if (pm == 0) // am
					{
						if (syt.wHour == 12) //오전 12: 는 0: 이다.
							syt.wHour = 0;
					}
					else // pm
					{
						ASSERT(1 <= syt.wHour && syt.wHour <= 12); //오전 12: 는 0: 이다.
						if (syt.wHour != 12) //오후는 12:00 제외한 나머지는 +12를 해야 한다.
							syt.wHour += pm;
					}
				}
				syt.wMinute = _ttoi(art[1]);

				if (art.GetCount() > 2)
					syt.wSecond = _ttoi(art[2]);
			}
			else
			{
				int n = _ttoi(ps);
				if (1900 <= n && n <= 3000)	// year
					syt.wYear = n;
				else
				{
					if (syt.wDay == 0) //날짜 가 먼저 오는것으로 간주
					{
						ASSERT(1 <= n && n <= 31);
						syt.wDay = n;
					}
					else // month
					{
						ASSERT(1 <= n && n <= 12);	// month
						ASSERT(syt.wMonth == 0);
						syt.wMonth = n;
					}
				}
			}
		}
	}

	if (bDateOnly)
	{
		syt.wHour = syt.wMinute = syt.wSecond = 0;
	}

	if (pSyt)
	{
		if (syt.wYear == 0 && syt.wMonth == 0 && syt.wDay == 0)
		{
			// 시간만 있는 경우, 시간만 복사 한다. 리턴값은 1980/1/1 로 리턴 한다.
			// 가져온 pSyt 의 날짜 부분을 손상 하지 않는다.
			pSyt->wHour = syt.wHour;
			pSyt->wMinute = syt.wMinute;
			pSyt->wSecond = syt.wSecond;

			if (pSyt->wYear == 0 && pSyt->wMonth == 0 && pSyt->wDay == 0)
			{
				// 가져온것, 만든것 둘다 날짜가 없으면 default 1/1/1980 로 한다.
				syt.wYear = NULLYEAR;
				syt.wMonth = syt.wDay = 1;
				return pSyt;
			}
			else
				return pSyt;
		}
		else
			*pSyt = syt;
	}
	//The upper date limit is 12/31/3000. The lower limit is 1/1/1970 12:00:00 AM GMT.
	if (syt.wYear == 0 && syt.wMonth == 0 && syt.wDay == 0)
	{
		syt.wYear = NULLYEAR;
		syt.wMonth = syt.wDay = 1;
	}
	return pSyt;
}

SYSTEMTIME* UcParseTimeStr5(LPCTSTR psEngTime, SYSTEMTIME* pSyt, bool bDateOnly)
{
	return UcParseTimeStr_SYSTEMTIME(psEngTime, pSyt, bDateOnly);
}
__time64_t UcParseTimeStr4(LPCTSTR psEngTime, SYSTEMTIME* pSyt, bool bDateOnly)
{
	SYSTEMTIME syt = { 0, };
	if (pSyt == NULL)
		pSyt = &syt;
	UcParseTimeStr5(psEngTime, pSyt, bDateOnly);
	return UcSystimeToCtime(*pSyt);
}

LPSYSTEMTIME UcCStringToTime10(LPCTSTR strData, LPSYSTEMTIME pSt, bool bDateOnly)
{
	static LPCTSTR _seps = _T("- :/.");
	int lsep = 5;
	TCHAR seps[10] = { '\0', };
	//UcZeroMemory(seps);
	//memset(seps, 0, 10);
	_tcscpy_s(seps, sizeof(seps), _seps);
	TCHAR* tok = NULL;
	LPCTSTR par[10] = { 0, };
	int warr[8] = { NULLYEAR, NULLMONTH, NULLDAY, 0, 0, 0, 0, 0 }; // static 이면 이전 값이 그래로 있잖아.

	int nAm = 0;// 'PM' 인경우 12 로 바뀐다.

	int len = (int)_tcslen(strData);
	ASSERT(len > 3);
	bool bAllDigit = true;
	for_each0(len)
	{
		TCHAR c = strData[_i];
		if (!UcIsDigit(c))
		{
			bAllDigit = false;
			break;
		}
	}

	if (bAllDigit)
	{
		return UcCStringAllDigitTo_SYSTEMTIME(strData, pSt, bDateOnly);
	}
	else
	{
		//"8/9/2006 8:00 AM" 의 경우는?
		if (len > 0)//strData)
		{
			//			CString sSeps = seps;
			//		<dc:date>2007-07-04T11:54:22+09:00</dc:date> 
			if (strData[10] == 'T')
			{
				seps[lsep] = 'T';	lsep++;
				seps[lsep] = '\0';
				//				sSeps += 'T';
				// 				_stscanf(strData, _T("%04d-%02d-%02dT%02d%02d%02d"), 
				// 					warr, warr+1, warr+2, warr+3, warr+4, warr+5);		
			}

			//CAutoBufWrapperW ptr(tchlen(strData)+1);
			//auto htr = std::shared_ptr<WCHAR>(new WCHAR[tchlen(strData) + 1]); // 안좋음
			//auto htr = std::shared_ptr<WCHAR>(new WCHAR[tchlen(strData) + 1], [](WCHAR* p) { delete[] p; });//이렇게 쓰거나
			std::unique_ptr<TCHAR[]> htr(new TCHAR[tchlen(strData) + 1]);//이걸 써야 함.

			auto ptr = htr.get();
			tchcpy(ptr, (LPCTSTR)strData);
			TCHAR* next_token1 = NULL;

			tok = _tcstok_s(ptr, seps, &next_token1);
			int nitem = 0;
			for (int i = 0; tok != NULL && i < 8; i++)
			{
				par[i] = tok;
				tok = _tcstok_s(NULL, seps, &next_token1);
				nitem++;
			}

			if (tchlen(par[2]) == 4)
			{// ("8/19/2006 8:00 AM"); // 년도가 3번째 오는 경우 월/일/년 => 년/월/일 
				LPCTSTR y = par[2];
				LPCTSTR m = par[0];
				LPCTSTR d = par[1];
				par[0] = y;	// 2006, 8, 19
				par[1] = m;
				par[2] = d;

			}

			if ((par[5] && tchicmp(_T("PM"), par[5])) || (par[6] && tchicmp(_T("PM"), par[6])))
			{
				nAm = 12;
			}
			for_each0(nitem)
			{
				if (UcIsDigit(par[_i][0]))
					warr[_i] = UcAtoi(par[_i]);
#ifdef _DEBUG
				else
					ASSERT(nAm == 12);//+strData	L"25 January 1996"
#endif // _DEBUG
			}
		}
	}


	// 	if(warr[2] > 1000)
	// 	{
	// 		int y = warr[2];
	// 		int m = warr[0];
	// 		int d = warr[1];
	// 		warr[0] = y;
	// 		warr[1] = m;
	// 		warr[2] = d;
	// 	}

	if (0 <= warr[0] && warr[0] < 30)
		warr[0] += 2000;
	else if (80 <= warr[0] && warr[0] < 100)
		warr[0] += 1900;

	ASSERT(warr[0] >= 0);
	pSt->wYear = warr[0];
	pSt->wMonth = warr[1];
	pSt->wDay = warr[2];
	pSt->wHour = bDateOnly ? 0 : warr[3];
	pSt->wMinute = bDateOnly ? 0 : warr[4];
	pSt->wSecond = bDateOnly ? 0 : warr[5];
	pSt->wMilliseconds = bDateOnly ? 0 : warr[6];
	return pSt;
}


COleDateTime UcGetCurrentTimeV(int bUTC)
{
#ifdef _DEBUGx
	COTime to = COleDateTime::GetCurrentTime();
	CString sto = to.Format(_T("%Y-%m-%d(%a), %H:%M:%S"));
	CTime tn = CTime::GetCurrentTime();
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);
#endif // _DEBUG
	SYSTEMTIME st2;
	if (bUTC)
		GetSystemTime(&st2);
	else
		GetLocalTime(&st2);//오직 이것만이 CE 에서도 현시각 리턴 한다.
	COleDateTime t(st2);//st2.wYear, st2.wMonth, st2.wDay, st2.wHour, st2.wMinute, st2.wSecond);
	return t;
	// -		st2	{wYear=2008 wMonth=5 wDayOfWeek=2 ...}	_SYSTEMTIME
	// 		wYear	2008	unsigned short
	// 		wMonth	5	unsigned short
	// 		wDayOfWeek	2	unsigned short
	// 		wDay	13	unsigned short
	// 		wHour	17	unsigned short
	// 		wMinute	39	unsigned short
	// 		wSecond	20	unsigned short
	// 		wMilliseconds	0	unsigned short
	// -		sysTime	{wYear=2008 wMonth=5 wDayOfWeek=2 ...}	_SYSTEMTIME
	// 		wYear	2008	unsigned short
	// 		wMonth	5	unsigned short
	// 		wDayOfWeek	2	unsigned short
	// 		wDay	13	unsigned short
	// 		wHour	8	unsigned short
	// 		wMinute	39	unsigned short
	// 		wSecond	19	unsigned short
	// 		wMilliseconds	0	unsigned short
	// -		tn	{m_s={...}}	CTime
	// +		[ATL::CTime]	{m_time=1210667958}	ATL::CTime
	// -		m_s	{0x003a7e80}	ATL::CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t> > >
	// -		[ATL::CSimpleStringT<wchar_t,0>]	{0x003a7e80}	ATL::CSimpleStringT<wchar_t,0>
	// +		m_pszData	0x003a7e80 "2008-05-13 01:39(화)"	wchar_t*
	//현재 2008-05-13 오후 5:39

}

CTime UcVarDateToCTime(DATE dt)
{
	SYSTEMTIME st = { 0, };
	VariantTimeToSystemTime(dt, &st);
	CTime t(st);
	return t;
}

//see also SetTimeZoneInformation
CTime UcGetCurrentTime(int bUTC)
{
	COleDateTime dt = UcGetCurrentTimeV(bUTC);
	return UcVarDateToCTime(dt);
}
/// <summary>
/// "2024-04-01 10:19:02" 형태의 현시각 문자열
/// </summary>
/// <param name="bUTC">
/// eCtLocal, // app 이 실행 되는 곳
/// eCtUTC,   // GMT
/// eCtSvr,   // server machine의 로컬 시각
/// </param>
/// <returns>ex:"2024-04-01 10:19:02"</returns>
UCTOOLDYNAMIC
CString UcGetCurrentTimeString(int bUTC)
{
	CTime now = UcGetCurrentTime(bUTC);
	return UcCTimeToString(now);
}



bool UcInitSysTime(SYSTEMTIME& st)
{
	if (st.wYear < NULLYEAR || st.wYear > 3000)
	{
		st.wYear = NULLYEAR;
		st.wMonth = NULLMONTH;
		st.wDay = NULLDAY;
		st.wHour = 0;
		st.wMinute = 0;
		st.wSecond = 0;
		st.wMilliseconds = 0;
		st.wDayOfWeek = 0;
		return false;
	}
	return true;
}
inline LPSYSTEMTIME UcCStringToTime(LPCTSTR strData, LPSYSTEMTIME pSt, bool bDateOnly = false)
{
	return UcCStringToTime10(strData, pSt, bDateOnly);
}
LPSYSTEMTIME UcCStringToSysTime(LPCTSTR strData, LPSYSTEMTIME pSt)
{
	SYSTEMTIME st1 = { 0, };
	UcInitSysTime(st1);

	UcCStringToTime(strData, &st1);

	WORD wMilliseconds = st1.wMilliseconds;
	COleDateTime dt(st1.wYear, st1.wMonth, st1.wDay, st1.wHour, st1.wMinute, st1.wSecond);
	if (dt.GetStatus() != COleDateTime::valid)
	{
		CTime dt0(st1.wYear, st1.wMonth, st1.wDay, st1.wHour, st1.wMinute, st1.wSecond);
		SYSTEMTIME st0 = { (WORD)dt0.GetYear(), (WORD)dt0.GetMonth(), (WORD)dt0.GetDay(), (WORD)dt0.GetHour(), (WORD)dt0.GetMinute(), (WORD)dt0.GetSecond(), };
		*pSt = st0;
	}
	else
	{
#ifdef _DEBUGx
		CTime dt0(st1.wYear, st1.wMonth, st1.wDay, st1.wHour, st1.wMinute, st1.wSecond);
		CString s1; s1.Format(_T("%04d%02d%02d%02d%02d%02d"), dt.GetYear(), dt.GetMonth(), dt.GetDay(), dt.GetHour(), dt.GetMinute(), dt.GetSecond());
		CString s0; s0.Format(_T("%04d%02d%02d%02d%02d%02d"), dt0.GetYear(), dt0.GetMonth(), dt0.GetDay(), dt0.GetHour(), dt0.GetMinute(), dt0.GetSecond());
#endif // _DEBUG
		try
		{
			dt.GetAsSystemTime(*pSt);// 여기서 시각이 잘못되었다면.. pSt의 값은 바뀐다.
		}
		catch (...)
		{
			pSt->wYear = 0;
			UcInitSysTime(*pSt);
			return pSt;
		}
	}
	pSt->wMilliseconds = wMilliseconds;//warr[6];
	return pSt;
}
__time64_t UcCStringToCTime(LPCTSTR strData, CTime& cTime)
{
	SYSTEMTIME st = { 0, };
	UcInitSysTime(st);//UcCStringToSysTime 에서 한값을 덮어 오므로 할필요 없는것 같다.

	UcCStringToSysTime(strData, &st);

	//	UcInitSysTime(st);
	CTime cTime0(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	cTime = cTime0;//담아서 보내야지
	ASSERT(cTime.GetYear() >= NULLYEAR);
	ASSERT(cTime.GetTime() >= 0);
	return cTime.GetTime();
}
CTime UcCStringToCTime(LPCTSTR strData)
{
	CTime t;
	UcCStringToCTime(strData, t);
	return t;
}

/// <summary>
/// 
/// </summary>
/// <param name="tSysTime"></param>
/// <param name="minLocal">현위치 관점에서 GMT는 이만큼 차이난다. Korea:-540</param>
/// <returns></returns>
std::shared_ptr<SYSTEMTIME> UcSystimeZoneChange(SYSTEMTIME& tSysTime, int minLocal)
{
	TIME_ZONE_INFORMATION timeZoneInfo;
	memset(&timeZoneInfo, 0, sizeof(TIME_ZONE_INFORMATION));
	// 시간대 정보 설정
	// timeZoneInfo 에서 Bias 필드는 UTC와의 시간 차이를 분 단위로 나타냅니다.
	timeZoneInfo.Bias = minLocal; // 예: 0이면, UTC와 동일한 시간대
	// 시간대 정보를 기반으로 시간 변환
	SYSTEMTIME outSysTime = { 0 };
	BOOL b = SystemTimeToTzSpecificLocalTime(&timeZoneInfo, &tSysTime, &outSysTime);
	if (b)
		return std::make_shared<SYSTEMTIME>(outSysTime);
	else
		return nullptr;
#ifdef _Usage__
	auto zST = UcSystimeZoneChange(tSysTime, -540);
	if (zST)
		std_cout2 << "Even number: " << *zST << '\n';
	else
		std_cout2 << "Not an even number.\n";
#endif // _Usage__

}
bool UcParseHttpDate(const std::string& httpDate, SYSTEMTIME& outSysTime, int minLocal)
{
	std::tm tm = {};
	std::istringstream ss(httpDate);
	ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT"); // RFC 1123 format
	if (ss.fail())
		return false;
	// std::tm을 SYSTEMTIME으로 변환
	SYSTEMTIME tSysTime;
	tSysTime.wYear = tm.tm_year + 1900;
	tSysTime.wMonth = tm.tm_mon + 1;
	tSysTime.wDayOfWeek = tm.tm_wday;
	tSysTime.wDay = tm.tm_mday;
	tSysTime.wHour = tm.tm_hour;
	tSysTime.wMinute = tm.tm_min;
	tSysTime.wSecond = tm.tm_sec;
	tSysTime.wMilliseconds = 0;

	auto zST = UcSystimeZoneChange(tSysTime, minLocal);
	if (zST)//auto b = zST.has_value();
		outSysTime = *zST;
	//TIME_ZONE_INFORMATION timeZoneInfo;
	//memset(&timeZoneInfo, 0, sizeof(TIME_ZONE_INFORMATION));
	//// 시간대 정보 설정
	//// timeZoneInfo 에서 Bias 필드는 UTC와의 시간 차이를 분 단위로 나타냅니다.
	//timeZoneInfo.Bias = minLocal; // 예: 0이면, UTC와 동일한 시간대
	//// 시간대 정보를 기반으로 시간 변환
	//BOOL b = SystemTimeToTzSpecificLocalTime(&timeZoneInfo, &tSysTime, &outSysTime);
	return (bool)zST;// b;
}

bool UcParseHttpDateToLocal(const std::string& httpDate, SYSTEMTIME& outSysTime)
{
	TIME_ZONE_INFORMATION timeZoneInfo;
	DWORD result = GetTimeZoneInformation(&timeZoneInfo);
	//TRACE(L"timeZone minute:%d\n", timeZoneInfo.Bias);//-540 만큼 UTC가 로컬 보다, 이만큼 차이난다는 의미구나.
	bool btK = UcParseHttpDate(httpDate, outSysTime, timeZoneInfo.Bias);//이거 제대로 안된다.(19:23) 함수 내부에서 -540으로 전환 하니 13:23 원래 파일 시간이 된다.
	return btK;
}

std::shared_ptr<SYSTEMTIME> UcTimeZoneChange(SYSTEMTIME sysT, bool bToLocal)
{
	TIME_ZONE_INFORMATION timeZoneInfo;
	DWORD result = GetTimeZoneInformation(&timeZoneInfo);
	auto bias = timeZoneInfo.Bias * (bToLocal ? 1 : -1);
	return UcSystimeZoneChange(sysT, bias);//-5540
	//if (oLocal.has_value())
	//	tLC = oLocal.value();
}
std::shared_ptr<SYSTEMTIME> UcTimeZoneToLocal(SYSTEMTIME sysT)
{
	return UcTimeZoneChange(sysT, true);
}
std::shared_ptr<SYSTEMTIME> UcTimeZoneToLocal(CTime t)
{
	SYSTEMTIME sysT = { (WORD)t.GetYear(), (WORD)t.GetMonth(), (WORD)t.GetDayOfWeek(),
		(WORD)t.GetDay(), (WORD)t.GetHour(), (WORD)t.GetMinute(), (WORD)t.GetSecond(), };
	return UcTimeZoneChange(sysT, true);
}
std::shared_ptr<SYSTEMTIME> UcTimeZoneToGMT(SYSTEMTIME sysT)
{
	return UcTimeZoneChange(sysT, false);
}

std::shared_ptr<SYSTEMTIME> UcTimeZoneToGMT(CTime t)
{
	SYSTEMTIME sysT = { (WORD)t.GetYear(), (WORD)t.GetMonth(), (WORD)t.GetDayOfWeek(),
		(WORD)t.GetDay(), (WORD)t.GetHour(), (WORD)t.GetMinute(), (WORD)t.GetSecond(), };
	auto gt = UcTimeZoneChange(sysT, false);
	//if (gt.has_value())	{	}
	return gt;
}
CString UcTimeZoneToGmtStr(CTime t)
{
	SYSTEMTIME sysT = { (WORD)t.GetYear(), (WORD)t.GetMonth(), (WORD)t.GetDayOfWeek(),
		(WORD)t.GetDay(), (WORD)t.GetHour(), (WORD)t.GetMinute(), (WORD)t.GetSecond(), };
	std::shared_ptr<SYSTEMTIME> gt = UcTimeZoneChange(sysT, false);
	if (gt)
		return UcSystimeToString(&*gt);
	return{};
}
CTime UcStrTimeToLocal(CString strT)
{
	SYSTEMTIME sysT = { 0, };
	if (UcCStringToSysTime(strT, &sysT))
	{
		std::shared_ptr<SYSTEMTIME> lt = UcTimeZoneToLocal(sysT);
		if (lt)
		{
			return UcSystimeToCtime(*lt);
		}
	}
	return{};
}
CString UcStrTimeToLocalStr(CString strT)
{
	SYSTEMTIME sysT = { 0, };
	if (UcCStringToSysTime(strT, &sysT))
	{
		std::shared_ptr<SYSTEMTIME> lt = UcTimeZoneToLocal(sysT);
		if (lt)
		{
			CTime ct = UcSystimeToCtime(*lt);
			return UcCTimeToString(ct);
		}
	}
	return{};
}


CString UcGetCurrentTimeStamp(int bUTC)
{
	SYSTEMTIME st;
	if (bUTC)
		GetSystemTime(&st);
	else
		GetLocalTime(&st);
	CString s; s.Format(_T("%04d%02d%02d%02d%02d%02d%03d"),
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return s;
}

//"2024-06-01 00:00:00"
bool UcQaTestExpired(CStringA sTimeExpire)
{
	CString stw(sTimeExpire);
	COleDateTime dt;
	dt.ParseDateTime(stw);
	SYSTEMTIME sysTime{};
	if (dt.GetAsSystemTime(sysTime))
	{
		CTime tExp = CTime(sysTime);
		auto tNow = UcGetCurrentTime();
		return tExp < tNow;//tExp = {m_time=1717167600 } tNow = {m_time=1718084211 }
	}
	ASSERT(0);
	return true;
}

bool UcQaTestExpired(CTime tExp)
{
	//CTime tExp = UcParseTimeStrA4(sTimeExpire);
	auto tNow = UcGetCurrentTime();
	return tExp < tNow;// 지금이 QA test 끝 날 지났으면 TRUE
}




//KStdMap<INT_PTR, KLambdaTimer*> KLambdaTimer::_mapIdThis;
//KList<KLambdaTimer*> KLambdaTimer::_queTimer;

// wchar_t* ws___ = L" \t\n\r\f\v";
// char* as___ = " \t\n\r\f\v";


/// _wnd가 널일때 메인프레임에 타이머 셋팅한 경우만 여기서
static void CALLBACK EXPORT TimerProcLbd(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime)
{
	ASSERT(WM_TIMER == nMsg);
	auto* wnd = AfxGetMainWnd();// 윈도우를 줘야 nIDEvent가 보존 된다.
	if (hWnd == wnd->GetSafeHwnd())
	{
		if (::IsWindow(hWnd))
		{
			auto id = (INT_PTR)(nIDEvent / 1000) * 1000; // 1000 아래 자리 자른다. 
			auto& mapIdThis = *GSingleton<KStdMap<INT_PTR, KLambdaTimer*>>::GetInstance();

			auto it = mapIdThis.find(id);
			if (it != mapIdThis.end())
			{
				auto th = it->second;
				th->DoTimerTask(nIDEvent);
			}
		}
		else
		{
			TRACE("TimerProcLbd: window Handle is invalid. >>\n");
		}
	}
}

UINT_PTR KLambdaTimer::SetTimerEx(UINT_PTR nIDEvent, UINT nElapse, TIMERPROC lpfnTimer)
{
	UINT_PTR rv = 0;
	if (_wnd == nullptr)
	{
		auto* wnd = AfxGetMainWnd();// 윈도우를 줘야 nIDEvent가 보존 된다.
		rv = ::SetTimer((HWND)wnd->GetSafeHwnd(), nIDEvent, nElapse, (TIMERPROC)TimerProcLbd);

		auto& queTimer = KLambdaTimer::GetQueTimer();
		queTimer.push_back(this);
		//KLambdaTimer::_queTimer.push_back(this);
	}
	else
	{
		auto bPtr = AfxIsValidAddress(_wnd, sizeof(CWnd), 0);
		ASSERT(bPtr);
		_hWnd = _wnd->GetSafeHwnd();//지금 널이 아니겠지.
		rv = _wnd->SetTimer(nIDEvent, nElapse, NULL);
	}
	return rv;
}
BOOL KLambdaTimer::KillTimerEx(UINT_PTR nIDEvent)
{
	BOOL rv = FALSE;
	if (_wnd == nullptr)
	{
		ASSERT(nIDEvent > LAMBDATIMERID);
		auto* wnd = AfxGetMainWnd();// 윈도우를 줘야 nIDEvent가 보존 된다.
		rv = ::KillTimer(wnd->GetSafeHwnd(), nIDEvent);
	}
	else
		rv = _wnd->KillTimer(nIDEvent);
	return rv;
}
/// <summary>
/// 타이머를 시작 한다. 이미 같은 아이디(문자열)타이머가 등록 되어 있으면 Kill 하고 다시 시작 한다.
/// 따라서 변수들 elapsed 람다함수 등이 바뀐후 적용된다.
/// </summary>
/// <param name="sid"></param>
/// <param name="elapsed"></param>
/// <param name="lmda"></param>
/// <param name="maxCount"></param>
void KLambdaTimer::SetLambdaTimerImple(LPCSTR sid, UINT elapsed, function<void(LPVOID)> lmda, int maxCount /*= 0*/
	, function<void(LPVOID)> lmdaFinish/* = NULL*/, LPCSTR fnc, int line
)
{
	// _wnd가 null이 아닐 때만 _wnd로 적용
	if (_wnd == nullptr) {
		if (auto* pWnd = dynamic_cast<CWnd*>(this))
			_wnd = pWnd;
	}

	auto bPtr = AfxIsValidAddress(_wnd, sizeof(CWnd), 0);
	ASSERT(bPtr);///, KLambdaTimer(this) 생성자에서 안해 줬군.
	if (_wnd == NULL)
		return;
	_hWnd = _wnd->GetSafeHwnd();//지금 널이 아니겠지.
	UINT_PTR idTm = 0;
	SHP<KTimerObj> tobj;
	if (_mapTmObj.Has(sid))
	{
		idTm = _mapTmID[sid];
		tobj = _mapTmObj[sid];

		if (tobj->_stat == "started")
			KillTimerEx(tobj->_idTimer);
		//TRACE("Lambda Timer: %s %d msec restarted~\n", sid, (int)(t2 - tobj->_tickStart));
	}
	else
	{
		_idTm++;/// timer ID는 내부적으로 LAMBDATIMERID 부터 하나씩 증가 _mapTmID 에서 sid 와 매핑된다.
		idTm = _idx + _idTm;

		_mapTmID[sid] = idTm;
		_mapRTmID[idTm] = sid;
		tobj = make_shared<KTimerObj>();
		_mapTmObj[sid] = tobj;
		//TRACE("Lambda Timer: %s SetTimer <<\n", sid);
	}

	tobj->_maxCount = maxCount;/// 0이면 무한
	tobj->_idTimer = idTm;
	tobj->_elapsed = elapsed;
	tobj->_fnc = fnc;
	tobj->_line = line;
	tobj->_stat = "started";
	tobj->_tickStart = GetTickCount64();
	_mapTimer[idTm] = lmda;
	if (lmdaFinish)
		_mapTimerFinish[idTm] = lmdaFinish;

	SetTimerEx(idTm, elapsed, NULL);
}

void KLambdaTimer::ChangeLambdaTaskImple(LPCSTR sid, function<void(LPVOID)> lmda)
{
	if (!_mapTmID.Has(sid))
	{
		TRACE("ChangeLambdaTaskImple: %s ID invalid. >>\n", sid);
		return;
	}
	UINT_PTR idTm = _mapTmID[sid];
	_mapTimer[idTm] = lmda;
}

void KLambdaTimer::ChangeInterval(LPCSTR sid, UINT elapsed)
{
	if (!_mapTmID.Has(sid))
	{
		TRACE("ChangeInterval: %s ID invalid. >>\n", sid);
		return;
	}
	UINT_PTR idTm = _mapTmID[sid];
	auto tobj = _mapTmObj[sid];
	ASSERT(tobj);
	if (tobj->_elapsed != elapsed)
	{
		KillTimerEx(idTm);
		tobj->_elapsed = elapsed;
		SetTimerEx(_idTm, elapsed, NULL);
		//_wnd->SetTimer(_idTm, elapsed, NULL);
	}
}
int KLambdaTimer::GetInterval(LPCSTR sid)
{
	if (_mapTmObj.Has(sid))
	{
		auto tobj = _mapTmObj[sid];
		if (tobj)
			return tobj->_elapsed;
	}
	TRACE("GetInterval: %s ID invalid. >>\n", sid);
	return 0;
}
SHP<KTimerObj> KLambdaTimer::GetTimerInfo(LPCSTR sid)
{
	if (_mapTmObj.Has(sid))
	{
		auto tobj = _mapTmObj[sid];
		return tobj;
	}
	TRACE("GetTimerInfo: %s ID invalid. >>\n", sid);
	return NULL;
}
BOOL KLambdaTimer::PauseTimer(LPCSTR sid)
{
	if (!_mapTmID.Has(sid))
	{
		TRACE("PauseTimer: %s ID invalid. >>\n", sid);
		return FALSE;
	}
	UINT_PTR idTm = _mapTmID[sid];
	if (_mapTmObj.Has(sid))
	{
		auto tobj = _mapTmObj[sid];
		if (tobj)
		{
			if (tobj->_stat != "stopped")
			{
				KillTimerEx(idTm);
				tobj->_stat = "stopped";
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL KLambdaTimer::RestartTimer(LPCSTR sid)
{
	if (!_mapTmID.Has(sid))
	{
		TRACE("RestartTimer: %s ID invalid. >>\n", sid);
		return FALSE;
	}
	UINT_PTR idTm = _mapTmID[sid];
	auto tobj = _mapTmObj[sid];
	KillTimerEx(idTm);
	ASSERT(tobj);
	SetTimerEx(idTm, tobj->_elapsed, NULL);
	tobj->_stat = "started";
	return TRUE;
}

/// <summary>
/// 다시는 이 타이머를 안쓸때 관련 정보를 보무 삭제 한다.
/// 또 같은걸 사용할때는 PauseTimer를 쓴다. RestartTimer를 쓴다.
/// </summary>
/// <param name="sid"></param>
/// <param name="bKill"></param>
BOOL KLambdaTimer::KillLambdaTimer(LPCSTR sid, bool bKill /*= true*/)
{
	if (!::IsWindow(_hWnd))
	{
		TRACE("DoTimerTask: window Handle is invalid. >>\n");
		return FALSE;
	}

	if (!_mapTmID.Has(sid))
	{
		TRACE("KillLambdaTimer: %s ID invalid. >>\n", sid);
		return FALSE;
	}
	UINT_PTR idTm = _mapTmID[sid];
	if (bKill)
		KillTimerEx(idTm);
	//TRACE("Lambda Timer: %s KillTime >>\n", sid);

	_mapTmObj.RemoveKey(sid);
	_mapTmID.RemoveKey(sid);
	_mapTimer.RemoveKey(idTm);
	_mapTimerFinish.RemoveKey(idTm);
	_mapRTmID.RemoveKey(idTm);
	return TRUE;
}

int KLambdaTimer::DoTimerTask(UINT_PTR nIDEvent)
{
	ASSERT(_wnd);
	//const CWnd* thWnd = dynamic_cast<CWnd*>(this);
	//ASSERT(thWnd == _wnd); 이게 더이상 윈도우를 계승 하지 않을 수 있다.
#ifdef _DEBUG
	if (!AfxIsValidAddress(_wnd, sizeof(CWnd), 0))
	{
		TRACE("DoTimerTask: IsValidAddress return FALSE. >>\n");
		return 1;//0이 아닌 값: 메시지를 처리하지 않았음을 의미 (Windows가 기본 처리를 하도록 함)
	}
#endif // _DEBUG
	if (_hWnd == NULL)/// 이경우 일반 SetTimer 썼나 보다.
		_hWnd = _wnd->GetSafeHwnd();

	if (!::IsWindow(_hWnd))//this가 날라간 경우?를 위해
	{
		TRACE("DoTimerTask: window Handle is invalid. >>\n");
		return 2;
	}

	if (!_mapTimer.Has(nIDEvent))
		return 3;
	auto sid = _mapRTmID[nIDEvent];
	auto tobj = _mapTmObj[sid];
	ASSERT(tobj);
	if (tobj)
	{
		BOOL bLast = tobj->_maxCount > 0 && (tobj->_i + 1) >= tobj->_maxCount;
		if (bLast)
			KillTimerEx(nIDEvent);

		auto t2 = GetTickCount64();
		//TRACE("Lambda Timer: %s(%d) %d msec Task Done!\n", sid.c_str(), tobj->_i, (int)(t2 - tobj->_tickStart));
		tobj->_tickStart = t2;
		auto Fnc = _mapTimer[nIDEvent];
		auto FncFinish = _mapTimerFinish[nIDEvent];
		int i = tobj->_i;
		if (bLast)
			KillLambdaTimer(sid.c_str(), false);/// GetTmObj 로 람다 안에서 부를 수 있는데, 여기서 해제 해버리면 얻으게
		else
			tobj->_i++;
		ASSERT(Fnc);
		KTimerParam pr(i, sid.c_str());
		Fnc(&pr);// i, sid.c_str());
		/// 만약 타이머 안에서 팝업이 뜨면 여기서 계속 기다린다. Kill 하고 맨 나중에 한다.
		if (bLast)
		{
			if (FncFinish)
				FncFinish(&pr);//i, sid.c_str());
		}
	}
	return 0;//0: 메시지를 처리했음을 의미 (Windows에게 "이 메시지는 내가 처리했으니 더 이상 처리하지 마라")
}

