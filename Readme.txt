CONTENT

1. About codes
2. License
3. Output
4. Fabric functions
5. DLLs for run
6. Usage for MJPEG
7. Usage for H264
8. Contact

---------------------
Codes.

Handling streams of IP cameras:

1. MJPEG by HTTP
2. H264 by RTSP
CodeBlocks project with using FFMPEG and OpenCV libraries

------------------------------------------
License

This code is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This code is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this code; if not, contact to me via http://www.kvy.com.ua 

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
		//wait for the first frame
		while ((iSize = pP->GetSize(x, y)) == 0)
		{
			Sleep(100);
			i++;
			if (i == n)
				break;
		}
		if (iSize != 0)
		{//receive only one frame
			printf("ok\n");
			pP->Lock();
			iSize = pP->GetSize(x, y);
			pBuf = new unsigned char [iSize];
			pP->GetBuffer(pBuf); //get data in BGR24
			pP->Unlock();
			
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
Contact


Victor Kulichkin
http://www.kvy.com.ua
