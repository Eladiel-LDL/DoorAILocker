//CLIENT
#include <winsock2.h>
#include <iostream>
#include <fstream>

byte readJPEG2File(char* fileName, SOCKET &soc);

using namespace std;

int main(int argc, char* argv[])
{
    //variebles and data
    char serverIP[41];
    //char buffer[40];
    //int reseivedBytes = 0;

    //ввод IP адреса дл€ подключени€
    cout << "Enter ip: ";
    cin.getline(serverIP,sizeof(serverIP),'\n');

    //загрузим нужную версию библиотеки
    WSADATA wsaData;
    WORD dllVersion = MAKEWORD(2,1);    //верси€ которую будем запрашивать
    if(WSAStartup(dllVersion, &wsaData) != 0){
        cout << "ERROR: Version is not found" << endl;
        exit(1);
    }

    //создадим структуру адреса
    SOCKADDR_IN addr;
    addr.sin_addr.S_un.S_addr   = inet_addr(serverIP);
    addr.sin_port               = htons(12345);
    addr.sin_family             = AF_INET;


    //подключимc€ к серверу
    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if(connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0){
        cout << "ERROR: connection to server failed" << endl;
        exit(1);
    }else{
        cout << "SUCCESSFULL: connect to server" << endl;
        cout << "IP:" << serverIP << endl << "PORT: 12345 \n\n";


            ///создать обработчик команд (пока лишь одну фукцию чтени€ JPEG)
            readJPEG2File("lastreseive.jpg", Connection);
        //}

    }




    system("pause");
    return 0;
}

#define FTP_BUFER 10050
#define FTP_RTBREAK 5

#define FTP_TRUEEND 0
#define FTP_NOCONECT 1
#define FTP_TERMINATE 2

#define FTP_EOPSIZE 19 ///<<<<<<<<<<<<<<<<

byte readJPEG2File(char* fileName, SOCKET &soc)
{
    int filesize = 0;
    int resBytes = 0, resdBytes = 0;
    bool readResolution = true;
    unsigned char bufer[FTP_BUFER];
    bufer[0] = 0;
    int RTBreaker = FTP_RTBREAK;
    unsigned char* buferOffset = bufer;
    bool RPresolution = 1;

    cout << endl <<"PROGRAMM: JPEG2FILE programm started (programm started)" << endl;

    ///создать и открыть файл в бинарном формате
    ofstream File;
    File.open(fileName, ios_base::binary);
    ///____проверка корректного открыти€ файла____
    if(File.is_open())
        cout << endl <<"[FTP] PROCESS: File is open." << endl;
    else{
        cout << endl <<"[FTP] ERROR: File is`t open. (program terminate)" << endl;
        return FTP_TERMINATE;
    }
    ///____конец проверки__________________________
    cout << endl <<"[FTP] PROCESS: File downloading is started. (process started)" << endl;

    //send(soc, "T", 1,0);
    ///читать по одному пакету до сообщени€ конца файла
    while(readResolution){
        ///отправить подтверждение готовности прин€ть пакет
        send(soc, "[FTP_DATA]-RTR-[ED]", 50,0);

        //resBytes = recv(soc,(char*)bufer,sizeof(bufer),0);
        ///считывать пакет побайтово пока не получим сообщение о конце ересылки пакета <<<<<<
        resBytes = 0;
        RPresolution = 1;
        buferOffset = bufer;
        cout << "&bufer = " << (int)bufer << endl << endl;
        while(RPresolution){
            resdBytes = recv(soc, (char*)buferOffset, sizeof(buferOffset), 0); //читаем часть данных в массив со смещением
            if(resdBytes == -1){
                cout << endl <<"[FTP] ERROR: Interrupt of downloading file. (disconnect)" << endl;
                readResolution = 0;
                ///* закрыть файл
                File.close();
                return FTP_NOCONECT;
            }
            resBytes += resdBytes; //узнаем сколько всего было прин€то байт в пакете
            bufer[resBytes] = 0;    //установим нулевой символ [не об€зательно]

            cout << "resBytes = " << resBytes << endl;
            //cout << "buferOffset = " << (char*)buferOffset << endl;
            //cout << "bufer = " << (char*)bufer << endl; //<< endl;
            buferOffset = bufer + resBytes; //cout << "&buferOffset = " << (int)buferOffset << endl; //сместим указатель на €чейку массива буфера


            if(!strcmp((char*)(buferOffset-FTP_EOPSIZE),"[FTP_DATA]-EOP-[ED]")){
                resBytes -= FTP_EOPSIZE;
                //bufer[resBytes] = 0;
                *(buferOffset-FTP_EOPSIZE) = 0;
                //cout << "[Packet] EOP detected = Break" << endl;
                cout << bufer << endl;
                //break;
                RPresolution = 0;

            }
        }


        if(resBytes == -1){
            cout << endl <<"[FTP] ERROR: Interrupt of downloading file. (disconnect)" << endl;
            readResolution = 0;
            ///* закрыть файл
            File.close();
            return FTP_NOCONECT;

        }else if(resBytes > FTP_BUFER){ ///если получен некорректный пакет
            ///отправить повторный запрос пакета
            send(soc, "[FTP_DATA]-BROKEN-[ED]", 50,0);
            --RTBreaker;
            ///если отправлено больше 5 повторных запросов остановить выполнение программы
            if(RTBreaker == 0){
                cout << endl << "[FTP] ERROR: Runtime error. (program terminated)" << endl;
                return FTP_TERMINATE;
            }

        }else{ ///если пакет удовлетвор€ет услови€м
            ///проверить на наличие команды протокола и выполнить команду
            bufer[resBytes] = 0;
            ///-(проверка на конец пересылки данных)
            if(!strcmp((char*)bufer, "[FTP_DATA]-EOF-[ED]")){
                cout << endl << "[FTP] PROCEDURE: Downloading is ended." << endl;
                cout << endl << "File: " << fileName << "   " << filesize/1024 << "." << filesize%1024 << "Kb" << endl;
                File.close();
                return FTP_TRUEEND;
            }

            ///считать буфер побайтово в количестве равном размеру пакета.
            for(int ind = 0; ind < resBytes; ind++)
                File << bufer[ind];

            ///завершающие прием пакета процедуры
            filesize += resBytes;
            RTBreaker = FTP_RTBREAK;
            cout << "Reseived: " << resBytes << "    File size: " << filesize << " byte"<< endl;

        }
    }
    ///* закрыть файл
    File.close();
    return FTP_TERMINATE;
}
