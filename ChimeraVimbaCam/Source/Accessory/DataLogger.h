// created by Mark O. Brown
#pragma once
//#include "RealTimeDataAnalysis/DataAnalysisControl.h"					  
//#include "Andor/CameraImageDimensions.h"
//#include "Andor/AndorRunSettings.h"
//#include <Accessory/Matrix.h>
//#include <DigitalOutput/DoCore.h>
#include <Accessory/IChimeraSystem.h>
#include <CMOSSetting.h>
// there's potentially a typedef conflict with a python file which also typedefs ssize_t.
#define ssize_t h5_ssize_t
#include "H5Cpp.h"
#undef ssize_t
#include <vector>
#include <string>
#include <functional>

//class AoSystem;
//class DdsSystem;
//class OlSystem;
//struct ExperimentThreadInput;
/*
 * Handles the writing of h5 files. Some parts of this are effectively HDF5 wrappers.
 */

class cameraMainWindow;
class DataLogger : public IChimeraSystem {
	public:
		DataLogger(std::string systemLocation, cameraMainWindow* parent);
		~DataLogger( );
		//void logMasterRuntime ( unsigned repNumber, std::vector<parameterType> params );
		void logError ( H5::Exception& err );
		void initializeDataFiles( std::string filePath, bool needsCal=true);
		//void writeAndorPic( Matrix<long> image, imageParameters dims );
		void writeMakoPic(std::vector<double> image, int width, int height);
		void writeTemperature(std::pair<std::vector<long long>, std::vector<double>> timedata, std::string identifier );
		//void writeVolts ( unsigned currentVoltNumber, std::vector<float64> data );
		//void assertCalibrationFilesExist ();
		//void logMasterInput( ExperimentThreadInput* input );
		//void logMiscellaneousStart();
		//void logParameters( const std::vector<parameterType>& variables, H5::Group& group );
		//void logFunctions( H5::Group& group );
		//void logAoSystemSettings ( AoSystem& aoSys);
		//void logDoSystemSettings ( DoCore& doSys );
		//void logOlSystemSettings(OlSystem& aoSys);
		//void logDdsSystemSettings(DdsSystem& ddsSys);
		//void logPlotData ( std::string name );
		//int getCalibrationFileIndex ();
		static void getDataLocation ( std::string base, std::string& todayFolder, std::string& fullPath );
		void normalCloseFile();
		void deleteFile();
		int getDataFileNumber( );
		void assertClosed ();
		std::string getFullError (H5::Exception& err);
		//void initOptimizationFile ( );
		//void updateOptimizationFile ( std::string appendTxt );
		//void finOptimizationFile ( );
		bool andorDataSetShouldBeValid = false;
		unsigned getNextFileNumber ( );
		std::string getMostRecentDateString ( );
		// the core file.
		H5::H5File file;
		// a bunch of overloaded wrapper functions for making the main "log" functions above much cleaner.
		H5::DataSet writeDataSet (bool data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (unsigned data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (unsigned __int64 data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (int data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (double data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (std::vector<double> data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (std::vector<float> data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (std::vector<long long> data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (std::string data, std::string name, H5::Group& group);
		H5::DataSet writeDataSet (std::vector<std::string> dataVec, std::string name, H5::Group& group);
		void writeAttribute (double data, std::string name, H5::DataSet& dset);
		void writeAttribute (bool data, std::string name, H5::DataSet& dset);
		H5::DataSet AndorPictureDataset, voltsDataSet/*, MakoPictureDataset*/;
		H5::DataSet MakoPictureDataset;
		// for the entire set
		H5::DataSpace AndorPicureSetDataSpace/*, MakoPicureSetDataSpace*/;
		H5::DataSpace MakoPicureSetDataSpace;
		// just one pic
		H5::DataSpace AndorPicDataSpace/*, MakoPicDataSpace*/;
		H5::DataSpace MakoPicDataSpace;

		H5::DataSpace voltsDataSpace, voltsSetDataSpace;
		unsigned currentAndorPicNumber/*, currentMakoPicNumber*/;
		unsigned currentMakoPicNumber;
		//std::map<CameraInfo::name, unsigned> currentMakoPicNumber;
		std::string mr_dayStr, mr_monthStr, mr_yearStr;

	private:
		std::ofstream optFile;
	    bool fileIsOpen;
		std::string mostRecentInitializationDate;
		std::string dataFilesBaseLocation;
		std::string todayFolder;
		int currentDataFileNumber;
		std::string mostRecentDateString;
};


template <class type> void writeDataSet( type data, H5::Group group ){
	H5::DataSet rightSet = imageDims.createDataSet( "Right", H5::PredType::NATIVE_INT, H5::DataSpace( 1, rank1 ) );
	rightSet.write( &settings.imageSettings.right, H5::PredType::NATIVE_INT );
}

