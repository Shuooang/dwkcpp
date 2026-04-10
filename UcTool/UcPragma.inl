#pragma once
//#include "UcPragma.inl"
#ifndef FILINDWK //이름 바꾸자.//dwk: 2025-07-15 11:33 
#define FILINDWK(msg) __FILE__ "(" _CRT_STRINGIZE(__LINE__) "): dwk - " msg
#define FILINWARN(msg) __FILE__ "(" _CRT_STRINGIZE(__LINE__) "): warning - " msg
#define DWKBLD(msg) __pragma(message(FILINDWK(msg)))
#define DWKWARN(msg) __pragma(message(FILINWARN(msg)))
//exold: 매크로없이 쓰려면: #pragma message(__FILE__ "(" _CRT_STRINGIZE(__LINE__) "): dwk - " "여기부터 메시지 이러쿵저러쿵.")
/// 아래는 디버그용 마지막 빌드 마커로 샘플로 남겨 둠.(__pragma는 비표준 MSVC전용)
#define DWK_LASTWORK   static_assert(false, "dwk: LAST WORKING here")
#define DWKREMINDER(msg) __pragma(message(FILINDWK("REMINDER: " msg)))

//[deprecated]
#define DWK_LAST_WORKING                                  \
    __pragma(message(__FILE__ "(" _CRT_STRINGIZE(__LINE__)   \
        "): dwk: error LAST WORKING in [" __FUNCTION__ "]")) \
    static_assert(false, "") //VS2015에서는 ""필수

#endif //FILINDWK


#ifdef __Sample__
//deprecated #pragma message(FILINDWK("메시지 다블클릭하면 코드로 이동"))
DWKBLD("Sample message: 컴파일 시 이 메시지가 출력됩니다.")
DWK_LAST_WORKING;
#endif
