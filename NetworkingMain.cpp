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
using namespace std;
struct vec3 { float x = 0.F; float y = 0.F; float z = 0.F; };
enum axes { x, y, z };
enum NetAuthority { Offline, Server, Client };

vector<SOCKET> clientArray;
WSADATA wsaData;
SOCKET serverSocket;
SOCKET clientSocket;
sockaddr_in clientAddr;
sockaddr_in serverAddr;
NetAuthority netAuth;

__declspec(dllexport) vec3 trackedVector = vec3{ 0.F,0.F,0.F };

// Function prototypes for the functions to be exposed
extern "C" {
    __declspec(dllexport) void __stdcall SetupServer();
    __declspec(dllexport) void __stdcall SetupClient();
    __declspec(dllexport) void __stdcall CloseServer();
    __declspec(dllexport) void __stdcall CloseClient();
}

bool alive = true;
//A thread is created running this function per socket connected
//This function recieves incoming packets from clients, and redistributes them to all other clients.
void ServerClientListener() {
    //Log("ServerClientListener started", FOREGROUND_GREEN);

    //Log("ServerClientListener started #" + to_string(clientArray.size()), FOREGROUND_GREEN);

    while (1 > 0) {//Wait to recieve message
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        //buffer[bytesRead] = '\0';
        //Log("Received | " + *buffer, FOREGROUND_INTENSITY);
        //writeToSaveFile(buffer);
        // Broadcast the message to all other clients
        //for (int i = 0;i < clientArray.size(); i++) {
        //    SetColor(FOREGROUND_BLUE);
        //    cout << "Broadcasting message to client #" << i << endl;
        //    send(clientArray[i], buffer, bytesRead, 0);
        //}
    }
    cout << "ending client handler" << endl;
    // Remove client from list and close socket
    closesocket(clientSocket);
    clientArray.erase(remove(clientArray.begin(), clientArray.end(), clientSocket), clientArray.end());
}
int ServerWaitNewClients() {//Continously wait for new connections
    //Log("ServerWaitNewClients started", FOREGROUND_GREEN);
    int clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        //Log("Accept failed", FOREGROUND_RED);
        return 1;
    }
    //Log("New client connected, starting ServerClientListener for them", FOREGROUND_BLUE);
    clientArray.push_back(clientSocket);
    //thread t = thread(&ServerClientListener);
    //t.detach();
    return 0;
}
void ServerTick() {
    cout << "ServerTick() called" << endl;
    while (netAuth == NetAuthority::Server) {
        //Send out synced transforms
        string message = "V";
        vec3 testVector = vec3{ 1.1235F, 2.1233452F, 3.5234234F };
        message += "x";
        message += std::to_string(testVector.x);
        message += "?";//END OF NUM

        message += "y";
        message += std::to_string(testVector.y);
        message += "?";//END OF NUM

        message += "z";
        message += std::to_string(testVector.z);
        message += "?";//END OF NUM

        message += ";";//END MESSAGE
        const char* buff = message.c_str();
        for (int i = 0; i < clientArray.size(); i++) {
            send(clientArray[i], buff, sizeof(message), 0);
        }
        cout << ("[ServerTick] Sending message : " + message) << endl;
        //Output : "t1X1.00000Y1.00000Z1.00000"
    }
    cout << "ServerTick() exiting" << endl;
}
void SetupServer() {
    alive = true;
    cout << "SetupServer() called" << endl;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //Log("WSA Startup failed", FOREGROUND_RED);
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        //Log("Socket creation failed", FOREGROUND_RED);
        WSACleanup();
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        //Log("Bind failed", FOREGROUND_RED);

        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        //Log("Listen failed", FOREGROUND_RED);
        closesocket(serverSocket);
        WSACleanup();
        return;
    }
    //Log("Server started, netAuth set to Server", FOREGROUND_GREEN);

    netAuth = NetAuthority::Server;

    thread(&ServerWaitNewClients).detach();//Thread for accepting new clients
    thread(&ServerTick).detach();//Thread for sending out server info
    //while (alive) {}
    cout << "SetupServer() exiting" << endl;
}
void CloseServer() {
    closesocket(serverSocket);
    WSACleanup();
    netAuth = NetAuthority::Offline;
}
void ClientTick() {
    cout << "ClientTick() called" << endl;
    while (netAuth == NetAuthority::Client) {
        //Send stuff
        //send(clientSocket, message.c_str(), message.length(), 0);//Send to server

        //Recieve stuff
        char recvBuffer[1024];
        int bytesRead = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
        if (bytesRead > 0) {
            //recvBuffer[bytesRead] = '\0';
            string newData = string(recvBuffer);
            vec3 newVec = vec3{ 0.f,0.f,0.f };
            string curNum = "";
            axes axis = (axes)0;
            bool done = false;
            for (int i = 0; i < newData.length(); i++) {
                if (done) { break; }
                switch ((char)(newData[i]))
                {
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
                        newVec.x = stof(curNum);
                        break;
                    case(axes::y):
                        newVec.y = stof(curNum);
                        break;
                    case(axes::z):
                        newVec.z = stof(curNum);
                        break;
                    };
                    break;
                case ';'://End of message
                    done = true;
                    break;
                default://If the char can be parsed to an int, add it to whichever axis we are currently working
                    curNum += newData[i];
                    break;
                };
            }

            //cout<<"[ClientTick] : Received : " + string(recvBuffer)<<endl;
            trackedVector = newVec;
            cout << "[ClientTick] : Built Vec3 : X" << newVec.x << " Y" << newVec.y << " Z" << newVec.z << endl;
        }
    }
    cout << "ClientTick() exiting" << endl;
}
//Initialize a client
void SetupClient() {
    alive = true;
    cout << "SetupClient() called" << endl;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //Log("WSAStartup failed", FOREGROUND_RED);
        return;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        //Log("Socket creation failed", FOREGROUND_RED);
        WSACleanup();
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);

    //Convert IP address to wide string and use InetPtonW
    if (InetPtonW(AF_INET, (PCWSTR)L"127.0.0.1", &serverAddr.sin_addr) != 1) {
        //Log("InetPtonW failed", FOREGROUND_RED);
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        //Log("Connect failed", FOREGROUND_RED);
        closesocket(clientSocket);
        WSACleanup();
        return;
    }


    string joinedMessage = "Player connected to the server.";
    //string joinedMessage = (getTime() + username + " connected to the server.");
    send(clientSocket, joinedMessage.c_str(), joinedMessage.length(), 0);
    netAuth = NetAuthority::Client;
    //while (true) {//Client loop
    //    ClientLoop(clientSocket);
    //}
    thread t = thread(&ClientTick);
    t.detach();
    //while (alive) {}
    cout << "SetupClient() exiting" << endl;
}
void CloseClient() {

    //Log("Closing client", FOREGROUND_BLUE);
    closesocket(clientSocket);
    WSACleanup();
    netAuth = NetAuthority::Offline;
}
/*
int main()
{
    char c;
    cin >> c;
    if (c == 's') {
        SetupServer();
    }
    else {
        SetupClient();
    }
}*/