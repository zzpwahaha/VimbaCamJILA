#include "camera.h"

#include <string>
#include <QtWidgets>

#include "VimbaCPP/Include/VimbaCPP.h"
//#include <QMainWindow>
using namespace AVT;

Camera::Camera(QWidget* parent) : QWidget(parent), sys(VmbAPI::VimbaSystem::GetInstance())
{
	setWindowTitle("MOT camera test");
	setCamLayout();

	//VmbAPI::VimbaSystem& sys = VmbAPI::VimbaSystem::GetInstance();
	VmbErrorType err = sys.Startup();
	VmbAPI::CameraPtrVector cameras;
	
	std::string strError;
	
	if (VmbErrorSuccess == err)
	{
		err = sys.GetCameras(cameras);
	}
}

void Camera::getScreenGeometry()
{
	QList <QScreen*> screens = QGuiApplication::screens();
	screenGeometry = screens[0]->geometry();
}

void Camera::setCamLayout()
{
	getScreenGeometry();
	QGridLayout* layout = new QGridLayout(this);
	/*setGeometry(screenGeometry);*/
	setGeometry(200,300,600,400);

	//startCapture = new QPushButton;
	//startCapture->setText("start capture");
	//startCapture->setParent(this);
	startCaptureButton = new QPushButton("start capture", this);
	//startCaptureButton->setGeometry(size().width()*0+50, size().height()-50,100,30);
	
	layout->addWidget(startCaptureButton,1,1);

	listCameraBox = new QComboBox(this);
	layout->addWidget(listCameraBox, 1, 0);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 0);
	

	plotArea = new QLabel(this);
	layout->addWidget(plotArea, 0, 0, 1, 2);
	//layout->set
}