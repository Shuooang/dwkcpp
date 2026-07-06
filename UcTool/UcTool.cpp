// UcTool.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//

#include "pch.h"
//#include "framework.h"
#include <mutex>
#include <regex> // for std::wregex

#include <PathCch.h>//PathCchCanonicalize
#pragma comment(lib, "PathCch.lib")

#include <ktmw32.h>
#pragma comment(lib, "KtmW32.lib") //CreateTransaction

#include "UcTool.h"
#include "UcBaseTools.h"
#include "UcBinary.h"
#include "UcTimeTools.h"
#include "UcNetwork.h"
#include "UcDebug.h"


// C++14 호환성을 위한 static 멤버 정의
#if CPP_BEFORE_17
DWKREMINDER("tmplate GSingleton<T>은 모듈마다 만들어 지므로, UCTOOLDYNAMIC GetKTrace() export 된 함수를 따로 만들어야 한다.")
//#pragma message(FILINDWK("C++14 is supported. (Not C++17)"))
// KException 클래스의 static 멤버들
std::function<void(KException*)> KException::s_fncExceptionDealer;
std::function<bool(CException*, KException*)> KException::s_fncCExceptionChecker;
std::function<bool(std::exception*, KException*)> KException::s_fncStdExceptionChecker;
std::map<CStringW, std::shared_ptr<KException>> KException::s_exceptionMap;
std::mutex KException::s_mutex;

// GCStringBuffer 클래스의 static 멤버들
std::deque<CStringW> GCStringBuffer::s_stringBuffer;
std::mutex GCStringBuffer::s_bufferMutex;
// MAX_BUFFER_SIZE는 const이므로 별도 정의 불필요



// C++14에서만 필요한 명시적 인스턴스화
// 실제 사용되는 타입들에 대한 명시적 인스턴스화
DWKREMINDER("실제 사용되는 GSingleton<KStdMap<__int64, KLambdaTimer*>>::mutex_ defined.")
template class GSingleton<KStdMap<__int64, KLambdaTimer*>>;
DWKREMINDER("실제 사용되는 GSingleton<KList<KLambdaTimer*>>::mutex_ defined.")
template class GSingleton<KList<KLambdaTimer*>>;

// C++14에서 필요한 GSingleton 템플릿 인스턴스화들
template class GSingleton<UcIdStrMap>;
//template class GSingleton<NvIdStrMap>;
//template class GSingleton<UcParamMap>;
template class GSingleton<UcSharedPtrTool>;
template class GSingleton<MyCla1>;
template class GSingleton<MyCla2>;
#endif



#ifdef _Sample__
class CMyData : public CObject
{
public:
	CStringW m_name;
	int m_value;

	DECLARE_SERIAL(CMyData)

	CMyData() : m_name(_T("")), m_value(0) {}

	virtual void Serialize(CArchive& ar) override
	{
		CObject::Serialize(ar);
		if (ar.IsStoring())
			ar << m_name << m_value;
		else
			ar >> m_name >> m_value;
	}
};
IMPLEMENT_SERIAL(CMyData, CObject, 1)

//template <>
//IMPLEMENT_SERIAL(KSharedObjList<CMyData>, CObject, 1) //error

class KMyDataList : public KSharedObjList<CMyData>
{
public:
	DECLARE_SERIAL(KMyDataList) // MFC 직렬화를 위한 매크로
		KMyDataList() = default;
	virtual ~KMyDataList() {}
};
IMPLEMENT_SERIAL(KMyDataList, CObject, 1)
#endif // _Sample__


UCTOOLDYNAMIC
BOOL UcIfFileExistEx(LPCTSTR filePath, BOOL* pbDir)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFile = FindFirstFile(filePath, &wfd);
	KAtEnd d_hFile([hFile]() {
		if (hFile)
			FindClose(hFile);
		});
	BOOL exists = (hFile != INVALID_HANDLE_VALUE);
	if (exists)
	{
		BOOL bDir = UcAttr(wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		if (pbDir)
			*pbDir = bDir;
		return !bDir;
	}
	return FALSE;
}

UCTOOLDYNAMIC
CStringW UcUTF8ToWchar(LPCSTR sUtf8)//dwk: 2025-02-3 14:21  
{
	CStringW sWstr;
	UcUTF8ToWchar(sUtf8, sWstr);
	return sWstr;
}


//LPCWSTR UcUTF8ToWchar(LPCSTR sUtf8, CStringW& sWstr)
//{
//	int lenUtf8 = lstrlenA(sUtf8) + 1;
//	int lenTar = MultiByteToWideChar(CP_UTF8, 0, sUtf8, lenUtf8, NULL, 0);
//	LPWSTR pw = sWstr.GetBuffer(lenTar);//맨압에 '\0'를 넣어 
//	MultiByteToWideChar(CP_UTF8, 0, sUtf8, lenUtf8, pw, lenTar);
//	sWstr.ReleaseBuffer();//버퍼 끝에 '\0'을 넣는다. 더 짦게 문자열을 끊을려면 거기에 '\0'을 넣어 준다.
//	return (LPCWSTR)sWstr;
//}
UCTOOLDYNAMIC
LPCWSTR UcUTF8ToWchar(LPCSTR sUtf8, CStringW& sWstr)
{
	// 1) 넓은 문자열이 얼마나 필요한지 얻는다 (NULL 자동 포함)
	int lenTar = MultiByteToWideChar(CP_UTF8, 0, sUtf8, -1, NULL, 0);
	// 2) CStringW 버퍼 확보
	LPWSTR pw = sWstr.GetBuffer(lenTar);
	// 3) 실제 변환
	MultiByteToWideChar(CP_UTF8, 0, sUtf8, -1, pw, lenTar);
	// 4) NULL 종료 유지
	sWstr.ReleaseBuffer();
	return (LPCWSTR)sWstr;
}

UCTOOLDYNAMIC
CStringA UnicodeToUTF8(const CStringW& unicodeStr) {
	// 1️⃣ 변환 후 필요한 UTF-8 바이트 수 계산
	int len = WideCharToMultiByte(CP_UTF8, 0, unicodeStr, -1, NULL, 0, NULL, NULL);
	if (len == 0) return ""; // 변환 실패 시 빈 문자열 반환

	// 2️⃣ CStringA에 버퍼 할당 후 변환 수행
	CStringA utf8Str;
	WideCharToMultiByte(CP_UTF8, 0, unicodeStr, -1, utf8Str.GetBuffer(len), len, NULL, NULL);
	utf8Str.ReleaseBuffer();

	return utf8Str;
}

UCTOOLDYNAMIC
CStringA UcWcharToUTF8(LPCWSTR sWstr)
{
	CStringA sUtf8;
	UcWcharToUTF8(sWstr, sUtf8);
	return sUtf8;//dwk: 2025-02-3 13:30 c++17 부터 알아서 Move 
}

UCTOOLDYNAMIC
LPCSTR UcWcharToUTF8(LPCWSTR sWstr, CStringA& sUtf8)
{
	char* pUtf8 = NULL;
	int nLength2 = WideCharToMultiByte(CP_UTF8, 0, sWstr, -1, pUtf8, 0, NULL, NULL);
	pUtf8 = sUtf8.GetBuffer(nLength2 + 1);//char*)malloc(nLength2+1); 
	WideCharToMultiByte(CP_UTF8, 0, sWstr, -1, pUtf8, nLength2, NULL, NULL);
	sUtf8.ReleaseBuffer();

	return sUtf8;
}

/// 위랑 똑같지만 KBinary만 다름.
LPCSTR UcWcharToUTF8(LPCWSTR sWstr, KBinary& sUtf8)
{
	char* pUtf8 = NULL;
	int nLength2 = WideCharToMultiByte(CP_UTF8, 0, sWstr, -1, pUtf8, 0, NULL, NULL);
	pUtf8 = sUtf8.Alloc(nLength2);//char*)malloc(nLength2+1); 
	WideCharToMultiByte(CP_UTF8, 0, sWstr, -1, pUtf8, nLength2, NULL, NULL);
	return pUtf8;
}

#include "RPCDCE.H"
#pragma comment(lib, "rpcrt4.lib")

int UcGetFormattedGuid(GUID& guid, CString& rString, bool bHipn)
{
	guid = GUID_NULL;
	HRESULT hr = UuidCreate(&guid);
	if (HRESULT_CODE(hr) != RPC_S_OK)
		return 1;//		throw "Unable to get a GUID.";
	if (guid == GUID_NULL)
		return 2;//		throw "Unable to create new GUID.";
	// Warnings
	if (HRESULT_CODE(hr) == RPC_S_UUID_NO_ADDRESS)
		return 3;//		throw "Cannot get the hardware address for this computer.";
	if (HRESULT_CODE(hr) == RPC_S_UUID_LOCAL_ONLY)
		return 4;//		throw "Warning: Unable to determine your network address.\r\n  The UUID generated is unique on this computer only.\r\n  It should not be used on another computer.";

	LPCTSTR strFormat = bHipn ?
		_T("%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X") : _T("%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X");

	rString.Format(strFormat,
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	return 0;
}

//int UcGetFormattedGuidA(GUID& guid, CStringA& rString, bool bHipn)
//{
//	CString rStringW;
//	int rv = UcGetFormattedGuid(guid, rStringW, bHipn);
//	rString = CStringA(rStringW);
//	return rv;
//}
UCTOOLDYNAMIC
CString UcGetFormattedGuid(bool bHipn)//=true
{
	GUID guid;
	CString rString;
	UcGetFormattedGuid(guid, rString, bHipn);
	return std::move(rString);
}
//CStringA UcGetFormattedGuidA(bool bHipn)//=true
//{
//	GUID guid;
//	CStringA rString;
//	UcGetFormattedGuidA(guid, rString, bHipn);
//	return std::move(rString);
//}

#ifdef _DEBUG_seeif
#include <random>
#include <sstream>
std::string generateUuid() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 15);
	std::uniform_int_distribution<> dis2(8, 11);

	std::ostringstream oss;
	oss << std::hex;
	for (int i = 0; i < 8; ++i) oss << dis(gen);
	oss << "-";
	for (int i = 0; i < 4; ++i) oss << dis(gen);
	oss << "-4"; // UUID 버전 4
	for (int i = 0; i < 3; ++i) oss << dis(gen);
	oss << "-";
	oss << dis2(gen); // UUID variant
	for (int i = 0; i < 3; ++i) oss << dis(gen);
	oss << "-";
	for (int i = 0; i < 12; ++i) oss << dis(gen);

	return oss.str();
}
#endif // _DEBUG_참조




/// ////////////////////////////////////////////////////////////////////////////////
//#include "UcJson.h"

//SHP<char> UcJsonToData(UcJObj& jbj)
//{
//	try
//	{
//		CStringA sUtf8 = jbj.ToJsonStringUtf8(2, NULL);
//	}
//	catch (CException*)
//	{
//		throw;
//	}
//}

UCTOOLDYNAMIC
bool UcIsDirExists(LPCTSTR sDir)
{
	if (tchlen(sDir)) {
		DWORD dw = ::GetFileAttributes(sDir);
		return (dw & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}
	return false;
}

// full path 의 디렉토리를 만들어 준다.
// bToEnd가 FALSE이면 맨 마지막 것은 FILE NAME 이므로 만들 필요 없다는 뜻.
// reffer to: File::createDirectory : recursive call
UCTOOLDYNAMIC
int UcCheckTargetDir(PS sFullT, BOOL bCreate, BOOL bToEnd, int iStart)
{
	if (sFullT == nullptr || tchlen(sFullT) == 0)
		return -4;
	CString sFull(sFullT);
	//CStringArray ar;
#ifdef _DEBUG
	if (tchstr((PS)sFull, _T("CADian Update")))// LR"(C:\Temp\CopyByMyApp\IRX\3DConnexionModule\3DxWareSDK\lib\bin\)"))
		_break;
#endif // _DEBUG
	if (bCreate)
		_break;
	vector<CString> ar;
	UcCutByToken(sFull.GetString(), _T("\\"), ar);
	if (ar.size() == 0)
		return -3;
	CStringW sPath;
	for (int i = 0; i <= iStart; i++)
	{
		if (sPath.GetLength() > 0 && sPath.Right(1) != '\\')
			sPath += (TCHAR)'\\';
		sPath += ar[i];//여기까지는 만들어져 있으니 체크할 필요 없고.
	}
	for (int i = iStart + 1; i < (int)ar.size(); i++)
	{
		if (i == (ar.size() - 1) && bToEnd == FALSE) // last name is file not directory
			break;
		sPath += (TCHAR)'\\';
		sPath += ar[i];

		DWORD dw = ::GetFileAttributesW(sPath);
		if (dw != INVALID_FILE_ATTRIBUTES)
		{
			if (dw & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			else
				return -1;// 디렉토리가 아냐?
		}
		else
		{ // 없어
			if (bCreate) {
				if (CreateDirectoryW(sPath, NULL) == FALSE) {
					bool bOK = false;
					for (int j = 0; j < 4; j++) {
						DWORD dw = ::GetFileAttributesW(sPath);
						if (dw & FILE_ATTRIBUTE_DIRECTORY) {
							bOK = true;
							break;
						}
						Sleep(1);
					}
					if (!bOK) {
						TRACE(L"CreateDirectory %s\t\t########### ERROR ###########\n", sPath);
						return GetLastError();
					}
				}
			}
			else
				return -2;//없어
		}
	}
	return 0;
}
// protected: KLIBAPI가 없잖아
CStringW& UcMoneyToStrGoodNum(CStringW& sm, int nUnderDot, bool bComma, bool bTruncate)
{
	//?warning
	// 무조건 0으로 바꾸면.. "0.14" 를 입력 할때 "0." 의 '.'이 입력이 안되면.
	// 어떻게 입력 해?
	if (nUnderDot == 0) //소수점 아래 없다면.. 무조건 소수 아래 0은 잘라야지..
		bTruncate = true;
	if (sm.IsEmpty() || sm == _T("0.") || sm == _T("0"))
	{
		if (bTruncate)
			sm = '0';
	}
	else
	{
		int ng = 0;
		int iDot = -1;// 뒤에서 부터 아직 dot 발견 안한것 false
		bool bDot = sm.Find('.') >= 0;

		ASSERT(sm.GetLength() < 128);
		const int jc = 128;
		TCHAR buf[jc];
#ifdef _DEBUG
		memset(buf, ' ', jc);
#endif // _DEBUG
		int j = jc - 1;
		buf[j] = '\0';
		--j;// 뒤에서 부터 글자가 들어갈 위치로...

		for (int i = sm.GetLength() - 1; i >= 0; i--)
		{
			int c = (int)(TBYTE)sm[i];

			if (iDot >= 0 || !bDot) //소수점 위쪽이면
				ng++;     // 앞쪽으로 전진 한칸더
			else if (c == '.') // 드디어 . 발견.. true
				iDot = j; // . 들어간 위치

			ASSERT(('0' <= c && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'E' || c == 'e');
			//|| c=='e'

			buf[j--] = c;

			if (bComma && ng == 3 && // . 부터 앞(높은)쪽으로 3칸 추가되었고
				i > 0 &&    // 맨 앞이 아니고, 맨앞에 ',' 오면 안되잖아
				('0' <= sm[i - 1] && sm[i - 1] <= '9')) // 앞에 글자가 숫자 이면
			{
				buf[j--] = ',';// ','를 추가 한다.
				ng = 0;
			}
		}

		j++; // 맨앞 글자 위치로 이동..

		//맨끝 글자위치부터 0을 자르고 맨 뒤가 . 으로 끝나면 그것도 자른다. 단 iDot >= 0 일때만
		bool bEndZero = false;
		for (int ipos = jc - 2; ipos > j && bDot; ipos--)
		{
			ASSERT(ipos >= 0);
			if (ipos < 0 || ipos >= jc) // 경계 검사 추가 C6385 이거 해봐야 아래 생김
				break;// code analysys ChatGPT
			if (!bEndZero && (buf[ipos] == '0' || buf[ipos] == '.'))//?CDANAL warning C6385: Reading invalid data from 'buf'.
			{
				TCHAR ch = buf[ipos];
				if (bTruncate) // '0'또는 '.'을 제거 한다.
					buf[ipos] = '\0';

				if (ch == '.')
					break;
			}
			else
			{
				bEndZero = true;

				if (nUnderDot >= 0)
				{
					if ((iDot + nUnderDot) < ipos)
						buf[ipos] = '\0';
					else
						break;
				}
			}
		}

		sm = buf + j;// 맨앞 글자 위치// 
	}
	return sm;
}

// ',' 컴마를 붙여 준다.
CStringW UcMoneyToStr(double dv, int nUnderDot, bool bComma, bool bTruncate)
{
	CStringW sbuf;
	sbuf.Format(L"%.2f", dv);
	UcMoneyToStrGoodNum(sbuf, nUnderDot, bComma, bTruncate); //뒤에 0 만 처리 하겠군.
	return sbuf;
}


int UcRecursiveDirLambda(CString dir, std::set<CString> stExt,
	std::function<int(CString, const WIN32_FIND_DATA&)> fFile, // OK면 0리턴
	std::function<int(CString, int)> fDir,  // OK면 0리턴
	BOOL bSubDir, BOOL bExitOnError, int nLevel)
{
	ASSERT(stExt.size() > 0);
	//선택한 확장자가 없다. "*" 이라도 넣어야지.
	//std::set<wstring> stExt = {"*",};

	KArray<CString> sAr;
	CString path = dir;// .c_str();

	if (path.GetAt(path.GetLength() - 1) == (TCHAR)'\\')
		path = path.Left(path.GetLength() - 1);
	//ASSERT(stExt.size() == 1 || stExt.find(L"*") == stExt.end());// "*"일때는 하나여야 std::string::npos);< string::find()아님
	CString sExt(_T("*.*"));
	//sExt.Format(L"*.%s", (stExt.size() == 1) ? (*stExt.begin()).c_str() : L"*.*");

	//확장자 여러개 주니, 일단 다 읽어야지.
	// 하나면 그냥 그것만 읽지.
	WIN32_FIND_DATA wfd = { 0 };
	CString full = path + (TCHAR)'\\' + sExt;
	// 점
	auto FnIsUp = [](LPCTSTR fn) -> bool {
		static std::set<CString> jm = { _T("."),_T("..") };
		return (jm.find(fn) == jm.end());	//!tchsame((LPCSTR)wfd.cFileName, ".") && ".."
		};

	HANDLE hFile = NULL;
	KAtEnd d_hFile([hFile]() {
		if (hFile)
			FindClose(hFile);
		});

	BOOL bGo = TRUE;
	for (int jump = 0; ; jump++)
	{
		if (hFile == NULL)
		{
			hFile = FindFirstFile(full, &wfd);
			if (hFile == INVALID_HANDLE_VALUE)
				return -1;
		}
		else
		{
			bGo = FindNextFile(hFile, &wfd);
			if (!bGo)
				break;
		}

		CString sfile(wfd.cFileName);

		if (UcAttr(wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			if (bSubDir)
			{
				if (FnIsUp(sfile.GetString()))//	if(wfd.cFileName[0] != (TCHAR)'.')
					sAr.Add(sfile);// . 과 ..이 아닌 디렉토리면
			}
		}
		else
		{
			bool bMatch = false;//(stExt.size() == 1) && (*stExt.begin()) == L"*";// 모두 다 삭제 
			if ((stExt.size() == 1) && (((*stExt.begin()) == L"*") || (*stExt.begin()) == _T("*.*")))
				bMatch = true;
			else
			{
				//CString sfile = st;
				auto ip = sfile.ReverseFind('.');
				if (ip >= 0)
				{
					CString ext = sfile.Mid(ip + 1);
					bMatch = stExt.find(ext) != stExt.end();//확장자 매치
				}
			}

			if (bMatch)
			{
				//INT64 size = 0;
				//size = wfd.nFileSizeHigh;
				//size <<= 32;
				//size |= wfd.nFileSizeLow;

				CString sFullPath; sFullPath.Format(_T("%s\\%s"), path.GetString(), wfd.cFileName);
				int rv = fFile(sFullPath, wfd);
				if (rv && bExitOnError)//0이면 계속
					return rv;
			}
		}
	}

	int rv = 0;
	if (fDir)//디렉토리 관련 람다는 NULL일 수 있다.
	{
		int rv = fDir((PS)path, nLevel);
		if (rv && bExitOnError)//0이면 계속
			return rv;
	}

	if (bSubDir)// 는 나중에 해야 정렬이 되지.
	{
		for (int i = 0; i < (int)sAr.size(); i++)
		{
			CString newfile = dir;
			newfile += (TCHAR)'\\';
			newfile += sAr[i];
			/// 여기서 recursive로 자기 자신을 호출 한다. 디렉토리 경로가 추가된 채로
			rv = UcRecursiveDirLambda(newfile, stExt, fFile, fDir, bSubDir, bExitOnError, nLevel + 1);
			/// 여기서 서브디렉토리에서 실패 한 경우 끝내지 않고 계속 진행 한다.		
			if (rv && bExitOnError)
				return rv;
		}
	}

	return rv;
}
//C:\Users\keeps\AppData\Local\ITCKR\UcRoot\UpdateCourierCheckRemoveTest 리커시브하게 왕창 날려 버린다.
DWORD UcRemoveDir(LPCWSTR pDir, function<int(PWS sm, LPCWSTR sf)> cbDel)
{
	//vector<wstring> sAr;
	CStringW path(pDir);
	CStringW spath(path);// path.c_str());
	spath.TrimRight((WCHAR*)L"\\");
	path = (PWS)spath;

	WIN32_FIND_DATAW wfd;
	CStringW fullW = path + L"\\*.*";
	HANDLE h = FindFirstFileW(fullW, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return -1;

	vector<CStringW> arSub;
	for (;;)
	{
		CStringW sFile(wfd.cFileName);
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (sFile != _T(".") && sFile != _T(".."))
				arSub.push_back(sFile);//sFile = L"UpdateCourierCheck_Ba"
		}
		else
		{
			CStringW fullD = path + L"\\" + sFile;// C:\Users\keeps\AppData\Local\ITCKR\UcRoot\UpdateCourierCheckRemoveTest\IK_GUID.json
			int ir = 0;
			if (cbDel)
				ir = cbDel(L"file", fullD);
			if (ir == 0)
			{
				if (!DeleteFileW(fullD))//path + (TCHAR)'\\' + sFile))
				{
					DWORD dwErr = GetLastError();
					//TRACE("DeleteFile(%s) ERROR (%ld)\n", (LPCTSTR)(path + sFile), dwErr);

					cbDel(L"error", (PWS)UcErrorToStrW(dwErr));
				}
			}
		}

		if (FindNextFileW(h, &wfd) == FALSE)
			break;
	}
	FindClose(h);

	for (int i = 0; i < (int)arSub.size(); i++)
	{
		CStringW fullSub = path + L"\\" + arSub[i];
		if (cbDel)
			cbDel(L"folder", fullSub);
		UcRemoveDir(fullSub, cbDel);// C:\Users\keeps\AppData\Local\ITCKR\UcRoot\UpdateCourierCheckRemoveTest\GuidIndex
	}

	if (cbDel)
		cbDel(L"root", pDir);
	if (!RemoveDirectoryW(pDir))
	{
		DWORD dwErr = GetLastError();
		//TRACE("RemoveDirectory(%s) ERROR (%lu)\n", (LPCTSTR)pDir, dwErr);
		cbDel(L"error", (PWS)UcErrorToStrW(dwErr));
		return dwErr;
	}
	return 0;
}

CStringW UcMakeLongNameWorks(PWS lpszFileName, int extLen)
{
	CStringW sFileName(lpszFileName);
	if ((tchlen(lpszFileName) + extLen) >= _MAX_PATH || extLen == -1)//무조건
	{
		//ASSERT(FALSE); // MFC requires paths with length < _MAX_PATH
//		TRACE(L"File length is longer than _MAX_PATH. (%s)\n", lpszFileName);
		//return FALSE;
		PWS hd = LR"(\\?\)";
		if (sFileName.Left(4) != hd)
		{
			sFileName = hd;
			sFileName += lpszFileName;
		}
	}
	return sFileName;
}


void AFX_CDECL AfxTimeToFileTime(const CTime& time, LPFILETIME pFileTime);
BOOL AFXAPI AfxFullPath(_Pre_notnull_ _Post_z_ LPTSTR lpszPathOut, LPCTSTR lpszFileIn);

/// 긴경로 파일도 가능
void UcSetFileStatus(PWS lpszFileName, const CFileStatus& status)
{
	ASSERT(lpszFileName != NULL);
	CStringW sFileName = UcMakeLongNameWorks(lpszFileName);
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	LPFILETIME lpCreationTime = NULL;
	LPFILETIME lpLastAccessTime = NULL;
	LPFILETIME lpLastWriteTime = NULL;

	DWORD wAttr = GetFileAttributesW((PWS)sFileName);// 앞에 "\\?\"가 경로 앞에 붙어 있으면 에러 안난다. C:\asldkfa 
	WIN32_FILE_ATTRIBUTE_DATA fileAttrData = { 0 };
	if (wAttr == (DWORD)-1L)
	{
		if (GetFileAttributesExW(sFileName, GetFileExInfoStandard, &fileAttrData))
			wAttr = fileAttrData.dwFileAttributes;// 파일 정보를 사용
		else
			CFileException::ThrowOsError((LONG)GetLastError(), CString(sFileName));
	}

	DWORD tmpAttr = wAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
	if ((wAttr & CFile::readOnly))//status.m_attribute != wAttr && 
	{
		// Set file attribute, only if currently readonly.
		// This way we will be able to modify the time assuming the
		// caller changed the file from readonly.
		/// 일단 임시로 읽기전용 속성을 다 풀고
		BOOL bRes = SetFileAttributesW((PWS)sFileName, tmpAttr); //status.m_attribute);
		if (!bRes)
		{
			wchar_t shortPath[MAX_PATH];
			HRESULT hr = PathCchCanonicalize(shortPath, ARRAYSIZE(shortPath), sFileName);
			if (SUCCEEDED(hr))
			{
				BOOL result = SetFileAttributesW(shortPath, tmpAttr); //status.m_attribute);
			}
			else
				CFileException::ThrowOsError((LONG)GetLastError(), CString(sFileName));
		}
	}

	// last modification time
	if (status.m_mtime.GetTime() != 0)
	{
		AfxTimeToFileTime(status.m_mtime, &lastWriteTime);
		lpLastWriteTime = &lastWriteTime;

		// last access time
		if (status.m_atime.GetTime() != 0)
		{
			AfxTimeToFileTime(status.m_atime, &lastAccessTime);
			lpLastAccessTime = &lastAccessTime;
		}

		// create time
		if (status.m_ctime.GetTime() != 0)
		{
			AfxTimeToFileTime(status.m_ctime, &creationTime);
			lpCreationTime = &creationTime;
		}

		HANDLE hFile = ::CreateFileW(sFileName, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
			CFileException::ThrowOsError((LONG)::GetLastError(), CString(sFileName));

		if (!SetFileTime((HANDLE)hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime))
		{
			LONG sc = (LONG)::GetLastError();
			::CloseHandle(hFile);
			CFileException::ThrowOsError(sc, CString(sFileName));
		}

		if (!::CloseHandle(hFile))
			CFileException::ThrowOsError((LONG)::GetLastError(), CString(sFileName));
	}

	//if(status.m_attribute != wAttr && !(wAttr & CFile::readOnly))
	{
		BOOL bRes = SetFileAttributesW((PWS)sFileName, status.m_attribute);
		if (!bRes)
			CFileException::ThrowOsError((LONG)GetLastError(), CString(sFileName));
	}
}

/// 긴경로 파일도 가능
BOOL UcGetFileStatus(LPCTSTR lpszFileName, CFileStatus& rStatus)
{
	ASSERT(lpszFileName != NULL);

	if (lpszFileName == NULL)
	{
		return FALSE;
	}
	CStringW sFileName = UcMakeLongNameWorks(CStringW(lpszFileName));
	ASSERT(!sFileName.IsEmpty());
	// attempt to fully qualify path first 이걸 왜 뺏지?
	if (!AfxFullPath(rStatus.m_szFullName, CString(sFileName)))
	{
		rStatus.m_szFullName[0] = '\0';
		return FALSE;
	}

	WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;

	if (!GetFileAttributesExW(sFileName, GetFileExInfoStandard, &fileAttributeData))
		return FALSE;

	// strip attribute of NORMAL bit, our API doesn't have a "normal" bit.
	rStatus.m_attribute = (fileAttributeData.dwFileAttributes & ~FILE_ATTRIBUTE_NORMAL);

	rStatus.m_size = fileAttributeData.nFileSizeHigh;
	rStatus.m_size <<= 32;
	rStatus.m_size |= fileAttributeData.nFileSizeLow;

	// convert times as appropriate
	if (CTime::IsValidFILETIME(fileAttributeData.ftCreationTime))
	{
		rStatus.m_ctime = CTime(fileAttributeData.ftCreationTime);
	}
	else
	{
		rStatus.m_ctime = CTime();
	}

	if (CTime::IsValidFILETIME(fileAttributeData.ftLastAccessTime))
	{
		rStatus.m_atime = CTime(fileAttributeData.ftLastAccessTime);
	}
	else
	{
		rStatus.m_atime = CTime();
	}

	if (CTime::IsValidFILETIME(fileAttributeData.ftLastWriteTime))
	{
		rStatus.m_mtime = CTime(fileAttributeData.ftLastWriteTime);
	}
	else
	{
		rStatus.m_mtime = CTime();
	}

	if (rStatus.m_ctime.GetTime() == 0)
		rStatus.m_ctime = rStatus.m_mtime;

	if (rStatus.m_atime.GetTime() == 0)
		rStatus.m_atime = rStatus.m_mtime;

	return TRUE;
}


BOOL UcSafeCopy::SafeCopyFile(LPCTSTR sSrcFile, LPCTSTR sTarFile, BOOL bOverWrite, BOOL bCheckFolder)
{
	ASSERT(_hTransaction);
	BOOL bExistsTar = FALSE;
	try
	{
		CFileStatus stsTar;
		//CFileStatus stsSrc;
		//if (CFile::GetStatus(sSrcFile, stsTar) == FALSE)
		//	return GetLastError();
		if (bOverWrite)
		{//sTarFile =  L"C:\\server\\update\\Pro\\CADian_12.1.260.33187.P.VC16.x64.Alpha\\AdkGDAL.dll.capch"
			bExistsTar = UcIfFileExistEx(sTarFile);
			if (bExistsTar)
			{
				//KwSetReadOnly(target, false);
				if (CFile::GetStatus(sTarFile, stsTar) == FALSE)
					return GetLastError();
				if (stsTar.m_attribute & CFile::system)
					return FALSE;
				if ((stsTar.m_attribute & CFile::readOnly) || (stsTar.m_attribute & CFile::hidden))
				{
					stsTar.m_attribute &= ~(CFile::readOnly | CFile::hidden);//속성 비트 제거
					CFile::SetStatus(sTarFile, stsTar);
				}
			}
		}
		if (bCheckFolder)
		{
			CStringW sTar(sTarFile);
			int itar = sTar.ReverseFind('\\');
			CStringW sTarFolder;
			if (itar >= 0)
				sTarFolder = sTar.Left(itar);
			UcCheckTargetDir(CString(sTarFolder));
		}
		//HANDLE hTransaction = CreateTransaction(NULL, 0, 0, 0, 0, 0, NULL);
		//if (hTransaction == INVALID_HANDLE_VALUE)
		//	return FALSE;
		//KAtEnd defer([&hTransaction]() {
		//	CloseHandle(hTransaction);
		//	});
		_bSuccess = CopyFileTransacted(sSrcFile, sTarFile, NULL, NULL, FALSE, 0, _hTransaction);
		// 속성도 같이 복사 되잖아. 속성은 복사된다.
//		CFile::SetStatus(sTarFile, stsSrc);//소스의 속성과 같게 리스토어

		/// 날짜는 변경날짜만 복사되고, 생성날짜와 접근 날짜는 새로생긴다.
		/// 생성날짜는 복사한 시점이고 접근도 그렇다.
		/// 변경날짜만 복사 되면 되는데,

		//if (_bSuccess)
		//{
		//	if (CommitTransaction(_hTransaction))
		//		return TRUE;
		//}
		//else
		//	RollbackTransaction(_hTransaction);
	}
	catch (CFileException* e)
	{
		_bSuccess = FALSE;
		auto rc = e->GetRuntimeClass();
		auto er = GetLastError();
		auto ser = UcErrorToStrW(er);
		TRACE(L"CFileException(%u: %s) %s\n", er, ser);
	}
	catch (CException* e)
	{
		_bSuccess = FALSE;
		auto rc = e->GetRuntimeClass();
		auto er = GetLastError();
		auto ser = UcErrorToStrW(er);
		TRACE(L"CException(%u: %s) %s\n", er, ser);
	}
	if (!_bSuccess)
		_fails[CStringW(sTarFile)] = _bSuccess;
	return _bSuccess;
}

BOOL UcSafeCopyFile(LPCTSTR sSrcFile, LPCTSTR sTarFile, BOOL bOverWrite, BOOL bCheckFolder)
{
	UcSafeCopy cp(TRUE);
	cp.SafeCopyFile(sSrcFile, sTarFile, bOverWrite, bCheckFolder);
	return cp.IsAllSuccess();

#ifdef _deprecated__
	BOOL bExistsTar = FALSE;
	BOOL bSuccess = FALSE;
	try
	{
		CFileStatus stsTar;
		//CFileStatus stsSrc;
		//if (CFile::GetStatus(sSrcFile, stsTar) == FALSE)
		//	return GetLastError();
		if (bOverWrite)
		{//sTarFile =  L"C:\\server\\update\\Pro\\CADian_12.1.260.33187.P.VC16.x64.Alpha\\AdkGDAL.dll.capch"
			bExistsTar = UcIfFileExistEx(sTarFile);
			if (bExistsTar)
			{
				//KwSetReadOnly(target, false);
				if (CFile::GetStatus(sTarFile, stsTar) == FALSE)
					return GetLastError();
				if (stsTar.m_attribute & CFile::system)
					return FALSE;
				if ((stsTar.m_attribute & CFile::readOnly) || (stsTar.m_attribute & CFile::hidden))
				{
					stsTar.m_attribute &= ~(CFile::readOnly | CFile::hidden);//속성 비트 제거
					CFile::SetStatus(sTarFile, stsTar);
				}
			}
		}
		if (bCheckFolder)
		{
			CStringW sTar(sTarFile);
			int itar = sTar.ReverseFind('\\');
			CStringW sTarFolder;
			if (itar >= 0)
				sTarFolder = sTar.Left(itar);
			UcCheckTargetDir(sTarFolder);
		}
		HANDLE hTransaction = CreateTransaction(NULL, 0, 0, 0, 0, 0, NULL);
		if (hTransaction == INVALID_HANDLE_VALUE)
			return FALSE;
		KAtEnd defer([&hTransaction]() {
			CloseHandle(hTransaction);
			});
		bSuccess = CopyFileTransacted(sSrcFile, sTarFile, NULL, NULL, FALSE, 0, hTransaction);

		// 속성도 같이 복사 되잖아. 속성은 복사된다.
//		CFile::SetStatus(sTarFile, stsSrc);//소스의 속성과 같게 리스토어

		/// 날짜는 변경날짜만 복사되고, 생성날짜와 접근 날짜는 새로생긴다.
		/// 생성날짜는 복사한 시점이고 접근도 그렇다.
		/// 변경날짜만 복사 되면 되는데,

		if (bSuccess)
		{
			if (CommitTransaction(hTransaction))
				return TRUE;
		}
		else
			RollbackTransaction(hTransaction);
	}
	catch (CFileException* e)
	{
		auto rc = e->GetRuntimeClass();
		auto er = GetLastError();
		auto ser = UcErrorToStrW(er);
		TRACE(L"CFileException(%u: %s) %s\n", er, ser);
	}
	catch (CException* e)
	{
		auto rc = e->GetRuntimeClass();
		auto er = GetLastError();
		auto ser = UcErrorToStrW(er);
		TRACE(L"CException(%u: %s) %s\n", er, ser);
	}
	return bSuccess;
#endif // _deprecated__
}

#ifdef _DEBUGx
BOOL UcSafeCopyFile(std::vector<std::pair<std::wstring, std::wstring>> filesToCopy, BOOL bOverWrite, BOOL bCheckFolder
	, function<int(wstring, wstring)> cbBefore
	, function<void(wstring, wstring)> cbAfter
	, function<void(wstring, wstring, int, BOOL)> cbError)
{
	HANDLE hTransaction = CreateTransaction(NULL, 0, 0, 0, 0, 0, NULL);
	if (hTransaction == INVALID_HANDLE_VALUE)
	{
		//throw_str(L"Failed to create transaction.");
		return FALSE;
	}
	KAtEnd defer([&hTransaction]() {
		// 트랜잭션 핸들 닫기
		CloseHandle(hTransaction);
		});
	BOOL bExistsTar = FALSE;
	BOOL bSuccess = FALSE;

	// 복사할 파일 목록
	//std::vector<std::pair<std::wstring, std::wstring>> filesToCopy ={
	//	{ L"source_file1.txt", L"destination_file1.txt" },
	//	 { L"source_file2.txt", L"destination_file2.txt" },
	//};

	// 파일 복사 트랜잭션 시작
	for (const auto& filePair : filesToCopy)
	{
		CStringW sTarFile = filePair.second.c_str();
		CFileStatus stsTar;
		if (bOverWrite)
		{
			bExistsTar = UcIfFileExistEx(sTarFile);
			if (bExistsTar)
			{
				//KwSetReadOnly(target, false);
				if (CFile::GetStatus(sTarFile, stsTar) == FALSE)
					return GetLastError();
				if (stsTar.m_attribute & CFile::system)
					return FALSE;
				if ((stsTar.m_attribute & CFile::readOnly) || (stsTar.m_attribute & CFile::hidden))
				{
					stsTar.m_attribute &= ~(CFile::readOnly | CFile::hidden);//속성 비트 제거
					CFile::SetStatus(sTarFile, stsTar);
				}
			}
		}
		if (bCheckFolder)
		{
			CStringW sTar(sTarFile);
			int itar = sTar.ReverseFind('\\');
			CStringW sTarFolder;
			if (itar >= 0)
				sTarFolder = sTar.Left(itar);
			UcCheckTargetDir(sTarFolder);
		}
		int iBf = 0;
		if (cbBefore)
			iBf = cbBefore(filePair.first, filePair.second);
		if (iBf == 0)
		{
			int nReTry = 3;
			BOOL bResult = FALSE;
			DWORD err = 0;
			while (--nReTry > 0 && !bResult)
			{
				bResult = CopyFileTransacted(
					filePair.first.c_str(),    // 원본 파일 경로
					sTarFile,   // 대상 파일 경로
					NULL,                      // 복사할 파일의 진행 상황을 나타내는 콜백 함수 포인터
					NULL,                      // 콜백 함수에 전달되는 사용자 데이터 포인터
					NULL,                      // 콜백 함수가 호출될 때의 플래그
					0,                         // 복사 옵션
					hTransaction               // 트랜잭션 핸들
				);
				//if (nReTry > 1)
				//	bResult = FALSE;
				if (!bResult)
				{
					err = GetLastError();
					if (cbError)
						cbError(filePair.first, filePair.second, err, TRUE);
				}
			}
			if (!bResult)
			{
				//err = GetLastError();
				TRACE(L"######################### Failed to copy file. Rolling back transaction.\n");
				RollbackTransaction(hTransaction);
				if (cbError)
					cbError(filePair.first, filePair.second, err, FALSE);
				return FALSE;
			}
			if (cbAfter)
				cbAfter(filePair.first, filePair.second);
		}
		else if (iBf == -1)
		{

		}
		else if (iBf == -2)
		{
			RollbackTransaction(hTransaction);
			return FALSE;
		}
	}

	// 트랜잭션 커밋
	if (!CommitTransaction(hTransaction))
	{
		//throw_str(L"Failed to commit transaction.");
		auto err = GetLastError();
		CStringW sErr = UcErrorToStrW(err);
		if (CTime::GetCurrentTime() < CTime(2024, 4, 20, 0, 0, 0))
			UcMessageBoxError(L"CommitTransaction(%X) err(%s:%d)", (INT64)hTransaction, sErr, err);
		return FALSE;
	}
	TRACE(L":) Files copied successfully.(^o^)\n");

	//std_cout2 << "Files copied successfully." << std::endl;
	return TRUE;
}
#endif // _DEBUG


int UcSafeCopyFileEx(std::vector<std::pair<std::wstring, std::wstring>>& filesToCopy, BOOL bOverWrite, BOOL bCheckFolder
	, function<int(wstring, wstring)> cbBefore
	, function<void(wstring, wstring)> cbAfter
	, function<void(wstring, wstring, int, BOOL)> cbError)
{
	HANDLE hTransaction = CreateTransaction(NULL, 0, 0, 0, 0, 0, NULL);
	if (hTransaction == INVALID_HANDLE_VALUE)
	{
		//throw_str(L"Failed to create transaction.");
		return eScpTransactionError;
	}
	KAtEnd defer([&hTransaction]() {
		// 트랜잭션 핸들 닫기
		CloseHandle(hTransaction);
		});
	BOOL bExistsTar = FALSE;
	BOOL bSuccess = FALSE;

	// 복사할 파일 목록
	//std::vector<std::pair<std::wstring, std::wstring>> filesToCopy ={
	//	{ L"source_file1.txt", L"destination_file1.txt" },
	//	 { L"source_file2.txt", L"destination_file2.txt" },
	//};

	// 파일 복사 트랜잭션 시작
	for (const auto& filePair : filesToCopy)
	{
		CString sTarFile(filePair.second.c_str());
		CFileStatus stsTar;
		if (bOverWrite)
		{
			bExistsTar = UcIfFileExistEx(CString(sTarFile));
			if (bExistsTar)
			{
				//KwSetReadOnly(target, false);
				if (CFile::GetStatus(sTarFile, stsTar) == FALSE)
					return GetLastError();
				if (stsTar.m_attribute & CFile::system)
					return CFile::system;
				if ((stsTar.m_attribute & CFile::readOnly) || (stsTar.m_attribute & CFile::hidden))
				{
					stsTar.m_attribute &= ~(CFile::readOnly | CFile::hidden);//속성 비트 제거
					CFile::SetStatus(sTarFile, stsTar);
				}
			}
		}
		if (bCheckFolder)
		{
			CStringW sTar(sTarFile);
			int itar = sTar.ReverseFind('\\');
			CString sTarFolder;
			if (itar >= 0)
				sTarFolder = sTar.Left(itar);
			UcCheckTargetDir(sTarFolder);
		}
		int iBf = 0;
		if (cbBefore)
			iBf = cbBefore(filePair.first, filePair.second);

		if (iBf == eScpOK)
		{
			int nReTry = 3;
			BOOL bResult = FALSE;
			DWORD err = 0;
			while (--nReTry > 0 && !bResult)
			{
				bResult = CopyFileTransactedW(filePair.first.c_str(),    // 원본 파일 경로
					CStringW(sTarFile),   // 대상 파일 경로
					NULL,                      // 복사할 파일의 진행 상황을 나타내는 콜백 함수 포인터
					NULL,                      // 콜백 함수에 전달되는 사용자 데이터 포인터
					NULL,                      // 콜백 함수가 호출될 때의 플래그
					0,                         // 복사 옵션
					hTransaction               // 트랜잭션 핸들
				);
				//if (nReTry > 1)
				//	bResult = FALSE;
				if (!bResult)
				{
					err = GetLastError();
					if (cbError)
						cbError(filePair.first, filePair.second, err, TRUE);
				}
			}
			if (!bResult)
			{
				//err = GetLastError();
				TRACE(L"######################### Failed to copy file. Rolling back transaction.\n");
				RollbackTransaction(hTransaction);
				if (cbError)
					cbError(filePair.first, filePair.second, err, FALSE);
				return eScpCpError;
			}
			if (cbAfter)
				cbAfter(filePair.first, filePair.second);
		}
		else if (iBf == eScpDontCopy)
		{
		}
		else if (iBf == eScpAbort)
		{
			RollbackTransaction(hTransaction);
			return eScpAbort;
		}
	}

	// 트랜잭션 커밋
	if (!CommitTransaction(hTransaction))
	{
		//throw_str(L"Failed to commit transaction.");
		auto err = GetLastError();
		CStringW sErr = UcErrorToStrW(err);
		if (CTime::GetCurrentTime() < CTime(2024, 4, 20, 0, 0, 0))
			UcMessageBoxError(L"CommitTransaction(%X) err(%s:%d)", (INT64)hTransaction, sErr, err);
		return err;
	}
	TRACE(L":) Files copied successfully.(^o^)\n");

	return 0;
}

UCTOOLDYNAMIC
BOOL UcSafeCopyFile(std::vector<std::pair<std::wstring, std::wstring>>& filesToCopy, BOOL bOverWrite, BOOL bCheckFolder
	, function<int(wstring, wstring)> cbBefore, function<void(wstring, wstring)> cbAfter, function<void(wstring, wstring, int, BOOL)> cbError)
{
	int rcp = UcSafeCopyFileEx(filesToCopy, bOverWrite, bCheckFolder, cbBefore, cbAfter, cbError);
	return rcp == eScpOK;
}

UCTOOLDYNAMIC
long UcWriteSmallTextFileA(LPCTSTR filename, CStringA& text, BOOL bOverwrite)
{
	ASSERT(bOverwrite || !UcIfFileExistEx(filename));
	CStdioFile cFile;
	if (!cFile.Open(filename, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))	// 
		return -1;
	LPSTR ptr = (LPSTR)text.GetBuffer();
	cFile.Write((LPSTR)ptr, text.GetLength());
	text.ReleaseBuffer();
	cFile.Close();
	return text.GetLength();
}
static const WORD s_wfeff = 0xfeff;

// UNICODE 인경우 0xfeff 를 헤더로 넣는다.
UCTOOLDYNAMIC
long UcWriteSmallTextFileW(LPCWSTR filename, CStringW& text)
{
	ASSERT(!UcIfFileExistEx(CString(filename)));
	CStdioFile cFile;
	//?warning CFile::typeBinary 를 반드시 주어야 한다. UNICODE text를 쓸때 0a00 -> 0d0a00 을 바뀌어서 쓰인다.
	if (!cFile.Open(CString(filename), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))	// 
		return -1;
	//	cFile.WriteString(text);
	cFile.Write(&s_wfeff, sizeof(s_wfeff));//s, s.GetLength());

	//LPCWSTR ptr = (LPCWSTR)text;//.GetBuffer();
	long len = (long)text.GetLength() * sizeof(WCHAR); //tchlen(buf);
	cFile.Write((LPCWSTR)text, len);
	//text.ReleaseBuffer();

	cFile.Close();
	return len;
}

#include <winhttp.h>
#include "UcWindow.h"
//#define ERRSTR(err) {err, #err}
#define ERRSTR(err) s_mapErr->insert(std::pair<UINT, std::string>(err, #err))

shared_ptr<std::map<UINT, std::string>> s_mapErr;
static std::once_flag s_mapErrOnce;

std::shared_ptr<std::map<UINT, std::string>> UcGetErrorMap() {
	std::call_once(s_mapErrOnce, []() {
		s_mapErr = make_shared<std::map<UINT, std::string>>();
		//s_mapErr->insert(std::pair());
//		s_mapErr->insert(std::pair<UINT, std::string>(ERROR_SUCCESS, "ERROR_SUCCESS"));

		ERRSTR(ERROR_SUCCESS);	//	0L
		ERRSTR(NO_ERROR);	//	0L                                                 // dderror
		ERRSTR(SEC_E_OK);	//	((HRESULT)0x00000000L)
		ERRSTR(ERROR_INVALID_FUNCTION);	//	1L    // dderror
		ERRSTR(ERROR_FILE_NOT_FOUND);	//	2L
		ERRSTR(ERROR_PATH_NOT_FOUND);	//	3L
		ERRSTR(ERROR_TOO_MANY_OPEN_FILES);	//	4L
		ERRSTR(ERROR_ACCESS_DENIED);	//	5L
		ERRSTR(ERROR_INVALID_HANDLE);	//	6L
		ERRSTR(ERROR_ARENA_TRASHED);	//	7L
		ERRSTR(ERROR_NOT_ENOUGH_MEMORY);	//	8L    // dderror
		ERRSTR(ERROR_INVALID_BLOCK);	//	9L
		ERRSTR(ERROR_BAD_ENVIRONMENT);	//	10L
		ERRSTR(ERROR_BAD_FORMAT);	//	11L
		ERRSTR(ERROR_INVALID_ACCESS);	//	12L
		ERRSTR(ERROR_INVALID_DATA);	//	13L
		ERRSTR(ERROR_OUTOFMEMORY);	//	14L
		ERRSTR(ERROR_INVALID_DRIVE);	//	15L
		ERRSTR(ERROR_CURRENT_DIRECTORY);	//	16L
		ERRSTR(ERROR_NOT_SAME_DEVICE);	//	17L
		ERRSTR(ERROR_NO_MORE_FILES);	//	18L
		ERRSTR(ERROR_WRITE_PROTECT);	//	19L
		ERRSTR(ERROR_BAD_UNIT);	//	20L
		ERRSTR(ERROR_NOT_READY);	//	21L
		ERRSTR(ERROR_BAD_COMMAND);	//	22L
		ERRSTR(ERROR_CRC);	//	23L
		ERRSTR(ERROR_BAD_LENGTH);	//	24L
		ERRSTR(ERROR_SEEK);	//	25L
		ERRSTR(ERROR_NOT_DOS_DISK);	//	26L
		ERRSTR(ERROR_SECTOR_NOT_FOUND);	//	27L
		ERRSTR(ERROR_OUT_OF_PAPER);	//	28L
		ERRSTR(ERROR_WRITE_FAULT);	//	29L
		ERRSTR(ERROR_READ_FAULT);	//	30L
		ERRSTR(ERROR_GEN_FAILURE);	//	31L
		ERRSTR(ERROR_SHARING_VIOLATION);	//	32L
		ERRSTR(ERROR_LOCK_VIOLATION);	//	33L
		ERRSTR(ERROR_WRONG_DISK);	//	34L
		ERRSTR(ERROR_SHARING_BUFFER_EXCEEDED);	//	36L
		ERRSTR(ERROR_HANDLE_EOF);	//	38L
		ERRSTR(ERROR_HANDLE_DISK_FULL);	//	39L
		ERRSTR(ERROR_NOT_SUPPORTED);	//	50L
		ERRSTR(ERROR_REM_NOT_LIST);	//	51L
		ERRSTR(ERROR_DUP_NAME);	//	52L
		ERRSTR(ERROR_BAD_NETPATH);	//	53L
		ERRSTR(ERROR_NETWORK_BUSY);	//	54L
		ERRSTR(ERROR_DEV_NOT_EXIST);	//	55L    // dderror
		ERRSTR(ERROR_TOO_MANY_CMDS);	//	56L
		ERRSTR(ERROR_ADAP_HDW_ERR);	//	57L
		ERRSTR(ERROR_BAD_NET_RESP);	//	58L
		ERRSTR(ERROR_UNEXP_NET_ERR);	//	59L
		ERRSTR(ERROR_BAD_REM_ADAP);	//	60L
		ERRSTR(ERROR_PRINTQ_FULL);	//	61L
		ERRSTR(ERROR_NO_SPOOL_SPACE);	//	62L
		ERRSTR(ERROR_PRINT_CANCELLED);	//	63L
		ERRSTR(ERROR_NETNAME_DELETED);	//	64L
		ERRSTR(ERROR_NETWORK_ACCESS_DENIED);	//	65L
		ERRSTR(ERROR_BAD_DEV_TYPE);	//	66L
		ERRSTR(ERROR_BAD_NET_NAME);	//	67L
		ERRSTR(ERROR_TOO_MANY_NAMES);	//	68L
		ERRSTR(ERROR_TOO_MANY_SESS);	//	69L
		ERRSTR(ERROR_SHARING_PAUSED);	//	70L
		ERRSTR(ERROR_REQ_NOT_ACCEP);	//	71L
		ERRSTR(ERROR_REDIR_PAUSED);	//	72L
		ERRSTR(ERROR_FILE_EXISTS);	//	80L
		ERRSTR(ERROR_CANNOT_MAKE);	//	82L
		ERRSTR(ERROR_FAIL_I24);	//	83L
		ERRSTR(ERROR_OUT_OF_STRUCTURES);	//	84L
		ERRSTR(ERROR_ALREADY_ASSIGNED);	//	85L
		ERRSTR(ERROR_INVALID_PASSWORD);	//	86L
		ERRSTR(ERROR_INVALID_PARAMETER);	//	87L    // dderror
		ERRSTR(ERROR_NET_WRITE_FAULT);	//	88L
		ERRSTR(ERROR_NO_PROC_SLOTS);	//	89L
		ERRSTR(ERROR_TOO_MANY_SEMAPHORES);	//	100L
		ERRSTR(ERROR_EXCL_SEM_ALREADY_OWNED);	//	101L
		ERRSTR(ERROR_SEM_IS_SET);	//	102L
		ERRSTR(ERROR_TOO_MANY_SEM_REQUESTS);	//	103L
		ERRSTR(ERROR_INVALID_AT_INTERRUPT_TIME);	//	104L
		ERRSTR(ERROR_SEM_OWNER_DIED);	//	105L
		ERRSTR(ERROR_SEM_USER_LIMIT);	//	106L
		ERRSTR(ERROR_DISK_CHANGE);	//	107L
		ERRSTR(ERROR_DRIVE_LOCKED);	//	108L
		ERRSTR(ERROR_BROKEN_PIPE);	//	109L
		ERRSTR(ERROR_OPEN_FAILED);	//	110L
		ERRSTR(ERROR_BUFFER_OVERFLOW);	//	111L
		ERRSTR(ERROR_DISK_FULL);	//	112L
		ERRSTR(ERROR_NO_MORE_SEARCH_HANDLES);	//	113L
		ERRSTR(ERROR_INVALID_TARGET_HANDLE);	//	114L
		ERRSTR(ERROR_INVALID_CATEGORY);	//	117L
		ERRSTR(ERROR_INVALID_VERIFY_SWITCH);	//	118L
		ERRSTR(ERROR_BAD_DRIVER_LEVEL);	//	119L
		ERRSTR(ERROR_CALL_NOT_IMPLEMENTED);	//	120L
		ERRSTR(ERROR_SEM_TIMEOUT);	//	121L
		ERRSTR(ERROR_INSUFFICIENT_BUFFER);	//	122L    // dderror
		ERRSTR(ERROR_INVALID_NAME);	//	123L    // dderror
		ERRSTR(ERROR_INVALID_LEVEL);	//	124L
		ERRSTR(ERROR_NO_VOLUME_LABEL);	//	125L
		ERRSTR(ERROR_MOD_NOT_FOUND);	//	126L
		ERRSTR(ERROR_PROC_NOT_FOUND);	//	127L
		ERRSTR(ERROR_WAIT_NO_CHILDREN);	//	128L
		ERRSTR(ERROR_CHILD_NOT_COMPLETE);	//	129L
		ERRSTR(ERROR_DIRECT_ACCESS_HANDLE);	//	130L
		ERRSTR(ERROR_NEGATIVE_SEEK);	//	131L
		ERRSTR(ERROR_SEEK_ON_DEVICE);	//	132L
		ERRSTR(ERROR_IS_JOIN_TARGET);	//	133L
		ERRSTR(ERROR_IS_JOINED);	//	134L
		ERRSTR(ERROR_IS_SUBSTED);	//	135L
		ERRSTR(ERROR_NOT_JOINED);	//	136L
		ERRSTR(ERROR_NOT_SUBSTED);	//	137L
		ERRSTR(ERROR_JOIN_TO_JOIN);	//	138L
		ERRSTR(ERROR_SUBST_TO_SUBST);	//	139L
		ERRSTR(ERROR_JOIN_TO_SUBST);	//	140L
		ERRSTR(ERROR_SUBST_TO_JOIN);	//	141L
		ERRSTR(ERROR_BUSY_DRIVE);	//	142L
		ERRSTR(ERROR_SAME_DRIVE);	//	143L
		ERRSTR(ERROR_DIR_NOT_ROOT);	//	144L
		ERRSTR(ERROR_DIR_NOT_EMPTY);	//	145L
		ERRSTR(ERROR_IS_SUBST_PATH);	//	146L
		ERRSTR(ERROR_IS_JOIN_PATH);	//	147L
		ERRSTR(ERROR_PATH_BUSY);	//	148L
		ERRSTR(ERROR_IS_SUBST_TARGET);	//	149L
		ERRSTR(ERROR_SYSTEM_TRACE);	//	150L
		ERRSTR(ERROR_INVALID_EVENT_COUNT);	//	151L
		ERRSTR(ERROR_TOO_MANY_MUXWAITERS);	//	152L
		ERRSTR(ERROR_INVALID_LIST_FORMAT);	//	153L
		ERRSTR(ERROR_LABEL_TOO_LONG);	//	154L
		ERRSTR(ERROR_TOO_MANY_TCBS);	//	155L
		ERRSTR(ERROR_SIGNAL_REFUSED);	//	156L
		ERRSTR(ERROR_DISCARDED);	//	157L
		ERRSTR(ERROR_NOT_LOCKED);	//	158L
		ERRSTR(ERROR_BAD_THREADID_ADDR);	//	159L
		ERRSTR(ERROR_BAD_ARGUMENTS);	//	160L
		ERRSTR(ERROR_BAD_PATHNAME);	//	161L
		ERRSTR(ERROR_SIGNAL_PENDING);	//	162L
		ERRSTR(ERROR_MAX_THRDS_REACHED);	//	164L
		ERRSTR(ERROR_LOCK_FAILED);	//	167L
		ERRSTR(ERROR_BUSY);	//	170L    // dderror
		ERRSTR(ERROR_DEVICE_SUPPORT_IN_PROGRESS);	//	171L
		ERRSTR(ERROR_CANCEL_VIOLATION);	//	173L
		ERRSTR(ERROR_ATOMIC_LOCKS_NOT_SUPPORTED);	//	174L
		ERRSTR(ERROR_INVALID_SEGMENT_NUMBER);	//	180L
		ERRSTR(ERROR_INVALID_ORDINAL);	//	182L
		ERRSTR(ERROR_ALREADY_EXISTS);	//	183L
		ERRSTR(ERROR_INVALID_FLAG_NUMBER);	//	186L
		ERRSTR(ERROR_SEM_NOT_FOUND);	//	187L
		ERRSTR(ERROR_INVALID_STARTING_CODESEG);	//	188L
		ERRSTR(ERROR_INVALID_STACKSEG);	//	189L
		ERRSTR(ERROR_INVALID_MODULETYPE);	//	190L
		ERRSTR(ERROR_INVALID_EXE_SIGNATURE);	//	191L
		ERRSTR(ERROR_EXE_MARKED_INVALID);	//	192L
		ERRSTR(ERROR_BAD_EXE_FORMAT);	//	193L
		ERRSTR(ERROR_ITERATED_DATA_EXCEEDS_64k);	//	194L
		ERRSTR(ERROR_INVALID_MINALLOCSIZE);	//	195L
		ERRSTR(ERROR_DYNLINK_FROM_INVALID_RING);	//	196L
		ERRSTR(ERROR_IOPL_NOT_ENABLED);	//	197L
		ERRSTR(ERROR_INVALID_SEGDPL);	//	198L
		ERRSTR(ERROR_AUTODATASEG_EXCEEDS_64k);	//	199L
		ERRSTR(ERROR_RING2SEG_MUST_BE_MOVABLE);	//	200L
		ERRSTR(ERROR_RELOC_CHAIN_XEEDS_SEGLIM);	//	201L
		ERRSTR(ERROR_INFLOOP_IN_RELOC_CHAIN);	//	202L
		ERRSTR(ERROR_ENVVAR_NOT_FOUND);	//	203L
		ERRSTR(ERROR_NO_SIGNAL_SENT);	//	205L
		ERRSTR(ERROR_FILENAME_EXCED_RANGE);	//	206L
		ERRSTR(ERROR_RING2_STACK_IN_USE);	//	207L
		ERRSTR(ERROR_META_EXPANSION_TOO_LONG);	//	208L
		ERRSTR(ERROR_INVALID_SIGNAL_NUMBER);	//	209L
		ERRSTR(ERROR_THREAD_1_INACTIVE);	//	210L
		ERRSTR(ERROR_LOCKED);	//	212L
		ERRSTR(ERROR_TOO_MANY_MODULES);	//	214L
		ERRSTR(ERROR_NESTING_NOT_ALLOWED);	//	215L
		ERRSTR(ERROR_EXE_MACHINE_TYPE_MISMATCH);	//	216L
		ERRSTR(ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY);	//	217L
		ERRSTR(ERROR_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY);	//	218L
		ERRSTR(ERROR_FILE_CHECKED_OUT);	//	220L
		ERRSTR(ERROR_CHECKOUT_REQUIRED);	//	221L
		ERRSTR(ERROR_BAD_FILE_TYPE);	//	222L
		ERRSTR(ERROR_FILE_TOO_LARGE);	//	223L
		ERRSTR(ERROR_FORMS_AUTH_REQUIRED);	//	224L
		ERRSTR(ERROR_VIRUS_INFECTED);	//	225L
		ERRSTR(ERROR_VIRUS_DELETED);	//	226L
		ERRSTR(ERROR_PIPE_LOCAL);	//	229L
		ERRSTR(ERROR_BAD_PIPE);	//	230L
		ERRSTR(ERROR_PIPE_BUSY);	//	231L
		ERRSTR(ERROR_NO_DATA);	//	232L
		ERRSTR(ERROR_PIPE_NOT_CONNECTED);	//	233L
		ERRSTR(ERROR_MORE_DATA);	//	234L    // dderror
		ERRSTR(ERROR_NO_WORK_DONE);	//	235L
		ERRSTR(ERROR_VC_DISCONNECTED);	//	240L
		ERRSTR(ERROR_INVALID_EA_NAME);	//	254L
		ERRSTR(ERROR_EA_LIST_INCONSISTENT);	//	255L
		ERRSTR(WAIT_TIMEOUT);	//	258L    // dderror
		ERRSTR(ERROR_NO_MORE_ITEMS);	//	259L
		ERRSTR(ERROR_CANNOT_COPY);	//	266L
		ERRSTR(ERROR_DIRECTORY);	//	267L
		ERRSTR(ERROR_EAS_DIDNT_FIT);	//	275L
		ERRSTR(ERROR_EA_FILE_CORRUPT);	//	276L
		ERRSTR(ERROR_EA_TABLE_FULL);	//	277L
		ERRSTR(ERROR_INVALID_EA_HANDLE);	//	278L
		ERRSTR(ERROR_EAS_NOT_SUPPORTED);	//	282L
		ERRSTR(ERROR_NOT_OWNER);	//	288L
		ERRSTR(ERROR_TOO_MANY_POSTS);	//	298L
		ERRSTR(ERROR_PARTIAL_COPY);	//	299L
		ERRSTR(ERROR_OPLOCK_NOT_GRANTED);	//	300L
		ERRSTR(ERROR_INVALID_OPLOCK_PROTOCOL);	//	301L
		ERRSTR(ERROR_DISK_TOO_FRAGMENTED);	//	302L
		ERRSTR(ERROR_DELETE_PENDING);	//	303L
		ERRSTR(ERROR_INCOMPATIBLE_WITH_GLOBAL_SHORT_NAME_REGISTRY_SETTING);	//	304L
		ERRSTR(ERROR_SHORT_NAMES_NOT_ENABLED_ON_VOLUME);	//	305L
		ERRSTR(ERROR_SECURITY_STREAM_IS_INCONSISTENT);	//	306L
		ERRSTR(ERROR_INVALID_LOCK_RANGE);	//	307L
		ERRSTR(ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT);	//	308L
		ERRSTR(ERROR_NOTIFICATION_GUID_ALREADY_DEFINED);	//	309L
		ERRSTR(ERROR_INVALID_EXCEPTION_HANDLER);	//	310L
		ERRSTR(ERROR_DUPLICATE_PRIVILEGES);	//	311L
		ERRSTR(ERROR_NO_RANGES_PROCESSED);	//	312L
		ERRSTR(ERROR_NOT_ALLOWED_ON_SYSTEM_FILE);	//	313L
		ERRSTR(ERROR_DISK_RESOURCES_EXHAUSTED);	//	314L
		ERRSTR(ERROR_INVALID_TOKEN);	//	315L
		ERRSTR(ERROR_DEVICE_FEATURE_NOT_SUPPORTED);	//	316L
		ERRSTR(ERROR_MR_MID_NOT_FOUND);	//	317L
		ERRSTR(ERROR_SCOPE_NOT_FOUND);	//	318L
		ERRSTR(ERROR_UNDEFINED_SCOPE);	//	319L
		ERRSTR(ERROR_INVALID_CAP);	//	320L
		ERRSTR(ERROR_DEVICE_UNREACHABLE);	//	321L
		ERRSTR(ERROR_DEVICE_NO_RESOURCES);	//	322L
		ERRSTR(ERROR_DATA_CHECKSUM_ERROR);	//	323L
		ERRSTR(ERROR_INTERMIXED_KERNEL_EA_OPERATION);	//	324L
		ERRSTR(ERROR_FILE_LEVEL_TRIM_NOT_SUPPORTED);	//	326L
		ERRSTR(ERROR_OFFSET_ALIGNMENT_VIOLATION);	//	327L
		ERRSTR(ERROR_INVALID_FIELD_IN_PARAMETER_LIST);	//	328L
		ERRSTR(ERROR_OPERATION_IN_PROGRESS);	//	329L
		ERRSTR(ERROR_BAD_DEVICE_PATH);	//	330L
		ERRSTR(ERROR_TOO_MANY_DESCRIPTORS);	//	331L
		ERRSTR(ERROR_SCRUB_DATA_DISABLED);	//	332L
		ERRSTR(ERROR_NOT_REDUNDANT_STORAGE);	//	333L
		ERRSTR(ERROR_RESIDENT_FILE_NOT_SUPPORTED);	//	334L
		ERRSTR(ERROR_COMPRESSED_FILE_NOT_SUPPORTED);	//	335L
		ERRSTR(ERROR_DIRECTORY_NOT_SUPPORTED);	//	336L
		ERRSTR(ERROR_NOT_READ_FROM_COPY);	//	337L
		ERRSTR(ERROR_FT_WRITE_FAILURE);	//	338L
		ERRSTR(ERROR_FT_DI_SCAN_REQUIRED);	//	339L
		ERRSTR(ERROR_INVALID_KERNEL_INFO_VERSION);	//	340L
		ERRSTR(ERROR_INVALID_PEP_INFO_VERSION);	//	341L
		ERRSTR(ERROR_OBJECT_NOT_EXTERNALLY_BACKED);	//	342L
		ERRSTR(ERROR_EXTERNAL_BACKING_PROVIDER_UNKNOWN);	//	343L
		ERRSTR(ERROR_COMPRESSION_NOT_BENEFICIAL);	//	344L
		ERRSTR(ERROR_STORAGE_TOPOLOGY_ID_MISMATCH);	//	345L
		ERRSTR(ERROR_BLOCKED_BY_PARENTAL_CONTROLS);	//	346L
		ERRSTR(ERROR_BLOCK_TOO_MANY_REFERENCES);	//	347L
		ERRSTR(ERROR_MARKED_TO_DISALLOW_WRITES);	//	348L
		ERRSTR(ERROR_ENCLAVE_FAILURE);	//	349L
		ERRSTR(ERROR_FAIL_NOACTION_REBOOT);	//	350L
		ERRSTR(ERROR_FAIL_SHUTDOWN);	//	351L
		ERRSTR(ERROR_FAIL_RESTART);	//	352L
		ERRSTR(ERROR_MAX_SESSIONS_REACHED);	//	353L
		ERRSTR(ERROR_NETWORK_ACCESS_DENIED_EDP);	//	354L
		ERRSTR(ERROR_DEVICE_HINT_NAME_BUFFER_TOO_SMALL);	//	355L
		ERRSTR(ERROR_EDP_POLICY_DENIES_OPERATION);	//	356L
		ERRSTR(ERROR_EDP_DPL_POLICY_CANT_BE_SATISFIED);	//	357L
		ERRSTR(ERROR_CLOUD_FILE_SYNC_ROOT_METADATA_CORRUPT);	//	358L
		ERRSTR(ERROR_DEVICE_IN_MAINTENANCE);	//	359L
		ERRSTR(ERROR_NOT_SUPPORTED_ON_DAX);	//	360L
		ERRSTR(ERROR_DAX_MAPPING_EXISTS);	//	361L
		ERRSTR(ERROR_CLOUD_FILE_PROVIDER_NOT_RUNNING);	//	362L
		ERRSTR(ERROR_CLOUD_FILE_METADATA_CORRUPT);	//	363L
		ERRSTR(ERROR_CLOUD_FILE_METADATA_TOO_LARGE);	//	364L
		ERRSTR(ERROR_CLOUD_FILE_PROPERTY_BLOB_TOO_LARGE);	//	365L
		ERRSTR(ERROR_CLOUD_FILE_PROPERTY_BLOB_CHECKSUM_MISMATCH);	//	366L
		ERRSTR(ERROR_CHILD_PROCESS_BLOCKED);	//	367L
		ERRSTR(ERROR_STORAGE_LOST_DATA_PERSISTENCE);	//	368L
		ERRSTR(ERROR_FILE_SYSTEM_VIRTUALIZATION_UNAVAILABLE);	//	369L
		ERRSTR(ERROR_FILE_SYSTEM_VIRTUALIZATION_METADATA_CORRUPT);	//	370L
		ERRSTR(ERROR_FILE_SYSTEM_VIRTUALIZATION_BUSY);	//	371L
		ERRSTR(ERROR_FILE_SYSTEM_VIRTUALIZATION_PROVIDER_UNKNOWN);	//	372L
		ERRSTR(ERROR_GDI_HANDLE_LEAK);	//	373L
		ERRSTR(ERROR_CLOUD_FILE_TOO_MANY_PROPERTY_BLOBS);	//	374L
		ERRSTR(ERROR_CLOUD_FILE_PROPERTY_VERSION_NOT_SUPPORTED);	//	375L
		ERRSTR(ERROR_NOT_A_CLOUD_FILE);	//	376L
		ERRSTR(ERROR_CLOUD_FILE_NOT_IN_SYNC);	//	377L
		ERRSTR(ERROR_CLOUD_FILE_ALREADY_CONNECTED);	//	378L
		ERRSTR(ERROR_CLOUD_FILE_NOT_SUPPORTED);	//	379L
		ERRSTR(ERROR_CLOUD_FILE_INVALID_REQUEST);	//	380L
		ERRSTR(ERROR_CLOUD_FILE_READ_ONLY_VOLUME);	//	381L
		ERRSTR(ERROR_CLOUD_FILE_CONNECTED_PROVIDER_ONLY);	//	382L
		ERRSTR(ERROR_CLOUD_FILE_VALIDATION_FAILED);	//	383L
		ERRSTR(ERROR_SMB1_NOT_AVAILABLE);	//	384L
		ERRSTR(ERROR_FILE_SYSTEM_VIRTUALIZATION_INVALID_OPERATION);	//	385L
		ERRSTR(ERROR_CLOUD_FILE_AUTHENTICATION_FAILED);	//	386L
		ERRSTR(ERROR_CLOUD_FILE_INSUFFICIENT_RESOURCES);	//	387L
		ERRSTR(ERROR_CLOUD_FILE_NETWORK_UNAVAILABLE);	//	388L
		ERRSTR(ERROR_CLOUD_FILE_UNSUCCESSFUL);	//	389L
		ERRSTR(ERROR_CLOUD_FILE_NOT_UNDER_SYNC_ROOT);	//	390L
		ERRSTR(ERROR_CLOUD_FILE_IN_USE);	//	391L
		ERRSTR(ERROR_CLOUD_FILE_PINNED);	//	392L
		ERRSTR(ERROR_CLOUD_FILE_REQUEST_ABORTED);	//	393L
		ERRSTR(ERROR_CLOUD_FILE_PROPERTY_CORRUPT);	//	394L
		ERRSTR(ERROR_CLOUD_FILE_ACCESS_DENIED);	//	395L
		ERRSTR(ERROR_CLOUD_FILE_INCOMPATIBLE_HARDLINKS);	//	396L
		ERRSTR(ERROR_CLOUD_FILE_PROPERTY_LOCK_CONFLICT);	//	397L
		ERRSTR(ERROR_CLOUD_FILE_REQUEST_CANCELED);	//	398L
		ERRSTR(ERROR_EXTERNAL_SYSKEY_NOT_SUPPORTED);	//	399L
		ERRSTR(ERROR_THREAD_MODE_ALREADY_BACKGROUND);	//	400L
		ERRSTR(ERROR_THREAD_MODE_NOT_BACKGROUND);	//	401L
		ERRSTR(ERROR_PROCESS_MODE_ALREADY_BACKGROUND);	//	402L
		ERRSTR(ERROR_PROCESS_MODE_NOT_BACKGROUND);	//	403L
		ERRSTR(ERROR_CLOUD_FILE_PROVIDER_TERMINATED);	//	404L
		ERRSTR(ERROR_NOT_A_CLOUD_SYNC_ROOT);	//	405L
		ERRSTR(ERROR_FILE_PROTECTED_UNDER_DPL);	//	406L
		ERRSTR(ERROR_VOLUME_NOT_CLUSTER_ALIGNED);	//	407L
		ERRSTR(ERROR_NO_PHYSICALLY_ALIGNED_FREE_SPACE_FOUND);	//	408L
		ERRSTR(ERROR_APPX_FILE_NOT_ENCRYPTED);	//	409L
		ERRSTR(ERROR_RWRAW_ENCRYPTED_FILE_NOT_ENCRYPTED);	//	410L
		ERRSTR(ERROR_RWRAW_ENCRYPTED_INVALID_EDATAINFO_FILEOFFSET);	//	411L
		ERRSTR(ERROR_RWRAW_ENCRYPTED_INVALID_EDATAINFO_FILERANGE);	//	412L
		ERRSTR(ERROR_RWRAW_ENCRYPTED_INVALID_EDATAINFO_PARAMETER);	//	413L
		ERRSTR(ERROR_LINUX_SUBSYSTEM_NOT_PRESENT);	//	414L
		ERRSTR(ERROR_FT_READ_FAILURE);	//	415L
		ERRSTR(ERROR_STORAGE_RESERVE_ID_INVALID);	//	416L
		ERRSTR(ERROR_STORAGE_RESERVE_DOES_NOT_EXIST);	//	417L
		ERRSTR(ERROR_STORAGE_RESERVE_ALREADY_EXISTS);	//	418L
		ERRSTR(ERROR_STORAGE_RESERVE_NOT_EMPTY);	//	419L
		ERRSTR(ERROR_NOT_A_DAX_VOLUME);	//	420L
		ERRSTR(ERROR_NOT_DAX_MAPPABLE);	//	421L
		ERRSTR(ERROR_TIME_SENSITIVE_THREAD);	//	422L
		ERRSTR(ERROR_DPL_NOT_SUPPORTED_FOR_USER);	//	423L
		ERRSTR(ERROR_CASE_DIFFERING_NAMES_IN_DIR);	//	424L
		ERRSTR(ERROR_FILE_NOT_SUPPORTED);	//	425L
		ERRSTR(ERROR_CLOUD_FILE_REQUEST_TIMEOUT);	//	426L
		ERRSTR(ERROR_NO_TASK_QUEUE);	//	427L
		ERRSTR(ERROR_SRC_SRV_DLL_LOAD_FAILED);	//	428L
		ERRSTR(ERROR_NOT_SUPPORTED_WITH_BTT);	//	429L
		ERRSTR(ERROR_ENCRYPTION_DISABLED);	//	430L
		ERRSTR(ERROR_ENCRYPTING_METADATA_DISALLOWED);	//	431L
		ERRSTR(ERROR_CANT_CLEAR_ENCRYPTION_FLAG);	//	432L
		ERRSTR(ERROR_NO_SUCH_DEVICE);	//	433L
		ERRSTR(ERROR_CLOUD_FILE_DEHYDRATION_DISALLOWED);	//	434L
		ERRSTR(ERROR_FILE_SNAP_IN_PROGRESS);	//	435L
		ERRSTR(ERROR_FILE_SNAP_USER_SECTION_NOT_SUPPORTED);	//	436L
		ERRSTR(ERROR_FILE_SNAP_MODIFY_NOT_SUPPORTED);	//	437L
		ERRSTR(ERROR_FILE_SNAP_IO_NOT_COORDINATED);	//	438L
		ERRSTR(ERROR_FILE_SNAP_UNEXPECTED_ERROR);	//	439L
		ERRSTR(ERROR_FILE_SNAP_INVALID_PARAMETER);	//	440L
		ERRSTR(ERROR_UNSATISFIED_DEPENDENCIES);	//	441L
		ERRSTR(ERROR_CASE_SENSITIVE_PATH);	//	442L
		ERRSTR(ERROR_UNEXPECTED_NTCACHEMANAGER_ERROR);	//	443L
		ERRSTR(ERROR_LINUX_SUBSYSTEM_UPDATE_REQUIRED);	//	444L
		ERRSTR(ERROR_DLP_POLICY_WARNS_AGAINST_OPERATION);	//	445L
		ERRSTR(ERROR_DLP_POLICY_DENIES_OPERATION);	//	446L
		ERRSTR(ERROR_DLP_POLICY_SILENTLY_FAIL);	//	449L
		ERRSTR(ERROR_CAPAUTHZ_NOT_DEVUNLOCKED);	//	450L
		ERRSTR(ERROR_CAPAUTHZ_CHANGE_TYPE);	//	451L
		ERRSTR(ERROR_CAPAUTHZ_NOT_PROVISIONED);	//	452L
		ERRSTR(ERROR_CAPAUTHZ_NOT_AUTHORIZED);	//	453L
		ERRSTR(ERROR_CAPAUTHZ_NO_POLICY);	//	454L
		ERRSTR(ERROR_CAPAUTHZ_DB_CORRUPTED);	//	455L
		ERRSTR(ERROR_CAPAUTHZ_SCCD_INVALID_CATALOG);	//	456L
		ERRSTR(ERROR_CAPAUTHZ_SCCD_NO_AUTH_ENTITY);	//	457L
		ERRSTR(ERROR_CAPAUTHZ_SCCD_PARSE_ERROR);	//	458L
		ERRSTR(ERROR_CAPAUTHZ_SCCD_DEV_MODE_REQUIRED);	//	459L
		ERRSTR(ERROR_CAPAUTHZ_SCCD_NO_CAPABILITY_MATCH);	//	460L
		ERRSTR(ERROR_CIMFS_IMAGE_CORRUPT);	//	470L
		ERRSTR(ERROR_PNP_QUERY_REMOVE_DEVICE_TIMEOUT);	//	480L
		ERRSTR(ERROR_PNP_QUERY_REMOVE_RELATED_DEVICE_TIMEOUT);	//	481L
		ERRSTR(ERROR_PNP_QUERY_REMOVE_UNRELATED_DEVICE_TIMEOUT);	//	482L
		ERRSTR(ERROR_DEVICE_HARDWARE_ERROR);	//	483L
		ERRSTR(ERROR_INVALID_ADDRESS);	//	487L
		ERRSTR(ERROR_HAS_SYSTEM_CRITICAL_FILES);	//	488L
		//ERRSTR(ERROR_VRF_CFG_AND_IO_ENABLED);	//	1183L
		ERRSTR(ERROR_PARTITION_TERMINATING);	//	1184L
		ERRSTR(ERROR_USER_PROFILE_LOAD);	//	500L
		ERRSTR(ERROR_ARITHMETIC_OVERFLOW);	//	534L
		ERRSTR(ERROR_PIPE_CONNECTED);	//	535L
		ERRSTR(ERROR_PIPE_LISTENING);	//	536L
		ERRSTR(ERROR_VERIFIER_STOP);	//	537L
		ERRSTR(ERROR_ABIOS_ERROR);	//	538L
		ERRSTR(ERROR_WX86_WARNING);	//	539L
		ERRSTR(ERROR_WX86_ERROR);	//	540L
		ERRSTR(ERROR_TIMER_NOT_CANCELED);	//	541L
		ERRSTR(ERROR_UNWIND);	//	542L
		ERRSTR(ERROR_BAD_STACK);	//	543L
		ERRSTR(ERROR_INVALID_UNWIND_TARGET);	//	544L
		ERRSTR(ERROR_INVALID_PORT_ATTRIBUTES);	//	545L
		ERRSTR(ERROR_PORT_MESSAGE_TOO_LONG);	//	546L
		ERRSTR(ERROR_INVALID_QUOTA_LOWER);	//	547L
		ERRSTR(ERROR_DEVICE_ALREADY_ATTACHED);	//	548L
		ERRSTR(ERROR_INSTRUCTION_MISALIGNMENT);	//	549L
		ERRSTR(ERROR_PROFILING_NOT_STARTED);	//	550L
		ERRSTR(ERROR_PROFILING_NOT_STOPPED);	//	551L
		ERRSTR(ERROR_COULD_NOT_INTERPRET);	//	552L
		ERRSTR(ERROR_PROFILING_AT_LIMIT);	//	553L
		ERRSTR(ERROR_CANT_WAIT);	//	554L
		ERRSTR(ERROR_CANT_TERMINATE_SELF);	//	555L
		ERRSTR(ERROR_UNEXPECTED_MM_CREATE_ERR);	//	556L
		ERRSTR(ERROR_UNEXPECTED_MM_MAP_ERROR);	//	557L
		ERRSTR(ERROR_UNEXPECTED_MM_EXTEND_ERR);	//	558L
		ERRSTR(ERROR_BAD_FUNCTION_TABLE);	//	559L
		ERRSTR(ERROR_NO_GUID_TRANSLATION);	//	560L
		ERRSTR(ERROR_INVALID_LDT_SIZE);	//	561L
		ERRSTR(ERROR_INVALID_LDT_OFFSET);	//	563L
		ERRSTR(ERROR_INVALID_LDT_DESCRIPTOR);	//	564L
		ERRSTR(ERROR_TOO_MANY_THREADS);	//	565L
		ERRSTR(ERROR_THREAD_NOT_IN_PROCESS);	//	566L
		ERRSTR(ERROR_PAGEFILE_QUOTA_EXCEEDED);	//	567L
		ERRSTR(ERROR_LOGON_SERVER_CONFLICT);	//	568L
		ERRSTR(ERROR_SYNCHRONIZATION_REQUIRED);	//	569L
		ERRSTR(ERROR_NET_OPEN_FAILED);	//	570L
		ERRSTR(ERROR_IO_PRIVILEGE_FAILED);	//	571L
		ERRSTR(ERROR_CONTROL_C_EXIT);	//	572L    // winnt
		ERRSTR(ERROR_MISSING_SYSTEMFILE);	//	573L
		ERRSTR(ERROR_UNHANDLED_EXCEPTION);	//	574L
		ERRSTR(ERROR_APP_INIT_FAILURE);	//	575L
		ERRSTR(ERROR_PAGEFILE_CREATE_FAILED);	//	576L
		ERRSTR(ERROR_INVALID_IMAGE_HASH);	//	577L
		ERRSTR(ERROR_NO_PAGEFILE);	//	578L
		ERRSTR(ERROR_ILLEGAL_FLOAT_CONTEXT);	//	579L
		ERRSTR(ERROR_NO_EVENT_PAIR);	//	580L
		ERRSTR(ERROR_DOMAIN_CTRLR_CONFIG_ERROR);	//	581L
		ERRSTR(ERROR_ILLEGAL_CHARACTER);	//	582L
		ERRSTR(ERROR_UNDEFINED_CHARACTER);	//	583L
		ERRSTR(ERROR_FLOPPY_VOLUME);	//	584L
		ERRSTR(ERROR_BIOS_FAILED_TO_CONNECT_INTERRUPT);	//	585L
		ERRSTR(ERROR_BACKUP_CONTROLLER);	//	586L
		ERRSTR(ERROR_MUTANT_LIMIT_EXCEEDED);	//	587L
		ERRSTR(ERROR_FS_DRIVER_REQUIRED);	//	588L
		ERRSTR(ERROR_CANNOT_LOAD_REGISTRY_FILE);	//	589L
		ERRSTR(ERROR_DEBUG_ATTACH_FAILED);	//	590L
		ERRSTR(ERROR_SYSTEM_PROCESS_TERMINATED);	//	591L
		ERRSTR(ERROR_DATA_NOT_ACCEPTED);	//	592L
		ERRSTR(ERROR_VDM_HARD_ERROR);	//	593L
		ERRSTR(ERROR_DRIVER_CANCEL_TIMEOUT);	//	594L
		ERRSTR(ERROR_REPLY_MESSAGE_MISMATCH);	//	595L
		ERRSTR(ERROR_LOST_WRITEBEHIND_DATA);	//	596L
		ERRSTR(ERROR_CLIENT_SERVER_PARAMETERS_INVALID);	//	597L
		ERRSTR(ERROR_NOT_TINY_STREAM);	//	598L
		ERRSTR(ERROR_STACK_OVERFLOW_READ);	//	599L
		ERRSTR(ERROR_CONVERT_TO_LARGE);	//	600L
		ERRSTR(ERROR_FOUND_OUT_OF_SCOPE);	//	601L
		ERRSTR(ERROR_ALLOCATE_BUCKET);	//	602L
		ERRSTR(ERROR_MARSHALL_OVERFLOW);	//	603L
		ERRSTR(ERROR_INVALID_VARIANT);	//	604L
		ERRSTR(ERROR_BAD_COMPRESSION_BUFFER);	//	605L
		ERRSTR(ERROR_AUDIT_FAILED);	//	606L
		ERRSTR(ERROR_TIMER_RESOLUTION_NOT_SET);	//	607L
		ERRSTR(ERROR_INSUFFICIENT_LOGON_INFO);	//	608L
		ERRSTR(ERROR_BAD_DLL_ENTRYPOINT);	//	609L
		ERRSTR(ERROR_BAD_SERVICE_ENTRYPOINT);	//	610L
		ERRSTR(ERROR_IP_ADDRESS_CONFLICT1);	//	611L
		ERRSTR(ERROR_IP_ADDRESS_CONFLICT2);	//	612L
		ERRSTR(ERROR_REGISTRY_QUOTA_LIMIT);	//	613L
		ERRSTR(ERROR_NO_CALLBACK_ACTIVE);	//	614L
		ERRSTR(ERROR_PWD_TOO_SHORT);	//	615L
		ERRSTR(ERROR_PWD_TOO_RECENT);	//	616L
		ERRSTR(ERROR_PWD_HISTORY_CONFLICT);	//	617L
		ERRSTR(ERROR_UNSUPPORTED_COMPRESSION);	//	618L
		ERRSTR(ERROR_INVALID_HW_PROFILE);	//	619L
		ERRSTR(ERROR_INVALID_PLUGPLAY_DEVICE_PATH);	//	620L
		ERRSTR(ERROR_QUOTA_LIST_INCONSISTENT);	//	621L
		ERRSTR(ERROR_EVALUATION_EXPIRATION);	//	622L
		ERRSTR(ERROR_ILLEGAL_DLL_RELOCATION);	//	623L
		ERRSTR(ERROR_DLL_INIT_FAILED_LOGOFF);	//	624L
		ERRSTR(ERROR_VALIDATE_CONTINUE);	//	625L
		ERRSTR(ERROR_NO_MORE_MATCHES);	//	626L
		ERRSTR(ERROR_RANGE_LIST_CONFLICT);	//	627L
		ERRSTR(ERROR_SERVER_SID_MISMATCH);	//	628L
		ERRSTR(ERROR_CANT_ENABLE_DENY_ONLY);	//	629L
		ERRSTR(ERROR_FLOAT_MULTIPLE_FAULTS);	//	630L    // winnt
		ERRSTR(ERROR_FLOAT_MULTIPLE_TRAPS);	//	631L    // winnt
		ERRSTR(ERROR_NOINTERFACE);	//	632L
		ERRSTR(ERROR_DRIVER_FAILED_SLEEP);	//	633L
		ERRSTR(ERROR_CORRUPT_SYSTEM_FILE);	//	634L
		ERRSTR(ERROR_COMMITMENT_MINIMUM);	//	635L
		ERRSTR(ERROR_PNP_RESTART_ENUMERATION);	//	636L
		ERRSTR(ERROR_SYSTEM_IMAGE_BAD_SIGNATURE);	//	637L
		ERRSTR(ERROR_PNP_REBOOT_REQUIRED);	//	638L
		ERRSTR(ERROR_INSUFFICIENT_POWER);	//	639L
		ERRSTR(ERROR_MULTIPLE_FAULT_VIOLATION);	//	640L
		ERRSTR(ERROR_SYSTEM_SHUTDOWN);	//	641L
		ERRSTR(ERROR_PORT_NOT_SET);	//	642L
		ERRSTR(ERROR_DS_VERSION_CHECK_FAILURE);	//	643L
		ERRSTR(ERROR_RANGE_NOT_FOUND);	//	644L
		ERRSTR(ERROR_NOT_SAFE_MODE_DRIVER);	//	646L
		ERRSTR(ERROR_FAILED_DRIVER_ENTRY);	//	647L
		ERRSTR(ERROR_DEVICE_ENUMERATION_ERROR);	//	648L
		ERRSTR(ERROR_MOUNT_POINT_NOT_RESOLVED);	//	649L
		ERRSTR(ERROR_INVALID_DEVICE_OBJECT_PARAMETER);	//	650L
		ERRSTR(ERROR_MCA_OCCURED);	//	651L
		ERRSTR(ERROR_DRIVER_DATABASE_ERROR);	//	652L
		ERRSTR(ERROR_SYSTEM_HIVE_TOO_LARGE);	//	653L
		ERRSTR(ERROR_DRIVER_FAILED_PRIOR_UNLOAD);	//	654L
		ERRSTR(ERROR_VOLSNAP_PREPARE_HIBERNATE);	//	655L
		ERRSTR(ERROR_HIBERNATION_FAILURE);	//	656L
		ERRSTR(ERROR_PWD_TOO_LONG);	//	657L
		ERRSTR(ERROR_FILE_SYSTEM_LIMITATION);	//	665L
		ERRSTR(ERROR_ASSERTION_FAILURE);	//	668L
		ERRSTR(ERROR_ACPI_ERROR);	//	669L
		ERRSTR(ERROR_WOW_ASSERTION);	//	670L
		ERRSTR(ERROR_PNP_BAD_MPS_TABLE);	//	671L
		ERRSTR(ERROR_PNP_TRANSLATION_FAILED);	//	672L
		ERRSTR(ERROR_PNP_IRQ_TRANSLATION_FAILED);	//	673L
		ERRSTR(ERROR_PNP_INVALID_ID);	//	674L
		ERRSTR(ERROR_WAKE_SYSTEM_DEBUGGER);	//	675L
		ERRSTR(ERROR_HANDLES_CLOSED);	//	676L
		ERRSTR(ERROR_EXTRANEOUS_INFORMATION);	//	677L
		ERRSTR(ERROR_RXACT_COMMIT_NECESSARY);	//	678L
		ERRSTR(ERROR_MEDIA_CHECK);	//	679L
		ERRSTR(ERROR_GUID_SUBSTITUTION_MADE);	//	680L
		ERRSTR(ERROR_STOPPED_ON_SYMLINK);	//	681L
		ERRSTR(ERROR_LONGJUMP);	//	682L
		ERRSTR(ERROR_PLUGPLAY_QUERY_VETOED);	//	683L
		ERRSTR(ERROR_UNWIND_CONSOLIDATE);	//	684L
		ERRSTR(ERROR_REGISTRY_HIVE_RECOVERED);	//	685L
		ERRSTR(ERROR_DLL_MIGHT_BE_INSECURE);	//	686L
		ERRSTR(ERROR_DLL_MIGHT_BE_INCOMPATIBLE);	//	687L
		ERRSTR(ERROR_DBG_EXCEPTION_NOT_HANDLED);	//	688L    // winnt
		ERRSTR(ERROR_DBG_REPLY_LATER);	//	689L
		ERRSTR(ERROR_DBG_UNABLE_TO_PROVIDE_HANDLE);	//	690L
		ERRSTR(ERROR_DBG_TERMINATE_THREAD);	//	691L    // winnt
		ERRSTR(ERROR_DBG_TERMINATE_PROCESS);	//	692L    // winnt
		ERRSTR(ERROR_DBG_CONTROL_C);	//	693L    // winnt
		ERRSTR(ERROR_DBG_PRINTEXCEPTION_C);	//	694L
		ERRSTR(ERROR_DBG_RIPEXCEPTION);	//	695L
		ERRSTR(ERROR_DBG_CONTROL_BREAK);	//	696L    // winnt
		ERRSTR(ERROR_DBG_COMMAND_EXCEPTION);	//	697L    // winnt
		ERRSTR(ERROR_OBJECT_NAME_EXISTS);	//	698L
		ERRSTR(ERROR_THREAD_WAS_SUSPENDED);	//	699L
		ERRSTR(ERROR_IMAGE_NOT_AT_BASE);	//	700L
		ERRSTR(ERROR_RXACT_STATE_CREATED);	//	701L
		ERRSTR(ERROR_SEGMENT_NOTIFICATION);	//	702L    // winnt
		ERRSTR(ERROR_BAD_CURRENT_DIRECTORY);	//	703L
		ERRSTR(ERROR_FT_READ_RECOVERY_FROM_BACKUP);	//	704L
		ERRSTR(ERROR_FT_WRITE_RECOVERY);	//	705L
		ERRSTR(ERROR_IMAGE_MACHINE_TYPE_MISMATCH);	//	706L
		ERRSTR(ERROR_RECEIVE_PARTIAL);	//	707L
		ERRSTR(ERROR_RECEIVE_EXPEDITED);	//	708L
		ERRSTR(ERROR_RECEIVE_PARTIAL_EXPEDITED);	//	709L
		ERRSTR(ERROR_EVENT_DONE);	//	710L
		ERRSTR(ERROR_EVENT_PENDING);	//	711L
		ERRSTR(ERROR_CHECKING_FILE_SYSTEM);	//	712L
		ERRSTR(ERROR_FATAL_APP_EXIT);	//	713L
		ERRSTR(ERROR_PREDEFINED_HANDLE);	//	714L
		ERRSTR(ERROR_WAS_UNLOCKED);	//	715L
		ERRSTR(ERROR_SERVICE_NOTIFICATION);	//	716L
		ERRSTR(ERROR_WAS_LOCKED);	//	717L
		ERRSTR(ERROR_LOG_HARD_ERROR);	//	718L
		ERRSTR(ERROR_ALREADY_WIN32);	//	719L
		ERRSTR(ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE);	//	720L
		ERRSTR(ERROR_NO_YIELD_PERFORMED);	//	721L
		ERRSTR(ERROR_TIMER_RESUME_IGNORED);	//	722L
		ERRSTR(ERROR_ARBITRATION_UNHANDLED);	//	723L
		ERRSTR(ERROR_CARDBUS_NOT_SUPPORTED);	//	724L
		ERRSTR(ERROR_MP_PROCESSOR_MISMATCH);	//	725L
		ERRSTR(ERROR_HIBERNATED);	//	726L    
		ERRSTR(ERROR_RESUME_HIBERNATION);	//	727L    
		ERRSTR(ERROR_FIRMWARE_UPDATED);	//	728L
		ERRSTR(ERROR_DRIVERS_LEAKING_LOCKED_PAGES);	//	729L
		ERRSTR(ERROR_WAKE_SYSTEM);	//	730L
		ERRSTR(ERROR_WAIT_1);	//	731L
		ERRSTR(ERROR_WAIT_2);	//	732L
		ERRSTR(ERROR_WAIT_3);	//	733L
		ERRSTR(ERROR_WAIT_63);	//	734L
		ERRSTR(ERROR_ABANDONED_WAIT_0);	//	735L    // winnt
		ERRSTR(ERROR_ABANDONED_WAIT_63);	//	736L
		ERRSTR(ERROR_USER_APC);	//	737L    // winnt
		ERRSTR(ERROR_KERNEL_APC);	//	738L
		ERRSTR(ERROR_ALERTED);	//	739L
		ERRSTR(ERROR_ELEVATION_REQUIRED);	//	740L
		ERRSTR(ERROR_REPARSE);	//	741L
		ERRSTR(ERROR_OPLOCK_BREAK_IN_PROGRESS);	//	742L
		ERRSTR(ERROR_VOLUME_MOUNTED);	//	743L
		ERRSTR(ERROR_RXACT_COMMITTED);	//	744L
		ERRSTR(ERROR_NOTIFY_CLEANUP);	//	745L
		ERRSTR(ERROR_PRIMARY_TRANSPORT_CONNECT_FAILED);	//	746L
		ERRSTR(ERROR_PAGE_FAULT_TRANSITION);	//	747L
		ERRSTR(ERROR_PAGE_FAULT_DEMAND_ZERO);	//	748L
		ERRSTR(ERROR_PAGE_FAULT_COPY_ON_WRITE);	//	749L
		ERRSTR(ERROR_PAGE_FAULT_GUARD_PAGE);	//	750L
		ERRSTR(ERROR_PAGE_FAULT_PAGING_FILE);	//	751L
		ERRSTR(ERROR_CACHE_PAGE_LOCKED);	//	752L
		ERRSTR(ERROR_CRASH_DUMP);	//	753L
		ERRSTR(ERROR_BUFFER_ALL_ZEROS);	//	754L
		ERRSTR(ERROR_REPARSE_OBJECT);	//	755L
		ERRSTR(ERROR_RESOURCE_REQUIREMENTS_CHANGED);	//	756L
		ERRSTR(ERROR_TRANSLATION_COMPLETE);	//	757L
		ERRSTR(ERROR_NOTHING_TO_TERMINATE);	//	758L
		ERRSTR(ERROR_PROCESS_NOT_IN_JOB);	//	759L
		ERRSTR(ERROR_PROCESS_IN_JOB);	//	760L
		ERRSTR(ERROR_VOLSNAP_HIBERNATE_READY);	//	761L
		ERRSTR(ERROR_FSFILTER_OP_COMPLETED_SUCCESSFULLY);	//	762L
		ERRSTR(ERROR_INTERRUPT_VECTOR_ALREADY_CONNECTED);	//	763L
		ERRSTR(ERROR_INTERRUPT_STILL_CONNECTED);	//	764L
		ERRSTR(ERROR_WAIT_FOR_OPLOCK);	//	765L
		ERRSTR(ERROR_DBG_EXCEPTION_HANDLED);	//	766L    // winnt
		ERRSTR(ERROR_DBG_CONTINUE);	//	767L    // winnt
		ERRSTR(ERROR_CALLBACK_POP_STACK);	//	768L
		ERRSTR(ERROR_COMPRESSION_DISABLED);	//	769L
		ERRSTR(ERROR_CANTFETCHBACKWARDS);	//	770L
		ERRSTR(ERROR_CANTSCROLLBACKWARDS);	//	771L
		ERRSTR(ERROR_ROWSNOTRELEASED);	//	772L
		ERRSTR(ERROR_BAD_ACCESSOR_FLAGS);	//	773L
		ERRSTR(ERROR_ERRORS_ENCOUNTERED);	//	774L
		ERRSTR(ERROR_NOT_CAPABLE);	//	775L
		ERRSTR(ERROR_REQUEST_OUT_OF_SEQUENCE);	//	776L
		ERRSTR(ERROR_VERSION_PARSE_ERROR);	//	777L
		ERRSTR(ERROR_BADSTARTPOSITION);	//	778L
		ERRSTR(ERROR_MEMORY_HARDWARE);	//	779L
		ERRSTR(ERROR_DISK_REPAIR_DISABLED);	//	780L
		ERRSTR(ERROR_INSUFFICIENT_RESOURCE_FOR_SPECIFIED_SHARED_SECTION_SIZE);	//	781L
		ERRSTR(ERROR_SYSTEM_POWERSTATE_TRANSITION);	//	782L
		ERRSTR(ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION);	//	783L
		ERRSTR(ERROR_MCA_EXCEPTION);	//	784L
		ERRSTR(ERROR_ACCESS_AUDIT_BY_POLICY);	//	785L
		ERRSTR(ERROR_ACCESS_DISABLED_NO_SAFER_UI_BY_POLICY);	//	786L
		ERRSTR(ERROR_ABANDON_HIBERFILE);	//	787L
		ERRSTR(ERROR_LOST_WRITEBEHIND_DATA_NETWORK_DISCONNECTED);	//	788L
		ERRSTR(ERROR_LOST_WRITEBEHIND_DATA_NETWORK_SERVER_ERROR);	//	789L
		ERRSTR(ERROR_LOST_WRITEBEHIND_DATA_LOCAL_DISK_ERROR);	//	790L
		ERRSTR(ERROR_BAD_MCFG_TABLE);	//	791L
		ERRSTR(ERROR_DISK_REPAIR_REDIRECTED);	//	792L
		ERRSTR(ERROR_DISK_REPAIR_UNSUCCESSFUL);	//	793L
		ERRSTR(ERROR_CORRUPT_LOG_OVERFULL);	//	794L
		ERRSTR(ERROR_CORRUPT_LOG_CORRUPTED);	//	795L
		ERRSTR(ERROR_CORRUPT_LOG_UNAVAILABLE);	//	796L
		ERRSTR(ERROR_CORRUPT_LOG_DELETED_FULL);	//	797L
		ERRSTR(ERROR_CORRUPT_LOG_CLEARED);	//	798L
		ERRSTR(ERROR_ORPHAN_NAME_EXHAUSTED);	//	799L
		ERRSTR(ERROR_OPLOCK_SWITCHED_TO_NEW_HANDLE);	//	800L
		ERRSTR(ERROR_CANNOT_GRANT_REQUESTED_OPLOCK);	//	801L
		ERRSTR(ERROR_CANNOT_BREAK_OPLOCK);	//	802L
		ERRSTR(ERROR_OPLOCK_HANDLE_CLOSED);	//	803L
		ERRSTR(ERROR_NO_ACE_CONDITION);	//	804L
		ERRSTR(ERROR_INVALID_ACE_CONDITION);	//	805L
		ERRSTR(ERROR_FILE_HANDLE_REVOKED);	//	806L
		ERRSTR(ERROR_IMAGE_AT_DIFFERENT_BASE);	//	807L
		ERRSTR(ERROR_ENCRYPTED_IO_NOT_POSSIBLE);	//	808L
		ERRSTR(ERROR_FILE_METADATA_OPTIMIZATION_IN_PROGRESS);	//	809L
		ERRSTR(ERROR_QUOTA_ACTIVITY);	//	810L
		ERRSTR(ERROR_HANDLE_REVOKED);	//	811L
		ERRSTR(ERROR_CALLBACK_INVOKE_INLINE);	//	812L
		ERRSTR(ERROR_CPU_SET_INVALID);	//	813L
		ERRSTR(ERROR_ENCLAVE_NOT_TERMINATED);	//	814L
		ERRSTR(ERROR_ENCLAVE_VIOLATION);	//	815L
		ERRSTR(ERROR_EA_ACCESS_DENIED);	//	994L
		ERRSTR(ERROR_OPERATION_ABORTED);	//	995L
		ERRSTR(ERROR_IO_INCOMPLETE);	//	996L
		ERRSTR(ERROR_IO_PENDING);	//	997L    // dderror
		ERRSTR(ERROR_NOACCESS);	//	998L
		ERRSTR(ERROR_SWAPERROR);	//	999L
		ERRSTR(ERROR_STACK_OVERFLOW);	//	1001L
		ERRSTR(ERROR_INVALID_MESSAGE);	//	1002L
		ERRSTR(ERROR_CAN_NOT_COMPLETE);	//	1003L
		ERRSTR(ERROR_INVALID_FLAGS);	//	1004L
		ERRSTR(ERROR_UNRECOGNIZED_VOLUME);	//	1005L
		ERRSTR(ERROR_FILE_INVALID);	//	1006L
		ERRSTR(ERROR_FULLSCREEN_MODE);	//	1007L
		ERRSTR(ERROR_NO_TOKEN);	//	1008L
		ERRSTR(ERROR_BADDB);	//	1009L
		ERRSTR(ERROR_BADKEY);	//	1010L
		ERRSTR(ERROR_CANTOPEN);	//	1011L
		ERRSTR(ERROR_CANTREAD);	//	1012L
		ERRSTR(ERROR_CANTWRITE);	//	1013L
		ERRSTR(ERROR_REGISTRY_RECOVERED);	//	1014L
		ERRSTR(ERROR_REGISTRY_CORRUPT);	//	1015L
		ERRSTR(ERROR_REGISTRY_IO_FAILED);	//	1016L
		ERRSTR(ERROR_NOT_REGISTRY_FILE);	//	1017L
		ERRSTR(ERROR_KEY_DELETED);	//	1018L
		ERRSTR(ERROR_NO_LOG_SPACE);	//	1019L
		ERRSTR(ERROR_KEY_HAS_CHILDREN);	//	1020L
		ERRSTR(ERROR_CHILD_MUST_BE_VOLATILE);	//	1021L
		ERRSTR(ERROR_NOTIFY_ENUM_DIR);	//	1022L
		ERRSTR(ERROR_DEPENDENT_SERVICES_RUNNING);	//	1051L
		ERRSTR(ERROR_INVALID_SERVICE_CONTROL);	//	1052L
		ERRSTR(ERROR_SERVICE_REQUEST_TIMEOUT);	//	1053L
		ERRSTR(ERROR_SERVICE_NO_THREAD);	//	1054L
		ERRSTR(ERROR_SERVICE_DATABASE_LOCKED);	//	1055L
		ERRSTR(ERROR_SERVICE_ALREADY_RUNNING);	//	1056L
		ERRSTR(ERROR_INVALID_SERVICE_ACCOUNT);	//	1057L
		ERRSTR(ERROR_SERVICE_DISABLED);	//	1058L
		ERRSTR(ERROR_CIRCULAR_DEPENDENCY);	//	1059L
		ERRSTR(ERROR_SERVICE_DOES_NOT_EXIST);	//	1060L
		ERRSTR(ERROR_SERVICE_CANNOT_ACCEPT_CTRL);	//	1061L
		ERRSTR(ERROR_SERVICE_NOT_ACTIVE);	//	1062L
		ERRSTR(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);	//	1063L
		ERRSTR(ERROR_EXCEPTION_IN_SERVICE);	//	1064L
		ERRSTR(ERROR_DATABASE_DOES_NOT_EXIST);	//	1065L
		ERRSTR(ERROR_SERVICE_SPECIFIC_ERROR);	//	1066L
		ERRSTR(ERROR_PROCESS_ABORTED);	//	1067L
		ERRSTR(ERROR_SERVICE_DEPENDENCY_FAIL);	//	1068L
		ERRSTR(ERROR_SERVICE_LOGON_FAILED);	//	1069L
		ERRSTR(ERROR_SERVICE_START_HANG);	//	1070L
		ERRSTR(ERROR_INVALID_SERVICE_LOCK);	//	1071L
		ERRSTR(ERROR_SERVICE_MARKED_FOR_DELETE);	//	1072L
		ERRSTR(ERROR_SERVICE_EXISTS);	//	1073L
		ERRSTR(ERROR_ALREADY_RUNNING_LKG);	//	1074L
		ERRSTR(ERROR_SERVICE_DEPENDENCY_DELETED);	//	1075L
		ERRSTR(ERROR_BOOT_ALREADY_ACCEPTED);	//	1076L
		ERRSTR(ERROR_SERVICE_NEVER_STARTED);	//	1077L
		ERRSTR(ERROR_DUPLICATE_SERVICE_NAME);	//	1078L
		ERRSTR(ERROR_DIFFERENT_SERVICE_ACCOUNT);	//	1079L
		ERRSTR(ERROR_CANNOT_DETECT_DRIVER_FAILURE);	//	1080L
		ERRSTR(ERROR_CANNOT_DETECT_PROCESS_ABORT);	//	1081L
		ERRSTR(ERROR_NO_RECOVERY_PROGRAM);	//	1082L
		ERRSTR(ERROR_SERVICE_NOT_IN_EXE);	//	1083L
		ERRSTR(ERROR_NOT_SAFEBOOT_SERVICE);	//	1084L
		ERRSTR(ERROR_END_OF_MEDIA);	//	1100L
		ERRSTR(ERROR_FILEMARK_DETECTED);	//	1101L
		ERRSTR(ERROR_BEGINNING_OF_MEDIA);	//	1102L
		ERRSTR(ERROR_SETMARK_DETECTED);	//	1103L
		ERRSTR(ERROR_NO_DATA_DETECTED);	//	1104L
		ERRSTR(ERROR_PARTITION_FAILURE);	//	1105L
		ERRSTR(ERROR_INVALID_BLOCK_LENGTH);	//	1106L
		ERRSTR(ERROR_DEVICE_NOT_PARTITIONED);	//	1107L
		ERRSTR(ERROR_UNABLE_TO_LOCK_MEDIA);	//	1108L
		ERRSTR(ERROR_UNABLE_TO_UNLOAD_MEDIA);	//	1109L
		ERRSTR(ERROR_MEDIA_CHANGED);	//	1110L
		ERRSTR(ERROR_BUS_RESET);	//	1111L
		ERRSTR(ERROR_NO_MEDIA_IN_DRIVE);	//	1112L
		ERRSTR(ERROR_NO_UNICODE_TRANSLATION);	//	1113L
		ERRSTR(ERROR_DLL_INIT_FAILED);	//	1114L
		ERRSTR(ERROR_SHUTDOWN_IN_PROGRESS);	//	1115L
		ERRSTR(ERROR_NO_SHUTDOWN_IN_PROGRESS);	//	1116L
		ERRSTR(ERROR_IO_DEVICE);	//	1117L
		ERRSTR(ERROR_SERIAL_NO_DEVICE);	//	1118L
		ERRSTR(ERROR_IRQ_BUSY);	//	1119L
		ERRSTR(ERROR_MORE_WRITES);	//	1120L
		ERRSTR(ERROR_COUNTER_TIMEOUT);	//	1121L
		ERRSTR(ERROR_FLOPPY_ID_MARK_NOT_FOUND);	//	1122L
		ERRSTR(ERROR_FLOPPY_WRONG_CYLINDER);	//	1123L
		ERRSTR(ERROR_FLOPPY_UNKNOWN_ERROR);	//	1124L
		ERRSTR(ERROR_FLOPPY_BAD_REGISTERS);	//	1125L
		ERRSTR(ERROR_DISK_RECALIBRATE_FAILED);	//	1126L
		ERRSTR(ERROR_DISK_OPERATION_FAILED);	//	1127L
		ERRSTR(ERROR_DISK_RESET_FAILED);	//	1128L
		ERRSTR(ERROR_EOM_OVERFLOW);	//	1129L
		ERRSTR(ERROR_NOT_ENOUGH_SERVER_MEMORY);	//	1130L
		ERRSTR(ERROR_POSSIBLE_DEADLOCK);	//	1131L
		ERRSTR(ERROR_MAPPED_ALIGNMENT);	//	1132L
		ERRSTR(ERROR_SET_POWER_STATE_VETOED);	//	1140L
		ERRSTR(ERROR_SET_POWER_STATE_FAILED);	//	1141L
		ERRSTR(ERROR_TOO_MANY_LINKS);	//	1142L
		ERRSTR(ERROR_OLD_WIN_VERSION);	//	1150L
		ERRSTR(ERROR_APP_WRONG_OS);	//	1151L
		ERRSTR(ERROR_SINGLE_INSTANCE_APP);	//	1152L
		ERRSTR(ERROR_RMODE_APP);	//	1153L
		ERRSTR(ERROR_INVALID_DLL);	//	1154L
		ERRSTR(ERROR_NO_ASSOCIATION);	//	1155L
		ERRSTR(ERROR_DDE_FAIL);	//	1156L
		ERRSTR(ERROR_DLL_NOT_FOUND);	//	1157L
		ERRSTR(ERROR_NO_MORE_USER_HANDLES);	//	1158L
		ERRSTR(ERROR_MESSAGE_SYNC_ONLY);	//	1159L
		ERRSTR(ERROR_SOURCE_ELEMENT_EMPTY);	//	1160L
		ERRSTR(ERROR_DESTINATION_ELEMENT_FULL);	//	1161L
		ERRSTR(ERROR_ILLEGAL_ELEMENT_ADDRESS);	//	1162L
		ERRSTR(ERROR_MAGAZINE_NOT_PRESENT);	//	1163L
		ERRSTR(ERROR_DEVICE_REINITIALIZATION_NEEDED);	//	1164L    // dderror
		ERRSTR(ERROR_DEVICE_REQUIRES_CLEANING);	//	1165L
		ERRSTR(ERROR_DEVICE_DOOR_OPEN);	//	1166L
		ERRSTR(ERROR_DEVICE_NOT_CONNECTED);	//	1167L
		ERRSTR(ERROR_NOT_FOUND);	//	1168L
		ERRSTR(ERROR_NO_MATCH);	//	1169L
		ERRSTR(ERROR_SET_NOT_FOUND);	//	1170L
		ERRSTR(ERROR_POINT_NOT_FOUND);	//	1171L
		ERRSTR(ERROR_NO_TRACKING_SERVICE);	//	1172L
		ERRSTR(ERROR_NO_VOLUME_ID);	//	1173L
		ERRSTR(ERROR_UNABLE_TO_REMOVE_REPLACED);	//	1175L
		ERRSTR(ERROR_UNABLE_TO_MOVE_REPLACEMENT);	//	1176L
		ERRSTR(ERROR_UNABLE_TO_MOVE_REPLACEMENT_2);	//	1177L
		ERRSTR(ERROR_JOURNAL_DELETE_IN_PROGRESS);	//	1178L
		ERRSTR(ERROR_JOURNAL_NOT_ACTIVE);	//	1179L
		ERRSTR(ERROR_POTENTIAL_FILE_FOUND);	//	1180L
		ERRSTR(ERROR_JOURNAL_ENTRY_DELETED);	//	1181L
		ERRSTR(ERROR_SHUTDOWN_IS_SCHEDULED);	//	1190L
		ERRSTR(ERROR_SHUTDOWN_USERS_LOGGED_ON);	//	1191L
		ERRSTR(ERROR_BAD_DEVICE);	//	1200L
		ERRSTR(ERROR_CONNECTION_UNAVAIL);	//	1201L
		ERRSTR(ERROR_DEVICE_ALREADY_REMEMBERED);	//	1202L
		ERRSTR(ERROR_NO_NET_OR_BAD_PATH);	//	1203L
		ERRSTR(ERROR_BAD_PROVIDER);	//	1204L
		ERRSTR(ERROR_CANNOT_OPEN_PROFILE);	//	1205L
		ERRSTR(ERROR_BAD_PROFILE);	//	1206L
		ERRSTR(ERROR_NOT_CONTAINER);	//	1207L
		ERRSTR(ERROR_EXTENDED_ERROR);	//	1208L
		ERRSTR(ERROR_INVALID_GROUPNAME);	//	1209L
		ERRSTR(ERROR_INVALID_COMPUTERNAME);	//	1210L
		ERRSTR(ERROR_INVALID_EVENTNAME);	//	1211L
		ERRSTR(ERROR_INVALID_DOMAINNAME);	//	1212L
		ERRSTR(ERROR_INVALID_SERVICENAME);	//	1213L
		ERRSTR(ERROR_INVALID_NETNAME);	//	1214L
		ERRSTR(ERROR_INVALID_SHARENAME);	//	1215L
		ERRSTR(ERROR_INVALID_PASSWORDNAME);	//	1216L
		ERRSTR(ERROR_INVALID_MESSAGENAME);	//	1217L
		ERRSTR(ERROR_INVALID_MESSAGEDEST);	//	1218L
		ERRSTR(ERROR_SESSION_CREDENTIAL_CONFLICT);	//	1219L
		ERRSTR(ERROR_REMOTE_SESSION_LIMIT_EXCEEDED);	//	1220L
		ERRSTR(ERROR_DUP_DOMAINNAME);	//	1221L
		ERRSTR(ERROR_NO_NETWORK);	//	1222L
		ERRSTR(ERROR_CANCELLED);	//	1223L
		ERRSTR(ERROR_USER_MAPPED_FILE);	//	1224L
		ERRSTR(ERROR_CONNECTION_REFUSED);	//	1225L
		ERRSTR(ERROR_GRACEFUL_DISCONNECT);	//	1226L
		ERRSTR(ERROR_ADDRESS_ALREADY_ASSOCIATED);	//	1227L
		ERRSTR(ERROR_ADDRESS_NOT_ASSOCIATED);	//	1228L
		ERRSTR(ERROR_CONNECTION_INVALID);	//	1229L
		ERRSTR(ERROR_CONNECTION_ACTIVE);	//	1230L
		ERRSTR(ERROR_NETWORK_UNREACHABLE);	//	1231L
		ERRSTR(ERROR_HOST_UNREACHABLE);	//	1232L
		ERRSTR(ERROR_PROTOCOL_UNREACHABLE);	//	1233L
		ERRSTR(ERROR_PORT_UNREACHABLE);	//	1234L
		ERRSTR(ERROR_REQUEST_ABORTED);	//	1235L
		ERRSTR(ERROR_CONNECTION_ABORTED);	//	1236L
		ERRSTR(ERROR_RETRY);	//	1237L
		ERRSTR(ERROR_CONNECTION_COUNT_LIMIT);	//	1238L
		ERRSTR(ERROR_LOGIN_TIME_RESTRICTION);	//	1239L
		ERRSTR(ERROR_LOGIN_WKSTA_RESTRICTION);	//	1240L
		ERRSTR(ERROR_INCORRECT_ADDRESS);	//	1241L
		ERRSTR(ERROR_ALREADY_REGISTERED);	//	1242L
		ERRSTR(ERROR_SERVICE_NOT_FOUND);	//	1243L
		ERRSTR(ERROR_NOT_AUTHENTICATED);	//	1244L
		ERRSTR(ERROR_NOT_LOGGED_ON);	//	1245L
		ERRSTR(ERROR_CONTINUE);	//	1246L    // dderror
		ERRSTR(ERROR_ALREADY_INITIALIZED);	//	1247L
		ERRSTR(ERROR_NO_MORE_DEVICES);	//	1248L    // dderror
		ERRSTR(ERROR_NO_SUCH_SITE);	//	1249L
		ERRSTR(ERROR_DOMAIN_CONTROLLER_EXISTS);	//	1250L
		ERRSTR(ERROR_ONLY_IF_CONNECTED);	//	1251L
		ERRSTR(ERROR_OVERRIDE_NOCHANGES);	//	1252L
		ERRSTR(ERROR_BAD_USER_PROFILE);	//	1253L
		ERRSTR(ERROR_NOT_SUPPORTED_ON_SBS);	//	1254L
		ERRSTR(ERROR_SERVER_SHUTDOWN_IN_PROGRESS);	//	1255L
		ERRSTR(ERROR_HOST_DOWN);	//	1256L
		ERRSTR(ERROR_NON_ACCOUNT_SID);	//	1257L
		ERRSTR(ERROR_NON_DOMAIN_SID);	//	1258L
		ERRSTR(ERROR_APPHELP_BLOCK);	//	1259L
		ERRSTR(ERROR_ACCESS_DISABLED_BY_POLICY);	//	1260L
		ERRSTR(ERROR_REG_NAT_CONSUMPTION);	//	1261L
		ERRSTR(ERROR_CSCSHARE_OFFLINE);	//	1262L
		ERRSTR(ERROR_PKINIT_FAILURE);	//	1263L
		ERRSTR(ERROR_SMARTCARD_SUBSYSTEM_FAILURE);	//	1264L
		ERRSTR(ERROR_DOWNGRADE_DETECTED);	//	1265L
		ERRSTR(ERROR_MACHINE_LOCKED);	//	1271L
		ERRSTR(ERROR_SMB_GUEST_LOGON_BLOCKED);	//	1272L
		ERRSTR(ERROR_CALLBACK_SUPPLIED_INVALID_DATA);	//	1273L
		ERRSTR(ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED);	//	1274L
		ERRSTR(ERROR_DRIVER_BLOCKED);	//	1275L
		ERRSTR(ERROR_INVALID_IMPORT_OF_NON_DLL);	//	1276L
		ERRSTR(ERROR_ACCESS_DISABLED_WEBBLADE);	//	1277L
		ERRSTR(ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER);	//	1278L
		ERRSTR(ERROR_RECOVERY_FAILURE);	//	1279L
		ERRSTR(ERROR_ALREADY_FIBER);	//	1280L
		ERRSTR(ERROR_ALREADY_THREAD);	//	1281L
		ERRSTR(ERROR_STACK_BUFFER_OVERRUN);	//	1282L
		ERRSTR(ERROR_PARAMETER_QUOTA_EXCEEDED);	//	1283L
		ERRSTR(ERROR_DEBUGGER_INACTIVE);	//	1284L
		ERRSTR(ERROR_DELAY_LOAD_FAILED);	//	1285L
		ERRSTR(ERROR_VDM_DISALLOWED);	//	1286L
		ERRSTR(ERROR_UNIDENTIFIED_ERROR);	//	1287L
		ERRSTR(ERROR_INVALID_CRUNTIME_PARAMETER);	//	1288L
		ERRSTR(ERROR_BEYOND_VDL);	//	1289L
		ERRSTR(ERROR_INCOMPATIBLE_SERVICE_SID_TYPE);	//	1290L
		ERRSTR(ERROR_DRIVER_PROCESS_TERMINATED);	//	1291L
		ERRSTR(ERROR_IMPLEMENTATION_LIMIT);	//	1292L
		ERRSTR(ERROR_PROCESS_IS_PROTECTED);	//	1293L
		ERRSTR(ERROR_SERVICE_NOTIFY_CLIENT_LAGGING);	//	1294L
		ERRSTR(ERROR_DISK_QUOTA_EXCEEDED);	//	1295L
		ERRSTR(ERROR_CONTENT_BLOCKED);	//	1296L
		ERRSTR(ERROR_INCOMPATIBLE_SERVICE_PRIVILEGE);	//	1297L
		ERRSTR(ERROR_APP_HANG);	//	1298L
		ERRSTR(ERROR_INVALID_LABEL);	//	1299L
		ERRSTR(ERROR_NOT_ALL_ASSIGNED);	//	1300L
		ERRSTR(ERROR_SOME_NOT_MAPPED);	//	1301L
		ERRSTR(ERROR_NO_QUOTAS_FOR_ACCOUNT);	//	1302L
		ERRSTR(ERROR_LOCAL_USER_SESSION_KEY);	//	1303L
		ERRSTR(ERROR_NULL_LM_PASSWORD);	//	1304L
		ERRSTR(ERROR_UNKNOWN_REVISION);	//	1305L
		ERRSTR(ERROR_REVISION_MISMATCH);	//	1306L
		ERRSTR(ERROR_INVALID_OWNER);	//	1307L
		ERRSTR(ERROR_INVALID_PRIMARY_GROUP);	//	1308L
		ERRSTR(ERROR_NO_IMPERSONATION_TOKEN);	//	1309L
		ERRSTR(ERROR_CANT_DISABLE_MANDATORY);	//	1310L
		ERRSTR(ERROR_NO_LOGON_SERVERS);	//	1311L
		ERRSTR(ERROR_NO_SUCH_LOGON_SESSION);	//	1312L
		ERRSTR(ERROR_NO_SUCH_PRIVILEGE);	//	1313L
		ERRSTR(ERROR_PRIVILEGE_NOT_HELD);	//	1314L
		ERRSTR(ERROR_INVALID_ACCOUNT_NAME);	//	1315L
		ERRSTR(ERROR_USER_EXISTS);	//	1316L
		ERRSTR(ERROR_NO_SUCH_USER);	//	1317L
		ERRSTR(ERROR_GROUP_EXISTS);	//	1318L
		ERRSTR(ERROR_NO_SUCH_GROUP);	//	1319L
		ERRSTR(ERROR_MEMBER_IN_GROUP);	//	1320L
		ERRSTR(ERROR_MEMBER_NOT_IN_GROUP);	//	1321L
		ERRSTR(ERROR_LAST_ADMIN);	//	1322L
		ERRSTR(ERROR_WRONG_PASSWORD);	//	1323L
		ERRSTR(ERROR_ILL_FORMED_PASSWORD);	//	1324L
		ERRSTR(ERROR_PASSWORD_RESTRICTION);	//	1325L
		ERRSTR(ERROR_LOGON_FAILURE);	//	1326L
		ERRSTR(ERROR_ACCOUNT_RESTRICTION);	//	1327L
		ERRSTR(ERROR_INVALID_LOGON_HOURS);	//	1328L
		ERRSTR(ERROR_INVALID_WORKSTATION);	//	1329L
		ERRSTR(ERROR_PASSWORD_EXPIRED);	//	1330L
		ERRSTR(ERROR_ACCOUNT_DISABLED);	//	1331L
		ERRSTR(ERROR_NONE_MAPPED);	//	1332L
		ERRSTR(ERROR_TOO_MANY_LUIDS_REQUESTED);	//	1333L
		ERRSTR(ERROR_LUIDS_EXHAUSTED);	//	1334L
		ERRSTR(ERROR_INVALID_SUB_AUTHORITY);	//	1335L
		ERRSTR(ERROR_INVALID_ACL);	//	1336L
		ERRSTR(ERROR_INVALID_SID);	//	1337L
		ERRSTR(ERROR_INVALID_SECURITY_DESCR);	//	1338L
		ERRSTR(ERROR_BAD_INHERITANCE_ACL);	//	1340L
		ERRSTR(ERROR_SERVER_DISABLED);	//	1341L
		ERRSTR(ERROR_SERVER_NOT_DISABLED);	//	1342L
		ERRSTR(ERROR_INVALID_ID_AUTHORITY);	//	1343L
		ERRSTR(ERROR_ALLOTTED_SPACE_EXCEEDED);	//	1344L
		ERRSTR(ERROR_INVALID_GROUP_ATTRIBUTES);	//	1345L
		ERRSTR(ERROR_BAD_IMPERSONATION_LEVEL);	//	1346L
		ERRSTR(ERROR_CANT_OPEN_ANONYMOUS);	//	1347L
		ERRSTR(ERROR_BAD_VALIDATION_CLASS);	//	1348L
		ERRSTR(ERROR_BAD_TOKEN_TYPE);	//	1349L
		ERRSTR(ERROR_NO_SECURITY_ON_OBJECT);	//	1350L
		ERRSTR(ERROR_CANT_ACCESS_DOMAIN_INFO);	//	1351L
		ERRSTR(ERROR_INVALID_SERVER_STATE);	//	1352L
		ERRSTR(ERROR_INVALID_DOMAIN_STATE);	//	1353L
		ERRSTR(ERROR_INVALID_DOMAIN_ROLE);	//	1354L
		ERRSTR(ERROR_NO_SUCH_DOMAIN);	//	1355L
		ERRSTR(ERROR_DOMAIN_EXISTS);	//	1356L
		ERRSTR(ERROR_DOMAIN_LIMIT_EXCEEDED);	//	1357L
		ERRSTR(ERROR_INTERNAL_DB_CORRUPTION);	//	1358L
		ERRSTR(ERROR_INTERNAL_ERROR);	//	1359L
		ERRSTR(ERROR_GENERIC_NOT_MAPPED);	//	1360L
		ERRSTR(ERROR_BAD_DESCRIPTOR_FORMAT);	//	1361L
		ERRSTR(ERROR_NOT_LOGON_PROCESS);	//	1362L
		ERRSTR(ERROR_LOGON_SESSION_EXISTS);	//	1363L
		ERRSTR(ERROR_NO_SUCH_PACKAGE);	//	1364L
		ERRSTR(ERROR_BAD_LOGON_SESSION_STATE);	//	1365L
		ERRSTR(ERROR_LOGON_SESSION_COLLISION);	//	1366L
		ERRSTR(ERROR_INVALID_LOGON_TYPE);	//	1367L
		ERRSTR(ERROR_CANNOT_IMPERSONATE);	//	1368L
		ERRSTR(ERROR_RXACT_INVALID_STATE);	//	1369L
		ERRSTR(ERROR_RXACT_COMMIT_FAILURE);	//	1370L
		ERRSTR(ERROR_SPECIAL_ACCOUNT);	//	1371L
		ERRSTR(ERROR_SPECIAL_GROUP);	//	1372L
		ERRSTR(ERROR_SPECIAL_USER);	//	1373L
		ERRSTR(ERROR_MEMBERS_PRIMARY_GROUP);	//	1374L
		ERRSTR(ERROR_TOKEN_ALREADY_IN_USE);	//	1375L
		ERRSTR(ERROR_NO_SUCH_ALIAS);	//	1376L
		ERRSTR(ERROR_MEMBER_NOT_IN_ALIAS);	//	1377L
		ERRSTR(ERROR_MEMBER_IN_ALIAS);	//	1378L
		ERRSTR(ERROR_ALIAS_EXISTS);	//	1379L
		ERRSTR(ERROR_LOGON_NOT_GRANTED);	//	1380L
		ERRSTR(ERROR_TOO_MANY_SECRETS);	//	1381L
		ERRSTR(ERROR_SECRET_TOO_LONG);	//	1382L
		ERRSTR(ERROR_INTERNAL_DB_ERROR);	//	1383L
		ERRSTR(ERROR_TOO_MANY_CONTEXT_IDS);	//	1384L
		ERRSTR(ERROR_LOGON_TYPE_NOT_GRANTED);	//	1385L
		ERRSTR(ERROR_NT_CROSS_ENCRYPTION_REQUIRED);	//	1386L
		ERRSTR(ERROR_NO_SUCH_MEMBER);	//	1387L
		ERRSTR(ERROR_INVALID_MEMBER);	//	1388L
		ERRSTR(ERROR_TOO_MANY_SIDS);	//	1389L
		ERRSTR(ERROR_LM_CROSS_ENCRYPTION_REQUIRED);	//	1390L
		ERRSTR(ERROR_NO_INHERITANCE);	//	1391L
		ERRSTR(ERROR_FILE_CORRUPT);	//	1392L
		ERRSTR(ERROR_DISK_CORRUPT);	//	1393L
		ERRSTR(ERROR_NO_USER_SESSION_KEY);	//	1394L
		ERRSTR(ERROR_LICENSE_QUOTA_EXCEEDED);	//	1395L
		ERRSTR(ERROR_WRONG_TARGET_NAME);	//	1396L
		ERRSTR(ERROR_MUTUAL_AUTH_FAILED);	//	1397L
		ERRSTR(ERROR_TIME_SKEW);	//	1398L
		ERRSTR(ERROR_CURRENT_DOMAIN_NOT_ALLOWED);	//	1399L
		ERRSTR(ERROR_INVALID_WINDOW_HANDLE);	//	1400L
		ERRSTR(ERROR_INVALID_MENU_HANDLE);	//	1401L
		ERRSTR(ERROR_INVALID_CURSOR_HANDLE);	//	1402L
		ERRSTR(ERROR_INVALID_ACCEL_HANDLE);	//	1403L
		ERRSTR(ERROR_INVALID_HOOK_HANDLE);	//	1404L
		ERRSTR(ERROR_INVALID_DWP_HANDLE);	//	1405L
		ERRSTR(ERROR_TLW_WITH_WSCHILD);	//	1406L
		ERRSTR(ERROR_CANNOT_FIND_WND_CLASS);	//	1407L
		ERRSTR(ERROR_WINDOW_OF_OTHER_THREAD);	//	1408L
		ERRSTR(ERROR_HOTKEY_ALREADY_REGISTERED);	//	1409L
		ERRSTR(ERROR_CLASS_ALREADY_EXISTS);	//	1410L
		ERRSTR(ERROR_CLASS_DOES_NOT_EXIST);	//	1411L
		ERRSTR(ERROR_CLASS_HAS_WINDOWS);	//	1412L
		ERRSTR(ERROR_INVALID_INDEX);	//	1413L
		ERRSTR(ERROR_INVALID_ICON_HANDLE);	//	1414L
		ERRSTR(ERROR_PRIVATE_DIALOG_INDEX);	//	1415L
		ERRSTR(ERROR_LISTBOX_ID_NOT_FOUND);	//	1416L
		ERRSTR(ERROR_NO_WILDCARD_CHARACTERS);	//	1417L
		ERRSTR(ERROR_CLIPBOARD_NOT_OPEN);	//	1418L
		ERRSTR(ERROR_HOTKEY_NOT_REGISTERED);	//	1419L
		ERRSTR(ERROR_WINDOW_NOT_DIALOG);	//	1420L
		ERRSTR(ERROR_CONTROL_ID_NOT_FOUND);	//	1421L
		ERRSTR(ERROR_INVALID_COMBOBOX_MESSAGE);	//	1422L
		ERRSTR(ERROR_WINDOW_NOT_COMBOBOX);	//	1423L
		ERRSTR(ERROR_INVALID_EDIT_HEIGHT);	//	1424L
		ERRSTR(ERROR_DC_NOT_FOUND);	//	1425L
		ERRSTR(ERROR_INVALID_HOOK_FILTER);	//	1426L
		ERRSTR(ERROR_INVALID_FILTER_PROC);	//	1427L
		ERRSTR(ERROR_HOOK_NEEDS_HMOD);	//	1428L
		ERRSTR(ERROR_GLOBAL_ONLY_HOOK);	//	1429L
		ERRSTR(ERROR_JOURNAL_HOOK_SET);	//	1430L
		ERRSTR(ERROR_HOOK_NOT_INSTALLED);	//	1431L
		ERRSTR(ERROR_INVALID_LB_MESSAGE);	//	1432L
		ERRSTR(ERROR_SETCOUNT_ON_BAD_LB);	//	1433L
		ERRSTR(ERROR_LB_WITHOUT_TABSTOPS);	//	1434L
		ERRSTR(ERROR_DESTROY_OBJECT_OF_OTHER_THREAD);	//	1435L
		ERRSTR(ERROR_CHILD_WINDOW_MENU);	//	1436L
		ERRSTR(ERROR_NO_SYSTEM_MENU);	//	1437L
		ERRSTR(ERROR_INVALID_MSGBOX_STYLE);	//	1438L
		ERRSTR(ERROR_INVALID_SPI_VALUE);	//	1439L
		ERRSTR(ERROR_SCREEN_ALREADY_LOCKED);	//	1440L
		ERRSTR(ERROR_HWNDS_HAVE_DIFF_PARENT);	//	1441L
		ERRSTR(ERROR_NOT_CHILD_WINDOW);	//	1442L
		ERRSTR(ERROR_INVALID_GW_COMMAND);	//	1443L
		ERRSTR(ERROR_INVALID_THREAD_ID);	//	1444L
		ERRSTR(ERROR_NON_MDICHILD_WINDOW);	//	1445L
		ERRSTR(ERROR_POPUP_ALREADY_ACTIVE);	//	1446L
		ERRSTR(ERROR_NO_SCROLLBARS);	//	1447L
		ERRSTR(ERROR_INVALID_SCROLLBAR_RANGE);	//	1448L
		ERRSTR(ERROR_INVALID_SHOWWIN_COMMAND);	//	1449L
		ERRSTR(ERROR_NO_SYSTEM_RESOURCES);	//	1450L
		ERRSTR(ERROR_NONPAGED_SYSTEM_RESOURCES);	//	1451L
		ERRSTR(ERROR_PAGED_SYSTEM_RESOURCES);	//	1452L
		ERRSTR(ERROR_WORKING_SET_QUOTA);	//	1453L
		ERRSTR(ERROR_PAGEFILE_QUOTA);	//	1454L
		ERRSTR(ERROR_COMMITMENT_LIMIT);	//	1455L
		ERRSTR(ERROR_MENU_ITEM_NOT_FOUND);	//	1456L
		ERRSTR(ERROR_INVALID_KEYBOARD_HANDLE);	//	1457L
		ERRSTR(ERROR_HOOK_TYPE_NOT_ALLOWED);	//	1458L
		ERRSTR(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);	//	1459L
		ERRSTR(ERROR_TIMEOUT);	//	1460L
		ERRSTR(ERROR_INVALID_MONITOR_HANDLE);	//	1461L
		ERRSTR(ERROR_INCORRECT_SIZE);	//	1462L
		ERRSTR(ERROR_SYMLINK_CLASS_DISABLED);	//	1463L
		ERRSTR(ERROR_SYMLINK_NOT_SUPPORTED);	//	1464L
		ERRSTR(ERROR_XML_PARSE_ERROR);	//	1465L
		ERRSTR(ERROR_XMLDSIG_ERROR);	//	1466L
		ERRSTR(ERROR_RESTART_APPLICATION);	//	1467L
		ERRSTR(ERROR_WRONG_COMPARTMENT);	//	1468L
		ERRSTR(ERROR_AUTHIP_FAILURE);	//	1469L
		ERRSTR(ERROR_NO_NVRAM_RESOURCES);	//	1470L
		ERRSTR(ERROR_NOT_GUI_PROCESS);	//	1471L
		ERRSTR(ERROR_EVENTLOG_FILE_CORRUPT);	//	1500L
		ERRSTR(ERROR_EVENTLOG_CANT_START);	//	1501L
		ERRSTR(ERROR_LOG_FILE_FULL);	//	1502L
		ERRSTR(ERROR_EVENTLOG_FILE_CHANGED);	//	1503L
		ERRSTR(ERROR_CONTAINER_ASSIGNED);	//	1504L
		ERRSTR(ERROR_JOB_NO_CONTAINER);	//	1505L
		ERRSTR(ERROR_INVALID_TASK_NAME);	//	1550L
		ERRSTR(ERROR_INVALID_TASK_INDEX);	//	1551L
		ERRSTR(ERROR_THREAD_ALREADY_IN_TASK);	//	1552L
		ERRSTR(ERROR_INSTALL_SERVICE_FAILURE);	//	1601L
		ERRSTR(ERROR_INSTALL_USEREXIT);	//	1602L
		ERRSTR(ERROR_INSTALL_FAILURE);	//	1603L
		ERRSTR(ERROR_INSTALL_SUSPEND);	//	1604L
		ERRSTR(ERROR_UNKNOWN_PRODUCT);	//	1605L
		ERRSTR(ERROR_UNKNOWN_FEATURE);	//	1606L
		ERRSTR(ERROR_UNKNOWN_COMPONENT);	//	1607L
		ERRSTR(ERROR_UNKNOWN_PROPERTY);	//	1608L
		ERRSTR(ERROR_INVALID_HANDLE_STATE);	//	1609L
		ERRSTR(ERROR_BAD_CONFIGURATION);	//	1610L
		ERRSTR(ERROR_INDEX_ABSENT);	//	1611L
		ERRSTR(ERROR_INSTALL_SOURCE_ABSENT);	//	1612L
		ERRSTR(ERROR_INSTALL_PACKAGE_VERSION);	//	1613L
		ERRSTR(ERROR_PRODUCT_UNINSTALLED);	//	1614L
		ERRSTR(ERROR_BAD_QUERY_SYNTAX);	//	1615L
		ERRSTR(ERROR_INVALID_FIELD);	//	1616L
		ERRSTR(ERROR_DEVICE_REMOVED);	//	1617L
		ERRSTR(ERROR_INSTALL_ALREADY_RUNNING);	//	1618L
		ERRSTR(ERROR_INSTALL_PACKAGE_OPEN_FAILED);	//	1619L
		ERRSTR(ERROR_INSTALL_PACKAGE_INVALID);	//	1620L
		ERRSTR(ERROR_INSTALL_UI_FAILURE);	//	1621L
		ERRSTR(ERROR_INSTALL_LOG_FAILURE);	//	1622L
		ERRSTR(ERROR_INSTALL_LANGUAGE_UNSUPPORTED);	//	1623L
		ERRSTR(ERROR_INSTALL_TRANSFORM_FAILURE);	//	1624L
		ERRSTR(ERROR_INSTALL_PACKAGE_REJECTED);	//	1625L
		ERRSTR(ERROR_FUNCTION_NOT_CALLED);	//	1626L
		ERRSTR(ERROR_FUNCTION_FAILED);	//	1627L
		ERRSTR(ERROR_INVALID_TABLE);	//	1628L
		ERRSTR(ERROR_DATATYPE_MISMATCH);	//	1629L
		ERRSTR(ERROR_UNSUPPORTED_TYPE);	//	1630L
		ERRSTR(ERROR_CREATE_FAILED);	//	1631L
		ERRSTR(ERROR_INSTALL_TEMP_UNWRITABLE);	//	1632L
		ERRSTR(ERROR_INSTALL_PLATFORM_UNSUPPORTED);	//	1633L
		ERRSTR(ERROR_INSTALL_NOTUSED);	//	1634L
		ERRSTR(ERROR_PATCH_PACKAGE_OPEN_FAILED);	//	1635L
		ERRSTR(ERROR_PATCH_PACKAGE_INVALID);	//	1636L
		ERRSTR(ERROR_PATCH_PACKAGE_UNSUPPORTED);	//	1637L
		ERRSTR(ERROR_PRODUCT_VERSION);	//	1638L
		ERRSTR(ERROR_INVALID_COMMAND_LINE);	//	1639L
		ERRSTR(ERROR_INSTALL_REMOTE_DISALLOWED);	//	1640L
		ERRSTR(ERROR_SUCCESS_REBOOT_INITIATED);	//	1641L
		ERRSTR(ERROR_PATCH_TARGET_NOT_FOUND);	//	1642L
		ERRSTR(ERROR_PATCH_PACKAGE_REJECTED);	//	1643L
		ERRSTR(ERROR_INSTALL_TRANSFORM_REJECTED);	//	1644L
		ERRSTR(ERROR_INSTALL_REMOTE_PROHIBITED);	//	1645L
		ERRSTR(ERROR_PATCH_REMOVAL_UNSUPPORTED);	//	1646L
		ERRSTR(ERROR_UNKNOWN_PATCH);	//	1647L
		ERRSTR(ERROR_PATCH_NO_SEQUENCE);	//	1648L
		ERRSTR(ERROR_PATCH_REMOVAL_DISALLOWED);	//	1649L
		ERRSTR(ERROR_INVALID_PATCH_XML);	//	1650L
		ERRSTR(ERROR_PATCH_MANAGED_ADVERTISED_PRODUCT);	//	1651L
		ERRSTR(ERROR_INSTALL_SERVICE_SAFEBOOT);	//	1652L
		ERRSTR(ERROR_FAIL_FAST_EXCEPTION);	//	1653L
		ERRSTR(ERROR_INSTALL_REJECTED);	//	1654L
		ERRSTR(ERROR_DYNAMIC_CODE_BLOCKED);	//	1655L
		ERRSTR(ERROR_NOT_SAME_OBJECT);	//	1656L
		ERRSTR(ERROR_STRICT_CFG_VIOLATION);	//	1657L
		ERRSTR(ERROR_SET_CONTEXT_DENIED);	//	1660L
		ERRSTR(ERROR_CROSS_PARTITION_VIOLATION);	//	1661L
		ERRSTR(ERROR_RETURN_ADDRESS_HIJACK_ATTEMPT);	//	1662L
		ERRSTR(RPC_S_INVALID_STRING_BINDING);	//	1700L
		ERRSTR(RPC_S_WRONG_KIND_OF_BINDING);	//	1701L
		ERRSTR(RPC_S_INVALID_BINDING);	//	1702L
		ERRSTR(RPC_S_PROTSEQ_NOT_SUPPORTED);	//	1703L
		ERRSTR(RPC_S_INVALID_RPC_PROTSEQ);	//	1704L
		ERRSTR(RPC_S_INVALID_STRING_UUID);	//	1705L
		ERRSTR(RPC_S_INVALID_ENDPOINT_FORMAT);	//	1706L
		ERRSTR(RPC_S_INVALID_NET_ADDR);	//	1707L
		ERRSTR(RPC_S_NO_ENDPOINT_FOUND);	//	1708L
		ERRSTR(RPC_S_INVALID_TIMEOUT);	//	1709L
		ERRSTR(RPC_S_OBJECT_NOT_FOUND);	//	1710L
		ERRSTR(RPC_S_ALREADY_REGISTERED);	//	1711L
		ERRSTR(RPC_S_TYPE_ALREADY_REGISTERED);	//	1712L
		ERRSTR(RPC_S_ALREADY_LISTENING);	//	1713L
		ERRSTR(RPC_S_NO_PROTSEQS_REGISTERED);	//	1714L
		ERRSTR(RPC_S_NOT_LISTENING);	//	1715L
		ERRSTR(RPC_S_UNKNOWN_MGR_TYPE);	//	1716L
		ERRSTR(RPC_S_UNKNOWN_IF);	//	1717L
		ERRSTR(RPC_S_NO_BINDINGS);	//	1718L
		ERRSTR(RPC_S_NO_PROTSEQS);	//	1719L
		ERRSTR(RPC_S_CANT_CREATE_ENDPOINT);	//	1720L
		ERRSTR(RPC_S_OUT_OF_RESOURCES);	//	1721L
		ERRSTR(RPC_S_SERVER_UNAVAILABLE);	//	1722L
		ERRSTR(RPC_S_SERVER_TOO_BUSY);	//	1723L
		ERRSTR(RPC_S_INVALID_NETWORK_OPTIONS);	//	1724L
		ERRSTR(RPC_S_NO_CALL_ACTIVE);	//	1725L
		ERRSTR(RPC_S_CALL_FAILED);	//	1726L
		ERRSTR(RPC_S_CALL_FAILED_DNE);	//	1727L
		ERRSTR(RPC_S_PROTOCOL_ERROR);	//	1728L
		ERRSTR(RPC_S_PROXY_ACCESS_DENIED);	//	1729L
		ERRSTR(RPC_S_UNSUPPORTED_TRANS_SYN);	//	1730L
		ERRSTR(RPC_S_UNSUPPORTED_TYPE);	//	1732L
		ERRSTR(RPC_S_INVALID_TAG);	//	1733L
		ERRSTR(RPC_S_INVALID_BOUND);	//	1734L
		ERRSTR(RPC_S_NO_ENTRY_NAME);	//	1735L
		ERRSTR(RPC_S_INVALID_NAME_SYNTAX);	//	1736L
		ERRSTR(RPC_S_UNSUPPORTED_NAME_SYNTAX);	//	1737L
		ERRSTR(RPC_S_UUID_NO_ADDRESS);	//	1739L
		ERRSTR(RPC_S_DUPLICATE_ENDPOINT);	//	1740L
		ERRSTR(RPC_S_UNKNOWN_AUTHN_TYPE);	//	1741L
		ERRSTR(RPC_S_MAX_CALLS_TOO_SMALL);	//	1742L
		ERRSTR(RPC_S_STRING_TOO_LONG);	//	1743L
		ERRSTR(RPC_S_PROTSEQ_NOT_FOUND);	//	1744L
		ERRSTR(RPC_S_PROCNUM_OUT_OF_RANGE);	//	1745L
		ERRSTR(RPC_S_BINDING_HAS_NO_AUTH);	//	1746L
		ERRSTR(RPC_S_UNKNOWN_AUTHN_SERVICE);	//	1747L
		ERRSTR(RPC_S_UNKNOWN_AUTHN_LEVEL);	//	1748L
		ERRSTR(RPC_S_INVALID_AUTH_IDENTITY);	//	1749L
		ERRSTR(RPC_S_UNKNOWN_AUTHZ_SERVICE);	//	1750L
		ERRSTR(EPT_S_INVALID_ENTRY);	//	1751L
		ERRSTR(EPT_S_CANT_PERFORM_OP);	//	1752L
		ERRSTR(EPT_S_NOT_REGISTERED);	//	1753L
		ERRSTR(RPC_S_NOTHING_TO_EXPORT);	//	1754L
		ERRSTR(RPC_S_INCOMPLETE_NAME);	//	1755L
		ERRSTR(RPC_S_INVALID_VERS_OPTION);	//	1756L
		ERRSTR(RPC_S_NO_MORE_MEMBERS);	//	1757L
		ERRSTR(RPC_S_NOT_ALL_OBJS_UNEXPORTED);	//	1758L
		ERRSTR(RPC_S_INTERFACE_NOT_FOUND);	//	1759L
		ERRSTR(RPC_S_ENTRY_ALREADY_EXISTS);	//	1760L
		ERRSTR(RPC_S_ENTRY_NOT_FOUND);	//	1761L
		ERRSTR(RPC_S_NAME_SERVICE_UNAVAILABLE);	//	1762L
		ERRSTR(RPC_S_INVALID_NAF_ID);	//	1763L
		ERRSTR(RPC_S_CANNOT_SUPPORT);	//	1764L
		ERRSTR(RPC_S_NO_CONTEXT_AVAILABLE);	//	1765L
		ERRSTR(RPC_S_INTERNAL_ERROR);	//	1766L
		ERRSTR(RPC_S_ZERO_DIVIDE);	//	1767L
		ERRSTR(RPC_S_ADDRESS_ERROR);	//	1768L
		ERRSTR(RPC_S_FP_DIV_ZERO);	//	1769L
		ERRSTR(RPC_S_FP_UNDERFLOW);	//	1770L
		ERRSTR(RPC_S_FP_OVERFLOW);	//	1771L
		ERRSTR(RPC_X_NO_MORE_ENTRIES);	//	1772L
		ERRSTR(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);	//	1773L
		ERRSTR(RPC_X_SS_CHAR_TRANS_SHORT_FILE);	//	1774L
		ERRSTR(RPC_X_SS_IN_NULL_CONTEXT);	//	1775L
		ERRSTR(RPC_X_SS_CONTEXT_DAMAGED);	//	1777L
		ERRSTR(RPC_X_SS_HANDLES_MISMATCH);	//	1778L
		ERRSTR(RPC_X_SS_CANNOT_GET_CALL_HANDLE);	//	1779L
		ERRSTR(RPC_X_NULL_REF_POINTER);	//	1780L
		ERRSTR(RPC_X_ENUM_VALUE_OUT_OF_RANGE);	//	1781L
		ERRSTR(RPC_X_BYTE_COUNT_TOO_SMALL);	//	1782L
		ERRSTR(RPC_X_BAD_STUB_DATA);	//	1783L
		ERRSTR(ERROR_INVALID_USER_BUFFER);	//	1784L
		ERRSTR(ERROR_UNRECOGNIZED_MEDIA);	//	1785L
		ERRSTR(ERROR_NO_TRUST_LSA_SECRET);	//	1786L
		ERRSTR(ERROR_NO_TRUST_SAM_ACCOUNT);	//	1787L
		ERRSTR(ERROR_TRUSTED_DOMAIN_FAILURE);	//	1788L
		ERRSTR(ERROR_TRUSTED_RELATIONSHIP_FAILURE);	//	1789L
		ERRSTR(ERROR_TRUST_FAILURE);	//	1790L
		ERRSTR(RPC_S_CALL_IN_PROGRESS);	//	1791L
		ERRSTR(ERROR_NETLOGON_NOT_STARTED);	//	1792L
		ERRSTR(ERROR_ACCOUNT_EXPIRED);	//	1793L
		ERRSTR(ERROR_REDIRECTOR_HAS_OPEN_HANDLES);	//	1794L
		ERRSTR(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);	//	1795L
		ERRSTR(ERROR_UNKNOWN_PORT);	//	1796L
		ERRSTR(ERROR_UNKNOWN_PRINTER_DRIVER);	//	1797L
		ERRSTR(ERROR_UNKNOWN_PRINTPROCESSOR);	//	1798L
		ERRSTR(ERROR_INVALID_SEPARATOR_FILE);	//	1799L
		ERRSTR(ERROR_INVALID_PRIORITY);	//	1800L
		ERRSTR(ERROR_INVALID_PRINTER_NAME);	//	1801L
		ERRSTR(ERROR_PRINTER_ALREADY_EXISTS);	//	1802L
		ERRSTR(ERROR_INVALID_PRINTER_COMMAND);	//	1803L
		ERRSTR(ERROR_INVALID_DATATYPE);	//	1804L
		ERRSTR(ERROR_INVALID_ENVIRONMENT);	//	1805L
		ERRSTR(RPC_S_NO_MORE_BINDINGS);	//	1806L
		ERRSTR(ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT);	//	1807L
		ERRSTR(ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT);	//	1808L
		ERRSTR(ERROR_NOLOGON_SERVER_TRUST_ACCOUNT);	//	1809L
		ERRSTR(ERROR_DOMAIN_TRUST_INCONSISTENT);	//	1810L
		ERRSTR(ERROR_SERVER_HAS_OPEN_HANDLES);	//	1811L
		ERRSTR(ERROR_RESOURCE_DATA_NOT_FOUND);	//	1812L
		ERRSTR(ERROR_RESOURCE_TYPE_NOT_FOUND);	//	1813L
		ERRSTR(ERROR_RESOURCE_NAME_NOT_FOUND);	//	1814L
		ERRSTR(ERROR_RESOURCE_LANG_NOT_FOUND);	//	1815L
		ERRSTR(ERROR_NOT_ENOUGH_QUOTA);	//	1816L
		ERRSTR(RPC_S_NO_INTERFACES);	//	1817L
		ERRSTR(RPC_S_CALL_CANCELLED);	//	1818L
		ERRSTR(RPC_S_BINDING_INCOMPLETE);	//	1819L
		ERRSTR(RPC_S_COMM_FAILURE);	//	1820L
		ERRSTR(RPC_S_UNSUPPORTED_AUTHN_LEVEL);	//	1821L
		ERRSTR(RPC_S_NO_PRINC_NAME);	//	1822L
		ERRSTR(RPC_S_NOT_RPC_ERROR);	//	1823L
		ERRSTR(RPC_S_UUID_LOCAL_ONLY);	//	1824L
		ERRSTR(RPC_S_SEC_PKG_ERROR);	//	1825L
		ERRSTR(RPC_S_NOT_CANCELLED);	//	1826L
		ERRSTR(RPC_X_INVALID_ES_ACTION);	//	1827L
		ERRSTR(RPC_X_WRONG_ES_VERSION);	//	1828L
		ERRSTR(RPC_X_WRONG_STUB_VERSION);	//	1829L
		ERRSTR(RPC_X_INVALID_PIPE_OBJECT);	//	1830L
		ERRSTR(RPC_X_WRONG_PIPE_ORDER);	//	1831L
		ERRSTR(RPC_X_WRONG_PIPE_VERSION);	//	1832L
		ERRSTR(RPC_S_COOKIE_AUTH_FAILED);	//	1833L
		ERRSTR(RPC_S_DO_NOT_DISTURB);	//	1834L
		ERRSTR(RPC_S_SYSTEM_HANDLE_COUNT_EXCEEDED);	//	1835L
		ERRSTR(RPC_S_SYSTEM_HANDLE_TYPE_MISMATCH);	//	1836L
		ERRSTR(RPC_S_GROUP_MEMBER_NOT_FOUND);	//	1898L
		ERRSTR(EPT_S_CANT_CREATE);	//	1899L
		ERRSTR(RPC_S_INVALID_OBJECT);	//	1900L
		ERRSTR(ERROR_INVALID_TIME);	//	1901L
		ERRSTR(ERROR_INVALID_FORM_NAME);	//	1902L
		ERRSTR(ERROR_INVALID_FORM_SIZE);	//	1903L
		ERRSTR(ERROR_ALREADY_WAITING);	//	1904L
		ERRSTR(ERROR_PRINTER_DELETED);	//	1905L
		ERRSTR(ERROR_INVALID_PRINTER_STATE);	//	1906L
		ERRSTR(ERROR_PASSWORD_MUST_CHANGE);	//	1907L
		ERRSTR(ERROR_DOMAIN_CONTROLLER_NOT_FOUND);	//	1908L
		ERRSTR(ERROR_ACCOUNT_LOCKED_OUT);	//	1909L
		ERRSTR(OR_INVALID_OXID);	//	1910L
		ERRSTR(OR_INVALID_OID);	//	1911L
		ERRSTR(OR_INVALID_SET);	//	1912L
		ERRSTR(RPC_S_SEND_INCOMPLETE);	//	1913L
		ERRSTR(RPC_S_INVALID_ASYNC_HANDLE);	//	1914L
		ERRSTR(RPC_S_INVALID_ASYNC_CALL);	//	1915L
		ERRSTR(RPC_X_PIPE_CLOSED);	//	1916L
		ERRSTR(RPC_X_PIPE_DISCIPLINE_ERROR);	//	1917L
		ERRSTR(RPC_X_PIPE_EMPTY);	//	1918L
		ERRSTR(ERROR_NO_SITENAME);	//	1919L
		ERRSTR(ERROR_CANT_ACCESS_FILE);	//	1920L
		ERRSTR(ERROR_CANT_RESOLVE_FILENAME);	//	1921L
		ERRSTR(RPC_S_ENTRY_TYPE_MISMATCH);	//	1922L
		ERRSTR(RPC_S_NOT_ALL_OBJS_EXPORTED);	//	1923L
		ERRSTR(RPC_S_INTERFACE_NOT_EXPORTED);	//	1924L
		ERRSTR(RPC_S_PROFILE_NOT_ADDED);	//	1925L
		ERRSTR(RPC_S_PRF_ELT_NOT_ADDED);	//	1926L
		ERRSTR(RPC_S_PRF_ELT_NOT_REMOVED);	//	1927L
		ERRSTR(RPC_S_GRP_ELT_NOT_ADDED);	//	1928L
		ERRSTR(RPC_S_GRP_ELT_NOT_REMOVED);	//	1929L
		ERRSTR(ERROR_KM_DRIVER_BLOCKED);	//	1930L
		ERRSTR(ERROR_CONTEXT_EXPIRED);	//	1931L
		ERRSTR(ERROR_PER_USER_TRUST_QUOTA_EXCEEDED);	//	1932L
		ERRSTR(ERROR_ALL_USER_TRUST_QUOTA_EXCEEDED);	//	1933L
		ERRSTR(ERROR_USER_DELETE_TRUST_QUOTA_EXCEEDED);	//	1934L
		ERRSTR(ERROR_AUTHENTICATION_FIREWALL_FAILED);	//	1935L
		ERRSTR(ERROR_REMOTE_PRINT_CONNECTIONS_BLOCKED);	//	1936L
		ERRSTR(ERROR_NTLM_BLOCKED);	//	1937L
		ERRSTR(ERROR_PASSWORD_CHANGE_REQUIRED);	//	1938L
		ERRSTR(ERROR_LOST_MODE_LOGON_RESTRICTION);	//	1939L
		ERRSTR(ERROR_INVALID_PIXEL_FORMAT);	//	2000L
		ERRSTR(ERROR_BAD_DRIVER);	//	2001L
		ERRSTR(ERROR_INVALID_WINDOW_STYLE);	//	2002L
		ERRSTR(ERROR_METAFILE_NOT_SUPPORTED);	//	2003L
		ERRSTR(ERROR_TRANSFORM_NOT_SUPPORTED);	//	2004L
		ERRSTR(ERROR_CLIPPING_NOT_SUPPORTED);	//	2005L
		ERRSTR(ERROR_INVALID_CMM);	//	2010L
		ERRSTR(ERROR_INVALID_PROFILE);	//	2011L
		ERRSTR(ERROR_TAG_NOT_FOUND);	//	2012L
		ERRSTR(ERROR_TAG_NOT_PRESENT);	//	2013L
		ERRSTR(ERROR_DUPLICATE_TAG);	//	2014L
		ERRSTR(ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE);	//	2015L
		ERRSTR(ERROR_PROFILE_NOT_FOUND);	//	2016L
		ERRSTR(ERROR_INVALID_COLORSPACE);	//	2017L
		ERRSTR(ERROR_ICM_NOT_ENABLED);	//	2018L
		ERRSTR(ERROR_DELETING_ICM_XFORM);	//	2019L
		ERRSTR(ERROR_INVALID_TRANSFORM);	//	2020L
		ERRSTR(ERROR_COLORSPACE_MISMATCH);	//	2021L
		ERRSTR(ERROR_INVALID_COLORINDEX);	//	2022L
		ERRSTR(ERROR_PROFILE_DOES_NOT_MATCH_DEVICE);	//	2023L
		ERRSTR(ERROR_CONNECTED_OTHER_PASSWORD);	//	2108L
		ERRSTR(ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT);	//	2109L
		ERRSTR(ERROR_BAD_USERNAME);	//	2202L
		ERRSTR(ERROR_NOT_CONNECTED);	//	2250L
		ERRSTR(ERROR_OPEN_FILES);	//	2401L
		ERRSTR(ERROR_ACTIVE_CONNECTIONS);	//	2402L
		ERRSTR(ERROR_DEVICE_IN_USE);	//	2404L
		ERRSTR(ERROR_UNKNOWN_PRINT_MONITOR);	//	3000L
		ERRSTR(ERROR_PRINTER_DRIVER_IN_USE);	//	3001L
		ERRSTR(ERROR_SPOOL_FILE_NOT_FOUND);	//	3002L
		ERRSTR(ERROR_SPL_NO_STARTDOC);	//	3003L
		ERRSTR(ERROR_SPL_NO_ADDJOB);	//	3004L
		ERRSTR(ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED);	//	3005L
		ERRSTR(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);	//	3006L
		ERRSTR(ERROR_INVALID_PRINT_MONITOR);	//	3007L
		ERRSTR(ERROR_PRINT_MONITOR_IN_USE);	//	3008L
		ERRSTR(ERROR_PRINTER_HAS_JOBS_QUEUED);	//	3009L
		ERRSTR(ERROR_SUCCESS_REBOOT_REQUIRED);	//	3010L
		ERRSTR(ERROR_SUCCESS_RESTART_REQUIRED);	//	3011L
		ERRSTR(ERROR_PRINTER_NOT_FOUND);	//	3012L
		ERRSTR(ERROR_PRINTER_DRIVER_WARNED);	//	3013L
		ERRSTR(ERROR_PRINTER_DRIVER_BLOCKED);	//	3014L
		ERRSTR(ERROR_PRINTER_DRIVER_PACKAGE_IN_USE);	//	3015L
		ERRSTR(ERROR_CORE_DRIVER_PACKAGE_NOT_FOUND);	//	3016L
		ERRSTR(ERROR_FAIL_REBOOT_REQUIRED);	//	3017L
		ERRSTR(ERROR_FAIL_REBOOT_INITIATED);	//	3018L
		ERRSTR(ERROR_PRINTER_DRIVER_DOWNLOAD_NEEDED);	//	3019L
		ERRSTR(ERROR_PRINT_JOB_RESTART_REQUIRED);	//	3020L
		ERRSTR(ERROR_INVALID_PRINTER_DRIVER_MANIFEST);	//	3021L
		ERRSTR(ERROR_PRINTER_NOT_SHAREABLE);	//	3022L
		ERRSTR(ERROR_REQUEST_PAUSED);	//	3050L
		ERRSTR(ERROR_APPEXEC_CONDITION_NOT_SATISFIED);	//	3060L
		ERRSTR(ERROR_APPEXEC_HANDLE_INVALIDATED);	//	3061L
		ERRSTR(ERROR_APPEXEC_INVALID_HOST_GENERATION);	//	3062L
		ERRSTR(ERROR_APPEXEC_UNEXPECTED_PROCESS_REGISTRATION);	//	3063L
		ERRSTR(ERROR_APPEXEC_INVALID_HOST_STATE);	//	3064L
		ERRSTR(ERROR_APPEXEC_NO_DONOR);	//	3065L
		ERRSTR(ERROR_APPEXEC_HOST_ID_MISMATCH);	//	3066L
		ERRSTR(ERROR_APPEXEC_UNKNOWN_USER);	//	3067L
		ERRSTR(ERROR_IO_REISSUE_AS_CACHED);	//	3950L
		ERRSTR(ERROR_WINS_INTERNAL);	//	4000L
		ERRSTR(ERROR_CAN_NOT_DEL_LOCAL_WINS);	//	4001L
		ERRSTR(ERROR_STATIC_INIT);	//	4002L
		ERRSTR(ERROR_INC_BACKUP);	//	4003L
		ERRSTR(ERROR_FULL_BACKUP);	//	4004L
		ERRSTR(ERROR_REC_NON_EXISTENT);	//	4005L
		ERRSTR(ERROR_RPL_NOT_ALLOWED);	//	4006L
		ERRSTR(PEERDIST_ERROR_CONTENTINFO_VERSION_UNSUPPORTED);	//	4050L
		ERRSTR(PEERDIST_ERROR_CANNOT_PARSE_CONTENTINFO);	//	4051L
		ERRSTR(PEERDIST_ERROR_MISSING_DATA);	//	4052L
		ERRSTR(PEERDIST_ERROR_NO_MORE);	//	4053L
		ERRSTR(PEERDIST_ERROR_NOT_INITIALIZED);	//	4054L
		ERRSTR(PEERDIST_ERROR_ALREADY_INITIALIZED);	//	4055L
		ERRSTR(PEERDIST_ERROR_SHUTDOWN_IN_PROGRESS);	//	4056L
		ERRSTR(PEERDIST_ERROR_INVALIDATED);	//	4057L
		ERRSTR(PEERDIST_ERROR_ALREADY_EXISTS);	//	4058L
		ERRSTR(PEERDIST_ERROR_OPERATION_NOTFOUND);	//	4059L
		ERRSTR(PEERDIST_ERROR_ALREADY_COMPLETED);	//	4060L
		ERRSTR(PEERDIST_ERROR_OUT_OF_BOUNDS);	//	4061L
		ERRSTR(PEERDIST_ERROR_VERSION_UNSUPPORTED);	//	4062L
		ERRSTR(PEERDIST_ERROR_INVALID_CONFIGURATION);	//	4063L
		ERRSTR(PEERDIST_ERROR_NOT_LICENSED);	//	4064L
		ERRSTR(PEERDIST_ERROR_SERVICE_UNAVAILABLE);	//	4065L
		ERRSTR(PEERDIST_ERROR_TRUST_FAILURE);	//	4066L
		ERRSTR(ERROR_DHCP_ADDRESS_CONFLICT);	//	4100L
		ERRSTR(ERROR_WMI_GUID_NOT_FOUND);	//	4200L
		ERRSTR(ERROR_WMI_INSTANCE_NOT_FOUND);	//	4201L
		ERRSTR(ERROR_WMI_ITEMID_NOT_FOUND);	//	4202L
		ERRSTR(ERROR_WMI_TRY_AGAIN);	//	4203L
		ERRSTR(ERROR_WMI_DP_NOT_FOUND);	//	4204L
		ERRSTR(ERROR_WMI_UNRESOLVED_INSTANCE_REF);	//	4205L
		ERRSTR(ERROR_WMI_ALREADY_ENABLED);	//	4206L
		ERRSTR(ERROR_WMI_GUID_DISCONNECTED);	//	4207L
		ERRSTR(ERROR_WMI_SERVER_UNAVAILABLE);	//	4208L
		ERRSTR(ERROR_WMI_DP_FAILED);	//	4209L
		ERRSTR(ERROR_WMI_INVALID_MOF);	//	4210L
		ERRSTR(ERROR_WMI_INVALID_REGINFO);	//	4211L
		ERRSTR(ERROR_WMI_ALREADY_DISABLED);	//	4212L
		ERRSTR(ERROR_WMI_READ_ONLY);	//	4213L
		ERRSTR(ERROR_WMI_SET_FAILURE);	//	4214L
		ERRSTR(ERROR_NOT_APPCONTAINER);	//	4250L
		ERRSTR(ERROR_APPCONTAINER_REQUIRED);	//	4251L
		ERRSTR(ERROR_NOT_SUPPORTED_IN_APPCONTAINER);	//	4252L
		ERRSTR(ERROR_INVALID_PACKAGE_SID_LENGTH);	//	4253L
		ERRSTR(ERROR_INVALID_MEDIA);	//	4300L
		ERRSTR(ERROR_INVALID_LIBRARY);	//	4301L
		ERRSTR(ERROR_INVALID_MEDIA_POOL);	//	4302L
		ERRSTR(ERROR_DRIVE_MEDIA_MISMATCH);	//	4303L
		ERRSTR(ERROR_MEDIA_OFFLINE);	//	4304L
		ERRSTR(ERROR_LIBRARY_OFFLINE);	//	4305L
		ERRSTR(ERROR_EMPTY);	//	4306L
		ERRSTR(ERROR_NOT_EMPTY);	//	4307L
		ERRSTR(ERROR_MEDIA_UNAVAILABLE);	//	4308L
		ERRSTR(ERROR_RESOURCE_DISABLED);	//	4309L
		ERRSTR(ERROR_INVALID_CLEANER);	//	4310L
		ERRSTR(ERROR_UNABLE_TO_CLEAN);	//	4311L
		ERRSTR(ERROR_OBJECT_NOT_FOUND);	//	4312L
		ERRSTR(ERROR_DATABASE_FAILURE);	//	4313L
		ERRSTR(ERROR_DATABASE_FULL);	//	4314L
		ERRSTR(ERROR_MEDIA_INCOMPATIBLE);	//	4315L
		ERRSTR(ERROR_RESOURCE_NOT_PRESENT);	//	4316L
		ERRSTR(ERROR_INVALID_OPERATION);	//	4317L
		ERRSTR(ERROR_MEDIA_NOT_AVAILABLE);	//	4318L
		ERRSTR(ERROR_DEVICE_NOT_AVAILABLE);	//	4319L
		ERRSTR(ERROR_REQUEST_REFUSED);	//	4320L
		ERRSTR(ERROR_INVALID_DRIVE_OBJECT);	//	4321L
		ERRSTR(ERROR_LIBRARY_FULL);	//	4322L
		ERRSTR(ERROR_MEDIUM_NOT_ACCESSIBLE);	//	4323L
		ERRSTR(ERROR_UNABLE_TO_LOAD_MEDIUM);	//	4324L
		ERRSTR(ERROR_UNABLE_TO_INVENTORY_DRIVE);	//	4325L
		ERRSTR(ERROR_UNABLE_TO_INVENTORY_SLOT);	//	4326L
		ERRSTR(ERROR_UNABLE_TO_INVENTORY_TRANSPORT);	//	4327L
		ERRSTR(ERROR_TRANSPORT_FULL);	//	4328L
		ERRSTR(ERROR_CONTROLLING_IEPORT);	//	4329L
		ERRSTR(ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA);	//	4330L
		ERRSTR(ERROR_CLEANER_SLOT_SET);	//	4331L
		ERRSTR(ERROR_CLEANER_SLOT_NOT_SET);	//	4332L
		ERRSTR(ERROR_CLEANER_CARTRIDGE_SPENT);	//	4333L
		ERRSTR(ERROR_UNEXPECTED_OMID);	//	4334L
		ERRSTR(ERROR_CANT_DELETE_LAST_ITEM);	//	4335L
		ERRSTR(ERROR_MESSAGE_EXCEEDS_MAX_SIZE);	//	4336L
		ERRSTR(ERROR_VOLUME_CONTAINS_SYS_FILES);	//	4337L
		ERRSTR(ERROR_INDIGENOUS_TYPE);	//	4338L
		ERRSTR(ERROR_NO_SUPPORTING_DRIVES);	//	4339L
		ERRSTR(ERROR_CLEANER_CARTRIDGE_INSTALLED);	//	4340L
		ERRSTR(ERROR_IEPORT_FULL);	//	4341L
		ERRSTR(ERROR_FILE_OFFLINE);	//	4350L
		ERRSTR(ERROR_REMOTE_STORAGE_NOT_ACTIVE);	//	4351L
		ERRSTR(ERROR_REMOTE_STORAGE_MEDIA_ERROR);	//	4352L
		ERRSTR(ERROR_NOT_A_REPARSE_POINT);	//	4390L
		ERRSTR(ERROR_REPARSE_ATTRIBUTE_CONFLICT);	//	4391L
		ERRSTR(ERROR_INVALID_REPARSE_DATA);	//	4392L
		ERRSTR(ERROR_REPARSE_TAG_INVALID);	//	4393L
		ERRSTR(ERROR_REPARSE_TAG_MISMATCH);	//	4394L
		ERRSTR(ERROR_REPARSE_POINT_ENCOUNTERED);	//	4395L
		ERRSTR(ERROR_APP_DATA_NOT_FOUND);	//	4400L
		ERRSTR(ERROR_APP_DATA_EXPIRED);	//	4401L
		ERRSTR(ERROR_APP_DATA_CORRUPT);	//	4402L
		ERRSTR(ERROR_APP_DATA_LIMIT_EXCEEDED);	//	4403L
		ERRSTR(ERROR_APP_DATA_REBOOT_REQUIRED);	//	4404L
		ERRSTR(ERROR_SECUREBOOT_ROLLBACK_DETECTED);	//	4420L
		ERRSTR(ERROR_SECUREBOOT_POLICY_VIOLATION);	//	4421L
		ERRSTR(ERROR_SECUREBOOT_INVALID_POLICY);	//	4422L
		ERRSTR(ERROR_SECUREBOOT_POLICY_PUBLISHER_NOT_FOUND);	//	4423L
		ERRSTR(ERROR_SECUREBOOT_POLICY_NOT_SIGNED);	//	4424L
		ERRSTR(ERROR_SECUREBOOT_NOT_ENABLED);	//	4425L
		ERRSTR(ERROR_SECUREBOOT_FILE_REPLACED);	//	4426L
		ERRSTR(ERROR_SECUREBOOT_POLICY_NOT_AUTHORIZED);	//	4427L
		ERRSTR(ERROR_SECUREBOOT_POLICY_UNKNOWN);	//	4428L
		ERRSTR(ERROR_SECUREBOOT_POLICY_MISSING_ANTIROLLBACKVERSION);	//	4429L
		ERRSTR(ERROR_SECUREBOOT_PLATFORM_ID_MISMATCH);	//	4430L
		ERRSTR(ERROR_SECUREBOOT_POLICY_ROLLBACK_DETECTED);	//	4431L
		ERRSTR(ERROR_SECUREBOOT_POLICY_UPGRADE_MISMATCH);	//	4432L
		ERRSTR(ERROR_SECUREBOOT_REQUIRED_POLICY_FILE_MISSING);	//	4433L
		ERRSTR(ERROR_SECUREBOOT_NOT_BASE_POLICY);	//	4434L
		ERRSTR(ERROR_SECUREBOOT_NOT_SUPPLEMENTAL_POLICY);	//	4435L
		ERRSTR(ERROR_OFFLOAD_READ_FLT_NOT_SUPPORTED);	//	4440L
		ERRSTR(ERROR_OFFLOAD_WRITE_FLT_NOT_SUPPORTED);	//	4441L
		ERRSTR(ERROR_OFFLOAD_READ_FILE_NOT_SUPPORTED);	//	4442L
		ERRSTR(ERROR_OFFLOAD_WRITE_FILE_NOT_SUPPORTED);	//	4443L
		ERRSTR(ERROR_ALREADY_HAS_STREAM_ID);	//	4444L
		ERRSTR(ERROR_SMR_GARBAGE_COLLECTION_REQUIRED);	//	4445L
		ERRSTR(ERROR_WOF_WIM_HEADER_CORRUPT);	//	4446L
		ERRSTR(ERROR_WOF_WIM_RESOURCE_TABLE_CORRUPT);	//	4447L
		ERRSTR(ERROR_WOF_FILE_RESOURCE_TABLE_CORRUPT);	//	4448L
		ERRSTR(ERROR_VOLUME_NOT_SIS_ENABLED);	//	4500L
		ERRSTR(ERROR_SYSTEM_INTEGRITY_ROLLBACK_DETECTED);	//	4550L
		ERRSTR(ERROR_SYSTEM_INTEGRITY_POLICY_VIOLATION);	//	4551L
		ERRSTR(ERROR_SYSTEM_INTEGRITY_INVALID_POLICY);	//	4552L
		ERRSTR(ERROR_SYSTEM_INTEGRITY_POLICY_NOT_SIGNED);	//	4553L
		ERRSTR(ERROR_SYSTEM_INTEGRITY_TOO_MANY_POLICIES);	//	4554L
		ERRSTR(ERROR_SYSTEM_INTEGRITY_SUPPLEMENTAL_POLICY_NOT_AUTHORIZED);	//	4555L
		ERRSTR(ERROR_VSM_NOT_INITIALIZED);	//	4560L
		ERRSTR(ERROR_VSM_DMA_PROTECTION_NOT_IN_USE);	//	4561L
		ERRSTR(ERROR_PLATFORM_MANIFEST_NOT_AUTHORIZED);	//	4570L
		ERRSTR(ERROR_PLATFORM_MANIFEST_INVALID);	//	4571L
		ERRSTR(ERROR_PLATFORM_MANIFEST_FILE_NOT_AUTHORIZED);	//	4572L
		ERRSTR(ERROR_PLATFORM_MANIFEST_CATALOG_NOT_AUTHORIZED);	//	4573L
		ERRSTR(ERROR_PLATFORM_MANIFEST_BINARY_ID_NOT_FOUND);	//	4574L
		ERRSTR(ERROR_PLATFORM_MANIFEST_NOT_ACTIVE);	//	4575L
		ERRSTR(ERROR_PLATFORM_MANIFEST_NOT_SIGNED);	//	4576L
		ERRSTR(ERROR_DEPENDENT_RESOURCE_EXISTS);	//	5001L
		ERRSTR(ERROR_DEPENDENCY_NOT_FOUND);	//	5002L
		ERRSTR(ERROR_DEPENDENCY_ALREADY_EXISTS);	//	5003L
		ERRSTR(ERROR_RESOURCE_NOT_ONLINE);	//	5004L
		ERRSTR(ERROR_HOST_NODE_NOT_AVAILABLE);	//	5005L
		ERRSTR(ERROR_RESOURCE_NOT_AVAILABLE);	//	5006L
		ERRSTR(ERROR_RESOURCE_NOT_FOUND);	//	5007L
		ERRSTR(ERROR_SHUTDOWN_CLUSTER);	//	5008L
		ERRSTR(ERROR_CANT_EVICT_ACTIVE_NODE);	//	5009L
		ERRSTR(ERROR_OBJECT_ALREADY_EXISTS);	//	5010L
		ERRSTR(ERROR_OBJECT_IN_LIST);	//	5011L
		ERRSTR(ERROR_GROUP_NOT_AVAILABLE);	//	5012L
		ERRSTR(ERROR_GROUP_NOT_FOUND);	//	5013L
		ERRSTR(ERROR_GROUP_NOT_ONLINE);	//	5014L
		ERRSTR(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);	//	5015L
		ERRSTR(ERROR_HOST_NODE_NOT_GROUP_OWNER);	//	5016L
		ERRSTR(ERROR_RESMON_CREATE_FAILED);	//	5017L
		ERRSTR(ERROR_RESMON_ONLINE_FAILED);	//	5018L
		ERRSTR(ERROR_RESOURCE_ONLINE);	//	5019L
		ERRSTR(ERROR_QUORUM_RESOURCE);	//	5020L
		ERRSTR(ERROR_NOT_QUORUM_CAPABLE);	//	5021L
		ERRSTR(ERROR_CLUSTER_SHUTTING_DOWN);	//	5022L
		ERRSTR(ERROR_INVALID_STATE);	//	5023L
		ERRSTR(ERROR_RESOURCE_PROPERTIES_STORED);	//	5024L
		ERRSTR(ERROR_NOT_QUORUM_CLASS);	//	5025L
		ERRSTR(ERROR_CORE_RESOURCE);	//	5026L
		ERRSTR(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);	//	5027L
		ERRSTR(ERROR_QUORUMLOG_OPEN_FAILED);	//	5028L
		ERRSTR(ERROR_CLUSTERLOG_CORRUPT);	//	5029L
		ERRSTR(ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE);	//	5030L
		ERRSTR(ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE);	//	5031L
		ERRSTR(ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND);	//	5032L
		ERRSTR(ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE);	//	5033L
		ERRSTR(ERROR_QUORUM_OWNER_ALIVE);	//	5034L
		ERRSTR(ERROR_NETWORK_NOT_AVAILABLE);	//	5035L
		ERRSTR(ERROR_NODE_NOT_AVAILABLE);	//	5036L
		ERRSTR(ERROR_ALL_NODES_NOT_AVAILABLE);	//	5037L
		ERRSTR(ERROR_RESOURCE_FAILED);	//	5038L
		ERRSTR(ERROR_CLUSTER_INVALID_NODE);	//	5039L
		ERRSTR(ERROR_CLUSTER_NODE_EXISTS);	//	5040L
		ERRSTR(ERROR_CLUSTER_JOIN_IN_PROGRESS);	//	5041L
		ERRSTR(ERROR_CLUSTER_NODE_NOT_FOUND);	//	5042L
		ERRSTR(ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND);	//	5043L
		ERRSTR(ERROR_CLUSTER_NETWORK_EXISTS);	//	5044L
		ERRSTR(ERROR_CLUSTER_NETWORK_NOT_FOUND);	//	5045L
		ERRSTR(ERROR_CLUSTER_NETINTERFACE_EXISTS);	//	5046L
		ERRSTR(ERROR_CLUSTER_NETINTERFACE_NOT_FOUND);	//	5047L
		ERRSTR(ERROR_CLUSTER_INVALID_REQUEST);	//	5048L
		ERRSTR(ERROR_CLUSTER_INVALID_NETWORK_PROVIDER);	//	5049L
		ERRSTR(ERROR_CLUSTER_NODE_DOWN);	//	5050L
		ERRSTR(ERROR_CLUSTER_NODE_UNREACHABLE);	//	5051L
		ERRSTR(ERROR_CLUSTER_NODE_NOT_MEMBER);	//	5052L
		ERRSTR(ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS);	//	5053L
		ERRSTR(ERROR_CLUSTER_INVALID_NETWORK);	//	5054L
		ERRSTR(ERROR_CLUSTER_NODE_UP);	//	5056L
		ERRSTR(ERROR_CLUSTER_IPADDR_IN_USE);	//	5057L
		ERRSTR(ERROR_CLUSTER_NODE_NOT_PAUSED);	//	5058L
		ERRSTR(ERROR_CLUSTER_NO_SECURITY_CONTEXT);	//	5059L
		ERRSTR(ERROR_CLUSTER_NETWORK_NOT_INTERNAL);	//	5060L
		ERRSTR(ERROR_CLUSTER_NODE_ALREADY_UP);	//	5061L
		ERRSTR(ERROR_CLUSTER_NODE_ALREADY_DOWN);	//	5062L
		ERRSTR(ERROR_CLUSTER_NETWORK_ALREADY_ONLINE);	//	5063L
		ERRSTR(ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE);	//	5064L
		ERRSTR(ERROR_CLUSTER_NODE_ALREADY_MEMBER);	//	5065L
		ERRSTR(ERROR_CLUSTER_LAST_INTERNAL_NETWORK);	//	5066L
		ERRSTR(ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS);	//	5067L
		ERRSTR(ERROR_INVALID_OPERATION_ON_QUORUM);	//	5068L
		ERRSTR(ERROR_DEPENDENCY_NOT_ALLOWED);	//	5069L
		ERRSTR(ERROR_CLUSTER_NODE_PAUSED);	//	5070L
		ERRSTR(ERROR_NODE_CANT_HOST_RESOURCE);	//	5071L
		ERRSTR(ERROR_CLUSTER_NODE_NOT_READY);	//	5072L
		ERRSTR(ERROR_CLUSTER_NODE_SHUTTING_DOWN);	//	5073L
		ERRSTR(ERROR_CLUSTER_JOIN_ABORTED);	//	5074L
		ERRSTR(ERROR_CLUSTER_INCOMPATIBLE_VERSIONS);	//	5075L
		ERRSTR(ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED);	//	5076L
		ERRSTR(ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED);	//	5077L
		ERRSTR(ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND);	//	5078L
		ERRSTR(ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED);	//	5079L
		ERRSTR(ERROR_CLUSTER_RESNAME_NOT_FOUND);	//	5080L
		ERRSTR(ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED);	//	5081L
		ERRSTR(ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST);	//	5082L
		ERRSTR(ERROR_CLUSTER_DATABASE_SEQMISMATCH);	//	5083L
		ERRSTR(ERROR_RESMON_INVALID_STATE);	//	5084L
		ERRSTR(ERROR_CLUSTER_GUM_NOT_LOCKER);	//	5085L
		ERRSTR(ERROR_QUORUM_DISK_NOT_FOUND);	//	5086L
		ERRSTR(ERROR_DATABASE_BACKUP_CORRUPT);	//	5087L
		ERRSTR(ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT);	//	5088L
		ERRSTR(ERROR_RESOURCE_PROPERTY_UNCHANGEABLE);	//	5089L
		ERRSTR(ERROR_NO_ADMIN_ACCESS_POINT);	//	5090L
		ERRSTR(ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE);	//	5890L
		ERRSTR(ERROR_CLUSTER_QUORUMLOG_NOT_FOUND);	//	5891L
		ERRSTR(ERROR_CLUSTER_MEMBERSHIP_HALT);	//	5892L
		ERRSTR(ERROR_CLUSTER_INSTANCE_ID_MISMATCH);	//	5893L
		ERRSTR(ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP);	//	5894L
		ERRSTR(ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH);	//	5895L
		ERRSTR(ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP);	//	5896L
		ERRSTR(ERROR_CLUSTER_PARAMETER_MISMATCH);	//	5897L
		ERRSTR(ERROR_NODE_CANNOT_BE_CLUSTERED);	//	5898L
		ERRSTR(ERROR_CLUSTER_WRONG_OS_VERSION);	//	5899L
		ERRSTR(ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME);	//	5900L
		ERRSTR(ERROR_CLUSCFG_ALREADY_COMMITTED);	//	5901L
		ERRSTR(ERROR_CLUSCFG_ROLLBACK_FAILED);	//	5902L
		ERRSTR(ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT);	//	5903L
		ERRSTR(ERROR_CLUSTER_OLD_VERSION);	//	5904L
		ERRSTR(ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME);	//	5905L
		ERRSTR(ERROR_CLUSTER_NO_NET_ADAPTERS);	//	5906L
		ERRSTR(ERROR_CLUSTER_POISONED);	//	5907L
		ERRSTR(ERROR_CLUSTER_GROUP_MOVING);	//	5908L
		ERRSTR(ERROR_CLUSTER_RESOURCE_TYPE_BUSY);	//	5909L
		ERRSTR(ERROR_RESOURCE_CALL_TIMED_OUT);	//	5910L
		ERRSTR(ERROR_INVALID_CLUSTER_IPV6_ADDRESS);	//	5911L
		ERRSTR(ERROR_CLUSTER_INTERNAL_INVALID_FUNCTION);	//	5912L
		ERRSTR(ERROR_CLUSTER_PARAMETER_OUT_OF_BOUNDS);	//	5913L
		ERRSTR(ERROR_CLUSTER_PARTIAL_SEND);	//	5914L
		ERRSTR(ERROR_CLUSTER_REGISTRY_INVALID_FUNCTION);	//	5915L
		ERRSTR(ERROR_CLUSTER_INVALID_STRING_TERMINATION);	//	5916L
		ERRSTR(ERROR_CLUSTER_INVALID_STRING_FORMAT);	//	5917L
		ERRSTR(ERROR_CLUSTER_DATABASE_TRANSACTION_IN_PROGRESS);	//	5918L
		ERRSTR(ERROR_CLUSTER_DATABASE_TRANSACTION_NOT_IN_PROGRESS);	//	5919L
		ERRSTR(ERROR_CLUSTER_NULL_DATA);	//	5920L
		ERRSTR(ERROR_CLUSTER_PARTIAL_READ);	//	5921L
		ERRSTR(ERROR_CLUSTER_PARTIAL_WRITE);	//	5922L
		ERRSTR(ERROR_CLUSTER_CANT_DESERIALIZE_DATA);	//	5923L
		ERRSTR(ERROR_DEPENDENT_RESOURCE_PROPERTY_CONFLICT);	//	5924L
		ERRSTR(ERROR_CLUSTER_NO_QUORUM);	//	5925L
		ERRSTR(ERROR_CLUSTER_INVALID_IPV6_NETWORK);	//	5926L
		ERRSTR(ERROR_CLUSTER_INVALID_IPV6_TUNNEL_NETWORK);	//	5927L
		ERRSTR(ERROR_QUORUM_NOT_ALLOWED_IN_THIS_GROUP);	//	5928L
		ERRSTR(ERROR_DEPENDENCY_TREE_TOO_COMPLEX);	//	5929L
		ERRSTR(ERROR_EXCEPTION_IN_RESOURCE_CALL);	//	5930L
		ERRSTR(ERROR_CLUSTER_RHS_FAILED_INITIALIZATION);	//	5931L
		ERRSTR(ERROR_CLUSTER_NOT_INSTALLED);	//	5932L
		ERRSTR(ERROR_CLUSTER_RESOURCES_MUST_BE_ONLINE_ON_THE_SAME_NODE);	//	5933L
		ERRSTR(ERROR_CLUSTER_MAX_NODES_IN_CLUSTER);	//	5934L
		ERRSTR(ERROR_CLUSTER_TOO_MANY_NODES);	//	5935L
		ERRSTR(ERROR_CLUSTER_OBJECT_ALREADY_USED);	//	5936L
		ERRSTR(ERROR_NONCORE_GROUPS_FOUND);	//	5937L
		ERRSTR(ERROR_FILE_SHARE_RESOURCE_CONFLICT);	//	5938L
		ERRSTR(ERROR_CLUSTER_EVICT_INVALID_REQUEST);	//	5939L
		ERRSTR(ERROR_CLUSTER_SINGLETON_RESOURCE);	//	5940L
		ERRSTR(ERROR_CLUSTER_GROUP_SINGLETON_RESOURCE);	//	5941L
		ERRSTR(ERROR_CLUSTER_RESOURCE_PROVIDER_FAILED);	//	5942L
		ERRSTR(ERROR_CLUSTER_RESOURCE_CONFIGURATION_ERROR);	//	5943L
		ERRSTR(ERROR_CLUSTER_GROUP_BUSY);	//	5944L
		ERRSTR(ERROR_CLUSTER_NOT_SHARED_VOLUME);	//	5945L
		ERRSTR(ERROR_CLUSTER_INVALID_SECURITY_DESCRIPTOR);	//	5946L
		ERRSTR(ERROR_CLUSTER_SHARED_VOLUMES_IN_USE);	//	5947L
		ERRSTR(ERROR_CLUSTER_USE_SHARED_VOLUMES_API);	//	5948L
		ERRSTR(ERROR_CLUSTER_BACKUP_IN_PROGRESS);	//	5949L
		ERRSTR(ERROR_NON_CSV_PATH);	//	5950L
		ERRSTR(ERROR_CSV_VOLUME_NOT_LOCAL);	//	5951L
		ERRSTR(ERROR_CLUSTER_WATCHDOG_TERMINATING);	//	5952L
		ERRSTR(ERROR_CLUSTER_RESOURCE_VETOED_MOVE_INCOMPATIBLE_NODES);	//	5953L
		ERRSTR(ERROR_CLUSTER_INVALID_NODE_WEIGHT);	//	5954L
		ERRSTR(ERROR_CLUSTER_RESOURCE_VETOED_CALL);	//	5955L
		ERRSTR(ERROR_RESMON_SYSTEM_RESOURCES_LACKING);	//	5956L
		ERRSTR(ERROR_CLUSTER_RESOURCE_VETOED_MOVE_NOT_ENOUGH_RESOURCES_ON_DESTINATION);	//	5957L
		ERRSTR(ERROR_CLUSTER_RESOURCE_VETOED_MOVE_NOT_ENOUGH_RESOURCES_ON_SOURCE);	//	5958L
		ERRSTR(ERROR_CLUSTER_GROUP_QUEUED);	//	5959L
		ERRSTR(ERROR_CLUSTER_RESOURCE_LOCKED_STATUS);	//	5960L
		ERRSTR(ERROR_CLUSTER_SHARED_VOLUME_FAILOVER_NOT_ALLOWED);	//	5961L
		ERRSTR(ERROR_CLUSTER_NODE_DRAIN_IN_PROGRESS);	//	5962L
		ERRSTR(ERROR_CLUSTER_DISK_NOT_CONNECTED);	//	5963L
		ERRSTR(ERROR_DISK_NOT_CSV_CAPABLE);	//	5964L
		ERRSTR(ERROR_RESOURCE_NOT_IN_AVAILABLE_STORAGE);	//	5965L
		ERRSTR(ERROR_CLUSTER_SHARED_VOLUME_REDIRECTED);	//	5966L
		ERRSTR(ERROR_CLUSTER_SHARED_VOLUME_NOT_REDIRECTED);	//	5967L
		ERRSTR(ERROR_CLUSTER_CANNOT_RETURN_PROPERTIES);	//	5968L
		ERRSTR(ERROR_CLUSTER_RESOURCE_CONTAINS_UNSUPPORTED_DIFF_AREA_FOR_SHARED_VOLUMES);	//	5969L
		ERRSTR(ERROR_CLUSTER_RESOURCE_IS_IN_MAINTENANCE_MODE);	//	5970L
		ERRSTR(ERROR_CLUSTER_AFFINITY_CONFLICT);	//	5971L
		ERRSTR(ERROR_CLUSTER_RESOURCE_IS_REPLICA_VIRTUAL_MACHINE);	//	5972L
		ERRSTR(ERROR_CLUSTER_UPGRADE_INCOMPATIBLE_VERSIONS);	//	5973L
		ERRSTR(ERROR_CLUSTER_UPGRADE_FIX_QUORUM_NOT_SUPPORTED);	//	5974L
		ERRSTR(ERROR_CLUSTER_UPGRADE_RESTART_REQUIRED);	//	5975L
		ERRSTR(ERROR_CLUSTER_UPGRADE_IN_PROGRESS);	//	5976L
		ERRSTR(ERROR_CLUSTER_UPGRADE_INCOMPLETE);	//	5977L
		ERRSTR(ERROR_CLUSTER_NODE_IN_GRACE_PERIOD);	//	5978L
		ERRSTR(ERROR_CLUSTER_CSV_IO_PAUSE_TIMEOUT);	//	5979L
		ERRSTR(ERROR_NODE_NOT_ACTIVE_CLUSTER_MEMBER);	//	5980L
		ERRSTR(ERROR_CLUSTER_RESOURCE_NOT_MONITORED);	//	5981L
		ERRSTR(ERROR_CLUSTER_RESOURCE_DOES_NOT_SUPPORT_UNMONITORED);	//	5982L
		ERRSTR(ERROR_CLUSTER_RESOURCE_IS_REPLICATED);	//	5983L
		ERRSTR(ERROR_CLUSTER_NODE_ISOLATED);	//	5984L
		ERRSTR(ERROR_CLUSTER_NODE_QUARANTINED);	//	5985L
		ERRSTR(ERROR_CLUSTER_DATABASE_UPDATE_CONDITION_FAILED);	//	5986L
		ERRSTR(ERROR_CLUSTER_SPACE_DEGRADED);	//	5987L
		ERRSTR(ERROR_CLUSTER_TOKEN_DELEGATION_NOT_SUPPORTED);	//	5988L
		ERRSTR(ERROR_CLUSTER_CSV_INVALID_HANDLE);	//	5989L
		ERRSTR(ERROR_CLUSTER_CSV_SUPPORTED_ONLY_ON_COORDINATOR);	//	5990L
		ERRSTR(ERROR_GROUPSET_NOT_AVAILABLE);	//	5991L
		ERRSTR(ERROR_GROUPSET_NOT_FOUND);	//	5992L
		ERRSTR(ERROR_GROUPSET_CANT_PROVIDE);	//	5993L
		ERRSTR(ERROR_CLUSTER_FAULT_DOMAIN_PARENT_NOT_FOUND);	//	5994L
		ERRSTR(ERROR_CLUSTER_FAULT_DOMAIN_INVALID_HIERARCHY);	//	5995L
		ERRSTR(ERROR_CLUSTER_FAULT_DOMAIN_FAILED_S2D_VALIDATION);	//	5996L
		ERRSTR(ERROR_CLUSTER_FAULT_DOMAIN_S2D_CONNECTIVITY_LOSS);	//	5997L
		ERRSTR(ERROR_CLUSTER_INVALID_INFRASTRUCTURE_FILESERVER_NAME);	//	5998L
		ERRSTR(ERROR_CLUSTERSET_MANAGEMENT_CLUSTER_UNREACHABLE);	//	5999L
		ERRSTR(ERROR_ENCRYPTION_FAILED);	//	6000L
		ERRSTR(ERROR_DECRYPTION_FAILED);	//	6001L
		ERRSTR(ERROR_FILE_ENCRYPTED);	//	6002L
		ERRSTR(ERROR_NO_RECOVERY_POLICY);	//	6003L
		ERRSTR(ERROR_NO_EFS);	//	6004L
		ERRSTR(ERROR_WRONG_EFS);	//	6005L
		ERRSTR(ERROR_NO_USER_KEYS);	//	6006L
		ERRSTR(ERROR_FILE_NOT_ENCRYPTED);	//	6007L
		ERRSTR(ERROR_NOT_EXPORT_FORMAT);	//	6008L
		ERRSTR(ERROR_FILE_READ_ONLY);	//	6009L
		ERRSTR(ERROR_DIR_EFS_DISALLOWED);	//	6010L
		ERRSTR(ERROR_EFS_SERVER_NOT_TRUSTED);	//	6011L
		ERRSTR(ERROR_BAD_RECOVERY_POLICY);	//	6012L
		ERRSTR(ERROR_EFS_ALG_BLOB_TOO_BIG);	//	6013L
		ERRSTR(ERROR_VOLUME_NOT_SUPPORT_EFS);	//	6014L
		ERRSTR(ERROR_EFS_DISABLED);	//	6015L
		ERRSTR(ERROR_EFS_VERSION_NOT_SUPPORT);	//	6016L
		ERRSTR(ERROR_CS_ENCRYPTION_INVALID_SERVER_RESPONSE);	//	6017L
		ERRSTR(ERROR_CS_ENCRYPTION_UNSUPPORTED_SERVER);	//	6018L
		ERRSTR(ERROR_CS_ENCRYPTION_EXISTING_ENCRYPTED_FILE);	//	6019L
		ERRSTR(ERROR_CS_ENCRYPTION_NEW_ENCRYPTED_FILE);	//	6020L
		ERRSTR(ERROR_CS_ENCRYPTION_FILE_NOT_CSE);	//	6021L
		ERRSTR(ERROR_ENCRYPTION_POLICY_DENIES_OPERATION);	//	6022L
		ERRSTR(ERROR_NO_BROWSER_SERVERS_FOUND);	//	6118L
		ERRSTR(SCHED_E_SERVICE_NOT_LOCALSYSTEM);	//	6200L
		ERRSTR(ERROR_LOG_SECTOR_INVALID);	//	6600L
		ERRSTR(ERROR_LOG_SECTOR_PARITY_INVALID);	//	6601L
		ERRSTR(ERROR_LOG_SECTOR_REMAPPED);	//	6602L
		ERRSTR(ERROR_LOG_BLOCK_INCOMPLETE);	//	6603L
		ERRSTR(ERROR_LOG_INVALID_RANGE);	//	6604L
		ERRSTR(ERROR_LOG_BLOCKS_EXHAUSTED);	//	6605L
		ERRSTR(ERROR_LOG_READ_CONTEXT_INVALID);	//	6606L
		ERRSTR(ERROR_LOG_RESTART_INVALID);	//	6607L
		ERRSTR(ERROR_LOG_BLOCK_VERSION);	//	6608L
		ERRSTR(ERROR_LOG_BLOCK_INVALID);	//	6609L
		ERRSTR(ERROR_LOG_READ_MODE_INVALID);	//	6610L
		ERRSTR(ERROR_LOG_NO_RESTART);	//	6611L
		ERRSTR(ERROR_LOG_METADATA_CORRUPT);	//	6612L
		ERRSTR(ERROR_LOG_METADATA_INVALID);	//	6613L
		ERRSTR(ERROR_LOG_METADATA_INCONSISTENT);	//	6614L
		ERRSTR(ERROR_LOG_RESERVATION_INVALID);	//	6615L
		ERRSTR(ERROR_LOG_CANT_DELETE);	//	6616L
		ERRSTR(ERROR_LOG_CONTAINER_LIMIT_EXCEEDED);	//	6617L
		ERRSTR(ERROR_LOG_START_OF_LOG);	//	6618L
		ERRSTR(ERROR_LOG_POLICY_ALREADY_INSTALLED);	//	6619L
		ERRSTR(ERROR_LOG_POLICY_NOT_INSTALLED);	//	6620L
		ERRSTR(ERROR_LOG_POLICY_INVALID);	//	6621L
		ERRSTR(ERROR_LOG_POLICY_CONFLICT);	//	6622L
		ERRSTR(ERROR_LOG_PINNED_ARCHIVE_TAIL);	//	6623L
		ERRSTR(ERROR_LOG_RECORD_NONEXISTENT);	//	6624L
		ERRSTR(ERROR_LOG_RECORDS_RESERVED_INVALID);	//	6625L
		ERRSTR(ERROR_LOG_SPACE_RESERVED_INVALID);	//	6626L
		ERRSTR(ERROR_LOG_TAIL_INVALID);	//	6627L
		ERRSTR(ERROR_LOG_FULL);	//	6628L
		ERRSTR(ERROR_COULD_NOT_RESIZE_LOG);	//	6629L
		ERRSTR(ERROR_LOG_MULTIPLEXED);	//	6630L
		ERRSTR(ERROR_LOG_DEDICATED);	//	6631L
		ERRSTR(ERROR_LOG_ARCHIVE_NOT_IN_PROGRESS);	//	6632L
		ERRSTR(ERROR_LOG_ARCHIVE_IN_PROGRESS);	//	6633L
		ERRSTR(ERROR_LOG_EPHEMERAL);	//	6634L
		ERRSTR(ERROR_LOG_NOT_ENOUGH_CONTAINERS);	//	6635L
		ERRSTR(ERROR_LOG_CLIENT_ALREADY_REGISTERED);	//	6636L
		ERRSTR(ERROR_LOG_CLIENT_NOT_REGISTERED);	//	6637L
		ERRSTR(ERROR_LOG_FULL_HANDLER_IN_PROGRESS);	//	6638L
		ERRSTR(ERROR_LOG_CONTAINER_READ_FAILED);	//	6639L
		ERRSTR(ERROR_LOG_CONTAINER_WRITE_FAILED);	//	6640L
		ERRSTR(ERROR_LOG_CONTAINER_OPEN_FAILED);	//	6641L
		ERRSTR(ERROR_LOG_CONTAINER_STATE_INVALID);	//	6642L
		ERRSTR(ERROR_LOG_STATE_INVALID);	//	6643L
		ERRSTR(ERROR_LOG_PINNED);	//	6644L
		ERRSTR(ERROR_LOG_METADATA_FLUSH_FAILED);	//	6645L
		ERRSTR(ERROR_LOG_INCONSISTENT_SECURITY);	//	6646L
		ERRSTR(ERROR_LOG_APPENDED_FLUSH_FAILED);	//	6647L
		ERRSTR(ERROR_LOG_PINNED_RESERVATION);	//	6648L
		ERRSTR(ERROR_INVALID_TRANSACTION);	//	6700L
		ERRSTR(ERROR_TRANSACTION_NOT_ACTIVE);	//	6701L
		ERRSTR(ERROR_TRANSACTION_REQUEST_NOT_VALID);	//	6702L
		ERRSTR(ERROR_TRANSACTION_NOT_REQUESTED);	//	6703L
		ERRSTR(ERROR_TRANSACTION_ALREADY_ABORTED);	//	6704L
		ERRSTR(ERROR_TRANSACTION_ALREADY_COMMITTED);	//	6705L
		ERRSTR(ERROR_TM_INITIALIZATION_FAILED);	//	6706L
		ERRSTR(ERROR_RESOURCEMANAGER_READ_ONLY);	//	6707L
		ERRSTR(ERROR_TRANSACTION_NOT_JOINED);	//	6708L
		ERRSTR(ERROR_TRANSACTION_SUPERIOR_EXISTS);	//	6709L
		ERRSTR(ERROR_CRM_PROTOCOL_ALREADY_EXISTS);	//	6710L
		ERRSTR(ERROR_TRANSACTION_PROPAGATION_FAILED);	//	6711L
		ERRSTR(ERROR_CRM_PROTOCOL_NOT_FOUND);	//	6712L
		ERRSTR(ERROR_TRANSACTION_INVALID_MARSHALL_BUFFER);	//	6713L
		ERRSTR(ERROR_CURRENT_TRANSACTION_NOT_VALID);	//	6714L
		ERRSTR(ERROR_TRANSACTION_NOT_FOUND);	//	6715L
		ERRSTR(ERROR_RESOURCEMANAGER_NOT_FOUND);	//	6716L
		ERRSTR(ERROR_ENLISTMENT_NOT_FOUND);	//	6717L
		ERRSTR(ERROR_TRANSACTIONMANAGER_NOT_FOUND);	//	6718L
		ERRSTR(ERROR_TRANSACTIONMANAGER_NOT_ONLINE);	//	6719L
		ERRSTR(ERROR_TRANSACTIONMANAGER_RECOVERY_NAME_COLLISION);	//	6720L
		ERRSTR(ERROR_TRANSACTION_NOT_ROOT);	//	6721L
		ERRSTR(ERROR_TRANSACTION_OBJECT_EXPIRED);	//	6722L
		ERRSTR(ERROR_TRANSACTION_RESPONSE_NOT_ENLISTED);	//	6723L
		ERRSTR(ERROR_TRANSACTION_RECORD_TOO_LONG);	//	6724L
		ERRSTR(ERROR_IMPLICIT_TRANSACTION_NOT_SUPPORTED);	//	6725L
		ERRSTR(ERROR_TRANSACTION_INTEGRITY_VIOLATED);	//	6726L
		ERRSTR(ERROR_TRANSACTIONMANAGER_IDENTITY_MISMATCH);	//	6727L
		ERRSTR(ERROR_RM_CANNOT_BE_FROZEN_FOR_SNAPSHOT);	//	6728L
		ERRSTR(ERROR_TRANSACTION_MUST_WRITETHROUGH);	//	6729L
		ERRSTR(ERROR_TRANSACTION_NO_SUPERIOR);	//	6730L
		ERRSTR(ERROR_HEURISTIC_DAMAGE_POSSIBLE);	//	6731L
		ERRSTR(ERROR_TRANSACTIONAL_CONFLICT);	//	6800L
		ERRSTR(ERROR_RM_NOT_ACTIVE);	//	6801L
		ERRSTR(ERROR_RM_METADATA_CORRUPT);	//	6802L
		ERRSTR(ERROR_DIRECTORY_NOT_RM);	//	6803L
		ERRSTR(ERROR_TRANSACTIONS_UNSUPPORTED_REMOTE);	//	6805L
		ERRSTR(ERROR_LOG_RESIZE_INVALID_SIZE);	//	6806L
		ERRSTR(ERROR_OBJECT_NO_LONGER_EXISTS);	//	6807L
		ERRSTR(ERROR_STREAM_MINIVERSION_NOT_FOUND);	//	6808L
		ERRSTR(ERROR_STREAM_MINIVERSION_NOT_VALID);	//	6809L
		ERRSTR(ERROR_MINIVERSION_INACCESSIBLE_FROM_SPECIFIED_TRANSACTION);	//	6810L
		ERRSTR(ERROR_CANT_OPEN_MINIVERSION_WITH_MODIFY_INTENT);	//	6811L
		ERRSTR(ERROR_CANT_CREATE_MORE_STREAM_MINIVERSIONS);	//	6812L
		ERRSTR(ERROR_REMOTE_FILE_VERSION_MISMATCH);	//	6814L
		ERRSTR(ERROR_HANDLE_NO_LONGER_VALID);	//	6815L
		ERRSTR(ERROR_NO_TXF_METADATA);	//	6816L
		ERRSTR(ERROR_LOG_CORRUPTION_DETECTED);	//	6817L
		ERRSTR(ERROR_CANT_RECOVER_WITH_HANDLE_OPEN);	//	6818L
		ERRSTR(ERROR_RM_DISCONNECTED);	//	6819L
		ERRSTR(ERROR_ENLISTMENT_NOT_SUPERIOR);	//	6820L
		ERRSTR(ERROR_RECOVERY_NOT_NEEDED);	//	6821L
		ERRSTR(ERROR_RM_ALREADY_STARTED);	//	6822L
		ERRSTR(ERROR_FILE_IDENTITY_NOT_PERSISTENT);	//	6823L
		ERRSTR(ERROR_CANT_BREAK_TRANSACTIONAL_DEPENDENCY);	//	6824L
		ERRSTR(ERROR_CANT_CROSS_RM_BOUNDARY);	//	6825L
		ERRSTR(ERROR_TXF_DIR_NOT_EMPTY);	//	6826L
		ERRSTR(ERROR_INDOUBT_TRANSACTIONS_EXIST);	//	6827L
		ERRSTR(ERROR_TM_VOLATILE);	//	6828L
		ERRSTR(ERROR_ROLLBACK_TIMER_EXPIRED);	//	6829L
		ERRSTR(ERROR_TXF_ATTRIBUTE_CORRUPT);	//	6830L
		ERRSTR(ERROR_EFS_NOT_ALLOWED_IN_TRANSACTION);	//	6831L
		ERRSTR(ERROR_TRANSACTIONAL_OPEN_NOT_ALLOWED);	//	6832L
		ERRSTR(ERROR_LOG_GROWTH_FAILED);	//	6833L
		ERRSTR(ERROR_TRANSACTED_MAPPING_UNSUPPORTED_REMOTE);	//	6834L
		ERRSTR(ERROR_TXF_METADATA_ALREADY_PRESENT);	//	6835L
		ERRSTR(ERROR_TRANSACTION_SCOPE_CALLBACKS_NOT_SET);	//	6836L
		ERRSTR(ERROR_TRANSACTION_REQUIRED_PROMOTION);	//	6837L
		ERRSTR(ERROR_CANNOT_EXECUTE_FILE_IN_TRANSACTION);	//	6838L
		ERRSTR(ERROR_TRANSACTIONS_NOT_FROZEN);	//	6839L
		ERRSTR(ERROR_TRANSACTION_FREEZE_IN_PROGRESS);	//	6840L
		ERRSTR(ERROR_NOT_SNAPSHOT_VOLUME);	//	6841L
		ERRSTR(ERROR_NO_SAVEPOINT_WITH_OPEN_FILES);	//	6842L
		ERRSTR(ERROR_DATA_LOST_REPAIR);	//	6843L
		ERRSTR(ERROR_SPARSE_NOT_ALLOWED_IN_TRANSACTION);	//	6844L
		ERRSTR(ERROR_TM_IDENTITY_MISMATCH);	//	6845L
		ERRSTR(ERROR_FLOATED_SECTION);	//	6846L
		ERRSTR(ERROR_CANNOT_ACCEPT_TRANSACTED_WORK);	//	6847L
		ERRSTR(ERROR_CANNOT_ABORT_TRANSACTIONS);	//	6848L
		ERRSTR(ERROR_BAD_CLUSTERS);	//	6849L
		ERRSTR(ERROR_COMPRESSION_NOT_ALLOWED_IN_TRANSACTION);	//	6850L
		ERRSTR(ERROR_VOLUME_DIRTY);	//	6851L
		ERRSTR(ERROR_NO_LINK_TRACKING_IN_TRANSACTION);	//	6852L
		ERRSTR(ERROR_OPERATION_NOT_SUPPORTED_IN_TRANSACTION);	//	6853L
		ERRSTR(ERROR_EXPIRED_HANDLE);	//	6854L
		ERRSTR(ERROR_TRANSACTION_NOT_ENLISTED);	//	6855L
		ERRSTR(ERROR_CTX_WINSTATION_NAME_INVALID);	//	7001L
		ERRSTR(ERROR_CTX_INVALID_PD);	//	7002L
		ERRSTR(ERROR_CTX_PD_NOT_FOUND);	//	7003L
		ERRSTR(ERROR_CTX_WD_NOT_FOUND);	//	7004L
		ERRSTR(ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY);	//	7005L
		ERRSTR(ERROR_CTX_SERVICE_NAME_COLLISION);	//	7006L
		ERRSTR(ERROR_CTX_CLOSE_PENDING);	//	7007L
		ERRSTR(ERROR_CTX_NO_OUTBUF);	//	7008L
		ERRSTR(ERROR_CTX_MODEM_INF_NOT_FOUND);	//	7009L
		ERRSTR(ERROR_CTX_INVALID_MODEMNAME);	//	7010L
		ERRSTR(ERROR_CTX_MODEM_RESPONSE_ERROR);	//	7011L
		ERRSTR(ERROR_CTX_MODEM_RESPONSE_TIMEOUT);	//	7012L
		ERRSTR(ERROR_CTX_MODEM_RESPONSE_NO_CARRIER);	//	7013L
		ERRSTR(ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE);	//	7014L
		ERRSTR(ERROR_CTX_MODEM_RESPONSE_BUSY);	//	7015L
		ERRSTR(ERROR_CTX_MODEM_RESPONSE_VOICE);	//	7016L
		ERRSTR(ERROR_CTX_TD_ERROR);	//	7017L
		ERRSTR(ERROR_CTX_WINSTATION_NOT_FOUND);	//	7022L
		ERRSTR(ERROR_CTX_WINSTATION_ALREADY_EXISTS);	//	7023L
		ERRSTR(ERROR_CTX_WINSTATION_BUSY);	//	7024L
		ERRSTR(ERROR_CTX_BAD_VIDEO_MODE);	//	7025L
		ERRSTR(ERROR_CTX_GRAPHICS_INVALID);	//	7035L
		ERRSTR(ERROR_CTX_LOGON_DISABLED);	//	7037L
		ERRSTR(ERROR_CTX_NOT_CONSOLE);	//	7038L
		ERRSTR(ERROR_CTX_CLIENT_QUERY_TIMEOUT);	//	7040L
		ERRSTR(ERROR_CTX_CONSOLE_DISCONNECT);	//	7041L
		ERRSTR(ERROR_CTX_CONSOLE_CONNECT);	//	7042L
		ERRSTR(ERROR_CTX_SHADOW_DENIED);	//	7044L
		ERRSTR(ERROR_CTX_WINSTATION_ACCESS_DENIED);	//	7045L
		ERRSTR(ERROR_CTX_INVALID_WD);	//	7049L
		ERRSTR(ERROR_CTX_SHADOW_INVALID);	//	7050L
		ERRSTR(ERROR_CTX_SHADOW_DISABLED);	//	7051L
		ERRSTR(ERROR_CTX_CLIENT_LICENSE_IN_USE);	//	7052L
		ERRSTR(ERROR_CTX_CLIENT_LICENSE_NOT_SET);	//	7053L
		ERRSTR(ERROR_CTX_LICENSE_NOT_AVAILABLE);	//	7054L
		ERRSTR(ERROR_CTX_LICENSE_CLIENT_INVALID);	//	7055L
		ERRSTR(ERROR_CTX_LICENSE_EXPIRED);	//	7056L
		ERRSTR(ERROR_CTX_SHADOW_NOT_RUNNING);	//	7057L
		ERRSTR(ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE);	//	7058L
		ERRSTR(ERROR_ACTIVATION_COUNT_EXCEEDED);	//	7059L
		ERRSTR(ERROR_CTX_WINSTATIONS_DISABLED);	//	7060L
		ERRSTR(ERROR_CTX_ENCRYPTION_LEVEL_REQUIRED);	//	7061L
		ERRSTR(ERROR_CTX_SESSION_IN_USE);	//	7062L
		ERRSTR(ERROR_CTX_NO_FORCE_LOGOFF);	//	7063L
		ERRSTR(ERROR_CTX_ACCOUNT_RESTRICTION);	//	7064L
		ERRSTR(ERROR_RDP_PROTOCOL_ERROR);	//	7065L
		ERRSTR(ERROR_CTX_CDM_CONNECT);	//	7066L
		ERRSTR(ERROR_CTX_CDM_DISCONNECT);	//	7067L
		ERRSTR(ERROR_CTX_SECURITY_LAYER_ERROR);	//	7068L
		ERRSTR(ERROR_TS_INCOMPATIBLE_SESSIONS);	//	7069L
		ERRSTR(ERROR_TS_VIDEO_SUBSYSTEM_ERROR);	//	7070L
		ERRSTR(FRS_ERR_INVALID_API_SEQUENCE);	//	8001L
		ERRSTR(FRS_ERR_STARTING_SERVICE);	//	8002L
		ERRSTR(FRS_ERR_STOPPING_SERVICE);	//	8003L
		ERRSTR(FRS_ERR_INTERNAL_API);	//	8004L
		ERRSTR(FRS_ERR_INTERNAL);	//	8005L
		ERRSTR(FRS_ERR_SERVICE_COMM);	//	8006L
		ERRSTR(FRS_ERR_INSUFFICIENT_PRIV);	//	8007L
		ERRSTR(FRS_ERR_AUTHENTICATION);	//	8008L
		ERRSTR(FRS_ERR_PARENT_INSUFFICIENT_PRIV);	//	8009L
		ERRSTR(FRS_ERR_PARENT_AUTHENTICATION);	//	8010L
		ERRSTR(FRS_ERR_CHILD_TO_PARENT_COMM);	//	8011L
		ERRSTR(FRS_ERR_PARENT_TO_CHILD_COMM);	//	8012L
		ERRSTR(FRS_ERR_SYSVOL_POPULATE);	//	8013L
		ERRSTR(FRS_ERR_SYSVOL_POPULATE_TIMEOUT);	//	8014L
		ERRSTR(FRS_ERR_SYSVOL_IS_BUSY);	//	8015L
		ERRSTR(FRS_ERR_SYSVOL_DEMOTE);	//	8016L
		ERRSTR(FRS_ERR_INVALID_SERVICE_PARAMETER);	//	8017L
		ERRSTR(DS_S_SUCCESS);	//	NO_ERROR
		ERRSTR(ERROR_DS_NOT_INSTALLED);	//	8200L
		ERRSTR(ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY);	//	8201L
		ERRSTR(ERROR_DS_NO_ATTRIBUTE_OR_VALUE);	//	8202L
		ERRSTR(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX);	//	8203L
		ERRSTR(ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED);	//	8204L
		ERRSTR(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS);	//	8205L
		ERRSTR(ERROR_DS_BUSY);	//	8206L
		ERRSTR(ERROR_DS_UNAVAILABLE);	//	8207L
		ERRSTR(ERROR_DS_NO_RIDS_ALLOCATED);	//	8208L
		ERRSTR(ERROR_DS_NO_MORE_RIDS);	//	8209L
		ERRSTR(ERROR_DS_INCORRECT_ROLE_OWNER);	//	8210L
		ERRSTR(ERROR_DS_RIDMGR_INIT_ERROR);	//	8211L
		ERRSTR(ERROR_DS_OBJ_CLASS_VIOLATION);	//	8212L
		ERRSTR(ERROR_DS_CANT_ON_NON_LEAF);	//	8213L
		ERRSTR(ERROR_DS_CANT_ON_RDN);	//	8214L
		ERRSTR(ERROR_DS_CANT_MOD_OBJ_CLASS);	//	8215L
		ERRSTR(ERROR_DS_CROSS_DOM_MOVE_ERROR);	//	8216L
		ERRSTR(ERROR_DS_GC_NOT_AVAILABLE);	//	8217L
		ERRSTR(ERROR_SHARED_POLICY);	//	8218L
		ERRSTR(ERROR_POLICY_OBJECT_NOT_FOUND);	//	8219L
		ERRSTR(ERROR_POLICY_ONLY_IN_DS);	//	8220L
		ERRSTR(ERROR_PROMOTION_ACTIVE);	//	8221L
		ERRSTR(ERROR_NO_PROMOTION_ACTIVE);	//	8222L
		ERRSTR(ERROR_DS_OPERATIONS_ERROR);	//	8224L
		ERRSTR(ERROR_DS_PROTOCOL_ERROR);	//	8225L
		ERRSTR(ERROR_DS_TIMELIMIT_EXCEEDED);	//	8226L
		ERRSTR(ERROR_DS_SIZELIMIT_EXCEEDED);	//	8227L
		ERRSTR(ERROR_DS_ADMIN_LIMIT_EXCEEDED);	//	8228L
		ERRSTR(ERROR_DS_COMPARE_FALSE);	//	8229L
		ERRSTR(ERROR_DS_COMPARE_TRUE);	//	8230L
		ERRSTR(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED);	//	8231L
		ERRSTR(ERROR_DS_STRONG_AUTH_REQUIRED);	//	8232L
		ERRSTR(ERROR_DS_INAPPROPRIATE_AUTH);	//	8233L
		ERRSTR(ERROR_DS_AUTH_UNKNOWN);	//	8234L
		ERRSTR(ERROR_DS_REFERRAL);	//	8235L
		ERRSTR(ERROR_DS_UNAVAILABLE_CRIT_EXTENSION);	//	8236L
		ERRSTR(ERROR_DS_CONFIDENTIALITY_REQUIRED);	//	8237L
		ERRSTR(ERROR_DS_INAPPROPRIATE_MATCHING);	//	8238L
		ERRSTR(ERROR_DS_CONSTRAINT_VIOLATION);	//	8239L
		ERRSTR(ERROR_DS_NO_SUCH_OBJECT);	//	8240L
		ERRSTR(ERROR_DS_ALIAS_PROBLEM);	//	8241L
		ERRSTR(ERROR_DS_INVALID_DN_SYNTAX);	//	8242L
		ERRSTR(ERROR_DS_IS_LEAF);	//	8243L
		ERRSTR(ERROR_DS_ALIAS_DEREF_PROBLEM);	//	8244L
		ERRSTR(ERROR_DS_UNWILLING_TO_PERFORM);	//	8245L
		ERRSTR(ERROR_DS_LOOP_DETECT);	//	8246L
		ERRSTR(ERROR_DS_NAMING_VIOLATION);	//	8247L
		ERRSTR(ERROR_DS_OBJECT_RESULTS_TOO_LARGE);	//	8248L
		ERRSTR(ERROR_DS_AFFECTS_MULTIPLE_DSAS);	//	8249L
		ERRSTR(ERROR_DS_SERVER_DOWN);	//	8250L
		ERRSTR(ERROR_DS_LOCAL_ERROR);	//	8251L
		ERRSTR(ERROR_DS_ENCODING_ERROR);	//	8252L
		ERRSTR(ERROR_DS_DECODING_ERROR);	//	8253L
		ERRSTR(ERROR_DS_FILTER_UNKNOWN);	//	8254L
		ERRSTR(ERROR_DS_PARAM_ERROR);	//	8255L
		ERRSTR(ERROR_DS_NOT_SUPPORTED);	//	8256L
		ERRSTR(ERROR_DS_NO_RESULTS_RETURNED);	//	8257L
		ERRSTR(ERROR_DS_CONTROL_NOT_FOUND);	//	8258L
		ERRSTR(ERROR_DS_CLIENT_LOOP);	//	8259L
		ERRSTR(ERROR_DS_REFERRAL_LIMIT_EXCEEDED);	//	8260L
		ERRSTR(ERROR_DS_SORT_CONTROL_MISSING);	//	8261L
		ERRSTR(ERROR_DS_OFFSET_RANGE_ERROR);	//	8262L
		ERRSTR(ERROR_DS_RIDMGR_DISABLED);	//	8263L
		ERRSTR(ERROR_DS_ROOT_MUST_BE_NC);	//	8301L
		ERRSTR(ERROR_DS_ADD_REPLICA_INHIBITED);	//	8302L
		ERRSTR(ERROR_DS_ATT_NOT_DEF_IN_SCHEMA);	//	8303L
		ERRSTR(ERROR_DS_MAX_OBJ_SIZE_EXCEEDED);	//	8304L
		ERRSTR(ERROR_DS_OBJ_STRING_NAME_EXISTS);	//	8305L
		ERRSTR(ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA);	//	8306L
		ERRSTR(ERROR_DS_RDN_DOESNT_MATCH_SCHEMA);	//	8307L
		ERRSTR(ERROR_DS_NO_REQUESTED_ATTS_FOUND);	//	8308L
		ERRSTR(ERROR_DS_USER_BUFFER_TO_SMALL);	//	8309L
		ERRSTR(ERROR_DS_ATT_IS_NOT_ON_OBJ);	//	8310L
		ERRSTR(ERROR_DS_ILLEGAL_MOD_OPERATION);	//	8311L
		ERRSTR(ERROR_DS_OBJ_TOO_LARGE);	//	8312L
		ERRSTR(ERROR_DS_BAD_INSTANCE_TYPE);	//	8313L
		ERRSTR(ERROR_DS_MASTERDSA_REQUIRED);	//	8314L
		ERRSTR(ERROR_DS_OBJECT_CLASS_REQUIRED);	//	8315L
		ERRSTR(ERROR_DS_MISSING_REQUIRED_ATT);	//	8316L
		ERRSTR(ERROR_DS_ATT_NOT_DEF_FOR_CLASS);	//	8317L
		ERRSTR(ERROR_DS_ATT_ALREADY_EXISTS);	//	8318L
		ERRSTR(ERROR_DS_CANT_ADD_ATT_VALUES);	//	8320L
		ERRSTR(ERROR_DS_SINGLE_VALUE_CONSTRAINT);	//	8321L
		ERRSTR(ERROR_DS_RANGE_CONSTRAINT);	//	8322L
		ERRSTR(ERROR_DS_ATT_VAL_ALREADY_EXISTS);	//	8323L
		ERRSTR(ERROR_DS_CANT_REM_MISSING_ATT);	//	8324L
		ERRSTR(ERROR_DS_CANT_REM_MISSING_ATT_VAL);	//	8325L
		ERRSTR(ERROR_DS_ROOT_CANT_BE_SUBREF);	//	8326L
		ERRSTR(ERROR_DS_NO_CHAINING);	//	8327L
		ERRSTR(ERROR_DS_NO_CHAINED_EVAL);	//	8328L
		ERRSTR(ERROR_DS_NO_PARENT_OBJECT);	//	8329L
		ERRSTR(ERROR_DS_PARENT_IS_AN_ALIAS);	//	8330L
		ERRSTR(ERROR_DS_CANT_MIX_MASTER_AND_REPS);	//	8331L
		ERRSTR(ERROR_DS_CHILDREN_EXIST);	//	8332L
		ERRSTR(ERROR_DS_OBJ_NOT_FOUND);	//	8333L
		ERRSTR(ERROR_DS_ALIASED_OBJ_MISSING);	//	8334L
		ERRSTR(ERROR_DS_BAD_NAME_SYNTAX);	//	8335L
		ERRSTR(ERROR_DS_ALIAS_POINTS_TO_ALIAS);	//	8336L
		ERRSTR(ERROR_DS_CANT_DEREF_ALIAS);	//	8337L
		ERRSTR(ERROR_DS_OUT_OF_SCOPE);	//	8338L
		ERRSTR(ERROR_DS_OBJECT_BEING_REMOVED);	//	8339L
		ERRSTR(ERROR_DS_CANT_DELETE_DSA_OBJ);	//	8340L
		ERRSTR(ERROR_DS_GENERIC_ERROR);	//	8341L
		ERRSTR(ERROR_DS_DSA_MUST_BE_INT_MASTER);	//	8342L
		ERRSTR(ERROR_DS_CLASS_NOT_DSA);	//	8343L
		ERRSTR(ERROR_DS_INSUFF_ACCESS_RIGHTS);	//	8344L
		ERRSTR(ERROR_DS_ILLEGAL_SUPERIOR);	//	8345L
		ERRSTR(ERROR_DS_ATTRIBUTE_OWNED_BY_SAM);	//	8346L
		ERRSTR(ERROR_DS_NAME_TOO_MANY_PARTS);	//	8347L
		ERRSTR(ERROR_DS_NAME_TOO_LONG);	//	8348L
		ERRSTR(ERROR_DS_NAME_VALUE_TOO_LONG);	//	8349L
		ERRSTR(ERROR_DS_NAME_UNPARSEABLE);	//	8350L
		ERRSTR(ERROR_DS_NAME_TYPE_UNKNOWN);	//	8351L
		ERRSTR(ERROR_DS_NOT_AN_OBJECT);	//	8352L
		ERRSTR(ERROR_DS_SEC_DESC_TOO_SHORT);	//	8353L
		ERRSTR(ERROR_DS_SEC_DESC_INVALID);	//	8354L
		ERRSTR(ERROR_DS_NO_DELETED_NAME);	//	8355L
		ERRSTR(ERROR_DS_SUBREF_MUST_HAVE_PARENT);	//	8356L
		ERRSTR(ERROR_DS_NCNAME_MUST_BE_NC);	//	8357L
		ERRSTR(ERROR_DS_CANT_ADD_SYSTEM_ONLY);	//	8358L
		ERRSTR(ERROR_DS_CLASS_MUST_BE_CONCRETE);	//	8359L
		ERRSTR(ERROR_DS_INVALID_DMD);	//	8360L
		ERRSTR(ERROR_DS_OBJ_GUID_EXISTS);	//	8361L
		ERRSTR(ERROR_DS_NOT_ON_BACKLINK);	//	8362L
		ERRSTR(ERROR_DS_NO_CROSSREF_FOR_NC);	//	8363L
		ERRSTR(ERROR_DS_SHUTTING_DOWN);	//	8364L
		ERRSTR(ERROR_DS_UNKNOWN_OPERATION);	//	8365L
		ERRSTR(ERROR_DS_INVALID_ROLE_OWNER);	//	8366L
		ERRSTR(ERROR_DS_COULDNT_CONTACT_FSMO);	//	8367L
		ERRSTR(ERROR_DS_CROSS_NC_DN_RENAME);	//	8368L
		ERRSTR(ERROR_DS_CANT_MOD_SYSTEM_ONLY);	//	8369L
		ERRSTR(ERROR_DS_REPLICATOR_ONLY);	//	8370L
		ERRSTR(ERROR_DS_OBJ_CLASS_NOT_DEFINED);	//	8371L
		ERRSTR(ERROR_DS_OBJ_CLASS_NOT_SUBCLASS);	//	8372L
		ERRSTR(ERROR_DS_NAME_REFERENCE_INVALID);	//	8373L
		ERRSTR(ERROR_DS_CROSS_REF_EXISTS);	//	8374L
		ERRSTR(ERROR_DS_CANT_DEL_MASTER_CROSSREF);	//	8375L
		ERRSTR(ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD);	//	8376L
		ERRSTR(ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX);	//	8377L
		ERRSTR(ERROR_DS_DUP_RDN);	//	8378L
		ERRSTR(ERROR_DS_DUP_OID);	//	8379L
		ERRSTR(ERROR_DS_DUP_MAPI_ID);	//	8380L
		ERRSTR(ERROR_DS_DUP_SCHEMA_ID_GUID);	//	8381L
		ERRSTR(ERROR_DS_DUP_LDAP_DISPLAY_NAME);	//	8382L
		ERRSTR(ERROR_DS_SEMANTIC_ATT_TEST);	//	8383L
		ERRSTR(ERROR_DS_SYNTAX_MISMATCH);	//	8384L
		ERRSTR(ERROR_DS_EXISTS_IN_MUST_HAVE);	//	8385L
		ERRSTR(ERROR_DS_EXISTS_IN_MAY_HAVE);	//	8386L
		ERRSTR(ERROR_DS_NONEXISTENT_MAY_HAVE);	//	8387L
		ERRSTR(ERROR_DS_NONEXISTENT_MUST_HAVE);	//	8388L
		ERRSTR(ERROR_DS_AUX_CLS_TEST_FAIL);	//	8389L
		ERRSTR(ERROR_DS_NONEXISTENT_POSS_SUP);	//	8390L
		ERRSTR(ERROR_DS_SUB_CLS_TEST_FAIL);	//	8391L
		ERRSTR(ERROR_DS_BAD_RDN_ATT_ID_SYNTAX);	//	8392L
		ERRSTR(ERROR_DS_EXISTS_IN_AUX_CLS);	//	8393L
		ERRSTR(ERROR_DS_EXISTS_IN_SUB_CLS);	//	8394L
		ERRSTR(ERROR_DS_EXISTS_IN_POSS_SUP);	//	8395L
		ERRSTR(ERROR_DS_RECALCSCHEMA_FAILED);	//	8396L
		ERRSTR(ERROR_DS_TREE_DELETE_NOT_FINISHED);	//	8397L
		ERRSTR(ERROR_DS_CANT_DELETE);	//	8398L
		ERRSTR(ERROR_DS_ATT_SCHEMA_REQ_ID);	//	8399L
		ERRSTR(ERROR_DS_BAD_ATT_SCHEMA_SYNTAX);	//	8400L
		ERRSTR(ERROR_DS_CANT_CACHE_ATT);	//	8401L
		ERRSTR(ERROR_DS_CANT_CACHE_CLASS);	//	8402L
		ERRSTR(ERROR_DS_CANT_REMOVE_ATT_CACHE);	//	8403L
		ERRSTR(ERROR_DS_CANT_REMOVE_CLASS_CACHE);	//	8404L
		ERRSTR(ERROR_DS_CANT_RETRIEVE_DN);	//	8405L
		ERRSTR(ERROR_DS_MISSING_SUPREF);	//	8406L
		ERRSTR(ERROR_DS_CANT_RETRIEVE_INSTANCE);	//	8407L
		ERRSTR(ERROR_DS_CODE_INCONSISTENCY);	//	8408L
		ERRSTR(ERROR_DS_DATABASE_ERROR);	//	8409L
		ERRSTR(ERROR_DS_GOVERNSID_MISSING);	//	8410L
		ERRSTR(ERROR_DS_MISSING_EXPECTED_ATT);	//	8411L
		ERRSTR(ERROR_DS_NCNAME_MISSING_CR_REF);	//	8412L
		ERRSTR(ERROR_DS_SECURITY_CHECKING_ERROR);	//	8413L
		ERRSTR(ERROR_DS_SCHEMA_NOT_LOADED);	//	8414L
		ERRSTR(ERROR_DS_SCHEMA_ALLOC_FAILED);	//	8415L
		ERRSTR(ERROR_DS_ATT_SCHEMA_REQ_SYNTAX);	//	8416L
		ERRSTR(ERROR_DS_GCVERIFY_ERROR);	//	8417L
		ERRSTR(ERROR_DS_DRA_SCHEMA_MISMATCH);	//	8418L
		ERRSTR(ERROR_DS_CANT_FIND_DSA_OBJ);	//	8419L
		ERRSTR(ERROR_DS_CANT_FIND_EXPECTED_NC);	//	8420L
		ERRSTR(ERROR_DS_CANT_FIND_NC_IN_CACHE);	//	8421L
		ERRSTR(ERROR_DS_CANT_RETRIEVE_CHILD);	//	8422L
		ERRSTR(ERROR_DS_SECURITY_ILLEGAL_MODIFY);	//	8423L
		ERRSTR(ERROR_DS_CANT_REPLACE_HIDDEN_REC);	//	8424L
		ERRSTR(ERROR_DS_BAD_HIERARCHY_FILE);	//	8425L
		ERRSTR(ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED);	//	8426L
		ERRSTR(ERROR_DS_CONFIG_PARAM_MISSING);	//	8427L
		ERRSTR(ERROR_DS_COUNTING_AB_INDICES_FAILED);	//	8428L
		ERRSTR(ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED);	//	8429L
		ERRSTR(ERROR_DS_INTERNAL_FAILURE);	//	8430L
		ERRSTR(ERROR_DS_UNKNOWN_ERROR);	//	8431L
		ERRSTR(ERROR_DS_ROOT_REQUIRES_CLASS_TOP);	//	8432L
		ERRSTR(ERROR_DS_REFUSING_FSMO_ROLES);	//	8433L
		ERRSTR(ERROR_DS_MISSING_FSMO_SETTINGS);	//	8434L
		ERRSTR(ERROR_DS_UNABLE_TO_SURRENDER_ROLES);	//	8435L
		ERRSTR(ERROR_DS_DRA_GENERIC);	//	8436L
		ERRSTR(ERROR_DS_DRA_INVALID_PARAMETER);	//	8437L
		ERRSTR(ERROR_DS_DRA_BUSY);	//	8438L
		ERRSTR(ERROR_DS_DRA_BAD_DN);	//	8439L
		ERRSTR(ERROR_DS_DRA_BAD_NC);	//	8440L
		ERRSTR(ERROR_DS_DRA_DN_EXISTS);	//	8441L
		ERRSTR(ERROR_DS_DRA_INTERNAL_ERROR);	//	8442L
		ERRSTR(ERROR_DS_DRA_INCONSISTENT_DIT);	//	8443L
		ERRSTR(ERROR_DS_DRA_CONNECTION_FAILED);	//	8444L
		ERRSTR(ERROR_DS_DRA_BAD_INSTANCE_TYPE);	//	8445L
		ERRSTR(ERROR_DS_DRA_OUT_OF_MEM);	//	8446L
		ERRSTR(ERROR_DS_DRA_MAIL_PROBLEM);	//	8447L
		ERRSTR(ERROR_DS_DRA_REF_ALREADY_EXISTS);	//	8448L
		ERRSTR(ERROR_DS_DRA_REF_NOT_FOUND);	//	8449L
		ERRSTR(ERROR_DS_DRA_OBJ_IS_REP_SOURCE);	//	8450L
		ERRSTR(ERROR_DS_DRA_DB_ERROR);	//	8451L
		ERRSTR(ERROR_DS_DRA_NO_REPLICA);	//	8452L
		ERRSTR(ERROR_DS_DRA_ACCESS_DENIED);	//	8453L
		ERRSTR(ERROR_DS_DRA_NOT_SUPPORTED);	//	8454L
		ERRSTR(ERROR_DS_DRA_RPC_CANCELLED);	//	8455L
		ERRSTR(ERROR_DS_DRA_SOURCE_DISABLED);	//	8456L
		ERRSTR(ERROR_DS_DRA_SINK_DISABLED);	//	8457L
		ERRSTR(ERROR_DS_DRA_NAME_COLLISION);	//	8458L
		ERRSTR(ERROR_DS_DRA_SOURCE_REINSTALLED);	//	8459L
		ERRSTR(ERROR_DS_DRA_MISSING_PARENT);	//	8460L
		ERRSTR(ERROR_DS_DRA_PREEMPTED);	//	8461L
		ERRSTR(ERROR_DS_DRA_ABANDON_SYNC);	//	8462L
		ERRSTR(ERROR_DS_DRA_SHUTDOWN);	//	8463L
		ERRSTR(ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET);	//	8464L
		ERRSTR(ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA);	//	8465L
		ERRSTR(ERROR_DS_DRA_EXTN_CONNECTION_FAILED);	//	8466L
		ERRSTR(ERROR_DS_INSTALL_SCHEMA_MISMATCH);	//	8467L
		ERRSTR(ERROR_DS_DUP_LINK_ID);	//	8468L
		ERRSTR(ERROR_DS_NAME_ERROR_RESOLVING);	//	8469L
		ERRSTR(ERROR_DS_NAME_ERROR_NOT_FOUND);	//	8470L
		ERRSTR(ERROR_DS_NAME_ERROR_NOT_UNIQUE);	//	8471L
		ERRSTR(ERROR_DS_NAME_ERROR_NO_MAPPING);	//	8472L
		ERRSTR(ERROR_DS_NAME_ERROR_DOMAIN_ONLY);	//	8473L
		ERRSTR(ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING);	//	8474L
		ERRSTR(ERROR_DS_CONSTRUCTED_ATT_MOD);	//	8475L
		ERRSTR(ERROR_DS_WRONG_OM_OBJ_CLASS);	//	8476L
		ERRSTR(ERROR_DS_DRA_REPL_PENDING);	//	8477L
		ERRSTR(ERROR_DS_DS_REQUIRED);	//	8478L
		ERRSTR(ERROR_DS_INVALID_LDAP_DISPLAY_NAME);	//	8479L
		ERRSTR(ERROR_DS_NON_BASE_SEARCH);	//	8480L
		ERRSTR(ERROR_DS_CANT_RETRIEVE_ATTS);	//	8481L
		ERRSTR(ERROR_DS_BACKLINK_WITHOUT_LINK);	//	8482L
		ERRSTR(ERROR_DS_EPOCH_MISMATCH);	//	8483L
		ERRSTR(ERROR_DS_SRC_NAME_MISMATCH);	//	8484L
		ERRSTR(ERROR_DS_SRC_AND_DST_NC_IDENTICAL);	//	8485L
		ERRSTR(ERROR_DS_DST_NC_MISMATCH);	//	8486L
		ERRSTR(ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC);	//	8487L
		ERRSTR(ERROR_DS_SRC_GUID_MISMATCH);	//	8488L
		ERRSTR(ERROR_DS_CANT_MOVE_DELETED_OBJECT);	//	8489L
		ERRSTR(ERROR_DS_PDC_OPERATION_IN_PROGRESS);	//	8490L
		ERRSTR(ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD);	//	8491L
		ERRSTR(ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION);	//	8492L
		ERRSTR(ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS);	//	8493L
		ERRSTR(ERROR_DS_NC_MUST_HAVE_NC_PARENT);	//	8494L
		ERRSTR(ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE);	//	8495L
		ERRSTR(ERROR_DS_DST_DOMAIN_NOT_NATIVE);	//	8496L
		ERRSTR(ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER);	//	8497L
		ERRSTR(ERROR_DS_CANT_MOVE_ACCOUNT_GROUP);	//	8498L
		ERRSTR(ERROR_DS_CANT_MOVE_RESOURCE_GROUP);	//	8499L
		ERRSTR(ERROR_DS_INVALID_SEARCH_FLAG);	//	8500L
		ERRSTR(ERROR_DS_NO_TREE_DELETE_ABOVE_NC);	//	8501L
		ERRSTR(ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE);	//	8502L
		ERRSTR(ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE);	//	8503L
		ERRSTR(ERROR_DS_SAM_INIT_FAILURE);	//	8504L
		ERRSTR(ERROR_DS_SENSITIVE_GROUP_VIOLATION);	//	8505L
		ERRSTR(ERROR_DS_CANT_MOD_PRIMARYGROUPID);	//	8506L
		ERRSTR(ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD);	//	8507L
		ERRSTR(ERROR_DS_NONSAFE_SCHEMA_CHANGE);	//	8508L
		ERRSTR(ERROR_DS_SCHEMA_UPDATE_DISALLOWED);	//	8509L
		ERRSTR(ERROR_DS_CANT_CREATE_UNDER_SCHEMA);	//	8510L
		ERRSTR(ERROR_DS_INSTALL_NO_SRC_SCH_VERSION);	//	8511L
		ERRSTR(ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE);	//	8512L
		ERRSTR(ERROR_DS_INVALID_GROUP_TYPE);	//	8513L
		ERRSTR(ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN);	//	8514L
		ERRSTR(ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN);	//	8515L
		ERRSTR(ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER);	//	8516L
		ERRSTR(ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER);	//	8517L
		ERRSTR(ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER);	//	8518L
		ERRSTR(ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER);	//	8519L
		ERRSTR(ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER);	//	8520L
		ERRSTR(ERROR_DS_HAVE_PRIMARY_MEMBERS);	//	8521L
		ERRSTR(ERROR_DS_STRING_SD_CONVERSION_FAILED);	//	8522L
		ERRSTR(ERROR_DS_NAMING_MASTER_GC);	//	8523L
		ERRSTR(ERROR_DS_DNS_LOOKUP_FAILURE);	//	8524L
		ERRSTR(ERROR_DS_COULDNT_UPDATE_SPNS);	//	8525L
		ERRSTR(ERROR_DS_CANT_RETRIEVE_SD);	//	8526L
		ERRSTR(ERROR_DS_KEY_NOT_UNIQUE);	//	8527L
		ERRSTR(ERROR_DS_WRONG_LINKED_ATT_SYNTAX);	//	8528L
		ERRSTR(ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD);	//	8529L
		ERRSTR(ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY);	//	8530L
		ERRSTR(ERROR_DS_CANT_START);	//	8531L
		ERRSTR(ERROR_DS_INIT_FAILURE);	//	8532L
		ERRSTR(ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION);	//	8533L
		ERRSTR(ERROR_DS_SOURCE_DOMAIN_IN_FOREST);	//	8534L
		ERRSTR(ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST);	//	8535L
		ERRSTR(ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED);	//	8536L
		ERRSTR(ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN);	//	8537L
		ERRSTR(ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER);	//	8538L
		ERRSTR(ERROR_DS_SRC_SID_EXISTS_IN_FOREST);	//	8539L
		ERRSTR(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);	//	8540L
		ERRSTR(ERROR_SAM_INIT_FAILURE);	//	8541L
		ERRSTR(ERROR_DS_DRA_SCHEMA_INFO_SHIP);	//	8542L
		ERRSTR(ERROR_DS_DRA_SCHEMA_CONFLICT);	//	8543L
		ERRSTR(ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT);	//	8544L
		ERRSTR(ERROR_DS_DRA_OBJ_NC_MISMATCH);	//	8545L
		ERRSTR(ERROR_DS_NC_STILL_HAS_DSAS);	//	8546L
		ERRSTR(ERROR_DS_GC_REQUIRED);	//	8547L
		ERRSTR(ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY);	//	8548L
		ERRSTR(ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS);	//	8549L
		ERRSTR(ERROR_DS_CANT_ADD_TO_GC);	//	8550L
		ERRSTR(ERROR_DS_NO_CHECKPOINT_WITH_PDC);	//	8551L
		ERRSTR(ERROR_DS_SOURCE_AUDITING_NOT_ENABLED);	//	8552L
		ERRSTR(ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC);	//	8553L
		ERRSTR(ERROR_DS_INVALID_NAME_FOR_SPN);	//	8554L
		ERRSTR(ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS);	//	8555L
		ERRSTR(ERROR_DS_UNICODEPWD_NOT_IN_QUOTES);	//	8556L
		ERRSTR(ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED);	//	8557L
		ERRSTR(ERROR_DS_MUST_BE_RUN_ON_DST_DC);	//	8558L
		ERRSTR(ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER);	//	8559L
		ERRSTR(ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ);	//	8560L
		ERRSTR(ERROR_DS_INIT_FAILURE_CONSOLE);	//	8561L
		ERRSTR(ERROR_DS_SAM_INIT_FAILURE_CONSOLE);	//	8562L
		ERRSTR(ERROR_DS_FOREST_VERSION_TOO_HIGH);	//	8563L
		ERRSTR(ERROR_DS_DOMAIN_VERSION_TOO_HIGH);	//	8564L
		ERRSTR(ERROR_DS_FOREST_VERSION_TOO_LOW);	//	8565L
		ERRSTR(ERROR_DS_DOMAIN_VERSION_TOO_LOW);	//	8566L
		ERRSTR(ERROR_DS_INCOMPATIBLE_VERSION);	//	8567L
		ERRSTR(ERROR_DS_LOW_DSA_VERSION);	//	8568L
		ERRSTR(ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN);	//	8569L
		ERRSTR(ERROR_DS_NOT_SUPPORTED_SORT_ORDER);	//	8570L
		ERRSTR(ERROR_DS_NAME_NOT_UNIQUE);	//	8571L
		ERRSTR(ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4);	//	8572L
		ERRSTR(ERROR_DS_OUT_OF_VERSION_STORE);	//	8573L
		ERRSTR(ERROR_DS_INCOMPATIBLE_CONTROLS_USED);	//	8574L
		ERRSTR(ERROR_DS_NO_REF_DOMAIN);	//	8575L
		ERRSTR(ERROR_DS_RESERVED_LINK_ID);	//	8576L
		ERRSTR(ERROR_DS_LINK_ID_NOT_AVAILABLE);	//	8577L
		ERRSTR(ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER);	//	8578L
		ERRSTR(ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE);	//	8579L
		ERRSTR(ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC);	//	8580L
		ERRSTR(ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG);	//	8581L
		ERRSTR(ERROR_DS_MODIFYDN_WRONG_GRANDPARENT);	//	8582L
		ERRSTR(ERROR_DS_NAME_ERROR_TRUST_REFERRAL);	//	8583L
		ERRSTR(ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER);	//	8584L
		ERRSTR(ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD);	//	8585L
		ERRSTR(ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2);	//	8586L
		ERRSTR(ERROR_DS_THREAD_LIMIT_EXCEEDED);	//	8587L
		ERRSTR(ERROR_DS_NOT_CLOSEST);	//	8588L
		ERRSTR(ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF);	//	8589L
		ERRSTR(ERROR_DS_SINGLE_USER_MODE_FAILED);	//	8590L
		ERRSTR(ERROR_DS_NTDSCRIPT_SYNTAX_ERROR);	//	8591L
		ERRSTR(ERROR_DS_NTDSCRIPT_PROCESS_ERROR);	//	8592L
		ERRSTR(ERROR_DS_DIFFERENT_REPL_EPOCHS);	//	8593L
		ERRSTR(ERROR_DS_DRS_EXTENSIONS_CHANGED);	//	8594L
		ERRSTR(ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR);	//	8595L
		ERRSTR(ERROR_DS_NO_MSDS_INTID);	//	8596L
		ERRSTR(ERROR_DS_DUP_MSDS_INTID);	//	8597L
		ERRSTR(ERROR_DS_EXISTS_IN_RDNATTID);	//	8598L
		ERRSTR(ERROR_DS_AUTHORIZATION_FAILED);	//	8599L
		ERRSTR(ERROR_DS_INVALID_SCRIPT);	//	8600L
		ERRSTR(ERROR_DS_REMOTE_CROSSREF_OP_FAILED);	//	8601L
		ERRSTR(ERROR_DS_CROSS_REF_BUSY);	//	8602L
		ERRSTR(ERROR_DS_CANT_DERIVE_SPN_FOR_DELETED_DOMAIN);	//	8603L
		ERRSTR(ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC);	//	8604L
		ERRSTR(ERROR_DS_DUPLICATE_ID_FOUND);	//	8605L
		ERRSTR(ERROR_DS_INSUFFICIENT_ATTR_TO_CREATE_OBJECT);	//	8606L
		ERRSTR(ERROR_DS_GROUP_CONVERSION_ERROR);	//	8607L
		ERRSTR(ERROR_DS_CANT_MOVE_APP_BASIC_GROUP);	//	8608L
		ERRSTR(ERROR_DS_CANT_MOVE_APP_QUERY_GROUP);	//	8609L
		ERRSTR(ERROR_DS_ROLE_NOT_VERIFIED);	//	8610L
		ERRSTR(ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL);	//	8611L
		ERRSTR(ERROR_DS_DOMAIN_RENAME_IN_PROGRESS);	//	8612L
		ERRSTR(ERROR_DS_EXISTING_AD_CHILD_NC);	//	8613L
		ERRSTR(ERROR_DS_REPL_LIFETIME_EXCEEDED);	//	8614L
		ERRSTR(ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER);	//	8615L
		ERRSTR(ERROR_DS_LDAP_SEND_QUEUE_FULL);	//	8616L
		ERRSTR(ERROR_DS_DRA_OUT_SCHEDULE_WINDOW);	//	8617L
		ERRSTR(ERROR_DS_POLICY_NOT_KNOWN);	//	8618L
		ERRSTR(ERROR_NO_SITE_SETTINGS_OBJECT);	//	8619L
		ERRSTR(ERROR_NO_SECRETS);	//	8620L
		ERRSTR(ERROR_NO_WRITABLE_DC_FOUND);	//	8621L
		ERRSTR(ERROR_DS_NO_SERVER_OBJECT);	//	8622L
		ERRSTR(ERROR_DS_NO_NTDSA_OBJECT);	//	8623L
		ERRSTR(ERROR_DS_NON_ASQ_SEARCH);	//	8624L
		ERRSTR(ERROR_DS_AUDIT_FAILURE);	//	8625L
		ERRSTR(ERROR_DS_INVALID_SEARCH_FLAG_SUBTREE);	//	8626L
		ERRSTR(ERROR_DS_INVALID_SEARCH_FLAG_TUPLE);	//	8627L
		ERRSTR(ERROR_DS_HIERARCHY_TABLE_TOO_DEEP);	//	8628L
		ERRSTR(ERROR_DS_DRA_CORRUPT_UTD_VECTOR);	//	8629L
		ERRSTR(ERROR_DS_DRA_SECRETS_DENIED);	//	8630L
		ERRSTR(ERROR_DS_RESERVED_MAPI_ID);	//	8631L
		ERRSTR(ERROR_DS_MAPI_ID_NOT_AVAILABLE);	//	8632L
		ERRSTR(ERROR_DS_DRA_MISSING_KRBTGT_SECRET);	//	8633L
		ERRSTR(ERROR_DS_DOMAIN_NAME_EXISTS_IN_FOREST);	//	8634L
		ERRSTR(ERROR_DS_FLAT_NAME_EXISTS_IN_FOREST);	//	8635L
		ERRSTR(ERROR_INVALID_USER_PRINCIPAL_NAME);	//	8636L
		ERRSTR(ERROR_DS_OID_MAPPED_GROUP_CANT_HAVE_MEMBERS);	//	8637L
		ERRSTR(ERROR_DS_OID_NOT_FOUND);	//	8638L
		ERRSTR(ERROR_DS_DRA_RECYCLED_TARGET);	//	8639L
		ERRSTR(ERROR_DS_DISALLOWED_NC_REDIRECT);	//	8640L
		ERRSTR(ERROR_DS_HIGH_ADLDS_FFL);	//	8641L
		ERRSTR(ERROR_DS_HIGH_DSA_VERSION);	//	8642L
		ERRSTR(ERROR_DS_LOW_ADLDS_FFL);	//	8643L
		ERRSTR(ERROR_DOMAIN_SID_SAME_AS_LOCAL_WORKSTATION);	//	8644L
		ERRSTR(ERROR_DS_UNDELETE_SAM_VALIDATION_FAILED);	//	8645L
		ERRSTR(ERROR_INCORRECT_ACCOUNT_TYPE);	//	8646L
		ERRSTR(ERROR_DS_SPN_VALUE_NOT_UNIQUE_IN_FOREST);	//	8647L
		ERRSTR(ERROR_DS_UPN_VALUE_NOT_UNIQUE_IN_FOREST);	//	8648L
		ERRSTR(DNS_ERROR_RESPONSE_CODES_BASE);	//	9000
		ERRSTR(DNS_ERROR_RCODE_NO_ERROR);	//	NO_ERROR
		ERRSTR(DNS_ERROR_MASK);	//	0x00002328 // 9000 or DNS_ERROR_RESPONSE_CODES_BASE
		ERRSTR(DNS_ERROR_RCODE_FORMAT_ERROR);	//	9001L
		ERRSTR(DNS_ERROR_RCODE_SERVER_FAILURE);	//	9002L
		ERRSTR(DNS_ERROR_RCODE_NAME_ERROR);	//	9003L
		ERRSTR(DNS_ERROR_RCODE_NOT_IMPLEMENTED);	//	9004L
		ERRSTR(DNS_ERROR_RCODE_REFUSED);	//	9005L
		ERRSTR(DNS_ERROR_RCODE_YXDOMAIN);	//	9006L
		ERRSTR(DNS_ERROR_RCODE_YXRRSET);	//	9007L
		ERRSTR(DNS_ERROR_RCODE_NXRRSET);	//	9008L
		ERRSTR(DNS_ERROR_RCODE_NOTAUTH);	//	9009L
		ERRSTR(DNS_ERROR_RCODE_NOTZONE);	//	9010L
		ERRSTR(DNS_ERROR_RCODE_BADSIG);	//	9016L
		ERRSTR(DNS_ERROR_RCODE_BADKEY);	//	9017L
		ERRSTR(DNS_ERROR_RCODE_BADTIME);	//	9018L
		ERRSTR(DNS_ERROR_RCODE_LAST);	//	DNS_ERROR_RCODE_BADTIME
		ERRSTR(DNS_ERROR_DNSSEC_BASE);	//	9100
		ERRSTR(DNS_ERROR_KEYMASTER_REQUIRED);	//	9101L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_ON_SIGNED_ZONE);	//	9102L
		ERRSTR(DNS_ERROR_NSEC3_INCOMPATIBLE_WITH_RSA_SHA1);	//	9103L
		ERRSTR(DNS_ERROR_NOT_ENOUGH_SIGNING_KEY_DESCRIPTORS);	//	9104L
		ERRSTR(DNS_ERROR_UNSUPPORTED_ALGORITHM);	//	9105L
		ERRSTR(DNS_ERROR_INVALID_KEY_SIZE);	//	9106L
		ERRSTR(DNS_ERROR_SIGNING_KEY_NOT_ACCESSIBLE);	//	9107L
		ERRSTR(DNS_ERROR_KSP_DOES_NOT_SUPPORT_PROTECTION);	//	9108L
		ERRSTR(DNS_ERROR_UNEXPECTED_DATA_PROTECTION_ERROR);	//	9109L
		ERRSTR(DNS_ERROR_UNEXPECTED_CNG_ERROR);	//	9110L
		ERRSTR(DNS_ERROR_UNKNOWN_SIGNING_PARAMETER_VERSION);	//	9111L
		ERRSTR(DNS_ERROR_KSP_NOT_ACCESSIBLE);	//	9112L
		ERRSTR(DNS_ERROR_TOO_MANY_SKDS);	//	9113L
		ERRSTR(DNS_ERROR_INVALID_ROLLOVER_PERIOD);	//	9114L
		ERRSTR(DNS_ERROR_INVALID_INITIAL_ROLLOVER_OFFSET);	//	9115L
		ERRSTR(DNS_ERROR_ROLLOVER_IN_PROGRESS);	//	9116L
		ERRSTR(DNS_ERROR_STANDBY_KEY_NOT_PRESENT);	//	9117L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_ON_ZSK);	//	9118L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_ON_ACTIVE_SKD);	//	9119L
		ERRSTR(DNS_ERROR_ROLLOVER_ALREADY_QUEUED);	//	9120L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_ON_UNSIGNED_ZONE);	//	9121L
		ERRSTR(DNS_ERROR_BAD_KEYMASTER);	//	9122L
		ERRSTR(DNS_ERROR_INVALID_SIGNATURE_VALIDITY_PERIOD);	//	9123L
		ERRSTR(DNS_ERROR_INVALID_NSEC3_ITERATION_COUNT);	//	9124L
		ERRSTR(DNS_ERROR_DNSSEC_IS_DISABLED);	//	9125L
		ERRSTR(DNS_ERROR_INVALID_XML);	//	9126L
		ERRSTR(DNS_ERROR_NO_VALID_TRUST_ANCHORS);	//	9127L
		ERRSTR(DNS_ERROR_ROLLOVER_NOT_POKEABLE);	//	9128L
		ERRSTR(DNS_ERROR_NSEC3_NAME_COLLISION);	//	9129L
		ERRSTR(DNS_ERROR_NSEC_INCOMPATIBLE_WITH_NSEC3_RSA_SHA1);	//	9130L
		ERRSTR(DNS_ERROR_PACKET_FMT_BASE);	//	9500
		ERRSTR(DNS_INFO_NO_RECORDS);	//	9501L
		ERRSTR(DNS_ERROR_BAD_PACKET);	//	9502L
		ERRSTR(DNS_ERROR_NO_PACKET);	//	9503L
		ERRSTR(DNS_ERROR_RCODE);	//	9504L
		ERRSTR(DNS_ERROR_UNSECURE_PACKET);	//	9505L
		ERRSTR(DNS_STATUS_PACKET_UNSECURE);	//	DNS_ERROR_UNSECURE_PACKET
		ERRSTR(DNS_REQUEST_PENDING);	//	9506L
		ERRSTR(DNS_ERROR_NO_MEMORY);	//	ERROR_OUTOFMEMORY
		ERRSTR(DNS_ERROR_INVALID_NAME);	//	ERROR_INVALID_NAME
		ERRSTR(DNS_ERROR_INVALID_DATA);	//	ERROR_INVALID_DATA
		ERRSTR(DNS_ERROR_GENERAL_API_BASE);	//	9550
		ERRSTR(DNS_ERROR_INVALID_TYPE);	//	9551L
		ERRSTR(DNS_ERROR_INVALID_IP_ADDRESS);	//	9552L
		ERRSTR(DNS_ERROR_INVALID_PROPERTY);	//	9553L
		ERRSTR(DNS_ERROR_TRY_AGAIN_LATER);	//	9554L
		ERRSTR(DNS_ERROR_NOT_UNIQUE);	//	9555L
		ERRSTR(DNS_ERROR_NON_RFC_NAME);	//	9556L
		ERRSTR(DNS_STATUS_FQDN);	//	9557L
		ERRSTR(DNS_STATUS_DOTTED_NAME);	//	9558L
		ERRSTR(DNS_STATUS_SINGLE_PART_NAME);	//	9559L
		ERRSTR(DNS_ERROR_INVALID_NAME_CHAR);	//	9560L
		ERRSTR(DNS_ERROR_NUMERIC_NAME);	//	9561L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER);	//	9562L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_UNDER_DELEGATION);	//	9563L
		ERRSTR(DNS_ERROR_CANNOT_FIND_ROOT_HINTS);	//	9564L
		ERRSTR(DNS_ERROR_INCONSISTENT_ROOT_HINTS);	//	9565L
		ERRSTR(DNS_ERROR_DWORD_VALUE_TOO_SMALL);	//	9566L
		ERRSTR(DNS_ERROR_DWORD_VALUE_TOO_LARGE);	//	9567L
		ERRSTR(DNS_ERROR_BACKGROUND_LOADING);	//	9568L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_ON_RODC);	//	9569L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_UNDER_DNAME);	//	9570L
		ERRSTR(DNS_ERROR_DELEGATION_REQUIRED);	//	9571L
		ERRSTR(DNS_ERROR_INVALID_POLICY_TABLE);	//	9572L
		ERRSTR(DNS_ERROR_ZONE_BASE);	//	9600
		ERRSTR(DNS_ERROR_ZONE_DOES_NOT_EXIST);	//	9601L
		ERRSTR(DNS_ERROR_NO_ZONE_INFO);	//	9602L
		ERRSTR(DNS_ERROR_INVALID_ZONE_OPERATION);	//	9603L
		ERRSTR(DNS_ERROR_ZONE_CONFIGURATION_ERROR);	//	9604L
		ERRSTR(DNS_ERROR_ZONE_HAS_NO_SOA_RECORD);	//	9605L
		ERRSTR(DNS_ERROR_ZONE_HAS_NO_NS_RECORDS);	//	9606L
		ERRSTR(DNS_ERROR_ZONE_LOCKED);	//	9607L
		ERRSTR(DNS_ERROR_ZONE_CREATION_FAILED);	//	9608L
		ERRSTR(DNS_ERROR_ZONE_ALREADY_EXISTS);	//	9609L
		ERRSTR(DNS_ERROR_AUTOZONE_ALREADY_EXISTS);	//	9610L
		ERRSTR(DNS_ERROR_INVALID_ZONE_TYPE);	//	9611L
		ERRSTR(DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP);	//	9612L
		ERRSTR(DNS_ERROR_ZONE_NOT_SECONDARY);	//	9613L
		ERRSTR(DNS_ERROR_NEED_SECONDARY_ADDRESSES);	//	9614L
		ERRSTR(DNS_ERROR_WINS_INIT_FAILED);	//	9615L
		ERRSTR(DNS_ERROR_NEED_WINS_SERVERS);	//	9616L
		ERRSTR(DNS_ERROR_NBSTAT_INIT_FAILED);	//	9617L
		ERRSTR(DNS_ERROR_SOA_DELETE_INVALID);	//	9618L
		ERRSTR(DNS_ERROR_FORWARDER_ALREADY_EXISTS);	//	9619L
		ERRSTR(DNS_ERROR_ZONE_REQUIRES_MASTER_IP);	//	9620L
		ERRSTR(DNS_ERROR_ZONE_IS_SHUTDOWN);	//	9621L
		ERRSTR(DNS_ERROR_ZONE_LOCKED_FOR_SIGNING);	//	9622L
		ERRSTR(DNS_ERROR_DATAFILE_BASE);	//	9650
		ERRSTR(DNS_ERROR_PRIMARY_REQUIRES_DATAFILE);	//	9651L
		ERRSTR(DNS_ERROR_INVALID_DATAFILE_NAME);	//	9652L
		ERRSTR(DNS_ERROR_DATAFILE_OPEN_FAILURE);	//	9653L
		ERRSTR(DNS_ERROR_FILE_WRITEBACK_FAILED);	//	9654L
		ERRSTR(DNS_ERROR_DATAFILE_PARSING);	//	9655L
		ERRSTR(DNS_ERROR_DATABASE_BASE);	//	9700
		ERRSTR(DNS_ERROR_RECORD_DOES_NOT_EXIST);	//	9701L
		ERRSTR(DNS_ERROR_RECORD_FORMAT);	//	9702L
		ERRSTR(DNS_ERROR_NODE_CREATION_FAILED);	//	9703L
		ERRSTR(DNS_ERROR_UNKNOWN_RECORD_TYPE);	//	9704L
		ERRSTR(DNS_ERROR_RECORD_TIMED_OUT);	//	9705L
		ERRSTR(DNS_ERROR_NAME_NOT_IN_ZONE);	//	9706L
		ERRSTR(DNS_ERROR_CNAME_LOOP);	//	9707L
		ERRSTR(DNS_ERROR_NODE_IS_CNAME);	//	9708L
		ERRSTR(DNS_ERROR_CNAME_COLLISION);	//	9709L
		ERRSTR(DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT);	//	9710L
		ERRSTR(DNS_ERROR_RECORD_ALREADY_EXISTS);	//	9711L
		ERRSTR(DNS_ERROR_SECONDARY_DATA);	//	9712L
		ERRSTR(DNS_ERROR_NO_CREATE_CACHE_DATA);	//	9713L
		ERRSTR(DNS_ERROR_NAME_DOES_NOT_EXIST);	//	9714L
		ERRSTR(DNS_WARNING_PTR_CREATE_FAILED);	//	9715L
		ERRSTR(DNS_WARNING_DOMAIN_UNDELETED);	//	9716L
		ERRSTR(DNS_ERROR_DS_UNAVAILABLE);	//	9717L
		ERRSTR(DNS_ERROR_DS_ZONE_ALREADY_EXISTS);	//	9718L
		ERRSTR(DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE);	//	9719L
		ERRSTR(DNS_ERROR_NODE_IS_DNAME);	//	9720L
		ERRSTR(DNS_ERROR_DNAME_COLLISION);	//	9721L
		ERRSTR(DNS_ERROR_ALIAS_LOOP);	//	9722L
		ERRSTR(DNS_ERROR_OPERATION_BASE);	//	9750
		ERRSTR(DNS_INFO_AXFR_COMPLETE);	//	9751L
		ERRSTR(DNS_ERROR_AXFR);	//	9752L
		ERRSTR(DNS_INFO_ADDED_LOCAL_WINS);	//	9753L
		ERRSTR(DNS_ERROR_SECURE_BASE);	//	9800
		ERRSTR(DNS_STATUS_CONTINUE_NEEDED);	//	9801L
		ERRSTR(DNS_ERROR_SETUP_BASE);	//	9850
		ERRSTR(DNS_ERROR_NO_TCPIP);	//	9851L
		ERRSTR(DNS_ERROR_NO_DNS_SERVERS);	//	9852L
		ERRSTR(DNS_ERROR_DP_BASE);	//	9900
		ERRSTR(DNS_ERROR_DP_DOES_NOT_EXIST);	//	9901L
		ERRSTR(DNS_ERROR_DP_ALREADY_EXISTS);	//	9902L
		ERRSTR(DNS_ERROR_DP_NOT_ENLISTED);	//	9903L
		ERRSTR(DNS_ERROR_DP_ALREADY_ENLISTED);	//	9904L
		ERRSTR(DNS_ERROR_DP_NOT_AVAILABLE);	//	9905L
		ERRSTR(DNS_ERROR_DP_FSMO_ERROR);	//	9906L
		ERRSTR(DNS_ERROR_ZONESCOPE_ALREADY_EXISTS);	//	9951L
		ERRSTR(DNS_ERROR_ZONESCOPE_DOES_NOT_EXIST);	//	9952L
		ERRSTR(DNS_ERROR_DEFAULT_ZONESCOPE);	//	9953L
		ERRSTR(DNS_ERROR_INVALID_ZONESCOPE_NAME);	//	9954L
		ERRSTR(DNS_ERROR_NOT_ALLOWED_WITH_ZONESCOPES);	//	9955L
		ERRSTR(DNS_ERROR_LOAD_ZONESCOPE_FAILED);	//	9956L
		ERRSTR(DNS_ERROR_ZONESCOPE_FILE_WRITEBACK_FAILED);	//	9957L
		ERRSTR(DNS_ERROR_INVALID_SCOPE_NAME);	//	9958L
		ERRSTR(DNS_ERROR_SCOPE_DOES_NOT_EXIST);	//	9959L
		ERRSTR(DNS_ERROR_DEFAULT_SCOPE);	//	9960L
		ERRSTR(DNS_ERROR_INVALID_SCOPE_OPERATION);	//	9961L
		ERRSTR(DNS_ERROR_SCOPE_LOCKED);	//	9962L
		ERRSTR(DNS_ERROR_SCOPE_ALREADY_EXISTS);	//	9963L

#if CPP17_OR_LATER
		ERRSTR(DNS_ERROR_ADDRESS_REQUIRED);	//	9573L
	
		ERRSTR(ERROR_WIP_ENCRYPTION_FAILED);	//	6023L
		ERRSTR(ERROR_CLUSTER_OBJECT_IS_CLUSTER_SET_VM);	//	6250L
		ERRSTR(ERROR_DS_MISSING_FOREST_TRUST);	//	8649L
		ERRSTR(ERROR_DS_VALUE_KEY_NOT_UNIQUE);	//	8650L

		ERRSTR(DNS_ERROR_RRL_NOT_ENABLED);	//	9911L
		ERRSTR(DNS_ERROR_RRL_INVALID_WINDOW_SIZE);	//	9912L
		ERRSTR(DNS_ERROR_RRL_INVALID_IPV4_PREFIX);	//	9913L
		ERRSTR(DNS_ERROR_RRL_INVALID_IPV6_PREFIX);	//	9914L
		ERRSTR(DNS_ERROR_RRL_INVALID_TC_RATE);	//	9915L
		ERRSTR(DNS_ERROR_RRL_INVALID_LEAK_RATE);	//	9916L
		ERRSTR(DNS_ERROR_RRL_LEAK_RATE_LESSTHAN_TC_RATE);	//	9917L
		ERRSTR(DNS_ERROR_VIRTUALIZATION_INSTANCE_ALREADY_EXISTS);	//	9921L
		ERRSTR(DNS_ERROR_VIRTUALIZATION_INSTANCE_DOES_NOT_EXIST);	//	9922L
		ERRSTR(DNS_ERROR_VIRTUALIZATION_TREE_LOCKED);	//	9923L
		ERRSTR(DNS_ERROR_INVAILD_VIRTUALIZATION_INSTANCE_NAME);	//	9924L
		ERRSTR(DNS_ERROR_DEFAULT_VIRTUALIZATION_INSTANCE);	//	9925L
		ERRSTR(DNS_ERROR_POLICY_ALREADY_EXISTS);	//	9971L
		ERRSTR(DNS_ERROR_POLICY_DOES_NOT_EXIST);	//	9972L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA);	//	9973L
		ERRSTR(DNS_ERROR_POLICY_INVALID_SETTINGS);	//	9974L
		ERRSTR(DNS_ERROR_CLIENT_SUBNET_IS_ACCESSED);	//	9975L
		ERRSTR(DNS_ERROR_CLIENT_SUBNET_DOES_NOT_EXIST);	//	9976L
		ERRSTR(DNS_ERROR_CLIENT_SUBNET_ALREADY_EXISTS);	//	9977L
		ERRSTR(DNS_ERROR_SUBNET_DOES_NOT_EXIST);	//	9978L
		ERRSTR(DNS_ERROR_SUBNET_ALREADY_EXISTS);	//	9979L
		ERRSTR(DNS_ERROR_POLICY_LOCKED);	//	9980L
		ERRSTR(DNS_ERROR_POLICY_INVALID_WEIGHT);	//	9981L
		ERRSTR(DNS_ERROR_POLICY_INVALID_NAME);	//	9982L
		ERRSTR(DNS_ERROR_POLICY_MISSING_CRITERIA);	//	9983L
		ERRSTR(DNS_ERROR_INVALID_CLIENT_SUBNET_NAME);	//	9984L
		ERRSTR(DNS_ERROR_POLICY_PROCESSING_ORDER_INVALID);	//	9985L
		ERRSTR(DNS_ERROR_POLICY_SCOPE_MISSING);	//	9986L
		ERRSTR(DNS_ERROR_POLICY_SCOPE_NOT_ALLOWED);	//	9987L
		ERRSTR(DNS_ERROR_SERVERSCOPE_IS_REFERENCED);	//	9988L
		ERRSTR(DNS_ERROR_ZONESCOPE_IS_REFERENCED);	//	9989L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_CLIENT_SUBNET);	//	9990L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_TRANSPORT_PROTOCOL);	//	9991L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_NETWORK_PROTOCOL);	//	9992L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_INTERFACE);	//	9993L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_FQDN);	//	9994L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_QUERY_TYPE);	//	9995L
		ERRSTR(DNS_ERROR_POLICY_INVALID_CRITERIA_TIME_OF_DAY);	//	9996L
#endif //CPP17_OR_LATER

		ERRSTR(WSABASEERR);	//	10000
		ERRSTR(WSAEINTR);	//	10004L
		ERRSTR(WSAEBADF);	//	10009L
		ERRSTR(WSAEACCES);	//	10013L
		ERRSTR(WSAEFAULT);	//	10014L
		ERRSTR(WSAEINVAL);	//	10022L
		ERRSTR(WSAEMFILE);	//	10024L
		ERRSTR(WSAEWOULDBLOCK);	//	10035L
		ERRSTR(WSAEINPROGRESS);	//	10036L
		ERRSTR(WSAEALREADY);	//	10037L
		ERRSTR(WSAENOTSOCK);	//	10038L
		ERRSTR(WSAEDESTADDRREQ);	//	10039L
		ERRSTR(WSAEMSGSIZE);	//	10040L
		ERRSTR(WSAEPROTOTYPE);	//	10041L
		ERRSTR(WSAENOPROTOOPT);	//	10042L
		ERRSTR(WSAEPROTONOSUPPORT);	//	10043L
		ERRSTR(WSAESOCKTNOSUPPORT);	//	10044L
		ERRSTR(WSAEOPNOTSUPP);	//	10045L
		ERRSTR(WSAEPFNOSUPPORT);	//	10046L
		ERRSTR(WSAEAFNOSUPPORT);	//	10047L
		ERRSTR(WSAEADDRINUSE);	//	10048L
		ERRSTR(WSAEADDRNOTAVAIL);	//	10049L
		ERRSTR(WSAENETDOWN);	//	10050L
		ERRSTR(WSAENETUNREACH);	//	10051L
		ERRSTR(WSAENETRESET);	//	10052L
		ERRSTR(WSAECONNABORTED);	//	10053L
		ERRSTR(WSAECONNRESET);	//	10054L
		ERRSTR(WSAENOBUFS);	//	10055L
		ERRSTR(WSAEISCONN);	//	10056L
		ERRSTR(WSAENOTCONN);	//	10057L
		ERRSTR(WSAESHUTDOWN);	//	10058L
		ERRSTR(WSAETOOMANYREFS);	//	10059L
		ERRSTR(WSAETIMEDOUT);	//	10060L
		ERRSTR(WSAECONNREFUSED);	//	10061L
		ERRSTR(WSAELOOP);	//	10062L
		ERRSTR(WSAENAMETOOLONG);	//	10063L
		ERRSTR(WSAEHOSTDOWN);	//	10064L
		ERRSTR(WSAEHOSTUNREACH);	//	10065L
		ERRSTR(WSAENOTEMPTY);	//	10066L
		ERRSTR(WSAEPROCLIM);	//	10067L
		ERRSTR(WSAEUSERS);	//	10068L
		ERRSTR(WSAEDQUOT);	//	10069L
		ERRSTR(WSAESTALE);	//	10070L
		ERRSTR(WSAEREMOTE);	//	10071L
		ERRSTR(WSASYSNOTREADY);	//	10091L
		ERRSTR(WSAVERNOTSUPPORTED);	//	10092L
		ERRSTR(WSANOTINITIALISED);	//	10093L
		ERRSTR(WSAEDISCON);	//	10101L
		ERRSTR(WSAENOMORE);	//	10102L
		ERRSTR(WSAECANCELLED);	//	10103L
		ERRSTR(WSAEINVALIDPROCTABLE);	//	10104L
		ERRSTR(WSAEINVALIDPROVIDER);	//	10105L
		ERRSTR(WSAEPROVIDERFAILEDINIT);	//	10106L
		ERRSTR(WSASYSCALLFAILURE);	//	10107L
		ERRSTR(WSASERVICE_NOT_FOUND);	//	10108L
		ERRSTR(WSATYPE_NOT_FOUND);	//	10109L
		ERRSTR(WSA_E_NO_MORE);	//	10110L
		ERRSTR(WSA_E_CANCELLED);	//	10111L
		ERRSTR(WSAEREFUSED);	//	10112L
		ERRSTR(WSAHOST_NOT_FOUND);	//	11001L
		ERRSTR(WSATRY_AGAIN);	//	11002L
		ERRSTR(WSANO_RECOVERY);	//	11003L
		ERRSTR(WSANO_DATA);	//	11004L
		ERRSTR(WSA_QOS_RECEIVERS);	//	11005L
		ERRSTR(WSA_QOS_SENDERS);	//	11006L
		ERRSTR(WSA_QOS_NO_SENDERS);	//	11007L
		ERRSTR(WSA_QOS_NO_RECEIVERS);	//	11008L
		ERRSTR(WSA_QOS_REQUEST_CONFIRMED);	//	11009L
		ERRSTR(WSA_QOS_ADMISSION_FAILURE);	//	11010L
		ERRSTR(WSA_QOS_POLICY_FAILURE);	//	11011L
		ERRSTR(WSA_QOS_BAD_STYLE);	//	11012L
		ERRSTR(WSA_QOS_BAD_OBJECT);	//	11013L
		ERRSTR(WSA_QOS_TRAFFIC_CTRL_ERROR);	//	11014L
		ERRSTR(WSA_QOS_GENERIC_ERROR);	//	11015L
		ERRSTR(WSA_QOS_ESERVICETYPE);	//	11016L
		ERRSTR(WSA_QOS_EFLOWSPEC);	//	11017L
		ERRSTR(WSA_QOS_EPROVSPECBUF);	//	11018L
		ERRSTR(WSA_QOS_EFILTERSTYLE);	//	11019L
		ERRSTR(WSA_QOS_EFILTERTYPE);	//	11020L
		ERRSTR(WSA_QOS_EFILTERCOUNT);	//	11021L
		ERRSTR(WSA_QOS_EOBJLENGTH);	//	11022L
		ERRSTR(WSA_QOS_EFLOWCOUNT);	//	11023L
		ERRSTR(WSA_QOS_EUNKOWNPSOBJ);	//	11024L
		ERRSTR(WSA_QOS_EPOLICYOBJ);	//	11025L
		ERRSTR(WSA_QOS_EFLOWDESC);	//	11026L
		ERRSTR(WSA_QOS_EPSFLOWSPEC);	//	11027L
		ERRSTR(WSA_QOS_EPSFILTERSPEC);	//	11028L
		ERRSTR(WSA_QOS_ESDMODEOBJ);	//	11029L
		ERRSTR(WSA_QOS_ESHAPERATEOBJ);	//	11030L
		ERRSTR(WSA_QOS_RESERVED_PETYPE);	//	11031L
		ERRSTR(WSA_SECURE_HOST_NOT_FOUND);	//	11032L
		ERRSTR(WSA_IPSEC_NAME_POLICY_ERROR);	//	11033L
		ERRSTR(ERROR_IPSEC_QM_POLICY_EXISTS);	//	13000L
		ERRSTR(ERROR_IPSEC_QM_POLICY_NOT_FOUND);	//	13001L
		ERRSTR(ERROR_IPSEC_QM_POLICY_IN_USE);	//	13002L
		ERRSTR(ERROR_IPSEC_MM_POLICY_EXISTS);	//	13003L
		ERRSTR(ERROR_IPSEC_MM_POLICY_NOT_FOUND);	//	13004L
		ERRSTR(ERROR_IPSEC_MM_POLICY_IN_USE);	//	13005L
		ERRSTR(ERROR_IPSEC_MM_FILTER_EXISTS);	//	13006L
		ERRSTR(ERROR_IPSEC_MM_FILTER_NOT_FOUND);	//	13007L
		ERRSTR(ERROR_IPSEC_TRANSPORT_FILTER_EXISTS);	//	13008L
		ERRSTR(ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND);	//	13009L
		ERRSTR(ERROR_IPSEC_MM_AUTH_EXISTS);	//	13010L
		ERRSTR(ERROR_IPSEC_MM_AUTH_NOT_FOUND);	//	13011L
		ERRSTR(ERROR_IPSEC_MM_AUTH_IN_USE);	//	13012L
		ERRSTR(ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND);	//	13013L
		ERRSTR(ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND);	//	13014L
		ERRSTR(ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND);	//	13015L
		ERRSTR(ERROR_IPSEC_TUNNEL_FILTER_EXISTS);	//	13016L
		ERRSTR(ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND);	//	13017L
		ERRSTR(ERROR_IPSEC_MM_FILTER_PENDING_DELETION);	//	13018L
		ERRSTR(ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION);	//	13019L
		ERRSTR(ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION);	//	13020L
		ERRSTR(ERROR_IPSEC_MM_POLICY_PENDING_DELETION);	//	13021L
		ERRSTR(ERROR_IPSEC_MM_AUTH_PENDING_DELETION);	//	13022L
		ERRSTR(ERROR_IPSEC_QM_POLICY_PENDING_DELETION);	//	13023L
		ERRSTR(WARNING_IPSEC_MM_POLICY_PRUNED);	//	13024L
		ERRSTR(WARNING_IPSEC_QM_POLICY_PRUNED);	//	13025L
		ERRSTR(ERROR_IPSEC_IKE_NEG_STATUS_BEGIN);	//	13800L
		ERRSTR(ERROR_IPSEC_IKE_AUTH_FAIL);	//	13801L
		ERRSTR(ERROR_IPSEC_IKE_ATTRIB_FAIL);	//	13802L
		ERRSTR(ERROR_IPSEC_IKE_NEGOTIATION_PENDING);	//	13803L
		ERRSTR(ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR);	//	13804L
		ERRSTR(ERROR_IPSEC_IKE_TIMED_OUT);	//	13805L
		ERRSTR(ERROR_IPSEC_IKE_NO_CERT);	//	13806L
		ERRSTR(ERROR_IPSEC_IKE_SA_DELETED);	//	13807L
		ERRSTR(ERROR_IPSEC_IKE_SA_REAPED);	//	13808L
		ERRSTR(ERROR_IPSEC_IKE_MM_ACQUIRE_DROP);	//	13809L
		ERRSTR(ERROR_IPSEC_IKE_QM_ACQUIRE_DROP);	//	13810L
		ERRSTR(ERROR_IPSEC_IKE_QUEUE_DROP_MM);	//	13811L
		ERRSTR(ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM);	//	13812L
		ERRSTR(ERROR_IPSEC_IKE_DROP_NO_RESPONSE);	//	13813L
		ERRSTR(ERROR_IPSEC_IKE_MM_DELAY_DROP);	//	13814L
		ERRSTR(ERROR_IPSEC_IKE_QM_DELAY_DROP);	//	13815L
		ERRSTR(ERROR_IPSEC_IKE_ERROR);	//	13816L
		ERRSTR(ERROR_IPSEC_IKE_CRL_FAILED);	//	13817L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_KEY_USAGE);	//	13818L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_CERT_TYPE);	//	13819L
		ERRSTR(ERROR_IPSEC_IKE_NO_PRIVATE_KEY);	//	13820L
		ERRSTR(ERROR_IPSEC_IKE_SIMULTANEOUS_REKEY);	//	13821L
		ERRSTR(ERROR_IPSEC_IKE_DH_FAIL);	//	13822L
		ERRSTR(ERROR_IPSEC_IKE_CRITICAL_PAYLOAD_NOT_RECOGNIZED);	//	13823L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_HEADER);	//	13824L
		ERRSTR(ERROR_IPSEC_IKE_NO_POLICY);	//	13825L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_SIGNATURE);	//	13826L
		ERRSTR(ERROR_IPSEC_IKE_KERBEROS_ERROR);	//	13827L
		ERRSTR(ERROR_IPSEC_IKE_NO_PUBLIC_KEY);	//	13828L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR);	//	13829L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_SA);	//	13830L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_PROP);	//	13831L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_TRANS);	//	13832L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_KE);	//	13833L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_ID);	//	13834L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_CERT);	//	13835L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ);	//	13836L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_HASH);	//	13837L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_SIG);	//	13838L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_NONCE);	//	13839L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY);	//	13840L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_DELETE);	//	13841L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR);	//	13842L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_PAYLOAD);	//	13843L
		ERRSTR(ERROR_IPSEC_IKE_LOAD_SOFT_SA);	//	13844L
		ERRSTR(ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN);	//	13845L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_COOKIE);	//	13846L
		ERRSTR(ERROR_IPSEC_IKE_NO_PEER_CERT);	//	13847L
		ERRSTR(ERROR_IPSEC_IKE_PEER_CRL_FAILED);	//	13848L
		ERRSTR(ERROR_IPSEC_IKE_POLICY_CHANGE);	//	13849L
		ERRSTR(ERROR_IPSEC_IKE_NO_MM_POLICY);	//	13850L
		ERRSTR(ERROR_IPSEC_IKE_NOTCBPRIV);	//	13851L
		ERRSTR(ERROR_IPSEC_IKE_SECLOADFAIL);	//	13852L
		ERRSTR(ERROR_IPSEC_IKE_FAILSSPINIT);	//	13853L
		ERRSTR(ERROR_IPSEC_IKE_FAILQUERYSSP);	//	13854L
		ERRSTR(ERROR_IPSEC_IKE_SRVACQFAIL);	//	13855L
		ERRSTR(ERROR_IPSEC_IKE_SRVQUERYCRED);	//	13856L
		ERRSTR(ERROR_IPSEC_IKE_GETSPIFAIL);	//	13857L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_FILTER);	//	13858L
		ERRSTR(ERROR_IPSEC_IKE_OUT_OF_MEMORY);	//	13859L
		ERRSTR(ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED);	//	13860L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_POLICY);	//	13861L
		ERRSTR(ERROR_IPSEC_IKE_UNKNOWN_DOI);	//	13862L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_SITUATION);	//	13863L
		ERRSTR(ERROR_IPSEC_IKE_DH_FAILURE);	//	13864L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_GROUP);	//	13865L
		ERRSTR(ERROR_IPSEC_IKE_ENCRYPT);	//	13866L
		ERRSTR(ERROR_IPSEC_IKE_DECRYPT);	//	13867L
		ERRSTR(ERROR_IPSEC_IKE_POLICY_MATCH);	//	13868L
		ERRSTR(ERROR_IPSEC_IKE_UNSUPPORTED_ID);	//	13869L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_HASH);	//	13870L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_HASH_ALG);	//	13871L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_HASH_SIZE);	//	13872L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG);	//	13873L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_AUTH_ALG);	//	13874L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_SIG);	//	13875L
		ERRSTR(ERROR_IPSEC_IKE_LOAD_FAILED);	//	13876L
		ERRSTR(ERROR_IPSEC_IKE_RPC_DELETE);	//	13877L
		ERRSTR(ERROR_IPSEC_IKE_BENIGN_REINIT);	//	13878L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY);	//	13879L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_MAJOR_VERSION);	//	13880L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN);	//	13881L
		ERRSTR(ERROR_IPSEC_IKE_MM_LIMIT);	//	13882L
		ERRSTR(ERROR_IPSEC_IKE_NEGOTIATION_DISABLED);	//	13883L
		ERRSTR(ERROR_IPSEC_IKE_QM_LIMIT);	//	13884L
		ERRSTR(ERROR_IPSEC_IKE_MM_EXPIRED);	//	13885L
		ERRSTR(ERROR_IPSEC_IKE_PEER_MM_ASSUMED_INVALID);	//	13886L
		ERRSTR(ERROR_IPSEC_IKE_CERT_CHAIN_POLICY_MISMATCH);	//	13887L
		ERRSTR(ERROR_IPSEC_IKE_UNEXPECTED_MESSAGE_ID);	//	13888L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_AUTH_PAYLOAD);	//	13889L
		ERRSTR(ERROR_IPSEC_IKE_DOS_COOKIE_SENT);	//	13890L
		ERRSTR(ERROR_IPSEC_IKE_SHUTTING_DOWN);	//	13891L
		ERRSTR(ERROR_IPSEC_IKE_CGA_AUTH_FAILED);	//	13892L
		ERRSTR(ERROR_IPSEC_IKE_PROCESS_ERR_NATOA);	//	13893L
		ERRSTR(ERROR_IPSEC_IKE_INVALID_MM_FOR_QM);	//	13894L
		ERRSTR(ERROR_IPSEC_IKE_QM_EXPIRED);	//	13895L
		ERRSTR(ERROR_IPSEC_IKE_TOO_MANY_FILTERS);	//	13896L
		ERRSTR(ERROR_IPSEC_IKE_NEG_STATUS_END);	//	13897L
		ERRSTR(ERROR_IPSEC_IKE_KILL_DUMMY_NAP_TUNNEL);	//	13898L
		ERRSTR(ERROR_IPSEC_IKE_INNER_IP_ASSIGNMENT_FAILURE);	//	13899L
		ERRSTR(ERROR_IPSEC_IKE_REQUIRE_CP_PAYLOAD_MISSING);	//	13900L
		ERRSTR(ERROR_IPSEC_KEY_MODULE_IMPERSONATION_NEGOTIATION_PENDING);	//	13901L
		ERRSTR(ERROR_IPSEC_IKE_COEXISTENCE_SUPPRESS);	//	13902L
		ERRSTR(ERROR_IPSEC_IKE_RATELIMIT_DROP);	//	13903L
		ERRSTR(ERROR_IPSEC_IKE_PEER_DOESNT_SUPPORT_MOBIKE);	//	13904L
		ERRSTR(ERROR_IPSEC_IKE_AUTHORIZATION_FAILURE);	//	13905L
		ERRSTR(ERROR_IPSEC_IKE_STRONG_CRED_AUTHORIZATION_FAILURE);	//	13906L
		ERRSTR(ERROR_IPSEC_IKE_AUTHORIZATION_FAILURE_WITH_OPTIONAL_RETRY);	//	13907L
		ERRSTR(ERROR_IPSEC_IKE_STRONG_CRED_AUTHORIZATION_AND_CERTMAP_FAILURE);	//	13908L
		ERRSTR(ERROR_IPSEC_IKE_NEG_STATUS_EXTENDED_END);	//	13909L
		ERRSTR(ERROR_IPSEC_BAD_SPI);	//	13910L
		ERRSTR(ERROR_IPSEC_SA_LIFETIME_EXPIRED);	//	13911L
		ERRSTR(ERROR_IPSEC_WRONG_SA);	//	13912L
		ERRSTR(ERROR_IPSEC_REPLAY_CHECK_FAILED);	//	13913L
		ERRSTR(ERROR_IPSEC_INVALID_PACKET);	//	13914L
		ERRSTR(ERROR_IPSEC_INTEGRITY_CHECK_FAILED);	//	13915L
		ERRSTR(ERROR_IPSEC_CLEAR_TEXT_DROP);	//	13916L
		ERRSTR(ERROR_IPSEC_AUTH_FIREWALL_DROP);	//	13917L
		ERRSTR(ERROR_IPSEC_THROTTLE_DROP);	//	13918L
		ERRSTR(ERROR_IPSEC_DOSP_BLOCK);	//	13925L
		ERRSTR(ERROR_IPSEC_DOSP_RECEIVED_MULTICAST);	//	13926L
		ERRSTR(ERROR_IPSEC_DOSP_INVALID_PACKET);	//	13927L
		ERRSTR(ERROR_IPSEC_DOSP_STATE_LOOKUP_FAILED);	//	13928L
		ERRSTR(ERROR_IPSEC_DOSP_MAX_ENTRIES);	//	13929L
		ERRSTR(ERROR_IPSEC_DOSP_KEYMOD_NOT_ALLOWED);	//	13930L
		ERRSTR(ERROR_IPSEC_DOSP_NOT_INSTALLED);	//	13931L
		ERRSTR(ERROR_IPSEC_DOSP_MAX_PER_IP_RATELIMIT_QUEUES);	//	13932L
		ERRSTR(ERROR_SXS_SECTION_NOT_FOUND);	//	14000L
		ERRSTR(ERROR_SXS_CANT_GEN_ACTCTX);	//	14001L
		ERRSTR(ERROR_SXS_INVALID_ACTCTXDATA_FORMAT);	//	14002L
		ERRSTR(ERROR_SXS_ASSEMBLY_NOT_FOUND);	//	14003L
		ERRSTR(ERROR_SXS_MANIFEST_FORMAT_ERROR);	//	14004L
		ERRSTR(ERROR_SXS_MANIFEST_PARSE_ERROR);	//	14005L
		ERRSTR(ERROR_SXS_ACTIVATION_CONTEXT_DISABLED);	//	14006L
		ERRSTR(ERROR_SXS_KEY_NOT_FOUND);	//	14007L
		ERRSTR(ERROR_SXS_VERSION_CONFLICT);	//	14008L
		ERRSTR(ERROR_SXS_WRONG_SECTION_TYPE);	//	14009L
		ERRSTR(ERROR_SXS_THREAD_QUERIES_DISABLED);	//	14010L
		ERRSTR(ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET);	//	14011L
		ERRSTR(ERROR_SXS_UNKNOWN_ENCODING_GROUP);	//	14012L
		ERRSTR(ERROR_SXS_UNKNOWN_ENCODING);	//	14013L
		ERRSTR(ERROR_SXS_INVALID_XML_NAMESPACE_URI);	//	14014L
		ERRSTR(ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED);	//	14015L
		ERRSTR(ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED);	//	14016L
		ERRSTR(ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);	//	14017L
		ERRSTR(ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE);	//	14018L
		ERRSTR(ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE);	//	14019L
		ERRSTR(ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT);	//	14020L
		ERRSTR(ERROR_SXS_DUPLICATE_DLL_NAME);	//	14021L
		ERRSTR(ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME);	//	14022L
		ERRSTR(ERROR_SXS_DUPLICATE_CLSID);	//	14023L
		ERRSTR(ERROR_SXS_DUPLICATE_IID);	//	14024L
		ERRSTR(ERROR_SXS_DUPLICATE_TLBID);	//	14025L
		ERRSTR(ERROR_SXS_DUPLICATE_PROGID);	//	14026L
		ERRSTR(ERROR_SXS_DUPLICATE_ASSEMBLY_NAME);	//	14027L
		ERRSTR(ERROR_SXS_FILE_HASH_MISMATCH);	//	14028L
		ERRSTR(ERROR_SXS_POLICY_PARSE_ERROR);	//	14029L
		ERRSTR(ERROR_SXS_XML_E_MISSINGQUOTE);	//	14030L
		ERRSTR(ERROR_SXS_XML_E_COMMENTSYNTAX);	//	14031L
		ERRSTR(ERROR_SXS_XML_E_BADSTARTNAMECHAR);	//	14032L
		ERRSTR(ERROR_SXS_XML_E_BADNAMECHAR);	//	14033L
		ERRSTR(ERROR_SXS_XML_E_BADCHARINSTRING);	//	14034L
		ERRSTR(ERROR_SXS_XML_E_XMLDECLSYNTAX);	//	14035L
		ERRSTR(ERROR_SXS_XML_E_BADCHARDATA);	//	14036L
		ERRSTR(ERROR_SXS_XML_E_MISSINGWHITESPACE);	//	14037L
		ERRSTR(ERROR_SXS_XML_E_EXPECTINGTAGEND);	//	14038L
		ERRSTR(ERROR_SXS_XML_E_MISSINGSEMICOLON);	//	14039L
		ERRSTR(ERROR_SXS_XML_E_UNBALANCEDPAREN);	//	14040L
		ERRSTR(ERROR_SXS_XML_E_INTERNALERROR);	//	14041L
		ERRSTR(ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE);	//	14042L
		ERRSTR(ERROR_SXS_XML_E_INCOMPLETE_ENCODING);	//	14043L
		ERRSTR(ERROR_SXS_XML_E_MISSING_PAREN);	//	14044L
		ERRSTR(ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE);	//	14045L
		ERRSTR(ERROR_SXS_XML_E_MULTIPLE_COLONS);	//	14046L
		ERRSTR(ERROR_SXS_XML_E_INVALID_DECIMAL);	//	14047L
		ERRSTR(ERROR_SXS_XML_E_INVALID_HEXIDECIMAL);	//	14048L
		ERRSTR(ERROR_SXS_XML_E_INVALID_UNICODE);	//	14049L
		ERRSTR(ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK);	//	14050L
		ERRSTR(ERROR_SXS_XML_E_UNEXPECTEDENDTAG);	//	14051L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDTAG);	//	14052L
		ERRSTR(ERROR_SXS_XML_E_DUPLICATEATTRIBUTE);	//	14053L
		ERRSTR(ERROR_SXS_XML_E_MULTIPLEROOTS);	//	14054L
		ERRSTR(ERROR_SXS_XML_E_INVALIDATROOTLEVEL);	//	14055L
		ERRSTR(ERROR_SXS_XML_E_BADXMLDECL);	//	14056L
		ERRSTR(ERROR_SXS_XML_E_MISSINGROOT);	//	14057L
		ERRSTR(ERROR_SXS_XML_E_UNEXPECTEDEOF);	//	14058L
		ERRSTR(ERROR_SXS_XML_E_BADPEREFINSUBSET);	//	14059L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDSTARTTAG);	//	14060L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDENDTAG);	//	14061L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDSTRING);	//	14062L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDCOMMENT);	//	14063L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDDECL);	//	14064L
		ERRSTR(ERROR_SXS_XML_E_UNCLOSEDCDATA);	//	14065L
		ERRSTR(ERROR_SXS_XML_E_RESERVEDNAMESPACE);	//	14066L
		ERRSTR(ERROR_SXS_XML_E_INVALIDENCODING);	//	14067L
		ERRSTR(ERROR_SXS_XML_E_INVALIDSWITCH);	//	14068L
		ERRSTR(ERROR_SXS_XML_E_BADXMLCASE);	//	14069L
		ERRSTR(ERROR_SXS_XML_E_INVALID_STANDALONE);	//	14070L
		ERRSTR(ERROR_SXS_XML_E_UNEXPECTED_STANDALONE);	//	14071L
		ERRSTR(ERROR_SXS_XML_E_INVALID_VERSION);	//	14072L
		ERRSTR(ERROR_SXS_XML_E_MISSINGEQUALS);	//	14073L
		ERRSTR(ERROR_SXS_PROTECTION_RECOVERY_FAILED);	//	14074L
		ERRSTR(ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT);	//	14075L
		ERRSTR(ERROR_SXS_PROTECTION_CATALOG_NOT_VALID);	//	14076L
		ERRSTR(ERROR_SXS_UNTRANSLATABLE_HRESULT);	//	14077L
		ERRSTR(ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING);	//	14078L
		ERRSTR(ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);	//	14079L
		ERRSTR(ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME);	//	14080L
		ERRSTR(ERROR_SXS_ASSEMBLY_MISSING);	//	14081L
		ERRSTR(ERROR_SXS_CORRUPT_ACTIVATION_STACK);	//	14082L
		ERRSTR(ERROR_SXS_CORRUPTION);	//	14083L
		ERRSTR(ERROR_SXS_EARLY_DEACTIVATION);	//	14084L
		ERRSTR(ERROR_SXS_INVALID_DEACTIVATION);	//	14085L
		ERRSTR(ERROR_SXS_MULTIPLE_DEACTIVATION);	//	14086L
		ERRSTR(ERROR_SXS_PROCESS_TERMINATION_REQUESTED);	//	14087L
		ERRSTR(ERROR_SXS_RELEASE_ACTIVATION_CONTEXT);	//	14088L
		ERRSTR(ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY);	//	14089L
		ERRSTR(ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE);	//	14090L
		ERRSTR(ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME);	//	14091L
		ERRSTR(ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE);	//	14092L
		ERRSTR(ERROR_SXS_IDENTITY_PARSE_ERROR);	//	14093L
		ERRSTR(ERROR_MALFORMED_SUBSTITUTION_STRING);	//	14094L
		ERRSTR(ERROR_SXS_INCORRECT_PUBLIC_KEY_TOKEN);	//	14095L
		ERRSTR(ERROR_UNMAPPED_SUBSTITUTION_STRING);	//	14096L
		ERRSTR(ERROR_SXS_ASSEMBLY_NOT_LOCKED);	//	14097L
		ERRSTR(ERROR_SXS_COMPONENT_STORE_CORRUPT);	//	14098L
		ERRSTR(ERROR_ADVANCED_INSTALLER_FAILED);	//	14099L
		ERRSTR(ERROR_XML_ENCODING_MISMATCH);	//	14100L
		ERRSTR(ERROR_SXS_MANIFEST_IDENTITY_SAME_BUT_CONTENTS_DIFFERENT);	//	14101L
		ERRSTR(ERROR_SXS_IDENTITIES_DIFFERENT);	//	14102L
		ERRSTR(ERROR_SXS_ASSEMBLY_IS_NOT_A_DEPLOYMENT);	//	14103L
		ERRSTR(ERROR_SXS_FILE_NOT_PART_OF_ASSEMBLY);	//	14104L
		ERRSTR(ERROR_SXS_MANIFEST_TOO_BIG);	//	14105L
		ERRSTR(ERROR_SXS_SETTING_NOT_REGISTERED);	//	14106L
		ERRSTR(ERROR_SXS_TRANSACTION_CLOSURE_INCOMPLETE);	//	14107L
		ERRSTR(ERROR_SMI_PRIMITIVE_INSTALLER_FAILED);	//	14108L
		ERRSTR(ERROR_GENERIC_COMMAND_FAILED);	//	14109L
		ERRSTR(ERROR_SXS_FILE_HASH_MISSING);	//	14110L
		ERRSTR(ERROR_EVT_INVALID_CHANNEL_PATH);	//	15000L
		ERRSTR(ERROR_EVT_INVALID_QUERY);	//	15001L
		ERRSTR(ERROR_EVT_PUBLISHER_METADATA_NOT_FOUND);	//	15002L
		ERRSTR(ERROR_EVT_EVENT_TEMPLATE_NOT_FOUND);	//	15003L
		ERRSTR(ERROR_EVT_INVALID_PUBLISHER_NAME);	//	15004L
		ERRSTR(ERROR_EVT_INVALID_EVENT_DATA);	//	15005L
		ERRSTR(ERROR_EVT_CHANNEL_NOT_FOUND);	//	15007L
		ERRSTR(ERROR_EVT_MALFORMED_XML_TEXT);	//	15008L
		ERRSTR(ERROR_EVT_SUBSCRIPTION_TO_DIRECT_CHANNEL);	//	15009L
		ERRSTR(ERROR_EVT_CONFIGURATION_ERROR);	//	15010L
		ERRSTR(ERROR_EVT_QUERY_RESULT_STALE);	//	15011L
		ERRSTR(ERROR_EVT_QUERY_RESULT_INVALID_POSITION);	//	15012L
		ERRSTR(ERROR_EVT_NON_VALIDATING_MSXML);	//	15013L
		ERRSTR(ERROR_EVT_FILTER_ALREADYSCOPED);	//	15014L
		ERRSTR(ERROR_EVT_FILTER_NOTELTSET);	//	15015L
		ERRSTR(ERROR_EVT_FILTER_INVARG);	//	15016L
		ERRSTR(ERROR_EVT_FILTER_INVTEST);	//	15017L
		ERRSTR(ERROR_EVT_FILTER_INVTYPE);	//	15018L
		ERRSTR(ERROR_EVT_FILTER_PARSEERR);	//	15019L
		ERRSTR(ERROR_EVT_FILTER_UNSUPPORTEDOP);	//	15020L
		ERRSTR(ERROR_EVT_FILTER_UNEXPECTEDTOKEN);	//	15021L
		ERRSTR(ERROR_EVT_INVALID_OPERATION_OVER_ENABLED_DIRECT_CHANNEL);	//	15022L
		ERRSTR(ERROR_EVT_INVALID_CHANNEL_PROPERTY_VALUE);	//	15023L
		ERRSTR(ERROR_EVT_INVALID_PUBLISHER_PROPERTY_VALUE);	//	15024L
		ERRSTR(ERROR_EVT_CHANNEL_CANNOT_ACTIVATE);	//	15025L
		ERRSTR(ERROR_EVT_FILTER_TOO_COMPLEX);	//	15026L
		ERRSTR(ERROR_EVT_MESSAGE_NOT_FOUND);	//	15027L
		ERRSTR(ERROR_EVT_MESSAGE_ID_NOT_FOUND);	//	15028L
		ERRSTR(ERROR_EVT_UNRESOLVED_VALUE_INSERT);	//	15029L
		ERRSTR(ERROR_EVT_UNRESOLVED_PARAMETER_INSERT);	//	15030L
		ERRSTR(ERROR_EVT_MAX_INSERTS_REACHED);	//	15031L
		ERRSTR(ERROR_EVT_EVENT_DEFINITION_NOT_FOUND);	//	15032L
		ERRSTR(ERROR_EVT_MESSAGE_LOCALE_NOT_FOUND);	//	15033L
		ERRSTR(ERROR_EVT_VERSION_TOO_OLD);	//	15034L
		ERRSTR(ERROR_EVT_VERSION_TOO_NEW);	//	15035L
		ERRSTR(ERROR_EVT_CANNOT_OPEN_CHANNEL_OF_QUERY);	//	15036L
		ERRSTR(ERROR_EVT_PUBLISHER_DISABLED);	//	15037L
		ERRSTR(ERROR_EVT_FILTER_OUT_OF_RANGE);	//	15038L
		ERRSTR(ERROR_EC_SUBSCRIPTION_CANNOT_ACTIVATE);	//	15080L
		ERRSTR(ERROR_EC_LOG_DISABLED);	//	15081L
		ERRSTR(ERROR_EC_CIRCULAR_FORWARDING);	//	15082L
		ERRSTR(ERROR_EC_CREDSTORE_FULL);	//	15083L
		ERRSTR(ERROR_EC_CRED_NOT_FOUND);	//	15084L
		ERRSTR(ERROR_EC_NO_ACTIVE_CHANNEL);	//	15085L
		ERRSTR(ERROR_MUI_FILE_NOT_FOUND);	//	15100L    
		ERRSTR(ERROR_MUI_INVALID_FILE);	//	15101L    
		ERRSTR(ERROR_MUI_INVALID_RC_CONFIG);	//	15102L    
		ERRSTR(ERROR_MUI_INVALID_LOCALE_NAME);	//	15103L    
		ERRSTR(ERROR_MUI_INVALID_ULTIMATEFALLBACK_NAME);	//	15104L    
		ERRSTR(ERROR_MUI_FILE_NOT_LOADED);	//	15105L    
		ERRSTR(ERROR_RESOURCE_ENUM_USER_STOP);	//	15106L
		ERRSTR(ERROR_MUI_INTLSETTINGS_UILANG_NOT_INSTALLED);	//	15107L
		ERRSTR(ERROR_MUI_INTLSETTINGS_INVALID_LOCALE_NAME);	//	15108L
		ERRSTR(ERROR_MRM_RUNTIME_NO_DEFAULT_OR_NEUTRAL_RESOURCE);	//	15110L
		ERRSTR(ERROR_MRM_INVALID_PRICONFIG);	//	15111L
		ERRSTR(ERROR_MRM_INVALID_FILE_TYPE);	//	15112L
		ERRSTR(ERROR_MRM_UNKNOWN_QUALIFIER);	//	15113L
		ERRSTR(ERROR_MRM_INVALID_QUALIFIER_VALUE);	//	15114L
		ERRSTR(ERROR_MRM_NO_CANDIDATE);	//	15115L
		ERRSTR(ERROR_MRM_NO_MATCH_OR_DEFAULT_CANDIDATE);	//	15116L
		ERRSTR(ERROR_MRM_RESOURCE_TYPE_MISMATCH);	//	15117L
		ERRSTR(ERROR_MRM_DUPLICATE_MAP_NAME);	//	15118L
		ERRSTR(ERROR_MRM_DUPLICATE_ENTRY);	//	15119L
		ERRSTR(ERROR_MRM_INVALID_RESOURCE_IDENTIFIER);	//	15120L
		ERRSTR(ERROR_MRM_FILEPATH_TOO_LONG);	//	15121L
		ERRSTR(ERROR_MRM_UNSUPPORTED_DIRECTORY_TYPE);	//	15122L
		ERRSTR(ERROR_MRM_INVALID_PRI_FILE);	//	15126L
		ERRSTR(ERROR_MRM_NAMED_RESOURCE_NOT_FOUND);	//	15127L
		ERRSTR(ERROR_MRM_MAP_NOT_FOUND);	//	15135L
		ERRSTR(ERROR_MRM_UNSUPPORTED_PROFILE_TYPE);	//	15136L
		ERRSTR(ERROR_MRM_INVALID_QUALIFIER_OPERATOR);	//	15137L
		ERRSTR(ERROR_MRM_INDETERMINATE_QUALIFIER_VALUE);	//	15138L
		ERRSTR(ERROR_MRM_AUTOMERGE_ENABLED);	//	15139L
		ERRSTR(ERROR_MRM_TOO_MANY_RESOURCES);	//	15140L
		ERRSTR(ERROR_MRM_UNSUPPORTED_FILE_TYPE_FOR_MERGE);	//	15141L
		ERRSTR(ERROR_MRM_UNSUPPORTED_FILE_TYPE_FOR_LOAD_UNLOAD_PRI_FILE);	//	15142L
		ERRSTR(ERROR_MRM_NO_CURRENT_VIEW_ON_THREAD);	//	15143L
		ERRSTR(ERROR_DIFFERENT_PROFILE_RESOURCE_MANAGER_EXIST);	//	15144L
		ERRSTR(ERROR_OPERATION_NOT_ALLOWED_FROM_SYSTEM_COMPONENT);	//	15145L
		ERRSTR(ERROR_MRM_DIRECT_REF_TO_NON_DEFAULT_RESOURCE);	//	15146L
		ERRSTR(ERROR_MRM_GENERATION_COUNT_MISMATCH);	//	15147L
#if CPP17_OR_LATER
		ERRSTR(ERROR_SXS_DUPLICATE_ACTIVATABLE_CLASS);	//	14111L
		ERRSTR(ERROR_PRI_MERGE_VERSION_MISMATCH);	//	15148L
		ERRSTR(ERROR_PRI_MERGE_MISSING_SCHEMA);	//	15149L
		ERRSTR(ERROR_PRI_MERGE_LOAD_FILE_FAILED);	//	15150L
		ERRSTR(ERROR_PRI_MERGE_ADD_FILE_FAILED);	//	15151L
		ERRSTR(ERROR_PRI_MERGE_WRITE_FILE_FAILED);	//	15152L
		ERRSTR(ERROR_PRI_MERGE_MULTIPLE_PACKAGE_FAMILIES_NOT_ALLOWED);	//	15153L
		ERRSTR(ERROR_PRI_MERGE_MULTIPLE_MAIN_PACKAGES_NOT_ALLOWED);	//	15154L
		ERRSTR(ERROR_PRI_MERGE_BUNDLE_PACKAGES_NOT_ALLOWED);	//	15155L
		ERRSTR(ERROR_PRI_MERGE_MAIN_PACKAGE_REQUIRED);	//	15156L
		ERRSTR(ERROR_PRI_MERGE_RESOURCE_PACKAGE_REQUIRED);	//	15157L
		ERRSTR(ERROR_PRI_MERGE_INVALID_FILE_NAME);	//	15158L
		ERRSTR(ERROR_MRM_PACKAGE_NOT_FOUND);	//	15159L
		ERRSTR(ERROR_MRM_MISSING_DEFAULT_LANGUAGE);	//	15160L
#endif
		ERRSTR(ERROR_MCA_INVALID_CAPABILITIES_STRING);	//	15200L
		ERRSTR(ERROR_MCA_INVALID_VCP_VERSION);	//	15201L
		ERRSTR(ERROR_MCA_MONITOR_VIOLATES_MCCS_SPECIFICATION);	//	15202L
		ERRSTR(ERROR_MCA_MCCS_VERSION_MISMATCH);	//	15203L
		ERRSTR(ERROR_MCA_UNSUPPORTED_MCCS_VERSION);	//	15204L
		ERRSTR(ERROR_MCA_INTERNAL_ERROR);	//	15205L
		ERRSTR(ERROR_MCA_INVALID_TECHNOLOGY_TYPE_RETURNED);	//	15206L
		ERRSTR(ERROR_MCA_UNSUPPORTED_COLOR_TEMPERATURE);	//	15207L
		ERRSTR(ERROR_AMBIGUOUS_SYSTEM_DEVICE);	//	15250L
		ERRSTR(ERROR_SYSTEM_DEVICE_NOT_FOUND);	//	15299L
		ERRSTR(ERROR_HASH_NOT_SUPPORTED);	//	15300L
		ERRSTR(ERROR_HASH_NOT_PRESENT);	//	15301L
		ERRSTR(ERROR_SECONDARY_IC_PROVIDER_NOT_REGISTERED);	//	15321L
		ERRSTR(ERROR_GPIO_CLIENT_INFORMATION_INVALID);	//	15322L
		ERRSTR(ERROR_GPIO_VERSION_NOT_SUPPORTED);	//	15323L
		ERRSTR(ERROR_GPIO_INVALID_REGISTRATION_PACKET);	//	15324L
		ERRSTR(ERROR_GPIO_OPERATION_DENIED);	//	15325L
		ERRSTR(ERROR_GPIO_INCOMPATIBLE_CONNECT_MODE);	//	15326L
		ERRSTR(ERROR_GPIO_INTERRUPT_ALREADY_UNMASKED);	//	15327L
		ERRSTR(ERROR_CANNOT_SWITCH_RUNLEVEL);	//	15400L
		ERRSTR(ERROR_INVALID_RUNLEVEL_SETTING);	//	15401L
		ERRSTR(ERROR_RUNLEVEL_SWITCH_TIMEOUT);	//	15402L
		ERRSTR(ERROR_RUNLEVEL_SWITCH_AGENT_TIMEOUT);	//	15403L
		ERRSTR(ERROR_RUNLEVEL_SWITCH_IN_PROGRESS);	//	15404L
		ERRSTR(ERROR_SERVICES_FAILED_AUTOSTART);	//	15405L
		ERRSTR(ERROR_COM_TASK_STOP_PENDING);	//	15501L
		ERRSTR(ERROR_INSTALL_OPEN_PACKAGE_FAILED);	//	15600L
		ERRSTR(ERROR_INSTALL_PACKAGE_NOT_FOUND);	//	15601L
		ERRSTR(ERROR_INSTALL_INVALID_PACKAGE);	//	15602L
		ERRSTR(ERROR_INSTALL_RESOLVE_DEPENDENCY_FAILED);	//	15603L
		ERRSTR(ERROR_INSTALL_OUT_OF_DISK_SPACE);	//	15604L
		ERRSTR(ERROR_INSTALL_NETWORK_FAILURE);	//	15605L
		ERRSTR(ERROR_INSTALL_REGISTRATION_FAILURE);	//	15606L
		ERRSTR(ERROR_INSTALL_DEREGISTRATION_FAILURE);	//	15607L
		ERRSTR(ERROR_INSTALL_CANCEL);	//	15608L
		ERRSTR(ERROR_INSTALL_FAILED);	//	15609L
		ERRSTR(ERROR_REMOVE_FAILED);	//	15610L
		ERRSTR(ERROR_PACKAGE_ALREADY_EXISTS);	//	15611L
		ERRSTR(ERROR_NEEDS_REMEDIATION);	//	15612L
		ERRSTR(ERROR_INSTALL_PREREQUISITE_FAILED);	//	15613L
		ERRSTR(ERROR_PACKAGE_REPOSITORY_CORRUPTED);	//	15614L
		ERRSTR(ERROR_INSTALL_POLICY_FAILURE);	//	15615L
		ERRSTR(ERROR_PACKAGE_UPDATING);	//	15616L
		ERRSTR(ERROR_DEPLOYMENT_BLOCKED_BY_POLICY);	//	15617L
		ERRSTR(ERROR_PACKAGES_IN_USE);	//	15618L
		ERRSTR(ERROR_RECOVERY_FILE_CORRUPT);	//	15619L
		ERRSTR(ERROR_INVALID_STAGED_SIGNATURE);	//	15620L
		ERRSTR(ERROR_DELETING_EXISTING_APPLICATIONDATA_STORE_FAILED);	//	15621L
		ERRSTR(ERROR_INSTALL_PACKAGE_DOWNGRADE);	//	15622L
		ERRSTR(ERROR_SYSTEM_NEEDS_REMEDIATION);	//	15623L
		ERRSTR(ERROR_APPX_INTEGRITY_FAILURE_CLR_NGEN);	//	15624L
		ERRSTR(ERROR_RESILIENCY_FILE_CORRUPT);	//	15625L
		ERRSTR(ERROR_INSTALL_FIREWALL_SERVICE_NOT_RUNNING);	//	15626L
#if CPP17_OR_LATER
		ERRSTR(ERROR_PACKAGE_MOVE_FAILED);	//	15627L
		ERRSTR(ERROR_INSTALL_VOLUME_NOT_EMPTY);	//	15628L
		ERRSTR(ERROR_INSTALL_VOLUME_OFFLINE);	//	15629L
		ERRSTR(ERROR_INSTALL_VOLUME_CORRUPT);	//	15630L
		ERRSTR(ERROR_NEEDS_REGISTRATION);	//	15631L
		ERRSTR(ERROR_INSTALL_WRONG_PROCESSOR_ARCHITECTURE);	//	15632L
		ERRSTR(ERROR_DEV_SIDELOAD_LIMIT_EXCEEDED);	//	15633L
		ERRSTR(ERROR_INSTALL_OPTIONAL_PACKAGE_REQUIRES_MAIN_PACKAGE);	//	15634L
		ERRSTR(ERROR_PACKAGE_NOT_SUPPORTED_ON_FILESYSTEM);	//	15635L
		ERRSTR(ERROR_PACKAGE_MOVE_BLOCKED_BY_STREAMING);	//	15636L
		ERRSTR(ERROR_INSTALL_OPTIONAL_PACKAGE_APPLICATIONID_NOT_UNIQUE);	//	15637L
		ERRSTR(ERROR_PACKAGE_STAGING_ONHOLD);	//	15638L
		ERRSTR(ERROR_INSTALL_INVALID_RELATED_SET_UPDATE);	//	15639L
		ERRSTR(ERROR_INSTALL_OPTIONAL_PACKAGE_REQUIRES_MAIN_PACKAGE_FULLTRUST_CAPABILITY);	//	15640L
		ERRSTR(ERROR_DEPLOYMENT_BLOCKED_BY_USER_LOG_OFF);	//	15641L
		ERRSTR(ERROR_PROVISION_OPTIONAL_PACKAGE_REQUIRES_MAIN_PACKAGE_PROVISIONED);	//	15642L
		ERRSTR(ERROR_PACKAGES_REPUTATION_CHECK_FAILED);	//	15643L
		ERRSTR(ERROR_PACKAGES_REPUTATION_CHECK_TIMEDOUT);	//	15644L
		ERRSTR(ERROR_DEPLOYMENT_OPTION_NOT_SUPPORTED);	//	15645L
		ERRSTR(ERROR_APPINSTALLER_ACTIVATION_BLOCKED);	//	15646L
		ERRSTR(ERROR_REGISTRATION_FROM_REMOTE_DRIVE_NOT_SUPPORTED);	//	15647L
		ERRSTR(ERROR_APPX_RAW_DATA_WRITE_FAILED);	//	15648L
		ERRSTR(ERROR_DEPLOYMENT_BLOCKED_BY_VOLUME_POLICY_PACKAGE);	//	15649L
		ERRSTR(ERROR_DEPLOYMENT_BLOCKED_BY_VOLUME_POLICY_MACHINE);	//	15650L
		ERRSTR(ERROR_DEPLOYMENT_BLOCKED_BY_PROFILE_POLICY);	//	15651L
		ERRSTR(ERROR_DEPLOYMENT_FAILED_CONFLICTING_MUTABLE_PACKAGE_DIRECTORY);	//	15652L
		ERRSTR(ERROR_SINGLETON_RESOURCE_INSTALLED_IN_ACTIVE_USER);	//	15653L
		ERRSTR(ERROR_DIFFERENT_VERSION_OF_PACKAGED_SERVICE_INSTALLED);	//	15654L
		ERRSTR(ERROR_SERVICE_EXISTS_AS_NON_PACKAGED_SERVICE);	//	15655L
		ERRSTR(ERROR_PACKAGED_SERVICE_REQUIRES_ADMIN_PRIVILEGES);	//	15656L
		ERRSTR(ERROR_REDIRECTION_TO_DEFAULT_ACCOUNT_NOT_ALLOWED);	//	15657L
		ERRSTR(ERROR_PACKAGE_LACKS_CAPABILITY_TO_DEPLOY_ON_HOST);	//	15658L
		ERRSTR(ERROR_UNSIGNED_PACKAGE_INVALID_CONTENT);	//	15659L
		ERRSTR(ERROR_UNSIGNED_PACKAGE_INVALID_PUBLISHER_NAMESPACE);	//	15660L
		ERRSTR(ERROR_SIGNED_PACKAGE_INVALID_PUBLISHER_NAMESPACE);	//	15661L
		ERRSTR(ERROR_PACKAGE_EXTERNAL_LOCATION_NOT_ALLOWED);	//	15662L
		ERRSTR(ERROR_INSTALL_FULLTRUST_HOSTRUNTIME_REQUIRES_MAIN_PACKAGE_FULLTRUST_CAPABILITY);	//	15663L

		ERRSTR(APPMODEL_ERROR_PACKAGE_NOT_AVAILABLE);	//	15706L
		ERRSTR(APPMODEL_ERROR_NO_MUTABLE_DIRECTORY);	//	15707L
#endif
		ERRSTR(APPMODEL_ERROR_NO_PACKAGE);	//	15700L
		ERRSTR(APPMODEL_ERROR_PACKAGE_RUNTIME_CORRUPT);	//	15701L
		ERRSTR(APPMODEL_ERROR_PACKAGE_IDENTITY_CORRUPT);	//	15702L
		ERRSTR(APPMODEL_ERROR_NO_APPLICATION);	//	15703L
		ERRSTR(APPMODEL_ERROR_DYNAMIC_PROPERTY_READ_FAILED);	//	15704L
		ERRSTR(APPMODEL_ERROR_DYNAMIC_PROPERTY_INVALID);	//	15705L
		ERRSTR(ERROR_STATE_LOAD_STORE_FAILED);	//	15800L
		ERRSTR(ERROR_STATE_GET_VERSION_FAILED);	//	15801L
		ERRSTR(ERROR_STATE_SET_VERSION_FAILED);	//	15802L
		ERRSTR(ERROR_STATE_STRUCTURED_RESET_FAILED);	//	15803L
		ERRSTR(ERROR_STATE_OPEN_CONTAINER_FAILED);	//	15804L
		ERRSTR(ERROR_STATE_CREATE_CONTAINER_FAILED);	//	15805L
		ERRSTR(ERROR_STATE_DELETE_CONTAINER_FAILED);	//	15806L
		ERRSTR(ERROR_STATE_READ_SETTING_FAILED);	//	15807L
		ERRSTR(ERROR_STATE_WRITE_SETTING_FAILED);	//	15808L
		ERRSTR(ERROR_STATE_DELETE_SETTING_FAILED);	//	15809L
		ERRSTR(ERROR_STATE_QUERY_SETTING_FAILED);	//	15810L
		ERRSTR(ERROR_STATE_READ_COMPOSITE_SETTING_FAILED);	//	15811L
		ERRSTR(ERROR_STATE_WRITE_COMPOSITE_SETTING_FAILED);	//	15812L
		ERRSTR(ERROR_STATE_ENUMERATE_CONTAINER_FAILED);	//	15813L
		ERRSTR(ERROR_STATE_ENUMERATE_SETTINGS_FAILED);	//	15814L
		ERRSTR(ERROR_STATE_COMPOSITE_SETTING_VALUE_SIZE_LIMIT_EXCEEDED);	//	15815L
		ERRSTR(ERROR_STATE_SETTING_VALUE_SIZE_LIMIT_EXCEEDED);	//	15816L
		ERRSTR(ERROR_STATE_SETTING_NAME_SIZE_LIMIT_EXCEEDED);	//	15817L
		ERRSTR(ERROR_STATE_CONTAINER_NAME_SIZE_LIMIT_EXCEEDED);	//	15818L
		ERRSTR(ERROR_API_UNAVAILABLE);	//	15841L
		ERRSTR(STORE_ERROR_UNLICENSED);	//	15861L
		ERRSTR(STORE_ERROR_UNLICENSED_USER);	//	15862L
		ERRSTR(STORE_ERROR_PENDING_COM_TRANSACTION);	//	15863L
		ERRSTR(STORE_ERROR_LICENSE_REVOKED);	//	15864L

		//#define WINHTTP_ERROR_BASE                     12000
		///?주의: HTTP STATUS 와는 다르다. 그것은 UcHttpStatusStr 참조. 예: HTTP_STATUS_OK(200)
		ERRSTR(ERROR_WINHTTP_OUT_OF_HANDLES);// (12001)
		ERRSTR(ERROR_WINHTTP_TIMEOUT);// (12002)
		ERRSTR(ERROR_WINHTTP_INTERNAL_ERROR);// (12004)
		ERRSTR(ERROR_WINHTTP_INVALID_URL);// (12005)
		ERRSTR(ERROR_WINHTTP_UNRECOGNIZED_SCHEME);// (12006)
		ERRSTR(ERROR_WINHTTP_NAME_NOT_RESOLVED);// (12007)
		ERRSTR(ERROR_WINHTTP_INVALID_OPTION);// (12009)
		ERRSTR(ERROR_WINHTTP_OPTION_NOT_SETTABLE);// (12011)
		ERRSTR(ERROR_WINHTTP_SHUTDOWN);// (12012)
		ERRSTR(ERROR_WINHTTP_LOGIN_FAILURE);// (12015)
		ERRSTR(ERROR_WINHTTP_OPERATION_CANCELLED);// (12017)
		ERRSTR(ERROR_WINHTTP_INCORRECT_HANDLE_TYPE);// (12018)
		ERRSTR(ERROR_WINHTTP_INCORRECT_HANDLE_STATE);// (12019)
		ERRSTR(ERROR_WINHTTP_CANNOT_CONNECT);// (12029)
		ERRSTR(ERROR_WINHTTP_CONNECTION_ERROR);// (12030)
		ERRSTR(ERROR_WINHTTP_RESEND_REQUEST);// (12032)
		ERRSTR(ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED);// (12044)
		ERRSTR(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);// (12100)
		ERRSTR(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);// (12101)
		ERRSTR(ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND);// (12102)
		ERRSTR(ERROR_WINHTTP_CANNOT_CALL_AFTER_OPEN);// (12103)
		ERRSTR(ERROR_WINHTTP_HEADER_NOT_FOUND);// (12150)
		ERRSTR(ERROR_WINHTTP_INVALID_SERVER_RESPONSE);// (12152)
		ERRSTR(ERROR_WINHTTP_INVALID_HEADER);// (12153)
		ERRSTR(ERROR_WINHTTP_INVALID_QUERY_REQUEST);// (12154)
		ERRSTR(ERROR_WINHTTP_HEADER_ALREADY_EXISTS);// (12155)
		ERRSTR(ERROR_WINHTTP_REDIRECT_FAILED);// (12156)
		ERRSTR(ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR);// (12178)
		ERRSTR(ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT);// (12166)
		ERRSTR(ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT);// (12167)
		ERRSTR(ERROR_WINHTTP_UNHANDLED_SCRIPT_TYPE);// (12176)
		ERRSTR(ERROR_WINHTTP_SCRIPT_EXECUTION_ERROR);// (12177)
		ERRSTR(ERROR_WINHTTP_NOT_INITIALIZED);// (12172)
		ERRSTR(ERROR_WINHTTP_SECURE_FAILURE);// (12175)
		ERRSTR(ERROR_WINHTTP_SECURE_CERT_DATE_INVALID);// (12037)
		ERRSTR(ERROR_WINHTTP_SECURE_CERT_CN_INVALID);// (12038)
		ERRSTR(ERROR_WINHTTP_SECURE_INVALID_CA);// (12045)
		ERRSTR(ERROR_WINHTTP_SECURE_CERT_REV_FAILED);// (12057)
		ERRSTR(ERROR_WINHTTP_SECURE_CHANNEL_ERROR);// (12157)
		ERRSTR(ERROR_WINHTTP_SECURE_INVALID_CERT);// (12169)
		ERRSTR(ERROR_WINHTTP_SECURE_CERT_REVOKED);// (12170)
		ERRSTR(ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE);// (12179)
		ERRSTR(ERROR_WINHTTP_AUTODETECTION_FAILED);// (12180)
		ERRSTR(ERROR_WINHTTP_HEADER_COUNT_EXCEEDED);// (12181)
		ERRSTR(ERROR_WINHTTP_HEADER_SIZE_OVERFLOW);// (12182)
		ERRSTR(ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW);// (12183)
		ERRSTR(ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW);// (12184)
		ERRSTR(ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY);// (12185)
		ERRSTR(ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY);// (12186)
#if CPP17_OR_LATER
		ERRSTR(ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED_PROXY);// (12187)
		ERRSTR(ERROR_WINHTTP_SECURE_FAILURE_PROXY);// (12188)
		ERRSTR(ERROR_WINHTTP_RESERVED_189);// (12189)
		ERRSTR(ERROR_WINHTTP_HTTP_PROTOCOL_MISMATCH);// (12190)
		ERRSTR(ERROR_WINHTTP_GLOBAL_CALLBACK_FAILED);// (12191)
		ERRSTR(ERROR_WINHTTP_FEATURE_DISABLED);// (12192)
#endif
	});
	return s_mapErr;
}
/// 별도로 UcErrorToStr에 뮤텍스를 둘 필요는 없습니다. 말씀하신 것처럼 문제는 “초기 한 번 채우는 구간”이었습니다.
//맵은 더 이상 insert 하지 않고, UcErrorToStr에서는 find 같은 읽기만 합니다. 쓰기 스레드가 없으면 여러 스레드에서 동시에 const 관찰(find 등)해도 std::map 사용상 문제 없습니다.
//맵이 완전히 채워진 뒤에야 다른 스레드가 call_once를 빠져나와 find를 하게 되므로, “반쯤 채워진 맵을 읽는다” 같은 상태는 나오지 않습니다.
//정리하면, UcErrorToStr만 따로 잠글 필요는 없고, 지금처럼 UcGetErrorMap 초기화를 call_once로 직렬화한 것으로 충분합니다.
CStringA UcErrorToStr(UINT err = 0xffffffff);
CStringA UcErrorToStr(UINT err)
{
	if (err == 0xffffffff)
		err = GetLastError();
	auto mapErr = UcGetErrorMap();
	CStringA sa;
	auto it = mapErr->find(err);
	if (it != mapErr->end())
		sa.Format("%s(%d)", it->second.c_str(), it->first);
	else
		sa.Format("UnknownError(%d)", err);

	return sa;
}

/// GetLastError를 여러번 불러서 담아 간다.
SHP<KStdMap<UINT, CStringW>> UcGetLastErros(int toFind)
{
	int nCntSame = 0;
	int errPrev = -1;
	//KStdMap<UINT, CStringW> uset;
	SHP<KStdMap<UINT, CStringW>> shSet;// = make_shared<std::set<UINT>>();
	for (int i = 0; i < 10; i++)
	{
		auto err = GetLastError();
		KAtEnd loopEnd([&errPrev, &err]() {
			errPrev = err;
			});

		if (toFind >= 0)
		{
			if (err == toFind)//ERROR_WINHTTP_CONNECTION_ERROR
			{
				shSet = make_shared<KStdMap<UINT, CStringW>>();
				CStringW sErr = UcErrorToStrW(err);
				shSet->insert({ err, sErr });
				return shSet;
			}
		}
		else // < 0 : 에러 모두 담아 달라는 거다.
		{
			if (err == ERROR_SUCCESS)
				continue;
			if (!shSet)
				shSet = make_shared<KStdMap<UINT, CStringW>>();
			if (shSet->find(err) == shSet->end())
			{
				CStringW sErr = UcErrorToStrW(err);
				shSet->insert({ err, sErr });
			}
		}

		if (errPrev == err)
		{
			++nCntSame;
			if (nCntSame > 1)// 같은게 중복 되면 더이상 에러가 없는 것으로 알고 중지
				break;
			else
				continue;
		}
	}
	return shSet;
}

bool UcErrorFound(int toFind)
{
	ASSERT(toFind != -1);
	auto errs = UcGetLastErros(toFind);//ERROR_WINHTTP_TIMEOUT);//여러 에러중 이게 있냐?
	return (bool)errs;
}
//bool UcTakecareOther(SHP<KStdMap<UINT, CStringW>> errs, int eExcept)
//{
//}
//CStringA UcGetErrorMsg(UINT err = 0xffffffff);
UCTOOLDYNAMIC
CStringA UcGetErrorMsg(UINT err)
{
	LPWSTR errorMessageBuffer = nullptr;
	if (err == 0xffffffff)
		err = GetLastError();
	// FormatMessage()를 사용하여 에러 코드를 문자열로 변환
	DWORD messageLength = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&errorMessageBuffer),
		0,
		nullptr
	);
	CStringA en = UcErrorToStr(err);
	en += "\t";
	en += errorMessageBuffer;
	return en;// CStringA(errorMessageBuffer);
}
//CStringW UcErrorToStrW(UINT err = 0xffffffff);
UCTOOLDYNAMIC
CStringW UcErrorToStrW(UINT err)
{
	CStringA sa = UcErrorToStr(err);
	CStringW sw(sa);
	return sw;
}



PWS UcWcharToUTF8ToHtmlUrl(CStringW& sWchar, CStringW& sWUrl)
{
	CStringA sUtf8;
	UcWcharToUTF8(sWchar, sUtf8);
	UcUTF8ToHtmlUrl(sUtf8, sWUrl);
	return sWUrl;
}
PWS UcUTF8ToHtmlUrl(CStringA& sUtf8, CStringW& sWstr)
{
	//eWstr.Empty();
	CStringW s;
	for_each0(sUtf8.GetLength())
	{
		char ch = sUtf8[_i];
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
#define UCVARFORMAT
UCTOOLDYNAMIC
CStringW UcFormatStringFromArgs(LPCWSTR fmt, va_list args) {
	int size = _vscwprintf(fmt, args) + 1;  // +1 for null terminator
	CStringW buffer;
	wchar_t* tempBuffer = buffer.GetBuffer(size);
	if (tempBuffer) {
		_vsnwprintf_s(tempBuffer, size, size - 1, fmt, args);
		buffer.ReleaseBuffer();
	}
	return buffer;
}

CStringW UcMessageBoxStatic::s_title;
HWND UcMessageBoxStatic::s_hWndParent = NULL;

/// <param name="nType">MB_OK</param>
UCTOOLDYNAMIC
int UcMessageBoxGeneral(UINT nType, LPCWSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CStringW buffer = UcFormatStringFromArgs(fmt, args);
	va_end(args);
	if (UcMessageBoxStatic::s_title.GetLength())
		return MessageBoxW(UcMessageBoxStatic::s_hWndParent, buffer, UcMessageBoxStatic::s_title, nType);
	return AfxMessageBox(CString(buffer), nType);
}

/// <summary>
/// 출력창에 이 포맷으로 출력 되면 더블클릭 했을 때 소스 위치로 간다. 맨 뒤에 반드시 "\n"이어야 한다.
/// </summary>
/// <param name="f">파일 경로</param>
/// <param name="l">라인번호</param>
/// <param name="sTrace">아무태그</param>
/// <returns>조합된 문자열 리턴</returns>
CStringW DblkGotoLine(LPCWSTR f, int l, LPCWSTR sTrace)
{
	CStringW sfmt;
	sfmt.Format(L"%s(%d):%s- ", f, l, sTrace);
	return sfmt;// 이문자열을 이용한 최종 문자열은 맨 뒤에 반드시 "\n"이어야 한다.
}

/// <summary>
/// `현재파일(라인):UcTRACE- `와 뒤에 문자열 까지 만들어 그 문자열을 리턴 한다.
/// 출력창에 뿌리지는 않는다.
/// </summary>
/// <returns>그 문자열을 리턴</returns>
CStringW GetFileLineW(LPCWSTR f, int l, LPCWSTR sTrace, LPCWSTR fmt, ...)
{
	CStringW sfmt = DblkGotoLine(f, l, sTrace);
	//sfmt.Format(L"%s(%d):%s- ", f, l, sTrace);
	sfmt += fmt;
	va_list args;
	va_start(args, (PWS)fmt);// ... 앞에 파라미터를 준다.  warning C5082: second argument to 'va_start' is not the last named parameter

	CStringW buffer = UcFormatStringFromArgs((PWS)sfmt, args);
	va_end(args);
	//CStringW str = DwkFormat((PWS)sfmt, ##__VA_ARGS__);
	return buffer;
}

UCTOOLDYNAMIC
CStringW UcMessageBoxGeneralStr(UINT nType, LPCWSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CStringW buffer = UcFormatStringFromArgs(fmt, args);
	va_end(args);
	if (nType & (MB_ICONSTOP | MB_ICONWARNING)) {
		no_throw_str(buffer);
	}
	if (UcMessageBoxStatic::s_title.GetLength())
		MessageBoxW(UcMessageBoxStatic::s_hWndParent, buffer, UcMessageBoxStatic::s_title, nType);
	else
		AfxMessageBox(CString(buffer), nType);
	return buffer;
}

[[deprecated]]
int UcMessageBoxLastError()
{
	auto err = GetLastError();
	CStringW sw(UcGetErrorMsg(err));

	return UcMessageBoxGeneral(MB_OK | MB_ICONEXCLAMATION, sw);
}

int UcMessageBoxException(CException* e)
{
	CString sw;
	e->GetErrorMessage(sw.GetBuffer(1024), 1024);
	sw.ReleaseBuffer();
	if (sw.IsEmpty())
	{
		KException* ke = dynamic_cast<KException*>(e);
		if (ke)
		{
			if (ke->m_strError.GetLength() > 0)
				sw += ke->m_strError;
			else if (ke->m_strStateNativeOrigin.GetLength() > 0)
				sw += ke->m_strStateNativeOrigin;
			else
				sw = L"Unknown error.";
			//sw.Format(L"%s\n%s", ke->m_strError)
		}
	}
	if (sw.IsEmpty())
		_break;
	return UcMessageBoxGeneral(MB_OK | MB_ICONEXCLAMATION, CStringW(sw));
}

int UcFileComp(CString sf1, CString sf2)
{
	CFile f1, f2;
	if (!f1.Open(sf1, CFile::modeRead))
		throw_str(L"File open error.(%s)", sf1.GetString());
	if (!f2.Open(sf2, CFile::modeRead))
		throw_str(L"File open error.(%s)", sf2.GetString());
	KAtEnd defer([&f1, &f2]() {
		f1.Close();
		f2.Close();
		});
	auto len1 = f1.GetLength();
	auto len2 = f2.GetLength();
#ifndef _DEBUG
	if (len1 != len2)
		return -1;
#endif // _DEBUG
#ifdef _UseStdBuffer_
	auto shBuf1 = SharedBuf(len1 + 1);//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
	auto shBuf2 = SharedBuf(len2 + 1);//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
	auto Buf1 = shBuf1.get();//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
	auto Buf2 = shBuf2.get();//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
#else
	CStringA shBuf1;
	CStringA shBuf2;
	auto Buf1 = shBuf1.GetBuffer((int)len1 + 1);
	auto Buf2 = shBuf2.GetBuffer((int)len2 + 1);
#endif // _UseStdBuffer_


	f1.Read(Buf1, (UINT)len1);
	f2.Read(Buf2, (UINT)len2);
	int nDiff = 0;
	auto lenN = len1 < len2 ? len1 : len2;
	auto lenX = len1 < len2 ? len2 : len1;
	int iwh = len1 < len2 ? 2 : len1 == len2 ? 0 : 1;
	for (int i1 = 0; i1 < lenN; i1++)
	{
		if (Buf1[i1] != Buf2[i1])
		{
			//TRACE(L"%8d. %02X != %02X\n", (UCHAR)Buf1[i1], (UCHAR)Buf2[i1]);
			nDiff++;
		}
	}
#ifdef _DEBUG
	if (iwh != 0)
	{
		for (auto i2 = lenN; i2 < lenX; i2++)
		{
			//if(iwh == 1)
			//	TRACE(L"> %8d. %02X != --\n", i2, (UCHAR)Buf1[i2]);
			//else
			//	TRACE(L"< %8d. -- != %02X\n", i2, (UCHAR)Buf2[i2]);
			nDiff++;
		}
	}
#endif // _DEBUG
	return nDiff;
}

//auto flds ={ FOLDERID_RoamingAppData, FOLDERID_LocalAppData, FOLDERID_LocalAppDataLow };
//C:\Users\keeps\AppData\Roaming
//C:\Users\keeps\AppData\Local
//C:\Users\keeps\AppData\LocalLow
//	DownloadFiles(L"http://localhost:8000/update/bigfile.exe", fileTmp);
CStringW UcGetShellFolder(REFKNOWNFOLDERID fid)
{
	CStringW fldTemp;
	PWSTR path = NULL;
	KAtEnd defer([&path]() {
		if (path)
			CoTaskMemFree(path);//SHGetKnownFolderPath로 받은 포이터 해제해야 한다.
		});
	if (SUCCEEDED(SHGetKnownFolderPath(fid, 0, NULL, &path)))
	{
		fldTemp = path;
		//TRACE(L"%s\n", path);
	}
	return fldTemp;
}

std::vector<CStringW> GetSegments(const CStringW& path)
{
	std::vector<CStringW> segments;
	int start = 0;
	CStringW token = path.Tokenize(LR"(/\)", start);
	while (start != -1)
	{
		segments.push_back(token);
		token = path.Tokenize(LR"(/\)", start);
	}
	size_t n = segments.size();
	return segments;
}
std::vector<CStringW> GetLastSegments(const CStringW& path, int nSegment)
{
	std::vector<CStringW> segments = GetSegments(path);

	size_t n = segments.size();
	if ((int)n >= nSegment)
	{
		vector<CStringW> seg;
		for (int ns = nSegment; ns > 0; ns--)
			seg.push_back(segments[n - ns]);
		return seg;
	}
	return {};
}


bool UcCompareLastMultiSegments(CStringW path1, CStringW path2, int nSegment)
{
	path1.MakeLower();
	path2.MakeLower();
	auto segments1 = GetLastSegments(path1, nSegment);
	auto segments2 = GetLastSegments(path2, nSegment);

	if (segments1.size() != nSegment || segments2.size() != nSegment)
		return false; // 하나 이상의 경로에 서브디렉토리가 2개 미만인 경우

	for (int i = 0; i < nSegment; i++)
	{
		if (segments1[i] != segments2[i])
			return false;
	}
	return true;
}

/// 서버 폴더와 URL directory 일부가 매치 되는지 뒤쪽 부터 비교한다.
bool UcCompareFolderWithUrlDir(CStringW sLocalFolder, CStringW sUrlDir)
{
	sLocalFolder.MakeLower();
	sUrlDir.MakeLower();
	auto segments1 = GetSegments(sLocalFolder);
	auto segments2 = GetSegments(sUrlDir);
	auto nFld = segments1.size();
	auto nDir = segments2.size();
	if (nFld < nDir)
		return false;
	for (int i1 = (int)nFld - 1, i2 = (int)nDir - 1; i2 >= 0; i1--, i2--)
	{
		auto& s1 = segments1[i1];
		auto& s2 = segments2[i2];
		if (s1 != s2)
			return false;
	}
	return true;
}

int UcRomoveExpiredFile(CString tarFld, int nDay)
{
	CTime now = UcGetCurrentTime();
	int nFilesInFolder = 0;
	vector<CString> arToDel;
	UcRecursiveDirLambda((LPCTSTR)tarFld, { _T("*") }, [&arToDel, &nFilesInFolder, nDay, now](CString sFile, auto& wsd) {
		nFilesInFolder++;
		CString ufj(sFile);//UpdateCourierCheck_20240416112823507.exe"
		auto ibs = ufj.ReverseFind('\\');
		if (ibs >= 0)
		{
			auto file_ext = ufj.Mid(ibs + 1);//ufld = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcRoot\\UpdateCourierCheck"
			auto idt = file_ext.ReverseFind('.');
			if (idt >= 0)
			{
				auto ius = file_ext.ReverseFind('_');
				auto sTime = file_ext.Mid(ius + 1, idt - ius - 1);//sTime = L"20240416104949854"
				if (UcIsDigitStr(CString(sTime)))//UpdateCouirierGlobalSettings_20240530135443083.json
				{
#ifdef _DEBUG
					SYSTEMTIME st = { 0 };
					UcCStringToTime10(CString(sTime), &st);
#endif // _DEBUG
					CTime tFile = UcCStringToCTime(sTime);
					auto sp = now - tFile;
#ifdef _DEBUGx
					if (nDay < sp.GetTotalSeconds())
#else
					if (nDay < sp.GetDays())
#endif // _DEBUG
						arToDel.push_back(CString(ufj));
					_break;
				}
			}
		}
		return 0;
		}, NULL, FALSE);

	try
	{
		for (auto& sFile : arToDel)
			CFile::Remove(CString(sFile));
	}
	catch (CFileException*)
	{
		_break;
	}
	catch (CException*)
	{
		_break;
	}
	return 0;
}
/// <summary>
/// 
/// </summary>
/// <param name="ufj">작성 하려는 풀 네임</param>
/// <param name="bBuFolder">백업폴더: filename_Backup 이라는 폴더가 길어진다.</param>
/// <param name="nLength">sNow = L"20240614094709473" 인데 너무 길어서 자르고 싶을때 길이, 0이면 모두 붙여,
///						8이면 날짜만 넣는다. 하루에 한번씩 파일 갱신 하는 로그 같은 경우</param>
/// <param name="slash">로컬: '\\', 리모트: '/'</param>
/// <returns></returns>
CStringW UcMakeBackupFileNameGeneral(CStringW ufj, bool bBuFolder, int nLength, WCHAR slash)
{//ufj = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcLog\\UpdateCourierServer\\UcException.log", true, 8
	auto i0 = ufj.ReverseFind('.');
	auto ibs = ufj.ReverseFind(slash);// '\\');
	if (i0 >= 0 && ibs >= 0)
	{
		auto ufld = ufj.Left(ibs);//ufld = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcLog\\UpdateCourierServer"
		auto ufjFn = ufj.Mid(ibs + 1, i0 - ibs - 1);//ufjFn = L"UcException"
		auto ufjExt = ufj.Mid(i0 + 1);//ufjExt = L"log"
		CStringW fldBU;
		if (bBuFolder)
			fldBU.Format(L"%s%c%s_Backup", ufld.GetString(), slash, ufjFn.GetString());//
		else
			fldBU = (ufld);//fldBU = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcLog\\UpdateCourierServer"

		auto sNow = UcGetCurrentTimeStamp();
		if (nLength > 0)
			sNow = sNow.Left(nLength);
		CStringW sbu; sbu.Format(L"%s%c%s_%s.%s", fldBU.GetString(), slash, ufjFn.GetString(), sNow.GetString(), ufjExt.GetString());
		return sbu;//sbu = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcLog\\UpdateCourierServer\\UcException_20240614.log"
	}
	return {};
}

[[deprecated]]
CStringW UcMakeBackupFileName(CStringW ufj, bool bBuFolder, int nLength)
{//ufj = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcLog\\UpdateCourierServer\\UcException.log", true, 8
	auto sFull = UcMakeBackupFileNameGeneral(ufj, bBuFolder, nLength, '\\');
	return sFull;//sFull = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcLog\\UpdateCourierServer\\UcException_20240614.log"
}

/// <summary>
///  파일명에 날짜시간을 넣어서 리네임 한다.
/// </summary>
/// <param name="ufj"></param>
/// <returns></returns>
int UcBackupFile(CString ufj, int nDayExpire)
{
	if (UcIfFileExistEx(ufj))
	{
		auto i0 = ufj.ReverseFind('.');
		auto ibs = ufj.ReverseFind('\\');
		if (i0 >= 0 && ibs >= 0)
		{
			auto ufld = ufj.Left(ibs);//ufld = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcRoot\\UpdateCourierCheck"
			auto ufjFn = ufj.Mid(ibs + 1, i0 - ibs - 1);//ufjFn = L"UpdateCourierCheck"
			auto ufjExt = ufj.Mid(i0 + 1);//ufjExt = L"exe"
			CString fldBU; fldBU.Format(_T("%s\\%s_Backup"), ufld.GetString(), ufjFn.GetString());//
			UcCheckTargetDir(fldBU, TRUE);//fldBU = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcRoot\\UpdateCourierCheck\\UpdateCourierCheck_Bakup"
			auto sNow = UcGetCurrentTimeStamp();
			CString sbu; sbu.Format(_T("%s\\%s_%s.%s"), fldBU.GetString(), ufjFn.GetString(), sNow.GetString(), ufjExt.GetString());
			try//sbu = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcRoot\\UpdateCourierCheck\\UpdateCourierCheck_Bakup\\UpdateCourierCheck_20240416112823507.exe"
			{
				CFile::Rename(ufj, sbu);
				if (nDayExpire > 0)
					UcRomoveExpiredFile((fldBU), nDayExpire);

				return TRUE;
			}
			catch (CException*)
			{
				auto er = GetLastError();
				auto ser = UcErrorToStrW(er); TRACE(L"CException(%u: %s) %s\n", er, ser);
				throw;
			}
		}
		//throw_str(L"Need full path : %s", ufj);
	}
	return FALSE;
}
// Implementation (use AfxThrowDBException to create)
IMPLEMENT_DYNAMIC(KException, CException)


/// 모든 exception을 처리 하는 기본 함수 이다. 사용자가 따로 처리 할일 이 있으면 s_fncExceptionDealer에 람다 함수를 넣어두면 불려 진다.
/// 참조: CatchAllKException, CATCH_ICEXEPT,  CIconTaxiControlApp::InitInstance()
void KException::FinishException()//int iOp)//?exception
{
	if (_bFinished)// Init에서 이미 호출. 여러번 불리는 경우 방지
		return;

	_stack = UcPrintStack();

	KAtEnd defer([this]() {
		this->_bFinished = TRUE;
		});
	auto* e = this;
	//TRACE("################### KException::FinishException Catched %s(%u) at %s:%d\n", e->_sExcept, e->_error, e->_func, e->_line);
	CStringW funcW(e->_func);
	CStringW fileTarW(e->_fileTar);
	CStringW sExceptW(e->_sExcept);
	CStringW classW(e->_class);
	CStringW sErr = UcErrorToStrW(e->_error);
	//CStringW ds; ds.Format(L"%s(%d) : KException - %s(%u) %s - %s\n", fileTarW, e->_line, sExceptW, e->_error, classW, funcW);
	CStringW ds; ds.Format(L"##### %s : %s %s - %s : %s %s in %s\n", sExceptW.GetString(),
		sErr.GetString(), classW.GetString(), sErr == m_strError ? L"" : (PWS)m_strError,
		m_strStateNativeOrigin.GetString(), e->_lastError.GetString(), funcW.GetString());
	//OutputDebugStringW(ds);
	if ((_iOp & eNoDebugOutput) == 0)// debug output 에서 제외: 이미 한 경우 이겠지.
		UcFileLineTrace(fileTarW, e->_line, L"KException", ds);

	if (s_fncExceptionDealer)
		s_fncExceptionDealer(e);
}

/// <summary>
/// 파일(라인):sOwner-
/// </summary>
/// <param name="sFile"></param>
/// <param name="nLine"></param>
/// <param name="sOwner"></param>
/// <param name="sTxt"></param>
void UcFileLineTrace(const CStringW& sFile, int nLine, const CStringW& sOwner, const CStringW& sTxt)
{
	CStringW ds = DblkGotoLine(sFile, nLine, sOwner) + sTxt + L"\n";
	//CStringW ds; ds.Format(L"%s(%d):%s- %s", sFile.GetString(), nLine, sOwner.GetString(), sTxt.GetString());
	OutputDebugStringW(ds);
}

/// 파일(라인) 만 출력 한다. RTRACE는 function name까지 출력 한다.
void UcTrace(const CStringW& sFile, int nLine, const CStringW& sOwner, LPCWSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CStringW buffer = UcFormatStringFromArgs(fmt, args);
	CStringW ds = DblkGotoLine(sFile, nLine, sOwner) + buffer + L"\n";
	//CStringW ds; ds.Format(L"%s(%d):%s- %s", sFile.GetString(), nLine, sOwner.GetString(), buffer.GetString());
	OutputDebugStringW(ds);
	va_end(args);
}

#define _ZLIB_
#ifdef _ZLIB_

#include <fstream>
#include <string>
#ifdef _UseItcZlib__
#include "../../../Licensed/OpenDesign/ThirdParty/ZLib/zlib.h"
//#include "../ZLib/zlib.h" for ZLib.64
#include <fstream>
//#include <zlib.h>

#define CHUNK 16384
//IntelliCAD\Source\IntelliCAD\api\icarx\opendcl_ported\Library\ZLib\ZLib.64
bool UcCompressFile(const wchar_t* wsourcePath, const wchar_t* wdestPath)
{
	CStringA sourcePath(wsourcePath);
	CStringA destPath(wdestPath);
	FILE* sourceFile{ NULL };
	FILE* destFile{ NULL };
	BYTE* buffer = new BYTE[CHUNK];
	BYTE* out = new BYTE[CHUNK];
	KAtEnd defer([&buffer, &out]() {
		if (buffer)
			delete buffer;
		if (out)
			delete out;
		});
	int ret{ 0 }, flush{ 0 };
	unsigned have{ 0 };
	z_stream stream{ 0 };
	BOOL bExistsTar = FALSE;

	try
	{
		// 입력 파일 열기
		if (fopen_s(&sourceFile, sourcePath, "rb") != 0)
			throw_str(L"Unable to open source file for reading: [%s]", wsourcePath);
		KAtEnd dfer1([&sourceFile]() { fclose(sourceFile); });

		CFileStatus stsTar;
		bExistsTar = UcIfFileExistEx(wdestPath);
		if (bExistsTar)
		{
			//KwSetReadOnly(target, false);
			if (CFile::GetStatus(wdestPath, stsTar) == FALSE)
				return GetLastError();
			if (stsTar.m_attribute & CFile::system)
				return FALSE;
			if ((stsTar.m_attribute & CFile::readOnly) || (stsTar.m_attribute & CFile::hidden))
			{
				stsTar.m_attribute &= ~(CFile::readOnly | CFile::hidden);//속성 비트 제거
				CFile::SetStatus(wdestPath, stsTar);
			}
		}
		if (1)
		{
			CStringW sTar(wdestPath);
			int itar = sTar.ReverseFind('\\');
			CStringW sTarFolder;
			if (itar >= 0)
				sTarFolder = sTar.Left(itar);
			UcCheckTargetDir(sTarFolder);
		}

		// 출력 파일 열기
		if (fopen_s(&destFile, destPath, "wb") != 0)
			throw_str(L"Unable to open destination file for writing: [%s]", wdestPath);
		KAtEnd dfer2([&destFile]() { fclose(destFile); });

		// zlib 초기화
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;
		if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK)
			throw_str(L"Unable to initialize zlib");
		KAtEnd dfer3([&stream]() { deflateEnd(&stream); });

		// 파일 압축
		do
		{
			// 파일에서 데이터를 읽음
			size_t bytesRead = fread(buffer, 1, CHUNK, sourceFile);
			if (bytesRead == 0)
				break;// 파일 끝에 도달하여 더 이상 읽을 데이터가 없으면 루프를 종료
			// zlib의 compress 함수를 사용하여 데이터를 압축
			stream.next_in = buffer;
			stream.avail_in = (uInt)bytesRead;
			do
			{
				// 압축된 데이터를 출력 버퍼에 쓰기
				stream.next_out = out;
				stream.avail_out = CHUNK;
				flush = (bytesRead < CHUNK) ? Z_FINISH : Z_NO_FLUSH;
				ret = deflate(&stream, flush);
				// 출력 버퍼에 쓴 바이트 수 갱신
				have = CHUNK - stream.avail_out;
				// 출력 파일에 쓰기
				fwrite(out, 1, have, destFile);
			} while (stream.avail_out == 0);
		} while (flush != Z_FINISH);
		return true;
	}
	UCCATCH_ALL;///KException.s_fncExceptionDealer 를 채워야 로그 처리가 된다.
	return false;
}

bool UcDecompressFile(const wchar_t* wsourcePath, const wchar_t* wdestPath)
{
	CStringA sourcePath(wsourcePath);
	CStringA destPath(wdestPath);
	FILE* sourceFile{ 0 };
	FILE* destFile{ 0 };
	//unsigned char buffer[CHUNK];
	//unsigned char out[CHUNK];
	BYTE* buffer = new BYTE[CHUNK];
	BYTE* out = new BYTE[CHUNK];
	KAtEnd defer([&buffer, &out]() {
		if (buffer)
			delete buffer;
		if (out)
			delete out;
		});
	int ret{ 0 };
	unsigned have{ 0 };
	z_stream stream{ 0 };

	try
	{
		// 입력 파일 열기
		if (fopen_s(&sourceFile, sourcePath, "rb") != 0)
			throw_str(L"Unable to open source file for reading: [%s]", wsourcePath);

		KAtEnd dfer1([&sourceFile]() { fclose(sourceFile); });
		// 출력 파일 열기
		if (fopen_s(&destFile, destPath, "wb") != 0)
			throw_str(L"Unable to open destination file for writing: [%s]", wdestPath);
		KAtEnd dfer2([&destFile]() { fclose(destFile); });

		// zlib 초기화
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;
		stream.avail_in = 0;
		stream.next_in = Z_NULL;
		if (inflateInit(&stream) != Z_OK)
			throw_str(L"Unable to initialize zlib");
		// 압축 해제 스트림 정리
		KAtEnd dfer3([&stream]() { inflateEnd(&stream); });

		// 파일 압축 해제
		do
		{
			// 파일에서 데이터를 읽음
			size_t bytesRead = fread(buffer, 1, CHUNK, sourceFile);
			if (bytesRead == 0)
			{
				// 파일 끝에 도달하여 더 이상 읽을 데이터가 없으면 루프를 종료
				break;
			}
			stream.next_in = buffer;
			stream.avail_in = (uInt)bytesRead;
			do
			{
				// 해제된 데이터를 출력 버퍼에 쓰기
				stream.next_out = out;
				stream.avail_out = CHUNK;
				ret = inflate(&stream, Z_NO_FLUSH);
				switch (ret)
				{
				case Z_NEED_DICT:
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					throw_str(L"inflate error");
					break;
				}
				// 출력 버퍼에 쓴 바이트 수 갱신
				have = CHUNK - stream.avail_out;
				// 출력 파일에 쓰기
				fwrite(out, 1, have, destFile);
			} while (stream.avail_out == 0);
		} while (ret != Z_STREAM_END);
		return true;
	}
	UCCATCH_ALL;///KException.s_fncExceptionDealer 를 채워야 로그 처리가 된다.
	return false;
}
#ifdef _zlib_sample__
int _zlib_sample__()
{
	const char* sourcePath = R"(C:\Outbin\UpdateCourier\x64\Debug\test\CommandGUI.dll)";//example.txt";= "input.txt";
	const char* compressedPath = R"(C:\Outbin\UpdateCourier\x64\Debug\test\CommandGUI.dll.gz)";//"example.txt.gz";=  "output.gz";
	const char* decompressedPath = R"(C:\Outbin\UpdateCourier\x64\Debug\test\CommandGUI.dll.recover)";//"example_decompressed.txt";= "output.txt";

	if (compressFile(sourcePath, compressedPath))
		TRACE("File compressed successfully.\n");
	else
		TRACE("Failed to compress file.\n");
	if (decompressFile(compressedPath, decompressedPath))
		TRACE("File decompressed successfully.\n");
	else
		TRACE("Failed to decompress file.\n");

	return 0;
}



#endif // _sample__
#endif // _ZLIB_
#endif // _UseItcZlib__

#include <winver.h>
#pragma comment(lib, "version.lib")

CStringW UcGetProductName()
{
	CStringW sPrd;
	TCHAR szFilePath[MAX_PATH];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);//L"C:\\Outbin\\UpdateCourier\\x64\\Debug\\UpdateCourierServer.exe"

	// 파일 정보를 저장할 구조체 생성 및 초기화
	DWORD dwHandle{};
	DWORD dwSize = GetFileVersionInfoSize(szFilePath, NULL);// &dwHandle);//dwSize = 1636
	if (dwSize > 0)
	{
		std::vector<BYTE> buffer(dwSize);
		if (GetFileVersionInfo(szFilePath, dwHandle, dwSize, &buffer[0]))
		{
			// PRODUCT_NAME 문자열 가져오기
			LPVOID lpBuffer;
			UINT uLen;
			if (!VerQueryValue(&buffer[0], _T("\\StringFileInfo\\040904B0\\ProductName"), &lpBuffer, &uLen))
			{//English(US)
				VerQueryValue(&buffer[0], _T("\\StringFileInfo\\041204b0\\ProductName"), &lpBuffer, &uLen);
				//Korea
			}
			if (uLen > 0)
				sPrd = CStringW(reinterpret_cast<LPCWSTR>(lpBuffer), uLen);
		}
	}
	return sPrd;
}

UCTOOLDYNAMIC
CString UcGetModulePath(BOOL bPathOnly)
{
	CString sPath;
	TCHAR* buf = sPath.GetBuffer(4096);

	// AfxGetResourceHandle()이 NULL인 경우 처리
	HINSTANCE hInstance = NULL;
	//hInstance = AfxGetResourceHandle();
	//if (hInstance == NULL) {
	//	// DLL에서 실행 중인 경우 현재 모듈 핸들 사용
	//	hInstance = GetModuleHandle(NULL);
	//}
	//
	// GetModuleFileName 호출 및 에러 처리
	DWORD result = GetModuleFileName(hInstance, buf, 4096);
	if (result == 0) {
		// 에러 발생 시 NULL 핸들로 재시도
		result = GetModuleFileName(NULL, buf, 4096);
		if (result == 0) {
			sPath.ReleaseBuffer();
			return _T(""); // 에러 시 빈 문자열 반환
		}
	}

	sPath.ReleaseBuffer();
	auto sExt = sPath.Right(3);
	sExt.MakeLower();
	ASSERT(sExt != L"dll");
	if (sPath.GetLength() > 0) {
		if (bPathOnly) {
			auto irv = sPath.ReverseFind('\\');
			if (irv >= 0)
				sPath = sPath.Left(irv);
		}
	}
	return sPath;
}

CString UcGetFileVersionRunning(LPCTSTR filePath)
{
	//static CStringW s_appVer;
	//if (!s_appVer.IsEmpty())
	//	return s_appVer;
	if (filePath == nullptr)
		return {};
	// 파일 경로로부터 모듈 핸들을 얻습니다.
	if (HMODULE hModule = ::GetModuleHandle(filePath)) //  로드 된 경우만 값을 리턴 한다.
	{
		DWORD dummy{};
		DWORD versionSize = GetFileVersionInfoSize(filePath, nullptr);// &dummy); // 모듈의 핸들을 사용하여 파일 버전 정보의 크기를 가져옵니다.
		if (versionSize > 0)
		{
			std::vector<BYTE> versionInfoBuffer(versionSize);
			if (GetFileVersionInfo(filePath, dummy, versionSize, versionInfoBuffer.data()))
			{
				VS_FIXEDFILEINFO* fileInfo;
				UINT fileInfoSize;
				if (VerQueryValue(versionInfoBuffer.data(), _T("\\"), (LPVOID*)&fileInfo, &fileInfoSize))
				{
					// 파일 버전 정보를 문자열로 변환
					WORD major = HIWORD(fileInfo->dwFileVersionMS);
					WORD minor = LOWORD(fileInfo->dwFileVersionMS);
					WORD build = HIWORD(fileInfo->dwFileVersionLS);
					WORD revision = LOWORD(fileInfo->dwFileVersionLS);

					CString versionString;
					versionString.Format(_T("%d.%d.%d.%d"), major, minor, build, revision);
					return versionString;
				}
			}
		}
	}
	return {};//L"(No Version)";
}

CString UcGetFileVersion(CString filePath, BOOL bProductVersion)
{
	CString versionStr;
	DWORD  verHandle = 0;
	DWORD  verSize = GetFileVersionInfoSize(filePath, nullptr);// &verHandle);

	if (verSize != 0)
	{
		CStringA sbuf;
		LPVOID verData = (LPVOID)sbuf.GetBuffer(verSize + 1);//new BYTE[verSize];
		verHandle = 0;
		if (GetFileVersionInfo(filePath, verHandle, verSize, verData))
		{
			VS_FIXEDFILEINFO* pVerInfo = NULL;
			UINT size = 0;
			if (VerQueryValue(verData, _T("\\"), reinterpret_cast<LPVOID*>(&pVerInfo), &size))
			{
				if (size && pVerInfo)
				{
					DWORD major = HIWORD(bProductVersion ? pVerInfo->dwProductVersionMS : pVerInfo->dwFileVersionMS);
					DWORD minor = LOWORD(bProductVersion ? pVerInfo->dwProductVersionMS : pVerInfo->dwFileVersionMS);
					DWORD build = HIWORD(bProductVersion ? pVerInfo->dwProductVersionLS : pVerInfo->dwFileVersionLS);
					DWORD revision = LOWORD(bProductVersion ? pVerInfo->dwProductVersionLS : pVerInfo->dwFileVersionLS);
					versionStr.Format(_T("%u.%u.%u.%u"), major, minor, build, revision);
				}
			}
		}
		//delete[] verData;
	}
	return versionStr;
}

CString UcGetProductVersion(LPCTSTR filePath)
{
	CString productVersion;
	DWORD dummy;
	DWORD size = GetFileVersionInfoSize(filePath, &dummy);
	if (size == 0) {
		//throw_str(L"Error getting version info size.\n");
		return {};
	}

	BYTE* buffer = new BYTE[size];
	KAtEnd db1([&buffer]() { if (buffer) delete[] buffer; });
	if (!GetFileVersionInfo(filePath, 0, size, buffer)) {
		//throw_str(L"Error getting version info.\n");
		return {};
	}

	UINT len{};
	TCHAR* versionInfo;
	if (VerQueryValue(buffer, _T("\\StringFileInfo\\040904b0\\ProductVersion"), (void**)&versionInfo, &len)) {
		if (len > 0 && versionInfo != NULL) {
			productVersion = versionInfo;// , _TRUNCATE);productVersion = L"13.0.160.33918.P.VC17.x64.Alpha"
		}
		else
			return {};
	}
	else {
		//throw_str(L"Product version not found.");
		return {};
	}
	// 버전 정보에서 제품 버전 문자열을 추출
	//VS_FIXEDFILEINFO* pFileInfo{};
	//if (!VerQueryValue(buffer, _T("\\"), (LPVOID*)&pFileInfo, &len))
	//	return {};

	//if (len > 0) {
	//	// 제품 버전을 문자열로 포맷
	//	productVersion.Format(L"%d.%d.%d.%d",
	//		HIWORD(pFileInfo->dwProductVersionMS), LOWORD(pFileInfo->dwProductVersionMS),
	//		HIWORD(pFileInfo->dwProductVersionLS), LOWORD(pFileInfo->dwProductVersionLS));
	//}
	return productVersion;
}


std::set<CString> UcEnumerateSubKeys(CString sRootKey)
{
	CRegKey key;
	LONG lResult;

	// HKEY_CURRENT_USER\SOFTWARE\ITCKR\ 열기
	// _T("SOFTWARE\\ITCKR\\" = sRootKey
	lResult = key.Open(HKEY_CURRENT_USER, sRootKey, KEY_READ);
	if (lResult != ERROR_SUCCESS)
	{
		TRACE(L"Unable to open registry key.\n");
		return {};
	}

	DWORD dwIndex = 0;
	TCHAR szName[256];
	DWORD dwNameSize = sizeof(szName) / sizeof(TCHAR);

	std::set<CString> st;
	// 서브키를 순회하며 이름을 출력
	while (ERROR_SUCCESS == (lResult = key.EnumKey(dwIndex, szName, &dwNameSize)))
	{
		std::wstringstream ss;
		ss << L"SubKey #" << dwIndex + 1 << L": " << szName << std::endl;
		TRACE(ss.str().c_str());
		st.insert(szName);
		// 다음 서브키를 위해 인덱스와 이름 크기를 초기화
		dwIndex++;
		dwNameSize = sizeof(szName) / sizeof(TCHAR);
	}

	if (lResult != ERROR_NO_MORE_ITEMS)
	{
		TRACE(L"An error occurred while enumerating subkeys.\n");
	}

	// 레지스트리 키 닫기
	key.Close();

	return st;
}

#include <thread>

// CPU 사용량을 가져오는 함수
[[deprecated]]
float UcGetCPUUsage()
{
	// 시스템의 과거 및 현재 시간을 가져옴
	FILETIME ftIdle, ftKernel, ftUser;
	ULARGE_INTEGER uliIdleTime, uliKernelTime, uliUserTime;

	if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser))
	{
		// 과거 및 현재 CPU 시간을 계산
		uliIdleTime.LowPart = ftIdle.dwLowDateTime;
		uliIdleTime.HighPart = ftIdle.dwHighDateTime;

		uliKernelTime.LowPart = ftKernel.dwLowDateTime;
		uliKernelTime.HighPart = ftKernel.dwHighDateTime;

		uliUserTime.LowPart = ftUser.dwLowDateTime;
		uliUserTime.HighPart = ftUser.dwHighDateTime;

		// 총 CPU 시간 계산
		ULARGE_INTEGER uliTotalTime;
		uliTotalTime.QuadPart = uliKernelTime.QuadPart + uliUserTime.QuadPart;

		// 총 CPU 사용량 계산
		float fCPUUsage = (1.0f - ((float)uliIdleTime.QuadPart / (float)uliTotalTime.QuadPart)) * 100.0f;
		return fCPUUsage;
	}

	return -1.0f; // 실패 시 -1 반환
}

// 디스크 사용량을 가져오는 함수
ULONGLONG UcGetDiskUsage()
{
	ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	if (GetDiskFreeSpaceEx(NULL, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
	{
		return totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
	}
	return 0;
}



#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

// 네트워크 사용량을 가져오는 함수
float UcGetNetworkUsage()
{
	MIB_IFTABLE* pIfTable = NULL;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	float totalUsage = 0.0f;

	// GetIfTable 호출을 위한 버퍼 크기 계산
	if (GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
	{
		pIfTable = (MIB_IFTABLE*)malloc(dwSize);

		// GetIfTable 호출
		if (pIfTable != NULL && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR)
		{
			// 각 네트워크 인터페이스의 사용량을 누적
			for (DWORD i = 0; i < pIfTable->dwNumEntries; i++)
			{
				totalUsage += pIfTable->table[i].dwInOctets + pIfTable->table[i].dwOutOctets;
			}
		}

		free(pIfTable);
	}

	return totalUsage;
}

[[deprecated]]
void UcEvaluateSystemState()
{
	// 시스템 상태 평가
	//float fCPUUsage = UcGetCPUUsage();
	ULONGLONG diskUsage = UcGetDiskUsage();
	float fNetworkUsage = UcGetNetworkUsage();

	// 상태 출력 (TRACE 사용)
	//TRACE(L"CPU using: %.2f%%\n", fCPUUsage);
	TRACE(L"Disk Used: %llu bytes\n", diskUsage);
	TRACE(L"Network Using: %.0f bytes\n", fNetworkUsage);
}

#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")
#define _SECOND ((ULONGLONG)10000000)

double UcGetPerformanceCounterValue(LPCTSTR counterName)
{
	PDH_HQUERY queryHandle;
	PDH_HCOUNTER counterHandle;
	PDH_STATUS status;
	stringstream ss;
	status = PdhOpenQuery(NULL, 0, &queryHandle);
	if (status != ERROR_SUCCESS)
	{
		ss << "Error opening query: " << status << std::endl;
		return -1.0;
	}
	KAtEnd defer([&queryHandle]() {
		PdhCloseQuery(queryHandle);
		});

	status = PdhAddCounter(queryHandle, counterName, 0, &counterHandle);
	if (status != ERROR_SUCCESS)
	{
		ss << "Error adding counter: " << status << std::endl;
		return -1.0;
	}

	status = PdhCollectQueryData(queryHandle);
	if (status != ERROR_SUCCESS)
	{
		ss << "Error collecting query data: " << status << std::endl;
		return -1.0;
	}

	PDH_FMT_COUNTERVALUE value;
	status = PdhGetFormattedCounterValue(counterHandle, PDH_FMT_DOUBLE, NULL, &value);
	if (status != ERROR_SUCCESS)
	{
		ss << "Error getting counter value: " << status << std::endl;
		return -1.0;
	}

	return value.doubleValue;
}


/// <summary>
/// 
/// </summary>
/// <param name="reg"></param>
/// <param name="sKey"></param>
/// <param name="kUp">HKEY_CURRENT_USER</param>
/// <returns></returns>
bool UcRegOpen(CRegKey& reg, CString sKey, HKEY kUp, DWORD iOp)
{
	LONG result = reg.Open(kUp, sKey, iOp);// KEY_READ | KEY_WRITE);
	if (result != ERROR_SUCCESS)
	{
		auto iOpCr = iOp & ~KEY_READ; // KEY_READ제거
		result = reg.Create(kUp, sKey, REG_NONE, REG_OPTION_NON_VOLATILE, iOpCr);// KEY_WRITE);
		if (result != ERROR_SUCCESS)
		{
			auto sErr = UcErrorToStrW(result);//5
			no_throw_str(L"CRegKey Error[%s]: Unable to create/open registry key(%s)", (PWS)sErr, CString(sKey));
			return false;
		}
	}
	return true;
}

#define RKEY2MAP(sm) {sm,#sm}
CStringW UcHKeyToStr(HKEY hk)
{
	static std::map<HKEY, LPCSTR> h2s = {
		RKEY2MAP(HKEY_CLASSES_ROOT),
		RKEY2MAP(HKEY_CURRENT_USER),
		RKEY2MAP(HKEY_LOCAL_MACHINE),
		RKEY2MAP(HKEY_USERS),
	};
	auto it = h2s.find(hk);
	if (it != h2s.end())
		return CStringW(it->second);
	return {};
}
#undef RKEY2MAP

/// <summary>
/// HKEY_CURRENT_USER 문자열 키를 오픈 해준다.
/// </summary>
/// <param name="sRegKey">L"[HKEY_CURRENT_USER\SOFTWARE\ITCKR\MyKey\MySubKey]"</param>
/// <param name="rkey"></param>
/// <param name="kUp">HKEY_CURRENT_USER</param>
/// <returns></returns>
CRegKey* UcParseAndSetRegistryKey(CString sRegKey, CRegKey& key, HKEY kUp)
{
	sRegKey.Trim(_T("[]"));
	CString currentPath;
	currentPath = sRegKey;
	auto mklen = UcHKeyToStr(kUp).GetLength();// tchlen(TO_STR(HKEY_CURRENT_USER));
	ASSERT(mklen > 0);
	CString sKey = currentPath.Mid(mklen + 1);
	//currentPath.Replace(_T("HKEY_CURRENT_USER\\"), _T(""));
//currentPath = L"HKEY_CURRENT_USER\\SOFTWARE\\ITCKR\\PatchStat"
	bool bReg = UcRegOpen(key, sKey, kUp);// HKEY_CURRENT_USER);
	if (!bReg)
		return  NULL;
	//if (key.Open(HKEY_CURRENT_USER, sKey, KEY_WRITE) != ERROR_SUCCESS)
	//{
	//	if (key.Create(HKEY_CURRENT_USER, sKey) != ERROR_SUCCESS)
	//		return  NULL;
	//}
	return &key;
}
#ifdef _Sample__DEBUG
CStringW sReg; sReg.Format(
	LR"([HKEY_CURRENT_USER\SOFTWARE\ITCKR\PatchStat]
		[HKEY_CURRENT_USER\SOFTWARE\ITCKR\PatchStat\%s]
		"nRevisionLast"="%d"
		"DwordTest"=dword:00000010
		"sAppFolder"="%s")"
	, L"433cc083-d08e-11ee-81c9-047c16b61111"
	, 11111
	, L"D:\\UpdateCourier\\FakeCADian"
);
UcParseAndSetRegistryValue(sReg);
#endif // _Sample__DEBUG

void UcParseAndSetRegistryValue(CString regContent)
{
	std::vector<CString> lines;
	CString resToken;
	int curPos = 0;
	resToken = regContent.Tokenize(_T("\n"), curPos);
	while (resToken != _T(""))
	{
		lines.push_back(resToken.Trim());
		resToken = regContent.Tokenize(_T("\n"), curPos);
	}

	CRegKey key;
	//CString currentPath;

	for (auto& line : lines)
	{
		if (line.Left(1) == _T("["))
		{
			if (UcParseAndSetRegistryKey(line, key) == NULL)
				throw_str(L"CRegKey Error: Unable to create/open registry key");
		}
		else if (!line.IsEmpty() && line.Find(_T("=")) != -1)
		{ // Value
			int pos = line.Find(_T("="));
			CString name = line.Left(pos);
			CString value = line.Mid(pos + 1);

			name.Trim(_T("\""));

			// Check value type
			if (value.Left(6).CompareNoCase(_T("dword:")) == 0)
			{//"DwordTest"=dword:00000010
				DWORD dwValue = _tcstoul(value.Mid(6), NULL, 16);//16진수 로
				key.SetDWORDValue(name, dwValue);
			}
			else if (value.Left(4).CompareNoCase(_T("hex(")) == 0)
			{
				ASSERT(value.Left(4).CompareNoCase(_T("hex(")) != 0);
				throw_str(L"Only string and DWORD type are suported in this function. %s", __STD_FUNCTIONW__);
			}
			else
			{
				value.Trim(_T("\""));
				key.SetStringValue(name, value);
			}
		}
	}

	key.Close();
}
#undef TO_STR
#ifdef _DEBUGx
Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\SOFTWARE\ITCKR\PatchStat1]

[HKEY_CURRENT_USER\SOFTWARE\ITCKR\PatchStat1\433cc083 - d08e - 11ee - 81c9 - 047c16b627a0]
"sAppFolder" = "D:\\\\UpdateCourier\\\\FakeCADian"
"sDownloadStat" = "downloadCompleted"
"sPatchApp" = "D:\\\\UpdateCourier\\\\FakeCADian\\\\UpdateCourierPatch.exe"
"DWordEx" = dword:000004d2
"binaryEx" = hex : 12, 13, 14, 15, 16
"QWordEx" = hex(b) : 45, 23, 51, 34, 12, 00, 00, 00
"multiStrEx" = hex(7) : 31, 00, 20, 00, 66, 00, 69, 00, 72, 00, 73, 00, 74, 00, 20, 00, 6c, 00, 69, \
00, 6e, 00, 65, 00, 00, 00, 32, 00, 20, 00, 73, 00, 65, 00, 63, 00, 6f, 00, 6e, 00, 64, 00, 20, 00, \
6c, 00, 69, 00, 6e, 00, 65, 00, 00, 00, 34, 00, 20, 00, 74, 00, 68, 00, 69, 00, 72, 00, 64, 00, 20, \
00, 69, 00, 73, 00, 20, 00, 65, 00, 6d, 00, 70, 00, 74, 00, 79, 00, 20, 00, 6c, 00, 69, 00, 6e, 00, \
65, 00, 20, 00, 69, 00, 73, 00, 20, 00, 70, 00, 72, 00, 6f, 00, 68, 00, 69, 00, 62, 00, 69, 00, 74, \
00, 65, 00, 64, 00, 00, 00, 00, 00
"expandEx" = hex(2) :61, 00, 73, 00, 64, 00, 66, 00, 61, 00, 73, 00, 64, 00, 66, 00, 00, 00

hex : REG_BINARY 유형을 나타내며, 일반적인 바이너리 데이터입니다.
hex(2) : REG_EXPAND_SZ 유형을 나타내며, 환경 변수를 포함할 수 있는 확장 가능한 문자열 값입니다.
hex(7) : REG_MULTI_SZ 유형을 나타내며, 여러 개의 문자열 데이터를 포함할 수 있으며 각각은 NULL로 구분됩니다.
hex(b) : REG_QWORD 유형을 나타내며, 64비트 숫자 값을 저장합니다.
#endif // _DEBUGx

int UcReverseFindStr(const CStringW& s, LPCWSTR needle)
{
	const int lenNeedle = (int)wcslen(needle);
	if (lenNeedle <= 0 || s.GetLength() < lenNeedle)
		return -1;
	for (int i = s.GetLength() - lenNeedle; i >= 0; --i)
		if (s.Mid(i, lenNeedle) == needle)
			return i;
	return -1;
}

CStringW UcMaskConnPwd(const CStringW& conn)
{
	static const CStringW keyPwd = L"PWD=";
	CStringW s(conn);
	int i = 0;
	while (i < s.GetLength())
	{
		int keyPos = -1;
		for (int j = i; j + keyPwd.GetLength() <= s.GetLength(); ++j)
		{
			if (s.Mid(j, keyPwd.GetLength()).CompareNoCase(keyPwd) == 0)
			{
				keyPos = j;
				break;
			}
		}
		if (keyPos < 0)
			break;

		const int valStart = keyPos + keyPwd.GetLength();
		int valEnd = valStart;
		const bool braced = (valStart < s.GetLength() && s[valStart] == L'{');
		if (braced)
		{
			valEnd = s.Find(L'}', valStart);
			if (valEnd < 0)
				valEnd = s.GetLength();
			else
				++valEnd;
		}
		else
		{
			while (valEnd < s.GetLength() && s[valEnd] != L';')
				++valEnd;
		}

		const CStringW masked = braced ? L"{***}" : L"***";
		s.Delete(valStart, valEnd - valStart);
		s.Insert(valStart, masked);
		i = valStart + masked.GetLength();
	}
	return s;
}

CStringW UcShortLambdaName(CStringW sFnc)
{
	CStringW  rs;
	int i0 = -1;
	int iStart = 0;
	do
	{
		i0 = sFnc.Find(L"lambda_", iStart);
		if (i0 >= 0)
		{
			// lambda_8e0e 2933 edc7 f8d6 d258 b2fe 7439 035d
			rs += sFnc.Mid(iStart, i0 + 7 + 4 - iStart);
			iStart = i0 + 7 + 32;
		}
	} while (i0 >= 0);
	rs += sFnc.Mid(iStart);

#ifdef _DEBUG
	CStringW rsl(rs);
#endif // _DEBUG
	static int s_lenMax = 0;
	auto l = rs.GetLength();
	if (s_lenMax < l)
		s_lenMax = l;
	else
	{
		auto lDiff = s_lenMax - l;
		if (lDiff > 0)
		{
			CStringW sp(' ', lDiff);//함수 이름 칸수를 맞추기 위해 스페이를 덧붙인다.
			rs += sp;
			if (lDiff > 30)//덧붙이는 스페이스가 너무 많으면 전체 길이를 50으로 한정 한다.
				s_lenMax = 50;
		}
	}
#ifdef _DEBUG
	CStringW rsd(rs);
	rsd.TrimRight(' ');
	ASSERT(rsl == rsd);
#endif // _DEBUG
	//if(rs.IsEmpty())
	//	rs = sFnc;
	return rs;
}



//BOOL UcAssertFailedLine(LPCWSTR sFunc, LPCSTR sFile, int nLine, LPCSTR sFlag, LPCWSTR smsg = NULL, bool bMDbgBox = true);
BOOL UcAssertFailedLine(LPCWSTR sFunc, LPCSTR sFile, int nLine, LPCSTR sFlag, LPCWSTR smsg, bool bMDbgBox)
{
	CStringW swFlag(sFlag);
	CString st; st.Format(_T("(%s)"), swFlag.GetString());
	CStringW sw(st);
	if (smsg)
	{
		sw += "";
		sw += smsg;
	}
	sw += " ";

	auto ke = new KException("UCASSERT", 0, 0, sw, NULL, sFunc, nLine, sFile, NULL,

#ifdef _DEBUG
		TRUE);
#else
		FALSE);
#endif // _DEBUG
	//tchlen(sFlag) > 0);
	//throw_common_gen(ke); // 진짜 throw 하려면

	//CFileLog log;
	//log.m_sFileName = L"error.log";
	//log.Log(sw);
	//if (s_pErrorHandler)
	//	s_pErrorHandler->_OnError(eErAssert, "ASSERT", lpszFileName, nLine, sl, CStringA(smsg), 0);
#ifdef _DEBUG
	if (bMDbgBox)
	{
		CString st1(sw);
		UINT rv = ::AfxMessageBox(st1, MB_ICONERROR | MB_OKCANCEL);//L"Logical Error!", 
		return rv == IDOK ? TRUE : FALSE;
	}
	else
		return FALSE;
#else
	return FALSE;
#endif // _DEBUG
}

[[deprecated]]
CString UcGetExeFilePath()
{
	CString exeFilePath;
	::GetModuleFileName(NULL, exeFilePath.GetBufferSetLength(MAX_PATH), MAX_PATH);
	exeFilePath.ReleaseBuffer();
	return exeFilePath;
}



#pragma comment(lib, "Version.lib")
//#include <wininet.h>//InternetTimeToSystemTime
CStringW UcGetProductVersion()
{
	static CStringW s_appVer;
	if (!s_appVer.IsEmpty())
		return s_appVer;

	if (HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION))
	{
		if (HGLOBAL hMemInfo = LoadResource(NULL, hResInfo))
		{
			KAtEnd defer([hMemInfo]() { FreeResource(hMemInfo); });

			if (LPVOID lpVerInfo = LockResource(hMemInfo))
			{
				// 버전 정보를 얻기 위한 올바른 방법
				UINT uLen;
				VS_FIXEDFILEINFO* pVerInfo = NULL;
				if (VerQueryValue(lpVerInfo, _T("\\"), (LPVOID*)&pVerInfo, &uLen))
				{
					// 파일 버전
					DWORD dwFileVersionMS = pVerInfo->dwFileVersionMS;
					DWORD dwFileVersionLS = pVerInfo->dwFileVersionLS;
					DWORD dwProductVersionMS = pVerInfo->dwProductVersionMS;
					DWORD dwProductVersionLS = pVerInfo->dwProductVersionLS;

					int nFileVersionMS_hi = HIWORD(dwFileVersionMS); //FILEVERSION 1,
					int nFileVersionMS_lo = LOWORD(dwFileVersionMS); //FILEVERSION    2,     
					int nFileVersionLS_hi = HIWORD(dwFileVersionLS); //FILEVERSION       3,  
					int nFileVersionLS_lo = LOWORD(dwFileVersionLS); //FILEVERSION          4

					CString strProductVersion;
					strProductVersion.Format(_T("%u.%u.%u.%u"),/// PRODUCTVERSION 1, 0, 0, 3 에 해당. 아래쪽 ProductVersion 아님.
						HIWORD(dwProductVersionMS), LOWORD(dwProductVersionMS), /// 리소스 작업 중에도 위에 값 바꾸면 아래는 저절로 바뀐다.
						HIWORD(dwProductVersionLS), LOWORD(dwProductVersionLS));
					s_appVer = strProductVersion;
					return s_appVer;//s_appVer = L"5.6.7.8"
				}
			}
		}
	}

	// 버전 정보 리소스 가져오기
	 //if (HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION))
	 //{
	 //	if (HGLOBAL hMemInfo = LoadResource(NULL, hResInfo))
	 //	{
	 //		KAtEnd defer([hMemInfo]() { FreeResource(hMemInfo); });
	 //		//DWORD dwVerInfoSize = GetFileVersionInfoSize(hResInfo, NULL);
	 //		if (LPVOID lpVerInfo = LockResource(hMemInfo))
	 //		{
	 //			// 버전 정보
	 //			VS_FIXEDFILEINFO *pVerInfo = (VS_FIXEDFILEINFO *)lpVerInfo;
	 //			CStringW strProductVersion;
	 //			strProductVersion.Format(_T("%d.%d.%d.%d"), pVerInfo->dwProductVersionMS, pVerInfo->dwProductVersionLS,
	 //									pVerInfo->dwFileVersionMS, pVerInfo->dwFileVersionLS);
	 //			s_appVer = strProductVersion;
	 //			return s_appVer;
	 //		}
	 //	}
	 //}
	 //	//if (hResInfo == NULL)
	 //	AfxMessageBox(_T("버전 정보 리소스를 찾을 수 없습니다."));
	 //HGLOBAL hMemInfo = LoadResource(NULL, hResInfo);
	 //	AfxMessageBox(_T("버전 정보 블록을 가져올 수 없습니다."));
	 //LPVOID lpVerInfo = LockResource(hMemInfo);
	 //	AfxMessageBox(_T("버전 정보 구조체 포인터를 얻을 수 없습니다."));
	 //VS_FIXEDFILEINFO *pVerInfo = (VS_FIXEDFILEINFO *)lpVerInfo;
	 //DWORD dwVerInfoSize = GetFileVersionInfoSize(hResInfo, &dwVerInfoSize);
	 //if (dwVerInfoSize == 0)
	 //	AfxMessageBox(_T("버전 정보를 가져올 수 없습니다."));
	 //// 버전 정보 출력
	 //CStringW strProductVersion;
	 //strProductVersion.Format(_T("%d.%d.%d.%d"), pVerInfo->dwProductVersionMS, pVerInfo->dwProductVersionLS,
	 //						pVerInfo->dwFileVersionMS, pVerInfo->dwFileVersionLS);
	 //AfxMessageBox(strProductVersion);
	 //// 버전 정보 리소스 해제
	 //FreeResource(hMemInfo);
	return L"(No Version)";
}

int UcRangedRand(int range_min, int range_max)
{
	//srand( (unsigned)time( NULL ) );

	// Generate random numbers in the half-closed interval [range_min, range_max). 
	// In other words, range_min <= random number < range_max
	int u = (int)((double)rand() / (RAND_MAX + 1) * (range_max - range_min) + range_min);
	return u;
}


#include <random>
//  C++11 이상의 버전
int UcGetRandomNumber(int minNum, int maxNum)
{
	// 난수 생성기를 초기화합니다. (시드값으로 시스템 시계를 사용)
	std::random_device rd;
	std::mt19937 gen(rd()); // 메르센 트위스터 알고리즘을 사용하는 엔진
	std::uniform_int_distribution<> dist(minNum, maxNum);
	return dist(gen); // 정의된 분포에 따라 난수를 생성하고 리턴합니다.
}



// BOOL KwCopyTextClipboad(CWnd* pwnd, LPCWSTR text)
BOOL UcCopyTextClipboad(LPCWSTR text, HWND hwnd)  // CWnd* pWnd)
{
	if (hwnd == NULL)
		hwnd = AfxGetApp()->GetMainWnd()->GetSafeHwnd();  /// 이거 때문에 죽어. Desktop으로 해보자.
	/// AfxGetMainWnd() 쓰지마
	if (::OpenClipboard(NULL))  // && ::EmptyClipboard())
	{
		KAtEnd d_cl([]() { ::CloseClipboard(); });
		auto wlen = wcslen(text);
		size_t len = sizeof(WCHAR) * (wlen + 1);  // wcslen(text) + 1;
		if (!EmptyClipboard())
		{
			TRACE("Failed to empty the clipboard.\n");
			return FALSE;
		}
		HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE,
			len);  // GMEM_DDESHARE, GMEM_MOVEABLE을 사용하는 것이 일반적입니다. DDE와 관련된
		// 특정 상황에서는 GMEM_DDESHARE를 사용할 수 있습니다.
		if (hData)
		{
			KAtEnd d_cl2([hData]() { ::GlobalFree(hData); });
			LPVOID lpOut = (LPVOID)GlobalLock(hData);
			if (lpOut)
			{
				KAtEnd d_cl3([hData]() { ::GlobalUnlock(hData); });
				WCHAR* pClipboardData = static_cast<WCHAR*>(GlobalLock(hData));
				if (pClipboardData)
				{
					wcscpy_s(pClipboardData, wlen + 1, text);  //?주의: len 쓰면 안되. wcscpy_s는 wlen +1 은 최대_문자열_길이
					// GlobalSize(hData); 클립보드 복사에서는 이건 안해도 된다.
					if (hData && ::SetClipboardData(CF_UNICODETEXT, hData))  // CF_TEXT
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}
#pragma region MemoApp//[
BOOL UcCopyTextClipboad(CWnd* pWnd, LPCWSTR text)
{
	if (!pWnd)
		pWnd = CWnd::GetDesktopWindow();
	return UcCopyTextClipboad(text, pWnd->GetSafeHwnd());
}

BOOL UcPasteTextClipboard(CWnd* pWnd, CString& str)
{
	HANDLE hData = 0;
	if (pWnd == NULL)
		pWnd = CWnd::GetDesktopWindow();

	if (::OpenClipboard(pWnd->GetSafeHwnd()))
	{
		if (::IsClipboardFormatAvailable(CF_UNICODETEXT))
			hData = ::GetClipboardData(CF_UNICODETEXT);
		else if (::IsClipboardFormatAvailable(CF_TEXT))
			hData = ::GetClipboardData(CF_TEXT);
		else
		{
			::CloseClipboard();
			return FALSE;
		}
		::CloseClipboard();
		if (hData)
		{
			LPVOID lp = GlobalLock(hData);
			if (lp)
			{
				if (::IsClipboardFormatAvailable(CF_UNICODETEXT))
					str = (LPCWSTR)lp;
				else
					str = (LPCSTR)lp;
				GlobalUnlock(hData);
				if (str.GetLength())
					return TRUE;
			}
		}
	}
	return FALSE;
}

int UcGetFileName(CString& cstr, UINT id, CWnd* pParent, BOOL bOpen)
{
	CString ext;
	if (ext.LoadString(id))
		return UcGetFileName(cstr, ext, pParent, bOpen);
	return IDCANCEL;
}

int UcGetFileName(CString& cstr, LPCTSTR ext, CWnd* pParent, BOOL bOpen)
{
	CFileDialog fdlg(bOpen, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		ext, pParent);
	if (fdlg.DoModal() == IDOK)
	{
		cstr = fdlg.GetPathName();
		return IDOK;
	}
	return IDCANCEL;
}

void UcSeparatePathFile(CString& full, CString& path, CString& file)
{
	auto pr = UcCutToFolderAndFile(full);
	path = pr.first;
	file = pr.second;
	if (path.GetLength() && path[path.GetLength() - 1] != _T('\\'))
		path += _T('\\');
}

void UcCheckMessage(DWORD dw, int period)
{
	if (dw % period)
		return;
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void UcAutoMessageBox(CWnd* pWnd, LPCTSTR msg, UINT delayMs)
{
	if (pWnd)
		pWnd->MessageBox(msg, AfxGetAppName(), MB_OK | MB_ICONINFORMATION);
	else
		AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
	if (delayMs > 0)
		UcReady((long)(delayMs / 10 + 1));
}

#pragma endregion//]MemoApp

#pragma region Psapi
#include <Psapi.h>
/// lpszProcessName에 full path 를 준 경우는 특정 경로의 프로세서만 체크하고,
/// 파일명만 주는 경우는, 경로와 상관 없이 체크한다.
int UcIsProcessRunningEx(LPCTSTR lpszProcessName, std::vector<tuple<DWORD, CString, CString>>* dwProcessId)
{
	//bool bRunning = FALSE;
	int nRunning = 0;
	if (!lpszProcessName)
		return 0;
	DWORD		dwSize = 1024 * sizeof(DWORD);
	CStringA sdwPIDs;
	LPDWORD		lpdwPIDs = (LPDWORD)sdwPIDs.GetBuffer(dwSize * sizeof(DWORD));// new DWORD[dwSize];// NULL;
	DWORD		dwSizeRet = 0L;
	do
	{
		if (lpdwPIDs == NULL)
			return 0;
		if (!EnumProcesses(lpdwPIDs, dwSize, &dwSizeRet))
			return 0;

	} while (dwSize == dwSizeRet);

	dwSizeRet /= sizeof(DWORD);

	for (DWORD dwIndex = 0; dwIndex < dwSizeRet; dwIndex++)
	{
		TCHAR	tszFileName[MAX_PATH * 4];//*sizeof(TCHAR)];
		tszFileName[0] = NULL;

		HANDLE	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 1, lpdwPIDs[dwIndex]);
		long PID = (long)lpdwPIDs[dwIndex];
		//TRACE(L"--- %4u(%u) %x %s\r\n", dwIndex, PID, hProcess, lpszProcessName);
		if (hProcess == NULL)
			continue;
		HMODULE  hMod = NULL;

		BOOL ben = EnumProcessModules(hProcess, &hMod, sizeof(hMod), &dwSize);
		if (GetModuleFileNameEx(hProcess, hMod, tszFileName, sizeof(tszFileName) / sizeof(TCHAR)))
		{
			TCHAR	tzsDrive[_MAX_DRIVE];
			TCHAR	tszFName[_MAX_FNAME * 4];
			TCHAR	tzsExt[_MAX_EXT];
			TCHAR	tzsDir[_MAX_DIR];// _MAX_PATH * 4];
			_tsplitpath_s(tszFileName, tzsDrive, _MAX_DRIVE, tzsDir, _MAX_DIR, tszFName, _MAX_FNAME * 4, tzsExt, _MAX_EXT);
			if (_tcsicmp(lpszProcessName, tszFileName) == 0)//tszFileName = L"D:\\Temp\\Alpha\\CADian_13.0.780.35303.P.VC17.x64.Alpha\\Icad.exe"
			{
				TRACE(_T(">>>>>> %4u(%u). %s == %s\r\n"), dwIndex, PID, tszFileName, lpszProcessName);
				nRunning++;
				if (dwProcessId)
				{
					CStringW s; s.Format(L"%4u(%u). %s == %s", dwIndex, PID, tszFileName, lpszProcessName);
					dwProcessId->push_back({ PID, tszFileName, lpszProcessName });
				}
			}
			else//lpszProcessName가 L"Icad.exe"로 경로 없이 주어질때
			{
				//TRACE(L"%4u(%u). %s\r\n", dwIndex, PID, tszFileName);
				_tcscpy_s(tszFileName, tszFName);
				_tcscat_s(tszFileName, tzsExt);//tszFileName = L"Icad.exe"
				if (!_tcsicmp(lpszProcessName, tszFileName))//lpszProcessName가 L"Icad.exe"로 경로 없이 주어질때
				{
					TRACE(_T(">>>>>> %4u(%u). %s == %s\r\n"), dwIndex, PID, tszFileName, lpszProcessName);
					nRunning++;
					if (dwProcessId)
					{
						//CString s; s.Format(_T("%4u(%u). %s == %s"), dwIndex, PID, tszFileName, lpszProcessName);
						dwProcessId->push_back({ PID, tszFileName, lpszProcessName });
					}
				}
			}
		}
		CloseHandle(hProcess);
	}

	return nRunning;
}

/// <summary>
/// 
/// </summary>
/// <param name="sExe">경로 없이 실행 파일명. ex: L"UpdateCourierCheck.exe"</param>
/// <param name="pskill">"kill %s"</param>
/// <returns></returns>
size_t UcKillProcessEx(LPCTSTR sExe, LPCTSTR pskill/* = NULL*/)
{
	DWORD pidToKill = 0;
	FILETIME fcr0 = { 0xffffffff,0xffffffff };//0,0};//

	std::vector<tuple<DWORD, CString, CString>> vtPid;

	if (UcIsProcessRunningEx(sExe, &vtPid) == 0)
		return 0;

	DWORD dwPidThis = GetCurrentProcessId();

	std::vector<tuple<DWORD, CString, CString>>::iterator it = vtPid.begin();
	for (; it != vtPid.end(); it++)
	{
#if CPP17_OR_LATER
		auto& [dwProcID, _1, _2] = *it;
#else
		auto& tuple_ref = *it;
		auto& dwProcID = std::get<0>(tuple_ref);
		auto& _1 = std::get<1>(tuple_ref);
		auto& _2 = std::get<2>(tuple_ref);
#endif
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcID);//	ASSERT(hProcess);
		if (hProcess)
		{
			KAtEnd defer([&hProcess]() {
				CloseHandle(hProcess);
				});
			FILETIME fcr, fex, fkn, fus;
			GetProcessTimes(hProcess, &fcr, &fex, &fkn, &fus);
			if (CompareFileTime(&fcr0, &fcr) > 0) // fcr0 > fcr
				pidToKill = dwProcID;
			fcr0 = fcr;
		}
	}

	if (pidToKill)
	{
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pidToKill);	//ASSERT(hProcess);
		if (hProcess)
		{
			KAtEnd defer([&hProcess]() {
				CloseHandle(hProcess);
				});
			BOOL b = TerminateProcess(hProcess, 1);
			if (!b)
			{
				return -1;
				//if (pskill)//PID가 있는 경우
				//{
				//	// pskill -t -nobanner 4920
				//	CStringW sk; sk.Format(pskill, pidToKill);
				//	_wsystem(sk);
				//}
			}
		}
	}

	return vtPid.size();
}

#pragma endregion Psapi


CStringW UcGetShallExcuteErrorStr(HINSTANCE hInstance)
{
#define ID2STR(x) std::make_pair((INT64)(x), L#x)
	static std::map<INT64, LPCWSTR> g_id2str =
	{
		ID2STR(ERROR_SUCCESS),	 //(0): 
		ID2STR(ERROR_FILE_NOT_FOUND),	 //(2): 지정된 파일을 찾을 수 없음
		ID2STR(ERROR_PATH_NOT_FOUND),	 //(3): 지정된 경로를 찾을 수 없음
		ID2STR(ERROR_BAD_FORMAT),		 //(11): 실행 파일이 손상되었거나 호환되지 않는 형식임
		ID2STR(SE_ERR_ACCESSDENIED),	 //(5): 액세스가 거부됨
		ID2STR(SE_ERR_ASSOCINCOMPLETE),	 //(27): 파일 연결이 완전하지 않거나 등록되지 않음
	};
	auto it = g_id2str.find((INT64)hInstance);
	if (it != g_id2str.end())
		return it->second;
	else
		return L"UnKnown Error";
}

HANDLE UcCheckMutex(CString mutexName, BOOL bMsg, BOOL bOwner)
{
	//LPCWSTR mutexName = L"Global\\MyUniqueApplicationMutexName";
	// Mutex 객체를 생성하거나 기존 객체의 핸들을 엽니다.
	CString sm; sm.Format(_T("Global/%s)"), mutexName.GetString());
	ASSERT(sm.Find('\\') < 0);
	HANDLE hMutex = CreateMutex(NULL, bOwner, sm);
	// CreateMutex 호출 실패를 체크합니다.
	if (hMutex == NULL)
	{
		auto err = GetLastError();///sErr = L"ERROR_PATH_NOT_FOUND(3)" 경로 문자 '\\' 가 들어 가면 안된다.
		CStringW sErr = UcErrorToStrW(err);
		no_throw_str(L"Fail to CreateMutex(%s) : %s", sm.GetString(), sErr.GetString());
		if (err == ERROR_ALREADY_EXISTS)
		{
			if (bMsg)
				UcMessageBoxError(sErr);
			// 애플리케이션이 이미 실행 중임을 의미합니다.
			return NULL; // 중복 실행을 방지하기 위해 종료합니다.
		}
		return NULL;
	}
	//KAtEnd defer([&hMutex]() {
	//	CloseHandle(hMutex);
	//	});

	// GetLastError를 사용하여 Mutex가 이미 존재하는지 체크합니다.
	//auto err = GetLastError();
	//CStringW sErr = UcErrorToStrW(err);
	//if (err == ERROR_ALREADY_EXISTS)
	//{
	//	if(bMsg)
	//		UcMessageBoxError(sErr);
	//	// 애플리케이션이 이미 실행 중임을 의미합니다.
	//	return NULL; // 중복 실행을 방지하기 위해 종료합니다.
	//}
	return hMutex;
}

// CUpdateCourierSetupApp initialization
BOOL UcRegisterAppForAutoStart(LPCTSTR pszAppName, LPCTSTR pszAppPath, bool bSetOrDelete)
{
	CRegKey regKey;
	LONG lResult;
	// HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run 경로에 접근
	lResult = regKey.Open(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), KEY_WRITE);
	if (lResult != ERROR_SUCCESS) // \HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
		return FALSE; // 레지스트리 키를 열 수 없음 . SOFTWARE는 대소문자 구분 없구만. 소문자 섞여도 지워 지는 군.

	// 앱 이름과 경로를 레지스트리에 추가
	if (bSetOrDelete)
		lResult = regKey.SetStringValue(pszAppName, pszAppPath);
	else
		lResult = regKey.DeleteValue(pszAppName);
	if (lResult != ERROR_SUCCESS)
		return FALSE; // 값 설정 실패
	return TRUE; // 성공적으로 등록 완료
}

bool UcWriteFileFromResource(HMODULE hModule, LPCTSTR lpRscName, LPCTSTR lpFile)
{
	bool bOk = false;
	if (hModule == NULL)
		hModule = AfxGetResourceHandle();
	if (hModule == NULL)
		return bOk;
	HRSRC hRes = FindResource(hModule, lpRscName, RT_RCDATA);//_T("RCDATA"));
	//                                 MAKEINTRESOURCE(IDR_UPDATECOURIERCHECK)
	//HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(IDR_MY_BINARY), RT_RCDATA);
	//HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(IDR_MY_BINARY), _T("RCDATA"));
	if (hRes)
	{
		HGLOBAL hData = LoadResource(hModule, hRes);
		if (hData)
		{
			void* pData = LockResource(hData);
			DWORD dwSize = SizeofResource(hModule, hRes);

			// 이제 pData에서 데이터에 접근하고, dwSize로 데이터 크기를 알 수 있습니다. IDR_UPDATECOURIERCHECK
			CFile file;
			if (file.Open(lpFile, CFile::modeCreate | CFile::modeWrite))
			{
				file.Write(pData, dwSize);
				file.Close();
				bOk = true;
			}
		}
	}
	return bOk;
}



/// std::map<int, CString> 임시 객체 생성 → 채워짐
/// 리턴 시 RVO(Return Value Optimization) 또는 move semantics 적용 가능
/// 최적화 켜져 있고, C++11 이상이라면 대부분 move로 넘어옴(deep copy 아님)
/// RVO가 적용되면 애초에 복사 / 이동조차 생략됨
/// _mapStr[LANG_KOREAN] = ... 대입 시 :
/// std::map::operator[] 가 새 std::map<int, CString> 객체를 디폴트 생성
/// 그다음 반환된 map과 move assignment(또는 copy assignment) 발생
/// 즉, 복사라기보다 "기본 생성 + 이동/복사 대입" 구조예요.
/// 실제로는 C++11 이상에서는 std::map도 move assignment를 지원하니까, 대부분 deep copy 대신 shallow move가 일어나고, 내부 노드들 통째로 옮겨져서 꽤 효율적입니다.
/// 
/// auto stringsEnglish2 = LoadAllResourceStringsMap(hModule, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
/// auto stringsKorean2  = LoadAllResourceStringsMap(hModule, MAKELANGID(LANG_KOREAN , SUBLANG_DEFAULT));
/// 
/// TCHAR 버전
std::map<int, CString> UcLoadAllResourceStringsMap(HMODULE hModule, LANGID langID)
{
	std::map<int, CString> strings;
	// 16개 문자열 블록 단위로 리소스를 검색
	for (int blockId = 0; blockId < 1000; ++blockId)
	{ // 예시로 1000개 블록을 검색
		if (HRSRC hResInfo = FindResourceEx(hModule, RT_STRING, MAKEINTRESOURCE(blockId + 1), langID)) {
			if (HGLOBAL hResData = LoadResource(hModule, hResInfo)) {
				if (const TCHAR* pResLoad = (const TCHAR*)LockResource(hResData)) {
					for (int strId = 0; strId < 16; ++strId)
					{ // 한 블록에 최대 16개 문자열
						int len = *pResLoad++;
						if (len > 0) {
							strings[(blockId * 16) + strId] = CString(pResLoad, len);
							pResLoad += len;
						}
					}//for
				}
			}
		}
	}
	return strings;
}

#include <algorithm>
#undef min
// 레벤슈타인 거리 계산 함수
#ifdef _DEBUG1
int levenshteinDistance(const std::string& s1, const std::string& s2) {
	int len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

	for (int i = 0; i <= len1; i++) d[i][0] = i;
	for (int j = 0; j <= len2; j++) d[0][j] = j;

	for (int i = 1; i <= len1; i++) {
		for (int j = 1; j <= len2; j++) {
			int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
			// std::min 정확한 사용을 위해 괄호 사용
			d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + cost);
		}
	}
	return d[len1][len2];
}
std::string findMostSimilarSentence(const std::string& target, const std::vector<std::string>& sentences) {
	if (sentences.empty()) return "[No sentences provided]";

	const std::string* mostSimilar = &sentences[0];
	int minDist = levenshteinDistance(target, sentences[0]);

	for (const auto& sentence : sentences) {
		int currentDist = levenshteinDistance(target, sentence);
		if (currentDist < minDist) {
			minDist = currentDist;
			mostSimilar = &sentence;
		}
	}

	return *mostSimilar;
}
#endif // _DEBUG1


int UcLevenshteinDistance(const CStringW& s1, const CStringW& s2) {
	int len1 = s1.GetLength(), len2 = s2.GetLength();
	std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

	for (int i = 0; i <= len1; i++) d[i][0] = i;
	for (int j = 0; j <= len2; j++) d[0][j] = j;

	for (int i = 1; i <= len1; i++) {
		for (int j = 1; j <= len2; j++) {
			int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
			// std::min 정확한 사용을 위해 괄호 사용
			//d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + cost);
			d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1,  d[i - 1][j - 1] + cost });

		}
	}
	return d[len1][len2];
}


// 가장 유사한 문장 찾기 함수
int UcFindMostSimilarSentence(CStringW target, int cnt, function<CStringW& (int)> getStr)
{
	int ri = -1;
	if (cnt == 0)
		return ri;

	CStringW mostSimilar = getStr(0);
	int minDist = UcLevenshteinDistance(target, mostSimilar);

	for (int i = 0; i < cnt; i++) {
		CStringW& sentence = getStr(i);
		int currentDist = UcLevenshteinDistance(target, sentence);
		if (currentDist <= minDist) {// string 리소스에 하나만 있는 경우, '<'만 하면 실패. '<='으로
			minDist = currentDist;
#ifdef _DEBUG
			mostSimilar = sentence;
#endif // _DEBUG
			ri = i;
		}
	}
	return ri;
}






void UcCpuBusy::initSystemCPUUsage() {

	FILETIME idleTime, kernelTime, userTime;
	GetSystemTimes(&idleTime, &kernelTime, &userTime);

	lastIdleTime.LowPart = idleTime.dwLowDateTime;
	lastIdleTime.HighPart = idleTime.dwHighDateTime;

	lastKernelTime.LowPart = kernelTime.dwLowDateTime;
	lastKernelTime.HighPart = kernelTime.dwHighDateTime;

	lastUserTime.LowPart = userTime.dwLowDateTime;
	lastUserTime.HighPart = userTime.dwHighDateTime;
}

double UcCpuBusy::getSystemCPUUsage()
{
	if (!_bInit)
	{
		initSystemCPUUsage();
		_bInit = true;
	}

	FILETIME idleTime{ 0 }, kernelTime{ 0 }, userTime{ 0 };
	ULARGE_INTEGER idle{ 0 }, kernel{ 0 }, user{ 0 };
	ULARGE_INTEGER sysKernel{ 0 }, sysUser{ 0 }, sysTotal{ 0 }, sysIdleTime{ 0 };

	GetSystemTimes(&idleTime, &kernelTime, &userTime);

	idle.LowPart = idleTime.dwLowDateTime;
	idle.HighPart = idleTime.dwHighDateTime;

	kernel.LowPart = kernelTime.dwLowDateTime;
	kernel.HighPart = kernelTime.dwHighDateTime;

	user.LowPart = userTime.dwLowDateTime;
	user.HighPart = userTime.dwHighDateTime;

	sysIdleTime.QuadPart = idle.QuadPart - lastIdleTime.QuadPart;
	sysKernel.QuadPart = kernel.QuadPart - lastKernelTime.QuadPart;
	sysUser.QuadPart = user.QuadPart - lastUserTime.QuadPart;
	sysTotal.QuadPart = sysKernel.QuadPart + sysUser.QuadPart;

	double percent = 0.0;
	if (sysTotal.QuadPart > 0) {
		percent = (double)(sysTotal.QuadPart - sysIdleTime.QuadPart) / sysTotal.QuadPart * 100.0;
	}

	lastIdleTime = idle;
	lastKernelTime = kernel;
	lastUserTime = user;

	return percent;
}

CString UcSetVerTitle(CWnd* wnd, LPCTSTR head, LPCTSTR tail)
{
	CString strTitle;
	wnd->GetWindowText(strTitle);

	// 버전 및 빌드 타임 정보를 타이틀 앞에 추가
	//auto buildDate = CStringW(__DATE__);  // "Mmm dd yyyy"
	//auto buildTime = CStringW(__TIME__);  // "hh:mm:ss"
	CString exe = UcGetModulePath();
	CString sCTime;
	CString sMTime;
	CString sATime;
	CFileStatus fs{};
	if (UcGetFileStatus(exe, fs))
	{
		sCTime = UcCTimeToString(fs.m_ctime);
		sMTime = UcCTimeToString(fs.m_mtime);//이게 빌드 타임 이네
		sATime = UcCTimeToString(fs.m_atime);
		//+		sATime	L"2024-05-22 10:59:18"
		//+		sCTime	L"2024-05-16 11:04:36"
		//+		sMTime	L"2024-05-22 10:59:17"
	}
	CString strNewTitle;
	//strNewTitle.Format(_T("%s %s - %s"), APP_VERSION, BUILD_TIME, strTitle);
	CString sTitle;
	//sTitle.Format(LR"(%s - Build [%s])", sText.GetString(), sCTime.GetString());//sTitle = L"UpdateCourierCheck Monitor 1.2.0.0"
	if (head)
	{
		if (tail)
			sTitle.Format(_T(R"([%s] %s %s %s)"), sMTime.GetString(), head, strTitle.GetString(), tail);//sTitle = L"UpdateCourierCheck Monitor 1.2.0.0"
		else
			sTitle.Format(_T(R"([%s] %s %s)"), sMTime.GetString(), head, strTitle.GetString());//sTitle = L"UpdateCourierCheck Monitor 1.2.0.0"
	}
	else
	{
		if (tail)
			sTitle.Format(_T(R"([%s] %s %s)"), sMTime.GetString(), strTitle.GetString(), tail);//sTitle = L"UpdateCourierCheck Monitor 1.2.0.0"
		else
			sTitle.Format(_T(R"([%s] %s)"), sMTime.GetString(), strTitle.GetString());//sTitle = L"UpdateCourierCheck Monitor 1.2.0.0"
	}

	wnd->SetWindowText(sTitle);
	return sTitle;
}

/// make_shared는 단일 객체를 할당하는 데 최적화되어 있으며, 배열을 할당하는 데는 적합하지 않습니다.
/// 따라서, std::make_shared를 사용하면 컴파일 오류가 발생합니다.
[[deprecated]] // use SharedBuf instead
std::shared_ptr<char> UcSharedBuffer(ULONG len)
{
	std::shared_ptr<char> shData1(new char[len] {'\0'}, std::default_delete<char[]>());
	return shData1;
}

#ifdef _Sample__
TRACE(L"_bRefreshing(%d), pNMLV->uNewState(%s)\n", _bRefreshing, (PS)UcBitToStr(pNMLV->uNewState, {
	ENUM2STRW(LVIS_FOCUSED),
	ENUM2STRW(LVIS_SELECTED),
	ENUM2STRW(LVIS_CUT),
	ENUM2STRW(LVIS_DROPHILITED),
	ENUM2STRW(LVIS_GLOW),
	ENUM2STRW(LVIS_ACTIVATING),
	ENUM2STRW(LVIS_OVERLAYMASK),
	ENUM2STRW(LVIS_STATEIMAGEMASK),
	}));
#endif // _Sample__
CStringW UcBitToStr(int val, std::map<int, PWS> mapBitStr)
{
	CStringW sbt;
	sbt.Format(L"%d:", val);
#if CPP17_OR_LATER
	for (auto& [k, v] : mapBitStr)
	{
#else
	for (auto& pair : mapBitStr)
	{
		auto& k = pair.first;
		auto& v = pair.second;
#endif
		if (val & k)
		{
			if (sbt.Right(1) != L":")
				sbt += L" | ";
			sbt += v;
		}
	}
	return sbt;
	}
CStringW UcEnumToStr(int val, std::map<int, PWS> mapBitStr)
{
	CStringW sbt;
	auto it = mapBitStr.find(val);
	if (it != mapBitStr.end())
		sbt.Format(L"%d: %s", val, it->second);
	else
		sbt.Format(L"%d: Unknown", val);
	return sbt;
}



std::pair<CStringW, CStringW> UcCutToFolderAndFile(CStringW full, WCHAR cut)
{
	auto vt = UcCutPath(full, 2, cut);
	if (vt.size() == 2)
		return std::make_pair(vt[1], vt[0]);
	else
		return {};
}

std::tuple<CStringW, CStringW, CStringW> UcCutToTwoFolderAndFile(CStringW full, WCHAR cut)
{
	auto vt = UcCutPath(full, 2, cut);
	if (vt.size() == 3)
		return std::make_tuple(vt[2], vt[1], vt[0]);
	else
		return {};
}

// full path를 뒤에서 부터 잘라서 갯수만큼 담아 리턴한다. 그러면 역순으로 쌓이겠지.
/// n==0 이면 파일, 경로 2개로 나눈다.
/// n > 1 이면 뒤에서 부터 잘라서 차례대로 넣는다.
std::vector<CStringW> UcCutPath(CStringW full, int n, WCHAR cut)
{
	std::vector<CStringW> arv;
	CStringW curPath(full);
	curPath.TrimRight(cut);// 맨뒤에 '/' 또는 '\\' 는 제거
	if (n == 0)
	{
		auto i0 = curPath.ReverseFind(cut);
		if (i0 >= 0)
		{
			auto sTail = curPath.Mid(i0 + 1);
			curPath = curPath.Left(i0);
			arv.push_back(sTail);
			arv.push_back(curPath);
		}
	}
	else
	{
		for (int i = 0; i < n; i++)
		{
			auto i0 = curPath.ReverseFind(cut);
			if (i0 >= 0)
			{
				auto sTail = curPath.Mid(i0 + 1);
				curPath = curPath.Left(i0);
				arv.push_back(sTail);
			}
			else
				break;
		}
	}
	return arv;
}

std::tuple<CStringW, CStringW, CStringW> UcCutFile(CStringW full, WCHAR cut)
{
	CStringW sPath, sFile, sFileOnly, sExt;
	auto ibs = full.ReverseFind(cut);
	if (ibs >= 0)
	{
		sFile = full.Mid(ibs + 1);
		sPath = full.Left(ibs);
	}

	auto idt = sFile.ReverseFind('.');
	if (idt >= 0)
	{
		sExt = sFile.Mid(idt + 1);
		sFileOnly = sFile.Left(idt);
	}
	else
		sFileOnly = sFile;
	return std::make_tuple(sPath, sFileOnly, sExt);
}

#include <wtsapi32.h>//WTSQuerySessionInformation 
#pragma comment(lib, "Wtsapi32.lib")//Windows 터미널 서비스(Terminal Services, TS) API

/// bUpload true일떄는 update/ ~부터 서브디렉토리/파일 까지 리턴
/// false download일떄는 https:// 부터 full path를 준다.
/// 하나라도 실패 하면 nullptr을 반환한다.
/// usage:
/// 				auto tp = UcGetMyComputerInfo();
/// if (tp) {
///     auto& [sIp, sPc, sUser] = *tp;
/// }
std::shared_ptr<std::tuple<CString, CString, CString>> UcGetMyComputerInfo(bool bIP, bool bDefault)
{
	CString user;

	//LPWSTR bufUsr = NULL;
	//DWORD bufferLength = 0;
	//if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &bufUsr, &bufferLength))
	HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;
	DWORD activeSsessionId = WTSGetActiveConsoleSessionId(); // 활성 콘솔 세션 ID 가져오기//이건 서버에서 공백이 온다.
	LPTSTR bufUsr = NULL;
	DWORD bufferLength = 0;

	//if (WTSQuerySessionInformation(hServer, activeSsessionId, WTSUserName, &bufUsr, &bufferLength)) //이건 서버에서 공백이 온다.
	if (WTSQuerySessionInformation(hServer, WTS_CURRENT_SESSION, WTSUserName, &bufUsr, &bufferLength))
	{
		TRACE(_T("Logged in user: %s\n "), bufUsr);
		user = bufUsr;
		WTSFreeMemory(bufUsr);
	}
	else
	{
		TRACE(_T("Failed to get user name, error: %u\n"), GetLastError());
		if (bDefault)
		{
			auto sGuid = UcGetFormattedGuid(false);
			user.Format(_T("ErrUser_%s"), (PS)sGuid.Left(8));
		}
		else
			return {};// std::nullopt; <optional> 일떄
	}
	static CString sIp;
	// 외부에서 보는 내 IP는 내 스스로 알 수 없다. 그래서 외부 사이트의 힘을 빌린다.
	if (sIp.IsEmpty())
	{
		if (bIP)
		{
			sIp = UcGetExternalIP().c_str();//+sIp	L"61.97.120.203" 외부 사이트에서 서비스 한다.
			if (sIp.IsEmpty())
			{
				if (bDefault)
				{
					auto sGuid = UcGetFormattedGuid(false);
					sIp.Format(_T("ErrIP_%s"), (PS)sGuid.Left(8));
				}
				else
					return {};//std::nullopt;
			}
		}
	}
	sIp.Trim();// 이거 안하면 HTTP_STATUS_BAD_REQUEST(400)

	CString sPC;
	DWORD size = 1024;// sizeof(sPC) / sizeof(sPC[0]);//sPC = "CADIAN-DEV-DWK" //?CDANAL
	auto bufPC = sPC.GetBuffer(size);
	if (!GetComputerNameEx(ComputerNameDnsFullyQualified, bufPC, &size))//sysinfoapi.h
	{
		sPC.ReleaseBuffer();
		no_throw_str(L"Failed to get the FQDN of the computer");
		if (bDefault)
		{
			auto sGuid = UcGetFormattedGuid(false);
			sPC.Format(_T("ErrPC_%s"), (PS)sGuid.Left(8));
		}
		else
			return nullptr; // 실패 시 nullptr 반환
	}
	else
		sPC.ReleaseBuffer();
	return std::make_shared<std::tuple<CString, CString, CString>>(sIp, sPC, user);
}


/// <summary>
/// 특정 문자로 구분 하여 몇조각까지만 있고 뒤를 잘라낸 나머지.
/// src = L"13.0.60.33826.P.VC17.x64.Alpha" 4를 주면 "13.0.60.33826"가 된다.
/// </summary>
/// <param name="src">원본</param>
/// <param name="cCut">자르는 글자</param>
/// <param name="nStr">몇 조각까지 보존</param>
/// <returns></returns>
CString UcCutStrByChar(CString src, TCHAR cCut, int nStr)
{
	ASSERT(nStr > 0);
	ASSERT(src.GetLength() > 0);
	//src = L"13.0.60.33826.P.VC17.x64.Alpha"
	src.Trim(cCut);
	int nFnd = 0;
	int iStart = 0;
	CString tar;
	for (;;)
	{
		int i0 = src.Find(cCut, iStart);
		if (i0 > 0)
			++nFnd;
		else
			break;
		if (tar.GetLength() > 0)
			tar += cCut;
		tar += src.Mid(iStart, i0 - iStart);
		if (nFnd == nStr)
			break;
		iStart = i0 + 1;
	}
	return tar;
}





#ifdef _UseReLauceModule__
#include <comdef.h>

//#pragma push_macro("byte")
//#undef byte
//#include <Wbemidl.h>
//#pragma pop_macro("byte")

//#define byte rpcndr_byte
#include <Wbemidl.h>   여기서 : error C2872: 'byte': ambiguous symbol  나므로 나중에 사용하려면 파일을 따로 뺴서 어느 using std 도 없이 써야 한다.
//#undef byte

#pragma comment(lib, "wbemuuid.lib")

bool InitializeCOM() {
	HRESULT hres;

	// Initialize COM.
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		//std::cerr << "Failed to initialize COM library. Error code = 0x"
		//	<< std::hex << hres << std::endl;
		return false;
	}

	// Initialize COM security.
	hres = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);

	if (FAILED(hres)) {
		//std::cerr << "Failed to initialize security. Error code = 0x"
		//	<< std::hex << hres << std::endl;
		CoUninitialize();
		return false;
	}

	return true;
}

void CleanupCOM() {
	CoUninitialize();
}

bool UcIsProcessWithArgsRunning(const std::wstring processName, const std::wstring args) {
	HRESULT hres;
	IWbemLocator* pLoc = NULL;
	IWbemServices* pSvc = NULL;

	// Obtain the initial locator to WMI.
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres)) {
		//std::cerr << "Failed to create IWbemLocator object. Error code = 0x"
		//	<< std::hex << hres << std::endl;
		return false;
	}

	// Connect to WMI.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc);

	if (FAILED(hres)) {
		//std::cerr << "Could not connect to WMI namespace ROOT\\CIMV2. Error code = 0x"
		//	<< std::hex << hres << std::endl;
		pLoc->Release();
		return false;
	}

	// Set security levels on the proxy.
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE);

	if (FAILED(hres)) {
		//std::cerr << "Could not set proxy blanket. Error code = 0x"
		//	<< std::hex << hres << std::endl;
		pSvc->Release();
		pLoc->Release();
		return false;
	}

	// Use WMI to retrieve the command line arguments of all processes.
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_Process"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres)) {
		//std::cerr << "WMI query for processes failed. Error code = 0x"
		//	<< std::hex << hres << std::endl;
		pSvc->Release();
		pLoc->Release();
		return false;
	}

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;
	bool isRunning = false;

	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn) {
			break;
		}

		VARIANT vtProp;
		hr = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
			std::wstring cmdLine(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
			if (cmdLine.find(processName) != std::wstring::npos) {
				if (cmdLine.find(args) != std::wstring::npos)//cmdLine = L"\"C:\\Outbin\\UpdateCourier\\x64\\Release\\UpdateCourierCheck.exe\" /NoMutax /templer"
					isRunning = true;
			}
		}
		VariantClear(&vtProp);

		pclsObj->Release();

		if (isRunning) {
			break;
		}
	}

	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();

	return isRunning;
}


#endif // _UseReLauceModule__





LONG RecursiveDeleteKey(HKEY hKeyRoot, LPCTSTR lpSubKey)
{
	CRegKey regKey;
	LONG lResult = regKey.Open(hKeyRoot, lpSubKey, KEY_READ | KEY_WRITE);
	if (lResult != ERROR_SUCCESS)
		return lResult;//HKEY_LOCAL_MACHINE인 경우 관리자 모드로만 가능 하다.

	// Get the first subkey name
	TCHAR szSubKeyName[MAX_PATH];
	DWORD dwSubKeyNameSize = MAX_PATH;
	FILETIME ftLastWriteTime;

	while (RegEnumKeyEx(regKey, 0, szSubKeyName, &dwSubKeyNameSize, NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS)
	{
		lResult = RecursiveDeleteKey(hKeyRoot, CString(lpSubKey) + _T("\\") + szSubKeyName);
		if (lResult != ERROR_SUCCESS)
			return lResult;

		dwSubKeyNameSize = MAX_PATH; // Reset the buffer size for the next iteration
	}

	regKey.Close();
	lResult = RegDeleteKey(hKeyRoot, lpSubKey);
	return lResult;
}

/// sSubKey = L"SOFTWARE\\MyCompany\\MyApp"
/// HKEY hKeyRoot = HKEY_CURRENT_USER
LONG UcDeleteKeyWithSubkeys(HKEY hKeyRoot, CString sSubKey)
{
	//HKEY hKeyRoot = HKEY_CURRENT_USER; // 예를 들어 HKEY_CURRENT_USER

	LONG lResult = RecursiveDeleteKey(hKeyRoot, sSubKey);
	return lResult; //ERROR_SUCCESS
}

/// registry 특정 키와 그 아래 키들을 2단계만 recursive 스캔 하면서 람다 함수 호출
int UcRegistryRecursiveGeneral2Level(HKEY hKeyParent, LPCTSTR lpszKeyName
	, function<void(CRegKey&)> cbRoot
	, function<int(CRegKey&, CString)> cbSub, REGSAM sam)
{
	CRegKey regKeyPt;
	LONG result = 0;
	//result = regKeyPt.Open(HKEY_CURRENT_USER, UC_REGKEY_PATCHSTAT, KEY_READ | KEY_WRITE);
	result = regKeyPt.Open(hKeyParent, lpszKeyName, sam);
	if (result != ERROR_SUCCESS)
		return -1;//throwLINE;

	if (cbRoot)
		cbRoot(regKeyPt);

	DWORD index = 0;
	DWORD nameLen = MAX_PATH;
	CString sName;
	LPTSTR bufGUID = sName.GetBuffer(MAX_PATH);
	while (ERROR_SUCCESS == regKeyPt.EnumKey(index, bufGUID, &nameLen))
	{
		index++;
		CRegKey regKeyApps;
		result = regKeyApps.Open(regKeyPt, bufGUID, KEY_READ | KEY_WRITE);
		if (result == ERROR_SUCCESS && cbSub)
		{
			if (cbSub(regKeyApps, bufGUID) == -1)// 콜백에서 -1을 리턴 하면 루프 탈출
				break;
		}
		nameLen = MAX_PATH;//?중요
		_break;
	}
	sName.ReleaseBuffer();
	return 0;
}


void UcSaveRegistryKeyValuesLoop(CRegKey & regKey, function<int(int, LPCTSTR, BYTE*, DWORD)> cb)//HKEY hKeyParent, LPCTSTR lpSubKey, LPCTSTR lpFilePath)
{
	LONG lResult;

	// 레지스트리 값 열거
	DWORD dwIndex = 0;
	TCHAR szValueName[256];
	BYTE szData[1024];
	DWORD dwValueNameSize = 256;
	DWORD dwDataSize = 1024;
	DWORD dwType = 0;

	while (true)
	{
		dwValueNameSize = 256; // 각 반복에서 크기를 재설정
		dwDataSize = 1024;     // 각 반복에서 크기를 재설정

		lResult = RegEnumValue(regKey, dwIndex, szValueName, &dwValueNameSize, NULL, &dwType, szData, &dwDataSize);
		if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break; // 모든 값을 열거한 경우 루프 종료
		}
		else if (lResult != ERROR_SUCCESS)
		{
			//"Failed to enumerate value, error: "
			break;
		}

		cb(dwIndex, szValueName, szData, dwType);

		dwIndex++;
	}
}




bool UcIsValidGUID(LPCWSTR guid)
{
	// 정규 표현식 패턴 (대소문자 구분 없이)
	std::wregex guidPattern(LR"(\{?[0-9A-Fa-f]{8}\-[0-9A-Fa-f]{4}\-[0-9A-Fa-f]{4}\-[0-9A-Fa-f]{4}\-[0-9A-Fa-f]{12}\}?)");
	//                             085E420C       -596B           -4285           -B09C           -1F4E38C373B2
	// CStringW을 std::wstring으로 변환
	std::wstring guidStr(guid);

	// 정규 표현식으로 GUID 검증
	return std::regex_match(guidStr, guidPattern);
}


int UcCutStrByChar(TCHAR c, CString s, std::function<void(LPCTSTR)> cb, bool bIgnorFirst, bool bIgnorLast)
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
					cb(_T(""));
					++n;
				}
				//ar.push_back(L"");
			}
			else//if(i1 != 0) //맨앞이 '~'이면 '~' 앞에는 널이므로 넣지 않는다. 즉 맨앞의 '~'는 무시 한다.
			{
				cb((LPCTSTR)s.Mid(i0, i1 - i0));
				++n;
				//ar.push_back((LPCWSTR)s.Mid(i0, i1 - i0));
				if (bIgnorLast && i1 == s.GetLength() - 1)
					return n;// ar.size();
			}
			i0 = i1 + 1;
		}
		else
			break;
	}

	if (i0 == s.GetLength())
	{
		cb(_T(""));
		++n;
	}
	//ar.push_back(L"");
	else
	{
		cb((LPCTSTR)s.Mid(i0));
		++n;
		//ar.push_back((LPCWSTR)s.Mid(i0));
	}
	return n;// ar.size();
	//return (int)ar.size();

}
//template<typename TCH>
//void UcCutByTokenT(const TCH* psSrc, const TCH* seps, std::function<void(const TCH*)> cb, bool bTrim)
//{
//	if (tchlen(psSrc) == 0)
//		return;
//	TCH* pSrc = new TCH[tchlen(psSrc) + 1];
//	KAtEnd _d([&pSrc]() { delete pSrc; });
//	//shared_ptr<TCHAR> _d(pSrc);
//	TCH* next_token1 = NULL;
//
//	ASSERT(pSrc);
//	tchcpy(pSrc, psSrc);
//	TCH* tok = _tcstok_s(pSrc, seps, &next_token1);
//	for (int i = 0; tok != NULL; i++)
//	{
//		if (bTrim)
//		{
//			CString stok(tok);
//			stok.Trim();
//			cb(stok);
//		}
//		else
//			cb(tok);
//		tok = _tcstok_s(NULL, seps, &next_token1);
//	}
//}
#include <cstring>     // for strtok_s
#include <cwchar>      // for wcstok_s
#include <tchar.h>     // for TCHAR, _tcstok_s
#include <functional>
#include <atlstr.h>    // for CString

template <typename TCH>
using TokenizerFunc = TCH * (*)(TCH*, const TCH*, TCH**);

template <typename TCH>
TokenizerFunc<TCH> GetTokenizer();

template <>
TokenizerFunc<char> GetTokenizer<char>() {
	return &strtok_s;
}

template <>
TokenizerFunc<wchar_t> GetTokenizer<wchar_t>() {
	return &wcstok_s;
}


#ifdef _Bad_Pointer_Dameged_
template <typename TCH, typename Callback>
void UcCutByTokenT_err(const TCH * pSrc, const TCH * seps, Callback cb, bool bTrim = false)
{
	TCH* next_token = nullptr;
	TokenizerFunc<TCH> tokenizer = GetTokenizer<TCH>();
	TCH* pSrc1 = (TCH*)pSrc;
	TCH* tok = tokenizer(pSrc1, seps, &next_token);
	for (int i = 0; tok != nullptr; i++)
	{
		if (bTrim)
		{
			CStringT<TCH, StrTraitATL<TCH>> stok(tok);
			stok.Trim();
			cb((const TCH*)stok.GetString());
		}
		else
		{
			cb(tok);
		}
		tok = tokenizer(nullptr, seps, &next_token);
	}
}
#endif // _Bad_Pointer_Dameged_


/// 문자열 자른 후, 람다함수로 넘긴다.
UCTOOLDYNAMIC
void UcCutByToken(LPCTSTR psSrc, LPCTSTR seps, std::function<void(LPCTSTR)> cb, bool bTrim)
{
	if (lstrlen(psSrc) == 0)
		return;
	TCHAR* pSrc = new TCHAR[_tcslen(psSrc) + 1];
	KAtEnd _d([&pSrc]() { delete pSrc; });
	//shared_ptr<TCHAR> _d(pSrc);
	TCHAR* next_token1 = NULL;

	ASSERT(pSrc);
	tchcpy(pSrc, psSrc);
	TCHAR* tok = _tcstok_s(pSrc, seps, &next_token1);
	for (int i = 0; tok != NULL; i++)
	{
		if (bTrim)
		{
			CString stok(tok);
			stok.Trim();
			cb(stok);
		}
		else
			cb(tok);
		tok = _tcstok_s(NULL, seps, &next_token1);
	}
}

UCTOOLDYNAMIC
void UcCutByTokenA(LPCSTR psSrc, LPCSTR seps, std::function<void(LPCSTR)> cb, bool bTrim)
{
	auto len = tchlen(psSrc);
	if (len == 0)
		return;
	CHAR* pSrc = new CHAR[len + 1];
	//shared_ptr<CHAR> _d(pSrc);
	KAtEnd _d([&pSrc]() { delete pSrc; });
	CHAR* next_token1 = NULL;

	ASSERT(pSrc);
	tchcpy(pSrc, psSrc);
	CHAR* tok = strtok_s(pSrc, seps, &next_token1);
	for (int i = 0; tok != NULL; i++)
	{
		if (bTrim) {
			CStringA stok(tok);
			stok.Trim();
			cb(stok);
		}
		else
			cb(tok);
		tok = strtok_s(NULL, seps, &next_token1);
	}
}

//dwk: 2025-10-24 15:32 
//std::vector<int> UcCutByTokenInt(LPCTSTR psSrc, LPCTSTR seps, bool bTrim)
//{
//	std::vector<int> ars;
//	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
//		ars.push_back(UcAtoi(str));//token.GetString()
//		}, bTrim);
//	return ars;
//}
//std::vector<double> UcCutByTokenDouble(LPCTSTR psSrc, LPCTSTR seps, bool bTrim)
//{
//	std::vector<double> ars;
//	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
//		ars.push_back(UcAtof(str));//token.GetString()
//		}, bTrim);
//	return ars;
//}



UCTOOLDYNAMIC
void UcCutByTokenW(LPCWSTR psSrc, LPCWSTR seps, std::vector<wstring>& ars, bool bTrim)
{
	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
		ars.push_back(str);
		}, bTrim);
}
//1 > error LNK2019 : "int UcCutByTokenA(char const *,char const *,class std::vector<string>&,int)" 

UCTOOLDYNAMIC
void UcCutByTokenA(LPCSTR psSrc, LPCSTR seps, std::vector<std::string>& ars, bool bTrim)
{
	UcCutByTokenT(psSrc, seps, [&ars](auto str) {
		ars.push_back(str);
		}, bTrim);
	//UcCutByTokenA(psSrc, seps, [&ars](LPCSTR str) {
	//	ars.push_back(str);
	//	}, bTrim);
}

void UcSwapBytes(char* p, int length)
{
	ASSERT(length % 2 == 0);
	for (int i = 0; i < length; i += 2)
		std::swap(p[i], p[i + 1]); // <alforithm>
}


std::map<string, int> UcContainsHowMany(const char* pA, const std::vector<std::string>&strings, UINT_PTR len)
{
	DWKFUNC;
	std::map<string, int> kcnt;
	if (!pA)
		return kcnt;
	int bLE = -1;
	BOOL bUni = UcIsUicodeFileData(pA, bLE);
	if (bUni)
	{
		char* ab = (char*)pA;
		if (!bLE)
			UcSwapBytes(ab, (int)len); // Big Endian
		std::vector<std::wstring> wstrings;
		for (const std::string& str : strings)
		{
			CStringW sw(str.c_str());
			wstrings.push_back((PWS)sw);
		}
		CStringW wa(reinterpret_cast<PWS>(pA), (int)len / sizeof(wchar_t));
		for (int i = 0; wa[i] != '\0'; ++i) {
			for (auto& str : wstrings) {
				int k = 0;
				while (wa[i + k] == str[k]) {//키하나 DWK__ 다 맞을때 까지
					++k;
					if (str[k] == '\0') { // "DWK__\0"
						CStringA sa(str.c_str());
						kcnt[(PAS)sa]++; // 하나 증가
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; pA[i] != '\0'; ++i) {
			for (auto& str : strings) {
				int k = 0;
				while (pA[i + k] == str[k]) {//키하나 DWK__ 다 맞을때 까지
					++k;
					if (str[k] == '\0') { // "DWK__\0"
						kcnt[str]++; // 하나 증가
					}
				}
			}
		}
	}
	return kcnt;
}

#ifdef _Use_WSH__
//#include <windows.h>
//#include <atlbase.h>
#include <comutil.h>
//#include <iostream>
#include <activscp.h>
#include <comdef.h>

#pragma comment(lib, "comsuppw.lib")
// CLSID for VBScript
EXTERN_C const CLSID CLSID_VBScript =
{ 0xB54F3741, 0x5B07, 0x11cf, {0xA4, 0xB0, 0x00, 0xAA, 0x00, 0x4A, 0x55, 0xE8} };

// CLSID for JScript
EXTERN_C const CLSID CLSID_JScript =
{ 0xF414C260, 0x6AC0, 0x11CF, {0xB6, 0xD1, 0x00, 0xAA, 0x00, 0xBB, 0xBB, 0x58} };
#include <activscp.h>
#include <atlbase.h> // For CComPtr and COM utilities

class UcScriptSite : public IActiveScriptSite {
public:
	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override {
		DWKFUNC;
		if (ppvObject == nullptr) return E_POINTER;
		if (riid == IID_IUnknown || riid == IID_IActiveScriptSite) {
			*ppvObject = static_cast<IActiveScriptSite*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)() override {
		DWKFUNC;
		return ++m_refCount;
	}

	STDMETHOD_(ULONG, Release)() override {
		DWKFUNC;
		if (--m_refCount == 0) {
			delete this;
			return 0;
		}
		return m_refCount;
	}

	// IActiveScriptSite methods
	STDMETHOD(GetLCID)(LCID* plcid) override {
		DWKFUNC;
		if (!plcid) return E_POINTER;
		*plcid = LOCALE_USER_DEFAULT;
		return S_OK;
	}

	STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppunkItem, ITypeInfo** ppTypeInfo) override {
		DWKFUNC;
		if (ppunkItem) *ppunkItem = nullptr;
		if (ppTypeInfo) *ppTypeInfo = nullptr;

#ifdef _InCase_has_namedItem__
		if (wcscmp(pstrName, L"app") == 0) {
			*ppunkItem = m_pApp; // m_pApp은 등록하려는 IDispatch* 객체
			if (*ppunkItem) {
				(*ppunkItem)->AddRef();
			}
			return S_OK;
		}
#endif // _InCase_has_namedItem__



		return TYPE_E_ELEMENTNOTFOUND;
	}

	STDMETHOD(GetDocVersionString)(BSTR* pbstrVersion) override {
		DWKFUNC;
		if (!pbstrVersion) return E_POINTER;
		*pbstrVersion = SysAllocString(L"1.0");
		return *pbstrVersion ? S_OK : E_OUTOFMEMORY;
	}
	STDMETHODIMP OnScriptTerminate(const VARIANT* pvarResult, const EXCEPINFO* pexcepinfo) override {
		DWKFUNC;
		//std::wcout << L"Script terminated.\n";
		if (pvarResult) {
			//std::wcout << L"Result: " << pvarResult->bstrVal << L"\n";
		}
		if (pexcepinfo) {
			//std::wcout << L"Error: " << pexcepinfo->bstrDescription << L"\n";
		}
		return S_OK;
	}
	STDMETHOD(OnScriptError)(IActiveScriptError* pError) override {
		DWKFUNC;
		if (!pError)
			return E_POINTER;

		EXCEPINFO excepInfo;
		if (SUCCEEDED(pError->GetExceptionInfo(&excepInfo))) {
			DWKTRACE(L"Script Error: %s\n", (excepInfo.bstrDescription ? excepInfo.bstrDescription : L"(No description)"));
		}

		DWORD dwContext;
		ULONG ulLineNumber{};
		LONG lCharPosition{};
		if (SUCCEEDED(pError->GetSourcePosition(&dwContext, &ulLineNumber, &lCharPosition))) {
			DWKTRACE(L"Line: %u, Char: %d\n", ulLineNumber, lCharPosition);
		}

		return S_OK;
	}

	STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState) override {
		DWKFUNC;
		return S_OK;
	}

	STDMETHOD(OnEnterScript)() override {
		DWKFUNC;
		return S_OK;
	}

	STDMETHOD(OnLeaveScript)() override {
		DWKFUNC;
		return S_OK;
	}

private:
	ULONG m_refCount = 1;
};

CStringW UcExecuteScript(const wchar_t* script, const wchar_t* language)
{
	ASSERT(!wcscmp(language, L"VBScript") || !wcscmp(language, L"JScript"));
	std::wstringstream ss;
	HRESULT hr;
	CComPtr<IActiveScript>      spActiveScript;
	//CComPtr<IActiveScriptParse> spActiveScriptParse;//dwk: interface 추출 방법1

	// Initialize COM
	hr = CoInitialize(nullptr);
	if (FAILED(hr))
	{
		//std::wcerr << L"Failed to initialize COM.\n";
		return {};
	}
	try
	{
		// Create the scripting engine
		hr = CoCreateInstance(
			(wcscmp(language, L"VBScript") == 0) ? CLSID_VBScript : CLSID_JScript,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IActiveScript,
			(void**)&spActiveScript
		);
		if (FAILED(hr))
		{
			//std::wcerr << L"Failed to create scripting engine.\n";
			throw hr;
		}

#define _dwk_try3__
		// Query for IActiveScriptParse
#ifdef _dwk_try2__
		CComQIPtr<IActiveScriptParse, &IID_IActiveScriptParse> spActiveScriptParse = spActiveScript;//dwk: interface 추출 방법2
		if (!spActiveScriptParse)
			throw hr;
#elif defined(_dwk_try1__)
		hr = spActiveScript->QueryInterface(IID_IActiveScriptParse, (void**)&spActiveScriptParse);//dwk: interface 추출 방법1.1
		if (FAILED(hr))//L"Failed to query IActiveScriptParse interface.\n";
			throw hr;
#elif defined(_dwk_try3__)
		CComQIPtr<IActiveScriptParse, &IID_IActiveScriptParse> spActiveScriptParse;//dwk: interface 추출 방법3
		HRESULT hr = spActiveScript.QueryInterface(&spActiveScriptParse);
		if (FAILED(hr))//L"Failed to query IActiveScriptParse interface.\n";
			throw hr;
#elif defined(_dwk_try4__)
		UcComQIPtr<IActiveScriptParse> spActiveScriptParse = spActiveScript;//dwk: interface 추출 방법4 -  ㄱㅖ속 에러
		if (spActiveScriptParse)//L"Failed to query IActiveScriptParse interface.\n";
			throw spActiveScriptParse._hr;
#endif
		// Initialize the script parser
		hr = spActiveScriptParse->InitNew();
		if (FAILED(hr))//std::wcerr << L"Failed to initialize script parser.\n";
			throw hr;

		// Set the script site (can be null for basic use)
		//hr = spActiveScript->SetScriptSite(nullptr);//이건 애초에 에러

		//auto pScriptSite = new UcScriptSite();//dwk: OK이지만, delete해야

		//CComPtr<UcScriptSite> pScriptSite;//dwk: com 객체가 아님 try 1 hr = E_POINTER Invalid pointer.
		//hr = pScriptSite.CoCreateInstance(__uuidof(UcScriptSite));

		auto pScriptSite = std::make_shared<UcScriptSite>();//dwk: try 2
		hr = spActiveScript->SetScriptSite(pScriptSite.get());
		if (FAILED(hr))//"Failed to set script site.\n";
			throw hr;


#ifdef _InCase_has_namedItem__
		hr = spActiveScript->AddNamedItem(L"app", SCRIPTITEM_ISVISIBLE);
		if (FAILED(hr)) {
			throw hr;
		}
#endif // _InCase_has_namedItem__

		// Execute the script
		VARIANT result;
		VariantInit(&result);
		EXCEPINFO ei = {};
		hr = spActiveScriptParse->ParseScriptText(
			script,     // Script code
			nullptr,    // Script item
			nullptr,    // Context object
			nullptr,    // Error context
			0,          // Starting line number
			0,          // Reserved for future use
			SCRIPTTEXT_ISEXPRESSION, // Flags to evaluate as an expression
			&result,    // Result
			&ei         // Exception info
		);

		if (FAILED(hr))
		{
			//std::wcerr << L"Failed to execute script.\n";
			throw hr;
		}

		// Set the script state to started
		hr = spActiveScript->SetScriptState(SCRIPTSTATE_STARTED);
		if (FAILED(hr))
		{
			//std::wcerr << L"Failed to set script state to started.\n";
			throw hr;
		}

		// Display the result
		if (result.vt == VT_BSTR) // String result
			ss << result.bstrVal;
		else if (result.vt == VT_I4) // Integer result
			ss << result.lVal;
		else
			ss << L"(unsupported type)";

		VariantClear(&result);
	}
	catch (HRESULT caughtHr) {
		hr = caughtHr;// Handle specific errors or cleanup if necessary
	}

	// Cleanup
	//spActiveScriptParse.Release();
	//spActiveScript.Release();
	CoUninitialize();
	if (SUCCEEDED(hr)) {// Script executed successfully.
	}
	return std::move(CStringW(ss.str().c_str()));
}

int Sample_script_()
{
	DWKFUNC;
	const wchar_t* script = L"1 + 1"; // JavaScript expression
	auto sr1 = UcExecuteScript(script, L"JScript");//sr1 = L"2"
	const wchar_t* vbscript = LR"("Hello " & "World!")";// L"2 + 2";
	auto sr2 = UcExecuteScript(vbscript, L"VBScript");//sr2 = L"Hello World!"
	return 0;
}
#endif // _Use_WSH__



std::tuple<CString, CString, CString, CString> UcSplitPath(LPCTSTR sFull)
{
	CString drive, dir, fname, ext;
	//pathname = L"C:\\scadaBin\\HMIBuilder\\x64\\DebugSUC\\HMIBuilder.exe"
	_tsplitpath_s(sFull, drive.GetBuffer(_MAX_DRIVE), _MAX_DRIVE, dir.GetBuffer(_MAX_DIR), _MAX_DIR,
		fname.GetBuffer(_MAX_FNAME), _MAX_FNAME, ext.GetBuffer(_MAX_EXT), _MAX_EXT);
	drive.ReleaseBuffer();
	dir.ReleaseBuffer();
	fname.ReleaseBuffer();
	ext.ReleaseBuffer();
	return std::make_tuple(std::move(drive), std::move(dir), std::move(fname), std::move(ext));
	//현대 컴파일러(C++17 이상)에서는 반환값 최적화(RVO)가 기본적으로 적용됩니다. 
	// 따라서 std::make_tuple 내부에서 로컬 변수를 직접 반환할 때도 이동 생성자가 활용될 가능성이 높습니다.
}

//#include <tchar.h>
#include <cctype>
#include <string>

bool UcIsValidFileName(const TCHAR * filename, bool bSpaseALlso) {
	// 금지된 문자 목록 (Windows 파일명 제한 문자)
	CString invalidChars = _T("\\/:*?\"<>|");
	if (bSpaseALlso)
		invalidChars += _T(" ");//dwk:302
	// 문자열이 비었거나 너무 길면 무효
	if (filename == nullptr || _tcslen(filename) == 0 || _tcslen(filename) > MAX_PATH) {
		return false;
	}
	for (const TCHAR* ptr = filename; *ptr != _T('\0'); ++ptr) {
		// 금지된 문자 포함 여부 확인
		if (_tcschr((PS)invalidChars, *ptr)) {
			return false;
		}
		// MBCS: isalnum을 통한 ASCII 문자 검사
		// UNICODE: _istalnum으로 확장 문자 포함 검사
		if (!_istalnum(*ptr) && !_istpunct(*ptr)) {
			return false;
		}
	}
	return true;
}



int UcRunBatchFileAndWait(LPCTSTR batchFilePath, std::function<void()> onSuccess)
{
	DWKFUNC;
#ifdef _DEBUGx
	int waitResult = WAIT_TIMEOUT;
	DWKTRACE(L"%s", DWK__EMAP(waitResult, WAIT_TIMEOUT, WAIT_FAILED, WAIT_ABANDONED, WAIT_IO_COMPLETION));
	return 0;
#endif // _DEBUG

	SHELLEXECUTEINFO shExecInfo = { 0 };
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS; // 프로세스 핸들을 가져오기 위해 설정
	shExecInfo.hwnd = NULL;
	shExecInfo.lpVerb = _T("open");
	shExecInfo.lpFile = batchFilePath; // 실행할 배치 파일 경로
	shExecInfo.lpParameters = NULL;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOW;
	shExecInfo.hInstApp = NULL;

	// 배치 파일 실행
	if (ShellExecuteEx(&shExecInfo)) {
		// 실행된 프로세스가 종료될 때까지 대기
		DWORD waitResult = WaitForSingleObject(shExecInfo.hProcess, 7000);// INFINITE);
		DWORD exitCode = 0;
		DWKTRACE(L"%s", DWK__EMAP(waitResult, WAIT_OBJECT_0, WAIT_TIMEOUT, WAIT_FAILED, WAIT_ABANDONED, WAIT_IO_COMPLETION));
		if (waitResult == WAIT_OBJECT_0) {
			if (GetExitCodeProcess(shExecInfo.hProcess, &exitCode) && exitCode == 0) {
				// 정상 종료 시 람다 실행
				if (onSuccess)
					onSuccess();
				CloseHandle(shExecInfo.hProcess);
				return 0; // 정상 종료
			}
			else {
				// 프로세스가 비정상 종료
				if (onSuccess)
					onSuccess();
				CloseHandle(shExecInfo.hProcess);
				return exitCode; // 종료 코드 반환
			}
		}
		else {
			// WaitForSingleObject 실패
			CloseHandle(shExecInfo.hProcess);
			return GetLastError(); // 에러 코드 반환
		}
	}
	else {
		DWORD errorCode = GetLastError();
		TRACE(_T("Failed to execute batch file. Error code: %lu\n"), errorCode);
		return errorCode; // ShellExecuteEx 실패 시 에러 코드 반환
	}
}


#ifdef _Sample__
int Sample_RunBatchFileAndWait() {
	// 실행할 배치 파일 경로
	TCHAR batchFilePath[] = _T("C:\\path\\to\\your\\script.bat");

	int result = UcRunBatchFileAndWait(batchFilePath, []() {
		// 정상 종료 시 실행할 람다 함수
		TRACE(_T("Batch file executed successfully!\n"));
		TCHAR resultFilePath[] = _T("C:\\path\\to\\output.txt");
		if (GetFileAttributes(resultFilePath) != INVALID_FILE_ATTRIBUTES) {
			TRACE(_T("File created successfully: %s\n"), resultFilePath);
		}
		else {
			TRACE(_T("File not found: %s\n"), resultFilePath);
		}
		});

	if (result != 0) {
		TRACE(_T("Batch file failed with error code: %d\n"), result);
	}

	return result;
}
#endif

#pragma warning(disable:4996)

#ifdef _DEBUGx
+ wcstok_s returned 0xd047d0d9f8 L"사과1"           wchar_t*
+context            0xd047d0da00 L"사과2 사과3"     wchar_t*
+delimiters         0xd0485ffd24 L" ,"              const    wchar_t[3]
lpParam             0xd047d0d9f8 void*
+str                0xd047d0d9f8 L"사과1"           wchar_t*
+token              0xd047d0d9f8 L"사과1"           wchar_t*

+wcstok_s returned  0xd047d0da00 L"사과2"           wchar_t*
+context            0xd047d0da08 L"사과3"           wchar_t*
+delimiters         0xd0485ffd24 L" ,"              const    wchar_t[3]
lpParam             0xd047d0d9f8 void*
+str                0xd047d0d9f8 L"사과1"           wchar_t*
+token              0xd047d0da00 L"사과2"           wchar_t*

+wcstok_s returned  0xd047d0da08 L"사과3"           wchar_t*
+context            0xd047d0da0e L""                wchar_t*
+delimiters         0xd0485ffd24 L" ,"              const    wchar_t[3]
lpParam             0xd047d0d9f8 void*
+str                0xd047d0d9f8 L"사과1"           wchar_t*
+token              0xd047d0da08 L"사과3"           wchar_t*
#endif // _DEBUGx
vector<CString> UcTokenizeString(LPCTSTR sStr, const TCHAR delimiters[])
{
	vector<CString> vts;
	auto n = UcTokenizeString<TCHAR>(sStr, delimiters, [&vts](int i, LPTSTR tok) {
		vts.push_back(tok);
		});
	return std::move(vts);
}


int Sample_TokenizeString() {
	// 나눌 문자열 2개
	TCHAR str1[] = _T("빵1, 빵2, 빵3");
	TCHAR str2[] = _T("사과1 사과2 사과3");

	const TCHAR delimiters[] = _T(", ");
	vector<CString> vts1 = UcTokenizeString(str1, delimiters);
	vector<CString> vts;
	auto n = UcTokenizeString<TCHAR>(str1, delimiters, [&vts](int i, LPTSTR tok) {
		vts.push_back(tok);
		});

	return 0;
}

void UcTest1()
{
	Sample_TokenizeString();
}
void UcTest2()
{
}
void UcTest3()
{
}
void UcTest4()
{
}


#include <cstddef>
//dwk: 2025-02-24 17:50  
UCTOOLDYNAMIC
bool UcIsUTF8String(const char* str, size_t length)//dwk: 2025-02-24 17:50  
{
	if (length >= 3 &&
		static_cast<unsigned char>(str[0]) == 0xEF &&
		static_cast<unsigned char>(str[1]) == 0xBB &&
		static_cast<unsigned char>(str[2]) == 0xBF) {
		return true; // ✅ BOM이 있으면 바로 UTF-8로 판별
	}

	int nBytes = 0;
	unsigned char chr = 0;

	for (size_t i = 0; i < length; i++)
	{
		chr = static_cast<unsigned char>(str[i]);

		if (nBytes == 0)
		{ // 첫 바이트 검사
			if ((chr & 0x80) == 0)
				continue; // ASCII 문자         (0xxx xxxx)
			else if ((chr & 0xE0) == 0xC0)
				nBytes = 1; // 2바이트 문자 시작 (110x xxxx)
			else if ((chr & 0xF0) == 0xE0)
				nBytes = 2; // 3바이트 문자 시작 (1110 xxxx)
			else if ((chr & 0xF8) == 0xF0)
				nBytes = 3; // 4바이트 문자 시작 (1111 0xxx)
			else
				return false; // ❌ 잘못된 UTF-8 😎💀
		}
		else
		{ // 후속 바이트 검사
			if ((chr & 0xC0) != 0x80)
				return false; // ❌ 잘못된 후속 바이트
			nBytes--;
		}
	}
	return nBytes == 0; // 모든 문자들이 정상적으로 해석되었는지 확인
}
#ifdef _Sample__
#include <iostream>
int main() {
	const char* utf8WithBOM = "\xEF\xBB\xBFHello, UTF-8!"; // UTF-8 with BOM
	const char* utf8WithoutBOM = "你好, 世界!"; // UTF-8 without BOM
	const char* notUtf8 = "\xC3\x28"; // 잘못된 UTF-8

	std::cout << "UTF-8 with BOM: " << (IsUTF8String(utf8WithBOM, strlen(utf8WithBOM)) ? "Yes" : "No") << std::endl;
	std::cout << "UTF-8 without BOM: " << (IsUTF8String(utf8WithoutBOM, strlen(utf8WithoutBOM)) ? "Yes" : "No") << std::endl;
	std::cout << "Invalid UTF-8: " << (IsUTF8String(notUtf8, strlen(notUtf8)) ? "Yes" : "No") << std::endl;

	return 0;
}
#endif // _Sample__


//#include <afx.h> // MFC 기본 헤더
//#include <list>
//#include <memory>

CStringA UcReadSmallTextFile(PWS fileName)//dwk: 2025-02-24 13:34  
{
	CStringA str;
	//	ASSERT(sizeof(TCHAR) == 1);
	CFile stream;
	if (stream.Open((LPCTSTR)fileName, CFile::modeRead) == 0) //|CFile::typeText) == 0)
	{
		DWORD dwErr = GetLastError(); // 32:ERROR_SHARING_VIOLATION
		return {};
	}

	DWORD fileLength = (DWORD)stream.GetLength();
	LPSTR fileBuf = NULL;
	try
	{
		bool bUnicode = false;
		if (fileLength == 0)
			throw "fileLength == 0";
		const WORD s_wfeff = 0xfeff;
		if (fileLength > 2)
		{
			WORD wfeffr;
			if (stream.Read(&wfeffr, (UINT)sizeof(WORD)) != (UINT)sizeof(WORD))
				throw "CFile.Read error! 2";
			bUnicode = wfeffr == s_wfeff;
		}

		if (bUnicode)
		{
			stream.Close();
//			ASSERT(0);//KwIsUnicodeTextFile 를 먼저 불러 봐야 한다.
			return {};
		}

		UINT len = 0;
		UINT lenChar = 0;

		{
			int hd = bUnicode ? 2 : 0;
			len = lenChar = fileLength - hd; // 0xfeff 를 뺀 것에서 2(WORD)로 나누면
			stream.Seek(CFile::begin, hd); // 0xfeff 없으니 다시 앞으로
		}
		fileBuf = (LPSTR)str.GetBuffer(len + 1);
		if (stream.Read(fileBuf, (UINT)len) != (UINT)len)
			throw "CFile.Read error! 3";

		fileBuf[lenChar] = '\0';
		str.ReleaseBuffer();//lenChar
	}
	catch (CException* e)
	{
		e;
		TRACE("%u\n", GetLastError());
		str.Empty();
	}
	catch (LPCSTR se)
	{
		TRACE("%s\n", se);
		str.Empty();
	}
	catch (...)
	{
		TRACE("catch (...)\n");
		str.Empty();
	}

	stream.Close();
	return str;
}

CStringW UcReadSmallTextFileW(PWS fileName)
{
	CFile stream;
	if (stream.Open((LPCTSTR)fileName, CFile::modeRead) == 0)
		return {};

	DWORD fileLength = (DWORD)stream.GetLength();
	if (fileLength == 0) {
		stream.Close();
		return {};
	}

	std::vector<BYTE> bytes(fileLength);
	if (stream.Read(bytes.data(), fileLength) != fileLength) {
		stream.Close();
		return {};
	}
	stream.Close();

	if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
		int wcharLen = (int)((bytes.size() - 2) / sizeof(wchar_t));
		const wchar_t* pW = reinterpret_cast<const wchar_t*>(bytes.data() + 2);
		return CStringW(pW, wcharLen);
	}

	int cp = CP_ACP;
	size_t off = 0;
	if (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
		cp = CP_UTF8;
		off = 3;
	}
	else if (UcIsUTF8String((const char*)bytes.data(), bytes.size())) {
		cp = CP_UTF8;
	}

	const char* pA = (const char*)bytes.data() + off;
	int cb = (int)(bytes.size() - off);
	if (cb <= 0)
		return {};

	int nW = MultiByteToWideChar(cp, 0, pA, cb, NULL, 0);
	if (nW <= 0)
		return {};

	CStringW sw;
	LPWSTR buf = sw.GetBuffer(nW);
	MultiByteToWideChar(cp, 0, pA, cb, buf, nW);
	sw.ReleaseBuffer(nW);
	return sw;
}

UCTOOLDYNAMIC
CStringW UcReadTextFileAnyEncoding(PWS fileName)
{
	CStringA sA = UcReadSmallTextFile(fileName);
	if (!sA.IsEmpty())
		return CStringW(sA);
	return UcReadSmallTextFileW(fileName);
}

UCTOOLDYNAMIC
wstring UcHinstanceErrStr(HINSTANCE hInst)//dwk: 2025-02-24 15:49 KwLib64A에서 가져옴 
{
	static std::map<int, wstring> mapInst = {
		{0,	L"메모리 부족 or 실행 파일 손상"},
		{2,	L"파일 없음(ERROR_FILE_NOT_FOUND)"},
		{3,	L"경로 없음(ERROR_PATH_NOT_FOUND)"},
		{5,	L"액세스 거부(ERROR_ACCESS_DENIED)"},
		{8,	L"메모리 부족(ERROR_NOT_ENOUGH_MEMORY)"},
		{11,	L"잘못된 실행 파일(ERROR_BAD_FORMAT)"},
		{26,	L"공유 버퍼 손상"},
		{27,	L"파일 연계 문제"},
		{28,	L"DDE 타임아웃"},
		{29,	L"DDE 응답 없음"},
		{30,	L"DDE 실행 실패"},
		{31,	L"파일 연결 없음(SE_ERR_NOASSOC)"},
	};
	auto ii = (INT64)hInst;
	auto it = mapInst.find((int)ii);
	if (it != mapInst.end())
		return it->second;
	return {};
}
//#include <atlconv.h>
//void UcAppendTextToFile(const CStringW filePath, const CStringW textToAppend) 
//{
//	std::wofstream file(filePath.GetString(), std::ios::app); // append 모드로 열기
//	if (!file) {
//		TRACE(L"파일을 열 수 없습니다: %s\n", filePath);
//		return;
//	}
//	UcWcharToUTF8(textToAppend);
//	CStringA utf8Text = UcWcharToUTF8(textToAppend);//CW2A(textToAppend, CP_UTF8); // UTF-8 변환
//	utf8Text += "\r\n"; // Windows CRLF 줄바꿈 추가
//	file.write((LPCVOID)utf8Text.GetString(), utf8Text.GetLength()); // UTF-8 문자열 저장
//
//	file << textToAppend << L"\r\n"; // 한 줄 추가
//	file.close();
//}
UCTOOLDYNAMIC
void UcAppendTextToFile_UTF16_BOM(const CString & filePath, const CStringW & textToAppendW)
{
	try
	{
		CFile file;
		bool isNewFile = !UcIfFileExistEx(filePath); // 파일 존재 여부 확인

		if (!file.Open(filePath, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::typeBinary))
		{
			TRACE(L"파일을 열 수 없습니다: %s\n", filePath.GetString());
			return;
		}

		if (isNewFile)
		{
			WORD bom = 0xFEFF;// UTF-16 BOM 추가 (Little Endian)
			file.Write(&bom, sizeof(WORD));
		}

		file.SeekToEnd(); // 파일 끝으로 이동
		file.Write(textToAppendW.GetString(), textToAppendW.GetLength() * sizeof(wchar_t));

		file.Close();
	}
	catch (CException* e)
	{
		e->ReportError();
		e->Delete();
	}
}

UCTOOLDYNAMIC
void UcAppendTextToFile_UTF8_BOM(const CString & filePath, const CStringW & textToAppendW)
{
	try
	{
		CFile file;
		bool isNewFile = !UcIfFileExistEx(filePath); // 파일 존재 여부 확인

		if (!file.Open(filePath, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::typeBinary))
		{
			TRACE(L"파일을 열 수 없습니다: %s\n", filePath.GetString());
			return;
		}

		if (isNewFile)
		{
			BYTE bom[] = { 0xEF, 0xBB, 0xBF };// UTF-8 BOM 추가 (EF BB BF)
			file.Write(bom, sizeof(bom));
		}

		// UTF-8 변환
		CStringA utf8Text = UcWcharToUTF8(textToAppendW);//.GetString(), CP_UTF8);
		utf8Text += "\r\n";

		file.SeekToEnd();
		file.Write(utf8Text, utf8Text.GetLength());

		file.Close();
	}
	catch (CException* e)
	{
		e->ReportError();
		e->Delete();
	}
}

UCTOOLDYNAMIC
bool UcIsCppSourceFile(LPCTSTR fullPath)
{
	if (fullPath == nullptr)
		return false;
	CString ext = fullPath;
	if (ext.IsEmpty())
		return false;

	ext.MakeLower(); // 대소문자 구분 없이 처리
	int pos = ext.ReverseFind('.');
	if (pos < 0)
		return false;

	ext = ext.Mid(pos); // 확장자만 추출 (.포함)

	static std::set<CString> cExt = {
		_T(".cpp"),
		_T(".cc") ,
		_T(".h")	 ,
		_T(".hh") ,
		_T(".c")	 ,
		_T(".cxx"),
		_T(".hpp"),
		_T(".hxx"),
		_T(".inl"),
		_T(".rc"),
	};
	auto it = cExt.find(ext.GetString());
	return it != cExt.end();
}

/// <summary>
/// log 파일이 무한정 커지는거 방지 하기 위해 자른다.
/// </summary>
/// <param name="sFile"></param>
/// <param name="MAX_LOG_SIZE"></param>
/// <returns></returns>
int UcCutFileToHalf(CString sFile, ULONGLONG MAX_LOG_SIZE)
{
	//constexpr ULONGLONG MAX_LOG_SIZE = 500ULL * 1024 * 1024; // 500MB
	constexpr DWORD BUFFER_SIZE = 1024 * 1024; // 1MB

	CFileStatus status;
	if (CFile::GetStatus(sFile, status))
	{
		if (status.m_size > MAX_LOG_SIZE)
		{
			try {
				CString sTempFile = sFile + _T(".tmp");

				CFile sourceFile;
				if (sourceFile.Open(sFile, CFile::modeRead | CFile::typeBinary)) {

					ULONGLONG halfPos = status.m_size / 2;
					sourceFile.Seek(halfPos, CFile::begin);

					CFile tempFile;
					if (tempFile.Open(sTempFile, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) {

						std::vector<BYTE> buffer(BUFFER_SIZE);
						UINT bytesRead = 0;
						while ((bytesRead = sourceFile.Read(buffer.data(), (UINT)buffer.size())) > 0) {
							tempFile.Write(buffer.data(), bytesRead);
						}
						tempFile.Close();
					}
					sourceFile.Close();
				}

				// 원본 삭제 & 임시 파일 -> 원래 이름으로 변경
				CFile::Remove(sFile);
				CFile::Rename(sTempFile, sFile);
			}
			catch (...) {
				TRACE(L"로그 자르기(버퍼 방식) 실패: %s", sFile);
			}
		}
	}

	return 0;
}


std::tuple<int, int, int, int> UcParseMilliseconds(double milliseconds)
{
	if (milliseconds < 0) milliseconds = 0;

	int totalMs = static_cast<int>(std::floor(milliseconds));
	int h = totalMs / (60 * 60 * 1000);
	totalMs %= (60 * 60 * 1000);

	int m = totalMs / (60 * 1000);
	totalMs %= (60 * 1000);

	int s = totalMs / 1000;
	int n = totalMs % 1000;

	return { h, m, s, n };
}

/// pos : 파일 끝에서 시작, 두번쨰 부터는 \n 위치로 들어 와서 pos-- 하면서 시작 하니 \n의 바로 앞부터 거꾸로 읽는다.
///
/// 	std::ifstream file(filePath, std::ios::binary);
///	if (!file.is_open())
///	return { -1, -1 };
///	
///	file.seekg(0, std::ios::end);
///	LONGLONG pos = (LONGLONG)file.tellg();
/// // pos 에 -1을 넣어 주면 파일 맨 끝으로 간다.
std::tuple<bool, std::string> UcGetLineReverse(std::fstream & file, LONGLONG & pos, bool bEmptyLineAlso)
{
	if (pos == 0)
		return make_tuple(false, "");
	//std::string outLine;// .clear();{
	if (pos == -1) {// 파일 끝으로 이동
		file.seekg(0, std::ios::end);
		pos = (LONGLONG)file.tellg();
	}

	std::string temp;
	while (pos > 0)
	{
		pos--; // 1 -> 0
		file.seekg(pos); //파일 맨앞 일수도 pos == 0 그러면 탈출
		//file.seekg(static_cast<std::streamoff>(pos));
		char ch = file.get();
		if (ch != '\n')
			temp.push_back(ch);
		else
		{
			// 줄 끝 발견 → 완성
			if (!temp.empty()) {
				if (temp.back() == '\r')
					temp.pop_back();
				std::reverse(temp.begin(), temp.end());
			}
			//outLine = temp;
			if (bEmptyLineAlso || !temp.empty())
				return make_tuple(true, temp);/// 공백 줄도 리턴. 받은 곳에서  
		}
	}
	// 파일 맨 처음 줄 처리
	if (!temp.empty()) {
		std::reverse(temp.begin(), temp.end());
		//outLine = temp;
		return make_tuple(true, temp);
	}
	return make_tuple(false, "");
}

CStringW UcCleanFuncName(const char* func_name) {
	std::string result;
	for (const char* p = func_name; *p; ++p) {
		if (std::isalnum(*p) || *p == '_') {
			result += *p;
		}
	}
	return CStringW(result.c_str());
}

//CString UcGetAbsolutePath(LPCTSTR szRelative)
//{
//	TCHAR buffer[MAX_PATH] = { 0 };
//	DWORD dwRet = GetFullPathName(szRelative, MAX_PATH, buffer, NULL);
//	return (dwRet > 0) ? CString(buffer) : CString();
//}

CString UcGetAbsolutePath(LPCTSTR szRelative)
{
	CString strFull;
	LPTSTR pszBuffer = strFull.GetBuffer(MAX_PATH);
	DWORD dwRet = GetFullPathName(szRelative, MAX_PATH, pszBuffer, nullptr);
	strFull.ReleaseBuffer();
	return strFull; // 👍 dwRet == 0 이면 strFull == ""
}
#ifdef _build_errror 
#include "UcWndInvokable.h"
//1>C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\atlmfc\include\atlpath.h(60,11): error C4995: 'PathAddExtensionA': 이름이 #pragma deprecated로 표시되었습니다.
/// 그래서 TcWndInvokable.cpp로 옮김
UCTOOLDYNAMIC
void UcPostMessageBoxError(LPCWSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CStringW sMsg = UcFormatStringFromArgs(fmt, args);
	va_end(args);
	PostMainTaskSelf(UcGetMainCWnd(), [sMsg](auto) {
		UcMessageBoxError(sMsg);
		});
}

#endif // _build_errror
