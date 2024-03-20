//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable:4996)
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <string>
#include <map>
#include <Winsock2.h>
#include <sstream>
#define PORT 15000
#define NAMELEN 56
#define BUFLEN 512
using namespace std;

typedef struct info {  // structure to store  user information
	int sock;
	char name[NAMELEN];
	char ip[20];
};
typedef map<int, info> clientInfo;

static int index = 0;
clientInfo clInfo;
struct sockaddr_in clientAdd;
int case_insensitive_match(string s1, string s2);
void getUserInfo(SOCKET sock, char buffer[]);
void receiveMessage(SOCKET sock, char buffer[]);
void forwardMessage(SOCKET sock, char buffer[]);
void requestType(SOCKET sock, char buffer[]);
int getSocketIndex(SOCKET sock);
void broadCast(SOCKET sock, char buffer[]);
void multiCast(SOCKET sock, char buffer[]);
void updateClientList();
void deleteUser(int sock);

int main()
{

	vector<SOCKET> nClientSocket(10, 0);
	struct sockaddr_in serverAddress;

	fd_set fr;
	int clientAddLen = sizeof(clientAdd);
	int nMaxFd;
	WSADATA wsdata;
	if (WSAStartup(MAKEWORD(2, 2), &wsdata) < 0)
	{
		cout << "Failed to load WS2_32.dll file with error code : " << WSAGetLastError() << endl;
		return (EXIT_FAILURE);
	}

	//Listener socket
	SOCKET nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (nSocket < 0)
	{
		cout << "Socket Creation fsiled with error code : " << WSAGetLastError() << endl;
		exit(EXIT_FAILURE);
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	memset(&serverAddress.sin_zero, 0, 8);

	int nRet = bind(nSocket, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr));
	if (nRet < 0)
	{
		cout << "Binding failed with error code : " << WSAGetLastError() << endl;
		exit(EXIT_FAILURE);
	}
	else
		cout << "Server created with port number : " << PORT << " and Socket : " << nSocket << endl;

	nRet = listen(nSocket, SOMAXCONN);
	if (nRet < 0)
	{
		cout << "The listen failed" << endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		cout << "Listening for incoming connection" << endl;
	}
	nMaxFd = nSocket;
	FD_ZERO(&fr);

	char name[50];

	while (1)
	{
		FD_ZERO(&fr);
		FD_SET(nSocket, &fr); // 

		for (int nIndex = 0; nIndex < nClientSocket.size(); nIndex++) //To add client sockets to fd_set
		{
			if (nClientSocket[nIndex] > 0)
			{
				FD_SET(nClientSocket[nIndex], &fr);
			}
		}
		
		nRet = select(nMaxFd + 1, &fr, nullptr, nullptr, nullptr); //it blocks the connection untill a request arrive
		if (nRet < 0)
		{
			cout << "Select api call failed. Will exit" << endl;
			return (EXIT_FAILURE);
		}
		else
		{
		
			if (FD_ISSET(nSocket, &fr))// To check whether nSocket is in fd_set or not
			{
				memset(&clientAdd, 0, sizeof(clientAdd));
				SOCKET nNewClient = accept(nSocket, (SOCKADDR*)&clientAdd, &clientAddLen);
				if (nNewClient < 0)
				{
					cout << "Not able to get a new client socket" << endl;
				}
				else
				{

					int nIndex;
					for (nIndex = 0; nIndex < nClientSocket.size(); nIndex++)
					{
						if (nClientSocket[nIndex] == 0)// To check if space available to add socket into SocketList
						{
							nClientSocket[nIndex] = nNewClient;
							if (nNewClient > nMaxFd)
							{
								nMaxFd = nNewClient; // To find max socket for select function call
							}
							break;
						}
					}

					if (nIndex == 10)
					{
						cout << "Server busy. Cannot accept anymore connections" << endl;
					}
				}
			}
			else
			{
				for (int nIndex = 0; nIndex < nClientSocket.size(); nIndex++) // To receive data from clients connected to server
				{
					if (nClientSocket[nIndex] > 0)
					{
						if (FD_ISSET(nClientSocket[nIndex], &fr))
						{
							//Read the data from client
							char sBuff[255];
							memset(sBuff, 0, 255);
							int nRet = recv(nClientSocket[nIndex], sBuff, 255, 0);

							if (nRet < 0)
							{
								
								cout << "Error at client socket" << endl;
								int index1 = getSocketIndex(nClientSocket[nIndex]);
								clInfo.erase(index1);
								cout << "Index\tSock\tIP\tUserName\n";
								for (auto it = clInfo.begin(); it != clInfo.end(); it++)
								{
									cout << it->first << "\t" << it->second.sock << "\t" << it->second.ip << "\t" << it->second.name << endl;
								}
								deleteUser(nClientSocket[nIndex]);
								closesocket(nClientSocket[nIndex]);
								
								nClientSocket[nIndex] = 0;
								
							}
							else
							{
								SOCKET in = nClientSocket[nIndex];
								requestType(nClientSocket[nIndex], sBuff);
								break;
							}

						}//end of 2nd inner if in the for loop

					}//end of 1st inner if in the for loop

				}//end of for loop
			}
		}

	}//end of while loop

}//end of main()

void requestType(SOCKET sock, char buffer[])
{
	char rt = buffer[0];
	int len = strlen(buffer);
	for (int i = 0; i < len; i++) {
		if (i < len - 2)
			buffer[i] = buffer[i + 2];
		else
		{
			buffer[i] = '\0';
			break;
		}

	}
	switch (rt)
	{
	case '1': getUserInfo(sock, buffer);
		break;
	case '2':receiveMessage(sock, buffer);
		break;
	case '3':forwardMessage(sock, buffer);
		break;
	case '4':broadCast(sock, buffer);
		break;
	case '5':multiCast(sock, buffer);
		break;
	}

}

void getUserInfo(SOCKET sock, char buffer[])
{

	info in;
	strcpy(in.name, buffer);
	char *ip = inet_ntoa(clientAdd.sin_addr);
	strcpy(in.ip, ip);
	in.sock = sock;
	
	if (index == 0)
	{
		++index;
		strcpy(clInfo[index].name, buffer);
		strcpy(clInfo[index].ip, inet_ntoa(clientAdd.sin_addr));
		clInfo[index].sock = sock;
	}
	else{
		for (auto it = clInfo.begin(); it != clInfo.end(); it++)
		{
			if (it->second.sock == in.sock)
				return;
			
		}
			++index;
			clInfo[index].sock = sock;
			strcpy(clInfo[index].name, in.name);
			strcpy(clInfo[index].ip, in.ip);
}
	
	updateClientList();
	cout << "Index\tSocket\tIP\tUserName\n";
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		cout << it->first << "\t" << it->second.sock << "\t"<<it->second.ip<<"\t"<< it->second.name << endl;
	}


}
void receiveMessage(SOCKET sock, char buffer[])
{
	int index;
	clientInfo::iterator it;
	//cout << "Buffer in receive : " << buffer << endl;
	index = getSocketIndex(sock);
	cout << "Mesaage from [ " << clInfo[index].name << " ] : " << buffer << endl;
	send(sock, buffer, strlen(buffer), 0);
}


void forwardMessage(SOCKET sock, char buffer[])
{
	int forward = 0;
	string user;
	int i;
	for (i = 0; i < strlen(buffer); i++)
	{
		if (buffer[i] == ',')
			break;
		user += buffer[i];
	}
	
	i++;
	int len = strlen(buffer);
	for (int j = 0; j != len; j++) {
		if (j < len - i)
			buffer[j] = buffer[j + i];
		else
		{
			buffer[j] = '\0';
			break;
		}

	}
	clientInfo::iterator it;
	
	int index;
	for (it = clInfo.begin(); it != clInfo.end(); it++)
	{

		if (case_insensitive_match(it->second.name, user))
		{
			index = it->first;
		}
	}
	string name(clInfo[index].name);
	string messsage = "1[ " + name + " ] : " + buffer;
	send(clInfo[index].sock, messsage.c_str(), messsage.size(), 0);
}

int case_insensitive_match(string s1, string s2) {

	transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
	transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
	if (s1.compare(s2) == 0)
		return 1;
	return 0;
}

int getSocketIndex(SOCKET sock) {
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		if (it->second.sock == sock)
			return it->first;
	}
	return -1;
}

void updateClientList()
{
	
	for (auto iter = clInfo.begin(); iter != clInfo.end(); iter++)
	{
		for (auto iter1 = clInfo.begin(); iter1 != clInfo.end(); iter1++)
		{
			int isend=send(iter->second.sock, (char*)&iter1->second, sizeof(info), 0);
			if (isend < 0)
				cout << "Send failed" << endl;
			else
				cout << "client sent succesfully" << endl;              
			Sleep(10);
		}
		
	}
}


void broadCast(SOCKET sock, char buffer[])
{
	char message[BUFLEN] = "1[ ";
	int index = getSocketIndex(sock);
	strcat(message, clInfo[index].name);
	strcat(message, " ] : ");
	strcat(message, buffer);
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		send(it->second.sock, message, strlen(message), 0);
	}
}

void multiCast(SOCKET sock, char buffer[])
{
	char message[BUFLEN] = { 0, };
	string word;
	vector<int> sockArray;
	int index1 = getSocketIndex(sock);
	strcat(message, "1[ ");
	strcat(message, clInfo[index1].name);
	strcat(message, " ] : ");
	for (int i = 0; buffer[i] != '\0'; i++)
	{
		
		if (buffer[i] == ',')
		{
			i++;
			for (auto it = clInfo.begin(); it != clInfo.end(); it++)
			{
				if (strcmpi(word.c_str(), it->second.name) == 0)
				{
					sockArray.push_back(it->second.sock);
					word.clear();
				}
			}
		}
		else if (buffer[i]=='.')
		{
			strcat(message, word.c_str());
			
		}
		else
		{
			word = word + buffer[i];
		}
	}
	for (int i = 0; i < sockArray.size(); i++)
	{
		send(sockArray[i], message, strlen(message), 0);
	}

}

void deleteUser(int sock)
{
	ostringstream str1;
	str1 << sock;
	string clSock = str1.str();
	string message = "2";
	message = message + clSock;
	for (auto it = clInfo.begin(); it != clInfo.end(); it++)
	{
		send(it->second.sock, message.c_str(), message.size(), 0);
		Sleep(10);
	}
}