#pragma once

#include <QtWidgets>
#include <QMainWindow>

#include "VimbaCPP/Include/VimbaCPP.h"
//#include <QMainWindow>
using namespace AVT;

class Camera : public QWidget
{
	Q_OBJECT
public:
	Camera(QWidget* parent = nullptr);
	~Camera() {};
	VmbAPI::VimbaSystem& sys;
	void setCamLayout();
	QPushButton* startCaptureButton;
	QComboBox* listCameraBox;
	QLabel* plotArea;

	QRect screenGeometry;
	void getScreenGeometry();
};

