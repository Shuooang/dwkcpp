#pragma once

/// DLL / 정적 겸용 내보내기
/// - UcTool 을 DLL 로 빌드할 때(DbgDll·RelDll 등): 전처리기에 UCTOOL_BUILD_DLL 정의 → dllexport
/// - UcTool.dll 을 링크하는 다른 프로젝트: UCTOOL_DLL 정의 → dllimport
/// - 정적 라이브러리만 쓸 때: 둘 다 없음 → 일반 링크
/// 작업: 링크오류 날 때 마다 하나 씩 붙여 간다.
#ifndef UCTOOLDYNAMIC

#if defined(UCTOOL_EXPORTS)
#define UCTOOLDYNAMIC __declspec(dllexport)
#elif defined(UCTOOL_IMPORTS)
#define UCTOOLDYNAMIC __declspec(dllimport)
#else
#define UCTOOLDYNAMIC
#endif

#endif // !UCTOOLDYNAMIC
