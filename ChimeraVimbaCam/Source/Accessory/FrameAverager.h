#pragma once
// created by Zhenpu Zhang
#include <atomic>
#include <deque>
#include <qvector.h>
#include <qobject.h>
#include <qmutex.h>
#include <array>


enum class avgType : unsigned
{
    normal = 0,
    exp = 1
};

class FrameAverager : public QObject
{
    Q_OBJECT
public:
	FrameAverager();
    void doAverage(QVector<double>& vec);
    std::string avgTypeName(avgType at);
    int currentAvgType();
    void setAvgType(int idx);
    int avgNumber();
    void setAvgNum(int num);

private:
    void normalAvg(QVector<double>& vec);
    void exponentialAvg(QVector<double>& vec);

signals:
    void avgNumberUpdate(QString avgNumStr);

public slots:
    void toggleDoAveraging(bool doAvg);


public:
    const std::array<avgType,2> allType = { avgType::normal, avgType::exp };
private:
    QMutex                                    m_mutex;
    std::atomic<bool>                         m_averaging;
    bool                                      m_avgfirstStart;
    QVector<double>                           m_avgResult;
    size_t                                    m_avgNum;
    avgType                                   m_avgType;

    std::deque<QVector<double>>               m_avgs; // for normal avg to store pics
    size_t                                    m_avgNumCntrs; // for exponential avg to store number of pics

    const double e = 2.71828182846;
};

