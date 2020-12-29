#pragma once


#ifndef CONTROLLERVIEWER_H
#define CONTROLLERVIEWER_H

//TODO sort the header you don´t need
#include <QtGui>
#include <QtWidgets>



class ControllerViewer : public QTreeView
{
	Q_OBJECT
	public:
		ControllerViewer ( QWidget *parent = 0 );
	   ~ControllerViewer();
	    void SetupTree( void );

		QStandardItemModel *m_Model;
		
	protected:
	private:

	public:



	protected:

	private:

private slots:

signals:
	
		
};










#endif