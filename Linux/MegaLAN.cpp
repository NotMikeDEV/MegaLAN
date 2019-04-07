#include "Globals.h"
#include <signal.h>

bool Dead = false;
void term(int signum)
{
	Dead = true;
	printf("Exiting.\n");
}

void print_usage_and_exit(const char* progname, const char* error = NULL)
{
	if (error)
		printf("%s\n", error);
	printf("Usage: %s <server> <username> <password>\tDisplays list of VLANS\n", progname);
	printf("Usage: %s <server> <username> <password> <VLAN> [options]\tConnects to selected VLAN\n", progname);
	printf("Options:\n");
	printf("\t-p <password>\tSpecifies the VLAN password.\n");
	printf("\t-i <iface>\tSpecifies the name of the interface.\n");
	exit(256);
}

int main(int argc, char* argv[])
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	setlocale(LC_ALL, "UTF-8");
	if (argc >= 4)
	{
		std::string Server = argv[1];
		std::string Username = argv[2];
		std::string Password = argv[3];
		memset(argv[3], '*', strlen(argv[3]));
		unsigned char UserID[20];
		SHA1((unsigned char*)Username.c_str(), Username.size(), UserID);
		unsigned char PasswordSHA1[20];
		SHA1((unsigned char*)(Username + Password).c_str(), (Username + Password).size(), PasswordSHA1);
		unsigned char PasswordSHA256[32];
		SHA256((unsigned char*)(Username + Password).c_str(), (Username + Password).size(), PasswordSHA256);
		if (argc == 4)
		{
			Listing List(Server, UserID, PasswordSHA1, PasswordSHA256);
		}
		else
		{
			std::string VLANName = argv[4];
			char* InterfaceName = NULL;
			std::string VLANPassword = "";
			if (argc > 5)
			{
				char** argp = &argv[5];
				while (*argp)
				{
					if (memcmp(argp[0], "-i", 2) == 0 && argp[1] && strlen(argp[1]) && !InterfaceName)
					{
						InterfaceName = argp[1];
						argp++;
						argp++;
					}
					else if (memcmp(argp[0], "-p", 2) == 0 && argp[1] && strlen(argp[1]) && !VLANPassword.length())
					{
						VLANPassword = argp[1];
						argp++;
						argp++;
					}
					else
					{
						std::string Error("Invalid option: ");
						Error += argp[0];
						print_usage_and_exit(argv[0], Error.c_str());
					}
				}
			}
			VLAN Network(Server, UserID, PasswordSHA1, PasswordSHA256, VLANName, VLANPassword, InterfaceName);
			while (!Dead)
				Network.DoLoop();
		}
	}
	else
	{
		print_usage_and_exit(argv[0]);
	}
	return 0;
}