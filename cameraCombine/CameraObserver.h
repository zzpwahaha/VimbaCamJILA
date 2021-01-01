
/*Description: A notification whenever device list has been changed*/
           

#ifndef CAMERAOBSERVER_H
#define CAMERAOBSERVER_H

#include <QObject>
#include <QTimer>
#include <VimbaCPP/Include/VimbaSystem.h>
#include <VimbaCPP/Include/ICameraListObserver.h>


class CameraObserver : public QObject, public AVT::VmbAPI::ICameraListObserver
{
    Q_OBJECT

    public:

        CameraObserver(void);
       ~CameraObserver(void);

        virtual void CameraListChanged( AVT::VmbAPI::CameraPtr pCam, AVT::VmbAPI::UpdateTriggerType reason );            

    protected:
    private:


    signals:
        void updateDeviceList ( void );



};

typedef AVT::VmbAPI::shared_ptr<CameraObserver> QtCameraObserverPtr;
#endif
