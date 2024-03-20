#pragma warning(disable:4996)
#pragma comment(lib,"WS2_32.lib");
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <thread>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#define NAMELEN    56
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 15000

static int index = 0;
typedef struct info {
	int sock;
	char name[NAMELEN];
	char ip[20];
};

typedef std::map<int, info> clientInfo;
clientInfo clInfo;
void updateClientList(char buffer[]);
void messageType(SOCKET* sock, char buffer[]);
void broadCast(SOCKET sock);
void multiCast(SOCKET sock);
void one_to_one(SOCKET sock);
void packMessage(int type, char buffer[]);
void Receive(SOCKET* pscktConnection);
void Send(SOCKET* pscktConnection);
int getSocketIndex(SOCKET sock);
void deleteUser(char buffer[]);

void main()
{
	// Variable definition
	int       nNameLength = 0;
	char     pcNameBuffer[NAMELEN];
	WSADATA   wsaData;

	sockaddr_in paddrServerAddress;

	// Code section

	// Initialize Winsock
	int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// If winsock dll loading has failed
	if (nResult != 0)
	{
		std::cout << "Failed loading winsock DLL with error code : " << WSAGetLastError() << std::endl;
		exit(1);
	}
	else
	{
		//Setup connection info
		ZeroMemory(&paddrServerAddress, sizeof(paddrServerAddress));
		paddrServerAddress.sin_family = AF_INET;
		paddrServerAddress.sin_port = htons(DEFAULT_PORT);
		paddrServerAddress.sin_addr.s_addr = inet_addr("192.168.1.108");

		// Request user for his name
		pcNameBuffer[0] = '\0';
		std::cout << "PLEASE ENTER YOUR NAME ->  ";
		std::cin.getline(pcNameBuffer, NAMELEN);
		std::cout << std::endl;

		// Creating the socket
		SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Connecting
		nResult = connect(clientSocket, (SOCKADDR*)&paddrServerAddress, sizeof(paddrServerAddress));


		// Creating of the socket has failed
		if (nResult == SOCKET_ERROR)
		{
			std::cout << "Socket creation failed with error code : " << WSAGetLastError() << std::endl;
			exit(1);
		}
		// Send server user's name
		else
		{
			char userInfo[DEFAULT_BUFLEN] = "1,";
			strcat(userInfo, pcNameBuffer);

			nResult = send(clientSocket, userInfo, strlen(userInfo), 0);

			// An error has occured while sending server the user's name
			if (nResult <= 0)
			{
				std::cout << "Some error has occured" << std::endl;
			}
			else
			{
				std::thread Read(Receive, &clientSocket);
				std::thread Write(Send, &clientSocket);
				Read.join();
				Write.join();
			}
		}

	}


	WSACleanup();
}


void Receive(SOCKET* sock)
{
	int  nReceivedBytes;
	int  nBufferLen = DEFAULT_BUFLEN;
	char pcBuffer[DEFAULT_BUFLEN];

	while (true)
	{
		memset(pcBuffer, 0, DEFAULT_BUFLEN);
		nReceivedBytes = recv((*sock), pcBuffer, nBufferLen, 0);
		if (nReceivedBytes < 0)
		{
			std::cout << "Failed to receive \n";
			exit(1);
		}

		int key = pcBuffer[0];
		if (key == '1')
		{
			int len = strlen(pcBuffer);
			for (int i = 0; i < len; i++) {
				if (i < len - 1)
					pcBuffer[i] = pcBuffer[i + 1];
				else
				{
					pcBuffer[i] = '\0';
					break;
				}

			}
			std::cout << pcBuffer << std::endl;
		}
		else if (key == '2')
		{
			int len = strlen(pcBuffer);
			for (int i = 0; i < len; i++) {
				if (i < len - 1)
					pcBuffer[i] = pcBuffer[i + 1];
				else
				{
					
					pcBuffer[i] = '\0';
					break;
				}
			}
			deleteUser(pcBuffer);
		}
		else
		{
			updateClientList(pcBuffer);
		}

	}
}


void Send(SOCKET* pscktConnection)
{

	int  nSentBytes;
	int  nBufferLen = DEFAULT_BUFLEN;
	char pcBuffer[DEFAULT_BUFLEN];

	pcBuffer[0] = '\0';



	while (true)
	{
		int ch;
		int nSentBytes = 0;
		std::cout << "Press 1 for Broadcast Message" << std::endl;
		std::cout << "Press 2 for MultiCast Message" << std::endl;
		std::cout << "Press 3 to send one-to-one Message" << std::endl;
		std::cout << "Enter your choice...." << std::endl;
		std::cin >> ch;
		switch (ch)
		{
		case 1:broadCast(*pscktConnection);
			break;
		case 2:multiCast(*pscktConnection);
			break;
		case 3:one_to_one(*pscktConnection);
			break;
		}
		std::cin.getline(pcBuffer, nBufferLen);

		while (pcBuffer[nSentBytes] != '\0')
		{
			++nSentBytes;
		}

		pcBuffer[nSentBytes] = '\0';

		nSentBytes = send((*pscktConnection), pcBuffer, nSentBytes, 0);

		if (nSentBytes == SOCKET_ERROR)
		{
			std::cout << "Failed to send " << std::endl;
		}
	}
}


void updateClientList(char buffer[])
{

	info* in = (info*)buffer;
	if (clInfo.empty())
	{
		++index;
		strcpy(clInfo[index].name, in->name);
		strcpy(clInfo[index].ip, in->ip);
		clInfo[index].sock = in->sock;
	}
	else
	{
		for (auto it = clInfo.begin(); it != clInfo.end(); it++)
		{
			if (in->sock == it->second.sock)
				return;
		}
		++index;
		strcpy(clInfo[index].name, in->name);
		clInfo[index].sock = in->sock;
		strcpy(clInfo[index].ip, in->ip);
	}

	std::cout << "Client List updated" << std::endl;
	std::cout << "Index\tSocket\tIP\tUserName\n";
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		std::cout << it->first << "\t" << it->second.sock << "\t" << it->second.ip << "\t" << it->second.name << std::endl;
	}
}

void broadCast(SOCKET sock)
{
	char input[DEFAULT_BUFLEN];
	std::cout << "Enter Message \n";
	std::cin.ignore();
	std::cin.getline(input, DEFAULT_BUFLEN);
	char message[DEFAULT_BUFLEN] = "4,";
	strcat(message, input);
	send(sock, message, strlen(message), 0);
}

void multiCast(SOCKET sock)
{
	std::cout << "Index\tSocket \tIP\tUser Name\n";
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		std::cout << it->first << "\t" << it->second.sock << "\t" << it->second.ip << "\t" << it->second.name << std::endl;
	}
	int n;

	std::cout << "Enter number of Clients\n";
	std::cin >> n;
	int* clList = new int[n];
	std::cout << "Enter indexes of selected clients\n";
	for (int i = 0; i < n; i++)
		std::cin >> clList[i];
	char input[DEFAULT_BUFLEN] = {0,};
	std::cin.ignore();
	std::cout << "Enter message " << std::endl;
	std::cin.getline(input, DEFAULT_BUFLEN);
	char nameList[DEFAULT_BUFLEN] = {0,};

	for (int i = 0; i < n; i++)
	{
		strcat(nameList, clInfo[clList[i]].name);
		strcat(nameList, ",");
	}
	char message[DEFAULT_BUFLEN] = {0,};
	strcat(message, "5,");
	strcat(message, nameList);
	strcat(message, input);
	strcat(message, ".");
	send(sock, message, strlen(message), 0);
}
void one_to_one(SOCKET sock)
{
	std::cout << "Index\tSocket \tUser Name\n";
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		std::cout << it->first << "\t" << it->second.sock << "\t" << it->second.name << std::endl;
	}
	int n;
	char input[DEFAULT_BUFLEN];
	std::cout << "Enter index of the user\n";
	std::cin >> n;
	std::cout << "Enter message \n";
	std::cin.ignore();
	std::cin.getline(input, DEFAULT_BUFLEN);
	char clName[NAMELEN];
	strcpy(clName, clInfo[n].name);
	char message[DEFAULT_BUFLEN] = "3,";
	strcat(message, clName);
	strcat(message, ",");
	strcat(message, input);
	send(sock, message, strlen(message), 0);

}
void deleteUser(char buffer[])
{
	std::string str(buffer);
	int sock = std::stoi(str);
	int index1 = getSocketIndex(sock);
	if (index!=-1)
		clInfo.erase(index1);
	std::cout << "Index\tSocket\tIP\tUserName\n";
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		std::cout << it->first << "\t" << it->second.sock << "\t" << it->second.ip << "\t" << it->second.name << std::endl;
	}
}

int getSocketIndex(SOCKET sock) {
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		if (it->second.sock == sock)
			return it->first;
	}
	return -1;
}