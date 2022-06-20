//Description: LineEdit used by Filter

#include "LineEditCompleter.h"
#include <qabstractitemview.h>


LineEditCompleter::LineEditCompleter(QWidget *parent) : QLineEdit(parent), c(0)
{
}

LineEditCompleter::~LineEditCompleter()
{
}

void LineEditCompleter::setCompleter(MultiCompleter *completer)
{
    if (c)
        QObject::disconnect(c, 0, this, 0);

    c = completer;

    if (!c)
        return;

    c->setWidget(this);
    connect(completer, SIGNAL(activated(const QString&)), this, SLOT(insertCompletion(const QString&)));
}

MultiCompleter *LineEditCompleter::completer() const
{
    return c;
}

void LineEditCompleter::insertCompletion(const QString& completion)
{
    setText(completion);
}

void LineEditCompleter::keyPressEvent(QKeyEvent *e)
{
    if (c && c->popup()->isVisible())
    {
        /* The following keys are forwarded by the completer to the widget */
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return; /* Let the completer do default behavior */
        }
    }

    bool isShortcut = (e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E;
    if (!isShortcut)
        QLineEdit::keyPressEvent(e); /* Don't send the shortcut (CTRL-E) to the text edit. */

    if (!c)
        return;

    bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!isShortcut && !ctrlOrShift && e->modifiers() != Qt::NoModifier)
    {
        c->popup()->hide();
        return;
    }

    c->update(text());
    c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));

    if( Qt::Key_Enter == e->key())
    {
        c->popup()->hide();
    }
}

