#include<iostream>
#include<WS2tcpip.h>
#include<array>
#include<mutex>
#include"protocol.h"

#pragma comment(lib, "ws2_32")

using namespace std;

void Display_Err(int Errcode);
void ConstructPacket(char* recvPacket); // ��Ŷ ������
void ProcessPacket(char* packetStart); // ��Ŷ ������ ��, ���� �ؼ� �� �ൿ
void ChangeRole(); // mutex �ʿ� ������? => change�� ���� ���� ���ٰ� ������
void SelectRole(); // mutex �ʿ� => �� Ŭ���̾�Ʈ�� ���ÿ� ���� �ɸ��� ������ �ع����� �ȵ�
void MovePacket(); // ������ �� ����, ����
void CheckJewelryEat();// ��� ���� Ȯ��
void CheckOpenDoor(); // �� ������ ���� Ȯ��


DWORD WINAPI ClientWorkThread(LPVOID arg);
struct MyThread
{
    int iIndex = 0;
    SOCKET sock = 0;
};


struct threadInfo {
	HANDLE h = NULL;
	SOCKET clientSocket;
};
array<threadInfo, 3> threadHandles;

//HANDLE loadFlag; =>����Ʈ���Ǳ�

//map<socket, Role> �ϴ°� ������?

int main(int argv, char** argc)
{
	wcout.imbue(std::locale("korean"));


	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		Display_Err(WSAGetLastError());
		return 1;
	}
	
	//loadFlag= CreateEvent(NULL, FALSE, FALSE, NULL);

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		Display_Err(WSAGetLastError());
		return 1;
	}

	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
		Display_Err(WSAGetLastError());
		return 1;
	}

	if (listen(listenSocket, 1) == SOCKET_ERROR) { // ������ Ŭ���̾�Ʈ�� 1��
		Display_Err(WSAGetLastError());
		return 1;
	}
	
	array<SOCKET, 3> clientSocket;

	for (int i = 0; i < 3; i++)
	{
		SOCKADDR_IN cl_addr;
		int addr_size = sizeof(cl_addr);
		clientSocket[i] = accept(listenSocket, reinterpret_cast<sockaddr*>(&cl_addr), &addr_size);
		if (clientSocket[i] == INVALID_SOCKET) {
			Display_Err(WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		
		S2CLoadingPacket load1;
		load1.type = S2CLoading;
		load1.id = i;
		
		
		threadHandles[i].h = CreateThread(NULL, 0, ClientWorkThread, reinterpret_cast<LPVOID>(clientSocket[i]), 0, NULL);//���ڰ� clientSocket�� �ƴ� �� ����
		threadHandles[i].clientSocket = clientSocket[i];
		
		
		
		if (i == 0) {
			send(clientSocket[i], (char*)&load1, sizeof(S2CLoadingPacket), 0);
		}
		if (i == 1) {
			send(clientSocket[i], (char*)&load1, sizeof(S2CLoadingPacket), 0);
		}
		if (i == 2) {
			
			S2CChangeStagePacket change1;
			change1.stageNum = 0;
			change1.type = S2CChangeStage;

			for(int x=0; x<3; x++)
				send(clientSocket[i], (char*)&change1, sizeof(S2CChangeStage), 0);
		}
		
	}

	closesocket(listenSocket);
	WSACleanup();

}

void Display_Err(int Errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, Errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 0, NULL);
	wcout << "ErrorCode: " << Errcode << " - " << (WCHAR*)lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

DWORD WINAPI ClientWorkThread(LPVOID arg)
{
	//WaitForSingleObject(loadFlag, INFINITE);


	SOCKET socket = *reinterpret_cast<SOCKET*>(arg);
	while (true) {
		//recv();
	}
	return 0;
}

void ProcessPacket(char* packetStart) // ���� �����ʴ� �Լ� - recv()�ϸ鼭 �ҷ���
{

	//changePacket() => send S2CChangeRolePacket
	//selectPacket() => mutex Role container and send S2CSelectPacket
	//movePacket(); => ���⼭ �浹 üũ, ���� üũ => ���⼭ ������ �� �Ծ��ٸ� �� Ŭ���̾�Ʈ���� �� ���� ��Ŷ ����, �� ����� ��Ŷ�� �����ؾߵǳ�
}
