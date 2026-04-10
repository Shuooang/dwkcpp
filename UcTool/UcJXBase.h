#pragma once
//dwk: 2025-12-17 12:30 this file is only to include for header file.
//#include "UcTool\UcJXBase.h"//#DocSerialize
#include <vector>
#include <string>
class CArchive;

class IDocSerialize {
public:
	virtual ~IDocSerialize() {}

	/// <summary>
	/// class명을 문자열로 리턴
	/// </summary>
	/// <returns></returns>
	virtual const wchar_t* GetMyClassName() = 0;

	/// <summary>
	/// 객체의 필드를 XML 또는 JSON 문자열로 저장 하거나,
	/// XML 또는 JSON 문자열로 부터 객체의 필드로 적용 한다.
	/// </summary>
	/// <param name="ard">CJXArchive& _jdata가 중간 데이터</param>
	virtual void DocSerialize(CArchive& ard) = 0;//dwk: 2026-02-05 10:31


	/// <summary>
	/// MyClass에 Serialize 하나만 구현하면,
	///		CObject와 IDocSerialize의 Serialize를 동시에 override하며
	///		어떤 base 포인터로 호출해도 MyClass::Serialize가 실행된다.
	/// </summary>
	/// <param name="ar">CJXArchive& _jdata가 중간 데이터</param>
	virtual void Serialize(CArchive& ar) = 0;

#ifdef _DEBUG
	/// Serialize 호출 추적용 플래그
	mutable int m_nSerializeCallCount = 0;  // Serialize 호출 횟수
	mutable int m_nDocSerializeCallCount = 0; // DocSerialize 호출 횟수
	std::vector<std::string> _arSrl;
	std::vector<std::string> _arDoc;
	void IncSerializeCount(const char* sFunc) {
		++m_nSerializeCallCount;
		_arSrl.push_back(sFunc);
	}
	void IncDocSrlCount(const char* sFunc) {
		++m_nDocSerializeCallCount;
		_arDoc.push_back(sFunc);
	}
#endif // _DEBUG
};

#include "UcJXDeclare.inl"//#DocSerialize i
//dwk: 2025-12-22 16:52 m_nDocSerializeCallCount
//dwk: 2026-01-06 11:09 Serialize 추가
//dwk: 2026-01-06 12:28 class CArchive;