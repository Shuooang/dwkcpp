#pragma once
#include "UcJXBase.h"//#DocSerialize
#include "UcJson.h"

template<class V>
inline std::wstring assign_wstring(const V& v)
{
	return JVal(v).AsString();
}
inline std::wstring ArcClassName(const char* funcName)
{
	std::string s(funcName);

	// 1) 람다 제거: ":: <lambda" 앞 부분만 사용
	size_t lambdaPos = s.find("::<lambda");
	if (lambdaPos != std::string::npos) {
		s = s.substr(0, lambdaPos);
	}

	// 2) operator 제거
	size_t opPos = s.find("::operator");
	if (opPos != std::string::npos) {
		s = s.substr(0, opPos);
	}

	// 3) "::" 구분자로 split
	std::vector<std::string> tokens;
	{
		size_t start = 0;
		while (true) {
			size_t pos = s.find("::", start);
			if (pos == std::string::npos) {
				tokens.push_back(s.substr(start));
				break;
			}
			tokens.push_back(s.substr(start, pos - start));
			start = pos + 2;
		}
	}

	// 토큰이 2개 이상이어야 class + function 구성됨
	if (tokens.size() < 2)
		return L"";

	// 마지막 항목은 function 이름
	// function 앞의 항목이 클래스명
	std::string className = tokens[tokens.size() - 2];

	// UTF-8 아닌 경우가 대부분이므로 그냥 wchar 변환
	return std::wstring(className.begin(), className.end()) + L".";
}
inline std::wstring ArcVarName(const wchar_t* varName) {
	CStringW vn(varName);
	vn.Replace(L"->", L".:");
	return vn.GetString();
}
inline std::wstring ArcClassKey(const char* funcName, const wchar_t* varName) {
	wstring clk = ArcClassName(funcName) + ArcVarName(varName);
	//DWKFUNCV(L"%v", clk);
	return clk;
}
#include <type_traits>
//==========================
//  키 변환 헬퍼 (C++14용)
//==========================
// TAG_ITM, JCOL_TAG는 UcJson.h에 정의되어 있음
// int → wstring
template <typename TSrc, typename TDst>
typename std::enable_if<std::is_same<TSrc, int>::value&& std::is_same<TDst, std::wstring>::value, TDst>::type
ConvertKey(const TSrc& src)
{
	return std::to_wstring(src);
}
// wstring → int
template <typename TSrc, typename TDst>
typename std::enable_if<std::is_same<TSrc, std::wstring>::value&& std::is_same<TDst, int>::value, TDst>::type
ConvertKey(const TSrc& src)
{
	return std::stoi(src);
}
// 그 외: 기본 변환(static_cast)
template <typename TSrc, typename TDst>
typename std::enable_if<!((std::is_same<TSrc, int>::value&& std::is_same<TDst, std::wstring>::value) ||
	(std::is_same<TSrc, std::wstring>::value && std::is_same<TDst, int>::value)), TDst>::type
	ConvertKey(const TSrc& src)
{
	return static_cast<TDst>(src);
}
//  Forward 변환 함수
template <typename TMapDst, typename TMapSrc>
TMapDst ConvertKeyType(const TMapSrc& src)
{
	TMapDst dst;
	for (auto& kv : src)
	{
		typename TMapDst::key_type newKey =
			ConvertKey<typename TMapSrc::key_type, typename TMapDst::key_type>(kv.first);
		dst[newKey] = kv.second;
	}
	return dst;
}
//  Reverse 변환 함수
template <typename TMapDst, typename TMapSrc>
void ConvertKeyTypeReverse(TMapDst& dst, const TMapSrc& src)
{
	for (auto& kv : src)
	{
		typename TMapDst::key_type newKey =
			ConvertKey<typename TMapSrc::key_type, typename TMapDst::key_type>(kv.first);
		dst[newKey] = kv.second;
	}
}
template <typename TObj>
void ConvertMapIntToWordPtr(const CMap<int, int, TObj*, TObj*>& src, CMapWordToPtr& dst)
{
	dst.RemoveAll();
	POSITION pos = src.GetStartPosition();
	while (pos)
	{
		int key;
		TObj* pVal = nullptr;
		src.GetNextAssoc(pos, key, pVal);
		// WORD는 0~65535 이므로, 범위 확인
		ASSERT(key >= 0 && key <= 0xFFFF);
		dst.SetAt(static_cast<WORD>(key), pVal);
	}
}
template <typename TObj>
void ConvertMapWordPtrToInt(const CMapWordToPtr& src, CMap<int, int, TObj*, TObj*>& dst)
{
	dst.RemoveAll();
	POSITION pos = src.GetStartPosition();
	while (pos)
	{
		WORD key;
		void* pVal = nullptr;
		src.GetNextAssoc(pos, key, pVal);
		dst.SetAt(static_cast<int>(key), reinterpret_cast<TObj*>(pVal));
	}
}

///?매우 중요: GetClassFactory()는 static으로 하면 모든 DLL마다 하나씩 생겨 버리므로, 
///	모든 모듈에 공유 되는 DLL 프로젝트에 AFX_EXT_CLASS하여 위치 해야 한다.
/// 만약 EXE단독 이면 EXE에 두면 된다.
/// 이 함수는 DLL 프로젝트에 있어야 하며 전 프로젝트에서 dllimport 되어 쓸수 있어야 한다.
/// 구현은 UcJXDoc.cpp 한 곳.
UCTOOLDYNAMIC 
std::shared_ptr<std::map<std::wstring, std::function<void* ()>>> 
		GetClassFactory();
///#include<map>
///#include<memory>
///#include<functional>
///AFX_EXT_CLASS std::shared_ptr<std::map<std::wstring, std::function<void* ()>>> GetClassFactory()//#DocSerialize struct factory
///{
///	static std::shared_ptr<std::map<std::wstring, std::function<void* ()>>> _mapFactory;//이거 싱글톤으로 해야 하고
///	if (!_mapFactory)
///		_mapFactory = std::make_shared<std::map<std::wstring, std::function<void* ()>>>();
///	return _mapFactory;
///}
template<typename T>
void* CreateClassFromName(std::wstring sClassName) {//#DocSerialize struct factory
	auto& _mapFactory = *GetClassFactory();
	auto it = _mapFactory.find(sClassName);
	if (it != _mapFactory.end()) {
		auto func = it->second;
		return (T*)func();
	}
	return nullptr;// new T();
}

/// ///////////////////////////////////////////////////////////////////////////
/// DocSerialize 선언 매크로: virtual void DocSerialize(CArchive& ard); + static std::wstring _class_; + virtual GetMyClassName()
#include "UcJXDeclare.inl"
///#ifndef DECLARE_ClassName_DocSerialize
///#define DECLARE_ClassName_DocSerialize() 
	//virtual void DocSerialize(CArchive& ard); 
	//virtual std::wstring GetMyClassName();
///#endif

class CSetFactory_Helper//#DocSerialize struct factory 1
{
public:
	CSetFactory_Helper(LPCWSTR sClass, function<void* ()> cbFactory) {
		auto& mapFactory = *GetClassFactory();
		auto it = mapFactory.find(sClass);
		ASSERT(it == mapFactory.end() || wcscmp(sClass, L"Matrix_Temp") == 0);
		if ((it == mapFactory.end()))
			mapFactory[sClass] = cbFactory;
	}
};


#define CLASS_EXPORT //AFX_EXT_DATA


#define SET_FACTORY(TClass) \
	const wchar_t* TClass::GetMyClassName(){ return L#TClass;}\
	CSetFactory_Helper g_##TClass(L#TClass, []() { return new TClass; })

//[deprecated]
//#define SET_FACTORY_NOEXPORT(TClass) \
//    CSetFactory_Helper g_##TClass(L#TClass, []() { return new TClass; })

#define SET_FACTORY_SUB(TNSpace, TClass) \
	const wchar_t* TNSpace::TClass::GetMyClassName(){ return L#TNSpace L"::" L#TClass;}\
	CSetFactory_Helper g_##TNSpace##_##TClass(L#TNSpace L"::" L#TClass, []() { return new TNSpace::TClass(); })

// sub 안에 또 sub가 있겠나?
#define SET_FACTORY_SUB2(TNSpace1, TNSpace2, TClass) \
	const wchar_t* TNSpace1::TNSpace2::TClass::GetMyClassName(){ return L#TNSpace1 L"::" L#TNSpace2 L"::" L#TClass;}\
	CSetFactory_Helper g_##TNSpace1##_##TNSpace2##_##TClass(L#TNSpace1 L"::" L#TNSpace2 L"::" L#TClass, []() { return new TNSpace1::TNSpace2::TClass(); })

//#define SET_FACTORY(TClass) \
//	CLASS_EXPORT std::wstring TClass::_class_ = L#TClass; \
//    CSetFactory_Helper g_##TClass(L#TClass, []() { return new TClass; })
//#define SET_FACTORY_NOEXPORT(TClass) \
//	std::wstring TClass::_class_ = L#TClass; \
//    CSetFactory_Helper g_##TClass(L#TClass, []() { return new TClass; })
//#define SET_FACTORY_SUB(TNSpace, TClass) \
//	CLASS_EXPORT std::wstring TNSpace::TClass::_class_ = L#TNSpace L"::" L#TClass; \
//	CSetFactory_Helper g_##TNSpace##_##TClass(L#TNSpace L"::" L#TClass, []() { return new TNSpace::TClass(); })
//#define SET_FACTORY_SUB2(TNSpace1, TNSpace2, TClass) \
//	CLASS_EXPORT std::wstring TNSpace1::TNSpace2::TClass::_class_ = L#TNSpace1 L"::" L#TNSpace2 L"::" L#TClass; \
//	CSetFactory_Helper g_##TNSpace1##_##TNSpace2##_##TClass(L#TNSpace1 L"::" L#TNSpace2 L"::" L#TClass, []() { return new TNSpace1::TNSpace2::TClass(); })

//SET_FACTORY(COptMap);//#DocSerialize struct factory
//SET_FACTORY(COptionElem);//#DocSerialize struct factory
//Arc_NoObj(_eoptElem);//#DocSerialize struct factory
//Arc_PtrNoObj(_eoptPtrElem);//#DocSerialize struct factory
//Arc_StdVectRefNoObj(opt_vector);
//Arc_StdVectPtrNoObj(opt_vector_ptr);

#define IMPLEMENT_DUMY_SERIALIZE(TCLASS, TBASE) \
void TCLASS::Serialize(CArchive &ar){\
	TBASE::Serialize(ar);\
	return_If_Doc_Call_DocSerialize(ar);\
}\
void TCLASS::DocSerialize(CArchive& ard){\
	ar_from_ard_ReadyForDocSerialize(ard);\
}

#define IMPLEMENT_DUMY_SERIALIZE1(TCLASS) \
void TCLASS::Serialize(CArchive &ar){\
	return_If_Doc_Call_DocSerialize(ar);\
}\
void TCLASS::DocSerialize(CArchive& ard){\
	ar_from_ard_ReadyForDocSerialize(ard);\
}

//[deprecated] is_shared_ptr trait for C++14 compatibility
// 목적: C++14에서는 std::is_shared_ptr가 없어서 직접 구현했던 타입 트레이트
//       - std::vector<std::vector<shared_ptr<TObj>>> 타입을 체크하기 위해 사용
//       - StdVect2D_ShpObj_1, StdVect2D_RefObj_1 함수에서 static_assert로 타입 검증에 사용
//       - 현재는 사용하지 않지만, 향후 필요할 수 있어 주석 처리로 보존
/*
namespace CJXArchiveDetail {
	template<typename T>
	struct is_shared_ptr_impl : std::false_type {};

	template<typename T>
	struct is_shared_ptr_impl<std::shared_ptr<T>> : std::true_type {};

	// SHP<T> (alias for std::shared_ptr<T>)도 지원
	template<typename T>
	struct is_shared_ptr : is_shared_ptr_impl<typename std::decay<T>::type> {};
}
*/


#define BASECJXArchive CArchive
//[deprecated]
//class IJXArchive
//{
//public:
//	virtual CStringA GetDataString() = 0;
//	virtual void SetDataFromString(const CStringA& dataStr) = 0;
//	virtual void ParseString(const CStringA& dataStr) = 0;
//};
////CExArchive
class UCTOOLDYNAMIC CJXArchive : public BASECJXArchive//CExArchive//dwk: 2025-02-12 09:50 JSON 1
	//, IJXArchive
{
public:
	//DECLARE_DYNAMIC(CJXArchive) // CArchive가 CObject 아님.
	//static const UINT MAGIC = 0x4A584152; // 'JXAR'
	//UINT m_magic = MAGIC;
	enum {
		eDocMode = 1 << 8   // 256
	};
	/// CExArchive에 있던거
	bool IsRoot() { return _pArUp == nullptr; }
	std::wstring _key;
protected:
	CArchive* _pArUp{ nullptr };//CExArchive* 
	int _curLevel{ 0 };
public:
	ShJObj _jData;
	bool _bBOM{ true };  // BOM 사용 여부 플래그
	/// <summary>
	/// 저장 전에 호출 되는 함수
	/// 람다 함수 매개 변수인 입력값은 _jData 로서 저장할 json 객체이며 
	/// 리턴 값은 입력값을 변형 한 후 실제로 저장할 json 객체이다.
	/// 주로 <!xml> 같은 헤더를 추가 하거나 암호화 등을 위해 사용 된다.
	/// </summary>
	std::function<ShJVal(ShJVal)> _beforeSave;
	std::function<ShJVal(ShJVal)> _afterLoad;
	std::string _sMode;
	bool IsDocMode() {
		auto be = (m_nMode & eDocMode) != 0;// CArchive::m_nMode protected
		///?주의: be가 false일때 this가 CArchive일수 있으니 _sMode에 접근 하면 큰일나
		return be ? _sMode.length() > 0 : false;
	}

	///디버깅용 변수. 스택이 얼마나 깊이 들어 가는지 기록
	shared_ptr<int> _statck;

	/// 예외 처리 헬퍼 함수들 (cpp로 분리, CallDocSerializeWithExceptionHandling 템플릿 함수에서만 사용)
	static void HandleCExceptionInDocSerialize(CException* e, shared_ptr<int>& statck, const char* pszFile, int nLine, const CStringA& saf);
	static void HandleStdExceptionInDocSerialize(const std::exception& e, shared_ptr<int>& statck, const char* pszFile, int nLine, const CStringA& saf);
	static void HandleUnknownExceptionInDocSerialize(shared_ptr<int>& statck, const char* pszFile, int nLine, const CStringA& saf);

	CJXArchive(CFile* pFile, const std::string sMode, UINT nMode, int nBufSize = 4096, void* lpBuf = NULL)
		: BASECJXArchive(pFile, nMode, nBufSize, lpBuf)
	{
		m_nMode |= eDocMode;// 2 -> 258
		_sMode = sMode;
		ASSERT(_curLevel == 0);
		_key = L"Document";
		if (!_statck)
			_statck = make_shared<int>(0);
	}

	CJXArchive(CJXArchive& arUp, LPCWSTR key = nullptr)
		: BASECJXArchive(arUp.m_pFile, arUp.m_nMode, arUp.m_nBufSize, arUp.m_lpBufStart) {
		ASSERT((m_nMode & eDocMode) != 0);
		ASSERT((arUp.m_nMode & eDocMode) != 0);
		_pArUp = &arUp;
		_sMode = arUp._sMode;
		_statck = arUp._statck;
		_curLevel = arUp._curLevel + 1; //이미 증가된 값으로 시작
		if (key) {
			_key = key;/// Serialize 중 첫번째 서브 Archive 일 때 key가 온다.
			auto& jup = ((CJXArchive*)_pArUp)->Jbj();
			_jData = jup.Get(_key);
		}
	}
	CJXArchive(CJXArchive& arUp, ShJObj jData)
		: BASECJXArchive(arUp.m_pFile, arUp.m_nMode, arUp.m_nBufSize, arUp.m_lpBufStart) {
		//_sClassDot = ArcClassName(__FUNCTION__);
		ASSERT((m_nMode & eDocMode) != 0);
		ASSERT((arUp.m_nMode & eDocMode) != 0);
		_pArUp = &arUp;
		_sMode = arUp._sMode;
		_statck = arUp._statck;
		_curLevel = arUp._curLevel + 1; //이미 증가된 값으로 시작
		_jData = jData;//SetJson
	}
	// CExArchive의 가상 함수들 구현
	virtual CStringA GetDataString() {
		ShJVal shToSave;
		if (_sMode == "JSON") {
			if (_beforeSave)
				shToSave = _beforeSave(GetJson());
			else
				shToSave = GetJson();
			return CJXArchive::JsonString(shToSave);
		}
		else if (_sMode == "XML") {
			if (_beforeSave)
				shToSave = _beforeSave(GetJson());//root element를 사용자정의로 할 경우
			else {
				///XML 문서는 반드시 하나의 루트 요소(root element) 를 가져야 합니다
				shToSave = make_shared<UcJObj>();
				auto& jbjRt = *shToSave->Dic();/// xml 의 <맨위 root
				jbjRt.Set("root", make_shared<JVal>(this->GetJson(), false));//clone 방지
			}
			return XmlString(shToSave);
		}
		else {
			ASSERT(0);
		}
		return {};
	}
	virtual void SetDataFromString(const CStringA& jsonStr) {
		if (_sMode == "JSON")
		{
			auto root = UcJson::ParseUtf8(jsonStr);
			if (root)
				SetJson(root);
		}
		else {
			ASSERT(0);
		}
	}
	/// jsonStr은 항상 UTF-8 인코딩이다.
	virtual void ParseString(const CStringA& jsonStr) {
		if (_sMode == "JSON")
		{
			auto root = UcJson::ParseUtf8(jsonStr);
			if (root)
				SetJson(root);
		}
		else {
			ASSERT(0);
		}
	}
	ShJObj GetJson() {
		if (!_jData)
			_jData = NEWSHP(UcJObj);
		return _jData;
	}
	void SetJson(ShJObj shJbj) {
		_jData = shJbj;
	}
	UcJObj& Jbj() {
		return *GetJson()->Dic();
	}
	static CStringA JsonString(ShJVal shDoc) {
		ASSERT(shDoc->IsDic());
		return shDoc->Dic()->ToJsonStringUtf8(3);
	}
	CStringA XmlString(ShJVal shDoc) {
		ASSERT(shDoc->IsDic());
		ASSERT(Jbj().size() > 0);
		return UcJson::StringifyXmlUtf8(shDoc).c_str();
	}


	static SHP<CArchive> CreateArchive(CFile* pFile, UINT bLoad, std::string sMode)
	{
		SHP<CArchive> sha;
		if (sMode.length() == 0)//CArchive ar(&file, CArchive::load);
			sha = NEWSHP(CArchive, pFile, CArchive::load);
		else
			sha = NEWSHP(CJXArchive, pFile, sMode, bLoad | CArchive::bNoFlushOnDelete);
		return std::static_pointer_cast<CArchive>(sha);
	}

	/// <summary>
	/// pXbj 객체의 필드에서 JSON 또는 XML 문자열로 바꾼다. 저장 또는 클립보드, 또는 네트워크 전송을 위해
	/// </summary>
	/// <param name="pXbj">IDocSerialize*</param>
	/// <param name="sDocMode">"XML" 또는 "JSON"</param>
	/// <param name="sRoot">최상위 키를 줘야 그 값의 UcJbj 을 가져온다. 기본은 "root" XML과 JSON 호환을 위해 루트를 반드시 둬야 함</param>
	/// <returns>UTF8 JSON 문자열</returns>
	static CStringA GetDocSerializedString(IDocSerialize* pXbj, std::string sDocMode = "JSON", std::string sRoot = "root")
	{
		CMemFile memFile;
		SHP<CArchive> shSaveArchive = CreateArchive(&memFile, CArchive::store, sDocMode); // #JSerialize CJXArchive
		auto& saveArchive = (CJXArchive&)*shSaveArchive;
		try {
			pXbj->Serialize(saveArchive);     // save me
			saveArchive.Close();
		} catch (CException* ) {
			ASSERT(0);
		}
		saveArchive._beforeSave = [sRoot](ShJVal shData) -> ShJVal {
			ShJVal valRt = make_shared<UcJObj>();//굳이 shared_ptr로 한 이유는 복사를 피하기 위해서다.
			auto& jbjRt = *valRt->Dic();/// xml 의 <맨위 root
			jbjRt.Set(sRoot.c_str(), make_shared<JVal>(shData, false));//root(ecm)로 감싼다. xml은 단일 루트 요소여야 하니깐
			return make_shared<JVal>(valRt, false);//반드시 ShJVal로 감싸서 리턴 해야
			};

		return saveArchive.GetDataString();
	}

	/// <summary>
	/// JSON 문자열을  pXbj 객체의 필드에 녹여 넣는다.
	/// </summary>
	/// <param name="pXbj">IDocSerialize*</param>
	/// <param name="sJson">UTF8 JSON 문자열</param>
	/// <param name="sDocMode">"XML" 또는 "JSON"</param>
	/// <param name="sRoot">최상위 키를 줘야 그 값의 UcJbj 을 가져온다. 기본은 "root" XML과 JSON 호환을 위해 루트를 반드시 둬야 함</param>
	static void SetDocSerializedString(IDocSerialize* pXbj, CStringA sJson, std::string sDocMode = "JSON", std::string sRoot = "root")
	{
		CMemFile memFile;
		SHP<CArchive> shSaveArchive = CreateArchive(&memFile, CArchive::load, sDocMode); // #JSerialize CJXArchive
		auto& saveArchive = (CJXArchive&)*shSaveArchive;
		saveArchive._afterLoad = [sRoot](ShJVal shDoc) -> ShJVal {//, &arx
			ShJVal root;
			if (shDoc) {
				auto& doc = *shDoc->Dic();
				root = doc.O(sRoot.c_str()); // 루트(ecm)를 벗겨 낸다.
			}
			return root;
			};
		ShJVal shDoc;
		if (sDocMode == "JSON")
			shDoc = UcJson::ParseUtf8(sJson);
		else if (sDocMode == "XML")
			shDoc = UcJson::ParseXml(sJson);

		ShJVal shRoot;
		if (shDoc) {
			if (saveArchive._afterLoad) {
				shRoot = saveArchive._afterLoad(shDoc);
			}
		}
		saveArchive.SetJson(shRoot);//메인 데이터 설정

		try {
			pXbj->Serialize(saveArchive);     // save me
			saveArchive.Close();
		} catch (CException* ) {
			ASSERT(0);
		}
	}

	/// <summary>
	/// map<wstring, IDocSerialize*> 형태의 여러 객체를 XML 또는 JSON 문자열로 직렬화합니다.
	/// </summary>
	/// <param name="mapObjects">직렬화할 객체들의 맵 (키는 "0", "1", "2" 또는 사용자 지정 키)</param>
	/// <param name="sDocMode">"XML" 또는 "JSON"</param>
	/// <param name="sRoot">최상위 키, 기본은 "root"</param>
	/// <returns>UTF8 XML 또는 JSON 문자열</returns>
	static CStringA GetDocSerializedStringMap(const std::map<std::wstring, IDocSerialize*>& mapObjects, std::string sDocMode = "JSON", std::string sRoot = "root")
	{
		CMemFile memFile;
		SHP<CArchive> shSaveArchive = CreateArchive(&memFile, CArchive::store, sDocMode);
		auto& saveArchive = (CJXArchive&)*shSaveArchive;
		
		try {
			// 각 객체를 직렬화하여 루트 객체에 추가
			auto& jRoot = *saveArchive.GetJson();
			
			for (const auto& pair : mapObjects) {
				const std::wstring& key = pair.first;
				IDocSerialize* pObj = pair.second;
				
				if (pObj) {
					// 서브 아카이브 생성
					CJXArchive subArchive(saveArchive, key.c_str());
					pObj->Serialize(subArchive);
					
					// 클래스명 저장 (CObject 기반인 경우)
					CObject* pCObj = dynamic_cast<CObject*>(pObj);
					if (pCObj && subArchive.GetJson() && subArchive.GetJson()->IsDic()) {
						ShJObj shSub = subArchive.GetJson();
						CJXArchive::UcDefaultSaveClassName(shSub, pCObj);
					}
					
					// 상위 노드에 저장 (이것이 없으면 root에 데이터가 추가되지 않음)
					subArchive.SaveToUpperNode();
				}
			}
			
			saveArchive.Close();
		} catch (CException* ) {
			ASSERT(0);
		}
		
		saveArchive._beforeSave = [sRoot](ShJVal shData) -> ShJVal {
			ShJVal valRt = make_shared<UcJObj>();
			auto& jbjRt = *valRt->Dic();
			jbjRt.Set(sRoot.c_str(), make_shared<JVal>(shData, false));/// "root"로 감싼 후,  JVal 로 재차 싼다. 동시에 두가지
			return make_shared<JVal>(valRt, false);
		};
		
		return saveArchive.GetDataString();
	}

	/// <summary>
	/// XML 또는 JSON 문자열을 map<wstring, IDocSerialize*> 형태로 역직렬화합니다.
	/// </summary>
	/// <param name="mapObjects">역직렬화된 객체들을 저장할 맵 (출력 파라미터)</param>
	/// <param name="sData">UTF8 XML 또는 JSON 문자열</param>
	/// <param name="sDocMode">"XML" 또는 "JSON"</param>
	/// <param name="sRoot">최상위 키, 기본은 "root"</param>
	static void SetDocSerializedStringMap(std::map<std::wstring, IDocSerialize*>& mapObjects, CStringA sData, std::string sDocMode = "JSON", std::string sRoot = "root")
	{
		CMemFile memFile;
		SHP<CArchive> shLoadArchive = CreateArchive(&memFile, CArchive::load, sDocMode);
		auto& loadArchive = (CJXArchive&)*shLoadArchive;
		
		// 루트에서 데이터 추출
		loadArchive._afterLoad = [sRoot](ShJVal shDoc) -> ShJVal {
			ShJVal root;
			if (shDoc) {
				auto& doc = *shDoc->Dic();
				root = doc.O(sRoot.c_str());
			}
			return root;
		};
		
		// 문자열 파싱
		ShJVal shDoc;
		if (sDocMode == "JSON")
			shDoc = UcJson::ParseUtf8(sData);
		else if (sDocMode == "XML")
			shDoc = UcJson::ParseXml(sData);
		
		ShJVal shRoot;
		if (shDoc) {
			if (loadArchive._afterLoad) {
				shRoot = loadArchive._afterLoad(shDoc);
			}
		}
		
		if (!shRoot || !shRoot->IsDic()) {
			return; // 파싱 실패
		}
		
		loadArchive.SetJson(shRoot);
		
		try {
			// 루트 객체의 각 키에 대해 객체 생성 및 역직렬화
			auto& jRoot = *shRoot->Dic();
			
			// 기존 맵 클리어
			// 주의: 기존 포인터들은 삭제하지 않음 (호출자가 관리)
			mapObjects.clear();
			
			// 각 키에 대해 객체 생성
			for (auto it = jRoot.begin(); it != jRoot.end(); ++it) {
				const std::wstring& key = it->first;
				ShJVal shSub = it->second;
				
				if (shSub && shSub->IsDic()) {
					// 클래스명으로 객체 생성
					CObject* pObj = UcDefaultCreateByClassName<CObject>(shSub, {}, 0);
					if (pObj) {
						IDocSerialize* pDocSerialize = dynamic_cast<IDocSerialize*>(pObj);
						if (pDocSerialize) {
							// 서브 아카이브로 역직렬화 (키를 전달하여 해당 키의 데이터를 로드)
							CJXArchive subArchive(loadArchive, key.c_str());
							pDocSerialize->Serialize(subArchive);
							
							// 맵에 추가
							mapObjects[key] = pDocSerialize;
						} else {
							delete pObj; // IDocSerialize가 아니면 삭제
						}
					}
				}
			}
			
			loadArchive.Close();
		} catch (CException* ) {
			ASSERT(0);
		}
	}

	/// <summary>
	/// StdMapStrPbjCnv 를 이용한 Arc 버전 직렬화 (기존 함수는 그대로 둠)
	/// </summary>
	static CStringA ArcGetDocSerializedStringMap(std::map<std::wstring, IDocSerialize*>& mapObjects, std::string sDocMode = "JSON", std::string sRoot = "root")
	{
		CMemFile memFile;
		SHP<CArchive> shSaveArchive = CreateArchive(&memFile, CArchive::store, sDocMode);
		auto& saveArchive = (CJXArchive&)*shSaveArchive;

		try {
			std::wstring wRoot(sRoot.begin(), sRoot.end());
			saveArchive.StdMapStrPbjCnv(mapObjects, wRoot); /// 여기서 이미 wRoot로 감싸서 저장됨 1.
			/// GetDataString 에서 호출: 이미 root:랑 연결 되어 있으므로 위와 JVal로 싸주면 된다. 2.
			saveArchive._beforeSave = [](ShJVal shData) -> ShJVal {
				return make_shared<JVal>(shData, false);///JVal로 싸주면 된다. 2.
				};
			saveArchive.Close();
		}
		catch (CException* ) {
			ASSERT(0);
		}

		return saveArchive.GetDataString();
	}

	/// <summary>
	/// StdMapStrPbjCnv 를 이용한 Arc 버전 역직렬화 (기존 함수는 그대로 둠)
	/// </summary>
	static void ArcSetDocSerializedStringMap(std::map<std::wstring, IDocSerialize*>& mapObjects, CStringA sData, std::string sDocMode = "JSON", std::string sRoot = "root")
	{
		CMemFile memFile;
		SHP<CArchive> shLoadArchive = CreateArchive(&memFile, CArchive::load, sDocMode);
		auto& loadArchive = (CJXArchive&)*shLoadArchive;

		// 문자열 파싱
		ShJVal shDoc;
		if (sDocMode == "JSON")
			shDoc = UcJson::ParseUtf8(sData);
		else if (sDocMode == "XML")
			shDoc = UcJson::ParseXml(sData);

		if (!shDoc || !shDoc->IsDic())
			return; // 파싱 실패

		// ArcGetDocSerializedStringMap에서 StdMapStrPbjCnv가 wRoot 키로 저장했으므로,
		// 파싱된 데이터를 그대로 설정 (root 키가 있는 상태)
		loadArchive.SetJson(shDoc);

		try {
			std::wstring wRoot(sRoot.begin(), sRoot.end());
			mapObjects.clear(); // 포인터 delete는 호출자 책임
			// StdMapStrPbjCnv가 wRoot 키로 저장된 데이터를 찾아서 로드
			loadArchive.StdMapStrPbjCnv(mapObjects, wRoot);
			loadArchive.Close();
		}
		catch (CException* ) {
			ASSERT(0);
		}
	}

	// 호환성을 위한 별칭
	//void SaveJsonToFile(const CStringA& jsonStr) { SaveToFile(jsonStr); }
	// C++14 호환: 불필요한 제약 제거
	template<typename VType>
	CJXArchive& operator<<(const std::tuple<wstring, VType*>& tp) {
		//DWKFUNC;
		try {
#if CPP17_OR_LATER
			auto [key, pVal] = tp;
#else
			auto key = std::get<0>(tp);
			auto pVal = std::get<1>(tp);
#endif
			auto& val = *pVal;
			auto bClass = UcIsClass<VType>();
			const char* sTp = typeid(val).name();
			auto bSerialize = has_serialize_v<VType>;
			bool bObj = UcIsObject(val);
			// CArray 타입은 별도로 체크 (복사 생성자가 삭제되어 튜플에 넣을 수 없음)
			bool bIsCArray = (typeid(val) == typeid(CArray<int, int>)) ||
				(typeid(val) == typeid(CArray<double, double>)) ||
				(typeid(val) == typeid(CArray<CStringW, CStringW>)) ||
				(typeid(val) == typeid(CArray<CStringA, CStringA>));
			bool bSupport = bObj && (bIsCArray || UcCheckMultipleBases(val, s_typeSupported));  // 여러 벡터 타입 검사
			auto bSerialize2 = false;
			if (bSupport || !bObj) {//dwk: 2025-03-28 15:51 !bObj 는 객체 아닌 단순 변수 
				ASSERT(!Jbj().Has(key));/// 똑같은 키가 중복 입력 되는 경우
				Jbj()(key) = *pVal;
			}
			else if (bObj) {
				bSerialize2 = UcCheckIfHasSerialize(val);//dwk: s_typeSupported 에 빠진 타입이 들어 옴
			}
			//DWKFUNCV(L"%v : %v (isClass:%v, %v) bSerialize:%v, bSupport:%v", key, val, bClass, sTp, bSerialize, bSupport);
		}
		catch (CException* e) {
			e;
			DWKFUNCV(L"%v", UcGetErrorMsg(::GetLastError()));// e->GetErrorMessage())
		}
		catch (...) {
			DWKFUNCV(L"%v", UcGetErrorMsg(::GetLastError()));// e->GetErrorMessage())
		}
		return *this;
	}
	template<typename VType>
	CJXArchive& operator<<(const std::tuple<wstring, VType*, std::function<void(VType&)>>& tp) {
		DWKFUNC;
		try {
#if CPP17_OR_LATER
			auto [key, pVal, fncBefore] = tp;
#else
			auto key = std::get<0>(tp);
			auto pVal = std::get<1>(tp);
			auto fncBefore = std::get<2>(tp);
#endif
			auto& val = *pVal;
			fncBefore(val);//람다에서 발을 구해 온다.
			//auto curKey = GetKey();
			Jbj()(key) = *pVal;
			//Jbj(curKey.c_str(), true)(key) = *pVal;
			auto bClass = UcIsClass<VType>();
			const char* sTp = typeid(val).name();
			auto bSerialize = has_serialize_v<VType>;
			DWKTRACE(L"%v : %v (isClass:%v, %v) bSerialize:%v", key, val, bClass, sTp, bSerialize);
			//DWKFUNCV(L"%v : %v", key, val);
		}
		catch (CException* e) {
			DWKTRACE(L"%v", UcGetErrorMsg(::GetLastError())); e;// e->GetErrorMessage())
		}
		catch (...) {
			DWKTRACE(L"%v", UcGetErrorMsg(::GetLastError()));// e->GetErrorMessage())
		}
		return *this;
	}
	template<typename VType>
	CJXArchive& operator>>(const std::tuple<wstring, VType*>& tp) {
		//DWKFUNC;
		try {
#if CPP17_OR_LATER
			auto [key, pVal] = tp;
#else
			auto key = std::get<0>(tp);
			auto pVal = std::get<1>(tp);
#endif
			auto& jbj = Jbj();
			auto& val = *pVal;
			JSonToVariable(jbj, key, val);//없으니까, 일단 새 저장 부터 해야지. 서브데이터 안들어가네~
			auto bClass = UcIsClass<VType>();
			auto sTp = UcShortTypeT(val);
			//const char* sTp = typeid(val).name();
			auto bSerialize = has_serialize_v<VType>;
			//DWKFUNCV(L"%v : %v (isClass:%v, %v) bSerialize:%v", key, val, bClass, sTp, bSerialize);
		}
		catch (CException* e) {
			DWKFUNCV(L"%v", UcGetErrorMsg(::GetLastError())); e;// e->GetErrorMessage())
		}
		catch (...) {
			DWKFUNCV(L"%v", UcGetErrorMsg(::GetLastError()));// e->GetErrorMessage())
		}
		return *this;
	}
	//typename = std::enable_if_t<std::is_invocable_v<std::function<void(VType&)>>>>  // 후처리 함수 포함 버전
	template<typename VType>
	CJXArchive& operator>>(const std::tuple<wstring, VType*, std::function<void(VType&)>>& tp)
	{
		//DWKFUNC;
		try {
#if CPP17_OR_LATER
			auto [key, pVal, fncAfter] = tp;
#else
			auto key = std::get<0>(tp);
			auto pVal = std::get<1>(tp);
			auto fncAfter = std::get<2>(tp);
#endif
			auto& val = *pVal;
			auto& jbj = Jbj();
			JSonToVariable(jbj, key, val);//없으니까, 일단 새 저장 부터 해야지. 서브데이터 안들어가네~
			fncAfter(val);
			auto bClass = UcIsClass<VType>();
			//const char* sTp = typeid(val).name();
			auto sTp = UcShortTypeT(val);
			auto bSerialize = has_serialize_v<VType>;
			DWKFUNCV(L"%v : %v (isClass:%v, %v) bSerialize:%v", key, val, bClass, sTp, bSerialize);
		}
		catch (CException* e) {
			DWKFUNCV(L"%v", UcGetErrorMsg(::GetLastError())); e;// e->GetErrorMessage())
		}
		catch (...) {
			DWKFUNCV(L"%v", UcGetErrorMsg(::GetLastError()));// e->GetErrorMessage())
		}
		return *this;
	}
	bool _bFinished = false;
	void SaveToFile(const CStringA& sData);
	void LoadFromFile(LPCTSTR lpszPathName);
	virtual void Write(const void* lpBuf, UINT nMax);
	virtual void Flush();
	virtual void Close();
	virtual UINT Read(void* lpBuf, UINT nMax);
	/// <summary>
	/// 구명:CreateSharedJsonArchive
	/// </summary>
	/// <param name="pFile"></param>
	/// <param name="bLoad">CArchive::store = 0, CArchive::load = 1, bNoFlushOnDelete = 2, bNoByteSwap = 4</param>
	/// <param name="sMode">JSON, XML</param>
	/// <param name="exOption"></param>
	/// <returns></returns>
	inline SHP<CArchive> CreateJXArchive(CFile* pFile, CArchive::Mode bLoad, const std::string sMode = "JSON", int exOption = 0)
	{
		auto sha = NEWSHP(CJXArchive, pFile, sMode, bLoad | CArchive::bNoFlushOnDelete | exOption);
		return std::static_pointer_cast<CArchive>(sha);
	}
	/// <summary>
	/// 변수에서 가져오거나 변수에 넣을 때 반드시 이 객체를 생성한 블러 안에 넣어야한다.
	/// 그래야 ~CSaveLoad 에서 저장 또는 로드가 완료 된다.
	/// </summary>
	class UCTOOLDYNAMIC CSaveLoad
	{
	public:
		CSaveLoad(CJXArchive& ar);
		~CSaveLoad();
	private:
		CJXArchive& _ar;
	};
	/// CMapWordToPtr 전용: WORD 키, 값은 TObj* (raw pointer)
	/// 주의: 로드 시 new TObj()로 생성하여 map에 넣습니다. 소멸 책임은 호출자에게 있습니다.
	/// GetMyClassName()을 직접 호출 (없으면 컴파일 오류)
	template<typename T, typename = void>
	struct has_GetMyClassName : std::false_type {};

	template<typename T>
	struct has_GetMyClassName<T, decltype(void(std::declval<T>().GetMyClassName()))> : std::true_type {};

	// 2) 포인터 버전 (TObj*)
	//    - GetMyClassName() 있으면 정상 호출
	//    - 없으면 컴파일 에러(static_assert) + ASSERT(0)

	/// IDocSerialize 인터페이스로 dynamic_cast하여 GetMyClassName() 호출
	template<typename TObj>
	static void UcDefaultSaveClassName(ShJObj shSub, TObj* pObj)
	{
		ASSERT(shSub->IsDic());

		if (!pObj) {
			shSub->Dic()->SetAttr(TAG_CLS, L"Unknown");
			ASSERT(0);
			return;
		}

		// IDocSerialize로 dynamic_cast
		IDocSerialize* pIDoc = dynamic_cast<IDocSerialize*>(pObj);
		if (!pIDoc) {
			ASSERT(0 && "IDocSerialize 인터페이스가 없습니다. IDocSerialize를 상속받았는지 확인하세요.");
			shSub->Dic()->SetAttr(TAG_CLS, L"Unknown");
			return;
		}

		const wchar_t* className = pIDoc->GetMyClassName();
#ifdef _DEBUG
		CStringW sClNm(className);
		auto i0 = sClNm.Find(L"CDataField");
		if (i0 < 0)
			_break;/// 여기서 pObj의 타입과 타입명을 비교 한다.
#endif // _DEBUG
		if (className && wcslen(className) > 0) {
			shSub->Dic()->SetAttr(TAG_CLS, className);
			return;
		}

		ASSERT(0);
		shSub->Dic()->SetAttr(TAG_CLS, L"Unknown");
	}

	// 참조 버전
	template<typename TObj>
	static void UcDefaultSaveClassName(ShJObj shSub, TObj& pObj)
	{
		ASSERT(shSub->IsDic());

		// IDocSerialize로 dynamic_cast
		IDocSerialize* pIDoc = dynamic_cast<IDocSerialize*>(&pObj);
		if (!pIDoc) {
			ASSERT(0 && "IDocSerialize 인터페이스가 없습니다. IDocSerialize를 상속받았는지 확인하세요.");
			shSub->Dic()->SetAttr(TAG_CLS, L"Unknown");
			return;
		}

		const wchar_t* className = pIDoc->GetMyClassName();
		if (className && wcslen(className) > 0) {
			shSub->Dic()->SetAttr(TAG_CLS, className);
			return;
		}

		ASSERT(0);
		shSub->Dic()->SetAttr(TAG_CLS, L"Unknown");
	}

	template<typename TObj>
	static void UcDefaultSaveClassName(ShJObj shSub, SHP<TObj> shObj) {
		CJXArchive::UcDefaultSaveClassName(shSub, shObj.get());
	}

	static CStringW CleanTypeName(const char* rawName)
	{
		CStringW s(rawName);
		// 제거해야 할 prefix 목록
		static const wchar_t* prefixes[] = {
			 L"struct tag",
			 L"struct ",
			 L"class ",
			 L"union ",
			 L"enum "
		};
		for (auto prefix : prefixes) {
			int len = (int)wcslen(prefix);
			if (s.Left(len).CompareNoCase(prefix) == 0)
				return s.Mid(len); // prefix 잘라냄
		}
		return s;
	}
	template<typename TObj>
	static CStringW GetCleanTypeName() {
		return CJXArchive::CleanTypeName(typeid(TObj).name());
	}
	/// DYNCREATE만 해서는 AfxGetModuleState에 등록이 안된다는 걸 확인 했던 함수.
	static void DumpMfcRuntimeClasses() {
#ifdef _DEBUG
		TRACE(L"\n=== MFC Runtime Classes ===\n");
		CRuntimeClass* pClass = nullptr;
		// MFC의 런타임 클래스 체인을 순회
		AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
		if (pModuleState && pModuleState->m_classList) {
			pClass = pModuleState->m_classList;
			while (pClass) {
				TRACE(L"  %s\n", CStringW(pClass->m_lpszClassName).GetString());
				pClass = pClass->m_pNextClass;
			}
		}
		TRACE(L"=== End of Runtime Classes ===\n\n");
#endif
	}

#ifdef CPP_BEFORE_17
	// 추상 클래스인 경우 (CObject 여부와 관계없이)
	// CObject가 아닌 경우만 선택되는 버전 (추상이 아닌 경우)
	template<typename TObj>
	static typename std::enable_if<!std::is_base_of<CObject, TObj>::value && !std::is_abstract<TObj>::value, TObj*>::type
		CreateDefaultObject() {
		return new TObj();/// CObject 계열은 직접 생성 안 함 : 런타임 클래스, IMPLEMENT_DYNCREATE, CreateObject 사용
	}

	template<typename TObj>
	static typename std::enable_if<std::is_abstract<TObj>::value, TObj*>::type
		CreateDefaultObject() {
		// 추상 클래스는 직접 인스턴스화할 수 없음
		return nullptr;
	}
	// CObject 또는 CObject 파생형일 경우 선택되는 버전 (추상이 아닌 경우)
	template<typename TObj>
	static typename std::enable_if<std::is_base_of<CObject, TObj>::value && !std::is_abstract<TObj>::value, TObj*>::type
		CreateDefaultObject() {	// CObject (특히 추상) 은 직접 생성 불가
		return new TObj();
		//return nullptr;
	}
#else // CPP_BEFORE_17
	template<typename TObj>
	static TObj* CreateDefaultObject()
	{
		if constexpr (std::is_abstract_v<TObj>)
			return nullptr;// 추상 클래스는 생성 불가
		else if constexpr (std::is_base_of_v<CObject, TObj>)
			return new TObj();// CObject 계열은 직접 생성 안 함 : 런타임 클래스, IMPLEMENT_DYNCREATE, pRC->CreateObject
		//return nullptr;
		else
			return new TObj();
	}
#endif // CPP_BEFORE_17

	template<typename TObj>
	static TObj* UcDefaultCreateByClassName(ShJVal shSub, wstring key, wstring keyId) {
		ASSERT(shSub->IsDic());
		if (shSub->IsDic()) {
			CString classType = shSub->Dic()->ST(TAG_CLS);
			if (classType.GetLength()) {
				// TObj가 CObject와 완전히 동일할 때 (IsDic() 조건도 만족할 때)
				// 즉, TObj가 CObject 자체일 때는 MFC 런타임 생성 로직을 시도합니다.
					// 런타임 클래스를 찾지 못했어도 이 블록은 CObject 타입이므로 new TObj() 호출을 건너뜁니다.
				if (std::is_base_of<CObject, TObj>::value)
				{
					// CObject 기반 → MFC 런타임 생성 시도
					if (auto prt = CRuntimeClass::FromName(classType.GetString()))
						return (TObj*)prt->CreateObject();
				}
				CStringW classTypeW(classType);
				auto pObj = (TObj*)CreateClassFromName<TObj>(classTypeW.GetString());
				ASSERT(pObj);// == "아마 SET_FACTORY를 뺴 먹은 듯.");
				return pObj;
			}
		}
		return CJXArchive::CreateDefaultObject<TObj>();
	}
	template<typename TObj>
	static TObj* UcDefaultCreateByClassName(ShJVal shSub, wstring key, DWORD id) {
		wstring keyId = std::to_wstring(id);
		return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, key, keyId);
	}
	//inline tuple<wstring, std::decay_t<T>> ValBfrAndAft(CJXArchive& ar, std::wstring key, function<void(T&)> fncStore, function<void(T&)> fncLoad)//dwk: 2025-03-7 08:57  
	template<typename T>
	inline void ValBfrAndAft(CJXArchive& ar, std::wstring key, function<void(T&)> fncStore, function<void(T&)> fncLoad)//dwk: 2025-03-7 08:57  
	{
		T val{};
		if (ar.IsStoring()) {
			fncStore(val);//val을 준비한다.
			ar << std::make_tuple(key, (&val));
		}
		else {
			ar >> std::make_tuple(key, (&val));
			fncLoad(val);//val을 처리한다.
		}
	}
	//#define SubObj_Serialize(ar, subObj) subObj.Serialize(CJXArchive(ar, L#subObj))
	void SaveToUpperNode(ShJBase shToSveToUp = {}, std::initializer_list<std::pair<wstring, wstring>> attrs = {}) {
		//DWKREMINDER("새 객체를 상위 노드에 키로 등록 하는 건 SaveToUpperNode 로 통일 하자.");
		ASSERT((m_nMode & eDocMode) != 0);
		auto pJbjUp = static_cast<CJXArchive*>(_pArUp); ASSERT(pJbjUp);
		auto& jbju = pJbjUp->Jbj();
		//pJbjUp->Jbj()(_key) = GetJson();/// 최상위가 아니면, 상위 CJXArchive 에 _key 로 clone 하여 넣는다.
		//pJbjUp->Jbj().Set(_key, make_shared<JVal>(GetJson(), false));
		ASSERT(_key.length() > 0);
		ASSERT(!jbju.Has(_key));
		ShJBase shv = shToSveToUp ? shToSveToUp : GetJson();
		jbju.SetNode(_key, shv, false);// 복사하지 않는다.
		for (auto& pr : attrs) {
			ASSERT(shv->IsDic());// array 일수 있다.attrs.size() == 0 이겠지.
			shv->Dic()->SetAttr(pr.first, pr.second);
		}
	}
	/// 자체 JSON Serialize가 구현된 객체
	template<typename TObj>
	void ObjectItself(TObj& obj, std::wstring key) {
		auto& ar = *this;
		CJXArchive ar2(ar, key.c_str());
		if (ar.IsStoring()) {
			dynamic_cast<IDocSerialize*>(&obj)->Serialize(ar2);
			CJXArchive::UcDefaultSaveClassName(ar2.GetJson(), &obj);//ObjectItself
			ar2.SaveToUpperNode(); //상위 노드에 key 로 넣어 준다.
		}
		else {
			if (ar2.Jbj().size()) {
				dynamic_cast<IDocSerialize*>(&obj)->Serialize(ar2);
			}
		}
	}
	/// 자체 JSON Serialize가 구현된 객체
	//template<typename TObj>
	//void NoObjItself(CJXArchive& ar, TObj& obj, std::wstring key) {
	//	ObjectItself(ar, obj, key);
	//}
	template<typename TObj>
	void ObjectPointer(TObj*& ptr, std::wstring key, bool bRenew = true) {
		auto& ar = *this;
		CJXArchive ar2(ar, key.c_str());
#if CPP17_OR_LATER
		if constexpr (std::is_pointer<TObj*>::value) {
#else
		if (std::is_pointer<TObj*>::value) {
#endif
			if (ptr) {//auto bPtr = AfxIsValidAddress(ptr, sizeof(TObj), 0);이거 쓰레기 임. 쓰지마.
				if (ar.IsStoring()) {
					dynamic_cast<IDocSerialize*>(ptr)->Serialize(ar2);
					/// 다양한 객체의 base class로 저장 될 때는 __class__를 반드시 넣어 줘야 한다.
					CJXArchive::UcDefaultSaveClassName(ar2.GetJson(), ptr);//ObjectPointer
					ar2.SaveToUpperNode(); //상위 노드에 key 로 넣어 준다.
				}
				else {
					if (bRenew) {
						// 로드 시점에는 포인터가 null이어야 정상. 이미 있으면 이전 할당이거나 메모리 누수 가능성. delete 안할려면 Arc_PtrObjNoDel
						ASSERT(0 && "ObjectPointer: 포인터가 이미 할당되어 있습니다. 로드 전에 정리하거나 Arc_PtrObjNoDel 써라.");
						delete ptr;// 기존 포인터를 delete하고 새로 할당
						ptr = nullptr;
						if (ar2.Jbj().size()) {
							ptr = UcDefaultCreateByClassName<TObj>(ar2.GetJson(), {}, 0);
							if (ptr)
								dynamic_cast<IDocSerialize*>(ptr)->Serialize(ar2);
						}
					}
					else {// NoDel
						dynamic_cast<IDocSerialize*>(ptr)->Serialize(ar2);
					}
				}
			}
			else {// ptr == nullptr
				if (ar.IsStoring()) {
					// nullptr이면 아무것도 안하니, 상위 키에 넣을 것도 없지.
				}
				else {//로드떄는 할당 해서라도 해야지.
					if (ar2.Jbj().size()) {
						// __class__에 따라 적절한 타입으로 생성 (base class 포인터일 수도 있음)
						ptr = UcDefaultCreateByClassName<TObj>(ar2.GetJson(), {}, 0);
						if (ptr)
							dynamic_cast<IDocSerialize*>(ptr)->Serialize(ar2);// ptr <= ar2.jbj()
					}
				}
			}
		}
		else {//std::cout << "Shared Pointer or other type\n";
			ASSERT(0 == "not a pointer of object.");
		}
		}
	//template<typename TObj>
	//void NoObjPointer(CJXArchive& ar, TObj*& ptr, std::wstring key) {
	//	ObjectPointer(ar, ptr, key);
	//DWKREMINDER("CObject아니면 DocSerialize만 달렸는데, 일반 SET_FACTORY 후 class에도 Serialize를 넣기로 헀다..");
	//}
	template<typename TPtr>
	void ObjectSharedPtr(SHP<TPtr>&ptr, std::wstring key) {
		auto& ar = *this;
		CJXArchive ar2(ar, key.c_str());
		if (ptr) {
			if (ar.IsStoring()) {
				dynamic_cast<IDocSerialize*>(ptr.get())->Serialize(ar2);
				/// 다양한 객체의 base class로 저장 될 때는 __class__를 반드시 넣어 줘야 한다.
				CJXArchive::UcDefaultSaveClassName(ar2.GetJson(), ptr);//ObjectSharedPtr
				ar2.SaveToUpperNode(); //주의: ~CSaveLoad -> SaveToFile 에서 한다.
			}
			else {
				// 로드 시점에는 포인터가 null이어야 정상. 이미 있으면 이전 할당이거나 메모리 누수 가능성
				ASSERT(0 && "ObjectSharedPtr: 포인터가 이미 할당되어 있습니다. 로드 전에 정리해야 합니다.");
				// 기존 shared_ptr을 reset하고 새로 할당
				ptr.reset();
				if (ar2.Jbj().size()) {
					auto pObj = UcDefaultCreateByClassName<TPtr>(ar2.GetJson(), {}, 0);
					if (pObj) {
						ptr = shared_ptr<TPtr>(pObj);
						dynamic_cast<IDocSerialize*>(ptr.get())->Serialize(ar2);
					}
				}
			}
		}
		else {
			if (ar.IsStoring()) {
				// nullptr이면 아무것도 안하니, 상위 키에 넣을 것도 없지.
			}
			else {//로드떄는 할당 해서라도 해야지.
				if (ar2.Jbj().size()) {
					// __class__에 따라 적절한 타입으로 생성 (base class 포인터일 수도 있음)
					auto pObj = UcDefaultCreateByClassName<TPtr>(ar2.GetJson(), {}, 0);
					if (pObj) {
						ptr = shared_ptr<TPtr>(pObj);// NEWSHP(TPtr);
						dynamic_cast<IDocSerialize*>(ptr.get())->Serialize(ar2);// ptr <= ar2.jbj()
					}
				}
			}
		}
	}
	/// user object의 list,vector,array 를 JSON 객체에 넣는다.
	/// <param name="lstTObj">KSharedPtrList<TObj></param>
	/// StdListRefObj_1 -> ArrayOfPbjToJsonWithKeyTmp -> ArrayOfObjSerializeLB
	/// single(std::vector,list> template parameter version
	//template<template<typename> class TList, typename TPObj>
	template<template<typename, typename...> class TList, typename TPObj, typename... TArgs>
	void StdArrTPbj_LB4(ShJArr shArr, TList<TPObj>&lstTObj
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(TList<TPObj>&, TPObj&)> cbAddItem
		, function<void(TList<TPObj>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, DWORD)> cbFactory
		, function<void(CJXArchive&, TPObj&)> CbLoad
	)
	{
		auto& ar = *this;
		if (ar.IsStoring())// CArchive::load == 1 과 값을 맞추기 위해 bStore를 쓰지 않는다.
		{
			for (auto& pItem : lstTObj)
			{
				CJXArchive ari(*this);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
				CbSave(ari, pItem);//shObj3->Serialize(ar3);//Serialize result to inside _jdata
				shArr->Arr()->Add(ari.GetJson(), false);// add to shArr
			}//array를 완성한다.
		}
		else
		{
			CbClear(lstTObj);
			auto pjarr = shArr->Arr();
			if (pjarr)
			{
				auto& jarr = *shArr->Arr();
				int i = 0;
				for (auto& shjv : jarr) {
					CJXArchive ari(*this, shjv);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
					TPObj newPbj;// = nullptr;
					if (cbFactory)
						newPbj = cbFactory(shjv, i);///step 3:  NEWSHP(TObj);/// new TObj 여기서 람다 함수로 []() -> shared_ptr 로 리턴 하게
					if (CbLoad)
						CbLoad(ari, newPbj);//newPbj->Serialize(arcItm);//JSON to TObj
					cbAddItem(lstTObj, newPbj);
					i++;
				}
			}
		}
	}
	/// MfcList는 for(GetHeadPosition GetNext) loop 사용
	/// MfcListOfPbjToJsonWithKey -> MfcListTPbj_step3 -> MfcListOfObjSerializeLB
	template<template<typename, typename> class TList, typename TPObj>
	void MfcListTPbj_LB4(CJXArchive & ar, ShJArr shArr, TList<TPObj, TPObj>&lstTObj
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(TList<TPObj, TPObj>&, TPObj&)> cbAddItem
		, function<void(TList<TPObj, TPObj>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, WORD)> cbFactory
		, function<void(CJXArchive&, TPObj&)> CbLoad
	)
	{
		//ASSERT(&ar == this);
		if (ar.IsStoring())// CArchive::load == 1 과 값을 맞추기 위해 bStore를 쓰지 않는다.
		{
			POSITION pos = lstTObj.GetHeadPosition();
			while (pos != NULL) {
				TPObj pItem = lstTObj.GetNext(pos);
				//if (pItem) {
				CJXArchive ari(*this);///step 1: 저장 할 Archive 생성
				CbSave(ari, pItem);
				shArr->Arr()->Add(ari.GetJson(), false);// add to shArr
				//}
			}
		}
		else
		{
			CbClear(lstTObj);
			auto pjarr = shArr->Arr();
			if (pjarr)
			{
				auto& jarr = *shArr->Arr();
				int i = 0;
				for (auto& shjv : jarr) {
					CJXArchive ar3(ar, shjv);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
					TPObj pObj3;// = nullptr;
					if (cbFactory)
						pObj3 = cbFactory(shjv, i);///step 3:  NEWSHP(TObj);/// new TObj 여기서 람다 함수로 []() -> shared_ptr 로 리턴 하게
					//ASSERT(pObj3);
					//if (pObj3) {
					if (CbLoad)
						CbLoad(ar3, pObj3);
					cbAddItem(lstTObj, pObj3);
					//}
					i++;
				}
			}
		}
	}
	/// MfcArrayOfPbjToJsonWithKey -> MfcArrayOfPbjToJsonWithKeyTmp -> MfcArrayTPbj_LB4
	template<template<typename, typename> class TList, typename TPObj, typename TPArg>
	void MfcArrayTPbj_LB4(CJXArchive & ar2, ShJArr shArr, TList<TPObj, TPArg>&lstTObj
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(TList<TPObj, TPArg>&, TPObj&)> cbAddItem
		, function<void(TList<TPObj, TPArg>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, WORD)> cbFactory
		, function<void(CJXArchive&, TPObj&)> CbLoad = {}
	)
	{
		if (ar2.IsStoring())// CArchive::load == 1 과 값을 맞추기 위해 bStore를 쓰지 않는다.
		{
			for (int i = 0; i < lstTObj.GetCount(); ++i) {
				TPObj pObj3 = lstTObj.GetAt(i);
				CJXArchive ari(ar2);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
				CbSave(ari, pObj3);
				auto shJbj3 = ari.GetJson();//데이터가 고스란히 ar3._jdata에 들어 있다.
				shArr->Arr()->Add(shJbj3, false);// add to shArr
			}//array를 완성한다.
		}
		else
		{
			CbClear(lstTObj);
			auto pjarr = shArr->Arr();
			if (pjarr)
			{
				auto& jarr = *shArr->Arr();
				int i = 0;
				for (auto& shjv : jarr) {
					CJXArchive ar3(ar2, shjv);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
					TPObj pObj3;///?ref 객체가 직접 올수도 있기 때문에 = nullptr;
					if (cbFactory)
						pObj3 = cbFactory(shjv, i);///step 3:  NEWSHP(TObj);/// new TObj 여기서 람다 함수로 []() -> shared_ptr 로 리턴 하게
					//ASSERT(pObj3);//?ref
					//if (pObj3) {///?ref 객체가 직접 올수도 있기 때문에
					CbLoad(ar3, pObj3);
					cbAddItem(lstTObj, pObj3);
					//}
					i++;
				}
			}
		}
	}
	/// StdListRefObj_1 -> StdArrTPbj_step3 -> ArrayOfObjSerializeLB
	/// single(std::vector,list> template parameter version
	/// 구: Array~tmp_3
	//template<template<typename> class TList, typename TPObj>
	//template<template<typename, typename...> class TList, typename TObj>
	template<template<typename, typename...> class TList, typename TPObj, typename... TArgs>
	void StdArrTPbj_step3(TList<TPObj>&lstTObj, wstring key
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(TList<TPObj>&, TPObj&)> CbAdd // load때만 쓰인다.
		, function<void(TList<TPObj>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, DWORD)> CbFactory // load때만 쓰인다.
		, function<void(CJXArchive&, TPObj&)> CbLoad
	)
	{
		DWKUSETRACE;
		try
		{
			auto& ar = *this;
			CJXArchive ar2(*this, key.c_str());
			if (this->IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				// 저장 모드에서는 cb__ 사용되지 않지만, 시그니처를 맞추기 위해 전달
				StdArrTPbj_LB4<TList, TPObj>(shArrNew, lstTObj, CbSave, {}, {}, {}, {});
				ar2.SaveToUpperNode(shArrNew);
			}
			else {// List<obj> load test
				ShJObj shObj = this->GetJson();
				auto& j1 = this->Jbj();
				ShJArr shArr;
				if (j1.Has(key)) {//키가 있나?
					auto shv = j1.Get(key);// .IsDic()
					if (shv->IsDic()) {//array인줄 알고 왔는데 Dic이라고? 그러면 필히 item 이 있겠지.
						auto& jd = *shv->Dic();
						if (jd.Has(TAG_ITM)) {
							auto shJit = jd.Get(TAG_ITM);
							if (shJit->IsArr())
								shArr = shJit;//최종적으로 array ShJVal을 구함
						}
						else {
							ASSERT("arrary 인데 Dic이 왔음" == nullptr);
						}
					}
					else if (shv->IsArr()) // 이 함수 자체가 array인 경우만 사용 하므로
						shArr = shv;
				}
				if (shArr)
					StdArrTPbj_LB4<TList, TPObj>(shArr, lstTObj, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
			}
		}
		catch (CException* e) {
			throw e;
		}
		catch (LPCWSTR serr) {
			serr;
			DWKTRACE(L"JSON error: %v", serr);
		}
	}
	/// MfcList는 for(GetHeadPosition GetNext) loop 사용
	/// MfcListOfPbjToJsonWithKey -> MfcListTPbj_step3 -> MfcListOfObjSerializeLB
	template<template<typename, typename> class TList, typename TPObj>
	void MfcListTPbj_step3(TList<TPObj, TPObj>&lstTObj, wstring key
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(TList<TPObj, TPObj>&, TPObj&)> CbAdd // load때만 쓰인다.
		, function<void(TList<TPObj, TPObj>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, WORD)> CbFactory// load때만 쓰인다.
		, function<void(CJXArchive&, TPObj&)> CbLoad// load때만 쓰인다.
	)
	{
		auto& ar = *this;
		//DWKFUNC;
		try
		{
			CJXArchive ar2(ar, key.c_str());
			if (ar.IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				// 저장 모드에서는 cbFactory와 cbAddItem이 사용되지 않지만, 시그니처를 맞추기 위해 전달
				MfcListTPbj_LB4<TList, TPObj>(ar2, shArrNew, lstTObj, CbSave, {}, {}, {}, {});
				ar2.SaveToUpperNode(shArrNew);
			}
			else {//xxx List<obj> load test
				ShJObj shObj = ar.GetJson();
				auto& j1 = ar.Jbj();
				ShJArr shArr;
				if (j1.Has(key)) {//키가 있나?
					auto shv = j1.Get(key);// .IsDic()
					if (shv->IsDic()) {//array인줄 알고 왔는데 Dic이라고? 그러면 필히 item 이 있겠지.
						auto& jd = *shv->Dic();
						if (jd.Has(TAG_ITM)) {
							auto shJit = jd.Get(TAG_ITM);
							if (shJit->IsArr())
								shArr = shJit;//최종적으로 array ShJVal을 구함
						}
						else { ASSERT("arrary 인데 Dic에 item 키 도 없음." == nullptr); }
					}
					else if (shv->IsArr()) // 이 함수 자체가 array인 경우만 사용 하므로
						shArr = shv;
				}
				if (shArr)
					MfcListTPbj_LB4<TList, TPObj>(ar2, shArr, lstTObj, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
			}
		}
		catch (CException* e) {
			throw e;
		}
		catch (LPCWSTR serr) {
			TRACE(L"JSON error: %s\n", serr); 
		}
	}
	/// MFC CArray<TObj*, TObj*> 용 (직접 지원)
	/// MfcArrayOfPbjToJsonWithKey -> MfcArrayOfPbjToJsonWithKeyTmp -> MfcArrayTPbj_LB4
	template<template<typename, typename> class TList, typename TPObj, typename TPArg>
	void MfcArrayPtr_step3(TList<TPObj, TPArg>&lstTObj, wstring key
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(TList<TPObj, TPArg>&, TPObj&)> CbAdd
		, function<void(TList<TPObj, TPArg>&)> CbClear
		, function<TPObj(ShJVal, WORD)> CbFactory
		, function<void(CJXArchive&, TPObj&)> CbLoad
	) // load때만 쓰인다.
	{
		auto& ar = *this;
		DWKUSETRACE;
		try
		{
			CJXArchive ar2(ar, key.c_str());
			if (ar.IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				// 저장 모드에서는 cb__ 사용되지 않지만, 시그니처를 맞추기 위해 전달
				MfcArrayTPbj_LB4<TList, TPObj, TPArg>(ar2, shArrNew, lstTObj, CbSave, {}, {}, {}, {});
				ar2.SaveToUpperNode(shArrNew);
				//ar.Jbj()(ar2._key) = shArrNew;//array of obj를 드디어 그 위 상위에 키로 넣는다.
			}
			else {// List<obj> load test
				ShJObj shObj = ar.GetJson();
				auto& j1 = ar.Jbj();
				ShJArr shArr;
				if (j1.Has(key)) {//키가 있나?
					auto shv = j1.Get(key);// .IsDic()
					if (shv->IsDic()) {//array인줄 알고 왔는데 Dic이라고? 그러면 필히 item 이 있겠지.
						auto& jd = *shv->Dic();
						if (jd.Has(TAG_ITM)) {
							auto shJit = jd.Get(TAG_ITM);
							if (shJit->IsArr())
								shArr = shJit;//최종적으로 array ShJVal을 구함
						}
						else { ASSERT("arrary 인데 Dic에 item 키 도 없음." == nullptr); }
					}
					else if (shv->IsArr()) // 이 함수 자체가 array인 경우만 사용 하므로
						shArr = shv;
				}
				if (shArr)
					MfcArrayTPbj_LB4<TList, TPObj, TPArg>(ar2, shArr, lstTObj, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
			}
		}
		catch (CException* e) {
			throw e;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr); serr;
		}
	}
	template<template<typename> class TList, typename TObj>
	void StdTArrTPobj_step3(CJXArchive & ar2, ShJArr shArr, TList<SHP<TObj>>&lstTObj)
	{
		StdArrTPbj_LB4<TList, SHP<TObj>>(ar2, shArr, lstTObj,
			[](ShJVal shSub, WORD id) -> SHP<TObj> {
				auto pObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
				return shared_ptr<TObj>(pObj);
			},
			[](TList<SHP<TObj>>& lst, SHP<TObj>& item) {
				lst.push_back(item);
			});
		/// SaveToUpperNode는 StdTArrShp2D_1에서 처리한다.
	}
	//[deprecated]
	template<typename Fnc, typename TContainer>
	void StdTArrTStructCustom_LB4(CJXArchive & ar2, ShJArr shArr, TContainer & lstTObj, Fnc cbCutom)//?step 3
	{
		using TVal = typename TContainer::value_type;
		if (ar2.IsStoring())// CArchive::load == 1 과 값을 맞추기 위해 bStore를 쓰지 않는다.
		{
			for (auto& item : lstTObj)
			{
				CJXArchive ar3(ar2);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
				cbCutom(ar3, item);/// //?step 4- /////////////////// Lambda /////////////////
				auto shJbj3 = ar3.GetJson();//데이터가 고스란히 ar3._jdata에 들어 있다.
				shArr->Arr()->Add(shJbj3, false);// add to shArr
			}//array를 완성한다.
		}
		else
		{
			lstTObj.clear();
			if (shArr)
			{
				auto pjarr = shArr->Arr();
				if (pjarr)
				{
					auto& jarr = *shArr->Arr();
					for (auto& shjv : jarr) {
						CJXArchive ar3(ar2, shjv);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
						//ar3.SetJson(shjv);// input: JSON
						TVal item{};
						cbCutom(ar3, item);
						lstTObj.push_back(item);// TObj to list
					}
				}
			}
		}
	}
	/// ArrayOfValJSerialize_Variant StdTArrTStruct_2 StdTArrTStructCustom_3
	template<typename TContainer, typename Fnc>
	void StdTArrTStruct_2(TContainer & lstTObj, wstring key, Fnc cbCustom)//?step 2
	{
		auto& ar = *this;
		DWKUSETRACE;
		try
		{
			using TVal = typename TContainer::value_type;
			CJXArchive ar2(ar, key.c_str());  // 복사 생성자 아님, 명시적 생성자 호출
			if (ar.IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				StdTArrTStructCustom_LB4(ar2, shArrNew, lstTObj, cbCustom);//?step 3- // shArr <= lstTObj
				ar2.SaveToUpperNode(shArrNew);//now
			}
			else {// List<obj> load test
				lstTObj.clear();
				ShJObj shObj = ar.GetJson();
				auto& j1 = *shObj->Dic();// == .Jbj();
				ShJArr shArr;
				if (ShJVal shItem = j1.Get(key)) {
					if (shItem->IsDic()) // XML에서는 <item> 이 하나 더 걸쳐 있을 수 있다.
					{
						auto& jd = *shItem->Dic();
						if (jd.Has(TAG_ITM)) {
							auto shJit = jd.Get(TAG_ITM);
							if (shJit->IsArr())
								shArr = shJit->Val()->AsArray();//최종적으로 array ShJVal을 구함
						}
					}
					else if (shItem->IsArr())
						shArr = shItem->Val()->AsArray();// ShJVal을 ShJArr로 변환
					else
						throw_str(L"Not a Array or item Object.");
					if (shArr)
						StdTArrTStructCustom_LB4(ar2, shArr, lstTObj, cbCustom);
				}
			}
		}
		catch (CException* e) {
			throw e;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr); serr;
		}
	}
#ifndef _UseClassVariableKey__
	// __FUNCTION__에서 클래스 이름 추출 헬퍼 함수

#define CLASSKEY(var) ArcClassKey(__FUNCTION__, L#var)
//#define CLASSKEY(var) ArcClassName(__FUNCTION__) + std::wstring(L#var)
#define KEYVALTP(var) std::make_tuple(CLASSKEY(var), &(var))
#define KEYVALTPNC(var) std::make_tuple(std::wstring(L#var), &(var))
#else
#define KEYVALTP(var) std::make_tuple(std::wstring(L#var), &(var))
#endif
	/// 키를 명시해야 하는 경우
//	#define KEYTpSKey(var,key)    std::make_tuple(std::wstring(key), &(var))
	/// 키를 명시해야 하는 경우 (const char* 또는 const wchar_t* 모두 지원)
	template<typename TVar>
	inline auto KEYTpSKey(TVar & var, const char* key) -> decltype(std::make_tuple(std::wstring(), &var)) {
		CStringW wkey(key);
		return std::make_tuple(std::wstring(wkey), &var);
	}
	template<typename TVar>
	inline auto KEYTpSKey(TVar & var, const wchar_t* key) -> decltype(std::make_tuple(std::wstring(), &var)) {
		return std::make_tuple(std::wstring(key), &var);
	}
	template<typename TVar>
	inline auto KEYTpSKey(TVar & var, const std::wstring & key) -> decltype(std::make_tuple(std::wstring(), &var)) {
		return std::make_tuple(key, &var);
	}
#define KEYTpKeyNC(var, key)    std::make_tuple(std::wstring(L#key), &(var))
#define KEYFieldNC(body, field) std::make_tuple(std::wstring(L#field), &(body.field))
#define KEYPtrFieldNC(body, field) std::make_tuple(std::wstring(L#field), &(body->field))
#define KEYTpKey(var, key)    std::make_tuple(CLASSKEY(var), &(key))
#define KEYField(body, field) std::make_tuple(CLASSKEY(field), &(body.field))
#define KEYPtrField(body, field) std::make_tuple(CLASSKEY(field), &(body->field))
	/// Value_RECT 처럼 struct의 멤버타입이 같지 않을때 tuple로 묶어서 처리
	template<typename TStruct, typename TVar>//FncSt, typename FncLd>
	void StructWithKeys_2(TStruct & rbj, wstring key
		, wstring sType, initializer_list<tuple<wstring, TVar>> prs
	)
	{
		auto& ar = *this;
		CJXArchive ari(ar, key.c_str());
		if (ari.IsStoring()) {
			for (auto& pr : prs)
				ari << pr;
			auto& jbji = ari.Jbj();
			if (sType.length())
				jbji.SetAttr(TAG_STC, sType);//L"POINT"
			ari.SaveToUpperNode();
			//ar.jbj().Set(ari._key) = ari._jData;//array of obj를 드디어 그 위 상위에 키로 넣는다.
		}
		else {
			auto& jbj = ar.Jbj();
			ari._jData = jbj.O(ari._key);
			auto& jbji = ari.Jbj();
			auto sTypeRead = jbji.S(TAG_STC);
			for (auto& pr : prs)
				ari >> pr;
		}
	}
	template<typename TContainer>
	void StdVect_POINT(TContainer & lstTObj, wstring key)// function<void(CJXArchive&, UcJObj&, TVal&)> cbCustom)
	{
		auto& ar = *this;
		using TVal = typename TContainer::value_type;
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {
			////if constexpr (std::is_pointer<TObj*>::value) {//std::cout << "Raw Pointer\n";
			auto& jbji = ari.Jbj();
			if (ari.IsStoring()) {
				ari << KEYTpKeyNC(r.x, x);//KEYVALTP(item.x);
				ari << KEYTpKeyNC(r.y, y);//KEYVALTP(item.y);
				jbji.SetAttr(TAG_STC, L"POINT");//StructWithKeys_2 참조
			}
			else {
				auto sTypeRead = jbji.S(TAG_STC);
				ASSERT(sTypeRead == L"POINT");
				ari >> KEYTpKeyNC(r.x, x);//KEYVALTP(item.x);
				ari >> KEYTpKeyNC(r.y, y);//KEYVALTP(item.y);
			}
			});
	}
	template<typename TContainer>
	void StdVect_PtrPOINT(TContainer & lstTObj, wstring key)// function<void(CJXArchive&, UcJObj&, TVal&)> cbCustom)
	{
		auto& ar = *this;
		using TVal = typename TContainer::value_type;            // POINT* 이미 포인터
		using TObject = std::remove_pointer_t<TVal>;             // POINT  포인터 제거
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {
			auto& jbji = ari.Jbj();
			if (ari.IsStoring()) {
				ari << KEYTpKeyNC(r->x, x);//KEYVALTP(item.x);
				ari << KEYTpKeyNC(r->y, y);//KEYVALTP(item.y);
				jbji.SetAttr(TAG_STC, L"POINT");//StructWithKeys_2 참조
			}
			else {
				if (!r)
					r = new TObject();//dwk: 2025-03-31 17:55 `new TVal` 하면 new POINT*() 가 되어 버림 -> 이건 POINT** 타입이 됨!
				auto sTypeRead = jbji.S(TAG_STC);
				ASSERT(sTypeRead == L"POINT");
				ari >> KEYTpKeyNC(r->x, x);//KEYVALTP(item.x);
				ari >> KEYTpKeyNC(r->y, y);//KEYVALTP(item.y);
			}
			});
	}
	// TVal 타입별 변환 헬퍼 함수
	template<typename TVal>
	typename std::enable_if<std::is_same<TVal, CString>::value || std::is_same<TVal, CStringW>::value || std::is_same<TVal, CStringA>::value, void>::type
		PairValFromJson(ShJVal shVal, TVal & val) {
		val = CString(shVal->Val()->AsString().c_str());
	}
	template<typename TVal>
	typename std::enable_if<std::is_same<TVal, std::wstring>::value || std::is_same<TVal, wstring>::value, void>::type
		PairValFromJson(ShJVal shVal, TVal & val) {
		val = std::wstring(shVal->Val()->AsString());
	}
	template<typename TVal>
	typename std::enable_if<std::is_integral<TVal>::value, void>::type
		PairValFromJson(ShJVal shVal, TVal & val) {
		val = static_cast<TVal>(shVal->Val()->AsInt64());
	}
	template<typename TVal>
	typename std::enable_if<std::is_floating_point<TVal>::value, void>::type
		PairValFromJson(ShJVal shVal, TVal & val) {
		val = static_cast<TVal>(shVal->Val()->AsDouble());
	}
	/// std::vector<std::pair<TKey, TVal>> 직렬화 (배열의 배열 형태: [[key1, val1], [key2, val2], ...])
	template<typename TContainer>
	void StdVect_Pair(TContainer & lstTObj, wstring key)
	{
		auto& ar = *this;
		using TPair = typename TContainer::value_type;
		using TKey = typename TPair::first_type;
		using TVal = typename TPair::second_type;
		StdTArrTStruct_2(lstTObj, key, [this](CJXArchive& ari, auto& r) {
			if (ari.IsStoring()) {
				// 배열 형태로 저장: [first, second]
				// ari는 이미 상위 배열의 각 항목을 위한 아카이브이므로, 배열을 JSON으로 설정
				ShJArr shArr = NEWSHP(UcJArr);
				shArr->Arr()->Add(ShJVal(new JVal(r.first)), false);
				shArr->Arr()->Add(ShJVal(new JVal(r.second)), false);
				// SetJson을 사용하여 ar3의 JSON을 배열로 설정 (StdTArrTStructCustom_LB4가 GetJson()을 배열에 추가함)
				ari.SetJson(shArr);
			}
			else {
				// 배열에서 읽기
				ShJObj shObj = ari.GetJson();
				ShJArr shArr;
				if (shObj->IsArr()) {
					shArr = shObj;
				}
				else if (shObj->IsDic()) {
					auto& jd = *shObj->Dic();
					if (jd.Has(TAG_ITM)) {
						auto shJit = jd.Get(TAG_ITM);
						if (shJit->IsArr())
							shArr = shJit;
					}
				}
				if (shArr && shArr->Arr()->size() >= 2) {
					auto& jarr = *shArr->Arr();
					// TKey 타입에 따라 변환
					if (std::is_integral<TKey>::value) {
						r.first = static_cast<TKey>(jarr[0]->Val()->AsInt64());
					}
					else {
						r.first = static_cast<TKey>(UcAtoi(jarr[0]->Val()->AsString().c_str()));
					}
					// TVal 타입에 따라 변환 - SFINAE를 사용하여 타입별로 처리
					PairValFromJson<TVal>(jarr[1], r.second);
				}
			}
			});
	}
	template<typename TContainer>
	void StdVect_ShpPOINT(TContainer & lstTObj, wstring key)// function<void(CJXArchive&, UcJObj&, TVal&)> cbCustom)
	{
		auto& ar = *this;
		using TVal = typename TContainer::value_type; // 자체가 shared_ptr 형
		using TObject = typename TVal::element_type; // shared_ptr의 내부 형
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {
			auto& jbji = ari.Jbj();
			if (ari.IsStoring()) {
				ari << KEYTpKeyNC(r->x, x);//KEYVALTP(item.x);
				ari << KEYTpKeyNC(r->y, y);//KEYVALTP(item.y);
				jbji.SetAttr(TAG_STC, L"POINT");//StructWithKeys_2 참조
			}
			else {
				if (!r)
					r = NEWSHP(TObject);//dwk: 2025-03-31 17:55  
				auto sTypeRead = jbji.S(TAG_STC);
				ASSERT(sTypeRead == L"POINT");
				ari >> KEYTpKeyNC(r->x, x);//KEYVALTP(item.x);
				ari >> KEYTpKeyNC(r->y, y);//KEYVALTP(item.y);
			}
			});
	}
	/// 3개(ptr, SHP, Val)를 하나로 합쳤지만 c++17에서만 가능 : constexpr
	template<typename TContainer>
	void StdTArr_POINT(TContainer & lstTObj, wstring key)
	{
		auto& ar = *this;
		using TVal = typename TContainer::value_type;
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {
			if (ari.IsStoring()) {
				ari.Value_POINT(VAR2STRNC(r));
			}
			else {
				if (!r) {
#if CPP17_OR_LATER
					if constexpr (std::is_pointer_v<TVal>) {
						r = new std::remove_pointer_t<TVal>();
					}
					else if constexpr (std::is_same_v<TVal, SHP<std::remove_pointer_t<TVal>>>) {
						r = NEWSHP(std::remove_pointer_t<TVal>);
					}
#else
					if (std::is_pointer<TVal>::value) {
						r = new typename std::remove_pointer<TVal>::type();
					}
					else if (std::is_same<TVal, SHP<typename std::remove_pointer<TVal>::type>>::value) {
						r = NEWSHP(typename std::remove_pointer<TVal>::type);
					}
#endif
				}
				ari.Value_POINT(VAR2STRNC(r));
			}
			});
	}
#ifdef _ToCustomStruct_Thinking__
	template<typename TStruct, typename TMbr>
	void Value_Struct(CJXArchive & ar, TStruct & rbj, wstring key, wstring sMbrType,
		std::initializer_list<std::tuple<wstring, TMbr>> prs)
		//vector<tuple<wstring, TMbr>> prs)//function<void(TStruct&)> cbMember)
	{
		vector<tuple<wstring, TMbr>> arV;
		for (const auto& a : prs)
			arV.push_back(a);
		//using PairType = std::tuple<std::wstring, decltype(&rbj.left)>;
		//std::vector<PairType> prs{
		//	KEYTpKeyNC(rbj.left  , left),
		//	KEYTpKeyNC(rbj.top   , top),
		//	KEYTpKeyNC(rbj.right , right),
		//	KEYTpKeyNC(rbj.bottom, bottom),
		//};
		StructWithKeys_2(rbj, key, sMbrType, prs);
	}
#endif // _ToCustomStruct_Thinking__
	template<typename TStruct>//, typename TMbr>
	void Value_RECT(TStruct & rbj, wstring key)
	{
		auto& ar = *this;
		StructWithKeys_2(rbj, key, L"RECT", {
			KEYTpKeyNC(rbj.left  , left),    //RECT.
			KEYTpKeyNC(rbj.top   , top),		//RECT.
			KEYTpKeyNC(rbj.right , right),	//RECT.
			KEYTpKeyNC(rbj.bottom, bottom),	//RECT.
			});
	}
	/// struct의 멤버타입이 같을때 tuple로 묶어서 처리
	template<typename TStruct>
	void Value_POINT(TStruct & rbj, wstring key)
	{
		auto& ar = *this;
		StructWithKeys_2(rbj, key, L"POINT", {
			KEYTpKeyNC(rbj.x, x),
			KEYTpKeyNC(rbj.y, y)
			});
		//<CXmlAppDoc.m_lpPoints __type__="POINT">
		//  <x>2</x>
		//  <y>3</y>
		//</CXmlAppDoc.m_lpPoints>
	}
	template<template<typename, typename> class TList, typename TObj>
	void MfcListPbj_2(TList<TObj*, TObj*>&lstTObj, wstring key
		, function<void(CJXArchive&, TObj*&)> CbSave
	)
	{
		auto& ar = *this;
		MfcListTPbj_step3<TList, TObj*>(lstTObj, key, CbSave,// CbSaveClassName,
			[](TList<TObj*, TObj*>& lst, TObj*& item) {
				lst.AddTail(item);
			},
			[](TList<TObj*, TObj*>& lst) {
				POSITION pos = lst.GetHeadPosition();
				while (pos != NULL) {
					TObj* pObj = lst.GetNext(pos);
					if (pObj)
						delete pObj;
				}
				lst.RemoveAll();
			},
			[](ShJVal shSub, WORD id) -> TObj* {
				return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			},
			[](CJXArchive& ari, TObj*& pItm) {dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari); }
		);
	}
	template<template<typename, typename> class TList, typename TObj>
	void MfcListPtrObj_1(TList<TObj*, TObj*>&lstTObj, wstring key
	) {
		auto& ar = *this;
		std::function<void(CJXArchive&, TObj*&)> CbSave = [](CJXArchive& ari, TObj*& pItm) {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, pItm);//MfcListPtrObj_1
			};
		MfcListPbj_2(lstTObj, key, CbSave);
	}
	/// TList는 MFC CArray<TObj*, TObj*> 또는 CList<TObj*, TObj*>
/// TObj가 CObject 기반이 아닌 경우 Serialize 멤버 함수가 없으므로
	template<template<typename, typename> class TList, typename TObj, typename TArg>
	void MfcArrayPtr_2(TList<TObj*, TArg*>&arrTObj, wstring key
		, function<void(CJXArchive&, TObj*&)> CbSave
		, function<void(CJXArchive&, TObj*&)> CbLoad
	)
	{
		auto& ar = *this;
		auto CbAdd = //[this](TList<TObj*, TObj*>& lst, TObj*& item) { lst.push_back(item); };
			[](TList<TObj*, TArg*>& lst, TObj*& item) {
			lst.Add(item);
			};
		auto CbClear = //[](TList<TObj*, TObj*>& lst) {
			[](TList<TObj*, TArg*>& lst) {
			for (auto& pObj : lst) {
				if (pObj)
					delete pObj;
			}
			lst.RemoveAll();
			};
		//lst.clear(); };
		auto CbFactory = [](ShJVal shSub, DWORD id) -> TObj* {
			return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			};
		MfcArrayPtr_step3<TList, TObj*, TArg*>(arrTObj, key,
			CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	template<template<typename, typename> class TList, typename TObj, typename TArg>
	void MfcArrayRef_2(TList<TObj, TArg>&arrTObj, wstring key//dwk: 2025-11-24 09:04 
		, function<void(CJXArchive&, TObj&)> CbSave
		, function<void(CJXArchive&, TObj&)> CbLoad
	)
	{
		auto& ar = *this;
		auto CbAdd = [](TList<TObj, TArg>& lst, TObj& item) {
			lst.Add(item);
			};
		auto CbClear = [](TList<TObj, TArg>& lst) {
			lst.RemoveAll();
			};
		auto CbFactory = [](ShJVal shSub, DWORD id) -> TObj {
			return TObj();// CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			};
		MfcArrayPtr_step3<TList, TObj, TArg>(arrTObj, key,
			CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	/// StdListShpObj_1 -> StdArrTPbj_step3 -> StdArrTPbj_LB4
	/// std::vector< SHP<TObj> > lstTObj;
	/// shared_ptr<TObj> 가 CObject 기반이므로 Serialize 멤버 함수가 있다.
	/// 저장은 class name 도 같이 저장 한다.
	template<template<typename> class TList, typename TObj>
	void StdListShpObj_1(TList<SHP<TObj>>&lstTObj, wstring key)
	{
		auto& ar = *this;
		auto CbSave = [](CJXArchive& ari, SHP<TObj>& pItm) {
			dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari);// CObject 기반이므로 Serialize 멤버 함수가 있다.
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, (SHP<TObj>)pItm);//StdListShpObj_1
			};// 저장은 class name 도 같이 저장 한다.
		auto CbAdd = [](TList<SHP<TObj>>& lst, SHP<TObj>& item) {lst.push_back(item); };// std vector, list 이므로 push_back이면 된다.
		auto CbClear = [](TList<SHP<TObj>>& lst) {	lst.clear(); };// std vector, list 이므로 clear이면 된다.
		auto CbFactory = [](ShJVal shSub, DWORD id) -> SHP<TObj> {
			auto pObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);//dwk: 2025-11-17 15:13 
			return shared_ptr<TObj>(pObj);// NEWSHP(TObj);
			};// SHP이므로 NEWSHP=make_shared로 할당한다.
		auto CbLoad = [](CJXArchive& ari, SHP<TObj>& pItm) {dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari); };//로드는 Serialize 멤버 함수로 한다.
		/// 여기서 overload오류 나면 위에 람다함수들을 function<> 으로 명시적으로 선언해 준다.
		StdArrTPbj_step3<TList, SHP<TObj>>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	/// std::list<SHP<TObj>> 오버로드 (allocator 파라미터 지원)
	template<typename TObj, typename... TArgs>
	void StdListShpObj_1(std::list<SHP<TObj>, TArgs...>&lstTObj, wstring key)
	{
		auto& ar = *this;
		auto CbSave = [](CJXArchive& ari, SHP<TObj>& pItm) {
			dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari);
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, (SHP<TObj>)pItm);
			};
		auto CbAdd = [](std::list<SHP<TObj>, TArgs...>& lst, SHP<TObj>& item) { lst.push_back(item); };
		auto CbClear = [](std::list<SHP<TObj>, TArgs...>& lst) { lst.clear(); };
		auto CbFactory = [](ShJVal shSub, DWORD id) -> SHP<TObj> {
			auto pObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			return shared_ptr<TObj>(pObj);
			};
		auto CbLoad = [](CJXArchive& ari, SHP<TObj>& pItm) { dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari); };
		StdArrTPbj_step3<std::list, SHP<TObj>, TArgs...>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	/// std::vector<SHP<TObj>> 오버로드 (allocator 파라미터 지원)
	template<typename TObj, typename... TArgs>
	void StdListShpObj_1(std::vector<SHP<TObj>, TArgs...>&lstTObj, wstring key)
	{
		auto& ar = *this;
		auto CbSave = [](CJXArchive& ari, SHP<TObj>& pItm) {
			dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari);
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, (SHP<TObj>)pItm);
			};
		auto CbAdd = [](std::vector<SHP<TObj>, TArgs...>& lst, SHP<TObj>& item) { lst.push_back(item); };
		auto CbClear = [](std::vector<SHP<TObj>, TArgs...>& lst) { lst.clear(); };
		auto CbFactory = [](ShJVal shSub, DWORD id) -> SHP<TObj> {
			auto pObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			return shared_ptr<TObj>(pObj);
			};
		auto CbLoad = [](CJXArchive& ari, SHP<TObj>& pItm) { dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari); };
		StdArrTPbj_step3<std::vector, SHP<TObj>>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	//template<template<typename> class TList, typename TObj>
	/// 이걸 쓰려면 복사 생성자와 operator= 가 제공 되어야 한다.
	/// COptMap(const COptMap& o) {}
	/// COptMap& COptMap::operator =(const COptMap&) { return *this; }
	template<template<typename, typename...> class TList, typename TObj>
	void StdListRefObj_1(TList<TObj>&lstTObj, wstring key)
	{
		auto& ar = *this;
		auto CbSave = [](CJXArchive& ari, TObj& pItm) {
			dynamic_cast<IDocSerialize*>(&pItm)->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, pItm);//StdListRefObj_1
			};
		auto CbAdd = [](TList<TObj>& lst, TObj& item) {	lst.push_back(item); };// std vector, list 이므로 push_back이면 된다.
		auto CbClear = [](TList<TObj>& lst) {	lst.clear(); };// std vector, list 이므로 clear이면 된다.
		auto CbFactory = [](ShJVal shSub, DWORD id) -> TObj {	return TObj(); };// SHP이므로 NEWSHP=make_shared로 할당한다.
		auto CbLoad = [](CJXArchive& ari, TObj& pItm) {dynamic_cast<IDocSerialize*>(&pItm)->Serialize(ari); };//로드는 Serialize 멤버 함수로 한다.
		StdArrTPbj_step3<TList, TObj>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);// CbSaveClassName,
	}
	//template<template<typename, typename...> class TList, typename TObj>
	//void StdListRefNoObj_1(CJXArchive& ar, TList<TObj>& lstTObj, wstring key)
	//{
	//	StdListRefObj_1<TList, TObj>(ar, lstTObj, key);
	//}
	/// std::vector< TObj* > lstTObj;
	//template<template<typename> class TList, typename TObj>
	template<template<typename, typename...> class TList, typename TObj, typename... TArgs>
	void StdListPtrObj_1(TList<TObj*>&lstTObj, wstring key)
	{
		auto& ar = *this;
		auto CbSave = [](CJXArchive& ari, TObj*& pItm) {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			CJXArchive::UcDefaultSaveClassName(ari.GetJson(), pItm);//StdListPtrObj_1
			};
		auto CbAdd = [](TList<TObj*>& lst, TObj*& item) {	lst.push_back(item); };
		auto CbClear = [](TList<TObj*>& lst) {	lst.clear(); };
		auto CbFactory = [](ShJVal shSub, DWORD id) -> TObj* {
			return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);//dwk: 2025-11-17 15:13 
			};
		auto CbLoad = [](CJXArchive& ari, TObj*& pItm) {dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari); };
		StdArrTPbj_step3<TList, TObj*>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
		//StdArrTPbj_step3<TList, TObj*>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	/// std::vector<TObj*> 오버로드 (allocator 파라미터 지원)
	template<typename TObj, typename... TArgs>
	void StdListPtrObj_1(std::vector<TObj*, TArgs...>&lstTObj, wstring key)
	{
		auto& ar = *this;
		auto CbSave = [](CJXArchive& ari, TObj*& pItm) {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);
			CJXArchive::UcDefaultSaveClassName(ari.GetJson(), pItm);
			};
		auto CbAdd = [](std::vector<TObj*, TArgs...>& lst, TObj*& item) { lst.push_back(item); };
		auto CbClear = [](std::vector<TObj*, TArgs...>& lst) { lst.clear(); };
		auto CbFactory = [](ShJVal shSub, DWORD id) -> TObj* {
			return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			};
		auto CbLoad = [](CJXArchive& ari, TObj*& pItm) { dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari); };
		StdArrTPbj_step3<std::vector, TObj*>(lstTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	//DWKREMINDER("template - std collection 객체는 typename... TArgs 로 해야 된다.")
		//template<template<typename> class TList, typename TObj>
		//template<template<typename, typename...> class TList, typename TObj, typename... TArgs>
		//void StdListPtrNoObj_1(CJXArchive& ar, TList<TObj*>& lstTObj, wstring key)
		//{
		//	StdListPtrObj_1<TList, TObj>(ar, lstTObj, key);
		//	/// DWKREMINDER("CObject아니면 DocSerialize만 달렸는데, 일반 class에도 Serialize를 넣기로 헀다..");
		//}
		/// MFC CArray<TObj*, TObj*> 용 (직접 지원)
		/// MfcArrayOfPbjToJsonWithKey -> MfcArrayOfPbjToJsonWithKeyTmp -> MfcArrayOfObjSerializeLB
	template<template<typename, typename> class TList, typename TObj>
	void MfcArrayPtrObj_1(TList<TObj*, TObj*>&arrTObj, wstring key)
	{
		auto& ar = *this;
		/// std::function<void(TObj*, CJXArchive&)>를 auto 로만 해도 오러로드 함수 없다고 C2672 에러가 난다.
		std::function<void(CJXArchive&, TObj*&)> CbSave = [](CJXArchive& ari, TObj*& pItm) -> void {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, pItm);//MfcArrayPtrObj_1
			};
		std::function<void(CJXArchive&, TObj*&)> CbLoad = [](CJXArchive& ari, TObj*& pItm) -> void {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			};
		MfcArrayPtr_2(arrTObj, key, CbSave, CbLoad);///std::function 로 만들어서 줘야 추론 실패	안함
	}
	/// MFC CArray<SHP<TObj>, SHP<TObj>&> 용 (shared_ptr 배열)
	/// MfcArrayOfPbjToJsonWithKey -> MfcArrayOfPbjToJsonWithKeyTmp -> MfcArrayOfObjSerializeLB
	template<template<typename, typename> class TList, typename TObj, typename TArg>
	void MfcArrayShpObj_1(TList<SHP<TObj>, TArg>&arrTObj, wstring key)
	{
		auto& ar = *this;
		std::function<void(CJXArchive&, SHP<TObj>&)> CbSave = [](CJXArchive& ari, SHP<TObj>& pItm) -> void {
			dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, pItm);
			};
		std::function<void(CJXArchive&, SHP<TObj>&)> CbLoad = [](CJXArchive& ari, SHP<TObj>& pItm) -> void {
			dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			};
		auto CbAdd = [](TList<SHP<TObj>, TArg>& lst, SHP<TObj>& item) {
			lst.Add(item);
			};
		auto CbClear = [](TList<SHP<TObj>, TArg>& lst) {
			lst.RemoveAll();
			};
		auto CbFactory = [](ShJVal shSub, DWORD id) -> SHP<TObj> {
			auto pObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
			return shared_ptr<TObj>(pObj);
			};
		MfcArrayPtr_step3<TList, SHP<TObj>, TArg>(arrTObj, key,
			CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}

	//template<template<typename, typename> class TList, typename TObj>
	//void MfcArrayRefObj_1(TList<TObj, TObj>& arrTObj, wstring key)//dwk: 2025-11-24 09:04 
	//{
	//	auto& ar = *this;
	//	/// std::function<void(TObj*, CJXArchive&)>를 auto 로만 해도 오러로드 함수 없다고 C2672 에러가 난다.
	//	std::function<void(CJXArchive&, TObj&)> CbSave = [](CJXArchive& ari, TObj& pItm) -> void {
	//		pItm.Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
	//		auto shSub = ari.GetJson();
	//		CJXArchive::UcDefaultSaveClassName(shSub, pItm);//MfcArrayPtrObj_1
	//		};
	//	std::function<void(CJXArchive&, TObj&)> CbLoad = [](CJXArchive& ari, TObj& pItm) -> void {
	//		pItm.Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
	//		};
	//	MfcArrayRef_2(arrTObj, key, CbSave, CbLoad);///std::function 로 만들어서 줘야 추론 실패	안함
	//}
	// CArray는 TYPE, ARG_TYPE 두 개 필요
	// CObject 기반 클래스는 복사 생성자가 없어서 CArray에 직접 저장할 수 없으므로 제외
	template<template<typename, typename> class TList, typename TObj, typename TArg>
	typename std::enable_if<!std::is_base_of<CObject, TObj>::value>::type
		MfcArrayRefObj_1(TList<TObj, TArg>&arrTObj, wstring key)
	{
		auto& ar = *this;
		std::function<void(CJXArchive&, TObj&)> CbSave = [](CJXArchive& ari, TObj& pItm) {
			dynamic_cast<IDocSerialize*>(&pItm)->Serialize(ari);
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, pItm);
			};
		std::function<void(CJXArchive&, TObj&)> CbLoad = [](CJXArchive& ari, TObj& pItm) {
			dynamic_cast<IDocSerialize*>(&pItm)->Serialize(ari);
			};
		MfcArrayRef_2(arrTObj, key, CbSave, CbLoad);
	}
	/// TList는 MFC CArray<TObj*, TObj*> 또는 CList<TObj*, TObj*>
	/// TObj가 _variant_* Serialize 멤버 함수가 없으므로
	template<template<typename, typename> class TList, typename TObj>
	void MfcArrayPtrVariant_1(TList<TObj*, TObj*>&arrTObj, wstring key)
	{
		auto& ar = *this;
		function<void(CJXArchive&, TObj*&)> CbSave = [](CJXArchive& ari, TObj*& pItm) {
			ari.ArcVariant_impl2(pItm);
			};
		function<void(CJXArchive&, TObj*&)> CbLoad = [](CJXArchive& ari, TObj*& pItm) {
			ari.ArcVariant_impl2(pItm);
			};
		MfcArrayPtr_2(arrTObj, key, CbSave, CbLoad);
	}
	template<template<typename> class TArr1, template<typename> class TArr2, typename TObj>
	void StdTArrShp2D_1(TArr1<SHP<TArr2<SHP<TObj>>>>&lstTArr, wstring key)
	{
		auto& ar = *this;
		DWKFUNC;
		// key : [ [v,v],[v,v],[v,v] ]
		try
		{
			CJXArchive ar2(ar, key.c_str());
			if (ar.IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				auto& jarrNew = *shArrNew->Arr();
				for (auto& shArr3 : lstTArr)//shArr3	SHP<KArray<SHP<CMyData>>> &
				{
					auto& lstTObj3 = *shArr3;
					CJXArchive ar3(ar2);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
					ShJArr shJArr3 = NEWSHP(UcJArr);
					StdTArrTPobj_step3(ar3, shJArr3, lstTObj3);// shArr <= lstTObj
					//auto shJrr3 = ar3.GetJson();//데이터가 고스란히 ar3._jdata에 들어 있다.
					jarrNew.push_back(shJArr3);//ar2는 스택변수니 참조 해가도 된다.
				}//array를 완성한다.
				//ar.Jbj()(ar2._key) = shArrNew;//array of obj를 드디어 그 위 상위에 키로 넣는다.
				ar2.SaveToUpperNode(shArrNew);
			}
			else {// List<obj> load test
				lstTArr.clear();//일단 비우고 시작
				auto& j1 = ar.Jbj();
				//auto& jarr = j1.GetArray(key.c_str()); 이거는 throw한다.
				auto shArr = j1.Array(key);
				if (shArr)
				{
					auto& jarr = *shArr->Arr();
					CJXArchive ar2(ar, key.c_str());
					for (auto& shjv3 : jarr) {/// 1차 JSON 배열 읽어서
						CJXArchive ar3(ar2);//key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
						ar3.SetJson(shjv3);// input: JSON
						auto shArr3 = NEWSHP(TArr2<SHP<TObj>>); /// 각 항목 2차 배열 생성
						StdTArrTPobj_step3(ar3, shjv3, *shArr3);/// 각 항목을 2차 배열에 담아준다. // shArr <= lstTObj3
						lstTArr.push_back(shArr3); /// 각 항목 2차 배열을 1차 배열에 추가 한다.
					}
				}
				else {
					DWKTRACE(L"key(%v)에 Array가 없네?", key);		//ASSERT(0 == "key에 Array가 없네?");
				}
			}
		}
		catch (CException* e) {
			e;
			throw;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr);
			throw;
		}
	}
	template<template<typename> class TList, typename TVAL>
	void ListToJsonWithKey(CJXArchive & ar, TList<TVAL>&lstTObj, wstring key)
	{
		DWKFUNC;
		try
		{
			CJXArchive ar2(ar, key.c_str());
			if (ar.IsStoring()) {
				ShJArr shArr = NEWSHP(UcJArr);
				auto& jarr = *shArr->Arr();
				for (TVAL& val : lstTObj)
				{
					jarr.Add(val, false);//ar2는 스택변수니 참조 해가도 된다.
				}//array를 완성한다.
				ar.Jbj()(ar2._key) = shArr;//array of obj를 드디어 그 위 상위에 키로 넣는다.
			}
			else {// List<obj> load test
				lstTObj.clear();
				auto& j1 = ar.Jbj();
				auto& jarr = j1.GetArray(key.c_str());
				CJXArchive ar2(ar, key.c_str());
				for (auto& jv : jarr) {
					auto& val = *jv->Val();
					if (val->IsNumber())
					{
						if (val->IsDouble())
							lstTObj.push_back((TVAL)val->AsDouble());// TObj to list
						else
							lstTObj.push_back((TVAL)val->AsInt64());// TObj to list
					}
					else if (val->IsString())
						lstTObj.push_back((TVAL)val->AsString());// TObj to list
					else {
						ASSERT(0 == "JSON이 숫자도 문자열도 아니면 뭔디?");
					}
				}
			}
		}
		catch (CException* e) {
			throw;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr);
			throw;
		}
	}
	//template<template<typename, typename> class TMap, typename TKey, typename TPObj>
	template<typename TKey, typename TPObj>
	void StdMapPtr_LB4(ShJObj shJbj, std::map<TKey, TPObj>&mapTObj, wstring key
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(std::map<TKey, TPObj>&, TKey, TPObj&)> cbAddItem
		, function<void(std::map<TKey, TPObj>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, TKey, TKey)> cbFactory
		, function<void(CJXArchive&, TPObj&)> CbLoad
	)
	{
		auto& ar = *this;
		//CJXArchive ar2(*this, key.c_str());
		if (this->IsStoring())// CArchive::load == 1 과 값을 맞추기 위해 bStore를 쓰지 않는다.
		{
			//ShJObj newShJbj = NEWSHP(UcJObj);
			for (auto& pair : mapTObj) {
				auto& key2 = pair.first;
				auto& pItem = pair.second;
				CJXArchive arcItm(*this);///step 1: 저장 할 Archive 생성
				CbSave(arcItm, pItem);
				shJbj->Dic()->SetObj(key2, arcItm.GetJson(), false);///step 3: 각 키로 넣는다.
			}//UcJObj를 완성한다.
		}
		else
		{
			CbClear(mapTObj);
			auto& jdata = this->Jbj();
			auto shObj = jdata.Obj(key.c_str());//key = L"_mapObj"
			if (shObj) {//이미 읽은 데이터
				ASSERT(shObj->IsDic());
				auto& jbj = *shObj->Dic();
				for (auto& pair : jbj) {
					auto& key2 = pair.first;
					auto& shjv = pair.second;
					CJXArchive arcItm(*this, shjv);///step 1: 로드할 Archive 생성. 각 객체별shObj2 Serialize하기 위해 임시 Archive 필요
					TPObj newPbj = cbFactory(shjv, key, key2);///step 3:  NEWSHP(TObj);/// new TObj 여기서 람다 함수로 []() -> shared_ptr 로 리턴 하게
					CbLoad(arcItm, newPbj);//pObj3->Serialize(ar2);///shObj3(JSON) to TObj의 각 멤버변수로 들어 간다.
					cbAddItem(mapTObj, key2, newPbj);//mapTObj[key2] = pObj3;// TObj to list
				}
			}
			else {
				//DWKTRACE(L"dwk key(%v)에 JObj가 없네?", key);
				ASSERT(0 == "key가 없네?");
			}
		}
	}
	/// key가 int 경우 CMapWordToPtr 로 변환해서 저장/로드 한다.
//[deprecated]
	//template<template<typename, typename, typename, typename> class TMap, typename TKey, typename TKeyA, typename TJKey, typename TPObj>
	template<typename TKey, typename TKeyA, typename TJKey, typename TPObj>
	void MfcMapPtr_LB4_deprecated(CJXArchive & ar, ShJObj shJbj, CMap<TKey, TKeyA, TPObj, TPObj>&mapTObj, TJKey key
		, function<void(CJXArchive&, TPObj)> CbSave
		, function<void(CMap<TKey, TKeyA, TPObj, TPObj>&, TJKey, TPObj)> cbAddItem
		, function<void(CMap<TKey, TKeyA, TPObj, TPObj>&)> CbClear // load때만 쓰인다.
		, function<TPObj(ShJVal, TJKey, TJKey)> cbFactory
		, function<void(CJXArchive&, TPObj)> CbLoad
	)
	{
		//CJXArchive ar2(*this, key.c_str());
		ASSERT(&ar == this);
		if (this->IsStoring())// CArchive::load == 1 과 값을 맞추기 위해 bStore를 쓰지 않는다.
		{
			//ShJObj newShJbj = NEWSHP(UcJObj);
			//for (auto& pair : mapTObj) {
			TKey key2;
			TPObj pItem{ nullptr };
			for (POSITION pos = mapTObj.GetStartPosition(); pos != NULL; ) {
				CJXArchive arcItm(*this);///step 1: 저장 할 Archive 생성
				mapTObj.GetNextAssoc(pos, key2, pItem);//이거 for뒤에 넣으면 첫회 때 널이야
				CbSave(arcItm, pItem);
				shJbj->Dic()->SetObj(key2, arcItm.GetJson(), false);///step 3: 각 키로 넣는다.
			}//UcJObj를 완성한다.
		}
		else
		{
			CbClear(mapTObj);
			auto& jdata = this->Jbj();
			auto shObj = jdata.Obj(key.c_str());//key = L"_mapObj"
			if (shObj) {//이미 읽은 데이터
				ASSERT(shObj->IsDic());
				auto& jbj = *shObj->Dic();
				for (auto& pair : jbj) {
					auto& key2 = pair.first;
					auto& shjv = pair.second;
					CJXArchive arcItm(*this, shjv);///step 1: 로드할 Archive 생성. 각 객체별shObj2 Serialize하기 위해 임시 Archive 필요
					TPObj newPbj = cbFactory(shjv, key, key2);///step 3:  NEWSHP(TObj);/// new TObj 여기서 람다 함수로 []() -> shared_ptr 로 리턴 하게
					CbLoad(arcItm, newPbj);//pObj3->Serialize(ar2);///shObj3(JSON) to TObj의 각 멤버변수로 들어 간다.
					cbAddItem(mapTObj, key2, newPbj);//mapTObj[key2] = pObj3;// TObj to list
				}
			}
			else {
				//DWKTRACE(L"dwk key(%v)에 JObj가 없네?", key);
				ASSERT(0 == "key가 없네?");
			}
		}
	}
	/// <summary>
	/// map에 값이 shared_ptr 또는 순수 포인터 둘다 제공.
	/// </summary>
	/// <typeparam name="TObj"></typeparam>
	/// <param name="ar"></param>
	/// <param name="mapTObj"></param>
	/// <param name="key"></param>
	/// <param name="newCB">값을 생성 하는 람다 함수 주로 new 또는 make_shared 를 포함한 함수</param>
	template<template<typename, typename> class TMap, typename TKey, typename TPObj>
	void StdMapTPbj_step3_1(TMap<TKey, TPObj>&mapTObj, TKey key
		, function<void(CJXArchive&, TPObj)> CbSave
		, function<void(TMap<TKey, TPObj>&, TKey, TPObj)> CbAdd
		, function<void(TMap<TKey, TPObj>&)> CbClear
		, function<TPObj(ShJVal, TKey, TKey)> CbFactory//keyUp, keyItem
		, function<void(CJXArchive&, TPObj)> CbLoad
	)
	{
		DWKFUNC;
		try
		{
			auto& ar = *this;
			CJXArchive ar2(*this, key.c_str());///key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
			if (ar.IsStoring()) {
				auto newShJbj = NEWSHP(UcJObj);
				StdMapPtr_LB4<TMap, TKey, TPObj>(ar, newShJbj, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
				//this->Jbj()(key) = newShJbj;//array of obj를 드디어 그 위 상위에 키로 넣는다.
				//this->Jbj().Set(key, make_shared<JVal>(newShJbj, false));
				//this->Jbj().SetObj(key, newShJbj, false);// 복사하지 않는다.
				ar2.SaveToUpperNode(newShJbj);
				ASSERT(key == ar2._key);
			}
			else {/// 여기서 이미 파일 읽어서 UcJObj에 들어 있는 상태에서 각 멤버로 옮기는 작업을 한다.
				mapTObj.clear();
				auto& jdata = ar.Jbj();
				auto shObj = jdata.Obj(key.c_str());//key = L"_mapObj"
				if (shObj) {
					StdMapPtr_LB4<TMap, TKey, TPObj>(ar, shObj, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
				}
				else {
					DWKTRACE(L"dwk key(%v)에 JObj가 없네?", key);
				}
			}
		}
		catch (CException* e) {
			e;
			throw;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr);
			throw;
		}
	}
	template<typename TKey, typename TPObj>
	void StdMapTPbj_step3(std::map<TKey, TPObj>&mapTObj, TKey key
		, function<void(CJXArchive&, TPObj&)> CbSave
		, function<void(std::map<TKey, TPObj>&, TKey, TPObj&)> CbAdd
		, function<void(std::map<TKey, TPObj>&)> CbClear
		, function<TPObj(ShJVal, TKey, TKey)> CbFactory//keyUp, keyItem
		, function<void(CJXArchive&, TPObj&)> CbLoad
	)
	{
		auto& ar = *this;
		DWKUSETRACE;
		try
		{
			CJXArchive ar2(*this, key.c_str());///key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
			if (ar.IsStoring()) {
				auto newShJbj = NEWSHP(UcJObj);
				StdMapPtr_LB4<TKey, TPObj>(newShJbj, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
				//this->Jbj()(key) = newShJbj;//array of obj를 드디어 그 위 상위에 키로 넣는다.
				//this->Jbj().SetObj(key, newShJbj, false);// 복사하지 않는다.
				ar2.SaveToUpperNode(newShJbj);
				//this->Jbj().Set(key, make_shared<JVal>(newShJbj, false));
				ASSERT(key == ar2._key);
			}
			else {/// 여기서 이미 파일 읽어서 UcJObj에 들어 있는 상태에서 각 멤버로 옮기는 작업을 한다.
				mapTObj.clear();
				auto& jdata = ar.Jbj();
				auto shObj = jdata.Obj(key.c_str());//key = L"_mapObj"
				if (shObj) {
					StdMapPtr_LB4<TKey, TPObj>(shObj, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
				}
				else {
					DWKTRACE(L"dwk key(%v)에 JObj가 없네?", key);
				}
			}
		}
		catch (CException* e) {
			e;
			throw;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr); serr;
			throw;
		}
	}
	/// map에 값이 순수 포인터 인 경우
	/// typename TPObj = TObj*> 이 방법은 추론 실패 한다. using 방법은 된다.
		//template<template<typename, typename> class TMap, typename TKey = wstring, typename TObj>
		//void StdMapPtrObj_1(CJXArchive& ar, TMap<TKey, TObj*>&mapTObj, TKey key)
	template<typename TKey = wstring, typename TObj>
	void StdMapPtrObj_1(std::map<TKey, TObj*>&mapTObj, const TKey & key)
	{
		using TPObj = TObj*;
		//using TMap = std::map<TKey, TPObj>;
		//DEFAULT_LAMBDA_FOR_std_MAP();
		auto CbSave = [](CJXArchive& ari, TPObj pItm) {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);
			CJXArchive::UcDefaultSaveClassName(ari.GetJson(), pItm);//StdMapPtrObj_1
			};
		auto CbAdd = [](std::map<TKey, TPObj>& lst, TKey kItm, TPObj& item) {	lst[kItm] = item; };
		auto CbClear = [](std::map<TKey, TPObj>& lst) {	lst.clear(); };
		auto CbLoad = [](CJXArchive& ari, TPObj& pItm) {dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari); };
		auto CbFactory = [](ShJVal shSub, TKey k1, TKey k2) -> TPObj {
			return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, k1, k2);//dwk: 2025-11-17 15:13 
			//return new TObj(); //아래 SHP<TObj> 와 이 줄만 다름.
			};
		StdMapTPbj_step3<TKey, TPObj>(mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	template<typename TKey = wstring, typename TObj>
	void StdMapRefObj_1(std::map<TKey, TObj>&mapTObj, const TKey & key)
	{
		using TPObj = TObj;
		auto CbSave = [](CJXArchive& ari, TPObj& pItm) {
			dynamic_cast<IDocSerialize*>(&pItm)->Serialize(ari);
			CJXArchive::UcDefaultSaveClassName(ari.GetJson(), pItm);//StdMapRefObj_1
			};
		auto CbAdd = [](std::map<TKey, TPObj>& lst, TKey kItm, TPObj& item) {	lst[kItm] = item; };
		auto CbClear = [](std::map<TKey, TPObj>& lst) {	lst.clear(); };
		auto CbLoad = [](CJXArchive& ari, TPObj& pItm) {	dynamic_cast<IDocSerialize*>(&pItm)->Serialize(ari); };
		auto CbFactory = [](ShJVal shSub, TKey k1, TKey k2) -> TPObj {
			return TPObj(); /// TPObj()는 이미 우측값이므로 std::move는 불필요합니다.
			//포인트가 아니니 UcDefaultCreateByClassName 안 부름.
			//return UcDefaultCreateByClassName<TObj>(shSub, k1, k2);//dwk: 2025-11-17 15:13 
			};
		StdMapTPbj_step3<TKey, TPObj>(mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	/// map에 값이 shared_ptr 인 경우
	//template<template<typename, typename> class TMap, typename TKey, typename TObj>
	template<typename TKey = wstring, typename TObj>
	void StdMapShpObj_1(std::map<TKey, SHP<TObj>>&mapTObj, TKey key)
	{
		using TPObj = SHP<TObj>;
		//DEFAULT_LAMBDA_FOR_MAP();
		auto CbSave = [](CJXArchive& ari, TPObj pItm) {
			dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari);
			CJXArchive::UcDefaultSaveClassName(ari.GetJson(), pItm);
			};
		auto CbAdd = [](std::map<TKey, TPObj>& lst, TKey kItm, TPObj item) {	lst[kItm] = item; };
		auto CbClear = [](std::map<TKey, TPObj>& lst) {	lst.clear(); };
		auto CbLoad = [](CJXArchive& ari, TPObj pItm) {dynamic_cast<IDocSerialize*>(pItm.get())->Serialize(ari); };
		auto CbFactory = [](ShJVal shSub, TKey k1, TKey k2) -> TPObj {
			auto pObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, k1, k2);//dwk: 2025-11-17 15:13 
			return shared_ptr<TObj>(pObj);// NEWSHP(TObj);
			};
		StdMapTPbj_step3<TKey, TPObj>(mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
	/// std::map<wstring, COleVariant*> 처리 (VARIANT 포인터 맵)
	template<typename TVariant = COleVariant>
	void StdMapStrVariantCnv(std::map<wstring, TVariant*>&mapVariant, const wstring & key)
	{
		using TPObj = TVariant*;
		using TKey = wstring;
		auto CbSave = [](CJXArchive& ari, TPObj& pItm) {
			ari.ArcVariant_impl2(pItm);  // VARIANT 처리
			};
		auto CbAdd = [](std::map<TKey, TPObj>& lst, TKey kItm, TPObj& item) {
			lst[kItm] = item;
			};
		auto CbClear = [](std::map<TKey, TPObj>& lst) {
			for (auto& pr : lst) {
				if (pr.second)
					delete pr.second;
			}
			lst.clear();
			};
		auto CbLoad = [](CJXArchive& ari, TPObj& pItm) {
			ari.ArcVariant_impl2(pItm);  // VARIANT 처리
			};
		auto CbFactory = [](ShJVal shSub, TKey k1, TKey k2) -> TPObj {
			return new TVariant();  // 새 VARIANT 생성
			};
		StdMapTPbj_step3<TKey, TPObj>(mapVariant, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
	}
#define ArcStdMapStrVariantCnv(ar, v) ar.StdMapStrVariantCnv(VAR2STR(v))
	/// std::map<wstring, TVariant*> 저장 (키를 wstring으로 변환하여 처리)
#define Arc_StdMapStrVariantCnv(v) ArcStdMapStrVariantCnv(ar, v)
	/// CMap<TKey, TKeyA, TVariant*, TVariant*>를 std::map<wstring, TVariant*>로 변환 (키를 wstring으로)
	template<typename TKey, typename TKeyA, typename TVariant, typename TJKey>
	std::map<wstring, TVariant*> MfcToStdMapStrVariant(CMap<TKey, TKeyA, TVariant*, TVariant*>&mapMfc)
	{
		std::map<wstring, TVariant*> stdMap;
		TKey key;
		TVariant* pVal;
		for (POSITION pos = mapMfc.GetStartPosition(); pos != NULL; ) {
			mapMfc.GetNextAssoc(pos, key, pVal);
			wstring keyW = assign_wstring(key);  // 키를 wstring으로 변환
			stdMap[keyW] = pVal;
		}
		return stdMap;
	}
	/// std::map<wstring, TVariant*>를 CMap<TKey, TKeyA, TVariant*, TVariant*>로 역변환 (wstring을 원래 키 타입으로)
	template<typename TKey, typename TKeyA, typename TVariant, typename TJKey>
	void StdToMfcMapStrVariant(std::map<wstring, TVariant*>&stdMap, CMap<TKey, TKeyA, TVariant*, TVariant*>&mapMfc,
		function<TKey(const wstring&)> cbCnvKeyOut)  // wstring -> 원래 키 타입 변환 함수
	{
		mapMfc.RemoveAll();
		for (auto& pr : stdMap) {
			TKey key = cbCnvKeyOut(pr.first);  // wstring을 원래 키 타입으로 변환
			TKeyA keyA(key);  // TKeyA로 변환 (CMap의 두 번째 템플릿 인자)
			mapMfc.SetAt(keyA, pr.second);
		}
	}
	/// CMap<TKey, TKeyA, TVariant*, TVariant*> 처리 (키를 wstring으로 변환하여 처리)
	template<typename TKey, typename TKeyA, typename TVariant, typename TJKey>
	void MfcMapVariantCnv(CMap<TKey, TKeyA, TVariant*, TVariant*>&mapMfc, TJKey key)
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			// CMap을 std::map<wstring, TVariant*>로 변환
			auto stdMap = MfcToStdMapStrVariant<TKey, TKeyA, TVariant, TJKey>(mapMfc);
			// std::map을 처리
			StdMapStrVariantCnv<TVariant>(stdMap, key);
		}
		else {
			// std::map<wstring, TVariant*>로 로드
			std::map<wstring, TVariant*> stdMap;
			StdMapStrVariantCnv<TVariant>(stdMap, key);
			// wstring을 원래 키 타입으로 변환하는 함수
			function<TKey(const wstring&)> cbCnvKeyOut = [](const wstring& ws) -> TKey {
				return static_cast<TKey>(UcAtoi(ws.c_str()));  // int, WORD 등 숫자 타입
				};
			// std::map을 CMap으로 역변환
			StdToMfcMapStrVariant<TKey, TKeyA, TVariant, TJKey>(stdMap, mapMfc, cbCnvKeyOut);
		}
	}
#define ArcMfcMapVariantCnv(ar, v) ar.MfcMapVariantCnv(VAR2STR(v))
	/// CMap<TKey, TKeyA, TVariant*, TVariant*> 저장 (키를 wstring으로 변환하여 처리)
#define Arc_MfcMapVariantCnv(v) ArcMfcMapVariantCnv(ar, v)
#ifdef _DEBUGx
#endif // _DEBUGx
	/// key가 int 경우 CMapWordToPtr 로 변환해서 저장/로드 한다.
//[deprecated]
	//template<template<typename, typename, typename, typename> class TMap, typename TKey, typename TKeyA, typename TJKey, typename TPObj>
	template<typename TKey, typename TKeyA, typename TJKey, typename TPObj>
	void MfcMapTPbj_step3_deprecated(CJXArchive & ar, CMap<TKey, TKeyA, TPObj, TPObj>&mapTObj, TJKey key
		, function<void(CJXArchive&, TPObj)> CbSave
		, function<void(CMap<TKey, TKeyA, TPObj, TPObj>&, TJKey, TPObj)> CbAdd
		, function<void(CMap<TKey, TKeyA, TPObj, TPObj>&)> CbClear
		, function<TPObj(ShJVal, TJKey, TJKey)> CbFactory//keyUp, keyItem
		, function<void(CJXArchive&, TPObj)> CbLoad
	)
	{
		DWKFUNC;
		try
		{
			auto& ar = *this;
			CJXArchive ar2(*this, key.c_str());///key를 주면 상위 jbj 넣는다. 안주면 아무 짓도 안한다.
			if (ar.IsStoring()) {
				auto newShJbj = NEWSHP(UcJObj);
				MfcMapPtr_LB4_deprecated(ar, newShJbj, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
				//this->Jbj()(key) = newShJbj;//array of obj를 드디어 그 위 상위에 키로 넣는다.
				ar2.SaveToUpperNode(newShJbj);
				//this->Jbj().Set(key, make_shared<JVal>(newShJbj, false));
				ASSERT(key == ar2._key);
			}
			else {/// 여기서 이미 파일 읽어서 UcJObj에 들어 있는 상태에서 각 멤버로 옮기는 작업을 한다.
				CbClear(mapTObj);// .clear();
				auto& jdata = ar.Jbj();
				auto shObj = jdata.Obj(key.c_str());//key = L"_mapObj"
				if (shObj) {
					MfcMapPtr_LB4_deprecated(ar, shObj, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
				}
				else {
					DWKTRACE(L"dwk key(%v)에 JObj가 없네?", key);
				}
			}
		}
		catch (CException* e) {
			e;
			throw;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr);
			throw;
		}
	}
	/// key가 int 경우 CMapWordToPtr 로 변환해서 저장/로드 한다.
	template<typename TKey, typename TKeyA, typename TJKey, typename TObj>
	void MfcMapPtrObj_1(CMap<TKey, TKeyA, TObj*, TObj*>&mapTObj, TJKey key)
	{
		auto& ar = *this;
		using TPObj = TObj*;
		if (ar.IsStoring()) {
			auto stdMapToStore = MfcToStdMapStr<TKey, TKeyA, TObj*, TObj*, TJKey>(mapTObj);
			StdMapPbjCnv(stdMapToStore, key);
		}
		else {
			std::map<TKey, TObj*> mapLoaded;
			/// 중요: overload 추론 실패 원인 -> 명시적 형변환 function<TKey(const wstring&)>
			///		auto를 쓰거나, 람다로 바로 넣는 경우 실패 함.
			function<TKey(const wstring&)> cbCnvKeyOut = [](const wstring& ws) -> TKey {
				return CStringW(ws.c_str());
				};
			StdMapPbjCnv(mapLoaded, key, cbCnvKeyOut);//[](const wstring& ws) -> TKey { return CStringW(ws.c_str()); });
			MfcToStdMapLoading<TKey, TKeyA, TObj*, TObj*, TJKey>(mapLoaded, mapTObj);
		}
	}
	//[deprecated]
//template<template<typename, typename, typename, typename> class TMap, typename TKey, typename TKeyA, typename TJKey, typename TObj>
	template<typename TKey, typename TKeyA, typename TJKey, typename TObj>
	void MfcMapPtrObj_1_deprecated(CJXArchive & ar, CMap<TKey, TKeyA, TObj*, TObj*>&mapTObj, TJKey key)
	{
		using TPObj = TObj*;
		function<void(CJXArchive&, TPObj)> CbSave = [](CJXArchive& ari, TPObj pItm) {
			dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari);				/// CObject 기반이므로 Serialize 멤버 함수가 있다.
			auto shSub = ari.GetJson();
			CJXArchive::UcDefaultSaveClassName(shSub, pItm);//CObbject IMPLEMENT_DYNCREATE 기반이면 class name 도 같이 저장 한다.
			};
		function<void(CMap<TKey, TKeyA, TPObj, TPObj>&, TJKey, TPObj) > CbAdd
			= [](CMap<TKey, TKeyA, TPObj, TPObj>& mapWP, TJKey key, TPObj item) {
			auto keyw = TKeyA(key.c_str());// std::to_wstring(key);
			mapWP.SetAt(keyw, item);
			};
		function<void(CMap<TKey, TKeyA, TPObj, TPObj>&)> CbClear = [](CMap<TKey, TKeyA, TPObj, TPObj>& mapWP) {
			TKey nid;
			TPObj ptr{ nullptr };
			for (POSITION pos = mapWP.GetStartPosition(); pos != NULL; ) {
				mapWP.GetNextAssoc(pos, nid, ptr);
				if (ptr) {
					delete ptr;
					ptr = nullptr;
				}
			}
			mapWP.RemoveAll();
			};
		function<void(CJXArchive&, TPObj)> CbLoad = [](CJXArchive& ari, TPObj pItm) {dynamic_cast<IDocSerialize*>(pItm)->Serialize(ari); };
		function<TPObj(ShJVal, TJKey, TJKey)> CbFactory = [](ShJVal shSub, TJKey keyUp, TJKey keyItm) -> TPObj {
			return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, keyUp, keyItm);//dwk: 2025-11-17 15:13 
			//return new TObj();
			};
		MfcMapTPbj_step3_deprecated<TKey, TKeyA, TJKey, TPObj>(ar, mapTObj, key, CbSave, CbAdd, CbClear, CbFactory, CbLoad);
		//DWK_LAST_WORKING;
	}
	/// <summary>
	/// CMapWordToPtr 전용 + factory: WORD 키, 값은 가변 파생형(TObj 파생 포함)
	/// factory(shSubJson) 가 적절한 파생 인스턴스를 생성해 반환. nullptr 반환 시 기본 new TObj()로 대체.
	/// </summary>
	/// <typeparam name="TObj"></typeparam>
	/// <param name="ar"></param>
	/// <param name="mapWP"></param>
	/// <param name="key"></param>
	/// <param name="cbFactory">__class__ 으로 Create Object를 대신 해주는 람다 함수</param>
	/// <param name="CbLoadItem">저장 할때 CRunTimeClass 참조 하여 class 명을 __class__  키로 저장 해주는 람다 함수</param>
	template<typename TObj>
	void MapWordPtr_LB2(CMapWordToPtr & mapWP, std::wstring key
		, function<void(CJXArchive&, TObj*)> CbSave
		, function<TObj * (ShJVal, WORD)> CbFactory // 객체 생성을 커스텀으로 하고 Serialize 전 작업을 할수 있다.
		, function<void(CJXArchive&, TObj*)> CbLoad
	)
	{
		DWKFUNC;
		try
		{
			auto& ar = *this;
			CJXArchive ar2(ar, key.c_str());
			if (ar.IsStoring()) {
				ShJObj newShJbj = NEWSHP(UcJObj);
				auto& newJbj = *newShJbj->Dic();
				POSITION pos = mapWP.GetStartPosition();
				while (pos != NULL)
				{
					WORD nid = 0;
					void* ptr = nullptr;
					mapWP.GetNextAssoc(pos, nid, ptr);
					CJXArchive arcItm(*this);///step 1: 저장할 Archive 생성
					auto pItm = reinterpret_cast<TObj*>(ptr);
					if (pItm) {
						ASSERT(CbSave);//pItm->Serialize(ar3);
						CbSave(arcItm, pItm);// class 데이터가 일단 arcItm._jData에 들어 간다.
						auto shJbj3 = arcItm.GetJson();
						//	UcDefaultSaveClassName(shJbj3, pItm);
						std::wstring skey = std::to_wstring(static_cast<unsigned>(nid));
						newJbj.SetObj(skey, shJbj3, false);///step 3: 각 키로 넣는다
					}
				}
				//ar.Jbj()(key) = shJbj;
				//this->Jbj().Set(key, make_shared<JVal>(shJbj, false));// 복사하지 않는다.
				//this->Jbj().SetObj(key, newShJbj, false);// 복사하지 않는다.
				ar2.SaveToUpperNode(newShJbj);
			}
			else {
				mapWP.RemoveAll();
				auto& jdata = ar.Jbj();
				auto shObj = jdata.Obj(key.c_str());
				if (shObj) {//로드 한 데이터가 이 안으로 일단 들어왔다.
					auto& jbj = *shObj->Dic();
					for (auto& pair : jbj) {
						const auto& skey = pair.first; // std::wstring
						const auto& shSub = pair.second;
						auto arItm = CJXArchive(*this, shSub);
						WORD nid = (WORD)_wtoi(skey.c_str());
						TObj* newPbj = nullptr;
						ASSERT(CbFactory);
						//if (CbFactory)
						newPbj = CbFactory(shSub, nid);/// 람다함수 팩토리에서 클래스명을 읽어 생성하도록 위임
						ASSERT(newPbj);
						if (newPbj) {
							ASSERT(CbLoad);
							//if (CbLoad)
							CbLoad(arItm, newPbj);//pNew->Serialize(ar3);
							/// 드디어 CMapWordToPtr 에 넣는다.
							mapWP.SetAt(nid, newPbj);
						}
					}
				}
				else {
					DWKTRACE(L"dwk key(%v)에 JObj가 없네?", key);
				}
			}
		}
		catch (CException* e) {
			e; throw;
		}
		catch (LPCWSTR serr) {
			DWKTRACE(L"JSON error: %v", serr); serr;
			throw;
		}
	}
	/// CMapWordToPtr 전용 + factory: WORD 키, 값은 가변 파생형(TObj 파생 포함)
	/// 일단 TObj는 CObject 파생형으로 Serialize 멤버 함수가 있다.
	template<typename TObj>
	void MapWordPtr_1(CMapWordToPtr & mapWP, std::wstring key
		, function<void(CJXArchive&, TObj*)> CbSave = nullptr
		, function<TObj * (ShJVal, WORD)> cbFactory = nullptr // 객체 생성을 커스텀으로 하고 Serialize 전 작업을 할수 있다.
		, function<void(CJXArchive&, TObj*)> CbLoad = nullptr
	)
	{
		auto& ar = *this;
		// CObject 기반인 경우에만 디폴트 값 지정
#if CPP17_OR_LATER
		if constexpr (std::is_base_of_v<CObject, TObj>)
#else
		if (std::is_base_of<CObject, TObj>::value)
#endif
		{//CObject 기반인 경우 디폴트 값 지정
			if (!CbSave)
				CbSave = [](CJXArchive& ari, TObj* pItem) {///추가 저장할 항목 여기서
				auto shSub = ari.GetJson();
				ASSERT(shSub->IsDic());
				pItem->Serialize(ari);
				CJXArchive::UcDefaultSaveClassName(shSub, pItem);//shSub->Dic()->SetAttr(TAG_CLS, pObj->GetRuntimeClass()->m_lpszClassName);
				///auto& jb = *shSub->Dic();
				//ArcValueField(ar, _kObj, _data2);
				///jb("extraData") = "Extra Data";//추가로 더 저장할게 있으면
				};
			if (!cbFactory)
				cbFactory = [](ShJVal shSub, WORD id) -> TObj* {
				ASSERT(shSub->IsDic());
				if (shSub->IsDic())
					return CJXArchive::UcDefaultCreateByClassName<TObj>(shSub, {}, id);
				return nullptr;//return (CObject*)AddNewTemplate(shSub);
				};
			if (!CbLoad)
				CbLoad = [](CJXArchive& ari, TObj* pItem) {
				pItem->Serialize(ari);
				};
		}
		else {//CObject 기반이 아닌 경우(_variant_t 계열 등)는 매개변수 필수
			ASSERT(CbSave);
			ASSERT(cbFactory);
			ASSERT(CbLoad);
		}
		// CObject 기반이 아닌 경우(_variant_t 계열 등)는 매개변수 필수
		// 디폴트 값 없음 - 호출자가 반드시 CbSave, cbFactory, CbLoad를 제공해야 함
		MapWordPtr_LB2<TObj>(mapWP, key, CbSave, cbFactory, CbLoad);
	}
	/// ///////////////////////////////////////////////////////////////////////
	/// JSON Serialize 방법 //
	/// 1. OnSaveDocument, OnOpenDocument 에서 애초에 CJsonArchive 로 시작 한다.
	/// * Store 와 Load 는 기본적으로 같은 방식으로 함.
	///	- Store에는 :
	/// 1. Primary type은 
	///	- before: ar << m_SomeValue;
	///	- after:  ar << KEYVALTP(m_SomeValue);
	/// 
	/// 2. 필드 중 CObject 타입 의 JSerialize
	/// 
#define VAR2STR(v) v, CLASSKEY(v)
	//NC 는 앞에 class. 이 안붙는다.
#define VAR2STRNC(v) v, wstring(L#v)
//#define VAR2STR(v) v, wstring(L#v)
#define IVAR2STR(v) {VAR2STR(v)}
#define IVAR2STRNC(v) {VAR2STRNC(v)}//NC 는 앞에 class. 이 안붙는다.
#define ArcValueSKey(ar, var, key) \
    do{if (ar.IsStoring()) {ar << ar.KEYTpSKey(var, key);} else {ar >> ar.KEYTpSKey(var, key);}}while(0)
#define ArcValueKey(ar, var, key) \
    do{if (ar.IsStoring()) {ar << KEYTpKey(var, key);} else {ar >> KEYTpKey(var, key);}}while(0)
	/// "CXmlAppDoc._rc1" + "left" 로 키를 만들어서
#define ArcStructMember(ar, var) do{\
		CStringW sVar(L#var); int idot= -1; idot = sVar.Find('.'); CStringW key(sVar);\
		if(idot >= 0){	key = CStringW(ar._key.c_str()) + sVar.Mid(idot);}\
		if (ar.IsStoring()) {ar << make_tuple(std::wstring(key.GetString()), &(var));} \
		else {ar >> make_tuple(std::wstring(key.GetString()), &(var));}\
	}while(0)
/// 구조체 멤버 변수 저장 (arm을 사용하는 특수 케이스)
#define Arc_StructMember(var) ArcStructMember(arm, var)
	/// 저장할 변수가 다른 class body와 member 가 붙어 있는 경우. body.field
#define ArcValueField(ar, body, field) \
    do{if (ar.IsStoring()) {ar << KEYField(body, field);} else {ar >> KEYField(body, field);}}while(0)
#define ArcValuePtrField(ar, body, field) \
    do{if (ar.IsStoring()) {ar << KEYPtrField(body, field);} else {ar >> KEYPtrField(body, field);}}while(0)
#define ArcValue(ar, var) \
     do{ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr); if (ar.IsStoring()) {ar << KEYVALTP(var);} else {ar >> KEYVALTP(var);}}while(0)
	//do { if (ar.IsStoring()) { ar << KEYVALTP(var); } else { ar >> KEYVALTP(var); } } while (0)
	// class. 없이 
#define ArcValueNC(ar, var) \
     do{ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr); if (ar.IsStoring()) {ar << KEYVALTPNC(var);} else {ar >> KEYVALTPNC(var);}}while(0)
#define ArcMember(ar, var) do{\
		CStringW sVar(L#var); int idot= -1; idot = sVar.Find('.'); CStringW key(sVar);\
		if(idot >= 0){	key = sVar.Mid(idot+1);}\
		if (ar.IsStoring()) {ar << make_tuple(std::wstring(key.GetString()), &(var));} \
		else {ar >> make_tuple(std::wstring(key.GetString()), &(var));}\
	}while(0)
/// 구조체/클래스 멤버 변수 저장 (arm을 사용, 멤버 접근자 '.' 처리)
#define Arc_Member(var) ArcMember(arm, var)
#define ArcEnum(ar, TYPE, var) \
	do{if (ar.IsStoring()) {ar << std::make_tuple(std::wstring(L#var), &((int)var));}\
	else {ar >> std::make_tuple(std::wstring(L#var), &((int)var));}}while(0)
#define ArcVariant(ar, v, k) \
	/// 데이터 Before Store 또는 After Load 작업 중 하나만 람다 함수로 명시
	/// fnc는 IsStore 이면 전에, 아니면 후에 불려진다.
#define KeyValBfrOrAft(var, fnc) std::make_tuple<wstring, decltype(var)*, std::function<void(decltype(var)&)>>(std::wstring(L#var), &(var), (fnc))
	/// <summary>
	/// _variant_t 를 JSON/XMLb 으로 저장/로드
	/// </summary>
	/// <typeparam name="TVari"></typeparam>
	/// <param name="ar"></param>
	/// <param name="pItem">_variant_t derived pointer</param>
	template<typename TVari>
	inline void ArcVariant_impl2(TVari * pItem)
	{
		auto& ar = *this;
		static KStdMap<VARTYPE, wstring> s_mapVarType = {
				IVAR2STRNC(VT_BOOL),//NC 는 앞에 class. 이 안붙는다.
				IVAR2STRNC(VT_R4),
				IVAR2STRNC(VT_R8),
				IVAR2STRNC(VT_BSTR),
				IVAR2STRNC(VT_I2),
				IVAR2STRNC(VT_I4),
				IVAR2STRNC(VT_I8),
				IVAR2STRNC(VT_INT),
		};
		_variant_t& vari = (_variant_t&)*pItem;
		auto& jbj = ar.Jbj();
		if (ar.IsStoring()) {
			jbj.SetAttr("vt", vari.vt);
			wstring svt = s_mapVarType.Get(vari.vt);
			ASSERT(svt.length());
			jbj.SetAttr("svt", svt);
			ArcVariableSave_3(ar, vari);
		}
		else {
			int vt{ 0 };
			ArcValueNC(ar, vt);//vt = 11
			CString svt;
			ArcValueNC(ar, svt);//svt = "VT_BOOL"
			ArcVariableLoad_3(ar, vari, vt);
		}
	}
	/// ArrayOfValJSerialize_Variant StdTArrTStruct_2 StdTArrTStructCustom_3
	template<typename TContainer>
	void StdVect_variant_1(TContainer & lstTObj, wstring key)// function<void(CJXArchive&, UcJObj&, TVal&)> cbCustom)
	{
		auto& ar = *this;
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {
			ari.ArcVariant_impl2(&r);
			});
	}
	/// ArrayOfValJSerialize_VectorVector: TContainer 처리 (2차원 컨테이너 버전)
	/// TContainer는 std::vector<std::vector<T>>, std::list<std::list<T>> 등 2차원 컨테이너
	/// T는 CString, int, double, std::string 등 어떤 타입이든 가능
	/// std::vector는 할당자 템플릿 매개변수가 있어서 template<typename> class로는 매칭 안됨
	//template<template<typename> class TOuterList, template<typename> class TInnerList, typename TObj>
	//void ArrayOfValJSerialize_VectorVector(CJXArchive& ar, TOuterList<TInnerList<TObj>>& lstTObj, wstring key)
	template<typename TContainer>
	void StdVect2D_Val_1(TContainer & lstTObj, wstring key)//?step 1
	{
		auto& ar = *this;
		// 외부 컨테이너의 각 항목(내부 컨테이너)을 처리
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {//?step 2-
			// r은 내부 컨테이너 타입 (예: std::vector<CString>, std::list<int> 등)
			ArcValueSKey(ari, r, TAG_ARR);//?step 4 // item 안에 다시 inItem 이 있어야 계층이 성립 된다.
			});
	}
	/// std::vector<std::vector<std::vector<TVal>>> 3D 벡터 처리
	template<typename TContainer>
	void StdVect3D_Val_1(TContainer & lstTObj, wstring key)
	{
		auto& ar = *this;
		// 외부 컨테이너의 각 항목(2차원 컨테이너)을 처리
		StdTArrTStruct_2(lstTObj, key, [this](CJXArchive& ari, auto& r) {
			// r은 2차원 컨테이너 타입 (예: std::vector<std::vector<double>> 등)
			// 2차원 벡터를 직접 처리 - array로 처리 (Dictionary를 만들지 않음)
			if (ari.IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				// ari의 현재 노드(Dictionary) 안에 JCOL_TAG로 array를 저장
				CJXArchive ari2(ari); // key를 주지 않으면 같은 상위 노드 사용
				StdTArrTStructCustom_LB4(ari2, shArrNew, r, [this](CJXArchive& ari3, auto& r2) {
					// r2는 1차원 컨테이너 타입 (예: std::vector<double> 등)
					ArcValueSKey(ari3, r2, TAG_ITM);// 최종 1차원 벡터 처리
					});
				// ari의 현재 노드(Dictionary)에 JCOL_TAG로 array 추가
				auto shJbj3 = ari.GetJson();
				if (shJbj3->IsDic()) {
					shJbj3->Dic()->SetNode(TAG_ARR, shArrNew, false);
				}
			}
			else {
				// ari의 현재 노드(Dictionary)에서 JCOL_TAG로 array를 읽기
				ShJObj shObj = ari.GetJson();
				ShJArr shArr;
				if (shObj->IsDic()) {
					auto& j1 = *shObj->Dic();
					if (ShJVal shItem = j1.Get(TAG_ARR)) {
						if (shItem->IsArr()) {
							shArr = shItem;
						}
						else if (shItem->IsDic()) {
							// JCOL_TAG가 Dictionary이면, 그 안에서 JROW_TAG로 다시 찾기
							auto& j2 = *shItem->Dic();
							if (ShJVal shItem2 = j2.Get(TAG_ITM)) {
								if (shItem2->IsArr()) {
									shArr = shItem2;
								}
								else {
									throw_str(L"Not a Array.");
								}
							}
						}
						else {
							throw_str(L"Not a Array.");
						}
					}
				}
				if (shArr) {
					CJXArchive ari2(ari); // key를 주지 않으면 같은 상위 노드 사용
					StdTArrTStructCustom_LB4(ari2, shArr, r, [this](CJXArchive& ari3, auto& r2) {
						ArcValueSKey(ari3, r2, TAG_ITM);
						});
				}
			}
			});
	}
	template<typename TContainer>
	void StdVect2D_variant(TContainer & lstTObj, wstring key)//?step 1
	{
		auto& ar = *this;
		// 외부 컨테이너의 각 항목(내부 컨테이너)을 처리
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& rbj) {//?step 2-
			//+		rbj	{ size=4 }	std::vector<_variant_t> &
			ari.StdVect_variant_1(rbj, TAG_ARR);
			});
	}
	template<typename TContainer>
	void StdVect2D_Pbj_1(TContainer & lstTObj, wstring key)//?step 1
	{
		auto& ar = *this;
		// 외부 컨테이너의 각 항목(내부 컨테이너)을 처리
		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& r) {//?step 2-
			ari.ObjectPointer(r, TAG_ARR);
			});
	}
	/// std::vector<std::vector<shared_ptr<TObj>>> 전용
	template<typename TContainer>
	void StdVect2D_ShpObj_1(TContainer & lstTObj, wstring key)
	{
		using TInnerContainer = typename TContainer::value_type;
		using TObjectHolder = typename TInnerContainer::value_type;
//		static_assert(CJXArchiveDetail::is_shared_ptr<TObjectHolder>::value,
//			"StdVect2D_ShpObj_1은 vector<vector<shared_ptr<TObj>>>만 지원합니다.");

		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& inner) {
			ari.StdListShpObj_1(inner, TAG_ARR);
			});
	}
	/// std::vector<std::vector<TObj*>> 전용
	template<typename TContainer>
	void StdVect2D_PtrObj_1(TContainer & lstTObj, wstring key)
	{
		using TInnerContainer = typename TContainer::value_type;
		using TObjectHolder = typename TInnerContainer::value_type;
		static_assert(std::is_pointer<TObjectHolder>::value,
			"StdVect2D_PtrObj_1은 vector<vector<TObj*>>만 지원합니다.");

		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& inner) {
			ari.StdListPtrObj_1(inner, TAG_ARR);
			});
	}
	/// std::vector<std::vector<TObj>> 전용 (객체가 직접 들어감)
	template<typename TContainer>
	void StdVect2D_RefObj_1(TContainer & lstTObj, wstring key)
	{
		using TInnerContainer = typename TContainer::value_type;
		using TObjectHolder = typename TInnerContainer::value_type;
//		static_assert(!std::is_pointer<TObjectHolder>::value && !CJXArchiveDetail::is_shared_ptr<TObjectHolder>::value,
//			"StdVect2D_RefObj_1은 vector<vector<TObj>>만 지원합니다. (포인터나 shared_ptr이 아닌 객체 직접)");

		StdTArrTStruct_2(lstTObj, key, [](CJXArchive& ari, auto& inner) {
			ari.StdListRefObj_1<std::vector>(inner, TAG_ARR);
			});
	}
#pragma region [Arc_StdMapPtrObjCNV
	//dwk: 2025-11-12 14:02
	/// std::map 의 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	/// TCompare 기본값은 std::less<TMapKey>로, 커스텀 비교자도 지원한다.
	template<typename TMapKey, typename TVal, typename TCompare = std::less<TMapKey>>
	void StdMapValCnv(std::map<TMapKey, TVal, TCompare>&mapTVal, const std::wstring key,
		function<TMapKey(const wstring&)> cbCnvKeyOut = nullptr,//읽은 후 wstring을 map의 키로 변환 //Loding
		function<TVal(const wstring&)> cbCnvValOut = nullptr,//읽은 후 wstring을 map의 값으로 변환 //Loding
		function<wstring(const TMapKey&)> cbCnvKeyIn = [](const TMapKey& key) -> wstring { return assign_wstring(key); },//쓰기 전 map의 키를 wstring으로 변환
		function<wstring(const TVal&)> cbCnvValIn = [](const TVal& val) -> wstring { return assign_wstring(val); }//쓰기 전 map의 값을 wstring으로 변환
		/// 4번째는 디폴트와 같으므로 안해도 된다. assign_wstring 가 여러가지 처리 한다.
	)
	{
		auto& ar = *this;
		std::map<std::wstring, std::wstring> vecTmp;
		ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr);
		if (ar.IsStoring()) {
			for (auto& pr : mapTVal) {
				wstring k2 = cbCnvKeyIn(pr.first);
				vecTmp[k2] = cbCnvValIn(pr.second);
			}
			ar << ar.KEYTpSKey(vecTmp, key);
		}
		else {
			ar >> ar.KEYTpSKey(vecTmp, key);
			for (auto& pr : vecTmp) {
				ASSERT(cbCnvKeyOut);
				ASSERT(cbCnvValOut);
				auto k2 = cbCnvKeyOut(pr.first);// (TMapKey)UcAtoi(pr.first.c_str());
				mapTVal[k2] = cbCnvValOut(pr.second);// std::wstring(pr.second.begin(), pr.second.end());
			}
		}
	}
#ifdef _Sample__
	// std::map<UINT, std::string> 예제
	ar.StdMapValCnv<UINT, std::string>(VAR2STR(itemName_),
		[](auto& key) -> UINT {return std::stoul(key); },//읽은 후 wstring을 map의 키 UINT로 변환
		[](auto& val) -> std::string {return CStringA(val.c_str()).GetString(); }//읽은 후 wstring을 map의 값 string으로 변환
	);
	// std::map<double, long> 예제 - Arc_StdMapVal 사용 (타입 자동 추론)
	std::map<double, long> m_mapClass;
	m_mapClass[1.5] = 100;
	m_mapClass[2.7] = 200;
	m_mapClass[3.9] = 300;
	Arc_StdMapVal(m_mapClass);
#endif // _Sample__
	template<typename TMapKey, typename TVal, typename TCompare = std::less<TMapKey>>
	void StdMapCStrCStrCnv(std::map<TMapKey, TVal, TCompare>&mapTObj, const std::wstring key)
	{
		StdMapValCnv<TMapKey, TVal, TCompare>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				return (TMapKey)CString(key.c_str()); //읽은 후 wstring을 원래 map의 키 CString 로 변환
			},
			[](const wstring& val) -> TVal {
				return TVal(val.c_str()).GetString(); //읽은 후 wstring을 원래 map의 값 string으로 변환
			}
		);
	}
#define ArcStdMapCStr(ar, v) ar.StdMapCStrCStrCnv(VAR2STR(v))
	/// std::map<CString, CString> 저장 (키와 값을 wstring으로 변환하여 처리)
	/// @deprecated Arc_StdMapVal 사용 권장
#define Arc_StdMapCStr(v) ArcStdMapCStr(ar, v)
	template<typename TMapKey, typename TVal, typename TCompare = std::less<TMapKey>>
	void StdMapCStrIntCnv(std::map<TMapKey, TVal, TCompare>&mapTObj, const std::wstring key)
	{
		StdMapValCnv<TMapKey, TVal, TCompare>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				return (TMapKey)CString(key.c_str()); //읽은 후 wstring을 원래 map의 키 CString 로 변환
			},
			[](const wstring& val) -> TVal {
				return TVal(_wtoi(val.c_str())); //읽은 후 wstring을 원래 map의 값 int로 변환
			}
		);
	}
#define ArcStdMapCStrInt(ar, v) ar.StdMapCStrIntCnv<CString,int>(VAR2STR(v))
	/// std::map<CString, int> 저장 (키와 값을 wstring으로 변환하여 처리)
	/// @deprecated Arc_StdMapVal 사용 권장
#define Arc_StdMapCStrInt(v) ArcStdMapCStrInt(ar, v) 
	/// std::map<int, string> 의 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	template<typename TMapKey, typename TVal>
	void StdMapUIntStrCnv(std::map<TMapKey, TVal>&mapTObj, const std::wstring key)
	{
		StdMapValCnv<TMapKey, TVal>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				return (TMapKey)UcAtoU(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
			},
			[](const wstring& val) -> TVal {//std::string
				return CStringA(val.c_str()).GetString(); //읽은 후 wstring을 원래 map의 값 string으로 변환
			}
		);
	}
#define ArcStdMapUIntStrCnv(ar, v) ar.StdMapUIntStrCnv(VAR2STR(v))
	/// std::map<unsigned int, string> 저장 (키와 값을 wstring으로 변환하여 처리)
	/// @deprecated Arc_StdMapVal 사용 권장
#define Arc_StdMapUIntStrCnv(v) ArcStdMapUIntStrCnv(ar, v)
	/// std::map<int, string> 의 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	template<typename TMapKey, typename TVal>
	void StdMapIntStrCnv(std::map<TMapKey, TVal>&mapTObj, const std::wstring key)
	{
		StdMapValCnv<TMapKey, TVal>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				return (TMapKey)UcAtoi(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
			},
			[](auto& val) -> std::string {
				return CStringA(val.c_str()).GetString(); //읽은 후 wstring을 원래 map의 값 string으로 변환
			}
		);
	}
#define ArcStdMapIntStrCnv(ar, v) ar.StdMapIntStrCnv(VAR2STR(v))
	/// std::map<int, string> 저장 (키와 값을 wstring으로 변환하여 처리)
	/// @deprecated Arc_StdMapVal 사용 권장
#define Arc_StdMapIntStrCnv(v) ArcStdMapIntStrCnv(ar, v)
	// wstring을 다양한 타입으로 변환하는 헬퍼 함수들 (C++14 호환)
	// 숫자 타입
	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return (T)_wtof(s.c_str());
	}
	template<typename T>
	typename std::enable_if<std::is_integral<T>::value&& std::is_signed<T>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return (T)std::stoll(s);
	}
	template<typename T>
	typename std::enable_if<std::is_integral<T>::value&& std::is_unsigned<T>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return (T)std::stoull(s);
	}
	// 문자열 타입
	template<typename T>
	typename std::enable_if<std::is_same<T, std::string>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return CStringA(s.c_str()).GetString();
	}
	template<typename T>
	typename std::enable_if<std::is_same<T, CString>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return CString(s.c_str());
	}
#ifdef _UNICODE
	template<typename T>
	typename std::enable_if<std::is_same<T, CStringA>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return CStringA(s.c_str());
	}
#else
	template<typename T>
	typename std::enable_if<std::is_same<T, CStringW>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return CStringW(s.c_str());
	}
#endif
	template<typename T>
	typename std::enable_if<std::is_same<T, std::wstring>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		return s; // wstring 그대로
	}
	// 그 외 타입 (컴파일 오류 방지)
	template<typename T>
	typename std::enable_if<!std::is_arithmetic<T>::value &&
		!std::is_same<T, std::string>::value &&
		!std::is_same<T, CString>::value &&
		!std::is_same<T, CStringW>::value &&
		!std::is_same<T, std::wstring>::value, T>::type
		ConvertWstringToNum(const wstring & s) {
		static_assert(sizeof(T) == 0, "Unsupported type for ConvertWstringToNum");
		return T{};
	}
	/// std::map 의 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다. (일반 버전)
	/// 사용 예: Arc_StdMapVal(m_mapClass) - 타입은 자동 추론됨
	template<typename TMapKey, typename TVal>
	void StdMapVal(std::map<TMapKey, TVal>&mapTVal, const std::wstring key)
	{
		StdMapValCnv<TMapKey, TVal>(mapTVal, key,
			[this](const wstring& key) -> TMapKey {
				return ConvertWstringToNum<TMapKey>(key);
			},
			[this](const wstring& val) -> TVal {
				return ConvertWstringToNum<TVal>(val);
			}
		);
	}
#define ArcStdMapVal(ar, v) ar.StdMapVal(VAR2STR(v))
	/// std::map<TMapKey, TVal> 저장 (키와 값을 wstring으로 변환하여 처리, 일반 버전)
	/// 사용 예: Arc_StdMapVal(m_mapClass) - std::map<double, long> 등
#define Arc_StdMapVal(v) ArcStdMapVal(ar, v)

	/// std::map<TMapKey, std::vector<TVecVal>> 직렬화 (값을 배열로 처리)
	template<typename TMapKey, typename TVecVal>
	void StdMapVector(std::map<TMapKey, std::vector<TVecVal>>&mapTVec, const std::wstring key,
		function<TMapKey(const wstring&)> cbCnvKeyOut = nullptr,// wstring -> key (Loading)
		function<TVecVal(const wstring&)> cbCnvValOut = nullptr,// wstring -> vec elem (Loading)
		function<wstring(const TMapKey&)> cbCnvKeyIn = [](const TMapKey& key) -> wstring { return assign_wstring(key); },// key -> wstring (Saving)
		function<wstring(const TVecVal&)> cbCnvValIn = [](const TVecVal& val) -> wstring { return assign_wstring(val); }// vec elem -> wstring (Saving)
	)
	{
		auto& ar = *this;
		if (!cbCnvKeyOut) {
			cbCnvKeyOut = [this](const wstring& key) -> TMapKey { return this->ConvertWstringToNum<TMapKey>(key); };
		}
		if (!cbCnvValOut) {
			cbCnvValOut = [this](const wstring& val) -> TVecVal { return this->ConvertWstringToNum<TVecVal>(val); };
		}
		std::map<std::wstring, std::vector<std::wstring>> tmp;
		ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr);
		if (ar.IsStoring()) {
			for (auto& pr : mapTVec) {
				std::vector<std::wstring> vecWs;
				vecWs.reserve(pr.second.size());
				for (auto& v : pr.second) {
					vecWs.push_back(cbCnvValIn(v));
				}
				tmp[cbCnvKeyIn(pr.first)] = std::move(vecWs);
			}
			ar << ar.KEYTpSKey(tmp, key);
		}
		else {
			ar >> ar.KEYTpSKey(tmp, key);
			mapTVec.clear();
			for (auto& pr : tmp) {
				ASSERT(cbCnvKeyOut && cbCnvValOut);
				std::vector<TVecVal> vecVal;
				vecVal.reserve(pr.second.size());
				for (auto& ws : pr.second) {
					vecVal.push_back(cbCnvValOut(ws));
				}
				mapTVec[cbCnvKeyOut(pr.first)] = std::move(vecVal);
			}
		}
	}
#define ArcStdMapVect(ar, v) ar.StdMapVector(VAR2STR(v))
	/// std::map<TMapKey, std::vector<TVecVal>> 저장 (값을 배열로 처리)
#define Arc_StdMapVect(v) ArcStdMapVect(ar, v)

	/// std::map<TMapKey, std::pair<TFirst, TSecond>> 직렬화 (pair를 배열 2칸으로 저장: __type__="array", __r_ 2개)
	template<typename TMapKey, typename TFirst, typename TSecond>
	void StdMapPair(std::map<TMapKey, std::pair<TFirst, TSecond>>&mapTPair, const std::wstring key)
	{
		auto& ar = *this;
		using TPair = std::pair<TFirst, TSecond>;
		std::map<TMapKey, std::vector<std::wstring>> tmp;

		// 공통 변환자 (키/값)
		auto cnvKeyOut = [this](const wstring& keyStr) -> TMapKey { return ConvertWstringToNum<TMapKey>(keyStr); };
		auto cnvKeyIn = [](const TMapKey& key) -> wstring { return assign_wstring(key); };
		auto idValOut = [](const wstring& v) -> std::wstring { return v; };
		auto idValIn = [](const std::wstring& v) -> std::wstring { return v; };

		if (ar.IsStoring()) {
			for (auto& pr : mapTPair) {
				std::vector<std::wstring> vec;
				vec.reserve(2);
				vec.push_back(assign_wstring(pr.second.first));
				vec.push_back(assign_wstring(pr.second.second));
				tmp[pr.first] = std::move(vec);
			}
			StdMapVector<TMapKey, std::wstring>(tmp, key, cnvKeyOut, idValOut, cnvKeyIn, idValIn);
		}
		else {
			StdMapVector<TMapKey, std::wstring>(tmp, key, cnvKeyOut, idValOut, cnvKeyIn, idValIn);
			mapTPair.clear();
			for (auto& pr : tmp) {
				if (pr.second.size() >= 2) {
					TFirst first = ConvertWstringToNum<TFirst>(pr.second[0]);
					TSecond second = ConvertWstringToNum<TSecond>(pr.second[1]);
					mapTPair[pr.first] = std::make_pair(first, second);
				}
			}
		}
	}
#define ArcStdMapPair(ar, v) ar.StdMapPair(VAR2STR(v))
	/// std::map<TMapKey, std::pair<TFirst, TSecond>> 저장 (pair를 배열 형태로 저장)
	/// 사용 예: Arc_StdMapPair(m_mapControlItems) - std::map<CString, pair<double,double>> 등
#define Arc_StdMapPair(v) ArcStdMapPair(ar, v)

	/// std::vector<TVal> 의 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	template<typename TVal>
	void StdVectValCnv(std::vector<TVal>&vecTVal, const std::wstring key,
		function<TVal(const wstring&)> cbCnvValOut = nullptr,//읽은 후 wstring을 vector의 값으로 변환 //Loading
		function<wstring(const TVal&)> cbCnvValIn = [](const TVal& val) -> wstring { return assign_wstring(val); }//쓰기 전 vector의 값을 wstring으로 변환
	)
	{
		auto& ar = *this;
		std::vector<std::wstring> vecTmp;
		ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr);
		if (ar.IsStoring()) {
			for (auto& val : vecTVal) {
				vecTmp.push_back(cbCnvValIn(val));
			}
			ar << ar.KEYTpSKey(vecTmp, key);
		}
		else {
			ar >> ar.KEYTpSKey(vecTmp, key);
			vecTVal.clear();
			for (auto& val : vecTmp) {
				ASSERT(cbCnvValOut);
				vecTVal.push_back(cbCnvValOut(val));
			}
		}
	}
	/// std::vector<enum> 의 값을 wstring 으로 변환하여 저장/로드 한다.
	template<typename TEnum>
	void StdVectEnumCnv(std::vector<TEnum>&vecEnum, const std::wstring key)
	{
		StdVectValCnv<TEnum>(vecEnum, key,
			[](const wstring& val) -> TEnum {
				return static_cast<TEnum>(UcAtoi(val.c_str())); // wstring -> int -> enum
			},
			[](const TEnum& val) -> wstring {
				return std::to_wstring(static_cast<int>(val)); // enum -> int -> wstring
			}
		);
	}
	/// BYTE/char 배열을 null-terminated 문자열들로 해석하여 vector<std::string>으로 저장/로드
	/// 저장: 배열에서 null-terminated 문자열들을 추출하여 vector<std::string>으로 변환
	/// 로드: vector<std::string>에서 읽어서 배열에 null-terminated로 순차 저장
	template<typename TINT, size_t N>
	inline void BufferForCharString(char(&buffer)[N], TINT & countVar, const std::wstring key)
	{
		auto& ar = *this;
		std::vector<std::string> vecTmp;
		if (ar.IsStoring()) {
			const char* p = buffer;
			const char* pEnd = p + N;//		pEnd - p == 65536	
			TINT nCount = 0;
			while (p < pEnd) {
				// 연속된 null 문자들을 건너뜀
				while (p < pEnd && *p == '\0')
					++p;
				if (p >= pEnd) break; // 버퍼 끝에 도달

				const char* pStrStart = p;
				while (p < pEnd && *p != '\0')
					++p;
				size_t len = p - pStrStart;
				if (len > 0) {
					vecTmp.push_back(std::string(pStrStart, len));
					++nCount;
					if (countVar > 0 && countVar == nCount)
						break;
				}
				// null terminator는 이미 위의 while 루프에서 건너뛰었거나, 다음 루프에서 건너뛸 것임
			}
			//countVar = nCount;
			ar << ar.KEYTpSKey(vecTmp, key.empty() ? L"" : key);
		}
		else {
			ar >> ar.KEYTpSKey(vecTmp, key.empty() ? L"" : key);
			memset(buffer, 0, N);
			char* pBuf = buffer;
			const char* pBufEnd = pBuf + N;
			TINT nCount = 0;
			for (const auto& str : vecTmp) {
				if (pBuf + str.length() + 1 > pBufEnd) break;
				memcpy(pBuf, str.c_str(), str.length());
				pBuf += str.length();
				*pBuf++ = '\0';
				++nCount;
			}
			countVar = nCount;
		}
	}

	/// unsigned char 배열을 null-terminated 문자열들로 해석하여 vector<std::string>으로 저장/로드
	template<typename TINT, size_t N>
	inline void BufferForCharString(unsigned char(&buffer)[N], TINT & countVar, const std::wstring key)
	{
		BufferForCharString(reinterpret_cast<char(&)[N]>(buffer), countVar, key);
	}

	/// wchar_t 배열을 null-terminated 문자열들로 해석하여 vector<std::wstring>으로 저장/로드
	template<typename TINT, size_t N>
	inline void BufferForCharString(wchar_t(&buffer)[N], TINT & countVar, const std::wstring key)
	{
		auto& ar = *this;
		std::vector<std::wstring> vecTmp;
		if (ar.IsStoring()) {
			const wchar_t* p = buffer;
			const wchar_t* pEnd = p + N;
			TINT nCount = 0;
			while (p < pEnd) {
				// 연속된 null 문자들을 건너뜀
				while (p < pEnd && *p == L'\0')
					++p;
				if (p >= pEnd) break; // 버퍼 끝에 도달

				const wchar_t* pStrStart = p;
				while (p < pEnd && *p != L'\0')
					++p;
				size_t len = p - pStrStart;
				if (len > 0) {
					vecTmp.push_back(std::wstring(pStrStart, len));
					++nCount;
					if (countVar > 0 && countVar == nCount)
						break;
				}
				// null terminator는 이미 위의 while 루프에서 건너뛰었거나, 다음 루프에서 건너뛸 것임
			}
			countVar = nCount;
			ar << ar.KEYTpSKey(vecTmp, key.empty() ? L"" : key);
		}
		else {
			ar >> ar.KEYTpSKey(vecTmp, key.empty() ? L"" : key);
			memset(buffer, 0, N * sizeof(wchar_t));
			wchar_t* pBuf = buffer;
			const wchar_t* pBufEnd = pBuf + N;
			TINT nCount = 0;
			for (const auto& str : vecTmp) {
				if (pBuf + str.length() + 1 > pBufEnd) break;
				memcpy(pBuf, str.c_str(), str.length() * sizeof(wchar_t));
				pBuf += str.length();
				*pBuf++ = L'\0';
				++nCount;
			}
			countVar = nCount;
		}
	}
#define ArcStdVectEnumCnv(ar, v) ar.StdVectEnumCnv(VAR2STR(v))
	/// std::vector<enum> 저장 (enum을 int로 변환하여 wstring으로 처리)
#define Arc_StdVectEnumCnv(v) ArcStdVectEnumCnv(ar, v)
	template<typename TBstNum>
	void StdVect_uint_EnumCnv(std::vector<TBstNum>&vecEnum, const std::wstring key)
	{
		StdVectValCnv<TBstNum>(vecEnum, key,
			[](const wstring& val) -> TBstNum {
				return static_cast<TBstNum>(UcAtoi(val.c_str())); // wstring -> int -> enum
			},
			[](const TBstNum& val) -> wstring {
				return std::to_wstring(static_cast<unsigned int>(val)); // enum -> int -> wstring
			}
		);
	}
#define ArcStdVectBstNumCnv(ar, v) ar.StdVect_uint_EnumCnv(VAR2STR(v))
	/// std::vector<boost::uint> 저장 (boost::uint를 int로 변환하여 wstring으로 처리)
#define Arc_StdVectBstNumCnv(v) ArcStdVectBstNumCnv(ar, v)
	/// std::map 의 키만 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	//template<typename TMapKey, typename TObj>
	//template<template<typename, typename, typename...> class TMap, typename TMapKey, typename TObj, typename... TArgs>
	//void StdMapPbjCnv(TMap<TMapKey, TObj*>& mapTObj, const std::wstring key,
	template<typename TMapKey, typename TObj>
	void StdMapPbjCnv(std::map<TMapKey, TObj*>&mapTObj, const std::wstring key,
		function<TMapKey(const wstring&)> cbCnvKeyOut = nullptr,//이건 리턴값에 따라 커스텀 [](const wstring& key) -> TMapKey { return key; },//읽은 후 wstring을 map의 키로 변환
		function<wstring(const TMapKey&)> cbCnvKeyIn = [](const TMapKey& key) -> wstring { return assign_wstring(key); }//쓰기 전 map의 키를 wstring으로 변환
	)
	{
		auto& ar = *this;
		std::map<std::wstring, TObj*> vecTmp;
		if (this->IsStoring()) {
			for (auto& pr : mapTObj) {
				wstring k2 = cbCnvKeyIn(pr.first);
				//auto k2 = std::to_wstring((pr.first));
				vecTmp[k2] = pr.second;
			}
		}
		ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr);
		ar.StdMapPtrObj_1<std::wstring, TObj>(vecTmp, key);//ArcStdMapPtrObj
		if (!this->IsStoring()) {
			for (auto& pr : vecTmp) {
				ASSERT(cbCnvKeyOut);
				if (cbCnvKeyOut) {
					auto k2 = cbCnvKeyOut(pr.first);// (TMapKey)UcAtoi(pr.first.c_str());
					mapTObj[k2] = pr.second;
				}
			}
		}
	}
	template<template<typename, typename, typename...> class TMap, typename TMapKey, typename TObj, typename... TArgs>
	void StdMapRefCnv(TMap<TMapKey, TObj>&mapTObj, const std::wstring key,
		function<TMapKey(const wstring&)> cbCnvKeyOut = nullptr,//이건 리턴값에 따라 커스텀 [](const wstring& key) -> TMapKey { return key; },//읽은 후 wstring을 map의 키로 변환
		function<wstring(const TMapKey&)> cbCnvKeyIn = [](const TMapKey& key) -> wstring { return assign_wstring(key); }//쓰기 전 map의 키를 wstring으로 변환
	)
	{
		auto& ar = *this;
		TMap<std::wstring, TObj> vecTmp;
		if (this->IsStoring()) {
			for (auto& pr : mapTObj) {
				wstring k2 = cbCnvKeyIn(pr.first);
				vecTmp[k2] = pr.second;
			}
		}
		ASSERT(dynamic_cast<CJXArchive*>(&ar) != nullptr);
		ar.StdMapRefObj_1<std::wstring, TObj>(vecTmp, key);//ArcStdMapPtrObj
		if (!this->IsStoring()) {
			for (auto& pr : vecTmp) {
				ASSERT(cbCnvKeyOut);
				if (cbCnvKeyOut) {
					auto k2 = cbCnvKeyOut(pr.first);// (TMapKey)UcAtoi(pr.first.c_str());
					mapTObj[k2] = pr.second;
				}
			}
		}
	}
	//template<template<typename, typename, typename...> class TMap, typename TMapKey, typename TObj, typename... TArgs>
	//void StdMapStrNoRefCnv(TMap<TMapKey, TObj>& mapTObj, const std::wstring key)
	template<typename TMapKey, typename TObj>
	void StdMapSAtrRefCnv(std::map<TMapKey, TObj>&mapTObj, const std::wstring key)
	{
		function<TMapKey(const wstring&)> cbCnvKeyOut = [](const wstring& key) -> TMapKey {
			CStringA sa(key.c_str());
			return (TMapKey)sa.GetString();// UcAtoi(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
			};
		function<wstring(const TMapKey&)> cbCnvKeyIn = [](const TMapKey& key) -> wstring {
			CStringW sw(key.c_str());
			return sw.GetString();
			};
		StdMapRefCnv<std::map, TMapKey, TObj>(mapTObj, key, cbCnvKeyOut, cbCnvKeyIn);
	}
	/// 키가 숫자이고 값이 Object pointer 타입인 경우 std::map <std::string, COptionElem>
#define ArcStdMapSAtrRefObjCnv(ar, v) ar.StdMapSAtrRefCnv(VAR2STR(v))
/// std::map<std::string, TObj> 저장 (키를 wstring으로 변환하여 처리, 객체 직접)
#define Arc_StdMapSAtrRefObjCnv(v) ArcStdMapSAtrRefObjCnv(ar, v)
//	template<typename TMapKey, typename TObj>
//	void StdMapSAtrNoPbjCnv(std::map<TMapKey, TObj*>& mapTObj, const std::wstring key)
//	{
//		StdMapPbjCnv<TMapKey, TObj>(mapTObj, key,
//			[](const wstring& key) -> TMapKey {
//				CStringA sa(key.c_str());
//				return (TMapKey)sa.GetString();// UcAtoi(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
//			},
//			[](const TMapKey& key) -> wstring {
//				CStringW sw(key.c_str());
//				return sw.GetString();
//			}
//		);
//	}
//	/// 키가 숫자이고 값이 Object pointer 타입인 경우 std::map <std::string, COptionElem*>
//#define ArcStdMapSAtrNoPbjCnv(ar, v) ar.StdMapSAtrNoPbjCnv(VAR2STR(v))
//#define Arc_StdMapSAtrNoPbjCnv(v) ArcStdMapSAtrNoPbjCnv(ar, v)
	template<typename TMapKey, typename TObj>
	void StdMapSAtrPbjCnv(std::map<TMapKey, TObj*>&mapTObj, const std::wstring key)
	{
		StdMapPbjCnv<TMapKey, TObj>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				CStringA sa(key.c_str());
				return (TMapKey)sa.GetString();// UcAtoi(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
			},
			[](const TMapKey& key) -> wstring {
				CStringW sw(key.c_str());
				return sw.GetString();
			}
		);
	}
	/// 키가 숫자이고 값이 Object pointer 타입인 경우 std::map <std::string, COptionElem*>
#define ArcStdMapSAtrPbjCnv(ar, v) ar.StdMapSAtrPbjCnv(VAR2STR(v))
/// std::map<std::string, TObj*> 저장 (키를 wstring으로 변환하여 처리, 포인터)
#define Arc_StdMapSAtrPbjCnv(v) ArcStdMapSAtrPbjCnv(ar, v)
	/// std::map<int, TObj*> 의 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	// TMapKey가 std::wstring인 경우 특수화
	template<typename TObj>
	void StdMapStrPbjCnv(std::map<std::wstring, TObj*>&mapTObj, const std::wstring key)
	{
		StdMapPbjCnv<std::wstring, TObj>(mapTObj, key,
			[](const wstring& key) -> std::wstring {
				return key; // wstring은 그대로 반환
			},
			[](const std::wstring& key) -> wstring {
				return key; // wstring은 그대로 반환
			}
		);
	}
	// TMapKey가 std::string인 경우 특수화
	template<typename TObj>
	void StdMapStrPbjCnv(std::map<std::string, TObj*>&mapTObj, const std::wstring key)
	{
		StdMapPbjCnv<std::string, TObj>(mapTObj, key,
			[](const wstring& key) -> std::string {
				CStringA sa(key.c_str());
				return sa.GetString(); // wstring을 string으로 변환
			},
			[](const std::string& key) -> wstring {
				CStringW sw(key.c_str());
				return sw.GetString(); // string을 wstring으로 변환
			}
		);
	}
	// 그 외의 경우 (일반 템플릿)
	template<typename TMapKey, typename TObj>
	typename std::enable_if<!std::is_same<TMapKey, std::wstring>::value && !std::is_same<TMapKey, std::string>::value>::type
	StdMapStrPbjCnv(std::map<TMapKey, TObj*>&mapTObj, const std::wstring key)
	{
		StdMapPbjCnv<TMapKey, TObj>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				CStringA sa(key.c_str());
				return (TMapKey)sa.GetString();// UcAtoi(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
			},
			[](const TMapKey& key) -> wstring {
				CStringW sw(key.c_str());
				return sw.GetString();
			}
		);
	}
	/// 키가 숫자이고 값이 Object pointer 타입인 경우 <std::map, std::string>
#define ArcStdMapStrPbjCnv(ar, v) ar.StdMapStrPbjCnv(VAR2STR(v))
/// std::map<std::string, TObj*> 저장 (키를 wstring으로 변환하여 처리)
#define Arc_StdMapStrPbjCnv(v) ArcStdMapStrPbjCnv(ar, v)
	/// std::map<int, TObj*> 의 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
	template<typename TMapKey, typename TObj>
	void StdMapIntPbjCnv(std::map<TMapKey, TObj*>&mapTObj, const std::wstring key)
	{
		StdMapPbjCnv<TMapKey, TObj>(mapTObj, key,
			[](const wstring& key) -> TMapKey {
				return (TMapKey)UcAtoi(key.c_str()); //읽은 후 wstring을 원래 map의 키 int 로 변환
			}// 역변환은 결국 wstring으로 바뀌 므로 디폴트 람다 함수로 처리 된다.
		);
	}
	/// 키가 숫자이고 값이 Object pointer 타입인 경우
#define ArcStdMapIntPbjCnv(ar, v) ar.StdMapIntPbjCnv(VAR2STR(v))
/// std::map<int, TObj*> 저장 (키를 wstring으로 변환하여 처리)
#define Arc_StdMapIntPbjCnv(v) ArcStdMapIntPbjCnv(ar, v)
#pragma endregion ]Arc_StdMapPtrObjCNV
#pragma region [MfcToStdMap
	/// CMap<,,> 의 데이터를 std::map<,> 로 복사하여 반환 한다.
	template<typename TKey, typename TKeyA, typename TVal, typename TValA, typename TJKey>
	std::map<TKey, TVal> MfcToStdMapStr(CMap<TKey, TKeyA, TVal, TValA>&mapTObj)
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			TKey key;
			TVal val;
			std::map<TKey, TVal> stdMap;
			for (POSITION pos = mapTObj.GetStartPosition(); pos != NULL; ) {
				mapTObj.GetNextAssoc(pos, key, val);
				stdMap[key] = val;
			}
			return stdMap;
		}
		ASSERT(0);
		return {};
	}
	/// std::map<,> 로 로드한 데이터를 CMap<,,> 에 복사 한다.
	template<typename TKey, typename TKeyA, typename TVal, typename TValA, typename TJKey>
	void MfcToStdMapLoading(std::map<TKey, TVal> mapLoaded, CMap<TKey, TKeyA, TVal, TValA>&mapTObj)
	{
		auto& ar = *this;
		ASSERT(!ar.IsStoring());
		if (!ar.IsStoring()) {
			for (auto& pr : mapLoaded) {
				mapTObj.SetAt(pr.first, pr.second);
			}
		}
	}
	/// CMap<,,> 과 std::map<,> 상호 변환 저장/로드
	template<typename TKey, typename TKeyA, typename TVal, typename TValA, typename TJKey>
	void MfcToStdMapVal_imple(CMap<TKey, TKeyA, TVal, TValA>&mapTObj, TJKey key)
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			auto stdMapToStore = MfcToStdMapStr<TKey, TKeyA, TVal, TValA, TJKey>(mapTObj);
			ar << ar.KEYTpSKey(stdMapToStore, key);
		}
		else {
			std::map<TKey, TVal> mapLoaded;
			ar >> ar.KEYTpSKey(mapLoaded, key);
			MfcToStdMapLoading<TKey, TKeyA, TVal, TValA, TJKey>(mapLoaded, mapTObj);
		}
	}
#define ArcMfcMapVal(ar, v) ar.MfcToStdMapVal_imple(VAR2STR(v))
	/// CMap<TKey, TKeyA, TVal, TValA> 저장 (CMap을 std::map으로 변환하여 처리)
#define Arc_MfcMapVal(v) ArcMfcMapVal(ar, v)
	template<typename TKey, typename TKeyA, typename TObj, typename TJKey>
	void MfcToStdMapPbj_imple(CMap<TKey, TKeyA, TObj*, TObj*>&mapTObj, TJKey key)
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			auto stdMapToStore = MfcToStdMapStr<TKey, TKeyA, TObj*, TObj*, TJKey>(mapTObj);
			ar.StdMapPtrObj_1(stdMapToStore, key);
			//ar << ar.KEYTpSKey(stdMapToStore, key);
		}
		else {
			std::map<TKey, TObj*> mapLoaded;
			ar.StdMapPtrObj_1(mapLoaded, key);
			//ar >> ar.KEYTpSKey(mapLoaded, key);
			MfcToStdMapLoading<TKey, TKeyA, TObj*, TObj*, TJKey>(mapLoaded, mapTObj);
		}
	}
#define ArcMfcMapPbj(ar, v) ar.MfcToStdMapPbj_imple(VAR2STR(v))
	/// CMap<TKey, TKeyA, TObj*, TObj*> 저장 (CMap을 std::map으로 변환하여 처리, 포인터)
#define Arc_MfcMapPbj(v) ArcMfcMapPbj(ar, v)
	/// CMap<,,> 과 std::map<,> 상호 변환 저장/로드
	template<typename TKey, typename TKeyA, typename TVal, typename TValA, typename TJKey>
	void MfcMapIntStrCnv(CMap<TKey, TKeyA, TVal, TValA>&mapTObj, TJKey key)
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			auto stdMapToStore = MfcToStdMapStr<TKey, TKeyA, TVal, TValA, TJKey>(mapTObj);
			StdMapIntStrCnv(stdMapToStore, key);
		}
		else {
			std::map<TKey, TVal> mapLoaded;
			StdMapIntStrCnv(mapLoaded, key);
			MfcToStdMapLoading<TKey, TKeyA, TVal, TValA, TJKey>(mapLoaded, mapTObj);
		}
	}
	/// Mfc CMap<int, CString> std::map<int, string> 변환 후, 키와 값을 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
#define ArcMfcMapIntStrCnv(ar, v) ar.MfcMapIntStrCnv(VAR2STR(v))
/// CMap<int, CString> 저장 (CMap을 std::map으로 변환 후 키와 값을 wstring으로 처리)
#define Arc_MfcMapIntStrCnv(v) ArcMfcMapIntStrCnv(ar, v)
	/// CMap<,,> 과 std::map<,> 상호 변환 저장/로드
	template<typename TKey, typename TKeyA, typename TObj, typename TJKey>
	void MfcMapIntPbjCnv(CMap<TKey, TKeyA, TObj*, TObj*>&mapTObj, TJKey key)
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			auto stdMapToStore = MfcToStdMapStr<TKey, TKeyA, TObj*, TObj*, TJKey>(mapTObj);
			StdMapIntPbjCnv(stdMapToStore, key);
		}
		else {
			std::map<TKey, TObj*> mapLoaded;
			StdMapIntPbjCnv(mapLoaded, key);
			MfcToStdMapLoading<TKey, TKeyA, TObj*, TObj*, TJKey>(mapLoaded, mapTObj);
		}
	}
	/// Mfc CMap<int, CObject*> std::map<int, TObj*> 변환 후, 키 int를 wstring 으로 변환하여 저장(디폴트)/로드(커스텀 필수) 한다.
#define ArcMfcMapIntPbjCnv(ar, v) ar.MfcMapIntPbjCnv(VAR2STR(v))
/// CMap<int, TObj*> 저장 (CMap을 std::map으로 변환 후 키를 wstring으로 처리, 포인터)
#define Arc_MfcMapIntPbjCnv(v) ArcMfcMapIntPbjCnv(ar, v)
#pragma endregion ]MfcToStdMap
	//ar.StdMapPtrObj_1(ar, VAR2STR(mapTObj))
	/* 2차원 배열 vector<vector<>>는 이런 식이다. r은 뭐지
	<m_vecOptionValue __type__="array">
		 <item>
			<inItem __type__="array">
			  <item>Opt_1_1</item>
			  <item>Opt_1_2</item>
			  <item>Opt_1_3</item>
			  <item>Opt_1_4</item>
			</inItem>
		 </item>
		 <item>
			<inItem __type__="array">
			  <item>Opt_2_1</item>
			  <item>Opt_2_2</item>
			  <item>Opt_2_3</item>
			  <item>Opt_2_4</item>
			</inItem>
		 </item>
		 <item>
			<inItem __type__="array">
			  <item>Opt_3_1</item>
			  <item>Opt_3_2</item>
			  <item>Opt_3_3</item>
			  <item>Opt_3_4</item>
			</inItem>
		 </item>
	</m_vecOptionValue>
		/// ArrayOfValJSerialize_VectorCString: std::vector<std::vector<CString>> 처리 (하위 호환성용)
		template<typename TContainer>
		void ArrayOfValJSerialize_VectorCString(CJXArchive& ar, TContainer& lstTObj, wstring key)
		{
			// 템플릿 버전으로 위임
			ArrayOfValJSerialize_VectorVector(ar, lstTObj, key);
		}
	*/
	/// 데이터 Before Store 와 After Load 둘다 람다 함수로 명시
	/// 멤버 필드변수가 없어서, getter하고 setter 람다 함수를 제공 하는 경우
	/// 키와, 변수타입을 명시해 줘야 하는 경우
	/// if(ar.IsStore()) 블럭에 상관 없이 블럭 밖에 한번만 해주면 된다.
#define ArcValBfrAndAft(ar, key, TYPE, rdStore, rdLoad) ValBfrAndAft<TYPE>(ar, L#key, rdStore, rdLoad)
#ifdef _Sample__
	ArcValBfrAndAft(ar, m_aBarY1.Gap, long,
		[&](auto& v) { v = m_aBarY1.GetGap(); },
		[&](auto& v) { m_aBarY1.SetGap(v); });
#endif // _Sample__
	//#define ArcNoObj(ar, subObj) ar.NoObjItself(ar, VAR2STR(subObj))//
#define ArcObject(ar, subObj) ar.ObjectItself(VAR2STR(subObj))//
	/// 필드 중 CObject* 타입 의 JSerialize
/// subObj가 shared_ptr 또는 일반 포인터이고 empty 이거나 NULL 일 수 있다.
#define ArcPtrObj(ar, subObj) ar.ObjectPointer(VAR2STR(subObj))
#define ArcPtrObjNoDel(ar, subObj) ar.ObjectPointer(VAR2STR(subObj), false)
//#define ArcPtrNoObj(ar, subObj) ar.NoObjPointer(ar, VAR2STR(subObj))
/// 필드 중 CObject* 타입 의 JSerialize
/// subObj가 shared_ptr 또는 일반 포인터이고 empty 이거나 NULL 일 수 있다.
#define ArcShpObj(ar, subObj) ar.ObjectSharedPtr(VAR2STR(subObj))
	//#define ArcObject(ar, subObj) subObj.Serialize(CJXArchive(ar, L#subObj))//
	 //     SubPtrObj 와 글자수 맞춤	
#ifdef _Never_Used_
#define Struct_JSerialize(ar, rbj)	ar.StructWithKeys_2(VAR2STR(rbj))
#endif // _Never_Used_
/// 필드 중 KSharedPtrList<CMyData> CObject 타입 의 JSerialize
/// ex: using CParamObjList = KList<SHP<CParamObj>>;
#define ArcStdVectRefObj(ar, lstTObj)	ar.StdListRefObj_1<std::vector>(VAR2STR(lstTObj))//
//#define ArcStdVectRefNoObj(ar, lstTObj)	ar.StdListRefNoObj_1<std::vector>(ar, VAR2STR(lstTObj))//
#define ArcStdVectPtrObj(ar, lstTObj)	ar.StdListPtrObj_1<std::vector>(VAR2STR(lstTObj))//
#define ArcStdVectShpObj(ar, lstTObj)	ar.StdListShpObj_1(VAR2STR(lstTObj))//
//#define ArcStdVectRefObj(ar, lstTObj)	ar.StdVectRefObj_1<std::vector>(ar, VAR2STR(lstTObj))//
#define ArcStdListRefObj(ar, lstTObj)	ar.StdListRefObj_1<std::list>(VAR2STR(lstTObj))//
#define ArcStdListShpObj(ar, lstTObj)	ar.StdListShpObj_1(VAR2STR(lstTObj))//
#define ArcStdListPtrObj(ar, lstTObj)	ar.StdListPtrObj_1(VAR2STR(lstTObj))//
#define ArcStdVect_variant(ar, rbj)		ar.StdVect_variant_1(VAR2STR(rbj))
	//																<RECT, int>
#define ArcValue_RECT(ar, rbj)			ar.Value_RECT(VAR2STR(rbj))
#define ArcValue_POINT(ar, rbj)			ar.Value_POINT(VAR2STR(rbj))
#define ArcStdVect_POINT(ar, rbj)		ar.StdVect_POINT(VAR2STR(rbj))
#define ArcStdVect_PtrPOINT(ar, rbj)	ar.StdVect_PtrPOINT(VAR2STR(rbj))
#define ArcStdVect_ShpPOINT(ar, rbj)	ar.StdVect_ShpPOINT(VAR2STR(rbj))
#define ArcStdTArr_POINT(ar, rbj)		ar.StdTArr_POINT(VAR2STR(rbj))
#define ArcStdVect2D_Val(ar, rbj)		ar.StdVect2D_Val_1(VAR2STR(rbj))
#define ArcStdVect3D_Val(ar, rbj)		ar.StdVect3D_Val_1(VAR2STR(rbj))// std::vector<std::vector<std::vector<TVal>>> 전용
#define ArcStdVect2D_variant(ar, rbj)	ar.StdVect2D_variant(VAR2STR(rbj))
#define ArcStdVectPair(ar, rbj)			ar.StdVect_Pair(VAR2STR(rbj))
/// std::vector<std::pair<TKey, TVal>> 저장 (pair를 배열 [key, value] 형태로 저장)
#define Arc_StdVectPair(v)				ArcStdVectPair(ar, v)
#define ArcStdVect2DShpObj(ar, rbj)		ar.StdVect2D_ShpObj_1(VAR2STR(rbj))//dwk: 2025-11-26 08:56 
#define ArcStdVect2DPtrObj(ar, rbj)		ar.StdVect2D_PtrObj_1(VAR2STR(rbj))// std::vector<std::vector<TObj*>> 전용
#define ArcStdVect2DRefObj(ar, rbj)		ar.StdVect2D_RefObj_1(VAR2STR(rbj))// std::vector<std::vector<TObj>> 전용 (객체 직접)
#define ArcStdTArrShp2D(ar, lstTObj)	ar.StdTArrShp2D_1(VAR2STR(lstTObj))//
/// 필드 중 CList<SHP<TObj>> 또는 CList<TObj*> 타입 의 JSerialize
/// ex: CList<KObject*, KObject*> m_lstLink;
//#define MfcListOfObj_JSerialize(ar, lstTObj)  MfcListOfObjToJsonWithKey(ar, lstTObj, L#lstTObj)//
#define ArcMfcListPtrObj(ar, lstTObj)			ar.MfcListPtrObj_1(VAR2STR(lstTObj))//
#define ArcMfcListShpObj(ar, lstTObj)			ar.MfcListShpObj_1(VAR2STR(lstTObj))//not implemented
#define ArcMfcListRefObj(ar, lstTObj)			ar.MfcListRefObj_1(VAR2STR(lstTObj))//not implemented
#define ArcMfcArrayPtrObj(ar, lstTObj)			ar.MfcArrayPtrObj_1(VAR2STR(lstTObj))//
#define ArcMfcArrayShpObj(ar, lstTObj)			ar.MfcArrayShpObj_1(VAR2STR(lstTObj))
#define ArcMfcArrayRefObj(ar, lstTObj)			ar.MfcArrayRefObj_1(VAR2STR(lstTObj))//not implemented
#define Arc_MfcArrayShpObj(v) ArcMfcArrayShpObj(ar, v)
#define ArcMfcArrayPtrVariant(ar, lstTObj)	ar.MfcArrayPtrVariant_1(VAR2STR(lstTObj))//
#define ArcMfcArrayShpVariant(ar, lstTObj)	ar.MfcArrayShpVariant_1(VAR2STR(lstTObj))//not implemented
#define ArcMfcArrayRefVariant(ar, lstTObj)	ar.MfcArrayRefVariant_1(VAR2STR(lstTObj))//not implemented
#define ArcStdMapShpObj(ar, mapTObj)			ar.StdMapShpObj_1(VAR2STR(mapTObj))//
#define ArcStdMapPtrObj(ar, mapTObj)			ar.StdMapPtrObj_1(VAR2STR(mapTObj))//
#define ArcStdMapRefObj(ar, mapTObj)			ar.StdMapRefObj_1(VAR2STR(mapTObj))//not implemented
#define ArcMfcMapPtrObj(ar, mapTObj)			ar.MfcMapPtrObj_1(VAR2STR(mapTObj))//[deprecated]
#define ArcMapWordPtr(ar, mapTObj, TObj) ar.MapWordPtr_1<TObj>(VAR2STR(mapTObj))//
/// 줄인 형태. >> 와 << 를 배제
/// Primitive Type, CString, std::string, vector<..>, list<..>, map<wstring,..>, CStringArray
/// 	 아래 Arc_ 로 시작 하는 매크로 함수는 CArchive변수가 ar 이라는 전제로 만든 함수
/// 단순변수 (Primitive Type) 저장. 변수명을 키로 쓴다. 예: "m_nValue"
#define Arc_Value(v) ArcValue(ar, v)
/// 키 이름을 변수명이 아닌 다른 값으로 지정할 때 사용
#define Arc_ValueKey(v, k) ArcValueSKey(ar, v, k)
/// variant_t 타입 변수 저장 (키 이름 지정)
#define Arc_variant_t(v, k) ArcVariant(ar, v, k)
/// std::list<단순변수> 저장
#define Arc_StdListVal(v) ArcValue(ar, v)
/// std::vector<단순변수> 저장
#define Arc_StdVectVal(v) ArcValue(ar, v)
/// CArray<Primitive Type> 저장
#define Arc_MfcArray(v) ArcValue(ar, v)
/// CStringList / CList<단순문자열> 저장
#define Arc_MfcList(v)  ArcValue(ar, v)
/// std::map<std::wstring, wstring> 저장
#define Arc_StdMapWStr(v) ArcValue(ar, v)
/// std::map<std::wstring, int> 저장
#define Arc_StdMapInt(v) ArcValue(ar, v)
/// std::map<std::wstring, double> 저장
#define Arc_StdMapDouble(v) ArcValue(ar, v)
/// enum 타입 저장 (내부적으로 정수로 변환하여 저장)
#define Arc_Enum(TYPE, v) ArcEnum(ar, TYPE, v)
/// std::vector<TObj> 저장 (객체 직접 저장, 참조)
#define Arc_StdVectRefObj(v) ArcStdVectRefObj(ar, v)
//#define Arc_StdVectRefoObj(v) ArcStdVectRefNoObj(ar, v) // std::List<shared_ptr<CObject>>
/// std::vector<TObj*> 저장 (포인터 배열)
#define Arc_StdVectPtrObj(v) ArcStdVectPtrObj(ar, v)
/// std::vector<shared_ptr<TObj>> 저장
#define Arc_StdVectShpObj(v) ArcStdVectShpObj(ar, v)
/// std::list<TObj> 저장 (객체 직접 저장, 참조)
#define Arc_StdListRefObj(v) ArcStdListRefObj(ar, v)
/// std::list<shared_ptr<TObj>> 저장
#define Arc_StdListShpObj(v) ArcStdListShpObj(ar, v)
/// std::list<TObj*> 저장 (포인터 리스트)
#define Arc_StdListPtrObj(v) ArcStdListPtrObj(ar, v)
//#define ArcListObj(v) MfcListOfObj_JSerialize(ar, v)
/// CList<TObj*, TObj*> 저장 (MFC 리스트, 포인터)
#define Arc_MfcListPtrObj(v) ArcMfcListPtrObj(ar, v)
/// CArray<TObj*, TObj*> 저장 (MFC 배열, 포인터)
#define Arc_MfcArrayPtrObj(v) ArcMfcArrayPtrObj(ar, v)
/// CArray<TObj, TObj> 저장 (MFC 배열, 객체 직접)
#define Arc_MfcArrayRefObj(v) ArcMfcArrayRefObj(ar, v)
/// CArray<_variant_t*, _variant_t*> 저장 (MFC 배열, variant 포인터)
#define Arc_MfcArrayPtrVariant(v) ArcMfcArrayPtrVariant(ar, v)
/// std::map<wstring, shared_ptr<TObj>> 저장
#define Arc_StdMapShpObj(v) ArcStdMapShpObj(ar, v)
/// std::map<wstring, TObj*> 저장
#define Arc_StdMapPtrObj(v) ArcStdMapPtrObj(ar, v)
/// CMap<wstring, wstring, TObj*, TObj*> 저장 (deprecated)
#define Arc_MfcMapPtrObj(v) ArcMfcMapPtrObj(ar, v)
//#define ArcStdMapPtrObj(ar, mapTObj) ar.StdMapPtrObj_1(ar, VAR2STR(mapTObj))
/// 객체 직접 저장 (Serialize 메서드 호출)
#define Arc_Object(v) ArcObject(ar, v)
/// 객체 포인터 저장 (TObj*)
#define Arc_PtrObj(v) ArcPtrObj(ar, v)
#define Arc_PtrObjNoDel(v) ArcPtrObjNoDel(ar, v)

/// shared_ptr 객체 저장
#define Arc_ShpObj(v) ArcShpObj(ar, v)
/// POINT 구조체 저장
#define Arc_Value_POINT(v) ArcValue_POINT(ar, v)
/// RECT 구조체 저장
#define Arc_Value_RECT(v) ArcValue_RECT(ar, v)
/// std::vector<_variant_t> 저장
#define Arc_StdVect_variant(v) ArcStdVect_variant(ar, v)
/// std::vector<std::vector<TVal>> 저장 (2차원 값 벡터)
#define Arc_StdVect2D_Val(v) ArcStdVect2D_Val(ar, v)
/// std::vector<std::vector<std::vector<TVal>>> 저장 (3차원 값 벡터)
#define Arc_StdVect3D_Val(v) ArcStdVect3D_Val(ar, v)
/// std::vector<std::vector<_variant_t>> 저장 (2차원 variant 벡터)
#define Arc_StdVect2D_variant(v) ArcStdVect2D_variant(ar, v)
/// std::vector<std::vector<shared_ptr<TObj>>> 저장 (2차원 shared_ptr 벡터)
#define Arc_StdVect2DShpObj(v) ArcStdVect2DShpObj(ar, v)
/// std::vector<std::vector<TObj*>> 저장 (2차원 포인터 벡터)
#define Arc_StdVect2DPtrObj(v) ArcStdVect2DPtrObj(ar, v)
/// std::vector<std::vector<TObj>> 저장 (2차원 객체 벡터, 객체 직접)
#define Arc_StdVect2DRefObj(v) ArcStdVect2DRefObj(ar, v)
/// std::vector<POINT> 저장
#define Arc_StdVect_POINT(v) ArcStdVect_POINT(ar, v)
/// std::vector<shared_ptr<POINT>> 저장
#define Arc_StdVect_ShpPOINT(v) ArcStdVect_ShpPOINT(ar, v)
/// std::vector<POINT*> 저장
#define Arc_StdVect_PtrPOINT(v) ArcStdVect_PtrPOINT(ar, v)
/// std::vector/std::list/std::array<POINT> 저장 (템플릿 컨테이너, POINT 타입)
#define Arc_StdTArr_POINT(v) ArcStdTArr_POINT(ar, v)
/// CMapWordToPtr<CObject> 저장
#define Arc_MapWordPtrObject(v) ArcMapWordPtr(ar, v, CObject)
/// CMapWordToPtr<TObj> 저장 (CObject가 아닌 경우)
#define Arc_MapWordPtrNoCObject(v, TObj) ArcMapWordPtr(ar, v, TObj)
//#define ArcCListPbj(v) ArcMapWordPtr(ar, v, CObject)
	template<typename TVal>
	void PArrayTval_impl(TVal * &pVal, wstring key, int& nSize, function<void(TVal*&, int)> cbRealloc) {
		auto& ar = *this;
		if (ar.IsStoring()) {
			std::vector<TVal> vecTmp(pVal, pVal + nSize);
			ar << ar.KEYTpSKey(vecTmp, key);
		}
		else {
			std::vector<TVal> vecTmp;
			ar >> ar.KEYTpSKey(vecTmp, key);
			int newSize = static_cast<int>(vecTmp.size());
			//ASSERT(newSize == nSize);//nSize=0일수 있지. 읽기 전이라 모를수 있으니. 쓸때는 갯수가 명확해야 하지만
			nSize = newSize;//가지고 돌아간다. 읽을 때는 중요 하지.
			cbRealloc(pVal, newSize);//읽을때는 vector로 읽으니 틀릴수 있슴.
			if (pVal && newSize > 0) {
				memcpy(pVal, vecTmp.data(), newSize * sizeof(TVal));
			}
		};
	}
	template<typename TVal>
	void PArrayTValNew(TVal * &pVal, wstring key, int& nSize) {
		PArrayTval_impl<TVal>(pVal, key, nSize, [nSize](TVal*& pb, int newSize) {
			if (pb) {
				delete (pb);
				pb = nullptr;
			}
			if (newSize > 0)
				pb = static_cast<TVal*>(new TVal[newSize]);
			});
	}
	template<typename TVal>
	void PArrayTValAlloc(TVal * &pVal, wstring key, int& nSize) {
		PArrayTval_impl<TVal>(pVal, key, nSize, [nSize](TVal*& pb, int newSize) {
			if (pb) {
				free(pb);
				pb = nullptr;
			}
			if (newSize > 0)
				pb = static_cast<TVal*>(calloc(newSize, sizeof(TVal)));
			});
	}
	/// 포인터 배열 처리 (HiddenLayer** hiddenLayers_ 같은 포인터 배열)
	/// 저장 시: 각 포인터에 대해 __class__ 속성 저장 후 직렬화
	/// 로드 시: 각 JSON 항목에서 UcDefaultCreateByClassName으로 객체 생성 후 역직렬화
	template<typename TObj>
	void PArrayPtrObj_impl(TObj * *&ppObj, wstring key, int& nSize,
		function<void(TObj**&, int)> cbRealloc) {
		DWKUSETRACE;//DWKFUNC;
		auto& ar = *this;
		try
		{
			CJXArchive ar2(*this, key.c_str());
			if (ar.IsStoring()) {
				ShJArr shArrNew = NEWSHP(UcJArr);
				// 각 포인터에 대해 저장
				for (int i = 0; i < nSize; i++) {
					if (ppObj[i]) {
						CJXArchive ari(*this);
						// Serialize 호출
						ppObj[i]->Serialize(ari);
						// __class__ 속성 저장
						CJXArchive::UcDefaultSaveClassName(ari.GetJson(), ppObj[i]);
						shArrNew->Arr()->Add(ari.GetJson(), false);
					}
				}
				ar2.SaveToUpperNode(shArrNew);
			}
			else {
				// 기존 포인터 배열 해제
				if (ppObj && nSize > 0) {
					for (int i = 0; i < nSize; i++) {
						if (ppObj[i])
							delete ppObj[i];
					}
				}
				if (ppObj) {
					cbRealloc(ppObj, 0); // 기존 배열 해제
				}

				// JSON에서 읽기
				ShJObj shObj = ar.GetJson();
				auto& j1 = ar.Jbj();
				ShJArr shArr;
				if (j1.Has(key)) {
					auto shv = j1.Get(key);
					if (shv->IsDic()) {
						auto& jd = *shv->Dic();
						if (jd.Has(TAG_ITM)) {
							auto shJit = jd.Get(TAG_ITM);
							if (shJit->IsArr())
								shArr = shJit;
						}
						else {
							ASSERT("array 인데 Dic에 item 키 도 없음." == nullptr);
						}
					}
					else if (shv->IsArr()) {
						shArr = shv;
					}
				}

				if (shArr) {
					auto pjarr = shArr->Arr();
					if (pjarr) {
						auto& jarr = *shArr->Arr();
						int newSize = static_cast<int>(jarr.size());
						nSize = newSize;

						// 새 포인터 배열 할당
						if (newSize > 0) {
							cbRealloc(ppObj, newSize);
							int i = 0;
							for (auto& shjv : jarr) {
								CJXArchive ari(*this, shjv);
								// UcDefaultCreateByClassName으로 객체 생성 (derived class 지원)
								TObj* pNewObj = CJXArchive::UcDefaultCreateByClassName<TObj>(shjv, {}, i);
								if (pNewObj) {
									// 역직렬화
									pNewObj->Serialize(ari);
									ppObj[i] = pNewObj;
								}
								else {
									ppObj[i] = nullptr;
								}
								i++;
							}
						}
						else {
							ppObj = nullptr;
						}
					}
				}
			}
		}
		catch (CException* e) {
			throw e;
		}
		catch (LPCWSTR serr) {
			serr;
			DWKTRACE(L"JSON error: %v", serr);
		}
	}
	template<typename TObj>
	void PArrayPtrObjNew(TObj * *&ppObj, wstring key, int& nSize) {
		PArrayPtrObj_impl<TObj>(ppObj, key, nSize, [](TObj**& pp, int newSize) {
			if (pp) {
				delete[] pp;
				pp = nullptr;
			}
			if (newSize > 0)
				pp = new TObj * [newSize];
			});
	}
	template<typename TObj>
	void PArrayPtrObjAlloc(TObj * *&ppObj, wstring key, int& nSize) {
		PArrayPtrObj_impl<TObj>(ppObj, key, nSize, [](TObj**& pp, int newSize) {
			if (pp) {
				free(pp);
				pp = nullptr;
			}
			if (newSize > 0)
				pp = static_cast<TObj**>(calloc(newSize, sizeof(TObj*)));
			});
	}
	/// 2차원 포인터 배열 처리 (double **m_ppClusterCenter 같은 2차원 배열)
	template<typename TVal>
	void PArray2DTVal_impl(TVal * *&ppVal, wstring key, int& nRow, int& nCol,
		function<void(TVal**&, int)> cbFreeRowPtrArray,  // 행 포인터 배열 해제: (ppVal, nRow)
		function<TVal * *(int)> cbAllocRowPtrArray,        // 행 포인터 배열 할당: (nRow) -> TVal**
		function<void(TVal*&)> cbFreeRow,                // 각 행 해제: (ppVal[i])
		function<TVal * (int)> cbAllocRow)                 // 각 행 할당: (nCol) -> TVal*
	{
		auto& ar = *this;
		if (ar.IsStoring()) {
			// 2차원 포인터를 vector<vector<>>로 변환
			std::vector<std::vector<TVal>> vec2D;
			for (int i = 0; i < nRow; i++) {
				std::vector<TVal> row(ppVal[i], ppVal[i] + nCol);
				vec2D.push_back(row);
			}
			this->StdVect2D_Val_1(vec2D, key);
		}
		else {
			// vector<vector<>>로 읽은 후 2차원 포인터로 변환
			std::vector<std::vector<TVal>> vec2D;
			this->StdVect2D_Val_1(vec2D, key);

			// 기존 메모리 해제
			if (ppVal) {
				for (int i = 0; i < nRow; i++) {
					if (ppVal[i]) {
						cbFreeRow(ppVal[i]);
					}
				}
				cbFreeRowPtrArray(ppVal, nRow);
			}

			// 새 크기로 할당
			nRow = static_cast<int>(vec2D.size());
			if (nRow > 0) {
				nCol = static_cast<int>(vec2D[0].size());
				// 행 포인터 배열 할당
				ppVal = cbAllocRowPtrArray(nRow);
				// 각 행 할당 및 데이터 복사
				for (int i = 0; i < nRow; i++) {
					int colSize = static_cast<int>(vec2D[i].size());
					if (colSize > 0) {
						ppVal[i] = cbAllocRow(colSize);
						memcpy(ppVal[i], vec2D[i].data(), colSize * sizeof(TVal));
					}
					else {
						ppVal[i] = nullptr;
					}
				}
			}
			else {
				nCol = 0;
				ppVal = nullptr;
			}
		}
	}
	template<typename TVal>
	void PArray2DTValNew(TVal * *&ppVal, wstring key, int& nRow, int& nCol) {
		PArray2DTVal_impl<TVal>(ppVal, key, nRow, nCol,
			[](TVal**& pp, int nR) {  // 행 포인터 배열 해제
				if (pp) {
					delete[] pp;
					pp = nullptr;
				}
			},
			[](int nR) -> TVal** {  // 행 포인터 배열 할당
				return new TVal * [nR];
			},
			[](TVal*& p) {  // 각 행 해제
				if (p) {
					delete[] p;
					p = nullptr;
				}
			},
			[](int nC) -> TVal* {  // 각 행 할당
				return new TVal[nC];
			}
		);
	}
	template<typename TVal>
	void PArray2DTValAlloc(TVal * *&ppVal, wstring key, int& nRow, int& nCol) {
		PArray2DTVal_impl<TVal>(ppVal, key, nRow, nCol,
			[](TVal**& pp, int nR) {  // 행 포인터 배열 해제
				if (pp) {
					free(pp);
					pp = nullptr;
				}
			},
			[](int nR) -> TVal** {  // 행 포인터 배열 할당
				return static_cast<TVal**>(calloc(nR, sizeof(TVal*)));
			},
			[](TVal*& p) {  // 각 행 해제
				if (p) {
					free(p);
					p = nullptr;
				}
			},
			[](int nC) -> TVal* {  // 각 행 할당
				return static_cast<TVal*>(calloc(nC, sizeof(TVal)));
			}
		);
	}
	/// C 스타일 배열 처리 (int carr[5] 같은 고정 크기 배열)
	template<typename TVal, size_t N>
	void ScArrayTVal(TVal(&arr)[N], wstring key) {
		auto& ar = *this;
		if (ar.IsStoring()) {
			std::vector<TVal> vecTmp;// (arr, arr + N);
			for (int i = 0; i < N; i++)
				vecTmp.push_back(arr[i]);
			ar << ar.KEYTpSKey(vecTmp, key);
		}
		else {
			std::vector<TVal> vecTmp;
			ar >> ar.KEYTpSKey(vecTmp, key);
			for (int i = 0; i < vecTmp.size() && i < N; i++)
				arr[i] = vecTmp[i];
		}
	}
	//[deprecated]
	void PArrayBOOL_impl(BOOL * &pBool, wstring key, int nSize, function<void(BOOL*&, int)> cbRealloc) {
		auto& ar = *this;
		if (ar.IsStoring()) {
			std::vector<BOOL> vecTmp(pBool, pBool + nSize);
			ar << ar.KEYTpSKey(vecTmp, key);
		}
		else {
			std::vector<BOOL> vecTmp;
			ar >> ar.KEYTpSKey(vecTmp, key);
			int newSize = static_cast<int>(vecTmp.size());
			ASSERT(newSize == nSize);
			cbRealloc(pBool, newSize);
			if (pBool && newSize > 0) {
				memcpy(pBool, vecTmp.data(), newSize * sizeof(BOOL));
			}
		};
		//ArcValueSKey(*this, vecTmp, key);
	}
	//[deprecated]
	void PArrayBOOLAlloc(BOOL * &pBool, wstring key, int nSize) {
		PArrayBOOL_impl(pBool, key, nSize, [nSize](BOOL*& pb, int newSize) {
			if (pb) {
				free(pb);
				pb = nullptr;
			}
			if (newSize > 0)
				pb = static_cast<BOOL*>(calloc(newSize, sizeof(BOOL)));
			});
	}
	void PArrayBOOLNew(BOOL * &pBool, wstring key, int nSize) {
		PArrayBOOL_impl(pBool, key, nSize, [nSize](BOOL*& pb, int newSize) {
			if (pb) {
				delete (pb);
				pb = nullptr;
			}
			if (newSize > 0)
				pb = static_cast<BOOL*>(calloc(newSize, sizeof(BOOL)));
			});
	}
#define ArcPArrayBOOLNew(ar, parb, sz) ar.PArrayTValNew<BOOL>(VAR2STR(parb), sz) 
#define Arc_PArrayBOOLNew(parb, sz) ArcPArrayBOOLNew(ar, parb, sz) 
#define ArcPArrayBOOLAlloc(ar, parb, sz) ar.PArrayTValAlloc<BOOL>(VAR2STR(parb), sz)
	/// BOOL* 포인터 배열 저장 (calloc/free 사용)
#define Arc_PArrayBOOLAlloc(parb, sz) ArcPArrayBOOLAlloc(ar, parb, sz) 
#define ArcPArrayDoubleNew(ar, parb, sz) ar.PArrayTValNew<double>(VAR2STR(parb), sz)
/// double* 포인터 배열 저장 (new/delete[] 사용)
#define Arc_PArrayDoubleNew(parb, sz) ArcPArrayDoubleNew(ar, parb, sz) 
#define ArcPArrayDoubleAlloc(ar, parb, sz) ar.PArrayTValAlloc<double>(VAR2STR(parb), sz)
/// double* 포인터 배열 저장 (calloc/free 사용)
#define Arc_PArrayDoubleAlloc(parb, sz) ArcPArrayDoubleAlloc(ar, parb, sz) 
#define ArcPArray2DDoubleNew(ar, pparb, nRow, nCol) ar.PArray2DTValNew<double>(VAR2STR(pparb), nRow, nCol)
/// double** 2차원 포인터 배열 저장 (new/delete[] 사용)
#define Arc_PArray2DDoubleNew(pparb, nRow, nCol) ArcPArray2DDoubleNew(ar, pparb, nRow, nCol)
#define ArcPArray2DDoubleAlloc(ar, pparb, nRow, nCol) ar.PArray2DTValAlloc<double>(VAR2STR(pparb), nRow, nCol)
/// double** 2차원 포인터 배열 저장 (calloc/free 사용)
#define Arc_PArray2DDoubleAlloc(pparb, nRow, nCol) ArcPArray2DDoubleAlloc(ar, pparb, nRow, nCol)
#define ArcPArray2DIntNew(ar, pparb, nRow, nCol) ar.PArray2DTValNew<int>(VAR2STR(pparb), nRow, nCol)
/// int** 2차원 포인터 배열 저장 (new/delete[] 사용)
#define Arc_PArray2DIntNew(pparb, nRow, nCol) ArcPArray2DIntNew(ar, pparb, nRow, nCol)
#define ArcPArray2DIntAlloc(ar, pparb, nRow, nCol) ar.PArray2DTValAlloc<int>(VAR2STR(pparb), nRow, nCol)
/// int** 2차원 포인터 배열 저장 (calloc/free 사용)
#define Arc_PArray2DIntAlloc(pparb, nRow, nCol) ArcPArray2DIntAlloc(ar, pparb, nRow, nCol)
	/// 2차원 포인터 배열 처리 (real_t** weights_ 같은 2차원 배열, size_t/int 크기 지원)
	/// 기존 PArray2DTVal_impl을 재사용하여 중복 코드 최소화
	template<typename TVal, typename TSize>
	void PArray2DTValNew_size_t(TVal * *&ppVal, wstring key, TSize & nRow, TSize & nCol) {
		int nRowInt = static_cast<int>(nRow);
		int nColInt = static_cast<int>(nCol);
		PArray2DTValNew<TVal>(ppVal, key, nRowInt, nColInt);
		nRow = static_cast<TSize>(nRowInt);
		nCol = static_cast<TSize>(nColInt);
	}
	template<typename TVal, typename TSize>
	void PArray2DTValAlloc_size_t(TVal * *&ppVal, wstring key, TSize & nRow, TSize & nCol) {
		int nRowInt = static_cast<int>(nRow);
		int nColInt = static_cast<int>(nCol);
		PArray2DTValAlloc<TVal>(ppVal, key, nRowInt, nColInt);
		nRow = static_cast<TSize>(nRowInt);
		nCol = static_cast<TSize>(nColInt);
	}
#define ArcPArrayIntAlloc(ar, parb, sz) ar.PArrayTValAlloc<int>(VAR2STR(parb), sz)
	/// int* 포인터 배열 저장 (calloc/free 사용)
#define Arc_PArrayIntAlloc(parb, sz) ArcPArrayIntAlloc(ar, parb, sz) 
#define ArcPArrayIntNew(ar, parb, sz) ar.PArrayTValNew<int>(VAR2STR(parb), sz)
/// int* 포인터 배열 저장 (new/delete[] 사용)
#define Arc_PArrayIntNew(parb, sz) ArcPArrayIntNew(ar, parb, sz) 
	/// C 스타일 배열용 매크로
	/// 사용 예: int carr[5]; Arc_ScArrayTVal(carr);
#define ArcScArrayTVal(ar, arr) ar.ScArrayTVal(VAR2STR(arr))
/// C 스타일 고정 크기 배열 저장 (예: int arr[5])
#define Arc_ScArrayTVal(arr) ArcScArrayTVal(ar, arr)
	/// 2차원 포인터 배열용 매크로 (size_t/int 크기 지원, 템플릿)
	/// 사용 예: real_t** weights_; size_t prevNodeCount_, nodeCount_;
	///         Arc_C_2DPtrValNew(weights_, prevNodeCount_, nodeCount_);
	///         또는 int nRow, nCol; Arc_C_2DPtrValNew(weights_, nRow, nCol);
#define ArcC2DPtrValNew(ar, pparb, nRow, nCol) ar.PArray2DTValNew_size_t(VAR2STR(pparb), nRow, nCol)
/// TVal** 2차원 포인터 배열 저장 (size_t/int 크기, new/delete[] 사용)
#define Arc_C_2DPtrValNew(pparb, nRow, nCol) ArcC2DPtrValNew(ar, pparb, nRow, nCol)
#define ArcC2DPtrValAlloc(ar, pparb, nRow, nCol) ar.PArray2DTValAlloc_size_t(VAR2STR(pparb), nRow, nCol)
/// TVal** 2차원 포인터 배열 저장 (size_t/int 크기, calloc/free 사용)
#define Arc_C_2DPtrValAlloc(pparb, nRow, nCol) ArcC2DPtrValAlloc(ar, pparb, nRow, nCol)
	/// 포인터 배열용 매크로
	/// 사용 예: HiddenLayer** hiddenLayers_; int nHiddenLayers;
	///         Arc_PArrayPtrObjNew(hiddenLayers_, nHiddenLayers);
#define ArcPArrayPtrObjNew(ar, pparb, nSize) ar.PArrayPtrObjNew(VAR2STR(pparb), nSize)
/// TObj** 객체 포인터 배열 저장 (new/delete[] 사용, 다형성 지원)
#define Arc_PArrayPtrObjNew(pparb, nSize) ArcPArrayPtrObjNew(ar, pparb, nSize)
#define ArcPArrayPtrObjAlloc(ar, pparb, nSize) ar.PArrayPtrObjAlloc(VAR2STR(pparb), nSize)
/// TObj** 객체 포인터 배열 저장 (calloc/free 사용, 다형성 지원)
#define Arc_PArrayPtrObjAlloc(pparb, nSize) ArcPArrayPtrObjAlloc(ar, pparb, nSize)
#pragma region //[키가 정수인 맵 을 CMapWordPtr 로 저장 하는 루틴
	//[deprecated]
	template <typename TObj>
	void ConvertMapIntToWordPtr(const CMap<int, int, TObj*, TObj*>&src, CMapWordToPtr & dst)
	{
		dst.RemoveAll();
		POSITION pos = src.GetStartPosition();
		while (pos)
		{
			int key;
			TObj* pVal = nullptr;
			src.GetNextAssoc(pos, key, pVal);
			// WORD는 0~65535 이므로, 범위 확인
			ASSERT(key >= 0 && key <= 0xFFFF);
			dst.SetAt(static_cast<WORD>(key), pVal);
		}
	}
	//[deprecated]
	template <typename TObj>
	void ConvertMapWordPtrToInt(const CMapWordToPtr & src, CMap<int, int, TObj*, TObj*>&dst)
	{
		dst.RemoveAll();
		POSITION pos = src.GetStartPosition();
		while (pos)
		{
			WORD key;
			void* pVal = nullptr;
			src.GetNextAssoc(pos, key, pVal);
			dst.SetAt(static_cast<int>(key), reinterpret_cast<TObj*>(pVal));
		}
	}
	/// CMap <int, int, KObject*, KObject*> 을 MapWordPtr 로 바꿧 했었는데,
	/// 이제 MfcMapIntPbjCnv -> StdMapIntPbjCnv 로 바꿨다.//dwk: 2025-11-13 11:21 
	//[deprecated]
	template<typename TObj>
	void MapIntToWordPtr(CMap<int, int, TObj*, TObj*>&mpInt, wstring key)
	{
		auto& ar = *this;
		CMapWordToPtr mapTemp;
		if (ar.IsStoring()) {
			ConvertMapIntToWordPtr(mpInt, mapTemp);
			//Arc_MapWordPtrObject(mapTemp);
//#define ArcMapWordPtr(ar, mapTObj, TObj) ar.MapWordPtr_1<TObj>(ar, VAR2STR(mapTObj))//
			ar.MapWordPtr_1<TObj>(mapTemp, key);
		}
		else {
			ar.MapWordPtr_1<TObj>(mapTemp, key);
			//Arc_MapWordPtrObject(mapTemp);
			ConvertMapWordPtrToInt(mapTemp, mpInt);
		}
	}
	//[deprecated]
#define ArcMapIntToWordPtr(ar, mapTObj) ar.MapIntToWordPtr(VAR2STR(mapTObj))
/// CMap<int, int, TObj*, TObj*>를 CMapWordToPtr로 변환하여 저장 (deprecated)
#define Arc_MapIntToWordPtr(mapTObj) ArcMapIntToWordPtr(ar, mapTObj)
#pragma endregion //]키가 정수인 맵 을 CMapWordPtr 로 저장 하는 루틴
	void ArcVariableSave_3(CJXArchive & ari, _variant_t & vari) {
		switch (vari.vt)
		{
		case VT_BOOL://enum VARENUM
		{
			bool bVal = (bool)vari;
			ArcValueSKey(ari, bVal, L"value");
			break;
		}
		case VT_R4:
		case VT_R8:
		{
			double dVal = (double)vari;
			ArcValueSKey(ari, dVal, L"value");
			break;
		}
		case VT_BSTR:
		{
			CString sVal = (CString)vari;
			ArcValueSKey(ari, sVal, L"value");
			break;
		}
		case VT_I2:
		case VT_I4:
		case VT_INT:
		{
			int iVal = (int)vari;
			ArcValueSKey(ari, iVal, L"value");
			break;
		}
		case VT_I8:
		{
			__int64 i64Temp{ (__int64)vari };
			ArcValueSKey(ari, i64Temp, L"value");
			vari = (__int64)i64Temp;
			break;
		}
		default:
			ASSERT(0);//이 정도 지원 하면 다 한 듯.
			break;
		}
	}
	inline void ArcVariableLoad_3(CJXArchive & ari, _variant_t & vari, int vt)
	{
		CString szTemp;
		short nTemp{};
		double fTemp{};
		bool bTemp{};
		switch ((VARTYPE)vt)
		{
		case VT_BOOL:
			ArcValueSKey(ari, bTemp, L"value");
			vari = (bool)bTemp;
			break;
		case VT_R4:
		case VT_R8:
			ArcValueSKey(ari, fTemp, L"value");
			vari = (double)fTemp;
			break;
		case VT_BSTR:
			ArcValueSKey(ari, szTemp, L"value");
			vari = (_bstr_t)szTemp;
			break;
		case VT_I2:
		case VT_I4:
		case VT_INT:
		{
			int iTemp{ 0 };
			ArcValueSKey(ari, iTemp, L"value");
			vari = (int)iTemp;
			break;
		}
		case VT_I8:
		{
			__int64 i64Temp{ 0 };
			ArcValueSKey(ari, i64Temp, L"value");
			vari = (__int64)i64Temp;
		}
		break;
		default:
			ASSERT(0);//이 정도 지원 하면 다 한 듯.
			break;
		}
	}
	/// <summary>
	/// 객체를 JSON으로 클립보드에 복사한다. 일단 보류
	/// </summary>
	/// <typeparam name="TListObj"></typeparam>
	/// <param name="pw"></param>
	/// <param name="listObj"></param>
	/// <param name="fmtClip"></param>
	/// <returns></returns>
	template<typename TListObj>
	bool UcCopyListByJson(CWnd * pw, TListObj & listObj, UINT fmtClip, LPCSTR sMode)//dwk: 2025-02-17 13:30 
	{
		//try {
		//	pw->OpenClipboard();
		//	EmptyClipboard();
		//	CMemoryFile file;
		//	auto shAr = CreateJXArchive(&file, CArchive::store, sMode); // CJXArchive CreateJsonArchive
		//	auto& ar = (CJXArchive&)(*shAr);
		//	KAtEnd arEnd([&] {
		//		ar.Close();
		//		SetClipboardData(fmtClip, file.Detach());
		//		CloseClipboard();
		//		});
		//	ArcStdListRefObj(ar, listObj);
		//	return true;
		//}catch (...) {		}
		return false;
		//ar << listObj;
		//ar << mlist.GetCount();
		//POSITION posX = mlist.GetHeadPosition();
		//while (posX != NULL){
		//	CConditionObj* pObjTemp = (CConditionObj*)mlist.GetNext(posX);
		//	ar << (CConditionObj*)pObjTemp;
		//}
		// 
		//ar.Close();
		//::SetClipboardData(fmtClip, file.Detach());
		//CloseClipboard();
	}
	/// <summary>
	/// 객체를 클립보드에서 JSON으로 복사해 온다.
	/// </summary>
	/// <typeparam name="TListObj"></typeparam>
	/// <param name="pw"></param>
	/// <param name="listObj"></param>
	/// <param name="fmtClip"></param>
	/// <returns></returns>
	template<typename TListObj>
	bool UcPasteListByJson(CWnd * pw, TListObj & listObj, UINT fmtClip)//dwk: 2025-02-17 13:30 
	{
		COleDataObject dataObject;
		dataObject.AttachClipboard();
		//CRecipeObjList listCopy;
		if (dataObject.IsDataAvailable(fmtClip))
		{
			CFile* pFile = dataObject.GetFileData(fmtClip);
			if (pFile == NULL)
				return false;
			try {
				auto shAr = CreateJXArchive(pFile, CArchive::load); // CJXArchive
				auto& ar = (CJXArchive&)(*shAr);
				KAtEnd arEnd([&] {
					ar.Close();
					delete pFile;
					});
				ArcStdListRefObj(ar, listObj);
				return true;
			}
			catch (...) {
			}
		}
		return false;
	}
	};// end of namespace CJXArchive
/// 구조체를 위한 별도 아카이브
/// SET_FACTORY 방식으로 하면서 쓸일이 없게 되었지만, 유사시를 위해 남겨 둠
/// 특징: POINT, RECT등에 사용 하고
///		class 명을 뺀 멤버  
template<typename TStruct>
class CJXStructArchive : public CJXArchive {
public:
	using SaveFunc = std::function<void(CJXStructArchive&, TStruct&)>;
	using LoadFunc = std::function<void(CJXStructArchive&, TStruct&)>;
	//using LoadFunc = std::function<void(TStruct&, CJXStructArchive&)>;
protected:
	//std::wstring key_;
	//TStruct* data_ = nullptr;
	SaveFunc _saveFunc;
	LoadFunc _loadFunc;
	TStruct& _stctVal;
public:
	CJXStructArchive(CJXArchive& arUp, TStruct& stctVal, const std::wstring& key, SaveFunc cbSave, LoadFunc cbLoad)
		: CJXArchive(arUp, key.c_str())
		, _stctVal(stctVal)
	{
		CJXArchive::CSaveLoad saveLoad(*this);
		if (cbSave)
			_saveFunc = cbSave;
		if (cbLoad)
			_loadFunc = cbLoad;
		if (arUp.IsStoring()) {
			if (_saveFunc)
				_saveFunc(*this, _stctVal);//key = L"CXmlAppDoc._rc1"
			CStringW sTName = GetCleanTypeName<TStruct>();//name = L"COptionElem"
			//CStringW sTName(typeid(TStruct).name());//L"RECT" sTName = L"struct tagRECT"
			//if (sTName.Left(10) == L"struct tag")
			//	sTName = sTName.Mid(10);
			//else if (sTName.Left(7) == L"struct ")//CRect를 줘도 L"struct tagRECT" 가 온다.
			//	sTName = sTName.Mid(7);
			//else if (sTName.Left(6) == L"class ")//CRect를 줘도 L"struct tagRECT" 가 온다.
			//	sTName = sTName.Mid(6);
			SaveToUpperNode({}, { {TAG_STC, sTName.GetString()} });
		}
		else {
			if (_loadFunc)
				_loadFunc(*this, _stctVal);
		}
	}
	//void SetSaveFunction(SaveFunc func) { saveFunc_ = func; }
	//void SetLoadFunction(LoadFunc func) { loadFunc_ = func; }
	//// 구조체 저장
	//void Serialize() override {
	//	if (_saveFunc && data_)
	//		saveFunc_(*data_, *this);
	//}
	//// 구조체 로드
	//void Deserialize(TStruct& obj) {
	//	if (_loadFunc)
	//		_loadFunc(obj, *this);
	//	data_ = &obj; // 데이터 포인터 설정
	//}
#ifdef _Sample__
	CJXStructArchive<RECT> arClass(ar, VAR2STR(_rc2),
		[](auto& arm, auto& rc) {
			Arc_Member(rc.left);
			Arc_Member(rc.top);
			Arc_Member(rc.right);
			Arc_Member(rc.bottom);
		},
		[](auto& arm, auto& rc) {
			Arc_Member(rc.left);
			Arc_Member(rc.top);
			Arc_Member(rc.right);
			Arc_Member(rc.bottom);
		}
	);
	// 결과 예:
	<CXmlAppDoc._rc2 __struct__ = "RECT">
		< bottom>1240 < / bottom >
		< left>1100 < / left >
		< right>1200 < / right >
		< top>1120 < / top >
		< / CXmlAppDoc._rc2>
#endif // _Sample__
};// end of class CJXStructArchive
//#DocSerialize 2.1
#ifdef _DEBUG_noTrack
/// Serialize 호출 추적용 RAII 클래스
class CSerializeTracker {
	IDocSerialize* m_pObj;
	bool m_bDocMode;
public:
	CSerializeTracker(IDocSerialize* pObj, bool bDocMode, LPCSTR sFunc) 
		: m_pObj(pObj), m_bDocMode(bDocMode) {
		if (m_pObj) {
			m_pObj->IncSerializeCount(sFunc);
		}
	}
	~CSerializeTracker();
};

#define return_If_Doc_Call_DocSerialize(ar)\
	do{auto& ard = (CJXArchive&)ar;\
		bool bDocMode = ard.IsDocMode();\
		CSerializeTracker _tracker(static_cast<IDocSerialize*>(this), bDocMode, __FUNCTION__);\
		if (bDocMode) {\
			CallDocSerializeWithExceptionHandling<THISTYPE>(static_cast<THISTYPE*>(this), ard, ar, __FUNCTION__, __FILE__, __LINE__);\
			return;\
		}\
	} while(0)
#else
#define return_If_Doc_Call_DocSerialize(ar)\
	do{auto& ard = (CJXArchive&)ar;\
		bool bDocMode = ard.IsDocMode();\
		if (bDocMode) {\
			CallDocSerializeWithExceptionHandling<THISTYPE>(static_cast<THISTYPE*>(this), ard, ar, __FUNCTION__, __FILE__, __LINE__);\
			return;\
		}\
	} while(0)
#endif // _DEBUG
/// DocSerialize 호출 래퍼 함수 (예외 처리 포함)
template<typename TObj>
inline void CallDocSerializeWithExceptionHandling(TObj* pThis, CJXArchive& ard, CArchive& ar,
	const char* pszFunction, const char* pszFile, int nLine)
{
	CStringA saf(pszFunction);
	auto icr = saf.Find("::");
	if (icr >= 0)
		saf = saf.Left(icr);
	try {
		CJXArchive::CSaveLoad saveload(ard);
		++(*ard._statck);
#ifdef _DEBUG
		CStringA str; str.Format("%s(%d): dwk - arc_stack: [%d] %s\n", pszFile, nLine, *ard._statck, saf.GetString());
		OutputDebugStringA(str.GetString());
#endif // _DEBUG
		pThis->TObj::DocSerialize(ar);
		--(*ard._statck);
	}
	catch (CException* e) {
		CJXArchive::HandleCExceptionInDocSerialize(e, ard._statck, pszFile, nLine, saf);
	}
	catch (const std::exception& e) {
		CJXArchive::HandleStdExceptionInDocSerialize(e, ard._statck, pszFile, nLine, saf);
	}
	catch (...) {
		CJXArchive::HandleUnknownExceptionInDocSerialize(ard._statck, pszFile, nLine, saf);
	}
}

#ifdef _DEBUG_noTrack
#define ar_from_ard_ReadyForDocSerialize(ard) \
	static_cast<IDocSerialize*>(this)->IncDocSrlCount(__FUNCTION__);\
	auto& ar = (CJXArchive&)ard
#else
#define ar_from_ard_ReadyForDocSerialize(ard) \
	auto& ar = (CJXArchive&)ard
#endif // _DEBUG


struct UCTOOLDYNAMIC Matrix_Temp
	: public IDocSerialize //#DocSerialize 1
{
	//SET_FACTORY(MatrixTemp);
	vector<vector<double>> _data;
	void set(int r, int c, double dv) {
		// r, c에 따라 필요한 크기만큼 resize
		if (r >= static_cast<int>(_data.size()))
			_data.resize(r + 1);
		if (c >= static_cast<int>(_data[r].size()))
			_data[r].resize(c + 1);
		_data[r][c] = dv;
	}
	double at(int r, int c) {
		if (r < static_cast<int>(_data.size()))
			if (c < static_cast<int>(_data[r].size()))
				return _data[r][c];
		ASSERT(0);
		return 0.0;
	}
	template<typename TMatrix, typename Func>
	void FromMatrixT(const TMatrix& mat, Func cbSet) {
		int rows = mat.Nrows();
		int cols = mat.Ncols();
		_data.assign(rows, std::vector<double>(cols));
		for (int r = 0; r < rows; r++)// 루프는 0-based
			for (int c = 0; c < cols; c++)
				cbSet(mat, r, c);// mat(r + 1, c + 1);   // NEWMAT은 1-based
	}
	template<typename TMatrix, typename Func>
	void ToMatrixT(TMatrix& mat, Func cbSet) const {
		int rows = (int)_data.size();
		int cols = rows > 0 ? (int)_data[0].size() : 0;
		mat.resize(rows, cols);//NEWMAT.Matrix.resize 
		for (int r = 0; r < rows; r++)
			for (int c = 0; c < cols; c++)
				cbSet(mat, r, c);
	}
	template<typename TMatrix>
	void FromMatrix(const TMatrix& mat) {
		FromMatrixT(mat, [this](const TMatrix& mat1, int r, int c) {
			_data[r][c] = mat1(r + 1, c + 1);   // NEWMAT은 1-based
			});
	}
	template<typename TMatrix>
	void ToMatrix(TMatrix& mat) const {
		ToMatrixT(mat, [this](TMatrix& mat1, int r, int c) {
			mat1(r + 1, c + 1) = _data[r][c];
			});
	}
	template<typename TMatrix>
	void FromMatrixElement(const TMatrix& mat) {
		FromMatrixT(mat, [this](const TMatrix& mat1, int r, int c) {
			_data[r][c] = mat1.element(c, r);
			});
	}
	template<typename TMatrix>
	void ToMatrixElement(TMatrix& mat) const {
		ToMatrixT(mat, [this](TMatrix& mat1, int r, int c) {
			mat1.element(c, r) = _data[r][c];
			});
	}
	template<typename TMatrix>
	static void FromMatrix(const vector<TMatrix>& src, vector<Matrix_Temp>& tar)
	{
		tar.clear();
		tar.resize(src.size());
		for (size_t i = 0; i < src.size(); i++)
			tar[i].FromMatrix(src[i]);
	}
	template<typename TMatrix>
	static void ToMatrix(const vector<Matrix_Temp>& src, vector<TMatrix>& tar)
	{
		tar.clear();
		tar.resize(src.size());
		for (size_t i = 0; i < src.size(); i++)
			src[i].ToMatrix(tar[i]);
	}
	virtual void Serialize(CArchive& ar) {
		return_If_Doc_Call_DocSerialize(ar);//#DocSerialize 2.1
	}

	DECLARE_ClassName_DocSerialize();//#DocSerialize 2
};// end of struct MatrixTemp

/// Matrix::operator(int r,int c) 용
#define Arc_MatrixRC(mat) do{CLASSKEY(mat);\
	Matrix_Temp mat##_tmp;\
	if (ar.IsStoring()){\
		mat##_tmp.FromMatrix(mat);\
		Arc_Object(mat##_tmp);\
	} else {	Arc_Object(mat##_tmp);\
		mat##_tmp.ToMatrix(mat);}\
}while (0)
#define MatrixToTemp(tarTmp, srcTx, ni, nj)\
			for (int_t i = 0; i < ni; ++i) {\
				for (int_t j = 0; j < nj; ++j)\
					tarTmp.set(j, i, srcTx.element(j, i));}

#define Arc_MatrixElement(mat) do{CLASSKEY(mat);\
	Matrix_Temp mat##_tmp;\
	if (ar.IsStoring()){\
		mat##_tmp.FromMatrixElement(mat);\
		Arc_Object(mat##_tmp);\
	} else {	Arc_Object(mat##_tmp);\
		mat##_tmp.ToMatrixElement(mat);}\
}while (0)
#define Arc_StdVectMatrix2D(vmat) do{CLASSKEY(vmat);\
	vector<Matrix_Temp> vmat##_tmp;\
	if (ar.IsStoring()){\
		Matrix_Temp::FromMatrix(vmat, vmat##_tmp);\
		Arc_StdVectRefObj(vmat##_tmp);	\
	} else {	Arc_StdVectRefObj(vmat##_tmp);\
		Matrix_Temp::ToMatrix(vmat##_tmp, vmat);}\
}while (0)
//dwk: 2024-11-22 11:12
//주요 작업 내용:
//불필요한 CJXArchive& ar 파라미터 제거 및 auto& ar = *this; 추가
//매크로에서 중복 ar 파라미터 제거
//람다 함수 내부에서 ari 파라미터 올바르게 사용하도록 수정
//멤버 변수를 사용하지 않는 함수들을 static으로 변경
//Static 함수 호출 시 CJXArchive:: 접두사 명시
//불필요한 [this] 캡처 제거로 실수 방지
//Factory 람다에서 __class__ 정보를 활용한 다형성 생성 지원
//결과:
//코드 일관성 향상
//실수 가능성 감소
//포인터 컬렉션의 다형성 지원 완료
//빌드도 성공했고 코드 품질도 개선되었습니다. 
//DWKREMINDER("///========== 다시 빌드가 17.044 초가 걸림 ====홈   PC======")
//DWKREMINDER("///========== 다시 빌드가 29.737 초가 걸림 ====회사 PC======")
//DWKREMINDER("///========== 다시 빌드가 73.66  초가 걸림 ====VDI  PC======")

/// Arc_MatrixElement 로 대채 할수 있다.
#define Arc_ColumnVector(cv) \
	do { int nr{ 0 };\
		vector<Real> cv##_tmp; \
		if (ar.IsStoring()) { \
			nr = Probility.Nrows(); \
			for (int i = 0; i < nr; i++) \
			cv##_tmp.push_back(cv.element(i)); \
			Arc_StdVectVal(cv##_tmp); \
		} else { \
			Arc_StdVectVal(cv##_tmp); \
			nr = cv##_tmp.size();\
			for (int i = 0; i < nr; i++) \
				cv.element(i) = cv##_tmp[i]; \
		} \
	} while(0)

/// boost::uint_t 벡터 저장 (int로 변환하여 처리)
#define Arc_StdVectBoostUinT(cv)\
    do { \
	vector<UINT> cv##_tmp;\
	if (ar.IsStoring()) {\
		for (auto& u : cv)\
			cv##_tmp.push_back((int)u);\
		Arc_StdVectVal(cv##_tmp);\
        } else { \
		Arc_StdVectVal(cv##_tmp);\
		for (auto& u : cv##_tmp)\
			cv.push_back((uint_t)u);\
        } \
    } while(0)

/// std::vector<enum> 저장 (enum을 int로 변환하여 저장/로드)
#define Arc_StdVectEnum(v)\
    do { \
	vector<int> v##_tmp;\
	if (ar.IsStoring()) {\
		for (auto& e : v)\
			v##_tmp.push_back(static_cast<int>(e));\
		Arc_StdVectVal(v##_tmp);\
        } else { \
		Arc_StdVectVal(v##_tmp);\
		v.clear();\
		for (auto& i : v##_tmp)\
			v.push_back(static_cast<decltype(v)::value_type>(i));\
        } \
    } while(0)

/// BYTE/char/wchar_t 배열을 null-terminated 문자열들로 해석하여 vector<std::string>으로 저장/로드
/// 템플릿 함수 BufferForCharString을 호출하는 매크로
#define ArcBufferForCharString(ar, buf, countVar) ar.BufferForCharString(buf, countVar, L#buf)
#define Arc_BufferForCharString(buf, countVar) ArcBufferForCharString(ar, buf, countVar)

/// C 스타일 배열을 vector로 변환하여 처리
#define Arc_C_ArrayToVector(src, TYPE, n)\
	do{vector<TYPE> src##_tmp; \
		if(ar.IsStoring()){\
			for (int i = 0; i < n; i++)\
				src##_tmp.push_back(src[i]);\
			Arc_StdVectVal(src##_tmp);\
		} else {\
			 Arc_StdVectVal(src##_tmp);\
			for (int i = 0; i < n; i++)\
				src[i] = src##_tmp[i];\
		}}while(0)



#define IMPLEMENT_SERIAL_FACTORY(TOBJ, TBASE) IMPLEMENT_SERIAL(TOBJ, TBASE, 1)\
	SET_FACTORY(TOBJ);


std::string GetFormatByExtension(LPCTSTR fullPath, LPCTSTR baseExt = _T("ecm"));

//dwk: 2025-12-01 12:20 
//dwk: 2025-12-01 16:35 
//dwk: 2025-12-02 10:52 StdVectShpObj
//dwk: 2025-12-02 11:09 Arc_MfcArray CArray<VAR, const VAR&> 때문에 둘다 template 으로 변경
//dwk: 2025-12-08 15:04 serr; _DWKTRACE 포함, 미포함
//dwk: 2025-12-09 17:18 map val, 2D vector
//dwk: 2025-12-10 13:10 UcJXBase.h 제거
//dwk: 2025-12-10 16:33 CArchive가 Serialize로 바로 올때 까지 처리
//dwk: 2025-12-11 16:54 map<k,vector<v>>
//dwk: 2025-12-12 10:35 ArcClassKey
//dwk: 2025-12-15 12:11 DECLARE_ClassName_DocSerialize
//dwk: 2025-12-16 11:15 _class_ export실패하고, GetMyClassName으로 대체
//dwk: 2025-12-17 12:19 VDI: Tmp 처리 방식 포기, svm.h svm_parameter svm_model에 DocSerializ 추가
//dwk: 2025-12-17 17:29 UcDefaultSaveClassName GetMyClassName()로 통일
//dwk: 2025-12-18 10:32 Arc_StdVectEnum
//dwk: 2025-12-18 12:44 vect<std::string>
//dwk: 2025-12-18 16:08 ArcBufferForCharString
//dwk: 2025-12-19 13:21  StdMapCStrIntCnv StdMapCStrCStrCnv TCompare  추가
//dwk: 2025-12-22 16:50 CJXArchive::Read, Write, Flush, Close
//dwk: 2025-12-23 11:19 IMPLEMENT_DUMY_SERIALIZE 에 return_If_Doc_Call_DocSerialize 적용
//dwk: 2025-12-24 16:23 Arc_PtrObjNoDel 추가
//dwk: 2025-12-24 17:36 return_If_Doc_Call_DocSerialize 에 try catch 적용.
//dwk: 2025-12-29 11:18 catch 핸들링 함수 cpp로 이동
