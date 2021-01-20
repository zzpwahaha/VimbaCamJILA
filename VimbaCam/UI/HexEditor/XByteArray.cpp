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
#include "XByteArray.h"

XByteArray::XByteArray()
{
    m_OldSize = -99;
    m_AddressNumbers = 4;
    m_AddressOffset = 0;

}

int XByteArray::addressOffset()
{
    return m_AddressOffset;
}

void XByteArray::setAddressOffset(int offset)
{
    m_AddressOffset = offset;
}

int XByteArray::addressWidth()
{
    return m_AddressNumbers;
}

void XByteArray::setAddressWidth(int width)
{
    if ((width >= 0) && (width<=6))
    {
        m_AddressNumbers = width;
    }
}

QByteArray & XByteArray::data()
{
    return m_Data;
}

void XByteArray::setData(QByteArray data)
{
    m_Data = data;
    m_ChangedData = QByteArray(data.length(), char(0));
}

bool XByteArray::dataChanged(int i)
{
    return bool(m_ChangedData[i]);
}

QByteArray XByteArray::dataChanged(int i, int len)
{
    return m_ChangedData.mid(i, len);
}

void XByteArray::setDataChanged(int i, bool state)
{
    m_ChangedData[i] = char(state);
}

void XByteArray::setDataChanged(int i, const QByteArray & state)
{
    int length = state.length();
    int len;
    if ((i + length) > m_ChangedData.length())
        len = m_ChangedData.length() - i;
    else
        len = length;
    m_ChangedData.replace(i, len, state);
}

int XByteArray::realAddressNumbers()
{
    if (m_OldSize != m_Data.size())
    {
        // is addressNumbers wide enought?
        QString test = QString("%1")
            .arg(m_Data.size() + m_AddressOffset, m_AddressNumbers, 16, QChar('0'));
        m_RealAddressNumbers = test.size();
    }
    return m_RealAddressNumbers;
}

int XByteArray::size()
{
    return m_Data.size();
}

QByteArray & XByteArray::insert(int i, char ch)
{
    m_Data.insert(i, ch);
    m_ChangedData.insert(i, char(1));
    return m_Data;
}

QByteArray & XByteArray::insert(int i, const QByteArray & ba)
{
    m_Data.insert(i, ba);
    m_ChangedData.insert(i, QByteArray(ba.length(), char(1)));
    return m_Data;
}

QByteArray & XByteArray::remove(int i, int len)
{
    m_Data.remove(i, len);
    m_ChangedData.remove(i, len);
    return m_Data;
}

QByteArray & XByteArray::replace(int index, char ch)
{
    m_Data[index] = ch;
    m_ChangedData[index] = char(1);
    return m_Data;
}

QByteArray & XByteArray::replace(int index, const QByteArray & ba)
{
    int len = ba.length();
    return replace(index, len, ba);
}

QByteArray & XByteArray::replace(int index, int length, const QByteArray & ba)
{
    int len;
    if ((index + length) > m_Data.length())
        len = m_Data.length() - index;
    else
        len = length;
    m_Data.replace(index, len, ba.mid(0, len));
    m_ChangedData.replace(index, len, QByteArray(len, char(1)));
    return m_Data;
}

QChar XByteArray::asciiChar(int index)
{
    char ch = m_Data[index];
    if ((ch < 0x20) || (ch > 0x7e))
        ch = '.';
    return QChar(ch);
}

QString XByteArray::toRedableString(int start, int end)
{
    int adrWidth = realAddressNumbers();
    if (m_AddressNumbers > adrWidth)
        adrWidth = m_AddressNumbers;
    if (end < 0)
        end = m_Data.size();

    QString result;
    for (int i=start; i < end; i += 16)
    {
        QString adrStr = QString("%1").arg(m_AddressOffset + i, adrWidth, 16, QChar('0'));
        QString hexStr;
        QString ascStr;
        for (int j=0; j<16; j++)
        {
            if ((i + j) < m_Data.size())
            {
                hexStr.append(" ").append(m_Data.mid(i+j, 1).toHex());
                ascStr.append(asciiChar(i+j));
            }
        }
        result += adrStr + " " + QString("%1").arg(hexStr, -48) + "  " + QString("%1").arg(ascStr, -17) + "\n";
    }
    return result;
}
