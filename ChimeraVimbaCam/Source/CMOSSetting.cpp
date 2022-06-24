#include "stdafx.h"
#include "CMOSSetting.h"

//const std::array<CameraInfo::name, MAKO_NUMBER> CameraInfo::allCams = { name::Mako1, name::Mako2 };

//std::string CameraInfo::toStr(name m_)
//{
//	switch (m_) {
//	case CameraInfo::name::Mako1:
//		return "Mako1";
//	case CameraInfo::name::Mako2:
//		return "Mako2";
//	}
//	return "";
//}
//
//CameraInfo::name CameraInfo::fromStr(std::string txt)
//{
//	for (auto opt : allCams) {
//		if (toStr(opt) == txt) {
//			return opt;
//		}
//	}
//	thrower("Failed to convert string to CMOS option!");
//	return CameraInfo::name::Mako1;
//}



std::string CMOSAutoExposure::toStr(CMOSAutoExposure::mode m) {
	switch (m) {
	case mode::Continuous:
		return "Auto-Exposure-Continous";
	case mode::Once:
		return "Auto-Exposure-Once";
	case mode::Off:
		return "Auto-Exposure-Off";
	default:
		return "None";
	}
}

CMOSAutoExposure::mode CMOSAutoExposure::fromStr(std::string txt) {
	for (auto m : { mode::Continuous, mode::Once, mode::Off }) {
		if (txt == toStr(m)) {
			return m;
		}
	}
	errBox("Failed to convert text (" + txt + ") to balser auto exposure mode! defaulting to auto exposure off.");
	return CMOSAutoExposure::mode::Off;
}


std::string CMOSTrigger::toStr(CMOSTrigger::mode m) {
	switch (m) {
	case mode::External:
		return "External-Trigger";
	case mode::AutomaticSoftware:
		return "Automatic-Software-Trigger";
	case mode::ManualSoftware:
		return "Manual-Software-Trigger";
	case mode::ContinuousSoftware:
		return "Continuous-Software-Streaming";
	default:
		return "None";
	}
}


CMOSTrigger::mode CMOSTrigger::fromStr(std::string txt) {
	for (auto m : { mode::External, mode::ManualSoftware, mode::AutomaticSoftware, mode::ContinuousSoftware }) {
		if (txt == toStr(m)) {
			return m;
		}
	}
	// doesn't match any.
	errBox("Failed to convert text (" + txt + ") to balser trigger mode! defaulting to external trigger.");
	return CMOSTrigger::mode::External;
}

CMOSAcquisition::mode CMOSAcquisition::fromStr(std::string txt) {
	for (auto m : { mode::Continuous, mode::Finite }) {
		if (txt == toStr(m)) {
			return m;
		}
	}
	errBox("Failed to convert text (" + txt + ") to balser acquisition mode! defaulting to Finite acquisition.");
	return CMOSAcquisition::mode::Finite;
}

std::string CMOSAcquisition::toStr(CMOSAcquisition::mode m) {
	switch (m) {
	case mode::Finite:
		return "Finite-Acquisition";
	case mode::Continuous:
		return "Continuous-Acquisition";
	default:
		return "None";
	}
}

std::string MakoTrigger::toStr(mode m)
{
	switch (m) {
	case mode::External:
		return "Line1";
	case mode::AutomaticSoftware:
		return "FixedRate";
	case mode::ManualSoftware:
		return "Software";
	case mode::ContinuousSoftware:
		return "Freerun";
	default:
		return "None";
	}
}

MakoTrigger::mode MakoTrigger::fromStr(std::string txt)
{
	for (auto m : { mode::External, mode::ManualSoftware, mode::AutomaticSoftware, mode::ContinuousSoftware }) {
		if (txt == toStr(m)) {
			return m;
		}
	}
	// doesn't match any.
	errBox("Failed to convert text (" + txt + ") to Mako trigger mode! defaulting to sofware fixedrate trigger.");
	return CMOSTrigger::mode::AutomaticSoftware;
}
