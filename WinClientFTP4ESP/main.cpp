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

    //���� IP ������ ��� �����������
    cout << "Enter ip: ";
    cin.getline(serverIP,sizeof(serverIP),'\n');

    //�������� ������ ������ ����������
    WSADATA wsaData;
    WORD dllVersion = MAKEWORD(2,1);    //������ ������� ����� �����������
    if(WSAStartup(dllVersion, &wsaData) != 0){
        cout << "ERROR: Version is not found" << endl;
        exit(1);
    }

    //�������� ��������� ������
    SOCKADDR_IN addr;
    addr.sin_addr.S_un.S_addr   = inet_addr(serverIP);
    addr.sin_port               = htons(12345);
    addr.sin_family             = AF_INET;


    //���������c� � �������
    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if(connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0){
        cout << "ERROR: connection to server failed" << endl;
        exit(1);
    }else{
        cout << "SUCCESSFULL: connect to server" << endl;
        cout << "IP:" << serverIP << endl << "PORT: 12345 \n\n";


        ///������ ������ JPEG
        readJPEG2File("lastreseive.jpg", Connection);

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
    int filesize = 0;                   //������ ��������� �����
    int resBytes = 0, resdBytes = 0;    //������ ������; ���������� �������� ������ recv() ���� ������� ������;
    bool readResolution = true;         //���������� �� ���� ������ �����
    unsigned char bufer[FTP_BUFER];     //����� �������� ������������ ������
    bufer[0] = 0;
    int RTBreaker = FTP_RTBREAK;        //������� timeout'a ������ ������������� ������
    unsigned char* buferOffset = bufer; //��������� �� �������� �� ������� ������
    bool RPresolution = 1;              //���������� �� ������ ������

    cout << endl <<"PROGRAMM: JPEG2FILE programm started (programm started)" << endl;

    ///������� � ������� ���� � �������� ������� (��� ���������� �� ������ ��������� ��������)
    ofstream File;
    File.open(fileName, ios_base::binary);
    ///____�������� ����������� �������� �����____
    if(File.is_open())
        cout << endl <<"[FTP] PROCESS: File is open." << endl;
    else{
        cout << endl <<"[FTP] ERROR: File is`t open. (program terminate)" << endl;
        return FTP_TERMINATE;
    }
    ///____����� ��������__________________________
    cout << endl <<"[FTP] PROCESS: File downloading is started. (process started)" << endl;


    ///������ �� ������ ������ �� ��������� ����� �����
    while(readResolution){
        ///��������� ������� ������������� ���������� ������� �����
        send(soc, "[FTP_DATA]-RTR-[ED]", 50,0);

        ///��������� ����� ��������� ���� �� ������� ��������� � ����� ��������� ������ <<<<<<
        resBytes = 0;                           //������� ������� �������� ����
        RPresolution = 1;                       //�������� ���������� �� ������ ������
        buferOffset = bufer;                    //��������� ������ ������ �������
        cout << "&bufer = " << (int)bufer << endl << endl;  //������� � ������� ����� �������� �� ������� ������
        while(RPresolution){                    //���� ��������� ������ ������
            resdBytes = recv(soc, (char*)buferOffset, sizeof(buferOffset), 0); //������ � ���������� ����� ������ � ������ ������� � ������ ��������
            if(resdBytes == -1){                //���� ��������� ���������� �� �������
                cout << endl <<"[FTP] ERROR: Interrupt of downloading file. (disconnect)" << endl;
                readResolution = 0;             //�������� ������ ����� (��������� ��������� readJPEG2File())
                File.close();                   //������� ���� ��� ����������� ���������� ������ ����� ����������� ���������
                return FTP_NOCONECT;            //������ ��� ������
            }
            resBytes += resdBytes;              //���������� ���������� �������� ���� (������ ������� ����� ���� ������� ���� � ������)
            bufer[resBytes] = 0;                //��������� ������� ������ [�� �����������]

            cout << "resBytes = " << resBytes << endl;  //���������� ���������� (���������� �������� ���� � ������)
            buferOffset = bufer + resBytes;     //�������� �������� �� ������� ������ (������� ��������� �� ������ ������� ������)


            if(!strcmp((char*)(buferOffset-FTP_EOPSIZE),"[FTP_DATA]-EOP-[ED]")){    //���� ������ ���� ����� ������
                resBytes -= FTP_EOPSIZE;        //������ ������ ����� �� ������ ��������� �������� ����
                *(buferOffset-FTP_EOPSIZE) = 0; //������� ���� � ����� ������ �� �������� ������
                RPresolution = 0;               //�������� ������ ������
                break;                          //������ �� ����� ������ ������
            }
        } //while(RPresolution) end


        if(resBytes == -1){
            cout << endl <<"[FTP] ERROR: Interrupt of downloading file. (disconnect)" << endl;
            readResolution = 0;
            File.close();
            return FTP_NOCONECT;

        }else if(resBytes > FTP_BUFER){ ///���� ������� ������������ �����
            ///��������� ��������� ������ ������
            send(soc, "[FTP_DATA]-BROKEN-[ED]", 50,0);
            --RTBreaker;
            ///���� ���������� ������ FTP_RTBREAK ��������� �������� ���������� ���������� ���������
            if(RTBreaker == 0){
                cout << endl << "[FTP] ERROR: Runtime error. (program terminated)" << endl;
                File.close();
                return FTP_TERMINATE;   //readJPEG2File() ������ ��� ������
            }

        }else{ ///���� ����� ������������� ���� ��������
            ///��������� �� ������� ������� ��������� � ��������� �������
            bufer[resBytes] = 0;
            if(!strcmp((char*)bufer, "[FTP_DATA]-EOF-[ED]")){   //(�������� �� ����� ��������� �����) ���� [FTP_DATA]-EOF-[ED] ������������ ��������� �������
                cout << endl << "[FTP] PROCEDURE: Downloading is ended." << endl;
                cout << endl << "File: " << fileName << "   " << filesize/1024 << "." << filesize%1024 << "Kb" << endl; //���������� � ���������� �����
                File.close();           //������� ����
                return FTP_TRUEEND;     //������ ��� ��������� ���������� ������ ����� � ������ �� ������� readJPEG2File()
            }

            ///�������� ����� � ���� ��������� � ���������� ������ ������� ������.
            for(int ind = 0; ind < resBytes; ind++)
                File << bufer[ind];

            ///����������� ����� ������ ���������
            filesize += resBytes;           //������� ������� �����
            RTBreaker = FTP_RTBREAK;        //��������� �������� timout'a
            cout << "Reseived: " << resBytes << "    File size: " << filesize << " byte"<< endl;    //������������� ���������� � ������� ������� ����� � �������� ������ � ������
        }
    }
    ///������� ����
    File.close();
    return FTP_TERMINATE;
}
