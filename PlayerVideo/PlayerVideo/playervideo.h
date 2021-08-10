#ifndef PLAYERVIDEO_H
#define PLAYERVIDEO_H

#include <QtWidgets/QMainWindow>
#include "ui_playervideo.h"
#include <QThread>
#include <Windows.h>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QTimer>
#include "RZFFMpegCapture.h"
#define SAFE_DELETE(x) if(x) delete x; x = NULL;
#define SAFE_DELETE_TYPE(type,x) type t = (type)x; if(t) delete t; x = NULL;

#include "irrlicht.h"
#ifndef WIN64
	#pragma comment(lib,"Irrlicht.lib")
#else
	#pragma comment(lib,"RZEngine.lib")
#endif

class PVThread;
class PlayerVideo : public QWidget/*QMainWindow*/
{
	Q_OBJECT

public:
	PlayerVideo(QWidget *parent = 0);
	~PlayerVideo();

	void InitUi();

	void Stop();

	void irrOpenEngine(WId hWnd);
	void irrCloseEngine();

	void irrBegin();
	void irrEnd();
	void irrRender();

	bool FFMpeg_GetFrame(double dCurTimelinePos, void* lpVoid, uchar* pdst, int w, int h, QRect& rcCrop1, int nDstW, int nDstH);
	bool FFMpeg_Init(void* lpVoid, int w, int h);
	virtual void paintEvent(QPaintEvent *);
	//void showEvent(QShowEvent *event);
	bool event(QEvent *event);

signals:
	void SignalRender();
public slots:
	void OnTimer();
	void OnRender() { irrRender(); }
	void OnOpenFile();
	virtual void keyPressEvent(QKeyEvent *);

private:
	irr::IrrlichtDevice*          m_pirrDevice;
	irr::video::IVideoDriver*     m_pirrVideoDriver;
	irr::scene::ISceneManager*    m_pirrScenceManager;
	irr::scene::ICameraSceneNode* m_pirrCamera;
	irr::video::ITexture*         m_pTex;
	PVThread *m_pThread;
	QTimer m_timer;
	QLabel *m_pPlayWnd;
	int m_nMoveX;
	int m_nMoveY;
	double m_dCurPos;
	qint64 m_pdwFFMpeg;
	int m_nVideoWidth;
	int m_nVideoHeight;
	bool m_bGetFirstFrameOfVideo;
	QString m_strFileName;
	Ui::PlayerVideoClass ui;
};

class PVThread:public QThread
{
	Q_OBJECT
public:
	PVThread(PlayerVideo * pDlg);

	void Start();
	void Exit();
	virtual void run();
private:
	PlayerVideo *m_pDlg;
	bool m_bExit;
};
#endif // PLAYERVIDEO_H
