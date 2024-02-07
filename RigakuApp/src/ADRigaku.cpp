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
	
	this->lock();
	
	switch (status)
	{
		case UHSS::StateChanged:
		{	
			if (state.acquisitionState == UHSS::Status::NORMAL_END ||
				state.operationState == UHSS::Status::IDLE)
			{
				setIntegerParam(this->ADAcquire, 0);
				setIntegerParam(this->ADStatus, ADStatusIdle);
				callParamCallbacks();
			}
		
			if (state.acquisitionState == UHSS::Status::TERMINATED ||
			    state.acquisitionState == UHSS::Status::NORMAL_END)
			{
				int corrections;
	
				this->getIntegerParam(this->RigakuCorrections, &corrections);
	
				if (corrections == 0)    { api.deleteDataset(0); }
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
			setIntegerParam(ADMaxSizeX, config.numColumns);
			setIntegerParam(ADMaxSizeY, config.numRows);
			
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
			
			int calculated_size = image_dims[0] * image_dims[1];
			int HEADER_OFFSET = 1024;
			
			switch (state.outputDataset.outputMode)
			{
				case UHSS::OutputMode::UNSIGNED_8BIT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDUInt8, 0, NULL);
					break;
					
				case UHSS::OutputMode::UNSIGNED_16BIT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDUInt16, 0, NULL);
					calculated_size *= 2;
					break;
					
				case UHSS::OutputMode::SIGNED_32BIT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDInt32, 0, NULL);
					calculated_size *= 4;
					break;
					
				case UHSS::OutputMode::IEEE_FLOAT:
					this->pArrays[0] = this->pNDArrayPool->alloc(2, image_dims, NDFloat64, 0, NULL);
					calculated_size *=4;
					break;
					
				default:
					printf("Unknown output type\n");
					break;
			}
			
			//printf("Calculated Size: %d\n", calculated_size);
			//printf("Should be: %d\n", (state.outputDataset.frameSize - HEADER_OFFSET));
			
			if (calculated_size != (state.outputDataset.frameSize - HEADER_OFFSET))
			{
				printf("Something is wrong with my calculations\n");
				break;
			}
			
			memcpy(this->pArrays[0]->pData, &buffer[HEADER_OFFSET], calculated_size);
			
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

			setIntegerParam(this->ADStatus, ADStatusError);
			setIntegerParam(this->ADAcquire, 0);
			callParamCallbacks();
			break;
		}
	}
	
	this->callParamCallbacks();
	
	this->unlock();
}

void ADRigaku::processImage()
{	
	int image_number;
	int total_images;

	this->getIntegerParam(this->NDArrayCounter,     &image_number);
	this->getIntegerParam(this->ADNumImagesCounter, &total_images);
	image_number += 1;
	total_images += 1;
	setIntegerParam(this->NDArrayCounter,     image_number);
	setIntegerParam(this->ADNumImagesCounter, total_images);

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
	ADDriver::createParam(RigakuAcquisitionDelayString, asynParamFloat64, &RigakuAcquisitionDelay);
	
	ADDriver::createParam(RigakuTriggerEdgeString, asynParamInt32, &RigakuTriggerEdge);
	ADDriver::createParam(RigakuOutputResolutionString, asynParamInt32, &RigakuOutputResolution);
	ADDriver::createParam(RigakuNoiseEliminationString, asynParamInt32, &RigakuNoiseElimination);
	
	ADDriver::createParam(RigakuCorrectionsString, asynParamInt32, &RigakuCorrections);
	ADDriver::createParam(RigakuBadPixelString, asynParamInt32, &RigakuBadPixel);
	ADDriver::createParam(RigakuCountingRateString, asynParamInt32, &RigakuCountingRate);
	ADDriver::createParam(RigakuInterChipString, asynParamInt32, &RigakuInterChip);
	ADDriver::createParam(RigakuFlatFieldString, asynParamInt32, &RigakuFlatField);
	ADDriver::createParam(RigakuRedistributionString, asynParamInt32, &RigakuRedistribution);
	ADDriver::createParam(RigakuDifferentiationString, asynParamInt32, &RigakuDifferentiation);
	ADDriver::createParam(RigakuOuterEdgeString, asynParamInt32, &RigakuOuterEdge);
	ADDriver::createParam(RigakuPileupString, asynParamInt32, &RigakuPileup);

	ADDriver::createParam(RigakuThresholdSetString, asynParamInt32, &RigakuThresholdSet);
	ADDriver::createParam(RigakuUpperThresholdString, asynParamFloat64, &RigakuUpperThreshold);
	ADDriver::createParam(RigakuLowerThresholdString, asynParamFloat64, &RigakuLowerThreshold);
	
	ADDriver::createParam(RigakuCalibrationLabelString, asynParamOctet, &RigakuCalibrationLabel);
	
	ADDriver::createParam(RigakuSparseEnableString, asynParamInt32, &RigakuSparseEnable);
	ADDriver::createParam(RigakuFilepathString, asynParamOctet, &RigakuFilepath);
	ADDriver::createParam(RigakuFilenameString, asynParamOctet, &RigakuFilename);
	
	
	setStringParam(RigakuCalibrationLabel, "");
	setStringParam(RigakuFilepath, "/");
	setStringParam(RigakuFilename, "test");
	
	this->connect(pasynUserSelf);
	
	setIntegerParam(ADStatus, ADStatusInitializing);
	this->callParamCallbacks();
	
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
	
	setIntegerParam(function, value);
	
	if (function == ADAcquire)
	{ 
		if ((adStatus == ADStatusIdle || adStatus == ADStatusError) && value == 1)
		{
			this->setStringParam(ADStatusMessage, "Starting Acquisition...");
			callParamCallbacks();
			status = this->startAcquisition();
		}
		else if (adStatus == ADStatusAcquire && value == 0)
		{
			this->setStringParam(ADStatusMessage, "Terminating Acquisition...");
			callParamCallbacks();
			this->stopAcquisition();
		}
	}
	else if (function == RigakuCorrections)
	{
		if (value == 1)
		{
			//std::string user, pass, path;
			
			//this->getStringParam(RigakuUsername, user);
			//this->getStringParam(RigakuPassword, pass);
			//this->getStringParam(RigakuSharepath, path);
		
			api.controlCorrection("Diversion", 1); // Switch sparse matrix mode ON
			//api.controlCorrection("Diversion", "user", user.c_str()); // Set username required to mount the network drive
			//api.controlCorrection("Diversion", "password", pass.c_str()); // Set password required to mount the network drive
			//api.controlCorrection("Diversion", "share", path.c_str()); // Set path to the network drive.
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

asynStatus ADRigaku::startAcquisition()
{
	UHSS::Parameters params;
	
	this->getIntegerParam(ADNumImages, &params.numFrames);
	this->getIntegerParam(RigakuTriggerEdge, &params.acqTriggerMode);
	this->getIntegerParam(RigakuNoiseElimination, &params.noiseElimination);

	
	int trigger, mode, resolution, threshold;
	
	this->getIntegerParam(ADImageMode, &mode);
	this->getIntegerParam(ADTriggerMode, &trigger);
	this->getIntegerParam(RigakuOutputResolution, &resolution);
	this->getIntegerParam(RigakuThresholdSet, &threshold);
	
	// Get rid of low hanging fruit for errors, cleans up later code
	
	if (threshold == DUAL_THRESHOLD && (mode == BURST_MODE || mode == ZDT_MODE))
	{
		this->setStringParam(ADStatusMessage, "Image Mode / Threshold Mismatch");
		setIntegerParam(this->ADAcquire, 0);
		setIntegerParam(this->ADStatus, ADStatusError);
		return asynError;
	}
	
	if ((resolution == OUT_3_BIT || resolution == OUT_1_BIT) && mode != BURST_MODE)
	{
		this->setStringParam(ADStatusMessage, "Resolution / Image Mode Mismatch");
		setIntegerParam(this->ADAcquire, 0);
		setIntegerParam(this->ADStatus, ADStatusError);
		return asynError;
	}
	
	if (mode == STANDARD_MODE)
	{
		switch (trigger)
		{
			case _FIXED_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::FIXED_TIME;
				break;
				
			case _CONTINUOUS_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::CONT_WITH_TRIGGER;
				break;
				
			case _START_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER;
				break;
				
			case _GATED_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::GATED_TRIGGER;
				break;
				
			case _TRIGGER_SYNC:
				params.acquisitionMode = UHSS::AcquisitionMode::TRIGGER_SYNC;
				break;
				
			case _FIXED_START_WITH_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_FIXED_TIME;
				break;
			
			default:
				this->setStringParam(ADStatusMessage, "Trigger / Image Mode Mismatch");
				setIntegerParam(this->ADAcquire, 0);
				setIntegerParam(this->ADStatus, ADStatusError);
				return asynError;
		}
	}
	else if (mode == ZDT_MODE)
	{
		switch (trigger)
		{
			case _FIXED_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::FIXED_TIME_ZERO_DEAD;
				break;
				
			case _CONTINUOUS_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::CONT_WITH_TRIGGER_ZERO_DEAD;
				break;
				
			case _START_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_ZERO_DEAD;
				break;
				
			case _FIXED_START_WITH_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_FIXED_TIME_ZERO_DEAD;
				break;
			
			default:
				this->setStringParam(ADStatusMessage, "Trigger / Image Mode Mismatch");
				setIntegerParam(this->ADAcquire, 0);
				setIntegerParam(this->ADStatus, ADStatusError);
				return asynError;
		}
	}
	else if (mode == BURST_MODE)
	{
		if (resolution < OUT_3_BIT)
		{
			this->setStringParam(ADStatusMessage, "Resolution / Image Mode Mismatch");
			setIntegerParam(this->ADAcquire, 0);
			setIntegerParam(this->ADStatus, ADStatusError);
			return asynError;
		}
	
		switch (trigger)
		{
			case _BURST_MODE_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::BURST;
				break;
				
			case _START_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_BURST;
				break;
				
			case _GATED_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::GATED_TRIGGER_BURST;
				break;

			
			default:
				this->setStringParam(ADStatusMessage, "Trigger / Image Mode Mismatch");
				setIntegerParam(this->ADAcquire, 0);
				setIntegerParam(this->ADStatus, ADStatusError);
				return asynError;
		}
	}
	else if (mode == PILEUP_MODE)
	{
		switch (trigger)
		{				
			case _START_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::START_WITH_TRIGGER_PILEUP;
				break;
				
			case _GATED_TRIGGER:
				params.acquisitionMode = UHSS::AcquisitionMode::GATED_TRIGGER_PILEUP;
				break;
				
			case _TRIGGER_SYNC:
				params.acquisitionMode = UHSS::AcquisitionMode::TRIGGER_SYNC_PILEUP;
				break;
			
			default:
				this->setStringParam(ADStatusMessage, "Trigger / Image Mode Mismatch");
				setIntegerParam(this->ADAcquire, 0);
				setIntegerParam(this->ADStatus, ADStatusError);
				return asynError;
		}
	}
	else if (mode == PILEUP_PPP_MODE)
	{
		switch (trigger)
		{				
			case _START_TRIGGER:
				params.acquisitionMode = 16;
				break;
			
			default:
				this->setStringParam(ADStatusMessage, "Trigger / Image Mode Mismatch");
				setIntegerParam(this->ADAcquire, 0);
				setIntegerParam(this->ADStatus, ADStatusError);
				return asynError;
		}
	}
	
	if (resolution == OUT_32_BIT)
	{
		if (threshold == DUAL_THRESHOLD)
		{
			this->setStringParam(ADStatusMessage, "Resolution / Threshold Mismatch");
			setIntegerParam(this->ADAcquire, 0);
			setIntegerParam(this->ADStatus, ADStatusError);
			return asynError;
		}
	
		switch (mode)
		{
			case STANDARD_MODE:
			case PILEUP_MODE:
			case PILEUP_PPP_MODE:
				params.imagingMode = UHSS::ImagingMode::B32_SINGLE;
				params.outputMode  = UHSS::OutputMode::SIGNED_32BIT;
				setIntegerParam(NDDataType, NDInt32);
				break;
				
			default:
				this->setStringParam(ADStatusMessage, "Resolution / Image Mode Mismatch");
				setIntegerParam(this->ADStatus, ADStatusError);
				setIntegerParam(this->ADAcquire, 0);
				return asynError;
		}
	}
	else if (resolution == OUT_16_BIT)
	{		
		if      (mode == ZDT_MODE)              { params.imagingMode = UHSS::ImagingMode::B16_ZERO_DEADTIME; }
		else if (threshold == SINGLE_THRESHOLD) { params.imagingMode = UHSS::ImagingMode::B16_1S; }
		else                                    { params.imagingMode = UHSS::ImagingMode::B16_2S; }
		
		params.outputMode = UHSS::OutputMode::UNSIGNED_16BIT;
		
		setIntegerParam(NDDataType, NDUInt16);
	}
	else if (resolution == OUT_8_BIT)
	{
		if      (mode == ZDT_MODE)              { params.imagingMode = UHSS::ImagingMode::B8_ZERO_DEADTIME; }
		else if (threshold == SINGLE_THRESHOLD) { params.imagingMode = UHSS::ImagingMode::B8_1S; }
		else                                    { params.imagingMode = UHSS::ImagingMode::B8_2S; }
		
		params.outputMode = UHSS::OutputMode::UNSIGNED_8BIT;
		
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (resolution == OUT_4_BIT)
	{
		if      (mode == ZDT_MODE)              { params.imagingMode = UHSS::ImagingMode::B4_ZERO_DEADTIME; }
		else if (threshold == SINGLE_THRESHOLD) { params.imagingMode = UHSS::ImagingMode::B4_1S; }
		else                                    { params.imagingMode = UHSS::ImagingMode::B4_2S; }
		
		params.outputMode = UHSS::OutputMode::UNSIGNED_8BIT;
		
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (resolution == OUT_3_BIT)
	{		
		params.outputMode = UHSS::OutputMode::UNSIGNED_8BIT;
		params.imagingMode = UHSS::ImagingMode::B3_BURST;
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (resolution == OUT_2_BIT)
	{
		switch (mode)
		{
			case STANDARD_MODE:
			case PILEUP_MODE:
			case PILEUP_PPP_MODE:
				if (threshold == SINGLE_THRESHOLD)    { params.imagingMode = UHSS::ImagingMode::B2_1S; }
				else                                  { params.imagingMode = UHSS::ImagingMode::B2_2S; }
				break;
				
			case ZDT_MODE:
				params.imagingMode = UHSS::ImagingMode::B2_ZERO_DEADTIME;
				break;
				
			case BURST_MODE:
				params.imagingMode = UHSS::ImagingMode::B2_BURST;
				break;
		}
		
		params.outputMode = UHSS::OutputMode::UNSIGNED_8BIT;
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (resolution == OUT_1_BIT)
	{
		params.outputMode = UHSS::OutputMode::UNSIGNED_8BIT;
		params.imagingMode = UHSS::ImagingMode::B1_BURST;
		setIntegerParam(NDDataType, NDUInt8);
	}
	
	params.expTriggerMode = UHSS::TriggerMode::RISING_EDGE;
	params.exposureDelay = 0.0;
	params.readoutBits = 0;
	
	double exposure, interval, exp_delay, acq_delay;
	
	this->getDoubleParam(ADAcquireTime, &exposure);
	this->getDoubleParam(ADAcquirePeriod, &interval);
	this->getDoubleParam(RigakuAcquisitionDelay, &acq_delay);
	
	//Exposure time is set in seconds but sent in milliseconds
	params.exposureTime = exposure * 1000;
	params.exposureInterval = interval * 1000;
	params.acquisitionDelay = acq_delay;
	
	this->unlock();
	
	api.setParameters(params);
	
	int corrections_enabled;

	this->getIntegerParam(RigakuCorrections, &corrections_enabled);
	
	if (corrections_enabled)
	{
		int values[9] = { 0 };
		int test[1] = { 0 };
		
		this->getIntegerParam(RigakuBadPixel, &values[0]);
		this->getIntegerParam(RigakuCountingRate, &values[1]);
		this->getIntegerParam(RigakuInterChip, &values[2]);
		this->getIntegerParam(RigakuFlatField, &values[3]);
		this->getIntegerParam(RigakuRedistribution, &values[4]);
		this->getIntegerParam(RigakuDifferentiation, &values[5]);
		this->getIntegerParam(RigakuOuterEdge, &values[6]);
		this->getIntegerParam(RigakuPileup, &values[7]);

		api.controlCorrections(test, 8, values);
	}

	std::string filename;
	std::string filepath;
	
	this->getStringParam(RigakuFilepath, filepath);
	this->getStringParam(RigakuFilename, filename);
	
	std::string fullpath = filepath;
	
	if (filepath.size() > 0 && filepath.at(filepath.length() - 1) != '/')    { fullpath += "/"; }
	
	fullpath += filename;
	
	api.controlCorrection("Diversion", "filepath", fullpath.c_str());

	
	if(api.startAcq())
	{
		this->lock();
		this->setStringParam(ADStatusMessage, "");
		setIntegerParam(ADAcquire, 1);
		setIntegerParam(ADStatus, ADStatusAcquire);
		return asynSuccess;
	}
	else
	{
		this->lock();
		this->setStringParam(ADStatusMessage, "Error in starting acquire");
		setIntegerParam(ADAcquire, 0);
		setIntegerParam(ADStatus, ADStatusError);
		return asynError;
	}
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
