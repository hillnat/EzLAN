#pragma once
#include "pch.h"
#define _CRT_SECURE_NO_WARNINGS //Allow old time functions
#ifdef BUILD_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif
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
#include <vector>
#define EZLAN_PORT 7777
using namespace std;
enum axes {id, x, y, z };
enum netAuthority { Offline, Server, Client };
enum logs { ServerStarted, ServerEnded, ClientConnected,ClientStarted,ClientEnded,IdSet,FATAL,FATAL0,FATAL1,FATAL2,FATAL3,FATAL4,FATAL5,FATAL6,FATAL7 };
__declspec(dllexport) struct sVec3 { int id; float x; float y; float z; };//Datatype for vector including ID
struct spawnRequest { int id; };//Datatype for vector including ID
struct vec3 { float x = 0.F; float y = 0.F; float z = 0.F; };

PCWSTR SERVERIP = L"127.0.0.1";

vector<sVec3> vecsToSend;//To send out to other sockets
vector<sVec3> vecsToProcess;//To be processed in unity

vector<SOCKET> clientArray;
WSADATA wsaData;
SOCKET serverSocket;
SOCKET clientSocket;
sockaddr_in clientAddr;
sockaddr_in serverAddr;
int myId = -1;
thread waitForClientsThread;

netAuthority netAuth = netAuthority::Offline;

vector<int> logsList;

extern "C" {
	__declspec(dllexport) void __stdcall extern_SetupServer();
	__declspec(dllexport) void __stdcall extern_ServerTick();
	__declspec(dllexport) void __stdcall extern_CloseServer();
	__declspec(dllexport) void __stdcall extern_SetupClient();
	__declspec(dllexport) void __stdcall extern_ClientTick();
	__declspec(dllexport) void __stdcall extern_CloseClient();
	__declspec(dllexport) int __stdcall extern_GetNetAuthority();
	__declspec(dllexport) int __stdcall extern_GetClientCount();
	__declspec(dllexport) int __stdcall extern_HasLog();
	__declspec(dllexport) int __stdcall extern_GetNextLog();
	__declspec(dllexport) int __stdcall extern_GetId();
	__declspec(dllexport) sVec3 __stdcall extern_GetNextVecToProcess();
	__declspec(dllexport) void __stdcall extern_AddVecToSend(int id, float x, float y, float z);
	__declspec(dllexport) int __stdcall extern_HasVecToProcess();

}


int extern_GetNetAuthority() { return (int)netAuth; }
int extern_GetClientCount() { return (int)clientArray.size(); }
int extern_GetNextLog() {
	int c = logsList[0];
	logsList.erase(logsList.begin());
	return c;
}
int extern_HasLog() {
	if (logsList.size() > 0) {
		return 0;
	}
	else {
		return 1;
	}
}
int extern_GetId() {
	return myId;
}

void intern_CloseClientArray() {
	if (netAuth != netAuthority::Server) {
		return;
	}
	for (int i = 0; i < clientArray.size(); i++) {
		closesocket(clientArray[i]);
	}
	clientArray.clear();
}

void extern_AddVecToSend(int id, float x, float y, float z) { vecsToSend.push_back(sVec3{ id,x,y,z }); }
sVec3 extern_GetNextVecToProcess() {
	sVec3 s{ vecsToProcess[0].id, vecsToProcess[0].x, vecsToProcess[0].y, vecsToProcess[0].z};
	vecsToProcess.erase(vecsToProcess.begin());
	return s;
}
int extern_HasVecToProcess() { return vecsToProcess.size(); }


sVec3 intern_MakeSVecFromData(const string data) {
	sVec3 output = sVec3{ 0, 0.f,0.f,0.f };
	string curNum = "";
	axes axis = (axes)0;
	bool done = false;
	for (int i = 0; i < data.length(); i++) {
		if (done) { break; }
		switch ((char)(data[i]))
		{
		case 'i':
			axis = axes::id;
			curNum = "";
			break;
		case 'x':
			axis = axes::x;
			curNum = "";
			break;
		case 'y':
			axis = axes::y;
			curNum = "";
			break;
		case 'z':
			axis = axes::z;
			curNum = "";
			break;
		case '?'://End of number. When we reach this, apply the number weve constructed to our vec3
			switch (axis)
			{
			case(axes::x):
				output.x = stof(curNum);
				break;
			case(axes::y):
				output.y = stof(curNum);
				break;
			case(axes::z):
				output.z = stof(curNum);
				break;
			case(axes::id):
				output.z = stof(curNum);
				break;
			};
			break;
		case ';'://End of message
			done = true;
			break;
		default://If the char can be parsed to an int, add it to whichever axis we are currently working
			curNum += data[i];
			break;
		};

	}

	//std::cout<<"[extern_ClientTick] : Received : " + string(recvBuffer)<<endl;
	return output;
}
string intern_MakeDataFromSVec(sVec3 vec) {
	string message = "i";
	message += vec.id;
	message += "x";
	message += std::to_string(vec.x);
	message += "?";//END OF NUM

	message += "y";
	message += std::to_string(vec.y);
	message += "?";//END OF NUM

	message += "z";
	message += std::to_string(vec.z);
	message += "?";//END OF NUM

	message += ";";//END MESSAGE
	return message;
}

void intern_SendAllVecs() {
	//Send out all synced vectors
	for (int i = 0; i < vecsToSend.size(); i++) {
		string message = intern_MakeDataFromSVec(vecsToSend[i]);
		const char* buff = message.c_str();
		for (int i = 0; i < clientArray.size(); i++) {
			send(clientArray[i], buff, sizeof(message), 0);
		}
	}
	vecsToSend.clear();
}

void intern_ServerWaitNewClients() {//Continously wait for new connections
	while (true) {
		int clientAddrLen = sizeof(clientAddr);
		clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
		if (clientSocket == INVALID_SOCKET) {
			//return 1;//Failure
			continue;
		}
		clientArray.push_back(clientSocket);
		//Send the new client their ID
		string message = to_string(clientArray.size());
		const char* buff = message.c_str();
		send(clientSocket, buff, sizeof(message), 0);//Send their id

		logsList.push_back(logs::ClientConnected);
	}
	//thread t = thread(&ServerClientListener); //Run this method to enable a 2 way connection
	//t.detach();
}


void extern_SetupServer() {

	if (netAuth != netAuthority::Offline) {
		return;
	}
	intern_CloseClientArray();
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		//Log("WSA Startup failed", FOREGROUND_RED);
		logsList.push_back(logs::FATAL0);
		return;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		//Log("Socket creation failed", FOREGROUND_RED);
		WSACleanup();
		logsList.push_back(logs::FATAL1);
		return;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(EZLAN_PORT);

	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		//Log("Bind failed", FOREGROUND_RED);

		closesocket(serverSocket);
		WSACleanup();
		logsList.push_back(logs::FATAL2);
		return;
	}

	if (listen(serverSocket, 5) == SOCKET_ERROR) {
		//Log("Listen failed", FOREGROUND_RED);
		closesocket(serverSocket);
		WSACleanup();
		logsList.push_back(logs::FATAL3);
		return;
	}
	//Log("Server started, netAuth set to Server", FOREGROUND_GREEN);

	netAuth = netAuthority::Server;
	myId = 9999;

	thread(&intern_ServerWaitNewClients).detach();//Thread for accepting new clients
	//thread(&ServersideClientRepeater).detach();//Thread for accepting new clients
	logsList.push_back(logs::ServerStarted);
}
void extern_ServerTick() {

	if (netAuth != netAuthority::Server) { return; }
	intern_SendAllVecs();
	//Wait for incoming data to process
	char recvBuffer[1024];
	int bytesRead = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
	if (bytesRead > 0) {
		//recvBuffer[bytesRead] = '\0';
		string newData = string(recvBuffer);

		//Broadcast anything we recieve back out to all clients, raw
		for (int i = 0; i < clientArray.size(); i++) {
			//cout << "Broadcasting message to client #" << i << endl;
			send(clientArray[i], recvBuffer, bytesRead, 0);
		}

		vecsToProcess.push_back(intern_MakeSVecFromData(newData));
	}
}
void extern_CloseServer() {
	if (netAuth != netAuthority::Server) {
		return;
	}
	closesocket(serverSocket);
	intern_CloseClientArray();
	WSACleanup();
	netAuth = netAuthority::Offline;
	logsList.push_back(logs::ClientStarted);
}


void extern_SetupClient() {

	if (netAuth != netAuthority::Offline) {
		return;
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		logsList.push_back(logs::FATAL0);
		return;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		closesocket(clientSocket);
		WSACleanup();
		logsList.push_back(logs::FATAL1);

		return;
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(EZLAN_PORT);


	//serverAddr.sin_family = AF_INET;
	//serverAddr.sin_port = htons(EZLAN_PORT);

	//Convert IP address to wide string and use InetPtonW
	if (InetPtonW(AF_INET, SERVERIP, &serverAddr.sin_addr) != 1) {
		closesocket(clientSocket);
		WSACleanup();
		logsList.push_back(logs::FATAL2);

		return;
	}

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(clientSocket);
		WSACleanup();
		logsList.push_back(logs::FATAL3);

		return;
	}


	
	//Wait until we recieve our ID
	/*while(myId==-1) {
		char recvBuffer[1024];
		int bytesRead = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		string newData = string(recvBuffer);

		myId = stoi(newData);
	}*/
	netAuth = netAuthority::Client;
	myId = 2222;
	logsList.push_back(logs::IdSet);

	//thread t = thread(&extern_ClientTick);
	//t.detach();
	
	logsList.push_back(logs::ClientStarted);

}
void extern_ClientTick() {

	if (netAuth != netAuthority::Client || myId==-1) {
		return;
	}
	intern_SendAllVecs();

	//Wait for incoming data to process
	char recvBuffer[1024];
	int bytesRead = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
	if (bytesRead > 0) {
		//recvBuffer[bytesRead] = '\0';
		string newData = string(recvBuffer);
		vecsToProcess.push_back(intern_MakeSVecFromData(newData));        
	}
}
void extern_CloseClient() {

	//Log("Closing client", FOREGROUND_BLUE);
	if (netAuth != netAuthority::Client) {
		return;
	}
	closesocket(clientSocket);
	WSACleanup();
	netAuth = netAuthority::Offline;
	logsList.push_back(logs::ClientEnded);
}


