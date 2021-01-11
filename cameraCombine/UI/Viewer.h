//Description: - Displaying data from the camera in RGB32 format.
//             - Scrolling, Rotating, Saving single image (saving not yet implemented)





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
            void zoomIn     (QWheelEvent* event);
            void zoomOut    (QWheelEvent* event);

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