#include "playervideo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
	QApplication a(argc, argv);
	PlayerVideo w;
	w.show();
	
	a.exec();
	CoUninitialize();
	return 0;
}
