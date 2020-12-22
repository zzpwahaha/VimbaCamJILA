#include "AsynchronousGrab.h"

#include <QApplication>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	AsynchronousGrab w;
	w.show();
	return app.exec();
}
