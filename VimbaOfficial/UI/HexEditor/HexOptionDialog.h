/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HexOptionDialog.cpp

  Description: a setting dialog for hex editor

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


#ifndef HEXOPTIONDIALOG_H
#define HEXOPTIONDIALOG_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

namespace Ui {
    class OptionDialog;
}

class HexOptionDialog : public QDialog
{
    Q_OBJECT

    public:
               explicit HexOptionDialog(QWidget *parent = 0);
                       ~HexOptionDialog();

              Ui::OptionDialog *ui;
    
    public slots:
               virtual void accept();

    private slots:
               void on_pbHighlightingColor_clicked();
               void on_pbAddressAreaColor_clicked();
               void on_pbSelectionColor_clicked();
               void on_pbWidgetFont_clicked();

    private:
               void readSettings();
               void writeSettings();
               void setColor(QWidget *widget, QColor color);
};

#endif