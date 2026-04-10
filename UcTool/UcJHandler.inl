#define ListGEN(TLIST, TYPE, ASVAL) { typeid(TLIST<TYPE>), [](UcJObj& j1, wstring k2, void* v3) {\
			auto& sarr = *((TLIST<TYPE>*)v3);sarr.clear();\
			if (j1.Has(k2)) {\
				auto& jarr = j1.GetArray(k2.c_str()); \
				for (auto& jv : jarr) \
					sarr.push_back(jv->Val()->ASVAL());\
		}}}
#define VECTORGEN(TYPE, ASVAL) ListGEN(std::vector, TYPE, ASVAL)
#define KArrayGEN(TYPE, ASVAL) ListGEN(KArray, TYPE, ASVAL)
#define KListGEN(TYPE, ASVAL) ListGEN(KList, TYPE, ASVAL)
#define STDListGEN(TYPE, ASVAL) ListGEN(std::list, TYPE, ASVAL)
#define MapGEN(TMAP, TKEY, TYPE, ASVAL) \
		{	typeid(TMAP<TKEY,TYPE>), [](UcJObj& j1, wstring k2, void* v3) {\
			auto& smap = *((TMAP<TKEY,TYPE>*)v3);smap.clear();\
			auto shObj = j1.O(k2.c_str());\
			if (shObj) {\
				auto& jbj = *shObj->Dic();\
				for (auto& kjv : jbj){\
					auto& k = kjv.first;\
					auto& jv = kjv.second;\
					smap[k] = jv->Val()->ASVAL();}\
		}}}
#define KStdMapGEN(TKEY, TYPE, ASVAL) MapGEN(KStdMap, TKEY, TYPE, ASVAL)
#define STDMAPGEN(TKEY, TYPE, ASVAL) MapGEN(std::map, TKEY, TYPE, ASVAL)

#if CPP17_OR_LATER // C++17부터는 inline 변수 지원. 이 부분은 헤더파일에 넣어도 된다.
EXTERN_STATIC
#endif
std::unordered_map<std::type_index, std::function<void(UcJObj&, wstring, void*)>> handlers_ = {
	{ typeid(USHORT        ), [](UcJObj& j1, wstring k2, void* v3) { *((USHORT        *)v3) = (USHORT     )j1.I   (k2);}},
	{ typeid(UINT          ), [](UcJObj& j1, wstring k2, void *v3) { *((UINT          *)v3) = (ULONG      )j1.I64 (k2);}},
	{ typeid(ULONG         ), [](UcJObj& j1, wstring k2, void *v3) { *((ULONG         *)v3) = (ULONG      )j1.I64 (k2);}},
	{ typeid(ULONGLONG     ), [](UcJObj& j1, wstring k2, void *v3) { *((ULONGLONG     *)v3) = (ULONGLONG  )j1.I64 (k2);}},
	{ typeid(short         ), [](UcJObj& j1, wstring k2, void *v3) { *((short         *)v3) = (short      )j1.I   (k2);}},
	{ typeid(int           ), [](UcJObj& j1, wstring k2, void *v3) { *((int           *)v3) = (int        )j1.I   (k2);}}, // unnamed-enum도 int
	{ typeid(WORD          ), [](UcJObj& j1, wstring k2, void *v3) { *((WORD          *)v3) = (WORD       )j1.I   (k2);}}, // unnamed-enum도 int
	{ typeid(BYTE          ), [](UcJObj& j1, wstring k2, void *v3) { *((BYTE          *)v3) = (BYTE       )j1.I   (k2);}}, // unnamed-enum도 int
	{ typeid(long          ), [](UcJObj& j1, wstring k2, void *v3) { *((long          *)v3) = (long       )j1.I   (k2);}},
	{ typeid(__int64       ), [](UcJObj& j1, wstring k2, void *v3) { *((__int64       *)v3) = (__int64    )j1.I64 (k2);}},
	{ typeid(UINT          ), [](UcJObj& j1, wstring k2, void *v3) { *((UINT          *)v3) = (UINT       )j1.I64 (k2);}},
	{ typeid(float         ), [](UcJObj& j1, wstring k2, void *v3) { *((float         *)v3) = (float      )j1.D   (k2);}},
	{ typeid(double        ), [](UcJObj& j1, wstring k2, void *v3) { *((double        *)v3) = (double     )j1.D   (k2);}},
	{ typeid(bool          ), [](UcJObj& j1, wstring k2, void *v3) { *((bool          *)v3) = (bool       )j1.b   (k2);}},
	{ typeid(CStringW      ), [](UcJObj& j1, wstring k2, void *v3) { *((CStringW      *)v3) = (CStringW   )j1.S   (k2);}},
	{ typeid(CStringA      ), [](UcJObj& j1, wstring k2, void *v3) { *((CStringA      *)v3) = (CStringA   )j1.SA  (k2);}},
	{ typeid(wstring       ), [](UcJObj& j1, wstring k2, void *v3) { *((wstring       *)v3) = (wstring    )j1.wstr(k2);}},
	{ typeid(std::string   ), [](UcJObj& j1, wstring k2, void *v3) { *((std::string   *)v3) = (std::string)j1.str (k2);}},

	///CArray<int,int> 처럼, 두개씩 들어가는 거에 주의
#define MfcArrayGEN(TLIST, TYPE, ASVAL) \
		{ typeid(TLIST<TYPE,TYPE>), [](UcJObj& j1, wstring k2, void* v3) {\
			auto& sarr = *((TLIST<TYPE,TYPE>*)v3);sarr.RemoveAll();\
			if (j1.Has(k2)) {\
				auto& jarr = j1.GetArray(k2.c_str()); \
				for (auto& jv : jarr) \
					sarr.Add(jv->Val()->ASVAL());\
		}}}
#define CArrayGEN(TYPE, ASVAL) MfcArrayGEN(CArray, TYPE, ASVAL)
	CArrayGEN(CStringW, S),
	CArrayGEN(CStringA, SA),
	CArrayGEN(int, I),
	CArrayGEN(double, D),
	//{ typeid(CArray<CStringW>), [](UcJObj& j1, wstring k2, void* v3) {
	//	auto& sarr = *((CArray<CStringW>*)v3); sarr.RemoveAll();
	//	auto& jarr = j1.GetArray(k2.c_str());
	//	for (auto& jv : jarr)
	//		sarr.Add(jv->Val()->S());
	//}},
	//{ typeid(CArray<CStringA>), [](UcJObj& j1, wstring k2, void* v3) {
	//	auto& sarr = *((CArray<CStringA>*)v3); sarr.RemoveAll();
	//	auto& jarr = j1.GetArray(k2.c_str());
	//	for (auto& jv : jarr)
	//		sarr.Add(jv->Val()->SA());
	//}},
	//{ typeid(CArray<int>), [](UcJObj& j1, wstring k2, void* v3) {
	//	auto& sarr = *((CArray<int>*)v3); sarr.RemoveAll();
	//	auto& jarr = j1.GetArray(k2.c_str());
	//	for (auto& jv : jarr)
	//		sarr.Add(jv->Val()->I());
	//}},

	{ typeid(CStringArray), [](UcJObj& j1, wstring k2, void* v3) {
		auto& sarr = *((CStringArray*)v3); sarr.RemoveAll();
		auto& jarr = j1.GetArray(k2.c_str());
		for (auto& jv : jarr)
#ifdef _MBCS
			sarr.Add(jv->Val()->SA());
#else
			sarr.Add(jv->Val()->S());
#endif // _MBCS
	}},

	{ typeid(CStringList), [](UcJObj& j1, wstring k2, void* v3) {
		auto& slist = *((CStringList*)v3); slist.RemoveAll();
		if (j1.Has(k2)) {
			auto& jarr = j1.GetArray(k2.c_str());
			for (auto& jv : jarr)
#ifdef _MBCS
				slist.AddTail(jv->Val()->SA());
#else
				slist.AddTail(jv->Val()->S());
#endif // _MBCS
		}}},

	VECTORGEN(std::wstring, AsString),
	VECTORGEN(std::string, AsStringA),
	VECTORGEN(CStringW    , S),
	VECTORGEN(CStringA    , SA),
	VECTORGEN(int         , AsInt),
#ifdef _DEBUG
	{
		typeid(std::vector<double>), [](UcJObj& j1, wstring k2, void* v3) {
			auto& sarr = *((std::vector<double>*)v3); sarr.clear();
			if (j1.Has(k2)) {
					auto& jarr = j1.GetArray(k2.c_str());
					for (auto& jv : jarr)
						sarr.push_back(jv->Val()->AsDouble());
			}}
},
#else
	VECTORGEN(double      , AsDouble),
	//ListGEN(std::vector, double, AsDouble)
#endif // _DEBUG
	VECTORGEN(INT64       , AsInt64),
	VECTORGEN(size_t      , AsInt64),

	KArrayGEN(std::wstring, AsString),
	KArrayGEN(CStringW    , S),
	KArrayGEN(CStringA    , SA),
	KArrayGEN(int         , AsInt),
	KArrayGEN(double      , AsDouble),
	KArrayGEN(INT64       , AsInt64),

	//KListGEN(std::wstring, AsString),
		{ typeid(KList<std::wstring>), [](UcJObj& j1, wstring k2, void* v3) {
		auto& sarr = *((KList<std::wstring>*)v3); sarr.clear();
		if (j1.Has(k2)) {
			auto& jarr = j1.GetArray(k2.c_str());
			for (auto& jv : jarr)
				sarr.push_back(jv->Val()->AsString());
		}}},

	KListGEN(std::string, AsStringA),
	KListGEN(CStringW    , S),
	//KListGEN(CStringA    , SA),
	//ListGEN(KList, CStringA    , SA)
	{ typeid(KList<CStringA>), [](UcJObj& j1, wstring k2, void* v3) {
		auto& sarr = *((KList<CStringA>*)v3); sarr.clear();
		if (j1.Has(k2)) {
			auto& jarr = j1.GetArray(k2.c_str());
			for (auto& jv : jarr)
				sarr.push_back(jv->Val()->SA());
		}}},
	KListGEN(int         , AsInt),
	KListGEN(double      , AsDouble),
	KListGEN(INT64       , AsInt64),

	STDListGEN(std::wstring, AsString),
	STDListGEN(CStringW, S),

	//ListGEN(TLIST, TYPE, ASVAL)
	{ typeid(std::list<CStringA>), [](UcJObj& j1, wstring k2, void* v3) {
			auto& sarr = *((std::list<CStringA>*)v3); sarr.clear();
			auto& jarr = j1.GetArray(k2.c_str());
			for (auto& jv : jarr)
				sarr.push_back(jv->Val()->SA());
		}},//#define STDListGEN(CStringA, SA) ListGEN(std::list, CStringA, SA)
	//STDListGEN(CStringA, SA),
	STDListGEN(int, AsInt),
	STDListGEN(double, AsDouble),
	STDListGEN(INT64, AsInt64),
	STDListGEN(size_t, AsInt64),

	KStdMapGEN(std::wstring, std::wstring, AsString),
	KStdMapGEN(std::wstring, CStringW, S),
	KStdMapGEN(std::wstring, CStringA, SA),
	KStdMapGEN(std::wstring, int, AsInt),
	KStdMapGEN(std::wstring, double, AsDouble),
	KStdMapGEN(std::wstring, INT64, AsInt64),

	STDMAPGEN(std::wstring, std::wstring, AsString),
	STDMAPGEN(std::wstring, CStringW, S),
	STDMAPGEN(std::wstring, CStringA, SA),
	STDMAPGEN(std::wstring, int, AsInt),
	STDMAPGEN(std::wstring, double, AsDouble),
	STDMAPGEN(std::wstring, INT64, AsInt64),
	{ typeid(std::map<std::wstring, std::vector<std::wstring>>), [](UcJObj& j1, wstring k2, void* v3) {
		auto& smap = *((std::map<std::wstring, std::vector<std::wstring>>*)v3); smap.clear();
		auto shObj = j1.O(k2.c_str());
		if (shObj && shObj->IsDic()) {
			auto& jbj = *shObj->Dic();
			for (auto& kjv : jbj) {
				auto& k = kjv.first;
				auto& jv = kjv.second;
				if (jv->IsArr()) {
					auto& jarr = *jv->Arr();
					std::vector<std::wstring> vec;
					vec.reserve(jarr.size());
					for (auto& elt : jarr) {
						vec.push_back(elt->Val()->AsString());
					}
					smap[k] = std::move(vec);
				}
				else if (jv->IsDic() && jv->Dic()->Has(TAG_ITM)) {
					auto shArr = jv->Dic()->Get(TAG_ITM);
					if (shArr->IsArr()) {
						auto& jarr = *shArr->Arr();
						std::vector<std::wstring> vec;
						vec.reserve(jarr.size());
						for (auto& elt : jarr) {
							vec.push_back(elt->Val()->AsString());
						}
						smap[k] = std::move(vec);
					}
				}
			}
		}
	}},
	// std::map<std::wstring, std::vector<std::wstring>> 지원 (값이 배열)
	{ typeid(std::map<std::wstring, std::vector<std::wstring>>), [](UcJObj& j1, wstring k2, void* v3) {
		auto& smap = *((std::map<std::wstring, std::vector<std::wstring>>*)v3); smap.clear();
		auto shObj = j1.O(k2.c_str());
		if (shObj && shObj->IsDic()) {
			auto& jbj = *shObj->Dic();
			for (auto& kjv : jbj) {
				auto& k = kjv.first;
				auto& jv = kjv.second;
				if (jv->IsArr()) {
					auto& jarr = *jv->Arr();
					std::vector<std::wstring> vec;
					vec.reserve(jarr.size());
					for (auto& elt : jarr) {
						vec.push_back(elt->Val()->AsString());
					}
					smap[k] = std::move(vec);
				}
				else if (jv->IsDic() && jv->Dic()->Has(TAG_ITM)) {
					auto shArr = jv->Dic()->Get(TAG_ITM);
					if (shArr->IsArr()) {
						auto& jarr = *shArr->Arr();
						std::vector<std::wstring> vec;
						vec.reserve(jarr.size());
						for (auto& elt : jarr) {
							vec.push_back(elt->Val()->AsString());
						}
						smap[k] = std::move(vec);
					}
				}
			}
		}
	}},

};
/// 내부에서만 쓰므로 헤더 파일 영향을 줄이기 위해 undef
#undef ListGEN
#undef VECTORGEN
#undef KArrayGEN
#undef KListGEN
#undef STDListGEN
#undef MapGEN
#undef KStdMapGEN
#undef STDMAPGEN

//dwk: 2025-12-11 16:54 map<k,vector<v>>
//dwk: 2025-12-18 12:44  vect<std::string>
//dwk: 2026-01-05 15:28 handlers_ 에 USHORT, short 추가