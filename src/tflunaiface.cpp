// #include <unistd.h>
#include <errno.h>
// #include <fcntl.h>

#include "tflunaiface.h"
#include "fn_log.h"
#include "Globals.h"

TFLunaIface::TFLunaIface() {
    isPresented = false;
    // portFD = -1;
    portFD = INVALID_HANDLE_VALUE;
    currDistance = -1;
    currTemp = -273;
}

TFLunaIface::~TFLunaIface()
{
    ClosePort();
}

void TFLunaIface::ClosePort()
{
    if (portFD != INVALID_HANDLE_VALUE) {
        LogDebug() << "Закрытие порта " << portFD;
    // if (portFD != -1) {
        CloseHandle(portFD);
        portFD = INVALID_HANDLE_VALUE;
        // close(portFD);
        // portFD = -1;
    }

}

void TFLunaIface::LidarDetect()
{
    const uint8_t getVerCmd[] = {0x5A, 0x04, 0x14, 0x72};
    PurgeComm(portFD, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    sendtoport(getVerCmd, 4);
    uint8_t answ[255];
    size_t answLen;
    if (readfromport(answ, &answLen, 255, 0x5A, 0x14)) {
        answ[29] = 0;
        char *tmp = (char*)&answ[3];
        // std::string TFInfo = tmp;
        LogInfo() << "Информация: '" << tmp << "'";
        const uint8_t getQRCode[] = {0x5A, 0x04, 0x12, 0x70};
        sendtoport(getQRCode, 4);
        if (readfromport(answ, &answLen, 255, 0x5A, 0x12)) {
            answ[17] = 0;
            tmp = (char*)&answ[3];
            // std::string TFInfo = tmp;
            LogInfo() << "QRCode: '" << tmp << "'";
        }
    }


}

bool TFLunaIface::InitLuna(const char *dev)
{
    LogDebug() << "Открытие устройства '" << dev << "'";
    ClosePort();
    portFD = CreateFileA(dev, GENERIC_READ | GENERIC_WRITE,
                       0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    // portFD = open(dev, O_RDWR | O_NONBLOCK);
    // if (portFD != -1) {
    if (portFD != INVALID_HANDLE_VALUE) {
        LogDebug() << "Устройство '" << dev << "' открыто ID " << portFD;
        PurgeComm(portFD, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

        SetupComm(portFD,300,300);
        DCB dcb;
        GetCommState(portFD, &dcb);
        dcb.BaudRate = CBR_115200;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        if(SetCommState(portFD, &dcb)) {
            COMMTIMEOUTS tms;
            tms.ReadIntervalTimeout = 100;
            tms.ReadTotalTimeoutMultiplier = 0;
            tms.ReadTotalTimeoutConstant =100;
            tms.WriteTotalTimeoutMultiplier = 0;
            tms.WriteTotalTimeoutConstant = 0;
            if(SetCommTimeouts(portFD,&tms)) {
                LogInfo() << "Устройство '" << dev << "' открыто. Определение подключенного лидара";
                LidarDetect();
            } else {
                // dwError=GetLastError();
                int locErr = errno;
                LogError() << "Ошибка установки таймаутов код ошибки :'" << locErr << "'";
            }
        } else {
            // dwError=GetLastError();
            int locErr = errno;
            LogError() << "Ошибка установки скорости код ошибки :'" << locErr << "'";
        }
    } else {
        int locErr = errno;
        LogError() << "Ошибка открытия устройства '" << dev << "' код ошибки :'" << locErr << "'";
    }

    return isPresented;
}

bool TFLunaIface::IsPresented()
{
    return isPresented;
}

int TFLunaIface::GetCurrDistance()
{
    return currDistance;
}

// ---------------------------------------------------------------------------
DWORD WINAPI ReadCardThreadFunc() {
    DWORD dwRetCode = 0x100;

    luna.ProcessLongRead();

    ExitThread(dwRetCode);
    return 0;
}

// ---------------------------------------------------------------------------
void TFLunaIface::StartReadThread()
{
    hReadThread = CreateThread(NULL, // default security attributes
                               0, // use default stack size
                               (LPTHREAD_START_ROUTINE) ReadCardThreadFunc, // thread function
                               NULL, // no thread function argument
                               0, // use default creation flags
                               &readThreadID); // returns thread identifier
}

// ---------------------------------------------------------------------------
bool TFLunaIface::WaitReadThread() {
    bool bRet = true;
    DWORD dwRetResult;
    dwRetResult = WaitForSingleObject(hReadThread, INFINITE);
    if (dwRetResult == WAIT_OBJECT_0) {
        if (GetExitCodeThread(hReadThread, &dwRetResult)) {
            // LogString("Поток чтения закончен с результатом " + IntToHex((uint64_t)dwRetResult, 8));
            ;
        }
        else
            // LogString("GetExitCodeThread fail");
            ;
    }
    else {
        uint64_t lastErr = GetLastError();
        LogError() << "WaitForSingleObject '" << dwRetResult << "'";
        LogError() << "GetLastError '" << lastErr << "'";
        bRet = false;
    }
    return bRet;
}

// ---------------------------------------------------------------------------
void TFLunaIface::ProcessLongRead() {
    // LongReadCardFun();
}

//---------------------------------------------------------------------------
void TFLunaIface::sendtoport(const uint8_t *buf, uint16_t MesLen)
{
    bool fSuccess;
    unsigned long out;
    WriteFile(portFD, buf, MesLen, &out, NULL);
    LogTrace() << "Sended '" << out << "' bytes";
}
//---------------------------------------------------------------------------
bool TFLunaIface::readfromport(uint8_t *MesRet, size_t *MesRetLen, size_t iMaxReadLen, uint8_t prefix, uint8_t code)
{
    bool bRet;
    time_t start = time(NULL);
    size_t i = 0; //текущая позиция в возвратном буфере
    bool prefixFind = false;
    bool commandFind = false;
    unsigned long r; // фактически прочитано
    size_t iToReadLen = 1; //начинаем по 1 байту

    do
    {
        ReadFile(portFD, &MesRet[i], iToReadLen, &r, NULL);
        LogTrace() << "Read '" << MesRet[i] << "' bytes";
        if (r >= iToReadLen) {
            if ((!prefixFind) && (MesRet[i] == prefix)) { //кажись то что надо
                prefixFind = true;
                iToReadLen = 2;
                i += r;
            } else if ((!commandFind) && (MesRet[i+iToReadLen-1] == code)) {
                commandFind = true;
                if (MesRet[i+1] <= (iMaxReadLen - i - 2)) {
                    iToReadLen = MesRet[i];
                    i += r;
                } else {
                    i = 0; //Мы это вычитать не сможем
                    prefixFind = false;
                    commandFind = false;
                }
            } else if (prefixFind && commandFind) {
                bRet = true;
            } else {
                i = 0; //Миша всё по новой
                prefixFind = false;
                commandFind = false;
            }
        } else {
            Sleep(100);
        }
    // } while ((!bRet)); //ждем 5 сек
    } while (((time(NULL) - start) <= 5) && (!bRet)); //ждем 5 сек

    // *MesRetLen=i;
    return bRet;
}

