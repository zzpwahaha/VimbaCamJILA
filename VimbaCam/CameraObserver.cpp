
//Description: A notification whenever device list has been changed


#include "CameraObserver.h"

CameraObserver::CameraObserver ( void )
{ 

}

CameraObserver::~CameraObserver ( void )
{

}

void CameraObserver::CameraListChanged ( AVT::VmbAPI::CameraPtr pCam, AVT::VmbAPI::UpdateTriggerType reason )
{
    emit updateDeviceList ( );
}