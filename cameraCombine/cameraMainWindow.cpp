#include "cameraMainWindow.h"

#include <QtWidgets>
#include <QRegExp>
#include <algorithm>


#include "CameraObserver.h"


cameraMainWindow::cameraMainWindow(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_bIsCurrentModelChecked(false)
    , m_Logger(nullptr)
    , m_VimbaSystem(AVT::VmbAPI::VimbaSystem::GetInstance())
    , m_bIsInitialized(false)
    , m_Lock(QMutex::Recursive)
    , m_bIsAutoAdjustPacketSize(false)
{
	qRegisterMetaType<CameraPtr>("CameraPtr");
	setGeometry(0, 0, qApp->screens()[0]->geometry().width(), 
		qApp->screens()[0]->geometry().height()-100);

	/* use setting position and geometry from the last session*/
	/*QSettings settings("Allied Vision Technologies", "Vimba Viewer");
	this->restoreGeometry(settings.value("geometrymainwindow").toByteArray());
	this->restoreState(settings.value("state").toByteArray(), 0);*/


    m_CameraTree = new CameraTreeWindow(this);
    m_Logger = new LoggerWindow(this);

    /*create menu*/
	m_createMenu();
    connect(m_CameraTree, SIGNAL(cameraClicked(const QString&, const bool&)), this, SLOT(onCameraClicked(const QString&, const bool&)));
    /*Initializing Vimba is set in the showEvent, which overide the virtual function in QMainWindow*/
    //onInitializeVimba();

    /*initialize selector for active view widget*/
    m_activeViewerGrid = new Selector(false, -1, ViewerGridGeometry::total);

    /*initialize layout*/
    QWidget* window = new QWidget(this);
    m_ViewerGridLayout = new QGridLayout();
    for (size_t indx = 0; indx < ViewerGridGeometry::total; indx++)
    {
        m_ViewerGridPlaceHolder.append(new QWidget);
        m_ViewerGridPlaceHolder.at(indx)->setStyleSheet("border: 1px solid red");
        QPushButton* connectCamB = new QPushButton("&Connect To", m_ViewerGridPlaceHolder.at(indx));
        connectCamB->setStyleSheet("font-size: 16px; background-color: rgb(51, 255, 153);");
        connectCamB->setMinimumWidth(200);
            
        QVBoxLayout* layoutPH = new QVBoxLayout(m_ViewerGridPlaceHolder.at(indx));
        layoutPH->addWidget(connectCamB);
        layoutPH->setAlignment(Qt::AlignHCenter);
        
        m_ViewerGridLayout->addWidget(m_ViewerGridPlaceHolder.at(indx),
            indx / ViewerGridGeometry::ncols, indx % ViewerGridGeometry::ncols);
            
        connect(connectCamB, &QPushButton::clicked, this, [&,indx]() {
            m_dCamList->setWindowTitle("Camera List : " +
                QString::fromStdString(std::to_string(indx)));
            m_activeViewerGrid->setActiveIndex(indx);
            m_activeViewerGrid->Activate();
            onUpdateDeviceList();//force to update camlist
            m_dCamList->show(); 
            });
    }
    window->setLayout(m_ViewerGridLayout);
    this->setCentralWidget(window);
    
    connect(m_dCamList, &QDialog::finished, this, [&]() {m_activeViewerGrid->deActivate(); });


    //m_ViewerGridLayout->setRowMinimumHeight(0,750);
    //m_ViewerGridLayout->setAlignment(Qt::AlignCenter);
    //m_ViewerGridLayout
}

cameraMainWindow::~cameraMainWindow()
{
    m_VimbaSystem.Shutdown();
}

void cameraMainWindow::m_createMenu()
{
	QMenu* camM = menuBar()->addMenu("&Camera");
		
    /*logger dialog*/
    m_aMenuLog = camM->addAction("&Log");
    m_dLog = new QDialog(this);
    m_dLog->setWindowTitle("Log");
    m_dLog->setMinimumWidth(750);
    QVBoxLayout* loggerlayout = new QVBoxLayout(m_dLog);
    //loggerlayout->addWidget(new QLabel("Log"));
    loggerlayout->addWidget(m_Logger);
    connect(m_aMenuLog, &QAction::triggered, this, [&]() {m_dLog->show(); });

    /*cameralist dialog*/
    m_aMenuCameras = camM->addAction("&Camera List");
    m_dCamList = new QDialog(this);
    m_dCamList->setWindowTitle("Camera List");
    m_dCamList->setMinimumWidth(750);
    QVBoxLayout* camlistlayout = new QVBoxLayout(m_dCamList);
    //camlistlayout->addWidget(new QLabel("Camera list"));
    camlistlayout->addWidget(m_CameraTree);
    connect(m_aMenuCameras, &QAction::triggered, this, [&]() {
        m_dCamList->setWindowTitle("Camera List");
        onUpdateDeviceList();//force update camlist
        m_dCamList->exec(); });
	
    /*refresh camera list*/
    m_aRefreshCamList = camM->addAction("&Refresh");
    connect(m_aRefreshCamList, &QAction::triggered, this, &cameraMainWindow::on_ActionDiscover_triggered);
    
    
    
    /*camera start button*/
    //m_aMenuStart = camM->addAction("Start acquisition");
    //m_aMenuStart->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    
    /*connect(m_aMenuStart, &QAction::triggered, this,
		&AsynchronousGrab::OnBnClickedButtonStartstop);*/
}


struct FindWindowByCameraPtr
{
private:
    CameraPtr m_Cam;
public:
    FindWindowByCameraPtr(CameraPtr cam)
        : m_Cam(cam)
    {}
    bool operator () (ViewerWidget* v) const
    {
        return v->isControlledCamera(m_Cam);
    }
};

void cameraMainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!m_bIsInitialized)
    {
        QTreeWidgetItem* itemRoot = new QTreeWidgetItem(m_CameraTree);
        m_CameraTree->setText(itemRoot, "Please wait...");
        m_CameraTree->setEnabled(false);

        QTimer::singleShot(50, this, SLOT(onInitializeVimba()));        /* This slot will be called AFTER the main window is shown. This is needed, since the enumeration of CL cameras might take a while and it should not prevent the main window from being initially drawn */
        
    }
}

/* this will be called on destruction of main window in case at least one viewer window is open */
void cameraMainWindow::closeEvent(QCloseEvent* event)
{
    QMutexLocker local_lock(&m_Lock);
    if (!m_Viewer.empty())
    {
        QVector<ViewerWidget*>::iterator it = m_Viewer.begin();
        while (it != m_Viewer.end())
        {
            delete* it;
            *it = NULL;
            ++it;
        }
        m_Viewer.clear();
    }
}


VmbErrorType cameraMainWindow::getCameraDisplayName(const CameraPtr& camera, std::string& sDisplayName)
{
    VmbErrorType  error;
    std::string sID;
    std::string sSN;

    error = camera->GetModel(sDisplayName);
    if (VmbErrorSuccess != error)
    {
        m_Logger->logging(tr("GetModel Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
        return error;
    }
    else
    {
        error = camera->GetSerialNumber(sSN);
        if (VmbErrorSuccess != error)
        {
            m_Logger->logging(tr("GetSerialNumber Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
            return error;
        }
        else
        {
            error = camera->GetID(sID);
            if (VmbErrorSuccess != error)
            {
                m_Logger->logging(tr("GetID Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
                return error;
            }
            else
            {
                std::string sDisplayNameEnding;

                sDisplayNameEnding.append("-");
                sDisplayNameEnding.append(sSN);

                if (0 != sSN.compare(sID))
                {
                    sDisplayNameEnding.append("(");
                    sDisplayNameEnding.append(sID);
                    sDisplayNameEnding.append(")");
                }

                std::string sLegacyDisplayName = sDisplayName + sDisplayNameEnding;

                VmbInterfaceType cameraIFType;
                error = camera->GetInterfaceType(cameraIFType);

                if (VmbErrorSuccess != error)
                {
                    m_Logger->logging(tr("GetInterfaceType Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
                    return error;
                }

                // camera interface type is GigE. update display name with IP address 
                if (VmbInterfaceEthernet == cameraIFType)
                {
                    // lookup the IP address of the camera         
                    QString sIPAddress;
                    error = getIPAddress(camera, sIPAddress);

                    // replace the model ID with the IP address
                    if (VmbErrorSuccess == error && !sIPAddress.isEmpty())
                    {
                        QString sTempDisplayName = QString::fromStdString(sDisplayName);
                        QRegExp regExp("\\(([^)]+)\\)");

                        // to account for cameras without model name in parenthesis
                        if (-1 == regExp.indexIn(sTempDisplayName))
                        {
                            sDisplayName.append(sIPAddress.toStdString());
                        }
                        else
                        {
                            sTempDisplayName.replace(regExp, sIPAddress);
                            sDisplayName = sTempDisplayName.toStdString();
                        }

                        sDisplayName.append(sDisplayNameEnding);
                    }
                }
                // other camera interface types use legacy naming convention
                else
                {
                    sDisplayName = sLegacyDisplayName;
                }
            }
        }
    }
    return error;
}

VmbErrorType cameraMainWindow::getIPAddress(const AVT::VmbAPI::CameraPtr& camera, QString& sIPAdress)
{
    VmbErrorType error;
    std::string sCameraID, sInterfaceID, sDeviceID;
    AVT::VmbAPI::InterfacePtr   pInterface;
    AVT::VmbAPI::FeaturePtr     pSelectorFeature, pSelectedDeviceID, pSelectedIPAddress;
    VmbInt64_t                  nMinRange, nMaxRange, nIP;
    VmbInt32_t                  nIP_32;

    // get the camera ID
    error = camera->GetID(sCameraID);
    if (VmbErrorSuccess == error)
    {
        // get the interface ID
        error = camera->GetInterfaceID(sInterfaceID);
        if (VmbErrorSuccess == error)
        {
            // get a pointer to the interface
            error = m_VimbaSystem.GetInterfaceByID(sInterfaceID.c_str(), pInterface);
            if (VmbErrorSuccess == error)
            {
                // open the interface 
                error = pInterface->Open();
                if (VmbErrorSuccess == error)
                {
                    // get the device selector
                    error = pInterface->GetFeatureByName("DeviceSelector", pSelectorFeature);
                    if (VmbErrorSuccess == error)
                    {
                        // get the range of the available devices 
                        error = pSelectorFeature->GetRange(nMinRange, nMaxRange);

                        // check for negative range in case requested feature contains no items
                        if (VmbErrorSuccess == error && nMaxRange >= 0)
                        {
                            // get the device ID pointer
                            error = pInterface->GetFeatureByName("DeviceID", pSelectedDeviceID);
                            if (VmbErrorSuccess == error)
                            {
                                // get IP addresses of all cameras connected to interface
                                error = pInterface->GetFeatureByName("GevDeviceIPAddress", pSelectedIPAddress);
                                if (VmbErrorSuccess == error)
                                {
                                    // find the IP address of the desired camera 
                                    for (VmbInt64_t intNo = nMinRange; intNo <= nMaxRange; ++intNo)
                                    {
                                        error = pSelectorFeature->SetValue(intNo);
                                        if (VmbErrorSuccess == error)
                                        {
                                            error = pSelectedDeviceID->GetValue(sDeviceID);
                                            if (VmbErrorSuccess == error)
                                            {
                                                if (0 == sDeviceID.compare(sCameraID))
                                                {
                                                    error = pSelectedIPAddress->GetValue(nIP);
                                                    if (VmbErrorSuccess == error)
                                                    {
                                                        nIP_32 = static_cast<VmbInt32_t>(nIP);

                                                        // format IP address string
                                                        sIPAdress = QString("(%1)").arg(Helper::IPv4ToString(qFromBigEndian(nIP_32), true));

                                                        // close the interface
                                                        error = pInterface->Close();
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            sIPAdress.clear();
                            error = VmbErrorNotFound;
                        }

                    }
                }
            }
        }
    }

    return error;
}

void cameraMainWindow::searchCameras(const CameraPtrVector& Cameras)
{
    /* If update device list callback triggered: Recopy before clearing. If the viewer is already open, use the access list from CopyMap,
       this will avoid the change of menu access entry  info.
       Access information the viewer got when camera is opened will be kept.*/
    
    m_CameraTree->clear();

    QTreeWidgetItem* itemGigERoot = nullptr;
    QTreeWidgetItem* item1394Root = nullptr;
    QTreeWidgetItem* itemUSBRoot = nullptr;
    QTreeWidgetItem* itemCLRoot = nullptr;
    QTreeWidgetItem* itemCSI2Root = nullptr;

    m_GigE.clear();
    m_1394.clear();
    m_USB.clear();
    m_CL.clear();
    m_CSI2.clear();

    QMap <QString, QString>     ifTypeMap;
    InterfacePtrVector          ifPtrVec;
    std::string                 sInterfaceID;
    VmbErrorType                error;

    /* list all interfaces found by VmbAPI */
    error = m_VimbaSystem.GetInterfaces(ifPtrVec);

    if (VmbErrorSuccess != error)
    {
        m_Logger->logging(tr("GetInterfaces Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
        return;
    }

    /* check type of Interfaces */
    for (unsigned int i = 0; i < ifPtrVec.size(); i++)
    {
        error = ifPtrVec.at(i)->GetID(sInterfaceID);
        if (VmbErrorSuccess != error)
        {
            m_Logger->logging("GetID <Interface " + QString::number(i) + " Failed, Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
            continue;
        }

        VmbInterfaceType    ifType = VmbInterfaceUnknown;
        VmbErrorType        errorGetType = ifPtrVec.at(i)->GetType(ifType);
        if (VmbErrorSuccess != errorGetType)
        {
            m_Logger->logging("GetType <Interface " + QString::number(i) + " Failed, Error: " + QString::number(errorGetType) + " " + Helper::mapReturnCodeToString(errorGetType), VimbaViewerLogCategory_ERROR);
            continue;
        }

        switch (ifType)
        {
        case VmbInterfaceEthernet:
            ifTypeMap[QString::fromStdString(sInterfaceID)] = "GigE";
            if (NULL == itemGigERoot)
            {
                itemGigERoot = new QTreeWidgetItem(m_CameraTree);
                m_CameraTree->setText(itemGigERoot, "GigE");
                //m_MainWindow.ActionStartOptions->setEnabled(true);
                //will be setStartEnabled in viewer window
            }
            break;

        case VmbInterfaceFirewire:
            ifTypeMap[QString::fromStdString(sInterfaceID)] = "1394";
            if (NULL == item1394Root)
            {
                item1394Root = new QTreeWidgetItem(m_CameraTree);
                m_CameraTree->setText(item1394Root, "1394");
            }
            break;

        case VmbInterfaceUsb:
            ifTypeMap[QString::fromStdString(sInterfaceID)] = "USB";
            if (NULL == itemUSBRoot)
            {
                itemUSBRoot = new QTreeWidgetItem(m_CameraTree);
                m_CameraTree->setText(itemUSBRoot, "USB");
            }
            break;

        case VmbInterfaceCL:
            ifTypeMap[QString::fromStdString(sInterfaceID)] = "CL";
            if (NULL == itemCLRoot)
            {
                itemCLRoot = new QTreeWidgetItem(m_CameraTree);
                m_CameraTree->setText(itemCLRoot, "CL");
            }
            break;
        case VmbInterfaceCSI2:
            ifTypeMap[QString::fromStdString(sInterfaceID)] = "CSI-2";
            if (NULL == itemCSI2Root)
            {
                itemCSI2Root = new QTreeWidgetItem(m_CameraTree);
                m_CameraTree->setText(itemCSI2Root, "CSI-2");
            }
            break;
        default: break;
        }
    } /* for */

    /* sort the cameras */

    if (!Cameras.empty())
        m_CameraTree->setCursor(Qt::PointingHandCursor);
    else
        m_CameraTree->setCursor(Qt::ArrowCursor);

    for (unsigned int i = 0; i < Cameras.size(); i++)
    {
        CameraInfoVector::iterator currentCamera = std::find(m_CameraInfos.begin(), m_CameraInfos.end(), Cameras[i]);
        if (currentCamera == m_CameraInfos.end())
        {
            std::string displayName;
            error = getCameraDisplayName(Cameras[i], displayName);
            if (VmbErrorSuccess != error)
            {
                m_Logger->logging("GetDisplayName error for camera " + QString::number(i) + "Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
                continue;
            }
            currentCamera = m_CameraInfos.insert(m_CameraInfos.end(), CameraInfo(Cameras[i], QString::fromStdString(displayName)));
            /*std::end point to the one following the last element, which signaling the end of the vector, i.e. the terminaotr*/
            /*if currrentCamera is not in the vector, append it to the end of the vector*/
        }



        QString     sAccess;
        QStringList sListPermittedAccess;
        QStringList sListPermittedAccessState;


        /* camera is open, so keep the information */
        if (currentCamera->IsOpen())
        {
            /* keep the last information */
            sListPermittedAccess = currentCamera->PermittedAccess();
            sListPermittedAccessState = currentCamera->PermittedAccessState();

            if (sListPermittedAccess.contains(tr("Open FULL ACCESS")) && sListPermittedAccess.contains(tr("Open READ ONLY")))
            {
                sAccess = "FULL ACCESS";
            }
            else if (sListPermittedAccess.contains(tr("Open READ ONLY")))
            {
                sAccess = "READ ONLY";
            }
        }
        else /* camera is not open, so get the current supported access */
        {
            /* check access */
            VmbAccessModeType accessType = VmbAccessModeFull;
            if (VmbErrorSuccess == currentCamera->Cam()->GetPermittedAccess(accessType))
            {
                if (accessType & VmbAccessModeFull)
                {
                    sAccess = "FULL ACCESS";
                    switch (currentCamera->InterfaceType())
                    {
                    case VmbInterfaceEthernet:
                    {
                        sListPermittedAccess << tr("Open FULL ACCESS") << tr("Open READ ONLY") << tr("Open CONFIG");
                        sListPermittedAccessState << "false" << "false" << "false";
                        break;
                    }
                    case VmbInterfaceFirewire:
                    {
                        sListPermittedAccess << tr("Open FULL ACCESS") << tr("Open READ ONLY");
                        sListPermittedAccessState << "false" << "false";
                        break;
                    }
                    case VmbInterfaceUsb:
                    {
                        sListPermittedAccess << tr("Open FULL ACCESS") << tr("Open READ ONLY");
                        sListPermittedAccessState << "false" << "false";
                        break;
                    }
                    case VmbInterfaceCL:
                    case VmbInterfaceCSI2:
                    {
                        sListPermittedAccess << tr("Open FULL ACCESS");
                        sListPermittedAccessState << "false";
                        break;
                    }
                    }
                }
                else if (accessType & VmbAccessModeRead)
                {
                    sAccess = "READ ONLY";
                    switch (currentCamera->InterfaceType())
                    {
                    case VmbInterfaceEthernet:
                    {
                        //sListPermittedAccess      << "Open READ ONLY" << "Open CONFIG";
                        sListPermittedAccess << tr("Open READ ONLY");
                        sListPermittedAccessState << "false";//<< "false";
                        break;
                    }
                    case VmbInterfaceFirewire:
                    {
                        sListPermittedAccess << tr("Open READ ONLY");
                        sListPermittedAccessState << "false";
                        break;
                    }
                    case VmbInterfaceUsb:
                    {
                        sListPermittedAccess << tr("Open READ ONLY");
                        sListPermittedAccessState << "false";
                        break;
                    }
                    }
                }
                else if (accessType & VmbAccessModeConfig)
                {
                    sAccess = "DEVICE CONFIGURATION";
                    if (currentCamera->InterfaceType() == VmbInterfaceEthernet)
                    {
                        sListPermittedAccess << tr("Open CONFIG");
                        sListPermittedAccessState << "false";
                    }
                }
                else if (accessType & VmbAccessModeLite)
                {
                    sAccess = "RW WITH NO FEATURE ACCESS";
                    sListPermittedAccess << tr("Open LITE");
                    sListPermittedAccessState << "false";
                }
                else
                {
                    sAccess = "NO ACCESS";
                }
            }
            currentCamera->PermittedAccess(sListPermittedAccess);
            currentCamera->PermittedAccessState(sListPermittedAccessState);
        }

        switch (currentCamera->InterfaceType())
        {
        case VmbInterfaceEthernet:
        {
            QTreeWidgetItem* itemIF_GigE_ID = m_CameraTree->createItem(itemGigERoot, true);
            itemIF_GigE_ID->setText(0, currentCamera->DisplayName());
            //itemIF_GigE_ID->setIcon(0, QIcon(":/VimbaViewer/Images/stripes_256.png"));
            if (sAccess == "READ ONLY" || sAccess == "NO ACCESS")
            {
                itemIF_GigE_ID->setBackgroundColor(0, Qt::black);
                itemIF_GigE_ID->setForeground(0, Qt::white);
                //itemIF_GigE_ID->setIcon(0, QIcon(":/VimbaViewer/Images/lock.png"));
                if (sAccess == "NO ACCESS")
                {
                    itemIF_GigE_ID->setDisabled(true);
                }
            }
            if (currentCamera->IsOpen())
            {
                itemIF_GigE_ID->setCheckState(0, Qt::Checked);
            }

            m_GigE.append(itemIF_GigE_ID);
            break;
        }

        case VmbInterfaceFirewire:
        {
            QTreeWidgetItem* itemIF_1394_ID = m_CameraTree->createItem(item1394Root, true);
            itemIF_1394_ID->setText(0, currentCamera->DisplayName());
            //itemIF_1394_ID->setIcon(0, QIcon(":/VimbaViewer/Images/stripes_256.png"));
            if (sAccess == "READ ONLY" || sAccess == "NO ACCESS")
            {
                itemIF_1394_ID->setBackgroundColor(0, Qt::black);
                itemIF_1394_ID->setForeground(0, Qt::white);
                //itemIF_1394_ID->setIcon(0, QIcon(":/VimbaViewer/Images/lock.png"));
                if (sAccess == "NO ACCESS")
                {
                    itemIF_1394_ID->setDisabled(true);
                }
            }
            if (currentCamera->IsOpen())
            {
                itemIF_1394_ID->setCheckState(0, Qt::Checked);
            }

            m_1394.append(itemIF_1394_ID);
            break;
        }

        case VmbInterfaceUsb:
        {
            QTreeWidgetItem* itemIF_USB_ID = m_CameraTree->createItem(itemUSBRoot, true);
            itemIF_USB_ID->setText(0, currentCamera->DisplayName());
            //itemIF_USB_ID->setIcon(0, QIcon(":/VimbaViewer/Images/stripes_256.png"));
            if (sAccess == "READ ONLY" || sAccess == "NO ACCESS")
            {
                itemIF_USB_ID->setBackgroundColor(0, Qt::black);
                itemIF_USB_ID->setForeground(0, Qt::white);
                //itemIF_USB_ID->setIcon(0, QIcon(":/VimbaViewer/Images/lock.png"));
                if (sAccess == "NO ACCESS")
                {
                    itemIF_USB_ID->setDisabled(true);
                }
            }
            if (currentCamera->IsOpen())
            {
                itemIF_USB_ID->setCheckState(0, Qt::Checked);
            }

            m_USB.append(itemIF_USB_ID);
            break;
        }

        case VmbInterfaceCL:
        {
            QTreeWidgetItem* itemIF_CL_ID = m_CameraTree->createItem(itemCLRoot, true);
            itemIF_CL_ID->setText(0, currentCamera->DisplayName());
            //itemIF_CL_ID->setIcon(0, QIcon(":/VimbaViewer/Images/stripes_256.png"));
            if (!currentCamera->IsOpen())
            {
                if (0 == sAccess.compare("READ ONLY") || 0 == sAccess.compare("NO ACCESS"))
                {
                    itemIF_CL_ID->setBackgroundColor(0, Qt::black);
                    itemIF_CL_ID->setForeground(0, Qt::white);
                    //itemIF_CL_ID->setIcon(0, QIcon(":/VimbaViewer/Images/lock.png"));
                    if (0 == sAccess.compare("NO ACCESS"))
                        itemIF_CL_ID->setDisabled(true);
                }
            }

            m_CL.append(itemIF_CL_ID);
            break;
        }
        case VmbInterfaceCSI2:
        {
            QTreeWidgetItem* itemIF_CSI2_ID = m_CameraTree->createItem(itemCSI2Root, true);
            itemIF_CSI2_ID->setText(0, currentCamera->DisplayName());
            //itemIF_CSI2_ID->setIcon(0, QIcon(":/VimbaViewer/Images/stripes_256.png"));
            if (!currentCamera->IsOpen())
            {
                if (0 == sAccess.compare("READ ONLY") || 0 == sAccess.compare("NO ACCESS"))
                {
                    itemIF_CSI2_ID->setBackgroundColor(0, Qt::black);
                    itemIF_CSI2_ID->setForeground(0, Qt::white);
                    //itemIF_CSI2_ID->setIcon(0, QIcon(":/VimbaViewer/Images/lock.png"));
                    if (0 == sAccess.compare("NO ACCESS"))
                        itemIF_CSI2_ID->setDisabled(true);
                }
            }

            m_CSI2.append(itemIF_CSI2_ID);
            break;
        }
        }
    }

    m_CameraTree->expandAll();
}

void cameraMainWindow::onCloseFromViewer(CameraPtr cam)
{
    m_CameraTree->setEnabled(false);
    QMutexLocker            local_lock(&m_Lock);

    /* iterate the tree and find the position of the string, if found uncheck the checkbox */
    CameraInfoVector::const_iterator findPos = std::find(m_CameraInfos.begin(), m_CameraInfos.end(), cam);
    if (findPos == m_CameraInfos.end())
    {
        m_Logger->logging(tr("closing camera that is not in list"), VimbaViewerLogCategory_ERROR);
        return;
    }
    QTreeWidgetItemIterator it(m_CameraTree);
    while (*it)
    {
        if ((*it)->text(0) == findPos->DisplayName())
        {
            if (Qt::Checked == (*it)->checkState(false))
            {
                (*it)->setSelected(true);
                (*it)->setCheckState(0, Qt::Unchecked);
                break;
            }
        }
        ++it;
    }
    closeViewer(cam);
    m_CameraTree->setEnabled(true);
}


void CheckOpen(QVector<QTreeWidgetItem*>& items, const QString& CameraID)
{
    for (int pos = 0; pos < items.size(); pos++)
    {
        if (items[pos]->text(0) == CameraID)
        {
            items[pos]->setCheckState(0, Qt::Checked);
            return;
        }
    }
}

void cameraMainWindow::onInitializeVimba()
{
    VmbErrorType error = m_VimbaSystem.Startup();
    if (VmbErrorSuccess != error)
    {
        m_Logger->logging(("Startup failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
        return;
    }

    // get VimbaC feature to check if GigE TL is present
    AVT::VmbAPI::FeaturePtr gevFeature;
    error = m_VimbaSystem.GetFeatureByName("GeVTLIsPresent", gevFeature);
    if (VmbErrorSuccess == error)
    {
        // check if GigE TL is present
        bool gevTLisPresent = false;
        error = gevFeature->GetValue(gevTLisPresent);
        if ((VmbErrorSuccess == error) && (true == gevTLisPresent))
        {
            /*m_ForceIPDialog = new ForceIPDialog(m_VimbaSystem);
            m_ActionCommandDialog = new ActionCommandDialog(m_VimbaSystem);
            m_ActionCommandFlag = true;*/
            m_Logger->logging("GigE TL is present, can access camera with IP, but not implemented here", VimbaViewerLogCategory_INFO);
        }
    }

    /*    register the camera observer to get notification when camera list has been updated, e.g:
    *     - plug / unplug camera(s)
    *     - bus reset occurs
    */

    try
    {
        CameraPtrVector tmpCameras;
        error = m_VimbaSystem.GetCameras(tmpCameras);
        if (VmbErrorSuccess == error)
        {
            searchCameras(tmpCameras);
            QtCameraObserverPtr pDeviceObs(new CameraObserver());
            error = m_VimbaSystem.RegisterCameraListObserver(pDeviceObs);
            if (VmbErrorSuccess == error)
            {
                connect(pDeviceObs.get(), SIGNAL(updateDeviceList()), this, SLOT(onUpdateDeviceList()));
            }
            else
            {
                m_Logger->logging(tr("RegisterCameraListObserver Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
            }
        }
        else
        {
            m_Logger->logging(tr("could not get camera list, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
        }
        // m_CameraTree->expandAll();
    }
    catch (const std::exception& e)
    {
        m_Logger->logging("MainWindow <constructor> Exception: " + QString::fromStdString(e.what()), VimbaViewerLogCategory_ERROR);
    }

    m_CameraTree->setEnabled(true);
    m_bIsInitialized = true;
}


void cameraMainWindow::onUpdateDeviceList(void)
{

    CameraPtrVector     currentListedCameras;

    QMutexLocker        local_lock(&m_Lock);

    VmbErrorType        error = m_VimbaSystem.GetCameras(currentListedCameras);
    CameraInfoVector    remainingCameras;
    if (VmbErrorSuccess == error)
    {
        /* disconnect */
        /* find out what camera is not available */
        CameraPtrVector DisconnectedCameras;
        for (unsigned int i = 0; i < m_CameraInfos.size(); i++)
        {
            if (currentListedCameras.end() == std::find_if(currentListedCameras.begin(), currentListedCameras.end(), CameraPtrCompare(m_CameraInfos[i].Cam()))) /*quite clever trick -zzp*/
            {
                DisconnectedCameras.push_back(m_CameraInfos[i].Cam());
            }
            else
            {
                remainingCameras.push_back(m_CameraInfos[i]);
            }
        }
        for (size_t pos = 0; pos < DisconnectedCameras.size(); ++pos)
        {
            closeViewer(DisconnectedCameras[pos]);
        }
        m_CameraInfos = remainingCameras;
    }
    else
    {
        m_Logger->logging(tr("GetCameras Failed, Error: ") + QString::number(error) + " " + Helper::mapReturnCodeToString(error), VimbaViewerLogCategory_ERROR);
    }

    searchCameras(currentListedCameras);

    /* update tree */
    for (int i = 0; i < m_Viewer.size(); i++)
    {
        CheckOpen(m_GigE, m_Viewer[i]->getCameraID());
        CheckOpen(m_1394, m_Viewer[i]->getCameraID());
        CheckOpen(m_USB, m_Viewer[i]->getCameraID());
        CheckOpen(m_CL, m_Viewer[i]->getCameraID());
        CheckOpen(m_CSI2, m_Viewer[i]->getCameraID());
    }
}


void cameraMainWindow::onCameraClicked(const QString& sModel, const bool& bIsChecked)
{
    CameraInfo selectedCamera;
    m_CameraTree->setEnabled(false);

    QMutexLocker local_lock(&m_Lock);

    m_SelectedCamera = sModel;
    m_bIsCurrentModelChecked = bIsChecked;
    CameraInfoVector::iterator find_pos = std::find_if(m_CameraInfos.begin(), m_CameraInfos.end(), FindByName(sModel));
    if (find_pos == m_CameraInfos.end())
    {
        if (!m_CameraTree->isEnabled())
            m_CameraTree->setEnabled(true);
        m_Logger->logging("MainWindow <onCameraClicked> could not find camera: " + sModel, VimbaViewerLogCategory_ERROR);
        return;
    }
    selectedCamera = *find_pos;
    /*did not use the right mouse clicked function*/
    //if (m_bIsRightMouseClicked)
    //{
    ////    
    ////    m_bIsRightMouseClicked = false;
    ////    /* show permitted access */
    ////    if (!m_RightMouseAction.isEmpty())
    ////    {
    ////        for (int nPos = 0; nPos < m_RightMouseAction.size(); nPos++)
    ////        {
    ////            delete m_RightMouseAction.at(nPos);
    ////        }
    ////
    ////        m_RightMouseAction.clear();
    ////    }
    ////
    ////    QStringList sPermittedAccess = selectedCamera.PermittedAccess();
    ////
    ////    for (int i = 0; i < sPermittedAccess.size(); i++)
    ////    {
    ////        m_RightMouseAction.push_back(new QAction(this));
    ////        m_RightMouseAction.last()->setObjectName(sPermittedAccess.at(i));
    ////        m_RightMouseAction.last()->setCheckable(true);
    ////        m_RightMouseAction.last()->setText(sPermittedAccess.at(i));
    ////
    ////        connect(m_RightMouseAction.last(), SIGNAL(toggled(bool)), this, SLOT(rightMouseOpenCamera(bool)));
    ////        rightMouseMenu.addAction(m_RightMouseAction.last());
    ////    }
    ////
    ////    bool bIsOneOfThemChecked = false;
    ////    QStringList PermittedAccessState = selectedCamera.PermittedAccessState();
    ////    for (int i = 0; i < PermittedAccessState.size(); i++)
    ////    {
    ////        if (PermittedAccessState[i] == "true")
    ////        {
    ////            m_RightMouseAction[i]->setChecked(true);
    ////            bIsOneOfThemChecked = true;
    ////        }
    ////        else if (PermittedAccessState[i] == "false")
    ////        {
    ////            m_RightMouseAction.at(i)->setChecked(false);
    ////        }
    ////    }
    ////
    ////    /* disabling other menus entries if one of already selected */
    ////    if (bIsOneOfThemChecked)
    ////    {
    ////        for (int i = 0; i < PermittedAccessState.size(); i++)
    ////        {
    ////            if (PermittedAccessState[i] == "true")
    ////            {
    ////                m_RightMouseAction[i]->setEnabled(false);
    ////            }
    ////        }
    ////    }
    ////
    ////
    ////    rightMouseMenu.exec(QCursor::pos());
    ////    
    //} 
    //else
    {
        try
        {
            if (bIsChecked)
            {
                openViewer(selectedCamera);
            }
            else
            {
                closeViewer(selectedCamera.Cam());
                onUpdateDeviceList();
            }
        }
        catch (const std::exception& e)
        {
            m_Logger->logging("MainWindow <onCameraClicked> Exception: " + QString::fromStdString(e.what()), VimbaViewerLogCategory_ERROR);
        }
    }

    if (!m_CameraTree->isEnabled())
        m_CameraTree->setEnabled(true);

}

void cameraMainWindow::on_ActionDiscover_triggered(void)
{
    m_CameraTree->setEnabled(false);
    QMutexLocker local_lock(&m_Lock);
    QVector<CameraPtr> tmpCameras;
    {
        for (int i = 0; i < m_Viewer.size(); i++)
        {
            if (m_Viewer[i]->getCamOpenStatus())
            {
                tmpCameras.push_back(m_Viewer[i]->getCameraPtr());
            }
        }
    }
    for (int i = 0; i < tmpCameras.size(); ++i)
    {
        //closeViewer(tmpCameras[i]);
        std::string camName;
        VmbErrorType error = getCameraDisplayName(tmpCameras[i], (camName));
        if (VmbErrorSuccess == error)
        {
            m_Logger->logging("Refreshing camera list, not closing opened camera " + QString::fromStdString(camName), VimbaViewerLogCategory_INFO);
        }
        else
        {
            m_Logger->logging("Refreshing camera list, can not read the opened camera display name " + QString::fromStdString(camName), VimbaViewerLogCategory_WARNING);
        }
        
    }
    onUpdateDeviceList();
    m_CameraTree->setEnabled(true);
}



class IsEqualCamera
{
private:
    CameraPtr m_pCam;
public:
    IsEqualCamera() {}
    IsEqualCamera(CameraPtr pCam) : m_pCam(pCam) {}
    bool operator() (const CameraPtr pCam)
    {
        return SP_ISEQUAL(m_pCam, pCam);
    }
};

void cameraMainWindow::closeViewer(CameraPtr cam, int closefromDisconnect)
{
    /* copy ID for logging because sCamID will be deleted (ref) when m_Viewer destroyed */

    CameraInfoVector::iterator findPos = std::find(m_CameraInfos.begin(), m_CameraInfos.end(), cam);
    if (m_CameraInfos.end() != findPos)
    {
        findPos->SetOpen(false);
    }

    QVector<ViewerWidget*>::iterator findWindowPos = std::find_if(m_Viewer.begin(), m_Viewer.end(), FindWindowByCameraPtr(cam));
    if (m_Viewer.end() != findWindowPos)
    {

        if ((m_activeViewerGrid->isActive() && m_activeViewerGrid->isValid()) || (closefromDisconnect > 0))
        {
            int indx;
            closefromDisconnect >= 0 ? indx = closefromDisconnect : indx = m_activeViewerGrid->getActiveIndex();
            m_ViewerGridLayout->replaceWidget(*findWindowPos, m_ViewerGridPlaceHolder.at(indx) );
            m_ViewerGridPlaceHolder.at(indx)->show();
            //(*findWindowPos)->menu
            closefromDisconnect >= 0 ? m_Logger->logging("Closed from disconnect action, not from camera tree window, window index : " + QString::fromStdString(std::to_string(indx)), VimbaViewerLogCategory_INFO) : 0;

        }
        else
        {
            m_Logger->logging("Add placeholder widget to the layout failed: the selector is not activated or the index range is invalid ", VimbaViewerLogCategory_ERROR);
        }
        delete* findWindowPos;
        *findWindowPos = NULL;
        m_Viewer.erase(findWindowPos);
        m_Logger->logging(tr("Closing The Viewer:") + "\t" + findPos->DisplayName(), VimbaViewerLogCategory_INFO);
        findPos->ResetPermittedAccessState(); /* reset all */
    }
    if (!m_CameraTree->isEnabled())
    {
        m_CameraTree->setDisabled(false);
    }
}

/*locked by on camera clicked*/
void cameraMainWindow::openViewer(CameraInfo& info)
{
    try
    {
        if (!info.IsOpen())
        {
            /*did not use right mouse clicked function*/
            /* if it's not triggered from right mouse click menu, open with best case */
            //if (!m_bIsOpenByRightMouseClick)
            //    m_sOpenAccessType = getBestAccess(info);

            //keep in mind what viewer is open

            m_Viewer.append(new ViewerWidget(this, 0, info.DisplayName(), m_bIsAutoAdjustPacketSize, info.Cam()));

            if (!connect(m_Viewer.back(), SIGNAL(closeViewer(CameraPtr)), this, SLOT(onCloseFromViewer(CameraPtr))))
                m_Logger->logging("MainWindow: Failed connecting SIGNAL (<CameraTreeWindow>closeViewer) to SLOT(<MainWindow>onCloseFromViewer)", VimbaViewerLogCategory_ERROR);

            QString sBestAccess = getBestAccess(info);

            if (m_Viewer.back()->getCamOpenStatus())
            {
                m_Logger->logging(tr("Opening The Viewer:") + "\t" + info.DisplayName(), VimbaViewerLogCategory_INFO);
                info.SetOpen(true);
                /*Use best case in case open camera directly with no  right mouse click */
               /* if (!m_bIsOpenByRightMouseClick)
                {
                    info.SetPermittedAccessState(0, "true");
                }*/

                info.SetPermittedAccessState(0, "true");
                QString sAdjustMsg;
                if (m_Viewer.back()->getAdjustPacketSizeMessage(sAdjustMsg))
                {
                    sAdjustMsg.contains("Failed", Qt::CaseInsensitive) ? m_Logger->logging(sAdjustMsg + info.DisplayName(), VimbaViewerLogCategory_INFO) : m_Logger->logging(sAdjustMsg + info.DisplayName(), VimbaViewerLogCategory_OK);
                }
            }
            else
            {
                info.SetOpen(false);
                if (VmbErrorInvalidAccess == m_Viewer.back()->getOpenError())
                {
                    m_Logger->logging(tr("Opening The Viewer:") + "\t" +
                        info.DisplayName() +
                        " failed! <Cannot open the camera because it is already under control by another application>", VimbaViewerLogCategory_WARNING);
                }
                else
                {
                    m_Logger->logging(tr("Opening The Viewer:") + "\t" +
                        info.DisplayName() +
                        " failed! - Error: " +
                        QString::number(m_Viewer.back()->getOpenError()) +
                        Helper::mapReturnCodeToString(m_Viewer.back()->getOpenError()), VimbaViewerLogCategory_WARNING);
                }

                onCloseFromViewer(info.Cam());
                info.ResetPermittedAccessState();

                /*if (m_sOpenAccessType.isEmpty())
                {
                    if (!m_Viewer.empty())
                    {
                        ViewerWidget* tmpWin = m_Viewer.last();
                        m_Viewer.pop_back();
                        delete tmpWin;

                    }
                }*/
            }
        }
        else
        {
            m_Logger->logging(tr("Opening The Viewer:") + "\t" +
                info.DisplayName() +
                " failed! <Cannot open the same camera twice>", VimbaViewerLogCategory_WARNING);
        }
    }
    catch (const std::exception& e)
    {
        m_Logger->logging("MainWindow <openViewer> Exception: " + QString::fromStdString(e.what()), VimbaViewerLogCategory_ERROR);
    }


    if (m_activeViewerGrid->isActive() && m_activeViewerGrid->isValid())
    {
        m_ViewerGridPlaceHolder.at(m_activeViewerGrid->getActiveIndex())->hide();
        m_ViewerGridLayout->replaceWidget(
            m_ViewerGridPlaceHolder.at(m_activeViewerGrid->getActiveIndex()),
            m_Viewer.back());
        //m_ViewerGridLayout->addWidget()
        m_Viewer.back()->show();
        //m_Viewer.back()->setStyleSheet("border: 1px solid red");
        m_Viewer.back()->getmContextMenu()->addSeparator();
        
        auto indx = m_activeViewerGrid->getActiveIndex();
        auto actions = m_Viewer.back()->getmContextMenu()->actions();
        connect(actions.at(actions.size() - 1 - 2), &QAction::triggered,
            this, [&, indx]() {
                m_dCamList->setWindowTitle("Camera List : " +
                    QString::fromStdString(std::to_string(indx)));
                m_activeViewerGrid->setActiveIndex(indx);
                m_activeViewerGrid->Activate();
                onUpdateDeviceList();//force update camlist
                m_dCamList->show();
            });

        connect(actions.at(actions.size() - 1 - 1), &QAction::triggered,
            this, [&, indx, info]() {
                m_activeViewerGrid->setActiveIndex(indx);
                m_activeViewerGrid->Activate();
                onCloseFromViewer(info.Cam()); });
    }
    else
    {
        m_Logger->logging("Add viewer widget to the layout failed: the selector is not activated or the index range is invalid ", VimbaViewerLogCategory_ERROR);
    }
    //for (size_t i = 0; i < m_Viewer.size(); i++)
    //{
    //    /*i < 3 ? m_ViewerGridLayout->addWidget(m_Viewer[i], 0, i) :
    //        m_ViewerGridLayout->addWidget(m_Viewer[i], 1, i - 3);*/
    //    //m_Viewer[i]->show();
    //    //m_ViewerGridLayout->removeWidget();
    //    m_ViewerGridLayout->addWidget(m_Viewer[i], 0, i);
    //    m_ViewerGridLayout->setRowStretch(0, 1);
    //   
    //}
    //this->setLayout(m_ViewerGridLayout);
    //m_ViewerGridLayout->setAlignment(Qt::AlignCenter);
    //this->centralWidget()->setLayout(m_ViewerGridLayout);



    onUpdateDeviceList();//force update camlist
        /* */
    //std::for_each(m_Viewer.begin(), m_Viewer.end(), (m_ViewerGrid->addWidget));
    //if (m_bIsOpenByRightMouseClick)
    //    m_bIsOpenByRightMouseClick = false;
}


QString  cameraMainWindow::getBestAccess(const CameraInfo& info)
{
    QString sBestAccess;
    QStringList sBestList = info.PermittedAccess();

    if (0 != sBestList.size())
        sBestAccess = sBestList[0];

    return sBestAccess;
}

