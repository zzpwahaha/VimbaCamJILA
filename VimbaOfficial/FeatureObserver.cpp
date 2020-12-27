/*=============================================================================
  Copyright (C) 2012 - 2016 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        FeatureObserver.cpp

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


#include "FeatureObserver.h"

#ifndef WIN32
    #include <cstring>
#endif


FeatureObserver::FeatureObserver ( CameraPtr pCam ) : m_bIsEventRunning ( false )
{ 
    m_pCam = pCam;
    m_Timer = new QTimer(this);
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(checkSendEvent()));
}

FeatureObserver::~FeatureObserver ( void )
{

}

void FeatureObserver::startObserverTimer ( void )
{
    if( !m_Timer->isActive())
    {
        m_Timer->start(500);
    }
}

void FeatureObserver::stopObserverTimer ( void )
{
    if( m_Timer->isActive())
    {
        m_Timer->stop();
    }
}

void FeatureObserver::checkSendEvent ( void )
{
    emit setEventMessage(m_Message);
    m_Message.clear();
    stopObserverTimer();
    m_bIsEventRunning = false;
}

bool FeatureObserver::isEventRunning ( void )
{
    return m_bIsEventRunning;
}

void FeatureObserver::FeatureChanged ( const AVT::VmbAPI::FeaturePtr &feature )
{
    if ( feature != NULL )
    {
        std::string stdName("");

        VmbError_t error = feature->GetDisplayName(stdName);
        if(stdName.empty())
        {
            error = feature->GetName(stdName);
        }

        if(VmbErrorSuccess == error)    
        {
            QString sName = QString::fromStdString(stdName);

            VmbFeatureDataType pDataType = VmbFeatureDataUnknown;
            error = feature->GetDataType(pDataType);

            if(VmbErrorSuccess == error)
            {
                VmbInt64_t  nValue64    = 0;
                double      dValue      = 0;
                bool        bValue      = false;

                QString sValue("");
                std::string stdValue;

                switch(pDataType)
                {
                case VmbFeatureDataInt:
                    if(VmbErrorSuccess == feature->GetValue(nValue64))
                    {
                        sValue = QString::number(nValue64);
                        isEventFeature( feature );
                    }
                    break;

                case VmbFeatureDataFloat:
                    if(VmbErrorSuccess == feature->GetValue(dValue))
                        sValue = QString::number(dValue);
                    break;

                case VmbFeatureDataEnum:
                    if(VmbErrorSuccess == feature->GetValue(stdValue))
                        sValue = QString::fromStdString(stdValue);
                    break;

                case VmbFeatureDataString:
                    if(VmbErrorSuccess == feature->GetValue(stdValue))
                        sValue = QString::fromStdString (stdValue);
                    break;

                case VmbFeatureDataBool:
                    if(VmbErrorSuccess == feature->GetValue(bValue))
                        bValue ? sValue = "true" : sValue = "false";
                    break;

                case VmbFeatureDataCommand: 
                    sValue = "[COMMAND]";
                    break;

                case VmbFeatureDataRaw: 
                    sValue = "Click here to open";
                    break;

                default: break;

                }

                bool bIsWritable = false;
                if( VmbErrorSuccess == feature->IsWritable(bIsWritable) )
                {
                    emit setChangedFeature(sName, sValue, bIsWritable );
                }
            }
        }
    }
}

bool FeatureObserver::isEventFeature( const FeaturePtr pFeature )
{
    std::string sCategory;
    if (    !SP_ISNULL( pFeature )                                                  // Test for being beneath EventID category
         && VmbErrorSuccess == SP_ACCESS( pFeature )->GetCategory( sCategory )
         && std::strstr( sCategory.c_str(), "/EventID" ))
    {
        sendEventMessage( pFeature );                                               // Send Qt Signal to display event data
        return true;
    }
    else
    {
        return false;
    }
}

void FeatureObserver::sendEventMessage ( const FeaturePtr pFeature )
{
    FeaturePtr pFeatureTimeStamp;                           // Every event feature has two according event data features. These are
    FeaturePtr pFeatureFrameID;                             // XYZTimestamp and XYZFrameID

    std::string name;
    std::string nameTimestamp;
    std::string nameFrameID;

    VmbInt64_t nValue;
    VmbInt64_t nValueTimeStamp = 0;
    VmbInt64_t nValueFrameID = 0;
    
    static int i = 0;

    if ( VmbErrorSuccess == SP_ACCESS( pFeature )->GetName( name ))
    {
        nameTimestamp = nameFrameID = name;
        nameTimestamp.append( "Timestamp" );                // Construct the Timestamp feature name
        nameFrameID.append( "FrameID" );                    // Construct the FrameID feature name
        if (    VmbErrorSuccess == SP_ACCESS( m_pCam )->GetFeatureByName( nameTimestamp.c_str(), pFeatureTimeStamp )
             && VmbErrorSuccess == SP_ACCESS( m_pCam )->GetFeatureByName( nameFrameID.c_str(), pFeatureFrameID )        // Get the values of:
             && VmbErrorSuccess == SP_ACCESS( pFeatureTimeStamp )->GetValue( nValueTimeStamp )                          // - the according timestamp event data
             && VmbErrorSuccess == SP_ACCESS( pFeatureFrameID )->GetValue( nValueFrameID )                              // - the according frame ID event data
             && VmbErrorSuccess == SP_ACCESS( pFeature )->GetValue( nValue ))                                           // - the feature itself
        {
            ++i;

            QString sMessage( name.c_str() );
            sMessage.   append( " (" ).append( QString::number( nValue )).append( ")     " ).append( "\t" ).
                        append( "Timestamp: " ).append( QString::number( nValueTimeStamp )).
                        append( ", FrameID: " ).append(QString::number(nValueFrameID));

            m_Message << sMessage;
            m_bIsEventRunning = true;

            if( MAX_EVENTS == i )
            {
                emit setEventMessage( m_Message );
                m_Message.clear();
                i = 0;
            }
        }
    }
}