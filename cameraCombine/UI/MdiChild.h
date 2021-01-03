//Description: MDI Widget for loggingand event viewer

#ifndef MDICHILD_H
#define MDICHILD_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include "Helper.h"

class MdiChild : public QWidget
{
    Q_OBJECT
    
    public:

    protected:

    private:
        QString         m_sName;
        QTextEdit      *m_Logger;
        QPushButton    *m_StartStopBtn;
        QPushButton    *m_ClearBtn;

        bool            m_bIsPause;
    
    public:
        MdiChild(QString sTitle);
        ~MdiChild();

        QString getName  ( void );
        void setLogger ( const QStringList &sInfo, const VimbaViewerLogCategory &logCategory );
        void stopLogger  ( void );
        void clearLogger ( void );
        bool getPauseState ( void );

    protected:

    private:

    private slots:
        void onClearLogger ( void );
        void onPauseLogger ( void );
};

#endif
