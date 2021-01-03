//Description: QCompleter with multi selection

#ifndef MULTICOMPLETER_H
#define MULTICOMPLETER_H

#include <QCompleter>
#include <QStringList>
#include <QLineEdit>
#include <QModelIndex>
#include <QStringListModel>

class MultiCompleter : public QCompleter
{
    Q_OBJECT

    private:
              QStringList      m_list;
              QStringListModel m_model;
              QString          m_word;

    public:
              MultiCompleter( const QStringList& items, QObject* parent );
             ~MultiCompleter();

              inline void update(QString word)
              {
                  /* Do any filtering you like */
                  int pos = word.lastIndexOf( '|' ) + 1;

                  while ( pos < word.length() && word.at( pos ) == QLatin1Char( ' ' ) )
                      pos++;

                  word = word.mid( pos ) ;
                  /* Include all items that contain word */
                  QStringList filtered = m_list.filter(word, caseSensitivity());
                  m_model.setStringList(filtered);
                  m_word = word;
                  complete();    
              }

              inline QString word()
              {
                  return m_word;
              }

    public:
              QString pathFromIndex( const QModelIndex& index ) const;
};

#endif
