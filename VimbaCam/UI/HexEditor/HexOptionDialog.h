

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