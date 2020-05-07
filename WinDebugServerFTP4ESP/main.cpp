//SERVER
#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>

using namespace std;

int main(int argc, char* argv[])
{
    unsigned char bufer[100];
    int resBytes = 0;

    //загрузим нужную версию библиотеки
    WSADATA wsaData;
    WORD dllVersion = MAKEWORD(2,1);    //версия которую будем запрашивать
    if(WSAStartup(dllVersion, &wsaData) != 0){
        cout << "ERROR: Version is not found" << endl;
        exit(1);
    }

    //создадим структуру адреса
    SOCKADDR_IN addr;
    addr.sin_addr.S_un.S_addr    = INADDR_ANY;
    addr.sin_port           = htons(12345);
    addr.sin_family         = AF_INET;

    //создадим прослушивающий сокет
    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    //подключим клиент к этому серверу
    SOCKET newConnection;
    int addrsize = sizeof(addr);
    newConnection = accept(sListen, (SOCKADDR*)&addr, &addrsize);

    //проверим подключение
    if(newConnection == 0){
        cout << "ERROR: Connection failed" << endl;
        exit(2);
    }else{
        cout << "SUCCESSFULL: client connected" << endl;
        cout << "IP: "  << int(addr.sin_addr.S_un.S_un_b.s_b1) << "."
                        << int(addr.sin_addr.S_un.S_un_b.s_b2) << "."
                        << int(addr.sin_addr.S_un.S_un_b.s_b3) << "."
                        << int(addr.sin_addr.S_un.S_un_b.s_b4) << "   PORT: 12345 \n\n";

        //отправка тестовых данных

        Sleep(1500);
        cout << "start data sending" <<endl;
        char *chr = new char;
        for(unsigned char ch = 0; ch <= 5/*MAXBYTE*/; ++ch){
            resBytes = recv(newConnection,(char*)bufer,sizeof(bufer),0);
            bufer[resBytes] = 0;
            if(!strcmp("[FTP_DATA]-BROKEN-[ED]", (char*)bufer)){
                --ch;
                continue;
            }
            else if(!strcmp("[FTP_DATA]-RTR-[ED]", (char*)bufer)){
                *chr = ch+'0';

                send(newConnection, "Packet: ", 8,0);
                send(newConnection, chr, 1, 0);
                //send(newConnection, (char*)&ch, 1,0);
                send(newConnection, " ADDED_TEST_STRING", 18, 0);
                send(newConnection, "Test1__## ", 10, 0);
                send(newConnection, "Test2__## ", 10, 0);
                send(newConnection, "Test3__## ", 10, 0);
                send(newConnection, "Test4__## ", 10, 0);
                send(newConnection, "Test5__## ", 10, 0);
                send(newConnection, "Test6__## ", 10, 0);
                send(newConnection, "Test7__## ", 10, 0);
                send(newConnection, "Test8__## ", 10, 0);
                send(newConnection, "Test9__## ", 10, 0);
                send(newConnection, "Test10__## ", 10, 0);
                send(newConnection, "Test11__## ", 10, 0);
                send(newConnection, "Test12__## ", 10, 0);
                send(newConnection, "Test13__## ", 10, 0);
                send(newConnection, "Test14__## ", 10, 0);
                send(newConnection, "Test15__## ", 10, 0);
                send(newConnection, "Test16__## ", 10, 0);
                send(newConnection, "Test17__## ", 10, 0);
                send(newConnection, "Test18__## ", 10, 0);
                send(newConnection, "Test19__## ", 10, 0);
                send(newConnection, "Test20__## ", 10, 0);
                send(newConnection, "[FTP_DATA]-EOP-[ED]", 19, 0);

            }
            if(ch == 0xFF)break;
        }
        resBytes = recv(newConnection,(char*)bufer,sizeof(bufer),0);
            bufer[resBytes] = 0;
            if(!strcmp("[FTP_DATA]-BROKEN-[ED]", (char*)bufer)){
                return 1;
            }
            else if(!strcmp("[FTP_DATA]-RTR-[ED]", (char*)bufer)){
                send(newConnection, "[FTP_DATA]-EOF-[ED]", 20,0);
                send(newConnection, "[FTP_DATA]-EOP-[ED]", 19, 0);
            }






        cout << "data sended" <<endl;

    }


    //cout << "Hello world!" << endl;
    system("pause");
    return 0;
}
