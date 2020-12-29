/*=============================================================================
  Copyright (C) 2012 - 2013 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        LineEditCompleter.h

  Description: LineEdit used by Filter

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
