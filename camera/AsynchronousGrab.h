#pragma once

#include <QtWidgets>
#include <QMainWindow>
#include <ApiController.h>

using namespace AVT;

class AsynchronousGrab : public QWidget
{
    Q_OBJECT

public:
    AsynchronousGrab( QWidget *parent = nullptr, Qt::WindowFlags flags = 0 );
    ~AsynchronousGrab();

private:
    // The Qt GUI
    /*Ui::AsynchronousGrabClass ui;*/
    QPushButton* m_ButtonStartStop;
    QLabel* m_LabelStream;
    QComboBox* m_ComboBoxCameras; //m_ListBoxCameras
    QListWidget* m_ListLog;

    // Our controller that wraps API access
    ApiController m_ApiController;
    // A list of known camera IDs
    std::vector<std::string> m_cameras;
    // Are we streaming?
    bool m_bIsStreaming;
    // Our Qt image to display
    QImage m_Image;

    //initialize 
    void setupGuiLayout();

    // Queries and lists all known camera
    void UpdateCameraListBox();
    
    // Prints out a given logging string, error code and the descriptive representation of that error code
    // Parameters:
    //  [in]    strMsg          A given message to be printed out
    //  [in]    eErr            The API status code
    void Log( std::wstring strMsg, VmbErrorType eErr );

    // Prints out a given logging string
    // Parameters:
    //  [in]    strMsg          A given message to be printed out
    void Log( std::wstring strMsg);
    

    // Copies the content of a byte buffer to a Qt image with respect to the image's alignment
    // Parameters:
    //  [in]    pInbuffer       The byte buffer as received from the cam
    //  [in]    ePixelFormat    The pixel format of the frame
    //  [out]   OutImage        The filled Qt image
    VmbErrorType CopyToImage( VmbUchar_t *pInBuffer, VmbPixelFormat_t ePixelFormat, QImage &pOutImage, const float *Matrix = NULL );

private slots:
    // The event handler for starting / stopping acquisition
    void OnBnClickedButtonStartstop();
    
    // This event handler (Qt slot) is triggered through a Qt signal posted by the frame observer
    // Parameters:
    //  [in]    status          The frame receive status (complete, incomplete, ...)
    void OnFrameReady( int status );
    
    // This event handler (Qt slot) is triggered through a Qt signal posted by the camera observer
    // Parameters:
    //  [in]    reason          The reason why the callback of the observer was triggered (plug-in, plug-out, ...)
    void OnCameraListChanged( int reason );
};

