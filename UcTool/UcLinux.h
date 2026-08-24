// Platform abstraction for Windows / Linux.
// Prefer std (string, chrono, filesystem, mutex, thread) in call sites.
// Use this header only where OS APIs truly differ (sockets, errno, …).
//
// NgsServer Linux migration order (SshTool):
//   1) SOCKET / WinSock API  → SocketHandle + helpers below
//   2) CDocument / MFC UI    → remove from gateway core
//   3) CStringW / LPCWSTR    → std::wstring / const wchar_t*
//   4) DWORD / CTime / TRACE → uint32_t / chrono / portable log
//   5) WinCrypt / _io.h 등   → OpenSSL / std::filesystem

#pragma once

#include <chrono>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>

// ---------------------------------------------------------------------------
// Platform detect
// ---------------------------------------------------------------------------
#ifdef _WIN32
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
	// WinSock2 before Windows.h (winsock.h conflict)
#	include <WinSock2.h>
#	include <WS2tcpip.h>
#	include <Windows.h>
#	pragma comment(lib, "ws2_32.lib")
#	define NS_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#	define NS_PLATFORM_LINUX 1
#	include <unistd.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/stat.h>
#	include <arpa/inet.h>
#	include <cstddef>
#	include <cstdint>
#	include <cstring>
#	include <cassert>
#	include <cstdarg>
#	include <cstdio>
#	include <cwchar>

	// Win32 / OLE transitional typedefs (Phase 4: replace call sites)
	using BOOL = int;
	using BYTE = unsigned char;
	using WORD = unsigned short;
	using DWORD = uint32_t;
	using UINT = unsigned int;
	using ULONG = unsigned long;
	using LONG = long;
	using LONGLONG = int64_t;
	using ULONGLONG = uint64_t;
	using INT_PTR = intptr_t;
	using UINT_PTR = uintptr_t;
	using LONG_PTR = intptr_t;
	using ULONG_PTR = uintptr_t;
	using HRESULT = long;
	using WCHAR = wchar_t;
	using CHAR = char;
	using TCHAR = wchar_t;
	using LPSTR = char*;
	using LPCSTR = const char*;
	using LPWSTR = wchar_t*;
	using LPCWSTR = const wchar_t*;
	using LPTSTR = wchar_t*;
	using LPCTSTR = const wchar_t*;
	using HANDLE = void*;
	using HWND = void*;
	using HMODULE = void*;

#	ifndef TRUE
#		define TRUE 1
#		define FALSE 0
#	endif
#	ifndef MAX_PATH
#		define MAX_PATH 260
#	endif
#	ifndef ASSERT
#		define ASSERT(x) assert(x)
#	endif
#	ifndef TRACE
#		define TRACE(...) ((void)0)
#	endif
#	ifndef _CRT_STRINGIZE
#		define _CRT_STRINGIZE_(x) #x
#		define _CRT_STRINGIZE(x) _CRT_STRINGIZE_(x)
#	endif
#	ifndef __FUNCTIONW__
#		define __FUNCTIONW__ L"(func)"
#	endif
#	ifndef _UNICODE
#		define _UNICODE 1
#	endif
#	ifndef UNICODE
#		define UNICODE 1
#	endif
#	ifndef sprintf_s
	template<std::size_t N>
	inline int uc_sprintf_s(char (&buf)[N], const char* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		const int n = std::vsnprintf(buf, N, fmt, ap);
		va_end(ap);
		return n;
	}
	inline int uc_sprintf_s(char* buf, std::size_t size, const char* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		const int n = std::vsnprintf(buf, size, fmt, ap);
		va_end(ap);
		return n;
	}
#		define sprintf_s uc_sprintf_s
#	endif
#	ifndef strcpy_s
#		define strcpy_s(dst, size, src) do { std::strncpy((dst), (src), (size)); (dst)[(size) ? (size) - 1 : 0] = '\0'; } while (0)
#	endif
#	ifndef wcscpy_s
#		define wcscpy_s(dst, size, src) do { std::wcsncpy((dst), (src), (size)); (dst)[(size) ? (size) - 1 : 0] = L'\0'; } while (0)
#	endif
#	ifndef _countof
#		define _countof(a) (sizeof(a) / sizeof((a)[0]))
#	endif

	// OLE VARIANT type tags used by VType (values match Windows)
#	ifndef VT_EMPTY
#		define VT_EMPTY            0
#		define VT_NULL             1
#		define VT_I2               2
#		define VT_R4               4
#		define VT_R8               5
#		define VT_DATE             7
#		define VT_BOOL             11
#		define VT_I8               20
#		define VT_UI8              21
#		define VT_INT              22
#		define VT_UINT             23
#		define VT_VOID             24
#		define VT_HRESULT          25
#		define VT_PTR              26
#		define VT_LPSTR            30
#		define VT_LPWSTR           31
#		define VT_FILETIME         64
#		define VT_BLOB             65
#		define VT_STREAM           66
#		define VT_STORAGE          67
#		define VT_STREAMED_OBJECT  68
#		define VT_STORED_OBJECT    69
#		define VT_BLOB_OBJECT      70
#		define VT_ARRAY            0x2000
#	endif
#else
#	error Unsupported platform
#endif

// ---------------------------------------------------------------------------
// Sleep (std)
// ---------------------------------------------------------------------------
inline void SleepMilliseconds(unsigned int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// ---------------------------------------------------------------------------
// Sockets
// ---------------------------------------------------------------------------
#if defined(_WIN32)
using SocketHandle = SOCKET;
constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
#	ifndef INVALID_SOCKET
#		define INVALID_SOCKET INVALID_SOCKET_HANDLE
#	endif
#endif

inline bool InitializeSocketLibrary()
{
#if defined(_WIN32)
	WSADATA wsaData{};
	return ::WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
	return true;
#endif
}

inline void UninitializeSocketLibrary()
{
#if defined(_WIN32)
	::WSACleanup();
#endif
}

inline void UcCloseSocket(SocketHandle socket)
{
	if (socket == INVALID_SOCKET_HANDLE)
		return;
#if defined(_WIN32)
	::closesocket(socket);
#else
	::close(socket);
#endif
}

inline int GetLastSocketError()
{
#if defined(_WIN32)
	return ::WSAGetLastError();
#else
	return errno;
#endif
}

// ---------------------------------------------------------------------------
// Directory (prefer std::filesystem in new code)
// ---------------------------------------------------------------------------
inline bool CreateDirectoryPortable(const std::filesystem::path& path)
{
	std::error_code error;
	if (std::filesystem::create_directories(path, error))
		return true;
	return std::filesystem::exists(path) && !error;
}

inline bool CreateDirectoryPlatform(const std::string& path)
{
	return CreateDirectoryPortable(std::filesystem::path(path));
}

// ---------------------------------------------------------------------------
// Mutex — std::mutex wrapper (API kept for existing call sites)
// ---------------------------------------------------------------------------
class CPlatformMutex
{
public:
	void Lock() { _mutex.lock(); }
	void Unlock() { _mutex.unlock(); }

private:
	std::mutex _mutex;
};

// ---------------------------------------------------------------------------
// Linux-only transitional CString shim (Phase 3: replace call sites with std)
// On Windows, MFC/ATL CString* remain the real types.
// ---------------------------------------------------------------------------
#if !defined(_WIN32)

#include <cctype>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <sstream>
#include <type_traits>

#ifndef _T
#	define _T(x) L##x
#endif

template<typename _CharType>
class CStringT
{
public:
	using value_type = _CharType;
	using StringType = std::basic_string<_CharType>;
	using size_type = typename StringType::size_type;

	CStringT() = default;
	CStringT(const _CharType* str) : m_str(str ? str : StringType()) {}
	CStringT(const StringType& str) : m_str(str) {}

	// Cross-encoding constructors (CStringW <-> CStringA / char* / wchar_t*)
	template<typename OtherChar,
		typename = std::enable_if_t<!std::is_same_v<OtherChar, _CharType>>>
	CStringT(const OtherChar* str)
	{
		AssignConverted(str);
	}

	template<typename OtherChar,
		typename = std::enable_if_t<!std::is_same_v<OtherChar, _CharType>>>
	CStringT(const CStringT<OtherChar>& other)
	{
		AssignConverted(other.GetString());
	}

	CStringT(const CStringT& other) = default;
	CStringT(CStringT&& other) noexcept = default;
	CStringT& operator=(const CStringT& other) = default;
	CStringT& operator=(CStringT&& other) noexcept = default;

	CStringT& operator=(const _CharType* str)
	{
		m_str = str ? str : StringType();
		return *this;
	}

	const _CharType* GetString() const { return m_str.c_str(); }
	operator const _CharType*() const { return m_str.c_str(); }

	bool IsEmpty() const { return m_str.empty(); }

	template<typename... Args>
	void Format(const _CharType* fmt, Args... args)
	{
		_CharType buf[1024]{};
		if constexpr (std::is_same_v<_CharType, char>)
			std::snprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, args...);
		else
			std::swprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, args...);
		m_str = buf;
	}

	CStringT& operator+=(const CStringT& rhs)
	{
		m_str += rhs.m_str;
		return *this;
	}

	bool operator==(const CStringT& rhs) const { return m_str == rhs.m_str; }
	bool operator!=(const CStringT& rhs) const { return m_str != rhs.m_str; }

	size_type GetLength() const { return m_str.length(); }

	CStringT Mid(size_type pos, size_type len = StringType::npos) const
	{
		return CStringT(m_str.substr(pos, len));
	}

	void Delete(size_type pos, size_type len = 1) { m_str.erase(pos, len); }
	void Insert(size_type pos, const _CharType* s) { m_str.insert(pos, s); }

	size_type Find(_CharType ch, size_type start = 0) const
	{
		return m_str.find(ch, start);
	}

	void FormatInt(int val)
	{
		std::basic_ostringstream<_CharType> oss;
		oss << val;
		m_str = oss.str();
	}

	int ToInt() const { return std::stoi(m_str); }
	void Empty() { m_str.clear(); }
	void Swap(CStringT& other) { m_str.swap(other.m_str); }

	void MakeUpper()
	{
		for (auto& c : m_str) {
			if constexpr (std::is_same_v<_CharType, char>)
				c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			else
				c = static_cast<_CharType>(std::towupper(c));
		}
	}
	void MakeLower()
	{
		for (auto& c : m_str) {
			if constexpr (std::is_same_v<_CharType, char>)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			else
				c = static_cast<_CharType>(std::towlower(c));
		}
	}

	friend CStringT operator+(const CStringT& lhs, const CStringT& rhs)
	{
		return CStringT(lhs.m_str + rhs.m_str);
	}

private:
	template<typename OtherChar>
	void AssignConverted(const OtherChar* str)
	{
		m_str.clear();
		if (!str)
			return;
		if constexpr (std::is_same_v<_CharType, wchar_t> && std::is_same_v<OtherChar, char>)
		{
			for (const char* p = str; *p; ++p)
				m_str.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
		}
		else if constexpr (std::is_same_v<_CharType, char> && std::is_same_v<OtherChar, wchar_t>)
		{
			for (const wchar_t* p = str; *p; ++p)
				m_str.push_back(static_cast<char>(*p < 128 ? *p : '?'));
		}
	}

	StringType m_str;
};

using CStringA = CStringT<char>;
using CStringW = CStringT<wchar_t>;
using CString = CStringT<wchar_t>;

#endif // !defined(_WIN32)
