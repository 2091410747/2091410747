#include "playervideo.h"
#include <QDebug>
#include <QEvent>
#include <QProgressBar>
#include <QPainter>
#include <QMutex>
#include <QProcess>
#include <QKeyEvent>
#include <QFileDialog>

#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"SDL2.lib")
//#pragma comment(lib,"RZPlay.lib")
//extern "C" _declspec(dllimport) int rzplaydll_Play(char* filename, void* /*HWND*/ hWnd, void* /*HWND*/ hNotifyWnd, long lFinishMsg, int bstrectch_tofill, double dStartTime, bool bCaptureInStartTime, const QString & lpszImageSave, bool bDisableVideo);
extern "C" int ffmpeg_play(int argc, char** argv);;
extern "C" void ffmpeg_Stop();
extern "C" void ffmpeg_Pause();
QMutex g_mutex;
static double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

PlayerVideo::PlayerVideo(QWidget *parent)
	: QWidget/*QMainWindow*/(parent)
{
	avcodec_register_all();
	avdevice_register_all();
	avfilter_register_all();
	av_register_all();
	connect(this, SIGNAL(SignalRender()), this, SLOT(OnRender()));
	//ui.setupUi(this);
	resize(670,490);
	m_bGetFirstFrameOfVideo = true;
	m_pirrDevice= nullptr;
	m_pirrVideoDriver= nullptr;
	m_pirrScenceManager= nullptr;
	m_pirrCamera= nullptr;
	m_pdwFFMpeg = 0;
	m_pTex= nullptr;
	m_nMoveX = 0;
	m_nMoveY = 0;
	m_dCurPos = 0;
	m_pThread = new PVThread(this);
	m_nVideoWidth = 0;
	m_nVideoHeight = 0;
	m_strFileName = "2.mp4"; 
	InitUi();

//	if(layout()) layout()->addItem(pLay);
	m_pPlayWnd->setUpdatesEnabled(false);
	irrOpenEngine(m_pPlayWnd->winId());
	connect(&m_timer,SIGNAL(timeout()),this,SLOT(OnTimer()));
	//m_pThread->start();
	FFMpeg_Init(nullptr, 0, 0);

	setFocus();
}

PlayerVideo::~PlayerVideo()
{
	Stop();
	m_pThread->Exit();
	irrCloseEngine();
	SAFE_DELETE(m_pThread);
}

void PlayerVideo::InitUi()
{
	
	QVBoxLayout *pVBoxLay = new QVBoxLayout(this);
	QHBoxLayout *pHBoxLay = new QHBoxLayout();

	QProgressBar *pProgress = new QProgressBar(this);
	pProgress->setObjectName("Progress");
	pProgress->setRange(0,100);
	pProgress->setValue(0);
	
	QPushButton * pOpen = new QPushButton("Open",this);
	connect(pOpen,SIGNAL(clicked()),this,SLOT(OnOpenFile()));
	pVBoxLay->addWidget(pOpen);
	m_pPlayWnd = new QLabel(this);
	pVBoxLay->addWidget(m_pPlayWnd);
	pVBoxLay->addWidget(pProgress);
	pVBoxLay->addLayout(pHBoxLay);

	pHBoxLay->addStretch(1); 
	QPushButton * p = new QPushButton("Play",this);
	pHBoxLay->addWidget(p);
	connect(p,&QPushButton::clicked,this,[this](){ 
		QPushButton* p = (QPushButton*)sender();
		if(p == nullptr) return;
		QString s = p->text();
		if(s == "Play")
		{
			if(m_pdwFFMpeg == 0)
				FFMpeg_Init(nullptr, 0, 0);
			if(!m_pThread->isRunning())
			{
				this->m_pThread->Start();
			}
			if(!m_timer.isActive()) 
				m_timer.start(30);
			p->setText("Stop");
		}
		else
		{
			Stop();

			p->setText("Play");
		}
	});
	
	p = new QPushButton("Pause",this);
	pHBoxLay->addWidget(p);
	connect(p,&QPushButton::clicked,this,[this]()
	{ 
		//Stop();
		if(m_timer.isActive())
		{
			m_timer.stop();
		}
		else
		{
			m_timer.start(30);
		}
		ffmpeg_Pause();
		//this->m_pThread->Exit();
	});
}

void PlayerVideo::Stop()
{
	m_dCurPos = 0;
	m_timer.stop();
	if(m_pThread->isRunning())
	{
		this->m_pThread->Exit();
	}
	SAFE_DELETE_TYPE(CRZFFMpegCapture*,m_pdwFFMpeg);
}

void PlayerVideo::irrOpenEngine(WId hWnd)
{
	//if (hWnd == NULL) return nullptr;
	//m_hCurrentFrameWnd = hWnd;

	irr::SIrrlichtCreationParameters param;
	bool b3D = false;
	HDC irrHDC = NULL;
	HGLRC irrHGLRC = NULL;
	if (b3D)
	{
		param.DriverType = irr::video::EDT_OPENGL;
	}
	else
	{
		param.DriverType = irr::video::/*EDT_SOFTWARE*/EDT_DIRECT3D9;
	}

	//param.DriverMultithreaded = true;
	param.Doublebuffer = true;
	param.ZBufferBits = 24;
	param.AntiAlias = 15;
	param.DeviceType = irr::EIDT_WIN32;
	param.WindowId = reinterpret_cast<void*>(hWnd);
	//创建 Device时，调试时直接 WIN + L 会创建失败！
	m_pirrDevice = createDeviceEx(param);
	m_pirrVideoDriver = m_pirrDevice->getVideoDriver();
	//m_pirrVideoDriver->setUseForConversion(true);

	m_pirrScenceManager = m_pirrDevice->getSceneManager();
	irr::video::SExposedVideoData videodata(0);
	//	if (param.DriverType == irr::video::EDT_OPENGL)
	//	{
	//		HDC HDc = ::GetDC((HWND)hWnd);
	//		PIXELFORMATDESCRIPTOR pfd = { 0 };
	//		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	//		int pf = GetPixelFormat(HDc);
	//		DescribePixelFormat(HDc, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	//		pfd.dwFlags |= PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	//		pfd.cDepthBits = 24;
	//		pfd.iPixelType = PFD_TYPE_RGBA;
	//		pfd.cColorBits = 32;
	//		pf = ChoosePixelFormat(HDc, &pfd);
	//		SetPixelFormat(HDc, pf, &pfd);
	//		if(irrHDC == 0)
	//		{
	//			videodata.OpenGLWin32.HDc = irrHDC = HDc;
	//			videodata.OpenGLWin32.HRc = irrHGLRC = wglCreateContext(HDc);
	//		}
	//		wglShareLists((HGLRC)pirrVideoDriver->getExposedVideoData().OpenGLWin32.HRc, (HGLRC)videodata.OpenGLWin32.HRc);
	//	}


	// 	if (m_pirrVideoDriver->queryFeature(irr::video::EVDF_RENDER_TO_TARGET))
	// 	{
	// 		m_bSupportRenderToTarget = true;
	// 	}
	// 	else
	// 	{
	// 		m_bSupportRenderToTarget = false;
	// 	}

    m_pirrDevice->setResizable(true);

    m_pirrVideoDriver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
    m_pirrVideoDriver->setTextureCreationFlag(irr::video::ETCF_CREATE_MIP_MAPS, false);
    m_pirrVideoDriver->setAllowZWriteOnTransparent(false);

//	m_pirrVideoDriver->setAllowZWriteOnTransparent(true);

    m_pirrCamera = m_pirrScenceManager->addCameraSceneNode(0, irr::core::vector3df(0.0f, 0.0f, -40.0f), irr::core::vector3df(0.0f, 0.0f, 0.0f));
    m_pirrCamera->setNearValue(0.001f);
    m_pirrCamera->setFarValue(10000.0f);

	if(b3D)
	{
		//	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_nOpenGLTextureMaxSize);
		// wcx 0907
		//CRZGlobal::OpenGLInitAllLyricsTexture();
		//	OpenGLCreateAll3DGroupTexture();
		//	if (!g_bUseMultiThreadForConversion)
		{
			//		wglMakeCurrent((HDC)irrHDC, irrHGLRC);
			//SetIrrHDC(GetIrrHDC(), nullptr);
		}
	}

	//return nullptr;
	//m_pTex = m_pirrVideoDriver->getTexture("D:/游侠对战平台logo/abcpic.png");
//	while(m_pirrDevice->run())
//	{
//		m_pirrVideoDriver->beginScene(true,true,irr::video::SColor(255,0,255,0));
//		m_pirrVideoDriver->draw2DImage(pTex,irr::core::position2d<irr::s32>(20,20));
//		m_pirrVideoDriver->endScene();
//		QThread::usleep(10);
//	}
	//return m_pirrVideoDriver;
}

void PlayerVideo::irrCloseEngine()
{
	if (NULL == m_pirrDevice)
	{
		m_pirrScenceManager = NULL;
		m_pirrVideoDriver = NULL;
		m_pirrCamera = NULL;
		//m_hCurrentFrameWnd = NULL;
		return;
	}

	//DeleteAllTimelineTexture3DGroup();

	m_pirrVideoDriver = NULL;
	m_pirrCamera = NULL;
	m_pirrScenceManager = NULL;
	if(m_pirrDevice)
	{
		m_pirrDevice->closeDevice();
		m_pirrDevice->drop();
		m_pirrDevice = nullptr;
	}

// 	if (m_irrHDC)
// 	{
// 		wglMakeCurrent((HDC)m_irrHDC, NULL);
// 		wglDeleteContext((HGLRC)m_irrHGLRC);
// 
// 		if (m_hCurrentFrameWnd)
// 		{
// 			::ReleaseDC((HWND)m_hCurrentFrameWnd, (HDC)m_irrHDC);
// 		}
// 
// 		m_irrHDC = NULL;
// 		m_irrHGLRC = NULL;
// 	}

	m_pirrDevice = NULL;
//	m_hCurrentFrameWnd = NULL;
}

void PlayerVideo::irrBegin()
{
	if(m_pPlayWnd == nullptr) return;
	//irr::core::dimension2d<irr::u32> size1 = m_pirrVideoDriver->getScreenSize();
	//int height = size1.Height;
	//int width = size1.Width;
	//if(m_pPlayWnd->size() != QSize(width,height))
	{
			//if (!m_bShow3DGroup)
			{
				int cx = m_pPlayWnd->rect().width();
				int cy = m_pPlayWnd->rect().height();

				irr::scene::ICameraSceneNode* pC = m_pirrScenceManager->getActiveCamera();

				if (pC)
				{
					irr::core::rect<irr::s32> rvp = m_pirrVideoDriver->getViewPort();

					if (rvp.LowerRightCorner.X != cx || rvp.LowerRightCorner.Y != cy)
					{
						const irr::core::dimension2d<irr::u32> screensize = irr::core::dimension2d<irr::u32>(cx, cy);
						irr::core::rect<irr::s32> rect(0, 0, cx, cy);

						m_pirrVideoDriver->setViewPort(rect);

						//好像解决了dx9 resize挂掉的问题，必须清除粒子系统当前的粒子，否则resize可能挂
						//如果不是opengl，则类保存的纹理对象指针会变化，所以先清理纹理对象指针

// 						if (m_pirrVideoDriver->getDriverType() != irr::video::EDT_OPENGL)
// 						{
// 							irrResetTextureForResize();
// 						}

						m_pirrVideoDriver->OnResize(screensize);

						irr::f32 fAspect = (float)cx / (float)cy;
						if (fAspect >= 0.f)
						{
							pC->setAspectRatio(fAspect);
						}
						else
						{
							Q_ASSERT(false);
						}
					}
				}
			}
	}

	m_pirrVideoDriver->beginScene(true,true,irr::video::SColor(255,0,255,0));
	m_pirrScenceManager->drawAll();
}

void PlayerVideo::irrEnd()
{
	if (m_pirrVideoDriver == nullptr) return;
	bool bReset = m_pirrVideoDriver->checkDriverReset();
	m_pirrVideoDriver->endScene();
}

void PlayerVideo::paintEvent(QPaintEvent *)
{
	//irrRender();
}

void PlayerVideo::OnTimer()
{
	m_dCurPos += 1.0/30;
	m_nMoveX ++ ;
	qDebug() << __FUNCTION__ << m_dCurPos;
	irrRender();
	if(m_nMoveX >= m_pPlayWnd->rect().width())
	{
		m_nMoveY+= 20;
		m_nMoveX = 0;
	}

	QProgressBar* progress = findChild<QProgressBar*>("Progress");
	if(progress == nullptr) return;
	qint64 curDual = m_dCurPos * 1000;
	if(curDual > progress->maximum())
	{
		progress->setValue(progress->maximum());
		Stop();
		return;
	}
	else
		progress->setValue(m_dCurPos*1000);
}

void PlayerVideo::OnOpenFile()
{
	QString strFlieName = QFileDialog::getOpenFileName(nullptr,"Open Video File","","Video File (*.mp4);;MP4 File (*.mp3)",0,QFileDialog::DontUseNativeDialog);
	if(strFlieName.isEmpty()) return;
	m_strFileName = strFlieName;
	SAFE_DELETE_TYPE(CRZFFMpegCapture*,m_pdwFFMpeg);
	if(m_pirrVideoDriver != nullptr)
	{
		m_pirrVideoDriver->removeTexture(m_pTex);
		m_pTex = nullptr;
	}
	FFMpeg_Init(nullptr, 0, 0);
	irrRender();
	setWindowTitle(strFlieName);

}

void PlayerVideo::keyPressEvent(QKeyEvent *e)
{
	e->accept();
	if(e->key() == Qt::Key_Space)
	{
		ffmpeg_Pause();
	}
}

// void PlayerVideo::showEvent(QShowEvent *event)
// {
// 	irrRender();
// }
bool PlayerVideo::event(QEvent *event1)
{
	switch(event1->type())
	{
	case QEvent::UpdateLater:
	case QEvent::WindowActivate:
	case QEvent::ActivationChange:
	case QEvent::UpdateRequest:
		//irrRender();
		break;
	}
	//qDebug() << QEvent::Type(event1->type());
	return QWidget::event(event1);
}

void PlayerVideo::irrRender()
{
	irrBegin();
	QImage img(m_nVideoWidth,m_nVideoHeight,QImage::Format_ARGB32);
	g_mutex.lock();
	uchar* bits = img.bits();
	FFMpeg_GetFrame(m_dCurPos,nullptr,bits, img.width(), img.height(),QRect(0, 0, img.width(), img.height()), img.width(), img.height());
// 	if (m_pTex)
// 	{
// 		m_pirrVideoDriver->removeTexture(m_pTex);
// 		m_pTex = nullptr;
// 	}
	if(m_pTex == nullptr)
	{
		m_pirrVideoDriver->setTextureCreationFlag(irr::video::ETCF_CREATE_MIP_MAPS, true);
		irr::video::IImage *iimg = m_pirrVideoDriver->createImageFromData(irr::video::ECF_A8R8G8B8,irr::core::dimension2du(img.width(),img.height()),bits,true,false);
		m_pTex = m_pirrVideoDriver->addTexture("r", iimg);
		iimg->drop();
		irr::core::dimension2du RZOptimizeSize(0, 0);

		if(m_pTex)
		{
			m_pTex->setRZOptimizeSize(RZOptimizeSize);
		}
	}
	else
	{
	
		QRgb clr = img.pixel(100,100);	
		qDebug()<<__FUNCTION__<<img.pixel(100,100);
		if(!img.isNull() && clr != 0)
		{
			int wi =img.width();
			int hi =img.height();
			uchar* bitsT = img.bits();
			void* pTexData = m_pTex->lock();
			qint64 size = hi * img.bytesPerLine();
			qint64 nbytes = size;
			//qint64 nbytes = img.sizeInBytes();
// 			static int i = 0;
// 			img.save(QString("c:/img/%1.png").arg(i++), "PNG");
			//int nbytes = img.sizeInBytes();
			//pBitmap->save("c:/1.png");
			/*pBitmap->save("c:/image/1.png");*/
			//memcpy(pTexData, bitsT, wi*hi*3);
			memcpy(pTexData, bitsT, nbytes);
			m_pTex->unlock();
		}
		//g_mutexRZDraw.unlock();
		m_pTex->regenerateMipMapLevels();
	}

// 	virtual void draw2DImage(const video::ITexture * texture, const core::rect<f32> & destRect,
// 		const core::rect<f32> & sourceRect, const core::vector2df & rotateCenter, const f32 rotation = 0, const bool filtering = false,
// 		const core::vector2df scale = core::vector2df(1.0f, 1.0f),
// 		SColor color = SColor(255, 255, 255, 255), bool useAlphaChannelOfTexture = false) = 0;

	g_mutex.unlock();
	QRect rcDest(0,0,img.width(),img.height());
	int sw = m_pTex->getRZOptimizeSize().Width;
	int sh = m_pTex->getRZOptimizeSize().Height;
	irr::core::vector2df rCenter(rcDest.center().x(), rcDest.center().y());
	irr::f32 fSrcXScale = irr::f32(sw) / irr::f32(rcDest.width());
	irr::f32 fSrcYScale = irr::f32(sh) / irr::f32(rcDest.height());

	m_pirrVideoDriver->enableMaterial2D();
	m_pirrVideoDriver->getMaterial2D().AntiAliasing = 15;
	m_pirrVideoDriver->getMaterial2D().TextureLayer[0].TextureWrapU = irr::video::ETC_CLAMP;
	m_pirrVideoDriver->getMaterial2D().TextureLayer[0].TextureWrapV = irr::video::ETC_CLAMP;
	//m_pirrVideoDriver->draw2DImage(m_pTex,irr::core::position2d<irr::s32>(0,0));

	m_pirrVideoDriver->draw2DImage(m_pTex, 
		irr::core::rect<irr::f32>(rcDest.top(), rcDest.left(), rcDest.right()+1, rcDest.bottom()+1), 
		irr::core::rect<irr::f32>(0, 0, sw, sh), irr::core::vector2df(rCenter),0,
		true,irr::core::vector2df(fSrcXScale, fSrcYScale), irr::video::SColor(255, 255, 255, 255),true);
	m_pirrVideoDriver->enableMaterial2D(false);
	
	irrEnd();
}

PVThread::PVThread(PlayerVideo * pDlg)
	:m_pDlg(pDlg)
{
	m_bExit = false;
}

void PVThread::Start()
{
	m_bExit = false;
	start();
}

void PVThread::Exit()
{
	m_bExit = true;
	ffmpeg_Stop();
	//exit();
	exit();
	wait();
}

void PVThread::run()
{
//	while(!m_bExit)
	{
		char** argv = new char* [3];
		for (int i = 0; i < 3; i++) { argv[i] = new char[2048]; memset(argv[i], 0, 2048); }
		//char argv[3][2048];
		strcpy(argv[0], "-i");
		strcpy(argv[1], "-nodisp");
		QString s = qApp->applicationDirPath() + "/2.mp4";
		strcpy(argv[2], (const char *)s.toLocal8Bit());
		ffmpeg_play(3, argv);
		for (int i = 0; i < 3; i++)
			delete[] argv[i];
		delete[]argv;
		m_bExit = true;
//		m_pDlg->SignalRender();
//		m_pDlg->irrRender();
		QThread::usleep(10);
	}
}


bool PlayerVideo::FFMpeg_GetFrame(double dCurTimelinePos, void* lpVoid, uchar* pdst, int w, int h, QRect& rcCrop1, int nDstW, int nDstH)
{
	// wcx 0612
	//LPRZMediaData lpData = (LPRZMediaData)lpVoid;
	//Q_ASSERT(lpData);
// 	if (lpData->eMediaType != RZMEDIATYPE_Video)
// 	{
// 		return false;
// 	}
	QRect rcCrop = rcCrop1;
// 	rcCrop.setX(rcCrop1.x());
// 	rcCrop.setY (rcCrop1.y());
// 	rcCrop.setWidth = rcCrop1.width();
// 	rcCrop.setHeight = rcCrop1.height();

	if (!/*CRZGlobal::*/true)
	{
		return false;
	}

// 	w = m_nVideoWidth;
// 	h = m_nVideoHeight;
// 
// 	nDstH = m_nVideoHeight;
// 	nDstW = m_nVideoWidth;
	CRZFFMpegCapture* pff = (CRZFFMpegCapture*)(/*lpData->pdwFFMpeg*/m_pdwFFMpeg);

	double ddurvs = 0.0;
	bool bInSnapshot = false;
	bool bInRewind = false;
// 	LPRZVideoSnapshotData pvsRewind = NULL;
// 	LPRZVideoSnapshotData pvsCurrent = NULL;
	double dCaptureTime = 0.0;

	double start = 0;
	double selTime = 0;
	double dPos = ((dCurTimelinePos - start - ddurvs) * /*lpData->dPlaybackRate */1+ selTime);

	if (/*lpData->bGetFirstFrameOfVideo*/ /*&& !bReverse*/ m_bGetFirstFrameOfVideo)//修正snapshot
	{
		//lWpData->bGetFirstFrameOfVideo = FALSE;
		m_bGetFirstFrameOfVideo = false;

		if (bInSnapshot)
		{
			dPos = dCaptureTime;
		}

		bInSnapshot = false;

		int64_t npts = pff->picture->pkt_pts;
		double d = double(npts) * r2d(pff->ic->streams[pff->video_stream]->time_base);
		while (dPos - d > 0.001)//基本解决掉帧严重的问题
		{
			if (pff->nextFrame())
			{
				pff->m_nCurFrameIndex++;
				npts = pff->picture->pkt_pts;
				d = double(npts) * r2d(pff->ic->streams[pff->video_stream]->time_base);
			}
			else
			{
				break;
			}
		}
	}

	if (!bInSnapshot /*&& !bReverse*/)
	{
		int64_t npts = pff->picture->pkt_pts;

		if (npts > 0)//同步
		{
			double d = double(npts) * r2d(pff->ic->streams[pff->video_stream]->time_base);

			while (dPos - d > 0.001)//基本解决掉帧严重的问题
			{
				if (pff->nextFrame())
				{
					pff->m_nCurFrameIndex++;
					npts = pff->picture->pkt_pts;
					d = double(npts) * r2d(pff->ic->streams[pff->video_stream]->time_base);
				}
				else
				{
					break;
				}
				//QThread::usleep(1);
			}
		}
		else
		{
			int64_t nCurFrameIndex = int64_t(dPos * (pff->get_fps()) + 0.0);

			if (nCurFrameIndex < pff->get_total_frames())
			{
				int64_t nDelFrame = nCurFrameIndex - pff->m_nCurFrameIndex;
				pff->m_nCurFrameIndex = nCurFrameIndex;
				for (int64_t i(0); i < nDelFrame; i++)
				{
					pff->nextFrame();
					//QThread::usleep(1);
				}
			}
		}
	}

	// 	if (bReverse)
	// 	{
	// 		pff->seek(dPos);
	// 	}

	if (!pff->retrieveFrameEx((unsigned char**)&pdst, nDstW, nDstH))
	{
		return false;
	}

	return true;
}

bool PlayerVideo::FFMpeg_Init(void* lpVoid, int w, int h)
{
	//LPRZMediaData lpData = (LPRZMediaData)lpVoid;
	//Q_ASSERT(lpData);
	//if (lpData->pdwFFMpeg) return true;
	if(m_pdwFFMpeg) return true;
	CRZFFMpegCapture* pff = new CRZFFMpegCapture(lpVoid, m_strFileName, false, SWS_BICUBIC);	
	if (pff->m_bFailed)
	{
		SAFE_DELETE(pff);
		return false;
	}
	m_nVideoWidth = pff->m_nSrcVideoWidth;
	m_nVideoHeight = pff->m_nSrcVideoHeight;
	QProgressBar * p = findChild<QProgressBar*>("Progress");
	p->setRange(0, pff->get_duration_sec() * 1000);
	double dES = 0;

	//	dES = lpData->GetEditStartTime();
	double dPos = 0/*GetCaptureFrameVideoSelTimeFromEditTimeByVoidPtr(lpData, dES)*/;
	pff->m_nCurFrameIndex = dPos * pff->get_fps();
	pff->seek(dPos);

	// = (long long)pff;
	m_pdwFFMpeg = qint64(pff);
	//lpData->pdwFFMpeg = long long(pff);

	return true;
}