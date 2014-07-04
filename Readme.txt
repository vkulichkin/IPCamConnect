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

1. #include "CIPCamConnect.h"

2. Create the object:

ICIPCameraMJPEGHTTP* pMJPEG = CreateICIPCameraMJPEGHTTP("http://......"); 

3.







Victor Kulichkin
http://www.kvy.com.ua