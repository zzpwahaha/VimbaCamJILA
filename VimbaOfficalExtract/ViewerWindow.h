/*=============================================================================
  Copyright (C) 2012 - 2019 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        ViewerWindow.h

  Description: The viewer window framework.
               This contains of dock widgets like camera feature tree, a histogram, a toolbar and MDI window


-------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/


#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <QtGui>
#include <QString>
#include <VimbaCPP/Include/VimbaSystem.h>
#include "ui_ViewerWindow.h"
#include "ui_SaveImageOption.h"
#include "ui_DirectAccess.h"
#include "ui_ViewerOption.h"
#include "ui_SavingProgress.h"
#include "FrameObserver.h"
#include "ImageWriter.h"
#include "ILogTarget.h"

#include <QtWidgets>

class DockWidgetWindow;
class ControllerTreeWindow;
class LineEditCompleter;
class Viewer;
class MainInformationWindow;
class HistogramWindow;
class TabExtensionInterface;
class QTimer;

class SaveImageThread : public QThread, public ILogTarget
{
    Q_OBJECT
        typedef std::pair<unsigned int,QImage>      ImagePair;
        typedef std::pair<unsigned int,tFrameInfo>  FramePair;

        ConsumerQueue<ImagePair>        m_Images;
        ConsumerQueue<FramePair>        m_16BitImages;
    private:
        unsigned int                    m_nTNumber2Save;

        QString                         m_sImagePath;
        QString                         m_sSaveFormat;
        QString                         m_sSaveName;
        bool                            m_bSave16BitImages;

    public:
        void SetNumberToSave( unsigned int NumberToSave)
        {
            m_nTNumber2Save = NumberToSave;
        }
        void SetPath( const QString & ImagePath)
        {
            m_sImagePath = ImagePath;
        }
        void SetBaseName( const QString & BaseName)
        {
            m_sSaveName = BaseName;
        }
        void SetSaveFormat( const QString & SaveFormat)
        {
            m_sSaveFormat = SaveFormat;
        }
        void SetSave16Bit( bool Save16Bit )
        {
            m_bSave16BitImages = Save16Bit;
        }
    public:
        SaveImageThread ( ){};
        ~SaveImageThread ()
        {
            StopProcessing();
            wait();
        };
        void Log( const QString&s)
        {
            emit LogMessage( s );
        }
        void StopProcessing()
        {
            m_Images.StopProcessing();
            m_16BitImages.StopProcessing();
        }
        bool ImageEmpty()       const   { return m_Images.IsEmpty(); }
        bool FrameInfoEmpty()   const   { return m_16BitImages.IsEmpty(); }
        void Enqueue( const QImage &image, unsigned int ImageCounter )
        {
            m_Images.Enqueue( std::make_pair(ImageCounter, image) );
        }
        void Enqueue( const tFrameInfo &info, unsigned int ImageCounter)
        {
            m_16BitImages.Enqueue( std::make_pair( ImageCounter,info ) );
        }
    private:

    protected:
              virtual void run();

    private slots:

    signals:
              void setPosition ( unsigned int nPos );
              void LogMessage( const QString &msg);
};

class ViewerWindow : public QMainWindow, private Ui::ViewerWindow
{
    Q_OBJECT
    public:

    protected:

    private:
                static const unsigned int           BUFFER_COUNT    =   7;
                static const unsigned int           IMAGES_COUNT    =   50;
                QTimer                             *m_Timer;
                QString                             m_sCameraID;
                VmbError_t                          m_OpenError;
                bool                                m_bIsCameraRunning;
                bool                                m_bHasJustStarted;
                bool                                m_bIsCamOpen;
                bool                                m_bIsFirstStart;
                bool                                m_bIsRedHighlighted;
                bool                                m_bIsViewerWindowClosing;

                QString                             m_sAccessMode;
                QAction                            *m_ResetPosition;
                QCheckBox                          *m_ToolTipCheckBox;

                DockWidgetWindow                   *m_DockController;
                DockWidgetWindow                   *m_DockInformation;
                DockWidgetWindow                   *m_DockHistogram;

                Viewer                             *m_ScreenViewer;
                MainInformationWindow              *m_InformationWindow;
                ControllerTreeWindow               *m_Controller;
                HistogramWindow                    *m_HistogramWindow;

                CameraPtr                           m_pCam;
                QTextEdit                          *m_Description;

                QLabel                             *m_OperatingStatusLabel;
                QLabel                             *m_FormatLabel;
                QLabel                             *m_ImageSizeLabel;
                QLabel                             *m_FramerateLabel;
                QLabel                             *m_FramesLabel;

                QSharedPointer<QGraphicsScene>      m_pScene;
                QGraphicsPixmapItem                *m_PixmapItem;
                QGraphicsTextItem                  *m_TextItem;

                SP_DECL( FrameObserver )            m_pFrameObs;

                QString                             m_SaveFileDir;
                QString                             m_SelectedExtension;
                QString                             m_SaveImageName;
                /* Save Image Option */
                Ui::SavingOptionsDialog             m_SaveImageOption;
                QDialog                            *m_ImageOptionDialog;
                QFileDialog                        *m_getDirDialog; // multiple images
                QFileDialog                        *m_saveFileDialog; // save an image

                /* Direct Access */
                Ui::DirectAccessDialog              m_DirectAccess;
                QDialog                            *m_AccessDialog;

                /*Viewer Option */
                Ui::DisplayOptionsDialog            m_ViewerOption;
                QDialog                            *m_ViewerOptionDialog;
                bool                                m_bIsDisplayEveryFrame;

                /* Filter Pattern */
                LineEditCompleter                  *m_FilterPatternLineEdit;

                /*  Saving images progress */
                QSharedPointer<SaveImageThread>     m_SaveImageThread;
                Ui::SaveImagesProgressDialog        m_SaveImagesUIDialog;
                QDialog                            *m_SaveImagesDialog;
                unsigned int                        m_nNumberOfFramesToSave;
                unsigned int                        m_nImageCounter;
                QString                             m_SaveName;
                QString                             m_SaveFormat;
                bool                                m_Allow16BitMultiSave;
                QString                             m_ImagePath;
                QVector<QRgb>                       m_ColorTable;
                bool                                m_bIsTriggeredByMultiSaveBtn;

                /*  Used to save full bit depth images */
                ImageWriter                         m_TiffWriter;
                tFrameInfo                          m_FullBitDepthImage;
                bool                                m_LibTiffAvailable;

                unsigned int                        m_FrameBufferCount;

                QVector<TabExtensionInterface*>     m_tabExtensionInterface; // Closed source injected
                int                                 m_TabPluginCount;

    public:
                ViewerWindow ( QWidget *parent = 0, Qt::WindowFlags flag = 0,  QString sID = " ", QString sAccess = " ", bool bAutoAdjustPacketSize = false, CameraPtr pCam = CameraPtr() );
               ~ViewerWindow ();

                bool        getCamOpenStatus            () const;
                bool        isControlledCamera          ( const CameraPtr &cam) const;
                CameraPtr   getCameraPtr                ();
                VmbError_t  getOpenError                () const;
                QString     getCameraID                 () const;
                bool        getAdjustPacketSizeMessage  ( QString &sMessage );

    protected:
                virtual void closeEvent                 ( QCloseEvent *event );

    private:
                void        showSplashScreen            ( const QString &,QWidget*parent);
                VmbError_t  releaseBuffer               ();
                void        checkDisplayInterval        ();
                void        changeEvent                 ( QEvent * event );
                bool        isDestPathWritable          ();
                bool        checkUsedName               ( const QStringList &files );
                template <typename T>
                void        endianessConvert            ( T &v );
                bool        CanReduceBpp                ();
                QImage      ReduceBpp                   ( QImage image );
                bool        isStreamingAvailable        ();
                bool        isSupportedPixelFormat      ();

    private slots:

                /* when you use this std convention, you don't need any "connect..." */
                void on_ActionFreerun_triggered         ();
                void on_ActionResetPosition_triggered   ();
                void on_ActionHistogram_triggered       ();
                void on_ActionOriginalSize_triggered    ();
                void on_ActionSaveAs_triggered          ();
                void on_ActionSaveOptions_triggered     ();
                void on_ActionSaveImages_triggered      ();
                void on_ActionRegister_triggered        ();
                void on_ActionDisplayOptions_triggered  ();
                void on_ActionFitToWindow_triggered     ();
                void on_ActionLeftRotation_triggered    ();
                void on_ActionRightRotation_triggered   ();
                void on_ActionZoomOut_triggered         ();
                void on_ActionZoomIn_triggered          ();
                void on_ActionLoadCameraSettings_triggered ();
                void on_ActionSaveCameraSettings_triggered ();
                void on_ActionLoadCameraSettingsMenu_triggered ();
                void on_ActionSaveCameraSettingsMenu_triggered ();
                void on_ActionAllow16BitTiffSaving_triggered ();

                /* custom */
                void onAcquisitionStartStop             ( const QString &sThisFeature );
                void onfloatingDockChanged              ( bool bIsFloating );
                void onVisibilityChanged                ( bool bIsVisible );
                void displayEveryFrameClick             ( bool bValue );
                void onSetDescription                   ( const QString &sDesc );
                void onimageReady                       ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth );
                void onFullBitDepthImageReady           ( tFrameInfo mFullImageInfo );
                void onSaving                           ( unsigned int nPos );
                void onSetEventMessage                  ( const QStringList &sMsg );
                void onSetCurrentFPS                    ( const QString &sFPS );
                void onSetFrameCounter                  ( const unsigned int &nNumberOfFrames );
                void onFeedLogger                       ( const QString &sMessage );
                void onResetFPS                         ();
                void getSaveDestinationPath             ();
                void acceptSaveImagesDlg                ();
                void rejectSaveImagesDlg                ();
                void writeRegisterData                  ();
                void readRegisterData                   ();
                void endianessChanged                   ( int );
                VmbError_t onPrepareCapture             ();
                void onSetColorInterpolation            ( const bool &bState );
                void onSetHistogramData                 ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle,
                                                          const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis, const QVector <QStringList> &statistics );
                void updateColorInterpolationState      ();
                void onTooltipCheckBoxClick             ( bool bValue );
                void enableMenuAndToolbar               ( bool bValue );
                void directAccessHexTextChanged         ( const QString &sText );
                void directAccessDecTextChanged         ( const QString &sText );
                void optionsFrameCountChanged           ( const QString &sText );
                void optionsAccepted                    ();
                void textFilterChanged                  ();
                bool loadPlugin                         ();  // Closed source injected


    signals:
                void closeViewer                        ( CameraPtr cam );
                void acquisitionRunning                 ( bool bValue );
};

#endif
