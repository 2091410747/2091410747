//////////////////////////////////////////
//#include "stdafx.h"
//#include "RZEngine_global.h"
#include "RZFFMpegCapture.h"
//#include <chrono>
#if defined(__APPLE__)
#define AV_NOPTS_VALUE_ ((int64_t)0x8000000000000000LL)
#else
#define AV_NOPTS_VALUE_ ((int64_t)AV_NOPTS_VALUE)
#endif

#pragma warning( disable: 4067 4996 4065 4060 )
#pragma comment(lib,"Winmm.lib")
#ifdef QT_PROJECT
#include <QDateTime>

#ifndef CALC_FFMPEG_VERSION
#define CALC_FFMPEG_VERSION(a,b,c) ( a<<16 | b<<8 | c )
#endif

#endif

#ifndef QT_PROJECT
CRZFFMpegCapture::CRZFFMpegCapture(LPRZMediaData lpData, LPCTSTR lpszFile, bool bInvertFrame, int nSwsFilter)
#else
CRZFFMpegCapture::CRZFFMpegCapture(void * lpData, const QString& lpszFile, bool bInvertFrame, int nSwsFilter)
#endif
{
	//m_lpData = (LPRZMediaData)lpData;
	m_lpData = lpData;
	m_nSwsFilter = nSwsFilter;
	m_nOutFormat = AV_PIX_FMT_RGB32/*AV_PIX_FMT_RGB24*/;
	m_bInvertFrame = bInvertFrame;
	m_nCurFrameIndex = 0;
	m_bSupportSink = true;
	m_nSrcVideoWidth = m_nSrcVideoHeight = 0;
	dataBuffer = NULL;
	init();

//	fmt_ctx = NULL;
	//dec_ctx = NULL;
	buffersink_ctx = NULL;
	buffersrc_ctx = NULL;
	filter_graph = NULL;
	//video_stream_index = -1;
	last_pts = AV_NOPTS_VALUE;

	m_strFileName = lpszFile;

#ifndef QT_PROJECT
	char szMediaFile[RZ_MAX_PATH] = { 0 };
	int len = WideCharToMultiByte(CP_UTF8, 0, lpszFile, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, lpszFile, -1, szMediaFile, len, NULL, NULL);
	m_bFailed = !open(szMediaFile);
#else
	m_bFailed = !open((const char *)lpszFile.toLocal8Bit());
#endif
}


CRZFFMpegCapture::~CRZFFMpegCapture(void)
{
	close();
}

void CRZFFMpegCapture::init()
{
#ifndef QT_PROJECT
	m_strFileName = L"";
#else
	m_strFileName = "";
#endif
	m_bFailed = false;
	ic = 0;
	video_stream = -1;
	video_st = 0;
	picture = 0;
	filt_frame = 0;
	picture_pts = AV_NOPTS_VALUE_;
	first_frame_number = -1;
	memset(&packet, 0, sizeof(packet));
	av_init_packet(&packet);
	img_convert_ctx = 0;

	avcodec = 0;
	frame_number = 0;
	eps_zero = 0.000025;
}

int CRZFFMpegCapture::get_number_of_cpus(void)
{
//#ifndef QT_PROJECT
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	return (int)sysinfo.dwNumberOfProcessors;
//#else
//	return CRZGlobal::get_number_of_cpus();
//#endif
}

QString CRZFFMpegCapture::GetMediaFile()
{
	return m_strFileName;
}

#ifndef QT_PROJECT
void CRZFFMpegCapture::SetMediaFile(LPCTSTR lpszFile)
#else
void CRZFFMpegCapture::SetMediaFile(const QString & lpszFile)
#endif
{
	m_strFileName = lpszFile;
}

bool CRZFFMpegCapture::open( const char* _filename )
{
	unsigned i;
	bool valid = false;

	close();

	//USES_CONVERSION;
	//m_strFileName = A2W(_filename);

#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(52, 111, 0)
	int err = avformat_open_input(&ic, _filename, NULL, NULL);
#else
	int err = av_open_input_file(&ic, _filename, NULL, 0, NULL);
#endif

	if (err < 0)
	{
		//CV_WARN("Error opening file");
		goto exit_func;
	}
	err =
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 6, 0)
		avformat_find_stream_info(ic, NULL);
#else
		av_find_stream_info(ic);
#endif
	if (err < 0)
	{
		//CV_WARN("Could not find codec parameters");
		goto exit_func;
	}
	for(i = 0; i < ic->nb_streams; i++)
	{
#if LIBAVFORMAT_BUILD > 4628
		AVCodecContext *enc = ic->streams[i]->codec;
#else
		AVCodecContext *enc = &ic->streams[i]->codec;
#endif

		//#ifdef FF_API_THREAD_INIT
		//        avcodec_thread_init(enc, get_number_of_cpus());
		//#else
		enc->thread_count = get_number_of_cpus();
		//#endif

#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 2, 0)
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#endif

		if( AVMEDIA_TYPE_VIDEO == enc->codec_type && video_stream < 0)
		{
			// backup encoder' width/height
			int enc_width = enc->width;
			int enc_height = enc->height;

			AVCodec *codec = avcodec_find_decoder(enc->codec_id);
			if (!codec ||
#if LIBAVCODEC_VERSION_INT >= ((53<<16)+(8<<8)+0)
				avcodec_open2(enc, codec, NULL)
#else
				avcodec_open(enc, codec)
#endif
				< 0)
			{
				
					goto exit_func;
			}
			// checking width/height (since decoder can sometimes alter it, eg. vp6f)
			if (enc_width && (enc->width != enc_width)) { enc->width = enc_width; }
			if (enc_height && (enc->height != enc_height)) { enc->height = enc_height; }

			m_nSrcVideoWidth = enc_width;
			m_nSrcVideoHeight = enc_height;

			video_stream = i;
			video_st = ic->streams[i];
			picture = av_frame_alloc();
			filt_frame = av_frame_alloc();

			break;
		}
	}

	if(video_stream >= 0) 
	{
		valid = true;
	}

exit_func:

	if( !valid )
	{
		close();
	}
	else
	{
		//nextFrame();
	}

	return valid;
}

void CRZFFMpegCapture::close()
{
	if (dataBuffer)
	{
		delete [] dataBuffer;
		dataBuffer = NULL;
	}

	if( img_convert_ctx )
	{
		sws_freeContext(img_convert_ctx);
		img_convert_ctx = 0;
	}

	if (filt_frame)
		av_free(filt_frame);
	if( picture )
		av_free(picture);

	if( video_st )
	{
#if LIBAVFORMAT_BUILD > 4628
		avcodec_close( video_st->codec );

#else
		avcodec_close( &(video_st->codec) );

#endif
		video_st = NULL;
	}

	if( ic )
	{
		avformat_close_input(&ic);
		ic = NULL;
	}

	// free last packet if exist
	if (packet.data) {
		av_free_packet (&packet);
		packet.data = NULL;
	}

	if (filter_graph)
	{
		avfilter_graph_free(&filter_graph);
	}

	init();
}

////////////////////////////////////////////
bool CRZFFMpegCapture::grabFrame()
{
	bool valid = false;
	int got_picture;

	int count_errs = 0;
	const int max_number_of_attempts = 1 << 9;

	if( !ic || !video_st )  return false;

	if( ic->streams[video_stream]->nb_frames > 0 &&
		frame_number > ic->streams[video_stream]->nb_frames )
		return false;

	av_free_packet (&packet);

	//picture_pts = AV_NOPTS_VALUE_;

	// get the next frame
	while (!valid)
	{
		int ret = av_read_frame(ic, &packet);
		if (ret == AVERROR(EAGAIN)) 
		{
			continue;
		}

		/* else if (ret < 0) break; */

		if( packet.stream_index != video_stream)
		{
			av_free_packet (&packet);
			count_errs++;
			if (count_errs > max_number_of_attempts)
			{
				break;
			}

			continue;
		}

		// Decode video frame
		avcodec_decode_video2(video_st->codec, picture, &got_picture, &packet);

		// Did we get a video frame?
		if(got_picture)
		{
			{
				picture_pts = av_frame_get_best_effort_timestamp(picture);///new add
			}

			frame_number++;
			valid = true;

			if (m_bInvertFrame)
			{
				picture->data[0] = picture->data[0]+picture->linesize[0] * (picture->height-1);  
				picture->data[1] = picture->data[1]+picture->linesize[1] * (picture->height/2-1);  
				picture->data[2] = picture->data[2]+picture->linesize[2] * (picture->height/2-1);  
				picture->linesize[0] *= -1;  
				picture->linesize[1] *= -1;  
				picture->linesize[2] *= -1;
			}
		}
		else
		{
			count_errs++;
			if (count_errs > max_number_of_attempts)
			{
				break;
			}
		}

		av_free_packet (&packet);
	}

	if( valid && first_frame_number < 0 )
		first_frame_number = dts_to_frame_number(picture_pts);

	// return if we have a new picture or not
	return valid;
}

double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

double CRZFFMpegCapture::r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

double CRZFFMpegCapture::get_duration_sec()
{
	double sec = (double)ic->duration / (double)AV_TIME_BASE;

	if (sec < eps_zero)
	{
		sec = (double)ic->streams[video_stream]->duration * r2d(ic->streams[video_stream]->time_base);
	}

	if (sec < eps_zero)
	{
		sec = (double)ic->streams[video_stream]->duration * r2d(ic->streams[video_stream]->time_base);
	}

	return sec;
}

int CRZFFMpegCapture::get_bitrate()
{
	return ic->bit_rate;
}

double CRZFFMpegCapture::get_fps()
{
	double fps = r2d(ic->streams[video_stream]->r_frame_rate);
	
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(52, 111, 0)
	if (fps < eps_zero)
	{
		fps = r2d(ic->streams[video_stream]->avg_frame_rate);
	}
#endif

	if (fps < eps_zero)
	{
		fps = 1.0 / r2d(ic->streams[video_stream]->codec->time_base);
	}

	return fps;
}

int64_t CRZFFMpegCapture::get_total_frames()
{
	int64_t nbf = ic->streams[video_stream]->nb_frames;

	if (nbf == 0)
	{
		nbf = (int64_t)floor(get_duration_sec() * get_fps() + 0.0);
	}
	return nbf;
}

int64_t CRZFFMpegCapture::dts_to_frame_number(int64_t dts)
{
	double sec = dts_to_sec(dts);
	return (int64_t)(get_fps() * sec + 0.0);
}

double CRZFFMpegCapture::dts_to_sec(int64_t dts)
{
	return (double)(dts - ic->streams[video_stream]->start_time) *
		r2d(ic->streams[video_stream]->time_base);
}

bool CRZFFMpegCapture::nextFrame()
{
	//return grabFrame();

	bool valid = false;
	int got_picture = 0;

	int count_errs = 0;
	const int max_number_of_attempts = 1 << 9;

	if( !ic || !video_st )  return false;

	av_free_packet (&packet);

	//packet.pts = AV_NOPTS_VALUE_;

	// get the next frame
	while (!valid)
	{
		int ret = av_read_frame(ic, &packet);
		if (ret == AVERROR(EAGAIN)) 
		{
			continue;
		}

		if( packet.stream_index != video_stream )
		{
			av_free_packet (&packet);
			count_errs++;
			if (count_errs > max_number_of_attempts)
			{
				break;
			}
			continue;
		}
		
		// Decode video frame
		avcodec_decode_video2(video_st->codec, picture, &got_picture, &packet);

		// Did we get a video frame?
		if(got_picture)
		{
			valid = true;

			picture_pts = (packet.pts != AV_NOPTS_VALUE_ && packet.pts != 0 ? packet.pts : packet.dts);

			//new
			picture->pkt_pts = av_frame_get_best_effort_timestamp(picture);
			
			//倒转
			if (m_bInvertFrame)
			{
				picture->data[0] = picture->data[0]+picture->linesize[0] * (picture->height-1);  
				picture->data[1] = picture->data[1]+picture->linesize[1] * (picture->height/2-1);  
				picture->data[2] = picture->data[2]+picture->linesize[2] * (picture->height/2-1);  
				picture->linesize[0] *= -1;  
				picture->linesize[1] *= -1;  
				picture->linesize[2] *= -1;
			}
		}
		else
		{
			count_errs++;
			if (count_errs > max_number_of_attempts)
			{
				break;
			}
		}

		av_free_packet (&packet);
	}

	return valid;
}

void CRZFFMpegCapture::_seek(int64_t _frame_number)
{
	//m_nCurFrameIndex = _frame_number;
	_frame_number = min(_frame_number, get_total_frames());
	int delta = 16;

	// if we have not grabbed a single frame before first seek, let's read the first frame
	// and get some valuable information during the process
	if( first_frame_number < 0 && get_total_frames() > 1 )
		grabFrame();

	for(;;)
	//for (int i(0); i < 1; i++)
	{
		int64_t _frame_number_temp = max(_frame_number-delta, (int64_t)0);
		double sec = (double)_frame_number_temp / get_fps();
		int64_t time_stamp = ic->streams[video_stream]->start_time;
		double  time_base  = r2d(ic->streams[video_stream]->time_base);
		time_stamp += (int64_t)(sec / time_base + 0.0);
		if (get_total_frames() > 1) 
		{
			av_seek_frame(ic, video_stream, time_stamp, AVSEEK_FLAG_BACKWARD);
		}
		
		avcodec_flush_buffers(ic->streams[video_stream]->codec);
		
		if( _frame_number > 0 )
		{
			grabFrame();
			
			if( _frame_number > 1 )
			{
				frame_number = dts_to_frame_number(picture_pts) - first_frame_number;
				
				if( frame_number < 0 || frame_number > _frame_number-1 )
				{
					if( _frame_number_temp == 0 || delta >= INT_MAX/4 )
						break;
					delta = delta < 16 ? delta*2 : delta*3/2;
					continue;
				}
				
				//while( frame_number < _frame_number-1 )
				{
					if(!grabFrame())
						break;
				}
				
				frame_number++;
				break;
			}
			else
			{
				frame_number = 1;
				break;
			}
		}
		else
		{
			frame_number = 0;
			break;
		}
	}
}

void CRZFFMpegCapture::seek(double sec)
{
	/*avcodec_flush_buffers(ic->streams[video_stream]->codec);
	double  time_base  = r2d(ic->streams[video_stream]->time_base);
	int64_t time_stamp = ic->streams[video_stream]->start_time + (int64_t)(sec / time_base + 0.0);
	av_seek_frame(ic, video_stream, time_stamp, AVSEEK_FLAG_BACKWARD);
	nextFrame();*/
	
	avcodec_flush_buffers(ic->streams[video_stream]->codec);
	int64_t nf = sec * get_fps() - 1;
	if (nf < 0)
	{
		nf = 0;
	}
	_seek(nf);
	nextFrame();
	
	int64_t npts = picture->pkt_pts;
	if (npts <= 0)
	{
		npts = picture->pkt_dts;
	}

	bool bReverse = false;
	if (m_lpData)
	{
//		bReverse = m_lpData->bReversePlayback;
	}
	bool bPlay = true;
// 	if (g_pEngineMappingData)
// 	{
// 		bPlay = g_pEngineMappingData->bForPlay;
// 	}

	double dDel = 0.01;
	if (bReverse && !bPlay)
	{
		dDel = 0.001;
	}

	if (npts > 0)//find adjust more nearly, slow mode
	{
		double d = double(npts) * r2d(ic->streams[video_stream]->time_base); 
//#ifndef QT_PROJECT
		DWORD l1 = timeGetTime();
//#else
//		unsigned long l1 = QDateTime::currentDateTime().toTime_t();
		//auto l1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
//#endif
		
		while (sec - d > dDel)
		{
			if (nextFrame())
			{
				npts = picture->pkt_pts;
				if (npts <= 0)
				{
					npts = picture->pkt_dts;
				}
				d = double(npts) * r2d(ic->streams[video_stream]->time_base); 
				if (bReverse && bPlay)
				{
//#ifndef QT_PROJECT
					DWORD l2 = timeGetTime();
//#else
//					unsigned long l2 = QDateTime::currentDateTime().toTime_t();
//					auto l2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
//#endif
					
					if (l2 - l1 > 100)
					{
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
	}
}

bool CRZFFMpegCapture::retrieveFrameEx(unsigned char** data, int wOut, int hOut)//wh必须是8的倍数
{
	if( !video_st || !picture->data)
	{
		return false;
	}

	if (filter_graph == NULL)
	{
		char filter_descr[1000];
		int x, y, w, h;
		bool bCrop = false;
// 		if (m_lpData != nullptr && m_lpData->pCropInfo)
// 		{
// 			if ( m_lpData->pCropInfo->bEnable)
// 			{
// 				x = m_lpData->pCropInfo->x;
// 				y = m_lpData->pCropInfo->y;
// 				w = m_lpData->pCropInfo->width;
// 				h = m_lpData->pCropInfo->height;
// 				if (x < 0) x = 0;
// 				if (y < 0) y = 0;
// 				if (x + w > m_nSrcVideoWidth) w = m_nSrcVideoWidth - x;
// 				if (y + h > m_nSrcVideoHeight) h = m_nSrcVideoHeight - y;
// 				if (w < 0) w = 0;
// 				if (h < 0) h = 0;
// 				if (x == 0 && y == 0 && (x + w == m_nSrcVideoWidth) && (y + h == m_nSrcVideoHeight))// && w != m_nSrcVideoWidth && h != m_nSrcVideoHeight)
// 				{
// 					bCrop = false;
// 				}
// 				else if (w > 0 && h > 0)
// 				{
// 					bCrop = true;
// 				}
// 			}
// 		}

		bool bFillBlur = false;
// 		if (m_lpData != nullptr && m_lpData->eImageFillMode == RIRM_FILL_KEEP_RATIO && m_lpData->bFillBlurToBlankWhenKeepRatio && m_lpData->eMediaTypeLine == RZMEDIATYPE_Line_Video)
// 		{
// 			bFillBlur = true;
// 		}

		if (!bFillBlur)
		{
			if (bCrop)
			{
				sprintf_s(filter_descr, "crop=%d:%d:%d:%d,scale=%d:%d", w, h, x, y, wOut, hOut);
			}
			else
			{
				//sprintf_s(filter_descr, "scale=%d:%d", wOut, hOut);
				//sprintf_s(filter_descr, "scale=%d:%d,split[1][2];[1]drawbox=c=red:t=%d[3];[2]scale=%d:%d[4];[3][4]overlay=x=10:y=10", wOut, hOut, max(wOut, hOut), wOut - 20, hOut - 20);
				sprintf_s(filter_descr, "scale=%d:%d", wOut, hOut);
			}
		}
		else//视频线如果要做fill blur for keep ratio的话直接在这做
		{
			if (bCrop)
			{
#ifndef QT_PROJECT
				CRect rc;
				int wk = rc.Width();
				int hk = rc.Height();
#else
				QRect rc;
				int wk = rc.width();
				int hk = rc.height();
#endif

//				CRZGlobal::GetKeepRatioDestRect(&rc, w, h, wOut, hOut);
				GetKeepRatioDestRect(&rc, w, h, wOut, hOut);
				int xk = (wOut - wk) / 2;
				int yk = (hOut - hk) / 2;
				bool bW = (xk > 0);
				bool bH = (yk > 0);
				if (xk < 2 && yk < 2)
				{
					bW = bH = false;
				}

				if (bW || bH)
				{
					if (bW)
					{
						int cropw = min(xk, wk / 2);
						int croph = hk;
						sprintf_s(filter_descr, 
							"crop=%d:%d:%d:%d,split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=x=%d[bk3];[bk3][src2]overlay=x=%d",
							w, h, x, y, wOut, hOut, wk, hk, cropw, croph, 0, 0, xk + 3, hk, cropw, croph, wk - cropw, 0, xk + 3, hk, xk + wk - 1, xk);
					}
					else
					{
						int croph = min(yk, hk / 2);
						int cropw = wk;
						sprintf_s(filter_descr, 
							"crop=%d:%d:%d:%d,split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=y=%d[bk3];[bk3][src2]overlay=y=%d",
							w, h, x, y, wOut, hOut, wk, hk, cropw, croph, 0, 0, wk, yk + 3, cropw, croph, 0, hk - croph, wk, yk + 3, yk + hk - 1, yk);
					}
				}
				else
				{
					sprintf_s(filter_descr, "crop=%d:%d:%d:%d,scale=%d:%d", w, h, x, y, wOut, hOut);
				}
			}
			else
			{
#ifndef QT_PROJECT
				CRect rc;
				int wk = rc.Width();
				int hk = rc.Height();
#else
				QRect rc;
				int wk = rc.width();
				int hk = rc.height();
#endif
				GetKeepRatioDestRect(&rc, 1024, 768, wOut, hOut);
				
				int xk = (wOut - wk) / 2;
				int yk = (hOut - hk) / 2;
				bool bW = (xk > 0);
				bool bH = (yk > 0);
				if (xk < 2 && yk < 2)
				{
					bW = bH = false;
				}

				if (bW || bH)
				{
					if (bW)
					{
						int cropw = min(xk, wk / 2);
						int croph = hk;
						sprintf_s(filter_descr, 
							"split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=x=%d[bk3];[bk3][src2]overlay=x=%d",
							wOut, hOut, wk, hk, cropw, croph, 0, 0, xk + 3, hk, cropw, croph, wk - cropw, 0, xk + 3, hk, xk + wk - 1, xk);
					}
					else
					{
						int croph = min(yk, hk / 2);
						int cropw = wk;
						sprintf_s(filter_descr, 
							"split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=y=%d[bk3];[bk3][src2]overlay=y=%d",
							wOut, hOut, wk, hk, cropw, croph, 0, 0, wk, yk + 3, cropw, croph, 0, hk - croph, wk, yk + 3, yk + hk - 1, yk);
					}
				}
				else
				{
					sprintf_s(filter_descr, "scale=%d:%d", wOut, hOut);
				}
			}
		}

		if (init_filters(filter_descr) < 0)
		{
			return false;
		}
	}

	/*char sz[100];
	static double q = 0.0;
	q+=0.01;
	if (q > 1.0)
	{
		q = 0.0;
	}
	sprintf_s(sz, "%3.2lf", q);
	if (avfilter_graph_send_command(filter_graph, "Parsed_rotate_0", "angle", sz, NULL, 0, 0) < 0)
	{
		return false;
	}*/

	if (av_buffersrc_add_frame_flags(buffersrc_ctx, picture, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) 
	{
		return false;
	}

	int ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
	if (ret < 0)
	{
		av_frame_unref(filt_frame);
		return false;
	}

	img_convert_ctx = sws_getCachedContext(
		img_convert_ctx,
		wOut, hOut, /*video_st->codec->width, video_st->codec->height,*/
		(AVPixelFormat)filt_frame->format,/*video_st->codec->pix_fmt,*/
		wOut, hOut,
		m_nOutFormat,
		m_nSwsFilter,//SWS_BICUBIC,SWS_FAST_BILINEAR
		NULL, NULL, NULL
		);

	if (img_convert_ctx == NULL)
	{
		av_frame_unref(filt_frame);
		return false;
	}

	AVFrame rgbFrame;
	rgbFrame.data[0] = *data;
	m_linesize = wOut * (m_nOutFormat == AV_PIX_FMT_RGB24 ? 3 : 4);
	m_imageSize = QSize(wOut, hOut);
	rgbFrame.linesize[0] = m_linesize;
	rgbFrame.width = wOut;
	rgbFrame.height = hOut;

	sws_scale(
		img_convert_ctx,
		filt_frame->data,
		filt_frame->linesize,
		0, hOut,
		rgbFrame.data,
		rgbFrame.linesize
		);

	av_frame_unref(filt_frame);

	return true;
}

int CRZFFMpegCapture::init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");     /* 输入buffer filter */
    const AVFilter *buffersink = avfilter_get_by_name("buffersink"); /* 输出buffer filter */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = ic->streams[video_stream]->time_base;   /* 时间基数 */
	AVCodecContext *dec_ctx = ic->streams[video_stream]->codec;
	
/*#ifndef SAVE_FILE
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
#else
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
#endif*/

	enum AVPixelFormat pix_fmts[] = {/*AV_PIX_FMT_RGB32*/AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();                     /* 创建graph  */
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    /* 创建并向FilterGraph中添加一个Filter */
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);           
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);          
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

     /* Set a binary option to an integer list. */
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);   
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    /* Add a graph described by a string to a graph */
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)    
        goto end;

    /* Check validity and configure all the links and formats in the graph */
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)   
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

// bool CRZFFMpegCapture::retrieveFrameEx_LyricBackground(unsigned char** data, int wOut, int hOut, bool bFillBlurForKeepRatio /*= false*/, LPRZCropInfo pCrop /*= NULL*/)
// {
// 	if( !video_st || !picture->data)
// 	{
// 		return false;
// 	}
// 
// 	if (filter_graph == NULL)
// 	{
// 		char filter_descr[1000];
// 		int x, y, w, h;
// 		bool bCrop = false;
// 		if (pCrop)
// 		{
// 			if (pCrop->bEnable)
// 			{
// 				x = pCrop->x;
// 				y = pCrop->y;
// 				w = pCrop->width;
// 				h = pCrop->height;
// 				if (x < 0) x = 0;
// 				if (y < 0) y = 0;
// 				if (x + w > m_nSrcVideoWidth) w = m_nSrcVideoWidth - x;
// 				if (y + h > m_nSrcVideoHeight) h = m_nSrcVideoHeight - y;
// 				if (w < 0) w = 0;
// 				if (h < 0) h = 0;
// 				if (x == 0 && y == 0 && (x + w == m_nSrcVideoWidth) && (y + h == m_nSrcVideoHeight))// && w != m_nSrcVideoWidth && h != m_nSrcVideoHeight)
// 				{
// 					bCrop = false;
// 				}
// 				else if (w > 0 && h > 0)
// 				{
// 					bCrop = true;
// 				}
// 			}
// 		}
// 
// 		bool bFillBlur = bFillBlurForKeepRatio;
// 		
// 		if (!bFillBlur)
// 		{
// 			if (bCrop)
// 			{
// 				sprintf_s(filter_descr, "crop=%d:%d:%d:%d,scale=%d:%d", w, h, x, y, wOut, hOut);
// 			}
// 			else
// 			{
// 				sprintf_s(filter_descr, "scale=%d:%d", wOut, hOut);
// 			}
// 		}
// 		else
// 		{
// 			if (bCrop)
// 			{
// #ifndef QT_PROJECT
// 				CRect rc;
// 				int wk = rc.Width();
// 				int hk = rc.Height();
// #else
// 				QRect rc;
// 				int wk = rc.width();
// 				int hk = rc.height();
// #endif
// 				CRZGlobal::GetKeepRatioDestRect(&rc, w, h, wOut, hOut);
// 				
// 				int xk = (wOut - wk) / 2;
// 				int yk = (hOut - hk) / 2;
// 				bool bW = (xk > 0);
// 				bool bH = (yk > 0);
// 				if (xk < 2 && yk < 2)
// 				{
// 					bW = bH = false;
// 				}
// 
// 				if (bW || bH)
// 				{
// 					if (bW)
// 					{
// 						int cropw = min(xk, wk / 2);
// 						int croph = hk;
// 						sprintf_s(filter_descr, 
// 							"crop=%d:%d:%d:%d,split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=x=%d[bk3];[bk3][src2]overlay=x=%d",
// 							w, h, x, y, wOut, hOut, wk, hk, cropw, croph, 0, 0, xk + 3, hk, cropw, croph, wk - cropw, 0, xk + 3, hk, xk + wk - 1, xk);
// 					}
// 					else
// 					{
// 						int croph = min(yk, hk / 2);
// 						int cropw = wk;
// 						sprintf_s(filter_descr, 
// 							"crop=%d:%d:%d:%d,split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=y=%d[bk3];[bk3][src2]overlay=y=%d",
// 							w, h, x, y, wOut, hOut, wk, hk, cropw, croph, 0, 0, wk, yk + 3, cropw, croph, 0, hk - croph, wk, yk + 3, yk + hk - 1, yk);
// 					}
// 				}
// 				else
// 				{
// 					sprintf_s(filter_descr, "crop=%d:%d:%d:%d,scale=%d:%d", w, h, x, y, wOut, hOut);
// 				}
// 			}
// 			else
// 			{
// #ifndef QT_PROJECT
// 				CRect rc;
// 				int wk = rc.Width();
// 				int hk = rc.Height();
// #else
// 				QRect rc;
// 				int wk = rc.width();
// 				int hk = rc.height();
// #endif
// 				CRZGlobal::GetKeepRatioDestRect(&rc, m_nSrcVideoWidth, m_nSrcVideoHeight, wOut, hOut);
// 				
// 				int xk = (wOut - wk) / 2;
// 				int yk = (hOut - hk) / 2;
// 				bool bW = (xk > 0);
// 				bool bH = (yk > 0);
// 				if (xk < 2 && yk < 2)
// 				{
// 					bW = bH = false;
// 				}
// 
// 				if (bW || bH)
// 				{
// 					if (bW)
// 					{
// 						int cropw = min(xk, wk / 2);
// 						int croph = hk;
// 						sprintf_s(filter_descr, 
// 							"split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=x=%d[bk3];[bk3][src2]overlay=x=%d",
// 							wOut, hOut, wk, hk, cropw, croph, 0, 0, xk + 3, hk, cropw, croph, wk - cropw, 0, xk + 3, hk, xk + wk - 1, xk);
// 					}
// 					else
// 					{
// 						int croph = min(yk, hk / 2);
// 						int cropw = wk;
// 						sprintf_s(filter_descr, 
// 							"split[bk][src];[bk]scale=%d:%d[bk1];[src]scale=%d:%d[src1];[src1]split=3[left][src2][right];[left]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[left1];[right]crop=%d:%d:%d:%d,scale=%d:%d,gblur=20:1[right1];[bk1][left1]overlay[bk2];[bk2][right1]overlay=y=%d[bk3];[bk3][src2]overlay=y=%d",
// 							wOut, hOut, wk, hk, cropw, croph, 0, 0, wk, yk + 3, cropw, croph, 0, hk - croph, wk, yk + 3, yk + hk - 1, yk);
// 					}
// 				}
// 				else
// 				{
// 					sprintf_s(filter_descr, "scale=%d:%d", wOut, hOut);
// 				}
// 			}
// 		}
// 
// 		if (init_filters(filter_descr) < 0)
// 		{
// 			return false;
// 		}
// 	}
// 
// 	if (av_buffersrc_add_frame_flags(buffersrc_ctx, picture, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) 
// 	{
// 		return false;
// 	}
// 
// 	int ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
// 	if (ret < 0)
// 	{
// 		av_frame_unref(filt_frame);
// 		return false;
// 	}
// 
// 	img_convert_ctx = sws_getCachedContext(
// 		img_convert_ctx,
// 		wOut, hOut,
// 		(AVPixelFormat)filt_frame->format/*video_st->codec->pix_fmt*/,
// 		wOut, hOut,
// 		m_nOutFormat,
// 		m_nSwsFilter,//SWS_BICUBIC,SWS_FAST_BILINEAR
// 		NULL, NULL, NULL
// 		);
// 
// 	if (img_convert_ctx == NULL)
// 	{
// 		av_frame_unref(filt_frame);
// 		return false;
// 	}
// 
// 	AVFrame rgbFrame;
// 	rgbFrame.data[0] = *data;
// 	rgbFrame.linesize[0] = wOut * (m_nOutFormat == AV_PIX_FMT_RGB24 ? 3 : 4);
// 	rgbFrame.width = wOut;
// 	rgbFrame.height = hOut;
// 
// 	sws_scale(
// 		img_convert_ctx,
// 		filt_frame->data,
// 		filt_frame->linesize,
// 		0, hOut,
// 		rgbFrame.data,
// 		rgbFrame.linesize
// 		);
// 
// 	av_frame_unref(filt_frame);
// 
// 	return true;
// }

void CRZFFMpegCapture::GetKeepRatioDestRect(QRect* lpQRect, int srcw, int srch, int dstw, int dsth)
{
	int w = srcw;
	int h = srch;
	double ds = double(w) / double(h);
	int wi = dstw;
	int hi = double(wi) / ds;
	if (hi > dsth)
	{
		hi = dsth;
		wi = double(hi) * ds;
	}

	lpQRect->setLeft((dstw - wi) / 2);
	lpQRect->setTop((dsth - hi) / 2);
	lpQRect->setRight(lpQRect->left() + wi);
	lpQRect->setBottom(lpQRect->top() + hi);
}