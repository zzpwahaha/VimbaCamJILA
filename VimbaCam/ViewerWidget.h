#pragma once
#include <QtWidgets>
#include <QString>



#include <VimbaCPP/Include/VimbaSystem.h>
#include "FrameObserver.h"
#include "ILogTarget.h"
#include "UI/ControllerTreeWindow.h"
#include "UI/MainInformationWindow.h"
#include "UI/LineEditCompleter.h"
#include "UI/SortFilterProxyModel.h"

#include "UI/ImageCalculatingThread.h"
#include "UI/RangeSlider.h"
#include "ExternLib/qcustomplot/qcustomplot.h"

class ViewerWidget :  public QWidget
{
    Q_OBJECT
public:
protected:
private:
    static const unsigned int           BUFFER_COUNT = 7;
    static const unsigned int           IMAGES_COUNT = 50;
    QTimer* m_Timer;
    QString                             m_sCameraID;
    VmbError_t                          m_OpenError;
    bool                                m_bIsCameraRunning;
    bool                                m_bHasJustStarted;
    bool                                m_bIsCamOpen;
    bool                                m_bIsFirstStart;
    bool                                m_bIsRedHighlighted;
    bool                                m_bIsViewerWindowClosing;

    QString                             m_sAccessMode;

    QDialog*                            m_DiagController;
    QDialog*                            m_DiagInfomation;
    QDialog*                            m_DiagRSlider;
    RangeSlider*                        m_RSliderV;
    QSpinBox*                           m_upperSB;
    QSpinBox*                           m_lowerSB;
    QString                            m_sSliderFormat;

    MainInformationWindow*              m_InformationWindow;
    ControllerTreeWindow*               m_Controller;


    CameraPtr                           m_pCam;
    QTextEdit*                          m_Description;

    QLabel*                             m_OperatingStatusLabel;
    QPushButton*                        m_FormatButton;
    QPushButton*                        m_ImageSizeButtonH;
    QPushButton*                        m_ImageSizeButtonW;
    QPushButton*                        m_FramerateButton;
    QLabel*                             m_FramesLabel;
    QLabel*                             m_CursorScenePosLabel;
    QPushButton*                        m_ExposureTimeButton;
    QPushButton*                        m_CameraGainButton;

    QSharedPointer<QCustomPlot>         m_QCP;
    QCPAxisRect*                        m_QCPcenterAxisRect;
    QCPAxisRect*                        m_QCPbottomAxisRect;
    QCPAxisRect*                        m_QCPleftAxisRect;
    QSharedPointer<QCPColorMap>         m_colorMap;
    QSharedPointer<QCPGraph>            m_bottomGraph;
    QSharedPointer<QCPGraph>            m_leftGraph;
    QCPColorScale*                      m_colorScale;
    QCPItemTracer*                      m_QCPtracerbottom;
    QCPItemText*                        m_QCPtraceTextbottom;
    QCPItemTracer*                      m_QCPtracerleft;
    QCPItemText*                        m_QCPtraceTextleft;

    QGraphicsTextItem*                  m_TextItem;
    QMenu*                              m_ContextMenu;
    QAction*                            m_aStartStopCap;
    QAction*                            m_aDiagCtrler;
    QAction*                            m_aDiagInfo;
    QAction*                            m_aSetCurrScrROI;
    QAction*                            m_aResetFullROI;
    QAction*                            m_aPlotTracer;
    QAction*                            m_aPlotFitter;
    QAction*                            m_aManualCscale;
    QAction*                            m_aSaveCamSetting;
    QAction*                            m_aLoadCamSetting;
    QAction*                            m_aSaveImg;
    QAction*                            m_aCamlist;
    QAction*                            m_aDisconnect;


    SP_DECL(FrameObserver)              m_pFrameObs;
    ImageCalculatingThread*             m_pImgCThread;


    /* Save Image Option */
    QString                             m_SaveFileDir;
    QString                             m_SelectedExtension;
    //QString                             m_SaveImageName;
    //Ui::SavingOptionsDialog             m_SaveImageOption;
    //QDialog* m_ImageOptionDialog;
    //QFileDialog* m_getDirDialog; // multiple images
    QFileDialog*                        m_saveFileDialog; // save an image

    bool                                m_bIsDisplayEveryFrame;

    /* Filter Pattern */
    LineEditCompleter*                  m_FilterPatternLineEdit;

    /*  Saving images progress */
    //QSharedPointer<SaveImageThread>     m_SaveImageThread;
    //Ui::SaveImagesProgressDialog        m_SaveImagesUIDialog;
    //QDialog* m_SaveImagesDialog;
    //unsigned int                        m_nNumberOfFramesToSave;
    //unsigned int                        m_nImageCounter;
    //QString                             m_SaveName;
    //QString                             m_SaveFormat;
    //bool                                m_Allow16BitMultiSave;
    //QString                             m_ImagePath;
    //QVector<QRgb>                       m_ColorTable;
    //bool                                m_bIsTriggeredByMultiSaveBtn;

    /*  Used to save full bit depth images */
    //ImageWriter                         m_TiffWriter;
    //tFrameInfo                          m_FullBitDepthImage;
    //bool                                m_LibTiffAvailable;

    unsigned int                        m_FrameBufferCount;

    /*tab extension*/
    //QVector<TabExtensionInterface*>     m_tabExtensionInterface; // Closed source injected
    //int                                 m_TabPluginCount;



public:
    ViewerWidget(QWidget* parent = 0, Qt::WindowFlags flag = 0, 
        QString sID = " ", 
        bool bAutoAdjustPacketSize = false, CameraPtr pCam = CameraPtr());
    ~ViewerWidget();

    bool        getCamOpenStatus() const;
    bool        isControlledCamera(const CameraPtr& cam) const;
    CameraPtr   getCameraPtr();
    VmbError_t  getOpenError() const;
    QString     getCameraID() const;
    bool        getAdjustPacketSizeMessage(QString& sMessage);
    QMenu*      getmContextMenu() const { return m_ContextMenu; }


protected:
    //virtual void closeEvent(QCloseEvent* event);

private:

    VmbError_t  releaseBuffer();
    void        checkDisplayInterval();
    bool        isStreamingAvailable();
    //void        changeEvent(QEvent* event);
    //bool        isDestPathWritable();
    //bool        checkUsedName(const QStringList& files);
    //template <typename T>
    //void        endianessConvert(T& v);
    //bool        CanReduceBpp();
    //QImage      ReduceBpp(QImage image);
    //bool        isSupportedPixelFormat();


public:
    //QAction* ActionFreerun;




private slots:
    
    /* when you use this std convention, you don't need any "connect..." */
    void on_ActionFreerun_triggered();
    void on_ActionSaveCameraSettings_triggered();
    void on_ActionLoadCameraSettings_triggered();
    void on_ActionSaveAs_triggered();
    //void on_ActionResetPosition_triggered();
    //void on_ActionHistogram_triggered();
    //void on_ActionOriginalSize_triggered();
    //void on_ActionSaveOptions_triggered();
    //void on_ActionSaveImages_triggered();
    //void on_ActionRegister_triggered();
    //void on_ActionDisplayOptions_triggered();
    //void on_ActionFitToWindow_triggered();
    //void on_ActionLeftRotation_triggered();
    //void on_ActionRightRotation_triggered();
    //void on_ActionZoomOut_triggered();
    //void on_ActionZoomIn_triggered();
    //void on_ActionLoadCameraSettingsMenu_triggered();
    //void on_ActionSaveCameraSettingsMenu_triggered();
    //void on_ActionAllow16BitTiffSaving_triggered();

    /* custom */
    void OnShowContextMenu(const QPoint& pt);
    void onAcquisitionStartStop(const QString& sThisFeature); //this is for the command from tree controller
    void onImageCalcStartStop(bool);

    void onSetDescription(const QString& sDesc);
    void onimageReadyFromCalc();
    void onFullBitDepthImageReady(tFrameInfo mFullImageInfo);
    
    void onSetEventMessage(const QStringList& sMsg);
    void onSetCurrentFPS(const QString& sFPS);
    void onSetFrameCounter(const unsigned int& nNumberOfFrames);
    void onFeedLogger(const QString& sMessage);
    void onResetFPS();
    VmbError_t onPrepareCapture();
    void textFilterChanged();
    void onSetMousePosInCMap(QMouseEvent* event);
    void SetCurrentScreenROI();
    void ResetFullROI(bool notStartReStart = false);
    void updateExposureTime();
    void updateCameraGain();
    //void onfloatingDockChanged(bool bIsFloating);
    //void onVisibilityChanged(bool bIsVisible);
    //void displayEveryFrameClick(bool bValue);
    //void onSaving(unsigned int nPos);
    //void getSaveDestinationPath();
    //void acceptSaveImagesDlg();
    //void rejectSaveImagesDlg();
    //void writeRegisterData();
    //void readRegisterData();
    //void endianessChanged(int);
    //void onSetColorInterpolation(const bool& bState);
    //void onSetHistogramData(const QVector<QVector <quint32> >& histData, const QString& sHistogramTitle,
    //    const double& nMaxHeight_YAxis, const double& nMaxWidth_XAxis, const QVector <QStringList>& statistics);
    //void updateColorInterpolationState();
    //void onTooltipCheckBoxClick(bool bValue);
    //void enableMenuAndToolbar(bool bValue);
    //void directAccessHexTextChanged(const QString& sText);
    //void directAccessDecTextChanged(const QString& sText);
    //void optionsFrameCountChanged(const QString& sText);
    //void optionsAccepted();
    //bool loadPlugin();  // Closed source injected
    //void onSetMousePosInScene(const QPointF& pPoint);


signals:
    void closeViewer(CameraPtr cam);
    void acquisitionRunning(bool bValue); // this is for the plugin, not needed here
};

