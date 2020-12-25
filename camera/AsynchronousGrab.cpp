
#include <sstream>

#include "AsynchronousGrab.h"
#include "VimbaImageTransform\Include\VmbTransform.h"
#define NUM_COLORS 3
#define BIT_DEPTH 8



using namespace AVT;

AsynchronousGrab::AsynchronousGrab( QWidget *parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags )
    , m_bIsStreaming( false )
{
    //Gui
    setupGuiLayout();
    creatMenu();
    
    // Connect GUI events with event handlers
    //QObject::connect( m_ButtonStartStop, SIGNAL( clicked() ), this, SLOT( OnBnClickedButtonStartstop() ) );

    // Start Vimba
    VmbErrorType err = m_ApiController.StartUp();
    setWindowTitle(QString("MOT Cam Mako") + QString::fromStdString(m_ApiController.GetVersion()));
    Log( L"Starting Vimba", err );

    if( VmbErrorSuccess == err )
    {
        // Connect new camera found event with event handler
        QObject::connect( m_ApiController.GetCameraObserver(), SIGNAL( CameraListChangedSignal(int) ), this, SLOT( OnCameraListChanged(int) ) );

        // Initially get all connected cameras
        UpdateCameraListMenu();
        std::wstringstream strMsg;
        strMsg << "Cameras found... " << m_cameras.size() << " in total";
        Log( strMsg.str() );
        
    }
}

AsynchronousGrab::~AsynchronousGrab()
{
    // if we are streaming stop streaming
    if( true == m_bIsStreaming )
    {
        OnBnClickedButtonStartstop();
    }

    // Before we close the application we stop Vimba
    m_ApiController.ShutDown();
}


void AsynchronousGrab::setupGuiLayout()
{
    m_centralWidget = new QWidget(this);
           
    setGeometry(200, 300, 800, 600);
    m_LabelStream = new QLabel(m_centralWidget);
    m_LabelStream->setAlignment(Qt::AlignCenter);
    //m_ButtonStartStop = new QPushButton("Start Acquisition", m_centralWidget);
    //m_ComboBoxCameras = new QComboBox(m_centralWidget);

    QGridLayout* layout = new QGridLayout(m_centralWidget);
    layout->addWidget(m_LabelStream, 0, 0, 1, 2);
    //layout->addWidget(m_ComboBoxCameras, 1, 0);
    //layout->addWidget(m_ButtonStartStop, 1, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    layout->setRowStretch(0, 1);
    m_statusBar = new QLabel("No camera selected now...");
    statusBar()->addPermanentWidget(m_statusBar, 1);
    m_statusBar->setAlignment(Qt::AlignLeft);
    this->setCentralWidget(m_centralWidget);

    /*Log Menu*/
    d_Log = new QDialog(this);
    d_Log->setWindowTitle("Log");
    //m_ListLog = new QListWidget(d_Log);
    m_ListLog = new QTextEdit(d_Log);
    m_ListLog->setReadOnly(TRUE);
    //m_ListLog->setFontPointSize(9);
    m_ListLog->setMinimumWidth(500);
    QVBoxLayout* Dlayout = new QVBoxLayout(d_Log);
    auto m_OkButton = new QDialogButtonBox(QDialogButtonBox::Ok, d_Log);
    connect(m_OkButton, &QDialogButtonBox::accepted, d_Log, &QDialog::accept);
    Dlayout->addWidget(m_ListLog, 1);
    Dlayout->addWidget(m_OkButton);

}

void AsynchronousGrab::creatMenu()
{
    //QMenuBar* menu = new QMenuBar(this);
    QMenu* camM = menuBar()->addMenu("&Camera");
    m_MenuCameras = camM->addMenu("&Camera List");

    a_MenuLog = camM->addAction("&Log");
    connect(a_MenuLog, &QAction::triggered, this, [&]() {d_Log->exec();});
    /*can also use [=](){this->d_Log->exec()} etc. see https://en.cppreference.com/w/cpp/language/lambda*/
    a_MenuStart = camM->addAction("Start acquisition");
    a_MenuStart->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(a_MenuStart, &QAction::triggered, this, 
        &AsynchronousGrab::OnBnClickedButtonStartstop);
}

void AsynchronousGrab::OnBnClickedButtonStartstop()
{
    VmbErrorType err;
    QList<QActionGroup*> camlist = m_MenuCameras->findChildren<QActionGroup*>();
    if ( (camlist.size() == 1) && camlist[0]->checkedAction() != nullptr)
    {
        QString cam_text = camlist[0]->checkedAction()->text();
        int nRow = camlist[0]->checkedAction()->data().toInt();
        if( false == m_bIsStreaming )
        {
            // Start acquisition
            err = m_ApiController.StartContinuousImageAcquisition( m_cameras[nRow] );
            // Set up Qt image
            if ( VmbErrorSuccess == err )
            {
                m_Image = QImage( m_ApiController.GetWidth(),
                                  m_ApiController.GetHeight(),
                                  QImage::Format_RGB888 );

                QObject::connect( m_ApiController.GetFrameObserver(), SIGNAL( FrameReceivedSignal(int) ), this, SLOT( OnFrameReady(int) ) );
            }
            Log( L"Starting Acquisition", err );
            setWindowTitle(cam_text + ", Capturing");
            m_bIsStreaming = VmbErrorSuccess == err;
        }
        else
        {
            m_bIsStreaming = false;
            // Stop acquisition
            err = m_ApiController.StopContinuousImageAcquisition();
            // Clear all frames that we have not picked up so far
            m_ApiController.ClearFrameQueue();
            m_Image = QImage();
            Log( L"Stopping Acquisition", err );
            setWindowTitle(QString("MOT Cam Mako") + QString::fromStdString(m_ApiController.GetVersion()));
        }

        if( false == m_bIsStreaming )
        {
            a_MenuStart->setText( QString( "Start Acquisition" ) );
        }
        else
        {
            a_MenuStart->setText( QString( "Stop Acquisition" ) );
        }
    }
}

// This event handler (Qt slot) is triggered through a Qt signal posted by the frame observer
// Parameters:
//  [in]    status          The frame receive status (complete, incomplete, ...)
void AsynchronousGrab::OnFrameReady( int status )
{
    if( true == m_bIsStreaming )
    {
        // Pick up frame
        VmbAPI::FramePtr pFrame = m_ApiController.GetFrame();
        if( SP_ISNULL( pFrame ) )
        {
            Log(L"frame pointer is NULL, late frame ready message");
            return;
        }
        // See if it is not corrupt
        if( VmbFrameStatusComplete == status )
        {
            VmbUchar_t *pBuffer;
            VmbErrorType err = SP_ACCESS( pFrame )->GetImage( pBuffer );
            if( VmbErrorSuccess == err )
            {
                VmbUint32_t nSize;
                err = SP_ACCESS( pFrame )->GetImageSize( nSize );
                if( VmbErrorSuccess == err )
                {
                    VmbPixelFormatType ePixelFormat = m_ApiController.GetPixelFormat();
                    if( ! m_Image.isNull() )
                    {
                        // Copy it
                        // We need that because Qt might repaint the view after we have released the frame already
                        /*if( ui.m_ColorProcessingCheckBox->checkState()==  Qt::Checked )
                        {
                            static const VmbFloat_t Matrix[] = {    8.0f, 0.1f, 0.1f, // this matrix just makes a quick color to mono conversion
                                                                    0.1f, 0.8f, 0.1f,
                                                                    0.0f, 0.0f, 1.0f };
                            if( VmbErrorSuccess != CopyToImage( pBuffer,ePixelFormat, m_Image, Matrix ) )
                            {
                                ui.m_ColorProcessingCheckBox->setChecked( false );
                            }
                        }
                        else
                        {
                            CopyToImage( pBuffer,ePixelFormat, m_Image );
                        }*/
                        CopyToImage(pBuffer, ePixelFormat, m_Image);
                        // Display it
                        const QSize s = m_LabelStream->size() ;
                        m_LabelStream->setPixmap( QPixmap::fromImage( m_Image ).scaled(s,Qt::KeepAspectRatio ) );
                    }
                }
            }
        }
        else
        {
            // If we receive an incomplete image we do nothing but logging
            Log( L"Failure in receiving image", VmbErrorOther );
        }

        // And queue it to continue streaming
        m_ApiController.QueueFrame( pFrame );
    }
}


// This event handler (Qt slot) is triggered through a Qt signal posted by the camera observer
// Parameters:
//  [in]    reason          The reason why the callback of the observer was triggered (plug-in, plug-out, ...)
void AsynchronousGrab::OnCameraListChanged( int reason )
{
    bool bUpdateList = false;

    // We only react on new cameras being found and known cameras being unplugged
    if (VmbAPI::UpdateTriggerPluggedIn == reason)
    {
        Log(L"Camera list changed. A new camera was discovered by Vimba.");
        bUpdateList = true;
    }
    else if (VmbAPI::UpdateTriggerPluggedOut == reason)
    {
        Log( L"Camera list changed. A camera was disconnected from Vimba." );
        if( true == m_bIsStreaming )
        {
            OnBnClickedButtonStartstop();
        }
        bUpdateList = true;
    }

    if( true == bUpdateList )
    {
        /*UpdateCameraListBox();*/
        UpdateCameraListMenu();
    }

    a_MenuStart->setEnabled( 0 < m_cameras.size() || m_bIsStreaming );
}

// Copies the content of a byte buffer to a Qt image with respect to the image's alignment
// Parameters:
//  [in]    pInbuffer       The byte buffer as received from the cam
//  [in]    ePixelFormat    The pixel format of the frame
//  [out]   OutImage        The filled Qt image
//
VmbErrorType AsynchronousGrab::CopyToImage( VmbUchar_t *pInBuffer, VmbPixelFormat_t ePixelFormat, QImage &pOutImage, const float *Matrix /*= NULL */ )
{
    const int           nHeight = m_ApiController.GetHeight();
    const int           nWidth  = m_ApiController.GetWidth();

    VmbImage            SourceImage,DestImage;
    VmbError_t          Result;
    SourceImage.Size    = sizeof( SourceImage );
    DestImage.Size      = sizeof( DestImage );

    Result = VmbSetImageInfoFromPixelFormat( ePixelFormat, nWidth, nHeight, & SourceImage );
    if( VmbErrorSuccess != Result )
    {
        Log( L"Could not set source image info", static_cast<VmbErrorType>( Result ) );
        return static_cast<VmbErrorType>( Result );
    }
    QString             OutputFormat;
    const int           bytes_per_line = pOutImage.bytesPerLine();
    switch( pOutImage.format() )
    {
    default:
        Log( L"unknown output format",VmbErrorBadParameter );
        return VmbErrorBadParameter;
    case QImage::Format_RGB888:
        if( nWidth*3 != bytes_per_line )
        {
            Log( L"image transform does not support stride",VmbErrorWrongType );
            return VmbErrorWrongType;
        }
        OutputFormat = "RGB24";
        break;
    }
    Result = VmbSetImageInfoFromString( OutputFormat.toStdString().c_str(), OutputFormat.length(),nWidth,nHeight, &DestImage );
    if( VmbErrorSuccess != Result )
    {
        Log( L"could not set output image info",static_cast<VmbErrorType>( Result ) );
        return static_cast<VmbErrorType>( Result );
    }
    SourceImage.Data    = pInBuffer;
    DestImage.Data      = pOutImage.bits();
    // do color processing? we do not do here so the if clasue can be ignored -zzp
    if( nullptr != Matrix )
    {
        VmbTransformInfo TransformParameter;
        Result = VmbSetColorCorrectionMatrix3x3( Matrix, &TransformParameter );
        if( VmbErrorSuccess == Result )
        {
            Result = VmbImageTransform( &SourceImage, &DestImage, &TransformParameter,1 );
        }
        else
        {
            Log( L"could not set matrix t o transform info ", static_cast<VmbErrorType>( Result ) );
            return static_cast<VmbErrorType>( Result );
        }
    }
    else
    {
        Result = VmbImageTransform(&SourceImage, &DestImage, nullptr, 0);
    }
    if( VmbErrorSuccess != Result )
    {
        Log( L"could not transform image", static_cast<VmbErrorType>( Result ) );
        return static_cast<VmbErrorType>( Result );
    }
    return static_cast<VmbErrorType>( Result );
}


void AsynchronousGrab::UpdateCameraListMenu()
{
    // Get all cameras currently connected to Vimba
    VmbAPI::CameraPtrVector cameras = m_ApiController.GetCameraList();

    // Simply forget about all cameras known so far
    m_MenuCameras->clear();
    QList<QActionGroup*> camlist = m_MenuCameras->findChildren<QActionGroup*>();
    std::for_each(camlist.begin(), camlist.end(), [](auto ctmp) {ctmp->~QActionGroup(); });
    //QList<QActionGroup*> camlist2 = m_MenuCameras->findChildren<QActionGroup*>();
    m_cameras.clear();
    QActionGroup* camgroup = new QActionGroup(m_MenuCameras);
    // And query the camera details again
    int iter_cnts = 0;
    for (VmbAPI::CameraPtrVector::const_iterator iter = cameras.begin();
        cameras.end() != iter;
        ++iter)
    {
        std::string strCameraName;
        std::string strCameraID;
        
        if (VmbErrorSuccess != (*iter)->GetName(strCameraName))
        {
            strCameraName = "[NoName]";
        }
        // If for any reason we cannot get the ID of a camera we skip it
        if (VmbErrorSuccess == (*iter)->GetID(strCameraID))
        {
            QAction* tmp = m_MenuCameras->addAction(QString::fromStdString(strCameraName + " " + strCameraID));
            tmp->setCheckable(true);
            tmp->setData(iter_cnts);
            camgroup->addAction(tmp);
            m_cameras.push_back(strCameraID);
        }
        iter_cnts += 1;
    }

    a_MenuStart->setEnabled(0 < m_cameras.size() || m_bIsStreaming);
}


// Prints out a given logging string, error code and the descriptive representation of that error code
// Parameters:
//  [in]    strMsg          A given message to be printed out
//  [in]    eErr            The API status code
void AsynchronousGrab::Log( std::wstring strMsg, VmbErrorType eErr )
{
    strMsg += L"..." + m_ApiController.ErrorCodeToMessage( eErr );
    m_ListLog->append( QString::fromWCharArray( strMsg.c_str() ) );
    m_statusBar->setText(QString::fromStdWString(strMsg.c_str()));
    //use QString::fromWCharArray( strMsg.c_str() ) instead of 
    //using QString::fromStdWString( strMsg )
    //just incase https://stackoverflow.com/questions/14726304/best-way-to-convert-stdwstring-to-qstring
}

// Prints out a given logging string
// Parameters:
//  [in]    strMsg          A given message to be printed out
void AsynchronousGrab::Log( std::wstring strMsg)
{
    m_ListLog->append(  QString::fromWCharArray(strMsg.c_str()));
    m_statusBar->setText(QString::fromStdWString(strMsg.c_str()));
    //use QString::fromWCharArray( strMsg.c_str() ) instead of 
    //using QString::fromStdWString( strMsg )
    //just incase https://stackoverflow.com/questions/14726304/best-way-to-convert-stdwstring-to-qstring

}
