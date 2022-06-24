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
    QCPColorMap* cmap() { return m_colorMap; }
    QCPAxisRect* centerAxes() { return m_QCPcenterAxisRect; }
    QCPAxisRect* leftAxes() { return m_QCPleftAxisRect; }
    QCPAxisRect* bottomAxes() { return m_QCPbottomAxisRect; }
    QCPGraph* bottomPlot() { return m_bottomGraph; }
    QCPGraph* leftPlot() { return m_leftGraph; }
    QCPItemTracer* bottomTracer() { return m_QCPtracerbottom; }
    QCPItemTracer* leftTracer() { return m_QCPtracerleft; }
    QCPItemText* bottomTracerText() { return m_QCPtraceTextbottom; }
    QCPItemText* leftTracerText() { return m_QCPtraceTextleft; }

    RangeSliderIntg* rangeSlider() { return intgSlider; }

    QDialog* manualColorScaleDlg() { return m_dCScale; }
private:
    QDialog*                            m_DiagRSlider;
    RangeSliderIntg*                    intgSlider;

    QSharedPointer<QCustomPlot>         m_QCP;
    QCPAxisRect*                        m_QCPcenterAxisRect;
    QCPAxisRect*                        m_QCPbottomAxisRect;
    QCPAxisRect*                        m_QCPleftAxisRect;
    QCPColorMap*         m_colorMap;
    QCPGraph*            m_bottomGraph;
    QCPGraph*            m_leftGraph;
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


};

