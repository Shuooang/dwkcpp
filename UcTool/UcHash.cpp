#include "pch.h"
#include "UcHash.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
#include "UcBaseTools.h"
#include "UcTool.h"


#ifdef _NEVER_USED__
uint32_t rotl32(uint32_t x, int n)
{
	ASSERT(n < 32);  // n이 32 미만임을 보장 cppcheck
	return (x << n) | (x >> (32 - n));
}

uint32_t fFnc(uint32_t a, uint32_t b, uint32_t c)
{
	return (b & c) | (~b & a);
}

uint32_t gFnc(uint32_t a, uint32_t b, uint32_t c)
{
	return (a & b) | (a & c) | (b & c);
}

uint32_t hFnc(uint32_t a, uint32_t b, uint32_t c)
{
	return a ^ b ^ c;
}

uint32_t iFnc(uint32_t a, uint32_t b, uint32_t c)
{
	return b ^ (a | ~c);
}

void MD5(const unsigned char* input, size_t length, unsigned char* output)
{
	uint32_t a0 = 0x67452301;
	uint32_t b0 = 0xEFCDAB89;
	uint32_t c0 = 0x98BADCFE;
	uint32_t d0 = 0x10325476;

	uint32_t a = a0;
	uint32_t b = b0;
	uint32_t c = c0;
	uint32_t d = d0;

	for (size_t i = 0; i < length; i += 64)
	{
		uint32_t m[64]; // for cppcheck, 16 to 64 modifioed
#pragma warning(disable: 26451)
		for (int j = 0; j < 16; j++)
		{
			m[j] = (input[i + j * 4] << 24) | (input[i + j * 4 + 1] << 16) | (input[i + j * 4 + 2] << 8) | (input[i + j * 4 + 3]);
		}
#pragma warning(default: 26451)
		for (int j = 0; j < 64; j++)
		{
			uint32_t f, g, h, i1;

			if (j < 16)
			{
				f = fFnc(b, c, d);
				g = 0;
				h = 0;
				i1 = 0;
			}
			else if (j < 32)
			{
				f = gFnc(b, c, d);
				g = 0;
				h = 0;
				i1 = 0;
			}
			else if (j < 48)
			{
				f = hFnc(b, c, d);
				g = 1;
				h = 0;
				i1 = 0;
			}
			else
			{
				f = iFnc(b, c, d);
				g = 1;
				h = 1;
				i1 = 1;
			}

			uint32_t temp = d;
			d = c;
			c = b;
			b = rotl32(a + f + m[j] + g, j);
			a = temp;
		}

		a += a0;
		b += b0;
		c += c0;
		d += d0;
	}

	output[0] = (a >> 24) & 0xFF;
	output[1] = (a >> 16) & 0xFF;
	output[2] = (a >> 8) & 0xFF;
	output[3] = a & 0xFF;

	output[4] = (b >> 24) & 0xFF;
	output[5] = (b >> 16) & 0xFF;
	output[6] = (b >> 8) & 0xFF;
	output[7] = b & 0xFF;

	output[8] = (c >> 24) & 0xFF;
	output[9] = (c >> 16) & 0xFF;
	output[10] = (c >> 8) & 0xFF;
	output[11] = c & 0xFF;

	output[12] = (d >> 24) & 0xFF;
	output[13] = (d >> 16) & 0xFF;
	output[14] = (d >> 8) & 0xFF;
	output[15] = d & 0xFF;
}
#endif // _NEVER_USED__

CStringW UcSimplestHash2(LPCSTR pd, int len)
{
	unsigned int hash = 0;
	for (int i=0;i<len;i++)//char ch : str)
	{
		char ch = pd[i];
		hash = 31 * hash + ch;
	}
	CStringW sh;sh.Format(L"%X", hash);
	return sh;
}

CStringW UcSimplestHash3(LPCSTR pd, int len)
{
	unsigned int hash = 0;
	unsigned int prime = 31;  // 소수를 사용
	for (int i=0;i<len;i++)//char ch : str)
	{
		char ch = pd[i];
		hash = hash * prime + ch;
		prime *= 37; // 다음 문자를 위해 다른 소수를 사용
	}
	CStringW sh;sh.Format(L"%X", hash);
	return sh;
}
UINT UcSimplestHashNum(LPCSTR pd, int len)
{
	const unsigned int prime1 = 31;
	const unsigned int prime2 = 37;
	UINT hash = 0;

	for (int i = 0; i < len; ++i)
	{
		hash = hash * prime1 + pd[i];
		hash = hash ^ (hash >> 16);
		hash = hash * prime2;
	}
	hash = hash ^ (hash >> 11);
	return hash;
}
CStringW UcSimplestHash(LPCSTR pd, int len)
{
	UINT hash = UcSimplestHashNum(pd, len);
	CStringW sh;sh.Format(L"%X", hash);
	return sh;
}

/// <summary>
/// MD5 알고리즘을 사용하여 파일의 해시값을 계산
/// </summary>
/// <returns></returns>
CStringW UcHashSimple(CString sFile)
{
	try
	{
#ifdef _DEBUG
		if(!UcIfFileExistEx(sFile))
			_break;
#endif // _DEBUG

#ifdef _DEBUGx
		auto t1 = GetTickCount64();
	// 파일 열기
		ifstream  ifs(sFile);//"test.txt");
		//std::basic_ifstream<wchar_t> ifs(sFile); 2byte씩 읽는다.
		// 파일 크기 계산
		ifs.seekg(0, ios::end);
		size_t length = ifs.tellg();
		ifs.seekg(0, ios::beg);

		if(sFile == L"C:\\server\\update\\Pro\\CADian_12.1.260.33187.P.VC16.x64.Alpha\\3DConnexionModule_23.12_16.irx")
			_break;
#else

		CFile ifs;
		if(!ifs.Open(sFile, CFile::modeRead))//|CFile::typeBinary))
			CFileException::ThrowOsError((LONG)::GetLastError(), sFile);
		KAtEnd defer([&ifs]() {
			ifs.Close();
		});
		size_t length = (size_t)ifs.GetLength();
#endif // _DEBUGx
		//sFile = L"C:\\server\\release\\cadian\\pro\\2024\\api-CADian2024_7.0.4.33141.P.VC16.x64\\icrx\\OpenDesignSDK\\lib\\VC16_amd64dll\\xerces-c_3_2.lib"
		ASSERT(length < 200'000'000);//186MB (195,169,814 바이트)
		// 파일 내용 읽기
		auto shBuf = SharedBuf(length);//std::shared_ptr<char>(new char[dwToRead + 1] {'\0'});//(std::make_shared<char[]>(dwToRead + 1));error C2440: '=': cannot convert from '_Ux (*const )' to 'char *'
		LPSTR input = shBuf.get();
		//unsigned char* input = new unsigned char[length];
#ifdef _DEBUGx
		ifs.read((char*)input, length);
#else
		ifs.Read(input, (UINT)length);
#endif // _DEBUGx

		// MD5 해시 계산
		//unsigned char output[16];
		//MD5((const unsigned char*)input, length, output);
		CStringW ssw = UcSimplestHash(input, (int)length);

#ifdef _DEBUGx
		// 해시값 출력
		// TRACE("HASH: %s\n", sFile.c_str());
		std::stringstream ss;
		for (int i = 0; i < 16; i++)
		{
			CStringA sa;sa.Format("%02X", output[i]);
			ss << (LPCSTR)sa;
			//TRACE("%02x\n", output[i]);
		}

		CStringW ssw(ss.str().c_str());
		//TRACE(L"%0s, elapsed: %3u, len: %10lld %s\n", ssw, GetTickCount() - t1, length, sFile.c_str());
#endif // _DEBUGx

		return ssw;
		//cout << endl;
	}
	catch (CMemoryException* e)
	{
		UNUSED_ALWAYS(e);
		ASSERT(0);
	}
	catch (CFileException* e)
	{
		UNUSED_ALWAYS(e);
		auto er = GetLastError();
		auto ser = UcErrorToStrW(er);
		TRACE(L"CFileException(%u: %s)\n", er, ser);
	}
	catch (CException* e)
	{
		UNUSED_ALWAYS(e);
		auto er = GetLastError();
		auto ser = UcErrorToStrW(er);
		TRACE(L"CException(%u: %s)\n", er, ser);
		ASSERT(0);
	}
	catch (std::exception e)
	{
		auto serr = e.what();
		ASSERT(0);
	}
	return L"";
}
