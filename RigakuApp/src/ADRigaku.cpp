#include <algorithm>
#include <iocsh.h>

#include <epicsTime.h>
#include <epicsExit.h>
#include <epicsExport.h>

#include <cstdio>

#include "ADRigaku.h"
#include "uhss.h"

extern "C"
{
	/* Callback function for exit hook */
	static void RigakuExit(void *pPvt)
	{ 
		delete (ADRigaku*) pPvt;
	}
}

void RigakuConfig(const char* portName, int maxBuffers, size_t maxMemory, int priority, int stackSize)
{
	ADRigaku* driver = new ADRigaku(portName, maxBuffers, maxMemory, priority, stackSize);
}

void ADRigaku::notify(UHSS::AcqManager& manager, UHSS::StatusEvent status)
{
	UHSS::State state = api.getState();
	
	switch (status)
	{
		case UHSS::StateChanged:
		{
			printf("State Change\n");
			
			if (state.acquisitionState != UHSS::Status::IN_PROGRESS)
			{
				this->setIntegerParam(ADStatus, ADStatusIdle);
				callParamCallbacks();
			}
			
			break;
		}
		case UHSS::EnvironmentChanged:
			printf("Environ Change\n");
			break;
			
		case UHSS::ConfigurationChanged:
			printf("Config Change\n");
			break;
			
		case UHSS::FrameAvailable:
		{
			size_t image_dims[2];

			image_dims[0] = state.outputDataset.numColumns;
			image_dims[1] = state.outputDataset.numRows;
			
			char* buffer = (char*) malloc(state.outputDataset.frameSize);
			api.getImages(buffer, 0, 1);
			
			switch (state.outputDataset.outputMode)
			{
				case UHSS::OutputMode::UNSIGNED_8BIT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDUInt8, 0, NULL);
					break;
					
				case UHSS::OutputMode::UNSIGNED_16BIT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDUInt16, 0, NULL);
					break;
					
				case UHSS::OutputMode::SIGNED_32BIT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDInt32, 0, NULL);
					break;
					
				case UHSS::OutputMode::IEEE_FLOAT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDFloat64, 0, NULL);
					break;
			}
			
			memcpy(this->pArrays[0]->pData, buffer, state.outputDataset.frameSize);
			this->processImage();
			
			break;
		}
		case UHSS::ErrorPushed:
		{
			UHSS::State state = api.getState();
			
			const UHSS::Array<int> &errors = state.errors;
			
			for (int index = 0; index < errors.size; index += 1)
			{
				printf("Error: %d\n", errors[index]);
			}
			
			api.clearError();
			break;
		}
	}
}

void ADRigaku::processImage()
{
	int image_number;
	int total_images;

	this->getIntegerParam(this->NDArrayCounter,     &image_number);
	this->getIntegerParam(this->ADNumImagesCounter, &total_images);
	image_number += 1;
	total_images += 1;
	this->setIntegerParam(this->NDArrayCounter,     image_number);
	this->setIntegerParam(this->ADNumImagesCounter, total_images);

	this->pArrays[0]->uniqueId = image_number;

	updateTimeStamp(&this->pArrays[0]->epicsTS);
	this->pArrays[0]->timeStamp = this->pArrays[0]->epicsTS.secPastEpoch + this->pArrays[0]->epicsTS.nsec/1e9;

	getAttributes(this->pArrays[0]->pAttributeList);

	doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);

	this->pArrays[0]->release();
	this->callParamCallbacks();
}

ADRigaku::ADRigaku(const char *portName, int maxBuffers, size_t maxMemory, int priority, int stackSize) :
	ADDriver(portName, 1, int(NUM_RIGAKU_PARAMS), maxBuffers, maxMemory,
	         asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize),
	api(UHSS::getAPI())
{
	ADDriver::createParam(RigakuZeroDeadString, asynParamInt32, &RigakuZeroDead);
	ADDriver::createParam(RigakuReadModeString, asynParamInt32, &RigakuReadMode);
	ADDriver::createParam(RigakuUpperThresholdString, asynParamFloat64, &RigakuUpperThreshold);
	ADDriver::createParam(RigakuLowerThresholdString, asynParamFloat64, &RigakuLowerThreshold);
	ADDriver::createParam(RigakuReferenceThresholdString, asynParamFloat64, &RigakuReferenceThreshold);
	
	this->connect(pasynUserSelf);
	
	api.setCallback(*this);
	api.initialize(false);
	
	epicsAtExit(RigakuExit, this);
}

ADRigaku::~ADRigaku()
{ 
	this->disconnect(pasynUserSelf);
	api.shutdown(0);
}

asynStatus ADRigaku::writeInt32(asynUser *pasynUser, epicsInt32 value) 
{
	int status = asynSuccess;
	int function = pasynUser->reason;
	int adStatus;
	
	//Record for later use
	this->getIntegerParam(ADStatus, &adStatus);
	
	this->setIntegerParam(function, value);
	
	if (function == ADAcquire)
	{ 
		if (adStatus == ADStatusIdle && value == 1)
		{
			this->startAcquisition();
		}
		else if (adStatus == ADStatusAcquire && value == 0)
		{
			this->stopAcquisition();
		}
	}
	else
	{
		status = ADDriver::writeInt32(pasynUser, value);
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}

asynStatus ADRigaku::writeFloat64(asynUser *pasynUser, epicsFloat64 value) 
{
	int status = asynSuccess;
	int function = pasynUser->reason;
	
	this->setDoubleParam(function, value);
	
	if (function == RigakuLowerThreshold)
	{ 
		UHSS::Configuration config = api.getConfiguration();
		
		api.setThreshold(value, config.upperThreshold, config.refThreshold);
	}
	else if (function == RigakuUpperThreshold)
	{ 
		UHSS::Configuration config = api.getConfiguration();
		
		api.setThreshold(config.lowerThreshold, value, config.refThreshold);
	}
	else if (function == RigakuReferenceThreshold)
	{ 
		UHSS::Configuration config = api.getConfiguration();
		
		api.setThreshold(config.lowerThreshold, config.upperThreshold, value);
	}
	else
	{
		status = ADDriver::writeFloat64(pasynUser, value);
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}


void ADRigaku::startAcquisition()
{
	UHSS::Parameters params;
	
	this->getIntegerParam(ADNumImages, &params.numFrames);
	
	int dead, trigger, mode;
	
	this->getIntegerParam(RigakuZeroDead, &dead);
	this->getIntegerParam(ADTriggerMode, &trigger);
	this->getIntegerParam(ADImageMode, &mode);
	
	if (dead == 0 && trigger == ADTriggerInternal)
	{
		params.acquisitionMode = UHSS::AcquisitionMode::FIXED_TIME;
	}
	else if (dead == 0 && trigger == ADTriggerExternal)
	{
		if (mode == ADImageContinuous)
		{
			params.acquisitionMode = UHSS::AcquisitionMode::CONT_WITH_TRIGGER;
		}
		else if (mode == ADImageSingle)
		{
			params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER;
		}
		else
		{
			params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_FIXED_TIME;
		}
	}
	else if (dead == 1 && trigger == ADTriggerInternal)
	{
		params.acquisitionMode = UHSS::AcquisitionMode::FIXED_TIME_ZERO_DEAD;
	}
	else if (dead == 1 && trigger == ADTriggerExternal)
	{
		if (mode == ADImageContinuous)
		{
			params.acquisitionMode = UHSS::AcquisitionMode::CONT_WITH_TRIGGER_ZERO_DEAD;
		}
		else if (mode == ADImageSingle)
		{
			params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_ZERO_DEAD;
		}
		else if (mode == ADImageMultiple)
		{
			params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_FIXED_TIME_ZERO_DEAD;
		}
	}
	
	int readmode;
	
	this->getIntegerParam(RigakuReadMode, &readmode);
	
	if (readmode == B32_Single)
	{
		if (dead == 1)
		{
			params.imagingMode = UHSS::ImagingMode::B16_ZERO_DEADTIME;
			
			this->setIntegerParam(RigakuReadMode, B16_Zero_Deadtime);
			this->callParamCallbacks();
		}
		else
		{
			params.imagingMode = UHSS::ImagingMode::B32_SINGLE;
		}
	}
	else if (readmode == B16_2S)
	{
		if (dead == 1)
		{
			params.imagingMode = UHSS::ImagingMode::B16_ZERO_DEADTIME;
			
			this->setIntegerParam(RigakuReadMode, B16_Zero_Deadtime);
			this->callParamCallbacks();
		}
		else
		{
			params.imagingMode = UHSS::ImagingMode::B16_2S;
		}
	}
	else if (readmode == B16_Zero_Deadtime)
	{
		if (dead == 0)
		{
			params.imagingMode = UHSS::ImagingMode::B32_SINGLE;
			
			this->setIntegerParam(RigakuReadMode, B32_Single);
			this->callParamCallbacks();
		}
		else
		{
			params.imagingMode = UHSS::ImagingMode::B16_ZERO_DEADTIME;
			params.zeroDeadTimeDataLength = UHSS::ZDTDataLength::ZDT_16BIT;
		}
	}
	else if (readmode == B8_Zero_Deadtime)
	{
		if (dead == 0)
		{
			params.imagingMode = UHSS::ImagingMode::B32_SINGLE;
			
			this->setIntegerParam(RigakuReadMode, B32_Single);
			this->callParamCallbacks();
		}
		else
		{
			params.imagingMode = UHSS::ImagingMode::B8_ZERO_DEADTIME;
			params.zeroDeadTimeDataLength = UHSS::ZDTDataLength::ZDT_8BIT;
		}
	}
	else if (readmode == B4_Zero_Deadtime)
	{
		if (dead == 0)
		{
			params.imagingMode = UHSS::ImagingMode::B32_SINGLE;
			
			this->setIntegerParam(RigakuReadMode, B32_Single);
			this->callParamCallbacks();
		}
		else
		{
			params.imagingMode = UHSS::ImagingMode::B4_ZERO_DEADTIME;
			params.zeroDeadTimeDataLength = UHSS::ZDTDataLength::ZDT_4BIT;
		}
	}
	else if (readmode == B2_Zero_Deadtime)
	{
		if (dead == 0)
		{
			params.imagingMode = UHSS::ImagingMode::B32_SINGLE;
			
			this->setIntegerParam(RigakuReadMode, B32_Single);
			this->callParamCallbacks();
		}
		else
		{
			params.imagingMode = UHSS::ImagingMode::B2_ZERO_DEADTIME;
			params.zeroDeadTimeDataLength = UHSS::ZDTDataLength::ZDT_2BIT;
		}
	}
	
	
	
	int datatype;
	
	this->getIntegerParam(NDDataType, &datatype);
	
	switch (datatype)
	{
		case NDUInt8:
			params.outputMode = UHSS::OutputMode::UNSIGNED_8BIT;
			break;
		
		case NDInt8:
			this->setIntegerParam(NDDataType, NDUInt16);
			this->callParamCallbacks();
			// Fall-Through
		case NDUInt16:
			params.outputMode = UHSS::OutputMode::UNSIGNED_16BIT;
			break;
		
		case NDInt16:
			this->setIntegerParam(NDDataType, NDInt32);
			this->callParamCallbacks();
			// Fall-Through
		case NDInt32:
			params.outputMode = UHSS::OutputMode::SIGNED_32BIT;
			break;
			
		case NDFloat32:
			this->setIntegerParam(NDDataType, NDFloat64);
			this->callParamCallbacks();
			// Fall-Through
		case NDFloat64:
			params.outputMode = UHSS::OutputMode::IEEE_FLOAT;
			break;
	}

	params.acqTriggerMode = UHSS::TriggerMode::RISING_EDGE;
	params.expTriggerMode = UHSS::TriggerMode::RISING_EDGE;
	params.readoutBits = 0;
	params.noiseElimination = UHSS::NoiseElimination::LOW;
	
	double exposure, period;
	
	this->getDoubleParam(ADAcquireTime, &exposure);
	this->getDoubleParam(ADAcquirePeriod, &period);
	
	params.exposureTime = exposure;
	params.exposureInterval = period - exposure;
	
	params.exposureDelay = 0.0;
	params.acquisitionDelay = 0.0;
	
	api.setParameters(params);
	api.startAcq();
}

void ADRigaku::stopAcquisition()
{
	api.stop();
}

/* Code for iocsh registration */

/* RigakuConfig */
static const iocshArg RigakuConfigArg0 = { "Port name", iocshArgString };
static const iocshArg RigakuConfigArg1 = { "maxBuffers", iocshArgInt };
static const iocshArg RigakuConfigArg2 = { "maxMemory", iocshArgInt };
static const iocshArg RigakuConfigArg3 = { "priority", iocshArgInt };
static const iocshArg RigakuConfigArg4 = { "stackSize", iocshArgInt };
static const iocshArg * const RigakuConfigArgs[] = { &RigakuConfigArg0,
	&RigakuConfigArg1, &RigakuConfigArg2, &RigakuConfigArg3, &RigakuConfigArg4};

static void configRigakuCallFunc(const iocshArgBuf *args) 
{
	RigakuConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival, args[4].ival);
}
static const iocshFuncDef configRigaku = { "RigakuConfig", 5, RigakuConfigArgs };

static void RigakuRegister(void) 
{
	iocshRegister(&configRigaku, configRigakuCallFunc);
}

extern "C" 
{
	epicsExportRegistrar(RigakuRegister);
}
