// KwLib64A\HttpSvr.cpp
#include "pch.h" /// 이 파일은 .. 에 있지만 여기서  "../pch.h" 로 주지 않는다. pre compiled header는 안한다.

#include <winsock2.h>
#include <ws2tcpip.h>

#include "UcThreadTool.h"
#include "UcBaseTools.h"
#include "UcBinary.h"
#include "UcHttpSvr.h"

#pragma comment(lib, "Httpapi.lib")
//#pragma comment (lib, "Ws2_32.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


UcHttpSvr::UcHttpSvr(void)
{
}

UcHttpSvr::~UcHttpSvr(void)
{
}


int  UcHttpSvr::InitSvr(LPCWSTR sUri)
{
	DWKFUNC;
	BACKGROUND(1);
	CSingleLock lk(&_mtx);

	ULONG           retCode = 0;
	HANDLE          hReqQueue = NULL;
	HTTPAPI_VERSION ver1 = HTTPAPI_VERSION_1;
	HTTPAPI_VERSION ver2 = HTTPAPI_VERSION_2;
	if (sUri == NULL)
		sUri = (PWS)_uri;
	ASSERT(tchlen(sUri));
	try
	{
		retCode = HttpInitialize(ver1, HTTP_INITIALIZE_SERVER, NULL);
		if (retCode != NO_ERROR)
		{
			throw_str(_T("HttpInitialize failed with %u \n"), retCode);
			//return retCode;
		}
		KDefer d_init([]() {
			ULONG ru = HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
			});
		retCode = HttpCreateHttpHandle(&hReqQueue, 0); // Req Queue Reserved
		if (retCode != NO_ERROR)
		{
			throw_str(_T("HttpCreateHttpHandle failed with %u \n"), retCode);
			//throw retCode;
		}
		KDefer d_create([hReqQueue, sUri]() { 
			HttpRemoveUrl(hReqQueue, sUri);
			});
		//   "http://www.adatum.com:80/vroot/"
		//   "https://adatum.com:443/secure/database/"
		//   "http://+:80/vroot/"
		_uri = sUri;
		retCode = HttpAddUrl(hReqQueue, sUri, NULL);// "http://+:19670/gps/" Req Queue// Fully qualified URL// Reserved
		if (retCode == NO_ERROR)
		{
			_httpSvr = hReqQueue;
		}
		else if (retCode == ERROR_ACCESS_DENIED)//5L
		{	// 관리자 모드로 VS를 실행 해보라. 방화벽이 안열린경우?
			throw_err_str(ERROR_ACCESS_DENIED, _T("ERROR_ACCESS_DENIED %u on HttpAddUrl(%s): Excute with admin access right"), retCode, sUri);
			//AfxMessageBox(_T("ERROR_ACCESS_DENIED. Excute with admin access right."));
			/// ms-settings:developers 해서 개발자 모드 켜도 이거 떨어지네?
		}
		else if (retCode == ERROR_ALREADY_ASSIGNED)//5L
		{	// 이미 소켓이 사용 되는 경우다. 
			throw_err_str(ERROR_ALREADY_ASSIGNED, _T("ERROR_ALREADY_ASSIGNED %u HttpAddUrl(%s)"), retCode, sUri);
		}
		
		else
		{	// 이미 소켓이 사용 되는 경우다. 관리자 모드로 VS를 실행 해보라.
			ULONG result = -1;
			result = DoReceiveRequests(hReqQueue);// CloseHandle 해도 떨어 진다. ExitInstance
			TRACE(_T("DoReceiveRequests failed with %u\r\n"), result);
			retCode = result;//result
			throw_str(_T("HttpAddUrl failed with %u"), retCode);
			//AfxMessageBox(_T("HttpAddUrl Error!."));
			//throw retCode;
		}

	}
	catch (CException* e)
	{
		e->Delete();// delete e;
	}
	catch (ULONG err)
	{
		TRACE(_T("error retCode(%u)\n"), err);
	}
	catch (...) {}

	//  Cleanup the HTTP Server API
	//HttpRemoveUrl(hReqQueue, sUri);// Req Queue// Fully qualified URL
	//ULONG ru = HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);

	return retCode;
}

ULONG UcHttpSvr::Stop(IN PWS sUri)
{
	CSingleLock lk(&_mtx);
	try
	{
		if (sUri == NULL)
			sUri = (PWS)_uri;
		ULONG ru = HttpRemoveUrl(_httpSvr, sUri);// Req Queue// Fully qualified URL
		ru = HttpShutdownRequestQueue(_httpSvr);
		return ru;//NO_ERROR
	}
	catch (CException* e)
	{
		e->Delete();//delete e;
	}
	return 0;
}
ULONG UcHttpSvr::Shutdown()
{
	try
	{
		ULONG ru = HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
		return ru;
	}
	catch (CException* e)
	{
		e->Delete();//delete e;
	}
	return 0;
}


//HttpHeaderContentLength
ULONG UcReqPack::GetHeaderUlong(const std::shared_ptr<HTTP_REQUEST> reqBuf, int idx)
{
	ULONG ul = 0;
	HTTP_KNOWN_HEADER& contentHeader = reqBuf->Headers.KnownHeaders[idx];
	if (contentHeader.RawValueLength > 0) {
		try {
			ul = std::stoul(contentHeader.pRawValue);
		}
		catch (const std::invalid_argument&) {
			// 잘못된 문자가 포함된 경우 기본값 반환
			ul = 0;
		}
		catch (const std::out_of_range&) {
			// 범위를 벗어난 값인 경우 기본값 반환
			ul = 0;
		}
	}
	return ul;
}

[[deprecated]]
CStringW UcReqPack::ResponseErrorJSON(LPCWSTR fmt, ...)//CString sResult)
{
	CStringFORMAT(buffer, fmt);
	CStringW serr; serr.Format(LR"(
{
  "return":"error", 
  "result":"%s"
}
)", (PWS)buffer);
	return serr;
}

void UcReqPack::ResponseErrorJObj(LPCWSTR fmt, ...)
{
	CStringFORMAT(sError, fmt);
	UcJObj jResp;
	jResp("return") = "error";
	jResp("result") = sError;
	this->ResponsJObj(jResp);
}
/// idx(ex): HttpHeaderContentType
CStringW UcReqPack::GetHeaderString(const std::shared_ptr<HTTP_REQUEST> reqBuf, int idx)
{
	HTTP_KNOWN_HEADER& contentHeader = reqBuf->Headers.KnownHeaders[idx];
	if (contentHeader.RawValueLength > 0) {
		std::string str(contentHeader.pRawValue, contentHeader.RawValueLength);
		//header는 ASCII인 URL 인코딩이다.  그냥 CStringW(str.c_str()); 해도 된다.
		//UcUTF8ToWchar(str.c_str(), swtr);
		CStringW swtr(str.c_str());
		return swtr;
	}
	return {};
	//EX: GetHeaderString(m_pReq, HttpHeaderContentType) == L"application/json";
}
//void AddCustomHeaders(HANDLE hRequestQueue, HTTP_REQUEST_ID requestId) {
//	const char* headers = "Content-Type: application/json\r\nCustom-Header: value\r\n";
//	ULONG result = HttpAddResponseHeaders(hRequestQueue, requestId, HTTP_ADDRESPONSE_FLAG_REPLACE, headers, (USHORT)strlen(headers));
//
//	if (result != NO_ERROR) {
//		//std::cerr << "Failed to add response headers, error: " << result << std::endl;
//	}
//}

//void SetHeaderString(HTTP_RESPONSE& response, const char* headerName, const CString& headerValue) {
//	// CString에서 std::string 또는 C 스타일 문자열로 변환
//	std::string value = CT2A(headerValue);
//
//	// 헤더 설정을 위한 HTTP Server API 호출
//	HttpAddResponseHeaders(
//		responseHandle,                       // 응답 핸들
//		(headerName + ": " + value + "\r\n").c_str(), // 헤더 이름과 값
//		HTTP_ADDREQ_FLAG_REPLACE              // 기존 헤더를 대체
//	);
//}

DWORD UcHttpSvr::DoReceiveRequests(HANDLE hReqQueue)
{
	BACKGROUND(1);

	ULONG              result = 0;
	HTTP_REQUEST_ID    requestId = 0;
	DWORD              bytesRead = 0;
	//PCHAR              pReqBuf;
	ULONG              uReqBufLen = 0;
	// Allocate a 2 KB buffer. This size should work for most 
	// requests. The buffer size can be increased if required. Space
	// is also required for an HTTP_REQUEST structure.
	const int c_uLen = sizeof(HTTP_REQUEST) + (1024 * 4);// 2kb 만 하라고 했는데 4k 한다.
	HTTP_SET_NULL_ID(&requestId);
	// Receive a Request
	for (;;)
	{
		uReqBufLen = c_uLen;

		//PHTTP_REQUEST pReq = (PHTTP_REQUEST)new char[uReqBufLen];//?see reqBuf.Detatch() => CTaskHttp => CHttpSmo::_Response
		//if (pReq == NULL) return ERROR_NOT_ENOUGH_MEMORY;
		//RtlZeroMemory((PHTTP_REQUEST)pReq, uReqBufLen);
		//shared_ptr<HTTP_REQUEST> reqBuf(pReq);//중간에 다시 realloc할수 있으니 반드시 reference로 넘겨 주는 auto free를 써야 한다.
		//CAutoFreePtr<HTTP_REQUEST> reqBuf(pReq);//중간에 다시 realloc할수 있으니 반드시 reference로 넘겨 주는 auto free를 써야 한다.

		//auto reqBuf = std::make_unique<HTTP_REQUEST>();
		//std::shared_ptr<HTTP_REQUEST> reqBuf(
		//	reinterpret_cast<HTTP_REQUEST*>(new char[uReqBufLen]),
		//	[](HTTP_REQUEST* ptr) { delete[] reinterpret_cast<char*>(ptr); }
		//);
		auto reqBuf = MakeSharedBuf<HTTP_REQUEST>(uReqBufLen);
		if (!reqBuf)
			return ERROR_NOT_ENOUGH_MEMORY;

		result = HttpReceiveHttpRequest(hReqQueue, requestId, 0, reqBuf.get(), uReqBufLen, &bytesRead, NULL);
		if (NO_ERROR == result)
		{
			switch (reqBuf->Verb)// POST or GET
			{
			case HttpVerbPOST:
			{
				int rv = _StartHttpPost(hReqQueue, reqBuf);
				if (rv < 0)
					continue;
			} break;
			case HttpVerbGET:
			{
				CStringW output;
				int rv = _StartHttpGet(hReqQueue, reqBuf, output);
				CStringA sa(output);//"Hey! You hit the server \r\n");
				result = SendHttpResponse(hReqQueue, reqBuf, 200, (PSTR)"OK", (PSTR)(LPCSTR)sa);
			}	break;
			default:
				TRACE(L"Got a unknown request for %ws \n", reqBuf->CookedUrl.pFullUrl);
				result = SendHttpResponse(hReqQueue, reqBuf, 503, (PSTR)"Not Implemented", NULL);
				break;
			}

			HTTP_SET_NULL_ID(&requestId);
			if (result != NO_ERROR)
				continue;
		}
		else if (result == ERROR_MORE_DATA)
		{
			//위에 HttpReceiveHttpRequest 를 몇번 더한다.
			requestId = reqBuf->RequestId;
			uReqBufLen = bytesRead;
			reqBuf = MakeSharedBuf<HTTP_REQUEST>(uReqBufLen);
			if (!reqBuf) {
				result = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
		}
		else if (ERROR_CONNECTION_INVALID == result && !HTTP_IS_NULL_ID(&requestId))
		{
			HTTP_SET_NULL_ID(&requestId);
		}
		else if (result == ERROR_INVALID_HANDLE)
		{
			HTTP_SET_NULL_ID(&requestId);
		}
		else//ERROR_OPERATION_ABORTED:Stop()하면
		{
			TRACE(L"HttpReceiveHttpRequest returns %u\n", result);
			break;// CloseHandle 해도 떨어 진다. ExitInstance
		}
	}
	return result;
}

// 여기서 reqBuf는 HttpReceiveHttpRequest 함수 호출로 이미 채워진 HTTP_REQUEST 포인터입니다.
//void ProcessHttpRequest(const std::shared_ptr<HTTP_REQUEST> reqBuf) {
//	if (reqBuf) {
//		// Content-Length 헤더 접근
//		ULONG contentLength = 0;
//		HTTP_KNOWN_HEADER& contentLengthHeader = reqBuf->Headers.KnownHeaders[HttpHeaderContentLength];
//		if (contentLengthHeader.RawValueLength > 0) {
//			contentLength = std::stoul(contentLengthHeader.pRawValue);
//			//std::cout << "Content-Length: " << contentLength << std::endl;
//		}
//
//		// Content-Type 헤더 접근
//		HTTP_KNOWN_HEADER& contentTypeHeader = reqBuf->Headers.KnownHeaders[HttpHeaderContentType];
//		if (contentTypeHeader.RawValueLength > 0) {
//			std::string str(contentTypeHeader.pRawValue, contentTypeHeader.RawValueLength);
//			KwUTF8ToWchar(str.c_str(), contentTypeHeader)
//			//std::cout << "Content-Type: " << contentType << std::endl;
//		}
//	}
//}

// 메인 함수 또는 HTTP 요청을 처리하는 부분에서 이 함수를 호출

DWORD UcHttpSvr::HttpPost_ReadBody(ShReqPack pak)
{
	SyncFnc(pak);
	if ((pak->m_pReq->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS) == 0)
		return 0;
	DWORD           result = NO_ERROR;
	UCHAR pEBuf[10240] = { 0, };
	ULONG uLenEBuf = sizeof(pEBuf);// 10240;

	try
	{
		do // 량이 많은 경우 여러번 읽는다.
		{
			ULONG uRead = 0;
			result = HttpReceiveRequestEntityBody(pak->m_hReqQ, pak->m_pReq->RequestId, 0, pEBuf, uLenEBuf, &uRead, NULL);
			if (result == NO_ERROR)
			{
				if (uRead > 0)
					pak->m_arc.Write(pEBuf, uRead);
			}
			else if (result != ERROR_MORE_DATA)// 234 , ERROR_HANDLE_EOF 38
				throw (DWORD)result;//아래 catch로
		} while (TRUE);
	}
	catch (CException* e)
	{
		pak->_pException = e;
		if (_fncOnError)
			_fncOnError(pak);
	}
	catch (DWORD err)
	{
		switch (err)
		{
		case ERROR_HANDLE_EOF://38
			TRACE(L"Reading has been Completed! ERROR_HANDLE_EOF(%lu) \n", err);
			break;
		case ERROR_INVALID_PARAMETER:
			break;
		default:
			TRACE(L"HttpPost_ReadBody failed with %lu \n", err);
			break;
		}
	}
	pak->_resultReceived = result;
	if (_fncOnReceived)
		_fncOnReceived(pak);
	return result;
}


DWORD UcHttpSvr::SendHttpResponse(
	IN HANDLE        hReqQueue,
	IN SHP<HTTP_REQUEST> pReq,
	IN USHORT        StatusCode,
	IN PSTR          pReason,
	IN PSTR          pEntityString)
{
	DWORD           result = 0;
	DWORD           bytesSent = 0;
	//memset(&response, 0, sizeof(HTTP_RESPONSE));
	//memset(&dataChunk[0], 0, sizeof(HTTP_DATA_CHUNK));
	// Initialize the HTTP response structure.
	//INITIALIZE_HTTP_RESPONSE(&response, StatusCode, pReason);
	HTTP_RESPONSE   response{};
	response.StatusCode = (StatusCode);
	response.pReason = (pReason);
	response.ReasonLength = (USHORT)strlen(pReason);

	// Add a known header.
	ADD_KNOWN_HEADER(response, HttpHeaderContentType, "application/json");//"text/html");

	HTTP_DATA_CHUNK dataChunk[1]{};
	if (pEntityString)
	{
		// Add an entity chunk.
		dataChunk[0].DataChunkType = HttpDataChunkFromMemory;
		dataChunk[0].FromMemory.pBuffer = pEntityString;
		dataChunk[0].FromMemory.BufferLength = (ULONG)strlen(pEntityString);
		response.EntityChunkCount = 1;
		response.pEntityChunks = dataChunk;
	}
	// 
	// Because the entity body is sent in one call, it is not
	// required to specify the Content-Length.
	result = HttpSendHttpResponse(
		hReqQueue,           // ReqQueueHandle
		pReq->RequestId, // Request ID
		0,                   // Flags
		&response,           // HTTP response
		NULL,                // pReserved1
		&bytesSent,          // bytes sent  (OPTIONAL)
		NULL,                // pReserved2  (must be NULL)
		0,                   // Reserved3   (must be 0)
		NULL,                // LPOVERLAPPED(OPTIONAL)
		NULL                 // pReserved4  (must be NULL)
	);
	if (result != NO_ERROR)
	{//87:ERROR_INVALID_PARAMETER
		TRACE(L"HttpSendHttpResponse failed with %lu \n", result);
	}
	return result;
}
/*
int UcHttpSvr::_StartHttpPost(IN HANDLE hReqQ, IN PHTTP_REQUEST pReq)
{	//동기식 방법: 비동기식은 참조 CHSSmo::_StartHttpPost
	int result = -1;
	if(pReq->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
	{
		UcReqPack opak(this, hReqQ, pReq);
		SyncFnc(opak);
		result = HttpPost_ReadBody(&opak);
		result = ResponseDefault(&opak);// hReqQ, pReq, &arc);
	}
	return result;
}*/


/// url 에서 & 로 분리된 파라미터를 = 로 나쥔 키와 값으로 분리
int KwGetUrlParams(PAS pUrl, KStdMap<CString, CString>& params)
{
	CString sUrl(pUrl);//this->m_pReq->pRawUrl);//+	pUrl "/gps/?Req_10NewUser=0"
	int i0 = sUrl.Find('?');//ReverseFind('/');
	if (i0 < 0)
		return 0;

	CString sParams = sUrl.Mid(i0 + 1);
	vector<CString> ar;
	UcCutStrByChar<TCHAR>('&', sParams, [&ar](LPCTSTR s) { ar.push_back((LPCTSTR)s); });
	int npr = 0;
	for(auto ws : ar)
	{
		CString k, v;
		int i1 = ws.Find('=');
		if (i1 >= 0)
		{
			k = ws.Left(i1);
			v = ws.Mid(i1 + 1);
			params.SetAt(k, v);
			npr++;
		}
	}
	return npr;
}


//?see _CreateServer()
int UcHttpSvr::_StartHttpPost(IN HANDLE hReqQ, IN SHP<HTTP_REQUEST> pReq)
{
	BACKGROUND(1);
	int result = 0;
	ShReqPack pak = make_shared<UcReqPack>(hReqQ, pReq);
	if (_fncOnConnected)
	{
		result = _fncOnConnected(pak);
		if (result < 0)
			return -1;
	}

	if (KwGetUrlParams(pak->m_pReq->pRawUrl, pak->m_params) == 0)
	{
		//OpenSiteFile(file);
		//return -1; 파라미터가 없어도 패스
	}

	_pool.EnqueueTask([this, pak]() {
		BACKGROUND(2);
		_AsynchResponsePost(pak);
		}, __FUNCTIONW__, __LINE__);
	return 0;
}

int UcHttpSvr::_StartHttpGet(IN HANDLE hReqQ, IN SHP<HTTP_REQUEST> pReq, CStringW& output)
{
	BACKGROUND(1);
	int result = 0;

	ShReqPack pak = make_shared<UcReqPack>(hReqQ, pReq);
	//UcReqPack* pak = new UcReqPack(hReqQ, pReq);
	if (_fncOnConnected)
	{
		result = _fncOnConnected(pak);
		if (result < 0)
			return -1;
	}
	//CALL_FNC_Rtn(_fncOnConnected, pak, result);

	if (KwGetUrlParams(pak->m_pReq->pRawUrl, pak->m_params) == 0)
	{
		//OpenSiteFile(file);
		return -1;//파라미터가 없으면 파일 열어야지.
	}
	//_pool.ThreadTask([this, pak]()-> void {..}); 이전 방식
	_pool.EnqueueTask([this, pak]() {
		BACKGROUND(2);
		_AsynchResponseGet(pak);
		}, __FUNCTIONW__, __LINE__);
	return 0;
}


//?주의: 위와 class 다름... 이름 비슷
//<= CTaskHttp::DoTask
int UcHttpSvr::_AsynchResponsePost(ShReqPack pak)//CTaskHttp* pTask)
{
	BACKGROUND(2);//1차적으로 서버가 백그이고, 하나 접속이 있으면 백그2로 분기 한다.
	int rv = 0;
	int result = 0;
	try
	{
		if (pak->m_pReq->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
		{
			result = HttpPost_ReadBody(pak);//pak->m_arc 에 POST data가 채워진다.
			if (result != NO_ERROR && result != ERROR_HANDLE_EOF) // 읽은게 없더라도 NO_ERROR 이다.
				throw result;
		}
		rv = ResponseDefault(pak);//_OnHttpReceivePostBuf + SendHttpPostResponse
		//DeleteMeSafe(pak);
	}
	catch (int erv)
	{
		TRACE("_AsynchResponse err(%d)", erv);
	}
	return rv;
}
int UcHttpSvr::_AsynchResponseGet(ShReqPack pak)//CTaskHttp* pTask)
{
	BACKGROUND(2);//1차적으로 서버가 백그이고, 하나 접속이 있으면 백그2로 분기 한다.
	int rv = 0;
	int result = 0;
	try
	{
		ASSERT((pak->m_pReq->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS) == 0);
		rv = ResponseDefault(pak);//_OnHttpReceivePostBuf + SendHttpPostResponse
		//DeleteMeSafe(pak);
	}
	catch (int erv)
	{
		TRACE("_AsynchResponse err(%d)", erv);
	}
	return rv;
}

/// <summary>
/// 여기서 드디어 요청의 리턴값을 만들어 응답한다.
/// </summary>
/// <param name="pak"></param>
/// <returns></returns>
int UcHttpSvr::ResponseDefault(ShReqPack pak)
{
	SyncFnc(pak);
	int rv = 0;

	auto verb = pak->GetVerb();
	switch (verb)// POST or GET
	{
	case HttpVerbPOST:
	{
		HRESULT hr = _OnHttpReceivePostBuf(pak);
	} break;
	case HttpVerbGET:
	{
		HRESULT hr = _OnHttpReceiveGetBuf(pak);
	}	break;
	default:
		break;
	}

	rv = SendHttpResponse(pak);
	return rv;
}



DWORD UcHttpSvr::SendHttpResponse(ShReqPack pak)
{
	//	SyncFnc(pak);
	UINT_PTR len = 0;
	BYTE* pBufToSend = (BYTE*)pak->m_binr.GetPtr(len);
	if (len == 0)
		return 0;
	DWORD           result = 0;
	HTTP_RESPONSE   response;
	DWORD           bytesSent = 0;
	CStringA        szContentLength;//[MAX_ULONG_STR];
	HTTP_DATA_CHUNK dataChunk[1];

	try
	{
		INITIALIZE_HTTP_RESPONSE(&response, 200, "OK");
		//ASSERT(pReq->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);
		// Mobile에서 보내니 이 Flags 가 0이네.
		szContentLength.Format("%d", len);
		ADD_KNOWN_HEADER(response, HttpHeaderContentLength, (LPCSTR)szContentLength);
		//pak->m_tic2 = GetTickCount64(); //응답 보내기 직전 까지 시간

		result = HttpSendHttpResponse(
			pak->m_hReqQ,           // ReqQueueHandle
			pak->m_pReq->RequestId, // Request ID
			HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
			&response,       // HTTP response
			NULL,            // pReserved1
			&bytesSent,      // bytes sent-optional
			NULL, 0, NULL, NULL);
		if (result != NO_ERROR)
			throw result;

		if (pBufToSend && len > 0)
		{
			dataChunk[0].DataChunkType = HttpDataChunkFromMemory;
			dataChunk[0].FromMemory.pBuffer = pBufToSend;//arcR.GetPtr();
			dataChunk[0].FromMemory.BufferLength = (ULONG)len;//arcR.GetLength();

			result = HttpSendResponseEntityBody(pak->m_hReqQ, pak->m_pReq->RequestId,
				0,           // This is the last send.
				1,           // Entity Chunk Count.
				dataChunk, NULL, NULL, 0, NULL, NULL);

			pak->_resultSent = result;

			if (_fncOnSent)
			{
				result = _fncOnSent(pak);
				if (result < 0)
					return -1;
			}

			if (result != NO_ERROR)
				throw result;
		}
	}
	catch (ULONG ue)
	{
		//pak->m_bResponsed = -1;
		switch (ue)
		{
		case ERROR_NOT_ENOUGH_MEMORY:
			TRACE(L"Insufficient resources \n");
			break;
		default:
			TRACE(L"HttpSendHttpResponse failed with %lu \n", ue);
			break;
		}
	}

	return result;
}

UcReqPack::UcReqPack(HANDLE hReqQ, SHP<HTTP_REQUEST> pReq, int serial)
	: m_hReqQ(hReqQ)
	, m_pReq(pReq)
{
}

UcReqPack::~UcReqPack()//?pak
{
}

HRESULT UcHttpSvr::_OnHttpReceivePostBuf(ShReqPack pak)
{
	SyncFnc(pak);
	LPCSTR pUrl = pak->m_pReq->pRawUrl;
	HRESULT hr = S_OK;
	CStringW sUrl(pUrl);
	try {
		ASSERT(_fncPOST);
		if (_fncPOST) {
			hr = _fncPOST(pak);
			return hr;
		}
		else {// 샘플: 고대로 에코 복사
			pak->m_binr.SetPtr((LPCSTR)pak->m_arc.GetPtr(), pak->m_arc.GetLength()); 
		}
	}
	catch (CException*) {
		//DelException(e);
	}
	catch (...) {
		//PushToNotifyException__();
	}
	return hr;
}

HRESULT UcHttpSvr::_OnHttpReceiveGetBuf(ShReqPack pak)
{
	SyncFnc(pak);
	LPCSTR pUrl = pak->m_pReq->pRawUrl;
	HRESULT hr = S_OK;
	CStringW sUrl(pUrl);
	int rv = 0;
	try {
		if (_fncGET) {
			rv = _fncGET(pak);
			return rv;
		}
	}
	catch (CException*) {
		//DelException(e);
	}
	catch (...) {
		//PushToNotifyException__();
	}
	return hr;
}

//void UcHttpSvr::AddErrorCallback(PAS sEvent, function<int(ShReqPack, CException*)> fnc)
//{
//	if (tchsame(sEvent, "Error"))
//		_fncOnError = fnc;
//}
void UcHttpSvr::AddCallback(PAS sEvent, function<int(ShReqPack)> fnc)
{
	if (tchsame(sEvent, "GET"))
		_fncGET = fnc;
	else if (tchsame(sEvent, "POST"))
		_fncPOST = fnc;
	else if (tchsame(sEvent, "Started"))
		_fncOnStarted = fnc;
	else if (tchsame(sEvent, "Stopped"))
		_fncOnStopped = fnc;
	else if (tchsame(sEvent, "Error"))
		_fncOnError = fnc;
	else if (tchsame(sEvent, "Connected"))
		_fncOnConnected = fnc;
	else if (tchsame(sEvent, "Disconnected"))
		_fncOnDisconnected = fnc;
	else if (tchsame(sEvent, "Received"))
		_fncOnReceived = fnc;
	else if (tchsame(sEvent, "Sent"))
		_fncOnSent = fnc;
	else {
		ASSERT(0);
	}
}


SHP<JBase> UcReqPack::GetJobj() 
{
	auto sJson = this->GetJson();
	auto jdoc = UcJson::Parse((LPCWSTR)sJson);
	auto jval = jdoc->Val();
	if (jval) {
		auto jDocData = jval->AsObject();
		return jDocData;
	}
	return {};
}
