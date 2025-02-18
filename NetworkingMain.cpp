#pragma once
// _CRT_SECURE_NO_WARNINGS //Make sure this is in C/C++ PreProccessor definitions

#include "pch.h"
/*#ifdef BUILD_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif*/
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
#include "NetworkingMain.h"
#define RECVBUFF 1024
#define SERVERPORT 7777

using namespace std;
enum sVecAxes {id, px, py, pz,ry };//Type for the various axis of our synced vector that is being sent over the network containing an ID, x,y,z position values, as well as Y euler rotation
enum netAuthority { Offline, Server, Client };
enum logs { ServerStarted, ServerEnded, ClientConnected,ClientStarted,ClientEnded,IdSet,FATAL,FATAL0,FATAL1,FATAL2,FATAL3,FATAL4,FATAL5,FATAL6,FATAL7,OUTOFLOGS };
__declspec(dllexport) struct sVec3 { int id=0; float px=0; float py=0; float pz=0; float ry=0; };//Declare our synced data type to the DLL

PCWSTR serverIp = L"127.0.0.1";

vector<sVec3> vecsToSend;//To send out to other sockets
vector<sVec3> vecsToProcess;//To be processed in unity
vector<SOCKET> clientArray;//Array of all clients connected to server socket. Only for computer acting as server
WSADATA wsaData;//Required for socket use
SOCKET mySocket;//Reference to our socket
sockaddr_in clientAddr;
sockaddr_in serverAddr;
int myId = -1;//My ID is -1 until the DLL has finished network setup for appropriate network authroity
thread waitForClientsThread;
netAuthority netAuth = netAuthority::Offline;//Our computers authrotity over the network
vector<int> logsList;//List of logs recieved from DLL

//Function export declarations for DLL file
extern "C" {

	__declspec(dllexport) void __stdcall extern_SetupServer();
	__declspec(dllexport) void __stdcall extern_CloseServer();
	__declspec(dllexport) void __stdcall extern_SetupClient();
	__declspec(dllexport) void __stdcall extern_CloseClient();
	__declspec(dllexport) int __stdcall extern_GetNetAuthority();
	__declspec(dllexport) int __stdcall extern_GetClientCount();
	__declspec(dllexport) int __stdcall extern_HasLog();
	__declspec(dllexport) int __stdcall extern_GetNextLog();
	__declspec(dllexport) int __stdcall extern_GetId();
	__declspec(dllexport) sVec3 __stdcall extern_GetNextVecToProcess();
	__declspec(dllexport) void __stdcall extern_AddVecToSend(int id, float px, float py, float pz, float ry);
	__declspec(dllexport) int __stdcall extern_HasVecToProcess();
	__declspec(dllexport) int __stdcall extern_SetIp(char* ip);
	__declspec(dllexport) uint32_t __stdcall extern_GetIp(char* outBuff, uint32_t inSize);


}
uint32_t extern_GetIp(char* outBuff, uint32_t inSize) {
	//This setup users IntPtr types to allow the c# script to allocate the memory buffer, then we can populate it and send it back.
	char szReturnString[500];
	wcstombs(szReturnString, serverIp, 500);//Shittiest method name of all time
	const uint32_t uiStringLength = strlen(szReturnString);

	if (inSize >= (uiStringLength + 1))
	{
		strcpy(outBuff, szReturnString);
		// Return the number of characters copied.
		return uiStringLength;
	}
	else
	{
		// Return the required size
		// (including the terminating NULL character).
		return uiStringLength + 1;
	}
}
PCWSTR intern_CharToPCWSTR(const char* str) {
	// Get the required size of the wide-character string buffer
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);

	// Allocate buffer for wide-character string
	wchar_t* wstr = new wchar_t[size_needed];//Probably a memory leak

	// Convert the char* (multi-byte) string to wchar_t* (wide-character string)
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, size_needed);

	// Return the wide string as a PCWSTR (const wchar_t*)
	return (PCWSTR)wstr;//Convert normal wide string to pointer to constant wide string
}

int extern_SetIp(char* ip) {
	serverIp = intern_CharToPCWSTR(ip);
	return 1; 
}
int extern_GetNetAuthority() { return (int)netAuth; }
int extern_GetClientCount() { return (int)clientArray.size(); }

int extern_GetId() {
	return myId;
}
void intern_CloseClientArray() {
	if (netAuth != netAuthority::Server||clientArray.size()==0) {
		return;
	}
	for (int i = 0; i < clientArray.size(); i++) {
		closesocket(clientArray[i]);
	}
	clientArray.clear();
}
void extern_AddVecToSend(int id, float px, float py, float pz, float ry) { vecsToSend.push_back(sVec3{id,px,py,pz,ry }); }
sVec3 extern_GetNextVecToProcess() {
	if (vecsToProcess.size() == 0) { return sVec3{ -99,0,0,0,0}; }
	const sVec3 s{ vecsToProcess[0].id, vecsToProcess[0].px, vecsToProcess[0].py, vecsToProcess[0].pz,vecsToProcess[0].ry};
	vecsToProcess.erase(vecsToProcess.begin());
	return s;
}
int extern_HasVecToProcess() { return (int)vecsToProcess.size(); }
#pragma region Logging
//Returns next log to c# script
int extern_GetNextLog() {
	if (logsList.size() == 0) {
		return OUTOFLOGS;
	}
	int c = logsList[0];
	logsList.erase(logsList.begin());
	return c;
}
//Returns 1 if we hajve a log
int extern_HasLog() {
	if (logsList.size() > 0) {
		return 1;
	}
	else {
		return 0;
	}
}
#pragma endregion
#pragma region Data Processing
sVec3 intern_MakeSVecFromData(const string data) {
	//For parsing x y and z chars represent poisiton xyz (px,py,pr)
	//HJK represent rotation of the vector (rx,ry,rz)
	//This is done to keep the identifier as a single char
	sVec3 output = sVec3{ 0, 0.f,0.f,0.f,0.f };
	string curNum = "";
	sVecAxes axis = (sVecAxes)0;
	bool done = false;
	for (int i = 0; i < data.length(); i++) {
		if (done) { break; }
		switch ((char)(data[i]))
		{
		case 'i':
			axis = sVecAxes::id;
			curNum = "";
			break;
		case 'x':
			axis = sVecAxes::px;
			curNum = "";
			break;
		case 'y':
			axis = sVecAxes::py;
			curNum = "";
			break;
		case 'z':
			axis = sVecAxes::pz;
			curNum = "";
			break;
		case 'j':
			axis = sVecAxes::ry;
			curNum = "";
			break;
		case '?'://End of number. When we reach this, apply the numbers weve gathered since last switching axis
			switch (axis)
			{
			case(sVecAxes::id):
				output.id = stoi(curNum);
				break;
			case(sVecAxes::px):
				output.px = stof(curNum);
				break;
			case(sVecAxes::py):
				output.py = stof(curNum);
				break;
			case(sVecAxes::pz):
				output.pz = stof(curNum);
				break;	
			case(sVecAxes::ry):
				output.ry = stof(curNum);
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
	string message="i";
	message += std::to_string(vec.id);
	message += "?";//END OF ID
	//Pos
	message += "x";
	message += std::to_string(vec.px);
	message += "?";//END OF NUM
	message += "y";
	message += std::to_string(vec.py);
	message += "?";//END OF NUM
	message += "z";
	message += std::to_string(vec.pz);
	message += "?";//END OF NUM
	//Rot
	message += "j";
	message += std::to_string(vec.ry);
	message += "?";//END OF NUM


	message += ";";//END MESSAGE
	return message;
}

#pragma endregion
#pragma region Server
//Thread running this exists once per client on the server machine
void intern_ServerRx(const int clientIndex) {
	//Wait for incoming data to process
	while (true) {
		if (clientArray.size() <= clientIndex) { return; }
		char recvBuffer[RECVBUFF];
		int bytesRead = recv(clientArray[clientIndex], recvBuffer, sizeof(recvBuffer), 0);
		if (bytesRead > 0) {
			//recvBuffer[bytesRead] = '\0';
			string newData = string(recvBuffer);
			/*
			//Broadcast anything we recieve back out to all clients, raw
			for (int i = 0; i < clientArray.size(); i++) {
				//cout << "Broadcasting message to client #" << i << endl;
				send(clientArray[i], recvBuffer, bytesRead, 0);
			}*/

			vecsToProcess.push_back(intern_MakeSVecFromData(newData));
		}
	}
}
void intern_ServerWaitNewClients() {//Continously wait for new connections
	while (true) {
		int clientAddrLen = sizeof(clientAddr);
		SOCKET newSocket = accept(mySocket, (sockaddr*)&clientAddr, &clientAddrLen);
		if (newSocket == INVALID_SOCKET) {
			//return 1;//Failure
			continue;
		}
		clientArray.push_back(newSocket);
		//Send the new client their ID
		string message = to_string(clientArray.size());
		const char* buff = message.c_str();
		send(newSocket, buff, sizeof(message), 0);//Send their id

		logsList.push_back(logs::ClientConnected);
		thread(&intern_ServerRx, clientArray.size() - 1).detach();
	}
}
void intern_ServerTx() {
	while (true) {
		//Send out all synced vectors
		if (vecsToSend.size() == 0) {
			continue;
		}
		for (int i = 0; i < vecsToSend.size(); i++) {
			string message = intern_MakeDataFromSVec(vecsToSend[i]);
			const char* buff = message.c_str();
			for (int i = 0; i < clientArray.size(); i++) {
				send(clientArray[i], buff, sizeof(message), 0);
			}
		}
		vecsToSend.clear();
	}
}
void extern_CloseServer() {
	if (netAuth != netAuthority::Server) {
		return;
	}
	intern_CloseClientArray();
	closesocket(mySocket);
	
	WSACleanup();
	netAuth = netAuthority::Offline;
	logsList.push_back(logs::ClientStarted);
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

	mySocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mySocket == INVALID_SOCKET) {
		//Log("Socket creation failed", FOREGROUND_RED);
		WSACleanup();
		logsList.push_back(logs::FATAL1);
		return;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);

	if (bind(mySocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		//Log("Bind failed", FOREGROUND_RED);

		closesocket(mySocket);
		WSACleanup();
		logsList.push_back(logs::FATAL2);
		return;
	}

	if (listen(mySocket, 5) == SOCKET_ERROR) {
		//Log("Listen failed", FOREGROUND_RED);
		closesocket(mySocket);
		WSACleanup();
		logsList.push_back(logs::FATAL3);
		return;
	}
	//Log("Server started, netAuth set to Server", FOREGROUND_GREEN);

	netAuth = netAuthority::Server;
	myId = 13;

	thread(&intern_ServerWaitNewClients).detach();//Thread for accepting new clients
	thread(&intern_ServerTx).detach();
	//thread(&intern_ServerRx).detach();
	logsList.push_back(logs::ServerStarted);
}
#pragma endregion
#pragma region Client
void intern_ClientRx() {
	while (true) {
		//Wait for incoming data to process
		char recvBuffer[RECVBUFF];
		int bytesRead = recv(mySocket, recvBuffer, sizeof(recvBuffer), 0);
		if (bytesRead > 0) {
			//recvBuffer[bytesRead] = '\0';
			string newData = string(recvBuffer);
			vecsToProcess.push_back(intern_MakeSVecFromData(newData));
		}
	}	
}
void intern_ClientTx() {
	while (true) {
		//Send out all synced vectors
		if (vecsToSend.size() == 0) {
			continue;
		}
		for (int i = 0; i < vecsToSend.size(); i++) {
			string message = intern_MakeDataFromSVec(vecsToSend[i]);
			const char* buff = message.c_str();
			send(mySocket, buff, sizeof(message), 0);

		}
		vecsToSend.clear();
	}
}
void extern_CloseClient() {

	//Log("Closing client", FOREGROUND_BLUE);
	if (netAuth != netAuthority::Client) {
		return;
	}
	closesocket(mySocket);
	WSACleanup();
	netAuth = netAuthority::Offline;
	logsList.push_back(logs::ClientEnded);
}
void extern_SetupClient() {

	if (netAuth != netAuthority::Offline) {
		return;
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		logsList.push_back(logs::FATAL0);
		return;
	}

	mySocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mySocket == INVALID_SOCKET) {
		closesocket(mySocket);
		WSACleanup();
		logsList.push_back(logs::FATAL1);

		return;
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);


	//Convert IP address to wide string and use InetPtonW
	if (InetPtonW(AF_INET, serverIp, &serverAddr.sin_addr) != 1) {
		closesocket(mySocket);
		WSACleanup();
		logsList.push_back(logs::FATAL2);

		return;
	}

	if (connect(mySocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(mySocket);
		WSACleanup();
		logsList.push_back(logs::FATAL3);

		return;
	}



	//Wait until we recieve our ID
	while (myId == -1) {
		char recvBuffer[1024];
		int bytesRead = recv(mySocket, recvBuffer, sizeof(recvBuffer), 0);
		string newData = string(recvBuffer);

		myId = stoi(newData);
	}
	netAuth = netAuthority::Client;
	logsList.push_back(logs::IdSet);

	//thread t = thread(&extern_ClientTick);
	//t.detach();


	thread(&intern_ClientRx).detach();//Thread for accepting new clients
	thread(&intern_ClientTx).detach();//Thread for accepting new clients

	logsList.push_back(logs::ClientStarted);

}
#pragma endregion

