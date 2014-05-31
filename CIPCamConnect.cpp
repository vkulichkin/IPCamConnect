#include "CIPCamConnect.h"
#include <process.h>

/* Signed. */
# define INT8_C(c)     c
# define INT16_C(c)     c
# define INT32_C(c)     c
# if __WORDSIZE == 64
# define INT64_C(c)    c ## L
# else
# define INT64_C(c)    c ## LL
# endif

/* Unsigned. */
# define UINT8_C(c)     c
# define UINT16_C(c)    c
# define UINT32_C(c)    c ## U
# if __WORDSIZE == 64
# define UINT64_C(c) c ## UL
# else
# define UINT64_C(c) c ## ULL
# endif


extern "C" {
#include "avcodec.h"
#include "avformat.h"
#include "fifo.h"
#include "swscale.h"
#include "opt.h"
#include "imgutils.h"
#include "swresample.h"
}

////////////////////////////////////////////
class CIPCamConnect : public ICIPCamConnect
{
    public:
        CIPCamConnect(const LPCSTR strURL);
        virtual ~CIPCamConnect();
        virtual void Release();
        static void Connect(void* ctx);
        virtual bool CheckErr(wchar_t* strErr);
        virtual void GetBuffer(unsigned char* pBuf);
        virtual void GetURL(char** strURL);
        virtual int  GetSize(int &iWidth, int &iHeight);
        virtual void Lock();
        virtual void Unlock();
    protected:
    private:
        char* m_strURL;
        bool m_bErr;
        wchar_t m_strErr[MAX_PATH];
       	bool m_bThreadRun;
        bool m_bStopThread;
        CRITICAL_SECTION m_cSection;
        unsigned char* m_pSumBuf;
        void PeekData(AVFrame *pFrame, int iWidth, int iHeight);
        int m_iWidth;
        int m_iHeight;
};
/////////////////////////////////////////////

CIPCamConnect::CIPCamConnect(const LPCSTR strURL)
{
    m_strURL = NULL;
    m_bErr = false;
    m_strErr[0] = 0;
    m_bThreadRun = false;
    m_bStopThread = false;
    m_pSumBuf = NULL;
    m_iHeight = 0;
    m_iWidth = 0;

    //MessageBoxA(0, strURL, "IP SplitCam", MB_OK | MB_ICONINFORMATION);

    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    av_register_all();
    avcodec_register_all();
	avformat_network_init(); //Do global initialization of network components.

    InitializeCriticalSection(&m_cSection);

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

CIPCamConnect::~CIPCamConnect()
{
  //  MessageBoxA(0, "Release", "IP SplitCam", MB_OK | MB_ICONINFORMATION);
 	m_bStopThread = true;
	int nCount = 1000;
	for (int i = 0; i < nCount; i++)
	{
		if (!m_bThreadRun)
			break;
		Sleep(10);
	}
	DeleteCriticalSection(&m_cSection);

    if (m_pSumBuf)
        delete [] m_pSumBuf;
    if (m_strURL)
        delete [] m_strURL;
    avformat_network_deinit();
}

void CIPCamConnect::Release()
{
    delete this;
}

void CIPCamConnect::Connect(void* ctx)
{
	CIPCamConnect* pParent = (CIPCamConnect*)ctx;
	pParent->m_bErr = false;
	pParent->m_bThreadRun = true;
	AVFormatContext* pAVContext = NULL;
	AVCodecContext* pInputCodecContext = NULL;
	AVCodec *pCodec = NULL;
    AVFrame* pFrame = NULL;
    AVFrame* pFrameRGB = NULL;
    SwsContext *pSwsContext = NULL;
//	AVStream *pInputStream;
	AVPacket pkt;
	int indexVideoStream = 0;
    int numBytes = 0;
    uint8_t* pBuffer = NULL;

	pAVContext = avformat_alloc_context();
    if (!pAVContext)
    {
        wcscpy(pParent->m_strErr, L"Cannot allocate AVContext");
        pParent->m_bErr = true;
        pParent->m_bThreadRun = false;
        pParent->m_bStopThread = false;
        _endthread();
        return;
    }
    //open rtsp
    if(avformat_open_input(&pAVContext, pParent->m_strURL, NULL,NULL) != 0)
    {
        wcscpy(pParent->m_strErr, L"Cannot open URL for stream");
        pParent->m_bErr = true;
        goto end;
    }
    if ( avformat_find_stream_info( pAVContext, NULL ) < 0 )
    {
        wcscpy(pParent->m_strErr, L"Cannot get stream info");
        pParent->m_bErr = true;
        goto end;
    }
    snprintf( pAVContext->filename, sizeof( pAVContext->filename ), "%s", pParent->m_strURL );

    //search video stream
    indexVideoStream = -1;
    for ( unsigned int i = 0; i < pAVContext->nb_streams; i++ )
    {
        pInputCodecContext = pAVContext->streams[i]->codec;
        if ( pInputCodecContext->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            //pInputStream = pAVContext->streams[i];
            indexVideoStream = i;
            break;
        }
    }
    if ( indexVideoStream < 0 )
    {
        wcscpy(pParent->m_strErr, L"Cannot find input video stream");
        pParent->m_bErr = true;
        goto end;
    }

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pInputCodecContext->codec_id);
    if (!pCodec)
    {
        wcscpy(pParent->m_strErr, L"Cannot find input decoder stream");
        pParent->m_bErr = true;
        goto end;
    }
    // Open codec
    if(avcodec_open2(pInputCodecContext, pCodec, NULL) < 0)
    {
        wcscpy(pParent->m_strErr, L"Cannot open decoder stream");
        pParent->m_bErr = true;
        goto end;
    }

     pSwsContext = sws_getCachedContext(NULL,
                                    pInputCodecContext->width, pInputCodecContext->height, pInputCodecContext->pix_fmt,
                                    pInputCodecContext->width, pInputCodecContext->height,
                                    PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
   if (!pSwsContext)
   {
        wcscpy(pParent->m_strErr, L"Cannot initialize the conversion context");
        pParent->m_bErr = true;
        avcodec_close(pInputCodecContext);
        goto end;
   }

    //prepare frames
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    if (!pFrame || !pFrameRGB)
    {
        wcscpy(pParent->m_strErr, L"No memory to allocate frames");
        pParent->m_bErr = true;
        sws_freeContext(pSwsContext);
        avcodec_close(pInputCodecContext);
        goto end;
    }
    numBytes = avpicture_get_size(PIX_FMT_BGR24, pInputCodecContext->width,
			      pInputCodecContext->height);
	pBuffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	avpicture_fill((AVPicture *)pFrameRGB, pBuffer, PIX_FMT_BGR24,
		  pInputCodecContext->width,  pInputCodecContext->height);

    //start reading packets from stream
    av_dump_format(pAVContext, 0, pAVContext->filename, 0 );
    av_init_packet(&pkt);
    while (av_read_frame(pAVContext, &pkt ) >= 0 && !(pParent->m_bStopThread))
    {
        if(pkt.stream_index == indexVideoStream)
        {
            int frameFinished;
            if (avcodec_decode_video2(pInputCodecContext, pFrame, &frameFinished, &pkt) > 0)
            {
                if(frameFinished)
                {
                    int iRes = 0;
                    iRes = sws_scale(pSwsContext, pFrame->data, pFrame->linesize, 0, pInputCodecContext->height,
                              pFrameRGB->data, pFrameRGB->linesize);
                    if (iRes == pInputCodecContext->height)
                        pParent->PeekData(pFrameRGB, pInputCodecContext->width,  pInputCodecContext->height);
                }
            }
        }
        av_free_packet( &pkt );
        av_init_packet( &pkt );
    }

    sws_freeContext(pSwsContext);
    av_free(pBuffer);
    av_free(pFrameRGB);
    av_free(pFrame);
    avcodec_close(pInputCodecContext);
 end:
    avformat_free_context(pAVContext);
	pParent->m_bThreadRun = false;
	pParent->m_bStopThread = false;
	_endthread();
}

void CIPCamConnect::PeekData(AVFrame *pFrame, int iWidth, int iHeight)
{
    if (!pFrame)
        return;
    int iSize;

    EnterCriticalSection(&m_cSection);
    m_iWidth = iWidth;
    m_iHeight = iHeight;
    iSize = 3 * m_iWidth * m_iHeight;
    if (iSize == 0)
    {
        LeaveCriticalSection(&m_cSection);
        return;
    }
    if (m_pSumBuf)
    {
        delete [] m_pSumBuf;
        m_pSumBuf = NULL;
    }
    m_pSumBuf = new unsigned char[iSize];
    if (m_pSumBuf)
    {
       memcpy(m_pSumBuf, pFrame->data[0], iSize);
    }
    LeaveCriticalSection(&m_cSection);
}

bool CIPCamConnect::CheckErr(wchar_t* strErr)
{
	if (m_bErr &&  strErr)
	{
		wcscpy(strErr, m_strErr);
	}
	return m_bErr;
}

void CIPCamConnect::Lock()
{
    EnterCriticalSection(&m_cSection);
}

void CIPCamConnect::Unlock()
{
    LeaveCriticalSection(&m_cSection);
}

void CIPCamConnect::GetBuffer(unsigned char* pBuf)
{
    if (!pBuf || !m_pSumBuf)
	{
	    return;
	}
	int iSumBufSize = 3 * m_iWidth * m_iHeight;
    memcpy(pBuf, m_pSumBuf, iSumBufSize);
    delete [] m_pSumBuf;
    m_pSumBuf = NULL;
    m_iWidth = 0;
    m_iHeight = 0;
}

int CIPCamConnect::GetSize(int &iWidth, int &iHeight)
{
    if (!m_iWidth || !m_iHeight)
        return 0;
    iWidth = m_iWidth;
    iHeight = m_iHeight;
    return 3 * m_iWidth * m_iHeight;
}

void CIPCamConnect::GetURL(char** strURL)
{
    *strURL = m_strURL;
}

///////////////////////////////////////////////////////////////////////////
// a sample exported function
DLL_EXPORT   ICIPCamConnect* WINAPI CreateIPCamConnectClass(const LPCSTR strURL)
{
    return new CIPCamConnect(strURL);
}
