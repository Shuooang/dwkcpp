// KwLib64A\HttpSvr.cpp
#pragma once

#pragma once
//#define     NOGDI
//#define     NOMINMAX
#include    <winsock2.h>
#include    <ws2tcpip.h>
#include    <objbase.h>
#include    <wtypes.h>
#include <afxmt.h>
#include <http.h>

//#include "Uctool.h"
#include "UcThreadTool.h"
#include "UcBinary.h"
#include "UcJson.h"


#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )\
do \
{	RtlZeroMemory( (resp), sizeof(*(resp)) );           \
	(resp)->StatusCode = (status);                      \
	(resp)->pReason = (reason);                         \
	(resp)->ReasonLength = (USHORT) strlen(reason);     \
} while(FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)  \
do \
{	(Response).Headers.KnownHeaders[(HeaderId)].pRawValue = (RawValue);\
	(Response).Headers.KnownHeaders[(HeaderId)].RawValueLength =(USHORT) strlen(RawValue);\
} while(FALSE)

ULONG GetHeaderUlong(const std::shared_ptr<HTTP_REQUEST> reqBuf, int idx);
CStringW GetHeaderString(const std::shared_ptr<HTTP_REQUEST> reqBuf, int idx);

class UCTOOLDYNAMIC UcReqPack
{
public:
	UcReqPack(HANDLE hReqQ = NULL, SHP<HTTP_REQUEST> pReq = nullptr, int serial = 0);
	virtual ~UcReqPack();

	HANDLE m_hReqQ;
	SHP<HTTP_REQUEST> m_pReq;// to be deleted

	CBufArchive m_arc;

	CCriticalSection m_sec;
	CCriticalSection* _GetCS() { return &m_sec; }

	KStdMap<CString, CString> m_params;

	KBinary m_binr;

	int _resultReceived;
	int _resultSent;

	CException* _pException{ nullptr };

	int GetVerb()	{
		return m_pReq->Verb;
	}
	CStringW GetUrl() {
		return CStringW(m_pReq->pRawUrl);
	}
	bool IsPOST() {
		return GetVerb() == HttpVerbPOST;
	}
	bool IsGET() {
		return GetVerb() == HttpVerbGET;
	}
	bool IsJson() {
		return GetHeaderString(m_pReq, HttpHeaderContentType) == L"application/json";
	}

	//"Content-Type: application/json; charset=utf-8"
	bool IsJsonUtf8(CStringW* pct = nullptr) {
		auto ct = GetHeaderString(m_pReq, HttpHeaderContentType);
		ct.MakeLower();
		if (pct)
			*pct = ct;// charset없으면 utf-8 이 default
		return ct == L"application/json" || ct == L"application/json; charset=utf-8";
	}

	CStringW GetJson() {
		ASSERT(IsJsonUtf8());
		CStringW sJson;
		if (IsJsonUtf8()) {//		"Content-Type: application/json; charset=utf-8"
			auto pUtf8 = m_arc.GetPtr();
			UcUTF8ToWchar((LPCSTR)pUtf8, sJson);
		}
		return sJson;
	}

	SHP<JBase> GetJobj();

	void ResponsJSON(CStringW& sJResp) {
		ASSERT(0);//이거 쓰나?
		CStringA sJsonUtf8;
		UcWcharToUTF8(sJResp, sJsonUtf8);
		this->m_binr.SetPtr((PCSTR)sJsonUtf8, sJsonUtf8.GetLength());
		// SendHttpResponse 에서 알아서 보낸다.
	}
	void ResponsJObj(UcJObj& jResp) {
		//CStringW srJson(jResp.ToJsonStringUtf8());
		//ResponsJSON(srJson);
		auto sJsonUtf8 = jResp.ToJsonStringUtf8();
		this->m_binr.SetPtr((PCSTR)sJsonUtf8, sJsonUtf8.GetLength());
	}

	CStringW ResponseErrorJSON(LPCWSTR fmt, ...);// CString sResult);
	void ResponseErrorJObj(LPCWSTR fmt, ...);// CString sResult);
	
	static CStringW GetHeaderString(const std::shared_ptr<HTTP_REQUEST> reqBuf, int idx);

	static ULONG GetHeaderUlong(const std::shared_ptr<HTTP_REQUEST> reqBuf, int idx);
};

typedef shared_ptr<UcReqPack> ShReqPack;


class UCTOOLDYNAMIC UcHttpSvr
{
public:
	UcHttpSvr(void);
	virtual ~UcHttpSvr(void);

	/// HTTP 요청 본문 읽기·콜백·응답까지 (리스너 스레드 블로킹 방지)
	UcStdThreadPool _pool{ 8, 16 };
	CMutex _mtx;

	HANDLE _httpSvr;
	CStringW _uri;


	int InitSvr(LPCWSTR sUri = NULL);
	ULONG Stop(IN PWS sUri = NULL);
	ULONG Shutdown();
	DWORD DoReceiveRequests(HANDLE hReqQueue);

	DWORD HttpPost_ReadBody(ShReqPack pak);

	int _AsynchResponsePost(ShReqPack pak);
	int _AsynchResponseGet(ShReqPack pak);

	DWORD SendHttpResponse(ShReqPack pak);
	static DWORD SendHttpResponse(IN HANDLE hReqQueue, IN SHP<HTTP_REQUEST> pRequest, IN USHORT StatusCode, IN PSTR pReason, IN PSTR pEntity);

	//DWORD SendHttpResponse2(IN HANDLE hReqQueue, IN PHTTP_REQUEST pRequest);
	virtual int _StartHttpPost(IN HANDLE hReqQ, IN SHP<HTTP_REQUEST> pReq);
	virtual int _StartHttpGet(IN HANDLE hReqQ, IN SHP<HTTP_REQUEST> pReq, CStringW& output);
	int ResponseDefault(ShReqPack pak);
	HRESULT _OnHttpReceivePostBuf(ShReqPack pak);
	HRESULT _OnHttpReceiveGetBuf(ShReqPack pak);

	function<int(ShReqPack)> _fncGET;
	function<int(ShReqPack)> _fncPOST;

	function<int(ShReqPack)> _fncOnConnected;
	function<int(ShReqPack)> _fncOnStarted;
	function<int(ShReqPack)> _fncOnStopped;
	function<int(ShReqPack)> _fncOnDisconnected;
	function<int(ShReqPack)> _fncOnReceived;
	function<int(ShReqPack)> _fncOnSent;
	function<int(ShReqPack)> _fncOnError;
	void AddCallback(PAS sEvent, function<int(ShReqPack)> fnc);

	//ex: 
	// AddCallback("POST", [&](RfReqPack rf) -> int {
	//    // do somthig here
	//		return 0;
	// });
	
	//function<int(ShReqPack, CException*)> _fncOnError;
	//void AddErrorCallback(PAS sEvent, function<int(ShReqPack, CException*)> fnc);
};

#define SyncFnc(pbj)     CSingleLock __sync((pbj)->_GetCS(), TRUE)
#define SyncFncN(pbj, n) CSingleLock __sync##n((pbj)->_GetCS(), TRUE)

///CNgsServerDoc::OnNewDocument
///+ CNgsServerDoc::StartListener
///| + UcHttpSvr.AddCallback
///| + [1] UcThread m_pListenerThread
///| : + UcHttpSvr.InitSvr
///| : | + UcHttpSvr::DoReceiveRequests
///| : | + UcHttpSvr::_StartHttpPost
///| : | + [2] _pool.EnqueueTask (UcStdThreadPool)
///| : | : + UcHttpSvr::_AsynchResponsePost
///| : | : + UcHttpSvr::HttpPost_ReadBody - 요청 읽는다.
///| : | : + UcHttpSvr::ResponseDefault
///| : | : | + UcHttpSvr::_OnHttpReceivePostBuf
///| : | : | : + function<int(ShReqPack)> _fncPOST - 비지니스 로직처리
///| : | : | : | + CNgsServerDoc::OnHttpPost (동기; 별도 앱 풀 없음)
///| : | : | + UcHttpSvr::_OnHttpReceiveGetBuf
///| : | : | : + function<int(ShReqPack)> _fncGET - GET일때는
///| : | : + UcHttpSvr::SendHttpResponse - 응답

/************************************************
			_StartHttpGet:FG case HttpVerbGET
				_fncOnConnected:FG
				KwGetUrlParams:FG - 여기서 있는 파일 그냥 응답 할수도
				_AsynchResponseGet:BG
					ResponseDefault:BG - 여기서 GET 인지 알겠지?
						// POST는 여기에 _OnHttpReceivePostBuf 가 JSON 처리
						_fncGET:BG
			SendHttpResponse
*/


