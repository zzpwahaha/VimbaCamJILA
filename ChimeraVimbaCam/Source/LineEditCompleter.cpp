#include "stdafx.h"
#include "LineEditCompleter.h"
#include <qabstractitemview.h>

LineEditCompleter::LineEditCompleter(QWidget* parent) : QLineEdit(parent), c(nullptr)
{
}

LineEditCompleter::~LineEditCompleter()
{
}

void LineEditCompleter::setCompleter(MultiCompleter* completer)
{
    if (nullptr == c)
        QObject::disconnect(c, 0, this, 0);

    c = completer;

    if (!c)
        return;
    c->setWidget(this);
    connect(completer, qOverload<const QString&>(&QCompleter::activated), this, &LineEditCompleter::insertCompletion);
}

MultiCompleter* LineEditCompleter::completer() const
{
    return c;
}

void LineEditCompleter::insertCompletion(const QString& completion)
{
    setText(completion);
}

void LineEditCompleter::keyPressEvent(QKeyEvent* e)
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

    if (Qt::Key_Enter == e->key())
    {
        c->popup()->hide();
    }
}

MultiCompleter::MultiCompleter(const QStringList& items, QObject* parent)
    : QCompleter(items, parent), m_list(items), m_model()
{
    setModel(&m_model);
}

MultiCompleter::~MultiCompleter()
{
}

// used to set the returned completion to contain everything on the left of SEPARATOR, since only words to the right
// of SEPARATOR is passed to model
QString MultiCompleter::pathFromIndex(const QModelIndex& index) const
{
    QString path = QCompleter::pathFromIndex(index);
    /*widget(): Returns the widget for which the completer object is providing completions.*/
    QString text = static_cast<QLineEdit*>(widget())->text();

    int pos = text.lastIndexOf(SEPARATOR);
    if (pos >= 0)
        path = text.left(pos) + SEPARATOR + path;

    return path;
}

