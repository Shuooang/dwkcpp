#pragma once
#include <winhttp.h>

// VS2015에서 정의되지 않은 HTTP 상태 코드 정의
#ifdef _MSC_VER
#if _MSC_VER <= 1900  // VS2015 포함 이하 버전
#define HTTP_STATUS_PERMANENT_REDIRECT 308
#endif
#endif

#include "UcTool.h"






CString UcHttpStatusStr(DWORD dwRet);


//CStringW UcGetHeaderStr(HINTERNET hRequest, int key);
//
//int UcGetHeaderNum(HINTERNET hRequest, int key);


#define CHECKELAPSED \
auto tik = GetTickCount64();\
KAtEnd tik2([this, &tik]() {\
	_elsp[__FUNCTION__] = GetTickCount64() - tik;\
})

#ifdef _Sample__
std::string KWinHttp_Sample()
{
	KWinHttp oHttp(L"WinHTTP UcGetExternalIP");
	oHttp.DownLoadRequest(L"https://ifconfig.me/ip", 0, L"GET");
	oHttp._dwContentLength = oHttp.GetHeaderNum(WINHTTP_QUERY_CONTENT_LENGTH);//dwContentLength = 286851
	int iStaus = oHttp.GetHeaderNum(WINHTTP_QUERY_STATUS_CODE);
	std::string response;
	if (iStaus == HTTP_STATUS_OK)
	{
		oHttp.DownLoadWrite([&response](LPVOID pBufSrc, ULONG64 qwCur, ULONG64 qwDownloaded) -> int {
			if (qwDownloaded > 0)
				response.append((LPCSTR)pBufSrc, qwDownloaded);
			return 0;
			});
	}
	return response;
}
#endif // _Sample__
class KWinHttp
{
public:
	explicit KWinHttp(PWS sAgent = NULL)
	{
		if (sAgent && tchlen(sAgent) > 0)
			this->Open(sAgent);
		else
		{
			ASSERT(0);
			_break;// Open 안하는 경우도 있나?
		}
	}
	virtual ~KWinHttp()
	{
		CloseAll();
	}


	std::list<std::tuple<CStringW, int, BOOL>> _lstConnect;
	//int _port;
	//CStringW _domain;
	//BOOL _bSecure{ FALSE };// SSL WINHTTP_FLAG_SECURE

	HINTERNET _hSession{ NULL };
	HINTERNET _hConnect{ NULL };
	HINTERNET _hRequest{ NULL };

	KStdMap<string, ULONGLONG> _elsp;

	std::tuple<CStringW, DWORD, CStringW, CStringW, DWORD> _openParam;

	DWORD _dwContentLength{ 0 };
	CStringW _sUrlTail;
	void InitRequestData()
	{
		_sUrlTail.Empty();
		//_dwContentLength = 0;// 이걸 여기서 0으로 꼭 할 필요가 있나?
	}


	void CloseRequest()
	{
		if (_hRequest)
		{
			WinHttpCloseHandle(_hRequest);
			_hRequest = NULL;
			InitRequestData();
		}
	}
	void CloseConnect()
	{
		if (_hConnect)
		{
			WinHttpCloseHandle(_hConnect);
			_hConnect = NULL;
		}
	}
	void CloseSession()
	{
		if (_hSession)
		{
			WinHttpCloseHandle(_hSession);
			_hSession = NULL;
		}
	}
	void CloseAll(bool bSession = true)
	{
		CHECKELAPSED;
		CloseRequest();
		CloseConnect();
		if(bSession)
			CloseSession();
	}



	void Open(CStringW sAgent, DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, LPCWSTR pszProxyW = WINHTTP_NO_PROXY_NAME, LPCWSTR pszProxyBypassW = WINHTTP_NO_PROXY_BYPASS, DWORD dwFlags = 0)
	{
		CHECKELAPSED;
		//auto tik = GetTickCount();
		//KAtEnd tik2([this, &tik]() {
		//	_elsp[__FUNCTION__] = GetTickCount() - tik;
		//});
		if (_hSession == NULL)
		{
			_hSession = WinHttpOpen(sAgent, dwAccessType, pszProxyW, pszProxyBypassW, dwFlags);
			if (_hSession == NULL)
				throwLINE;
			_openParam = make_tuple(sAgent, dwAccessType, pszProxyW, pszProxyBypassW, dwFlags);
		}
	}
	void Open()
	{
#if CPP17_OR_LATER
		auto [sAgent, dwAccessType, pszProxyW, pszProxyBypassW, dwFlags] = _openParam;
#else
		auto sAgent = std::get<0>(_openParam);
		auto dwAccessType = std::get<1>(_openParam);
		auto pszProxyW = std::get<2>(_openParam);
		auto pszProxyBypassW = std::get<3>(_openParam);
		auto dwFlags = std::get<4>(_openParam);
#endif
		Open(sAgent, dwAccessType, pszProxyW, pszProxyBypassW, dwFlags);
	}


	void Connect(CStringW sDmn, INTERNET_PORT iPort = INTERNET_DEFAULT_HTTP_PORT, BOOL bSSL = FALSE);





	void DownLoadRequest(CStringW sUrl, int nTimeout, CStringW sVerb = L"GET"
		, function<void(CStringW& sDirDn)> cbDir = NULL, vector<CStringW> headers = {}
		, function<void(shared_ptr<char>&, ULONG64&)> cbData = NULL);
	void DownLoadRequestDirect(CStringW sUrl, int nTimeout, CStringW sVerb = L"GET"
		, function<void(CStringW& sDirDn)> cbDir = NULL, vector<CStringW> headers = {}
	, function<void(LPSTR&, ULONG64&)> cbData = NULL);

	void DownLoadWrite(CFile& file, function<void(ULONG64)> cbFinish = NULL);
	void DownLoadWrite(function<int(LPVOID, ULONG64, ULONG64, ULONG64)> cbWrite, function<void(ULONG64)> cbFinish = NULL);

	void DownLoadResponse(function<void(std::vector<BYTE>, ULONG64)> cbFinish = NULL);


	void OpenRequest(CStringW sDirDn, CStringW sVerb = L"GET", LPCWSTR pwszVersion = NULL, LPCWSTR pwszReferrer = WINHTTP_NO_REFERER, LPCWSTR* ppwszAcceptTypes = NULL, DWORD dwFlags = NULL);

	BOOL AddRequestHeaders(CStringW lpszHeaders, DWORD dwHeadersLength = -1L, DWORD dwModifiers = WINHTTP_ADDREQ_FLAG_ADD)
	{
		CHECKELAPSED;
		UCASSERT(_hRequest);
//		BOOL WinHttpAddRequestHeaders(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers);
		/// CStringW srg;srg.Format(L"Range: bytes=%d-%d", startByte, startByte + (cntByte -1));
		/// bResults = WinHttpAddRequestHeaders(hRequest, srg, -1L, WINHTTP_ADDREQ_FLAG_ADD);
		if(_hRequest)
			return WinHttpAddRequestHeaders(_hRequest, lpszHeaders, dwHeadersLength, dwModifiers);
		return FALSE;
	}

	/// 이 코드는 HTTP 요청 헤더에 "Range" 헤더를 추가하는 것입니다. "Range" 헤더는 일부 리소스의 일부분만을 요청하는 데 사용됩니다.
	/// 보통 파일 다운로드 중에 특정 범위의 데이터만을 요청할 때 사용됩니다. 이 헤더를 사용하여 서버에게 요청하는 데이터의 범위를 지정할 수 있습니다.
	void SetRange(int startByte, int cntByte)
	{
		CHECKELAPSED;
		CStringW srg;srg.Format(L"Range: bytes=%d-%d", startByte, startByte + (cntByte -1));
		if(!AddRequestHeaders(srg))
			throwLINE;
	}

	BOOL SendRequest(CStringW lpszHeaders = WINHTTP_NO_ADDITIONAL_HEADERS, DWORD dwHeadersLength = 0, LPVOID lpOptional = WINHTTP_NO_REQUEST_DATA, DWORD dwOptionalLength = 0, DWORD dwTotalLength = 0, DWORD_PTR dwContext = 0);

	void RequestSimple(CStringW sDirHttp, CStringW sVerb = L"GET")
	{
		OpenRequest(sDirHttp, sVerb);
		// 이 둘 사이에 WinHttpAddRequestHeaders 를 끼어 넣는데, 이 함수는 그런게 없을 경우 사용 한다.
		SendRequest();
	}

	BOOL QueryHeaders(DWORD key, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex = WINHTTP_NO_HEADER_INDEX)
	{
		CHECKELAPSED;
		ASSERT(_hRequest);
		BOOL rv = WinHttpQueryHeaders(_hRequest, key, pwszName, lpBuffer, lpdwBufferLength, WINHTTP_NO_HEADER_INDEX);//		bufferSize	60	unsigned long
		if(!rv)
		{
			auto err = GetLastError();
			throwLINE;
		}
		return rv;
	}

	DWORD QueryHeadersSize(DWORD key, LPCWSTR pwszName = WINHTTP_HEADER_NAME_BY_INDEX);

	CStringW GetHeaderStr(int key, function<void(CStringW)> cbHeader = NULL);

	int GetHeaderNum(int key, function<void(int)> cbHeader = NULL)
	{
		CStringW sData = GetHeaderStr(key);
		if (sData.GetLength() > 0)
		{
			int nData = UcAtoi((PWS)sData);
			if (cbHeader)
				cbHeader(nData);
			return nData;
		}
		return -1;
		//return UcGetHeaderNum(_hRequest, key);
	}

	BOOL QueryDataAvailable(LPDWORD lpdwNumberOfBytesAvailable)
	{
		CHECKELAPSED;
		ASSERT(_hRequest);
		//BOOLAPI WinHttpQueryDataAvailable(IN HINTERNET hRequest, LPDWORD lpdwNumberOfBytesAvailable);
		return WinHttpQueryDataAvailable(_hRequest, lpdwNumberOfBytesAvailable);
	}

	BOOL ReadData(LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
	{
		CHECKELAPSED;
		ASSERT(_hRequest);
		return WinHttpReadData(_hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
	}

	void SetOption(DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength = 0)
	{
		CHECKELAPSED;
		WinHttpSetOption(_hSession, dwOption, lpBuffer, dwBufferLength);
	}

	/// <param name="dwOption">WINHTTP_OPTION_CONNECT_TIMEOUT
	///						   WINHTTP_OPTION_RECEIVE_TIMEOUT 서버의 바쁜 상태를 감지하는 데 
	///						   WINHTTP_OPTION_SEND_TIMEOUT</param>
	/// <param name="dwTimeout"></param>
	void SetTimeout(DWORD dwOption, DWORD dwTimeout)
	{
		CHECKELAPSED;
		//SetOption(dwOption, &dwTimeout, sizeof(dwTimeout));

		//HINTERNET hSession; // WinHttpOpen으로 생성된 세션 핸들
		//int resolveTimeout = 0, connectTimeout = 0, sendTimeout = 0, receiveTimeout = 0;

		// 현재 세션의 타임아웃 값을 가져옵니다.
		//WinHttpGetTimeouts(hSession, &resolveTimeout, &connectTimeout, &sendTimeout, &receiveTimeout);
		//WinHttpGetOption(_hSession, dwOption, lpBuffer, dwBufferLength);
		//WinHttpGetOption
		// 이제 sendTimeout만 변경하고자 한다면, sendTimeout 값을 원하는 값으로 설정합니다.
		//sendTimeout = 5000; // 예: 송신 타임아웃을 5000ms로 설정

		// 변경된 타임아웃 값으로 WinHttpSetTimeouts를 호출하여 업데이트합니다.
		//WinHttpSetTimeouts(_hSession, resolveTimeout, connectTimeout, sendTimeout, receiveTimeout);
		// 세션 핸들 hSession가 이미 생성되어 있다고 가정
		WinHttpSetTimeouts(_hSession, 10000, dwTimeout, dwTimeout, 5000);
		//WinHttpSetTimeouts(hRequest, resolveTimeout, connectTimeout, sendTimeout, receiveTimeout)) {
	}
};



CStringW UcGetMACAddress();

PWS UcStringToHtmlString(CStringW& sUtf8, CStringW& sWstr);

bool UcHttpStatusOK(int iStatus);


/// 읽어온 데이터 챙기는 부분은 람다함수로 처리 한다.
bool UcGetRemoteGeneral(CStringW sUrl, PWS sAgent, function<int(LPVOID, ULONG64, ULONG64, ULONG64)> cbWrite, int* piStatus = NULL, PWS sFunc = 0, PWS sFile = 0, int nLine = 0);

/// 원격 파일을 string으로 리턴
std::string UcGetRemoteString(CStringW sUrl, PWS sAgent, int* piStatus = NULL, PWS sFunc = 0, PWS sFile = 0, int nLine = 0);

std::shared_ptr<std::vector<BYTE>> UcGetRemoteBuffer(CStringW sUrl, PWS sAgent, int* piStatus = NULL, PWS sFunc = 0, PWS sFile = 0, int nLine = 0);



#define LUcGetRemoteString(a,b,c)  UcGetRemoteString(a,b,c,__FUNCTIONW__, __FILEW__, __LINE__)
#define LUcGetRemoteBuffer(a,b,c)  UcGetRemoteBuffer(a,b,c,__FUNCTIONW__, __FILEW__, __LINE__)




std::string UcGetExternalIP();

UCTOOLDYNAMIC std::tuple<std::string, int> UcGetPortFromSocket(SOCKET clientSock);















#include <winsock2.h>  // For sockets
#pragma comment(lib, "Ws2_32.lib")

#define LEN_PACKET 8

void UcHexEncodeLength(char* buffer, size_t length);

bool UcRecvWithSized(SOCKET socket, char* buffer, int totalBytes);


bool UcRecvSizedPacket(SOCKET clientSkt, std::string& data);

bool UcSendSizedPacket(SOCKET clientSkt, const char* data, int len);

bool UcIsValidIPAddress(const CString& ip);
