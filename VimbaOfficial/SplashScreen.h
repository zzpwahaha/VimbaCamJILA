/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        SplashScreen.h

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


#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H
 
#include <QFrame>
 
class SplashScreen : public QFrame
{
public:
        SplashScreen ( const QPixmap& pixmap, QWidget *parent = 0, Qt::WindowFlags f= Qt::SplashScreen );
        ~SplashScreen ( void ){};

        void clearMessage       ( void );
        void showMessage        ( const QString& theMessage, int theAlignment = Qt::AlignLeft, const QColor& theColor = Qt::black );
 
private:
        virtual void paintEvent ( QPaintEvent* event );
 
        int         m_nAlignment;
        QPixmap     m_Pixmap;
        QString     m_sMessage;
        QColor      m_Color;
};
 

#endif