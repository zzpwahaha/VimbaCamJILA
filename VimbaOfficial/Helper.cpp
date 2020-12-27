/*=============================================================================
  Copyright (C) 2012-2019 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        Helper.cpp

  Description: A Common helper class. Mostly used to define any constants and common methods.


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


#include "Helper.h"

#include <QtEndian>
#include <QStringList>

#include "memcpy_threaded.h"

/**copy construct from Vimba frame*/
tFrameInfo::tFrameInfo( const FramePtr& frame, bool color_interpolation )
    : BaseFrame( frame, color_interpolation )
{
    VmbError_t Result;
    VmbUchar_t* pData;
    Result = frame->GetImage( pData );
    if ( Result != VmbErrorSuccess )
    {
        std::runtime_error( "could not get frame data" );
    }
    m_pFrameData = FrameDataPtr( new VmbUchar_t[Size()], DeleteArray<VmbUchar_t> );
    if ( m_pFrameData == NULL )
    {
        throw std::bad_alloc();
    }
    memcpy_threaded<2>( m_pFrameData.data(), pData, Size() );
}

namespace Helper
{
const QString m_GIGE_STAT_FRAME_DELIVERED = "Stat Frames Delivered";
const QString m_GIGE_STAT_FRAME_DROPPED = "Stat Frames Dropped";
const QString m_GIGE_STAT_FRAME_RATE = "Stat Frame Rate";
const QString m_GIGE_STAT_FRAME_RESCUED = "Stat Frames Rescued";
const QString m_GIGE_STAT_FRAME_SHOVED = "Stat Frames Shoved";
const QString m_GIGE_STAT_FRAME_UNDERRUN = "Stat Frames Underrun";
const QString m_GIGE_STAT_LOCAL_RATE = "Stat Local Rate";
const QString m_GIGE_STAT_PACKET_ERRORS = "Stat Packets Errors";
const QString m_GIGE_STAT_PACKET_MISSED = "Stat Packets Missed";
const QString m_GIGE_STAT_PACKET_RECEIVED = "Stat Packets Received";
const QString m_GIGE_STAT_PACKET_REQUESTED = "Stat Packets Requested";
const QString m_GIGE_STAT_PACKET_RESENT = "Stat Packets Resent";
const QString m_GIGE_STAT_TIME_ELAPSED = "Stat Time Elapsed";


//
// The value sent by VmbAPI is big endian, e.g:
// Multicast IP Address: 823132143 = 0x 31 0F FF EF = 049.015.255.239
// and should be converted to little endian for GUI:  239.255.015.049
//
QString IPv4ToString( const VmbInt64_t nIPAddress, const bool swap )
{
    QString sIPv4;
    if( true == swap )
    {
        sIPv4 =     QString::number( 0xFF&nIPAddress ) + "."
                +   QString::number( 0xFF&(nIPAddress>>8) ) + "."
                +   QString::number( 0xFF&(nIPAddress>>16) ) + "."
                +   QString::number( 0xFF&(nIPAddress>>24) );
    }
    else
    {
        sIPv4 =     QString::number( 0xFF&(nIPAddress>>24) ) + "."
                +   QString::number( 0xFF&(nIPAddress>>16) ) + "."
                +   QString::number( 0xFF&(nIPAddress>>8) ) + "."
                +   QString::number( 0xFF&nIPAddress );
    }
    return sIPv4; 
}
//
// convert Mac address to string value format xx:xx:xx:xx:xx:xx
//
QString MACToString( const VmbInt64_t nNetworkOrderMacAddress )
{
    QString sIPv4;
    sIPv4 = QString().sprintf( "%02x:%02x:%02x:%02x:%02x:%02x",
        static_cast<unsigned int>(0xFF & (nNetworkOrderMacAddress >> 40)),
        static_cast<unsigned int>(0xFF & (nNetworkOrderMacAddress >> 32)),
        static_cast<unsigned int>(0xFF & (nNetworkOrderMacAddress >> 24)),
        static_cast<unsigned int>(0xFF & (nNetworkOrderMacAddress >> 16)),
        static_cast<unsigned int>(0xFF & (nNetworkOrderMacAddress >> 8)),
        static_cast<unsigned int>(0xFF & nNetworkOrderMacAddress) );
    return sIPv4;
}

QString displayValueToIPv4( const QString & sDecValue )
{
    VmbInt64_t    nValue64 = sDecValue.toLongLong();
    return IPv4ToString( nValue64, true );
}

QString displayValueToMAC( const QString & sMacAddress )
{
    VmbInt64_t    nValue64 = sMacAddress.toLongLong();
    return MACToString( nValue64 );
}

// maps return codes to explanation strings for output
QString mapReturnCodeToString( const VmbError_t nReturnCode )
{
    QString sResult = "(Return code undefined)";

    switch ( nReturnCode )
    {
        case VmbErrorSuccess: sResult = "(No error)"; break;
        case VmbErrorInternalFault: sResult = "(Unexpected fault in Vimba API or driver)"; break;
        case VmbErrorApiNotStarted: sResult = "(VmbStartup() was not called before the current command)"; break;
        case VmbErrorNotFound: sResult = "(The designated instance (camera, feature etc.) cannot be found)"; break;
        case VmbErrorBadHandle: sResult = "(The given handle is not valid)"; break;
        case VmbErrorDeviceNotOpen: sResult = "(Device was not opened for usage)"; break;
        case VmbErrorInvalidAccess: sResult = "(Operation is invalid with the current access mode)"; break;
        case VmbErrorBadParameter: sResult = "(One of the parameters was invalid (usually an illegal pointer))"; break;
        case VmbErrorStructSize: sResult = "(The given struct size is not valid for this version of the API)"; break;
        case VmbErrorMoreData: sResult = "(More data was returned in a string/list than space was provided)"; break;
        case VmbErrorWrongType: sResult = "(The feature type for this access function was wrong)"; break;
        case VmbErrorInvalidValue: sResult = "(The value was not valid; either out of bounds or not an increment of the minimum)"; break;
        case VmbErrorTimeout: sResult = "(Timeout during wait)"; break;
        case VmbErrorOther: sResult = "(Other error)"; break;
        case VmbErrorResources: sResult = "(Resources not available (e.g memory))"; break;
        case VmbErrorInvalidCall: sResult = "(Call is invalid in the current context (e.g callback))"; break;
        case VmbErrorNoTL: sResult = "(No transport layers were found)"; break;
        case VmbErrorNotImplemented: sResult = "(API feature is not implemented)"; break;
        case VmbErrorNotSupported: sResult = "(API feature is not supported)"; break;
        case VmbErrorIncomplete: sResult = "(A multiple register read or write was partially completed)"; break;
        case 7000: sResult = "<Viewer Timeout>";
        default: break;
    }

    return sResult;
}

QString convertFormatToString( const VmbPixelFormatType format )
{
    QString sFormat;

    switch ( format )
    {
        /* Monochrome, 8 bits */
        case VmbPixelFormatMono8:
            sFormat = "Mono8";
            break;

            /* Monochrome, 10 bits in 16 bits */
        case VmbPixelFormatMono10:
            sFormat = "Mono10";
            break;

            /* Monochrome, 10 bits packed */
        case VmbPixelFormatMono10p:
            sFormat = "Mono10p";
            break;

            /* Monochrome, 12 bits in 16 bits */
        case VmbPixelFormatMono12:
            sFormat = "Mono12";
            break;

            /* Monochrome, 12 bits packed */
        case VmbPixelFormatMono12p:
            sFormat = "Mono12p";
            break;

            /* Monochrome, 2x12 bits in 24 bits */
        case VmbPixelFormatMono12Packed:
            sFormat = "Mono12Packed";
            break;

            /* Monochrome, 14 bits in 16 bits */
        case VmbPixelFormatMono14:
            sFormat = "Mono14";
            break;

            /* Monochrome, 16 bits */
        case VmbPixelFormatMono16:
            sFormat = "Mono16";
            break;

            /* Bayer-color, 8 bits */
        case VmbPixelFormatBayerGR8:
            sFormat = "BayerGR8";
            break;

            /* Bayer-color, 8 bits */
        case VmbPixelFormatBayerRG8:
            sFormat = "BayerRG8";
            break;

            /* Bayer-color, 8 bits */
        case VmbPixelFormatBayerGB8:
            sFormat = "BayerGB8";
            break;

            /* Bayer-color, 8 bits */
        case VmbPixelFormatBayerBG8:
            sFormat = "BayerBG8";
            break;

            /* Bayer-color, 10 bits in 16 bits */
        case VmbPixelFormatBayerGR10:
            sFormat = "BayerGR10";
            break;

            /* Bayer-color, 10 bits in 16 bits */
        case VmbPixelFormatBayerRG10:
            sFormat = "BayerRG10";
            break;

            /* Bayer-color, 10 bits in 16 bits */
        case VmbPixelFormatBayerGB10:
            sFormat = "BayerGB10";
            break;

            /* Bayer-color, 10 bits in 16 bits */
        case VmbPixelFormatBayerBG10:
            sFormat = "BayerBG10";
            break;

            /* Bayer-color, 10 bits packed*/
        case VmbPixelFormatBayerGR10p:
            sFormat = "BayerGR10p";
            break;

            /* Bayer-color, 10 bits packed */
        case VmbPixelFormatBayerRG10p:
            sFormat = "BayerRG10p";
            break;

            /* Bayer-color, 10 bits packed */
        case VmbPixelFormatBayerGB10p:
            sFormat = "BayerGB10p";
            break;

            /* Bayer-color, 10 bits packed */
        case VmbPixelFormatBayerBG10p:
            sFormat = "BayerBG10p";
            break;

            /* Bayer-color, 12 bits in 16 bits */
        case VmbPixelFormatBayerGR12:
            sFormat = "BayerGR12";
            break;

            /* Bayer-color, 12 bits in 16 bits */
        case VmbPixelFormatBayerRG12:
            sFormat = "BayerRG12";
            break;

            /* Bayer-color, 12 bits in 16 bits */
        case VmbPixelFormatBayerGB12:
            sFormat = "BayerGB12";
            break;

            /* Bayer-color, 12 bits in 16 bits */
        case VmbPixelFormatBayerBG12:
            sFormat = "BayerBG12";
            break;

            /* Bayer-color, 12 bits packed*/
        case VmbPixelFormatBayerGR12p:
            sFormat = "BayerGR12p";
            break;

            /* Bayer-color, 12 bits packed */
        case VmbPixelFormatBayerRG12p:
            sFormat = "BayerRG12p";
            break;

            /* Bayer-color, 12 bits packed */
        case VmbPixelFormatBayerGB12p:
            sFormat = "BayerGB12p";
            break;

            /* Bayer-color, 12 bits packed */
        case VmbPixelFormatBayerBG12p:
            sFormat = "BayerBG12p";
            break;

            /* RGB, 8 bits x 3 */
        case VmbPixelFormatRgb8:
            sFormat = "RGB8";
            break;

            /* BGR, 8 bits x 3 */
        case VmbPixelFormatBgr8:
            sFormat = "BGR8";
            break;

            /* ARGB, 8 bits x 4 */
        case VmbPixelFormatArgb8:
            sFormat = "ARGB8";
            break;

            /* BGRA, 8 bits x 4 */
        case VmbPixelFormatBgra8:
            sFormat = "BGRA8";
            break;

            /* RGB, 16 bits x 3 */
        case VmbPixelFormatRgb16:
            sFormat = "RGB16";
            break;

            /* YUV 411 with 8 bits */
        case VmbPixelFormatYuv411:
            sFormat = "YUV411";
            break;

            /* YUV 411 with 8 bits */
        case VmbPixelFormatYCbCr411_8_CbYYCrYY:
            sFormat = "YCbCr411_8_CbYYCrYY";
            break;

            /* YUV  422 */
        case VmbPixelFormatYuv422:
            sFormat = "YUV422";
            break;
            /* YUV  422 */
        case VmbPixelFormatYCbCr422_8_CbYCrY:
            sFormat = "YCbCr422_8_CbYCrY";
            break;

            /* YUV 444 */
        case VmbPixelFormatYuv444:
            sFormat = "YUV444";
            break;

            /* YUV 444 */
        case VmbPixelFormatYCbCr8_CbYCr:
            sFormat = "YCbCr8_CbYCr";
            break;

            /* BAYERGR12PACKED Bayer-color, 2x12 bits in 24 bits, starting with GR line*/
        case VmbPixelFormatBayerGR12Packed:
            sFormat = "BayerGR12Packed";
            break;

            /* BAYERRG12PACKED Bayer-color, 2x12 bits in 24 bits, starting with RG line */
        case VmbPixelFormatBayerRG12Packed:
            sFormat = "BayerRG12Packed";
            break;

            /* BAYERGB12PACKED Bayer-color, 2x12 bits in 24 bits, starting with GB line*/
        case VmbPixelFormatBayerGB12Packed:
            sFormat = "BayerGB12Packed";
            break;

            /* BAYERBG12PACKED Bayer-color, 2x12 bits in 24 bits, starting with BG line*/
        case VmbPixelFormatBayerBG12Packed:
            sFormat = "BayerBG12Packed";
            break;

            /* Bayer-color, 16 bits, starting with GR line*/
        case VmbPixelFormatBayerGR16:
            sFormat = "BayerGR16";
            break;

            /* Bayer-color, 16 bits, starting with RG line */
        case VmbPixelFormatBayerRG16:
            sFormat = "BayerRG16";
            break;

            /* Bayer-color, 16 bits, starting with GB line */
        case VmbPixelFormatBayerGB16:
            sFormat = "BayerGB16";
            break;

            /* Bayer-color, 16 bits, starting with BG line */
        case VmbPixelFormatBayerBG16:
            sFormat = "BayerBG16";
            break;

        default:
            sFormat = " ";
            break;
    }

    return sFormat;
}

bool needsIPv4Format( const QString & sFeatureName )
{
    if ( sFeatureName == "Multicast IP Address" ||
        sFeatureName == "MulticastIPAddress" ||
        sFeatureName == "Current Subnet Mask" ||
        sFeatureName == "GevCurrentSubnetMask" ||
        sFeatureName == "Current IP Address" ||
        sFeatureName == "GevCurrentIPAddress" ||
        sFeatureName == "Current Default Gateway" ||
        sFeatureName == "GevCurrentDefaultGateway" ||
        sFeatureName == "Persistent IP Address" ||
        sFeatureName == "GevPersistentIPAddress" ||
        sFeatureName == "Persistent Default Gateway" ||
        sFeatureName == "GevPersistentDefaultGateway" ||
        sFeatureName == "Persistent Subnet Mask" ||
        sFeatureName == "GevPersistentSubnetMask" )
    {
        return true;
    }
    return false;
}

bool needsIPv4Format( const QString & sFeatureName, const std::string & sRepresentation )
{
    if ( sRepresentation == "IPV4Address" || needsIPv4Format( sFeatureName ) )
    {
        return true;
    }
    return false;
}

bool isAutoFeature( const QString & sFeatureName )
{
    return sFeatureName == "ExposureAuto"
        || sFeatureName == "Exposure Auto"
        || sFeatureName == "GainAuto"
        || sFeatureName == "Gain Auto"
        || sFeatureName == "Balance White Auto"
        || sFeatureName == "BalanceWhiteAuto";
}

VmbInt64_t StringToIPv4( const QString & sIPString, const bool swap )
{
    VmbInt64_t ipAddress = 0;
    VmbInt64_t temp = 0;
    QStringList ipParts;
    bool check = false;

    ipParts = sIPString.split( "." );

    if ( 4 == ipParts.size() )
    {
        if( swap )
        {
            // swap whole list for network byte order
            for ( int k = 0; k < (ipParts.size() / 2); k++ )
            {
                ipParts.swap( k, ipParts.size() - (1 + k) );
            }
        }

        for ( int i = 0; i < ipParts.size(); ++i )
        {
            temp = ipParts.at( i ).toUInt( &check, 10 );
            if ( false == check || temp > 255 )
            {
                break;
            }
            ipAddress = (ipAddress << 8) + temp;
        }
    }

    if ( false == check || temp > 255)
    {
        ipAddress = -1;
    }

    return ipAddress;
}

VmbInt64_t StringToMAC( const QString& sMACString )
{
    VmbInt64_t macAddress = 0;
    QStringList macParts;
    VmbUint32_t temp = 0;
    bool check = false;

    macParts = sMACString.split( ":" );
    if ( true == macParts.empty() )
    {
        macParts = sMACString.split( "-" );
    }

    if ( 6 == macParts.size() )
    {
        for ( int i=0; i<macParts.size(); ++i )
        {
            temp = macParts.at(i).toUInt( &check, 16 );
            if ( false == check )
            {
                break;
            }
            macAddress = (macAddress << 8) + temp;
        }
    }

    if ( false == check )
    {
        macAddress = -1;
    }

    return macAddress;
}

} // namespace Helper
