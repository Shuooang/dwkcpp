#pragma once

/// ///////////////////////////////////////////////
/// 여기에는 DocSerialize를 위한 공통 요소가 들어간다.
/// ///////////////////////////////////////////////

#ifndef DECLARE_ClassName_DocSerialize
#define DECLARE_ClassName_DocSerialize() \
public: \
	virtual void DocSerialize(CArchive& ard); \
	const wchar_t* GetMyClassName() override;\
	using IDocSerialize::Serialize;
	//bool IsNoneVirtual() { return false;}

///주의: 대행저장 하는 경우 virtual이 끼면 사이즈가 달라져서 값이 밀린다.
///see: svm_model_tmp : public svm_model
//#define DECLARE_ClassName_DocSerializeNoneVr() \
//	void DocSerialize(CArchive& ard); \
//	const wchar_t* GetMyClassName();\
//	bool IsNoneVirtual() { return true; }

#endif //DECLARE_ClassName_DocSerialize

///[deprecated] _class_ export하는데 애 먹어서 없애 버림//dwk: 2025-12-16 10:36 
//static std::wstring _class_; \
//const { return _class_; }\

//dwk: 2025-12-15 12:11 DECLARE_ClassName_DocSerialize
//dwk: 2026-01-06 12:26 using IDocSerialize::Serialize