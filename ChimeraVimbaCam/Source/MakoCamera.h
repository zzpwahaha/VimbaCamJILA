#pragma once
#include "Accessory/IChimeraSystem.h"
//#include "ConfigurationSystems/ConfigStream.h"
#include <QLabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <array>
#include "MakoCameraCore.h"
#include "ImageCalculatingThread.h"
#include <PictureViewer.h>

class cameraMainWindow;
class MakoCamera : public IChimeraSystem 
{
    Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	MakoCamera& operator=(const MakoCamera&) = delete;
	MakoCamera(const MakoCamera&) = delete;
	
	MakoCamera(CameraInfo camInfo, cameraMainWindow* parent);
    ~MakoCamera();

    void initialize();

	void releaseBuffer();

	void acquisitionStartStopFromCtrler(const QString& sThisFeature);

    void acquisitionStartStopFromAction();

    void setCurrentScreenROI();

    void resetFullROI(bool notStartReStart = false);

    void manualSaveImage();

    void updateStatusBar();

    //void prepareForExperiment();

    void setMOTCalcActive(bool active);

    MakoCameraCore& getMakoCore() { return core; };
    const CameraInfo& getCameraInfo() { return camInfo; }
    bool isExpStillRunning() { return isExpRunning; } // used in MakoWindow to verify all cam is finished writing data to hdf5



    QString getCameraID() { return qstr(core.CameraName()); }
    QMenu* getmContextMenu() { return viewer.contextMenu(); };

public slots:
    void handleExpImage(QVector<double> img, int width, int height);
    void startExp();
    void finishExp();

signals:
    void imgReadyForAnalysis(QVector<double> img, int width, int height, size_t currentNum);

private:
    void initPlotContextMenu();
    void handleStatusButtonClicked(QString featName);
    void createAvgControlWidget();
    void createRepSaveControlWidget();
    void repSave();

private:
    // this order matters since the ctor will initialize core first and then viewer and finally imgCThread
    MakoCameraCore core;
	PictureViewer viewer;
    ImageCalculatingThread imgCThread;
    CameraInfo camInfo;

    QDialog* makoCtrlDialog;
    QFileDialog* saveFileDialog;

	bool isCamRunning = false;

    unsigned int currentRepNumber;
    bool isExpRunning = false;
    bool MOTCalcActive = false; // will be initialized in Analysis window before every experiment

    QCheckBox*                          m_expActive;
    QSpinBox*                           m_picsPerRep;
	QLabel*                             m_OperatingStatusLabel;
    QPushButton*                        m_ImageSizeButtonX;
    QPushButton*                        m_ImageSizeButtonY;
    QPushButton*                        m_ImageSizeButtonH;
    QPushButton*                        m_ImageSizeButtonW;
    QPushButton*                        m_ImageBinButtonH;
    QPushButton*                        m_ImageBinButtonW;
    //QPushButton*                        m_FormatButton;
    QPushButton*                        m_TrigOnOffButton;
    QPushButton*                        m_TrigSourceButton;
    //QPushButton*                        m_FramerateButton;
    //QLabel*                             m_FramesLabel;
    QPushButton*                        m_avgSetButton;
    QLabel*                             m_CursorScenePosLabel;
    QLabel*                             m_DataMaxMinLabel;
    QPushButton*                        m_ExposureTimeButton;
    QPushButton*                        m_CameraGainButton;

	QAction*                            m_aStartStopCap;
    QAction*                            m_aManualCscale;
    //QString                             currentFormat;

    double                              m_repSaveTime;
    QString                             m_repSavePath;
    QTimer*                             m_repSaveTimer;
    size_t                              m_repSaveCnter;

};

Q_DECLARE_METATYPE(size_t);