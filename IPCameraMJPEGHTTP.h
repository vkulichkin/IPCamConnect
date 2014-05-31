#pragma once
#include <WinInet.h>
#include <deque>
#include "CIPCamConnect.h"

class CIPCameraMJPEGHTTP : public ICIPCameraMJPEGHTTP
{
public:
	CIPCameraMJPEGHTTP(const char* strURL);
	~CIPCameraMJPEGHTTP(void);
	virtual bool CheckErr(wchar_t* strErr);
	static void Connect(void* ctx);
	virtual void GetBuffer(unsigned char** pBuf, int &iSize);
	virtual void GetURL(char** strURL);
private:
	char* m_strURL;
	bool m_bErr;
	wchar_t m_strErr[MAX_PATH];
	int PeekData(HINTERNET hRequest);
	bool m_bThreadRun;
	bool m_bStopThread;
	CRITICAL_SECTION m_cSection;
	std::deque<unsigned char> m_deqSumBuf;
};

