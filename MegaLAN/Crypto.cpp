#include "stdafx.h"
#include <string>
#include "Crypto.h"
#include <Wincrypt.h>
BYTE Crypto::SHA1_Data[20];
BYTE Crypto::SHA256_Data[32];

Crypto::Crypto(BYTE Key[32])
{
	memcpy(this->Key, Key, 32);
}


Crypto::~Crypto()
{
}

BYTE* Crypto::SHA1(const BYTE* Buffer, int Length)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		printf("CryptAcquireContext failed: %d\n", GetLastError());
		return NULL;
	}
	HCRYPTHASH hHash = 0;
	if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
	{
		printf("CryptAcquireContext failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	if (!CryptHashData(hHash, Buffer, Length, 0))
	{
		printf("CryptHashData failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	DWORD cbHash = sizeof(SHA1_Data);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, SHA1_Data, &cbHash, 0))
	{
		printf("CryptGetHashParam failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	return SHA1_Data;
}

BYTE* Crypto::SHA1(std::string String)
{
	return SHA1((BYTE*)(String.c_str()), String.size());
}

BYTE* Crypto::SHA256(const BYTE* Buffer, int Length)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		printf("CryptAcquireContext failed: %d\n", GetLastError());
		return NULL;
	}
	HCRYPTHASH hHash = 0;
	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
	{
		printf("CryptCreateHash failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	if (!CryptHashData(hHash, Buffer, Length, 0))
	{
		printf("CryptHashData failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	DWORD cbHash = sizeof(SHA256_Data);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, SHA256_Data, &cbHash, 0))
	{
		printf("CryptGetHashParam failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	return SHA256_Data;
}

BYTE* Crypto::SHA256(std::string String)
{
	return SHA256((BYTE*)(String.c_str()), String.size());
}

typedef struct AES256KEYBLOB_ {
	BLOBHEADER bhHdr;
	DWORD dwKeySize;
	BYTE szBytes[32];
} AES256KEYBLOB;

DWORD Crypto::AES256_Decrypt(BYTE* Buffer, DWORD Length)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		printf("CryptAcquireContext failed: %d\n", GetLastError());
		return 0;
	}
	HCRYPTKEY hKey;
	AES256KEYBLOB AESBlob;
	AESBlob.bhHdr.bType = PLAINTEXTKEYBLOB;
	AESBlob.bhHdr.bVersion = CUR_BLOB_VERSION;
	AESBlob.bhHdr.reserved = 0;
	AESBlob.bhHdr.aiKeyAlg = CALG_AES_256;
	AESBlob.dwKeySize = 32;
	memcpy(AESBlob.szBytes, Key, 32);
	if (!CryptImportKey(hProv, (BYTE*)&AESBlob, sizeof(AESBlob), 0, CRYPT_EXPORTABLE, &hKey))
	{
		printf("CryptImportKey failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return 0;
	}

	if (!CryptDecrypt(hKey, NULL, TRUE, 0, Buffer, &Length)) {
		printf("CryptDecrypt failed: %d\n", GetLastError());
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
		printf("%s\n", messageBuffer);
		LocalFree(messageBuffer);
		CryptDestroyKey(hKey);
		CryptReleaseContext(hProv, 0);
		return 0;
	}
	CryptDestroyKey(hKey);
	CryptReleaseContext(hProv, 0);
	return Length;
}
DWORD Crypto::AES256_Encrypt(BYTE* Buffer, DWORD Length)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		printf("CryptAcquireContext failed: %d\n", GetLastError());
		return 0;
	}
	HCRYPTKEY hKey;
	AES256KEYBLOB AESBlob;
	AESBlob.bhHdr.bType = PLAINTEXTKEYBLOB;
	AESBlob.bhHdr.bVersion = CUR_BLOB_VERSION;
	AESBlob.bhHdr.reserved = 0;
	AESBlob.bhHdr.aiKeyAlg = CALG_AES_256;
	AESBlob.dwKeySize = 32;
	memcpy(AESBlob.szBytes, Key, 32);
	if (!CryptImportKey(hProv, (BYTE*)&AESBlob, sizeof(AESBlob), 0, CRYPT_EXPORTABLE, &hKey))
	{
		printf("CryptImportKey failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return 0;
	}

	if (!CryptEncrypt(hKey, NULL, TRUE, 0, Buffer, &Length, Length+32)) {
		printf("CryptEncrypt failed: %d\n", GetLastError());
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
		printf("%s\n", messageBuffer);
		LocalFree(messageBuffer);
		CryptDestroyKey(hKey);
		CryptReleaseContext(hProv, 0);
		return 0;
	}
	CryptDestroyKey(hKey);
	CryptReleaseContext(hProv, 0);
	return Length;
}