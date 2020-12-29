/*=============================================================================
  Copyright (C) 2012 - 2013 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        MultiCompleter.h

  Description: QCompleter with multi selection

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
