#include "stdafx.h"
#include <IChimeraSystem.h>
#include <cameraMainWindow.h>

IChimeraSystem::IChimeraSystem (cameraMainWindow* parent_in)
{
	parentWin = parent_in;
	connect (this, &IChimeraSystem::error, parentWin, &cameraMainWindow::reportErr);
	connect (this, &IChimeraSystem::warning, parentWin, &cameraMainWindow::reportErr);
	connect (this, &IChimeraSystem::notification, parentWin, &cameraMainWindow::reportStatus);
}

