#include "Globals.h"

Listing::Listing(const std::string &Servers, const unsigned char UserID[20], const unsigned char PasswordSHA1[20], const unsigned char PasswordSHA256[32])
{
	printf("Connecting to %s\n", Servers.c_str());
	Link = new Protocol(Servers, UserID, PasswordSHA1, PasswordSHA256);
	if (!Link->Auth())
	{
		printf("Server rejected username/password.\n");
		exit(1);
	}
	printf("Requesting VLAN listing\n");
	Link->GetListing(Listing::ListingCallback, this);
}

void Listing::ListingCallback(const void* CallerData, const unsigned char ID[20], std::string Name, std::string Description, bool Password)
{
	while (Name.length() < 30)
		Name += " ";
	printf("%s\t", Name.c_str());
	if (Password)
		printf("[P] ");
	else
		printf("    ");
	printf("%s\n", Description.c_str());
}

Listing::~Listing()
{
	delete Link;
}
