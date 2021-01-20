/*
| ==============================================================================
| Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.
|
| This code may be used in part, or in whole for your application development.
|
|==============================================================================
|
| THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
| WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
| NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
| DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
| LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
| OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF
| LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
| NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
| EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
| http://code.google.com/p/qhexedit2/
|==============================================================================
*/

#include <QtGui>

#include "QHexEdit.h"


QHexEdit::QHexEdit(QWidget *parent) : QScrollArea(parent)
{
    m_qHexEdit_p = new QHexEditPrivate(this);
    setWidget(m_qHexEdit_p);
    setWidgetResizable(true);

    connect(m_qHexEdit_p, SIGNAL(currentAddressChanged(int)), this, SIGNAL(currentAddressChanged(int)));
    connect(m_qHexEdit_p, SIGNAL(currentSizeChanged(int)), this, SIGNAL(currentSizeChanged(int)));
    connect(m_qHexEdit_p, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
    connect(m_qHexEdit_p, SIGNAL(overwriteModeChanged(bool)), this, SIGNAL(overwriteModeChanged(bool)));
    setFocusPolicy(Qt::NoFocus);
}

int QHexEdit::indexOf(const QByteArray & ba, int from) const
{
    return m_qHexEdit_p->indexOf(ba, from);
}

void QHexEdit::insert(int i, const QByteArray & ba)
{
    m_qHexEdit_p->insert(i, ba);
}

void QHexEdit::insert(int i, char ch)
{
    m_qHexEdit_p->insert(i, ch);
}

int QHexEdit::lastIndexOf(const QByteArray & ba, int from) const
{
    return m_qHexEdit_p->lastIndexOf(ba, from);
}

void QHexEdit::remove(int pos, int len)
{
    m_qHexEdit_p->remove(pos, len);
}

void QHexEdit::replace( int pos, int len, const QByteArray & after)
{
    m_qHexEdit_p->replace(pos, len, after);
}

QString QHexEdit::toReadableString()
{
    return m_qHexEdit_p->toRedableString();
}

QString QHexEdit::selectionToReadableString()
{
    return m_qHexEdit_p->selectionToReadableString();
}

void QHexEdit::setAddressArea(bool addressArea)
{
    m_qHexEdit_p->setAddressArea(addressArea);
}

void QHexEdit::redo()
{
    m_qHexEdit_p->redo();
}

void QHexEdit::undo()
{
    m_qHexEdit_p->undo();
}

void QHexEdit::setAddressWidth(int addressWidth)
{
    m_qHexEdit_p->setAddressWidth(addressWidth);
}

void QHexEdit::setAsciiArea(bool asciiArea)
{
    m_qHexEdit_p->setAsciiArea(asciiArea);
}

void QHexEdit::setHighlighting(bool mode)
{
    m_qHexEdit_p->setHighlighting(mode);
}

void QHexEdit::setAddressOffset(int offset)
{
    m_qHexEdit_p->setAddressOffset(offset);
}

int QHexEdit::addressOffset()
{
    return m_qHexEdit_p->addressOffset();
}

void QHexEdit::setCursorPosition(int cursorPos)
{
    // cursorPos in QHexEditPrivate is the position of the textcoursor without
    // blanks, means bytePos*2
    m_qHexEdit_p->setCursorPos(cursorPos*2);
}

int QHexEdit::cursorPosition()
{
    return m_qHexEdit_p->cursorPos() / 2;
}


void QHexEdit::setData(const QByteArray &data)
{
    m_qHexEdit_p->setData(data);
}

QByteArray QHexEdit::data()
{
    return m_qHexEdit_p->data();
}

void QHexEdit::setAddressAreaColor(const QColor &color)
{
    m_qHexEdit_p->setAddressAreaColor(color);
}

QColor QHexEdit::addressAreaColor()
{
    return m_qHexEdit_p->addressAreaColor();
}

void QHexEdit::setHighlightingColor(const QColor &color)
{
    m_qHexEdit_p->setHighlightingColor(color);
}

QColor QHexEdit::highlightingColor()
{
    return m_qHexEdit_p->highlightingColor();
}

void QHexEdit::setSelectionColor(const QColor &color)
{
    m_qHexEdit_p->setSelectionColor(color);
}

QColor QHexEdit::selectionColor()
{
    return m_qHexEdit_p->selectionColor();
}

void QHexEdit::setOverwriteMode(bool overwriteMode)
{
    m_qHexEdit_p->setOverwriteMode(overwriteMode);
}

bool QHexEdit::overwriteMode()
{
    return m_qHexEdit_p->overwriteMode();
}

void QHexEdit::setReadOnly(bool readOnly)
{
    m_qHexEdit_p->setReadOnly(readOnly);
}

bool QHexEdit::isReadOnly()
{
    return m_qHexEdit_p->isReadOnly();
}

void QHexEdit::setFont(const QFont &font)
{
    m_qHexEdit_p->setFont(font);
}

const QFont & QHexEdit::font() const
{
    return m_qHexEdit_p->font();
}
