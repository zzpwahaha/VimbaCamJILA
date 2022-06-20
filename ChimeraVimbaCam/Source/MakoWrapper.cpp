#include "stdafx.h"
#include "MakoWrapper.h"
#include <CMOSCamera/Helper.h>
#include <qendian.h>

InterfacePtr MakoWrapper::getInterfaceByID(VimbaSystem& vsys, std::string sInterfaceID)
{
    InterfacePtr interfacePtr;
    VmbErrorType error = vsys.GetInterfaceByID(sInterfaceID.c_str(), interfacePtr);
    if (VmbErrorSuccess != error) {
        thrower("GetType <Interface> by ID Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    return interfacePtr;
}

FeaturePtr MakoWrapper::getInterfaceFeatureByName(InterfacePtr interfaceP, std::string featurename)
{
    FeaturePtr featurePtr; 
    VmbErrorType error = interfaceP->GetFeatureByName(featurename.c_str(), featurePtr);
    if (VmbErrorSuccess != error) {
        thrower("GetType <Feature> by feature name Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    return featurePtr;
}

std::string MakoWrapper::getFeatureValue(FeaturePtr featPtr)
{
    VmbInt64_t  nValue64 = 0;
    double      dValue = 0;
    bool        bValue = false;
    std::string stdValue;

    std::string sValue;

    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    VmbError_t error = featPtr->GetDataType(dataType);

    if (VmbErrorSuccess == error)
    {
        switch (dataType)
        {
        case VmbFeatureDataInt:
            error = featPtr->GetValue(nValue64);
            if (VmbErrorSuccess == error || VmbErrorInvalidAccess == error){
                sValue = str(nValue64); 
            }
            else{ 
                thrower("Get feature value int Failed, Error: " + str(Helper::mapReturnCodeToString(error))); 
            }
            break;

        case VmbFeatureDataFloat:
            error = featPtr->GetValue(dValue);
            if (VmbErrorSuccess == error || VmbErrorInvalidAccess == error) {
                sValue = str(dValue, 4);
            }
            else {
                thrower("Get feature value double Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
            break;

        case VmbFeatureDataEnum:
            error = featPtr->GetValue(stdValue);
            if (VmbErrorSuccess == error || VmbErrorInvalidAccess == error) {
                sValue = stdValue;
            }
            else {
                thrower("Get feature value string Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
            break;

        case VmbFeatureDataString:
            error = featPtr->GetValue(stdValue);
            if (VmbErrorSuccess == error || VmbErrorInvalidAccess ==error) {
                sValue = stdValue;
            }
            else {
                thrower("Get feature value string Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
            break;

        case VmbFeatureDataBool:
            error = featPtr->GetValue(bValue);
            if (VmbErrorSuccess == error || VmbErrorInvalidAccess == error) {
                bValue ? sValue = "true" : sValue = "false";
            }
            else {
                thrower("Get feature value string bool Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
        case VmbFeatureDataCommand:
            sValue = "[COMMAND]";
            break;

        case VmbFeatureDataRaw:
            sValue = "Click here to open";
            break;
        default: break;
        }
    }
    else {
        thrower("Get feature type Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    return sValue;
}


void MakoWrapper::setIntegerValue(FeaturePtr featPtr, long long val)
{
    long long min, max;
    VmbErrorType error;
    error = featPtr->GetRange(min, max);
    if (VmbErrorSuccess != error) {
        thrower("get feature range Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    if (val < min) { val = min; }
    if (val > max) { val = max; }

    error = featPtr->SetValue(val);
    if (VmbErrorSuccess != error) {
        thrower("Set int feature value Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
}

void MakoWrapper::setFloatingValue(FeaturePtr featPtr, double dValue)
{
    double min, max;
    VmbErrorType error;
    error = featPtr->GetRange(min, max);
    if (VmbErrorSuccess != error) {
        thrower("get feature range Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    if (dValue < min) { dValue = min; }
    if (dValue > max) { dValue = max; }
    error = featPtr->SetValue(dValue);
    if (VmbErrorSuccess != error) {
        thrower("Set double feature value Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
}

std::string MakoWrapper::getFeatureInformation(FeaturePtr featPtr)
{
    std::string sInformation;
    VmbInt64_t nMin = 0, nMax = 0, nInc = 0;
    double dMin = 0, dMax = 0, dInc = 0;

    std::string sFeatureName;
    featPtr->GetName(sFeatureName);
    sInformation += "<b>FEATURE NAME:</b> " + sFeatureName + "<br/>";

    VmbFeatureVisibilityType visibility = VmbFeatureVisibilityUnknown;
    featPtr->GetVisibility(visibility);
    sInformation += "<b>VISIBILITY:</b> ";
    switch (visibility)
    {
    case VmbFeatureVisibilityUnknown:   sInformation.append("UNKNOWN<br/>");   break;
    case VmbFeatureVisibilityBeginner:  sInformation.append("BEGINNER<br/>");  break;
    case VmbFeatureVisibilityExpert:    sInformation.append("EXPERT<br/>");    break;
    case VmbFeatureVisibilityGuru:      sInformation.append("GURU<br/>");      break;
    case VmbFeatureVisibilityInvisible: sInformation.append("INVISIBLE<br/>"); break;
    default: sInformation.append("N/A<br/>"); break;
    }

    /* get feature type and type-specific info */
    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    VmbFeatureFlagsType flags;
    VmbError_t error = featPtr->GetDataType(dataType);
    if (VmbErrorSuccess == error)
    {
        switch (dataType)
        {
        case VmbFeatureDataInt:
        {
            /* only show range and increment for integer features that might change */
            if ((VmbErrorSuccess == featPtr->GetFlags(flags)) && ((((VmbFeatureFlagsVolatile | VmbFeatureFlagsWrite | VmbFeatureFlagsModifyWrite) & flags) != 0) || (VmbFeatureFlagsRead == flags)))
            {
                if (VmbErrorSuccess == featPtr->GetRange(nMin, nMax))
                    sInformation += "<b>TYPE:</b> Integer<br/><b>MINIMUM:</b> " + str(nMin) + "<br/><b>MAXIMUM:</b> " + str(nMax) + "<br/>";
                if ((VmbErrorSuccess == featPtr->GetIncrement(nInc)) && (1 != nInc))
                    sInformation += "<b>INTERVAL:</b> " + str(nInc) + "<br/>";
            }
            break;
        }
        case VmbFeatureDataFloat:
        {
            /* only show range and increment for float features that might change */
            VmbFeatureFlagsType flags;
            if ((VmbErrorSuccess == featPtr->GetFlags(flags)) && ((((VmbFeatureFlagsVolatile | VmbFeatureFlagsWrite | VmbFeatureFlagsModifyWrite) & flags) != 0) || (VmbFeatureFlagsRead == flags)))
            {
                if (VmbErrorSuccess == featPtr->GetRange(dMin, dMax))
                    sInformation += "<b>TYPE:</b> Float<br/><b>MINIMUM:</b> " + str(QString::number(dMin, 'g', 9)) + "<br/><b>MAXIMUM:</b> " + str(QString::number(dMax, 'g', 12)) + "<br/>";

                if (VmbErrorSuccess == featPtr->GetIncrement(dInc))
                    sInformation += "<b>INTERVAL:</b> " + str(QString::number(dInc, 'f', 10)) + "<br/>";
            }
            break;
        }
        case VmbFeatureDataEnum:    sInformation.append("<b>TYPE:</b> Enumeration<br/>"); break;
        case VmbFeatureDataString:  sInformation.append("<b>TYPE:</b> String<br/>");      break;
        case VmbFeatureDataBool:    sInformation.append("<b>TYPE:</b> Boolean<br/>");     break;
        case VmbFeatureDataCommand: sInformation.append("<b>TYPE:</b> Command<br/>");     break;
        case VmbFeatureDataRaw:     sInformation.append("<b>TYPE:</b> Raw<br/>");     break;
        default: break;
        }
    }

    std::string sCategory;
    featPtr->GetCategory(sCategory);
    sInformation += "<b>CATEGORY:</b> " + sCategory + "<br/>";

    FeaturePtrVector           featPtrVec;

    sInformation += "<br/><b>AFFECTED FEATURE(S):</b> ";
    featPtr->GetAffectedFeatures(featPtrVec);
    for (unsigned int i = 0; i < featPtrVec.size(); i++)
    {
        std::string sName;
        featPtrVec.at(i)->GetName(sName);

        if (0 == i)
            sInformation.append("<br/>");

        sInformation += sName;
        if (i + 1 != featPtrVec.size())
        {
            sInformation.append(", ");
            if (0 == ((i + 1) % 4) && (i != 0))
                sInformation.append("<br/>");
        }
    }

    if (0 == featPtrVec.size())
        sInformation += "N/A";

    sInformation.append("<br/>");
    return sInformation;
}

bool MakoWrapper::isEventFeature(FeaturePtr pFeature)
{
    std::string sCategory;
    if (!SP_ISNULL(pFeature)
        && VmbErrorSuccess == SP_ACCESS(pFeature)->GetCategory(sCategory)
        && std::strstr(sCategory.c_str(), "/EventID"))
    {
        return true;
    }
    else
    {
        return false;
    }
}


/*below two functions are directly copied from Vimba example and I do not want to mess up with it (even though it is definitly optimizable)*/
VmbErrorType MakoWrapper::getCameraDisplayName(const CameraPtr& camera, std::string& sDisplayName)
{
    VmbErrorType  error;
    std::string sID;
    std::string sSN;

    error = camera->GetModel(sDisplayName);
    if (VmbErrorSuccess != error) {
        thrower("GetModel Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        return error;
    }
    else
    {
        error = camera->GetSerialNumber(sSN);
        if (VmbErrorSuccess != error)
        {
            thrower("GetSerialNumber Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            return error;
        }
        else
        {
            error = camera->GetID(sID);
            if (VmbErrorSuccess != error)
            {
                thrower("GetID Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
                return error;
            }
            else
            {
                std::string sDisplayNameEnding;

                sDisplayNameEnding.append("-");
                sDisplayNameEnding.append(sSN);

                if (0 != sSN.compare(sID))
                {
                    sDisplayNameEnding.append("(");
                    sDisplayNameEnding.append(sID);
                    sDisplayNameEnding.append(")");
                }

                std::string sLegacyDisplayName = sDisplayName + sDisplayNameEnding;

                VmbInterfaceType cameraIFType;
                error = camera->GetInterfaceType(cameraIFType);

                if (VmbErrorSuccess != error) {
                    thrower("GetInterfaceType Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
                    return error;
                }

                // camera interface type is GigE. update display name with IP address 
                if (VmbInterfaceEthernet == cameraIFType)
                {
                    // lookup the IP address of the camera         
                    std::string sIPAddress;
                    error = getIPAddress(camera, sIPAddress);

                    // replace the model ID with the IP address
                    if (VmbErrorSuccess == error && !sIPAddress.empty())
                    {
                        QString sTempDisplayName = QString::fromStdString(sDisplayName);
                        QRegExp regExp("\\(([^)]+)\\)");

                        // to account for cameras without model name in parenthesis
                        if (-1 == regExp.indexIn(sTempDisplayName))
                        {
                            sDisplayName.append(sIPAddress);
                        }
                        else
                        {
                            sTempDisplayName.replace(regExp, qstr(sIPAddress));
                            sDisplayName = sTempDisplayName.toStdString();
                        }

                        sDisplayName.append(sDisplayNameEnding);
                    }
                }
                // other camera interface types use legacy naming convention
                else
                {
                    sDisplayName = sLegacyDisplayName;
                }
            }
        }
    }
    return error;
}

VmbErrorType MakoWrapper::getIPAddress(const AVT::VmbAPI::CameraPtr& camera, std::string& sIPAdress)
{
    VmbErrorType error;
    std::string sCameraID, sInterfaceID, sDeviceID;
    InterfacePtr   pInterface;
    FeaturePtr     pSelectorFeature, pSelectedDeviceID, pSelectedIPAddress;
    VmbInt64_t                  nMinRange, nMaxRange, nIP;
    VmbInt32_t                  nIP_32;

    // get the camera ID
    error = camera->GetID(sCameraID);
    if (VmbErrorSuccess == error)
    {
        // get the interface ID
        error = camera->GetInterfaceID(sInterfaceID);
        if (VmbErrorSuccess == error)
        {
            // get a pointer to the interface
            error = AVT::VmbAPI::VimbaSystem::GetInstance().GetInterfaceByID(sInterfaceID.c_str(), pInterface);
            if (VmbErrorSuccess == error)
            {
                // open the interface 
                error = pInterface->Open();
                if (VmbErrorSuccess == error)
                {
                    // get the device selector
                    error = pInterface->GetFeatureByName("DeviceSelector", pSelectorFeature);
                    if (VmbErrorSuccess == error)
                    {
                        // get the range of the available devices 
                        error = pSelectorFeature->GetRange(nMinRange, nMaxRange);

                        // check for negative range in case requested feature contains no items
                        if (VmbErrorSuccess == error && nMaxRange >= 0)
                        {
                            // get the device ID pointer
                            error = pInterface->GetFeatureByName("DeviceID", pSelectedDeviceID);
                            if (VmbErrorSuccess == error)
                            {
                                // get IP addresses of all cameras connected to interface
                                error = pInterface->GetFeatureByName("GevDeviceIPAddress", pSelectedIPAddress);
                                if (VmbErrorSuccess == error)
                                {
                                    // find the IP address of the desired camera 
                                    for (VmbInt64_t intNo = nMinRange; intNo <= nMaxRange; ++intNo)
                                    {
                                        error = pSelectorFeature->SetValue(intNo);
                                        if (VmbErrorSuccess == error)
                                        {
                                            error = pSelectedDeviceID->GetValue(sDeviceID);
                                            if (VmbErrorSuccess == error)
                                            {
                                                if (0 == sDeviceID.compare(sCameraID))
                                                {
                                                    error = pSelectedIPAddress->GetValue(nIP);
                                                    if (VmbErrorSuccess == error)
                                                    {
                                                        nIP_32 = static_cast<VmbInt32_t>(nIP);

                                                        // format IP address string
                                                        sIPAdress = str(QString("(%1)").arg(Helper::IPv4ToString(qFromBigEndian(nIP_32), true)));

                                                        // close the interface
                                                        error = pInterface->Close();
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            sIPAdress.clear();
                            error = VmbErrorNotFound;
                        }

                    }
                }
            }
        }
    }

    return error;
}
