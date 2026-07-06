#pragma once



#include "UcBaseTools.h"







#define MUTIPLELEN(len, mul)  ((len) % mul == 0 ? ((len)/mul)*mul : (((len)/mul)+1)*mul)
#define MUTIPLELEN8(len)  MUTIPLELEN(len, 8)

class UCTOOLDYNAMIC KBinary
{
public:

	~KBinary();
	explicit KBinary(LPCWSTR ps)//, size_t len = -1, int iVer = 0) 
	{
		Set((LPWSTR)ps);//len, 
	}
	explicit KBinary(CStringA& sa)//, size_t len = -1, int iVer = 0) 
	{
		Set(const_cast<char*>(sa.GetString()));//const_cast 는  상수성을 제거하는 데 사용
	}

	// 그대로 복사 하더라도 함수 파라 미터로 넘어 가더라도 부른쪽이 소유권을 가지고 있어야 한다.
	// 그렇지 않으면 양쪽 다 소유권을 가지게 되고 불린 함수가 리턴할때 free 해버린다. 그러면 부른쪽에서는 쓸수 없게 된다.
	KBinary(const KBinary& bin) // explicit 하면 안되
	{
		Clone(&bin);
	}

	// 그냥 할당만 할때
	explicit KBinary(UINT_PTR len, int iVer = 0)
		: m_bOwner(true)
		, m_len(len)
	{
		m_p = Alloc(len);
	}
	KBinary()
		: m_bOwner(false)
		, m_len(0)
		, m_p(nullptr)
	{
	}

	bool m_bOwner{ true };
	char* m_p{ NULL };//문자열아니다. 내부 내용이 WCHAR일수도 CHAR일수도 있다.
	// byte수 문자열길이가 아니다.(UNICODE인경우 달라져)
	//DWORD 
	UINT_PTR m_len{ 0 };
	UINT_PTR m_capa{ 0 };
	UINT m_multipleSize{ 8 };

	UINT_PTR Size() { return m_len; }
	UINT_PTR Capa() { return m_capa; }
	int m_version{ 0 };

private:
	CHAR* NewAlloc(UINT_PTR len);

public:
	// 	void Set(LPCWSTR ps);
	// 	void Set(LPCWSTR ps, int iVer);
	template<typename TPSTR>
	void Set(TPSTR ps, int iVer)
	{
		Set(ps);
		SetVer(iVer);
	}

	template<typename Tchar>
	void Set(Tchar* ps)
	{
		ASSERT(m_bOwner);
		ASSERT(m_len >= 0);
		//	ASSERT(len < 100000000); //100메가 짜리가 있나? 있다면 그때 보지
		size_t l0 = ps == NULL ? 0 : tchlen(ps);

		UINT_PTR len = (DWORD)((l0 == 0) ? 0 : (l0 + 1) * sizeof(Tchar));


		// 길이가 같거나, 
		if (len == m_len || (m_len > len && (m_len <= 100 || (m_len - len) <= 20)))
		{
		}
		else
		{
			if (m_len > 0)
				Free();
			ASSERT(len >= 0);
			m_p = Alloc(len);//new CHAR[m_len];//		memcpy(m_p, ps, m_len);
		}

		//if (len > 0)
		//	tchcpy((Tchar*)m_p, ps);//(TCHAR)'\0' 까지 복사
		//else if (m_len > 0)
		//	*((Tchar*)m_p) = (Tchar)0;
		//static_cast error C2440: 'static_cast': cannot convert from 'char *' to 'WCHAR *'
		if (len > 0)
			tchcpy(reinterpret_cast<Tchar*>(m_p), reinterpret_cast<Tchar*>(ps));  // 수정된 부분: 안전한 캐스팅 사용
		else if (m_len > 0)
			*reinterpret_cast<Tchar*>(m_p) = static_cast<Tchar>(0);  // 수정된 부분: 안전한 캐스팅 사용

		m_len = len;
	}






	operator LPCTSTR() const { return (LPCTSTR)m_p; }

	void Free();

	CHAR* Detach(UINT_PTR* plen = nullptr)
	{
		m_bOwner = false;
		if (plen)
			*plen = m_len;
		return m_p;
	}

	//샘플
// 	KBinary binr;
// 	pak->m_binr.Detach();//소유권포기
// 	binr.Attach(pak->m_binr.m_p, pak->m_binr.m_len);//소유권이전
	void Attach(KBinary& bin)
	{
		//ASSERT(bin.m_bOwner == false); // bin.Detach() 먼저 한 후 넣어 줘야...
		Free();

		m_p = bin.m_p;
		m_len = bin.m_len;
		m_capa = bin.m_capa;
		if (m_len > 0)
			m_bOwner = true;

		bin.m_bOwner = false;//이거 강제로 라도 해줘야지 안하면 심각한 메모리 버그...
		bin.Free();
	}

	void Attach(LPSTR p, UINT_PTR len)
	{
		ASSERT(p);
		ASSERT(len >= 0);
		Free();

		if (len > 0)
		{
			m_p = p;
			m_len = len;
			m_capa = len;
			m_bOwner = true;
		}
	}
	CHAR* Alloc(UINT_PTR len, bool bZeroFill = false, int valset = 0);

	// iVer = -1 이든 아니든 무조건 값 넣는다.
	void SetPtr(LPCSTR ps, INT_PTR len, int iVer = -1);

	void Wrap(const KBinary& bin);

	// owner 없이 껍질만 사용, Free하지 않는다.
	void Wrap(LPCSTR ps, UINT_PTR len, int iVer = -1);


	// 현재 할당된 memory에 Set character 한다.
	void SetCh(int idx, char ch) // 반드시 char이어야 한다. m_p가 byte 단위 이므로
	{
		ASSERT(idx < (int)m_len - 1);
		m_p[idx] = ch;
	}

	template<typename TYPECHAR>
	bool CheckNullTermT(TYPECHAR)
	{
		typedef TYPECHAR* LTYPESTR;
		if (m_len == 0)
			return true;
		LTYPESTR pt = reinterpret_cast<LTYPESTR>(m_p);
		for (DWORD i = 0; i <= (m_len / sizeof(TYPECHAR)); i++, pt++) //?주의: <= m_len 은 실제 할당은 '\0' 만큼 더 할당 되어있어야 한다.
		{
			if (*pt == '\0')
				return true;
		}
		return false;
	}

	bool CheckNullTerm();
	bool CheckNullTermA();
	bool CheckNullTermW();

	CStringW& GetStr(CStringW& sBuf);

	LPCSTR GetPtr(UINT_PTR& len)
	{
		len = m_len;
		return (LPCSTR)m_p;
	}

	CStringW GetS()
	{
		CStringW s = (LPCWSTR)m_p;
		return s;
	}
	LPCTSTR GetP()
	{
		ASSERT(CheckNullTerm());
		return (LPCTSTR)m_p;
	}
	LPCSTR GetPA()
	{
		ASSERT(CheckNullTermA());
		return (LPCSTR)m_p;
	}
	LPCWSTR GetPW()
	{
		ASSERT(CheckNullTermW());
		return (LPCWSTR)m_p;
	}

	ULONGLONG GetUL();
	__int64 GetI64();
	DWORD GetDW();
	UINT GetU();
	int GetI();
	int GetId(int idef = 0);
	double GetD();
	CTime GetT();

	void _SerializeVer(CArchive& arc, int iOp = 0);
	void Clone(const KBinary* pSrc0, int iOp = 0);
	bool IsSame(KBinary* pSrc, int iOp = 0);
	CHAR* ReAlloc(UINT_PTR len);
	CHAR* Resize(UINT_PTR len, bool bZeroFill = false);

	virtual int _Upgrade()//int verInit = -1)
	{
		return ++m_version;
	}
	// 상위 데이터 에서 복사해 오거나 참조한 경우 그것과 같이 해준다.
	void SetVer(int verUp)
	{
		m_version = verUp;
	}

};

// old name : CKArchive
class CArchiveBase
{
public:
	int  m_nMode;
public:
	CArchiveBase(bool nMode = store)
		: m_nMode(nMode)
	{

	}
public:
	enum Mode { store = 0, load = 1, bNoFlushOnDelete = 2, bNoByteSwap = 4 };

	BOOL IsLoading() const { return m_nMode == load; }
	BOOL IsStoring() const { return m_nMode == store; }
	virtual void Flush() {} // necessary in file
	virtual void Close() {} // necessary in file
	virtual UINT Read(void* lpBuf, UINT nMax) = NULL;
	virtual void Write(const void* lpBuf, UINT nMax) = NULL;


	template<typename T>
	CArchiveBase& WriteVal(T l)
	{
		Write(&l, sizeof(T));
		return *this;
	}

	template<typename T>
	CArchiveBase& ReadVal(T& l)
	{
		Read((void*)&l, sizeof(T));
		return *this;
	}
	CArchiveBase& operator<<(short  l) { return WriteVal(l); }
	CArchiveBase& operator<<(DWORD  l) { return WriteVal(l); }
	CArchiveBase& operator<<(WORD   l) { return WriteVal(l); }
	CArchiveBase& operator<<(int    l) { return WriteVal(l); }
	CArchiveBase& operator<<(UINT   l) { return WriteVal(l); }
	CArchiveBase& operator<<(float  l) { return WriteVal(l); }
	CArchiveBase& operator<<(double l) { return WriteVal(l); }
	CArchiveBase& operator<<(LONGLONG l) { return WriteVal(l); }
	CArchiveBase& operator<<(ULONGLONG l) { return WriteVal(l); }

	CArchiveBase& operator>>(short& l) { return ReadVal(l); }
	CArchiveBase& operator>>(DWORD& l) { return ReadVal(l); }
	CArchiveBase& operator>>(WORD& l) { return ReadVal(l); }
	CArchiveBase& operator>>(int& l) { return ReadVal(l); }
	CArchiveBase& operator>>(UINT& l) { return ReadVal(l); }
	CArchiveBase& operator>>(float& l) { return ReadVal(l); }
	CArchiveBase& operator>>(double& l) { return ReadVal(l); }
	CArchiveBase& operator>>(LONGLONG& l) { return ReadVal(l); }
	CArchiveBase& operator>>(ULONGLONG& l) { return ReadVal(l); }


	//void WriteCount(DWORD_PTR dwCount);

	DWORD_PTR ReadCount();
};


class CFileArchive
	: public CArchiveBase
{
public:
	FILE* m_file{};
public:
	CFileArchive(bool nMode = store)
		: CArchiveBase(nMode)
	{
	}
	// file Open Close는 SetFile을 부른 곳에서 처리 한다.
	void SetFile(FILE* file)
	{
		ASSERT(m_file == NULL);
		m_file = file;
	}
	virtual UINT Read(void* lpBuf, UINT nMax);
	virtual void Write(const void* lpBuf, UINT nMax);
};

class CBufArchive
	: public CArchiveBase
{
public:
	CBufArchive(bool nMode = store)
		: CArchiveBase(nMode)
		, m_bUserBuf(true) // memmory or file
		, m_lpBufStart(NULL)
		, m_lpBufCur(NULL)
		, m_nBufSize(0)
		//, m_nGrowSize(1024)
	{
	}
	~CBufArchive()
	{
		Free();
	}
	void Free()
	{
		DeleteMeSafe(m_lpBufStart);
		m_lpBufCur = m_lpBufStart = NULL;
		m_nBufSize = 0;
	}
	virtual UINT Read(void* lpBuf, UINT nMax);
	virtual void Write(const void* lpBuf, UINT nMax);

	bool m_bUserBuf;
	UINT_PTR m_nBufSize;
	BYTE* m_lpBufCur;
	BYTE* m_lpBufStart;
	// advanced parameters (controls performance with large archives)
	//UINT m_nGrowSize; 2배씩 불어 나도록 수정 2020-10-25

	// Write와 상관 없이 Alloc(uSize) 한 size
	UINT_PTR GetBufSize()
	{
		return m_nBufSize;
	}

	// 실제 Write 한 끝 위치
	UINT_PTR GetLength()
	{
		return GetCurPos();
	}

	UINT_PTR GetCurPos()
	{
		return (int)(m_lpBufCur - m_lpBufStart);
	}

	void Terminate()
	{
		// 항상 크기가 4byte는 여유 있으므로 뒤에 2byte는 0으로 채운다.
		if (m_lpBufCur)
		{
			m_lpBufCur[0] = '\0';
			m_lpBufCur[1] = '\0';
		}
	}

	BYTE* GetPtr()
	{
		return m_lpBufStart;
	}

	BYTE* Alloc(UINT_PTR uSize);

	BYTE* Detach()
	{
		BYTE* p = GetPtr();
		m_lpBufCur = m_lpBufStart = NULL;
		m_nBufSize = 0;
		return p;
	}

};
