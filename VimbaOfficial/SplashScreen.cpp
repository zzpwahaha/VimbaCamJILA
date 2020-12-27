/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        SplashScreen.cpp

  Description: Splash window that will be used when camera list has been changed or when camera open
               

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


#include <QPainter>
#include "SplashScreen.h"
#include <QStyle>

SplashScreen::SplashScreen ( const QPixmap& thePixmap, QWidget *parent, Qt::WindowFlags f )
 : QFrame ( parent, Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint ), m_Pixmap ( thePixmap )
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(m_Pixmap.size());
};
  
void SplashScreen::clearMessage ( void )
{
    m_sMessage.clear();
    repaint();
}
 
void SplashScreen::showMessage ( const QString& theMessage, int theAlignment/* = Qt::AlignLeft*/, const QColor& theColor/* = Qt::black*/ )
{
    m_sMessage   = theMessage;
    m_nAlignment = theAlignment;
    m_Color         = theColor;
    repaint();
}
 
void SplashScreen::paintEvent ( QPaintEvent* event )
{
    QRect aTextRect(rect());
    aTextRect.setRect(aTextRect.x() + 5, aTextRect.y() + 5, aTextRect.width() - 10, aTextRect.height() - 10);

    QPainter aPainter(this);

    QFont splashFont;
    splashFont.setFamily( "Verdana" );
    splashFont.setBold( true );
    splashFont.setPixelSize( 14 );
    aPainter.setFont(splashFont);

    aPainter.drawPixmap(rect(), m_Pixmap);
    
    aPainter.setPen(m_Color);
    aPainter.drawText(aTextRect, m_nAlignment, m_sMessage);
}