#include "ViewerWidget.h"
#include <QTimer>
#include "VmbImageTransformHelper.hpp"

using AVT::VmbAPI::Frame;
using AVT::VmbAPI::FramePtr;

ViewerWidget::ViewerWidget(QWidget* parent, Qt::WindowFlags flag,
    QString sID, 
    bool bAutoAdjustPacketSize, CameraPtr pCam)
    : QMainWindow(parent, flag)
    //, m_DockController(NULL)
    //, m_DockInformation(NULL)
    //, m_Controller(NULL)
    //, m_ScreenViewer(NULL)
    //, m_InformationWindow(NULL)
    //, m_bHasJustStarted(false)
    //, m_bIsFirstStart(true)
    //, m_bIsCameraRunning(false)
    //, m_bIsCamOpen(false)
    //, m_bIsRedHighlighted(false)
    //, m_bIsViewerWindowClosing(false)
    //, m_bIsDisplayEveryFrame(false)
    //, m_ImageOptionDialog(NULL)
    //, m_saveFileDialog(NULL)
    //, m_getDirDialog(NULL)
    //, m_bIsTriggeredByMultiSaveBtn(false)
    //, m_nNumberOfFramesToSave(0)
    //, m_FrameBufferCount(BUFFER_COUNT)
    , m_pCam(pCam)
{
    VmbError_t errorType;
    QTime openTimer;
    openTimer.start();

    errorType = m_pCam->Open(VmbAccessModeFull);
    m_sAccessMode = tr("(FULL ACCESS)"); //it is always full access since we do not allow other connection type
    bAutoAdjustPacketSize = false;
    m_OpenError = errorType;
    
    if (VmbErrorSuccess != errorType)
    {
        openTimer.elapsed();
        return;
    }

    m_sCameraID = sID;
    if (!m_sAccessMode.isEmpty())
    {
        sID.append(" ");
        sID.append(m_sAccessMode);
    }
    this->setWindowTitle(sID);
    /* add Viewer Widget to ViewerWindow*/
    m_pScene = QSharedPointer<QGraphicsScene>(new QGraphicsScene());
    m_PixmapItem = new QGraphicsPixmapItem();
    m_ScreenViewer = new Viewer(centralWidget());
    m_ScreenViewer->setScene(m_pScene.data());
    m_pScene->addItem(m_PixmapItem);
    this->setCentralWidget(m_ScreenViewer);
    
    m_ScreenViewer->show();
    m_bIsCamOpen = true;

}

ViewerWidget::~ViewerWidget()
{

}

QString ViewerWidget::getCameraID () const
{
    return m_sCameraID;
}

bool ViewerWidget::getCamOpenStatus() const
{
    return m_bIsCamOpen;
}

CameraPtr ViewerWidget::getCameraPtr()
{
    return m_pCam;
}

bool ViewerWidget::isControlledCamera(const CameraPtr& cam) const
{
    return SP_ACCESS(cam) == SP_ACCESS(m_pCam);
}


bool  ViewerWidget::getAdjustPacketSizeMessage(QString& sMessage)
{
    //if (m_Controller->isGigE())
    //{
    //    if (VmbErrorSuccess == m_Controller->getTreeStatus())
    //    {
    //        sMessage = "Packet Size Adjusted:\t";
    //    }
    //    else
    //    {
    //        sMessage = "Failed To Adjust Packet Size!";
    //        sMessage.append(" Reason: " + Helper::mapReturnCodeToString(m_Controller->getTreeStatus()));
    //    }

    //    return true;
    //}

    return false;
}

VmbError_t ViewerWidget::getOpenError() const
{
    return m_OpenError;
}