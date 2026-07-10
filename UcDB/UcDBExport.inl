#pragma once

/// DLL / 정적 겸용 내보내기
/// - UcTool 을 DLL 로 빌드할 때(DbgDll·RelDll 등): 전처리기에 UCTOOL_BUILD_DLL 정의 → dllexport
/// - UcTool.dll 을 링크하는 다른 프로젝트: UCTOOL_DLL 정의 → dllimport
/// - 정적 라이브러리만 쓸 때: 둘 다 없음 → 일반 링크
/// 작업: 링크오류 날 때 마다 하나 씩 붙여 간다.
#ifndef UCDBDYNAMIC_EXPORT

#if defined(UCDB_EXPORTS)
#pragma message("UCDB_EXPORTS defined: UCDBDYNAMIC = __declspec(dllexport)")
#define UCDBDYNAMIC __declspec(dllexport)
#elif defined(UCDB_IMPORTS)
#pragma message("UCDB_IMPORTS defined: UCDBDYNAMIC = __declspec(dllimport)")
#define UCDBDYNAMIC __declspec(dllimport)
#else
#pragma message("UCDB_EXPORTS/UCDB_IMPORTS not defined: UCDBDYNAMIC = empty")
#define UCDBDYNAMIC
#endif

#endif // !UCDBDYNAMIC_EXPORT
