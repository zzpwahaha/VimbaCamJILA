#pragma once
#include <string>
#include "Accessory/imageParameters.h"
#include "VimbaCPP/Include/VimbaSystem.h"
#include <qstringlist.h>
#include <QMetaType>

//struct CameraInfo
//{
//	//enum class name {
//	//	Mako1,Mako2
//	//};
//	//static const std::array<name, MAKO_NUMBER> allCams;
//	//CameraInfo::name camName;
//	std::string ip;
//	std::string delim;
//	const bool safemode = false;
//
//	//static std::string toStr(name m_);
//	//static CameraInfo::name fromStr(std::string txt);
//
//};


using namespace AVT::VmbAPI;
using AVT::VmbAPI::CameraPtrVector;
class CameraInfo
{
private:
    CameraPtr       m_Cam;
    QString         m_DisplayName;
    bool            m_IsOpen;
    QStringList     m_PermittedAccess;
    QStringList     m_PermittedAccessState;
public:
    CameraInfo()
        : m_IsOpen(false)
    {}
    explicit CameraInfo(CameraPtr cam, const QString& name)
        : m_Cam(cam)
        , m_DisplayName(name)
        , m_IsOpen(false)
    {}
    const CameraPtr& Cam()            const { return m_Cam; }
    CameraPtr& Cam() { return m_Cam; }
    const QString& DisplayName()      const { return m_DisplayName; }
    bool  IsOpen()                    const { return m_IsOpen; }
    void  SetOpen(bool s) { m_IsOpen = s; }
    const QStringList& PermittedAccess()   const { return m_PermittedAccess; }
    void               PermittedAccess(const QStringList& l) { m_PermittedAccess = l; }
    const QStringList& PermittedAccessState()   const { return m_PermittedAccessState; }
    void               PermittedAccessState(const QStringList& l) { m_PermittedAccessState = l; }
    void SetPermittedAccessState(unsigned int pos, QString status)
    {
        m_PermittedAccessState[pos] = status;
    }
    void ResetPermittedAccessState()
    {
        for (int pos = 0; pos < m_PermittedAccessState.size(); ++pos)
        {
            m_PermittedAccessState[pos] = "false";
        }
    }


    bool operator==(const CameraPtr& other)   const { return SP_ACCESS(other) == SP_ACCESS(m_Cam); }
    bool operator<(const CameraPtr& other)    const { return SP_ACCESS(other) < SP_ACCESS(m_Cam); }

    VmbInterfaceType InterfaceType() const
    {
        VmbInterfaceType interfaceType;
        VmbErrorType result = SP_ACCESS(m_Cam)->GetInterfaceType(interfaceType);
        if (result != VmbErrorSuccess)
        {
            throw std::runtime_error("could not read interface type from camera");
        }
        return interfaceType;
    }
    QString InterfaceTypeString() const
    {
        VmbInterfaceType interfaceType;
        VmbErrorType result = SP_ACCESS(m_Cam)->GetInterfaceType(interfaceType);
        if (result != VmbErrorSuccess)
        {
            throw std::runtime_error("could not read interface type from camera");
        }
        switch (interfaceType)
        {
        case VmbInterfaceEthernet:
            return "GigE";
        case VmbInterfaceFirewire:
            return "1394";
        case VmbInterfaceUsb:
            return "USB";
        case VmbInterfaceCL:
            return "CL";
        case VmbInterfaceCSI2:
            return "CSI2";
        }
    }
};


struct CMOSAutoExposure {
	enum class mode {
		Continuous,
		Off,
		Once
	};
	static std::string toStr(mode m);
	static mode fromStr(std::string txt);
};


struct CMOSTrigger {
	enum class mode {
		External,
		AutomaticSoftware,
		ManualSoftware,
		ContinuousSoftware
	};
	virtual std::string toStr(mode m);
	virtual mode fromStr(std::string txt);
};

struct CMOSAcquisition {
	enum class mode {
		Finite,
		Continuous
	};
	static std::string toStr(mode m);
	static mode fromStr(std::string m);
};


struct CMOSSettings {
	bool on;
	bool expActive;
	unsigned int rawGain;
	CMOSAutoExposure::mode exposureMode = CMOSAutoExposure::mode::Off;
	double exposureTime;
	CMOSAcquisition::mode acquisitionMode = CMOSAcquisition::mode::Continuous;
	unsigned picsPerRep;
	unsigned repsPerVar;
	unsigned variations;
	CMOSTrigger::mode triggerMode = CMOSTrigger::mode::External;
	double frameRate;
	imageParameters dims;
	unsigned totalPictures() { return picsPerRep * repsPerVar * variations; }
};

struct MakoTrigger : public CMOSTrigger
{
	static std::string toStr(mode m);
	static mode fromStr(std::string txt);
};

struct MakoSettings {
	bool on;
	bool expActive = false;
	unsigned int rawGain;
	bool trigOn;
	MakoTrigger::mode triggerMode = MakoTrigger::mode::External;
	double exposureTime;
	double frameRate;
	imageParameters dims; // mako do not do binning
	unsigned picsPerRep;
	unsigned repsPerVar;
	unsigned variations;
	unsigned totalPictures() { return picsPerRep * repsPerVar * variations; }
};


Q_DECLARE_METATYPE(MakoSettings*)
Q_DECLARE_METATYPE(CameraInfo)