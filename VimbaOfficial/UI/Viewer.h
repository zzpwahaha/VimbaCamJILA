/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        Viewer.h

  Description: - Displaying data from the camera in RGB32 format.
               - Scrolling, Rotating, Saving single image

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


#ifndef VIEWER_H
#define VIEWER_H

#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QtCore/QTextStream>
#include "Helper.h"

class Viewer : public QGraphicsView
{
    Q_OBJECT

    public: 
            QAction            *m_ColorInterpolationAct;

    protected:

    private:
            QGraphicsScene     *m_Scene;
            QGraphicsView      *m_View;

            double              m_ZoomFactor;
            bool                m_bColorInterpolationState;
            bool                m_bIsFitToWindow;
            bool                m_bIsPressed;

    public:
            Viewer ( QWidget *parent = NULL);
           ~Viewer ( void );

            void enableFitToWindow ( const bool &bIsEnable );
            void setDefaultSize ( void );
            void updateInterpolationState ( const bool &bState );
            void zoomIn  ( void ) ;
            void zoomOut ( void ) ;

    protected:
            QPointF             m_CurrentCenterPoint;

            /* From panning the view */
            QPoint              m_LastPanPoint;

            /* Set the current center point in the */
            void SetCenter ( const QPointF& centerPoint );
            QPointF GetCenter ( void );
            void updateCenter ( QPointF pointBeforeScale, QPointF screenCenter );

            /* Take over the interaction */
            virtual void mousePressEvent      ( QMouseEvent* event );
            virtual void mouseReleaseEvent    ( QMouseEvent* event );
            virtual void mouseMoveEvent       ( QMouseEvent* event );
            virtual void wheelEvent           ( QWheelEvent* event );
            virtual void resizeEvent          ( QResizeEvent* event);

    private:
               
    private slots:
            void saveImage ( void );
            void colorInterpolationChecked ( bool bState );

    signals:
            void savingImage ( void );
            void setColorInterpolationState ( const bool &bState);    
};

#endif