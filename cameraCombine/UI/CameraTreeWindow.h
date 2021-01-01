//Description:  This is the main window of the Vimba Viewer that
//lists all connected cameras.

#ifndef CAMERATREEWINDOW_H
#define CAMERATREEWINDOW_H


#include <QTreeWidget>
#include <QMouseEvent>
#include "Helper.h"


class CameraTreeWindow : public QTreeWidget
{
    Q_OBJECT
    public: 
    
    protected:
                
      
    private:
                bool   m_bIsChecked;
                bool   m_bIsCheckboxClicked;
                bool   m_bIsRightMouseClicked;
                QTreeWidgetItem *m_CurrentItem;
                
    public:
                CameraTreeWindow ( QWidget *parent);
               ~CameraTreeWindow ( void );

                QTreeWidgetItem *createItem ( void );
                QTreeWidgetItem *createItem ( QTreeWidgetItem *itemRef, const bool &bIsCheckable );
                void setText ( QTreeWidgetItem *itemRef, const QString &text );
                void setCheckCurrentItem ( const bool &bIsChecked );
    
    protected:
                void mousePressEvent ( QMouseEvent *event );                
    private:
             
    private slots:
                void clickOnCamera ( QTreeWidgetItem *item, int column );
                        
signals:
                void cameraClicked ( const QString &sModel, const bool &bIsChecked );
                void rightMouseClicked ( const bool &bIsClicked );
                
};

#endif