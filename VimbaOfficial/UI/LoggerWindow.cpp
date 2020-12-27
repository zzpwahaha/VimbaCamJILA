/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        LoggerWindow.cpp

  Description: Logging widget used by the MainWindow only.

-------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/


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
