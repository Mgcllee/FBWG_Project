#pragma once
#include "stdafx.h"
#include "Stage.h"
#include "Timer.h"
#include<time.h>


array<threadInfo, 3> threadHandles;
array<char, 3> playerRole = { 'f', 'f', 'f' };
mutex selectMutex;
array<char, 3> selectPlayerRole = { 'n', 'n', 'n' };
HANDLE multiEvenTthreadHadle[3];

int stageIndex = -1;

Stage StageMgr;

DWORD WINAPI ClientWorkThread(LPVOID arg);
DWORD WINAPI ServerWorkThread(LPVOID arg);

void TimeoutStage();
void StageTimerStart();


Timer _timer;

double timeoutSeconds = 60 * 5;

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

	if (::bind(listenSocket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
		Display_Err(WSAGetLastError());
		return 1;
	}

	if (listen(listenSocket, 1) == SOCKET_ERROR) { // 연결할 클라이언트는 1개
		Display_Err(WSAGetLastError());
		return 1;
	}

	for (int i = 0; i < 3; i++)
	{
		SOCKADDR_IN cl_addr;
		int addr_size = sizeof(cl_addr);
		threadHandles[i].clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&cl_addr), &addr_size);
		if (threadHandles[i].clientSocket == INVALID_SOCKET) {
			Display_Err(WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		cout << "Accept Client[" << i << "]" << endl;
		threadHandles[i].jumpEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
		ResetEvent(threadHandles[i].jumpEventHandle);

		S2CPlayerPacket loadPacket;
		loadPacket.type = S2CLoading;
		loadPacket.id = i;
		threadHandles[i].clientId = i;
		threadHandles[i].threadHandle = CreateThread(NULL, 0, ClientWorkThread, reinterpret_cast<LPVOID>(i), 0, NULL);
		multiEvenTthreadHadle[i] = threadHandles[i].threadHandle;
		send(threadHandles[i].clientSocket, (char*)&loadPacket, sizeof(S2CPlayerPacket), 0);//loading 패킷을 로그인 패킷으로 생각

		loadPacket.type = S2CAddPlayer;
		for (int j = 0; j < 3; j++) {
			if (i != j) {
				if (threadHandles[j].threadHandle != NULL) {
					send(threadHandles[j].clientSocket, (char*)&loadPacket, sizeof(S2CPlayerPacket), 0);//다른 Player 정보 패킷으로 생각 // j들한테 i의 정보를

					S2CPlayerPacket addPlayerPacket;
					addPlayerPacket.type = S2CAddPlayer;
					addPlayerPacket.id = j;
					send(threadHandles[i].clientSocket, (char*)&addPlayerPacket, sizeof(S2CPlayerPacket), 0);//다른 Player 정보 패킷으로 생각 // i한테 j의 정보를
				}
			}
		}

		if (i == 2) {
			S2CChangeStagePacket changePacket;
			changePacket.stageNum = STAGE_ROLE;
			changePacket.type = S2CChangeStage;

			for (int x = 0; x < 3; x++) {
				send(threadHandles[x].clientSocket, (char*)&changePacket, sizeof(S2CChangeStagePacket), 0);
			}
			stageIndex = STAGE_ROLE;
		}
	}

	HANDLE serverThread = CreateThread(NULL, 0, ServerWorkThread, reinterpret_cast<LPVOID>(1), 0, NULL);

	while (WSA_WAIT_EVENT_0 + 2 != WSAWaitForMultipleEvents(3, multiEvenTthreadHadle, TRUE, WSA_INFINITE, FALSE)) {}
	for (int j = 0; j < 3; j++) {
		CloseHandle(threadHandles[j].threadHandle);
	}
	CloseHandle(serverThread);


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

void ConstructPacket(threadInfo& clientInfo, int ioSize)
{
	int restSize = ioSize + clientInfo.prevSize;
	int needSize = 0;
	char* buf = clientInfo.recvBuf;
	while (restSize != 0) {
		needSize = GetPacketSize(reinterpret_cast<char*>(buf)[0]);
		if (restSize < needSize) {
			clientInfo.prevSize = restSize;
			return;
		}
		else {
			ProcessPacket(clientInfo, reinterpret_cast<char*>(buf));
			memcpy(buf, reinterpret_cast<char*>(buf) + needSize, restSize - needSize);
			restSize -= needSize;
		}
	}
}

DWORD WINAPI ClientWorkThread(LPVOID arg)
{
	//WaitForSingleObject(loadFlag, INFINITE);

	int myIndex = reinterpret_cast<int>(arg);
	while (true) {
		int recvRetVal = recv(threadHandles[myIndex].clientSocket, threadHandles[myIndex].recvBuf + threadHandles[myIndex].prevSize, MAX_BUF_SIZE - threadHandles[myIndex].prevSize, 0);
		if (recvRetVal != 0) {
			ConstructPacket(threadHandles[myIndex], recvRetVal);
		}
	}
	return 0;
}

DWORD WINAPI ServerWorkThread(LPVOID arg)
{
	while (true) {
		if (stageIndex == STAGE_ROLE) {
			bool isFinish = true;
			for (int i = 0; i < 3; i++) {
				if (selectPlayerRole[i] == 'n') {
					isFinish = false;
					break;
				}
			}
			if (isFinish) {
				//Send All Cleint Next Stage == Stage01

				// Stage 1 의 정보 획득
				StageMgr.Stage_1();
				// 최초 위치 설정
				MovePacket setPosition;
				setPosition.type = S2CMove;
				for (int i = 0; i < 3; ++i) {
					setPosition.id = i;
					setPosition.x = threadHandles[i].x;
					setPosition.y = threadHandles[i].y;
					for (int j = 0; j < 3; ++j) {
						send(threadHandles[j].clientSocket, reinterpret_cast<char*>(&setPosition), sizeof(MovePacket), 0);
					}
				}

				S2CChangeStagePacket changePacket;
				changePacket.stageNum = STAGE_01;
				changePacket.type = S2CChangeStage;
				for (int x = 0; x < 3; x++) {
					send(threadHandles[x].clientSocket, (char*)&changePacket, sizeof(S2CChangeStagePacket), 0);
				}
				stageIndex = STAGE_01;
				StageTimerStart();
			}
		}
		else if (stageIndex == STAGE_01) {
			for (int i = 0; i < 3; i++) {
				DWORD retVal = WaitForSingleObject(threadHandles[i].jumpEventHandle, 0);
				if (retVal == WAIT_OBJECT_0) {
					if (!threadHandles[i].isJump) {
						threadHandles[i].isJump = true;
						threadHandles[i].jumpStartTime = high_resolution_clock::now();
						threadHandles[i].jumpCurrentTime = high_resolution_clock::now();
						cout << "JumpStart Time: " << threadHandles[i].jumpStartTime.time_since_epoch().count() << endl;
						threadHandles[i].y += threadHandles[i].g;
						/*if (threadHandles[i].v < 30.f) {
							threadHandles[i].v += threadHandles[i].g;
							threadHandles[i].y -= threadHandles[i].v;
						}
						else {
							threadHandles[i].v = 0;
						}*/
						MovePacket mPacket;
						mPacket.id = threadHandles[i].clientId;
						mPacket.type = S2CMove;
						mPacket.x = threadHandles[i].x;
						mPacket.y = threadHandles[i].y;
						for (int j = 0; j < 3; j++) {
							send(threadHandles[j].clientSocket, reinterpret_cast<char*>(&mPacket), sizeof(MovePacket), 0);
						}
					}
					else {
						auto startDuration = high_resolution_clock::now() - threadHandles[i].jumpStartTime;
						auto currentDuration = high_resolution_clock::now() - threadHandles[i].jumpCurrentTime;
						cout << "current Duration Time: " << duration_cast<milliseconds>(currentDuration).count() << endl;

						if (duration_cast<milliseconds>(startDuration).count() > 300) {
							cout << "Over Time: " << duration_cast<milliseconds>(startDuration).count() << endl;
							cout << "Fall Time: " << duration_cast<milliseconds>(currentDuration).count() << endl;
							if (duration_cast<milliseconds>(startDuration).count() > 800) {
								ResetEvent(threadHandles[i].jumpEventHandle);
								threadHandles[i].isJump = false;
							}
							else if (duration_cast<milliseconds>(currentDuration).count() > 30) {
								threadHandles[i].y += 20/*threadHandles[i].g*/;
								cout << "Fall Time: " << currentDuration.count() << endl;
								cout << "client y cordinate: " << threadHandles[i].y << endl;
								threadHandles[i].jumpCurrentTime = high_resolution_clock::now();
							}
						}
						else if (duration_cast<milliseconds>(currentDuration).count() > 30) {
							threadHandles[i].y -= /*threadHandles[i].v*/10;
							//계속 점프
						/*	if (threadHandles[i].v < 30.f) {
								threadHandles[i].v += threadHandles[i].g;
							}*/
							/*else {
								threadHandles[i].v = 0;
							}*/
							cout << "Jump Time: " << duration_cast<milliseconds>(startDuration).count() << endl;
							cout << "client y cordinate: " << threadHandles[i].y << endl;
							threadHandles[i].jumpCurrentTime = high_resolution_clock::now();
						}
						MovePacket mPacket;
						mPacket.id = threadHandles[i].clientId;
						mPacket.type = S2CMove;
						mPacket.x = threadHandles[i].x;
						mPacket.y = threadHandles[i].y;
						for (int j = 0; j < 3; j++) {
							send(threadHandles[j].clientSocket, reinterpret_cast<char*>(&mPacket), sizeof(MovePacket), 0);
						}
					}

				}
			}
		}
	}
	return 0;
}

void StageTimerStart()
{
	if (_timer.IsRunning() == true)
	{
		return;
	}

	_timer.Start(std::chrono::milliseconds(1000), [=]
		{
			S2CStageTimePassPacket packet;
			packet.timePassed = _timer.GetElapsedTime() / (double)1000;

			for (int x = 0; x < 3; x++) {
				send(threadHandles[x].clientSocket, (char*)&packet, sizeof(S2CStageTimePassPacket), 0);
			}

			if (timeoutSeconds <= packet.timePassed)
			{
				TimeoutStage();
				typePacket timeoutPacket;
				for (int x = 0; x < 3; x++) {
					send(threadHandles[x].clientSocket, (char*)&timeoutPacket, sizeof(typePacket), 0);
				}
			}
		});
}

void TimeoutStage()
{
	_timer.Stop();
}

void ProcessPacket(threadInfo& clientInfo, char* packetStart) // 아직 쓰지않는 함수 - recv()하면서 불러줌
{
	//changePacket() => send S2CChangeRolePacket
	//selectPacket() => mutex Role container and send S2CSelectPacket
	//movePacket(); => 여기서 충돌 체크, 보석 체크 => 여기서 보석을 다 먹었다면 두 클라이언트에게 문 여는 패킷 전송, 문 들어가라는 패킷도 전송해야되네
	if (packetStart == nullptr)
		return;
	switch (reinterpret_cast<char*>(packetStart)[0]) {
	case C2SSelectRole:
	{
		C2SRolePacket* packet = reinterpret_cast<C2SRolePacket*>(packetStart);
		bool change = true;
		//already Exist Role Check

		//non exist Role
		selectMutex.lock();
		for (int i = 0; i < 3; i++) { // 성능이 구릴려나? 상관 없나?
			if (selectPlayerRole[i] == packet->role) {
				change = false;
				break;
			}
		}
		if (change)
			selectPlayerRole[clientInfo.clientId] = packet->role;
		selectMutex.unlock();
		if (change) {
			//send SelectPacket for all Client
			S2CRolePacket sendPacket;
			sendPacket.id = clientInfo.clientId;
			sendPacket.role = packet->role;
			sendPacket.type = S2CSelectRole;
			for (int i = 0; i < 3; i++) {
				send(threadHandles[i].clientSocket, reinterpret_cast<char*>(&sendPacket), sizeof(S2CRolePacket), 0);
			}
		}
	}
	break;
	case C2SChangRole:
	{
		C2SRolePacket* packet = reinterpret_cast<C2SRolePacket*>(packetStart);
		playerRole[clientInfo.clientId] = packet->role; // 캐릭터 둘러보는 것 정도는 상호배제 필요 없다고 생각됨
		//send changePacekt for all Client
		S2CRolePacket sendPacket;
		sendPacket.id = clientInfo.clientId;
		sendPacket.role = packet->role;
		sendPacket.type = S2CChangeRole;
		for (int i = 0; i < 3; i++) {
			send(threadHandles[i].clientSocket, reinterpret_cast<char*>(&sendPacket), sizeof(S2CRolePacket), 0);
		}
	}
	break;
	case C2SMove:
	{
		MovePacket* packet = reinterpret_cast<MovePacket*>(packetStart);
		packet->type = S2CMove;
		if (packet->y == SHRT_MAX) {
			//점프 이벤트 주자
			DWORD retVal = WaitForSingleObject(clientInfo.jumpEventHandle, 0);
			if (retVal == WAIT_OBJECT_0) {
				return;
			}
			SetEvent(clientInfo.jumpEventHandle);
			return;
		}
		if (clientInfo.wid_a <= 10.f)
			clientInfo.wid_a += 0.1f;
		if (clientInfo.wid_v <= 10.f)
			clientInfo.wid_v += clientInfo.wid_a;

		if (packet->x == 1) {
			clientInfo.x += clientInfo.wid_v;
		}
		if (packet->x == -1) {
			clientInfo.x -= clientInfo.wid_v;
		}
		//else if (packet->y == SHRT_MIN) {
		//	if (clientInfo.v < 30.f) {
		//		clientInfo.v += clientInfo.g;
		//		clientInfo.y += clientInfo.v;
		//	}
		//	else {
		//		clientInfo.v = 0.f;
		//		// clientInfo.y = clientInfo.ground;
		//	}
		//}

		if (packet->x == 0 && packet->y == 0) {
			// 캐릭터 속도, 가속도 초기화
			clientInfo.wid_v = 0.f;
			clientInfo.wid_a = 0.f;
		}

		packet->x = clientInfo.x;
		packet->y = clientInfo.y;

		for (int i = 0; i < 3; i++) {
			send(threadHandles[i].clientSocket, reinterpret_cast<char*>(packet), sizeof(MovePacket), 0);
		}
	}
	break;
	case C2SExitGame:
	{

	}
	break;
	case C2SRetry:
	{

	}
	break;
	default:
		// Packet Error
		break;
	}
}

int GetPacketSize(char packetType)
{
	int retVal = -1;
	switch (packetType)
	{
	case C2SChangRole:
	case C2SSelectRole:
		retVal = sizeof(C2SRolePacket);
		break;
	case C2SMove:
		retVal = sizeof(MovePacket);
		break;
	case C2SRetry:
	case C2SExitGame:
		retVal = sizeof(typePacket);
		break;
	default:
		break;
	}
	return retVal;
}
