/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        MdiChild.cpp

  Description: MDI Widget for logging and event viewer

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


#include <QtGui>
#include "MdiChild.h"
#include <QtWidgets>

MdiChild::MdiChild(QString sTitle) : m_bIsPause (false)
{
    m_sName = sTitle;
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(sTitle);

    QVBoxLayout *verticalLayout_Parent = new QVBoxLayout(this);
    verticalLayout_Parent->setObjectName("verticalLayout_Parent");

    QVBoxLayout *verticalLayout_Child = new QVBoxLayout();
    verticalLayout_Child->setObjectName("verticalLayout_Child");

    m_Logger = new QTextEdit(this);
    m_Logger->setObjectName("m_Logger");
    m_Logger->setReadOnly(true);
    m_Logger->setLineWrapMode(QTextEdit::NoWrap); 
    m_Logger->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);\n" "color: rgb(255, 255, 255);"));
    verticalLayout_Child->addWidget(m_Logger);
    verticalLayout_Parent->addLayout(verticalLayout_Child);

    /* if its an Event Viewer window, then add these two buttons on widget */
    if(0 == sTitle.compare("Event Viewer"))
    {
        QHBoxLayout *horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        m_StartStopBtn = new QPushButton(this);
        m_StartStopBtn->setObjectName(QString::fromUtf8("m_StartStopBtn"));
        
        horizontalLayout->addWidget(m_StartStopBtn);

        m_ClearBtn = new QPushButton(this);
        m_ClearBtn->setObjectName(QString::fromUtf8("m_ClearBtn"));
        
        horizontalLayout->addWidget(m_ClearBtn);
        m_StartStopBtn->setText("Pause");
        m_ClearBtn->setText("Clear");

        QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        verticalLayout_Parent->addLayout(horizontalLayout);

        connect(m_ClearBtn,     SIGNAL(clicked()), this, SLOT(onClearLogger()));
        connect(m_StartStopBtn, SIGNAL(clicked()), this, SLOT(onPauseLogger()));    
    }
}

MdiChild::~MdiChild()
{

}

void MdiChild::onClearLogger ( )
{
    m_Logger->clear();
}

void MdiChild::onPauseLogger ( )
{
    if(!m_bIsPause)
    {
        m_bIsPause = true;
        m_StartStopBtn->setText("Resume");
    }
    else
    {
        m_bIsPause = false;
        m_StartStopBtn->setText("Pause");
    }

}

bool MdiChild::getPauseState ( void )
{
    return m_bIsPause;
}

void MdiChild::setLogger ( const QStringList &sInfo, const VimbaViewerLogCategory &logCategory )
{
    if(!m_bIsPause)
    {
        if(VimbaViewerLogCategory_INFO == logCategory)
        {
            m_Logger->append(sInfo.join("\n"));
            QTextCursor c =  m_Logger->textCursor();
            c.movePosition(QTextCursor::End);
            m_Logger->setTextCursor(c);
            return;
        }
    }
    
    QString sInformation = "<br>" + sInfo.at(0);
    QString sErrorHtml = "<font color=\"Red\">";
    QString sOKHtml = "<font color=\"lime\">";
    QString sWarningHtml = "<font color=\"Yellow\">";
    QString sInfoHtml = "<font color=\"White\">";
    QString sEndHtml = "</font>";
    
    QTextCursor cursorText = m_Logger->textCursor();

    if(!m_bIsPause)
    {
        switch(logCategory)
        {
            case VimbaViewerLogCategory_OK: 
                sInformation = sOKHtml % sInformation;
                break;

            case VimbaViewerLogCategory_WARNING: 
                sInformation = sWarningHtml % sInformation;
                break;

            case VimbaViewerLogCategory_ERROR: 
                sInformation = sErrorHtml % sInformation;
                break;

            default: 
                sInformation = sInfoHtml % sInformation;
                break;
        }

        sInformation = sInformation % sEndHtml;
        m_Logger->insertHtml(sInformation);
        cursorText.movePosition(QTextCursor::End);
        m_Logger->setTextCursor(cursorText);        
    }
}

void MdiChild::stopLogger( void )
{
    m_bIsPause = true;
}

void MdiChild::clearLogger( void )
{
    onClearLogger ();
}

QString MdiChild::getName( void )
{
    return m_sName;
}
