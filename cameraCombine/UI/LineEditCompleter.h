//Description: LineEdit used by Filter

#ifndef LINEEDITCOMPLETER_H
#define LINEEDITCOMPLETER_H

#include <QLineEdit>
#include "MultiCompleter.h"
#include <QKeyEvent>

class LineEditCompleter : public QLineEdit
{
    Q_OBJECT

    public:
             LineEditCompleter(QWidget *parent = 0);
            ~LineEditCompleter();

             void setCompleter(MultiCompleter *c);
             MultiCompleter *completer() const;

    protected:
             void keyPressEvent(QKeyEvent *e);

    private slots:
             void insertCompletion(const QString &completion);
            
    private:
             MultiCompleter *c;
};

#endif
