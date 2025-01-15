#pragma once
/*#define _CRT_SECURE_NO_WARNINGS //Allow old time functions
#include <iostream>
#include <fstream> //File stream
#include <sstream> //String stream for reading entire files
#include <string>
#include <vector>
#include <ctime> //For time functions
#include <winsock2.h> //Provides functions and definitions for network programming using Winsock
#include <ws2tcpip.h> //Contains additional functions for TCP/IP networking, for InetPtonW and other functions
#pragma comment(lib, "ws2_32.lib") //Links against the Winsock library
#include <thread>
using namespace std;



namespace NetworkingMain
{
	enum NetAuthority { Offline, Server, Client };

	//extern const PCWSTR IP = L"127.0.0.1";//Public IP of the server
	////LocalHost : 127.0.0.1:8888

	//extern const int PORT = 8888;//Port of the server
	//extern const int BUFFER_SIZE = 1024;

	//Honestly idk how extern works, but this allows the vars to be defined in header and declared in cpp for namespaces
	extern vector<SOCKET> clientArray;
	extern WSADATA wsaData;
	extern SOCKET serverSocket;
	extern SOCKET clientSocket;
	extern sockaddr_in clientAddr;
	extern sockaddr_in serverAddr;
	extern NetAuthority netAuth;




	//Server
	int intern_ServerWaitNewClients();
	void extern_SetupServer();
	void extern_CloseServer();
	void extern_ServerTick();
	void ServerClientListener();

	//Client
	void extern_ClientTick();
	void extern_SetupClient();
	void extern_CloseClient();
	//void EvaluateIncomingMessage();
};*/