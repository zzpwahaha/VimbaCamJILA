#pragma once
#include <qlineedit.h>
#include <QKeyEvent>

#include <QCompleter>
#include <QStringList>
#include <QModelIndex>
#include <QStringListModel>

class MultiCompleter;
class LineEditCompleter : public QLineEdit
{
    Q_OBJECT

public:
    LineEditCompleter(QWidget* parent = 0);
    ~LineEditCompleter();

    void setCompleter(MultiCompleter* c);
    MultiCompleter* completer() const;

protected:
    void keyPressEvent(QKeyEvent* e);

private slots:
    void insertCompletion(const QString& completion);

private:
    MultiCompleter* c;
};

class MultiCompleter : public QCompleter
{
    Q_OBJECT

private:
    QStringList      m_list;
    QStringListModel m_model;
    QString          m_word;
public:
    // the SEPARATOR enables multiple search element in one lineedit as long as one also configure the search
    // engine to separate input words according to SEPARATOR
    const char SEPARATOR = '|';

public:
    MultiCompleter(const QStringList& items, QObject* parent);
    ~MultiCompleter();

    inline void update(QString word)
    {
        /* Do any filtering you like */
        int pos = word.lastIndexOf(SEPARATOR) + 1;

        while (pos < word.length() && word.at(pos) == QLatin1Char(' '))
            pos++;

        word = word.mid(pos); // get everything to the right of SEPARATOR
        /* Include all items that contain word, this is the core part of the completer */
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
    QString pathFromIndex(const QModelIndex& index) const;
};