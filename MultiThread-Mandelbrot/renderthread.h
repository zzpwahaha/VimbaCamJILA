#pragma once

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <QImage>

class RenderThread : public QThread
{
	Q_OBJECT

public:
	RenderThread(QObject* parent = nullptr);
	~RenderThread();
	void render(double centerX, double centerY, 
		double scaleFactor, QSize resultSize, double devicePixelRatio);

signals:
	void renderImage(const QImage& image, double scaleFactor);

protected:
	void run() override;

private:
	static uint rgbFromWaveLength(double wave);
	QWaitCondition condition;
	QMutex mutex;
	

	double centerX;
	double centerY;
	double scaleFactor;
	double devicePixelRatio;
	QSize resultSize;
	bool restart = false;
	bool abort = false;

	enum { ColormapSize = 512 };
	uint colormap[ColormapSize];

};

