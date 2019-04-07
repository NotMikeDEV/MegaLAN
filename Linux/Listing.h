#pragma once
class Listing
{
private:
	Protocol* Link;
	static LISTING_CALLBACK ListingCallback;
public:
	Listing(const std::string &Servers, const unsigned char UserID[20], const unsigned char PasswordSHA1[20], const unsigned char PasswordSHA256[32]);
	~Listing();
};

