#ifndef TFLUNAIFACE_H
#define TFLUNAIFACE_H

class TFLunaIface
{
private:
    bool isPresented;
    int currDistance;
    int currTemp;
public:
    TFLunaIface();
    bool InitLuna(const char *dev);
    bool IsPresented();
    int GetCurrDistance();
};

#endif // TFLUNAIFACE_H
