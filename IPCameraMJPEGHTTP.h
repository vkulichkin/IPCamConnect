#pragma once
#include <WinInet.h>
#include <deque>

class CIPCameraMJPEGHTTP
{
public:
	CIPCameraMJPEGHTTP(char* strURL);
	~CIPCameraMJPEGHTTP(void);
	bool CheckErr(TCHAR* strErr);
	static void Connect(void* ctx);
	void GetBuffer(unsigned char** pBuf, int &iSize);
	void GetURL(char** strURL);
private:
	char* m_strURL;
	bool m_bErr;
	TCHAR m_strErr[MAX_PATH];
	int PeekData(HINTERNET hRequest);
	//int m_iBufsize;
	//unsigned char* m_pBuf;
	bool m_bThreadRun;
	bool m_bStopThread;
	CRITICAL_SECTION m_cSection;
	std::deque<unsigned char> m_deqSumBuf;
};

