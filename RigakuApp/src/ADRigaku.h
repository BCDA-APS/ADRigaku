#ifndef INC_ADRIGAKU_H
#define INC_ADRIGAKU_H

#include "ADDriver.h"
#include "uhss.h"

#define RigakuAcquisitionDelayString "RIGAKU_ACQUISITION_DELAY"

#define RigakuTriggerEdgeString "RIGAKU_TRIGGER_EDGE"
#define RigakuOutputResolutionString "RIGAKU_OUTPUT_RESOLUTION"
#define RigakuNoiseEliminationString "RIGAKU_NOISE_ELIMINATION"

#define RigakuCorrectionsString "RIGAKU_CORRECTIONS"
#define RigakuCalibrationLabelString "RIGAKU_CALIBRATION_LABEL"
#define RigakuBadPixelString "RIGAKU_PIXEL_CORRECTION"
#define RigakuCountingRateString "RIGAKU_COUNTING_RATE"
#define RigakuInterChipString "RIGAKU_INTER_CHIP"
#define RigakuFlatFieldString "RIGAKU_FLAT_FIELD"
#define RigakuRedistributionString "RIGAKU_REDISTRIBUTION"
#define RigakuDifferentiationString "RIGAKU_DIFFERENTIATION"
#define RigakuOuterEdgeString "RIGAKU_OUTER_EDGE"
#define RigakuPileupString "RIGAKU_PILEUP"

#define RigakuSparseEnableString "RIGAKU_SPARSE_ENABLE"
#define RigakuFilepathString "RIGAKU_FILE_PATH"
#define RigakuFilenameString "RIGAKU_FILE_NAME"

#define RigakuThresholdSetString   "RIGAKU_THRESHOLD_SET"
#define RigakuUpperThresholdString "RIGAKU_UPPER_THRESHOLD"
#define RigakuLowerThresholdString "RIGAKU_LOWER_THRESHOLD"


#define _FIXED_TRIGGER               0
#define _CONTINUOUS_TRIGGER          1
#define _START_TRIGGER               2
#define _GATED_TRIGGER               3
#define _TRIGGER_SYNC                4
#define _FIXED_START_WITH_TRIGGER    5
#define _BURST_MODE_TRIGGER          6

#define STANDARD_MODE    0
#define ZDT_MODE         1
#define BURST_MODE       2
#define PILEUP_MODE      3
#define PILEUP_PPP_MODE  4

#define OUT_32_BIT  0
#define OUT_16_BIT  1
#define OUT_8_BIT   2
#define OUT_4_BIT   3
#define OUT_3_BIT   4
#define OUT_2_BIT   5
#define OUT_1_BIT   6

#define SINGLE_THRESHOLD  0
#define DUAL_THRESHOLD    1

class epicsShareClass ADRigaku: public ADDriver, public UHSS::ManagerCallback {
public:
    ADRigaku(const char *portName, const char* configuration, int maxBuffers, size_t maxMemory, int priority, int stackSize);
    ~ADRigaku();
	void notify(UHSS::AcqManager& manager, UHSS::StatusEvent status);
	
	asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);
	
	asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);
	asynStatus readFloat64(asynUser* pasynUser, epicsFloat64 *value);
	
	asynStatus writeOctet(asynUser* pasynUser, const char* value, size_t nChars, size_t* actual);
	
	asynStatus startAcquisition();
	void stopAcquisition();
	bool checkAcquisitionStatus();
	void processImage();

protected:
	int RigakuAcquisitionDelay;
	
	int RigakuTriggerEdge;
	int RigakuOutputResolution;
	int RigakuNoiseElimination;
	
	int RigakuCorrections;
	int RigakuCalibrationLabel;
	int RigakuBadPixel;
	int RigakuCountingRate;
	int RigakuInterChip;
	int RigakuFlatField;
	int RigakuRedistribution;
	int RigakuDifferentiation;
	int RigakuOuterEdge;
	int RigakuPileup;
	
	int RigakuSparseEnable;
	int RigakuFilepath;
	int RigakuFilename;

	int RigakuThresholdSet;
	int RigakuUpperThreshold;
	int RigakuLowerThreshold;
	
	
private:
	UHSS::AcqManager& api;
};

#define NUM_RIGAKU_PARAMS 20

#endif
