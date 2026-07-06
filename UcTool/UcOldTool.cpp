#include "pch.h"

#include "UcOldTool.h"



UCTOOLDYNAMIC CPushWorker::ThrdLockMap* CPushWorker::s_thrdLock = nullptr;



void DeleteMe(void* p)

{

	if (p)

		delete p;

}



int KwMessageBox(LPCTSTR lpFormat, ...)

{

	CString buf;

	va_list args;

	va_start(args, lpFormat);

	buf.FormatV(lpFormat, args);

	va_end(args);

	return AfxMessageBox(buf, MB_OK | MB_ICONEXCLAMATION);

}



LPCSTR KwReadSmallTextFileA(LPCTSTR fileName, CStringA& str)

{

	str = UcReadSmallTextFile(fileName);

	return str.IsEmpty() ? nullptr : (LPCSTR)str;

}



static HWND KwFindWindowRecurse(HWND hParent, LPCTSTR className, LPCTSTR lpWinText)

{

	LPCTSTR classNameUse = className;

	if (className && tchlen(className) == 0)

		classNameUse = nullptr;



	CString sWinText;

	if (lpWinText)

		sWinText = lpWinText;



	HWND hw = ::GetWindow(::GetWindow(hParent, GW_CHILD), GW_HWNDFIRST);

	for (; hw; hw = ::GetWindow(hw, GW_HWNDNEXT))

	{

		TCHAR buf[512];

		TCHAR buf1[512];

		::GetClassName(hw, buf, 512);

		::GetWindowText(hw, buf1, 512);

		CString sWinTextScan = buf1;

		if (classNameUse == nullptr)

		{

			if (sWinTextScan.Find(sWinText) >= 0)

				return hw;

		}

		else if (_tcsicmp(buf, classNameUse) == 0)

		{

			if (sWinText.IsEmpty() || sWinTextScan.Find(sWinText) >= 0)

				return hw;

		}

		HWND hw2 = KwFindWindowRecurse(hw, classNameUse, lpWinText);

		if (hw2)

			return hw2;

	}

	return nullptr;

}



HWND KwFindWindow(HWND hParent, LPCTSTR lpClassName, LPCTSTR lpWinText)

{

	return KwFindWindowRecurse(hParent, lpClassName, lpWinText);

}



HWND KwFindWindow(HWND hParent, CString sWndClass, LPCTSTR lpWinText)

{

	LPCTSTR lpClassName = sWndClass.GetLength() ? (LPCTSTR)sWndClass : nullptr;

	return KwFindWindow(hParent, lpClassName, lpWinText);

}



HWND KwFindWindow(HWND hParent, CString className, int count)

{

	int th = 1;

	HWND hw = ::GetWindow(::GetWindow(hParent, GW_CHILD), GW_HWNDFIRST);

	for (; hw; hw = ::GetWindow(hw, GW_HWNDNEXT))

	{

		TCHAR buf[127];

		::GetClassName(hw, buf, 126);

		if (_tcsicmp(buf, className) == 0)

		{

			if (th == count)

				return hw;

			th++;

		}

	}

	return nullptr;

}


