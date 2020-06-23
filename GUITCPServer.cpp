//GUITCPServer.cpp
#include <winsock2.h>
#include <cstdlib>
#include <stdio.h>
#include <ctime>

#define BUFSIZE 4056
#define MAXCLIENT 10

HANDLE hReadEvent, hWriteEvent;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void DisplayText(char *fmt, ...);

DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ProcessClient2(LPVOID arg);
DWORD WINAPI ProcessClient3(LPVOID arg);

void err_quit(char *msg);
void err_display(char *msg);
void Coordinate(int variavle, char* buf);

HINSTANCE hInst;
HWND hEdit;
CRITICAL_SECTION cs;

typedef struct
{
	unsigned int EnemyX;
	unsigned int EnemyY;
}EnemyData;

typedef struct
{
	unsigned int ClientX;
	unsigned int ClientY;
	unsigned int ClientItem;
	unsigned int PlayerDead;
	unsigned int GameState;
	unsigned int GameStart;
	unsigned int ShipFrame;
	unsigned int Missile;
	unsigned int MissileType;
	unsigned int MissileLevel;
	unsigned int Score;

	EnemyData ClientEnemy[5];
}ClientData;

ClientData ClientVar[5];

int angle=0;
int power=0;
int C1_X = 300, C1_Y=500;
int C2_X = 500, C2_Y=500;
int C1_item=0, C2_item=0;
int C1_PlayerDead=0, C2_PlayerDead=0;
int C1_GameState=0, C2_GameState=0;
int C1_GameStart=0, C2_GameStart=0;
int C1_ShipFrame=0, C2_ShipFrame=0;
int C1_Missile=0, C2_Missile=0;
int C1_MissileType=0, C2_MissileType=0;
int C1_MissileLevel=0, C2_MissileLevel=0;
int GameStart=0;
int clienttemp=0;
char client1recvbuf[BUFSIZE+1];
char client2recvbuf[BUFSIZE+1];
char client3recvbuf[BUFSIZE+1];

HWND hWnd;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{

	hInst = hInstance;
	InitializeCriticalSection(&cs);

	WNDCLASS wndclass;
	wndclass.cbClsExtra =0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hCursor = LoadIcon(NULL, IDC_ARROW);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hInstance = hInstance;
	wndclass.lpszClassName = "MyWindowClass";
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	if(!RegisterClass(&wndclass)) return -1;

	HWND hWnd = CreateWindow("MyWindowClass","TCP 서버", WS_OVERLAPPEDWINDOW, 0, 0, 600, 300, NULL, (HMENU)NULL, hInstance, NULL);
	if(hWnd == NULL) return -1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	DWORD ThreadId;
	CreateThread(NULL, 0, ServerMain, NULL, 0, &ThreadId);

	MSG msg;
	
	while(GetMessage(&msg, 0, 0, 0)>0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	

	DeleteCriticalSection(&cs);
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	switch(uMsg){
	case WM_CREATE:
		hEdit = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
			0, 0, 0, 0, hWnd, (HMENU)100, hInst, NULL);

		for(i=0; i<2; i++)
		{
			ClientVar[i].GameState	 = 0;
			ClientVar[i].GameStart	 = 0;
			ClientVar[i].ClientItem	 = 0;
			ClientVar[i].ClientX	 = 0;
			ClientVar[i].ClientY	 = 0;
			ClientVar[i].ShipFrame	 = 0;
			ClientVar[i].Missile	 = 0;
			ClientVar[i].MissileType = 0;
			ClientVar[i].MissileLevel= 0;
			ClientVar[i].ClientEnemy[0].EnemyX = 0;
			ClientVar[i].ClientEnemy[0].EnemyY = 0;
		}

		return 0;
	case WM_SIZE:
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	case WM_SETFOCUS:
//		SetFocus(hEdit);
		return 0;
	case WM_KEYDOWN:
		if(wParam == 'Y')
		{
			DisplayText("ClientVar[0].GameState=%d ClientVar[1].GameState=%d\r\n"
				, ClientVar[0].GameState, ClientVar[1].GameState);
			DisplayText("ClientVar[0].GameStart=%d ClientVar[1].GameStart=%d\r\n"
				, ClientVar[0].GameStart, ClientVar[1].GameStart);
			DisplayText("ClientVar[0].ClientItem=%d ClientVar[1].ClientItem=%d\r\n"
				, ClientVar[0].ClientItem, ClientVar[1].ClientItem);
			DisplayText("ClientVar[0].ClientEnemy[0].EnemyX=%d ClientVar[0].ClientEnemy[0].EnemyY=%d\r\n"
				, ClientVar[0].ClientEnemy[0].EnemyX, ClientVar[0].ClientEnemy[0].EnemyY);
			
		}

		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;
	

	WSADATA wsa;
	// 라이브러리 초기화
	// MAKEWORD 매크로 (하위바이트:메이저버전, 상위바이트:마이너버전)
	// 윈도즈 소켓 시스템 관련 정보를 반환할 WSADATA 구조체의 포인터
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) 
		return -1;

	// 속켓생성(AF_INET: Adress Family(인터넷 주소체제)
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");

	//주소정보 초기화
	SOCKADDR_IN serveraddr;// 주소체계에서 사용하는 구조체 선언
	ZeroMemory(&serveraddr, sizeof(serveraddr));// 0으로 초기화
	serveraddr.sin_family = AF_INET; //인터넛 프로토콜 체계
	serveraddr.sin_port = htons(9000); //포트번호 
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // 아이피 주소

	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");

	DisplayText("서버생성!! 클라이언트 접속을 기다립니다.!!\r\n");

	SOCKET client_sock[MAXCLIENT];
	SOCKADDR_IN clientaddr[MAXCLIENT];
	int addrlen;
	HANDLE hThread[MAXCLIENT];
	DWORD Threadld[MAXCLIENT];

	while(1){
		addrlen = sizeof(clientaddr[clienttemp]);

		client_sock[clienttemp] = accept(listen_sock, (SOCKADDR*)&clientaddr[clienttemp], &addrlen);
		if(client_sock[clienttemp] == INVALID_SOCKET){
			err_display("accept()");
			continue;
		}

		DisplayText("[TCP 서버] 클라이언트[%d] 접속: IP 주소 =%s, 포트 번호=%d\r\n", 
			clienttemp, inet_ntoa(clientaddr[clienttemp].sin_addr), ntohs(clientaddr[clienttemp].sin_port));
		

		if(clienttemp==1)
		{
			hThread[1] = CreateThread(NULL, 0, ProcessClient2, (LPVOID)client_sock[1], 0, &Threadld[1]);
			if(hThread[1] == NULL)
				DisplayText("[오류]스레드 생성 실패!\r\n");
			else
			{
				clienttemp++;
				CloseHandle(hThread[1]);
			}
		}
		else if(clienttemp==0)
		{
			hThread[0] = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock[0], 0, &Threadld[0]);
			if(hThread[0] == NULL)
				DisplayText("[오류]스레드 생성 실패!\r\n");
			else
			{
				clienttemp++;
				CloseHandle(hThread[0]);
			}
		}
		else if(clienttemp==2)
		{
			hThread[2] = CreateThread(NULL, 0, ProcessClient3, (LPVOID)client_sock[2], 0, &Threadld[2]);
			if(hThread[2] == NULL)
				DisplayText("[오류]스레드 생성 실패!\r\n");
			else
			{
				clienttemp++;
				CloseHandle(hThread[2]);
			}
		}


		
		DisplayText("clienttemp=%d\r\n",clienttemp);
	}

	closesocket(listen_sock); //소켓 해제

	WSACleanup(); //윈속종료
	return 0;
}


DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	char recvbuf1[BUFSIZE+1];
	SOCKADDR_IN clientaddr;
	int addrlen;
	int retval;

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

	while(1){

		WaitForSingleObject(hWriteEvent,INFINITE);

		retval = recv(client_sock, recvbuf1, 1048, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		}
		else if(retval==0)
			break;


		ClientVar[0].GameState	 = ((int)(recvbuf1[0]))-48;
		ClientVar[0].GameStart	 = ((int)(recvbuf1[1]))-48;
		ClientVar[0].ClientItem	 = ((int)(recvbuf1[2]))-48;
		ClientVar[0].ClientX	 = (((int)(recvbuf1[3]))-48)*100 + (((int)(recvbuf1[4]))-48)*10 + (((int)(recvbuf1[5]))-48);
		ClientVar[0].ClientY	 = (((int)(recvbuf1[6]))-48)*100 + (((int)(recvbuf1[7]))-48)*10 + (((int)(recvbuf1[8]))-48);
		ClientVar[0].ShipFrame	 = ((int)(recvbuf1[9]))-48;
		ClientVar[0].Missile	 = ((int)(recvbuf1[10]))-48;
		ClientVar[0].MissileType = ((int)(recvbuf1[11]))-48;
		ClientVar[0].MissileLevel= ((int)(recvbuf1[12]))-48;
		ClientVar[0].Score		 = (((int)(recvbuf1[13]))-48)*10000 + (((int)(recvbuf1[14]))-48)*1000 + (((int)(recvbuf1[15]))-48)*100 + (((int)(recvbuf1[16]))-48)*10 + (((int)(recvbuf1[17]))-48);
		ClientVar[0].PlayerDead    = ((int)(recvbuf1[18]))-48;

		if(ClientVar[0].GameStart==1 && ClientVar[1].GameStart==1)
			GameStart=1;

		
		*client1recvbuf='1';

		itoa(ClientVar[1].GameState, client1recvbuf, 10);
		itoa(GameStart, client1recvbuf+1, 10);
		itoa(ClientVar[1].ClientItem, client1recvbuf+2, 10);
		Coordinate(ClientVar[1].ClientX,client1recvbuf+3);
		Coordinate(ClientVar[1].ClientY,client1recvbuf+6);
		itoa(ClientVar[1].Score, client1recvbuf+9, 10);
		itoa(ClientVar[1].PlayerDead, client1recvbuf+14, 10);

		/*
		itoa(ClientVar[1].GameState, client1recvbuf, 10);
		client1recvbuf[1] = ',';
		itoa(GameStart, client1recvbuf+2, 10);
		client1recvbuf[3] = ',';
		itoa(ClientVar[1].ClientItem, client1recvbuf+4, 10);
		client1recvbuf[5] = ',';
		Coordinate(ClientVar[1].ClientX,client1recvbuf+6);
		client1recvbuf[9] = ',';
		Coordinate(ClientVar[1].ClientY,client1recvbuf+10);
		client1recvbuf[13] = ',';
		*/


		retval=send(client_sock,client1recvbuf,strlen(client1recvbuf),0);
		if(strlen(client1recvbuf)==0){
			err_display("send()");
			SetEvent(hReadEvent);
			continue;
		}

		SetEvent(hReadEvent);
	}

	closesocket(client_sock);
	C1_item=0;
	C2_item=0;
	DisplayText("C1_item=%d C2_item=%d\r\n", C1_item, C2_item);
	clienttemp--;
	DisplayText("[TCP 서버] 클라이언트 종료 : IP 주소=%s, 포트 번호=%d\r\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	DisplayText("clienttemp=%d\r\n",clienttemp);

	return 0;
}

DWORD WINAPI ProcessClient2(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	char recvbuf2[BUFSIZE+1];
	SOCKADDR_IN clientaddr;
	int addrlen;
	int retval;

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

	while(1){

		WaitForSingleObject(hWriteEvent,INFINITE);

		retval = recv(client_sock, recvbuf2, 1048, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		}
		else if(retval==0)
			break;

		ClientVar[1].GameState	 = ((int)(recvbuf2[0]))-48;
		ClientVar[1].GameStart	 = ((int)(recvbuf2[1]))-48;
		ClientVar[1].ClientItem	 = ((int)(recvbuf2[2]))-48;
		ClientVar[1].ClientX	 = (((int)(recvbuf2[3]))-48)*100 + (((int)(recvbuf2[4]))-48)*10 + (((int)(recvbuf2[5]))-48);
		ClientVar[1].ClientY	 = (((int)(recvbuf2[6]))-48)*100 + (((int)(recvbuf2[7]))-48)*10 + (((int)(recvbuf2[8]))-48);
		ClientVar[1].ShipFrame	 = ((int)(recvbuf2[9]))-48;
		ClientVar[1].Missile	 = ((int)(recvbuf2[10]))-48;
		ClientVar[1].MissileType = ((int)(recvbuf2[11]))-48;
		ClientVar[1].MissileLevel= ((int)(recvbuf2[12]))-48;
		ClientVar[1].Score		 = (((int)(recvbuf2[13]))-48)*10000 + (((int)(recvbuf2[14]))-48)*1000 + (((int)(recvbuf2[15]))-48)*100 + (((int)(recvbuf2[16]))-48)*10 + (((int)(recvbuf2[17]))-48);
		ClientVar[1].PlayerDead    = ((int)(recvbuf2[18]))-48;


		if(ClientVar[0].GameStart==1 && ClientVar[1].GameStart==1)
			GameStart=1;

		*client2recvbuf = '1';

		itoa(ClientVar[0].GameState, client2recvbuf, 10);
		itoa(GameStart, client2recvbuf+1, 10);
		itoa(ClientVar[0].ClientItem, client2recvbuf+2, 10);
		Coordinate(ClientVar[0].ClientX,client2recvbuf+3);
		Coordinate(ClientVar[0].ClientY,client2recvbuf+6);
		itoa(ClientVar[0].Score, client1recvbuf+9, 10);
		itoa(ClientVar[0].PlayerDead, client1recvbuf+14, 10);
		/*
		itoa(ClientVar[0].GameState, client2recvbuf, 10);
		client2recvbuf[1] = ',';
		itoa(GameStart, client2recvbuf+2, 10);
		client2recvbuf[3] = ',';
		itoa(ClientVar[0].ClientItem, client2recvbuf+4, 10);
		client2recvbuf[5] = ',';
		Coordinate(ClientVar[0].ClientX,client2recvbuf+6);
		client2recvbuf[9] = ',';
		Coordinate(ClientVar[0].ClientY,client2recvbuf+10);
		client2recvbuf[13] = ',';
*/
	

		retval=send(client_sock,client2recvbuf,strlen(client2recvbuf),0);
		if(strlen(client2recvbuf)==0){
			err_display("send()");
			SetEvent(hReadEvent);
			continue;
		}

		SetEvent(hReadEvent);
	}

	closesocket(client_sock);
	clienttemp--;
	C1_item=0;
	C2_item=0;
	DisplayText("C1_item=%d C2_item=%d\r\n", C1_item, C2_item);
	DisplayText("[TCP 서버] 클라이언트 종료 : IP 주소=%s, 포트 번호=%d\r\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	DisplayText("clienttemp=%d",clienttemp);

	return 0;
}

DWORD WINAPI ProcessClient3(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	char recvbuf2[BUFSIZE+1];
	SOCKADDR_IN clientaddr;
	int addrlen;
	int retval;

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

	while(1){

		WaitForSingleObject(hWriteEvent,INFINITE);

		retval = recv(client_sock, recvbuf2, 512, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		}
		else if(retval==0)
			break;
		
		*client3recvbuf = '1';

/*
		Coordinate(ClientVar[0].ClientX,client3recvbuf);
		Coordinate(ClientVar[0].ClientY,client3recvbuf+3);
		Coordinate(ClientVar[1].ClientX,client3recvbuf+6);
		Coordinate(ClientVar[1].ClientY,client3recvbuf+9);
		itoa(ClientVar[0].ShipFrame, client3recvbuf+10, 10);
		itoa(ClientVar[0].Missile, client3recvbuf+11, 10);
		itoa(ClientVar[0].MissileType, client3recvbuf+12, 10);
		itoa(ClientVar[0].MissileLevel, client3recvbuf+13, 10);
		itoa(ClientVar[1].ShipFrame, client3recvbuf+14, 10);
		itoa(ClientVar[1].Missile, client3recvbuf+15, 10);
		itoa(ClientVar[1].MissileType, client3recvbuf+16, 10);
		itoa(ClientVar[1].MissileLevel, client3recvbuf+17, 10);

		Coordinate(ClientVar[0].ClientX,client3recvbuf+18);
		Coordinate(ClientVar[0].ClientX,client3recvbuf+21);
		Coordinate(ClientVar[0].ClientX,client3recvbuf+24);
		Coordinate(ClientVar[0].ClientX,client3recvbuf+27);
		Coordinate(ClientVar[0].ClientX,client3recvbuf+30);

		Coordinate(ClientVar[0].ClientY,client3recvbuf+33);
		Coordinate(ClientVar[0].ClientY,client3recvbuf+36);
		Coordinate(ClientVar[0].ClientY,client3recvbuf+39);
		Coordinate(ClientVar[0].ClientY,client3recvbuf+42);
		Coordinate(ClientVar[0].ClientY,client3recvbuf+45);
*/
		
		Coordinate(ClientVar[0].ClientX,client3recvbuf);
		client3recvbuf[3] = ',';
		Coordinate(ClientVar[0].ClientY,client3recvbuf+4);
		client3recvbuf[7] = ',';
		Coordinate(ClientVar[1].ClientX,client3recvbuf+8);
		client3recvbuf[11] = ',';
		Coordinate(ClientVar[1].ClientY,client3recvbuf+12);
		client3recvbuf[15] = ',';
		itoa(ClientVar[0].ShipFrame, client3recvbuf+16, 10);
		client3recvbuf[17] = ',';
		itoa(ClientVar[0].Missile, client3recvbuf+18, 10);
		client3recvbuf[19] = ',';
		itoa(ClientVar[0].MissileType, client3recvbuf+20, 10);
		client3recvbuf[21] = ',';
		itoa(ClientVar[0].MissileLevel, client3recvbuf+22, 10);
		client3recvbuf[23] = ',';
		itoa(ClientVar[1].ShipFrame, client3recvbuf+24, 10);
		client3recvbuf[25] = ',';
		itoa(ClientVar[1].Missile, client3recvbuf+26, 10);
		client3recvbuf[27] = ',';
		itoa(ClientVar[1].MissileType, client3recvbuf+28, 10);
		client3recvbuf[29] = ',';
		itoa(ClientVar[1].MissileLevel, client3recvbuf+30, 10);
	/*	client3recvbuf[31] = ',';

		Coordinate(ClientVar[0].ClientEnemy[0].EnemyX,client3recvbuf+32);
		client3recvbuf[35] = ',';
		Coordinate(ClientVar[0].ClientEnemy[0].EnemyY,client3recvbuf+36);
		*/
	

		retval=send(client_sock,client3recvbuf,strlen(client3recvbuf),0);
		if(strlen(client3recvbuf)==0){
			err_display("send()");
			SetEvent(hReadEvent);
			continue;
		}

		SetEvent(hReadEvent);
	}

	closesocket(client_sock);
	clienttemp--;
	C1_item=0;
	C2_item=0;
	DisplayText("C1_item=%d C2_item=%d\r\n", C1_item, C2_item);
	DisplayText("[TCP 서버] 클라이언트 종료 : IP 주소=%s, 포트 번호=%d\r\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	DisplayText("clienttemp=%d",clienttemp);

	return 0;
}


void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg,fmt);

	char cbuf[BUFSIZE+512];
	for(int i=0; i<BUFSIZE+512; i++)
	{
		cbuf[i]=NULL;
		if(cbuf[i]!=NULL)
			cbuf[i]=NULL;
	}
	vsprintf(cbuf, fmt, arg);


	EnterCriticalSection(&cs);
	int nLength = GetWindowTextLength(hEdit);
	SendMessage(hEdit, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs);
	va_end(arg);
}


void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s]%s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void Coordinate(int variavle, char* buf)
{
	if(variavle<100)
	{
		itoa(0, buf, 10);
		itoa(variavle, buf+1, 10);
	}
	else if(variavle<10)
	{
		itoa(0, buf, 10);
		itoa(0, buf+1, 10);
		itoa(variavle, buf+2, 10);
	}
	else
	{
		itoa(variavle, buf, 10); 
	}
}
