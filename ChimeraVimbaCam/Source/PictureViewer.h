#pragma once
#include <qwidget.h>
#include <qdialog.h>
#include <qstring.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qmenu.h>
#include "../ExternLib/qcustomplot/qcustomplot.h"
#include <RangeSlider.h>
class PictureViewer : public QWidget
{
    Q_OBJECT


public:
    PictureViewer(std::string plotname, QWidget* parent = nullptr);
    ~PictureViewer();

    // the mouse pos info is set to label
    void onSetMousePosInCMap(QMouseEvent* event, QLabel* label);
    void initManualColorRangeAction(QAction* mcbarRange);
    void handleContextMenu();
    void loadColorCsv();
    void renderImgFromCalcThread(bool manualColorScale);

    QMenu* contextMenu() { return m_ContextMenu; }
    QCustomPlot* plot() { return m_QCP.get(); }
    QCPColorMap* cmap() { return m_colorMap.get(); }
    QCPAxisRect* centerAxes() { return m_QCPcenterAxisRect; }
    QCPAxisRect* leftAxes() { return m_QCPleftAxisRect; }
    QCPAxisRect* bottomAxes() { return m_QCPbottomAxisRect; }
    QCPGraph* bottomPlot() { return m_bottomGraph.get(); }
    QCPGraph* leftPlot() { return m_leftGraph.get(); }
    QCPItemTracer* bottomTracer() { return m_QCPtracerbottom; }
    QCPItemTracer* leftTracer() { return m_QCPtracerleft; }
    QCPItemText* bottomTracerText() { return m_QCPtraceTextbottom; }
    QCPItemText* leftTracerText() { return m_QCPtraceTextleft; }

    RangeSliderIntg* rangeSlider() { return intgSlider; }

    QDialog* manualColorScaleDlg() { return m_dCScale; }
private:
    //QDialog*                            m_DiagController;
    //QDialog*                            m_DiagInfomation;
    QDialog*                            m_DiagRSlider;
    RangeSliderIntg*                    intgSlider;
    //RangeSlider*                        m_RSliderV;
    //QSpinBox*                           m_upperSB;
    //QSpinBox*                           m_lowerSB;
    //QString                             m_sSliderFormat;


    //QLabel*                             m_OperatingStatusLabel;
    //QPushButton*                        m_FormatButton;
    //QPushButton*                        m_ImageSizeButtonH;
    //QPushButton*                        m_ImageSizeButtonW;
    //QPushButton*                        m_FramerateButton;
    //QLabel*                             m_FramesLabel;
    //QLabel*                             m_CursorScenePosLabel;
    //QPushButton*                        m_ExposureTimeButton;
    //QPushButton*                        m_CameraGainButton;

    QSharedPointer<QCustomPlot>         m_QCP;
    QCPAxisRect*                        m_QCPcenterAxisRect;
    QCPAxisRect*                        m_QCPbottomAxisRect;
    QCPAxisRect*                        m_QCPleftAxisRect;
    QSharedPointer<QCPColorMap>         m_colorMap;
    QSharedPointer<QCPGraph>            m_bottomGraph;
    QSharedPointer<QCPGraph>            m_leftGraph;
    QCPColorScale*                      m_colorScale;
    QVector<QCPColorGradient>           m_colorgradient;
    QMap<int, QString>                  m_cmapMap;
    QComboBox*                          m_cmapCombo;
    QDialog*                            m_dCScale;

    QCPItemTracer*                      m_QCPtracerbottom;
    QCPItemText*                        m_QCPtraceTextbottom;
    QCPItemTracer*                      m_QCPtracerleft;
    QCPItemText*                        m_QCPtraceTextleft;

    QMenu*                              m_ContextMenu;
    //QAction*                            m_aStartStopCap;
    //QAction*                            m_aDiagCtrler;
    //QAction*                            m_aDiagInfo;
    //QAction*                            m_aSetCurrScrROI;
    //QAction*                            m_aResetFullROI;
    //QAction*                            m_aPlotTracer;
    //QAction*                            m_aPlotFitter;
    //QAction*                            m_aPlotFitter2D;
    //QAction*                            m_aCscale;
    //QAction*                            m_aManualCscale;
    //QAction*                            m_aSaveCamSetting;
    //QAction*                            m_aLoadCamSetting;
    //QAction*                            m_aSaveImg;
    //QAction*                            m_aCamlist;
    //QAction*                            m_aDisconnect;

    ///* Save Image Option */
    //QString                             m_SaveFileDir;
    //QString                             m_SelectedExtension;
    //QFileDialog*                        m_saveFileDialog; // save an image

    //unsigned int                        m_FrameBufferCount;

};

