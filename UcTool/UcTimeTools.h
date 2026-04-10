#pragma once
#if CPP17_OR_LATER
#include <optional>
#endif
#include "UcExport.inl"//UCTOOLDYNAMIC
#include "UcTool.h"

#define NULLYEAR 1980
#define NULLMONTH 1
#define NULLDAY   1

enum ETmArea
{
	eCtLocal, // app 이 실행 되는 곳
	eCtUTC,   // GMT
	eCtSvr,   // server machine의 로컬 시각
};

int IsDateHan(WCHAR cd);
int IsDateEng(LPCTSTR cd);

LPCTSTR      UcTimeToString(CString& sTime, bool bSpace, TCHAR cSpDay, TCHAR cSpTime, int y, int m, int d, int hr, int mn, int sc);

//LPCTSTR      UcCTimeToString(CTime cTime, CString& sTime, bool bSpace = true, TCHAR cSpDay = '-', TCHAR cSpTime = ':');

CString UcCTimeToString(CTime t, bool bSpace = true, TCHAR cSpDay = '-', TCHAR cSpTime = ':');

COleDateTime UcAlldigitToOleTime(LPCTSTR strData, int flag = 0);

CTime UcOTimeToCTime(COleDateTime t);

LPCTSTR      UcOTimeToString(COleDateTime t, CString& sTime, bool bSpace = true, TCHAR cSpDay = '-', TCHAR cSpTime = ':');
__time64_t UcSystimeToCtime(SYSTEMTIME& st);
LPSYSTEMTIME UcCStringToTime10(LPCTSTR strData, LPSYSTEMTIME pSt, bool bDateOnly = false);

__time64_t  UcParseTimeStr4(LPCTSTR psEngTime, SYSTEMTIME* pSyt = NULL, bool bDateOnly = false);
inline __time64_t  UcParseTimeStrA4(LPCSTR psEngTime, SYSTEMTIME* pSyt = NULL, bool bDateOnly = false)
{
	CString sw(psEngTime);
	return UcParseTimeStr4(sw, pSyt, bDateOnly);
}


CTime UcGetCurrentTime(int bUTC = eCtLocal);

UCTOOLDYNAMIC
CString UcGetCurrentTimeString(int bUTC = eCtLocal);

CTime UcCStringToCTime(LPCTSTR strData);

std::shared_ptr<SYSTEMTIME> UcSystimeZoneChange(SYSTEMTIME& tSysTime, int minLocal);

/// <summary>
/// 임시 객체를 const 참조로 전달하는 것은 일반적인 패턴
/// </summary>
/// <param name="httpDate">"Mon, 05 Feb 2024 04:23:05 GMT"</param>
/// <param name="outSysTime"></param>
/// <param name="minLocal">0분이면 UTC(GMT), 한국은 +09:00이므로 540</param>
/// <returns></returns>
bool UcParseHttpDate(const std::string& httpDate, SYSTEMTIME& outSysTime, int minLocal = 0);
bool UcParseHttpDateToLocal(const std::string& httpDate, SYSTEMTIME& outSysTime);

std::shared_ptr<SYSTEMTIME> UcTimeZoneToLocal(SYSTEMTIME sysT);
std::shared_ptr<SYSTEMTIME> UcTimeZoneToLocal(CTime t);
std::shared_ptr<SYSTEMTIME> UcTimeZoneToGMT(SYSTEMTIME sysT);

std::shared_ptr<SYSTEMTIME> UcTimeZoneToGMT(CTime t);

CString UcTimeZoneToGmtStr(CTime t);

CTime UcStrTimeToLocal(CString strT);

CString UcStrTimeToLocalStr(CString strT);

CString UcGetCurrentTimeStamp(int bUTC = eCtLocal);

inline CString UcSystimeToString(const SYSTEMTIME* psyt, bool bSpace = true, TCHAR cSpDay = '-', TCHAR cSpTime = ':')
{
	CString sTime;
	UcTimeToString(sTime, bSpace, cSpDay, cSpTime, psyt->wYear, psyt->wMonth, psyt->wDay,
		psyt->wHour, psyt->wMinute, psyt->wSecond);
	return sTime;
}

//"2024-06-01 00:00:00"
bool UcQaTestExpired(CStringA sTimeExpire);
bool UcQaTestExpired(CTime tExp);



class KTimerObj
{
public:
	UINT_PTR _idTimer{ 0 };
	UINT _elapsed{ 1000 };
	int _i{ 0 };// 반복때 마다 1씩 증가. _maxCount -1 하고 멈춘다.
	int _maxCount{ 0 };//0이면 무한정
	CStringA _stat{ "stopped" };
	LPARAM _param{ NULL };
	LONGLONG _tickStart{ 0 };
	LPCSTR _fnc{ NULL };
	int _line{ 0 };
};

#define LAMBDATIMERID 947

class KTimerParam
{
public:
	KTimerParam(int iv, LPCSTR sid)
		: _i(iv), _sid(sid)
	{
	}
	KTimerParam(int iv, std::string& sid)
		: _i(iv), _sid(sid.c_str())
	{
	}
	int _i{ -1 };
	CStringA _sid;
};

/// <summary>
/// 이걸 계승 해서 람다 타이머를 사용 하려면 반드시
/// deprecated : 생성자에서 , KLambdaTimer(this) 를 해줘야 한다.ITimerTaskTool를 계승 하였기에 안해도 된다.
/// new : ITimerTaskTool를 계승 하였기에 
/// 타이머를 쓰지 않을 경우 WM_TIMER 때 dynamic_cast<ITimerTaskTool::ITimerTaskTool*>(this)
///	하여	DoTimerTask 를 호출 할지 판단 한다. OVERRIDE_WINDOWPROC_FOR_POSTMAINTASK 참조
/// </summary>
class UCTOOLDYNAMIC KLambdaTimer
	: public ITimerTaskTool
{
public:
	explicit KLambdaTimer(CWnd* wnd = nullptr)
		//: _wnd(wnd)
	{
		static INT_PTR s_id = LAMBDATIMERID;
		if (wnd) {
			_wnd = wnd;
			_hWnd = _wnd->GetSafeHwnd();//지금 널이겠지.
		}
		//ASSERT(::IsWindow(_hWnd)); 아직 윈도우 생성 안되고 CWnd*만 생겼지.

		s_id++;
		_idx = s_id * 1000;// 나중에 timer id 에서 1000윗쪽 값을 추출 할수 있다.
		KLambdaTimer::GetMapId2This()[_idx] = this;
	}
	~KLambdaTimer()
	{
		//_mapTmObj.DeleteAll();
	}

	//static inline KStdMap<INT_PTR, KLambdaTimer*> _mapIdThis;
	static KStdMap<INT_PTR, KLambdaTimer*>& GetMapId2This() {
		return *GSingleton<KStdMap<INT_PTR, KLambdaTimer*>>::GetInstance("mapTimer");
	}

public:
	/// , KLambdaTimer(this)에서 하지 않고 
	/// SetLambdaTimerImple에서 dynamic_cast<CWnd*>(this)로 가져온다.
	CWnd* _wnd{ nullptr };
	HWND _hWnd{ nullptr };

protected:
	INT_PTR _idx{ 0 };

	//static inline KList<KLambdaTimer*> _queTimer;
	static KList<KLambdaTimer*>& GetQueTimer() {
		return *GSingleton<KList<KLambdaTimer*>>::GetInstance("queTimer");
	}
	//	KStdMap < UINT_PTR, shared_ptr<function<void(LPVOID)>>> _mapTimer;
	KStdMap < UINT_PTR, function<void(LPVOID)>> _mapTimer;
	KStdMap < UINT_PTR, function<void(LPVOID)>> _mapTimerFinish;
	KStdMap <string, UINT_PTR> _mapTmID;
	KStdMap <UINT_PTR, string> _mapRTmID;
	//KStdMapPtr<string, KTimerObj> _mapTmObj;
	KStdMap<string, SHP<KTimerObj>> _mapTmObj;

	/// <summary>
	/// TimerProc 를 쓸경우 윈도우에서 주로 쓰는 timer id와 겹쳐서는 안된다.
	/// 그래서 특별히 아래 값 부터 시작 하여, 
	/// KillTimer(null,)할때도 App전체의 timer id와 중복을 피할수 있다.
	/// </summary>
	int _idTm{ 0 };// 실제 timer id 만들는 _id 합한 값으로 한다.

public:
	/// sid: 이 타이머 고유 ID (문자열)
	/// elapsed: mili seconds 시간 간격
	/// maxCount: 0이면 무한 반복, 아니면 반복 횟수
	/// lmda: 람다 함수 블럭 [](int nth, LPCSTR sid) -> void 형식. nth는 불려진 횟수
	/// 	SetTimerLambda("qrcd", 100, [this](auto pv) {
	///		auto pr = (KTimerParam)pv;//KTimerParam._i, _sid
	///		TimerWork(0);
	///	}, 0);
	virtual void SetTimerLambda(LPCSTR sid, UINT elapsed, function<void(LPVOID)> lmda, int maxCount = 0
		, function<void(LPVOID)> lmdaFinish = NULL, LPCSTR fnc = NULL, int line = 0
	)
	{
		SetLambdaTimerImple(sid, elapsed, lmda, maxCount, lmdaFinish, fnc, line);
	}

protected:
	virtual UINT_PTR SetTimerEx(UINT_PTR nIDEvent, UINT nElapse, TIMERPROC lpfnTimer = nullptr);
	//UINT_PTR SetTimerEx(UINT_PTR nIDEvent, UINT nElapse, void(*lpfnTimer)(HWND, UINT, UINT_PTR, DWORD) = NULL);
	virtual BOOL KillTimerEx(UINT_PTR nIDEvent);

public:
	virtual void SetLambdaTimerImple(LPCSTR sid, UINT elapsed, function<void(LPVOID)> lmda, int maxCount = 0
		, function<void(LPVOID)> lmdaFinish = NULL, LPCSTR fnc = NULL, int line = 0
	);
	void ChangeLambdaTaskImple(LPCSTR sid, function<void(LPVOID)> lmda);

	void ChangeInterval(LPCSTR sid, UINT elapsed);
	int GetInterval(LPCSTR sid);
	SHP<KTimerObj> GetTimerInfo(LPCSTR sid);
	BOOL PauseTimer(LPCSTR sid);
	BOOL RestartTimer(LPCSTR sid);
	BOOL KillLambdaTimer(LPCSTR sid, bool bKill = true);

	int DoTimerTask(UINT_PTR nIDEvent) override;

	SHP<KTimerObj> GetTmObj(LPCSTR sid)
	{
		auto it = _mapTmObj.find(sid);
		if (it != _mapTmObj.end())
			return it->second;
		///?주의: auto tobj = _mapTmObj[sid]; 이렇게 쓰면 키가 없는 경우 만들어져 버린다.
		return nullptr;
	}
	//?deprecated
	BOOL IsExists(LPCSTR sid)
	{
		ASSERT(0);
		return IsTimerExists(sid);
	}
	BOOL IsTimerExists(LPCSTR sid)
	{
		return GetTmObj(sid) != nullptr;
	}


	///  지연된 실행 예 이다.
	/// 이 함수의 취약점. 캡쳐된 변수들이 시간이 지나는 동안 변해도 캡쳐된 값은 최초 넘겨진 값 그대로 이다. 
	//template<typename TFNC>
	void DelayAndRunOnce(LPCSTR sid, UINT elapsed, function<void(LPVOID)> lmda, LPCSTR fnc = NULL, int line = 0)
	{
		if (!IsTimerExists(sid))// 처음 불려 질때
			SetTimerLambda(sid, elapsed, lmda, 1, NULL, fnc, line);
		else// 이미 타이머 작동중인데 또다시 불려졌을때
			RestartTimer(sid);
	}
	/// sample
#ifdef _Sample__
	int count = 3;// count == 0 이면 계속 반복
	SetTimerLambda("test", 1000, [&](int, LPCSTR) {
		// do some thing after 1000 msec
		}, count);

	_tmld.DelayAndRunOnce("delayedJob", 500, [](int, LPCSTR) {
		// 이 영역 코드는 500 msec 전에 재 호출 되면 이전 호출은 무시 된다.
		// 아무리 여러번 호출되어도 500 msec 경과 된 마지막 호출때만 실행된다.
		TRACE("Done\n");
		});
#endif // _Sample__

};

