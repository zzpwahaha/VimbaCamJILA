
#ifndef LOGGERWINDOW_H
#define LOGGERWINDOW_H

#include <QListWidget>
#include "Helper.h"

class LoggerWindow : public QListWidget
{
    Q_OBJECT
    public: 
    
    protected:            
      
    private:
                    
    public:
        LoggerWindow ( QWidget *parent );
        ~LoggerWindow ( void );
        void logging( const QString &sInfo , const VimbaViewerLogCategory &logCategory );
        void plainLogging ( const QString &sInfo );
                 
    protected:
                    
    private:
                                
};

#endif