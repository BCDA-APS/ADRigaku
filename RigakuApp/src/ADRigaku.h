#ifndef INC_ADRIGAKU_H
#define INC_ADRIGAKU_H

#include "ADDriver.h"
#include "uhss.h"


#define RigakuZeroDeadString "RIGAKU_ZERO_DEAD"
#define RigakuReadModeString "RIGAKU_READ_MODE"
#define RigakuUpperThresholdString "RIGAKU_UPPER_THRESHOLD"
#define RigakuLowerThresholdString "RIGAKU_LOWER_THRESHOLD"
#define RigakuReferenceThresholdString "RIGAKU_REF_THRESHOLD"
#define RigakuBadPixelString "RIGAKU_PIXEL_CORRECTION"
#define RigakuCountingRateString "RIGAKU_COUNTING_RATE"
#define RigakuInterChipString "RIGAKU_INTER_CHIP"
#define RigakuFlatFieldString "RIGAKU_FLAT_FIELD"
#define RigakuRedistributionString "RIGAKU_REDISTRIBUTION"
#define RigakuOuterEdgeString "RIGAKU_OUTER_EDGE"
#define RigakuPileupString "RIGAKU_PILEUP"

typedef enum
{
	B32_Single,
	B16_2S,
	B16_Zero_Deadtime,
	B8_Zero_Deadtime,
	B4_Zero_Deadtime,
	B2_Zero_Deadtime
} RigakuReadMode_t;

class epicsShareClass ADRigaku: public ADDriver, public UHSS::ManagerCallback {
public:
    ADRigaku(const char *portName, int maxBuffers, size_t maxMemory, int priority, int stackSize);
    ~ADRigaku();
	void notify(UHSS::AcqManager& manager, UHSS::StatusEvent status);
	
	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
	
	asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
	
	void startAcquisition();
	void stopAcquisition();
	void processImage();

protected:
	int RigakuZeroDead;
	int RigakuReadMode;
	int RigakuUpperThreshold;
	int RigakuLowerThreshold;
	int RigakuReferenceThreshold;
	
	int RigakuBadPixel;
	int RigakuCountingRate;
	int RigakuInterChip;
	int RigakuFlatField;
	int RigakuRedistribution;
	int RigakuOuterEdge;
	int RigakuPileup;
	
private:
	UHSS::AcqManager& api;
};

#define NUM_RIGAKU_PARAMS 12

#endif
