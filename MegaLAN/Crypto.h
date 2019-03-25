#pragma once
class Crypto
{
private:
	BYTE SHA1_Data[20];
	BYTE SHA256_Data[32];
public:
	Crypto();
	BYTE* SHA1(const BYTE* Buffer, int Length);
	BYTE* SHA1(std::string String);
	BYTE* SHA256(const BYTE* Buffer, int Length);
	BYTE* SHA256(std::string String);
	DWORD AES256_Decrypt(BYTE* Buffer, DWORD Length, BYTE* Key);
	DWORD AES256_Encrypt(BYTE* Buffer, DWORD Length, BYTE* Key);
	~Crypto();
};

extern Crypto Crypt;