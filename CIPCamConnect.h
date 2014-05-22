#ifndef CIPCAMCONNECT_H
#define CIPCAMCONNECT_H
#include <windows.h>

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif

struct ICIPCamConnect
{
    virtual void Release() = 0;
    virtual bool CheckErr(wchar_t* strErr) = 0;
    virtual void GetBuffer(unsigned char* pBuf) = 0;
    virtual int  GetSize(int &iWidth, int &iHeight) = 0;
    virtual void GetURL(char** strURL) = 0;
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};


extern "C"
{
    DLL_EXPORT ICIPCamConnect* WINAPI CreateIPCamConnectClass(const LPCSTR strURL);
}

#endif // CIPCAMCONNECT_H
