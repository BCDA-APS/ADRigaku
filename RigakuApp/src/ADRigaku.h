#ifndef INC_ADRIGAKU_H
#define INC_ADRIGAKU_H

#include "ADDriver.h"
#include "uhss.h"


#define RigakuImageModeString "RIGAKU_IMAGE_MODE"

#define RigakuUpperThresholdString "RIGAKU_UPPER_THRESHOLD"
#define RigakuLowerThresholdString "RIGAKU_LOWER_THRESHOLD"

#define RigakuBadPixelString "RIGAKU_PIXEL_CORRECTION"
#define RigakuCountingRateString "RIGAKU_COUNTING_RATE"
#define RigakuInterChipString "RIGAKU_INTER_CHIP"
#define RigakuFlatFieldString "RIGAKU_FLAT_FIELD"
#define RigakuRedistributionString "RIGAKU_REDISTRIBUTION"
#define RigakuOuterEdgeString "RIGAKU_OUTER_EDGE"
#define RigakuPileupString "RIGAKU_PILEUP"

#define RigakuAcquisitionDelayString "RIGAKU_ACQUISITION_DELAY"
#define RigakuExposureDelayString "RIGAKU_EXPOSURE_DELAY"
#define RigakuExposureIntervalString "RIGAKU_EXPOSURE_INTERVAL"

#define RigakuCalibrationLabelString "RIGAKU_CALIBRATION_LABEL"

#define RigakuUsernameString "RIGAKU_USERNAME"
#define RigakuPasswordString "RIGAKU_PASSWORD"
#define RigakuFileshareString "RIGAKU_SHARE_PATH"
#define RigakuFilepathString "RIGAKU_FILE_PATH"
#define RigakuFilenameString "RIGAKU_FILE_NAME"
#define RigakuCorrectionsString "RIGAKU_CORRECTIONS"


class epicsShareClass ADRigaku: public ADDriver, public UHSS::ManagerCallback {
public:
    ADRigaku(const char *portName, const char* configuration, int maxBuffers, size_t maxMemory, int priority, int stackSize);
    ~ADRigaku();
	void notify(UHSS::AcqManager& manager, UHSS::StatusEvent status);
	
	asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);
	
	asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);
	asynStatus readFloat64(asynUser* pasynUser, epicsFloat64 *value);
	
	asynStatus writeOctet(asynUser* pasynUser, const char* value, size_t nChars, size_t* actual);
	
	void startAcquisition();
	void stopAcquisition();
	void processImage();

protected:
	int RigakuUpperThreshold;
	int RigakuLowerThreshold;
	
	int RigakuBadPixel;
	int RigakuCountingRate;
	int RigakuInterChip;
	int RigakuFlatField;
	int RigakuRedistribution;
	int RigakuOuterEdge;
	int RigakuPileup;
	
	int RigakuAcquisitionDelay;
	int RigakuExposureDelay;
	int RigakuExposureInterval;
	
	int RigakuCalibrationLabel;
	
	int RigakuUsername;
	int RigakuPassword;
	int RigakuCorrections;
	int RigakuSharepath;
	int RigakuFilepath;
	int RigakuFilename;
	
private:
	UHSS::AcqManager& api;
};

#define NUM_RIGAKU_PARAMS 20

#endif
