#ifdef _MSC_VER
#pragma once
#endif

#ifndef __RZFFMPEGCAPTURE_H__
#define __RZFFMPEGCAPTURE_H__

#	ifdef __cplusplus
extern "C" {
#	endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswresample/swresample.h"
#	ifdef __cplusplus
}
#	endif

#include "irrTypes.h"
//#include "RZEngine_global.h"
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")
#include <Windows.h>

#include <QString>
#include <QSize>
#include <QRect>
class CRZFFMpegCapture
{
public:
#ifndef QT_PROJECT
	CRZFFMpegCapture(LPRZMediaData lpData, LPCTSTR lpszFile, bool bInvertFrame, int nSwsFilter);
#else
	CRZFFMpegCapture(void* lpData, const QString & lpszFile, bool bInvertFrame, int nSwsFilter);
#endif
	virtual ~CRZFFMpegCapture(void);
	bool m_bFailed;
	AVPixelFormat m_nOutFormat;
	int m_nSwsFilter;
	int m_nSrcVideoWidth;
	int m_nSrcVideoHeight;
	unsigned char* dataBuffer;
	void* m_lpData;
	
	void init();//
	int get_number_of_cpus(void);//
	bool open( const char* filename );//
    void close();//

    bool grabFrame();
	bool retrieveFrameEx(unsigned char** data, int wOut, int hOut);
//	bool retrieveFrameEx_LyricBackground(unsigned char** data, int wOut, int hOut, bool bFillBlurForKeepRatio = false, LPRZCropInfo pCrop = NULL);

    void    _seek(int64_t frame_number);
    void    seek(double sec);
    bool    slowSeek( int framenumber );
	bool nextFrame();

    int64_t get_total_frames();
    double  get_duration_sec();
    double  get_fps();
    int     get_bitrate();

    double  r2d(AVRational r);
    int64_t dts_to_frame_number(int64_t dts);
    double  dts_to_sec(int64_t dts);

    AVFormatContext * ic;
    AVCodec         * avcodec;
    int               video_stream;
    AVStream        * video_st;
    AVFrame         * picture;
    int64_t           picture_pts;

    AVPacket          packet;
    struct SwsContext *img_convert_ctx;

    int64_t frame_number, first_frame_number;
	int64_t m_nCurFrameIndex;

    double eps_zero;


/*
   'filename' contains the filename of the videosource,
   'filename==NULL' indicates that ffmpeg's seek support works
   for the particular file.
   'filename!=NULL' indicates that the slow fallback function is used for seeking,
   and so the filename is needed to reopen the file on backward seeking.
*/
    //char              * filename;
	QString m_strFileName;
	QString GetMediaFile();
#ifndef QT_PROJECT
	void SetMediaFile(LPCTSTR lpszFile);
#else
	void SetMediaFile(const QString & lpszFile);
#endif
	///////////////////////////////////////
	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph *filter_graph;
	AVFrame* filt_frame;// = av_frame_alloc();
	int64_t last_pts;// = AV_NOPTS_VALUE;
	bool m_bSupportSink;
	bool m_bInvertFrame;
	int init_filters(const char *filters_descr);
	void GetKeepRatioDestRect(QRect* lpQRect, int srcw, int srch, int dstw, int dsth);
	int GetLineSize() { return m_linesize;  }
	QSize GetImageSize() { return m_imageSize; }

	int m_linesize;
	QSize m_imageSize;

	void SetOutoutFormat(AVPixelFormat format1) { m_nOutFormat = format1; }
};

#endif//__RZFFMPEGCAPTURE_H__