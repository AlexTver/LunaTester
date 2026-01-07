#include "tflunaiface.h"

TFLunaIface::TFLunaIface() {
    isPresented = false;
    currDistance = -1;
    currTemp = -273;
}

bool TFLunaIface::InitLuna(const char *dev)
{
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
