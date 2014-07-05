Handling streams of IP cameras:

1. MJPEG by HTTP
2. H264 by RTSP

CodeBlocks project with using FFMPEG and OpenCV libraries

-------
Output:
in
\IPCamera\IPCamConnect\bin\Debug-Win32
\IPCamera\IPCamConnect\bin\Release-Win32

1. IPCamConnect.dll
2. IPCamConnect.lib - .lib for MSVC projects 
3. CIPCamConnect.h  - a header file for projects
------------------------

Export fabric functions:

1. ICIPCamConnect* WINAPI CreateIPCamConnectClass(const LPCSTR strURL);

- create the object of CIPCamConnect for H264 by RTSP

2. ICIPCameraMJPEGHTTP* WINAPI CreateICIPCameraMJPEGHTTP(const LPCSTR strURL);

- create the object of CIPCameraMJPEGHTTP for MJPEG by HTTP
-------------------------------------

DLLs from ffmpeg:

\IPCamera\IPCamConnect\ffmpeg_dll

------------------------------------
Usage for MJPEG

1. Add to the project 

#include "CIPCamConnect.h"

#pragma comment( lib, "IPCamConnect")

2. Create the object:

ICIPCameraMJPEGHTTP* pMJPEG = CreateICIPCameraMJPEGHTTP("http://......"); 

3.

Add later.......

-----------------------------------------------
Usage for H264


#include "CIPCamConnect.h"

#pragma comment( lib, "IPCamConnect")



int _tmain(int argc, _TCHAR* argv[])
{
	ICIPCamConnect* pP = NULL;
	pP =  CreateIPCamConnectClass("rtsp://user:passw@192.168.1.108/0");

	wchar_t strErr[MAX_PATH] = {0};
	bool bErr =  pP->CheckErr(strErr);
	
	if (!bErr)
	{
		PBYTE pBuf = NULL;
		int iSize  = 0;
		int x, y;
		int n = 50, i = 0; //timer 5 sec
		while ((iSize = pP->GetSize(x, y)) == 0)
		{
			Sleep(100);
			i++;
			if (i == n)
				break;
		}
		if (iSize != 0)
		{
			printf("ok\n");
			pBuf = new unsigned char [iSize];
			pP->GetBuffer(pBuf); //get data
			//......
			//do something with data here 
			//......
			delete [] pBuf;
		}
		else
			printf("Error\n");
	}
	else
		printf("Error: %s", strErr);

	if (pP)
		pP->Release();
	return 0;
}

-------------------------------------------------------------------------



Victor Kulichkin
http://www.kvy.com.ua