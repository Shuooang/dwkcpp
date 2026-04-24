#include "pch.h"
#include "UcJXDoc.h"
#include "UcBaseTools.h"

#include <map>
#include <memory>
#include <functional>

UCTOOLDYNAMIC std::shared_ptr<std::map<std::wstring, std::function<void* ()>>> GetClassFactory()
{
	static const auto s_pFactory = std::make_shared<std::map<std::wstring, std::function<void* ()>>>();
	return s_pFactory;
}

SET_FACTORY(Matrix_Temp);
//DWKREMINDER("Matrix_Temp 는 lib에 있으므로 export 하지 않는다. SET_FACTORY_NOEXPORT")
//IMPLEMENT_DYNAMIC(CJXArchive, CArchive)

void Matrix_Temp::DocSerialize(CArchive& ard)
{
	//#DocSerialize 2
	ar_from_ard_ReadyForDocSerialize(ard);//dwk: 2025-02-12 09:50 JSON ar Serialize
	Arc_StdVect2D_Val(_data);
}
//dwk: 2025-12-15 12:11 DECLARE_ClassName_DocSerialize

void CJXArchive::SaveToFile(const CStringA& sData)
{
	if (_bFinished)
		return;
	DWKFUNC;
	try
	{
		if (IsRoot())
		{
			CStringA sData = this->GetDataString();
			if (_bBOM) {
				// UTF-8 BOM 추가
				const char utf8BOM[] = { (char)0xEF, (char)0xBB, (char)0xBF };
				CJXArchive::Write(utf8BOM, 3);
			}
			CJXArchive::Write(sData.GetString(), sData.GetLength());  // 파일 전체 저장
		}
		else if (_key.length() > 0)/// sub jbj는 저장 하지 않고 상위 Jbj에 내 키로 붙인다.
		{
			ASSERT(0);
			///주의: 객체 인 경우만 saveload가 불려 지므로 여기서 부르면 안된다.
			//this->SaveToUpperNode();//주의: ~CSaveLoad -> SaveToFile 에서 한다.
		}
		_bFinished = true;
	}
	catch (CException*) {
		ASSERT(0);
	}
	catch (...) {}
}

void CJXArchive::LoadFromFile(LPCTSTR lpszPathName)
{
	DWKFUNC;
	if (_bFinished)
		return;
	try
	{
		if (IsRoot())
		{
			const CFile* pFile = CArchive::GetFile();
			auto len = pFile->GetLength();
			CStringA sJson;
			auto lpBuffer = sJson.GetBuffer((int)len);
			CJXArchive::Read(lpBuffer, (UINT)len);
			sJson.ReleaseBuffer();
			// UTF-8 BOM 체크 및 _bBOM 플래그 설정
			if (len >= 3 &&
				(unsigned char)lpBuffer[0] == 0xEF &&
				(unsigned char)lpBuffer[1] == 0xBB &&
				(unsigned char)lpBuffer[2] == 0xBF) {
				// BOM이 있으면 플래그 설정하고 3바이트 건너뛰기
				_bBOM = true;
				sJson = sJson.Mid(3);//Read를 두번 하는 거 보다 이게 낫지.
			}
			else {
				// BOM이 없으면 플래그 해제
				_bBOM = false;
			}
			ShJVal shDoc;
			if (this->_sMode == "JSON")
				shDoc = UcJson::ParseUtf8(sJson);
			else if (_sMode == "XML") {
				shDoc = UcJson::ParseXml(sJson);
				//CStringW swJson;
				//UcUTF8ToWchar(sJson, swJson);
				//shDoc = UcJson::ParseXml(swJson);
			}
			ShJVal shRoot;
			if (shDoc) {
				if (_afterLoad) {
					shRoot = _afterLoad(shDoc);
				}
				else {
					auto& doc = *shDoc->Dic();
					shRoot = doc.O("root");// 루트를 번겨 낸다.
				}
				SetJson(shRoot);//메인 데이터 설정
			}
		}
		else if (_key.length() > 0) {/// level > 0 이면(sub obj) 상위 _pArUp에서 내 키로 jbj를 가져 온다.
			ASSERT(_pArUp);
			//if (this->IsKindOf(RUNTIME_CLASS(CJXArchive)))
			if (typeid(*this) == typeid(CJXArchive)) {
				auto pJbj = static_cast<CJXArchive*>(_pArUp);
				auto& jbjUp = pJbj->Jbj();
				auto shJbj = jbjUp.O(_key);/// 상위 객체에서 _key 로 객체를 빼와서 SetJson한다.
				SetJson(shJbj);
			}
			else {
				ASSERT(0);
			}
		}
		_bFinished = true;
	}
	catch (CException*) {
		ASSERT(0);
	}
	catch (...) {}
}

CJXArchive::CSaveLoad::CSaveLoad(CJXArchive& ar) : _ar(ar)
{
	if (_ar.IsRoot()) {
		if (_ar.IsStoring()) {
			// CJsonSave의 생성자 로직 (저장 준비)
			// 실제 저장은 소멸자에서 처리
		}
		else {
			// CJsonLoad의 생성자 로직 (로딩)
			CString sPath;
			if (_ar.IsRoot()) {
				auto pFile = _ar.GetFile();
				sPath = pFile->GetFilePath();
			}
			_ar.LoadFromFile(sPath);
		}
	}
}

CJXArchive::CSaveLoad::~CSaveLoad()
{
	if (_ar.IsRoot() && _ar.IsStoring()) {
		///_ar.IsRoot() 이거까지 조건으로 넣어 버리니. 저장 외에 SaveToFile 에서 IsRoot 어차피 하잖아.
		// CJsonSave의 소멸자 로직 (_ar.IsRoot()인 경우만 실제 저장)
		//CStringA jsonStr = _ar.GetDataString();//dwk: 2025-11-14 11:01 
		//DWKREMINDER("~CSaveLoad에서 IsRoot인 경우만 저장을 하고, 상위노드에 등록은 직접 한다.");
		_ar.SaveToFile({});
	}
}

#ifdef _DEBUG_noTrack
CSerializeTracker::~CSerializeTracker()
{
	if (m_pObj && m_bDocMode) {
		// DocMode일 때는 반드시 DocSerialize가 호출되어야 함
		if (m_pObj->m_nDocSerializeCallCount == 0 || 
			(m_pObj->m_nDocSerializeCallCount != m_pObj->m_nSerializeCallCount)) {
			DWKFUNCV(L"ERROR: Serialize() called but DocSerialize() was NOT called for class: %v",
				m_pObj->GetMyClassName());
			ASSERT(0 && "DocSerialize() was not called after Serialize()");
		}
		// 다음 호출을 위해 리셋
		//m_pObj->m_nSerializeCallCount = 0;
		//m_pObj->m_nDocSerializeCallCount = 0;
	}
}
#endif // _DEBUG


void CJXArchive::Write(const void* lpBuf, UINT nMax)
{
	ASSERT_VALID(m_pFile);

	if (nMax == 0)
		return;

	ASSERT(lpBuf != NULL);

	if (lpBuf == NULL)
		return;

	ASSERT(AfxIsValidAddress(lpBuf, nMax, FALSE));  // read-only access needed
	ASSERT(m_bDirectBuffer || m_lpBufStart != NULL);
	ASSERT(m_bDirectBuffer || m_lpBufCur != NULL);
	ASSERT(m_lpBufStart == NULL ||
		AfxIsValidAddress(m_lpBufStart, UINT(m_lpBufMax - m_lpBufStart)));
	ASSERT(m_lpBufCur == NULL ||
		AfxIsValidAddress(m_lpBufCur, UINT(m_lpBufMax - m_lpBufCur)));
	ASSERT(IsStoring());

	if (!IsStoring())
		AfxThrowArchiveException(CArchiveException::readOnly, m_strFileName);

	// copy to buffer if possible
	UINT nTemp = min(nMax, (UINT)(m_lpBufMax - m_lpBufCur));
	Checked::memcpy_s(m_lpBufCur, (size_t)(m_lpBufMax - m_lpBufCur), lpBuf, nTemp);
	m_lpBufCur += nTemp;
	lpBuf = (BYTE*)lpBuf + nTemp;
	nMax -= nTemp;

	if (nMax > 0)
	{
		Flush();    // flush the full buffer

		// write rest of buffer size chunks
		nTemp = nMax - (nMax % m_nBufSize);
		m_pFile->Write(lpBuf, nTemp);
		DWKFUNCV(L"Write(%v)", nTemp);
		lpBuf = (BYTE*)lpBuf + nTemp;
		nMax -= nTemp;

		if (m_bDirectBuffer)
		{
			// sync up direct mode buffer to new file position
			VERIFY(m_pFile->GetBufferPtr(CFile::bufferWrite, m_nBufSize,
				(void**)&m_lpBufStart, (void**)&m_lpBufMax) == (UINT)m_nBufSize);
			ASSERT((UINT)m_nBufSize == (UINT)(m_lpBufMax - m_lpBufStart));
			m_lpBufCur = m_lpBufStart;
		}

		// copy remaining to active buffer
		ENSURE(nMax < (UINT)m_nBufSize);
		ENSURE(m_lpBufCur == m_lpBufStart);
		Checked::memcpy_s(m_lpBufCur, nMax, lpBuf, nMax);
		m_lpBufCur += nMax;
	}
}

void CJXArchive::Flush()
{
	ASSERT_VALID(m_pFile);
	ASSERT(m_bDirectBuffer || m_lpBufStart != NULL);
	ASSERT(m_bDirectBuffer || m_lpBufCur != NULL);
	ASSERT(m_lpBufStart == NULL ||
		AfxIsValidAddress(m_lpBufStart, UINT(m_lpBufMax - m_lpBufStart), IsStoring()));
	ASSERT(m_lpBufCur == NULL ||
		AfxIsValidAddress(m_lpBufCur, UINT(m_lpBufMax - m_lpBufCur), IsStoring()));

	if (IsLoading())
	{
		// unget the characters in the buffer, seek back unused amount
		if (m_lpBufMax != m_lpBufCur)
			m_pFile->Seek(-(int(m_lpBufMax - m_lpBufCur)), CFile::current);
		m_lpBufCur = m_lpBufMax;    // empty
	}
	else
	{
		if (!m_bDirectBuffer)
		{
			// write out the current buffer to file
			auto ulen = ULONG(m_lpBufCur - m_lpBufStart);
			if (m_lpBufCur != m_lpBufStart){
				m_pFile->Write(m_lpBufStart, ulen);
				DWKFUNCV(L"Write(%v)", ulen);
			}
		}
		else
		{
			// commit current buffer
			if (m_lpBufCur != m_lpBufStart)
				m_pFile->GetBufferPtr(CFile::bufferCommit, ULONG(m_lpBufCur - m_lpBufStart));
			// get next buffer
			VERIFY(m_pFile->GetBufferPtr(CFile::bufferWrite, m_nBufSize,
				(void**)&m_lpBufStart, (void**)&m_lpBufMax) == (UINT)m_nBufSize);
			ASSERT((UINT)m_nBufSize == (UINT)(m_lpBufMax - m_lpBufStart));
		}
		m_lpBufCur = m_lpBufStart;
	}
}
void CJXArchive::Close()
{
	ASSERT_VALID(m_pFile);

	Flush();
	m_pFile = NULL;
}

UINT CJXArchive::Read(void* lpBuf, UINT nMax)
{
	ASSERT_VALID(m_pFile);

	if (nMax == 0)
		return 0;

	ASSERT(lpBuf != NULL);

	if (lpBuf == NULL)
		return 0;

	ASSERT(AfxIsValidAddress(lpBuf, nMax));
	ASSERT(m_bDirectBuffer || m_lpBufStart != NULL);
	ASSERT(m_bDirectBuffer || m_lpBufCur != NULL);
	ASSERT(m_lpBufStart == NULL ||
		AfxIsValidAddress(m_lpBufStart, UINT(m_lpBufMax - m_lpBufStart), FALSE));
	ASSERT(m_lpBufCur == NULL ||
		AfxIsValidAddress(m_lpBufCur, UINT(m_lpBufMax - m_lpBufCur), FALSE));
	ASSERT(IsLoading());

	if (!IsLoading())
		AfxThrowArchiveException(CArchiveException::writeOnly, m_strFileName);

	// try to fill from buffer first
	UINT nMaxTemp = nMax;
	UINT nTemp = min(nMaxTemp, UINT(m_lpBufMax - m_lpBufCur));
	Checked::memcpy_s(lpBuf, nMaxTemp, m_lpBufCur, nTemp);
	m_lpBufCur += nTemp;
	lpBuf = (BYTE*)lpBuf + nTemp;
	nMaxTemp -= nTemp;

	if (nMaxTemp != 0)
	{
		ASSERT(m_lpBufCur == m_lpBufMax);

		// read rest in buffer size chunks
		nTemp = nMaxTemp - (nMaxTemp % m_nBufSize);
		UINT nRead = 0;

		UINT nLeft = nTemp;
		UINT nBytes;
		do
		{
			nBytes = m_pFile->Read(lpBuf, nLeft);
			lpBuf = (BYTE*)lpBuf + nBytes;
			nRead += nBytes;
			nLeft -= nBytes;
		} while ((nBytes > 0) && (nLeft > 0));

		nMaxTemp -= nRead;

		if (nMaxTemp > 0)
		{
			// read last chunk into buffer then copy
			if (nRead == nTemp)
			{
				ASSERT(m_lpBufCur == m_lpBufMax);
				ASSERT(nMaxTemp < UINT(m_nBufSize));

				// fill buffer (similar to CArchive::FillBuffer, but no exception)
				if (!m_bDirectBuffer)
				{
					UINT nLastLeft;
					UINT nLastBytes;

					if (!m_bBlocking)
						nLastLeft = max(nMaxTemp, UINT(m_nBufSize));
					else
						nLastLeft = nMaxTemp;
					BYTE* lpTemp = m_lpBufStart;
					nRead = 0;
					do
					{
						nLastBytes = m_pFile->Read(lpTemp, nLastLeft);
						lpTemp = lpTemp + nLastBytes;
						nRead += nLastBytes;
						nLastLeft -= nLastBytes;
					} while ((nLastBytes > 0) && (nLastLeft > 0) && nRead < nMaxTemp);

					m_lpBufCur = m_lpBufStart;
					m_lpBufMax = m_lpBufStart + nRead;
				}
				else
				{
					nRead = m_pFile->GetBufferPtr(CFile::bufferRead, m_nBufSize,
						(void**)&m_lpBufStart, (void**)&m_lpBufMax);
					ASSERT(nRead == size_t(m_lpBufMax - m_lpBufStart));
					m_lpBufCur = m_lpBufStart;
				}

				// use first part for rest of read
				nTemp = min(nMaxTemp, UINT(m_lpBufMax - m_lpBufCur));
				Checked::memcpy_s(lpBuf, nMaxTemp, m_lpBufCur, nTemp);
				m_lpBufCur += nTemp;
				nMaxTemp -= nTemp;
			}
		}
	}
	return nMax - nMaxTemp;
}
//dwk: 2025-12-22 16:50 CJXArchive::Read, Write, Flush, Close

//dwk: 2025-12-24 18:00 예외 처리 함수들 (빌드 타임 단축을 위해 cpp로 분리)
void CJXArchive::HandleCExceptionInDocSerialize(CException* e, shared_ptr<int>& statck, const char* pszFile, int nLine, const CStringA& saf) {
	CStringA strErr;
	strErr.Format("%s(%d) : DocSerialize - CException in return_If_Doc_Call_DocSerialize - %s\n", pszFile, nLine, saf.GetString());
	OutputDebugStringA(strErr.GetString());
	--(*statck);
	ASSERT(0 && "CException in return_If_Doc_Call_DocSerialize");
	e->Delete();
	throw e;
}
void CJXArchive::HandleStdExceptionInDocSerialize(const std::exception& e, shared_ptr<int>& statck, const char* pszFile, int nLine, const CStringA& saf) {
	CStringA strErr;
	strErr.Format("%s(%d) : DocSerialize - std::exception in return_If_Doc_Call_DocSerialize - e.what(%s) %s\n", pszFile, nLine, e.what(), saf.GetString());
	OutputDebugStringA(strErr.GetString());
	--(*statck);
	ASSERT(0 && "std::exception in return_If_Doc_Call_DocSerialize");
	throw e;
}
void CJXArchive::HandleUnknownExceptionInDocSerialize(shared_ptr<int>& statck, const char* pszFile, int nLine, const CStringA& saf) {
	CStringA strErr;
	strErr.Format("%s(%d) : DocSerialize - unknown exception in return_If_Doc_Call_DocSerialize - %s\n", pszFile, nLine, saf.GetString());
	OutputDebugStringA(strErr.GetString());
	--(*statck);
	ASSERT(0 && "unknown exception in return_If_Doc_Call_DocSerialize");
}

//dwk: 2025-12-29 11:18 catch 핸들링 함수 cpp로 이동
/// \brief 파일 풀 경로에서 확장자가 baseExt+"x"이면 "XML", baseExt+"j"이면 "JSON"을 반환.
/// \param fullPath  파일 풀 경로 (예: "C:\\path\\file.ecmx")
/// \param baseExt   확장자 공통 부분 (예: "ecm" -> .ecmx -> XML, .ecmj -> JSON)
/// \return "XML", "JSON", 또는 해당 없으면 ""
std::string GetFormatByExtension(LPCTSTR fullPath, LPCTSTR baseExt /*= _T("ecm")*/)
{
		//using std_tstring = tstring<TCHAR>;// std::basic_string<TCHAR>;
	tstring<TCHAR> path(fullPath);//UcBaseTools.h
	tstring<TCHAR> base(baseExt);

	const size_t lastDot = path.find_last_of(_T('.'));
	if (lastDot == tstring<TCHAR>::npos || lastDot == path.length() - 1)
		return "";

	auto ext = path.substr(lastDot + 1);
	// 소문자 통일
	//DWKREMINDER("ucstd:: 에 MakeStrLower 추가");
	ucstd::MakeStrLower(ext);
	ucstd::MakeStrLower(base);

	if (ext == base + _T("x"))
		return "XML";
	if (ext == base + _T("j"))
		return "JSON";
	return "";
}
