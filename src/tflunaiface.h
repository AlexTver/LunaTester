#ifndef TFLUNAIFACE_H
#define TFLUNAIFACE_H

#include <winsock2.h>
#include <Windows.h>
#include <stdint.h>

class TFLunaIface
{
private:
    bool isPresented;
    // int portFD;
    HANDLE portFD;
    int currDistance;
    int currTemp;
    void ClosePort();
    void LidarDetect();
    void StartReadThread();
    bool WaitReadThread();
    HANDLE hReadThread;
    DWORD readThreadID;
    void sendtoport(const uint8_t *buf, uint16_t MesLen);
    bool readfromport(uint8_t *MesRet, size_t *MesRetLen, size_t iMaxReadLen, uint8_t prefix, uint8_t code);

public:
    TFLunaIface();
    ~TFLunaIface();
    bool InitLuna(const char *dev);
    bool IsPresented();
    int GetCurrDistance();
    void ProcessLongRead();
};

DWORD WINAPI ReadCardThreadFunc();

#endif // TFLUNAIFACE_H
