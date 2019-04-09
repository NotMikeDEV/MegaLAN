#pragma once
class Crypto
{
private:
	static BYTE SHA1_Data[20];
	static BYTE SHA256_Data[32];
public:
	BYTE Key[32];
	Crypto(BYTE* Key);
	static BYTE* SHA1(const BYTE* Buffer, int Length);
	static BYTE* SHA1(std::string String);
	static BYTE* SHA256(const BYTE* Buffer, int Length);
	static BYTE* SHA256(std::string String);
	DWORD AES256_Decrypt(BYTE* Buffer, DWORD Length);
	DWORD AES256_Encrypt(BYTE* Buffer, DWORD Length);
	~Crypto();
};

extern Crypto Crypt;