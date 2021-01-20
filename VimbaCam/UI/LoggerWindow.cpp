
#include "LoggerWindow.h"
#include <QTime>
#include <QPainter>


LoggerWindow::LoggerWindow ( QWidget *parent ): QListWidget ( parent )
{
        
}

LoggerWindow::~LoggerWindow ( void )
{

}

void LoggerWindow::logging ( const QString &sInfo, const VimbaViewerLogCategory &logCategory )
{
    QIcon icon;

    QListWidgetItem *item = new QListWidgetItem(QTime::currentTime().toString("hh:mm:ss.zzz")+"   "+ sInfo);

    switch(logCategory)
    {
        case VimbaViewerLogCategory_OK: 
            icon.addFile(QString::fromUtf8(":/VimbaViewer/Images/okay.png"), QSize(), QIcon::Normal, QIcon::Off);
            break;

        case VimbaViewerLogCategory_WARNING:
            icon.addFile(QString::fromUtf8(":/VimbaViewer/Images/warning.png"), QSize(), QIcon::Normal, QIcon::Off);
            break;

        case VimbaViewerLogCategory_ERROR:
            icon.addFile( QString::fromUtf8(":/VimbaViewer/Images/error.png"), QSize(), QIcon::Normal, QIcon::Off);
            break;

        case VimbaViewerLogCategory_INFO:
            icon.addFile( QString::fromUtf8(":/VimbaViewer/Images/info.png"), QSize(), QIcon::Normal, QIcon::Off);
            break;

        default: break;
    }
    
    item->setIcon(icon);
    this->addItem(item);
    this->scrollToBottom();
}

void LoggerWindow::plainLogging ( const QString &sInfo )
{
    QListWidgetItem *item = new QListWidgetItem("   "+sInfo);
    this->addItem(new QListWidgetItem(""));
    this->addItem(item);
    this->scrollToBottom();
}
