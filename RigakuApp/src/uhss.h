/**
 * Class definitions for Ultra High Speed Server
 *
 */
#ifndef	_UHSS_H
#define	_UHSS_H
#ifndef	_WIN32
#define	UHSS_API
#else
#ifdef	UHSS_EXPORTS
#define	UHSS_API	__declspec(dllexport)
#else
#define	UHSS_API	__declspec(dllimport)
#endif
#endif

namespace UHSS {

	/**
	 * Status code definitions
	 */
	namespace	Status {

		const int	INIT			= 0;
		const int	IDLE			= 1;
		const int	BUSY			= 2;
		const int	PROCESSING		= 4;
		const int	IN_ERRORS		= 9;
		const int	CAMERA_DISCONNECTED	= 1;
		const int	CAMERA_CONNECTED	= 2;
		const int	CAMERA_SIMULATED	= 3;
		const int	CAMERA_SIMULATORERR	= 9;
		const int	MEASUREMENT		= 2;
		const int	RECORRECTION		= 3;
		const int	CALIBRATION		= 4;
		const int	NOIMAGE			= 0;
		const int	IN_PROGRESS		= 1;
		const int	NORMAL_END		= 2;
		const int	TERMINATED		= 3;

	};

	/**
	 * Acqision mode
	 */
	namespace	AcquisitionMode {

		const int	FIXED_TIME				= 0;
		const int	CONT_WITH_TRIGGER			= 1;
		const int	START_WITH_TRIGGER			= 2;
		const int	GATED_TRIGGER				= 3;
		const int	FIXED_TIME_ZERO_DEAD			= 4;
		const int	CONT_WITH_TRIGGER_ZERO_DEAD		= 5;
		const int	START_WITH_TRIGGER_ZERO_DEAD		= 6;
		const int	START_WITH_TRIGGER_FIXED_TIME		= 7;
		const int	START_WITH_TRIGGER_FIXED_TIME_ZERO_DEAD	= 8;
		const int	BURST					= 9;
		const int	START_WITH_TRIGGER_BURST		= 10;
		const int	GATED_TRIGGER_BURST			= 11;
		const int	TRIGGER_SYNC				= 12;
		const int	START_WITH_TRIGGER_PILEUP		= 13;
		const int	GATED_TRIGGER_PILEUP			= 14;
		const int	TRIGGER_SYNC_PILEUP			= 15;
		const int	START_WITH_TRIGGER_PILEUP_PPP		= 16;

	};

	/**
	 * Imaging mode
	 */
	namespace	ImagingMode {

		const int	B32_SINGLE		= 1;
		const int	B16_2S			= 2;
		const int	B16_ZERO_DEADTIME	= 4;
		const int	B8_ZERO_DEADTIME	= 5;
		const int	B4_ZERO_DEADTIME	= 6;
		const int	B2_ZERO_DEADTIME	= 7;
		const int	B8_2S			= 8;
		const int	B4_2S			= 9;
		const int	B2_2S			= 10;
		const int	B16_1S			= 11;
		const int	B8_1S			= 12;
		const int	B4_1S			= 13;
		const int	B2_1S			= 14;
		const int	B2_BURST		= 15;
		const int	B1_BURST		= 16;
		const int	B3_BURST		= 17;
		const int	B4_ZD_SPARSE_DATA	= 18;
		const int	B2_ZD_SPARSE_DATA	= 19;

	};

	/**
	 * Output mode
	 */
	namespace	OutputMode {

		const int	UNSIGNED_16BIT		= 0;
		const int	IEEE_FLOAT		= 1;
		const int	SIGNED_32BIT		= 2;
		const int	UNSIGNED_8BIT		= 3;

	};

	/**
	 * Trigger mode
	 */
	namespace	TriggerMode {

		const int	RISING_EDGE		= 0;
		const int	FALLING_EDGE		= 1;
	};

	/**
	 * Detector types
	 */
	namespace	DetectorType {

		const int	UNKNOWN			= 0;
		const int	DC_TYPE			= 1;
		const int	AC_TYPE			= 2;

	};

	namespace	ImageCorrection {

		// Displacement in an array

		const int	BAD_PIXEL		= 0;
		const int	COUNTING_RATE		= 1;
		const int	INTER_CHIP		= 2;
		const int	FLAT_FIELD		= 3;
		const int	REDISTRIBUTION		= 4;
		const int	DIFFERENTIATION		= 5;
		const int	OUTER_EDGE		= 6;
		const int	PILEUP			= 7;
		const int	DIVERSION		= 8;
		const int	CLIP			= 9;
		const int	BINNING			= 10;

		// Values of the elements

		const int	DISABLE			= 0;
		const int	ENABLE			= 1;
		const int	BP_ZERO			= 1;
		const int	BP_AVERAGE		= 2;
		const int	BP_FILTER		= 3;
		const int	CR_MAXIMUM		= 1;
		const int	CR_M3			= 2;
		const int	IC_INTERPOLATE		= 1;
		const int	IC_ZERO			= 2;
		const int	FF_ENERGYONLY		= 2;
		const int	TWO_BY_TWO		= 1;
		const int	FOUR_BY_FOUR		= 2;

	};

	/**
	 * Zero Dead-time data length
	 */
	namespace	ZDTDataLength {

		const int	ZDT_16BIT		= 0;
		const int	ZDT_8BIT		= 1;
		const int	ZDT_4BIT		= 2;
		const int	ZDT_2BIT		= 3;

	};

	/**
	 * Degree of noise elimination
	 */
	namespace	NoiseElimination {

		const int	LOW			= 0;
		const int	MIDDLE			= 1;
		const int	HIGH			= 2;
	};

	/**
	 * Enable OUT
	 */
	namespace	EnableOUT {

		const int	EXPOSURE_MODE		= 0;
		const int	ITERATION_MODE		= 1;
		const int	START_TO_STOP_MODE	= 2;
	}

	/**
	 * LED state
	 */
	namespace	LEDState {

		const int	OFF			= 0;
		const int	ON			= 1;
	}

	/**
	 * Offset Param
	 */
	namespace	OffsetParam {

		const int	TRIM			= 0;
		const int	BG			= 1;
	}

	/**
	 * Correction File Type
	 */
	namespace	CorrectionFileType {

		const int	COR_FILE		= 0;
		const int	VL_FILE			= 1;
	}

	/**
	 * Template of Array classes
	 */
	template<typename _T>	class	UHSS_API	Array {
	private:
		static	_T	defValue;
		_T		*array;
	public:
		int		size;
		void		setup(int sz, _T *ptr);
		_T		operator[](int idx) const;
				Array<_T>();
	};

	/**
	 * Dataset information
	 */
	typedef	struct {
		int		serialNumber;
		int		imagingMode;
		int		outputMode;
		int		numRows;
		int		numColumns;
		int		elementSize;
		int		frameSize;
		int		numFrames;
		int		attenuation;
		double		exposureTime;
		double		lowerEnergy;
		double		upperEnergy;
		double		resolution;
		double		shiftTo;
		int		parseDataSize;
		const char	*calibLabel;
		const char	*date;
	} DatasetInfo;

	/**
	 * Calibration table
	 */
	typedef	struct {
		const char	*label;
		double		minEnergy;
		double		maxEnergy;
		Array<double>	energy;
		Array<int>	attenuation;
	} CalibTable;

	typedef struct {
		int		RCrefPLX;
		int		bgmainSH;
		int		bbs;
		int		bcr;
		int		bofs;
		int		bgsh;
		int		EN_CAP_SW;
		int		bg;
		int		bgcas;
		int		bl;
		int		bh;
	} trimDAC;

	/**
	 * State information
	 */
	class	UHSS_API	State {
	public:
		bool			power;
		int			operationState;
		int			serverState;
		int			connectionState;
		int			serverOperationState;
		int			acquisitionState;
		int			numRequiredFrames;
		int			numAquiredFrames;
		int			numAvailableFrames;
		long long		elapseTime;
		long long		freeStorageSpace;
		DatasetInfo		outputDataset;
		Array<DatasetInfo>	datasetList;
		Array<int>		errors;
	};

	/**
	 * Environment information
	 */
	class	UHSS_API	Environment {
	public:
		Array<double>		moduleTemperatures;
		Array<double>		temperatureDifferences;
		Array<double>		moduleHumidities;
		Array<double>		FGTemperatues;
	};

	/**
	 * Detector onfiguration
	 */
	class	UHSS_API	Configuration {
	public:
		int			numModules;
		Array<const char *>	detectorIDs;
		int			numColumns;
		int			numRows;
		double			pixelSize;
		double			detectorWidth;	// Dimensions in mm
		double			detectorHeight;
		Array<int>		moduleWidth;
		Array<int>		moduleHeight;
		int			detectorType;
		int			ledState;
		int			enableOut;
		int			lowerThreshold;	// Discriminator
		int			upperThreshold;
		int			refThreshold;
		int			autoStreaming;
		Array<int>		clippingArea;
		Array<int>		corrections;
		CalibTable		calibTable;
		Array<CalibTable>	calibTableList;
		double			lowerEnergy;	// In keV
		double			upperEnergy;
		double			minEnergy;
		double			maxEnergy;
	};

	/**
	 * Acquisition parameters
	 */
	class	UHSS_API	Parameters {
	public:
		int			numFrames;
		int			numPileup;
		int			acquisitionMode;
		int			outputMode;
		int			imagingMode;
		int			acqTriggerMode;
		int			expTriggerMode;
		int			zeroDeadTimeDataLength;
		int			readoutBits;
		int			noiseElimination;
		int			parseDataSize;
		int			bitskip;
		unsigned int		exposureTime_clk;
		unsigned int		exposureInterval_clk;
		unsigned int		exposureDelay_clk;
		unsigned int		acquisitionDelay_clk;
		double			exposureTime;
		double			exposureInterval;// In mili-seconds
		double			exposureDelay;
		double			acquisitionDelay;

		Parameters		&operator=(const Parameters &);
					Parameters();
	};

	template <typename _T>	struct	PRange {
		_T			lower;
		_T			upper;
	};

	class	UHSS_API	ParameterRanges {
	public:
		PRange<int>		numFrames;
		PRange<int>		numPileup;
		PRange<int>		acquisitionMode;
		PRange<int>		outputMode;
		PRange<int>		imagingMode;
		PRange<int>		acqTriggerMode;
		PRange<int>		expTriggerMode;
		PRange<int>		zeroDeadTimeDataLength;
		PRange<int>		readoutBits;
		PRange<int>		noiseElimination;
		PRange<int>		parseDataSize;
		PRange<int>		bitskip;
		PRange<unsigned int>	exposureTime_clk;
		PRange<unsigned int>	exposureInterval_clk;
		PRange<unsigned int>	exposureDelay_clk;
		PRange<unsigned int>	acquisitionDelay_clk;
		PRange<double>		exposureTime;
		PRange<double>		exposureInterval;
		PRange<double>		exposureDelay;
		PRange<double>		acquisitionDelay;
	};

	enum	StatusEvent {
		StateChanged,
		EnvironmentChanged,
		ConfigurationChanged,
		FrameAvailable,
		ErrorPushed
	};

	class	UHSS_API	ManagerCallback;

	/**
	 * Acqisition Manager Interface
	 */
	class	UHSS_API	AcqManager {
	public:
		virtual	const State		&getState() = 0;
		virtual	const Environment	&getEnvironment() = 0;
		virtual	const Configuration	&getConfiguration() = 0;
		virtual	const Parameters	&getParameters() = 0;

		virtual	const char	*getVersion() = 0;
		virtual	bool		initialize(const char *) = 0;
		virtual	void		shutdown(int = 0) = 0;
		virtual	int		logLevel(int = -1) = 0;
		virtual	bool		clearError() = 0;
		virtual	bool		setParameters(const Parameters &) = 0;
		virtual	bool		restart(bool) = 0;
		virtual	bool		startAcq() = 0;
		virtual	bool		monitor() = 0;
		virtual	bool		startStreaming(
						int, int, int, int = -1) = 0;
		virtual	void		stop() = 0;
		virtual	int		getImages(char *, int, int) = 0;
		virtual bool		controlCorrections(int [4],
							int = 0, int * = 0) = 0;
		virtual bool		controlCorrection(const char *,
							const char *,
							int) = 0;
		virtual bool		controlCorrection(const char *,
							const char *,
							double) = 0;
		virtual bool		controlCorrection(const char *,
							const char *,
							const char *) = 0;
		virtual bool		controlCorrection(const char *,
							int) = 0;
		virtual	bool		deleteDataset(int) = 0;
		virtual bool		setEnableOUT(int) = 0;
		virtual bool		setLEDState(int) = 0;
		virtual	bool		setThreshold(int, int, int) = 0;
		virtual	bool		setEnergy(const char *,
						double, double, int) = 0;
		virtual bool		setAlternateGain(float, float) = 0;
		virtual	bool		setCalibTable(const char *,
						double, double, int, int) = 0;
		virtual	bool		reCorrect(int) = 0;
		virtual	bool		remakeCorrectionFiles(int, int) = 0;
		virtual	bool		savePower(int) = 0;
		virtual	void		emergency() = 0;
		virtual	bool		sendCommand(const char *) = 0;
		virtual	const char	*getResponse() = 0;
		virtual	const char	*translateError(int, int &) = 0;
		virtual	void		log(const char *, int = 2) = 0;
		virtual	bool		setFramePatternData(int,
						const char *, int, int = -1) = 0;
		virtual	bool		setFramePatternData(const char *,
						int, int = -1) = 0;
		virtual	bool		setFramePatternData(int, int = -1) = 0;
		virtual	bool		clearFramePatternData(int = -1) = 0;
		virtual	void		thScan(unsigned int, int, int, const char *) = 0;
		virtual	void		setTrimDAC(const char *) = 0;
		virtual	void		riceScan(const trimDAC &, unsigned int, int, int, const char *) = 0;
		virtual	void		riceCorrection(unsigned int, const char *, const char *) = 0;
		virtual	void		peakScan(const trimDAC &, unsigned int, int, int, const char *, int, bool) = 0;
		virtual	void		gainCorrection(unsigned int, const char *, const char *, int) = 0;
		virtual	void		thScanFit(const char *, const char *, const char *) = 0;
		virtual	bool		ZDFlatCorrection(double, int, int, int, double, int, int, int) = 0;
		virtual	const Array<const float*>&	getEnergyResolutionMap() = 0;
		virtual	const Array<const float*>&	getThresholdEnergyMap() = 0;
		virtual	const char*	getAppliedAlternateGainMap() = 0;
		virtual	void		setCallback(ManagerCallback &) = 0;

		virtual	const	ParameterRanges
					&getParameterRanges() = 0;
	};

	/**
	 * Callback
	 */
	class	UHSS_API	ManagerCallback {
	public:
		virtual	void		notify(AcqManager &, StatusEvent) = 0;
	};

	extern UHSS_API	AcqManager	&getAPI();

	extern	const char		*version;

};
#endif
