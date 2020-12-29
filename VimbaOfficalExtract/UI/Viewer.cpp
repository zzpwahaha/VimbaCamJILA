/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        Viewer.cpp

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


#include <UI/Viewer.h>
#include <QMenu>
#include <QMouseEvent>
#include <QFileDialog>
#include <QScrollBar>


Viewer::Viewer ( QWidget *parent ): QGraphicsView ( parent ), m_ZoomFactor ( 0 ), m_ColorInterpolationAct ( NULL ), 
                                                              m_bColorInterpolationState ( true ), m_bIsFitToWindow ( false ),
                                                              m_bIsPressed ( false )
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);    
    setMouseTracking(true);
}

Viewer::~Viewer ( void )
{

}

void Viewer::enableFitToWindow ( const bool &bIsEnable )
{
    m_bIsFitToWindow = bIsEnable;
}

void Viewer::setDefaultSize ( void )
{
    resetCachedContent();
    resetMatrix();
    resetTransform();
    m_ZoomFactor = 0;
}

void Viewer::SetCenter ( const QPointF& centerPoint )
{     
    /* Get the rectangle of the visible area in scene coords */
    QRectF visibleArea = mapToScene(rect()).boundingRect();

    /* Get the scene area */
    QRectF sceneBounds = sceneRect();
 
    double boundX = visibleArea.width() / 2.0;
    double boundY = visibleArea.height() / 2.0;
    double boundWidth = sceneBounds.width() - 2.0 * boundX;
    double boundHeight = sceneBounds.height() - 2.0 * boundY;
 
    /* The max boundary that the centerPoint can be to */
    QRectF bounds(boundX, boundY, boundWidth, boundHeight);
 
    if(bounds.contains(centerPoint)) 
    {   
        /* We are within the bounds */
        m_CurrentCenterPoint = centerPoint;
    } 
    else 
    {  
        /* We need to clamp or use the center of the screen */
        if(visibleArea.contains(sceneBounds)) 
        { 
            /* Use the center of scene ie. we can see the whole scene */
            m_CurrentCenterPoint = sceneBounds.center();
        } 
        else 
        {
            m_CurrentCenterPoint = centerPoint;
 
            /* We need to clamp the center. The centerPoint is too large */
            if(centerPoint.x() > bounds.x() + bounds.width()) 
            {
                m_CurrentCenterPoint.setX(bounds.x() + bounds.width());
            } 
            else if(centerPoint.x() < bounds.x()) 
            {
                m_CurrentCenterPoint.setX(bounds.x());
            }
 
            if(centerPoint.y() > bounds.y() + bounds.height()) 
            {
                m_CurrentCenterPoint.setY(bounds.y() + bounds.height());
            } 
            else if(centerPoint.y() < bounds.y()) 
            {
                m_CurrentCenterPoint.setY(bounds.y());
            }
        }
    }
 
    /* Update the scrollbars */
    centerOn(m_CurrentCenterPoint);
}
 
void Viewer::saveImage ( void ) 
{
    emit savingImage();
}

void Viewer::colorInterpolationChecked ( bool bState )
{
  m_bColorInterpolationState = bState;    
  emit setColorInterpolationState (m_bColorInterpolationState);
}

void Viewer::updateInterpolationState ( const bool &bState )
{
    m_bColorInterpolationState = bState;    
}

/* Handles when the mouse button is pressed */
void Viewer::mousePressEvent ( QMouseEvent* event ) 
{ 
    m_bIsPressed = true; 
    Qt::MouseButton mouseBtn = event->button();
    if( Qt::RightButton == mouseBtn )
    {
        QMenu menu;
        menu.addAction(QIcon(":/VimbaViewer/Images/save.png"), tr("Save Image..."), this, SLOT(saveImage()));
        
        if(NULL != m_ColorInterpolationAct)
        {
            disconnect(m_ColorInterpolationAct, SIGNAL(toggled(bool)), this, SLOT(colorInterpolationChecked(bool)) );
            delete m_ColorInterpolationAct;
            m_ColorInterpolationAct = NULL;
        }

        m_ColorInterpolationAct = new QAction(this);
        m_ColorInterpolationAct->setObjectName(QString::fromUtf8("ColorInterpolation"));
        m_ColorInterpolationAct->setCheckable(true);
        m_ColorInterpolationAct->setText("Color Interpolation");
        m_ColorInterpolationAct->setChecked(m_bColorInterpolationState);
        
        menu.addAction(m_ColorInterpolationAct);
        connect(m_ColorInterpolationAct, SIGNAL(toggled(bool)), this, SLOT(colorInterpolationChecked(bool)) );

        menu.exec(event->globalPos());
    }
    else
    {
        /* For panning the view */
        m_LastPanPoint = event->pos();

        if(horizontalScrollBar()->isVisible() || verticalScrollBar()->isVisible())
            setCursor(Qt::ClosedHandCursor);
        else if((!horizontalScrollBar()->isVisible() && !verticalScrollBar()->isVisible()))
            setCursor(Qt::ArrowCursor);
    }
}
 
/* Handles when the mouse button is released */
void Viewer::mouseReleaseEvent ( QMouseEvent* event ) 
{
    m_bIsPressed = false;
    if(horizontalScrollBar()->isVisible() || verticalScrollBar()->isVisible())
        setCursor(Qt::OpenHandCursor);
    m_LastPanPoint = QPoint();
}
 
/* Handles the mouse move event */
void Viewer::mouseMoveEvent ( QMouseEvent* event ) 
{ 
    if((horizontalScrollBar()->isVisible() || verticalScrollBar()->isVisible()) && !m_bIsPressed)
        setCursor(Qt::OpenHandCursor);
    else if((!horizontalScrollBar()->isVisible() && !verticalScrollBar()->isVisible()))
        setCursor(Qt::ArrowCursor);

    if(!m_LastPanPoint.isNull()) 
    {
        /* Get how much we panned */
        QPointF delta = mapToScene(m_LastPanPoint) - mapToScene(event->pos());
        m_LastPanPoint = event->pos();
 
        /* Update the center e.g do the pan */
        QPointF F = GetCenter();
        SetCenter(GetCenter() + delta);
    }
}
 
/*  Zoom the view in and out. */
void Viewer::wheelEvent ( QWheelEvent* event ) 
{   
    0 < event->delta() ? zoomIn() : zoomOut();
}
 
/**
  * Need to update the center so there is no jolt in the
  * interaction after resizing the widget.
  */
void Viewer::resizeEvent ( QResizeEvent* event ) 
{    
    /* Call the subclass resize so the scrollbars are updated correctly */
    if(m_bIsFitToWindow)
    {
        fitInView(scene()->itemsBoundingRect(), Qt::IgnoreAspectRatio);            
    }

    QGraphicsView::resizeEvent(event);
}

QPointF Viewer::GetCenter ( void )
{
    return m_CurrentCenterPoint;
}

void Viewer::zoomIn ( void ) 
{   
    /* Get the position of the mouse before scaling, in scene coords */
    QPointF pointBeforeScale(mapToScene(pos()));
    /* Get the original screen center point */
    QPointF screenCenter = GetCenter();
    /* How fast we zoom */
    double scaleFactor = 1.15; 
    scale(scaleFactor, scaleFactor);
    m_ZoomFactor++;
    updateCenter(pointBeforeScale, screenCenter);
}

void Viewer::zoomOut ( void ) 
{
    /* Get the position of the mouse before scaling, in scene coords */
    QPointF pointBeforeScale(mapToScene(pos()));
    /* Get the original screen center point */
    QPointF screenCenter = GetCenter();
    /* How fast we zoom */
    double scaleFactor = 1.15; 
    scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    m_ZoomFactor--;
    updateCenter(pointBeforeScale, screenCenter);
}

void Viewer::updateCenter ( QPointF pointBeforeScale, QPointF screenCenter ) 
{
    /* Get the position after scaling, in scene coords */
    QPointF pointAfterScale(mapToScene(pos()));

    /* Get the offset of how the screen moved */
    QPointF offset = pointBeforeScale - pointAfterScale;

    /* Adjust to the new center for correct zooming */
    QPointF newCenter = screenCenter + offset;
    SetCenter(newCenter);
}
