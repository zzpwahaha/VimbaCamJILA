#pragma once
#include <QThread>
#include <VimbaCPP/Include/VimbaSystem.h>
#include "FrameObserver.h"

#include "ExternLib/qcustomplot/qcustomplot.h"

class ImageCalculatingThread :
    public QThread
{
    Q_OBJECT
private:
    SP_DECL(FrameObserver)                    m_pFrameObs;
    CameraPtr                                 m_pCam;
    QSharedPointer<ImageProcessingThread>     m_pProcessingThread;
    QSharedPointer<QCustomPlot>               m_pQCP;
    QSharedPointer<QCPColorMap>               m_pQCPColormap;
    QVector<ushort>                           m_uint16QVector;
    int                                       m_height;
    int                                       m_width;
    int                                       m_heightMax;
    int                                       m_widthMax;
    int                                       m_offsetX;
    int                                       m_offsetY;
    QString                                   m_format;
    bool                                      m_firstStart;

    bool                                      m_Stopping;

    QPoint                                    m_mousePos;
public:
    ImageCalculatingThread(const SP_DECL(FrameObserver)& ,
        const CameraPtr&,
        const QSharedPointer<QCustomPlot>&,
        const QSharedPointer<QCPColorMap>&);
    void StartProcessing();
    void StopProcessing();
    virtual void run() override;

    const QPoint& mousePos() const { return m_mousePos; }
    std::pair<int, int> maxWidthHeight() const { return std::pair(m_widthMax, m_heightMax); }
    std::pair<int, int> WidthHeight() const { return std::pair(m_width, m_height); }
    const QString& format() const { return m_format; }
    void updateXYOffset();
    QMutex& mutex() const { return m_pProcessingThread->mutex(); }

signals:
    void imageReadyForPlot();
    void logging(const QString& sMessage);
public slots:
    void assignValue(const QVector<ushort>& vec1d, const QString& sFormat, const int& Height, const int& Width, const int& offsetX, const int& offsetY);
    void updateMousePos(QMouseEvent* event);
};

