#include "UcWindow.h"
#include "UcTool.h"
#include "pch.h"
#include "UcWindow.h"
#include "UcBaseTools.h"
#include "UcTool.h"

#include <initializer_list>
#include <vector>
#include <tuple>
#include <string>







//dwStyle = LVS_SHOWSELALWAYS, DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
UCTOOLDYNAMIC void UcSetListReportStyle(CListCtrl* pList, DWORD dwStyle, DWORD dwExStyle)
{
	if (pList == nullptr)
		return;
	long lStyleOld = GetWindowLong(pList->GetSafeHwnd(), GWL_STYLE);//, bEx ? GWL_EXSTYLE : 
	lStyleOld |= dwStyle;
	SetWindowLong(pList->GetSafeHwnd(), GWL_STYLE, lStyleOld);

	DWORD dw1 = pList->GetExtendedStyle();// | LVS_EX_FULLROWSELECT;
	pList->SetExtendedStyle(dw1 | dwExStyle);
	// 	KwSetWindowStyle(pList->GetSafeHwnd(), LVS_SHOWSELALWAYS);
	// 	KwSetWindowStyle(m_cList1.GetSafeHwnd(), LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES, TRUE, true);
	// 	DWORD dw1 = m_cList1.GetExtendedStyle() | LVS_EX_FULLROWSELECT;
	// 	m_cList1.SetExtendedStyle(dw1);
}

/// sample
//UcSetListReportStyle(&m_list, LVS_SHOWSELALWAYS);
//UcSetListColumn(&m_list, {
//	{  60, 0, LVCFMT_LEFT , L"#"   },
//	{  60, 0, LVCFMT_RIGHT, L"idx" },
//	{ 200, 0, LVCFMT_LEFT , L"time"},
//	{ 800, 0, LVCFMT_LEFT , L"file"}, });



//                                                                     width  data   align  title
UCTOOLDYNAMIC void UcSetListColumn(CListCtrl* pList, std::initializer_list<std::tuple<int, LPARAM, int, CString>> tuples)
{
	int nCol = pList->GetHeaderCtrl()->GetItemCount();
	for (int i = 0; i < nCol; i++)
		pList->DeleteColumn(0);

	CHeaderCtrl* phd1 = pList->GetHeaderCtrl();
	//for(int i=0;i<tuples.size();i++)
	int i = 0;
	for (const auto& tuple : tuples)
	{
#if CPP17_OR_LATER
		auto& [width, data, LVCFMT, text] = tuple;
#else
		auto& width = std::get<0>(tuple);
		auto& data = std::get<1>(tuple);
		auto& LVCFMT = std::get<2>(tuple);
		auto& text = std::get<3>(tuple);
#endif
		pList->InsertColumn(i, text, LVCFMT);//LVCFMT_LEFT);
		pList->SetColumnWidth(i, width);

		HDITEM oHeaderItem;
		UcZeroMemory(oHeaderItem);
		oHeaderItem.mask = HDI_LPARAM;
		oHeaderItem.lParam = data;
		VERIFY(phd1->SetItem(i, &oHeaderItem));
		i++;
	}
}


int UcSelectListItemEx(CListCtrl* pl, int curSel, bool bShow)
{
	if (curSel < 0)
	{
		UcClearSelectedListItem(pl);
		return curSel;
	}
	//curSel = 0;
	int last = pl->GetItemCount() - 1;
	if (last < curSel)
		curSel = last;
	if (curSel >= 0)
		UcSelectListItem(pl, curSel, bShow);
	return curSel;
}

void UcSelectListItem(CListCtrl* pl, int iItem, bool bShow, int iOp)
{
	if (pl->GetItemCount() > iItem)
	{
		if (UcAttr(iOp, eLc_Clear))
			UcClearSelectedListItem(pl);
		if (UcAttr(iOp, eLc_Unselect))
		{
			if (bShow)
				pl->SetItemState(iItem, 0, LVIS_SELECTED | LVIS_FOCUSED);//LVIF_STATE);
			else
				pl->SetItemState(iItem, 0, LVIS_SELECTED);//LVIF_STATE);
		}
		else
		{
			if (bShow)
				pl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);//LVIF_STATE);
			else
				pl->SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);//LVIF_STATE);
		}
		pl->EnsureVisible(iItem, FALSE);
	}
}
void UcFocusListItem(CListCtrl* pl, int iItem)
{
	if (pl->GetItemCount() > iItem)
	{
		pl->SetItemState(iItem, LVIS_FOCUSED, LVIS_FOCUSED);//LVIF_STATE);
		pl->EnsureVisible(iItem, FALSE);
	}
}

int UcClearSelectedListItem(CListCtrl* pl)
{
	UINT i, uSelectedCount = pl->GetSelectedCount();
	int  nItem = -1;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			pl->SetItemState(nItem, 0, LVIS_SELECTED | LVIS_FOCUSED);//LVIF_STATE);
			nItem = pl->GetNextItem(nItem, LVNI_SELECTED | LVNI_FOCUSED);

			if (nItem >= 0)
				return nItem;
		}
	}
	return -1;
}

int UcGetSelectedListItem(CListCtrl* pl)
{
	UINT i, uSelectedCount = pl->GetSelectedCount();
	int  nItem = -1;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			nItem = pl->GetNextItem(nItem, LVNI_SELECTED | LVNI_FOCUSED);

			if (nItem >= 0)
				return nItem;
		}
	}
	return -1;
}
int UcGetSelectedListItem(CListCtrl* pl, CDWordArray& ar)
{
	UINT i;//, uSelectedCount = pl->GetSelectedCount();
	int  nItem = -1;

	ar.RemoveAll();
	//if(uSelectedCount > 0)
	{
		for (i = 0;; i++)//i < uSelectedCount
		{
			nItem = pl->GetNextItem(nItem, LVNI_SELECTED);
			if (nItem >= 0)
				ar.Add(nItem);
			else
				break;
		}
	}
	return (int)ar.GetCount();
}

int UcGetSelectedCount(CListCtrl* pl)
{
	int  nItem = 0;
	int  i = -1;
	for (;;)
	{
		i = pl->GetNextItem(i, LVNI_SELECTED);
		if (i >= 0)
			nItem++;
		else
			break;
	}
	return nItem;
}



void UcEnableWindow(CWnd* pw, CWnd* ctrl, BOOL bEnable)
{
	//if (pw->GetSafeHwnd() != NULL)
	//	if (::IsWindow(pw->GetSafeHwnd()))
	HWND hpw = NULL;
	if (pw && (hpw = pw->GetSafeHwnd()) && ::IsWindow(hpw)) // 이함수를 직접 부를 수 있으므로 pw 중복 검사
	{
		HWND hCtrl = NULL;
		if (ctrl && (hCtrl = ctrl->GetSafeHwnd()) && ::IsWindow(hCtrl))
			ctrl->EnableWindow(bEnable);
	}
}
void UcEnableWindow(CWnd* pw, int idc, BOOL bEnable)
{
	HWND hpw = NULL;//pw->GetSafeHwnd();
	if (pw && (hpw = pw->GetSafeHwnd()) && ::IsWindow(hpw))
	{
		//hthis에 할당 하므로 괄호를 반드시 해야 한다. 
		// pw 뒤에 ','로 하면 pw가 널인 경우도 뒤에 문을 실행 하므로 에러 난다.
		CWnd* ctrl = pw->GetDlgItem(idc);
		UcEnableWindow(pw, ctrl, bEnable);
	}
}
void UcEnableWindow(CWnd* pw, const int* idc, int cnt, BOOL bEnable)
{
	for (int i = 0; i < cnt; i++)
		UcEnableWindow(pw, idc[i], bEnable);
}
void UcEnableWindow(CWnd* pw, const int* idc, BOOL* bEnable, int cnt)
{
	for (int i = 0; i < cnt; i++)
		UcEnableWindow(pw, idc[i], bEnable[i]);
}
void UcEnableWindow(CWnd* pw, std::initializer_list<std::pair<int, BOOL>> arIdcBool)
{
	for (auto& ib : arIdcBool)
		UcEnableWindow(pw, ib.first, ib.second);
}

void UcEnableWindow(CWnd* pw, std::initializer_list<int> arIdc, BOOL bEnable)
{
	for (auto idc : arIdc)
		UcEnableWindow(pw, idc, bEnable);
}

void UcShowWindow(CWnd* pw, CWnd* ctrl, int eShow)
{
	HWND hpw = NULL;
	if (pw && (hpw = pw->GetSafeHwnd()) && ::IsWindow(hpw)) // 이함수를 직접 부를 수 있으므로 pw 중복 검사
	{
		HWND hCtrl = NULL;
		if (ctrl && (hCtrl = ctrl->GetSafeHwnd()) && ::IsWindow(hCtrl))
			ctrl->ShowWindow(eShow);
	}
}
void UcShowWindow(CWnd* pw, int idc, int eShow)
{
	HWND hpw = NULL;//pw->GetSafeHwnd();
	if (pw && (hpw = pw->GetSafeHwnd()) && ::IsWindow(hpw))
	{
		CWnd* ctrl = pw->GetDlgItem(idc);
		UcShowWindow(pw, ctrl, eShow);
	}
}
void UcShowWindow(CWnd* pw, std::initializer_list<int> arIdc, int eShow)
{
	for (auto idc : arIdc)
		UcShowWindow(pw, idc, eShow);
}

PAS UcShowValueToStr(UINT sv)
{
	switch (sv)
	{
		CASE_ASTR(SW_HIDE);
		CASE_ASTR(SW_SHOWNORMAL);
		CASE_ASTR(SW_SHOWMINIMIZED);
		CASE_ASTR(SW_SHOWMAXIMIZED);
		CASE_ASTR(SW_SHOWNOACTIVATE);
		CASE_ASTR(SW_SHOW);
	default:
		ASSERT(0);
		return "SW_UNKNOWN";
	}
}

BOOL UcIsScrollBarAtBottom(CWnd* pWnd, int hFont)//& richEditCtrl)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	pWnd->GetScrollInfo(SB_VERT, &si);
	int nPos = si.nPos;
	int nMin = si.nMin;
	int nMax = si.nMax;
	int nPage = si.nPage;//화면이 덜 찼을때는 0
	auto nPosEnd = si.nPos + si.nPage - 1;
	auto nTrkPosEnd = si.nTrackPos + si.nPage - 1;
	auto nEndChk = si.nMax - nPosEnd;
	auto nLine = (int)((double)nPage / (double)hFont);//font height를 20정도로 보게
	auto bEnd = nPage && (int)nEndChk < (2 * nLine);
	/*
	TRACE(L"cbSize: %u, fMask: %u, nPage: %3u, hFont:%d, nLine:%d, nMin: %d, nMax: %d, nPos:nPosEnd: %d:%d(%d), nTrackPos: %4d:%4d, nEndChk:%4d\n",
	si.cbSize,
	si.fMask,
	si.nPage, hFont, nLine,
	si.nMin,
	si.nMax,
	si.nPos, nPosEnd, bEnd,
	si.nTrackPos, nTrkPosEnd, nEndChk);
	*/
	// 만약 수직 스크롤바의 현재 위치와 페이지 크기의 합이 최대 스크롤 값과 같거나
	// 수직 스크롤바의 최대 값과 페이지 크기의 차이가 현재 위치와 같다면
	// 스크롤바가 맨 아래에 위치해 있다고 판단할 수 있습니다.
	if (bEnd)
		_break;
	else
		_break;

	return bEnd;// (nPos + nPage >= nMax) || (nMax - nPage == nPos);
}

int UcFindStrFromListCtrlItemData(CListCtrl* pList, LPCWSTR sData)
{
	auto sz = pList->GetItemCount();
	for (int i = 0; i < sz; ++i)
	{
		auto pItemData = (LPCWSTR)pList->GetItemData(i);
		TRACE(L"%s == %s\n", sData, pItemData);
		if (tchsame(pItemData, sData))
			return i;
	}
	return -1;
}




SYSTEMTIME UcGetItemDateTime(CDateTimeCtrl* cDate, CDateTimeCtrl* cTime)
{
	SYSTEMTIME st{ 0, };

	SYSTEMTIME systm;
	if (cDate)
	{
		cDate->GetTime(&systm);
		st.wYear = systm.wYear;
		st.wMonth = systm.wMonth;
		st.wDay = systm.wDay;
	}

	if (cTime)
	{
		cTime->GetTime(&systm);
		st.wHour = systm.wHour;
		st.wMinute = systm.wMinute;
		st.wSecond = systm.wSecond;
	}
	return std::move(st);
}

void UcSetItemDateTime(SYSTEMTIME st, CDateTimeCtrl* cDate, CDateTimeCtrl* cTime)
{
	// 날짜 컨트롤에 날짜 설정
	if (cDate && ::IsWindow(cDate->GetSafeHwnd()))
	{
		SYSTEMTIME dateOnly = {};
		dateOnly.wYear = st.wYear;
		dateOnly.wMonth = st.wMonth;
		dateOnly.wDay = st.wDay;

		cDate->SetTime(&dateOnly);
	}

	// 시간 컨트롤에 시간 설정
	if (cTime && ::IsWindow(cTime->GetSafeHwnd()))
	{
		SYSTEMTIME timeOnly = {};
		timeOnly.wHour = st.wHour;
		timeOnly.wMinute = st.wMinute;
		timeOnly.wSecond = st.wSecond;

		cTime->SetTime(&timeOnly);
	}
}
void UcSimulateDoubleClick(HWND hWnd, int x, int y)
{
	// 좌표값을 LPARAM으로 변환
	LPARAM lParam = MAKELPARAM(x, y);

	// 첫 번째 클릭
	PostMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
	PostMessage(hWnd, WM_LBUTTONUP, 0, lParam);

	// 두 번째 클릭 (더블 클릭의 일부로 처리됨)
	PostMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
	PostMessage(hWnd, WM_LBUTTONUP, 0, lParam);

	// 더블 클릭 메시지 전달
	PostMessage(hWnd, WM_LBUTTONDBLCLK, MK_LBUTTON, lParam);
}

/// <summary>
/// CCTV제어 프로그램에서 마우스 더블 클릭을 시뮬레이션하기 위한 함수.
/// 멀티 화면에서 한화면만 크게 보이게 하기 위해서는 더블 클릭 해야 하고, 함수로 지원 안해서 사용.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
void UcSimulateMouseDoubleClick(int x, int y)
{
	INPUT inputs[4] = {};// 마우스 클릭 이벤트 배열

	inputs[0].type = INPUT_MOUSE;
	inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;// 첫 번째 클릭: DOWN
	inputs[0].mi.dx = x;
	inputs[0].mi.dy = y;
	inputs[1].type = INPUT_MOUSE;
	inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;// 첫 번째 클릭: UP
	inputs[1].mi.dx = x;
	inputs[1].mi.dy = y;
	inputs[2].type = INPUT_MOUSE;
	inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;// 두 번째 클릭: DOWN
	inputs[2].mi.dx = x;
	inputs[2].mi.dy = y;
	inputs[3].type = INPUT_MOUSE;
	inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;// 두 번째 클릭: UP
	inputs[3].mi.dx = x;
	inputs[3].mi.dy = y;

	CPoint pt{ 0 };
	GetCursorPos(&pt);

	//SetCursorPos(x, y);// 마우스 커서를 이동
	RECT rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + 1; // 1x1 크기 영역
	rect.bottom = y + 1;

	// 마우스를 특정 위치에 고정
	ClipCursor(&rect);
	SendInput(4, inputs, sizeof(INPUT));// 입력 이벤트 실행
	ClipCursor(nullptr);
	SetCursorPos(pt.x, pt.y);//원래 위치로 복구
}
#include <shellscalingapi.h> // GetDpiForMonitor
#pragma comment(lib, "Shcore.lib")
BOOL CALLBACK MyMonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT lprc, LPARAM lParam)
{
	auto context = reinterpret_cast<MyMonitorEnumContext*>(lParam);
	MONITORINFOEX info = {};
	info.cbSize = sizeof(info);
	UINT dpiX{ 0 }, dpiY{ 0 };
	HRESULT hr = GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
	//배율(%)	DPI 값
	//	100 % 96
	//	125 % 120
	//	150 % 144
	//	175 % 168
	//	200 % 192
	if (GetMonitorInfo(hMon, &info)) {
		context->monitors.push_back(make_tuple(info, dpiX, dpiY));
	}
	return TRUE;
}
/// <summary>
/// QROut1에서 모니터 정보를 수집하기 위한 구조체. 
/// 특정 스크린에 창을 띄우기 위해 사용.
/// </summary>
/// <returns></returns>
std::shared_ptr<MyMonitorEnumContext> UcGetMonitorInfo()
{
	DWKFUNC;
	// 모니터 정보 수집
	auto context = NEWSHP(MyMonitorEnumContext);
	if (!EnumDisplayMonitors(nullptr, nullptr, MyMonitorEnumProc, reinterpret_cast<LPARAM>(context.get()))) {
		DWKTRACE(L"모니터 열거 실패",1);
		return {};
	}
	//EnumDisplayDevices(NULL, i, &dd, 0);
	//EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devmode);
	//DWKTRACE(L"Device: %s, Position: %d, %d\n", dd.DeviceName, devmode.dmPosition.x, devmode.dmPosition.y);
	// 모니터 정보 출력
#if CPP17_OR_LATER
	for (const auto& [mc, dX, dY] : context->monitors) {
		DWKTRACE(L"Device: %s, Position: %d, %d\n", mc.szDevice, mc.rcMonitor.left, mc.rcMonitor.top);
	}
#else
	for (const auto& tuple : context->monitors) {
		const auto& mc = std::get<0>(tuple);
		const auto& dX = std::get<1>(tuple);
		const auto& dY = std::get<2>(tuple);
		DWKTRACE(L"Device: %s, Position: %d, %d\n", mc.szDevice, mc.rcMonitor.left, mc.rcMonitor.top);
	}
#endif
	return context;
}

//void MoveWindowToMonitor(HWND hWnd, int monitorIndex);
/// <param name="monitorIndex">0,1,2..</param>
/// <param name="monitors"></param>
UCTOOLDYNAMIC void UcMoveWindowToMonitor(CWnd* pWnd, const MONITORINFOEX& mi)// std::vector<MONITORINFOEX>& monitors)
{
	//if (!pWnd || monitorIndex < 0 || monitorIndex >= monitors.size())
	//	return;
	//const MONITORINFOEX& mi = monitors[monitorIndex];
	CRect rc = mi.rcMonitor;
	// 윈도우 위치 및 크기 조절
//-		[0]tagMONITORINFO	{cbSize=104 rcMonitor={LT(0, 0) RB(1920, 1200)  [1920 x 1200]} rcWork={LT(0, 0) RB(1920, 1200)  [1920 x 1200]} ...}	tagMONITORINFO
//+		szDevice	L"\\\\.\\DISPLAY1"
//-		[1]tagMONITORINFO	{cbSize=104 rcMonitor={LT(1920, -954) RB(5760, 1206)  [3840 x 2160]} rcWork={LT(2055, -954) RB(5760, 1206)  [3705 x 2160]} ...}	tagMONITORINFO
//+		szDevice	L"\\\\.\\DISPLAY2"
//-		[2]tagMONITORINFO	{cbSize=104 rcMonitor={LT(5760, 0) RB(7680, 1080)  [1920 x 1080]} rcWork={LT(5760, 0) RB(7680, 1080)  [1920 x 1080]} ...}	tagMONITORINFO
//+		szDevice	L"\\\\.\\DISPLAY3"
	CRect rc2(CPoint(rc.left, rc.top + 75), CSize(rc.Width(), rc.Height() - 75));
	pWnd->SetWindowPos(nullptr, rc2.left, rc2.top, rc2.Width(), rc2.Height(), SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	pWnd->MoveWindow(rc2);
}

int UcMonitorDevice()
{
	DWKFUNC;
	DISPLAY_DEVICE dd;
	DEVMODE dm;
	int deviceIndex = 0;

	//wstringstream std_cout;
	STCOUT << "== 모니터 리스트 ==\n";

	while (true) {
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);

		// 모니터가 없으면 종료
		if (!EnumDisplayDevices(NULL, deviceIndex, &dd, 0))
			break;

		// 실제 연결된 모니터만 대상으로
		if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE) {
			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);

			if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
				STCOUT << L"디바이스 이름: " << dd.DeviceName << ENDL;//std::endl;
				STCOUT << L"화면 이름    : " << dd.DeviceString << ENDL;//std::endl;
				STCOUT << L"좌표         : (" << dm.dmPosition.x << L", " << dm.dmPosition.y << L")" << ENDL;//std::endl;
				STCOUT << L"해상도       : " << dm.dmPelsWidth << L"x" << dm.dmPelsHeight << ENDL;//std::endl;
				STCOUT << L"주 모니터    : " << ((dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? L"예" : L"아님") << ENDL;//std::endl;
				STCOUT << L"--------------" << ENDL;//std::endl;
			}
		}
		//디바이스 이름 : \\.\DISPLAY1
		//	화면 이름 : Intel(R) Iris(R) Xe Graphics
		//	좌표 : (0, 0)
		//	해상도 : 1920x1200
		//	주 모니터 : 예
		//	--------------
		//	디바이스 이름 : \\.\DISPLAY2
		//	화면 이름 : Intel(R) Iris(R) Xe Graphics
		//	좌표 : (1920, -960)
		//	해상도 : 3840x2160
		//	주 모니터 : 아님
		//	--------------
		//	디바이스 이름 : \\.\DISPLAY3
		//	화면 이름 : Intel(R) Iris(R) Xe Graphics
		//	좌표 : (0, 1200)
		//	해상도 : 1920x1080
		//	주 모니터 : 아님
		//	--------------

		deviceIndex++;
	}
	//DWKTRACE(L"%v", std_cout.str().c_str());

	return 0;
}


/// background 에서도 메인 윈도우를 가져 올수 있다. AfxGetMainWnd() 사용 하면 안됨
UCTOOLDYNAMIC HWND UcGetMainWnd()
{
	HWND h = nullptr;

	if (AfxGetApp() &&
		AfxGetApp()->m_pMainWnd &&
		::IsWindow(AfxGetApp()->m_pMainWnd->GetSafeHwnd()))
	{
		h = AfxGetApp()->m_pMainWnd->GetSafeHwnd();
	}
	return h;
}

