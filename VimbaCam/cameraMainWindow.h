#pragma once

#include <QtWidgets>
#include <QMutex>
#include <QMutexLocker>
#include <QVector>
#include "UI/LoggerWindow.h"
#include "UI/CameraTreeWindow.h"
#include "ViewerWidget.h"
#include "VimbaCPP/Include/VimbaSystem.h"


using namespace AVT::VmbAPI;
using AVT::VmbAPI::CameraPtrVector;


class CameraInfo;
typedef QVector<CameraInfo>     CameraInfoVector;

enum ViewerGridGeometry
{
    ncols = 2,
    nrows = 1,
    total = nrows * ncols
};

class Selector
{
private:
    bool bActive;
    int activeIndex;
    int validRange;
public:
    Selector()                  { bActive = false; activeIndex = -1; validRange = -1; }
    Selector(bool bActive, int activeIndex, int validRange)
        : bActive(bActive)
        , activeIndex(activeIndex)
        , validRange(validRange) {};
    bool isActive()             { return bActive; }
    bool isValid() { return ((activeIndex < validRange) && (activeIndex >= 0)); }
    int getActiveIndex()        { return activeIndex; }
    void setActiveIndex(int a)  { activeIndex = a; }
    void Activate()             { bActive = true; }
    void deActivate()           { bActive = false; }
};


class cameraMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	cameraMainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
	~cameraMainWindow();

private:
	QMutex                          m_Lock;
	LoggerWindow*                   m_Logger;
	CameraTreeWindow*				m_CameraTree;
    QVector <ViewerWidget*>         m_Viewer;
    CameraInfoVector                m_CameraInfos;
    VimbaSystem&                    m_VimbaSystem;
    QVector<QTreeWidgetItem*>       m_GigE;
    QVector<QTreeWidgetItem*>       m_1394;
    QVector<QTreeWidgetItem*>       m_USB;
    QVector<QTreeWidgetItem*>       m_CL;
    QVector<QTreeWidgetItem*>       m_CSI2;

    
    /*QString                         m_sOpenAccessType;*/
    QString                         m_sAPIVersion;
    QString                         m_SelectedCamera;
    bool                            m_bIsCurrentModelChecked;
    bool                            m_bIsInitialized;
    /*set m_bIsAutoAdjustPacketSize to false, not use it */
    bool                            m_bIsAutoAdjustPacketSize;

    /*  did not use right mouse clicked function*/
    //QMenu                           rightMouseMenu;
    //QString                         m_sOpenAccessType;
    //QList <QAction*>                m_RightMouseAction;
    //bool                            m_bIsRightMouseClicked;
    //bool                            m_bIsOpenByRightMouseClick;
    


    /*layout initialization*/
    QAction*                        m_aMenuCameras;
    QAction*                        m_aMenuLog;
    QAction*                        m_aRefreshCamList;
    QAction*                        m_aMenuStart;
    QDialog*                        m_dCamList;
    QDialog*                        m_dLog;
    QVector<QWidget*>               m_ViewerGridPlaceHolder;
    QGridLayout*                    m_ViewerGridLayout;
    Selector*                       m_activeViewerGrid; // control of active viewer widget
    void m_createMenu();

    void searchCameras          (const CameraPtrVector& Cameras);
    void openViewer             (CameraInfo& info);
    void closeViewer            (CameraPtr cam, int closefromDisconnect = -1);
    QString getBestAccess       (const CameraInfo& info);
//    unsigned int getAccessListPosition  (const QString& sAccessName);
    VmbErrorType getCameraDisplayName   (const CameraPtr& camera, std::string& sDisplayName);
    VmbErrorType getIPAddress           (const AVT::VmbAPI::CameraPtr& camera, QString& sIPAdress);
//
    virtual void closeEvent     (QCloseEvent* event);
    virtual void showEvent      (QShowEvent* event);
//
private slots:
    
//    /* when you use this std convention, you don't need any "connect..." */
    void on_ActionDiscover_triggered    (void);
//    void on_ActionClear_triggered       (void);
//    void on_ActionOpenByID_triggered    (void);
//    void on_ActionForceIP_triggered     (void);
//    void on_ActionCommand_triggered     (void);
//    void on_ActionStartOptions_triggered(void);
//
//    /* Custom */
    void onInitializeVimba();
    void onCameraClicked        (const QString& sModel, const bool& bIsChecked);
//    void onRightMouseClicked    (const bool& bIsClicked);
    void onCloseFromViewer      (CameraPtr cam);
    void onUpdateDeviceList     (void);
//    void about                  (void);
//    void rightMouseOpenCamera   (bool bOpenAccesState);
    

};



class CameraInfo
{
private:
    CameraPtr       m_Cam;
    QString         m_DisplayName;
    bool            m_IsOpen;
    QStringList     m_PermittedAccess;
    QStringList     m_PermittedAccessState;
public:
    CameraInfo()
        : m_IsOpen(false)
    {}
    explicit CameraInfo(CameraPtr cam, const QString& name)
        : m_Cam(cam)
        , m_DisplayName(name)
        , m_IsOpen(false)
    {}
    const CameraPtr& Cam()            const { return m_Cam; }
    CameraPtr& Cam()                        { return m_Cam; }
    const QString& DisplayName()      const { return m_DisplayName; }
    bool  IsOpen()                    const { return m_IsOpen; }
    void  SetOpen(bool s)                   { m_IsOpen = s; }
    const QStringList& PermittedAccess()   const             { return m_PermittedAccess; }
    void               PermittedAccess(const QStringList& l) { m_PermittedAccess = l; }
    const QStringList& PermittedAccessState()   const        { return m_PermittedAccessState; }
    void               PermittedAccessState(const QStringList& l) { m_PermittedAccessState = l; }
    void SetPermittedAccessState(unsigned int pos, QString status)
    {
        m_PermittedAccessState[pos] = status;
    }
    void ResetPermittedAccessState()
    {
        for (int pos = 0; pos < m_PermittedAccessState.size(); ++pos)
        {
            m_PermittedAccessState[pos] = "false";
        }
    }


    bool operator==(const CameraPtr& other)   const { return SP_ACCESS(other) == SP_ACCESS(m_Cam); }
    bool operator<(const CameraPtr& other)    const { return SP_ACCESS(other) < SP_ACCESS(m_Cam); }
    
    VmbInterfaceType InterfaceType() const
    {
        VmbInterfaceType interfaceType;
        VmbErrorType result = SP_ACCESS(m_Cam)->GetInterfaceType(interfaceType);
        if (result != VmbErrorSuccess)
        {
            throw std::runtime_error("could not read interface type from camera");
        }
        return interfaceType;
    }
    QString InterfaceTypeString() const
    {
        VmbInterfaceType interfaceType;
        VmbErrorType result = SP_ACCESS(m_Cam)->GetInterfaceType(interfaceType);
        if (result != VmbErrorSuccess)
        {
            throw std::runtime_error("could not read interface type from camera");
        }
        switch (interfaceType)
        {
        case VmbInterfaceEthernet:
            return "GigE";
        case VmbInterfaceFirewire:
            return "1394";
        case VmbInterfaceUsb:
            return "USB";
        case VmbInterfaceCL:
            return "CL";
        case VmbInterfaceCSI2:
            return "CSI2";
        }
    }
};

class FindByName
{
    const QString m_Name;
public:
    FindByName(const QString& s)
        : m_Name(s)
    {}
    bool operator()(const CameraInfo& i) const
    {
        return i.DisplayName() == m_Name;
    }
};

class CameraPtrCompare
{
    const CameraPtr m_Camera;
public:
    CameraPtrCompare(const CameraPtr& cam)
        : m_Camera(cam)
    {}
    bool operator() (const CameraPtr& other) const
    {
        return m_Camera.get() == other.get();
    }
};