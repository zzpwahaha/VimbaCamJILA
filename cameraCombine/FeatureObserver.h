/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        FeatureObserver.h

  Description: A notification whenever feature state or value has been changed
               

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


#ifndef FEATUREOBSERVER_H
#define FEATUREOBSERVER_H

#include <QStringList>
#include <QObject>
#include <QTimer>
#include <VimbaCPP/Include/Feature.h>
#include <VimbaCPP/Include/IFeatureObserver.h>
#include <VimbaCPP/Include/VimbaSystem.h>

const unsigned int MAX_EVENTS = 13;

using AVT::VmbAPI::FeaturePtr;
using AVT::VmbAPI::CameraPtr;

class FeatureObserver : public QObject, public AVT::VmbAPI::IFeatureObserver
{
    Q_OBJECT

    private:
          CameraPtr         m_pCam;
          QStringList       m_Message;
          QTimer           *m_Timer;
          bool              m_bIsEventRunning;
          
    public:
        
          FeatureObserver(CameraPtr pCam);
         ~FeatureObserver(void);

          void startObserverTimer ( void );
          void stopObserverTimer  ( void );
          bool isEventRunning     ( void );
          virtual void FeatureChanged(const FeaturePtr &feature);  

    protected:
    private:
          bool isEventFeature( const FeaturePtr pFeature );
          void sendEventMessage ( const FeaturePtr pFeature );
    
    private slots:
          void checkSendEvent ( void );

    signals:
          void setChangedFeature ( const QString &sFeat, const QString &sValue, const bool &bIsWritable );
          void setEventMessage  ( const QStringList &sMsg );
};

typedef SP_DECL(FeatureObserver) FeatureObserverPtr;

#endif