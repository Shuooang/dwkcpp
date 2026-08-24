#pragma once

#ifdef _WIN32

// #define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#define _AFX_ALL_WARNINGS

#include <afxwin.h>
#include <afxext.h>
#include <afxcview.h>

#include <afxdisp.h>



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Internet Explorer 4 
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // Windows  MFC
#endif // _AFX_NO_AFXCMN_SUPPORT

//#include <afxcontrolbars.h>//#gdiplus_error //dwk: 2026-01-15 10:35 c++17에서 std::byte와 충돌. cpp에만 넣어야. 리본 및 컨트롤 막대

#endif // _WIN32

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <sstream>
#include <initializer_list>
//#include <any> c++14에서 지원 안함
#include <list>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <atomic>
//24 -> 20초로 빨라짐.

#include "UcLinux.h"

#ifdef _WIN32
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
#endif // _WIN32
