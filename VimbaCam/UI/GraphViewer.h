#pragma once
#include <QGraphicsView>

class GraphViewer :
    public QGraphicsView
{
    Q_OBJECT


private:
    QPoint m_mouseScenePoint;
public:
    GraphViewer(QWidget* parent = NULL);
    ~GraphViewer(void);
    const QPoint& getMouseScenePos() const;
    //virtual void mousePressEvent(QMouseEvent* event);
    //virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    //virtual void resizeEvent(QResizeEvent* event);

signals:
    void mousePositionInScene(const QPointF& pPoint);
    
};

