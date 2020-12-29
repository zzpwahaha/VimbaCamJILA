/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        CameraTreeWindow.h

  Description:  This is the main window of the Vimba Viewer that 
                lists all connected cameras. 

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