#pragma once
#include <QtWidgets>
#include <QString>



#include "UI/Viewer.h"
#include <VimbaCPP/Include/VimbaSystem.h>
#include "FrameObserver.h"
#include "ILogTarget.h"
//#include "UI/DockWidgetWindow.h"
#include "UI/ControllerTreeWindow.h"
#include "UI/MainInformationWindow.h"
#include "UI/LineEditCompleter.h"
#include "UI/SortFilterProxyModel.h"

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
    QAction*                            m_ResetPosition;
    QCheckBox*                          m_ToolTipCheckBox;

    QDialog*                            m_DiagController;
    QDialog*                            m_DiagInfomation;
    //DockWidgetWindow* m_DockInformation;
    //DockWidgetWindow* m_DockHistogram;

    Viewer*                             m_ScreenViewer;
    MainInformationWindow*              m_InformationWindow;
    ControllerTreeWindow*               m_Controller;
    //HistogramWindow* m_HistogramWindow;

    CameraPtr                           m_pCam;
    QTextEdit*                          m_Description;

    QLabel*                             m_OperatingStatusLabel;
    QLabel*                             m_FormatLabel;
    QLabel*                             m_ImageSizeLabel;
    QLabel*                             m_FramerateLabel;
    QLabel*                             m_FramesLabel;

    QSharedPointer<QGraphicsScene>      m_pScene;
    QGraphicsPixmapItem*                m_PixmapItem;
    QGraphicsTextItem*                  m_TextItem;
    QMenu*                              m_ContextMenu;
    QAction*                            m_aStartStopCap;
    QAction*                            m_aDiagCtrler;
    QAction*                            m_aDiagInfo;
    QAction*                            m_aCamlist;
    QAction*                            m_aDisconnect;

    SP_DECL(FrameObserver)              m_pFrameObs;

    //QString                             m_SaveFileDir;
    //QString                             m_SelectedExtension;
    //QString                             m_SaveImageName;
    ///* Save Image Option */
    //Ui::SavingOptionsDialog             m_SaveImageOption;
    //QDialog* m_ImageOptionDialog;
    //QFileDialog* m_getDirDialog; // multiple images
    //QFileDialog* m_saveFileDialog; // save an image

    /* Direct Access */
    //Ui::DirectAccessDialog              m_DirectAccess;
    //QDialog* m_AccessDialog;

    /*Viewer Option */
    //Ui::DisplayOptionsDialog            m_ViewerOption;
    //QDialog* m_ViewerOptionDialog;
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

    //virtual void ViewerWidget::contextMenuEvent(QContextMenuEvent* event)
    //{
    //    QMenu contextMenu("Context menu", this);
    //    QAction action1("Remove Data Point", &contextMenu);
    //    //contextMenu.exec(QCursor::pos());
    //    contextMenu.exec(event->globalPos());
    //    //menu.exec(event->globalPos());
    //}

protected:
    //virtual void closeEvent(QCloseEvent* event);

private:

    VmbError_t  releaseBuffer();
    void        checkDisplayInterval();
    //void        changeEvent(QEvent* event);
    //bool        isDestPathWritable();
    //bool        checkUsedName(const QStringList& files);
    //template <typename T>
    //void        endianessConvert(T& v);
    //bool        CanReduceBpp();
    //QImage      ReduceBpp(QImage image);
    bool        isStreamingAvailable();
    //bool        isSupportedPixelFormat();


public:
    //QAction* ActionFreerun;




private slots:
    void OnShowContextMenu(const QPoint& pt);
    /* when you use this std convention, you don't need any "connect..." */
    void on_ActionFreerun_triggered();
    //void on_ActionResetPosition_triggered();
    //void on_ActionHistogram_triggered();
    //void on_ActionOriginalSize_triggered();
    //void on_ActionSaveAs_triggered();
    //void on_ActionSaveOptions_triggered();
    //void on_ActionSaveImages_triggered();
    //void on_ActionRegister_triggered();
    //void on_ActionDisplayOptions_triggered();
    //void on_ActionFitToWindow_triggered();
    //void on_ActionLeftRotation_triggered();
    //void on_ActionRightRotation_triggered();
    //void on_ActionZoomOut_triggered();
    //void on_ActionZoomIn_triggered();
    //void on_ActionLoadCameraSettings_triggered();
    //void on_ActionSaveCameraSettings_triggered();
    //void on_ActionLoadCameraSettingsMenu_triggered();
    //void on_ActionSaveCameraSettingsMenu_triggered();
    //void on_ActionAllow16BitTiffSaving_triggered();

    /* custom */
    void onAcquisitionStartStop(const QString& sThisFeature);
    //void onfloatingDockChanged(bool bIsFloating);
    //void onVisibilityChanged(bool bIsVisible);
    //void displayEveryFrameClick(bool bValue);
    void onSetDescription(const QString& sDesc);
    void onimageReady(QImage image, const QString& sFormat, const QString& sHeight, const QString& sWidth);
    void onFullBitDepthImageReady(tFrameInfo mFullImageInfo);
    //void onSaving(unsigned int nPos);
    void onSetEventMessage(const QStringList& sMsg);
    void onSetCurrentFPS(const QString& sFPS);
    void onSetFrameCounter(const unsigned int& nNumberOfFrames);
    void onFeedLogger(const QString& sMessage);
    void onResetFPS();
    //void getSaveDestinationPath();
    //void acceptSaveImagesDlg();
    //void rejectSaveImagesDlg();
    //void writeRegisterData();
    //void readRegisterData();
    //void endianessChanged(int);
    VmbError_t onPrepareCapture();
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
    void textFilterChanged();
    //bool loadPlugin();  // Closed source injected


signals:
    //void closeViewer(CameraPtr cam);
    void acquisitionRunning(bool bValue);
};

