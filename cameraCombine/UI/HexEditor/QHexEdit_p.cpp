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

#include "QHexEdit_p.h"
#include "Commands.h"

#include <QtWidgets>

const int HEXCHARS_IN_LINE = 47;
const int GAP_ADR_HEX = 10;
const int GAP_HEX_ASCII = 16;
const int BYTES_PER_LINE = 16;

QHexEditPrivate::QHexEditPrivate(QScrollArea *parent) : QWidget(parent)
{
    m_UndoStack = new QUndoStack(this);

    m_ScrollArea = parent;
    setAddressWidth(4);
    setAddressOffset(0);
    setAddressArea(true);
    setAsciiArea(true);
    setHighlighting(true);
    setOverwriteMode(true);
    setReadOnly(false);
    setAddressAreaColor(QColor(0xd4, 0xd4, 0xd4, 0xff));
    setHighlightingColor(QColor(0xff, 0xff, 0x99, 0xff));
    setSelectionColor(QColor(0x6d, 0x9e, 0xff, 0xff));
    setFont(QFont("Courier", 10));

    m_Size = 0;
    resetSelection(0);

    setFocusPolicy(Qt::StrongFocus);

    connect(&m_CursorTimer, SIGNAL(timeout()), this, SLOT(updateCursor()));
    m_CursorTimer.setInterval(500);
    m_CursorTimer.start();
}

void QHexEditPrivate::setAddressOffset(int offset)
{
    m_xData.setAddressOffset(offset);
    adjust();
}

int QHexEditPrivate::addressOffset()
{
    return m_xData.addressOffset();
}

void QHexEditPrivate::setData(const QByteArray &data)
{
    m_xData.setData(data);
    m_UndoStack->clear();
    adjust();
    setCursorPos(0);
}

QByteArray QHexEditPrivate::data()
{
    return m_xData.data();
}

void QHexEditPrivate::setAddressAreaColor(const QColor &color)
{
    m_AddressAreaColor = color;
    update();
}

QColor QHexEditPrivate::addressAreaColor()
{
    return m_AddressAreaColor;
}

void QHexEditPrivate::setHighlightingColor(const QColor &color)
{
    m_HighlightingColor = color;
    update();
}

QColor QHexEditPrivate::highlightingColor()
{
    return m_HighlightingColor;
}

void QHexEditPrivate::setSelectionColor(const QColor &color)
{
    m_SelectionColor = color;
    update();
}

QColor QHexEditPrivate::selectionColor()
{
    return m_SelectionColor;
}

void QHexEditPrivate::setReadOnly(bool readOnly)
{
    m_ReadOnly = readOnly;
}

bool QHexEditPrivate::isReadOnly()
{
    return m_ReadOnly;
}

XByteArray & QHexEditPrivate::xData()
{
    return m_xData;
}

int QHexEditPrivate::indexOf(const QByteArray & ba, int from)
{
    if (from > (m_xData.data().length() - 1))
        from = m_xData.data().length() - 1;
    int idx = m_xData.data().indexOf(ba, from);
    if (idx > -1)
    {
        int curPos = idx*2;
        setCursorPos(curPos + ba.length()*2);
        resetSelection(curPos);
        setSelection(curPos + ba.length()*2);
        ensureVisible();
    }
    return idx;
}

void QHexEditPrivate::insert(int index, const QByteArray & ba)
{
    if (ba.length() > 0)
    {
        if (m_OverwriteMode)
        {
            QUndoCommand *arrayCommand= new ArrayCommand(&m_xData, ArrayCommand::replace, index, ba, ba.length());
            m_UndoStack->push(arrayCommand);
            emit dataChanged();
        }
        else
        {
            QUndoCommand *arrayCommand= new ArrayCommand(&m_xData, ArrayCommand::insert, index, ba, ba.length());
            m_UndoStack->push(arrayCommand);
            emit dataChanged();
        }
    }
}

void QHexEditPrivate::insert(int index, char ch)
{
    QUndoCommand *charCommand = new CharCommand(&m_xData, CharCommand::insert, index, ch);
    m_UndoStack->push(charCommand);
    emit dataChanged();
}

int QHexEditPrivate::lastIndexOf(const QByteArray & ba, int from)
{
    from -= ba.length();
    if (from < 0)
        from = 0;
    int idx = m_xData.data().lastIndexOf(ba, from);
    if (idx > -1)
    {
        int curPos = idx*2;
        setCursorPos(curPos);
        resetSelection(curPos);
        setSelection(curPos + ba.length()*2);
        ensureVisible();
    }
    return idx;
}

void QHexEditPrivate::remove(int index, int len)
{
    if (len > 0)
    {
        if (len == 1)
        {
            if (m_OverwriteMode)
            {
                QUndoCommand *charCommand = new CharCommand(&m_xData, CharCommand::replace, index, char(0));
                m_UndoStack->push(charCommand);
                emit dataChanged();
            }
            else
            {
                QUndoCommand *charCommand = new CharCommand(&m_xData, CharCommand::remove, index, char(0));
                m_UndoStack->push(charCommand);
                emit dataChanged();
            }
        }
        else
        {
            QByteArray ba = QByteArray(len, char(0));
            if (m_OverwriteMode)
            {
                QUndoCommand *arrayCommand = new ArrayCommand(&m_xData, ArrayCommand::replace, index, ba, ba.length());
                m_UndoStack->push(arrayCommand);
                emit dataChanged();
            }
            else
            {
                QUndoCommand *arrayCommand= new ArrayCommand(&m_xData, ArrayCommand::remove, index, ba, len);
                m_UndoStack->push(arrayCommand);
                emit dataChanged();
            }
        }
    }
}

void QHexEditPrivate::replace(int index, char ch)
{
    QUndoCommand *charCommand = new CharCommand(&m_xData, CharCommand::replace, index, ch);
    m_UndoStack->push(charCommand);
    resetSelection();
    emit dataChanged();
}

void QHexEditPrivate::replace(int index, const QByteArray & ba)
{
    QUndoCommand *arrayCommand= new ArrayCommand(&m_xData, ArrayCommand::replace, index, ba, ba.length());
    m_UndoStack->push(arrayCommand);
    resetSelection();
    emit dataChanged();
}

void QHexEditPrivate::replace(int pos, int len, const QByteArray &after)
{
    QUndoCommand *arrayCommand= new ArrayCommand(&m_xData, ArrayCommand::replace, pos, after, len);
    m_UndoStack->push(arrayCommand);
    resetSelection();
    emit dataChanged();
}

void QHexEditPrivate::setAddressArea(bool addressArea)
{
    m_AddressArea = addressArea;
    adjust();

    setCursorPos(m_CursorPosition);
}

void QHexEditPrivate::setAddressWidth(int addressWidth)
{
    m_xData.setAddressWidth(addressWidth);

    setCursorPos(m_CursorPosition);
}

void QHexEditPrivate::setAsciiArea(bool asciiArea)
{
    m_AsciiArea = asciiArea;
    adjust();
}

void QHexEditPrivate::setFont(const QFont &font)
{
    QWidget::setFont(font);
    adjust();
}

void QHexEditPrivate::setHighlighting(bool mode)
{
    m_Highlighting = mode;
    update();
}

void QHexEditPrivate::setOverwriteMode(bool overwriteMode)
{
    m_OverwriteMode = overwriteMode;
}

bool QHexEditPrivate::overwriteMode()
{
    return m_OverwriteMode;
}

void QHexEditPrivate::redo()
{
    m_UndoStack->redo();
    emit dataChanged();
    setCursorPos(m_CursorPosition);
    update();
}

void QHexEditPrivate::undo()
{
    m_UndoStack->undo();
    emit dataChanged();
    setCursorPos(m_CursorPosition);
    update();
}

QString QHexEditPrivate::toRedableString()
{
    return m_xData.toRedableString();
}


QString QHexEditPrivate::selectionToReadableString()
{
    return m_xData.toRedableString(getSelectionBegin(), getSelectionEnd());
}

void QHexEditPrivate::keyPressEvent(QKeyEvent *event)
{
    int charX = (m_CursorX - m_xPosHex) / m_CharWidth;
    int posX = (charX / 3) * 2 + (charX % 3);
    int posBa = (m_CursorY / m_CharHeight) * BYTES_PER_LINE + posX / 2;


    /*****************************************************************************/
    /* Cursor movements */
    /*****************************************************************************/

    if (event->matches(QKeySequence::MoveToNextChar))
    {
        setCursorPos(m_CursorPosition + 1);
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToPreviousChar))
    {
        setCursorPos(m_CursorPosition - 1);
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToEndOfLine))
    {
        setCursorPos(m_CursorPosition | (2 * BYTES_PER_LINE -1));
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToStartOfLine))
    {
        setCursorPos(m_CursorPosition - (m_CursorPosition % (2 * BYTES_PER_LINE)));
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToPreviousLine))
    {
        setCursorPos(m_CursorPosition - (2 * BYTES_PER_LINE));
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToNextLine))
    {
        setCursorPos(m_CursorPosition + (2 * BYTES_PER_LINE));
        resetSelection(m_CursorPosition);
    }

    if (event->matches(QKeySequence::MoveToNextPage))
    {
        setCursorPos(m_CursorPosition + (((m_ScrollArea->viewport()->height() / m_CharHeight) - 1) * 2 * BYTES_PER_LINE));
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToPreviousPage))
    {
        setCursorPos(m_CursorPosition - (((m_ScrollArea->viewport()->height() / m_CharHeight) - 1) * 2 * BYTES_PER_LINE));
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToEndOfDocument))
    {
        setCursorPos(m_xData.size() * 2);
        resetSelection(m_CursorPosition);
    }
    if (event->matches(QKeySequence::MoveToStartOfDocument))
    {
        setCursorPos(0);
        resetSelection(m_CursorPosition);
    }

    /*****************************************************************************/
    /* Select commands */
    /*****************************************************************************/
    if (event->matches(QKeySequence::SelectAll))
    {
        resetSelection(0);
        setSelection(2*m_xData.size() + 1);
    }
    if (event->matches(QKeySequence::SelectNextChar))
    {
        int pos = m_CursorPosition + 1;
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectPreviousChar))
    {
        int pos = m_CursorPosition - 1;
        setSelection(pos);
        setCursorPos(pos);
    }
    if (event->matches(QKeySequence::SelectEndOfLine))
    {
        int pos = m_CursorPosition - (m_CursorPosition % (2 * BYTES_PER_LINE)) + (2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectStartOfLine))
    {
        int pos = m_CursorPosition - (m_CursorPosition % (2 * BYTES_PER_LINE));
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectPreviousLine))
    {
        int pos = m_CursorPosition - (2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectNextLine))
    {
        int pos = m_CursorPosition + (2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }

    if (event->matches(QKeySequence::SelectNextPage))
    {
        int pos = m_CursorPosition + (((m_ScrollArea->viewport()->height() / m_CharHeight) - 1) * 2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectPreviousPage))
    {
        int pos = m_CursorPosition - (((m_ScrollArea->viewport()->height() / m_CharHeight) - 1) * 2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectEndOfDocument))
    {
        int pos = m_xData.size() * 2;
        setCursorPos(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectStartOfDocument))
    {
        int pos = 0;
        setCursorPos(pos);
        setSelection(pos);
    }

    /*****************************************************************************/
    /* Edit Commands */
    /*****************************************************************************/
    if (!m_ReadOnly)
    {
        /* Hex input */
        int key = int(event->text().toUtf8()[0]);
        if ((key>='0' && key<='9') || (key>='a' && key <= 'f'))
        {
            if (getSelectionBegin() != getSelectionEnd())
            {
                posBa = getSelectionBegin();
                remove(posBa, getSelectionEnd() - posBa);
                setCursorPos(2*posBa);
                resetSelection(2*posBa);
            }

            // If insert mode, then insert a byte
            if (m_OverwriteMode == false)
                if ((charX % 3) == 0)
                {
                    insert(posBa, char(0));
                }

                // Change content
                if (m_xData.size() > 0)
                {
                    QByteArray hexValue = m_xData.data().mid(posBa, 1).toHex();
                    if ((charX % 3) == 0)
                        hexValue[0] = key;
                    else
                        hexValue[1] = key;

                    replace(posBa, QByteArray().fromHex(hexValue)[0]);

                    setCursorPos(m_CursorPosition + 1);
                    resetSelection(m_CursorPosition);
                }
        }

        /* Cut & Paste */
        if (event->matches(QKeySequence::Cut))
        {
            QString result = QString();
            for (int idx = getSelectionBegin(); idx < getSelectionEnd(); idx++)
            {
                result += m_xData.data().mid(idx, 1).toHex() + " ";
                if ((idx % 16) == 15)
                    result.append("\n");
            }
            remove(getSelectionBegin(), getSelectionEnd() - getSelectionBegin());
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(result);
            setCursorPos(getSelectionBegin());
            resetSelection(getSelectionBegin());
        }

        if (event->matches(QKeySequence::Paste))
        {
            QClipboard *clipboard = QApplication::clipboard();
            QByteArray ba = QByteArray().fromHex(clipboard->text().toLatin1());
            insert(m_CursorPosition / 2, ba);
            setCursorPos(m_CursorPosition + 2 * ba.length());
            resetSelection(getSelectionBegin());
        }


        /* Delete char */
        if (event->matches(QKeySequence::Delete))
        {
            if (getSelectionBegin() != getSelectionEnd())
            {
                posBa = getSelectionBegin();
                remove(posBa, getSelectionEnd() - posBa);
                setCursorPos(2*posBa);
                resetSelection(2*posBa);
            }
            else
            {
                if (m_OverwriteMode)
                    replace(posBa, char(0));
                else
                    remove(posBa, 1);
            }
        }

        /* Backspace */
        if ((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
        {
            if (getSelectionBegin() != getSelectionEnd())
            {
                posBa = getSelectionBegin();
                remove(posBa, getSelectionEnd() - posBa);
                setCursorPos(2*posBa);
                resetSelection(2*posBa);
            }
            else
            {
                if (posBa > 0)
                {
                    if (m_OverwriteMode)
                        replace(posBa - 1, char(0));
                    else
                        remove(posBa - 1, 1);
                    setCursorPos(m_CursorPosition - 2);
                }
            }
        }

        /* undo */
        if (event->matches(QKeySequence::Undo))
        {
            undo();
        }

        /* redo */
        if (event->matches(QKeySequence::Redo))
        {
            redo();
        }

    }

    if (event->matches(QKeySequence::Copy))
    {
        QString result = QString();
        for (int idx = getSelectionBegin(); idx < getSelectionEnd(); idx++)
        {
            result += m_xData.data().mid(idx, 1).toHex() + " ";
            if ((idx % 16) == 15)
                result.append('\n');
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(result);
    }

    // Switch between insert/overwrite mode
    if ((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
    {
        m_OverwriteMode = !m_OverwriteMode;
        setCursorPos(m_CursorPosition);
        overwriteModeChanged(m_OverwriteMode);
    }

    ensureVisible();
    update();
}

void QHexEditPrivate::mouseMoveEvent(QMouseEvent * event)
{
    m_Blink = false;
    update();
    int actPos = cursorPos(event->pos());
    setCursorPos(actPos);
    setSelection(actPos);
}

void QHexEditPrivate::mousePressEvent(QMouseEvent * event)
{
    m_Blink = false;
    update();
    int cPos = cursorPos(event->pos());
    resetSelection(cPos);
    setCursorPos(cPos);
}

void QHexEditPrivate::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // draw some patterns if needed
    painter.fillRect(event->rect(), this->palette().color(QPalette::Base));
    if (m_AddressArea)
        painter.fillRect(QRect(m_xPosAdr, event->rect().top(), m_xPosHex - GAP_ADR_HEX + 2, height()), m_AddressAreaColor);
    if (m_AsciiArea)
    {
        int linePos = m_xPosAscii - (GAP_HEX_ASCII / 2);
        painter.setPen(Qt::gray);
        painter.drawLine(linePos, event->rect().top(), linePos, height());
    }

    painter.setPen(this->palette().color(QPalette::WindowText));

    // calc position
    int firstLineIdx = ((event->rect().top()/ m_CharHeight) - m_CharHeight) * BYTES_PER_LINE;
    if (firstLineIdx < 0)
        firstLineIdx = 0;
    int lastLineIdx = ((event->rect().bottom() / m_CharHeight) + m_CharHeight) * BYTES_PER_LINE;
    if (lastLineIdx > m_xData.size())
        lastLineIdx = m_xData.size();
    int yPosStart = ((firstLineIdx) / BYTES_PER_LINE) * m_CharHeight + m_CharHeight;

    // paint address area
    if (m_AddressArea)
    {
        for (int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=m_CharHeight)
        {
            QString address = QString("%1")
                .arg(lineIdx + m_xData.addressOffset(), m_xData.realAddressNumbers(), 16, QChar('0'));
            painter.drawText(m_xPosAdr, yPos, address);
        }
    }

    // paint hex area
    QByteArray hexBa(m_xData.data().mid(firstLineIdx, lastLineIdx - firstLineIdx + 1).toHex());
    QBrush highLighted = QBrush(m_HighlightingColor);
    QPen colHighlighted = QPen(this->palette().color(QPalette::WindowText));
    QBrush selected = QBrush(m_SelectionColor);
    QPen colSelected = QPen(Qt::white);
    QPen colStandard = QPen(this->palette().color(QPalette::WindowText));

    painter.setBackgroundMode(Qt::TransparentMode);

    for (int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=m_CharHeight)
    {
        QByteArray hex;
        int xPos = m_xPosHex;
        for (int colIdx = 0; ((lineIdx + colIdx) < m_xData.size() && (colIdx < BYTES_PER_LINE)); colIdx++)
        {
            int posBa = lineIdx + colIdx;
            if ((getSelectionBegin() <= posBa) && (getSelectionEnd() > posBa))
            {
                painter.setBackground(selected);
                painter.setBackgroundMode(Qt::OpaqueMode);
                painter.setPen(colSelected);
            }
            else
            {
                if (m_Highlighting)
                {
                    // hilight diff bytes
                    painter.setBackground(highLighted);
                    if (m_xData.dataChanged(posBa))
                    {
                        painter.setPen(colHighlighted);
                        painter.setBackgroundMode(Qt::OpaqueMode);
                    }
                    else
                    {
                        painter.setPen(colStandard);
                        painter.setBackgroundMode(Qt::TransparentMode);
                    }
                }
            }

            // render hex value
            if (colIdx == 0)
            {
                hex = hexBa.mid((lineIdx - firstLineIdx) * 2, 2);
                painter.drawText(xPos, yPos, hex);
                xPos += 2 * m_CharWidth;
            } else {
                hex = hexBa.mid((lineIdx + colIdx - firstLineIdx) * 2, 2).prepend(" ");
                painter.drawText(xPos, yPos, hex);
                xPos += 3 * m_CharWidth;
            }

        }
    }
    painter.setBackgroundMode(Qt::TransparentMode);
    painter.setPen(this->palette().color(QPalette::WindowText));

    // paint ascii area
    if (m_AsciiArea)
    {
        for (int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=m_CharHeight)
        {
            int xPosAscii = m_xPosAscii;
            for (int colIdx = 0; ((lineIdx + colIdx) < m_xData.size() && (colIdx < BYTES_PER_LINE)); colIdx++)
            {
                painter.drawText(xPosAscii, yPos, m_xData.asciiChar(lineIdx + colIdx));
                xPosAscii += m_CharWidth;
            }
        }
    }

    // paint cursor
    if (m_Blink && !m_ReadOnly && hasFocus())
    {
        if (m_OverwriteMode)
            painter.fillRect(m_CursorX, m_CursorY + m_CharHeight - 2, m_CharWidth, 2, this->palette().color(QPalette::WindowText));
        else
            painter.fillRect(m_CursorX, m_CursorY, 2, m_CharHeight, this->palette().color(QPalette::WindowText));
    }

    if (m_Size != m_xData.size())
    {
        m_Size = m_xData.size();
        emit currentSizeChanged(m_Size);
    }
}

void QHexEditPrivate::setCursorPos(int position)
{
    // delete cursor
    m_Blink = false;
    update();

    // cursor in range?
    if (m_OverwriteMode)
    {
        if (position > (m_xData.size() * 2 - 1))
            position = m_xData.size() * 2 - 1;
    } else {
        if (position > (m_xData.size() * 2))
            position = m_xData.size() * 2;
    }

    if (position < 0)
        position = 0;

    // calc position
    m_CursorPosition = position;
    m_CursorY = (position / (2 * BYTES_PER_LINE)) * m_CharHeight + 4;
    int x = (position % (2 * BYTES_PER_LINE));
    m_CursorX = (((x / 2) * 3) + (x % 2)) * m_CharWidth + m_xPosHex;

    // immiadately draw cursor
    m_Blink = true;
    update();
    emit currentAddressChanged(m_CursorPosition/2);
}

int QHexEditPrivate::cursorPos(QPoint pos)
{
    int result = -1;
    // find char under cursor
    if ((pos.x() >= m_xPosHex) && (pos.x() < (m_xPosHex + HEXCHARS_IN_LINE * m_CharWidth)))
    {
        int x = (pos.x() - m_xPosHex) / m_CharWidth;
        if ((x % 3) == 0)
            x = (x / 3) * 2;
        else
            x = ((x / 3) * 2) + 1;
        int y = ((pos.y() - 3) / m_CharHeight) * 2 * BYTES_PER_LINE;
        result = x + y;
    }
    return result;
}

int QHexEditPrivate::cursorPos()
{
    return m_CursorPosition;
}

void QHexEditPrivate::resetSelection()
{
    m_SelectionBegin = m_SelectionInit;
    m_SelectionEnd = m_SelectionInit;
}

void QHexEditPrivate::resetSelection(int pos)
{
    if (pos < 0)
        pos = 0;
    pos = pos / 2;
    m_SelectionInit = pos;
    m_SelectionBegin = pos;
    m_SelectionEnd = pos;
}

void QHexEditPrivate::setSelection(int pos)
{
    if (pos < 0)
        pos = 0;
    pos = pos / 2;
    if (pos >= m_SelectionInit)
    {
        m_SelectionEnd = pos;
        m_SelectionBegin = m_SelectionInit;
    }
    else
    {
        m_SelectionBegin = pos;
        m_SelectionEnd = m_SelectionInit;
    }
}

int QHexEditPrivate::getSelectionBegin()
{
    return m_SelectionBegin;
}

int QHexEditPrivate::getSelectionEnd()
{
    return m_SelectionEnd;
}


void QHexEditPrivate::updateCursor()
{
    if (m_Blink)
        m_Blink = false;
    else
        m_Blink = true;
    update(m_CursorX, m_CursorY, m_CharWidth, m_CharHeight);
}

void QHexEditPrivate::adjust()
{
    m_CharWidth = fontMetrics().width(QLatin1Char('9'));
    m_CharHeight = fontMetrics().height();

    m_xPosAdr = 0;
    if (m_AddressArea)
        m_xPosHex = m_xData.realAddressNumbers()*m_CharWidth + GAP_ADR_HEX;
    else
        m_xPosHex = 0;
    m_xPosAscii = m_xPosHex + HEXCHARS_IN_LINE * m_CharWidth + GAP_HEX_ASCII;

    // tell QAbstractScollbar, how big we are
    setMinimumHeight(((m_xData.size()/16 + 1) * m_CharHeight) + 5);
    if(m_AsciiArea)
        setMinimumWidth(m_xPosAscii + (BYTES_PER_LINE * m_CharWidth));
    else
        setMinimumWidth(m_xPosHex + HEXCHARS_IN_LINE * m_CharWidth);

    update();
}

void QHexEditPrivate::ensureVisible()
{
    // scrolls to cursorx, cusory (which are set by setCursorPos)
    // x-margin is 3 pixels, y-margin is half of charHeight
    m_ScrollArea->ensureVisible(m_CursorX, m_CursorY + m_CharHeight/2, 3, m_CharHeight/2 + 2);
}
