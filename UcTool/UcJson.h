#pragma once
/// UcJson.h
/// 2025-11-08 04:10 에러 없음.

#include <memory>  // 스마트 포인터 포함
#include <array>   // std::array 포함
#include <initializer_list>
#include <type_traits>  // std::enable_if, std::is_same 등 포함
#include <functional>  // std::function 포함
#include <xutility>

#include <afxtempl.h>
#include <ATLComTime.h>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
#pragma warning(disable : 4503)

#include "UcTool.h"//throw_str
#include "UcBinary.h"//KBinary
//#include "UcJXBase.h"//CExArchive

//C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\atlmfc\include\afxtempl.h

class UCTOOLDYNAMIC ONULL {// usedd in JUnit
public:
	char* v{ nullptr };
};

class UCTOOLDYNAMIC CDicBase {
public:
	virtual ~CDicBase() {}
};


///	JSON 직렬화 인터페이스
//template <typename DerivedT>
class UCTOOLDYNAMIC IJXSerializable//_virtual_wrong
{
public:
	/// <summary>
	/// 주의: Serialize 에서 반드시 자기 것만 불러야 한다.
	/// 방법: Base::DocSerialize(ar); 처럼 불러야 한다.
	/// 이유: Serialize가 base부른 후 derived 가 불려 지므로 DocSerialize를 그냥 부르면 derived의 것이 불려지고
	///       다시 derived::Serialize 에서 DocSerialize 가 중복해서 불려지기 때문이다.
	/// </summary>
	/// <param name="ar"></param>
	virtual void DocSerialize(CArchive& ar) {}
};
//template <typename DerivedT>
//class IJXSerializable_fail {
//public:
//	void SerializeDoc(CArchive& ar) {
//		static_cast<DerivedT*>(this)->DocSerialize(ar); 이거 제대로 안됨
//	}
//};


//class ONULL	{ --> UcBaseTools.h 옮겨짐


using JsonType = VType;

// JSON 배열 태그 매크로 (모두 같은 값으로 설정하여 나중에 되돌릴 수 있도록)
#define TAG_ITM L"_r_"
#define TAG_ARR L"_array_"
#define TAG_CLS L"_class_"
#define TAG_TPY L"_type_"
#define TAG_STC L"_struct_"
//#define JITM_TAG L"__r_"

class JVal;
class UcJObj;
class UcJArr;
//class JBase;


/// JSON 예외 정보를 담는 구조체
class UCTOOLDYNAMIC JException : public std::exception
{
public:
	std::string sFunction;    /// 함수명
	int line{ 0 };               /// 소스 코드 라인
	std::string message;     /// 에러 메시지
	std::wstring jsonPos;    /// JSON 위치
	int jsonLine{ 0 };           /// JSON 파일 라인
	int jsonColumn{ 0 };         /// JSON 파일 컬럼

	explicit JException() {}
	explicit JException(const std::string& func, int ln, const std::string& msg,
		const std::wstring& pos, int jLine, int jCol)
		: sFunction(func), line(ln), message(msg), jsonPos(pos), jsonLine(jLine), jsonColumn(jCol) {
	}
};

// 주석 풀고 wchar 키도 테스트 해야

//#define _UseWStringKey_ // 키를 string으로 한거야?
//#ifdef _UseWStringKey_ // 키를 string으로 한거야?
using JKEYSTR = LPCWSTR;
using JString = CStringW;
using jstring = std::wstring;

///PTstr --> UcBaseTools.h(2271) 옮겨짐




class UCTOOLDYNAMIC JBase {
public:
	virtual ~JBase() {}

	virtual UcJObj* Dic();
	virtual UcJArr* Arr();
	virtual JVal* Val();

	virtual BOOL IsDic();
	virtual BOOL IsArr();
	virtual BOOL IsVal();
};


using ShJBase = shared_ptr<JBase>; /// 앞으로 이거만 써야 한다.
using ShJVal = shared_ptr<JBase>;//[[deprecated("ShJVal is deprecated, use ShJBase instead.")]]
using ShJObj = shared_ptr<JBase>;//[[deprecated("ShJObj is deprecated, use ShJBase instead.")]]
using ShJArr = shared_ptr<JBase>;//[[deprecated("ShJArr is deprecated, use ShJBase instead.")]]


class UCTOOLDYNAMIC JUnit
{
public:

	UcJObj* m_pCJobj;
	jstring m_k;

	JUnit(UcJObj* th, JKEYSTR k)
		: m_pCJobj(th), m_k(k)
	{
	}
	JUnit(UcJObj* th, const wstring& k)
		: m_pCJobj(th), m_k(k)
	{
		//m_k = CString(k.c_str());
	}
	void operator=(const JUnit& v);
	//void operator=(KVal& v);
	void operator=(const wchar_t* v);
#ifdef _DEBUGx
	void JUnit::operator=(const wchar_t* v)
	{
		ShJVal sjv = v ? make_shared<JVal>(v) : sjv = make_shared<JVal>();
		m_pCJobj->Set(m_k, sjv);
	}
#endif // _DEBUGx

	void operator=(const CStringW& v);
	void operator=(const CStringA& v);
	void operator=(const wstring& v);
#if CPP17_OR_LATER
	void operator=(const std::wstring_view v);
#endif
	void operator=(const string& v);
	//void operator=(const string v);
	void operator=(const char* v);



	BOOL operator==(const wchar_t* v);
	BOOL operator==(int v);
	BOOL operator==(__int64 v);
	BOOL operator==(double v);

	BOOL operator<(const wchar_t* v);
	BOOL operator<(int v);
	BOOL operator<(__int64 v);
	BOOL operator<(double v);

	BOOL operator>(const wchar_t* v);
	BOOL operator>(int v);
	BOOL operator>(__int64 v);
	BOOL operator>(double v);


	BOOL operator==(const char* v) { return operator==(CStringW(v).GetString()); }
	BOOL operator<(const char* v) { return operator<(CStringW(v).GetString()); }
	BOOL operator>(const char* v) { return operator>(CStringW(v).GetString()); }
	BOOL operator>=(const wchar_t* v) { return !operator<(v); }
	BOOL operator>=(const char* v) { return !operator<(v); }
	BOOL operator>=(int v) { return !operator<(v); }
	BOOL operator>=(__int64 v) { return !operator<(v); }
	BOOL operator>=(double v) { return !operator<(v); }

	BOOL operator<=(const wchar_t* v) { return !operator>(v); }
	BOOL operator<=(const char* v) { return !operator>(v); }
	BOOL operator<=(int v) { return !operator>(v); }
	BOOL operator<=(__int64 v) { return !operator>(v); }
	BOOL operator<=(double v) { return !operator>(v); }

	BOOL operator!=(const wchar_t* v) { return !operator==(v); }
	BOOL operator!=(const char* v) { return !operator==(v); }
	BOOL operator!=(double v) { return !operator==(v); }
	BOOL operator!=(__int64 v) { return !operator==(v); }
	BOOL operator!=(int v) { return !operator==(v); }


	void operator+=(const wchar_t* v);//여기서 하면 아직 선언안되 포인터를 쓰게 되므로 옮겨야 { ShJVal sjv = m_pCJobj->Get(m_k); sjv->operator+=(v); }
	void operator+=(const CStringW& v)
	{
		operator+=((LPCWSTR)v);
	}
	void operator+=(const CStringA& v)
	{
		CStringW sw(v);	operator+=(sw);
	}
	void operator+=(const char* v)
	{
		CStringW sw(v);	operator+=(sw);
	}

	void operator+=(double v);
	void operator+=(__int64 v);
	void operator+=(int v) { operator+=((__int64)v); }
	void operator+=(unsigned int v) { operator+=((__int64)v); }

	void operator+=(CTimeSpan v);
	void operator+=(COleDateTimeSpan v);

	// Numeric and other types
	void operator=(__int64 v);
	void operator=(unsigned __int64 v); // 설마 내부변수가 int64인데 최대수를 넘지는 않겠지?
	void operator=(int v);
	void operator=(WORD v);
	void operator=(BYTE v);
	//	void operator=(long v); 만들면 오류 경고 나오면 cast하면 됨
	void operator=(unsigned int v);
	void operator=(unsigned long v) { operator=((unsigned int)v); }
	void operator=(CTime v);
	void operator=(COleDateTime v);
	void operator=(double v);//error C2593: 'operator ='이(가) 모호합니다. 0 일때만 그르네
	//void operator=(ShJObj v);
	void operator=(ShJVal v);
	void operator=(JVal& v);
	void operator=(UcJObj& v);//clone해야 한다.
	void operator=(const UcJObj& v) {//clone해야 한다.
		operator=((UcJObj&)v);
	}
	void operator=(UcJArr& v);
	void operator=(ONULL& v);
	void operator=(CStringArray& v);//dwk: 2025-02-12 15:55 JSON 추가 필드 1
	void operator=(CStringList& v);//dwk: 2025-12-02 11:20 JSON CStringList 지원
	//void operator=(SHP<CStringArray>& v);//dwk: 2025-02-12 15:55 JSON 추가 필드 1
	template<typename TArrayElem, typename TArg>
	void operator=(CArray<TArrayElem, TArg>& v);

	// STL containers
	void operator=(std::vector<std::wstring>& v);
	void operator=(std::vector<std::string>& v);
	void operator=(std::vector<int>& v);
	void operator=(std::vector<unsigned int>& v);
	void operator=(std::vector<INT64>& v);
	void operator=(std::vector<double>& v);
	void operator=(std::vector<size_t>& v);
	void operator=(std::vector<CStringW>& v);
	void operator=(std::vector<CStringA>& v);
	void operator=(std::vector<_variant_t>& v);
	
	void operator=(std::list<std::wstring>& v);
	void operator=(std::list<TCString<TCHAR>>& v);
#ifdef _MBCS
	void operator=(std::list<CStringW>& v);
#else
	void operator=(std::list<CStringA>& v);
#endif
	void operator=(std::list<int>& v);
	void operator=(std::list<INT64>& v);
	void operator=(std::list<double>& v);
	void operator=(std::list<size_t>& v);

	void operator=(CArray<int, int>& v);
	void operator=(CArray<INT64, INT64>& v);
	void operator=(CArray<double, double>& v);
	void operator=(CArray<TCString<TCHAR>, TCString<TCHAR>>& v);
#ifdef _MBCS
	void operator=(CArray<CStringW, CStringW>& v);
#else
	void operator=(CArray<CStringA, CStringA>& v);
#endif
	// VS2015에서 CArray<CString, const TYPE&> 형태를 위해 추가
	void operator=(CArray<CString, const CString&>& v);

	void operator=(std::map<std::wstring, std::wstring>& v);
	void operator=(std::map<std::wstring, CStringW>& v);
	void operator=(std::map<std::wstring, CStringA>& v);
	void operator=(std::map<std::wstring, int>& v);
	void operator=(std::map<std::wstring, INT64>& v);
	void operator=(std::map<std::wstring, double>& v);
	void operator=(std::map<std::wstring, std::vector<std::wstring>>& v);

	/// tuple과 pair 지원을 위한 operator=
	// Array types
	void operator=(const std::array<int, 2>& v);
	void operator=(const std::array<int, 4>& v);
	void operator=(const std::array<double, 2>& v);
	void operator=(const std::array<double, 4>& v);
	void operator=(const std::array<float, 2>& v);
	void operator=(const std::array<float, 4>& v);

	// String utility methods
	CStringW Left(int len);
	CStringW Right(int len);
	CStringW Mid(int pos, int len = -1);
	int Find(LPCWSTR s, int istart = 0);
	int ReverseFind(wchar_t ch);
	void Trim();
	bool IsEmpty();

	/// 시간 type인 경우 COleDateTime.Format에 따라 문자열로 시간을 만들어서 리턴 한다.
	// ex: .Format(L"%H:%M:%s) 는 "14:03:23" 형식으로 리턴한다.
	CString Format(LPCTSTR fmt);

	/// status도 valid 이어야 하고, 날짜도 1900-01-01 , 1899-12-30, 1970-01-01 도 아니어야 한다.
	BOOL IsValidDateTime();

	/// 중첩 객체 접근을 위한 operator()
	JUnit operator()(const wchar_t* key);

	JUnit operator()(const jstring& key) {
		return operator()(key.c_str());
	}

	JUnit operator()(int key);
};




/// <summary>
/// [JSON table 규칙]
/// - 루트 객체의 키(예: "table_users") 아래 값은 객체이며 다음 세 멤버를 둔다.
///   - "type": 문자열 "table" (필수)
///   - "fields": 배열 — 열 정의(메타). 열 개수 = ColSize().
///   - "rows": 배열 — 각 원소는 한 행을 나타내는 JSON 배열(셀 값들). 행 개수 = RowSize().
/// [API]
/// - ShJVal sh = j.Cell("table_users", col, row);  // col=열 인덱스, row=행 인덱스 (0부터)
/// - UcJObj::Cell(j, "table_users", col, row);     // 동일, 정적 래퍼
/// - j.ColSize("table_users"), j.RowSize("table_users") — size_t
/// - j.CellS / CellI / CellI64 / CellD / CellF — 셀 값을 JVal::S / I / I64 / N 등 기존 변환과 동일하게 반환
/// - 스키마가 아니거나 col·row 범위를 벗어나면 throw_str.
/// </summary>
/// auto sample_table_json =
///R"(
///{
///  "table_users": {
///    "type": "table",
///    "fields": [
///      { "name": "id", "type": "int" },
///      { "name": "name", "type": "string" },
///      { "name": "age", "type": "int" }
///    ],
///    "rows": [
///      [1, "kim", 30],
///      [2, "lee", 25]
///    ]
///  }
///}
///)";
/// UcJTable은 UcJObj::Table(tableKey);로 구한다.
class UcJObj;
class UCTOOLDYNAMIC UcJTable {
public:
	UcJObj* _tbl{ nullptr };
	ShJArr _fields;// = tbl->A(L"fields", false);
	ShJArr _rows;// = tbl->A(L"rows", false);
	std::map<std::wstring, int> _mFieldCol;

	UcJObj* JObj() { return _tbl; }
public:
	size_t ColSize();
	size_t RowSize();
private:
	template <typename T> 
	struct _always_false : std::false_type {};
	
	template <typename TCol>
	size_t ColIndex(const TCol& col)
	{
		using DCol = std::decay_t<TCol>;//TCol이 int&, const int&면 → int처럼 본질적인 타입으로 맞춤
		/// decay_t 의 역할
		//참조 제거 — TCol이 int&, const int&면 → int처럼 본질적인 타입으로 맞춤
		//const/volatile 정리 — 값 타입 기준으로 쓸 때 흔한 형태로 맞춤
		//배열·함수 — 배열은 포인터로, 함수는 함수 포인터로 감쇠(decay) (이름 그대로 C에서 배열 인자로 넘길 때 일어나는 것과 같은 규칙)
		if constexpr (std::is_integral_v<DCol>) {
			return (size_t)col;
		}
		else if constexpr (std::is_same_v<DCol, std::wstring>) {
			return (size_t)Col(col.c_str());
		}
		else if constexpr (std::is_same_v<DCol, CStringW>) {
			return (size_t)Col(col.GetString());
		}
		else if constexpr (std::is_convertible_v<DCol, LPCWSTR>) {
			// Col()은 내부에서 throw_str로 실패를 처리하므로, 음수/0 캐스팅 걱정이 없다.
			return Col((LPCWSTR)col);
		}
		else if constexpr (std::is_convertible_v<DCol, LPCSTR>) {
			// Col()은 내부에서 throw_str로 실패를 처리하므로, 음수/0 캐스팅 걱정이 없다.
			return Col((LPCSTR)col);
		}
		else {
			static_assert(_always_false<DCol>::value, "UcJTable::Cell* unsupported col type");
			return 0;
		}
	}
public:
	int Col(LPCWSTR colName);
	int Col(LPCSTR colName);

	int Col(const std::wstring& colName) { return Col(colName.c_str()); }

	JVal* Cell(size_t col, size_t row);
	ShJBase GetRow(size_t row);
	ShJBase GetCell(size_t col, size_t row);
	// Cell은 Row 중 하나 이기 때문에 GetCol 은 없다.

	/// TCol 번호(0,1,2...) 일수도 있고, 열 이름일 수도 있다. 열 이름이면 내부적으로 번호로 바꿔서 Cell()을 호출한다.
	template <typename TCol>
	JVal* Cell(const TCol& col, size_t row) { return Cell(ColIndex(col), row); }
	
	template <typename TCol>
	CStringW CellS(const TCol& col, size_t row, LPCWSTR def = L"") { return Cell(col, row)->S(def); }
	template <typename TCol>
	CStringA CellSA(const TCol& col, size_t row, LPCSTR def = "") { 
		return CStringA(Cell(col, row)->S(def));
	}

	template <typename TCol>
	int      CellI(const TCol& col, size_t row, int def = 0) { return Cell(col, row)->I(def); }

	template <typename TCol>
	__int64  CellI64(const TCol& col, size_t row, __int64 def = 0) { return Cell(col, row)->I64(def); }

	template <typename TCol>
	double   CellD(const TCol& col, size_t row, double def = 0.) { return Cell(col, row)->N(def); }

	template <typename TCol>
	float    CellF(const TCol& col, size_t row, float def = 0.f) { return (float)Cell(col, row)->N((double)def); }

	// INSERT_YOUR_CODE
	/// 지정한 row(행)의 모든 필드를 읽어, UcJObj로 모아 반환한다.
	/// @param row: 추출할 row index
	/// @return: 각 필드가 key-value로 들어간 ShJObj(shared_ptr<UcJObj>)
	ShJBase RowObj(size_t row);
};

/// 함수도 foward declaration로 선언해 놓고,그래야 UcJObj에서 쓸 수 있다.
template<typename T> std::wstring PTstr(T&& k);


//#ifdef _UnsortedHash_
//	public KStdHashMap<jstring, ShJVal>// 릴리즈에서는 가장 빠른 unsorted map을 쓴다.
//#else
//#endif // _UseOrderedWatch__
//public CDicBase
/// 키 값을 갖는 JSON 객체
 // 디버깅중에는 내부를 Watch로 볼때 정렬되어 있는것으로 볼수 있게 이걸 쓰고
class UCTOOLDYNAMIC UcJObj : public KStdMap<jstring, ShJVal>, public JBase
{
public:
	static std::function<BOOL(jstring& k)> _fncFieldCheck;
	static BOOL s_bSkipFieldCheck;
#ifdef _DEBUG
	static bool LoadSqlBackupFieldNames(LPCWSTR pathToSqlFile);
	static BOOL FieldCheckAgainstLoadedSqlBackupFields(jstring& k);
#endif

	struct InitItem {
		jstring key;
		ShJVal val;

		template<typename TKEY>
		InitItem(TKEY k, ShJVal v)
			: key(PTstr(k)), val(v)
		{
		}

		template<typename TKEY, typename TVAL>
		InitItem(TKEY k, const TVAL& v);
	};

	UcJObj() {}
	~UcJObj() override {};
	UcJObj(std::initializer_list<InitItem> initList);

	UcJObj(UcJObj& jobj) noexcept
	{
		//if (&jobj == nullptr)
		//	throw_str(L"UcJObj::UcJObj(jobj jobj,) jobj is empty.");
		Clone(&jobj, true);
	}
	UcJObj(UcJObj&& jobj) noexcept
	{
		Clone(&jobj, true);
	}
	UcJObj(UcJObj& jobj, bool bClone)// = true)
	{
		//if (&jobj == nullptr)
		//	throw_str(L"UcJObj::UcJObj(jobj jobj,) jobj is empty.");
		Clone(&jobj, bClone);
	}


	//ex: SHP<UcJObj> jbo1 = make_shared<UcJObj>(jb1, false); 
	/// bClone=false : 이면,  bClone이 계속 전달 되다가 JVal::JVal -> JVal::Clone 에서 객체인 경우만 shared_ptr 이 복사된다.
	/// 1. JObj를 그대로 공유 하려면 shared_ptr 할당하면 된다. ex: auto shJo2 = shJo1;
	/// 2. JObj의 깊이 1 레벨의 primitive 값은 복사 되고 Dic이거나 Arr인 경우는 shared_ptr인 UcJObj, JArr는 공유된다. 
	//		ex: auto shJo2 = make_shared<UcJObj>(shJo1, false); 특별한 경우 내부 Arr이 테이블의 row인 경우
	/// 3. bClone=true : JObj의 deep copy로 맨끝까지 완전히 clone한다.
	UcJObj(ShJVal sjo, bool bClone)// = true)
	{
		ASSERT(sjo);
		if (sjo) {
			//throw_str("UcJObj::UcJObj(ShJVal sjo,) sjo is empty.");
			ASSERT(IsDic());
			Clone(sjo->Dic(), bClone);
		}
	}


	void operator=(const UcJObj& jbj) {
		Clone(&jbj, true);
	}
	void operator=(ShJVal sjo)
	{
		ASSERT(IsDic());
		Clone(sjo->Dic(), true);
	}
	void operator=(std::initializer_list<InitItem> initList);

	BOOL IsSame(UcJObj& jbj2);
	BOOL operator==(UcJObj& jbj2)
	{
		return IsSame(jbj2);
	}
	BOOL operator!=(UcJObj& jbj2)
	{
		return !IsSame(jbj2);	//!(this->operator==(jbj2));
	}

	void ErrTest();

	/// just for DEBUG. see Txt for release
	void toString();

	//export된 static변수도 

	template <typename TKEY>
	void Set(TKEY k, ShJVal val)
	{
		BOOL bOK = TRUE;
		jstring kw = PTstr(k);//(k);
#ifdef _DEBUG //필드체크
		//if (this->Has(k))
		//	_break;//디버그 용도//dwk: 2025-11-14 10:55 ~CSaveLoad 에서 상위 노드에 넣고, 또 넣는지 보려고
		ASSERT(val->IsVal());///JVal로 싸서 줘야지. SHP<UcJObj> 를 바로 오면 안됨.
		//if (!s_bSkipFieldCheck && _fncFieldCheck)
		//	bOK = _fncFieldCheck(kw);
#endif // _DEBUG
		//if (bOK)
			SetAt(kw, val);
	}
	/// Format은 Set과 같은 역할을 하지만, val이 공유되는 것이 아니라 복사된다.
	template <typename TKEY>
	void Format(TKEY k, LPCWSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		int size = _vscwprintf(fmt, args) + 1; // +1 for null-terminator
		CStringW buffer;
		if (buffer.GetBuffer(size))
		{
			_vsnwprintf_s(buffer.GetBuffer(size), size, size - 1, fmt, args);
			buffer.ReleaseBuffer();
		}
		va_end(args);
		auto val = make_shared<JVal>(buffer);
		Set(k, val);
	}

	/// CopyAt은 SetAt과 같은 역할을 하지만, val이 공유되는 것이 아니라 복사된다.
	template <typename TKEY>
	void CopyAt(TKEY k, ShJVal val)
	{
		ShJVal nv = make_shared<JVal>(val);
		SetAt(PTstr(k), nv);
	}

	template <typename TKEY>
	void SetNode(TKEY k, ShJVal sjo, bool bClone = false)
	{
		ASSERT(IsDic());
		ASSERT(sjo->IsDic() || sjo->IsArr());
		ShJVal sjv = make_shared<JVal>(sjo, bClone);
		Set(PTstr(k), sjv);
	}
	/// SetObj은 Set과 같은 역할을 하지만, sxo가 false 공유되는 것이 아니라 true 복사된다.
	template <typename TKEY>
	void SetObj(TKEY k, ShJVal sjo, bool bClone = false)
	{
		ASSERT(IsDic());
		ASSERT(sjo->IsDic());
		SetNode(k, sjo, bClone);
		//ShJVal sjv = make_shared<JVal>(sjo, bClone);
		//Set(PTstr(k), sjv);
	}

	/// SetArray은 Set과 같은 역할을 하지만, sja가 false 공유되는 것이 아니라 true 복사된다.
	template <typename TKEY>
	void SetArray(TKEY k, ShJArr sja, bool bClone = false)
	{
#ifdef _DEBUG
		auto pVal1 = dynamic_cast<JVal*>(this);
		auto pVal2 = dynamic_cast<UcJObj*>(this);//  not NULL
		auto pVal3 = dynamic_cast<UcJArr*>(this);
		ASSERT(pVal1 == nullptr);
		ASSERT(pVal2);
		ASSERT(pVal3 == nullptr);
		ASSERT(sja->IsArr());
		ASSERT(!sja->IsDic());
		ASSERT(!sja->IsVal());
#endif // _DEBUG
		SetNode(k, sja, bClone);
		//ShJVal sjv = make_shared<JVal>(sja, bClone);
		//Set(PTstr(k), sjv);
	}

	/// XML 인경우만 사용
	template <typename TKEY, typename TVAL>
	void SetAttr(TKEY k, TVAL sAttr)
	{
		ASSERT(IsDic());
		ShJVal sjv = make_shared<JVal>(sAttr);
		sjv->Val()->_bAttr = true;
		Set(PTstr(k), sjv);
	}


	template <typename TKEY>
	UcJArr& SetArray(TKEY k);
	//{
	//	ASSERT(IsArr());
	//	ShJVal sjv = make_shared<JVal>(make_shared<UcJArr>());
	//	Set(PTstr(k), sjv);
	//	return *sjv->Arr();
	//}
	template <typename TKEY>
	bool DeleteKey(TKEY k)
	{
		auto kw = PTstr(k);
		//if (Has(kw)) // erase가 없으면 아무짓도 안한다.
		//{
		return this->erase(kw) == 1;
		//return true;
	//}
	//return false;
	}
	template <typename TKEY>
	bool DeleteKey(std::initializer_list<TKEY> kar)
	{
		bool brv = true;
		for (const auto& key : kar)
		{
			auto brv1 = this->DeleteKey(key);
			brv = brv && brv1;
		}
		return brv;
	}

	/// bClone:어차피 value가 shared_ptr이므로 알아서 share하는데
	/// 안에 값을 따로 갈 경우만 true로 하면 된다.
	bool Clone(const UcJObj* src, bool bClone);
	bool Clone(UcJObj* src, bool bClone);
	bool Clone(ShJVal& src, bool bClone)
	{
		if (!src)
		{
			//no_throw_str("UcJObj::Clone(src,) src is empty.");
			return false;
		}
		ASSERT(IsDic());
		return Clone(src->Dic(), bClone);
	}

	static bool CloneObject(UcJObj* source, UcJObj& tar, bool bClone = true);

	static bool CloneObject(ShJVal src, ShJVal tar, bool bClone = true);
	/// Merge two ShJVal objects, preferring values from the first argument (parent), and filling missing keys from the second (child).
	/// If both are null, returns a new empty ShJVal.
	/// @param parent The parent object (priority source)
	/// @param child The child object (fallback source)
	/// @return Merged ShJVal object
	static ShJVal MergeJsonObj(ShJVal parent, ShJVal child);

	template<typename FNC>
	static bool CloneObjCond(const UcJObj& source, UcJObj& tar, FNC lmbdaCondition = [](auto k, auto sjv) {return true; }, bool bClone = true)
	{
#if CPP17_OR_LATER
		for (const auto& [k, sjv] : source) {
#else
		for (const auto& pair : source) {
			const auto& k = pair.first;
			const auto& sjv = pair.second;
#endif
			if (lmbdaCondition(k, sjv))
				tar.SetAt(k, make_shared<JVal>(*sjv, bClone));
		}
		return true;
		}

	BOOL Lookup(jstring k, ShJVal & v) override
	{
		BOOL bOK = TRUE;
#ifdef _DEBUG //필드체크 Lookup
		if (!UcJObj::s_bSkipFieldCheck && _fncFieldCheck)
		{
			//JString kw(k);
			bOK = _fncFieldCheck(k);
		}
#endif // _DEBUG
		return __super::Lookup(k, v);
	}


	template <typename TKEY>
	ShJVal Get(TKEY k)
	{
		ShJVal sjv;
		Lookup(PTstr(k), sjv);//can be FALSE
		return sjv;
	}


	/// double 실수 인경우 소수점 아래 수 참조 하여 문자열 리턴 123.12343421234 => "123.12"
	template <typename TKEY>
	CStringW FStr(TKEY k, int point = 2, LPCWSTR def = L"0");

	/// 항목이 있고 IsString 이면 리턴. 아니면 널

	template <typename TKEY>
	CStringW S(TKEY k, LPCWSTR def = L"");

	template <typename TKEY>
	CStringA SA(TKEY k, LPCSTR def = "");

	template <typename TKEY>
	CString ST(TKEY k, LPCTSTR def = _T(""));

	template <typename TKEY>
	LPCWSTR SP(TKEY k, LPCWSTR def = L"");

	template <typename TKEY>
	LPCWSTR Ptr(TKEY k);

	template <typename TKEY>
	LPCWSTR Txt(TKEY k, LPCWSTR def = L"");

	CStringW QS(JKEYSTR k, BOOL bNullIfEmpty = TRUE, BOOL bQuat = TRUE, BOOL bNecessary = FALSE);

	/// 아래 5개도 template <typename TKEY> 으로 전환 해야 할 듯

	/// 길이가 1이상 이면 리턴. 아니면 널
	size_t Length(JKEYSTR k);
	template <typename TKEY>
	BOOL IsEmpty(TKEY k) { return !Len(k); }
	LPCWSTR LenS(JKEYSTR k, CStringW & sv);
	//BOOL SameSA(JKEYSTR k, LPCWSTR strk) { return SameS(k, strk); }
	BOOL OrStr(JKEYSTR k, LPCWSTR str, char tok = '|');

	template <typename TKEY>
	BOOL Len(TKEY k)
	{
		return Length(JString(k)) > 0;
	}
	template <typename TKEY>
	BOOL SameS(TKEY k, LPCWSTR str)
	{
		if (this == nullptr)
			return str == nullptr;
		LPCWSTR sn = SP(k);
		return tchsame(sn, str);
	}
	template <typename TKEY>
	BOOL SameOr(TKEY k, vector<LPCWSTR> && vstr)
	{//{"xxx","xxx"}처럼 초기화로 부르므로 이럴때는 &하나만 있는 참조로는 안된다. RValue 참조를 이럴때 쓴다.
		if (this == nullptr)
			return FALSE;
		LPCWSTR sn = SP(k);//SN은 string type이어야 한다.
		for (const auto& str : vstr)
		{
			if (tchsame(sn, str))
				return TRUE;
		}
		//if(std::any_of(vstr.begin(), vstr.end(), [&sn](const std::string& str) {
		//	return tchsame(sn, str);			}) return TRUE;
		return FALSE;
	}
	void StrCat(JKEYSTR k, LPCWSTR str)
	{
		if (this == nullptr)
			return;
		auto s = S(k);
		(*this)(k) = s + str;
	}
	void StrCatLeft(JKEYSTR k, LPCWSTR str)
	{
		if (this == nullptr)
			return;
		auto s = S(k);
		(*this)(k) = str + s;
	}

	template <typename TKEY>
	CTime T(TKEY k);

	/// flag: 0 full, 1 VAR_TIMEVALUEONLY, 2 VAR_DATEVALUEONLY
	template <typename TKEY>
	COleDateTime TO(TKEY k, DWORD flag = 0);

	template <typename TKEY>
	SYSTEMTIME TSys(TKEY k);

	template <typename TKEY> COleDateTime TOnlyDate(TKEY k) { return TO(k, 2); }
	template <typename TKEY> COleDateTime TOnlyTime(TKEY k) { return TO(k, 1); }

	template <typename TKEY>
	BOOL IsValidDateTime(TKEY k);


	static COleDateTime __base;// {COleDateTime(1980, 1, 1, 0, 0, 0)};

	template <typename TKEY>
	__time64_t Td(TKEY k)
	{
		auto t = T(k);
		return t.GetTime();
	}

	///?deprecated instead use this ex: _data("key").Format(L"%Y-%m-%d %H:%M:%S")
	//org ex: _data.TFmt("key", L"%Y-%m-%d %H:%M:%S")
	template <typename TKEY>
	CStringW TFmt(TKEY k, LPCWSTR fmt)
	{
		CStringW sfmt = fmt ? fmt : L"%Y-%m-%d %H:%M:%S";
		COleDateTime t = TO(k);
		return t.Format(sfmt);
	}
	template <typename TKEY>
	[[deprecated]]
	CStringW TFmt(TKEY k, LPCSTR fmt)
	{
		return TFmt(k, fmt ? CStringW(fmt) : (LPCWSTR)NULL);
	}
	template <typename TKEY>
	CStringW TFmt(TKEY k)
	{
		return TFmt(k, L"%Y-%m-%d %H:%M:%S");
	}



	/// 교체한 문자열을 리턴한다. 원래 내부값을 변경하지 않는다.
	template <typename TKEY>
	[[deprecated]]
	CStringW SReplace(TKEY k, LPCWSTR sOld, LPCWSTR sNew = L"")
	{
		CStringW sw = S(k);
		sw.Replace(sOld, sNew);
		return sw;
	}

	template <typename TKEY>
	CStringW Trim(TKEY k, PWS st = NULL);
	template <typename TKEY>
	CStringW TrimLeft(TKEY k, PWS st = NULL);
	template <typename TKEY>
	CStringW TrimRight(TKEY k, PWS st = NULL);

	template <typename TKEY>
	CStringW TrimStr(TKEY k, int dr, PWS st = NULL);


	/// 문자열을 교체하여 원래 내부값을 변경한다.
	template <typename TKEY>
	CStringW Replace(TKEY k, LPCWSTR sOld, LPCWSTR sNew = L"");
	/// 찾아서 제거. 즉, "" 과 Replace 한다.
	template <typename TKEY>
	CStringW FindAndDel(TKEY k, LPCWSTR sOld)
	{
		return Replace(k, sOld, L"");
	}

	/// 그냥 문자열이 있는지만 체크 한다.
	template <typename TKEY>
	BOOL Found(TKEY k, LPCWSTR str)
	{
		LPCWSTR sn = SP(k);
		return tchstr(sn, str) != NULL;
	}

	/// 1회 검색하여 위치를 알고 자플때 쓴다. 루프로 반복 할때는 UcJObj.S(k)로 먼저 받아서 써라.
	template <typename TKEY>
	int Find(TKEY k, LPCWSTR str, int istart = 0)
	{
		CStringW sn = S(k);
		return sn.Find(str, istart);
	}


	template <typename TKEY>
	BOOL BeginS(TKEY k, LPCWSTR str)
	{
		LPCWSTR sn = SP(k);
		return tchbegin(sn, str);
	}

	template <typename TKEY>
	BOOL Append(TKEY k, LPCWSTR str);

	/// src에서 tar로 필드 하나 복사
	/// souce키가 없으면 throw
	template <typename TKEY>
	void Copy(UcJObj & src, TKEY tarF, TKEY srcF = nullptr)
	{
		if (srcF == nullptr)
			srcF = tarF;
		LPCTSTR srcw = PTstr(srcF);
		BOOL bHas = CopyIf(src, PTstr(tarF), srcw);
		if (!bHas)
		{
			//CString s; s.Format(_T("UcJObj::Copy src(%s) field key Not found."), srcw);
			throw_str(_T("UcJObj::Copy src(%s) field key Not found."), srcw);


			//CString s773; s773.Format(_T("UcJObj::Copy src(%s) field key Not found."), srcw); {
			//	auto _ke = new KException("throw_gen", GetLastError(), 0, NULL, (PS)s773, __FUNCTIONW__, __LINE__, __FILE__, NULL, 0); throw _ke;
			//};
		}
	}
	/// source 키가 있는 경우만 복사. 
	template <typename TKEY>
	BOOL CopyIf(UcJObj & src, TKEY tarF, TKEY srcF = NULL)// = nullptr
	{
		if (!srcF)
			srcF = tarF;
		if (src.Has(PTstr(srcF)))//this->find(k) != this->end())
		{
			DeleteKey(PTstr(tarF));//타겟에 있으면 삭제
			auto sjvS = src[PTstr(srcF)];
			auto sjvT = make_shared<JVal>(sjvS, true);// ShJVal(new JVal(sjvS, true));
			Set(PTstr(tarF), sjvT);
			return TRUE;
		}
		return FALSE;
	}

	/// src에 필드가 있으면 복사
	//BOOL CopyIf(UcJObj& src, LPCSTR tarF, LPCSTR srcF = nullptr);// = nullptr
		/// 길이가 있고 내용이 같은 경우만 true 이다.
	int IsUpdated(UcJObj & src, JKEYSTR tarF, JKEYSTR srcF = nullptr);
	static int IsUpdated(JBase & src, JBase & tar, JKEYSTR tarF, JKEYSTR srcF = nullptr);
	static int IsUpdated(UcJObj & src, UcJObj & tar, JKEYSTR tarF, JKEYSTR srcF = nullptr);
	static int IsUpdated(ShJVal & src, ShJVal & tar, JKEYSTR tarF, JKEYSTR srcF = nullptr);
	static int IsUpdated(ShJVal & src, UcJObj & tar, JKEYSTR tarF, JKEYSTR srcF = nullptr);
	static int IsUpdated(UcJObj & src, ShJVal & tar, JKEYSTR tarF, JKEYSTR srcF = nullptr);

	/// CStringW으로 리턴
	template <typename TKEY>
	CStringW Str(TKEY k);

	template <typename TKEY>
	wstring wstr(TKEY k, wstring def = L"");

	template <typename TKEY>
	wstring wstr(TKEY k, UcJObj & def) {
		return wstr(k, def.wstr(k));
	}

	template <typename TKEY>
	std::string str(TKEY k, std::string def = "");

	template <typename TKEY>
	CStringW SLeft(TKEY k, int len);

	template <typename TKEY>
	CStringW SRight(TKEY k, int len);
	template <typename TKEY>
	CStringW SMid(TKEY k, int pos, int len = -1);

	template <typename TKEY>
	double N(TKEY k, double dfv = 0.);

	/// double 형으로 리턴
	template <typename TKEY>
	double D(TKEY k, double dfv = 0.) {
		return N(k, dfv);
	}
	template <typename TKEY>
	double D(TKEY k, UcJObj & def) {
		return D(k, def.D(k));
	}
	template <typename TKEY>
	float F(TKEY k, float dfv = 0.) {
		return N(k, dfv);
	}
	template <typename TKEY>
	float F(TKEY k, UcJObj & def) {
		return F(k, def.F(k));
	}

	// _buf.GetBuf(); 로 잡은 String buffer로 리턴 한다.
	// SQL 안에 쓸 문자 이므로 굳이 그렇게 해야 한다.
	CString QN(JKEYSTR k, int underDot = 0);
	///나중에 PTstr로
	//LPCWSTR QN(LPCSTR k, int underDot = 0) { CStringW sw(k); return QN(sw, underDot); }

	//double이 기본이지만 정수인지 확신할때
	template <typename TKEY>
	int I(TKEY k, int def = 0);

	template <typename TKEY>
	int I(TKEY k, UcJObj & def) {
		return I(k, def.I(k));
	}

	template <typename TKEY>
	DWORD DW(TKEY k, DWORD def = 0);

	template <typename TKEY>
	DWORD DW(TKEY k, UcJObj & def) {
		return DW(k, def.DW(k));
	}

	template <typename TKEY>
	COLORREF Color(TKEY k, COLORREF def = RGB(0, 0, 0));

	template <typename TKEY>
	COLORREF Color(TKEY k, UcJObj & def) {
		return Color(k, def.Color(k));
	}

	template <typename TKEY>
	__int64 I64(TKEY k, __int64 dfv = 0);

	template <typename TKEY>
	__int64 I64(TKEY k, UcJObj & def) {
		return I64(k, def.I64(k));
	}

	template <typename TKEY>
	bool b(TKEY k, bool dfv = false);

	template <typename TKEY>
	bool b(TKEY k, UcJObj & def) {
		return b(k, def.b(k));
	}

	template <typename TKEY>
	BOOL B(TKEY k, BOOL dfv = FALSE);

	template <typename TKEY>
	BOOL B(TKEY k, UcJObj & def) {
		return B(k, def.B(k));
	}

	/// "5,15,5,20" 형태의 문자열을 tuple<int,int,int,int>로 파싱
	template <typename TKEY>
	std::tuple<int, int, int, int> QuadI(TKEY k, std::tuple<int, int, int, int> def = { 0, 0, 0, 0 });

	template <typename TKEY>
	std::tuple<int, int, int, int> QuadI(TKEY k, UcJObj & def) {
		return QuadI(k, def.QuadI(k));
	}

	/// "5,15,20" 형태의 문자열을 tuple<int,int,int>로 파싱
	template <typename TKEY>
	std::tuple<int, int, int> TripleI(TKEY k, std::tuple<int, int, int> def = { 0, 0, 0 });

	template <typename TKEY>
	std::tuple<int, int, int> TripleI(TKEY k, UcJObj & def) {
		return TripleI(k, def.TripleI(k));
	}

	/// "5.5,15.2,5.8,20.1" 형태의 문자열을 tuple<double,double,double,double>로 파싱
	template <typename TKEY>
	std::tuple<double, double, double, double> QuadD(TKEY k, std::tuple<double, double, double, double> def = { 0., 0., 0., 0. });

	template <typename TKEY>
	std::tuple<double, double, double, double> QuadD(TKEY k, UcJObj & def) {
		return QuadD(k, def.QuadD(k));
	}

	/// "5.5,15.2,20.1" 형태의 문자열을 tuple<double,double,double>로 파싱
	template <typename TKEY>
	std::tuple<double, double, double> TripleD(TKEY k, std::tuple<double, double, double> def = { 0., 0., 0. });

	template <typename TKEY>
	std::tuple<double, double, double> TripleD(TKEY k, UcJObj & def) {
		return TripleD(k, def.TripleD(k));
	}

	template <typename TKEY>
	std::tuple<float, float, float, float> QuadF(TKEY k, std::tuple<float, float, float, float> def = { 0.f, 0.f, 0.f, 0.f });

	template <typename TKEY>
	std::tuple<float, float, float, float> QuadF(TKEY k, UcJObj & def) {
		return QuadF(k, def.QuadF(k));
	}

	/// "5.5,15.2,20.1" 형태의 문자열을 tuple<float,float,float>로 파싱
	template <typename TKEY>
	std::tuple<float, float, float> TripleF(TKEY k, std::tuple<float, float, float> def = { 0.f, 0.f, 0.f });

	template <typename TKEY>
	std::tuple<float, float, float> TripleF(TKEY k, UcJObj & def) {
		return TripleF(k, def.TripleF(k));
	}

	/// "100,200" 형태의 문자열을 pair<wstring,wstring>로 파싱
	template <typename TKEY>
	std::pair<std::wstring, std::wstring> PairS(TKEY k, std::pair<std::wstring, std::wstring> def = { L"", L"" });

	template <typename TKEY>
	std::tuple<std::wstring, std::wstring> PairS(TKEY k, UcJObj & def) {
		return PairS(k, def.PairS(k));
	}

	/// "100,200" 형태의 문자열을 pair<int,int>로 파싱
	template <typename TKEY>
	std::pair<int, int> PairI(TKEY k, std::pair<int, int> def = { 0, 0 });

	template <typename TKEY>
	std::tuple<int, int> PairI(TKEY k, UcJObj & def) {
		return PairI(k, def.PairI(k));
	}

	/// "100.5,200.3" 형태의 문자열을 pair<double,double>로 파싱
	template <typename TKEY>
	std::pair<double, double> PairD(TKEY k, std::pair<double, double> def = { 0., 0. });

	template <typename TKEY>
	std::tuple<double, double> PairD(TKEY k, UcJObj & def) {
		return PairD(k, def.PairD(k));
	}

	/// "100.5,200.3" 형태의 문자열을 pair<float,float>로 파싱
	template <typename TKEY>
	std::pair<float, float> PairF(TKEY k, std::pair<float, float> def = { 0.f, 0.f });

	template <typename TKEY>
	std::tuple<float, float> PairF(TKEY k, UcJObj & def) {
		return PairF(k, def.PairF(k));
	}

	template <typename TKEY>
	ShJArr A(TKEY k, bool bCreat = false);

	template <typename TKEY>
	ShJArr Array(TKEY k, bool bCreat = false) {
		return A(k, bCreat);
	}

	/// 이건 키가 없으면 exception 나므로 키 체크 먼저 하거나,  bCreate{true}로 불러야 한다.
	template <typename TKEY>
	UcJArr& GetArray(TKEY k, bool bCreat = false) {
		auto shArr = A(k, bCreat);
		if (shArr)
			return *shArr->Arr();
		else
			throw_str(L"UcJObj::A(%s, %d) returns null.", CStringW(k).GetString(), bCreat);
	}

	/// 이건 포인터로 받으니 널체크 해야 한다.
	/// 이건 주로 읽을때 쓰면 편리 하다.
	template <typename TKEY>
	UcJArr* GetArrayPtr(TKEY k, bool bCreat = false)
	{
		ShJVal sjv;
		jstring jk = PTstr(k);
		if (Lookup(jk, sjv)) {
			if (sjv->Val()->IsArray())
				return sjv->Arr();
		}
		return NULL;// shArr ? shArr->Arr() : NULL;
	}

	/// 복사 되는게 아니고, 내부 객체가 그대로 가르킨것이 shared_ptr로 리턴된다.
	template <typename TKEY>
	ShJVal O(TKEY k, bool bCreat = false);


	SHP<UcJTable> Table() {
		return Table((UcJObj*)this);
	}
	SHP<UcJTable> Table(UcJObj* pbj);//dwk: 2026-03-25 15:10 

	template <typename TKEY>
	SHP<UcJTable> Table(TKEY k);//dwk: 2026-03-25 15:10 

	/// 이건 위에 O()와 동일
	template <typename TKEY>
	ShJVal Obj(TKEY k, bool bCreat = false) {
		return O(k, bCreat);
	}

	/// 이건 주로 create하여 바로 쓸때 쓰며, 그외에는 키가 있는지 체크 하고 해야 써야 하므로 O(k)를 쓰는게 낫다.
	template <typename TKEY>
	UcJObj& GetObj(TKEY k, bool bCreat = false) {
		auto shObj = O(k, bCreat);
		if (shObj)
			return *shObj->Dic();
		else
			throw_str(L"UcJObj::O(%s, %d) returns null.", CStringW(k).GetString(), bCreat);
	}

	/// 이건 주로 읽을때 쓰면 편리 하다.
#ifdef _sample__
	auto pPatchApp = ver.GetObjPtr("PatchApp");
	if (pPatchApp)
	{
		auto shPatchAppVersions = pPatchApp->O("PatchAppVersions");
	}
#endif // _sample__
	template <typename TKEY>
	UcJObj* ODic(TKEY k, bool bCreat = false) {
		if (auto shObj = O(k, bCreat))
			return shObj->Dic();
		return  NULL;
	}

	template <typename TKEY>
	UcJObj* GetObjPtr_deprecated(TKEY k, bool bCreat = false) {
		return ODic(k, bCreat);
	}

	template <typename TKEY>
	ShJVal OO(TKEY k1, TKEY k2);

	//template <typename TKEY>
	//ShJObj OO(TKEY k1, string k2) { return OO(k1, k2.c_str());}
	//ShJObj OO(string k1, string k2) {return OO(k1.c_str(), k2.c_str());}

	template <typename TKEY>
	ShJArr OA(TKEY k1, TKEY k2);

	template <typename TKEY>
	ShJVal AO(TKEY k1, int k2);

	template <typename TKEY>
	CStringW OOS(TKEY k1, TKEY k2, TKEY k3);

	template <typename TKEY>
	CStringW AOS(TKEY k1, int k2, TKEY k3);

	/// GetArrayItem 참조 : 거기 내부 Array 접근 하는거 많이 만들어져 있음.

	template <typename TKEY>
	JUnit operator()(TKEY k) {
		return JUnit(this, PTstr(k));
	}

	/// 경로 파싱을 지원하는 operator() - "table/data/contents/0/0/font/name" 형태
	JUnit operator()(wchar_t const* path);

	template <typename TKEY>
	JUnit Unit(TKEY k)
	{
		return JUnit(this, PTstr(k));
	}

	/// 1차키의 값이 배열, 그 배열의 2차 인덱스가 UcJObj 크리고 2차키로 값을 담아 온다.
	/// 즉 table을 가져 와서 idx번째 row 중 k 키값을 가져 온다.
	bool GetArrayItem(JKEYSTR karr, int idx, JKEYSTR k, CStringW & rval);
	bool GetArrayItem(JKEYSTR karr, int idx, JKEYSTR k, int& rval);

	ShJVal GetArrayItem(JKEYSTR karr, int idx, JKEYSTR k);
	ShJObj GetArrayItem(JKEYSTR karr, int idx);

	static void DeepLoopRecursive(UcJObj & jRoot, vector<jstring>&keySub, int nth, vector<function<void(UcJObj&, int)>>&arFnc, bool bRecursive = true);
	static void DeepLoopRecursive(UcJArr & jArr, vector<jstring>&keySub, int nth, vector<function<void(UcJObj&, int)>>&arFnc, bool bRecursive = true);
	//static void DeepLoopRecursive(ShJBase jv   , vector<jstring>& keySub, int nth, vector<function<void(ShJBase,int)>>& arFnc, bool bRecursive = true);


	void DeepKeyLoop(std::initializer_list<string> initSub, std::initializer_list<std::function<void(UcJObj&, int)>> initCb)
	{
		vector<jstring> keySub;
		for (const auto& it : initSub)
		{
			CStringW kw(it.c_str());
			keySub.push_back((LPCWSTR)kw);
		}
		DeepKeyLoop(keySub, initCb);
	}
	void DeepKeyLoop(std::vector<jstring> initSub, std::initializer_list<std::function<void(UcJObj&, int)>> initCb, bool bRecursive = true);
	// n level JObj, 1 lambda
	void DeepKeyLoop(vector<jstring> keySub, function<void(UcJObj&, int)> initCb) {
		DeepKeyLoop(keySub, { initCb });
	}
	// 1 level JObj, 1 lambda
	void DeepKeyLoop(function<void(UcJObj&, int)> initCb, bool bRecursive = true) {
		DeepKeyLoop({}, { initCb }, bRecursive);
	}

	/// 키값이 있나 체크
	template <typename TKEY>
	bool Has(TKEY k) {
		return __super::Find(PTstr(k)) != this->end();
	}

	template <typename TKEY>
	ShJVal FindKey(TKEY k)
	{
		auto it = __super::Find(PTstr(k));
		return (it != this->end()) ? it->second : ShJVal();//여기서 ' error C2059: syntax error: '{}' 하면 안되.
	}

	/// 데이터가 없는 경우만 디폴트 값으로 넣는다. 
	/// <returns>Has의 리턴값과 같다. </returns>
	template<typename TKEY, typename TVAL> bool HasElse(TKEY k, TVAL v)
	{
		LPCWSTR pwk = PTstr(k);
		if (!Has(pwk))
		{
			(*this)(pwk) = v;
			return false;
		}
		return true;
	}
	

	template <typename TKEY> bool IsString(TKEY k);
	template <typename TKEY> bool IsArray(TKEY k);
	template <typename TKEY> bool IsObject(TKEY k);
	template <typename TKEY> bool IsNumber(TKEY k);
	template <typename TKEY> bool IsDouble(TKEY k);
	template <typename TKEY> bool IsInt64(TKEY k);
	template <typename TKEY> bool IsInt(TKEY k);
	template <typename TKEY> bool IsNull(TKEY k);


	std::wstring ToJsonStringWStr(int lvPreety = 2, function<int(LPCWSTR, int)> cbChk = NULL);
	CStringW ToJsonStringW(int lvPreety = 2, function<int(LPCWSTR, int)> cbChk = NULL);
	CStringA ToJsonStringUtf8(int lvPreety = 2, function<int(LPCWSTR, int)> cbChk = NULL);
	shared_ptr<KBinary> ToJsonBinaryUtf8(int lvPreety = 2);
	KBinary ToJsonData();
	bool CopyFielsIf(UcJObj & src, JKEYSTR key);
	int CopyFieldsAll(UcJObj & src);

	/// 키값이 없는 경우만 넣는다. 디폴트값으로 초기화 할때 쓰인다.
	template<typename TVAL>
	bool SetIfNull(LPCSTR key, TVAL val, bool bOverwrite = false)
	{
		if (!this->Has(key) || bOverwrite) {
			(*this)(key) = val;
			return true;
		}
		return false;
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
	#if CPP17_OR_LATER
			for (const auto& [key, value] : doubles) SetIfNull(key, value);
			for (const auto& [key, value] : ints) SetIfNull(key, value);
			for (const auto& [key, value] : strings) SetIfNull(key, value);
			for (const auto& [key, value] : bools) SetIfNull(key, value);
	#else
			for (const auto& pair : doubles) { const auto& key = pair.first; const auto& value = pair.second; SetIfNull(key, value); }
			for (const auto& pair : ints) { const auto& key = pair.first; const auto& value = pair.second; SetIfNull(key, value); }
			for (const auto& pair : strings) { const auto& key = pair.first; const auto& value = pair.second; SetIfNull(key, value); }
			for (const auto& pair : bools) { const auto& key = pair.first; const auto& value = pair.second; SetIfNull(key, value); }
	#endif
		}

		/// pair 배열 방식으로 여러 기본값 설정
		template<typename TVAL>
		void SetDefaultsFromPairs(const std::vector<std::pair<LPCSTR, TVAL>>& pairs)
		{
	#if CPP17_OR_LATER
			for (const auto& [key, value] : pairs) {
				SetIfNull(key, value);
			}
	#else
			for (const auto& pair : pairs) {
				const auto& key = pair.first;
				const auto& value = pair.second;
				SetIfNull(key, value);
			}
	#endif
		}
	*/
	/// 초기화 리스트 방식으로 여러 기본값 설정 (혼합 타입 지원)
	void SetDefaults(std::initializer_list<std::pair<LPCSTR, std_any>> pairs, bool bOverwrite = false);

	/// 숫자타입인 경우 값을 증감 시킨다. 값이 없으면 그 값을 넣는다.
	template <typename TKEY>
	void Inc(TKEY k, int inc = 1);

	/// JSON table 스키마: 최상위 키 아래에 "type":"table", "fields":[...], "rows":[[열…], …].
	/// Cell(table, col, row): col=열(0…), row=행(0…). 스키마·범위 오류는 throw_str.
	/// 실제 접근은 Table(tableKey) → UcJTable::Cell / CellS 등 사용.
#if 0 // 미구현 선언만 있었음(정의 없음). 미사용이라 인스턴스화 안 되어 빌드는 통과. 나중에 삭제하거나 구현 시 #if 0 제거.
	template <typename TKEY>
	static ShJVal Cell(UcJObj& root, TKEY tableName, size_t col, size_t row);
	template <typename TKEY>
	ShJVal Cell(TKEY tableName, size_t col, size_t row);
	template <typename TKEY>
	size_t ColSize(TKEY tableName);
	template <typename TKEY>
	size_t RowSize(TKEY tableName);
	template <typename TKEY>
	CStringW CellS(TKEY tableName, size_t col, size_t row, LPCWSTR def = L"");
	template <typename TKEY>
	int CellI(TKEY tableName, size_t col, size_t row, int def = 0);
	template <typename TKEY>
	__int64 CellI64(TKEY tableName, size_t col, size_t row, __int64 def = 0);
	template <typename TKEY>
	double CellD(TKEY tableName, size_t col, size_t row, double def = 0.);
	template <typename TKEY>
	float CellF(TKEY tableName, size_t col, size_t row, float def = 0.f);
#endif
};

#if CPP17_OR_LATER
/// 임의 키 타입 → wstring; `DyneStr`(UcBaseTools.h)로 변환 후 디버그 시 `_fncFieldCheck` 호출.
template<typename T>
inline std::wstring PTstr(T&& k)
{
	std::wstring kw = DyneStr(std::forward<T>(k));
#ifdef _DEBUG
	if (!UcJObj::s_bSkipFieldCheck && UcJObj::_fncFieldCheck)
		UcJObj::_fncFieldCheck(kw);
#endif
	return kw;
}
#endif



/// UcJArr는 JSON 객체 배열을 나타내는 클래스



/// UcJArr는 JSON 객체 배열을 나타내는 클래스
class UCTOOLDYNAMIC UcJArr : public KArray<ShJVal>, public JBase
{
public:
	UcJArr()
	{
	}
	explicit UcJArr(UcJArr& jarr, bool bClone = true);
	explicit UcJArr(ShJArr jarr, bool bClone = true);

	bool _bValueOwner{ true };
	~UcJArr() override {};
	static void CloneArray(UcJArr& src, UcJArr& tar, bool bClone = true); //JArr는 std::vector 이므로 자체 Clone이 없다.
	static void CloneArray(ShJVal src, ShJVal tar, bool bClone = true);
	void Clone(ShJVal src, bool bClone = true)
	{
		if (!src)
			throw_str(_T("UcJArr::Clone(src,) src is empty."));
		ASSERT(src->IsArr());
		auto& src1 = *src->Arr();// *dynamic_cast<UcJArr*>(&src);
		UcJArr::CloneArray(src1, *this, bClone);
	}
	void Add(const wchar_t* v);
	void Add(const char* v);
	void Add(const std::wstring& v) {
		Add(v.c_str());
	}
	void Add(const CStringW& v) {
		Add(v.GetString());
	}
	void Add(const CStringA& v) {
		CStringW sw(v);
		Add(sw.GetString());
	}
	void Add(double v);
	void Add(int v);
	void Add(INT64 v);
	void Add(unsigned __int64 v); // size_t 지원
	void Add(bool v);
	void Add(const _variant_t& v);
	void Add(UcJObj& v, bool bClone = true);
	///void Add(UcJArr& v, bool bClone = true); array안에 array넣는 것은 아직 없네?

	//ShJVal Add(ShJObj sv);
	ShJVal Add(ShJObj sv, bool bClone);// = true); 기본값을 없애야 실수 안한다.
	//void Add(ShJVal sv, bool bClone = true);
	//UcJObj& Add(UcJObj&& obj); error
	UcJObj& AddObj();

	UcJObj& InsertObj(int idx);

	void DeleteAt(int idx);

	//template <typename TObj>
	//TObj& Add(TObj&& obj)
	//{
	//	shared_ptr<TObj> sv = make_shared<UcJObj>(forward<UcJObj>(obj));
	//	ShJVal sjv = make_shared<JVal>(sv, false);
	//	__super::Add(sjv);
	//	return sv->IsDic() ?  *sv->Dic() : *sv->Arr();//error C2446: ':': 'Kw::UcJArr'에서 'Kw::UcJObj'(으)로 변환되지 않았습니다.
	//}
	// 
	//UcJObj& Add(UcJObj&& obj)
	//{
	//	std::shared_ptr<UcJObj> sv = std::make_shared<UcJObj>(std::forward<UcJObj>(obj));
	//	ShJVal sjv = make_shared<JVal>(sv, false);
	//	__super::Add(sjv);
	//	return *sv->Dic();
	//}


	ShJObj FindByValue(JKEYSTR field, LPCWSTR value);

	BOOL IsSame(UcJArr& jar2);
	BOOL operator==(UcJArr& jar2)// const
	{
		return IsSame(jar2);
	}
	BOOL operator!=(UcJArr& jar2)
	{
		return !IsSame(jar2);
	}
	UcJObj& GetObj(int i);
	ShJObj GetAtObj(int i);
	CStringW GetAtS(int i, LPCWSTR def = L"");
	int GetAtI(int i, int def = 0);
	INT64 GetAtI64(int i, INT64 def = 0);
	double GetAtD(int i, double dfv = 0.);

	void DeepArrayLoop(vector<jstring> keySub, initializer_list<function<void(UcJObj&, int)>> initCb, bool bRecursive = true);

};




/* ex:
class IStrConvert2
{
public:
	virtual const wchar_t* CharToString(const wchar_t* key, wchar_t ch) = NULL;
};
class MyStrConverter : public IStrConvert2 {
	public:
		virtual const wchar_t* CharToString(const wchar_t* key, wchar_t ch) override {
			switch(ch) {
				case L'\n': return L"\\n";      // 줄바꿈을 \n으로
				case L'\t': return L"\\t";      // 탭을 \t으로
				case L'"':  return L"\\\"";     // 따옴표를 \"으로
				case L'\\': return L"\\\\";     // 백슬래시를 \\으로
				default:    return nullptr;     // 변환하지 않음
			}
		}
	};
*/


/// <summary>
/// Represents a JSON parsing utility that tracks line and column numbers while processing wide-character data.
/// </summary>
class UCTOOLDYNAMIC JTrain {
public:
	const wchar_t** _ppData;
	size_t _len;
	size_t _cur;
	const wchar_t* _pStart;
	function<int(int, int, LPCWSTR)> _cb;

	// JSON 라인 번호 추적을 위한 멤버 추가
	int _jsonLine;
	int _jsonColumn;

	JTrain(const wchar_t** ppData, size_t len, function<int(int, int, LPCWSTR)> cb)
		: _ppData(ppData), _len(len), _cur(0), _pStart(*ppData), _cb(cb), _jsonLine(1), _jsonColumn(1)
	{
	}

	//void UpdateLineColumn(const wchar_t* currentPos)
	//{
	//	if (currentPos >= _pStart)
	//	{
	//		_jsonColumn = 1;
	//		for (const wchar_t* p = _pStart; p < currentPos; p++)
	//		{
	//			if (*p == L'\n')
	//			{
	//				_jsonLine++;
	//				_jsonColumn = 1;
	//			}
	//			else
	//			{
	//				_jsonColumn++;
	//			}
	//		}
	//	}
	//}

	void Check(const wchar_t** data);
};

//#define _USEKVAL






/// obj_ arr_ 를 하나로 합침.
/// 기존 코드 가독성을 위해 arr_를 코드에 남겨둠.

class UCTOOLDYNAMIC JVal : public JBase
{
public:
	//bool _bOwner{ false }; 
	/// UcJArr, UcJObj 가 자체적으로 내부 값 delete 하는 _bValueOwner를 가지고 delete 한다.
	/// 그래서 필요 없다.
	JVal();
	explicit JVal(const wchar_t* char_value1);
	explicit JVal(const char* char_value1);
	explicit JVal(const std::string str_value1);
	explicit JVal(const wstring& wstring_value1);
	explicit JVal(bool bool_value1);
	explicit JVal(double number_value1);
	explicit JVal(int int_value1);// long과 다름
	explicit JVal(__int64 int64_value1);
	explicit JVal(unsigned int int_value1);// long과 다름
	explicit JVal(unsigned __int64 int64_value1);
	// size_t는 unsigned __int64와 같은 타입일 수 있으므로 별도 오버로드 없음
	explicit JVal(CTime t);
	explicit JVal(COleDateTime t);

	/// const는 shared_ptr 값을 못바꾸고, 안에 pointer가 가르키는 객체는 바꿀수 있다.
	///?warning `= true`를 제거 하면, 내부 오류 나온다. xutility(255,61)
	explicit JVal(ShJVal sjv, bool bClone = true);
	/// 이걸 assign할때는 삭제 안되는 shared_ptr로 싸서 넣는데 그때 const이면 안들어 간다.
	explicit JVal(UcJObj& jv, bool bClone = true);//clone해야 한다. jv가 스택 변수 이고  false이면 큰일 단다.
	explicit JVal(UcJArr& jv, bool bClone = true);//clone해야 한다.//false를 주면 ~jv에서 free하지 않는다.

	//dwk: 2025-02-12 15:55 JSON 추가 필드 2 
	explicit JVal(CStringArray& jv);
	template<typename TArrayElem>
	explicit JVal(CArray<TArrayElem, TArrayElem>& jv);
	explicit JVal(std::vector<std::wstring>& jv);
	explicit JVal(std::vector<std::string>& jv);
	explicit JVal(std::vector<int>& jv);
	explicit JVal(std::vector<unsigned int>& jv);
	explicit JVal(std::vector<INT64>& jv);
	explicit JVal(std::vector<double>& jv);
	explicit JVal(std::vector<size_t>& jv);
	explicit JVal(std::vector<TCString<TCHAR>>& jv);
#ifdef _MBCS
	explicit JVal(std::vector<CStringW>& jv);
#else
	explicit JVal(std::vector<CStringA>& jv);
#endif
	explicit JVal(std::vector<_variant_t>& jv);

	explicit JVal(std::list<std::wstring>& jv);
	explicit JVal(std::list<int>& jv);
	explicit JVal(std::list<INT64>& jv);
	explicit JVal(std::list<double>& jv);
	explicit JVal(std::list<size_t>& jv);
	explicit JVal(std::list<TCString<TCHAR>>& jv);
#ifdef _MBCS
	explicit JVal(std::list<CStringW>& jv);
#else
	explicit JVal(std::list<CStringA>& jv);
#endif
	explicit JVal(CStringList& ja);		//dwk: 2025-12-02 11:20 JSON CStringList 지원

	explicit JVal(CArray<int, int>& ja);
	explicit JVal(CArray<INT64, INT64>& ja);
	explicit JVal(CArray<double, double>& ja);
	explicit JVal(CArray<TCString<TCHAR>, TCString<TCHAR>>& ja);
#ifdef _MBCS
	explicit JVal(CArray<CStringW, CStringW>& ja);
#else
	explicit JVal(CArray<CStringA, CStringA>& ja);
#endif
	// VS2015에서 CArray<CString, const TYPE&> 형태를 위해 추가
	explicit JVal(CArray<CString, const CString&>& ja);

	explicit JVal(CArray<CStringW, CStringA>& ja);
	explicit JVal(CArray<CStringA, CStringW>& ja);

	// CArray 템플릿 생성자 (CArray<T, T> 및 CArray<T, const T&> 모두 지원)
	template<typename TArrayElem, typename TArg>
	explicit JVal(CArray<TArrayElem, TArg>& ja);

	explicit JVal(std::map<std::wstring, std::wstring>& jv);
	explicit JVal(std::map<std::wstring, int>& jv);
	explicit JVal(std::map<std::wstring, INT64>& jv);
	explicit JVal(std::map<std::wstring, double>& jv);
	explicit JVal(std::map<std::wstring, CStringW>& jv);
	explicit JVal(std::map<std::wstring, CStringA>& jv);
	explicit JVal(std::map<std::wstring, std::vector<std::wstring>>& jv);



	/// array 지원을 위한 생성자 (initializer_list 대응)
	explicit JVal(const std::array<int, 2>& v);
	explicit JVal(const std::array<int, 4>& v);
	explicit JVal(const std::array<double, 2>& v);
	explicit JVal(const std::array<double, 4>& v);
	explicit JVal(const std::array<float, 2>& v);
	explicit JVal(const std::array<float, 4>& v);

	JVal(const JVal& jv, bool bClone);//clone해야 한다.
	JVal(const JVal& jv) {
		this->Clone(&jv, true);
	}
	~JVal() override {}


	/// 내부 저장 형태의 type
	JsonType _type{ eNul };

	/// memeber 5 variables
	wstring ___;

	ShJVal nod_;//shared_ptr<JBase>

	/// XML 속성 여부 (true: 속성, false: 서브키) JSON에서는 안쓴다.
	bool _bAttr{ false };
	enum { eOptional = 0, eRequired = 1, };
	int _option{ 0 };//

	void DumpDebug();

	void toString();
	void JSonTextVal(std::wstring& sts, int maxlen = 50, bool bQuat = true);



	// 생성 되면서 = 를 쓸경우는 JVal(JVal& jv)가 불린다.
	void operator=(const JVal& jv);


	/// clone 방지
	void ShareObj(UcJObj& obj1);

	bool IsNull() { return _type == eNul; }
	bool IsString() { return _type == eStr; }
	bool IsBool() { return _type == eBol; }
	bool IsDouble() { return _type == eFlt; }
	bool IsInt64() { return _type == eI64; }
	bool IsInt() { return _type == eInt; }
	bool IsTime() { return _type == eTme; }

	bool IsNumber() { return IsDouble() || IsInt() || IsInt64(); }// type == ejNumber;
	bool IsArray() { return _type == eArr; }
	bool IsObject() { return _type == eObj; }

	/*error C2664: 'void Kw::JVal::setInt(int)': 인수 1을(를) 'CStringW'에서 'int'(으)로 변환할 수 없습니다.
		template<typename TVAL>
		void setVal(TVAL v)
		{
			if(typeid(v) == typeid(LPCWSTR) || typeid(v) == typeid(CStringW))
			{
				setString(v);
			}
			else if(typeid(v) == typeid(LPCSTR) || typeid(v) == typeid(CStringA))
			{
				CStringW vw(v);
				setString(vw);
			}
			else if(typeid(v) == typeid(int))
			{
				setInt(v);
			}
			else if(typeid(v) == typeid(long))
			{
				setInt(v);
			}
			else if(typeid(v) == typeid(INT64))
			{
				setInt64(v);
			}
			else if(typeid(v) == typeid(CTime))
			{
				setTime(v);
			}
			else if(typeid(v) == typeid(COleDateTime))
			{
				setOTime(v);
			}
			else if(typeid(v) == typeid(UINT))
			{
				setUInt(v);
			}
			else if(typeid(v) == typeid(UINT64))
			{
				setUInt64(v);
			}
		}*/


	const wstring& AsString() { return ___; }
	std::string AsStringA() {
		std::string str = CStringA(___.c_str()).GetString();
		return str;
	}
	double AsDouble() {
		try {
			return std::stod(___);
		}
		catch (const std::invalid_argument& e) {
			e;
			ASSERT(IsDouble());
			ASSERT("변환 불가: " == 0);
			return 0;
		}
		catch (const std::out_of_range& e) {
			e;
			ASSERT("범위 초과: " == 0);
			return -1;
		}
	}	//double AsNumber() ;
	int AsInt() {
		//ASSERT(IsInt());	
		try {
			return std::stoi(___);
			//int a = std::stoi("123");           // 정상: a = 123
			//int b = std::stoi("abc");           // ❌ invalid_argument
			//int c = std::stoi("999999999999");  // ❌ out_of_range
		}
		catch (const std::invalid_argument& e) {
			e;
			ASSERT(IsInt());
			ASSERT("변환 불가: " == 0);
			return 0;
		}
		catch (const std::out_of_range& e) {
			e;
			ASSERT("범위 초과: " == 0);
			return -1;
		}
	}
	__int64 AsInt64() {
		try {
			return std::stoll(___);
		}
		catch (const std::invalid_argument& e) {
			e;
			ASSERT(IsInt64());
			ASSERT("변환 불가: " == 0);
			return 0;
		}
		catch (const std::out_of_range& e) {
			e;
			ASSERT("범위 초과: " == 0);
			return -1;
		}
	}

	bool AsBool(function<bool(CStringW&)> rd = nullptr);
	ShJArr AsArray() { ASSERT(IsArray());	return nod_; } //앞에 const 없앰. array편집 하려고
	ShJObj AsObject() { ASSERT(IsObject());	return nod_; }
	UcJObj* AsObjPtr();
	UcJArr* AsArrPtr();

	CStringW S(LPCWSTR def = L"")
	{
		if (IsNull() || ___.length() == 0)
			return def;// L"";
		return CStringW(___.c_str()); // error 나서 CStringW 로 싸서 리턴 // dwkang 2023-08-14 14:39
	}
	CStringW FStr(int point = 2);
	LPCWSTR Ptr();
#ifdef _UseGCBuf__
	CStringW& SRef();
#endif // _UseGCBuf__
	wstring& stref() { return ___; }
	CString ST(LPCTSTR def = _T(""));
	CStringA SA(LPCSTR def = "");
	LPCWSTR SP(LPCWSTR def = L""); // nullable
	LPCWSTR Txt(LPCWSTR def = L"") {
		if (IsString())
			return (LPCWSTR)___.c_str();
		else// if (IsNull())
			return def;
	}
	CTime T();

	LPCWSTR TypeStr();

	// #define VAR_TIMEVALUEONLY       ((DWORD)0x00000001)    /* return time value */
	// #define VAR_DATEVALUEONLY       ((DWORD)0x00000002)    /* return date value */
		/// flag: 0 full, 1 VAR_TIMEVALUEONLY, 2 VAR_DATEVALUEONLY
	COleDateTime TO(DWORD flag = 0);
	SYSTEMTIME TSys();
	CStringW SLeft(int len);
	CStringW SRight(int len);
	CStringW SMid(int pos, int len = -1);



	double N(double dfv = 0.);
	double D(double dfv = 0.)
	{
		return N(dfv);
	}
	int I(int def = 0);
	DWORD DW(DWORD def = 0);
	COLORREF Color(COLORREF def = RGB(0, 0, 0));
	__int64 I64(__int64 def = 0);

	/// 숫자타입인 경우 값을 증감 시킨다.
	void Inc(double inc = 1);


	void operator+=(const wchar_t* v)
	{
		ASSERT(this->IsString());
		wstring& sref = this->StrRef();
		sref += v;
	}

	void operator+=(double v)
	{
		switch (_type)
		{
		case eFlt:	this->Inc(v); //v_.n.d += v; 
			break;
		case eInt:	this->Inc(v);
			break;
		case eI64:	this->Inc(v); //this->v_.n.i += (__int64)v; 
			break;
		default:
			ASSERT(0);
		}
	}

	void operator+=(__int64 v)
	{
		switch (this->_type)
		{
		case eFlt:	this->Inc((double)v);
			break; //this->v_.n.d += (double)v; break;
		case eInt:this->Inc((double)v);
			break;
		case eI64:this->Inc((double)v);
			break;//	this->v_.n.i += v; break;
		default:
			ASSERT(0);
		}
	}
	void operator+=(CTimeSpan v);
	void operator+=(COleDateTimeSpan v);


	void Clone(const JVal* src, bool bClone = true);


	std::size_t CountChildren() const;
	bool HasChild(std::size_t index) const;
	ShJVal Child(std::size_t index);
	bool HasChild(JKEYSTR name) const;
	ShJVal Child(JKEYSTR name);
	wstring Stringify(const bool bUnicode = true, int prettyprint = 1, const wchar_t* key = nullptr, function<PWS(PWS, wchar_t)> pinf = {}, function<int(PWS, int)> cbChk = {});

	bool IsNan();


	static ShJVal Parse(const wchar_t** data);
	static ShJVal Parse(shared_ptr<JTrain> tr);

private:
	static wstring StringifyString(const bool bUnicode, const wstring& str, const wchar_t* key, function<PWS(PWS, wchar_t)> pinf = {});
	wstring StringifyImpl(const bool bUnicode, size_t const indentDepth, const wchar_t* key = nullptr, function<PWS(PWS, wchar_t)> pinf = {}, int tab = 0, function<int(PWS, int)> cbChk = {});

	[[deprecated]]
	static wstring Indent(size_t depth);

public:
	void InitArray();
	void InitObject();
	wstring& StrRef()
	{
		return ___;
	}
	void setString(LPCWSTR sw)
	{
		_type = eStr;
		___ = sw;
	}

	template<typename TNUM>
	void SetNumber(TNUM n)
	{
		___ = std::to_wstring(n);
		//#ifdef _DEBUG
		if (___.find('.') != wstring::npos)
		{
			wstrrtrim(___, L"0");
			wstrrtrim(___, L".");
		}
		//#endif // _DEBUG
	}

	void setDouble(double v)
	{
		_type = eFlt;
		SetNumber(v);
	}
	void setInt(int v)
	{
		_type = eInt;
		SetNumber(v);
	}
	void setInt64(INT64 v)
	{
		_type = eI64;
		SetNumber(v);
	}
	void setUInt(UINT v)
	{
		_type = eI64;
		SetNumber(v);
	}
	void setUInt64(ULONGLONG v)
	{
		_type = eI64;
		SetNumber(v);
	}
	void setBool(bool v)
	{
		_type = eBol;
		___ = v ? L"true" : L"false";
	}
	void setTime(CTime t);
	void setOTime(COleDateTime t);

	int setValue(ShJVal snd1);

	int CompareValue(JVal& jval1);
	int CompareValue(ShJVal snd1)
	{
		auto pVal = snd1->Val();
		return CompareValue(*pVal);
	}

	BOOL IsSame(JVal& jval1);
	BOOL IsSame(ShJVal snd1)
	{
		auto pVal = snd1->Val();
		return IsSame(*pVal);
	}
	BOOL operator==(JVal& jval1)// const
	{
		return IsSame(jval1);
	}
	BOOL operator!=(JVal& jval1)// const
	{
		return !IsSame(jval1);
	}

	std::wstring GetJSonText(int maxlen = 1024);


	/// see TO()
	COleDateTime ParseDateTime();
};




/// std::static_pointer_cast<JBase>(std::move(p));
// 함수 의미 설명: 
// std::shared_ptr<JVal> 타입의 포인터 p를 std::shared_ptr<JBase> 타입으로 변환하여 반환한다.
// JVal은 JBase를 상속받으므로, std::static_pointer_cast를 통해 안전하게 상위 타입 포인터로 변환 가능하다.
// move(p)를 쓰는 이유는 소유권을 이전하여 불필요한 참조 카운트 증가를 방지하려는 의도.
// 주로 JVal 스마트 포인터를, JBase를 사용하는 곳에 전달할 때 사용하는 변환 함수.

/// <summary>
/// static JSON 파서 및 유틸리티 클래스
/// </summary>
class UCTOOLDYNAMIC UcJson
{
	friend class JVal;

public:
	static ShJObj ParseUtf8(const char* data);
	static ShJVal ParseASCII(const char* data);
	static ShJVal Parse(const wchar_t* data, function<int(int, int, LPCWSTR)> cb = NULL);
	static ShJVal ParseTrain(SHP<JTrain> tr, function<int(int, int, LPCWSTR)> cb = NULL);
	static wstring Stringify(const ShJVal value);
	static bool SkipWhitespace(LPCWSTR* data, SHP<JTrain> tr = nullptr);
	static void SkipWhitespaceThrow(LPCWSTR* data, SHP<JTrain> tr = nullptr);
	static bool ExtractString(LPCWSTR* data, wstring& str, const wchar_t* startPos = nullptr);
	static double ParseInt(LPCWSTR* data);
	static double ParseDecimal(LPCWSTR* data);

	///                        FUNCTION LINE ERROR   JSON_POS   JSON_LINE   JSON_COLUMN
	static std::list<JException> s_errors;
	static void PushErr(const JException& ex);
	static JException PopErr();

	// JSON 라인 번호를 계산하여 예외를 던지는 헬퍼 함수
	static void ThrowJsonWithLineInfo(const char* function, int line, const string& message, const wchar_t* jsonPos, shared_ptr<JTrain> tr);


	static bool AsBool(CStringW sw);//dwk

	static ShJVal ParseXml(const wchar_t* data, function<int(int, int, LPCWSTR)> cb = {});
	// UTF-8 버전 (char*)
	static ShJVal ParseXml(const char* data, function<int(int, int, LPCWSTR)> cb = {});
	// UTF-8 버전 (std::string)
	static ShJVal ParseXml(const std::string& data, function<int(int, int, LPCWSTR)> cb = {});
	//static ShJVal ParseXmlNode(rpx::xml_node<wchar_t>* node); rpx::xml_node때문에 제외.

	static wstring StringifyXml(const ShJVal value);
	static std::string StringifyXmlUtf8(const ShJVal value);
	static bool SaveXmlToFile(const ShJVal value, const std::wstring& filePath);

private:
	UcJson();
};



#define throw_json(s, j) throw JException(__FUNCTION__, __LINE__, (s), (j), 0, 0)
#define throw_json_with_train(s, j, tr) UcJson::ThrowJsonWithLineInfo(__FUNCTION__, __LINE__, (s), (j), (tr))

// 서식 문자열을 지원하는 throw_json 매크로도 수정
#define throw_json_fmt(fmt, ...) do { \
    CStringW formatted; \
    formatted.Format(fmt, __VA_ARGS__); \
    throw JException(__FUNCTION__, __LINE__, "Error", formatted.GetString(), 0, 0); \
} while(0)







/// /////////////////////////////// inline function ////////////////////////////////////////////////
/// /////////////////////////////// inline function ////////////////////////////////////////////////
/// /////////////////////////////// inline function ////////////////////////////////////////////////
/// /////////////////////////////// inline function ////////////////////////////////////////////////

inline BOOL JBase::IsDic()
{
	const auto pVal = dynamic_cast<JVal*>(this);
	if (pVal)
		return pVal->nod_ ? pVal->nod_->IsDic() : FALSE;
	else
	{
		auto pObj = dynamic_cast<UcJObj*>(this);
		return pObj != nullptr;
	}
}

inline BOOL JBase::IsArr()
{
	if (auto pVal = dynamic_cast<JVal*>(this))
		return pVal->nod_ ? pVal->nod_->IsArr() : FALSE;
	else
	{
		auto pArr = dynamic_cast<UcJArr*>(this);
		return (pArr != NULL);
	}
}
inline BOOL JBase::IsVal()
{
	const auto pVal = dynamic_cast<JVal*>(this);
	return (pVal != NULL);
}

/// 아래 두 함수는 Dic인지 Arr인지 미리 알고(IsDic()로 체크) 부른다. 잘못 부르면 ASSERT.
/// NULL일수는 있지만 바꿔 부르면 안됨.
/// 주의: this 가 널이어도 멤버를 안쓰니 죽지는 않는다. null을 리턴 할 뿐
/// 그래서 이런 코드가 가능 하다. pJbj->O("CoverPageX")가 empty 여도 안죽는다.
/// auto CoverPage = pJbj->O("CoverPageX")->Dic();
/// 
inline UcJObj* JBase::Dic()
{
	if (this == nullptr)
		return nullptr;
	const auto pVal = dynamic_cast<JVal*>(this);
	if (pVal)
	{
		return pVal->nod_ ? pVal->nod_->Dic() : NULL;
	}
	else
	{
		const auto pObj = dynamic_cast<UcJObj*>(this);
		/// NULL 이면 둘다 NULL이어야 한다. 아니라면 이건 Array인데 Dic으로 잘못 부른 거지.
		if (pObj == NULL)
		{
			const auto pArr = dynamic_cast<UcJArr*>(this);
			//ASSERT(pArr == NULL);// array인데 Dic을 부르다니~~
			if (pArr) {
				throw_str(_T("It's Arr() not Dic()."));
			}
			//else
			//	throw_str(L"Dic() returns null.");
		}
		return pObj;//NULL이면 그런데로 그대로 리턴
	}
}

inline UcJArr* JBase::Arr()
{
	const auto pVal = dynamic_cast<JVal*>(this);
	if (pVal)
	{
		return pVal->nod_ ? pVal->nod_->Arr() : NULL;//recursive call
	}
	else
	{
		const auto pArr = dynamic_cast<UcJArr*>(this);
		if (pArr == NULL)
		{
			const auto pObj = dynamic_cast<UcJObj*>(this);
			//ASSERT(pObj == NULL);// dictionary인데 Arr을 부르다니
			if (pObj) {
				throw_str(_T("JBase::Arr() - It's Dic() not Arr()."));
			}
			//else
			//	throw_str(L"Arr() returns null.");
		}
		return pArr;
	}
}

inline JVal* JBase::Val()
{
	const auto pVal = dynamic_cast<JVal*>(this);
	return pVal;
}




inline CStringW JUnit::Left(int len)
{
	return m_pCJobj->Get(m_k)->Val()->SLeft(len);
}
inline CStringW JUnit::Right(int len)
{
	return m_pCJobj->Get(m_k)->Val()->SRight(len);
}
inline CStringW JUnit::Mid(int pos, int len)
{
	return m_pCJobj->Get(m_k)->Val()->SMid(pos, len);
}




inline void JUnit::operator+=(const wchar_t* v)
{
	m_pCJobj->Get(m_k)->Val()->operator+=(v);
}
inline void JUnit::operator+=(double v)
{
	m_pCJobj->Get(m_k)->Val()->operator+=(v);
}
inline void JUnit::operator+=(__int64 v)
{
	m_pCJobj->Get(m_k)->Val()->operator+=(v);
}
inline void JUnit::operator+=(CTimeSpan v)
{
	m_pCJobj->Get(m_k)->Val()->operator+=(v);
}
inline void JUnit::operator+=(COleDateTimeSpan v)
{
	m_pCJobj->Get(m_k)->Val()->operator+=(v);
}

inline BOOL JUnit::operator==(const wchar_t* v)
{
	return m_pCJobj->SameS(m_k, v);
}
inline BOOL JUnit::operator==(int v)
{
	return m_pCJobj->I(m_k) == v;
}
inline BOOL JUnit::operator==(__int64 v)
{
	return m_pCJobj->I64(m_k) == v;
}
inline BOOL JUnit::operator==(double v)
{
	return m_pCJobj->D(m_k) == v;
}

inline BOOL JUnit::operator<(const wchar_t* v)
{
	return m_pCJobj->Str(m_k) < v;
}
inline BOOL JUnit::operator<(int v)
{
	return m_pCJobj->I(m_k) < v;
}
inline BOOL JUnit::operator<(__int64 v)
{
	return m_pCJobj->I64(m_k) < v;
}
inline BOOL JUnit::operator<(double v)
{
	return m_pCJobj->D(m_k) < v;
}

inline BOOL JUnit::operator>(const wchar_t* v)
{
	return m_pCJobj->Str(m_k) > v;
}
inline BOOL JUnit::operator>(int v)
{
	return m_pCJobj->I(m_k) > v;
}
inline BOOL JUnit::operator>(__int64 v)
{
	return m_pCJobj->I64(m_k) > v;
}
inline BOOL JUnit::operator>(double v)
{
	return m_pCJobj->D(m_k) > v;
}


inline int JUnit::Find(LPCWSTR s, int istart)
{
	CStringW s2 = m_pCJobj->Str(m_k);
	return s2.Find(s, istart);
}
inline int JUnit::ReverseFind(wchar_t ch)
{
	CStringW s2 = m_pCJobj->Str(m_k);
	return s2.ReverseFind(ch);
}
inline void JUnit::Trim()
{
	m_pCJobj->Trim(m_k);
}
inline bool JUnit::IsEmpty()
{
	ShJVal sjv = m_pCJobj->Get(m_k);
	if (sjv)
	{
		if (sjv->Val()->IsString())
			return sjv->Val()->___.size() == 0;
		else
			throw_str(_T("IsEmpty() must be called in case IsString()."));
	}
	//다른 type이거나 
	return true;
}

inline CString JUnit::Format(LPCTSTR fmt)
{
	ShJVal sjv = m_pCJobj->Get(m_k);
	if (sjv)
	{
		auto jv = sjv->Val();
		if (jv->IsString() || jv->IsTime() || jv->IsInt64() || jv->IsDouble())
		{
			COleDateTime ot = m_pCJobj->TO(m_k);
			return ot.Format(fmt);
		}
	}
	return CString();
}

inline BOOL JUnit::IsValidDateTime()
{
	return m_pCJobj->IsValidDateTime(m_k);
}


inline ShJObj UcJArr::GetAtObj(int i)
{
	ShJObj row1;
	if (size() > (size_t)i)
	{
		ShJVal sjv = this->GetAt(i);
		auto jv = sjv->Val();
		if (jv->IsObject())
			row1 = jv->AsObject();
	}
	return row1;
}
inline UcJObj& UcJArr::GetObj(int i)
{
	ShJObj shObj = GetAtObj(i);
	return *shObj->Dic();
}
inline CStringW UcJArr::GetAtS(int i, LPCWSTR def)
{
	if (size() > (size_t)i)
	{
		ShJVal sjv = this->GetAt(i);
		auto v1 = sjv->Val();
		return v1->S(def);
	}
	return def;
}
inline int UcJArr::GetAtI(int i, int def)
{
	if (size() > (size_t)i)
	{
		ShJVal sjv = this->GetAt(i);
		auto v1 = sjv->Val();
		return v1->I(def);
	}
	return def;
}
inline INT64 UcJArr::GetAtI64(int i, INT64 def)
{
	if (size() > (size_t)i)
	{
		ShJVal sjv = this->GetAt(i);
		auto v1 = sjv->Val();
		return v1->I64(def);
	}
	return def;
}
inline double UcJArr::GetAtD(int i, double def)
{
	if (size() > (size_t)i)
	{
		ShJVal sjv = this->GetAt(i);
		auto v1 = sjv->Val();
		return v1->D(def);
	}
	return def;
}
inline UcJArr* JVal::AsArrPtr() { ASSERT(IsArray());	return nod_ ? nod_->Arr() : NULL; }
inline UcJObj* JVal::AsObjPtr() { ASSERT(IsObject());	return nod_ ? nod_->Dic() : NULL; }



/// double 실수 인경우 소수점 아래 수 참조 하여 문자열 리턴 123.12343421234 => "123.12"
template <typename TKEY>
inline CStringW UcJObj::FStr(TKEY k, int point, LPCWSTR def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->FStr(point);
	return def;
}

/// 항목이 있고 IsString 이면 리턴. 아니면 널

template <typename TKEY>
inline CStringW UcJObj::S(TKEY k, LPCWSTR def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->stref().c_str();
	return def;
}

template <typename TKEY>
inline CStringA UcJObj::SA(TKEY k, LPCSTR def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->SA(def);
	return def;
}

template <typename TKEY>
inline CString UcJObj::ST(TKEY k, LPCTSTR def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->ST(def);
	return def;
}

template <typename TKEY>
inline LPCWSTR UcJObj::SP(TKEY k, LPCWSTR def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->SP(def);
	return def;
}

template <typename TKEY>
inline LPCWSTR UcJObj::Ptr(TKEY k)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->Ptr();
	return (LPCWSTR)NULL;
}

template <typename TKEY>
inline LPCWSTR UcJObj::Txt(TKEY k, LPCWSTR def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->Txt(def);
	return def;
}

template <typename TKEY>
inline CTime UcJObj::T(TKEY k)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv)) //(*this)[(LPCWSTR)k]
		return sjv->Val()->T();
	return CTime();
}

template <typename TKEY>
inline COleDateTime UcJObj::TO(TKEY k, DWORD flag)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->TO(flag);
	return COleDateTime();
}

template <typename TKEY>
inline SYSTEMTIME UcJObj::TSys(TKEY k)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->TSys();
	return {};
}

template <typename TKEY>
inline BOOL UcJObj::IsValidDateTime(TKEY k)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
	{
		COleDateTime to = sjv->Val()->TO();//COleDateTime base(1980, 1, 1, 0, 0, 0);아직은 비지니스 기준을 정하지 않음.
		return to.GetStatus() == COleDateTime::valid
			&& to.m_dt > 0.0f// == to != COleDateTime(1899, 12, 30, 0, 0, 0)
			&& to != COleDateTime(1900, 1, 1, 0, 0, 0)
			&& to != COleDateTime(1970, 1, 1, 0, 0, 0);
	}
	return FALSE;
}

template <typename TKEY>
inline CStringW UcJObj::Trim(TKEY k, PWS st)
{
	return TrimStr(k, 0b01 | 0b10, st);
}

template <typename TKEY>
inline CStringW UcJObj::TrimLeft(TKEY k, PWS st)
{
	return TrimStr(k, 0b10);
}
template <typename TKEY>
inline CStringW UcJObj::TrimRight(TKEY k, PWS st)
{
	return TrimStr(k, 0b01);
}

/// <summary>
/// 
/// </summary>
/// <typeparam name="TKEY"></typeparam>
/// <param name="k"></param>
/// <param name="dr">
/// TKEY k: JSON 객체의 키
/// int dr : 제거 방향(비트 플래그)
/// 	0b01 : 오른쪽 공백 제거
/// 	0b10 : 왼쪽 공백 제거
/// 	0b01 | 0b10 : 양쪽 공백 제거
/// 	PWS st : 제거할 특정 문자들(NULL이면 기본 공백 문자들)
/// </param>
/// <param name="st"></param>
/// <returns></returns>
template <typename TKEY>
inline CStringW UcJObj::TrimStr(TKEY k, int dr, PWS st)
{
	ShJVal sjv;
	CStringW sw;
	if (Lookup(PTstr(k), sjv))
	{
		if (sjv->Val()->IsString())
		{
			sw = sjv->Val()->Val()->StrRef().c_str();
			if (tchlen(st) > 0)
			{
				if (dr & 0b01)
					sw.TrimRight(st);
				if (dr & 0b10)
					sw.TrimLeft(st);
			}
			else
			{
				if (dr & 0b01)
					sw.TrimRight();
				if (dr & 0b10)
					sw.TrimLeft();
			}
			//sw.Trim();
			sjv->Val()->___ = (LPCWSTR)sw;
			return sw;
		}
	}
	return sw;
}

template <typename TKEY>
inline CStringW UcJObj::Replace(TKEY k, LPCWSTR sOld, LPCWSTR sNew)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
	{
		if (sjv->Val()->IsString())
		{
			CStringW sw = sjv->Val()->StrRef().c_str();
			sw.Replace(sOld, sNew);
			sjv->Val()->setString((LPCWSTR)sw);
			return sw;
		}
	}
	return CStringW();
}
template <typename TKEY>
inline BOOL UcJObj::Append(TKEY k, LPCWSTR str)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
	{
		wstring& ws = sjv->Val()->AsString();
		ws += str;
		return TRUE;
	}
	(*this)(k) = str;
	return FALSE;
}
/// CStringW으로 리턴
template <typename TKEY>
inline CStringW UcJObj::Str(TKEY k)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->S();
	return CStringW();
}
template <typename TKEY>
inline wstring UcJObj::wstr(TKEY k, wstring def)// = L"")
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->AsString();
	return def;
}
template <typename TKEY>
inline std::string UcJObj::str(TKEY k, std::string def)// = "")
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return std::string(CStringA(sjv->Val()->AsString().c_str()));
	return def;
}

template <typename TKEY>
inline CStringW UcJObj::SLeft(TKEY k, int len)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->SLeft(len);
	return L"";
}

template <typename TKEY>
inline CStringW UcJObj::SRight(TKEY k, int len)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->SRight(len);
	return CStringW();
}

template <typename TKEY>
inline CStringW UcJObj::SMid(TKEY k, int pos, int len)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->SMid(pos, len);
	return CStringW();
}

template <typename TKEY>
inline int UcJObj::I(TKEY k, int def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->I(def);
	return def;
}
template <typename TKEY>
inline DWORD UcJObj::DW(TKEY k, DWORD def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->DW(def);
	return def;
}

template <typename TKEY>
inline COLORREF UcJObj::Color(TKEY k, COLORREF def)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->Color(def);
	return def;
}

template <typename TKEY>
inline __int64 UcJObj::I64(TKEY k, __int64 dfv)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->I64(dfv);
	return dfv;
}

template <typename TKEY>
inline bool UcJObj::b(TKEY k, bool dfv)
{
	return B(k, dfv ? TRUE : FALSE) ? true : false;
}

template <typename TKEY>
inline BOOL UcJObj::B(TKEY k, BOOL dfv)
{
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->AsBool();
	return dfv;
}

// JSON에서 "rect": "5,15,5,20" 형태의 데이터가 있을 때
// tuple 사용
// C++17 structured binding 사용 가능
//auto [left, top, right, bottom] = obj.QuadI("rect");

template <typename TKEY>
inline std::tuple<int, int, int, int> UcJObj::QuadI(TKEY k, std::tuple<int, int, int, int> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty())
	{
		str.Trim();
		std::vector<int> values = UcCutByTokenInt(str, L", \t", true);
		// 최대 4개 값만 사용
		auto& [l, t, r, b] = def;
		int result[4] = { l, t, r, b };
		for (int i = 0; i < 4 && i < (int)values.size(); i++)
			result[i] = values[i];

		return std::make_tuple(result[0], result[1], result[2], result[3]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [first, second, third] = obj.TripleI("position");

template <typename TKEY>
inline std::tuple<int, int, int> UcJObj::TripleI(TKEY k, std::tuple<int, int, int> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty())
	{
		str.Trim();//str = L"RGB(0,150,160)" -> {0,150,160) 앞에 RGB는 자동으로 제거 되네)
		std::vector<int> values = UcCutByTokenInt(str, L", \t", true);
		// 최대 3개 값만 사용
		auto& [first, second, third] = def;
		int result[3] = { first, second, third };
		for (int i = 0; i < 3 && i < (int)values.size(); i++)
			result[i] = values[i];

		return std::make_tuple(result[0], result[1], result[2]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [left, top, right, bottom] = obj.QuadD("rect");
template <typename TKEY>
inline std::tuple<double, double, double, double> UcJObj::QuadD(TKEY k, std::tuple<double, double, double, double> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<double> values = UcCutByTokenDouble(str, L", \t", true);
		// 최대 4개 값만 사용
		auto& [l, t, r, b] = def;
		double result[4] = { l, t, r, b };
		for (int i = 0; i < 4 && i < (int)values.size(); i++)
			result[i] = values[i];
		return std::make_tuple(result[0], result[1], result[2], result[3]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [first, second, third] = obj.TripleD("position");
template <typename TKEY>
inline std::tuple<double, double, double> UcJObj::TripleD(TKEY k, std::tuple<double, double, double> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<double> values = UcCutByTokenDouble(str, L", \t", true);
		// 최대 3개 값만 사용
		auto& [first, second, third] = def;
		double result[3] = { first, second, third };
		for (int i = 0; i < 3 && i < (int)values.size(); i++)
			result[i] = values[i];
		return std::make_tuple(result[0], result[1], result[2]);
	}
	return def;
}
template <typename TKEY>
inline std::tuple<float, float, float, float> UcJObj::QuadF(TKEY k, std::tuple<float, float, float, float> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<double> values = UcCutByTokenDouble(str, L", \t", true);
		auto& [l, t, r, b] = def;
		float result[4] = { l, t, r, b };
		for (int i = 0; i < 4 && i < (int)values.size(); i++)
			result[i] = values[i];// 최대 4개 값만 사용
		return std::make_tuple(result[0], result[1], result[2], result[3]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [first, second, third] = obj.TripleF("position");
template <typename TKEY>
inline std::tuple<float, float, float> UcJObj::TripleF(TKEY k, std::tuple<float, float, float> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<double> values = UcCutByTokenDouble(str, L", \t", true);
		auto& [first, second, third] = def;
		float result[3] = { first, second, third };
		for (int i = 0; i < 3 && i < (int)values.size(); i++)
			result[i] = values[i];// 최대 3개 값만 사용
		return std::make_tuple(result[0], result[1], result[2]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [first, second] = obj.PairS("pair");
template <typename TKEY>
inline std::pair<std::wstring, std::wstring> UcJObj::PairS(TKEY k, std::pair<std::wstring, std::wstring> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		CArray<CString> strValues;
		UcCutByToken(str, L", \t", strValues, true);
		auto& [first, second] = def;
		std::wstring result[2] = { first, second };
		for (int i = 0; i < 2 && i < strValues.GetSize(); i++)
			result[i] = (LPCTSTR)strValues[i];// 최대 2개 값만 사용
		return std::make_pair(result[0], result[1]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [x, y] = obj.PairI("position");
template <typename TKEY>
inline std::pair<int, int> UcJObj::PairI(TKEY k, std::pair<int, int> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<int> values = UcCutByTokenInt(str, L", \t", true);
		auto& [first, second] = def;
		int result[2] = { first, second };
		for (int i = 0; i < 2 && i < (int)values.size(); i++)
			result[i] = values[i];// 최대 2개 값만 사용
		return std::make_pair(result[0], result[1]);
	}
	return def;
}

// C++17 structured binding 사용 가능
//auto [x, y] = obj.PairD("position");
template <typename TKEY>
inline std::pair<double, double> UcJObj::PairD(TKEY k, std::pair<double, double> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<double> values = UcCutByTokenDouble(str, L", \t", true);
		auto& [first, second] = def;
		double result[2] = { first, second };
		for (int i = 0; i < 2 && i < (int)values.size(); i++)
			result[i] = values[i];// 최대 2개 값만 사용
		return std::make_pair(result[0], result[1]);
	}
	return def;
}
template <typename TKEY>
inline std::pair<float, float> UcJObj::PairF(TKEY k, std::pair<float, float> def)
{
	CStringW str = this->S(k, L"");
	if (!str.IsEmpty()) {
		str.Trim();
		std::vector<double> values = UcCutByTokenDouble(str, L", \t", true);
		auto& [first, second] = def;
		float result[2] = { first, second };
		for (int i = 0; i < 2 && i < (int)values.size(); i++)
			result[i] = values[i];// 최대 2개 값만 사용
		return std::make_pair(result[0], result[1]);
	}
	return def;
}

template <typename TKEY>
inline double UcJObj::N(TKEY k, double dfv)
{
	if (this == NULL)
		throw (L"UcJObj.this == NULL");
	//?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
	ShJVal sjv;
	if (Lookup(PTstr(k), sjv))
		return sjv->Val()->N(dfv);
	return dfv;
}

template <typename TKEY>
inline ShJArr UcJObj::A(TKEY k, bool bCreat)
{
	ShJVal sjv;
	jstring jk = PTstr(k);
	if (Lookup(jk, sjv))
	{
		if (sjv->Val()->IsArray())
			return sjv->Val()->AsArray();
		else
			throw_str(L"Not a Array.");
	}
	if (bCreat)
	{
		ShJArr sjo = make_shared<UcJArr>();
		SetArray(jk, sjo, false); /// 주의:false가 맞음. bCreat); 위에서 만들고 복사해서 넣고 임시로 만든걸 리턴 하면 어떡해
		return sjo;
	}
	else
	{
		//CString s;s.Format(L"Array key \"%s\" is not found.", CString(jk.c_str()));
		//throw_str(L"Array key \"%s\" is not found.", CString(jk.c_str()).GetString());//s);
	}
	return {};// ShJArr();
}



template <typename TKEY>
inline ShJObj UcJObj::O(TKEY k, bool bCreat)
{
	/// 아래 ->find에서 unhandled로 못빠져 나온다. 왜 unhandled
	//?주의: if((*this)[k]) 이걸 쓰는 쑨간 만들어져 버린다.
	if (this == nullptr)
	{
		auto kw = PTstr(k);
		throw_str(L"this == nullptr. key:%s , %d", kw.c_str(), bCreat);
	}
#ifdef _DEBUGx
	{
		auto kw = PTstr(k);
		throw_str(L"this == nullptr. key:%s , %d", kw.c_str(), bCreat);
	}
#endif // _DEBUGx
	ShJVal sjv;
	jstring jk = PTstr(k);
	if (Lookup(jk, sjv))
	{
		if (sjv->Val()->IsObject())
			return sjv->Val()->AsObject();// shared_ptr 내부가 그대로 노출 된다.
		else
			throw_str(L"Not a Object.");
	}
	if (bCreat)
	{
		ShJObj sjo = make_shared<UcJObj>();
		SetObj(jk, sjo, false); /// 주의:false가 맞음.
		return sjo;
	}
	return {};//ShJObj();
}


template <typename TKEY>
SHP<UcJTable> UcJObj::Table(TKEY key)//dwk: 2026-03-25 15:10 
{
	ShJObj shTbl = this->O(key, false);
	if (!shTbl || !shTbl->IsDic())
		return {};
	return Table(shTbl->Dic());
}

template <typename TKEY>
void UcJObj::Inc(TKEY k, int inc)
{
	ASSERT(inc != 0);
	auto kw = PTstr(k);
	ShJVal sjv;
	if (!Lookup(kw, sjv))
		(*this)(kw) = inc;
	else
		sjv->Val()->Inc(inc);
}

template <typename TKEY>
UcJArr& UcJObj::SetArray(TKEY k)
{
	ShJVal sjv = make_shared<JVal>(make_shared<UcJArr>());
	Set(PTstr(k), sjv);
	return *sjv->Val()->Arr();
}

//DWKREMINDER("매크로 남발 하지 말자. 비슷한 코드가 반복 되더라도 함수 정의는 그대로 해야 찾기 편하다.")
template <typename TKEY> inline bool UcJObj::IsString(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsString() : false;
}
template <typename TKEY> inline bool UcJObj::IsArray(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsArray() : false;
}
template <typename TKEY> inline bool UcJObj::IsObject(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsObject() : false;
}
template <typename TKEY> inline bool UcJObj::IsNumber(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsNumber() : false;
}
template <typename TKEY> inline bool UcJObj::IsDouble(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsDouble() : false;
}
template <typename TKEY> inline bool UcJObj::IsInt64(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsInt64() : false;
}
template <typename TKEY> inline bool UcJObj::IsInt(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsInt() : false;
}
template <typename TKEY> inline bool UcJObj::IsNull(TKEY k){
	ShJVal sjv;
	return Lookup(PTstr(k), sjv) ? sjv->Val()->IsNull() : false;
}
//IsValTYPE(String);
//IsValTYPE(Array);
//IsValTYPE(Object);
//IsValTYPE(Number);
//IsValTYPE(Double);
//IsValTYPE(Int64);
//IsValTYPE(Int);
//IsValTYPE(Null);


//}; // Kw

#define KJSPUT(val) js(#val) = val
#define KJSGETS(val) val = js.S(#val)
#define KJSGETSA(val) val = CStringA(js.S(#val))
#define KJSGETN(val) val = js.N(#val)
#define KJSGETI(val) val = js.I(#val)


template <typename TKEY>
inline ShJObj UcJObj::AO(TKEY k1, int k2)
{
	ASSERT(k2 >= 0);
	ShJObj sjo1 = A(k1);
	if (sjo1)
	{
		auto jo1 = sjo1->Arr();
		if (jo1 && jo1->size() <= k2)
			return jo1->GetAt(k2);
	}
	return {};
}

template <typename TKEY>
inline CStringW UcJObj::AOS(TKEY k1, int k2, TKEY k3)
{
	ShJObj sjo = AO(k1, k2);
	if (sjo)
	{
		auto ja = sjo->Dic();
		if (ja)
			return ja->S(k3);
	}
	return {};
}

template <typename TKEY>
inline CStringW UcJObj::OOS(TKEY k1, TKEY k2, TKEY k3)
{
	ShJObj sjo = OO(k1, k2);
	if (sjo)
	{
		auto jo1 = sjo->Dic();
		if (jo1)
			return jo1->S(k3);
	}
	return {};
}

template <typename TKEY>
inline ShJObj UcJObj::OO(TKEY k1, TKEY k2)
{
	ShJObj sjo1 = O(k1);
	if (sjo1)
	{
		auto jo1 = sjo1->Dic();
		if (jo1)
			return jo1->O(k2);
	}
	return {};
}

template <typename TKEY>
inline ShJArr UcJObj::OA(TKEY k1, TKEY k2)
{
	ShJObj sjo1 = O(k1);
	if (sjo1)//?error:cppcheck  <- !sjo1
	{
		auto jo1 = sjo1->Dic();
		if (jo1)
			return jo1->Array(k2);
	}
	return {};
}


#if CPP17_OR_LATER

void AnyToJSon(UcJObj& jbj, wstring key, const std_any& aval);

void PairToJSon(UcJObj& jbj, const std::pair<wstring, std_any>& pr);

void SaveFieldsToJSON(UcJObj& jbj, std::map<wstring, std_any>& mapToStore);
#endif


//using unsigned_long = unsigned long;
//using unsigned___int64 = unsigned __int64;
/// error C1128: number of sections exceeded object file format limit: compile with /bigobj
/// 아래 map을 함수 안에 있을때 너무 커져서 발생
#if CPP_BEFORE_17
EXTERN_STATIC std::unordered_map<std::type_index, std::function<void(UcJObj&, wstring, void*)>> handlers_;
#else
#include "UcJHandler.inl"
#endif

template< typename VType>
bool JSonToVariable(UcJObj& jbj, wstring key, VType& aval)
{
	DWKUSETRACE;
	bool bKey = jbj.Has(key);
	if (!bKey)
		return false;

	auto& theType = typeid(aval);
#ifdef _DEBUG
	std::type_index tidx = theType;
	auto pName1 = theType.name();//class std::shared_ptr<class CStringArray>
	auto pName = UcShortTypeT(aval);// theType.name();//class std::shared_ptr<class CStringArray>
#endif // _DEBUG	//DWKFUNCV("L[%s] \"%s\"", key, pName);
	auto it = handlers_.find(theType);

	auto fEnum = [](UcJObj& j1, wstring k2, void* v3) {
		*((int*)v3) = (int)j1.I(k2);
		};


#if CPP17_OR_LATER
	if constexpr (std::is_enum_v<std::decay_t<VType>>) {
		fEnum(jbj, key, (void*)&aval);// 이름 있는 enum은 모두 정수로 읽어 들인다. unnamed-enum은 그냥 정수로 인식 한다.
	}
#else
	if (std::is_enum<typename std::decay<VType>::type>::value) {
		fEnum(jbj, key, (void*)&aval);// aval 에 값을 넣어 주는 람다함수를 호출 한다.
	}
#endif
	else if (it != handlers_.end()) {
		ShJArr shArr;
		//DWKTRACE("[%s] \"%s\"", key, pName);
		//if (jbj.Has(key)) {
		auto shv = jbj.Get(key);// .IsDic()
		if (shv->IsDic()) {// dic or item key
			auto& jd = *shv->Dic();
			UcJObj* pjbj = nullptr;
			if (jd.Has(TAG_ITM)) {//item key 있네
				auto shJit = jd.Get(TAG_ITM);
				if (shJit->IsArr()) {// item 밑에 array 라는걸 확인(xml의 배열 형식)
					auto jarr = shJit->Arr();// 굳이 할 필요 있나? get array of item
					it->second(jd, TAG_ITM, (void*)&aval);//변수 aval에 배열을 쏙
				}
			}
			else {// IsDic 인데 일반 dic은
		it->second(jbj, key, (void*)&aval);
			}
		}
		else {// 배열이든 어떤 타입 이든 그냥 해당 람다 함수를 부름. if(shv->IsArr())
			it->second(jbj, key, (void*)&aval);
		}
		//}
	}
	else {
#ifdef _DEBUG
		DWKTRACE(L"[%v] \"%v\"", key, CStringW(pName1));//"class KArray<class std::basic_string<wchar_t,struct std::char_traits<wchar_t>,class std::allocator<wchar_t> > >"
#endif // _DEBUG
		/// handlers_에 등록된 가능한 타입 목록 디버그 출력
		for (const auto& kv : handlers_) {
			const std::type_index& ti = kv.first;
			CStringA nameA(ti.name());
			CStringW nameW(nameA);
#ifdef _DEBUG
			DWKTRACE(L"  %v : %v", tidx == ti, nameW);
#endif // _DEBUG
		}
		//ASSERT(0 == "json에서 변수로 넣는 타입 지원 안함");
		return false;
	}
	return true;
}



//현재 JUnit=에서 지원 하는  complex type들 체크 하여 ar << 가 가능 하게 한다.
EXTERN_STATIC std::tuple<
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
	//CArray<int, int>,
	//CArray<double, double>,
	//CArray<CStringW, CStringW>,
	//CArray<CStringA, CStringA>
> s_typeSupported;


/// 아래 두개면, for(auto& pObj : lst) 지원 가능하다고 해서 테스트//dwk: 2025-11-05 09:43 
template<typename T, typename ARG_T>
T* begin(CArray<T, ARG_T>& arr) { return arr.GetSize() ? &arr[0] : nullptr; }
template<typename T, typename ARG_T>
T* end(CArray<T, ARG_T>& arr) { return arr.GetSize() ? &arr[0] + arr.GetSize() : nullptr; }





/// JUnit의 tuple/pair operator= 구현


inline void JUnit::operator=(const std::array<int, 2>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

inline void JUnit::operator=(const std::array<int, 4>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

inline void JUnit::operator=(const std::array<double, 2>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

inline void JUnit::operator=(const std::array<double, 4>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

inline void JUnit::operator=(const std::array<float, 2>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

inline void JUnit::operator=(const std::array<float, 4>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

//inline void JUnit::operator=(const std::list<TCString<TCHAR>>& v)
//{
//	ShJVal sjv = make_shared<JVal>(v);
//	m_pCJobj->Set(m_k, sjv);
//}

/// JUnit::operator= for CArray<T, T> and CArray<T, const T&>
/// 두 버전 모두 동일한 구현이므로 템플릿 특수화로 통합
template<typename TArrayElem, typename TArg>
inline void JUnit::operator=(CArray<TArrayElem, TArg>& v)
{
	ShJVal sjv = make_shared<JVal>(v);
	m_pCJobj->Set(m_k, sjv);
}

/// JVal의 CArray 생성자 구현 (CArray<T, T> 및 CArray<T, const T&> 모두 지원)
template<typename TArrayElem, typename TArg>
inline JVal::JVal(CArray<TArrayElem, TArg>& ja)
{
	_type = eArr;
	InitArray();
	auto arr = this->Arr();
	for (int i = 0; i < ja.GetCount(); i++) {
		arr->Add(ja.GetAt(i));
	}
}

/// JVal의 array 생성자 구현
inline JVal::JVal(const std::array<int, 2>& rv)
{
	_type = eStr;
	CStringW str;
	auto& v = rv;
	str.Format(L"%d,%d", v.at(0), v.at(1));//c++17에서는 v[0] 이렇게 쓸수 있지만, c++14에서는 v.at(0) 이렇게 써야 한다.
	//C++14에서는 const std::array의 operator[]가 const로 오버로드되지 않아서 발생하는 문제였습니다. 
	// C++17에서는 const 버전의 operator[]가 추가되었습니다.
	___ = str.GetString();
}

inline JVal::JVal(const std::array<int, 4>& v)
{
	_type = eStr;
	CStringW str;
	str.Format(L"%d,%d,%d,%d", v.at(0), v.at(1), v.at(2), v.at(3));
	___ = str.GetString();
}

inline JVal::JVal(const std::array<double, 2>& v)
{
	_type = eStr;
	CStringW str;
	str.Format(L"%.3f,%.3f", v.at(0), v.at(1));
	___ = str.GetString();
}

inline JVal::JVal(const std::array<double, 4>& v)
{
	_type = eStr;
	CStringW str;
	str.Format(L"%.3f,%.3f,%.3f,%.3f", v.at(0), v.at(1), v.at(2), v.at(3));
	___ = str.GetString();
}

inline JVal::JVal(const std::array<float, 2>& v)
{
	_type = eStr;
	CStringW str;
	str.Format(L"%.3f,%.3f", v.at(0), v.at(1));
	___ = str.GetString();
}

inline JVal::JVal(const std::array<float, 4>& v)
{
	_type = eStr;
	CStringW str;
	str.Format(L"%.3f,%.3f,%.3f,%.3f", v.at(0), v.at(1), v.at(2), v.at(3));
	___ = str.GetString();
}

///[ Samples
// C:\Dropbox\Proj\KProj\ThreadCopy\UcAppSample1\UcDoc.cpp
// CUcDoc::Serialize
///] Samples

/// 초기화 리스트 방식으로 여러 기본값 설정 (혼합 타입 지원)
inline void UcJObj::SetDefaults(std::initializer_list<std::pair<LPCSTR, std_any>> pairs, bool bOverwrite)
{
	for (const auto& pair : pairs) {
		auto key = pair.first;
		auto value = pair.second;
		// std_any의 타입에 따라 적절한 SetIfNull 호출
		if (value.type() == typeid(double))
			SetIfNull(key, std_any_cast<double>(value), bOverwrite);
		else if (value.type() == typeid(int))
			SetIfNull(key, std_any_cast<int>(value), bOverwrite);
		else if (value.type() == typeid(float))
			SetIfNull(key, std_any_cast<float>(value), bOverwrite);
		else if (value.type() == typeid(LPCWSTR))
			SetIfNull(key, std_any_cast<LPCWSTR>(value), bOverwrite);
		else if (value.type() == typeid(bool))
			SetIfNull(key, std_any_cast<bool>(value), bOverwrite);
		else if (value.type() == typeid(DWORD))
			SetIfNull(key, std_any_cast<DWORD>(value), bOverwrite);
		else if (value.type() == typeid(__int64))
			SetIfNull(key, std_any_cast<__int64>(value), bOverwrite);

		else if (value.type() == typeid(std::array<int, 2>))
			SetIfNull(key, std_any_cast<std::array<int, 2>>(value), bOverwrite);
		else if (value.type() == typeid(std::array<int, 4>))
			SetIfNull(key, std_any_cast<std::array<int, 4>>(value), bOverwrite);
		else if (value.type() == typeid(std::array<double, 2>))
			SetIfNull(key, std_any_cast<std::array<double, 2>>(value), bOverwrite);
		else if (value.type() == typeid(std::array<double, 4>))
			SetIfNull(key, std_any_cast<std::array<double, 4>>(value), bOverwrite);
		else if (value.type() == typeid(std::array<float, 2>))
			SetIfNull(key, std_any_cast<std::array<float, 2>>(value), bOverwrite);
		else if (value.type() == typeid(std::array<float, 4>))
			SetIfNull(key, std_any_cast<std::array<float, 4>>(value), bOverwrite);
		else {
			ASSERT(0);
			throw_json_fmt(L"Unkown type to set.(%s)", CStringA(value.type().name()).GetString());
		}
	}
}


///[ moved from UcTool.h
#ifdef _DEBUGx // to go HttpClient
SHP<JBase> UcGetRemoteJson(CStringW sUrl, int* piStatus = NULL, PWS sFunc = 0, PWS sFile = 0, int nLine = 0);
#define LUcGetRemoteJson(sUrl,piStatus)      UcGetRemoteJson(sUrl, piStatus,__FUNCTIONW__, __FILEW__, __LINE__)
#define LUcGetRemoteJson1(sUrl)      UcGetRemoteJson(sUrl, NULL,__FUNCTIONW__, __FILEW__, __LINE__)
#endif

/// 특정 레지스트리 키아래 값을 루프로 돌면서 JSON객체에 다 넣는다.
void UcRegistryKeyValuesToJson(CRegKey& regKey, UcJObj& jbj);

UCTOOLDYNAMIC
void UcJsonToData(UcJObj& jDocData, SHP<JBase>& sjobj, bool bToJson);
void UcJsonSave(UcJObj& jDocData, CFile& oFile, function<int(LPCWSTR, int)> cbChk = NULL, int preety = 3);
//void UcJsonSave(UcJObj& jDocData, CStringW sFile, BOOL bBackup = FALSE);
int UcJsonLoad(SHP<JBase>& jDocData, CFile& oFile, function<int(int, int, LPCWSTR)> cb = nullptr);
int UcJsonLoad(SHP<JBase>& jDocData, CString sFile);
int UcJsonLoad(SHP<JBase>& jDocData, LPCSTR psUtf8, DWORD len, function<int(int, int, LPCWSTR)> cb = nullptr);

UCTOOLDYNAMIC
int UcJsonLoad(SHP<JBase>& jDocData, LPCWSTR sWstr, DWORD len, function<int(int, int, LPCWSTR)> cb = nullptr);

inline SHP<JBase> UcJsonLoad(LPCWSTR sWstr, DWORD len, function<int(int, int, LPCWSTR)> cb = nullptr){
	SHP<JBase> jDocData;
	int rv = UcJsonLoad(jDocData, sWstr, len, cb);
	return jDocData;
}
int UcJsonLoad(UcJObj& jDocData, CFile& oFile, function<int(int, int, LPCWSTR)> cb = nullptr);

void UcJsonSave(UcJObj& jDocData, CString sPath, BOOL bBackup = FALSE, int nDayExpire = 0, int preety = 3);
void UcJsonSave(SHP<JBase> jDocData, CString sPath, BOOL bBackup = FALSE, int nDayExpire = 0, int preety = 3);

//int UcJsonSerialize(UcJObj& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cbCustom = nullptr, function<int(LPCWSTR, int)> cbChk = NULL);

UCTOOLDYNAMIC
int UcJsonSerialize(UcJObj& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb = nullptr, function<int(LPCWSTR, int)> cbChk = NULL);

inline 
int UcJsonSerialize(SHP<JBase>& jDocData, CArchive& ar, function<int(int, int, LPCWSTR)> cb = nullptr, function<int(LPCWSTR, int)> cbChk = NULL){
	return UcJsonSerialize(*jDocData->Dic(), ar, cb, cbChk);
}



inline size_t UcJTable::ColSize()
{
	if (!_fields->IsArr())
		throw_str(L"no fields.");
	auto pFields = _fields->Arr();
	return pFields->size();
}
inline size_t UcJTable::RowSize()
{
	if (!_rows->IsArr())
		throw_str(L"no fields.");
	auto rows = _rows->Arr();
	return rows->size();
	int Col(LPCSTR colName);
}
inline int UcJTable::Col(LPCWSTR colName)
{
	if (!colName || !*colName)
		throw_str(L"UcJTable::Col() col name is empty.");
	std::wstring key = colName;
	auto it = _mFieldCol.find(key);
	if (it == _mFieldCol.end())
		throw_str(L"UcJTable::Col() field \"%s\" not found.", key.c_str());
	return it->second;
}
inline int UcJTable::Col(LPCSTR colName)
{
	CStringW wColName(colName);
	return Col(wColName.GetString());
}

//dwk: 2026-04-21 14:59 UcJTable::GetRow 새로 추가
inline ShJBase UcJTable::GetRow(size_t row)
{
	UcJArr* pFields = _fields->Arr();
	UcJArr* pRows = _rows->Arr();
	const size_t nRow = pRows->size();
	if (row >= nRow)
		throw_str(L"GetRow table row %v out of range (%v).", row, nRow);
	ShJBase sRow = pRows->GetAt((int)row);
	if (!sRow)
		throw_str(L"GetRow table row is not JVal(%v).", row);
	if (!sRow->IsArr())
		throw_str(L"GetRow row(%v) is not array.", row);
	return sRow;
}
inline ShJBase UcJTable::GetCell(size_t col, size_t row)
{
	ShJBase sRow = GetRow(row);
	if (!sRow->IsArr())
		throw_str(L"GetCell row(%v) is not array.", row);
	auto arrRow = sRow->Arr();
	const size_t rowColSize = arrRow->size();
	if (col >= rowColSize)
		throw_str(L"GetCell table col %v out of range for row %v (%v).", col, row, rowColSize);
	ShJBase cell = arrRow->GetAt((int)col);//dwk: 2026-04-21 14:59 //여기 col 적용을 누락 했군.
	return cell;
}

inline JVal* UcJTable::Cell(size_t col, size_t row)
{
	ShJBase sRow = GetCell(col, row);// pRows->GetAt((int)row);
	if (!sRow->IsVal())
		throw_str(L"UcJObj::Cell table row is not JVal(%v, %v).", col, row);
	auto pVal = sRow->Val();
	ASSERT(pVal);
	return pVal;
}

template<typename TKEY, typename TVAL>
inline UcJObj::InitItem::InitItem(TKEY k, const TVAL& v)
	: key(PTstr(k))
	, val(make_shared<JVal>(v))
{
}

inline UcJObj::UcJObj(std::initializer_list<InitItem> initList)
{
	for (const auto& it : initList) {
		Set(it.key.c_str(), it.val);
	}
}

inline void UcJObj::operator=(std::initializer_list<InitItem> initList)
{
	clear();
	for (const auto& it : initList) {
		Set(it.key.c_str(), it.val);
	}
}

inline JUnit JUnit::operator()(int key)
{
	return operator()(PTstr(key));
}

//dwk: 2025-12-01 12:35 
//dwk: 2025-12-02 11:09 Arc_MfcArray CArray<VAR, const VAR&> 때문에 둘다 template 으로 변경
//dwk: 2025-12-04 13:25 
//dwk: 2025-12-09 17:18 map val, 2D vector
//dwk: 2025-12-10 13:10 UcJXBase.h 제거
//dwk: 2025-12-19 16:03 CStringArray 추가 체크 리스트에만 누락
