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

    //ввод IP адреса для подключения
    cout << "Enter ip: ";
    cin.getline(serverIP,sizeof(serverIP),'\n');

    //загрузим нужную версию библиотеки
    WSADATA wsaData;
    WORD dllVersion = MAKEWORD(2,1);    //версия которую будем запрашивать
    if(WSAStartup(dllVersion, &wsaData) != 0){
        cout << "ERROR: Version is not found" << endl;
        exit(1);
    }

    //создадим структуру адреса
    SOCKADDR_IN addr;
    addr.sin_addr.S_un.S_addr   = inet_addr(serverIP);
    addr.sin_port               = htons(12345);
    addr.sin_family             = AF_INET;


    //подключимcя к серверу
    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if(connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0){
        cout << "ERROR: connection to server failed" << endl;
        exit(1);
    }else{
        cout << "SUCCESSFULL: connect to server" << endl;
        cout << "IP:" << serverIP << endl << "PORT: 12345 \n\n";


        ///фукция чтения JPEG
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
    int filesize = 0;                   //размер принятого файла
    int resBytes = 0, resdBytes = 0;    //размер пакета; количество принятых внутри recv() байт отчасти пакета;
    bool readResolution = true;         //разрешение на цикл чтения файла
    unsigned char bufer[FTP_BUFER];     //буфер хранения принимаемого пакета
    bufer[0] = 0;
    int RTBreaker = FTP_RTBREAK;        //счетчик timeout'a чтения некорректного пакета
    unsigned char* buferOffset = bufer; //указатель на смещение по массиву буфера
    bool RPresolution = 1;              //разрешение на чтение пакета

    cout << endl <<"PROGRAMM: JPEG2FILE programm started (programm started)" << endl;

    ///создать и открыть файл в бинарном формате (для избавления от записи служебных символов)
    ofstream File;
    File.open(fileName, ios_base::binary);
    ///____проверка корректного открытия файла____
    if(File.is_open())
        cout << endl <<"[FTP] PROCESS: File is open." << endl;
    else{
        cout << endl <<"[FTP] ERROR: File is`t open. (program terminate)" << endl;
        return FTP_TERMINATE;
    }
    ///____конец проверки__________________________
    cout << endl <<"[FTP] PROCESS: File downloading is started. (process started)" << endl;


    ///читать по одному пакету до сообщения конца файла
    while(readResolution){
        ///отправить серверу подтверждение готовности принять пакет
        send(soc, "[FTP_DATA]-RTR-[ED]", 50,0);

        ///считывать пакет побайтово пока не получим сообщение о конце пересылки пакета <<<<<<
        resBytes = 0;                           //обнулим счетчик принятых байт
        RPresolution = 1;                       //установи разрешение на чтение пакета
        buferOffset = bufer;                    //устанавка удреса начала массива
        cout << "&bufer = " << (int)bufer << endl << endl;  //выведем в консоль адрес смещения по массиву буфера
        while(RPresolution){                    //пока разрешено чтение пакета
            resdBytes = recv(soc, (char*)buferOffset, sizeof(buferOffset), 0); //читаем и записываем часть данных в массив начиная с адреса смещения
            if(resdBytes == -1){                //если произошло отключение от сервера
                cout << endl <<"[FTP] ERROR: Interrupt of downloading file. (disconnect)" << endl;
                readResolution = 0;             //запретим чтение файла (завершить обработку readJPEG2File())
                File.close();                   //закроем файл для корректного сохранения данных перед завершением программы
                return FTP_NOCONECT;            //вернем код ошибки
            }
            resBytes += resdBytes;              //подсчитаем количество принятых байт (узнаем сколько всего было принято байт в пакете)
            bufer[resBytes] = 0;                //установим нулевой символ [не обязательно]

            cout << "resBytes = " << resBytes << endl;  //отладочная информация (количество принятых байт в пакете)
            buferOffset = bufer + resBytes;     //выполним смещение по массиву буфера (сместим указатель на ячейку массива буфера)


            if(!strcmp((char*)(buferOffset-FTP_EOPSIZE),"[FTP_DATA]-EOP-[ED]")){    //если принят ключ конца пакета
                resBytes -= FTP_EOPSIZE;        //вычтем размер ключа из общего количесва принятых байт
                *(buferOffset-FTP_EOPSIZE) = 0; //отрежем ключ в конце пакета от полезных данных
                RPresolution = 0;               //запретим чтение пакета
                break;                          //выйдем из цыкла чтения пакета
            }
        } //while(RPresolution) end


        if(resBytes == -1){
            cout << endl <<"[FTP] ERROR: Interrupt of downloading file. (disconnect)" << endl;
            readResolution = 0;
            File.close();
            return FTP_NOCONECT;

        }else if(resBytes > FTP_BUFER){ ///если получен некорректный пакет
            ///отправить повторный запрос пакета
            send(soc, "[FTP_DATA]-BROKEN-[ED]", 50,0);
            --RTBreaker;
            ///если отправлено больше FTP_RTBREAK повторных запросов остановить выполнение программы
            if(RTBreaker == 0){
                cout << endl << "[FTP] ERROR: Runtime error. (program terminated)" << endl;
                File.close();
                return FTP_TERMINATE;   //readJPEG2File() вернет код ошибки
            }

        }else{ ///если пакет удовлетворяет всем условиям
            ///проверить на наличие команды протокола и выполнить команду
            bufer[resBytes] = 0;
            if(!strcmp((char*)bufer, "[FTP_DATA]-EOF-[ED]")){   //(проверка на конец пересылки файла) ключ [FTP_DATA]-EOF-[ED] отправляется отдельным пакетом
                cout << endl << "[FTP] PROCEDURE: Downloading is ended." << endl;
                cout << endl << "File: " << fileName << "   " << filesize/1024 << "." << filesize%1024 << "Kb" << endl; //информация о полученном файле
                File.close();           //закроем файл
                return FTP_TRUEEND;     //вернем код успешного завершения чтения файла и выйдем из функции readJPEG2File()
            }

            ///записать буфер в файл побайтово в количестве равном размеру пакета.
            for(int ind = 0; ind < resBytes; ind++)
                File << bufer[ind];

            ///завершающие прием пакета процедуры
            filesize += resBytes;           //подсчет размера файла
            RTBreaker = FTP_RTBREAK;        //обнуление счетчика timout'a
            cout << "Reseived: " << resBytes << "    File size: " << filesize << " byte"<< endl;    //промежуточная информация о текущем размере файла и принятых байтах в пакете
        }
    }
    ///закрыть файл
    File.close();
    return FTP_TERMINATE;
}
