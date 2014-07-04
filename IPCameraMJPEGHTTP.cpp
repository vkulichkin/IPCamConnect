#include "IPCameraMJPEGHTTP.h"
#include <string>
#include <vector>

DLL_EXPORT  ICIPCameraMJPEGHTTP* WINAPI CreateICIPCameraMJPEGHTTP(const LPCSTR strURL)
{
     return new CIPCameraMJPEGHTTP(strURL);
}

CIPCameraMJPEGHTTP::CIPCameraMJPEGHTTP(const char* strURL)
	: m_bErr(false)
	, m_bThreadRun(false)
	, m_bStopThread(false)
{
	InitializeCriticalSection(&m_cSection);
	if (!strURL)
	{
		m_bErr = true;
		wcscpy(m_strErr, L"Error: Empty URL for IPCamera");
		return;
	}

	m_strURL = new char [strlen(strURL) + 1];
	if (!m_strURL)
	{
		m_bErr = true;
		wcscpy(m_strErr, L"Error: No memory for IPCamera");
		return;
	}
	strcpy(m_strURL, strURL);
	wcscpy(m_strErr, L"No error");
	_beginthread(Connect, 0, this);
}


CIPCameraMJPEGHTTP::~CIPCameraMJPEGHTTP(void)
{
	m_bStopThread = true;
	int nCount = 2000;
	for (int i = 0; i < nCount; i++)
	{
		if (!m_bThreadRun)
			break;
		Sleep(10);
	}
	DeleteCriticalSection(&m_cSection);

	if (m_strURL)
		delete [] m_strURL;
}

bool CIPCameraMJPEGHTTP::CheckErr(wchar_t* strErr)
{
	if (m_bErr &&  strErr)
	{
		wcscpy(strErr, m_strErr);
	}
	return m_bErr;
}

void CIPCameraMJPEGHTTP::Connect(void* ctx)
{
	void* cstr = NULL;
	CIPCameraMJPEGHTTP* pParent = (CIPCameraMJPEGHTTP*)ctx;
	pParent->m_bErr = false;
	HINTERNET hSession = NULL;
	HINTERNET hRequest =  NULL;
	pParent->m_bThreadRun = true;

	hSession = InternetOpenA("SplitCam MJPG listener", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hSession)
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &cstr,
			0,
			NULL
		);
		if (cstr)
		{
			wcscpy(pParent->m_strErr, (wchar_t*)cstr);
			LocalFree(cstr);
		}
		else
		{
			swprintf(pParent->m_strErr, L"Cannot open a connection with IP Camera, error: %i",  GetLastError());
		}
		pParent->m_bErr = true;
		pParent->m_bThreadRun = false;
		pParent->m_bStopThread = false;
		_endthread();
		return;
	}
	hRequest = InternetOpenUrlA(hSession, (LPCSTR)pParent->m_strURL, NULL, 0, 0, 0);
	if (!hRequest)
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &cstr,
			0,
			NULL
		);
		if (cstr)
		{
			wcscpy(pParent->m_strErr, (wchar_t*)cstr);
			LocalFree(cstr);
		}
		else
		{
			swprintf(pParent->m_strErr, L"Cannot open a connection with IP Camera, error: %i",  GetLastError());
		}
		pParent->m_bErr = true;
		pParent->m_bThreadRun = false;
		pParent->m_bStopThread = false;
		_endthread();
		return;
	}

	pParent->PeekData(hRequest);

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hSession);
	pParent->m_bThreadRun = false;
	pParent->m_bStopThread = false;
	_endthread();
}

void CIPCameraMJPEGHTTP::GetBuffer(unsigned char** pBuf, int &iSize)
{
	unsigned char ch;
	unsigned int i, j;
	unsigned int nSizeFrame;

	if (*pBuf)
	{
		delete [] *pBuf;
		*pBuf = NULL;
	}
	iSize = 0;

	EnterCriticalSection(&m_cSection);
	if (m_deqSumBuf.empty())
	{
		LeaveCriticalSection(&m_cSection);
		return;
	}
	std::string strSumBuf;
	for (i = 0; i < m_deqSumBuf.size(); i++)
	{
		ch = m_deqSumBuf.at(i);
		if (ch == 0x00)
			break;
		strSumBuf.append(1, ch);
	}
	if (strSumBuf.empty())
	{
		LeaveCriticalSection(&m_cSection);
		return;
	}

	//get length frame
	i = strSumBuf.find("Content-Length: ");
	if (i == std::string::npos)
	{
		LeaveCriticalSection(&m_cSection);
		return;
	}
	i += strlen("Content-Length: ");
	strSumBuf = strSumBuf.substr(i, strSumBuf.length() - i);
	j = 0;
	char strLengthNumber[20] = {0};
	unsigned int size =  strSumBuf.size();
	if (size == 0)
	{
		LeaveCriticalSection(&m_cSection);
		return;
	}
	size--;
	while ((ch = strSumBuf.at(j)) != 0x0d)
	{
		strLengthNumber[j++] += ch;
		if (j > size)
		{
			LeaveCriticalSection(&m_cSection);
			return;
		}
	}
	nSizeFrame = atoi(strLengthNumber);
	i += strlen(strLengthNumber);
	if (nSizeFrame > m_deqSumBuf.size() - i - 4)
	{
		LeaveCriticalSection(&m_cSection);
		return;
	}
	j = strSumBuf.find("\r\n\r\n");
	if (j == std::string::npos)
	{
		LeaveCriticalSection(&m_cSection);
		return;
	}
	i += 4; //\r\n\r\n
	for (j = 0; j < i; j++)
		m_deqSumBuf.pop_front();

	iSize = nSizeFrame;
	*pBuf = new unsigned char [iSize];
	for (i = 0; i < nSizeFrame; i++)
	{
		ch = m_deqSumBuf.at(0);
		m_deqSumBuf.pop_front();
		(*pBuf)[i] = ch;
	}
	LeaveCriticalSection(&m_cSection);
}

int CIPCameraMJPEGHTTP::PeekData(HINTERNET hRequest)
{
	DWORD dwMessSize;
	unsigned char* pMessBuf = NULL;

	m_deqSumBuf.clear();

	if (!hRequest)
		return -1;
	while (!m_bStopThread)
	{
		dwMessSize = 0;
		// узнаём размер следующего пакета
		InternetQueryDataAvailable(hRequest, &dwMessSize, 0,0);
	   // выходим если размер нулевой
	   if(dwMessSize <= 0)
	   {
		   Sleep(10);
		   continue;
	   }
	   // зарезервируем массив байт для очередной порции
	   pMessBuf = new unsigned char[dwMessSize];
	   if(pMessBuf == NULL)
	   {
		   wcscpy(m_strErr, L"No memory");
		   m_bErr = true;
		   break;
	   }

	   DWORD dwBr=0;
	   if (!InternetReadFile(hRequest, pMessBuf, dwMessSize, &dwBr))
	   {
			void *cstr = NULL;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &cstr,
				0,
				NULL
			);
			if (cstr)
			{
				wcscpy(m_strErr, (wchar_t*)cstr);
				LocalFree(cstr);
			}
			else
			{
				swprintf(m_strErr, L"Cannot read IP Camera, error: %i",  GetLastError());
			}
		   m_bErr = true;
		   break;
		}

		EnterCriticalSection(&m_cSection);
		for (int i = 0; i < (int)dwMessSize; i++ )
		{
			m_deqSumBuf.push_back(pMessBuf[i]);
		}
		LeaveCriticalSection(&m_cSection);
		delete [] pMessBuf;

	}
	return 0;
}

void  CIPCameraMJPEGHTTP::GetURL(char** strURL)
{
	*strURL = m_strURL;
}
