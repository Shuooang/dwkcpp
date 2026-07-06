#include "pch.h"

//#include <memory>//static_pointer_cast
//#include <algorithm>  // 사용되지 않음 - algorithm 함수들 미사용
#include <cmath>
//#include <cstdlib>    // 사용되지 않음 - C 표준 라이브러리 함수들 미사용
//#include <cstring>    // 사용되지 않음 - C 문자열 함수들 미사용
//#include <iosfwd>     // 사용되지 않음 - iostream 전방 선언 미사용
//#include <iterator>   // 사용되지 않음 - iterator 관련 함수들 미사용
//#include <utility>    // 사용되지 않음 - utility 함수들 미사용
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <fstream>//std::ofstream
#include <iterator>
#if CPP17_OR_LATER
#include <any>
#endif
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <set>
#include <cctype>
#include <filesystem>
#include <mutex>

#include <basetsd.h>
#include <minwindef.h>
#include <wtypes.h>
#include <malloc.h>
#include <tchar.h>
#include <minwinbase.h>
#include <winnt.h>
#include <ATLComTime.h>
#include <atltime.h>

#include "UcJson.h"
#include "UcTimeTools.h"
#include "UcDebug.h"


#ifdef _Samples__
/// 은 KJson.cpp 상단에
#endif
using namespace std;

DWKREMINDER("매크로 남발 하지 말자. 비슷한 코드가 반복 되더라도 함수 정의는 그대로 해야 찾기 편하다.")


COleDateTime UcJObj::__base(1980, 1, 1, 0, 0, 0);


UCTOOLDYNAMIC std::function<BOOL(const jstring& k)> UcJObj::_fncFieldCheck;
UCTOOLDYNAMIC BOOL UcJObj::s_bCheckEachField{ FALSE };

namespace {
	std::set<std::wstring> s_sqlBackupFieldNames = {
		L"type",//CUcRecset::QueryToTableJson
		L"table",//CUcRecset::QueryToTableJson
		L"fields",//CUcRecset::QueryToTableJson
		L"rows",//CUcRecset::QueryToTableJson
	};

	/// UcLength(content)
	void CollectBacktickFNamesFromSqlUtf8(const std::string& content, std::set<std::wstring>& out)
	{
		size_t i0 = 0;
		auto len = UcLength(content);
		if (len >= 3
			&& (unsigned char)content[0] == 0xEF && (unsigned char)content[1] == 0xBB && (unsigned char)content[2] == 0xBF)
			i0 = 3;
		for (size_t i = i0; i + 2 < len; ++i) {
			if (content[i] != '`')
				continue;
			if (content[i + 1] != 'f')
				continue;
			size_t j = i + 2;
			while (j < len) {
				unsigned char c = (unsigned char)content[j];
				if (std::isalnum(c) || c == '_')
					++j;
				else
					break;
			}
			if (j < len && content[j] == '`' && j > i + 2) {
				std::wstring w;
				w.reserve(j - (i + 1));
				for (size_t k = i + 1; k < j; ++k)
					w += (wchar_t)(unsigned char)content[k];
				out.insert(std::move(w));
				i = j;
			}
		}
	}

	void CollectBacktickFNamesFromSqlWstr(const CStringW& content, std::set<std::wstring>& out)
	{
		size_t i0 = 0;
		auto len = UcLength(content);
		//if (len >= 3
		//	&& (unsigned char)content[0] == 0xEF && (unsigned char)content[1] == 0xBB && (unsigned char)content[2] == 0xBF)
		//	i0 = 3;
		for (size_t i = i0; i + 2 < len; ++i) {
			if (content[(int)i] != '`')
				continue;
			if (content[(int)i + 1] != 'f')
				continue;
			size_t j = i + 2;
			while (j < len) {
				wchar_t c = (wchar_t)content[(int)j];
				if (std::isalnum(c) || c == '_')
					++j;
				else
					break;
			}

			if (j < len && content[(int)j] == '`' && j > i + 2) {
				std::wstring w;
				w.reserve(j - (i + 1));
				for (size_t k = i + 1; k < j; ++k)
					w += (wchar_t)content[(int)k];
				out.insert(std::move(w));
				i = j;
			}
		}
	}

	//void MergeKnownJsonParamFieldNamesInto(std::set<std::wstring>& out, vector<PWS>& preFields)
	//{
	//	for (auto p : preFields)
	//		out.insert(p);
	//}

	bool LooksLikeDbHungarianField(const jstring& k)
	{
		if (k.size() < 2 || k[0] != L'f')
			return false;
		return k[1] >= L'A' && k[1] <= L'Z';
	}
}//namespace

bool UcJObj::LoadSqlBackupFieldNames(vector<PWS>& preFields, LPCWSTR pathToSqlFile)
{
	DWKFUNC;
	s_sqlBackupFieldNames.clear();
	std::error_code ec;
	const std::filesystem::path path(pathToSqlFile);//utf-8
	if (!std::filesystem::exists(path, ec)) {
		DWKTRACE(L"LoadSqlBackupFieldNames: 파일 없음 path=%ls ec=%d", pathToSqlFile, ec.value());
		return false;
	}
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) {
		DWKTRACE(L"LoadSqlBackupFieldNames: 열기 실패 path=%ls", pathToSqlFile);
		return false;
	}
	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	CStringW sqlW = UcUTF8ToWchar(content);
	CollectBacktickFNamesFromSqlWstr(sqlW, s_sqlBackupFieldNames);
	//CollectBacktickFNamesFromSqlUtf8(content, s_sqlBackupFieldNames);
	//MergeKnownJsonParamFieldNamesInto(s_sqlBackupFieldNames, preFields);
	for (auto p : preFields)
		s_sqlBackupFieldNames.insert(p);

	DWKTRACE(L"LoadSqlBackupFieldNames: %v 필드 path=%v", s_sqlBackupFieldNames.size(), pathToSqlFile);
	return !s_sqlBackupFieldNames.empty();
}

BOOL UcJObj::FieldCheckAgainstLoadedSqlBackupFields(const jstring& k)
{
	DWKUSETRACE;
	if (s_sqlBackupFieldNames.empty())
		return TRUE;
	if (!LooksLikeDbHungarianField(k))
		return TRUE;
	if (s_sqlBackupFieldNames.count(k))
		return TRUE;
#ifdef _DEBUG
	auto ds = DWKTRACE(L"dwk: 없는 키 '%v'", k);
	ASSERT("dwk: 없는 키" == 0);//?see: NgsCommon::EnsureUcJObjDebugSqlFieldNamesLoaded
#endif // _DEBUG
	return TRUE;
}

///                 FUNCTION LINE ERROR   JSON_POS   JSON_LINE   JSON_COLUMN
std::list<JException> UcJson::s_errors;
/// <summary>
/// 
/// </summary>
/// <param name="data">null terminated string</param>
/// <returns></returns>
ShJObj UcJson::ParseUtf8(const char* data)
{
	CStringW requ;
	UcUTF8ToWchar(data, requ);
	ShJVal shv = UcJson::Parse((LPCWSTR)requ);
	if (shv) {
		if (auto pVal = shv->Val())
			if (pVal->IsObject())
				return pVal->AsObject();
	}
	return ShJObj();
}
ShJVal UcJson::ParseASCII(const char* data)
{
	CStringW sw(data);
	auto shv = UcJson::Parse(sw, {});
	return shv;
	//auto shv = ShJVal();
	//size_t length = strlen(data) + 1;
	//auto p = malloc(length * sizeof(wchar_t));
	//wchar_t* w_data = reinterpret_cast<wchar_t*>(p);
	//size_t ret_value = 0;
	//if (mbstowcs_s(&ret_value, w_data, length, data, length) != 0)
	//{
	//	free(w_data);
	//	return shv;
	//}
	//shv = UcJson::Parse(w_data, {});
	//free(w_data);
	//return shv;
}

ShJVal UcJson::Parse(const wchar_t* data, function<int(int, int, LPCWSTR)> cb)
{
	auto tr = make_shared<JTrain>(&data, tchlen(data), cb);
	return UcJson::ParseTrain(tr, cb);
}

ShJVal UcJson::ParseTrain(SHP<JTrain> tr, function<int(int, int, LPCWSTR)> cb)
{
	auto shv = ShJVal();
	// Skip any preceding whitespace, end of data = no Json = fail
	if (!SkipWhitespace(tr->_ppData, tr))// tr->_ppData
		return shv;

	// We need the start of a value here now...
	shv = JVal::Parse(tr);
	tr->Check(tr->_ppData);
	if (shv)
	{
		SkipWhitespace(tr->_ppData, tr);
	}
	return shv;
}


wstring UcJson::Stringify(const ShJVal value)
{
	if (value != NULL)
		return value->Val()->Stringify();
	else
		return L"";
}

std::wstring JVal::Stringify(const bool bUnicode, int prettyprint, const wchar_t* key, function<LPCWSTR(PWS, wchar_t)> pinf, function<int(PWS, int)> cbChk)
{
	//size_t const indentDepth = prettyprint;// ? 1 : 0;//key = 0x0000000000000000 <NULL>
	/// prettyprint
	/// 0. 바짝 : error - object 시작시 '{' 중복 : 이건 잡았고
	/// 1. 최소 : No error - 약간 미완성 덜이쁨
	/// 2. 깔끔 : No error - 약간 미완성 덜이쁨
	/// 3. 표준 : error - 마지막 항목에 ',', array 마침 ']' 없슴.
	//ASSERT(prettyprint == 1 || prettyprint == 2);
	return StringifyImpl(bUnicode, prettyprint, key, pinf, 0, cbChk);
}

std::wstring JVal::StringifyImpl(const bool bUnicode, size_t const pretty, const wchar_t* key, function<PWS(PWS, wchar_t)> pinf, int tab, function<int(PWS, int)> cbChk)
{
	std::wstringstream rss;
	////key =  <NULL>
	//size_t const indentDepth1 = pretty ? pretty + 1 : 0;
	LPCWSTR stab = L"  ";
	std::wstringstream indent;// = L"  ";// Indent(pretty);
	for (int i = 0; i < tab; i++)
		indent << stab;
	//std::wstring const indent1 = L"  ";// Indent(indentDepth1);
	switch (_type)
	{
	case eNul:
	{
		rss << L"null";
		break;
	}
	case eStr:
	{
		rss << StringifyString(bUnicode, ___, key, pinf);
		break;
	}
	case eTme:
	{
		auto& stm = AsString();
		if (stm.length() > 0)
			rss << L"\"" << stm << L"\"";
		else
			rss << L"null";
		break;
	}
	case eBol:
	{
		rss << (AsBool() ? L"true" : L"false");
		break;
	}
	case eFlt:
	{
		if (IsNan())
			rss << L"null";
		else
		{
			std::wstringstream ss;
			ss.precision(15);
			ss << AsDouble();
			rss << ss.str();
		}
		break;
	}
	case eI64:
	{
		if (IsNan())
			rss << L"null";
		else
		{
			std::wstringstream ss;
			ss.precision(15);
			ss << AsInt64();
			rss << ss.str();
		}
		break;
	}
	case eInt:
	{
		if (IsNan())
			rss << L"null";
		else
		{
			std::wstringstream ss;
			ss.precision(15);
			ss << AsInt();
			rss << ss.str();
		}
		break;
	}
	case eArr:
	{
		//rss << indentDepth ? L"[\n" + indentStr1 : L"[";
		switch (pretty)
		{
		case 0: case 1:
			rss << L"[";
			break;
		case 2: case 3:
			rss << L"[\n";
			break;
		}
		//ShJArr::const_iterator iter = array_value.begin();
		WCHAR wbuf[20] = { 0, };
		//for(int i = 0; iter != array_value.end(); i++)
		auto pArr = nod_->Arr();
		ASSERT(pArr);
		auto& arr0 = *pArr;
		int szArray = (int)arr0.size();
		int i = 0;
		for (auto& sjv : arr0)
		{
			auto bVal = sjv->IsVal();
			auto bDic = sjv->IsDic();
			auto bArr = sjv->IsArr();
			BOOL bGrp = bDic || bArr;// sjv->Val()->_type == eObj || sjv->Val()->_type == eArr;
			wstring indent1 = indent.str() + stab;
			switch (pretty)
			{
			case 0: case 1:
				break;
			case 2:
				if (bGrp || i == 0)
					rss << indent1;
				break;
			case 3:
				rss << indent1;
				break;
			}
			//_itow_s((int)i, wbuf, 19, 10); //KwItoaW(i, wbuf, 19)
			///?todo wbuf는 특정문자에 대해서 임의로 변경 하려 할때 IStrConvert::CharToString 를 override한 interface를 제공 받아 한다.
			/// 이거는 후에 람다 방식으로 바꿔야 겠다.
#ifdef _DEBUGx
			wstring sObj = sjv->Val()->StringifyImpl(bUnicode, pretty, wbuf, pinf, tab + 1, cbChk);
			if (tstrfind(sObj, L"{\"col\":0, \"length\":1, \"name\":\"ORD_STATE\"") >= 0)
				_break;//앞에 탭 2개
			rss << sObj;
			if (sObj.length() > 120)
				___ = sObj.substr(0, 120); //debug용이라 60자로 제한. //___를 비어 두지 않고 dic이나 array일때 debug용으로 쓴다.
			else
				___ = sObj;
			if (___ == L"\"\"")
				_break;
#else
			if (bVal)
				rss << sjv->Val()->StringifyImpl(bUnicode, pretty, wbuf, pinf, tab + 1, cbChk);
			else if (bDic) {
				//auto dic = sjv->Dic();
				//JVal jv(*dic, false);//false를 주면 ~jv에서 free하지 않는다.
				JVal jv(sjv, false);//dwk: 2025-03-28 15:36  
				rss << jv.StringifyImpl(bUnicode, pretty, wbuf, pinf, tab + 1, cbChk);
				_break;
			}
			else if (bArr) {
				//auto arr = sjv->Arr();
				//JVal jv(*arr, false);
				JVal jv(sjv, false);//dwk: 2025-03-28 15:36  
				rss << jv.StringifyImpl(bUnicode, pretty, wbuf, pinf, tab + 1, cbChk);
				_break;
			}
#endif // _DEBUG
			// Not at the end - add a separator		//if(++iter != array_value.end())
			if (i != (szArray - 1))//마지막꺼가 아니면
			{
				switch (pretty)
				{
				case 0:	rss << L",";
					break;
				case 1:	rss << L", ";
					break;
				case 2:
					if (bGrp)
						rss << L",\n";
					else
						rss << L", ";
					break;
				case 3:
					rss << L",\n";// << indent;// 배열 2번째 부터 인덴트 하나 더 추가되어서 뺌.
					break;
				}
			}//rss << indentDepth ? L", " : L",\n" + indentStr1;
			else //마지막 항목
			{
				_break;
			}
			i++;
		}
		switch (pretty)
		{
		case 0:
			rss << L"]\n";
			break;
		case 1: case 2:
			rss << L"\n" << indent.str() << L"]";
			break;
		case 3:
			rss << L"\n" << indent.str() << L"]";
			break;
		}
		//rss << pretty ? L"\n" + indent + L"]" : L"]\n";
		break;
	}

	case eObj:
	{
		switch (pretty)
		{
		case 0:	rss << L"{";
			break;
		case 1:	case 2:	rss << indent.str() << L"{ ";
			break;
		case 3:	rss << indent.str() << L"{\n" << indent.str() << stab;
			break;
		}
		//			rss << pretty ? L"{\n" + indent1 : L"{";
		if (nod_)
		{
			auto ob = nod_->Dic();
			int szObj = (int)ob->size();
			int i = 0;
			size_t iCr = 0;
			for (auto& it : *ob)
			{
				KAtEnd ipp([&i]() {++i; });
				//auto& key1 = it.first;
				//CStringW keyW(it.first.c_str());
				wstring key = it.first;// ((LPCWSTR)keyW);
				if (key == L"id")
					_break;//앞에 탭 2개
				auto& sjv = it.second;
				auto eType = sjv->Val()->_type;
				BOOL bGrp = eType == eObj || eType == eArr;
				if (bGrp)
					switch (pretty)
					{
					case 0: case 1: case 2: case 3:
						rss << L"\n" << indent.str() << stab;// dwkang 2023-05-25
						break;
					}
				else if ((rss.str().length() - iCr) > 100)
					switch (pretty)
					{
					case 2: // dic안에서 너무 길어 지면 중간에 줄바꿈.
						//rss << L"\n" << indent.str() << stab; //2에서는 줄바꾸지 않기로 함. 2024-06-03 15:38:10
						iCr = rss.str().length();
						break;
					case 0: case 1: case 3:
						break;
					}

				auto bChk = 0;
				if (cbChk)// && key)
				{
					TRACE(L"|%s:%s\n", indent.str().c_str(), key.c_str());
					bChk = cbChk(key.c_str(), tab);
					//if (bChk == -1)//람다함수가 -1을 리턴 하면 이번 키는 뺀다.
					//	continue;// return rss.str();// ret_string;
				}


				if (bChk == 0)
				{
					if (i > 0)//_MoveToBefore__ 첫번째가 아니면
					{
						switch (pretty)
						{
						case 0: case 1: rss << L",";
							break;
						case 2:rss << L", ";
							break;
						case 3: rss << L",\n" << indent.str() << stab;
							break;
						}
					}
					//rss << StringifyString(bUnicode, key, NULL); // 구 버전
					// _bAttr이 true면 키 앞에 @ 추가
					std::wstring outputKey = key;
					if (sjv->Val()->_bAttr) {
						outputKey = L"@" + key;
						// 디버그: _bAttr이 true인 경우 확인
						//TRACE(L"DEBUG: _bAttr=true for key: %s\n", key.c_str());
					}
					rss << StringifyString(bUnicode, outputKey, NULL);
					rss << L":";
#ifdef _DEBUGx
					wstring sObj = sjv->Val()->StringifyImpl(bUnicode, pretty, key.c_str(), pinf, tab + 1, cbChk);// bUseConvt ? pinf : NULL);
					rss << sObj;
					if (sObj.length() > 120)
						___ = sObj.substr(0, 120); //debug용이라 60자로 제한. //___를 비어 두지 않고 dic이나 array일때 debug용으로 쓴다.
					else
						___ = sObj;
					if (___ == L"\"\"")
						_break;
#else
					rss << sjv->Val()->StringifyImpl(bUnicode, pretty, key.c_str(), pinf, tab + 1, cbChk);// bUseConvt ? pinf : NULL);
#endif // _DEBUG
				}
#ifdef _MoveToBefore__
				if (i != (szObj - 1))//마지막꺼가 아니면
				{
					switch (pretty)
					{
					case 0: case 1: rss << L",";
						break;
					case 2:rss << L", ";
						break;
					case 3: rss << L",\n" << indent.str() << stab;
						break;
					}
					//rss << indentDepth ? L", " : L",\n\t";
				}
#endif // _DEBUG
				//i++;
			}
		}

		switch (pretty)
		{
		case 0: case 1:
			rss << L"}";
			break;
		case 2: case 3:
			rss << L"\n" << indent.str() << L"}";
			break;
		}
		//rss << pretty ? L"\n" + indent + L"}" : L"}\n";
		break;
	}//case eObj: 
	}//switch(_type) 
	return rss.str();// ret_string;
}

std::wstring JVal::StringifyString(const bool bUnicode, const std::wstring& str, const wchar_t* key, function<PWS(PWS, wchar_t)> pinf)
{
	std::wstringstream str_out;
	str_out << L"\"";
#ifdef _DEBUGx
	//if(str.find(L"테스트지사") != std::string::npos)
	if (tstrfind(str, (L"테스트지사")) != -1)
		_break;
#endif // _DEBUG
	auto appendUnicodeEscape = [&](wchar_t c) {
		str_out << L"\\u";
		for (int i = 0; i < 4; i++)
		{
			int value = (c >> 12) & 0xf;
			if (value >= 0 && value <= 9)
				str_out << (wchar_t)('0' + value);
			else if (value >= 10 && value <= 15)
				str_out << (wchar_t)('A' + (value - 10));
			c <<= 4;
		}
	};

	auto iter = str.begin();
	while (iter != str.end())
	{
		wchar_t chr = *iter;
		const wchar_t* prv = NULL;

		if (pinf)
		{
			//prv = pinf->CharToString(key, chr);
			prv = pinf(key, chr);
		}
		if (prv)
		{
			str_out << prv;
		}
		else
		{
			if (chr == L'"' ||  // " => \" 로
				chr == L'\\'  // \ => \\ 로
				//	|| chr == L'/' 이건 굳이  / => \/ 로 Json표준(특이사항)할 필요 없다는데
				)
			{
				str_out << L'\\';
				str_out << chr;
			}
			else if (chr == L'\b')
				str_out << L"\\b";
			else if (chr == L'\f')
				str_out << L"\\f";
			else if (chr == L'\n')
				str_out << L"\\n";
			else if (chr == L'\r')
				str_out << L"\\r";
			else if (chr == L'\t')
				str_out << L"\\t";//DEL 문자는 이들 제어 문자와는 별개로 \u007F로 표현되어야
			else if (chr < L' ')
			{
				// ESC(0x1B) 등 나머지 제어 문자 — JSON 스펙상 bUnicode 무관 항상 \uXXXX
				appendUnicodeEscape(chr);
			}
			else if (chr > 126)
			{
				if (bUnicode)
				{
					if (
						(L'\uAC00' <= chr && chr <= L'\uD7A3') ||
						(L'\u1100' <= chr && chr <= L'\u1159') ||
						(L'\u00C0' <= chr && chr <= L'\u0491') ||
						(L'\u1E80' <= chr && chr <= L'\u1E85') ||
						(L'\u1EF2' <= chr && chr <= L'\u1EF3') ||
						(L'\u1E00' <= chr && chr <= L'\u1FFC')
						)
						str_out << chr;
					else
						appendUnicodeEscape(chr);
				}
				else
					str_out << chr;
			}
			else
				str_out << chr;
		}
		++iter;
	}

	str_out << L"\"";
	return str_out.str();
}



/// //////////////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////////////
/// //////////////////////// XML /////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////////////


#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

namespace rpx = rapidxml;

static bool try_parse_bool(const std::wstring& s, bool& out)
{
	if (s == L"true" || s == L"TRUE" || s == L"1") { out = true; return true; }
	if (s == L"false" || s == L"FALSE" || s == L"0") { out = false; return true; }
	return false;
}
static bool try_parse_int64(const std::wstring& s, __int64& out)
{
	if (s.empty()) return false;
	wchar_t* endp = nullptr;
	errno = 0;
	long long v = wcstoll(s.c_str(), &endp, 10);
	if (errno == 0 && endp && *endp == 0) { out = (__int64)v; return true; }
	return false;
}
static bool try_parse_double(const std::wstring& s, double& out)
{
	if (s.empty()) return false;
	wchar_t* endp = nullptr;
	errno = 0;
	double v = wcstod(s.c_str(), &endp);
	if (errno == 0 && endp && *endp == 0) { out = v; return true; }
	return false;
}

/// XML 텍스트/속성: 저장 직전 escape, 로드 직후 unescape. (ParseXmlNode / StringifyXml 양쪽에서 사용)
static std::wstring XmlEscape(const std::wstring& src)
{
	std::wstring out;
	out.reserve(src.size() + 8);
	for (wchar_t ch : src) {
		switch (ch) {
		case L'&':  out += L"&amp;";  break;
		case L'<':  out += L"&lt;";   break;
		case L'>':  out += L"&gt;";   break;
		case L'"':  out += L"&quot;"; break;
		case L'\'': out += L"&apos;"; break;
		default:   out.push_back(ch); break;
		}
	}
	return out;
}
static std::wstring XmlUnescape(const std::wstring& src)
{
	std::wstring out;
	out.reserve(src.size());
	for (size_t i = 0; i < src.size(); ++i) {
		if (src[i] == L'&' && (i + 1) < src.size()) {
			if (src.compare(i, 4, L"&lt;") == 0)   { out += L'<';  i += 3; continue; }
			if (src.compare(i, 4, L"&gt;") == 0)   { out += L'>';  i += 3; continue; }
			if (src.compare(i, 5, L"&amp;") == 0)  { out += L'&';  i += 4; continue; }
			if (src.compare(i, 6, L"&quot;") == 0) { out += L'"';  i += 5; continue; }
			if (src.compare(i, 6, L"&apos;") == 0) { out += L'\''; i += 5; continue; }
		}
		out.push_back(src[i]);
	}
	return out;
}

/// XML 태그 이름 인코딩: rapidxml에 넣기 전 적용. 공백→~, ~→|~| (태그에 공백/~ 불가 대응)
static std::wstring TagEncode(const std::wstring& src)
{
	std::wstring out;
	out.reserve(src.size() * 2);
	const std::wstring tildeEsc = L"|~|";
	for (size_t i = 0; i < src.size(); ++i) {
		if (src[i] == L'~')
			out += tildeEsc;
		else if (src[i] == L' ')
			out += L'~';
		else
			out.push_back(src[i]);
	}
	return out;
}

/// XML 태그 이름 디코딩: ParseXmlNode에서 node->name()/attr->name() 읽은 뒤 적용. |~|→~, ~→공백
static std::wstring TagDecode(const std::wstring& src)
{
	std::wstring out;
	out.reserve(src.size());
	for (size_t i = 0; i < src.size(); ) {
		if (i + 2 < src.size() && src[i] == L'|' && src[i + 1] == L'~' && src[i + 2] == L'|') {
			out += L'~';
			i += 3;
		}
		else if (src[i] == L'~') {
			out += L' ';
			i += 1;
		}
		else {
			out.push_back(src[i]);
			i += 1;
		}
	}
	return out;
}

/// 리턴: 기본 적으로 ShJVal을 리턴 하는데, UcJArr이나 UcJObj도 Val로 담아서 리턴 한다. 그냥 SHP<UcJObj> 를 리턴 하면 안된다.
/// UcJson:: 에 붙이지 않은이유는 rapidxml.hpp를 UcJson.h에 포함 시키지 않기 위해.
// 템플릿 선언
template<typename Ch>
static ShJVal ParseXmlNode(rpx::xml_node<Ch>* node);

// 기존 wchar_t 버전은 템플릿 특수화로 유지
//template<>
//ShJVal ParseXmlNode<wchar_t>(rpx::xml_node<wchar_t>* node)
//{
//	DWKUSETRACE;
//	if (!node)
//		return {};
//
//	// 텍스트 노드 처리
//	if (node->type() == rpx::node_data || node->type() == rpx::node_cdata)
//	{
//		std::wstring text = node->value();//맨끝 값일때 안들어간다.
//		if (!text.empty())
//		{
//			// 숫자나 불린 값인지 확인
//			bool bBool = false;
//			if (try_parse_bool(text, bBool))
//				return make_shared<JVal>(bBool);
//
//			__int64 i64 = 0;
//			if (try_parse_int64(text, i64))
//				return make_shared<JVal>(i64);
//
//			double d = 0;
//			if (try_parse_double(text, d))
//				return make_shared<JVal>(d);
//
//			// 문자열로 처리
//			return make_shared<JVal>(text);
//		}
//	}
//
//	ShJVal shVal;//리턴 할때 JVal형으로 하기 위해
//	ShJVal shObj;//임시 객체로 쓰려고
//	// 요소 노드 처리
//	if (node->type() == rpx::node_element)
//	{
//		//auto shObj = make_shared<UcJObj>();
//		UcJObj* pObj{ nullptr };
//		//auto pObj = shVal->Dic();
//		std::wstring nodeName = node->name();//root
//		DWKTRACE(L"nodeName:%v", nodeName);
//		if (nodeName == L"KDerived._data1")
//			_break;
//		// 속성들을 먼저 처리
//		int nAttr{ 0 };
//		std::wstring typeHint;  // #type 속성 값 저장
//		for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute())
//		{
//			std::wstring attrName = attr->name(); ///맨끝 값일때 안들어간다. //"version" 
//			std::wstring attrValue = attr->value();//"1.0"
//
//			// # 접두사가 있는 속성은 내부 메타데이터로 처리
////			if (attrName.substr(0, 2) == L"__") {
//				if (attrName == TAG_TPY) {
//					typeHint = attrValue;  // 타입 힌트 저장
//					DWKREMINDER("__type__ UcJbj에 저장 되지 않는다. RECT는 __struct__ 저장한다.");
//					continue;
//				}
//			}
//			// 일반 속성은 _bAttr = true로 설정
//			auto attrVal = make_shared<JVal>(attrValue);
//			attrVal->Val()->_bAttr = true;
//			// 디버그: XML 속성 파싱 확인
//			//TRACE(L"DEBUG: XML attribute parsed: %s = %s, _bAttr=true\n", attrName.c_str(), attrValue.c_str());
//
//			if (!shObj) {
//				shObj = make_shared<UcJObj>();
//				pObj = shObj->Dic();
//			}
//			pObj->SetAttr(attrName, attrValue);
//			nAttr++;
//		}
//
//		// 자식 노드들을 한 번의 순회로 처리(요소/텍스트 동시)
//		bool hasElementChildren = false;
//		bool hasTextNodes = false;  // 텍스트 노드가 실제로 존재했는지 추적
//		std::wstring textContent;
//		ShJArr sys_array;  // __type__="array"인 경우 사용
//
//		for (auto* child = node->first_node(); child; child = child->next_sibling())
//		{
//			if (child->type() == rpx::node_element)
//			{
//				hasElementChildren = true;///맨끝 값일때 안들어간다.
//				std::wstring childName = child->name();//role
//				DWKTRACE(L"childName:%v", childName);
//				if (childName == L"CXmlAppDoc.m_mapStatixData")
//					_break;
//				//DWKTRACE(L"\tchildName:%v", childName);
//				/// /////////////////////////////////////////////////////////////////////////
//				auto childVal = ParseXmlNode<wchar_t>(child); /// 리커시브 ////////////////////////////
//				/// /////////////////////////////////////////////////////////////////////////
//				if (childVal)
//				{
//					// __type__="array"인 경우 배열에 추가
//					if (typeHint == L"array") {
//						if (!sys_array)
//							sys_array = make_shared<UcJArr>();
//
//						sys_array->Arr()->Add(childVal, true);
//					}
//					else {
//						// 일반 객체 처리
//						if (!shObj) {
//							shObj = make_shared<UcJObj>();
//							pObj = shObj->Dic();
//						}
//						/// 같은 이름의 자식이 이미 있는지 확인 : 중복 되면 배열로 인식
//						if (pObj->Has(childName))//role
//						{
//							/// 배열로 변환 또는 기존 배열에 추가
//							if (ShJVal valExists = pObj->Get(childName)) {//role
//								auto jval = valExists->Val();//이미 Val 이면 valExist.get() == jval
//								if (jval->IsArray()) // role : [is array]
//								{
//									// 기존 배열에 새 값 추가
//									auto arr = jval->AsArray();
//									if (arr && arr->Arr())
//										arr->Arr()->Add(childVal, false);
//								}
//								else {
//									// 기존 값이 객체 또는 일반 값인 경우 배열로 변환
//									auto arr = make_shared<UcJArr>();
//									// 순서 중요: 기존 값 먼저, 새 값 나중에
//									arr->Add(valExists, false);  // 기존 값 복사
//									arr->Add(childVal, false);    // 새 값 복사
//									/// 기존 키를 배열로 덮어써서 교체 (기존 valExists는 받아 둬서 유지)
//									if (childName == L"CXmlAppDoc.m_mapStatixData")
//										_break;
//									pObj->Set(childName, make_shared<JVal>(arr, false)); // shObj는 키 roles의 Value 역할이다. 저 아래서 shVal 로 싸서 리턴 한다.
//								}
//							}
//						}
//						else
//						{
//							if (childName == L"CXmlAppDoc.m_mapStatixData")
//								_break;
//							pObj->Set(childName, childVal);
//						}
//					}
//				}
//			}
//			else if (child->type() == rpx::node_data || child->type() == rpx::node_cdata)
//			{
//				// 텍스트 노드가 존재함을 표시
//				hasTextNodes = true;
//				//<description>Hello <![CDATA[world]]> and more text</description>
//				// 총 3개의 텍스트 노드를 가집니다. 따라서 +=를 사용해서 모든 텍스트 노드의 내용을 누적해야 합니다:
//				textContent += child->value();//맨끝 값일때 들어간다.
//			}
//		}//for (auto* child = node->first_node(); child; child = child->next_sibling())
//
//
//
//		// __type__="array"인 경우 배열 반환 (아이템 태그명을 보존)
//		if (typeHint == L"array") {
//			//shVal = make_shared<JVal>(sys_array, false);
//			// 아이템 태그명 추출 (첫 번째 요소 기준)
//			std::wstring itemTag;
//			if (auto* first = node->first_node()) {
//				if (first->type() == rpx::node_element)
//					itemTag = first->name();
//			}
//			if (itemTag.empty())
//				itemTag = TAG_ITM; // 배열에 항목 이름이 없으면 디폴트로 <item>
//
//			// { item: [ ... ] } 형태로 감싸기
//			auto shItemObj = make_shared<UcJObj>();
//			auto arr = sys_array ? sys_array : make_shared<UcJArr>();
//			shItemObj->Set(itemTag, make_shared<JVal>(arr, false)); // clone=true 권장
//			shVal = make_shared<JVal>(shItemObj, false);
//		}
//		else if (hasTextNodes && !textContent.empty())
//		{
//			// 텍스트 노드가 실제로 존재하고, 의미있는 텍스트가 있으면 _text로 저장
//				// typeHint에 따라 텍스트를 적절한 타입으로 변환
//			ShJVal valText;
//			if (typeHint == L"int") {
//				__int64 val = _wtoi64(textContent.c_str());
//				valText = make_shared<JVal>(val);
//			}
//			else if (typeHint == L"int64") {
//				__int64 val = _wtoi64(textContent.c_str());
//				valText = make_shared<JVal>(val);
//			}
//			else if (typeHint == L"double") {
//				double val = _wtof(textContent.c_str());
//				valText = make_shared<JVal>(val);
//			}
//			else if (typeHint == L"bool") {
//				bool val = UcJson::AsBool(textContent.c_str());
//				valText = make_shared<JVal>(val);
//			}
//			else {
//				if (textContent == L"11000")
//					_break;
//				valText = make_shared<JVal>(textContent);// 기본값은 문자열
//			}
//
//			if (nAttr > 0) {//속성이 있으면 단순 텍스트가 아니라 이미 객체가 된다.
//				ASSERT(pObj);
//				pObj->Set(L"_text", valText);//이미 속성 몇개가 있으니 하나 더 추가 할 뿐
//				shVal = make_shared<JVal>(shObj, false);//객체는 다시 리턴할 shVal 로 넣어서 리턴 한다. clone 할 필요 없다.
//			}
//			else {//텍스트 인데, 값 온리 이면 그냥 SHP<JVal> 로 바꿔서 리턴
//				///굳이 객체로 갈꺼 없이 JVal만 리턴 하면 끝
//				ASSERT(pObj == nullptr);
//				shVal = valText;//이미 make_shared 해서 만든 거니, 그대로 리턴할 준비
//			}
//		}
//		else if (pObj) {//일반 객체
//			if (shObj->Dic()->S("KDerived._data22") == L"11000")
//				_break;
//			shVal = make_shared<JVal>(shObj, false);//객체는 다시 리턴할 shVal 로 넣어서 리턴 한다. clone 할 필요 없다.
//		}
//		return shVal;// make_shared<JVal>(shObj, false);
//	}
//	return {};
//}

// UTF-8용 ParseXmlNode 템플릿 특수화 (ParseXml 함수들보다 앞에 정의)
template<>
ShJVal ParseXmlNode<char>(rpx::xml_node<char>* node)
{
	DWKUSETRACE;
	if (!node)
		return {};

	// 텍스트 노드 처리
	if (node->type() == rpx::node_data || node->type() == rpx::node_cdata)
	{
		std::string text = node->value();
		if (!text.empty())
		{
			// UTF-8을 wchar_t로 변환 후 XML 엔티티 역변환
			CStringW textW = UcUTF8ToWchar(text);
			std::wstring wtext = XmlUnescape(std::wstring(textW.GetString()));

			// 숫자나 불린 값인지 확인
			bool bBool = false;
			if (try_parse_bool(wtext, bBool))
				return make_shared<JVal>(bBool);

			__int64 i64 = 0;
			if (try_parse_int64(wtext, i64))
				return make_shared<JVal>(i64);

			double d = 0;
			if (try_parse_double(wtext, d))
				return make_shared<JVal>(d);

			// 문자열로 처리
			return make_shared<JVal>(textW.GetString());
		}
	}

	ShJVal shVal;
	ShJVal shObj;
	// 요소 노드 처리
	if (node->type() == rpx::node_element)
	{
		UcJObj* pObj{ nullptr };
		// UTF-8을 wchar_t로 변환 후 태그 이름 디코딩 (|~|→~, ~→공백)
		std::wstring nodeNameDecoded = TagDecode(std::wstring(UcUTF8ToWchar(node->name()).GetString()));
		CStringW nodeNameW = nodeNameDecoded.c_str();
		
		//DWKTRACE(L"nodeName:%v", nodeNameW);
		if (nodeNameW == L"CModDerive.m_lstExpr")
			_break;
		
		// 속성들을 먼저 처리
		int nAttr{ 0 };
		std::wstring typeHint;
		for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attrNameUtf8 = attr->name();
			std::string attrValueUtf8 = attr->value();
			
			// UTF-8을 wchar_t로 변환, 태그 이름 디코딩, XML 엔티티 역변환
			CStringW attrNameW = TagDecode(std::wstring(UcUTF8ToWchar(attrNameUtf8).GetString())).c_str();
			CStringW attrValueW = UcUTF8ToWchar(attrValueUtf8);
			std::wstring attrValueUnescaped = XmlUnescape(std::wstring(attrValueW.GetString()));

			// # 접두사가 있는 속성은 내부 메타데이터로 처리
			if (attrNameW.Left(1) == L"_") {
				if (attrNameW == TAG_TPY) {
					typeHint = attrValueUnescaped;  // 타입 힌트 저장
					//DWKREMINDER("__type__ UcJbj에 저장 되지 않는다. RECT는 __struct__ 저장한다.");
					continue;
				}
			}
			// 일반 속성은 _bAttr = true로 설정
			auto attrVal = make_shared<JVal>(attrNameW);
			attrVal->Val()->_bAttr = true;

			if (!shObj) {
				shObj = make_shared<UcJObj>();
				pObj = shObj->Dic();
			}
			pObj->SetAttr(attrNameW, CStringW(attrValueUnescaped.c_str()));
			nAttr++;
		}

		// 자식 노드들을 한 번의 순회로 처리(요소/텍스트 동시)
		bool hasElementChildren = false;
		bool hasTextNodes = false;
		std::wstring textContent;
		ShJArr sys_array;

		for (auto* child = node->first_node(); child; child = child->next_sibling())
		{
			if (child->type() == rpx::node_element)
			{
				hasElementChildren = true;
				std::wstring childNameDecoded = TagDecode(std::wstring(UcUTF8ToWchar(child->name()).GetString()));
				CStringW childNameW = childNameDecoded.c_str();
				
				DWKTRACE(L"childName:%v", childNameW);
				//if (childNameW == L"CXmlAppDoc.m_mapStatixData")
				//	_break;
				
				auto childVal = ParseXmlNode<char>(child); // UTF-8 버전 호출
				if (childVal)
				{
					// __type__="array"인 경우 배열에 추가
					if (typeHint == L"array") {
						if (!sys_array)
							sys_array = make_shared<UcJArr>();

						sys_array->Arr()->Add(childVal, true);
					}
					else {
						if (!shObj) {// key로 붙이기 위해 몸체 확인
							shObj = make_shared<UcJObj>();
							pObj = shObj->Dic();
						}
						/// 같은 이름의 자식이 이미 있는지 확인 : 중복 되면 배열로 인식
						if (!pObj->Has(childNameW)) {// 새로 추가
							pObj->Set(childNameW, childVal);/// Val(arr) 받아서 여기서 [__array_ : Val(arr)] 단일 객체로 만든다.
						}
						else {
							ASSERT(0);
							/// 배열로 변환 또는 기존 배열에 추가. 여기는 깨진 XML 파일(__type__="array") 누락
							if (ShJVal valExists = pObj->Get(childNameW)) {
								auto jval = valExists->Val();
								if (jval->IsArray()) {
									// 기존 배열에 새 값 추가
									auto arr = jval->AsArray();
									if (arr && arr->Arr())
										arr->Arr()->Add(childVal, false);
								}
								else {
									// 기존 값을 배열로 변환
									auto arr = make_shared<UcJArr>();
									arr->Arr()->Add(valExists, false);
									arr->Arr()->Add(childVal, false);
									pObj->Set(childNameW, make_shared<JVal>(arr, false));
								}
							}
						}
					}
				}
			}
			else if (child->type() == rpx::node_data || child->type() == rpx::node_cdata)
			{
				hasTextNodes = true;
				std::string textUtf8 = child->value();
				CStringW textW = UcUTF8ToWchar(textUtf8);
				std::wstring textUnescaped = XmlUnescape(std::wstring(textW.GetString()));
				// 빈 문자열도 포함시켜야 함 (배열 항목 개수 유지)
				if (!textContent.empty())
					textContent += L" ";
				textContent += textUnescaped;
			}
		}

		// __type__="array"인 경우 배열 반환 (아이템 태그명을 보존)
		if (typeHint == L"array") {
			// 아이템 태그명 추출 (첫 번째 요소 기준), 태그 이름 디코딩
			CStringW itemTagW;
			if (auto* first = node->first_node()) {
				if (first->type() == rpx::node_element) {
					std::wstring itemTagDecoded = TagDecode(std::wstring(UcUTF8ToWchar(first->name()).GetString()));
					itemTagW = itemTagDecoded.c_str();
				}
			}
			if (itemTagW.IsEmpty())/// node->first_node() 라도 있었으면 __array_ 를 구했을 텐데, 빈 배열 이면 
				itemTagW = TAG_ARR; // 배열에 항목 이름이 없으면 디폴트로 <item>

			// { item: [ ... ] } 형태로 감싸기
			//auto shItemObj = make_shared<UcJObj>();
			auto arrChk = sys_array ? sys_array : make_shared<UcJArr>();// 배열 비었어도 만들어 리턴.
			//shItemObj->Set(itemTagW, make_shared<JVal>(arrChk, false));///주의: 객체로 만들어 주지 않는다.
			shVal = make_shared<JVal>(arrChk, false); //make_shared<JVal>(shItemObj, false);/// __r__ : 
			/// empty SHP 넣으면 JVal 에서 오류난다. 빈 배열이라도 넣어 줘야
		}
		else if (hasTextNodes)
		{
			// 텍스트 노드가 실제로 존재하면 _text로 저장 (빈 문자열도 포함)
			// typeHint에 따라 텍스트를 적절한 타입으로 변환
			ShJVal valText;
			if (typeHint == L"int") {
				__int64 val = _wtoi64(textContent.c_str());
				valText = make_shared<JVal>(val);
			}
			else if (typeHint == L"int64") {
				__int64 val = _wtoi64(textContent.c_str());
				valText = make_shared<JVal>(val);
			}
			else if (typeHint == L"double") {
				double val = _wtof(textContent.c_str());
				valText = make_shared<JVal>(val);
			}
			else if (typeHint == L"bool") {
				bool val = UcJson::AsBool(textContent.c_str());
				valText = make_shared<JVal>(val);
			}
			else {
				valText = make_shared<JVal>(textContent);// 기본값은 문자열
			}

			if (nAttr > 0) {///속성이 있으면 단순 텍스트가 아니라 이미 객체가 된다. 한 노드가 더 내려 간다.
				ASSERT(pObj);
				pObj->Set(L"_text", valText);//이미 속성 몇개가 있으니 하나 더 추가 할 뿐
				shVal = make_shared<JVal>(shObj, false);//객체는 다시 리턴할 shVal 로 넣어서 리턴 한다. clone 할 필요 없다.
			}
			else {//텍스트 인데, 값 온리 이면 그냥 SHP<JVal> 로 바꿔서 리턴
				///굳이 객체로 갈꺼 없이 JVal만 리턴 하면 끝
				ASSERT(pObj == nullptr);
				shVal = valText;//이미 make_shared 해서 만든 거니, 그대로 리턴할 준비
			}
		}
		else if (pObj) {//일반 객체
			shVal = make_shared<JVal>(shObj, false);///객체는 다시 리턴할 shVal 로 넣어서 리턴 한다. clone 할 필요 없다.
		}
		else {
			// 자식도 없고 텍스트도 없는 완전 빈 노드인데, 이름이 \"_r_\"이면
			// 배열 항목(빈 문자열)로 인식해야 하므로 빈 문자열 JVal을 리턴한다.
			if (nodeNameW == TAG_ITM) {
				shVal = make_shared<JVal>(std::wstring());
			}
		}
		return shVal;
	}
	return {};
}

//ShJVal UcJson::ParseXml(const wchar_t* data, function<int(int, int, LPCWSTR)> /*cb*/)
//{
//	// rapidxml는 입력 버퍼를 수정하므로 복사본 사용
//	if (data) {
//		/// /////////////////////////////////////////////////////////////////
//		rpx::xml_document<wchar_t> doc;
//		// rapidxml은 버퍼를 수정하므로 수정 가능한 버퍼 필요
//		// wcslen은 null-terminator를 제외한 길이를 반환하므로 +1 필요
//		size_t len = wcslen(data);
//		std::vector<wchar_t> buf(len + 1);
//		wcscpy_s(&buf[0], buf.size(), data);
//		buf[len] = L'\0'; // null-terminator 명시적 설정
//		try {
//#if CPP17_OR_LATER
//			// parse_validate_closing_tags는 태그 이름에 점(.)이 있을 때 문제가 발생할 수 있음
//			// parse_no_utf8: wchar_t는 이미 UTF-16이므로 UTF-8 처리를 비활성화하여 한글 문자 문제 방지
//			// parse_no_string_terminators: '\0'을 넣지 않아서 파싱 중간에 문장이 끝났다고 잘못 판단하는 것을 방지
//			const int optionParsing = rpx::parse_trim_whitespace | rpx::parse_no_utf8 | rpx::parse_no_string_terminators; // | rpx::parse_validate_closing_tags;
//			// rpx::parse_normalize_whitespace | rpx::parse_no_data_nodes;
//			/// /////////////////////////////////////////////////////////////////
//			doc.parse<optionParsing>(&buf[0]);
//#else
//			// C++14에서는 템플릿 매개변수를 직접 명시
//			// parse_validate_closing_tags는 태그 이름에 점(.)이 있을 때 문제가 발생할 수 있음
//			// parse_no_utf8: wchar_t는 이미 UTF-16이므로 UTF-8 처리를 비활성화하여 한글 문자 문제 방지
//			/// /////////////////////////////////////////////////////////////////
//			/// if (!(Flags & parse_no_string_terminators)) 에서 '\0'을 넣어 버리잖아.
//			doc.parse<rpx::parse_trim_whitespace | rpx::parse_no_utf8 | rpx::parse_no_string_terminators>(&buf[0]); // | rpx::parse_validate_closing_tags 제거
//#endif
//			/// /////////////////////////////////////////////////////////////////
//			if (auto* node = doc.first_node()) {
//				if (node->type() == rpx::node_declaration)
//					node = node->next_sibling();
//
//				//	return ParseXmlNode(node); <root> 태그 이름을 보존하기 위해 감싸기
//				// Preserve root element name by wrapping parsed content
//				if (node) {
//					std::wstring rootName = node->name();
//					ShJVal content = ParseXmlNode<wchar_t>(node);
//					if (content && !rootName.empty()) {
//						auto wrapObj = make_shared<UcJObj>();
//
//						// root 태그의 속성들을 먼저 추가
//						for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
//							std::wstring attrName = attr->name();
//							std::wstring attrValue = attr->value();
//
//							// __ 접두사가 없는 일반 속성만 처리
//							if (attrName.substr(0, 2) != L"__") {
//								auto attrVal = make_shared<JVal>(attrValue);
//								attrVal->Val()->_bAttr = true;
//								wrapObj->Dic()->Set(attrName.c_str(), attrVal);
//							}
//						}
//
//						// root 태그의 내용 추가
//						wrapObj->Dic()->Set(rootName.c_str(), content);
//						return make_shared<JVal>(wrapObj, false);
//					}
//					return content;
//				}
//			}
//		}
//		catch (const rpx::parse_error& e) {
//			// rapidxml parse_error에서 오류 정보 추출
//			TRACE("UcXml::Parse: rapidxml parse error: %s\n", e.what());
//			// 오류 위치 계산
//			wchar_t* errorPos = e.where<wchar_t>();
//			wchar_t* bufStart = &buf[0];
//			size_t offset = (errorPos && bufStart) ? (errorPos - bufStart) : 0;
//			TRACE("UcXml::Parse: error at offset: %zu\n", offset);
//			return {};
//		}
//		catch (...)
//		{
//			TRACE("UcXml::Parse: rapidxml parse error.\n");
//			return {};
//		}
//	}
//	//if (!node) return ShJVal();
//	//if (node->type() == rpx::node_declaration)
//	//	node = node->next_sibling();
//	//if (!node) return ShJVal();
//	return {};
//}

// UTF-8 버전 (char*)
ShJVal UcJson::ParseXml(const char* data, function<int(int, int, LPCWSTR)> /*cb*/)
{
	if (!data)
		return {};

	// UTF-8로 직접 파싱
	rpx::xml_document<char> doc;
	size_t len = strlen(data);
	std::vector<char> buf(len + 1);
	strcpy_s(&buf[0], buf.size(), data);
	buf[len] = '\0';

	try {
		const int optionParsing = rpx::parse_trim_whitespace;// | rpx::parse_no_string_terminators;
#if CPP17_OR_LATER
		// UTF-8 파싱: parse_no_utf8 플래그 제거 (UTF-8 처리 활성화)
		doc.parse<optionParsing>(&buf[0]);
#else
		doc.parse<optionParsing>(&buf[0]);
#endif
		if (auto* node = doc.first_node()) {
			if (node->type() == rpx::node_declaration)
				node = node->next_sibling();

			if (node) {
				// UTF-8 노드 이름을 wchar_t로 변환 후 태그 이름 디코딩
				CStringW rootNameW = TagDecode(std::wstring(UcUTF8ToWchar(node->name()).GetString())).c_str();

				ShJVal content = ParseXmlNode<char>(node);
				if (content && !rootNameW.IsEmpty()) {
					auto wrapObj = make_shared<UcJObj>();

					// root 태그의 속성들을 먼저 추가 (태그 이름 디코딩)
					for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
						CStringW attrNameW = TagDecode(std::wstring(UcUTF8ToWchar(attr->name()).GetString())).c_str();
						CStringW attrValueW = UcUTF8ToWchar(attr->value());
						std::wstring attrValueUnescaped = XmlUnescape(std::wstring(attrValueW.GetString()));

						// __ 접두사가 없는 일반 속성만 처리
						if (attrNameW.Left(2) != L"__") {
							auto attrVal = make_shared<JVal>(CStringW(attrValueUnescaped.c_str()));
							attrVal->Val()->_bAttr = true;
							wrapObj->Dic()->Set(attrNameW, attrVal);
						}
					}

					// root 태그의 내용 추가
					wrapObj->Dic()->Set(rootNameW, content);
					return make_shared<JVal>(wrapObj, false);
				}
				return content;
			}
		}
	}
	catch (const rpx::parse_error& e) {
		TRACE("UcXml::Parse (UTF-8): rapidxml parse error: %s\n", e.what());
		char* errorPos = e.where<char>();
		char* bufStart = &buf[0];
		size_t offset = (errorPos && bufStart) ? (errorPos - bufStart) : 0;
		TRACE("UcXml::Parse (UTF-8): error at offset: %zu\n", offset);
		return {};
	}
	catch (...) {
		TRACE("UcXml::Parse (UTF-8): rapidxml parse error.\n");
		return {};
	}

	return {};
}

// UTF-8 버전 (std::string)
ShJVal UcJson::ParseXml(const std::string& data, function<int(int, int, LPCWSTR)> cb)
{
	return ParseXml(data.c_str(), cb);
}




/// //////////////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////////////









void UcJson::PushErr(const JException& ex)
{
	s_errors.push_back(ex);
	if (s_errors.size() > 100)
		s_errors.pop_front();
}


JException UcJson::PopErr()
{
	if (s_errors.size() > 0)
	{
		auto b = s_errors.back();
		s_errors.pop_back();
		return b;
	}
	return JException();
}


void JTrain::Check(const wchar_t** data)
{
	if (_cb)
	{
		_cur = *data - _pStart;
#ifdef _DEBUGx
		CStringW sps(*data, 20);
		sps.Replace(L"\n", L"\\n");
		TRACE(L"%2d:%s\n", (int)((_cur * 100.) / _len), sps);
#endif // _DEBUG
		ASSERT(_cur < 0x7fffffff);
		_cb((int)_cur, (int)_len, *data);
	}

	// JSON 라인 번호는 이제 SkipWhitespace에서 추적하므로 여기서는 제거
	// UpdateLineColumn(*data);
}



void UcJson::SkipWhitespaceThrow(const wchar_t** data, SHP<JTrain> tr)
{
	if (!SkipWhitespace(data, tr))
	{//맨 마지막 '}' 가 빠졌을 때 발생했었다.2024-05-30 11:29:26
#ifdef _DEBUG
		auto tpl = make_tuple((int)__LINE__, string("Whitespace character ends abnormally."), *data);
		throw tpl;
#else
		throw_json("Whitespace character ends abnormally.", *data);
#endif // _DEBUG
	}
}

/// 아직 문장 중간에 있으면 true,  맨끝 '\0'이면 false
bool UcJson::SkipWhitespace(const wchar_t** data, SHP<JTrain> tr)
{
#ifdef _DEBUG
	if (**data == 0xfeff)// 메모장으로 json 수정 하면 이것 때문에 에러 났다. 2024-03-27 15:24:34
		_break;
#endif // _DEBUG
	while (**data != 0 && (**data == L' ' || **data == L'\t' || **data == L'\r' || **data == L'\n' || **data == 0xfeff))
	{
		// JSON 라인 번호 추적: \n 문자를 만날 때마다 라인 번호 증가
		if (tr && **data == L'\n')
		{
			tr->_jsonLine++;
			tr->_jsonColumn = 1;
		}
		else if (tr)
		{
			tr->_jsonColumn++;
		}
		(*data)++;
	}

	return **data != 0;
}


// JSON 라인 번호를 계산하는 헬퍼 함수
static void CalculateJsonLineColumn(const wchar_t* currentPos, const wchar_t* startPos, int& jsonLine, int& jsonColumn)
{
	jsonLine = 1, jsonColumn = 1;
	if (startPos && currentPos >= startPos)
	{
		jsonColumn = 1;
		for (const wchar_t* p = startPos; p < currentPos; p++)
		{
			if (*p == L'\n')
			{
				jsonLine++;
				jsonColumn = 1;
			}
			else
			{
				jsonColumn++;
			}
		}
	}
}

bool UcJson::ExtractString(const wchar_t** data, wstring& str, const wchar_t* startPos)
{
	str = L"";

	while (**data != 0)
	{
		// Save the char so we can change it if need be
		wchar_t next_char = **data;

		// Escaping something?
		if (next_char == L'\\')
		{
			// Move over the escape char
			(*data)++;

			// Deal with the escaped char
			switch (**data)
			{
			case L'"': next_char = L'"'; break;
			case L'\\': next_char = L'\\'; break;
			case L'/': next_char = L'/'; break; /// "\/" 도 이스케이프 시퀀스로 다루는데, '/'는 굳이 안다뤄도 되는데, 혹시 그런 경우도 처리 해준다.
			case L'b': next_char = L'\b'; break;
			case L'f': next_char = L'\f'; break;
			case L'n': next_char = L'\n'; break;
			case L'r': next_char = L'\r'; break;
			case L't': next_char = L'\t'; break;
			case L'u':
			{
				// We need 5 chars (4 hex + the 'u') or its not valid
				try{
				if (!HasMinLengthW(*data, 5))
				{
					// JSON 라인 번호 계산
					int jsonLine, jsonColumn;
					CalculateJsonLineColumn(*data, startPos, jsonLine, jsonColumn);
					throw JException(__FUNCTION__, __LINE__, "/u#### no number.", std::wstring(*data), jsonLine, jsonColumn);
				}
				}
				catch (...){
					TRACE("here\n");
				}

				// Deal with the chars
				next_char = 0;
				for (int i = 0; i < 4; i++)
				{
					// Do it first to move off the 'u' and leave us on the
					// final hex digit as we move on by one later on
					(*data)++;

					next_char <<= 4;

					// Parse the hex digit
					if (**data >= '0' && **data <= '9')
						next_char |= (**data - '0');
					else if (**data >= 'A' && **data <= 'F')
						next_char |= (10 + (**data - 'A'));
					else if (**data >= 'a' && **data <= 'f')
						next_char |= (10 + (**data - 'a'));
					else
					{
						// Invalid hex digit = invalid Json
						int jsonLine, jsonColumn;
						CalculateJsonLineColumn(*data, startPos, jsonLine, jsonColumn);
						throw JException(__FUNCTION__, __LINE__, "Invalid hex digit.", std::wstring(*data), jsonLine, jsonColumn);
					}
				}
				break;
			}

			// By the spec, only the above cases are allowed
			default:
			{
				int jsonLine, jsonColumn;
				CalculateJsonLineColumn(*data, startPos, jsonLine, jsonColumn);
				throw JException(__FUNCTION__, __LINE__, "'\\': only the above cases are allowed.", std::wstring(*data), jsonLine, jsonColumn);
			}
			}
		}

		// End of the string?
		else if (next_char == L'"')
		{
			(*data)++;
			//str.reserve(); //?error in c++20 Remove unused capacity
			return true;
		}

		// Disallowed char? 컨트롤 문자? 아스키 앞부분
		else if (next_char < L' ')
		{
			// SPEC Violation: Allow tabs due to real world cases
			const static wchar_t* arOK = L"\t\r\n\x1e\x1f";
			//                              {RS) (US) 30, 31
			if (!wcschr(arOK, next_char))
			{
				int jsonLine, jsonColumn;
				CalculateJsonLineColumn(*data, startPos, jsonLine, jsonColumn);
				throw JException(__FUNCTION__, __LINE__, "Invalid control character.", std::wstring(*data), jsonLine, jsonColumn);
			}
		}

		// Add the next char
		str += next_char;

		// Move on
		(*data)++;
	}

	// If we're here, the string ended incorrectly 맨뒤에 , 컴마가 붙었을 때
	int jsonLine, jsonColumn;
	CalculateJsonLineColumn(*data, startPos, jsonLine, jsonColumn);
	throw JException(__FUNCTION__, __LINE__, "the string or brace ended incorrectly.", std::wstring(*data), jsonLine, jsonColumn);
}

double UcJson::ParseInt(const wchar_t** data)
{
	double integer = 0;
	while (**data != 0 && **data >= '0' && **data <= '9')
		integer = integer * 10 + (*(*data)++ - '0');

	return integer;
}


double UcJson::ParseDecimal(const wchar_t** data)
{
	double decimal = 0.0;
	double factor = 0.1;
	while (**data != 0 && **data >= '0' && **data <= '9')
	{
		int digit = (*(*data)++ - '0');
		decimal = decimal + digit * factor;
		factor *= 0.1;
	}
	return decimal;
}


void UcJObj::toString()
{
#ifdef _DEBUGxx
	for (auto& it : *this)
		it.second->DumpDebug();
#endif // _DEBUG
#if _DEBUGx
	_txt.Empty();
	CStringW s;
	//for(auto&[k, v] : *this)
	for (auto& it : *this)
	{
		it.second->toString();
		// 			CStringW k = it.first.c_str();
		// 			CStringW v = it.second->FStr();
		// 			s.Format(L"%s: %s, ", k, v);// .c_str(), v->DStr());
		// 			_txt += s;
	}
	_aaa = (LPCWSTR)_txt;
#endif
}

/// ___가 전에는 디버깅 용으로 썼으나, 지금은 실제 데이터 이므로 건드리면 안된다.
[[deprecated]]
void JVal::DumpDebug()
{
#if _DEBUG
	wstring sts;
	JSonTextVal(sts, 50, false);
	___ = sts.c_str();
	if (___ == L"\"\"")
		_break;
#endif
}

[[deprecated]]
void JVal::toString()
{
#if _DEBUG
	//DumpDebug(); //___가 실제 데이터 이므로 건드리면 안된다.
#endif
}



bool UcJObj::Clone(UcJObj* src, bool bClone)
{
	return UcJObj::CloneObject(src, *this, bClone);
}
bool UcJObj::Clone(const UcJObj* src, bool bClone)
{
	return UcJObj::CloneObject((UcJObj*)src, *this, bClone);
}
bool UcJObj::CloneObject(ShJVal src, ShJVal tar, bool bClone)
{
	auto src1 = src->Dic();
	auto& tar1 = *tar->Dic();
	return CloneObject(src1, tar1, bClone);
}

/// JVal::Clone 도 같은 방식으로 포인터로 source를 받는다.
/// source가 null이면 false를 리턴한다.
/// static 함수. template 가 있으니 UcJObj과 중복을 피한다.
bool UcJObj::CloneObject(UcJObj* source, UcJObj& tar, bool bClone)
{
	return UcCloneObjectT<ShJVal, UcJObj, JVal>(source, tar, bClone);
	//ASSERT(source);// bClone과 상관 없이 source가 있어야 한다.
	////ASSERT(!bClone || source);// bClone이면 source가 있어야 한다.
	//if(!source)
	//	return false;
	////for (auto& it : *source) //c++11
	//for (auto& [k, sjo] : *source) //c++17
	//{
	//	//auto& k = it.first;
	//	//auto& sjo = it.second;
	//	ShJVal sjv;
	//	if (auto pval = sjo->Val())
	//		sjv = make_shared<JVal>(*sjo->Val(), bClone);
	//	else
	//		sjv = make_shared<JVal>();
	//	tar.SetAt(k, sjv);
	//}
	//return true;
}

ShJObj UcJArr::FindByValue(JKEYSTR field, LPCWSTR value)
{
	for (auto& sjv : *this)// 리니어 서치
	{
		if (sjv->Val()->IsObject())
		{
			auto sjoRow = sjv->Val()->AsObject();
			auto jo = sjoRow->Val()->Dic();
			if (jo->SameS(field, value))//발견
				return sjoRow;
		}
	}
	return nullptr;
}

void UcJArr::CloneArray(ShJVal src, ShJVal tar, bool bClone)
{
	ASSERT(src->IsArr());
	ASSERT(tar->IsArr());
	auto& src1 = *src->Arr();//*dynamic_cast<UcJArr*>(&src);
	auto& tar1 = *tar->Arr();//*dynamic_cast<UcJArr*>(&tar);
	CloneArray(src1, tar1, bClone);
}

/// static 함수. template 가 있으니 UcXArr과 중복을 피한다.
void UcJArr::CloneArray(UcJArr& src, UcJArr& tar, bool bClone)
{
	UcCloneArrayT<UcJObj, UcJArr, JVal>(src, tar, bClone);
}

void UcJObj::ErrTest()
{
	auto jnull = this->O(L"notexist");
	auto jo = jnull->Dic();
	auto an1y = jo->O("any");
}

namespace {

constexpr int kIsUpdatedNumericDecimalPlaces = 9;

/// JVal 은 wstring(___) 에 저장. AsDouble() 은 stod 로 파싱한다.
/// 소수 decimalPlaces 자리까지 반올림하면 같으면 동일 (tolerance = 0.5 * 10^-places).
bool JValNumericEqual(double a, double b, int decimalPlaces = kIsUpdatedNumericDecimalPlaces)
{
	if (!std::isfinite(a) || !std::isfinite(b))
		return a == b;
	if (decimalPlaces < 0)
		return std::fabs(a - b) < 1e-9;
	const double tol = 0.5 * std::pow(10.0, -decimalPlaces);
	return std::fabs(a - b) < tol;
}

bool JValNumericEqual(JVal* a, JVal* b, int decimalPlaces = kIsUpdatedNumericDecimalPlaces)
{
	if (!a || !b)
		return false;
	return JValNumericEqual(a->AsDouble(), b->AsDouble(), decimalPlaces);
}

} // namespace

/// 길이가 있고 내용이 같은 경우만 true 이다.
int UcJObj::IsUpdated(JBase& src, JBase& tar, JKEYSTR tarF, JKEYSTR srcF)
{
	auto& src1 = *dynamic_cast<UcJObj*>(&src);
	auto& tar1 = *dynamic_cast<UcJObj*>(&tar);
	return IsUpdated(src1, tar1, tarF, srcF);
}
int UcJObj::IsUpdated(UcJObj& src, UcJObj& tar, JKEYSTR tarF, JKEYSTR srcF)
{
	if (!srcF)
		srcF = tarF;
	jstring srf = srcF;// JString(srcF);
	jstring taf(tarF);

	if (src.Has(srf))//this->find(k) != this->end())
	{
		auto sjvS = src[srf];
		if (sjvS->Val()->IsNumber())
		{
			const auto& s = sjvS->Val()->AsString();
			if (tar.Has(srf))
			{
				auto sjvT = tar[taf];
				const auto& t = sjvT->Val()->AsString();
				if (s.empty() && t.empty())
					return -2;
				if (s.empty())
					return -1;
				if (t.empty())
					return 1;
				// wstring 저장값을 AsDouble(stod)로 파싱 후 소수 자릿수 tolerance 비교
				return JValNumericEqual(sjvS->Val(), sjvT->Val()) ? 0 : 1;
			}
			else
				return -1;
		}
		else
		{
			auto s = sjvS->Val()->AsString();
			if (tar.Has(srf))
			{
				auto sjvT = tar[taf];
				auto t = sjvT->Val()->AsString();
				if (s.size() > 0)
				{
					if (t.size() > 0)
						return s == t ? 0 : 1;// 내용 같다. : 변경 되었다.
					else
						return -1; // 지워졌다.
				}
				else
				{
					if (t.size() > 0)
						return 1;//변경 되었다.
					else
						return -2;// zero length로 같다. -1때 처럼 no request, clear output
				}
			}
			else // no tar
			{
				return s.size() > 0 ? -1 : -2;// 있다가 없어짐 : 없었고 계속 없음.
			}
		}
	}
	else // 없다가
	{
		if (tar.Has(srf))
		{
			auto sjvT = tar[taf];
			auto t = sjvT->Val()->AsString();
			return t.size() > 0 ? 1 : -2;//없다가 생김 : 없었고 계속 없음.
		}
		else
			return -2;
	}
}

int UcJObj::IsUpdated(ShJObj& src, ShJObj& tar, JKEYSTR tarF, JKEYSTR srcF)
{
	if (src.get() && tar.get())
	{
		ASSERT(src->IsDic());
		ASSERT(tar->IsDic());
		return IsUpdated(*src, *tar, tarF, srcF);
	}
	else
	{
		if (src.get())
		{
			ASSERT(src->IsDic());
			UcJObj tarj;
			return IsUpdated(*src, tarj, tarF, srcF);
		}
		else
		{
			if (tar.get())
			{
				ASSERT(tar->IsDic());
				UcJObj srcj;
				return IsUpdated(srcj, *tar, tarF, srcF);
			}
			else
				return -2;
		}
	}
}
int UcJObj::IsUpdated(ShJObj& src, UcJObj& tar, JKEYSTR tarF, JKEYSTR srcF)
{
	if (src.get())
	{
		ASSERT(src->IsDic());
		return IsUpdated(*src, tar, tarF, srcF);
	}
	else
	{
		UcJObj srcj;
		return IsUpdated(srcj, tar, tarF, srcF);
	}
}
int UcJObj::IsUpdated(UcJObj& src, ShJObj& tar, JKEYSTR tarF, JKEYSTR srcF)
{
	if (tar.get())
	{
		ASSERT(tar->IsDic());
		return IsUpdated(src, *tar, tarF, srcF);
	}
	else
	{
		UcJObj tarj;
		return IsUpdated(src, tarj, tarF, srcF);
	}
}
int UcJObj::IsUpdated(UcJObj& src, JKEYSTR tarF, JKEYSTR srcF)
{
	return IsUpdated(src, *this, tarF, srcF);
}



/// UcJObj::SP로 대체
//LPCWSTR UcJObj::Ptr(JKEYSTR k)
//{
//	///?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
//	if (Has(k))//this->find(k) != this->end())
//	{
//		auto sjv = (*this)[k];
//		return sjv->Val()->Ptr();// CStringW Buffer 에 담아서 포인터 리턴 한다. Gabage Collection
//	}
//	return NULL;
//}

/// 반드시 IsString 이어야 하고 포인터를 참조하는 동안 이 JSON객체가 살아 있어야 하며 이 데이터가 변경 되면 안된다.
LPCWSTR JVal::SP(LPCWSTR def)
{
	if (IsString())
		return (LPCWSTR)AsString().c_str(); // S() 는 복사 CStringW 이다. LPCWSTR이 유지되는 포인터
	else if (IsNull())
		return def;
	else
		throw_str(L"Not a string Type.[%s]", TypeStr());
	return def;
}

/// 이거는 .h 로 옯겼다. unresolved Link error가 발생하여 옮긴 후 없어졌다. ??? 2024-05-13 10:09:04
//원인: Output folder가 다른 경우 lib 파일을 이전꺼 쓰고있는 경우
//LPCWSTR JVal::Txt(LPCWSTR def)
//{
//	if (IsString())
//		return (LPCWSTR)___.c_str();
//	else// if (IsNull())
//		return def;
//}

LPCWSTR JVal::TypeStr()
{
	static KStdMap<int, LPCWSTR> esmap = {
		ENUM2STRW(eMpt),
		ENUM2STRW(eNul),
		ENUM2STRW(eStr),
		ENUM2STRW(eInt),
		ENUM2STRW(eFlt),
		ENUM2STRW(eTme),
		ENUM2STRW(eI64),
		ENUM2STRW(eI16),
		ENUM2STRW(eUnt),
		ENUM2STRW(eU64),
		ENUM2STRW(eBol),
		ENUM2STRW(ePtr),
		ENUM2STRW(eStA),
		ENUM2STRW(eBin),
		ENUM2STRW(eArr),
		ENUM2STRW(eObj),
	};
	LPCWSTR sType{ L"UnkownType" };
	if (esmap.Lookup((int)_type, sType))
		_break;
	else
		_break;
	return sType;
}

// nullable

CTime JVal::T()
{
	if (IsInt64())
	{
		/// time을 숫자로 넣는다고? 이건 특별한 경우에 내부 수가 time인경우
		auto i64 = AsInt64();
		CTime t(i64);
		return t;
	}
	else
	{
		COleDateTime to = TO();
		if (to.GetStatus() == COleDateTime::valid)
		{
			CTime t(to.GetYear(), to.GetMonth(), to.GetDay(), to.GetHour(), to.GetMinute(), to.GetSecond());
			return t;
		}
	}
	// 		if(IsString())
	// 		{
	// 			CString s = S();
	// 			if(s.GetLength() > 3)
	// 			{
	// 				CTime t = KwParseTimeStr_time64(s);
	// 				return t;
	// 			}
	// 		}
	// 		else if(IsInt64())
	// 		{
	// 			/// time을 숫자로 넣는다고? 이건 특별한 경우에 내부 수가 time인경우
	// 			auto i64 = AsInt64();
	// 			CTime t(i64);
	// 			return t;
	// 		}
	// 		else
	// 		{
	// 			ASSERT(0);
	// 		}
	return CTime();
}

/// flag 는 ParseDateTime 에 쓰인다./// flag: 0 full, 1 VAR_TIMEVALUEONLY, 2 VAR_DATEVALUEONLY
COleDateTime JVal::TO(DWORD flag)
{
	if (IsTime() || IsString())
	{
		const wstring& s = StrRef(); // S()는 IsString()일떄만 응답한다.
		CString st(s.c_str());
		if (s.length() > 3)
		{
			COleDateTime ot;
			ot.ParseDateTime(st, flag);
			// "25 January 1996"
			// "8:30:00"
			// "20:30:00"
			// "January 25, 1996 8:30:00"
			// "8:30:00 Jan. 25, 1996"
			// "1/25/1996 8:30:00" // always specify the full year, even in a 'short date' format
			if (ot.GetStatus() == COleDateTime::valid)
				return ot;
			else
			{
				// 20221110092258000
				// 20221110092258
				COleDateTime ot1 = UcAlldigitToOleTime(st, flag);
				return ot1;
			}
		}
	}
	else if (IsInt64())//입력 당시
	{
		/// time을 숫자로 넣는다고? 이건 특별한 경우에 내부 수가 time인경우
		auto t64 = AsInt64();
		if (t64 == 0)
		{
			COleDateTime ot;
			ot.SetStatus(COleDateTime::null);
			return ot;
		}
		else
		{
			COleDateTime otk(t64);
			return otk;
		}
	}
	else if (IsDouble())//입력 당시 DATE type 을 입력된것으로 간주
	{
		auto d = AsDouble();
		COleDateTime ot((DATE)d);
		if (d == 0)
			ot.SetStatus(COleDateTime::null);
		return ot;
	}
	else
	{
		ASSERT(0);
	}
	auto ot0 = COleDateTime();
	ot0.SetStatus(COleDateTime::null);
	return ot0;
}
SYSTEMTIME JVal::TSys()
{
	auto t = TO();
	SYSTEMTIME tSysTime = { 0 };
	t.GetAsSystemTime(tSysTime);
	//tSysTime.wYear  = t.GetYear();
	//tSysTime.wMonth = t.GetMonth();
	//tSysTime.wDayOfWeek = t.GetDayOfWeek();
	//tSysTime.wDay    = t.GetDay();
	//tSysTime.wHour   = t.GetHour();
	//tSysTime.wMinute = t.GetMinute();
	//tSysTime.wSecond = t.GetSecond();
	//tSysTime.wMilliseconds = 0;
	return tSysTime;
}





void JVal::setTime(CTime t)
{
	_type = eTme;
	CString s = UcCTimeToString(t);
	___ = CStringW(s).GetString();
}
void JVal::setOTime(COleDateTime t)
{
	_type = eTme;
	if (t.GetStatus() == COleDateTime::valid)
	{
		CString s;
		UcOTimeToString(t, s);
		___ = CStringW(s).GetString();
	}
	else
		___.clear();// = L"";
}

///?working 이건 왜 만든거지?
COleDateTime JVal::ParseDateTime()
{
	COleDateTime ot;
	ot.SetStatus(COleDateTime::invalid);

	if (IsString())
	{
		const auto& str = StrRef();
		CString st(str.c_str());
		if (str.length() == 14 && UcIsDigitStr(st))
		{
			CString sw; sw.Format(_T("%s-%s-%s %s:%s:%s")
				, str.substr(0, 4).c_str(), str.substr(4, 4 + 2).c_str(), str.substr(6, 6 + 2).c_str()
				, str.substr(8, 8 + 2).c_str(), str.substr(10, 10 + 2).c_str(), str.substr(12, 12 + 2).c_str());
			ot.ParseDateTime(sw);
			return ot;
		}
		else
		{
			ot.ParseDateTime(st);
			return ot;
		}
	}
	return ot;
}

/// 길이가 되면 담아온다.
LPCWSTR UcJObj::LenS(JKEYSTR k, CStringW& sv)
{
	if (this != nullptr)
	{
		//CStringW kw(k);
		ShJVal sjv;
		if (Lookup(k, sjv))
		{
			if (sjv->Val()->IsString())
			{
				const auto& ws = sjv->Val()->StrRef();
				if (ws.length() > 0)
					sv = ws.c_str();
				return (LPCWSTR)sv;
			}
		}
	}
	return NULL;
}

/// <summary>
/// 
/// </summary>
/// <param name="k"></param>
/// <param name="str"></param>
/// <param name="tok">사이 문자 '|' 또는 ';' 등</param>
/// <returns>없어서 들어 갔으면 TRUE </returns>
BOOL UcJObj::OrStr(JKEYSTR k, LPCWSTR str, char tok)
{
	if (!Find(k, str))
	{
		CStringW sapp;
		if (LenS(k, sapp))
			sapp += tok;
		sapp += str;
		(*this)(k) = sapp;
		return TRUE;// Append(k, sapp);
	}
	return FALSE;
}

CString JVal::ST(LPCTSTR def)/// const : _bufa 때문에 const를 못쓴다.
{
	//CStringA& sbufA = _bufa.GetBuf();
	CStringW defw(def);
	auto sw = S(defw);
	CString sbufT(sw);
	return sbufT;
}
CStringA JVal::SA(LPCSTR def)/// const : _bufa 때문에 const를 못쓴다.
{
	//CStringA& sbufA = _bufa.GetBuf();
	CStringW defw(def);
	//auto sw = S(defw);
	if (___.length() == 0)
		return {};
	return CStringA(___.c_str());
	//return sbufA;
	//if (def)
	//{
	//	sbufA = S(defw);
	//	return sbufA;
	//}
	//auto sw = S((LPCWSTR)NULL);
	//if (sw)
	//{
	//	sbufA = sw;// sa <= sw
	//	return sbufA;
	//}
	//return def;//NULL
}
// 	LPCSTR UcJObj::SA(LPCSTR k)
// 	{
// 		CString kw = CString(k);
// 		ShJVal sjv;
// 		if(Lookup((LPCWSTR)kw, sjv))
// 			return sjv->SA();
// 		return NULL;
// 	}



	/// <summary>
	/// Quata string: SQL 문에 데이터로 쓰일때 '%s' 대신 %s 만 써도 ' '를 붙여준다. 없는 경우 NULL 을 sql문에 맞게 준다.
	/// </summary>
	/// <param name="k"></param>
	/// <param name="bNullIfEmpty">공백 ""을  </param>
	/// <returns></returns>
CStringW UcJObj::QS(JKEYSTR k, BOOL bNullIfEmpty, BOOL bQuat, BOOL bNecessary)
{
	//CString& sbuf = _buf.GetBuf();
	CStringW sbuf;
	//auto buf = sbuf.GetBuffer(lenBuf);

	if (Has(k))//this->find(k) != this->end())
	{
		auto sjv = (*this)[k];
		if (sjv->Val()->IsString())
		{
			const auto& str = sjv->Val()->StrRef();

			LPCWSTR val = str.c_str();
			int len = tchlen(val);
			if (bNecessary && len == 0)
			{
				return nullptr;
			}
			if (bNullIfEmpty && len == 0)
				return L"NULL";
			else
			{
				if (bQuat)
					sbuf.Format(L"'%s'", val);
				else
					sbuf = val;
				return sbuf;
			}
		}
		else if (sjv->Val()->IsDouble())
		{
			//CString fmt; fmt = L"%%f";
			double d = sjv->Val()->AsDouble();
			sbuf.Format(L"%f", (double)d);
			return sbuf;
		}
		else if (sjv->Val()->IsInt())
		{
			//CString fmt; fmt = L"%%d";
			auto d = sjv->Val()->AsInt();
			sbuf.Format(L"%d", (int)d);
			return sbuf;
		}
		else if (sjv->Val()->IsInt64())
		{
			//CString fmt;fmt = L"%%I64d";
			auto d = sjv->Val()->AsInt64();
			sbuf.Format(L"%I64d", (__int64)d);
			return sbuf;
		}
		else if (sjv->Val()->IsBool())
			return sjv->Val()->AsBool() ? L"1" : L"0";
		else if (sjv->Val()->IsNull())
			return L"NULL";
		else if (sjv->Val()->IsObject())
			return L"[obj]";//이래야 SQL에러가 나도록 유도 하지.
		else if (sjv->Val()->IsArray())
			return L"[array]";
		else
		{
			ASSERT(0);
		}
	}
	else if (bNecessary)
	{
		return nullptr;
	}
	return L"NULL";
}




// SQL query문에 쓰일 문자열을 구한다.
CString UcJObj::QN(JKEYSTR k, int underDot)
{
	CStringW sbuf;

	if (this == NULL)
		throw_str(_T("UcJObj.this == NULL"));
	//?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
	if (Has(k))
	{
		bool bDone = false;
		auto v = (*this)[k];
		if (!v)
			throw_str(_T("UcJObj value is NULL"));
		if (!v->Val())
			return { _T("NULL") };

		auto val = v->Val();
		std::wstring sts;
		val->JSonTextVal(sts, 1000000);

#ifdef _as_JSonTextVal__
		if (val->IsDouble())
		{
			bDone = true;
			auto str = val->GetJSonText();
			sbuf = str.c_str();
		}
		else if (val->IsInt())
		{
			bDone = true;
			auto str = val->GetJSonText();
			sbuf = str.c_str();
		}
		else if (val->IsInt64())
		{
			bDone = true;
			auto str = val->GetJSonText();
			sbuf = str.c_str();
			//	fmt = L"%I64d";
		}
		else if (val->IsString())
		{
			bDone = true;
			auto str = val->GetJSonText();
			sbuf = str.c_str();
		}

		if (bDone)
			return (LPCWSTR)sbuf;

		// 여기 부터는 상수 문자열을 리턴 하므로 굳이 sbuf를 쓸 필요 없다.
		if (val->IsBool())// mySQL에서는 SQL에서 boolean 값은 0, 1 로 처리 되므로
			return val->AsBool() ? L"1" : L"0";
		else if (val->IsNull())
			return L"NULL";
		else if (val->IsObject())
			return L"[obj]";//이래야 SQL에러가 나도록 유도 하지.
		else if (val->IsArray())
			return L"[array]";
		else
		{
			ASSERT(0);
		}
#endif // _as_JSonTextVal__
	}
	return _T("NULL");
}

/// 내부 배열을 리턴 없으면 NULL
// 	ShJArr UcJObj::Array(LPCWSTR k)
// 	{
// 		ShJVal sjv;
// 		if(Lookup(k, sjv))
// 		{
// 			if(sjv->IsArray())
// 				return sjv->AsArray();
// 			else
// 				throw_str(L"IsArray() false.");
// 		}
// 		return NULL;
// 	}
	/// 내부 배열을 없으면 만들어서 라도 리턴
// 	ShJArr UcJObj::AMake(LPCWSTR k)
// 	{
// 		ShJVal sjv;
// 		if(Lookup(k, sjv))
// 		{
// 			if(sjv->IsArray())
// 				return sjv->AsArray();// shared_ptr 내부가 그대로 노출 된다.
// 			else
// 				throw_str(L"IsObject() false.");
// 		}
// 		ShJArr sjo = make_shared<UcJArr>();
// 		SetArray(k, sjo, false);
// 		return sjo;
// 	}








/// 복사 되는게 아니고, 내부 객체가 그대로 가르킨것이 포인터로 리턴된다.
// 	ShJObj UcJObj::Obj(LPCWSTR k)
// 	{
// 		/// 아래 ->find에서 unhandled로 못빠져 나온다. 왜 unhandled
// 		//?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
// 		if(this == nullptr)
// 			throw_str(L"this == nullptr.");
// 
// 		ShJVal sjv;
// 		if(Lookup(k, sjv))
// 		{
// 			if(sjv->IsObject())
// 				return sjv->AsObject();// shared_ptr 내부가 그대로 노출 된다.
// 			else
// 				throw_str(L"IsObject() false.");
// 		}
// 		return ShJObj();
// 	}
	/// make_shared<UcJObj>() 로 만들어 넣어서 빈객체라도 넣는다.
// 	ShJObj UcJObj::OMake(LPCWSTR k)
// 	{
// 		ShJVal sjv;
// 		if(Lookup(k, sjv))
// 		{
// 			if(sjv->IsObject())
// 				return sjv->AsObject();// shared_ptr 내부가 그대로 노출 된다.
// 			else
// 				throw_str(L"IsObject() false.");
// 		}
// 		ShJObj sjo = make_shared<UcJObj>();
// 		SetObj(k, sjo, false);
// 		return sjo;
// 	}
// 
#ifdef _UseGCBuf__
CStringW& JVal::SRef()
{
	CStringW& sbuf = _buf.GetBuf();
	sbuf = ___.c_str();
	return sbuf;
}
#endif // _UseGCBuf__

CStringW JVal::SLeft(int len)
{
	auto s = S();
	return s.Left(len);
}

CStringW JVal::SRight(int len)
{
	auto s = S();
	return s.Right(len);
}

CStringW JVal::SMid(int pos, int len)
{
	if (len > 0)
		return CStringW();
	auto s = S();
	return s.Mid(pos, len);
}


double JVal::N(double defv)
{
	if (this == NULL)
		throw (L"UcJObj.JVal == NULL");
	//?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
	auto sjv = this;
	if (sjv->IsDouble())
		return sjv->AsDouble();
	else if (sjv->IsInt())
		return (double)sjv->AsInt();
	else if (sjv->IsInt64())
		return (double)sjv->AsInt64();
	else if (sjv->IsString())
	{
		const auto& str = sjv->StrRef();
		CString s(str.c_str());
		double dr = defv;
		//if (!s.IsEmpty())
		if (UcIsFloatStr(s))
			dr = (double)UcAtof((LPCTSTR)s);
		//TRACE(L"Return double converted String JVal. %s => %f def(%f)\n", str.c_str(), dr, defv);
		return dr;
	}
	return defv;
}
void JVal::Inc(double inc)
{
	if (this == NULL)
		throw (L"UcJObj.JVal == NULL");
	//?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
	//auto sjv = this;
	switch (_type)
	{
	case VType::eInt:
	{
		auto v = AsInt();
		v += (int)inc;
		setInt(v);
		break;
	}
	case VType::eI64:
	{
		auto v = AsInt64();
		v += (INT64)inc;
		setInt64(v);
		break;
	}
	case VType::eFlt:
	{
		auto v = AsDouble();
		v += (double)inc;
		setDouble(v);
		break;
	}
	case VType::eStr:
	{
		const auto& str = StrRef();
		CString st(str.c_str());

		if (str.length() > 0) {
			if (UcIsFloatStr(st))
			{
				try {
					auto v = std::stod(str);
					v += (double)inc;
					___ = std::to_wstring(v);
				}
				catch (const std::invalid_argument&) {
					// 잘못된 문자가 포함된 경우 무시
				}
				catch (const std::out_of_range&) {
					// 범위를 벗어난 값인 경우 무시
				}
			}
		}
		break;
	}
	}
}

int JVal::I(int defv)
{
	int rv = defv;
	switch (_type)
	{
	case VType::eInt:
		rv = (int)AsInt();
		break;
	case VType::eI64:
		rv = (int)AsInt64();
		break;
	case VType::eFlt:
		rv = (int)AsDouble();
		break;
	case VType::eStr:
	{
#ifdef _DEBUGx
		auto ps = AsString().c_str();///주의: 이렇게 받으면 임시 wstring이 날라간다.
		CStringW sw(ps);
#endif // _DEBUG
		const auto& s = StrRef();
		// rv = UcAtoi(s.c_str());// 여기서 tchcpynum 로 숫자만 복사한후 변형 한다. - 기존 코드
		try {
			rv = std::stoi(s);
		}
		catch (const std::invalid_argument&) {
			// 잘못된 문자가 포함된 경우 기본값 반환
			rv = defv;
		}
		catch (const std::out_of_range&) {
			// 범위를 벗어난 값인 경우 기본값 반환
			rv = defv;
		}
		//(UcIsIntStr(s.c_str()))
			//TRACE(L"Return int value converted from String JVal.(%s => %d)\n", s.c_str(), rv);
		//TRACE(L"invalid string for int.(%s)\n", s.c_str());
	}	break;
	case VType::eBol:
		rv = AsBool() ? 1 : 0;
		break;
	case VType::eNul:
		rv = 0; // _type이 같으니. jv2.IsNull();//둘다 널이면
		break;
	default:
		rv = defv;
		break;
	}
	return rv;
}
DWORD JVal::DW(DWORD def)
{
	DWORD rv = def;
	switch (_type)
	{
	case VType::eInt:
		rv = (DWORD)AsInt();
		break;
	case VType::eI64:
		rv = (DWORD)AsInt64();
		break;
	case VType::eFlt:
		rv = (DWORD)AsDouble();
		break;
	case VType::eStr:
	{
		auto s = S();
		//if(s.Left(1) == L"#") // "#000000"}
		//	s = s.Mid(1); // '#' 제거
		try {
			rv = std::stoul(s.GetString());// StrRef());
			if (rv > 0xFFFFFFFFUL) { // DWORD 범위 체크
				throw std::out_of_range("value too big for DWORD");
			}
		}
		catch (const std::invalid_argument&) {
			// 잘못된 문자가 포함된 경우 기본값 반환
			rv = def;
		}
		catch (const std::out_of_range&) {
			// 범위를 벗어난 값인 경우 기본값 반환
			rv = def;
		}
	}	break;
	case VType::eBol:
		rv = AsBool() ? 1 : 0;
		break;
	case VType::eNul:
		rv = 0; // _type이 같으니. jv2.IsNull();//둘다 널이면
		break;
	default:
		rv = def;
		break;
	}
	return rv;
}

COLORREF JVal::Color(COLORREF def)
{
	COLORREF rv = def;
	switch (_type)
	{
	case VType::eInt:
		rv = (COLORREF)AsInt();
		break;
	case VType::eI64:
		rv = (COLORREF)AsInt64();
		break;
	case VType::eFlt:
		rv = (COLORREF)AsDouble();
		break;
	case VType::eStr:
	{
		auto s = S();
		if (!s.IsEmpty())
		{
			s.Trim();
			// 쉼표로 구분된 RGB 형식 처리 (예: "71,108,152")
			if (s.Find(L',') >= 0 && s.Left(1) != L"#")
			{
				std::vector<int> values = UcCutByTokenInt(s.GetString(), L", \t", true);
				if (values.size() >= 3)
				{
					// RGB 형식: values[0]=R, values[1]=G, values[2]=B
					rv = RGB(values[0], values[1], values[2]);
				}
				else if (values.size() == 1)
				{
					// 단일 값 - 그레이스케일로 처리
					rv = RGB(values[0], values[0], values[0]);
				}
				else
				{
					// 파싱 실패 시 기본값 반환
					rv = def;
				}
			}
			else if (s.Left(1) == L"#") {
				// 16진수 형식 처리
				s = s.Mid(1); // '#' 제거
				try {
					if (s.GetLength() == 8) {
						// ARGB 형식: "#00000000" -> AARRGGBB
						rv = (COLORREF)std::stoul(s.GetString(), nullptr, 16);
					}
					else if (s.GetLength() == 6) {
						// RGB 형식: "#000000" -> 00RRGGBB
						rv = (COLORREF)std::stoul(s.GetString(), nullptr, 16);
					}
					else {
						// 일반 숫자 문자열로 처리
						rv = (COLORREF)std::stoul(s.GetString());
					}
				}
				catch (const std::invalid_argument&) {
					// 잘못된 문자가 포함된 경우 기본값 반환
					rv = def;
				}
				catch (const std::out_of_range&) {
					// 범위를 벗어난 값인 경우 기본값 반환
					rv = def;
				}
			}
			else {
				// 일반 숫자 문자열로 처리
				try {
					rv = (COLORREF)std::stoul(s.GetString());
				}
				catch (const std::invalid_argument&) {
					// 잘못된 문자가 포함된 경우 기본값 반환
					rv = def;
				}
				catch (const std::out_of_range&) {
					// 범위를 벗어난 값인 경우 기본값 반환
					rv = def;
				}
			}
		}
	}	break;
	case VType::eBol:
		rv = AsBool() ? RGB(255, 255, 255) : RGB(0, 0, 0);
		break;
	case VType::eNul:
		rv = RGB(255, 255, 255);
		break;
	default:
		rv = def;
		break;
	}
	return rv;
}

__int64 JVal::I64(__int64 def)
{
	__int64 rv = def;
	switch (_type)
	{
	case VType::eInt:
		rv = (__int64)AsInt();
		break;
	case VType::eI64:
		rv = (__int64)AsInt64();
		break;
	case VType::eFlt:
		rv = (__int64)AsDouble();
		break;
	case VType::eStr:
	{
		TRACE("Return int converted String JVal.\n");
		auto& str = AsString();
		LPCWSTR s = str.c_str();
		CString st(str.c_str());
		// if (UcIsIntStr(st))
		// 	rv = UcAtoi64((LPCTSTR)st); - 기존 코드
		try {
			rv = std::stoll(str);
		}
		catch (const std::invalid_argument&) {
			// 잘못된 문자가 포함된 경우 기본값 반환
			rv = def;
		}
		catch (const std::out_of_range&) {
			// 범위를 벗어난 값인 경우 기본값 반환
			rv = def;
		}
	}	break;
	case VType::eBol:
		rv = AsBool() ? 1 : 0;
		break;
	case VType::eNul:
		rv = 0; // _type이 같으니. jv2.IsNull();//둘다 널이면
		break;
	default:
		//			ASSERT(0);
		rv = def;
		break;
	}
	return rv;
}
// 	double UcJObj::N(LPCWSTR k)
// 	{
// 		if(this == NULL)
// 			throw (L"UcJObj.this == NULL");
///		//?주의: if((*this)[k]) 이걸 [k] 쓰는 쑨간 만들어져 버린다.
// 		ShJVal sjv;
// 		if(Lookup(k, sjv))
// 			return sjv->N();
// 		return 0.;
// 	}

std::wstring UcJObj::ToJsonStringWStr(int lvPreety /*= 2*/, function<int(LPCWSTR, int)> cbChk /*= NULL*/)
{
	/// 여기서 JVal jsv(*jov) 하고 아래에서 없어지면서 안데 object도 날려 버리기 때문에 모든 문제가 생겼다.
	JVal jsv;
	jsv.ShareObj(*this);// JVal 함수를 쓰려고, 잠시 참조한다.
	std::wstring temp = jsv.Stringify(true, lvPreety, NULL, NULL, cbChk); // std::wstring으로 받기
	return temp;
}

CStringW UcJObj::ToJsonStringW(int lvPreety, function<int(LPCWSTR, int)> cbChk)
{
	std::wstring temp = ToJsonStringWStr(lvPreety, cbChk);
	CStringW sw = temp.c_str(); // std::wstring의 데이터를 CStringW에 복사 cppcheck
	return sw;
}

CStringA UcJObj::ToJsonStringUtf8(int lvPreety, function<int(LPCWSTR, int)> cbChk)
{
	std::wstring temp = ToJsonStringWStr(lvPreety, cbChk);
	return UcWcharToUTF8(temp.c_str());
}
shared_ptr<KBinary> UcJObj::ToJsonBinaryUtf8(int lvPreety)
{
	std::wstring temp = ToJsonStringWStr(lvPreety);
	shared_ptr<KBinary> binutf8 = make_shared<KBinary>();
	UcWcharToUTF8(temp.c_str(), *binutf8);
	return binutf8;
}

KBinary UcJObj::ToJsonData()
{
	CStringA jutf8 = this->ToJsonStringUtf8();
	KBinary bin;
	bin.SetPtr((LPCSTR)jutf8, jutf8.GetLength());
	return bin;
}

bool UcJObj::CopyFielsIf(UcJObj& src, JKEYSTR key)
{
	if (src.Has(key))
	{
		Set(key, src[key]);
		//(*this)[key] = src[key];
		return true;
	}
	return false;
}

int UcJObj::CopyFieldsAll(UcJObj& src)
{
	int n = 0;
	//src.for_loop([&](jstring key, auto rootVal) -> void
	for (auto& it : src)
	{
		auto& k = it.first;
		auto& v = it.second;
		// 		for(auto&[key, rootVal] : src)
		// 		{
		ShJVal shv = make_shared<JVal>(*v->Val());
		Set(k.c_str(), shv);//ShJVal(new JVal(*v)));
		//(*this)[key.c_str()] = ShJVal(new JVal(*rootVal));
		n++;
	}
	return n;
}
//void UcJObj::SetMove(LPCWSTR k, UcJObj& src)
//{
//	//src.get()->_bValueOwner = false; MoveObj에서 한다.
//	auto jv = new JVal();
//	jv->MoveObj(*src);
//	(*this)[k] = jv;// new JVal(v);
//}

UcJArr::UcJArr(UcJArr& jobj, bool bClone)
{
	CloneArray(jobj, *this, bClone);
}
UcJArr::UcJArr(ShJArr sja, bool bClone)
{
	auto& jov = *sja->Arr();
	CloneArray(jov, *this, bClone);
	//*this = *sja;
}







JUnit JUnit::operator()(wchar_t const* key)
{
	// 현재 키로 객체를 가져오거나 생성
	ShJVal sjv = m_pCJobj->Get(m_k);
	UcJObj* pObj = nullptr;

	if (!sjv || !sjv->Val()->IsObject())
	{
		// 객체가 없으면 새로 생성
		sjv = make_shared<JVal>();
		sjv->Val()->InitObject();
		m_pCJobj->Set(m_k, sjv);
	}
	//LNK2019: unresolved external symbol "public: class JUnit __cdecl UcJObj::operator()(wchar_t const *)"
	pObj = sjv->Val()->AsObjPtr();
	return JUnit(pObj, key);
}

void JUnit::operator=(const JUnit& v)
{
	ShJVal shv = m_pCJobj->Get(m_k);
	ShJVal shv2 = v.m_pCJobj->Get(v.m_k);
	if (shv2)
	{
		if (!shv)
		{
			shv = make_shared<JVal>(shv2, true);
			m_pCJobj->Set(m_k, shv);
		}
		else
			shv->operator=(*shv2);//*shv = *shv2 , JVal::operator=(JVal& jv)
	}
	else // shv2가 없으면 shv도 없어야 하는거 아닌가?
	{
		//shv = nullptr;//이거 해봐야 자기 자신만 empty가 되고 m_pCJobj안에 값은 보존된다. 
		//shared_ptr은 reset이나 null을 주면 원본은 참조가 줄어 든다.

		/// source가 없는 경우 target을 어떻게 할까.
		// operator니까 ASSERT 해서 소스가 무조건 있을때만 이함수를 사용 하고, 업는 경우는 SetIf 라든가 다른 함수를 쓴다.
		/// ASSERT(shv2); 없는 경우도 막 복사 한다.
		TRACE(L"%s <= source key (%s) isn't found!!!\n", m_k, v.m_k);
	}
}

///?error 이게 bool로 들어 가더니만, 그냥 재빌드 하니 된다. 빌드 꼬이면 bool type으로 들어 갈 수도 있다.
void JUnit::operator=(const char* va)
{
	CStringW v(va);
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setString(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}

void JUnit::operator=(const CStringW& v)
{
	*this = (LPCWSTR)v;
	//ShJVal sjv;
	//if (m_pCJobj->Lookup(m_k, sjv))
	//	sjv->Val()->setString(v);
	//else
	//	m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
void JUnit::operator=(const CStringA& va)
{
	*this = (LPCSTR)va;
	//CStringW v(va);
	//ShJVal sjv;
	//if (m_pCJobj->Lookup(m_k, sjv))
	//	sjv->Val()->setString(v);
	//else
	//	m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
void JUnit::operator=(const string& va)
{
	*this = va.c_str();
	//CStringW v(va.c_str());
	//ShJVal sjv;
	//if (m_pCJobj->Lookup(m_k, sjv))
	//	sjv->Val()->setString(v);
	//else
	//	m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
void JUnit::operator=(const wstring& va)
{
	*this = va.c_str();
	//CStringW v(va.c_str());
	//ShJVal sjv;
	//if (m_pCJobj->Lookup(m_k, sjv))
	//	sjv->Val()->setString(v);
	//else
	//	m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
#if CPP17_OR_LATER
void JUnit::operator=(const wstring_view va)
{
	*this = va.data();
	//CStringW v(va.data());
	//ShJVal sjv;
	//if (m_pCJobj->Lookup(m_k, sjv))
	//	sjv->Val()->setString(v);
	//else
	//	m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
#endif
void JUnit::operator=(const wchar_t* v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setString(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
//error C2593: 'operator ='이(가) 모호합니다.
void JUnit::operator=(__int64 v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setInt64(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
void JUnit::operator=(int v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setInt(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
void JUnit::operator=(WORD v)
{
	this->operator=((int)v);
}
void JUnit::operator=(BYTE v)
{
	this->operator=((int)v);
}

void JUnit::operator=(unsigned __int64 v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setUInt64(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}
void JUnit::operator=(unsigned int v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setUInt(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}

void JUnit::operator=(CTime v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setTime(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}

void JUnit::operator=(COleDateTime v)
{
	ShJVal sjv;// = make_shared<JVal>(v);
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setOTime(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}

void JUnit::operator=(double v)
{
	ShJVal sjv;
	if (m_pCJobj->Lookup(m_k, sjv))
		sjv->Val()->setDouble(v);
	else
		m_pCJobj->SetAt(m_k, make_shared<JVal>(v));
}

/// Sh??? 를 줄때는 그냥 share이지만
//void JUnit::operator=(ShJObj sv)
//{
//	m_pCJobj->SetAt(m_k, make_shared<JVal>(sv));//JVal(ShJBase) 에서 구분 한다.
//}
void JUnit::operator=(ShJVal sv)
{
	ShJVal sjv = make_shared<JVal>(sv);//여기서 이미 clone true 되어 버렸다.
#ifdef _DEBUG
	if (m_pCJobj->Has(m_k))
		_break;//디버그 용도//dwk: 2025-11-14 10:55 CJXArchive에서 ~CSaveLoad 에서 상위 노드에 넣고, 또 넣는지 보려고
#endif // _DEBUG
	m_pCJobj->Set(m_k, sjv);
}
void JUnit::operator=(JVal& jv)
{
	ShJVal sjv = make_shared<JVal>(jv);
	m_pCJobj->Set(m_k, sjv);
}

/// 아래 처럼 객체를 직접 주는 경우는 clone한다.
void JUnit::operator=(UcJObj& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}
void JUnit::operator=(UcJArr& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}


void JUnit::operator=(ONULL& v)
{
	m_pCJobj->Set(m_k, ShJVal(new JVal()));
}

#define SET_SHARED_JVAL(v) \
	ShJVal sjv = make_shared<JVal>(v);\
	m_pCJobj->Set(m_k, sjv);

void JUnit::operator=(CStringArray& v) { SET_SHARED_JVAL(v); }
//void JUnit::operator=(SHP<CStringArray>& v){SET_SHARED_JVAL(v);}
void JUnit::operator=(std::vector<std::wstring>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::vector<std::string>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::vector<int>& v) { SET_SHARED_JVAL(v); }//dwk: 2025-03-06 22:55
void JUnit::operator=(std::vector<unsigned int>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::vector<INT64>& v) { SET_SHARED_JVAL(v); }//dwk: 2025-03-06 22:55
void JUnit::operator=(std::vector<double>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::vector<size_t>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::vector<_variant_t>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::vector<TCString<TCHAR>>& v) { SET_SHARED_JVAL(v); }
#ifdef _MBCS
void JUnit::operator=(std::vector<CStringW>& v) { SET_SHARED_JVAL(v); }
#else
void JUnit::operator=(std::vector<CStringA>& v) { SET_SHARED_JVAL(v); }
#endif
void JUnit::operator=(std::list<std::wstring>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::list<int>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::list<INT64>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::list<double>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::list<size_t>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::list<TCString<TCHAR>>& v) { SET_SHARED_JVAL(v); }
#ifdef _MBCS
void JUnit::operator=(std::list<CStringW>& v) { SET_SHARED_JVAL(v); }
#else
void JUnit::operator=(std::list<CStringA>& v) { SET_SHARED_JVAL(v); }
#endif
void JUnit::operator=(CStringList& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(CArray<int, int>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(CArray<INT64, INT64>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(CArray<double, double>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(CArray<TCString<TCHAR>, TCString<TCHAR>>& v) { SET_SHARED_JVAL(v); }
#ifdef _MBCS
void JUnit::operator=(CArray<CStringW, CStringW>& v) { SET_SHARED_JVAL(v); }
#else
void JUnit::operator=(CArray<CStringA, CStringA>& v) { SET_SHARED_JVAL(v); }
#endif
void JUnit::operator=(CArray<CString, const CString&>& v) { SET_SHARED_JVAL(v); }

//void JUnit::operator=(std::list<CStringW>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::map<std::wstring, std::wstring>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::map<std::wstring, CStringW>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::map<std::wstring, int>& v) { SET_SHARED_JVAL(v); }
void JUnit::operator=(std::map<std::wstring, double>& v) { SET_SHARED_JVAL(v); }

void JUnit::operator=(std::map<std::wstring, std::vector<std::wstring>>& v) { SET_SHARED_JVAL(v); }



[[deprecated]]
ShJVal JVal::Parse(const wchar_t** data)
{
	ASSERT(0);
	return {};
}

/// <summary>
/// JSON 라인 번호를 사용하여 예외를 던지는 헬퍼 함수
/// JVal::Parse() 에서 catch (std::tuple<string, int, string, wstring, int, int>& stp)로 받는다.
/// tuple 안의 문자열 type 주의.
/// </summary>
/// <param name="function"></param>
/// <param name="line"></param>
/// <param name="message"></param>
/// <param name="jsonPos">JSON 문자열으리 현 위치(포인터)</param>
/// <param name="tr">JSON 정보를 가지고 다닌다.</param>
void UcJson::ThrowJsonWithLineInfo(const char* function, int line, const string& message, const wchar_t* jsonPos, shared_ptr<JTrain> tr)
{
	// SkipWhitespace에서 이미 tr->_jsonLine과 tr->_jsonColumn을 업데이트하고 있으므로
	 //auto tp = std::make_tuple(function, line, message, jsonPos, tr->_jsonLine, tr->_jsonColumn);
	 /// 이렇게 하면 catch (std::tuple<char const *,int,std::string,wchar_t const *,int,int> ) 이렇게 해야 되거든. 
	 //auto tp = std::make_tuple(std::string(function), line, std::string(message), std::wstring(jsonPos), tr->_jsonLine, tr->_jsonColumn); //OK 
	 /// catch (std::tuple<string, int, string, wstring, int, int>& stp) 랑 타입을 정확히 맞춰야 한다.
	 //throw tp;
	 //throw JException(function, line, message, jsonPos, tr->_jsonLine, tr->_jsonColumn);
	 ///  error C2664:가 나면 <std::string, int, std::string, std::wstring, int, int>를 제거 하거나, 일일이 std::move()를 붙인다.
	 //int line1 = line;
	 //int jline4 = tr ? tr->_jsonLine : -1;
	 //int jcol5 = tr ? tr->_jsonColumn : -1;
	 //throw std::make_tuple<std::string, int, std::string, std::wstring, int, int>(
	 //  std::string(function), //string 싸 줘야한다.
	 //  std::move(line1),//int는 move 필요
	 //  std::string(message), //string 싸 줘야한다.
	 //  std::wstring(jsonPos),//wstring 싸 줘야한다.
	 //  std::move(jline4),//int는 move 필요
	 //  std::move(jcol5)//int는 move 필요
	 //);//OK
	// JException 구조체를 사용하여 예외를 던집니다
	throw JException(std::string(function), line, std::string(message), std::wstring(jsonPos), tr->_jsonLine, tr->_jsonColumn);
}

ShJVal JVal::Parse(shared_ptr<JTrain> tr)
{
	const wchar_t** data = tr->_ppData;
	try
	{
		// Is it a string?
		if (**data == '"')
		{
			wstring str;
//-	[ptr]	0x0000022579074bf0 {_ppData=0x0000009ef9efd1f8 {0x0000022500eddd52 L""} _len=3981 _cur=0 ...}	JTrain *
//+	_ppData	0x0000009ef9efd1f8 {0x0000022500eddd52 L""}	const wchar_t * *
//		_len	3981	unsigned __int64
//		_cur	0	unsigned __int64
//+	_pStart	0x0000022500edbe38 L"{\"fRidRelayID\":\"B3F9A02AF37B4B08A2D27166D90A8C0B\",\"msg\":\"OK\",\"result\":\"SUCCESS\"\n  ,\"table\":{\n    \"fields\":[{\"name\":\"fQidRequestID\",\"type\":\"string\"},{\"name\":\"fRidRelayID\",\"type\":\"string\"},{\"name\":\"fFun...	const wchar_t *
//+	_cb	empty	std::function<int __cdecl(int,int,wchar_t const *)>
//		_jsonLine	12	int
//		_jsonColumn	1	int
			if (tr->_len == 3981 && 
				tr->_cur == 0 &&
				tchncmp(tr->_pStart, LR"({"fRidRelayID")", 14) == 0 &&
				tr->_jsonLine == 12 &&
				tr->_jsonColumn == 1
				)
				_break;
			if (!UcJson::ExtractString(&(++(*data)), str, tr->_pStart))
				throw_json_with_train("ExtractString", *data, tr);//에서 직접 throw하므로 여기 안온다.
			else
			{
				tr->Check(data);
				return ShJVal(new JVal(str));
			}
		}
		else if ((HasMinLengthW(*data, 4) && _wcsnicmp(*data, L"true", 4) == 0) ||
			(HasMinLengthW(*data, 5) && _wcsnicmp(*data, L"false", 5) == 0))
		{// Is it a boolean?
			bool value = _wcsnicmp(*data, L"true", 4) == 0;
			(*data) += value ? 4 : 5;
			return ShJVal(new JVal(value));
		}
		else if (HasMinLengthW(*data, 4) && _wcsnicmp(*data, L"null", 4) == 0)
		{// Is it a null?
			(*data) += 4;
			tr->Check(data);
			return ShJVal(new JVal());
		}
		else if (**data == L'-' || (**data >= L'0' && **data <= L'9'))
		{// Is it a number?
			// Negative?
			bool neg = **data == L'-';
			if (neg)
				(*data)++;
#ifdef _DEBUG
			int len = tchlen(*data);
#endif // _DEBUG

			//bool bDot = false;
			bool bFloat = false;
			//bool bInt = false;
			//bool bLong = false;
			double number = 0.0;
			// Parse the whole part of the number - only if it wasn't 0
			if (**data == L'0')
				(*data)++;
			else if (**data >= L'1' && **data <= L'9')
			{
				number = UcJson::ParseInt(data);
				tr->Check(data);
			}
			else
				throw_json_with_train("Invalid number format", *data, tr);

			// Could be a decimal now...
			if (**data == '.')
			{
				bFloat = true;
				//bDot = true;
				(*data)++;

				// Not get any digits?
				if (!(**data >= L'0' && **data <= L'9'))
					throw (int)__LINE__;// return NULL;

				// Find the decimal and sort the decimal place out
				// Use ParseDecimal as ParseInt won't work with decimals less than 0.1
				// thanks to Javier Abadia for the report & fix
				double decimal = UcJson::ParseDecimal(data);
				tr->Check(data);
				// Save the number
				number += decimal;
			}

			// Could be an exponent now...
			if (**data == L'E' || **data == L'e')
			{
				bFloat = true;
				(*data)++;

				// Check signage of expo
				bool neg_expo = false;
				if (**data == L'-' || **data == L'+')
				{
					neg_expo = **data == L'-';
					(*data)++;
				}

				// Not get any digits?
				if (!(**data >= L'0' && **data <= L'9'))
					throw_json_with_train("Invalid exponent format", *data, tr);

				// Sort the expo out
				double expo = UcJson::ParseInt(data);
				tr->Check(data);
				for (double i = 0.0; i < expo; i++)
					number = neg_expo ? (number / 10.0) : (number * 10.0);
			}

			// Was it neg?
			if (neg)
				number *= -1;
			if (!bFloat)
			{
				if (number <= 2147483648)//len <= 9)// 2,147,483,648
				{
					return ShJVal(new JVal((int)number));
				}
				else
				{
					return ShJVal(new JVal((__int64)number));
				}
			}
			return ShJVal(new JVal(number));
		}
		else if (**data == L'{')// An object?
		{
			auto sjo = make_shared<UcJObj>();
			auto jo = sjo->Dic();
			//JStrArray array_key;
			(*data)++;
			while (**data != 0)
			{
				// Whitespace at the start?
				UcJson::SkipWhitespaceThrow(data, tr);

				// Special case - empty object
				if (jo->size() == 0 && **data == L'}')
				{
					(*data)++;
					auto jv = new JVal(sjo, true);
					return ShJVal(jv);
				}

				tr->Check(data);
				// We want a string now...
				wstring name;
				if (!UcJson::ExtractString(&(++(*data)), name, tr->_pStart))
					throw_json_with_train("ExtractString", *data, tr);//에서 직접 throw하므로 여기 안온다.

				// @ 접두사 체크하여 _bAttr 설정
				bool isAttr = false;
				if (name.length() > 0 && name[0] == L'@') {
					isAttr = true;
					name = name.substr(1); // @ 제거
				}
				// More whitespace?
				tr->Check(data);
				UcJson::SkipWhitespaceThrow(data, tr);

				// Need a : now
				if (*((*data)++) != L':')
					throw_json_with_train("Need a ':' now.", *data, tr);// 키에 따옴표 안함, 맨끝 항목에 ','컴마 붙임. 

				// More whitespace?
				UcJson::SkipWhitespaceThrow(data, tr);

				// The value is here
				tr->Check(data);
				ShJVal value = Parse(tr);/// throw하니 또 throw 하면 안되.
				if (!value)
					return value;
				/// 여기서 또 throw 하면 이중으로 쌓임. throw_json_with_train("Parse returns empty value.", *data, tr);

			// Add the name:value
				if (jo->Has(name.c_str()))
					jo->DeleteKey(name.c_str());

				// _bAttr 플래그 설정
				if (isAttr) {
					value->Val()->_bAttr = true;
				}

				JString nameA(name.c_str());
				//JString jk(nameA);
				jo->SetAt((JKEYSTR)nameA, value);////////////////////////////// insert ////////////////////////////////////////
				//auto key1 = value->GetKey();

				// More whitespace?
				UcJson::SkipWhitespaceThrow(data, tr);

				/// End of object?
				if (**data == L'}')
				{
					(*data)++;
					tr->Check(data);
					ShJVal newJv = make_shared<JVal>(sjo, true);
					return newJv;// ShJVal(new JVal(sjo));
				}

				// Want a , now
				if (**data != L',')
					throw_json_with_train("Need a ',' now.", *data, tr);
				tr->Check(data);
				(*data)++;
			}
		}
		else if (**data == L'[')// An array?
		{
			auto shArr = ShJArr(new UcJArr());

			(*data)++;

			while (**data != 0)
			{
				// Whitespace at the start?
				UcJson::SkipWhitespaceThrow(data, tr);

				UcJArr* jarr = shArr->Arr();
				// Special case - empty array
				if (jarr->size() == 0 && **data == L']')
				{
					(*data)++;
					tr->Check(data);
					auto jv = new JVal(shArr, true);
					return ShJVal(jv);
				}

				// Get the value
				tr->Check(data);
				ShJVal sjv = Parse(tr);/// throw하니 또 throw 하면 안되.
				if (!sjv)
					return {};
				// Add the value
				jarr->Add(sjv, false);

				// More whitespace?
				UcJson::SkipWhitespaceThrow(data, tr);

				// End of array?
				if (**data == L']')
				{
					(*data)++;
					tr->Check(data);
					//auto jv = new JVal(shArr, true); return ShJVal(jv);
					auto rShJVal = make_shared<JVal>(shArr, true);
					return rShJVal;
				}

				// Want a , now
				if (**data != L',')
					throw_json_with_train("Want a ',' now.", *data, tr);

				tr->Check(data);
				(*data)++;
			}
		}
		else// Ran out of possibilites, it's bad!
			throw_json_with_train("Ran out of possibilites, it's bad!", *data, tr);
	}
	catch (KException* e)
	{
		TRACE(L"throw_str: %s[%s]\n", e->m_strStateNativeOrigin, e->_stack);
	}
	catch(CException* e)
	{
		CString serr;
		TRACE(L"%d line JSON parsing error.\n", e->GetErrorMessage(serr.GetBuffer(512), 512));
	}
	catch(std::exception e)
	{
		TRACE("std::exception: %s\n", e.what());
	}
	catch (const JException& ex)
	{
		auto wspcSt = ex.jsonPos.length() > 30 ? ex.jsonPos.substr(0, 30) : ex.jsonPos;
#ifdef _DEBUG
		CStringW sw(ex.message.c_str());// 경로에 \\ 하지 않고 '\' 만 한 경우
		TRACE(L"%d line JSON parsing error:%s. JSON Line:%d, Column:%d\n", ex.line, sw, ex.jsonLine, ex.jsonColumn);
#endif // _DEBUG
		UcJson::PushErr(ex);
	}
	catch (const std::tuple<const char*, int, const char*, const wchar_t*, int, int>& stp)
	{
		auto [fnc, errLine, msg, jsonPos, jsonLine, jsonColumn] = stp;
		JException ex(
			fnc ? fnc : __FUNCTION__,
			errLine,
			msg ? msg : "Tuple parse error",
			jsonPos ? jsonPos : L"",
			jsonLine,
			jsonColumn
		);
#ifdef _DEBUG
		CStringW sw(ex.message.c_str());
		TRACE(L"%d line JSON parsing error:%s. JSON Line:%d, Column:%d\n", ex.line, sw, ex.jsonLine, ex.jsonColumn);
#endif // _DEBUG
		UcJson::PushErr(ex);
	}
	catch (const std::tuple<string, int, string, wstring, int, int>& stp)
	{
//#define throw_json_with_train(s, j, tr) UcJson::ThrowJsonWithLineInfo
		//UcJson::ThrowJsonWithLineInfo
		auto [fnc, errLine, msg, jsonPos, jsonLine, jsonColumn] = stp;
		JException ex(
			fnc,
			errLine,
			msg,
			jsonPos,
			jsonLine,
			jsonColumn
		);
#ifdef _DEBUG
		CStringW sw(ex.message.c_str());
		TRACE(L"%d line JSON parsing error:%s. JSON Line:%d, Column:%d\n", ex.line, sw, ex.jsonLine, ex.jsonColumn);
#endif // _DEBUG
		UcJson::PushErr(ex);
	}
	catch (LPCWSTR eline)
	{
		TRACE("%s JSON parsing error.\n", eline);
	}
	catch (LPCSTR eline)
	{
		TRACE("%s JSON parsing error.\n", eline);
	}
	catch (int eline)
	{
		TRACE("%d line JSON parsing error.\n", eline);
	}
	catch (long eline)
	{
		TRACE("%d line JSON parsing error.\n", eline);
	}
	catch (...) {
		// catch-all에 들어오면 실제 타입을 한 번 더 분해해서 원인 손실을 막는다.
		try {
			throw;
		}
		catch (const std::tuple<const char*, int, const char*, const wchar_t*, int, int>& stp2)
		{
			auto [fnc, errLine, msg, jsonPos, jsonLine, jsonColumn] = stp2;
			JException ex(
				fnc ? fnc : __FUNCTION__,
				errLine,
				msg ? msg : "Tuple parse error (rethrown)",
				jsonPos ? jsonPos : L"",
				jsonLine,
				jsonColumn
			);
			UcJson::PushErr(ex);
		}
		catch (const std::tuple<string, int, string, wstring, int, int>& stp2)
		{
			auto [fnc, errLine, msg, jsonPos, jsonLine, jsonColumn] = stp2;
			JException ex(fnc, errLine, msg, jsonPos, jsonLine, jsonColumn);
			UcJson::PushErr(ex);
		}
		catch (const JException& ex2)
		{
			UcJson::PushErr(ex2);
		}
		catch (...) {
			JException ex(__FUNCTION__, __LINE__, "Unknown Error", L"", 0, 0);
			UcJson::PushErr(ex); ASSERT(0);
		}
	}
	return {};// ShJVal();//(int)__LINE__ cast를 해야 한다. 
}

JVal::JVal()
{
	_type = eNul;
	//toString();
}
JVal::JVal(const wchar_t* v1)//	: ___(v1)
{
	setString(v1);
}
JVal::JVal(const char* v1)
{
	setString(CStringW(v1).GetString());
}
JVal::JVal(const std::string v1)
{
	setString(CStringW(v1.c_str()).GetString());
}

JVal::JVal(const wstring& v1)//	: ___(v1)
{
	setString(v1.c_str());
}

JVal::JVal(bool v1)
{
	setBool(v1);
}
JVal::JVal(double v1)
{
	setDouble(v1);
}
JVal::JVal(int v1)
{
	setInt(v1);
}
JVal::JVal(__int64 v1)
{
	setInt64(v1);
}
JVal::JVal(unsigned int v1)
{
	setUInt(v1);
}
JVal::JVal(unsigned __int64 v1)
{
	setUInt64(v1);
}
// size_t는 unsigned __int64와 같은 타입일 수 있으므로 별도 구현 없음
JVal::JVal(CTime v1)
{
	setTime(v1);
}
JVal::JVal(COleDateTime v1)
{
	setOTime(v1);
}



/// object value가 clone된다.
JVal::JVal(ShJObj sjv, bool bClone)//, JStrArray& array_key1)
{
	if (!sjv)
	{
		_break;
		ASSERT(0);
	}
	if (sjv->IsDic())
	{
		_type = eObj;
		if (bClone)
		{
			InitObject();
			UcJObj::CloneObject(sjv, nod_, bClone);
		}
		else
		{
			nod_ = sjv;
		}
	}
	else if (sjv->IsArr())
	{
		_type = eArr;
		if (bClone)
		{
			InitArray();
			UcJArr::CloneArray(sjv, nod_, bClone);
		}
		else
			nod_ = sjv;
		///error SHP<UcJArr> &SHP<UcJArr>::operator=<UcJObj>(const SHP<UcJObj> &) noexcept'
	}
	else if (sjv->IsVal())
	{
		this->Clone(sjv->Val(), bClone);
	}
	else
	{
		ASSERT(0);//일단 테스트
	}
	//toString();
}

JVal::JVal(UcJArr& ja, bool bClone)
{
	_type = eArr;
	if (bClone)
	{
		InitArray();
		auto& tar = *nod_->Arr();
		UcJArr::CloneArray(ja, tar, bClone);
	}
	else {
		///dwk: 2025-03-19 16:18 내부 객체 _obj를 JVal함수를 쓰려는데, 임시 껍데기 JVal을 만들어 bClone=false로 주면 
		/// 자기가 죽을때 안건든다.
		//nod_ = ShJArr(&ja, TNotFree());
		nod_ = ShJArr(&ja, TNotFree([](const void* p) {
#ifdef _DEBUG
			auto pp = (UcJArr*)p;
			TRACE("ShJObj nod_ not free. %x\n", pp);
#endif // _DEBUG
			}));
	}
}
JVal::JVal(CStringArray& ja)//dwk: 2025-02-12 15:55 JSON 추가 필드 3
{
	_type = eArr;
	InitArray();
	auto arr = this->Arr();
	for (int i = 0; i < ja.GetCount(); i++)
		arr->Add(ja.GetAt(i));
}
#define ARRAYTOJARRAY 	_type = eArr;\
InitArray();\
auto arr = this->Arr();\
for (auto& s : ja)\
	arr->Add(s);\

JVal::JVal(std::vector<std::wstring>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::vector<std::string>& ja)
{
	_type = eArr;
	InitArray();
	auto arr = this->Arr();
	for (auto& s : ja)
		arr->Add(CStringW(s.c_str())); // std::string을 CStringW로 변환하여 추가
}
JVal::JVal(std::vector<int>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::vector<unsigned int>& ja)
{
	_type = eArr;
	InitArray();
	auto arr = this->Arr();
	for (auto& s : ja)
		arr->Add(static_cast<unsigned __int64>(s)); // 명시적 캐스팅으로 오버로드 모호성 제거
}
JVal::JVal(std::vector<INT64>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::vector<double>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::vector<size_t>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::vector<TCString<TCHAR>>& ja) { ARRAYTOJARRAY; }
#ifdef _MBCS
JVal::JVal(std::vector<CStringW>& ja) { ARRAYTOJARRAY; }
#else
JVal::JVal(std::vector<CStringA>& ja) { ARRAYTOJARRAY; }
#endif
JVal::JVal(std::vector<_variant_t>& ja)
{
	_type = eArr;
	InitArray();
	auto arr = this->Arr();
	for (auto& v : ja)
	{
		// _variant_t 전용 Add 오버로드를 사용하여 타입별로 안전하게 추가
		arr->Add(v);
	}
}

JVal::JVal(std::list<std::wstring>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::list<int>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::list<INT64>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::list<double>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::list<size_t>& ja) { ARRAYTOJARRAY; }
JVal::JVal(std::list<TCString<TCHAR>>& ja) { ARRAYTOJARRAY; }
#ifdef _MBCS
JVal::JVal(std::list<CStringW>& ja) { ARRAYTOJARRAY; }
#else
JVal::JVal(std::list<CStringA>& ja) { ARRAYTOJARRAY; }
#endif
JVal::JVal(CStringList& ja)
{
	_type = eArr;
	InitArray();
	auto arr = this->Arr();
	POSITION pos = ja.GetHeadPosition();
	while (pos) {
		const CString& s = ja.GetNext(pos);
		arr->Add((LPCTSTR)s);
	}
}
JVal::JVal(CArray<int, int>& ja) { ARRAYTOJARRAY; }
JVal::JVal(CArray<INT64, INT64>& ja) { ARRAYTOJARRAY; }
JVal::JVal(CArray<double, double>& ja) { ARRAYTOJARRAY; }
JVal::JVal(CArray<TCString<TCHAR>, TCString<TCHAR>>& ja) { ARRAYTOJARRAY; }
#ifdef _MBCS
JVal::JVal(CArray<CStringW, CStringW>& ja) { ARRAYTOJARRAY; }
#else
JVal::JVal(CArray<CStringA, CStringA>& ja) { ARRAYTOJARRAY; }
#endif
JVal::JVal(CArray<CString, const CString&>& ja) { ARRAYTOJARRAY; }

JVal::JVal(CArray<CStringW, CStringA>& ja) { ARRAYTOJARRAY; }
JVal::JVal(CArray<CStringA, CStringW>& ja) { ARRAYTOJARRAY; }

#if CPP17_OR_LATER
#define MAPTOJDIC 	_type = eObj;\
InitObject();\
auto& dic = *this->Dic();\
for (auto& [k, v] : ja)\
	dic(k) = v;
#else
#define MAPTOJDIC 	_type = eObj;\
InitObject();\
auto& dic = *this->Dic();\
for (auto& kv : ja){\
	auto& k = kv.first; auto& v = kv.second;\
	dic(k) = v;}
#endif

JVal::JVal(std::map<std::wstring, std::wstring>& ja)
{
	MAPTOJDIC;
}
JVal::JVal(std::map<std::wstring, CStringW>& ja) { MAPTOJDIC; }
JVal::JVal(std::map<std::wstring, int>& ja) { MAPTOJDIC; }
JVal::JVal(std::map<std::wstring, double>& ja) { MAPTOJDIC; }
JVal::JVal(std::map<std::wstring, std::vector<std::wstring>>& ja) { MAPTOJDIC; }


/// object value가 clone된다. fail
/// constructor라 아직 object_value가 NULL구조체 라고 나오고 아직 초기화가 안되어 있다.
JVal::JVal(UcJObj& jo, bool bClone)//, JStrArray& array_key1)
{
	_type = eObj;
	if (bClone)
	{
		InitObject();
		auto& tar = *nod_->Dic();
		UcJObj::CloneObject(&jo, tar, bClone);
	}
	else
	{
		///dwk: 2025-03-19 16:18 내부 객체 _obj를 JVal함수를 쓰려는데, 임시 껍데기 JVal을 만들어 bClone=false로 주면 
		/// 자기가 죽을때 안건든다. typedef shared_ptr<JBase> ShJObj
		nod_ = shared_ptr<JBase>(&jo, TNotFree([](const void* p) {
#ifdef _DEBUG
			auto pp = (UcJObj*)p;
			TRACE("ShJObj obj_ not free. %x\n", pp);
#endif // _DEBUG
			}));
	}
	//toString();
}

/// <summary>
/// object나 array일때만 bClone의 영향을 받는다.
/// </summary>
/// <param name="msource"></param>
/// <param name="bClone"></param>
JVal::JVal(const JVal& msource, bool bClone)
{
	this->Clone(&msource, bClone);
}
//JVal::JVal(ShJVal msource, bool bClone)
//{
//	this->Clone(*msource, bClone);
//}













/// this가 object value의 남의 객체에 껍질만 보유하여 free되지 않는다.
void JVal::ShareObj(UcJObj& obj1)//, JStrArray& array_key1)
{
	//ASSERT(0); /// 쓰는 경우가 있나?
	_type = eObj;
	nod_ = ShJObj(&obj1, TNotFree());
	/// ShJObj인 object_value는 어미인 this가 사라질때 TNotFree가 불려 져서 
	/// reference를 줄이거나 삭제 되지 않는다. 임시로 싸고 있다가 껍질만 사라진다.
}

bool JVal::AsBool(function<bool(CStringW&)> rd)
{
	switch (_type)
	{
	case eNul:
		return false;
	case eBol:
	case eStr:
	{
		CStringW sw(AsString().c_str());
		sw.MakeLower();
		if (rd)//람다 함수를 직접 주면. 한글이나 복잡한 경우
			return rd(sw);

		return UcJson::AsBool(sw);
	}	break;
	case eTme:
	{
		return !(___ == L"" || ___ == L"1900-01-01 00:00:00");//?error:cppcheck// dwkang 2023-05-26 14:06
		//return ___ == L"" || ___ == L"1900-01-01 00:00:00";
	}
	case eFlt:
	{
		if (IsNan())
			return false;
		else
			return AsDouble() != 0.;
	}
	case eI64:
	{
		if (IsNan())
			return false;
		else
			return AsInt64() != 0;
	}

	case eInt: ///저장 할때 int로 한다.
	{
		if (IsNan())
			return false;
		else
			return AsInt() != 0;
	}
	default:
		ASSERT(0);
		return false;
	}
	// orginal code : ASSERT(IsInt()); return  v_.I() != 0; 
}

bool UcJson::AsBool(CStringW sw)//dwk: 2025-10-20 16:06 
{
	sw.MakeLower();
	bool bFalse = (sw == L"false"
		|| sw == L"0" || sw == L"" || sw == L"n"
		|| sw == L"f"
		|| sw == L"no"
		|| sw == L"not"
		//|| sw == L"아니오"
		);
	return !bFalse;
}
/*
void JVal::WrappObj(UcJObj& obj1)//, JStrArray& array_key1)
{
	type = eObj;

	object_value = ShJObj(&obj1, TNotFree());
}

/// 다른 object value가 옮겨 와서 소유권도 가져 오며, 그 남의 객체는 껍질만 보유하며 free되지 않는다.
void JVal::MoveObj(UcJObj& obj1)//, JStrArray& array_key1)
{
	type = eObj;
	object_value = obj1;
	object_value._bValueOwner = true;// 포인터 소유권이 옮겨간다.
	obj1._bValueOwner = false;// 포인터 소유권이 없다.JVal가 껍데기만 쓴다.
}

void JVal::MoveArray(ShJArr& arr1)//, JStrArray& array_key1)
{
	type = eArr;
	array_value = arr1;
	array_value._bValueOwner = true;// 포인터 소유권이 옮겨간다.
	arr1._bValueOwner = false;// 포인터 소유권이 없다.JVal가 껍데기만 쓴다.
}
*/


//JVal::~JVal()
//{
//}

LPCWSTR JVal::Ptr()
{
	return SP(nullptr);
	//auto sjv = this;
	//if (sjv->IsString())
	//{
	//	auto& str = sjv->AsString();
	//	LPCWSTR rootVal = str.c_str();//여기서 로컬 포인터를 리턴 하면 안되지.
	//	return rootVal;
	//}
	//else if (sjv->IsNull())
	//{
	//	return nullptr;
	//}
	//else
	//{
	//	//ASSERT(0);//문자열 일때만 요청해야 한다. 아니면 GetText 를 쓰던가.
	//	throw_str(L"Not a string type. Ptr()");
	//}
	//return nullptr;
}
CStringW JVal::FStr(int point)
{
	//CString& sbuf = _buf.GetBuf();
	CStringW sbuf;

	auto sjv = this;
	if (sjv->IsString())
	{
		/// 아래와 같이 넘기면 포인터가 휘발성으로 날라 간다.
// 			LPCWSTR rootVal = sjv->AsString().c_str();
// 			return rootVal;
		const wstring& ws = sjv->AsString();
		sbuf = ws.c_str();
		return sbuf;
	}
	else if (sjv->IsDouble())
	{
		//CString fmt; fmt = L"%%f";
		double d = sjv->AsDouble();
		CStringW fmt; fmt.Format(L"%%.%df", point);
		sbuf.Format(fmt, (double)d);
		return sbuf;
	}
	else if (sjv->IsInt())
	{
		auto d = sjv->AsInt();
		sbuf.Format(L"%d", (int)d);
		return sbuf;
	}
	else if (sjv->IsInt64())
	{
		auto d = sjv->AsInt64();
		sbuf.Format(L"%I64d", (__int64)d);
		return sbuf;
	}
	else if (sjv->IsBool())
		return sjv->AsBool() ? L"1" : L"0";
	else if (sjv->IsNull())
		return L"NULL";
	else if (sjv->IsObject())
		return L"[obj]";//이래야 SQL에러가 나도록 유도 하지.
	else if (sjv->IsArray())
		return L"[array]";
	else
	{
		throw_str(_T("Not a string type. JVal::DStr()"));
	}
	return {};
}

/// UcJObj::CloneObject 도 같은 방식으로 포인터로 source를 받는다.
/// pjv가 null이면 clone하지 않는다.
void JVal::Clone(const JVal* pjv, bool bClone)
{
	ASSERT(pjv);// bClone과 상관 없이 pjv가 있어야 한다.
	//ASSERT(!bClone || pjv);// bClone이면 source가 있어야 한다.
	ASSERT(pjv);
	if (!pjv) {
		_type = eNul;
		return;
	}
	auto& jv = *pjv;
	//parent = msource.parent;
	_type = jv._type;
	_bAttr = jv._bAttr;//dwk: 2025-10-20 17:19 

	switch (_type)
	{
	case eStr:
	case eFlt:
	case eBol:
	case eInt:
	case eI64:
	case eTme:// dwkang 2023-05-26 14:01 이걸 빠트리다니
		___ = jv.___;
		break;
	case eArr:
	{
		if (bClone)
		{
			InitArray();//init nod_
			UcJArr::CloneArray(jv.nod_, nod_, bClone);
		}
		else
			nod_ = jv.nod_;// ShJArr(&ja, TNotFree());
		break;
	}
	case eObj:
	{
		if (bClone)
		{
			InitObject();//init obj_
			UcJObj::CloneObject(jv.nod_, nod_, bClone);
		}
		else
			nod_ = jv.nod_;
		break;
	}
	case eNul:		// Nothing to do.
		break;
	}
	//toString();
}

std::size_t JVal::CountChildren() const
{
	switch (_type)
	{
	case eArr:
		return nod_ ? nod_->Arr()->size() : 0;
	case eObj:
		return nod_ ? nod_->Dic()->size() : 0;
	default:
		return 0;
	}
}

bool JVal::HasChild(std::size_t index) const
{
	if (_type == eArr)
	{
		return index < nod_->Arr()->size();
	}
	else
	{
		return false;
	}
}

ShJVal JVal::Child(std::size_t index)
{
	auto arr = nod_->Arr();
	if (index < arr->size())
	{
		return arr->GetAt((int)index);
		//return (*array_value)[index];
	}
	else
	{
		return NULL;
	}
}
bool JVal::HasChild(JKEYSTR name) const
{
	return nod_ ? nod_->Dic()->Has(name) : false;
}
ShJVal JVal::Child(JKEYSTR name)
{
	ShJVal sjv;
	if (nod_)
		nod_->Dic()->Lookup(name, sjv);
	return sjv;
}

bool JVal::IsNan()
{
	auto& t = _type;
	if (t == eInt || t == eI64 || t == eUnt || t == eU64)
		return false;
	else
	{
		if (t == eFlt)
		{
			auto d = AsDouble();
#ifdef _DEBUG
			auto bnan = isnan(d);
			auto binf = isinf(d);
			auto bfinite = isfinite(d);
#endif // _DEBUG
			return isnan(d) || isinf(d) || !isfinite(d);
		}
		else
			return false;
	}
}


[[deprecated]]
std::wstring JVal::Indent(size_t depth)
{
	const size_t indent_step = 2;
	depth ? --depth : 0;
	std::wstring indentStr(depth * indent_step, ' ');
	return indentStr;
}



int JVal::setValue(ShJVal snd1)
{
	*this = *snd1->Val();
	int rv = 0;

	//toString();
	return rv;
}

std::wstring JVal::GetJSonText(int maxlen)
{
	std::wstring sts;
	JSonTextVal(sts, 100);
	return sts;
}

void JVal::JSonTextVal(std::wstring& sts, int maxlen, bool bQuat)
{
	sts.clear();
	CStringW sbuf;
	if (this->IsString())
	{
		auto& wstrr = this->AsString();
		if (wstrr.length() == 0)
			return;

		if (wstrr.length() <= (size_t)maxlen)
		{
			if (!bQuat)
				sts = wstrr.c_str();
			else
			{
				sbuf.Format(L"\"%s\"", wstrr.c_str());
				sts = sbuf;
			}
		}
		else // 내부 디버그 문자열 너무 길면 줄인다.
		{
			wstring wstr;
			wstr = wstrr.substr(0, maxlen);
			if (!bQuat)
				sbuf.Format(L"%s...", wstr.c_str());
			else
				sbuf.Format(L"\"%s...\"", wstr.c_str());
			sts = sbuf;
		}
		return;
	}

	//auto sbuf = make_shared<wchar_t>(1024);
	//const int lBuf = 1024;
	//wchar_t* buf = sbuf.GetBuffer(lBuf);// new wchar_t[1024];//[1024];
	//wchar_t* buf = sbuf.get();// new wchar_t[1024];//[1024];
	//KAtEnd d_buf([&, buf]() { DeleteMeSafe(buf); });

	if (this->IsDouble())
	{
		auto dn = this->AsDouble();
		if (dn == 0.)
			sbuf = L"0.";
		else
			sbuf.Format(L"%.f", dn);
	}
	else if (this->IsInt())
	{
		auto dn = this->AsInt();
		sbuf.Format(L"%d", (int)dn);
	}
	else if (this->IsInt64())
	{
		auto dn = this->AsInt64();
		sbuf.Format(L"%I64d", (__int64)dn);
	}
	else if (this->IsBool())
		sbuf.Format(L"%s", this->AsBool() ? L"true" : L"false");
	else if (this->IsNull())
		sbuf = (L"(null)");
	else if (this->IsArray())
		sbuf = (L"(array)");
	else if (this->IsObject())
		sbuf = (L"(object)");
	else
		sbuf = (L"(unknown)");
	sts = sbuf;
#ifdef _DEBUG
	if (sts == L"\"\"")
		_break;
#endif // _DEBUG
	//sbuf.ReleaseBuffer();
	return;
}

void JVal::InitArray()
{
	ASSERT(this->_type == eArr);
	auto arr = Arr();
	if (!arr)
		nod_ = make_shared<UcJArr>();
	else
		arr->clear();
}

void JVal::InitObject()
{
	ASSERT(this->_type == eObj);
	auto obj = Dic();
	if (!obj)
		nod_ = make_shared<UcJObj>();
	else
		obj->clear();
}

void JVal::operator=(const JVal& jv)
{
	Clone(&jv, true);
}

void JVal::operator+=(CTimeSpan v)
{
	CTime t = T();
	t += v;
	//CStringW s;
	//UcCTimeToString(t, s);
	setTime(t);
}
void JVal::operator+=(COleDateTimeSpan v)
{
	COleDateTime t = TO();
	t += v;
#ifdef _DEBUG
	//CString s = UcOTimeToString(t);
#endif // _DEBUG
	setOTime(t);
}

BOOL UcJObj::IsSame(UcJObj& jbj2)// const
{
	for (const auto& it : jbj2)
	{
		auto k2 = it.first;
		auto sjv2 = it.second;
		auto sjv1 = (*this)[k2];
		if (!sjv1->Val()->IsSame(sjv2)) // if(*sjv1 != *sjv2)
			return FALSE;// 다른게 하나라도 있으면 전체가 다른것으로 간주.
	}
	return TRUE;
}

BOOL UcJArr::IsSame(UcJArr& jar2)// const
{
	if (size() == jar2.size())
	{
		for (size_t i = 0; i < size(); i++)
		{
			auto sjv2 = jar2[i];
			auto sjv1 = (*this)[i];
			if (!sjv1->Val()->IsSame(sjv2)) // if(*sjv1 != *sjv2)
				return FALSE;// 다른게 하나라도 있으면 전체가 다른것으로 간주.
		}
	}
	return TRUE;
}

// 0: same
// 1: this < jval1
// -1: this > jval1
int JVal::CompareValue(JVal& jval1)
{
	int rv = -1;// object 인경우 -1 리턴
	if (this->_type == jval1._type)
	{
		switch (_type)
		{
		case VType::eNul:
			rv = 0; // _type이 같으니. jv2.IsNull();//둘다 널이면
			break;
		case VType::eStr:
		{
			const auto& str1 = this->StrRef();
			const auto& str2 = jval1.StrRef();
			rv = str1.compare(str2);//?error:cppcheck// wcscmp(str1.c_str(), str2.c_str());//str1 == str2 ? 0 : ; //
		}
		break;
		case VType::eBol:
			rv = (jval1.AsBool() ? 1 : 0) - (this->AsBool() ? 1 : 0);
			break;
		case VType::eFlt:
		{
			double drv = jval1.AsDouble() - this->AsDouble();
			rv = (drv == 0.) ? 0 : (drv > 0.) ? 1 : -1;
		}	break;
		case VType::eInt:	//
		{
			auto rv1 = jval1.AsInt() - this->AsInt();
			rv = (rv1 == 0) ? 0 : (rv1 > 0) ? 1 : -1;
			//rv = jval1.AsInt() == AsInt();
		}	break;
		case VType::eI64:
		{
			auto rv1 = jval1.AsInt64() - this->AsInt64();
			rv = (rv1 == 0) ? 0 : (rv1 > 0) ? 1 : -1;
		}	break;
#ifdef _CompareArrayObject
#endif // _CompareArrayObject
		//TODO: _CompareArrayObject
		case VType::eArr:
		{
			auto shar0 = AsArray();
			auto shar1 = jval1.AsArray();
			rv = shar0 == shar1 ? 0 : 1;//shar1->size() - shar0->size();
		}	break;
		case VType::eObj:
		{
			auto shar0 = AsObject();
			auto shar1 = jval1.AsObject();
			rv = shar0 == shar1 ? 0 : 1;//shar1->size() - shar0->size();
		}	break;
		default:
			ASSERT(0);
			break;
		}
	}
	return rv;
}

BOOL JVal::IsSame(JVal& jval1)
{
	BOOL rv = FALSE;// object 인경우 -1 리턴
	switch (_type)
	{
	case VType::eArr:
	{
		auto shar0 = AsArray();
		auto shar1 = jval1.AsArray();
		rv = *shar1->Arr() == *shar0->Arr();// shar1->size() - shar0->size();
	}	break;
	case VType::eObj:
	{
		auto shar0 = AsObject();
		auto shar1 = jval1.AsObject();
		rv = *shar1->Dic() == *shar0->Dic();// shar1->size() - shar0->size();
	}	break;
	default:
		double drv = CompareValue(jval1);
		rv = drv == 0.;// ? 1 : 0;//drv < 0 ? -1 : 1;// object 인경우 -1 리턴
		break;
	}
	//if (this->type == snd1->type)
	//{
	//	if (snd1->IsString())
	//		rv = this->AsString() == snd1->AsString() ? 1 : 0;
	//	else if (snd1->IsNumber())
	//		rv = this->AsNumber() == snd1->AsNumber() ? 1 : 0;
	//	else if (snd1->IsBool())
	//		rv = this->AsBool() == snd1->AsBool() ? 1 : 0;
	//}
	return rv;
}
// out-of-line definition: UcJObj::operator()(const wchar_t*)
//JUnit UcJObj::operator()(const wchar_t* key)
//{
//	return JUnit(this, key);
//}

//dwk: 2025-08-22 09:19 cursor AI
JUnit UcJObj::operator()(const wchar_t* path)
{
	// 경로에 '/'가 없으면 일반 키로 처리
	if (wcschr(path, L'/') == nullptr)
		return JUnit(this, path);
	// 경로를 '/'로 분할
	std::vector<jstring> pathParts;
	CStringW pathStr(path);
	int pos = 0;
	CStringW token = pathStr.Tokenize(L"/", pos);
	while (!token.IsEmpty()) {
		pathParts.push_back((LPCWSTR)token);
		token = pathStr.Tokenize(L"/", pos);
	}
	// 마지막 키를 제외한 모든 경로를 순회하며 객체 생성
	UcJObj* currentObj = this;
	for (size_t i = 0; i < pathParts.size() - 1; i++) {
		ShJVal sjv = currentObj->Get(pathParts[i]);
		if (!sjv || !sjv->Val()->IsObject()) {
			// 객체가 없으면 새로 생성
			sjv = make_shared<JVal>();
			sjv->Val()->InitObject();
			currentObj->Set(pathParts[i], sjv);
		}
		currentObj = sjv->Val()->AsObjPtr();
	}
	// 마지막 키로 JUnit 반환
	return JUnit(currentObj, pathParts.back());
}

/// 구한 배열내 가져올 데이터가 여러개인 경우 
ShJObj UcJObj::GetArrayItem(JKEYSTR karr, int idx)
{
	if (Has(karr))
	{
		if (IsArray(karr))
		{
			auto sjarr = Array(karr);
			auto jarr = sjarr->Arr();
			if (jarr->size() > (size_t)idx)
			{
				auto sjv = jarr->at(idx);
				if (sjv->Val()->IsObject())
				{
					return sjv->Val()->AsObject();
				}
			}
		}
	}
	return nullptr;
}

// k의 value가 어떤 type 인지 모르니까 JVal로 받는다.
ShJVal UcJObj::GetArrayItem(JKEYSTR karr, int idx, JKEYSTR k)
{
	ShJVal sjv;
	ShJObj sjo = GetArrayItem(karr, idx);
	if (!sjo)
		return sjv;
	auto jo = sjo->Dic();
	if (jo->Lookup(k, sjv))//sjos->Has(k))
	{
		if (!sjv->Val()->IsNull())
			return sjv;
	}
	return sjv;
}

bool UcJObj::GetArrayItem(JKEYSTR karr, int idx, JKEYSTR k, CStringW& rval)
{
	ShJVal sjv = GetArrayItem(karr, idx, k);
	if (!sjv)
		return false;
	rval = sjv->Val()->S();
	if (rval.GetLength() > 0)
		return true;
	return false;
}

bool UcJObj::GetArrayItem(JKEYSTR karr, int idx, JKEYSTR k, int& rval)
{
	ShJVal sjv = GetArrayItem(karr, idx, k);
	if (!sjv)
		return false;
	if (sjv->Val()->IsInt())
	{
		rval = sjv->Val()->AsInt();
		return true;
	}
	else
		throw_str(_T("Wrong return type. GetArrayItem"));
	return false;
}









void UcJArr::Add(const wchar_t* v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}
void UcJArr::Add(const char* v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}
void UcJArr::Add(double v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}
void UcJArr::Add(int v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}
void UcJArr::Add(INT64 v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}
void UcJArr::Add(unsigned __int64 v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}
//inline void Add(long v)
//{
//	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
//}
void UcJArr::Add(bool v)
{
	KArray<ShJVal>::Add(ShJVal(new JVal(v)));
}

void UcJArr::Add(const _variant_t& v)
{
	// _variant_t 내용을 적절한 JSON 타입으로 변환해서 추가
	switch (v.vt)
	{
	case VT_I1: case VT_I2: case VT_I4: case VT_INT:
	case VT_UI1: case VT_UI2: case VT_UI4: case VT_UINT:
		Add(static_cast<INT64>(static_cast<LONGLONG>(v))); // 정수형
		break;
	case VT_I8:
		Add(static_cast<INT64>(v.llVal));
		break;
	case VT_UI8:
		Add(static_cast<unsigned __int64>(v.ullVal));
		break;
	case VT_R4:
		Add(static_cast<double>(v.fltVal));
		break;
	case VT_R8:
		Add(static_cast<double>(v.dblVal));
		break;
	case VT_BSTR:
	{
		_bstr_t bs(v);
		Add(static_cast<LPCWSTR>(bs));
		break;
	}
	case VT_BOOL:
		Add(v.boolVal == VARIANT_TRUE);
		break;
	default:
	{
		// 지원하지 않는 타입은 문자열로 fallback
		_bstr_t bs(v);
		Add(static_cast<LPCWSTR>(bs));
		break;
	}
	}
}

/// 메모리 비효율적. 코드 간편
void UcJArr::Add(UcJObj& v, bool bClone)
{
	//auto jv = new JVal(v, bClone);//v는 복제 clone 된다.
	ShJVal sjv = make_shared<JVal>(v, bClone);
	KArray<ShJVal>::Add(sjv);// ShJVal(jv));
}

//ShJVal UcJArr::Add(ShJObj sv)//2024-01-22 09:30:35
//{
//	ShJVal sjv = make_shared<JVal>(sv, false);
//	__super::Add(sjv);
//	return sjv;//*sv->Dic();
//}
ShJVal UcJArr::Add(ShJVal sv, bool bClone)
{
#ifdef _DEBUG
	if (bClone)// ShJVal를 Add 하면서 복사 할 일이 있나?
		_break;
#endif // _DEBUG
	if (!sv)
		return {};
	ShJVal sjv;
	if (bClone)
		sjv = make_shared<JVal>(sv, bClone);
	else
		sjv = sv;
	KArray<ShJVal>::Add(sjv);//push_back(sjv)
	return sjv;
}
//UcJObj& UcJArr::Add(UcJObj&& obj)
//{
//	shared_ptr<UcJObj> sv = make_shared<UcJObj>(std::forward<UcJObj>(obj));//'Kw::UcJObj': 복사 생성자를 사용할 수 없거나 복사 생성자가 'explicit'으로 선언되었습니다.
//	ShJVal sjv = make_shared<JVal>(sv, false);
//	__super::Add(sjv);//error
//	return *sjv->Dic();
//}
UcJObj& UcJArr::AddObj()
{
	ShJVal sv = make_shared<UcJObj>();
	ShJVal sjv = make_shared<JVal>(sv, false);
	KArray<ShJVal>::Add(sjv);
	return *sjv->Dic();
}
UcJObj& UcJArr::InsertObj(int idx)
{
	ShJVal sv = make_shared<UcJObj>();
	ShJVal sjv = make_shared<JVal>(sv, false);
	std::vector<ShJVal>::insert(this->begin() + idx, sjv);
	return *sjv->Dic();
}

void UcJArr::DeleteAt(int idx)
{
	KArray<ShJVal>::erase(this->begin() + idx);
}

/// 메모리 효율적. 코드 복잡
//void UcJArr::Add(ShJObj sv, bool bClone)
//{
//	//		auto jv = new JVal(sv);// sv안에 있는 UcJObj 는 계속 share된다.
//	ShJVal sjv = make_shared<JVal>(sv, bClone);
//	__super::Add(sjv);
//	//KArray<ShJVal>::Add
//}


// obj 밑에 배열 있고, 그 배열 항목 하나에 또 배열이 있고, 그렇게 루트까지 함쳐서 4단계 obj가 있다면
// 각 obj 별로 람다함수를 불러서 처리하는 엔진이다.

/// <summary>
/// UcJObj만 모두 찾아서 람다함수로 넘겨 준다
/// </summary>
/// <param name="jRoot"></param>
/// <param name="keySub"></param>
/// <param name="nth"></param>
/// <param name="arFnc"></param>
/// <param name="bRecursive"></param>
void UcJObj::DeepLoopRecursive(UcJArr& jArr, vector<jstring>& keySub, int nth, vector<function<void(UcJObj&, int)>>& arFnc, bool bRecursive)
{
	try
	{
		for (auto& sub1 : jArr)
		{
			if (sub1->IsDic())
			{
				auto* jSub = sub1->Dic();
				if (jSub)
				{
					auto& jSub1 = *jSub;
					DeepLoopRecursive(jSub1, keySub, nth + 1, arFnc);
				}
			}
			else if (sub1->IsArr())
			{
				auto& arSub1 = *sub1->Arr();
				DeepLoopRecursive(arSub1, keySub, nth + 1, arFnc);
			}
		}
	}
	catch (CString e)
	{
		throw_str(e);
	}
}

void UcJObj::DeepLoopRecursive(UcJObj& jRoot, vector<jstring>& keySub, int nth, vector<function<void(UcJObj&, int)>>& arFnc, bool bRecursive)
{
	if (jRoot.size() == 0)// 키가 없으므로
		return;

	try
	{
		int iCB = nth;// 트리 단계에서 루트는 n번째 레벨 이면 N번째 fnc를 실행 한다.
		if (arFnc.size() <= (size_t)iCB)
			iCB = (int)arFnc.size() - 1;// callback lambda가 부족 하면 맨 뒤에 것을 사용 한다.
		while (!arFnc[iCB]) //  N번째 함수가 NULL이면 점점 루트쪽에 가까운 함수를 쓴다.
			iCB--;
		ASSERT(iCB >= 0);// 적어도 하나는 있겠지.

		/// 람다함수 실행 ///////////////////////////////////////////////////////////////
		arFnc[iCB](jRoot, nth);//?step 1
		/// 람다함수 실행 ///////////////////////////////////////////////////////////////

		auto CallRecursive = [&keySub, &nth, &arFnc](ShJVal sjv) {
			if (sjv->IsArr())//배열이면 항목이 Dic인 경우만 리커시브 호출한다
			{
				auto& arSub1 = *sjv->Arr();
				DeepLoopRecursive(arSub1, keySub, nth + 1, arFnc);
				//for (auto& sub1 : arSub1)
				//{
				//	if(sub1->IsDic())
				//	{
				//		auto* jSub = sub1->Dic();
				//		if (jSub)
				//		{
				//			auto& jSub1 = *jSub;
				//			DeepLoopRecursive(jSub1, keySub, nth + 1, arFnc);
				//		}
				//	}
				//}
			}
			else if (sjv->IsDic())
			{
				auto& objSub1 = *sjv->Dic();
#if CPP17_OR_LATER
				for (auto& [k, sub1] : objSub1) { //Dic이면 Vaule가 Dic인 경우만 리커시브 호출한다
#else
				for (auto& pair : objSub1) {
					auto& k = pair.first;
					auto& sub1 = pair.second;
#endif
					if (sub1->IsDic())
					{
						auto jSub = sub1->Dic();
						if (jSub)
						{
							auto& jSub1 = *jSub;//sub1->Dic();
							DeepLoopRecursive(jSub1, keySub, nth + 1, arFnc);
						}
					}
					else if (sub1->IsArr())
					{
						auto& arSub1 = *sub1->Arr();
						DeepLoopRecursive(arSub1, keySub, nth + 1, arFnc);
					}
				}
				}
			//else 면 일반 값
			};
		if (bRecursive)
		{
			if (keySub.size() <= (size_t)nth)//특정 키만 Array 또는 Object인지 체크
			{
#if CPP17_OR_LATER
				for (auto& [k, sjv] : jRoot) {
#else
				for (auto& pair : jRoot) {
					auto& k = pair.first;
					auto& sjv = pair.second;
#endif
					CallRecursive(sjv);
				}
				}
			else//모든 키 점
			{
				auto kSub1 = keySub[nth];
				ShJVal sjv;
				if (jRoot.Lookup(kSub1, sjv))
					CallRecursive(sjv);//?step 2
			}
			}
		}
	catch (CString e)
	{
		throw_str(e);
	}
			}

//void UcJObj::DeepLoopRecursive(ShJBase jv, vector<jstring>& keySub, int nth, vector<function<void(ShJBase, int)>>& arFnc, bool bRecursive)
//{
	//auto CallRecursive = [&keySub, &nth, &arFnc](ShJVal sjv) {
	//	if (sjv->IsArr())
	//	{
	//		auto& arSub1 = *sjv->Arr();
	//		//auto& arSub1 = jRoot.GetArray(kSub1);// n번째 키로 배열ㅇ르 가져온다.
	//		for (auto& sub1 : arSub1)
	//		{
	//			if (sub1->IsDic())
	//			{
	//				auto* jSub = sub1->Dic();
	//				if (jSub)
	//				{
	//					auto& jSub1 = *jSub;
	//					DeepLoopRecursive(sub1, keySub, nth + 1, arFnc);
	//				}
	//			}
	//		}
	//	}
	//	else if (sjv->IsDic())
	//	{
	//		auto& objSub1 = *sjv->Dic();
	//		//ASSERT(0); ///TODO: 계층 구조에서 다음 차원이 obj인 경우는 아직 test 안했다.
	//		for (auto& [k, sub1] : objSub1)
	//		{
	//			if (sub1->IsDic())
	//			{
	//				auto jSub = sub1->Dic();
	//				if (jSub)
	//				{
	//					auto& jSub1 = *jSub;//sub1->Dic();
	//					DeepLoopRecursive(sub1, keySub, nth + 1, arFnc);
	//				}
	//			}
	//			else if (sjv->IsArr())
	//			{
	//				ASSERT(0);
	//			}
	//		}
	//	}
	//};
	//try
	//{
	//	if (jv->IsDic())
	//	{
	//		auto& jRoot = *jv->Dic();
	//		int iCB = nth;// 트리 단계에서 루트는 n번째 레벨 이면 N번째 fnc를 실행 한다.
	//		if (arFnc.size() <= iCB)
	//			iCB = (int)arFnc.size() - 1;// callback lambda가 부족 하면 맨 뒤에 것을 사용 한다.
	//		while (!arFnc[iCB]) //  N번째 함수가 NULL이면 점점 루트쪽에 가까운 함수를 쓴다.
	//			iCB--;
	//		ASSERT(iCB >= 0);// 적어도 하나는 있겠지.
	//		/// 람다함수 실행 ///
	//		arFnc[iCB](jv, nth);
	//		if (jRoot.size() == 0)
	//			return;
	//		if (bRecursive)
	//		{
	//			if (keySub.size() > nth)//특정 키만 Array 또는 Object인지 체크
	//			{
	//				auto kSub1 = keySub[nth];
	//				ShJVal sjv;
	//				if (jRoot.Lookup(kSub1, sjv))
	//					CallRecursive(sjv);
	//			}
	//			else//모든 키 점
	//			{
	//				for (auto& [k, sjv] : jRoot)
	//					CallRecursive(sjv);
	//			}
	//		}
	//	}
	//	else if (jv->IsArr())
	//	{
	//		auto& jArr = *jv->Arr();
	//		int iCB = nth;// 트리 단계에서 루트는 n번째 레벨 이면 N번째 fnc를 실행 한다.
	//		if (arFnc.size() <= iCB)
	//			iCB = (int)arFnc.size() - 1;// callback lambda가 부족 하면 맨 뒤에 것을 사용 한다.
	//		while (!arFnc[iCB]) //  N번째 함수가 NULL이면 점점 루트쪽에 가까운 함수를 쓴다.
	//			iCB--;
	//		ASSERT(iCB >= 0);// 적어도 하나는 있겠지.
	//		/// 람다함수 실행 ///
	//		arFnc[iCB](jv, nth);
	//		if (jArr.size() == 0)
	//			return;
	//		if (bRecursive)
	//		{
	//			if (keySub.size() > nth)//특정 키만 Array 또는 Object인지 체크
	//			{
	//				auto kSub1 = keySub[nth];
	//				ShJVal sjv;
	//				if (jRoot.Lookup(kSub1, sjv))
	//					CallRecursive(sjv);
	//			}
	//			else//모든 키 점
	//			{
	//				for (auto& [k, sjv] : jRoot)
	//					CallRecursive(sjv);
	//			}
	//		}
	//	}
	//}
	//catch (CString e)
	//{
	//	throw_str(e);
	//}
//}
void UcJObj::DeepKeyLoop(vector<jstring> keySub, initializer_list<function<void(UcJObj&, int)>> initCb, bool bRecursive)
{
	if (this->size() == 0)
		return;
	//vector<jstring> keySub(initSub);
	vector<function<void(UcJObj&, int)>> arFnc(initCb);// = {cbRoot(initSub);
	UcJObj::DeepLoopRecursive(*this, keySub, 0, arFnc, bRecursive);
#ifdef __sample__
	void CServerView::DeepKeyLoop(UcJObj & js, function<void(UcJObj&, int)> cbApp1
		, function<void(UcJObj&, int)> cbVer2
		, function<void(UcJObj&, int)> cbFile3)
	{
		js.DeepKeyLoop({ "vtVerList", "vtFiles" }, { cbApp1, cbVer2, cbFile3 });
	}
#endif // __sample__
}

void UcJArr::DeepArrayLoop(vector<jstring> keySub, initializer_list<function<void(UcJObj&, int)>> initCb, bool bRecursive)
{
	if (this->size() == 0)
		return;
	//vector<jstring> keySub(initSub);
	vector<function<void(UcJObj&, int)>> arFnc(initCb);// = {cbRoot(initSub);
	UcJObj::DeepLoopRecursive(*this, keySub, 0, arFnc, bRecursive);
}

//// n level JObj, 1 lambda
//void UcJObj::DeepKeyLoop(vector<jstring> keySub, function<void(UcJObj&, int)> initCb)
//{
//	DeepKeyLoop(keySub, { initCb });
//}
//// 1 level JObj, 1 lambda
//void UcJObj::DeepKeyLoop(function<void(UcJObj&, int)> initCb)
//{
//	DeepKeyLoop({}, { initCb });
//}


//};








/// /////////////////////////////////////////
/// JSON 저장. CArchive Serialize에서 쉽게 하는법 연구
/// C:\Dropbox\Proj\KProj\ThreadCopy\NoCrush2\NoCrush2.cpp
/// ///////////////////////////////////////////////////

#include <type_traits>

bool IsEnumType(const std_any & value) {
	return value.has_value() && std::is_enum<std::decay_t<decltype(value.type())>>::value;
}

template<typename TITEM>
void VectorToJSon(UcJObj & j1, wstring k2, const std_any & v3) {
	//auto& vti : 'initializing': cannot convert from 'std::vector<T,std::allocator<int>>' to 'std::vector<T,std::allocator<int>> &'
	auto vti = std_any_cast<std::vector<TITEM>>(v3);
	UcJArr jar;
	for (const TITEM& v : vti)
		jar.Add(v);
	j1(k2) = jar;
}

template<typename TITEM>
void MapToJSon(UcJObj & j1, wstring k2, const std_any & v3) {
	auto mpi = std_any_cast<std::map<wstring, TITEM>>(v3);
	UcJObj newJbj;
#if CPP17_OR_LATER
	for (auto& [key, val] : mpi) {
#else
	for (auto& pair : mpi) {
		auto& key = pair.first;
		auto& val = pair.second;
#endif
		newJbj(key) = val;
	}
	j1(k2) = newJbj;
	}


#if CPP17_OR_LATER
void AnyToJSon(UcJObj& jbj, wstring key, const std::any& aval)
{
	int iType = -1;
	static std::unordered_map<std::type_index, std::function<void(UcJObj&, wstring, const std::any&)>> handlers_ = {
		{	typeid(unsigned int), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 1; j1(k2) = std::any_cast<unsigned int>(v3); }},
		{	typeid(unsigned long), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 2; j1(k2) = std::any_cast<unsigned long>(v3); }},
		{	typeid(unsigned __int64), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 3; j1(k2) = std::any_cast<unsigned __int64>(v3); }},
		{	typeid(int), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 4; j1(k2) = std::any_cast<int>(v3); }} ,// unnamed-enum도 int
		{	typeid(short), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 4; j1(k2) = (int)std::any_cast<short>(v3); }},
		{	typeid(unsigned short), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 1; j1(k2) = (unsigned int)std::any_cast<unsigned short>(v3); }},
		{	typeid(USHORT), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 1; j1(k2) = (unsigned int)std::any_cast<USHORT>(v3); }},
		{	typeid(long), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 5; j1(k2) = (int)std::any_cast<long>(v3); }},
		{	typeid(__int64), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 6; j1(k2) = std::any_cast<__int64>(v3); }} ,
		{	typeid(UINT), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 7; j1(k2) = std::any_cast<UINT>(v3); }} ,
		{	typeid(float), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 8; j1(k2) = std::any_cast<float>(v3); }} ,
		{	typeid(double), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 9; j1(k2) = std::any_cast<double>(v3); }} ,
		{	typeid(bool), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 10; j1(k2) = std::any_cast<bool>(v3); }} ,
		{	typeid(CStringW), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 11; j1(k2) = std::any_cast<const CStringW&>(v3); }},
		{	typeid(wstring), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 12; j1(k2) = std::any_cast<const wstring&>(v3); }},
		{	typeid(UcJObj), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 13; j1(k2) = std::any_cast<const UcJObj&>(v3); }},
		{	typeid(std::vector<int>), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 14; VectorToJSon<int>(j1, k2, v3); }},
		{	typeid(std::vector<CStringW>), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 15; VectorToJSon<CStringW>(j1, k2, v3); }},
		{	typeid(std::vector<wstring>), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 16; VectorToJSon<wstring>(j1, k2, v3); }},
		{	typeid(std::map<wstring, wstring>), [&iType](UcJObj& j1, wstring k2, const std::any& v3) { iType = 16; MapToJSon<wstring>(j1, k2, v3); }},
	};

	//auto& theType = typeid(aval);
	auto& theType = aval.type();
	auto pName = theType.name();
	auto it = handlers_.find(theType);

	if (it != handlers_.end()) {
		it->second(jbj, key, aval);
		TRACE("%d.[%s] \"%s\"\n", iType, CStringA(key.c_str()).GetString(), pName);
	}
	else {
		if (aval.has_value() && std::is_enum<std::decay_t<decltype(aval.type())>>::value) {
			int enumValue = std::any_cast<int>(aval);//static_cast<int>(std::any_cast<Color>(myAny)); // ✅ int로 변환
			auto it1 = handlers_.find(typeid(int));
			ASSERT(it1 != handlers_.end());
			it1->second(jbj, key, aval);
		}
		TRACE("%d.[%s] \"%s\"\n", iType, CStringA(key.c_str()).GetString(), pName);
		ASSERT(0 == "no type for JSON");
	}
}
void PairToJSon(UcJObj & jbj, const std::pair<wstring, std::any>&pr)
{
	AnyToJSon(jbj, pr.first, pr.second);
}
/// <summary>
/// 자동으로 변수명을 키를 생성 하여 JSON으로 저장 한다.
/// </summary>
/// <param name="jbj"></param>
/// <param name="mapToStore"></param>
void SaveFieldsToJSON(UcJObj & jbj, std::map<wstring, std::any>&mapToStore)
{
	try
	{
		for (auto& [key, aval] : mapToStore) {
			AnyToJSon(jbj, key, aval);
		}
#ifdef _DEBUGx
		auto sJson = jbj.ToJsonStringW(3);
		TRACE(L"%s\n", sJson.GetString());
#endif // _DEBUG
	}
#ifdef _DEBUG
	catch (CException* e) {
		e;
		throw;  // ✅ `e`를 그대로 다시 던짐
	}
#endif // _DEBUG
	catch (...) {
		//CException* e = dynamic_cast<CException*>(_exception_info());  // 현재 예외 객체 확인
		throw;//그대로 다시 던짐
	}
}
#else
#include "UcJHandler.inl"

std::tuple<
	int, INT64, UINT64, WORD, BYTE, UINT, ULONG, CTime, COleDateTime, double,
	CStringW, CStringA, CStringArray, CStringList,
	//ShJVal, //JVal, UcJObj, UcJArr, //ONULL,
	std::wstring, std::string,
	std::vector<wstring>,
	std::vector<CStringW>,
	std::vector<CStringA>,
	std::vector<int>,
	std::vector<unsigned int>,
	std::vector<INT64>,
	std::vector<double>,
	std::vector<size_t>,
	std::vector<std::string>,
	std::vector<_variant_t>,
	std::list<wstring>,
	std::list<CStringW>,
	std::list<CStringA>,
	std::list<int>,
	std::list<INT64>,
	std::list<double>,
	std::list<size_t>,
	std::map<std::wstring, wstring>,
	std::map<std::wstring, CStringW>,
	std::map<std::wstring, CStringA>,
	std::map<std::wstring, int>,
	std::map<std::wstring, INT64>,
	std::map<std::wstring, double>,
	std::map<std::wstring, std::vector<std::wstring>>
	// CArray는 복사 생성자가 삭제되어 있어서 튜플에 넣을 수 없음. 별도로 체크 필요
	// CArray<int, int>,
	// CArray<double, double>,
	// CArray<CStringW, CStringW>,
	// CArray<CStringA, CStringA>
> s_typeSupported;

#endif //CPP17_OR_LATER

#ifdef _DEBUGx
void VariableToJSonTest()
{
	int nVersion{ 32 }, nCount{ 5 };
	CStringW strPassword{ L"1234" }, strProjectName{ L"Builder" };
	bool m_bWithShadow{ true };
	auto e1 = uea;
	EEEE e2 = ea;
	CStringW sMoreData{ L"moreData" };
	CStringArray arStr;//복사 생성자가 없으면 안된다.
	arStr.Add(L"abc");
	arStr.Add(L"def");
	arStr.Add(L"ghi");

	/// vector 도 넣기
	vector<wstring> vtStr;
	vtStr.push_back(L"vabc");
	vtStr.push_back(L"vdef");
	vtStr.push_back(L"vghi");

	/// 가변 여러개 넣기 : std::map<std::wstring, std::any>
	auto mapVars = VarsToJMap(e1, e2, vtStr, nVersion, nCount, strPassword, strProjectName, m_bWithShadow);

	/// 추가 변수 넣기
	VarToJMap(mapVars, sMoreData);

	std::map<wstring, wstring> mapStr;
	mapStr[L"key1"] = L"vabc1";
	mapStr[L"key2"] = L"vdef2";
	mapStr[L"key3"] = L"vghi3";

	/// 추가 변수 map 넣기
	VarToJMap(mapVars, mapStr);


	UcJObj jbj2;
#if CPP17_OR_LATER
	for (auto& [key, aval] : mapVars) {
#else
	for (auto& pair : mapVars) {
		auto& key = pair.first;
		auto& aval = pair.second;
#endif
		AnyToJSon(jbj2, key, aval);
	}


	auto sJson = jbj2.ToJsonStringW(3);
	TRACE("JSON ...\n");
	TRACE(L"%s\n", sJson.GetString());
	}
#endif // _DEBUGx


ShJObj UcJObj::MergeJsonObj(ShJObj parent, ShJObj child)
{
	// If both are null, return a new empty ShJObj
	if (!parent && !child) {
		return make_shared<UcJObj>();
	}

	// If only parent exists, return a copy of parent
	if (parent && !child) {
		return make_shared<UcJObj>(*parent->Dic(), true);
	}

	// If only child exists, return a copy of child
	if (!parent && child) {
		return make_shared<UcJObj>(*child->Dic(), true);
	}

	// Both exist, merge them
	ShJObj result = make_shared<UcJObj>(*child->Dic(), true); // Start with child as base

	// Add/override with parent values
#if CPP17_OR_LATER
	for (const auto& [key, value] : *parent->Dic()) {
#else
	for (const auto& pair : *parent->Dic()) {
		const auto& key = pair.first;
		const auto& value = pair.second;
#endif
		result->Dic()->SetAt(key, make_shared<JVal>(value, true));
	}

	return result;
	}

//SHP<UcJTable> UcJObj::Table(UcJObj* pbj) //dwk: 2026-03-25 15:10
SHP<UcJTable> UcJObj::Table(ShJVal shTbl2) //dwk: 2026-03-25 15:10
{
	if(!shTbl2)
	//if (pbj == nullptr)
		throw_str(L"pbj is null.");

	auto tb = NEWSHP(UcJTable);				//+pbj	[L"fields"]	{_type=eArr 
	tb->_tbl = shTbl2;// pbj;// 							//+		[L"rows"]	{_type=eArr 
														//-		[L"type"]	{_type=eStr L"table"
	auto tblObj = tb->_tbl->Dic();
	CStringW ty = tblObj->S(L"type", L"");
	if (ty != L"table")
		throw_str(L"UcJObj::Cell table type must be \"table\" (got \"%s\").", ty.GetString());
	tb->_fields = tblObj->A(L"fields", false);
	tb->_rows = tblObj->A(L"rows", false);
	if (!tb->_fields)
		throw_str(L"UcJObj::Cell table missing \"fields\" array.");
	if (!tb->_rows)
		throw_str(L"UcJObj::Cell table missing \"rows\" array.");
	{
		auto pFields = tb->_fields->Arr();
		if (!pFields)
			throw_str(L"UcJObj::Cell table fields is null.");
		for (int i = 0; i < (int)pFields->size(); ++i)
		{
			ShJVal sf = pFields->GetAt(i);
			if (!sf || !sf->IsVal())
				throw_str(L"UcJObj::Cell table fields[%d] is invalid.", i);
			CStringW colName;
			auto fv = sf->Val();
			if (fv->IsObject()) {
				auto fo = fv->AsObjPtr();
				colName = fo ? fo->S(L"name", L"") : CStringW{};
			}
			else {
				colName = fv->S(L"");
			}
			colName.Trim();
			if (colName.IsEmpty())
				throw_str(L"UcJObj::Cell table fields[%d] has empty name.", i);
			std::wstring key = colName.GetString();
			if (tb->_mFieldCol.find(key) != tb->_mFieldCol.end())
				throw_str(L"UcJObj::Cell table duplicate field name \"%s\".", key.c_str());
			tb->_mFieldCol[key] = i;
		}
	}
	return tb;
}

/*
/// 여러 개의 기본값을 한 번에 설정 (가변 매개변수)
template<typename... Args>
void SetDefaults(Args&&... args)
{
(SetIfNull(std::forward<Args>(args)), ...);
}

/// 타입별로 여러 기본값 설정 (더 안전한 방식)
void SetDefaults(const std::vector<std::pair<LPCSTR, double>>& doubles,
const std::vector<std::pair<LPCSTR, int>>& ints = {},
const std::vector<std::pair<LPCSTR, LPCWSTR>>& strings = {},
const std::vector<std::pair<LPCSTR, bool>>& bools = {})
{
for (const auto& [key, value] : doubles) SetIfNull(key, value);
for (const auto& [key, value] : ints) SetIfNull(key, value);
for (const auto& [key, value] : strings) SetIfNull(key, value);
for (const auto& [key, value] : bools) SetIfNull(key, value);
}

/// pair 배열 방식으로 여러 기본값 설정
template<typename TVAL>
void SetDefaultsFromPairs(const std::vector<std::pair<LPCSTR, TVAL>>& pairs)
{
for (const auto& [key, value] : pairs) {
SetIfNull(key, value);
}
}
*/

UCTOOLDYNAMIC
void UcJsonToData(UcJObj & jDocData, ShJObj & sjobj, bool bToJson)
{
	if (bToJson)//ar.IsStoring())
	{
		if (!sjobj)
			sjobj = std::make_shared<UcJObj>();///이거 때문에 파라미터를 ShJObj&
		sjobj->Dic()->Clone(&jDocData, true);
	}
	else
	{
		jDocData.Clone(sjobj, true);
		//예외적 처리는 호출한 쪽에서 한다.
		//		jDocData("_bRepeat") = 0;//파일 열면 초기화 하고, 연결되면 TRUE. 서버크러시떄 복구 위해 하는 거
	}
}

void UcJsonSave(UcJObj & jDocData, CFile & oFile, function<int(LPCWSTR, int)> cbChk, int preety)
{
	try {
		CStringA sUtf8 = jDocData.ToJsonStringUtf8(preety, cbChk);
		oFile.Write((LPCSTR)sUtf8, sUtf8.GetLength());
	}
	catch (CException*) {
		throw;
	}
}

/// <summary>
/// 열린 파일 객체로 json object로
/// </summary>
/// <param name="jDocData"></param>
/// <param name="oFile"></param>
/// <param name="cb"></param>
/// <returns></returns>
int UcJsonLoad(ShJBase & jDocData, CFile & oFile, function<int(int, int, LPCWSTR)> cb)
{
	try {
		CFile* fr = &oFile;//ar.GetFile();
		int len = (int)fr->GetLength();
		if (len == 0)
			throw_str(L"File(%s) length is 0.\n", oFile.GetFileName().GetString());
		CStringA sa;
		char* buf = sa.GetBufferSetLength(len + 1);
		if (fr->Read(buf, len) > 0) {
			int rv = UcJsonLoad(jDocData, buf, len, cb);
			return rv;
		}
		else
			CFileException::ThrowOsError((LONG)::GetLastError(), oFile.GetFileName());
	}
	catch (CException*) {
		throw;
	}
	return 0;
}

/// <summary>
/// utf8 data를 json obj로
/// </summary>
/// <param name="jDocData"></param>
/// <param name="psUtf8"></param>
/// <param name="len"></param>
/// <param name="cb"></param>
/// <returns></returns>
UCTOOLDYNAMIC
int UcJsonLoad(ShJBase& jDocData, LPCSTR psUtf8, DWORD len, function<int(int, int, LPCWSTR)> cb)
{
	CStringW sWstr;
	//try
	//{
	if (len == 0)
		throw_str(L"length is 0.\n");
	CStringA sa(psUtf8, len);
	UcUTF8ToWchar(sa, sWstr);

	int rv = UcJsonLoad(jDocData, sWstr, len, cb);
	return rv;

	//		auto jdoc = Json::Parse((LPCWSTR)sWstr, cb);
	//		if (!jdoc)//.get() == nullptr)
	//		{
	//			auto [fnc0, line1, msg2, str3] = Json::PopErr();
	//			CStringW sFnc(fnc0.c_str());
	//			CStringW sMsg(msg2.c_str());
	//			throw_str(L"File(JSON) parsing error. %s(%d) %s", sFnc.GetString(), line1, sMsg.GetString());//,  sWstr.Left(20));
	//		}
	//#ifdef _DEBUGx
	//		auto jd = jdoc->Dic();//+		jd	{ size=6 }	UcJObj *
	//#endif // _DEBUG
	//		jDocData = jdoc->Val()->AsObject();
		//}
		//catch (CException*)
		//{
		//	throw;
		//}
		//return 0;
}
int UcJsonLoad(ShJBase & jDocData, LPCWSTR sWstr, DWORD len, function<int(int, int, LPCWSTR)> cb)
{
	//CStringW sWstr;
	try
	{
		if (len == 0)
			throw_str(L"length is 0.\n");

		if (auto jdoc = UcJson::Parse((LPCWSTR)sWstr, cb)) {
			jDocData = jdoc->Val()->AsObject();
		}
		else {// 여기서 JException 이 KException으로 바뀐다.
			auto ex = UcJson::PopErr();
			CStringW sFnc(ex.sFunction.c_str());
			CStringW sMsg(ex.message.c_str());
			throw_str(L"File(JSON) parsing error. %s [JSON Line:%d, Column:%d]", sMsg.GetString(), ex.jsonLine, ex.jsonColumn);
			//throw_str(L"File(JSON) parsing error. %s(%d) %s [JSON Line:%d, Column:%d]", sFnc.GetString(), ex.line, sMsg.GetString(), ex.jsonLine, ex.jsonColumn);
		}
	}
	catch (CException*) {
		throw;
	}
	return 0;
}

UCTOOLDYNAMIC
int UcJsonLoad(ShJBase& jDocData, CString sFullJsonFile)
{
	try
	{  //sFull = L"C:\\Users\\keeps\\AppData\\Local\\ITCKR\\UcRoot\\IK_Patch_stats.json"


		if (!jDocData)// make_shared  해서 보내야지.
			jDocData = make_shared<UcJObj>();
		//throwLINE;
		if (UcIfFileExistEx(sFullJsonFile))//sFull = L"C:\\server\\update\\IK_GUID.json"
		{
			CFile fr;
			for (int i = 0; !fr.Open(sFullJsonFile, CFile::modeRead); ++i) {
				Sleep(100);
				if (i > 10)
					CFileException::ThrowOsError((LONG)::GetLastError(), sFullJsonFile);
			}

			KAtEnd defer([&fr]() {fr.Close(); });
			int rv = UcJsonLoad(jDocData, fr);
			if (rv != 0)
				throw_str(L"File(JSON) open error.(%s)", sFullJsonFile.GetString());
		}
		else {
			throw_str(L"File not found.(%s)", sFullJsonFile.GetString());
		}
	}
	catch (KException*) {
		//JException 이 KException 로 바뀌어 throw된다.
		throw;
	}
	catch (CException*) {
		//auto er = GetLastError();
		//auto ser = UcErrorToStrW(er);
		//UcMessageBoxError(L"Failed to load global settings!\n%s\n%s", ser, sGS);
		//return -1;
		//UcMessageRelese(L"%s(%d)", __FUNCTIONW__, __LINE__);
		throw;
	}
	return 0;
}

//[[deprecated]]
int UcJsonLoad(UcJObj & jDocData, CFile & oFile, function<int(int, int, LPCWSTR)> cb)
{
	ShJObj shjDoc;
	try {
		if (UcJsonLoad(shjDoc, oFile, cb) == 0) {
			jDocData.Clone(shjDoc->Dic(), true);
		}
	}
	catch (CException*) {
		throw;
	}
	return 0;
}
UCTOOLDYNAMIC
void UcJsonSave(UcJObj& jDocData, CString sFull, BOOL bBackup, int nDayExpire, int preety)
{
	ASSERT(sFull.GetLength() > 0);
	try
	{
		if (UcIfFileExistEx(sFull))
		{
			if (bBackup)
				UcBackupFile(sFull, nDayExpire);
		}
		UcCheckTargetDir(sFull, TRUE, FALSE);
		CFile fr;
		if (!fr.Open(sFull, CFile::modeCreate | CFile::modeWrite))
			CFileException::ThrowOsError((LONG)::GetLastError(), sFull);
		KAtEnd defer([&fr]() {fr.Close(); });
		UcJsonSave(jDocData, fr, NULL, preety);
	}
	catch (CException* e)
	{
		CString s; e->GetErrorMessage(s.GetBuffer(1024), 1024); s.ReleaseBuffer();
		//TRACE(L"%s\n", s);
		throw;
	}
}

void UcJsonSave(SHP<JBase> shDocData, CString sPath, BOOL bBackup, int nDayExpire, int preety)
{
	if (shDocData)
	{
		UcJObj& jDocData = *shDocData->Dic();
		UcJsonSave(jDocData, sPath, bBackup, nDayExpire, preety);
	}
}

//[[deprecated]]
//int UcJsonSerialize(UcJObj& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb, function<int(LPCWSTR, int)> cbChk)
//{
//	if (ar.IsStoring())//write
//		UcJsonSave(jDocData, *ar.GetFile(), cbChk);
//	else //read
//		return UcJsonLoad(jDocData, *ar.GetFile(), cb);
//	return 0;
//}
UCTOOLDYNAMIC
int UcJsonSerialize(UcJObj& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb, function<int(LPCWSTR, int)> cbChk)
{
	if (ar.IsStoring())//write
		UcJsonSave(jDocData, *ar.GetFile(), cbChk);
	else //read
		return UcJsonLoad(jDocData, *ar.GetFile(), cb);
	return 0;
}

#ifdef _DEBUGx // to go HttpClient
ShJBase UcGetRemoteJson(CStringW sUrl, int* piStatus, PWS sFunc, PWS sFile, int nLine)
{
	ShJBase jDoc;
	try
	{
		int iStatus = 0;//sUrl = L"https://update.cadian.com/update/LOCK.json"
		SHP<vector<BYTE>> bufRd = UcGetRemoteBuffer(sUrl, __FUNCTIONW__, &iStatus, sFunc, sFile, nLine);
		if (piStatus)
			*piStatus = iStatus;
		if (iStatus == HTTP_STATUS_OK)
		{
			auto pBuf = (LPCSTR)bufRd->data();
			auto szJson = (DWORD)bufRd->size();
			int rv = UcJsonLoad(jDoc, pBuf, szJson);
		}
		else
		{
			auto sStatus = UcHttpStatusStr(iStatus);
			no_throw_str(L"Certification process error: (%s)", CStringW(sStatus).GetString());//?CDANAL
		}
	}
	catch (CException*)
	{//+		m_strStateNativeOrigin	L"File(JSON) parsing error. Json::ExtractString(287) the string or brace ended incorrectly."	ATL::CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsCRT<wchar_t>>>
		TRACE(L"\n");
	}
	return jDoc;
}
#endif // _DEBUGx

void UcRegistryKeyValuesToJson(CRegKey & regKey, UcJObj & jbj)
{
	UcSaveRegistryKeyValuesLoop(regKey, [&jbj](int idx, LPCTSTR szValueName, BYTE* szData, DWORD dwType) -> int {
		switch (dwType)
		{
		case REG_SZ:
			jbj(szValueName) = (LPCTSTR)szData;
			//outFile << L"Data: " << (LPCTSTR)szData << std::endl;
			break;
		case REG_DWORD:
			jbj(szValueName) = *(DWORD*)szData;
			//outFile << L"Data: " << *(DWORD*)szData << std::endl;
			break;
		case REG_BINARY:
			ASSERT(0);
			//for (DWORD i = 0; i < dwDataSize; ++i)
			//	outFile << std::hex << (int)szData[i] << " ";
			return 1;
			break;
			// 다른 데이터 유형도 추가할 수 있음
		default:
			ASSERT(0);
			return 1;
			break;
		}
		return 0;
		});
}

/// 이름 없는 namespace {}: 완전히 숨김(내부 링크)
namespace {
	static const wchar_t* kItemFallbackTag = TAG_ITM;

	inline wchar_t* alloc_w(rpx::xml_document<wchar_t>& doc, const std::wstring& s) {
		return doc.allocate_string(s.c_str());
	}

	inline std::wstring to_wstring(JVal* v) {
		if (!v) return L"";
		switch (v->_type) {
		case VType::eStr: return v->AsString();
		case VType::eBol: return v->AsBool() ? L"true" : L"false";
		case VType::eFlt: {
			std::wstringstream ss; ss.precision(15); ss << v->AsDouble(); return ss.str();
		}
		case VType::eI64: {
			std::wstringstream ss; ss << v->AsInt64(); return ss.str();
		}
		case VType::eInt: {
			std::wstringstream ss; ss << v->AsInt(); return ss.str();
		}
		case VType::eNul: default: return L"";
		}
	}

	void xml_ValueNode(rpx::xml_document<wchar_t>& doc, rpx::xml_node<wchar_t>* parent, const std::wstring& key, const ShJVal& val);

	void xml_ChildObj(rpx::xml_document<wchar_t>& doc, rpx::xml_node<wchar_t>* elem, ShJObj obj)
	{
		DWKUSETRACE;// V(L"out:%v", out);
		if (!obj) return;
#if CPP17_OR_LATER
		for (auto& [k, sjv] : *obj->Dic()) {
#else
		for (auto& pair : *obj->Dic()) {
			auto& k = pair.first;
			auto& sjv = pair.second;
#endif
			//if (k == L"user")
			//	_break;
			// Attribute key: either flagged or key starts with '@' or '#'
			JVal* jv = sjv->IsVal() ? sjv->Val() : nullptr;
			bool isAttr = (jv && jv->_bAttr) || (!k.empty() && (k[0] == L'@'));// || k[0] == L'#'

			std::wstring name = k;
			//DWKTRACE(L"name: %v", k);
			if (!k.empty() && k[0] == L'@')
				name = k.substr(1);
			else if (!k.empty() && k[0] == L'#')
				name = L"_sys_" + k.substr(1);

			if (name == L"_text") {// Text content
				if (jv) {
					ASSERT(0);//"_text" 가 있나?
					std::wstring txt = to_wstring(jv);
					if (!txt.empty()) {
						auto escapedTxt = XmlEscape(txt);
						elem->value(alloc_w(doc, escapedTxt));
					}
				}
				continue;
			}
			if (isAttr) {
				if (jv) {
					std::wstring v = to_wstring(jv);
					if (!name.empty()) {
						auto escapedV = XmlEscape(v);
						elem->append_attribute(doc.allocate_attribute(alloc_w(doc, TagEncode(name)), alloc_w(doc, escapedV)));
					}
				}
				continue;
			}

			// Child element(s)
			if (sjv->IsArr()) {
				// For arrays, create a separate element with __type__="array" and add items inside
				auto arr = sjv->Arr();
				if (arr && arr->Arr()) {
					// Create a new element for the array
					auto* arrayElem = doc.allocate_node(rapidxml::node_element, alloc_w(doc, TagEncode(name)));

					// Add __type__="array" attribute to the array element
					arrayElem->append_attribute(doc.allocate_attribute(alloc_w(doc, TAG_TPY), alloc_w(doc, L"array")));

					/// Add each item to the array element
					for (auto& item : *arr->Arr()) {
						if (item) // item이 유효하기만 하면 진입 (IsDic, IsArr, IsVal 모두 처리)
							xml_ValueNode(doc, arrayElem, TAG_ITM, item);
					}
					// Add the array element to the parent
					elem->append_node(arrayElem);
				}
			}//IsArray
			else {//IsObj or etc
				xml_ValueNode(doc, elem, name, sjv);
			}
		}//for ShJObj obj
		}

	void xml_StringNode(std::wstring & out, rpx::xml_node<wchar_t>*node, int indent)
	{
		DWKUSETRACE;
		//DWKFUNCV(L"out:%v", out);
		if (!node) return;

		std::wstring indentStr(indent * 2, L' ');

		if (node->type() == rpx::node_declaration) {
#ifdef _DEBUGx
			wstring sName = node->name();//wchar*
			DWKTRACE(L"ND: %v", sName);//DWKUSETRACE;
#endif // _DEBUG
			// XML 선언부 처리
			out += L"<?" + std::wstring(node->name());
			if (node->value() && wcslen(node->value()) > 0)
				out += L" " + std::wstring(node->value());
			out += L"?>\n";
		}
		else if (node->type() == rpx::node_element)
		{
#ifdef _DEBUGx
			wstring sName = node->name();//wchar*
			DWKTRACE(L"NE: %v", sName);//DWKUSETRACE;
			if (sName == L"CXmlAppDoc._lstPbj")
				_break;
#endif // _DEBUG
			out += indentStr + L"<" + node->name();
			// 속성들 추가
			for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute())
				out += L" " + std::wstring(attr->name()) + L"=\"" + std::wstring(attr->value()) + L"\"";

			// 자식 노드가 있으면
			if (node->first_node()) {
				out += L">\n";
				for (auto* child = node->first_node(); child; child = child->next_sibling())
					xml_StringNode(out, child, indent + 1);/// RECURSIVE
				out += indentStr + L"</" + node->name() + L">\n";
			}
			else if (node->value() && wcslen(node->value()) > 0)// 텍스트 내용이 있으면
				out += L">" + std::wstring(node->value()) + L"</" + node->name() + L">\n";
			else// 빈 요소
				out += L"/>\n";
		}
	}

	void xml_ValueNode(rpx::xml_document<wchar_t>&doc, rpx::xml_node<wchar_t>*parent, const std::wstring & key, const ShJVal & val)
	{
		DWKUSETRACE;
		//if (key == L"user")
		//	_break;
		if (!val) // val이 null이면 리턴
			return;

#ifdef _DEBUGx		// Debug: check item type
		std::wstring itemType;
		if (val->IsDic())
			itemType = L"object";
		else if (val->IsArr())
			itemType = L"array";
		else if (val->IsVal())
			itemType = L"scalar";
		else
			itemType = L"unknown";
		DWKTRACE(L"key:%v [itemType:%v]", key, itemType);
#endif
		// Debug: check if this is a user array item
		//if (key == L"user" && val->IsArr()){
		//	DWKTRACE(L"ERROR: user array item is recognized as array, not object!", 1);
		//	// Check if rootVal is Dic or Arr
		//	if (val->IsDic())
		//		DWKTRACE(L"val->IsDic() = true", 1);
		//	if (val->IsArr())
		//		DWKTRACE(L"val->IsArr() = true", 1);
		//	if (val->IsVal())
		//		DWKTRACE(L"val->IsVal() = true", 1);
		//}
		// Create element
		std::wstring tag = key.empty() ? kItemFallbackTag : key;
		rpx::xml_node<wchar_t>* elem = doc.allocate_node(rpx::node_element, alloc_w(doc, TagEncode(tag)));
		parent->append_node(elem);

		// Check if this is an array first (arrays are not objects)
		if (val->IsArr()) {
			// Array at this level: add __type__="array" attribute and create repeated children
			auto arr = val->Arr();
			if (arr) {//&& arr->Arr())
				// Add __type__="array" attribute to indicate this is an array (only if not already present)
				if (!elem->first_attribute(TAG_TPY))
					elem->append_attribute(doc.allocate_attribute(alloc_w(doc, TAG_TPY), alloc_w(doc, L"array")));

#ifdef _DEBUGx
				// Debug: check array size
				size_t arrSize = arr->Arr()->size();
				// Add debug attribute to show array size
				elem->append_attribute(doc.allocate_attribute(alloc_w(doc, L"_debug_size"), alloc_w(doc, std::to_wstring(arrSize))));
#endif // _DEBUG

				// Debug: iterate through array items
				int itemCount = 0;
				for (auto& item : *arr->Arr())
				{
					itemCount++;
					if (!item) {// Debug: check if item is valid
						DWKTRACE(L"ERROR: arrayItem[%d] is null!", itemCount);
						continue;
					}
					auto bv = item->IsVal();
					auto bo = item->IsDic();
#ifdef _DEBUG
					auto ba = item->IsArr();
#endif // _DEBUG
					if (bo)
						_break;

					// Debug: check item type
#ifdef _DEBUGx
					std::wstring itemType;
					if (item->IsDic())
						itemType = L"object";
					else if (item->IsArr())
						itemType = L"array";
					else if (item->IsVal())
						itemType = L"scalar";
					else
						itemType = L"unknown";
					DWKTRACE(L"arrayItem[%d] itemType:%v", itemCount, itemType);
					// This will be added to the parent element, not the item
					elem->append_attribute(doc.allocate_attribute(alloc_w(doc, L"_debug_item" + std::to_wstring(itemCount)), alloc_w(doc, itemType)));
#endif // _DEBUG
					// For array items, use "__r_" as tag name for each item
					// This will recursively call xml_ValueNode, which will handle objects correctly
					// item이 유효하기만 하면 진입 (IsDic, IsArr, IsVal 모두 처리)
					xml_ValueNode(doc, elem, TAG_ITM, item);
				}
				// Add debug attribute to show how many items were processed
#ifdef _DEBUGx
				elem->append_attribute(doc.allocate_attribute(alloc_w(doc, L"_debug_processed"), alloc_w(doc, std::to_wstring(itemCount))));
#endif // _DEBUGx
			}
		}//if (IsArray
		else if (val->IsDic()) {
			// Check if this is a dictionary/object (after checking array)
			// This handles the case where array items are objects (Dic arrays) when called recursively
			// Debug: add attribute to show this is an object
#ifdef _DEBUGx
			elem->append_attribute(doc.allocate_attribute(alloc_w(doc, L"_debug_type"), alloc_w(doc, L"object")));
#endif // _DEBUGx

			xml_ChildObj(doc, elem, val);
		}
		else if (val->IsVal())
		{
			// Scalar value becomes text (XML escape: <None> 등 특수문자 보존)
			JVal* jv = val->Val();
			if (jv) {
				std::wstring txt = to_wstring(jv);
				if (!txt.empty()) elem->value(alloc_w(doc, XmlEscape(txt)));
			}
		}
	}
	}

// UTF-16 XML 문자열 생성
wstring UcJson::StringifyXml(const ShJVal value)
{
	DWKFUNC;
	rpx::xml_document<wchar_t> doc;
	if (!value)
		return L"";

	// Add XML declaration
	doc.append_node(doc.allocate_node(rpx::node_declaration, L"xml", LR"(version="1.0" encoding="UTF-8")"));


	JVal* v = value->Val();
	if (v && v->IsObject())
	{
		// Find root element (_bAttr = false) and attributes (_bAttr = true)
		auto dict = value->Dic();
		if (dict && dict->Dic()->size() > 0)
		{
			rpx::xml_node<wchar_t>* root = nullptr;

			// First pass: find root element (_bAttr = false)
			for (auto it = dict->Dic()->begin(); it != dict->Dic()->end(); ++it)
			{
				const std::wstring& rootName = it->first;
				DWKTRACE(L"rootName: %v", rootName);//DWKUSETRACE;
				ShJVal rootVal = it->second;
				JVal* rv = rootVal ? rootVal->Val() : nullptr;
				if (rv && !rv->_bAttr)
				{
					// This is the root element
					root = doc.allocate_node(rpx::node_element, alloc_w(doc, TagEncode(rootName))); ///?루트 1 사용자 루트
					doc.append_node(root);///?루트 1.1

					// Append the corresponding subtree under the created root node
					//JVal* rv = jv;
					if (rv)
					{
						if (rv->IsObject()) {
							xml_ChildObj(doc, root, rootVal);
						}
						else if (rv->IsArray()) {
							ASSERT(0);//루트 부터 배열이 있나?
							auto arr = rootVal->Arr();
							if (arr && arr->Arr()) {
								// If array under root key, repeat child elements with fallback tag
								for (auto& item : *arr->Arr())
									xml_ValueNode(doc, root, kItemFallbackTag, item);
							}
						}
						else {
							ASSERT(0);//루트 부터 값만있다고?
							std::wstring txt = to_wstring(rv);
							if (!txt.empty())
								root->value(alloc_w(doc, XmlEscape(txt)));
						}
					}
					break; // Found root element, exit loop
				}
			}

			// Second pass: process attributes (_bAttr = true)
			if (root)
			{
				for (auto it = dict->Dic()->begin(); it != dict->Dic()->end(); ++it)
				{
					ShJVal attrVal = it->second;
					JVal* av = attrVal ? attrVal->Val() : nullptr;
					if (av && av->_bAttr)
					{
						// This is an attribute
						std::wstring attrName = it->first;
						std::wstring attrValue = to_wstring(av);
						root->append_attribute(doc.allocate_attribute(alloc_w(doc, TagEncode(attrName)), alloc_w(doc, XmlEscape(attrValue))));
					}
				}
			}
			else
			{
				// No root element found, create a default root and process all as children
				rpx::xml_node<wchar_t>* defaultRoot = doc.allocate_node(rpx::node_element, L"root");///?루트 1 root
				doc.append_node(defaultRoot);///?루트 1.1
				xml_ChildObj(doc, defaultRoot, value);
			}
		}
		else
		{
			// Empty object => empty doc
		}
	}
	else
	{
		// Non-object root: append directly without wrapping
		xml_ValueNode(doc, &doc, kItemFallbackTag, value);

		// 기존 코드 (중복 root 문제로 주석 처리)
		// Non-object root: wrap in a generic <root>
		///	rpx::xml_node<wchar_t>* root = doc.allocate_node(rpx::node_element, L"root");
		///	doc.append_node(root);
		///	xml_ValueNode(doc, root, kItemFallbackTag, value);
	}

	// rapidxml print 함수 대신 수동으로 XML 문자열 생성
	std::wstring out;
	out.reserve(10240);
	//rpx::print(std::back_inserter(out), doc, 0);  
	//out += L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";	
  // XML 선언부부터 모든 노드 출력
	auto* node = doc.first_node();
	while (node) {
		xml_StringNode(out, node, 0);
		node = node->next_sibling();
	}

	return out;
}

// UTF-16 XML 문자열을 UTF-8로 변환
std::string UcJson::StringifyXmlUtf8(const ShJVal value)
{
	std::wstring xmlW = StringifyXml(value);// UTF-16 XML 문자열 생성
	// UTF-16을 UTF-8로 변환
	std::string xmlUtf8;
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, xmlW.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8Len > 0) {
		xmlUtf8.resize(utf8Len - 1); // null terminator 제외
		WideCharToMultiByte(CP_UTF8, 0, xmlW.c_str(), -1, &xmlUtf8[0], utf8Len, NULL, NULL);
	}
#ifdef _DEBUG
	CStringW swXml;
	UcUTF8ToWchar(xmlUtf8.c_str(), swXml);
	ASSERT(swXml == xmlW.c_str());
#endif // _DEBUG
	return xmlUtf8;
}

// UTF-8 XML 문자열을 파일로 저장
bool UcJson::SaveXmlToFile(const ShJVal value, const std::wstring& filePath)
{
	try {
		// UTF-8 XML 문자열 생성
		std::string xmlUtf8 = StringifyXmlUtf8(value);

		// 파일로 저장
		std::ofstream file(filePath, std::ios::binary);
		if (file.is_open()) {
			file.write(xmlUtf8.c_str(), xmlUtf8.length());
			file.close();
			return true;
		}
	}
	catch(std::exception e){
		ASSERT(0);
		TRACE("%s\r\n", e.what());
	}
	catch (...) {
		ASSERT(0);	// 파일 저장 실패
	}
	return false;
}


ShJBase UcJTable::RowObj(size_t row)
{
	auto shObj = NEWSHP(UcJObj);
	if (this->RowSize() <= row)
		throw_str(L"DB RowSize is less than row:%v.", row);

	// _mFieldCol : vector<pair<fieldName, colIndex>>
	for (const auto& fc : _mFieldCol) {
		const auto& fieldName = fc.first;
		const size_t colIdx = fc.second;
		auto sRow = GetCell(colIdx, row);//ShJBase
		//JVal* pVal = Cell(colIdx, row);
		if (sRow) {
			shObj->Set(fieldName, sRow);// std::make_shared<JVal>(*pVal));
		}
	}//                      *shObj, false  주면 안됨 true 줘야 해
	auto shVal = NEWSHP(JVal, shObj, false);//no copy, just share the pointer to the same object.
	return shVal;
}

//dwk: 2025-12-01 16:52 
//dwk: 2025-12-09 17:18 map val, 2D vector
//dwk: 2025-12-10 13:10 UcJXBase.h 제거
//dwk: 2025-12-18 12:44  vect<std::string>
//dwk: 2025-12-19 16:03 CStringArray 추가 체크 리스트에만 누락
//dwk: 2025-12-19 18:05 공백문자열을 항목을 무시하는 경우 수정
//dwk: 2026-04-17 11:11 UcJTable::RowObj
