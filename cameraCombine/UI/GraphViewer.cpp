#include "UI/GraphViewer.h"

#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>

GraphViewer::GraphViewer(QWidget* parent) 
    : QGraphicsView(parent)
    , m_mouseScenePoint(0,0)
{
    /* We dont need to register QPointF because it is already known to Qt's meta-object system */
    /*qRegisterMetaType<QPointF>("QPointF");*/
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setMouseTracking(true);
    setDragMode(QGraphicsView::ScrollHandDrag);

}

GraphViewer::~GraphViewer()
{

}

void GraphViewer::wheelEvent(QWheelEvent* event)
{
    double zoomFactor = 1.0;
    zoomFactor *= 0 < event->angleDelta().y() ? 1.15 : 1.0/1.15;
    /*QPointF pointBeforeScale(mapToScene(event->pos()));*/
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scale(zoomFactor, zoomFactor);
    /*stuffs when AnchorUnderMouse does not work*/
    /*QPointF pointAfterScale(mapToScene(event->pos()));
    auto delta = pointAfterScale - pointBeforeScale;
    translate(delta.x(), delta.y());*/
}

void GraphViewer::mouseMoveEvent(QMouseEvent* event)
{
    m_mouseScenePoint = mapToScene(event->pos()).toPoint();
    emit mousePositionInScene(mapToScene(event->pos()));
    /*qDebug() << event->pos() << mapToScene(event->pos()) << mapToGlobal(event->pos()) << mapToParent(event->pos());*/
    QGraphicsView::mouseMoveEvent(event);
}

const QPoint& GraphViewer::getMouseScenePos() const
{
    return m_mouseScenePoint;
}