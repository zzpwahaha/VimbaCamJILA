/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TAB_EXTENSIONINTERFACE_H
#define TAB_EXTENSIONINTERFACE_H
#include <QString>
#include <VimbaCPP/Include/VimbaSystem.h>
#include "TabExtensionResult.h"

typedef QSharedPointer<QVector<AVT::VmbAPI::FeaturePtr> > QFeatureVectorPtr;

/*Tab Extension Interface*/
class TabExtensionInterface : public QObject
{
public:
    virtual ~TabExtensionInterface() {}
    virtual TabExtensionResult  get(const QSharedPointer<QVector<AVT::VmbAPI::FeaturePtr> > &qFeatVec, QWidget &widget, QMap<QString, QVariant> *parameters = NULL) = 0;
    virtual bool                connectToResetFps(QObject* pReceiver, const char* pszSlot) = 0;
    virtual bool                connectFromAcquire(QObject* pSender, const char* pszSignal) = 0;
};

Q_DECLARE_INTERFACE(TabExtensionInterface,
                    "com.trolltech.Plugin.TabExtensionInterface/1.0");

#endif
