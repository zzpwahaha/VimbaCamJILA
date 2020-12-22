
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

    // Connect GUI events with event handlers
    QObject::connect( m_ButtonStartStop, SIGNAL( clicked() ), this, SLOT( OnBnClickedButtonStartstop() ) );

    // Start Vimba
    VmbErrorType err = m_ApiController.StartUp();
    setWindowTitle( QString( "AsynchronousGrab (Qt version) Vimba C++ API Version " )+ QString::fromStdString( m_ApiController.GetVersion() ) );
    Log( "Starting Vimba", err );

    if( VmbErrorSuccess == err )
    {
        // Connect new camera found event with event handler
        QObject::connect( m_ApiController.GetCameraObserver(), SIGNAL( CameraListChangedSignal(int) ), this, SLOT( OnCameraListChanged(int) ) );

        // Initially get all connected cameras
        UpdateCameraListBox();
        std::stringstream strMsg;
        strMsg << "Cameras found..." << m_cameras.size();
        Log(strMsg.str() );
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
    setGeometry(200, 300, 600, 400);
    m_LabelStream = new QLabel;
    m_LabelStream->setAlignment(Qt::AlignCenter);
    m_ButtonStartStop = new QPushButton("Start Acquisition", this);
    m_ComboBoxCameras = new QComboBox(this);
    m_ListLog = new QListWidget(this);

    QGridLayout* layout = new QGridLayout(this);

    layout->addWidget(m_LabelStream, 0, 0, 1, 2);
    layout->addWidget(m_ComboBoxCameras, 1, 0);
    layout->addWidget(m_ButtonStartStop, 1, 1);
    layout->addWidget(m_ListLog, 2, 0, 1, 2);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    
}


void AsynchronousGrab::OnBnClickedButtonStartstop()
{
    VmbErrorType err;
    int nRow = m_ComboBoxCameras->currentIndex();
    //int nRow = ui.m_ListBoxCameras->currentRow();

    if( -1 < nRow )
    {
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
            Log( "Starting Acquisition", err );
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
            Log( "Stopping Acquisition", err );
        }

        if( false == m_bIsStreaming )
        {
            m_ButtonStartStop->setText( QString( "Start Acquisition" ) );
        }
        else
        {
            m_ButtonStartStop->setText( QString( "Stop Acquisition" ) );
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
            Log("frame pointer is NULL, late frame ready message");
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
            Log( "Failure in receiving image", VmbErrorOther );
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
        Log("Camera list changed. A new camera was discovered by Vimba.");
        bUpdateList = true;
    }
    else if (VmbAPI::UpdateTriggerPluggedOut == reason)
    {
        Log( "Camera list changed. A camera was disconnected from Vimba." );
        if( true == m_bIsStreaming )
        {
            OnBnClickedButtonStartstop();
        }
        bUpdateList = true;
    }

    if( true == bUpdateList )
    {
        UpdateCameraListBox();
    }

    m_ButtonStartStop->setEnabled( 0 < m_cameras.size() || m_bIsStreaming );
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
        Log( "Could not set source image info", static_cast<VmbErrorType>( Result ) );
        return static_cast<VmbErrorType>( Result );
    }
    QString             OutputFormat;
    const int           bytes_per_line = pOutImage.bytesPerLine();
    switch( pOutImage.format() )
    {
    default:
        Log( "unknown output format",VmbErrorBadParameter );
        return VmbErrorBadParameter;
    case QImage::Format_RGB888:
        if( nWidth*3 != bytes_per_line )
        {
            Log( "image transform does not support stride",VmbErrorWrongType );
            return VmbErrorWrongType;
        }
        OutputFormat = "RGB24";
        break;
    }
    Result = VmbSetImageInfoFromString( OutputFormat.toStdString().c_str(), OutputFormat.length(),nWidth,nHeight, &DestImage );
    if( VmbErrorSuccess != Result )
    {
        Log( "could not set output image info",static_cast<VmbErrorType>( Result ) );
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
            Log( "could not set matrix t o transform info ", static_cast<VmbErrorType>( Result ) );
            return static_cast<VmbErrorType>( Result );
        }
    }
    else
    {
        Result = VmbImageTransform(&SourceImage, &DestImage, nullptr, 0);
    }
    if( VmbErrorSuccess != Result )
    {
        Log( "could not transform image", static_cast<VmbErrorType>( Result ) );
        return static_cast<VmbErrorType>( Result );
    }
    return static_cast<VmbErrorType>( Result );
}


// Queries and lists all known camera
void AsynchronousGrab::UpdateCameraListBox()
{
    // Get all cameras currently connected to Vimba
    VmbAPI::CameraPtrVector cameras = m_ApiController.GetCameraList();

    // Simply forget about all cameras known so far
    m_ComboBoxCameras->clear();
    m_cameras.clear();

    // And query the camera details again
    for(    VmbAPI::CameraPtrVector::const_iterator iter = cameras.begin();
            cameras.end() != iter;
            ++iter )
    {
        std::string strCameraName;
        std::string strCameraID;
        if( VmbErrorSuccess != (*iter)->GetName( strCameraName ) )
        {
            strCameraName = "[NoName]";
        }
        // If for any reason we cannot get the ID of a camera we skip it
        if( VmbErrorSuccess == (*iter)->GetID( strCameraID ) )
        {
            m_ComboBoxCameras->addItem( QString::fromStdString( strCameraName + " " +strCameraID ) );
            m_cameras.push_back( strCameraID );
        }
    }

    m_ButtonStartStop->setEnabled( 0 < m_cameras.size() || m_bIsStreaming );
}

// Prints out a given logging string, error code and the descriptive representation of that error code
// Parameters:
//  [in]    strMsg          A given message to be printed out
//  [in]    eErr            The API status code
void AsynchronousGrab::Log( std::string strMsg, VmbErrorType eErr )
{
    strMsg += "..." + m_ApiController.ErrorCodeToMessage( eErr );
    m_ListLog->insertItem( 0, QString::fromStdString( strMsg ) );
}

// Prints out a given logging string
// Parameters:
//  [in]    strMsg          A given message to be printed out
void AsynchronousGrab::Log( std::string strMsg)
{
    m_ListLog->insertItem( 0, QString::fromStdString( strMsg ) );
}
