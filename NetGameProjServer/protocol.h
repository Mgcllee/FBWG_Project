#pragma once
#include<wtypes.h>

#define S2CLoading 0
#define S2CChangeStage 1
#define S2CJewelryVisibility 2
//RolePacket ����
#define C2SSelectRole 3// �ɸ��� ���ý� Ŭ���̾�Ʈ => ���� �� ���� ����
#define S2CSelectRole 4// ���� => Ŭ���̾�Ʈ ���� ������ ����
#define C2SChangRole 5 // �ٸ� Ŭ���̾�Ʈ�� � ������ ���� �ִ��� �˷��ֱ� ���� ��Ŷ
#define S2CChangeRole 6// ������ �ƴ� �ɸ��� �� ���� �ִ��� �ٸ� Ŭ���̾�Ʈ�� �˾ƾ� �ϴϱ� �ִ� ��Ŷ
//MovePacket ����
#define C2SMove 7
#define S2CMove 8
//type ����
#define C2SExitGame 9// Ŭ���̾�Ʈ�� ���� ���Ḧ ���� ��, ����
#define S2CExitGame 10// �������� �ٸ� Ŭ���̾�Ʈ���� ���� �˸�
#define C2SRetry 11// Ŭ���̾�Ʈ�� �������� �̹� �������� ��õ� ��û ��Ŷ
#define S2CDoorOpen 12// �������°� �˸��� ��Ŷ


#define PORT_NUM 9000

#pragma pack(push, 1)

struct S2CLoadingPacket { // �ٸ� Ŭ���̾�Ʈ ��ٸ������� ��Ŷ
	//unsigned char size;
	char type;
	char id; // Ŭ���̾�Ʈ ���� id ����
};

struct S2CChangeStagePacket { // ���������� �����ϱ� ���� ��Ŷ
	//unsigned char size;
	char type;
	short stageNum; // �������� ������ ���� ����
};

struct S2CJewelryVisibilityPacket { // �������� ������ �Ծ��ٰ� �Ǵ� ��, Ŭ���̾�Ʈ���� ������ ����
	//unsigned char size;
	char type;
	char jewelryType; // � ������ ��������
	int index; // ���° �ε����� ����� ��������
};

struct C2SRolePacket {
	//unsigned char size;
	char type;
	char role; //���� ����
};

struct S2CRolePacket {
	//unsigned char size;
	char type;
	char id; // � Ŭ���̾�Ʈ���� ������ ���� ����
	char role; // ���� ����
};

struct MovePacket { // ������ ���� ��Ŷ
	//unsigned char size;
	char type;
	char id; // Ŭ���̾�Ʈ ���� ��Ŷ
	float x, y; // ���� ��ġ
};

struct typePacket {
	//unsigned char size;
	char type;
};

#pragma pack(pop)