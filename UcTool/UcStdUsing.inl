#pragma once

/// 일반적으로 헤더헤서 포함 하여 가볍게 쓸 수 있는 using 선언을 모아 둔 파일입니다.
#ifndef UCSTDUSING
#define UCSTDUSING
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <utility>
#include <tuple>
#include <sstream>
#include <initializer_list>
#if CPP17_OR_LATER
#include <any>
#endif

//using namespace std; 대신 일부믄 사용 하기 위해. 이전 코드 호환을 위해.
//namespace Uc {
using std::string;
using std::wstring;
using std::function;
using std::vector;
//using std::map; //이건 너무 짧아서 쓸 때 마다 std::map으로 써야할듯
using std::pair;
using std::tuple;
using std::shared_ptr;
using std::make_shared;
using std::make_unique;
using std::make_tuple;
using std::wstringstream;
using std::stringstream;
using std::initializer_list;
using std::dynamic_pointer_cast;

#if CPP17_OR_LATER
using std::wstring_view;// c++17
using std::string_view;
using std::any_cast;
#endif

template<typename T>
using SHP = std::shared_ptr<T>;


#endif // !UCSTDUSING