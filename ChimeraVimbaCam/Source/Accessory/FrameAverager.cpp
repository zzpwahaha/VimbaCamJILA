#include "stdafx.h"
#include "FrameAverager.h"
#include <cmath>
#include <algorithm>



FrameAverager::FrameAverager()
	: m_averaging(false)
	, m_avgfirstStart(false)
    , m_avgNum(50)
    , m_avgType(avgType::exp)
{
}

void FrameAverager::toggleDoAveraging(bool doAvg)
{
    m_averaging = doAvg;
    if (doAvg) {
        m_avgfirstStart = true;
    }
}

void FrameAverager::doAverage(QVector<double>& vec)
{
    QMutexLocker locker(&m_mutex);
    if (m_averaging) {
        switch (m_avgType) {
        case avgType::normal:
            normalAvg(vec);
            break;
        case avgType::exp:
            exponentialAvg(vec);
            break;
        default:
            normalAvg(vec);
        }
    }
}

std::string FrameAverager::avgTypeName(avgType at)
{
    std::string name;
    switch (at) {
    case avgType::normal:
        name =  "Normal";
        break;
    case avgType::exp:
        name = "Exponential";
        break;
    }
    return name;
}

int FrameAverager::currentAvgType()
{
    return std::distance(allType.begin(), std::find(allType.begin(), allType.end(), m_avgType));
}

void FrameAverager::setAvgType(int idx)
{
    if (m_avgType != allType[idx]) {
        QMutexLocker locker(&m_mutex); //restart the averaging 
        m_avgType = allType[idx];
        m_avgfirstStart = true;
    }
}

int FrameAverager::avgNumber()
{
    return m_avgNum;
}

void FrameAverager::setAvgNum(int num)
{
    if (m_avgNum != num) {
        QMutexLocker locker(&m_mutex); //restart the averaging 
        m_avgNum = num;
        m_avgfirstStart = true;
    }
}

void FrameAverager::normalAvg(QVector<double>& vec)
{
    if (m_avgfirstStart) {
        m_avgResult = QVector<double>(vec.size(), 0);
        m_avgs.clear();
        m_avgfirstStart = false;
    }
    for (unsigned idx = 0; idx < m_avgResult.size(); idx++) {
        m_avgResult[idx] = m_avgResult[idx] * m_avgs.size() + vec[idx]
            - (m_avgs.size() == m_avgNum ? m_avgs.front()[idx] : 0);
        m_avgResult[idx] /= (m_avgs.size() == m_avgNum ? m_avgs.size() : (m_avgs.size() + 1));
    }

    if (m_avgs.size() == m_avgNum) {
        m_avgs.pop_front();
    }
    m_avgs.push_back(vec);
    emit avgNumberUpdate(qstr(m_avgs.size()) + "/" + qstr(m_avgNum));
    vec = m_avgResult;
}

void FrameAverager::exponentialAvg(QVector<double>& vec)
{
    if (m_avgfirstStart) {
        m_avgResult = vec;
        m_avgNumCntrs = 0;
        m_avgfirstStart = false;

        m_avgNumCntrs++;
        return;
    }
    const double base = std::pow(e, 1 / static_cast<double>(m_avgNum));
    for (unsigned idx = 0; idx < m_avgResult.size(); idx++) {
        m_avgResult[idx] = m_avgResult[idx] + vec[idx] * ((1 - std::pow(base, -double(m_avgNumCntrs))) / (base - 1));
        m_avgResult[idx] /= (base - std::pow(base, -double(m_avgNumCntrs))) / (base - 1);
    }
    m_avgNumCntrs++;
    emit avgNumberUpdate(qstr(m_avgNumCntrs % m_avgNum) + "/" + qstr(m_avgNum));
    vec = m_avgResult;
}
