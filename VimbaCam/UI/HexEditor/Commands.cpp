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

#include "Commands.h"

CharCommand::CharCommand(XByteArray * xData, Cmd cmd, int charPos, char newChar, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    m_xData = xData;
    m_CharPos = charPos;
    m_NewChar = newChar;
    m_Cmd = cmd;
}

bool CharCommand::mergeWith(const QUndoCommand *command)
{
    const CharCommand *nextCommand = static_cast<const CharCommand *>(command);
    bool result = false;

    if (m_Cmd != remove)
    {
        if (nextCommand->m_Cmd == replace)
            if (nextCommand->m_CharPos == m_CharPos)
            {
                m_NewChar = nextCommand->m_NewChar;
                result = true;
            }
    }
    return result;
}

void CharCommand::undo()
{
    switch (m_Cmd)
    {
        case insert:
            m_xData->remove(m_CharPos, 1);
            break;
        case replace:
            m_xData->replace(m_CharPos, m_OldChar);
            m_xData->setDataChanged(m_CharPos, m_WasChanged);
            break;
        case remove:
            m_xData->insert(m_CharPos, m_OldChar);
            m_xData->setDataChanged(m_CharPos, m_WasChanged);
            break;
    }
}

void CharCommand::redo()
{
    switch (m_Cmd)
    {
        case insert:
            m_xData->insert(m_CharPos, m_NewChar);
            break;
        case replace:
            m_OldChar = m_xData->data()[m_CharPos];
            m_WasChanged = m_xData->dataChanged(m_CharPos);
            m_xData->replace(m_CharPos, m_NewChar);
            break;
        case remove:
            m_OldChar = m_xData->data()[m_CharPos];
            m_WasChanged = m_xData->dataChanged(m_CharPos);
            m_xData->remove(m_CharPos, 1);
            break;
    }
}



ArrayCommand::ArrayCommand(XByteArray * xData, Cmd cmd, int baPos, QByteArray newBa, int len, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    m_Cmd = cmd;
    m_xData = xData;
    m_ByteArrayPos = baPos;
    m_NewByteArray = newBa;
    m_Length = len;
}

void ArrayCommand::undo()
{
    switch (m_Cmd)
    {
        case insert:
            m_xData->remove(m_ByteArrayPos, m_NewByteArray.length());
            break;
        case replace:
            m_xData->replace(m_ByteArrayPos, m_OldByteArray);
            m_xData->setDataChanged(m_ByteArrayPos, m_WasChanged);
            break;
        case remove:
            m_xData->insert(m_ByteArrayPos, m_OldByteArray);
            m_xData->setDataChanged(m_ByteArrayPos, m_WasChanged);
            break;
    }
}

void ArrayCommand::redo()
{
    switch (m_Cmd)
    {
        case insert:
            m_xData->insert(m_ByteArrayPos, m_NewByteArray);
            break;
        case replace:
            m_OldByteArray = m_xData->data().mid(m_ByteArrayPos, m_Length);
            m_WasChanged = m_xData->dataChanged(m_ByteArrayPos, m_Length);
            m_xData->replace(m_ByteArrayPos, m_NewByteArray);
            break;
        case remove:
            m_OldByteArray = m_xData->data().mid(m_ByteArrayPos, m_Length);
            m_WasChanged = m_xData->dataChanged(m_ByteArrayPos, m_Length);
            m_xData->remove(m_ByteArrayPos, m_Length);
            break;
    }
}
