#include "UcNetwork.h"
#include "pch.h"
#include <sstream>

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include "UcNetwork.h"
#include "UcTool.h"
#include "UcDebug.h"

/// WinHttpQueryHeaders(WINHTTP_QUERY_STATUS_CODE)
/// NOT by GetLastError : //?주의: GetLastError 로 받은게 아니다.
#define CASE_STR3(ev, nv, det)	case ev: {psErr = _T(#ev); nErr = nv; sDetail = det; sDetail.Trim(); break;}
CString UcHttpStatusStr(DWORD dwRet)
{
	CString psErr, sDetail, srv;
	int nErr{ 0 };
	switch (dwRet)
	{
		CASE_STR3( HTTP_STATUS_CONTINUE           , 100, "OK to continue with request"							 );
		CASE_STR3( HTTP_STATUS_SWITCH_PROTOCOLS   , 101, "server has switched protocols in upgrade header"	 );
		CASE_STR3( HTTP_STATUS_OK                 , 200, "request completed"											 );
		CASE_STR3( HTTP_STATUS_CREATED            , 201, "object created, reason = new URI"						 );
		CASE_STR3( HTTP_STATUS_ACCEPTED           , 202, "async completion (TBS)"									 );
		CASE_STR3( HTTP_STATUS_PARTIAL            , 203, "partial completion"										 );
		CASE_STR3( HTTP_STATUS_NO_CONTENT         , 204, "no info to return"											 );
		CASE_STR3( HTTP_STATUS_RESET_CONTENT      , 205, "request completed, but clear form"					 );
		CASE_STR3( HTTP_STATUS_PARTIAL_CONTENT    , 206, "partial GET fulfilled"									 );
		CASE_STR3( HTTP_STATUS_WEBDAV_MULTI_STATUS, 207, "WebDAV Multi-Status"										 );
		CASE_STR3( HTTP_STATUS_AMBIGUOUS          , 300, "server couldn't decide what to return"				 );
		CASE_STR3( HTTP_STATUS_MOVED              , 301, "object permanently moved"								 );
		CASE_STR3( HTTP_STATUS_REDIRECT           , 302, "object temporarily moved"								 );
		CASE_STR3( HTTP_STATUS_REDIRECT_METHOD    , 303, "redirection w/ new access method"						 );
		CASE_STR3( HTTP_STATUS_NOT_MODIFIED       , 304, "if-modified-since was not modified"					 );
		CASE_STR3( HTTP_STATUS_USE_PROXY          , 305, "redirection to proxy, location header specifies proxy to use");
		CASE_STR3( HTTP_STATUS_REDIRECT_KEEP_VERB , 307, "HTTP/1.1: keep same verb"								 );
		CASE_STR3( HTTP_STATUS_PERMANENT_REDIRECT , 308, "Object permanently moved keep verb"					 );
		CASE_STR3( HTTP_STATUS_BAD_REQUEST        , 400, "invalid syntax"												 );
		CASE_STR3( HTTP_STATUS_DENIED             , 401, "access denied"												 );
		CASE_STR3( HTTP_STATUS_PAYMENT_REQ        , 402, "payment required"											 );
		CASE_STR3( HTTP_STATUS_FORBIDDEN          , 403, "request forbidden"											 );
		CASE_STR3( HTTP_STATUS_NOT_FOUND          , 404, "object not found"											 );
		CASE_STR3( HTTP_STATUS_BAD_METHOD         , 405, "method is not allowed"									 );
		CASE_STR3( HTTP_STATUS_NONE_ACCEPTABLE    , 406, "no response acceptable to client found"				 );
		CASE_STR3( HTTP_STATUS_PROXY_AUTH_REQ     , 407, "proxy authentication required"							 );
		CASE_STR3( HTTP_STATUS_REQUEST_TIMEOUT    , 408, "server timed out waiting for request"				 );
		CASE_STR3( HTTP_STATUS_CONFLICT           , 409, "user should resubmit with more info"					 );
		CASE_STR3( HTTP_STATUS_GONE               , 410, "the resource is no longer available"					 );
		CASE_STR3( HTTP_STATUS_LENGTH_REQUIRED    , 411, "the server refused to accept request w/o a length");
		CASE_STR3( HTTP_STATUS_PRECOND_FAILED     , 412, "precondition given in request failed"				 );
		CASE_STR3( HTTP_STATUS_REQUEST_TOO_LARGE  , 413, "request entity was too large"							 );
		CASE_STR3( HTTP_STATUS_URI_TOO_LONG       , 414, "request URI too long"										 );
		CASE_STR3( HTTP_STATUS_UNSUPPORTED_MEDIA  , 415, "unsupported media type"									 );
		CASE_STR3( HTTP_STATUS_RETRY_WITH         , 449, "retry after doing the appropriate action."			 );
		CASE_STR3( HTTP_STATUS_SERVER_ERROR       , 500, "internal server error"									 );
		CASE_STR3( HTTP_STATUS_NOT_SUPPORTED      , 501, "required not supported"									 );
		CASE_STR3( HTTP_STATUS_BAD_GATEWAY        , 502, "error response received from gateway"				 );
		CASE_STR3( HTTP_STATUS_SERVICE_UNAVAIL    , 503, "temporarily overloaded"									 );
		CASE_STR3( HTTP_STATUS_GATEWAY_TIMEOUT    , 504, "timed out waiting for gateway"							 );
		CASE_STR3( HTTP_STATUS_VERSION_NOT_SUP    , 505, "HTTP version not supported"								 );
		//CASE_STR0(HTTP_STATUS_FIRST              , HTTP_STATUS_CONTINUE
		//CASE_STR0(HTTP_STATUS_LAST               , HTTP_STATUS_VERSION_NOT_SUP
	default:
	srv.Format(_T("HTTP_STATUS_UnknownError(%u): unkown"), dwRet);
	}
	if(srv.IsEmpty())
		srv.Format(_T("%s(%d): %s"), psErr.GetString(), nErr, sDetail.GetString());
	return srv;
}

/*
CString UcGetHeaderStr(HINTERNET hRequest, int key)
{
	int len = 1024;
	DWORD bufferSize = len;
	CString sbuf;
	WinHttpQueryHeaders(hRequest, key, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &bufferSize, WINHTTP_NO_HEADER_INDEX);//		bufferSize	60	unsigned long
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		if ((int)bufferSize > len)
			len = bufferSize;
		if (WinHttpQueryHeaders(hRequest, key, WINHTTP_HEADER_NAME_BY_INDEX, sbuf.GetBuffer(len), &bufferSize, WINHTTP_NO_HEADER_INDEX))//bufferSize = 58
			sbuf.ReleaseBuffer();//              여기서 위에서 받은 값을 넣어도 되지만, 넉넉히 1024를 넣는다.
	}
	return sbuf;
};

int UcGetHeaderNum(HINTERNET hRequest, int key)
{
	CString sData = UcGetHeaderStr(hRequest, key);
	if (sData.GetLength() > 0)
	{
		int nData = _wtoi(sData);
		return nData;
	}
	return -1;
};
*/


void KWinHttp::Connect(CStringW sDmn, INTERNET_PORT iPort/* = INTERNET_DEFAULT_HTTP_PORT*/, BOOL bSSL/* = FALSE*/)
{
	CHECKELAPSED;
	ASSERT(iPort != 8000);
	if (_hSession == NULL)
	{
		if (std::get<0>(_openParam).GetLength() > 0)
			Open();
		else
			throw_str(L"Internet never opened. Call Open() first");
	}
	bool bToConnect = _hConnect == NULL;
	if (!bToConnect)
	{//이전 연결이 되어 있고
		//auto lastCnct = _lstConnect.back();
		if (!_lstConnect.empty())
		{
#if CPP17_OR_LATER
			auto [prDmn, prePort, preSSL] = _lstConnect.back();
#else
			auto tuple_val = _lstConnect.back();
			auto prDmn = std::get<0>(tuple_val);
			auto prePort = std::get<1>(tuple_val);
			auto preSSL = std::get<2>(tuple_val);
#endif
			if (sDmn != prDmn || iPort != prePort || bSSL != preSSL)// 이전 연결 조건과 다르면
				bToConnect = true;
		}
		else
			throwLINE;
	}

	if (bToConnect)
	{
		this->CloseAll(false);//false: session은 Close하지 않는다.
		//HINTERNET WINAPI WinHttpConnect(HINTERNET hSession, LPCWSTR pswzServerName, INTERNET_PORT nServerPort, DWORD dwReserved);
		_hConnect = WinHttpConnect(_hSession, sDmn, iPort, 0); //INTERNET_DEFAULT_HTTP_PORT, 0);
		if (_hConnect == NULL)// fake.ip 를 줘도 연결은 되네?
			throwLINE;
		auto tpl = make_tuple(sDmn, iPort, bSSL);
		_lstConnect.push_back(tpl);
		if (_lstConnect.size() > 10)//maximum 10개 까지 보관
			_lstConnect.pop_front();
		//_port = iPort;
		//_domain = sDmn;
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="sUrl"></param>
/// <param name="nTimeout"></param>
/// <param name="sVerb">L"GET"</param>
/// <param name="cbDir"></param>
void KWinHttp::DownLoadRequest(CStringW sUrl, int nTimeout, CStringW sVerb
	, function<void(CStringW& sDirDn)> cbDir
	, vector<CStringW> headers
	, function<void(shared_ptr<char>&, ULONG64&)> cbDataToUpload
)
{
	auto& oHttp = *this;
	int iCSSl = sUrl.Find(L"://"); if (iCSSl < 0)
		throwLINE;

	CStringW http = sUrl.Left(iCSSl);//http = L"http"
	http.MakeLower();
	BOOL bSSL = http == L"https";
	CStringW sAddr = sUrl.Mid(iCSSl + 3); // `https://` 를 제외한 나머지 주소 sAddr = L"localhost:8000/update/Pro/patch_info.json"
	int iSl = sAddr.Find('/', iCSSl); // directory 시작 부분 찾기

	CStringW sDmn(sAddr), sDir;
	int iPort = bSSL ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT; // 443 : 80
	
	
	if (iSl >= 0)
		sDmn = sAddr.Left(iSl);//sDmn = L"localhost:80"
	int iColon = sDmn.Find(':');
	CStringW sPort;
	if (iColon >= 0 && (iSl == -1 || iColon < iSl)) // ':' 이 '/' 보다 앞에 있어야
	{
		if (iSl >= 0)
			sPort = sDmn.Mid(iColon + 1, iSl - (iColon + 1));
		else
			sPort = sDmn.Mid(iColon + 1);

		iPort = UcAtoi((PWS)sPort); // iPort = 8000
		sDmn = sDmn.Left(iColon);//sDmn = L"localhost"
	}
	if (iSl >= 0)//sDir = L"/api/v1/upload?fileName=IK_GUID.json&path=update/newprod4" (NodeJS 에 query시에는 HTML encoding을 안한다.)
		sDir = sUrl.Mid(iSl + 8); // 여기 까지, 파일 포함 sDir = L"update/Pro/patch_info.json"
	// sDir = L"update/Pro/CADian_12.1.260.33187.P.VC16.x64.Alpha/3DConnexionModule_23.12_16.irx.capch"
	CStringW sDirDn(sDir);
	/// URL 부분을 뗀 부분을 추가로 조작할 일이 있으면, 람다함수에서 한다.
	if (cbDir)
		cbDir(sDirDn);
	//CStringW sDirHttp;//, sDmnHttp;
	///?주의: UcWcharToUTF8ToHtmlUrl(sDmn, sDmnHttp);//sDmn = L"localhost" 도메인 http문자열로 바꾸면 https에서 Connect가 안된다.
	//UcWcharToUTF8ToHtmlUrl(sDirDn, sDirHttp);//sDirDn = L"update/Pro/patch_info.json"
	///sDirDn = L"/update/pro/33106/3DConnexionModule_23.12_16.irx.deflate"

	if (nTimeout > 0)
	{
		//oHttp.SetTimeout(WINHTTP_OPTION_CONNECT_TIMEOUT, nTimeout);
		oHttp.SetTimeout(WINHTTP_OPTION_RECEIVE_TIMEOUT, nTimeout);
		//oHttp.SetTimeout(WINHTTP_OPTION_SEND_TIMEOUT, nTimeout);
	}

	try
	{//sDmn = L"update.cadian.com" iPort = 443 bSSL = 1
		ASSERT(iPort != 8000);
		oHttp.Connect(sDmn, iPort, bSSL);
		oHttp.OpenRequest(sDirDn, sVerb);//sDirDn = L"/update/logon/61.97.120.203/CADIAN-DEV-DWK/keeps/newprod4/NodeJS_EnglishFileName.json"
		//iPort = 443

		//if (cbHeader)// header  추가 할거 있으면 여기서 한다.
		//	cbHeader();
		for(auto& hd : headers)
		{
			if (!oHttp.AddRequestHeaders(hd))
				throwLINE;
		}
		//- headers{ size = 5 }	std::vector<CStringW>
		//	+[0]	L"Accept: */*"	
		//	+[1]	L"Accept-Encoding: gzip, deflate, br"	
		//	+[2]	L"Connection: keep-alive"	
		//	+[3]	L"Content-Type: application/octet-stream"	
		//	+[4]	L"Content-Length: 1046"	

		///여기서 response 시간이 걸린다.
		LPVOID pData{ WINHTTP_NO_REQUEST_DATA };
		shared_ptr<char> shData;
		ULONG64 lenData{};
		if (cbDataToUpload)
			cbDataToUpload(shData, lenData);/// 여기서 데이터를 읽어 온다.

		if (shData)
			pData = shData.get();
		BOOL bSend = oHttp.SendRequest(WINHTTP_NO_ADDITIONAL_HEADERS, 0, pData, (DWORD)lenData, (DWORD)lenData);/// FALSE이면 TIMEOUT 일 공산이 크다.
	}
	catch (KException*)
	{
		//DWORD err = GetLastError();
		//TRACE(L"%s WinHttpQueryHeaders %s %u\n", __FUNCTIONW__, UcErrorToStrW(err), err);
		throw;//?ERROR_WINHTTP_TIMEOUT 1.1 SendRequest
	}
	catch (CException*)
	{
		throw;
	}
}
void KWinHttp::DownLoadRequestDirect(CStringW sUrl, int nTimeout, CStringW sVerb
	, function<void(CStringW& sDirDn)> cbDir
	, vector<CStringW> headers
	, function<void(LPSTR&, ULONG64&)> cbDataToUpload
)
{
	auto& oHttp = *this;
	int iCSSl = sUrl.Find(L"://"); if (iCSSl < 0)
		throwLINE;

	CStringW http = sUrl.Left(iCSSl);//http = L"http"
	http.MakeLower();
	BOOL bSSL = http == L"https";
	CStringW sAddr = sUrl.Mid(iCSSl + 3); // `https://` 를 제외한 나머지 주소 sAddr = L"localhost:8000/update/Pro/patch_info.json"
	int iSl = sAddr.Find('/', iCSSl); // directory 시작 부분 찾기

	CStringW sDmn(sAddr), sDir;
	int iPort = bSSL ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT; // 443 : 80


	if (iSl >= 0)
		sDmn = sAddr.Left(iSl);//sDmn = L"localhost:80"
	int iColon = sDmn.Find(':');
	CStringW sPort;
	if (iColon >= 0 && (iSl == -1 || iColon < iSl)) // ':' 이 '/' 보다 앞에 있어야
	{
		if (iSl >= 0)
			sPort = sDmn.Mid(iColon + 1, iSl - (iColon + 1));
		else
			sPort = sDmn.Mid(iColon + 1);

		iPort = UcAtoi((PWS)sPort); // iPort = 
		sDmn = sDmn.Left(iColon);//sDmn = L"localhost"
	}
	ASSERT(iPort != 8000);
	if (iSl >= 0)//sDir = L"/api/v1/upload?fileName=IK_GUID.json&path=update/newprod4" (NodeJS 에 query시에는 HTML encoding을 안한다.)
		sDir = sUrl.Mid(iSl + 8); // 여기 까지, 파일 포함 sDir = L"update/Pro/patch_info.json"
	// sDir = L"update/Pro/CADian_12.1.260.33187.P.VC16.x64.Alpha/3DConnexionModule_23.12_16.irx.capch"
	CStringW sDirDn(sDir);
	/// URL 부분을 뗀 부분을 추가로 조작할 일이 있으면, 람다함수에서 한다.
	if (cbDir)
		cbDir(sDirDn);
	//CStringW sDirHttp;//, sDmnHttp;
	///?주의: UcWcharToUTF8ToHtmlUrl(sDmn, sDmnHttp);//sDmn = L"localhost" 도메인 http문자열로 바꾸면 https에서 Connect가 안된다.
	//UcWcharToUTF8ToHtmlUrl(sDirDn, sDirHttp);//sDirDn = L"update/Pro/patch_info.json"
	///sDirDn = L"/update/pro/33106/3DConnexionModule_23.12_16.irx.deflate"

	if (nTimeout > 0)
	{
		//oHttp.SetTimeout(WINHTTP_OPTION_CONNECT_TIMEOUT, nTimeout);
		oHttp.SetTimeout(WINHTTP_OPTION_RECEIVE_TIMEOUT, nTimeout);
		//oHttp.SetTimeout(WINHTTP_OPTION_SEND_TIMEOUT, nTimeout);
	}

	try
	{//sDmn = L"update.cadian.com"
		oHttp.Connect(sDmn, iPort, bSSL);//sDmnHttp = L"localhost" iPort = 8000
		oHttp.OpenRequest(sDirDn, sVerb);//sDirDn = L"/update/IK_GUID.json" 이건 매번 다시 해야 한다.
		//iPort = 443

		//if (cbHeader)// header  추가 할거 있으면 여기서 한다.
		//	cbHeader();
		for (auto& hd : headers)
		{
			if (!oHttp.AddRequestHeaders(hd))
				throwLINE;
		}
		//- headers{ size = 5 }	std::vector<CStringW>
		//	+[0]	L"Accept: */*"	
		//	+[1]	L"Accept-Encoding: gzip, deflate, br"	
		//	+[2]	L"Connection: keep-alive"	
		//	+[3]	L"Content-Type: application/octet-stream"	
		//	+[4]	L"Content-Length: 1046"	

		///여기서 response 시간이 걸린다.
		LPVOID pData{ WINHTTP_NO_REQUEST_DATA };
		LPSTR shData{};
		ULONG64 lenData{};
		if (cbDataToUpload)
			cbDataToUpload(shData, lenData);/// 여기서 데이터를 읽어 온다.

		if (shData)
			pData = shData;// .get();
		BOOL bSend = oHttp.SendRequest(WINHTTP_NO_ADDITIONAL_HEADERS, 0, pData, (DWORD)lenData, (DWORD)lenData);/// FALSE이면 TIMEOUT 일 공산이 크다.
		_break;
	}
	catch (KException*)
	{
		//DWORD err = GetLastError();
		//TRACE(L"%s WinHttpQueryHeaders %s %u\n", __FUNCTIONW__, UcErrorToStrW(err), err);
		throw;//?ERROR_WINHTTP_TIMEOUT 1.1 SendRequest
	}
	catch (CException*)
	{
		throw;
	}
}


void KWinHttp::DownLoadWrite(CFile& file, function<void(ULONG64)> cbFinish /*= NULL*/)
{
	DownLoadWrite([this, &file](LPVOID pBuf, ULONG64 , ULONG64 dwDownloaded, ULONG64) -> int {
		if(dwDownloaded > 0)
			file.Write(pBuf, (UINT)dwDownloaded);
		return 0;
		}, cbFinish);
}


void KWinHttp::DownLoadWrite(function<int(
	LPVOID,  // 현재 다운로드한 버퍼 포인터
	ULONG64, // 현재 타겟 위치: 0 부터 시작
	ULONG64, // 읽은 버퍼 크기
	ULONG64  // 토탈 크기
	)> cbWrite, function<void(ULONG64)> cbFinish /*= NULL*/)
{
	auto& oHttp = *this;
	ULONG64 dwTillNow{ 0 };

	KAtEnd defef([this, &cbFinish, &dwTillNow]() {
		if (cbFinish)
			cbFinish(dwTillNow);//마무리
		this->CloseRequest();
		});

	ASSERT(_dwContentLength > 0);

	DWORD dwDownloaded = 0;
	int i = 0;
	while (1)
	{
		KAtEnd ipp([&i]() {i++; });
		DWORD dwToRead = 0;
		if (!oHttp.QueryDataAvailable(&dwToRead))
		{
			//TRACE(" ###########     Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
			no_throw_str(L" ###########     Error %u in WinHttpQueryDataAvailable.", GetLastError());
		}
		if (dwToRead == 0)
		{//여기는 안온다. 
			no_throw_str(L"  dwToRead(%.2fK) == 0 then break; %.2fK/%.2fM >>>>>>>>>>>>>>>>>", dwToRead / 1'000., dwTillNow / 1'000., _dwContentLength / 1'000'000.);
			break;
		}
		//TRACE(L"  dwToRead(%5.2fK) %8.2fK/%8.2fM\n", dwToRead/1'000., dwTillNow/1'000., _dwContentLength/1'000'000.);
		//TRACE("%u DataAvailable\n", dwSizeInOut);
		auto shBuf = SharedBuf(dwToRead + 1);//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
		if (!shBuf)
		{
			dwToRead = 0;
			throwLINE;
		}
		LPSTR pBuf = shBuf.get();
#ifdef _DEBUG
		ASSERT(dwToRead < 10240);
		char szBuf[10240]{};
		pBuf = szBuf;
#endif // _DEBUG
		if (oHttp.ReadData((LPVOID)pBuf, dwToRead, &dwDownloaded))
		{
			if (dwDownloaded > 0)
			{

				/// ////////////////////////////// ///
				auto rv = cbWrite(pBuf, // 현재 다운로드한 버퍼 포인터
					dwTillNow,			// 현재 타겟 위치: 0 부터 시작
					dwDownloaded,		// 읽은 버퍼 크기
					_dwContentLength);	// 토탈 크기
				/// ////////////////////////////// ///

				dwTillNow += dwDownloaded;

				if (0 < _dwContentLength && dwTillNow == _dwContentLength)
				{// 여기서 성공 여부가 갈린다.
					//TRACE("%d. dwDownloaded(%5u) dwTillNow:%.2fM/_dwContentLength:%.2fM then break >>>>>>>>>>>>>>>>>\n", i, dwDownloaded, dwTillNow/1'000'000., dwContentLength/1'000'000.);
					cbWrite(NULL, dwTillNow, 0, _dwContentLength);//end
					break;
				}
			}
			else
			{
				TRACE("%d. dwDownloaded(%5u) dwTillNow:%.2fM/_dwContentLength: %.2fM\n", i, dwDownloaded, dwTillNow / 1'000'000., _dwContentLength / 1'000'000.);
				cbWrite(NULL, dwTillNow, -1, _dwContentLength);// 0xffffffff no more : 이게 필요할까?
				//TRACE("[%d].  dwToRead: %u : %u/%.1fM loaded\n", iDL, dwToRead, dwDownloaded, dwTillNow/1'000'000.);
				break;
			}
			//				TRACE("%u : %u/%u loaded\n", dwToRead, dwDownloaded, dwTillNow);
		}
		else
		{
			no_throw_str(L"###########      Error %u in WinHttpReadData.\n", GetLastError());
		}
	};// while (dwSizeInOut > 0);
}


void KWinHttp::OpenRequest(CStringW sDirDn, CStringW sVerb, LPCWSTR pwszVersion, LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes, DWORD dwFlags)
{
	CHECKELAPSED;
	UCASSERT(_hConnect);
	if (_hRequest)
		CloseRequest();
	//UCASSERT(_hRequest == NULL);
	//HINTERNET WinHttpOpenRequest(HINTERNET hConnect, LPCWSTR pwszVerb, LPCWSTR pwszObjectName, LPCWSTR pwszVersion, LPCWSTR pwszReferrer, LPCWSTR * ppwszAcceptTypes, DWORD dwFlags);
	CStringW sDirHttp;//, sDmnHttp;
	UcWcharToUTF8ToHtmlUrl(sDirDn, sDirHttp);//sDirDn = L"update/Pro/patch_info.json"
	if (!_lstConnect.empty())
	{
#if CPP17_OR_LATER
		auto [prDmn, prePort, preSSL] = _lstConnect.back();
#else
		auto tuple_val = _lstConnect.back();
		auto prDmn = std::get<0>(tuple_val);
		auto prePort = std::get<1>(tuple_val);
		auto preSSL = std::get<2>(tuple_val);
#endif
		if (preSSL)//_bSecure)
			dwFlags |= WINHTTP_FLAG_SECURE;
		if (_hConnect)
		{
			_hRequest = WinHttpOpenRequest(_hConnect, sVerb, sDirHttp, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags);
			if (_hRequest == NULL)//엉뚱한 주소를 줘도 여기 까지는 진행 한다.
				throwLINE;
		}
		else
			throwLINE;
	}
	else
		throw_str(L"Internet never connected. Call Open() first");
	_sUrlTail = sDirDn;
}

BOOL KWinHttp::SendRequest(CStringW lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext)
{
	CHECKELAPSED;
	ASSERT(_hRequest);
	//BOOL WinHttpSendRequest(HINTERNET hRequest,  LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext);
	//BOOL WinHttpReceiveResponse(HINTERNET hRequest, LPVOID lpReserved);
	BOOL bResults = TRUE;
	if (_hRequest)
	{
		bResults = WinHttpSendRequest(_hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
		if (bResults)/// 이 위에서 ERROR_WINHTTP_TIMEOUT 나면 아래에서 bResult가 == FALSE가 된다.
		{
			bResults = WinHttpReceiveResponse(_hRequest, NULL);// FALSE 역시 받아야 결과를 아는 군.
			if (!bResults)
			{
				if (UcErrorFound(ERROR_WINHTTP_TIMEOUT))//여러 에러중 이게 있냐?
				{
					//+[  122]	L"ERROR_INSUFFICIENT_BUFFER(  122)"	
					//+[12002]	L"ERROR_WINHTTP_TIMEOUT    (12002)"	
					throw_gen(ERROR_WINHTTP_TIMEOUT, L"WinHttpReceiveResponse");//?ERROR_WINHTTP_TIMEOUT 1 SendRequest //?CDANAL
					///?결론: 시간 지체는 WinHttpSendRequest에서, ERROR_WINHTTP_TIMEOUT는 WinHttpReceiveResponse에서.
				}
			}
		}
		else
		{
#ifdef _DEBUGx
			try
			{
				_dwContentLength = GetHeaderNum(WINHTTP_QUERY_CONTENT_LENGTH);
				int iStaus = GetHeaderNum(WINHTTP_QUERY_STATUS_CODE);
			}
			catch (CException* e)
			{
				auto err = GetLastError();
				auto sErr = UcErrorToStrW(err); //+sErr	L"ERROR_INSUFFICIENT_BUFFER(122)"
				TRACE(L"%s\n", (PS)sErr);
			}
#endif // _DEBUG
			auto err = GetLastError();// 없는 주소에 접근 하면
			auto sErr = UcErrorToStrW(err); //+		sErr	L"ERROR_WINHTTP_NAME_NOT_RESOLVED(12007)"도메인 이름을 호스트 이름으로 변환할 수 없음
			throw_str(L"WinHttpSendRequest:%s", (PWS)sErr);//?ERROR_WINHTTP_TIMEOUT 1 SendRequest
		}
		//throwLINE;/// 신기한게 Send가 타임아웃인데, TRUE 리턴이고, Receive는 빨리 응답 하지만 FALSE를 리턴 하구만.
	}
	return bResults;
}

DWORD KWinHttp::QueryHeadersSize(DWORD key, LPCWSTR pwszName)
{
	CHECKELAPSED;
	ASSERT(_hRequest);
	//BOOL WinHttpQueryHeaders(HINTERNET hRequest, DWORD dwkey, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD   lpdwIndex)
	DWORD dwBufferLength{ 0 };
	if (!WinHttpQueryHeaders(_hRequest, key, pwszName, NULL, &dwBufferLength, WINHTTP_NO_HEADER_INDEX))
	{
		auto errs = UcGetLastErros();
		if (errs->Lookup(ERROR_INSUFFICIENT_BUFFER))//여러 에러중 이게 있냐?
		{
			/// 이건 버퍼 크기 알아 내는 정상적인 과정
		}
		else // 기타 에러는 throw
		{
#if CPP17_OR_LATER
			for (auto& [kerr, serr] : *errs)
			{
#else
			for (auto& pair : *errs)
			{
				auto kerr = pair.first;
				auto serr = pair.second;
#endif
				if (kerr != ERROR_INSUFFICIENT_BUFFER)
				{
					TRACE(L"%s WinHttpQueryHeaders %s\n", __FUNCTIONW__, serr);
					throw_err(kerr);// 저거 빼고 다른 에러가 더 있으면 throw 한개만 하겠지.
				}
			}
		}
	}
	return dwBufferLength;
}

CStringW KWinHttp::GetHeaderStr(int key, function<void(CStringW)> cbHeader)
{
	CHECKELAPSED;
	ASSERT(_hRequest);
	CStringW sbuf;
	try
	{
		DWORD dwLen = QueryHeadersSize(key);
		if (dwLen == 0)
			dwLen = 1024;
		if (dwLen > 0)
		{
			DWORD dwLenRv = dwLen;
			if (!QueryHeaders(key, WINHTTP_HEADER_NAME_BY_INDEX, sbuf.GetBuffer(dwLen), &dwLenRv))//bufferSize = 58
			{
				auto err = GetLastError();
				if (err != ERROR_WINHTTP_HEADER_NOT_FOUND)
					throwLINE;
			}
			//return UcGetHeaderStr(_hRequest, key);
			sbuf.ReleaseBuffer();
			if (cbHeader)
				cbHeader(sbuf);
		}
		else
		{
			_break;//WINHTTP_QUERY_LAST_MODIFIED : KHttpServer에서 제공 해줘야
		}
	}
	catch (KException* e)
	{
		// winpache 로 접속 하면 WINHTTP_QUERY_LAST_MODIFIED가 없다.
		TRACE(L"%s WinHttpQueryHeaders %s %u\n", __FUNCTIONW__, UcErrorToStrW(e->_error), e->_error);
		if (e->_error == ERROR_WINHTTP_HEADER_NOT_FOUND)
		{
			if (cbHeader)
				cbHeader(sbuf);
			return sbuf;
		}
		else
			throw;
	}
	catch (CException*)
	{
		throw;
	}
	return sbuf;
}


//#include "UcBaseTools.h"
#include <iomanip>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
CStringW UcGetMACAddress()
{
	DWORD dwSize = 0;
	ULONG outBufLen = 15000; // 초기 버퍼 크기 설정
	ULONG retVal = 0;
	CStringA sbuf;
	//auto pBuffer = static_cast<BYTE*>(sbuf.GetBuffer(outBufLen));
	auto pBuffer = sbuf.GetBuffer(outBufLen);
	CStringW rs;

	KAtEnd defer([&sbuf]() {
		sbuf.ReleaseBuffer();
	});
	auto FGetAddr = [&pBuffer, &outBufLen]() -> ULONG {
		return GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(pBuffer), &outBufLen);
		};
	// 네트워크 어댑터 정보 가져오기
	if ((retVal = FGetAddr()) == ERROR_BUFFER_OVERFLOW)
	{
		sbuf.ReleaseBuffer();
		if(outBufLen <= 0)
			return rs;
		pBuffer = sbuf.GetBuffer(outBufLen);
	}

	if ((retVal = FGetAddr()) == NO_ERROR)
	{
		// 어댑터 정보 순회
		auto pCurrAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(pBuffer);
		while (pCurrAddresses) {
			//std::cout << "Adapter name: " << pCurrAddresses->AdapterName);
			//Adapter name: {EBAF23B7-FDD7-4399-A65A-82B42F62CE06}
			// MAC 주소
			std::wstringstream ss;
			//ss << "MAC Address: ";
			for (UINT i = 0; i < pCurrAddresses->PhysicalAddressLength; i++) {
				//3>C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\iomanip(30,23)
				// : error C2338: static_assert failed: 'wrong character type for setfill'
				CStringW sw;sw.Format(L"%2x", (int)pCurrAddresses->PhysicalAddress[i]);
				//ss << std::hex << std::setfill('0') << std::setw(2) << (int)pCurrAddresses->PhysicalAddress[i];
				ss << (LPCWSTR)sw;
				if (i != (pCurrAddresses->PhysicalAddressLength - 1))
					ss << "-";
			}
			//ss);
			if (ss.str().length() > 0)//MAC Address: 04-7c-16-b6-27-a0
			{
				rs = ss.str().c_str();
				break;
			}
			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else {
		//std::cout << "GetAdaptersAddresses failed with error: " << retVal);
	}
	return rs;
}


void KWinHttp::DownLoadResponse(function<void(std::vector<BYTE>, ULONG64)> cbFinish /*= NULL*/)
{
	auto& oHttp = *this;
	ULONG64 dwTillNow{ 0 };

	std::vector<BYTE> bufResponse;
	KAtEnd defef([this, &bufResponse, &cbFinish, &dwTillNow]() {
		ULONG64 iResult = (0 < _dwContentLength && dwTillNow == _dwContentLength) ? 0 : -1;
		if (cbFinish)
			cbFinish(bufResponse, iResult);//마무리 dwTillNow == _dwContentLength 이어야 다 받은 거다.
		this->CloseRequest();
		});


	DWORD dwDownloaded = 0;
	int i = 0;
	while (1)
	{
		KAtEnd ipp([&i]() {i++; });
		DWORD dwToRead = 0;
		if (!oHttp.QueryDataAvailable(&dwToRead))
		{
			//TRACE(" ###########     Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
			no_throw_str(L" ###########     Error %u in WinHttpQueryDataAvailable.", GetLastError());
		}
		if (dwToRead == 0)
		{//여기는 안온다. 
			no_throw_str(L"  dwToRead(%.2fK) == 0 then break; %.2fK/%.2fM >>>>>>>>>>>>>>>>>", dwToRead / 1'000., dwTillNow / 1'000., _dwContentLength / 1'000'000.);
			break;
		}
		//TRACE(L"  dwToRead(%5.2fK) %8.2fK/%8.2fM\n", dwToRead/1'000., dwTillNow/1'000., _dwContentLength/1'000'000.);
		//TRACE("%u DataAvailable\n", dwSizeInOut);
		//std::shared_ptr<char> shBuf = SharedBuf(dwToRead + 1);//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
		std::vector<BYTE> temp(dwToRead);
		//if (!shBuf)
		//{
		//	dwToRead = 0;
		//	throwLINE;
		//}
		//LPSTR pBuf = shBuf.get();
		if (oHttp.ReadData((LPVOID)temp.data(), dwToRead, &dwDownloaded))
		{
			if (dwDownloaded > 0)//pBuf[dwToRead] = '\0';//뒤에 0을 삽입한다. SharedBuf가 처음에 0으로 모두 초기화 하니 할 필요 없다.
			{
				dwTillNow += dwDownloaded;
				//pBuf += dwDownloaded;
				//file.Write(pBuf, dwDownloaded);
				bufResponse.insert(bufResponse.end(), temp.begin(), temp.begin() + dwDownloaded);
				if (0 < _dwContentLength && dwTillNow == _dwContentLength) // WINHTTP_QUERY_CONTENT_LENGTH
				{
					//TRACE("OK %d. dwDownloaded(%5u) dwTillNow:%.2fM/_dwContentLength:%.2fM then break >>>>>>>>>>>>>>>>>\n", i, dwDownloaded, dwTillNow/1'000'000., dwContentLength/1'000'000.);
					break;// 여기서 성공 여부가 갈린다.
				}
			}
			else
			{
				TRACE("%d. dwDownloaded(%5u) dwTillNow:%.2fM/_dwContentLength: %.2fM\n", i, dwDownloaded, dwTillNow / 1'000'000., _dwContentLength / 1'000'000.);
				//TRACE("[%d].  dwToRead: %u : %u/%.1fM loaded\n", iDL, dwToRead, dwDownloaded, dwTillNow/1'000'000.);
				break;
			}
			//				TRACE("%u : %u/%u loaded\n", dwToRead, dwDownloaded, dwTillNow);
		}
		else
		{
			no_throw_str(L"###########      Error %u in WinHttpReadData.\n", GetLastError());
		}
	};// while (dwSizeInOut > 0);
}

PWS UcStringToHtmlString(CStringW& sUtf8, CStringW& sWstr)
{
	CStringW s;
	for (int i = 0;i<sUtf8.GetLength();++i)
	{
		int ch = sUtf8[i];
		if (UcIsAlNum(ch))
			sWstr += (WCHAR)ch;
		else
		{
			s.Format(L"%%%02X", (BYTE)ch);
			ASSERT(s.GetLength() == 3);
			sWstr += s;
		}
	}
	return sWstr;
}

/// <summary>
///  HTTP_STATUS_OK 만 성공한게 아니다. 성공에는 여러 값이 있다.
/// </summary>
/// <param name="iStatus"></param>
/// <returns></returns>
bool UcHttpStatusOK(int iStatus)
{
	return UcOrAny(iStatus, {
		0,//성공한 경우 0으로 리턴 하는 함수가 있어서
		HTTP_STATUS_OK				   ,
		HTTP_STATUS_CREATED			   ,//특히 업로드하면 OK가 안오고, 이게 온다.
		HTTP_STATUS_ACCEPTED		   ,
		HTTP_STATUS_PARTIAL			   ,
		HTTP_STATUS_NO_CONTENT		   ,
		HTTP_STATUS_RESET_CONTENT	   ,
		HTTP_STATUS_PARTIAL_CONTENT	   ,
		HTTP_STATUS_WEBDAV_MULTI_STATUS,
		}) ? true : false;
}



bool UcGetRemoteGeneral(CStringW sUrl, PWS sAgent, function<int(LPVOID, ULONG64, ULONG64, ULONG64)> cbWrite, int* piStatus, PWS sFunc, PWS sFile, int nLine)
{
	//TRACE(L"%s\n~~~~ Download: %s\n", __FUNCTIONW__, (PS)sUrl);
	CStringW sDmp; sDmp.Format(L"\nvvvv Download: %s\n", (PWS)sUrl);
	if (!sFile)
		TRACE(sDmp);
	else
		UcTrace(sFile, nLine, sFunc, sDmp);

	if (sAgent == NULL)
		sAgent = __FUNCTIONW__;
	KWinHttp oHttp(sAgent);
	try
	{
		oHttp.DownLoadRequest(sUrl, 0, L"GET");//sUrl = L"https://update.cadian.com/update/LOCK.json"
		oHttp._dwContentLength = oHttp.GetHeaderNum(WINHTTP_QUERY_CONTENT_LENGTH);
		int iStaus = oHttp.GetHeaderNum(WINHTTP_QUERY_STATUS_CODE);
		if (iStaus == HTTP_STATUS_OK)
			oHttp.DownLoadWrite(cbWrite);
		if (piStatus)
			*piStatus = iStaus;
		return true;// iStaus와 관계 없이 throw가 안났다.
	}
	catch (KException* e)
	{
		TRACE(L"%s %s %s\n", e->_sExcept, e->m_strError, e->m_strStateNativeOrigin);
	}
	return false;
}

std::string UcGetRemoteString(CStringW sUrl, PWS sAgent, int* piStatus, PWS sFunc, PWS sFile, int nLine)
{
	std::string response;
	try
	{
		bool brv = UcGetRemoteGeneral(sUrl, sAgent, [&response](LPVOID pBufSrc, ULONG64 qwCur, ULONG64 qwDownloaded, ULONG64 qwTotal) -> int {
			if (qwDownloaded > 0)
				response.append((LPCSTR)pBufSrc, (size_t)qwDownloaded);
			return 0;
			}, piStatus, sFunc, sFile, nLine);
	}
	catch (KException* e)
	{
		TRACE(L"%s %s %s\n", e->_sExcept, e->m_strError, e->m_strStateNativeOrigin);
	}
	return response;
}

std::shared_ptr<std::vector<BYTE>> UcGetRemoteBuffer(CStringW sUrl, PWS sAgent, int* piStatus, PWS sFunc, PWS sFile, int nLine)
{
	auto buffer = std::make_shared<std::vector<BYTE>>();
	bool brv = UcGetRemoteGeneral(sUrl, sAgent, [buffer](LPVOID pBufSrc, ULONG64 qwCur, ULONG64 qwDownloaded, ULONG64 qwTotal) -> int {
#ifdef _DEBUG
		ASSERT(qwTotal > 0);
		auto pBufStr = (LPCSTR)pBufSrc;
#endif // _DEBUG
		if (buffer->size() == 0)
			buffer->resize((size_t)qwTotal+1);
		auto pBufTar = buffer->data();// .get();이 아님
		if (qwDownloaded > 0)
		{
			memcpy(pBufTar + qwCur, pBufSrc, (size_t)qwDownloaded);
			pBufTar[qwCur + qwDownloaded] = (BYTE)0;
		}
		return 0;
		}, piStatus, sFunc, sFile, nLine);
	return buffer;
}

std::string UcGetExternalIP()
{
	static std::vector<PWS> arSite = {
#ifdef _DEBUGx
		L"https://fake.ip/",
#endif // _DEBUG
		L"https://ifconfig.me/ip",
		L"https://ipinfo.io/ip",
		L"https://icanhazip.com/",
		L"https://ident.me/",
	};
	std::string response;
	int iStatus{};
	for(int i=0;i<(int)arSite.size() && iStatus != HTTP_STATUS_OK;i++)
	{
		response = LUcGetRemoteString(arSite[i], L"WinHTTP UcGetExternalIP", &iStatus);
		if (7 <= response.length() && response.length() <= 15) // "0.0.0.0" "123.123.123.123"
			return response;
	}
	return response;//https://ipinfo.io/ip 이것도 된다.
}





/// <summary>
/// length 를 hexa 값으로 바꾼후 문자열 8자리로 바꾼다.
/// </summary>
/// <param name="buffer"></param>
/// <param name="length"></param>
void UcHexEncodeLength(char* buffer, size_t length)
{
	std::stringstream ss;
	ss << std::setw(8) << std::setfill('0') << std::hex << length;
    std::string hexLength = ss.str();
    // VS2015 Checked Iterators 경고 회피: 포인터 기반 복사
	memcpy(buffer, hexLength.data(), hexLength.size());
	//std::copy(hexLength.begin(), hexLength.end(), buffer);
//c:\program files(x86)\microsoft visual studio 14.0\vc\include\xutility(2372) : error C4996 : 'std::copy::_Unchecked_iterators::_Deprecate' : Call to 'std::copy' with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct.To disable this warning, use - D_SCL_SECURE_NO_WARNINGS.See documentation on how to use Visual C++ 'Checked Iterators'
//	1 > c:\program files(x86)\microsoft visual studio 14.0\vc\include\xutility(2372) : note: 'std::copy::_Unchecked_iterators::_Deprecate' 선언을 참조하십시오.
//	1 > c:\srce\test\mystaticlib1\uctoolmb14\ucnetwork.cpp(899) : note: 컴파일 중인 함수 템플릿 인스턴스화 '_OutIt *std::copy<std::_String_iterator<std::_String_val<std::_Simple_types<char>>>,char*>(_InIt,_InIt,_OutIt)'에 대한 참조를 확인하십시오.
//	1 > with
//	1 > [
//		1 > _OutIt = char *,
//			1 > _InIt = std::_String_iterator < std::_String_val<std::_Simple_types<char>>>
//			1 > ]
}


/// <summary>
/// 특정 크기만큼 패켓을 recv 한다.
/// </summary>
/// <param name="socket"></param>
/// <param name="buffer"></param>
/// <param name="totalBytes"></param>
/// <returns></returns>
bool UcRecvWithSized(SOCKET socket, char* buffer, int totalBytes)
{
	DWKFUNC;
	int received = 0;
	while (received < totalBytes)
	{
		int len = ::recv(socket, buffer + received, totalBytes - received, 0);
		if (len <= 0)
		{
			ASSERT(len == SOCKET_ERROR || len == 0);
			//std::cerr << "Connection closed or error during recv." << std::endl;
			auto ntErr = WSAGetLastError();// 를 호출해 오류 코드를 확인해야 함.
			Sleep(100);
			CStringW sErr;
			switch (ntErr)//주요 오류와 조치 방안 :
			{
				case WSAEWOULDBLOCK:
					//소켓이 논블로킹 모드로 설정된 경우 발생.
						//이 경우 데이터를 나중에 다시 시도할 수 있으므로 소켓을 닫지 않아도 됨.
					DWKTRACE(L"WSAEWOULDBLOCK", sErr);
					continue;
				case WSAETIMEDOUT:
				{
					//송수신 제한 시간이 초과된 경우 발생.
					//이 경우 연결 상태가 불안정할 수 있으므로 소켓을 닫고 다시 연결을 시도하는 것이 좋음.
					DWKTRACE(L"WSAETIMEDOUT", 1);
					return false;
				}
				case WSAECONNRESET:
					//클라이언트가 강제로 연결을 종료했거나 네트워크 문제가 발생한 경우.//소켓을 닫아야 함.
					/// client에서 앱 연결 중에 앱 죽이니 이값이 덜어진다.
				{
					DWKTRACE(L"WSAECONNRESET", 1);
					return false;
				}
				case WSAENETDOWN:
				{
					DWKTRACE(L"WSAENETDOWN", 1);
					return false;
				}
				case WSAENOTSOCK:
					//등 기타 심각한 오류 :
					//소켓이 유효하지 않거나 네트워크 문제가 발생.//소켓을 닫아야 함.
				{
					DWKTRACE(L"WSAENOTSOCK", 1);
					return false;
				}
				case 0:
				{
					DWKTRACE(L" = WSAGetLastError()", 1);
					//break; // 소켓 종료. 0일때도 종료로 본다.
					return false;
				}
			}
		}// if (len <= 0)
		DWKTRACE(L"received(%d) += len(%d)", received, len);
		received += len;
	}//while

	DWKTRACE(L"Total received(%d)", received);
	return true;
}


/// <summary>
/// size가 8바이트로 hex 값을 header로 하는 패킷 recv하기. size는 header를 포함한 총 길이
/// </summary>
/// <param name="clientSkt"></param>
/// <param name="data"></param>
/// <returns></returns>
bool UcRecvSizedPacket(SOCKET clientSkt, std::string& data)
{
	DWKFUNC;
	if (clientSkt == INVALID_SOCKET)
		return false;
	//char lengthBuffer[LEN_PACKET] = {};

	std::string hexLength;
	hexLength.resize(LEN_PACKET);
	char* lengthBuffer = &hexLength[0];
	if (!UcRecvWithSized(clientSkt, lengthBuffer, LEN_PACKET)) {
		//std::cerr << "Failed to receive the packet length." << std::endl;
		//client 에서 재접속 하고 새 번호표 요구 다시 하겠지?
		//closesocket(clientSkt);
		return false;
	}
	// 2. 수신한 8바이트를 정수로 변환 (16진수)
	//std::string hexLength(lengthBuffer, LEN_PACKET);
	int totalPacketLength = 0;
	try {
		totalPacketLength = std::stoi(hexLength, nullptr, 16); // 16진수에서 정수로 변환
	}
	catch (const std::invalid_argument&) {
		// 잘못된 문자가 포함된 경우 기본값 반환
		return false;
	}
	catch (const std::out_of_range&) {
		// 범위를 벗어난 값인 경우 기본값 반환
		return false;
	}
	// 44 = 36 + 8
	// 3. 전체 패킷 데이터를 받을 버퍼를 동적으로 할당
	///auto dataBuffer = std::make_shared<char[]>(totalPacketLength + 1);///char는 shared_ptr로 웬만 하면 쓰지 마라.
	std::string buffer;
	buffer.resize(totalPacketLength);// +1); 할필요 없다. 내부에서 알아서 한다.
	// 특정 위치에 포인터로 접근하여 값 설정
	char* ptr = &buffer[0];
	// 4. 이미 받은 8바이트 이후의 데이터를 계속 수신
	int remainingBytes = totalPacketLength - LEN_PACKET;
	if (!UcRecvWithSized(clientSkt, ptr, remainingBytes)) {
		//std::cerr << "Failed to receive the complete packet." << std::endl;
		//closesocket(clientSkt);
		return false;
	}
	//ptr[totalPacketLength] = '\0';// dwk 한바이트 더 할당 하여 거기에 0 터미네이터 한다. 처음 할당 할때 쓰레기 일까? 내부에서 알아서 한다.
	// 5. 데이터 부분만 문자열로 추출
	data.assign(ptr, remainingBytes);
	//data = std::move(buffer);
	DWKTRACE(L"Received data: %s", CStringW(data.c_str()).GetString());
	return true;
};
bool UcSendSizedPacket(SOCKET clientSkt, const char* data, int len)
{
	DWKFUNCV(L"%d", len);
	int lenSend = LEN_PACKET + len;
	CStringA sLen; sLen.Format(R"(%08X)", lenSend);// , sdata.c_str());
	std::string sdata;// (data, len);//혹시 data 맨뒤에 '\0'이 없을수 있으니
	sdata.resize(lenSend);
	char* ptr = &sdata[0];
	memcpy_s(ptr             , lenSend, sLen.GetString(), LEN_PACKET);
	memcpy_s(ptr + LEN_PACKET, len    , data            , len       );
	auto lenSent = ::send(clientSkt, ptr, lenSend, 0);
	return lenSend == lenSent;
}


//#include <afx.h>

bool UcIsValidIPAddress(const CString& ip)
{
	// CString을 표준 문자열로 변환
	CT2CA pszConvertedAnsiString(ip);
	std::string ipStr(pszConvertedAnsiString);

	// 빈 문자열 확인
	if (ipStr.empty()) {
		return false;
	}

	// '.'으로 분할
	std::vector<std::string> segments;
	std::stringstream ss(ipStr);
	std::string segment;

	while (std::getline(ss, segment, '.')) {
		segments.push_back(segment);
	}

	// IPv4 주소는 반드시 4개의 세그먼트여야 함
	if (segments.size() != 4) {
		return false;
	}

	for (const auto& seg : segments) {
		// 세그먼트가 비어 있거나 숫자가 아닌 경우
		if (seg.empty() || !std::all_of(seg.begin(), seg.end(), ::isdigit)) {
			return false;
		}

		// 정수 값 확인
		int value = 0;
		try {
			value = std::stoi(seg);
		}
		catch (const std::invalid_argument&) {			// 잘못된 문자가 포함된 경우 유효하지 않은 IP
			return false;
		}
		catch (const std::out_of_range&) {			// 범위를 벗어난 값인 경우 유효하지 않은 IP
			return false;
		}
		if (value < 0 || value > 255) {
			return false;
		}

		// 0으로 시작하는 경우 처리 (예: "01", "001" 등은 유효하지 않음)
		if (seg.size() > 1 && seg[0] == '0') {
			return false;
		}
	}

	return true;
}
