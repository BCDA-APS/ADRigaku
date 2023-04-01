#include <algorithm>
#include <iocsh.h>
#include <cstring>

#include <epicsThread.h>
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

void RigakuConfig(const char* portName, const char* configuration, int maxBuffers, size_t maxMemory, int priority, int stackSize)
{
	ADRigaku* driver = new ADRigaku(portName, configuration, maxBuffers, maxMemory, priority, stackSize);
}

void RigakuTest()
{
	UHSS::AcqManager& api = UHSS::getAPI();

	api.initialize("XSPA");
}

void spawn_thread_callback(void* arg)
{
	ADRigaku* driver = (ADRigaku*) arg;
	
	bool continuing = true;

	while (continuing)
	{
		continuing = !driver->checkAcquisitionStatus();
		epicsThreadSleep(1.0);
	}
}

void ADRigaku::notify(UHSS::AcqManager& manager, UHSS::StatusEvent status)
{
	UHSS::State state = api.getState();
	
	switch (status)
	{
		case UHSS::StateChanged:
		{	
			if (state.acquisitionState == UHSS::Status::TERMINATED ||
			    state.acquisitionState == UHSS::Status::NORMAL_END)
			{
				int corrections;
	
				this->getIntegerParam(this->RigakuCorrections, &corrections);
	
				if (corrections == 0)    { api.deleteDataset(0); }
			}
		
			if (state.operationState == UHSS::Status::IDLE)
			{			
				this->setIntegerParam(this->ADStatus, ADStatusIdle);
				this->setIntegerParam(this->ADAcquireBusy, 0);
				this->setIntegerParam(this->ADAcquire, 0);
				callParamCallbacks();
			}
			
			break;
		}
		case UHSS::EnvironmentChanged:
			break;
			
		case UHSS::ConfigurationChanged:
		{			
			UHSS::Configuration config = api.getConfiguration();

			if (config.calibTable.label == NULL)    { break; }

			this->setStringParam(RigakuCalibrationLabel, config.calibTable.label);
			this->setDoubleParam(RigakuUpperThreshold, config.upperEnergy);
			this->setDoubleParam(RigakuLowerThreshold, config.lowerEnergy);
			this->setIntegerParam(ADMaxSizeX, config.numColumns);
			this->setIntegerParam(ADMaxSizeY, config.numRows);
			
			this->callParamCallbacks();
			
			break;
		}
		case UHSS::FrameAvailable:
		{
			size_t image_dims[2];

			image_dims[0] = state.outputDataset.numColumns;
			image_dims[1] = state.outputDataset.numRows;
			
			if (state.outputDataset.imagingMode == UHSS::ImagingMode::B16_2S || 
			    state.outputDataset.imagingMode == UHSS::ImagingMode::B8_2S)
			{
				image_dims[1] = image_dims[1] * 2;
			}
			
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
			
			delete buffer;
			
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

ADRigaku::ADRigaku(const char *portName, const char* configuration, int maxBuffers, size_t maxMemory, int priority, int stackSize) :
	ADDriver(portName, 1, int(NUM_RIGAKU_PARAMS), maxBuffers, maxMemory,
	         asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize),
	api(UHSS::getAPI())
{
	ADDriver::createParam(RigakuUpperThresholdString, asynParamFloat64, &RigakuUpperThreshold);
	ADDriver::createParam(RigakuLowerThresholdString, asynParamFloat64, &RigakuLowerThreshold);
	
	ADDriver::createParam(RigakuBadPixelString, asynParamInt32, &RigakuBadPixel);
	ADDriver::createParam(RigakuCountingRateString, asynParamInt32, &RigakuCountingRate);
	ADDriver::createParam(RigakuInterChipString, asynParamInt32, &RigakuInterChip);
	ADDriver::createParam(RigakuFlatFieldString, asynParamInt32, &RigakuFlatField);
	ADDriver::createParam(RigakuRedistributionString, asynParamInt32, &RigakuRedistribution);
	ADDriver::createParam(RigakuOuterEdgeString, asynParamInt32, &RigakuOuterEdge);
	ADDriver::createParam(RigakuPileupString, asynParamInt32, &RigakuPileup);
	
	ADDriver::createParam(RigakuAcquisitionDelayString, asynParamFloat64, &RigakuAcquisitionDelay);
	ADDriver::createParam(RigakuExposureDelayString, asynParamFloat64, &RigakuExposureDelay);
	ADDriver::createParam(RigakuExposureIntervalString, asynParamFloat64, &RigakuExposureInterval);
	
	ADDriver::createParam(RigakuCalibrationLabelString, asynParamOctet, &RigakuCalibrationLabel);
	
	ADDriver::createParam(RigakuCorrectionsString, asynParamInt32, &RigakuCorrections);
	ADDriver::createParam(RigakuUsernameString, asynParamOctet, &RigakuUsername);
	ADDriver::createParam(RigakuPasswordString, asynParamOctet, &RigakuPassword);
	ADDriver::createParam(RigakuFileshareString, asynParamOctet, &RigakuSharepath);
	ADDriver::createParam(RigakuFilepathString, asynParamOctet, &RigakuFilepath);
	ADDriver::createParam(RigakuFilenameString, asynParamOctet, &RigakuFilename);
	
	
	setDoubleParam(RigakuExposureDelay, 0.0);
	setStringParam(RigakuCalibrationLabel, "");
	
	this->connect(pasynUserSelf);
	
	api.setCallback(*this);

	if (! api.initialize(configuration))
	{
		this->setStringParam(ADStatusMessage, "Couldn't connect to server");
		return;
	}

	this->callParamCallbacks();
	
	epicsAtExit(RigakuExit, this);
}

ADRigaku::~ADRigaku()
{ 
	api.deleteDataset(0);

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
	
	if (function == ADAcquire)
	{ 
		if (adStatus == ADStatusIdle && value == 1)
		{
			this->setStringParam(ADStatusMessage, "Starting Acquisition...");
			callParamCallbacks();
			this->startAcquisition();
		}
		else if (adStatus == ADStatusAcquire && value == 0)
		{
			this->setStringParam(ADStatusMessage, "Terminating Acquisition...");
			callParamCallbacks();
			this->stopAcquisition();
		}
	}
	else if (function == RigakuBadPixel || function == RigakuCountingRate || function == RigakuInterChip ||
	         function == RigakuFlatField || function == RigakuRedistribution || function == RigakuOuterEdge ||
	         function == RigakuPileup)
	{
		int values[9] = { 0 };
		int test[1] = { 0 };
		
		this->getIntegerParam(RigakuBadPixel, &values[0]);
		this->getIntegerParam(RigakuCountingRate, &values[1]);
		this->getIntegerParam(RigakuInterChip, &values[2]);
		this->getIntegerParam(RigakuFlatField, &values[3]);
		this->getIntegerParam(RigakuRedistribution, &values[4]);
		this->getIntegerParam(RigakuOuterEdge, &values[5]);
		this->getIntegerParam(RigakuPileup, &values[6]);

		// Only set when detector is idle
		
		int curr_status;

		this->getIntegerParam(this->ADStatus, &curr_status);
		
		while (curr_status != ADStatusIdle)
		{
			epicsThreadSleep(1);
			this->getIntegerParam(this->ADStatus, &curr_status);
		}
		
		api.controlCorrections(test, 7, values);
	}
	else if (function == RigakuCorrections)
	{
		if (value == 1)
		{
			std::string user, pass, path;
			
			this->getStringParam(RigakuUsername, user);
			this->getStringParam(RigakuPassword, pass);
			this->getStringParam(RigakuSharepath, path);
		
			api.controlCorrection("Diversion", 1); // Switch sparse matrix mode ON
			api.controlCorrection("Diversion", "user", user.c_str()); // Set username required to mount the network drive
			api.controlCorrection("Diversion", "password", pass.c_str()); // Set password required to mount the network drive
			api.controlCorrection("Diversion", "share", path.c_str()); // Set path to the network drive.
		}
		else
		{
			api.controlCorrection("Diversion", 0);
		}
	}
	else
	{
		status = ADDriver::writeInt32(pasynUser, value);
	}
	
	this->setIntegerParam(function, value);
	callParamCallbacks();
	return (asynStatus) status;
}

asynStatus ADRigaku::writeFloat64(asynUser *pasynUser, epicsFloat64 value) 
{
	int status = asynSuccess;
	int function = pasynUser->reason;
	
	this->setDoubleParam(function, value);
	
	UHSS::Configuration config = api.getConfiguration();

	std::string calib;
	
	this->getStringParam(RigakuCalibrationLabel, calib);
	
	if (function == RigakuLowerThreshold)
	{
		api.setEnergy(calib.c_str(), value, config.upperEnergy, 1);
	}
	else if (function == RigakuUpperThreshold)
	{
		api.setEnergy(calib.c_str(), config.lowerEnergy, value, 1);
	}
	else
	{
		status = ADDriver::writeFloat64(pasynUser, value);
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}

asynStatus ADRigaku::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
	int status = asynSuccess;
	int function = pasynUser->reason;
	
	UHSS::Configuration config = api.getConfiguration();
	
	if      (function == RigakuLowerThreshold)    { *value = config.lowerEnergy; }
	else if (function == RigakuUpperThreshold)    { *value = config.upperEnergy; }
	else
	{
		status = ADDriver::readFloat64(pasynUser, value);
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}

asynStatus ADRigaku::writeOctet(asynUser* pasynUser, const char* value, size_t nChars, size_t* actual)
{
	int function = pasynUser->reason;
	int status = asynSuccess;
	
	this->setStringParam(function, value);
	
	UHSS::Configuration config = api.getConfiguration();
	
	if (function == RigakuCalibrationLabel)
	{
		api.setEnergy(value, config.lowerEnergy, config.upperEnergy, 1);
		*actual = sizeof(value);
	}
	else
	{
		status = ADDriver::writeOctet(pasynUser, value, nChars, actual);
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}

void ADRigaku::startAcquisition()
{
	UHSS::Parameters params;
	
	this->getIntegerParam(ADNumImages, &params.numFrames);
	
	int trigger, mode;
	
	this->getIntegerParam(ADTriggerMode, &trigger);
	this->getIntegerParam(ADImageMode, &mode);
	
	params.acquisitionMode = trigger;
	params.imagingMode = mode;
	
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
	
	double exposure, interval, exp_delay, acq_delay;
	
	this->getDoubleParam(ADAcquireTime, &exposure);
	this->getDoubleParam(RigakuExposureInterval, &interval);
	this->getDoubleParam(RigakuExposureDelay, &exp_delay);
	this->getDoubleParam(RigakuAcquisitionDelay, &acq_delay);
	
	//Exposure time is set in seconds but sent in milliseconds
	params.exposureTime = exposure * 1000;
	params.exposureInterval = interval;
	params.exposureDelay = exp_delay;
	params.acquisitionDelay = acq_delay;
	
	api.setParameters(params);
	
	std::string filename;
	std::string filepath;
	
	this->getStringParam(RigakuFilepath, filepath);
	this->getStringParam(RigakuFilename, filename);
	
	std::string fullpath = filepath;
	
	if (filepath.at(filepath.length() - 1) != '/')    { fullpath += "/"; }
	
	fullpath += filename;
	
	api.controlCorrection("Diversion", "filepath", fullpath.c_str());
	
	if(api.startAcq())
	{
		this->setStringParam(ADStatusMessage, "");
		this->setIntegerParam(this->ADStatus, ADStatusAcquire);
	}
	else
	{
		this->setStringParam(ADStatusMessage, "Error in starting acquire");
		this->setIntegerParam(this->ADStatus, ADStatusError);
	}
	
	this->callParamCallbacks();
}

void ADRigaku::stopAcquisition()
{
	api.stop();

	epicsThreadCreate("Check stop thread",
	epicsThreadPriorityLow,
	epicsThreadGetStackSize(epicsThreadStackMedium),
	(EPICSTHREADFUNC)::spawn_thread_callback, (void*) this);
}

bool ADRigaku::checkAcquisitionStatus()
{
	UHSS::State state = api.getState();

	printf("State: %d\n", state.operationState);

	if (state.operationState == UHSS::Status::TERMINATED)
	{
		api.clearError();
			
		this->setStringParam(ADStatusMessage, "");
		callParamCallbacks();
		return true;
	}

	return false;
}

/* Code for iocsh registration */

/* RigakuConfig */
static const iocshArg RigakuConfigArg0 = { "Port name", iocshArgString };
static const iocshArg RigakuConfigArg1 = { "Configuration", iocshArgString };
static const iocshArg RigakuConfigArg2 = { "maxBuffers", iocshArgInt };
static const iocshArg RigakuConfigArg3 = { "maxMemory", iocshArgInt };
static const iocshArg RigakuConfigArg4 = { "priority", iocshArgInt };
static const iocshArg RigakuConfigArg5 = { "stackSize", iocshArgInt };
static const iocshArg * const RigakuConfigArgs[] = { &RigakuConfigArg0,
	&RigakuConfigArg1, &RigakuConfigArg2, &RigakuConfigArg3, &RigakuConfigArg4, &RigakuConfigArg5};

static const iocshArg * const RigakuTestArgs[] = {};
static void testRigakuCallFunc(const iocshArgBuf *args)
{
	RigakuTest();
}
static const iocshFuncDef testRigaku = { "RigakuTest", 0, RigakuTestArgs };

static void configRigakuCallFunc(const iocshArgBuf *args) 
{
	RigakuConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival, args[5].ival);
}
static const iocshFuncDef configRigaku = { "RigakuConfig", 6, RigakuConfigArgs };

static void RigakuRegister(void) 
{
	iocshRegister(&configRigaku, configRigakuCallFunc);
	iocshRegister(&testRigaku, testRigakuCallFunc);
}

extern "C" 
{
	epicsExportRegistrar(RigakuRegister);
}
