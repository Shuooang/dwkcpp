// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"

/// mfcs140ud.lib(dllmodul.obj) : error LNK2005: DllMain이(가) dllmain.obj에 이미 정의되어 있습니다.
//dwk: 2026-04-01 11:17 MFC를 쓰면 내부적으로 DllMain 을 만드나 보다. 그러니 
#if 0
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif //0
