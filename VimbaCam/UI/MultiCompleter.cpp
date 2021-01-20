//Description: QCompleter with multi selection

#include "MultiCompleter.h"


MultiCompleter::MultiCompleter ( const QStringList& items, QObject* parent )
    : QCompleter( items, parent ), m_list(items), m_model()
{
    setModel(&m_model);
}

MultiCompleter::~MultiCompleter()
{
}

QString MultiCompleter::pathFromIndex ( const QModelIndex& index ) const
{
    QString path = QCompleter::pathFromIndex( index );

    QString text = static_cast<QLineEdit*>( widget() )->text();

    int pos = text.lastIndexOf( '|' );
    if ( pos >= 0 )
        path = text.left( pos ) + "|" + path;

    return path;
}

