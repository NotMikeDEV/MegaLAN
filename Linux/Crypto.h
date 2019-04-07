#pragma once
class Crypto
{
private:
	unsigned char NetworkKey[32];
public:
	Crypto(const unsigned char* NetworkKey);
	int Decrypt(const unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext);
	int Encrypt(const unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext);
};