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

#include "HexOptionDialog.h"
#include "ui_OptionDialog.h"


HexOptionDialog::HexOptionDialog ( QWidget *parent) : QDialog( parent ), ui(new Ui::OptionDialog)
{
    ui->setupUi(this);
    readSettings();
    writeSettings();
}

HexOptionDialog::~HexOptionDialog ( void )
{
    delete ui;
}

void HexOptionDialog::accept()
{
    writeSettings();
    emit accepted();
    QDialog::hide();
}

void HexOptionDialog::readSettings()
{
    QSettings settings("Allied Vision", "Vimba Viewer");

    ui->cbAddressArea->setChecked(settings.value("AddressArea", true).toBool());
    ui->cbAsciiArea->setChecked(settings.value("AsciiArea", true).toBool());
    ui->cbHighlighting->setChecked(settings.value("Highlighting", true).toBool());
    ui->cbOverwriteMode->setChecked(settings.value("OverwriteMode", true).toBool());
    ui->cbReadOnly->setChecked(settings.value("ReadOnly").toBool());

    setColor(ui->lbHighlightingColor, settings.value("HighlightingColor", QColor(0xff, 0xff, 0x99, 0xff)).value<QColor>());
    setColor(ui->lbAddressAreaColor, settings.value("AddressAreaColor", QColor(0xd4, 0xd4, 0xd4, 0xff)).value<QColor>());
    setColor(ui->lbSelectionColor, settings.value("SelectionColor", QColor(0x6d, 0x9e, 0xff, 0xff)).value<QColor>());
    ui->leWidgetFont->setFont(settings.value("WidgetFont", QFont("Courier", 10)).value<QFont>());

    ui->sbAddressAreaWidth->setValue(settings.value("AddressAreaWidth", 4).toInt());
}

void HexOptionDialog::writeSettings()
{
    QSettings settings("Allied Vision", "Vimba Viewer");
    settings.setValue("AddressArea", ui->cbAddressArea->isChecked());
    settings.setValue("AsciiArea", ui->cbAsciiArea->isChecked());
    settings.setValue("Highlighting", ui->cbHighlighting->isChecked());
    settings.setValue("OverwriteMode", ui->cbOverwriteMode->isChecked());
    settings.setValue("ReadOnly", ui->cbReadOnly->isChecked());

    settings.setValue("HighlightingColor", ui->lbHighlightingColor->palette().color(QPalette::Background));
    settings.setValue("AddressAreaColor", ui->lbAddressAreaColor->palette().color(QPalette::Background));
    settings.setValue("SelectionColor", ui->lbSelectionColor->palette().color(QPalette::Background));
    settings.setValue("WidgetFont",ui->leWidgetFont->font());

    settings.setValue("AddressAreaWidth", ui->sbAddressAreaWidth->value());
}

void HexOptionDialog::setColor(QWidget *widget, QColor color)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Background, color);
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
}

void HexOptionDialog::on_pbHighlightingColor_clicked()
{
    QColor color = QColorDialog::getColor(ui->lbHighlightingColor->palette().color(QPalette::Background), this);
    if (color.isValid())
        setColor(ui->lbHighlightingColor, color);
}

void HexOptionDialog::on_pbAddressAreaColor_clicked()
{
    QColor color = QColorDialog::getColor(ui->lbAddressAreaColor->palette().color(QPalette::Background), this);
    if (color.isValid())
        setColor(ui->lbAddressAreaColor, color);
}

void HexOptionDialog::on_pbSelectionColor_clicked()
{
    QColor color = QColorDialog::getColor(ui->lbSelectionColor->palette().color(QPalette::Background), this);
    if (color.isValid())
        setColor(ui->lbSelectionColor, color);
}

void HexOptionDialog::on_pbWidgetFont_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, ui->leWidgetFont->font(), this);
    if (ok)
        ui->leWidgetFont->setFont(font);
}
