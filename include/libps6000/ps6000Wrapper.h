#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <vector>
#include <bitset>
#include <string>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include "ps6000Api.h"
#else
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#ifndef PS6000API
#include <libps6000/ps6000Api.h>
#endif

#ifndef PICO_STATUS
#include <libps6000/PicoStatus.h>
#endif

typedef enum enBOOL{FALSE,TRUE} BOOL;

typedef enum {
	MODEL_NONE = 0,
	MODEL_PS6402  = 0x6402, //Bandwidth: 350MHz, Memory: 32MS, AWG
	MODEL_PS6402A = 0xA402, //Bandwidth: 250MHz, Memory: 128MS, FG
	MODEL_PS6402B = 0xB402, //Bandwidth: 250MHz, Memory: 256MS, AWG
	MODEL_PS6402C = 0xC402, //Bandwidth: 350MHz, Memory: 256MS, AWG
	MODEL_PS6402D = 0xD402, //Bandwidth: 350MHz, Memory: 512MS, AWG
	MODEL_PS6403  = 0x6403, //Bandwidth: 350MHz, Memory: 1GS, AWG
	MODEL_PS6403A = 0xA403, //Bandwidth: 350MHz, Memory: 256MS, FG
	MODEL_PS6403B = 0xB403, //Bandwidth: 350MHz, Memory: 512MS, AWG
	MODEL_PS6403C = 0xC403, //Bandwidth: 350MHz, Memory: 512MS, AWG
	MODEL_PS6403D = 0xD403, //Bandwidth: 350MHz, Memory: 1GS, AWG
	MODEL_PS6404  = 0x6404, //Bandwidth: 500MHz, Memory: 1GS, AWG
	MODEL_PS6404A = 0xA404, //Bandwidth: 500MHz, Memory: 512MS, FG
	MODEL_PS6404B = 0xB404, //Bandwidth: 500MHz, Memory: 1GS, AWG
	MODEL_PS6404C = 0xC404, //Bandwidth: 350MHz, Memory: 1GS, AWG
	MODEL_PS6404D = 0xD404, //Bandwidth: 350MHz, Memory: 2GS, AWG
	MODEL_PS6407  = 0x6407, //Bandwidth: 1GHz,	 Memory: 2GS, AWG

} MODEL_TYPE;

typedef struct
{
	int16_t DCcoupled;
	int16_t range;
	int16_t enabled;
}CHANNEL_SETTINGS;

typedef struct tTriggerDirections
{
	enum enPS6000ThresholdDirection channelA;
	enum enPS6000ThresholdDirection channelB;
	enum enPS6000ThresholdDirection channelC;
	enum enPS6000ThresholdDirection channelD;
	enum enPS6000ThresholdDirection ext;
	enum enPS6000ThresholdDirection aux;
}TRIGGER_DIRECTIONS;

typedef struct tPwq
{
	struct tPS6000PwqConditions * conditions;
	int16_t nConditions;
	enum enPS6000ThresholdDirection direction;
	uint32_t lower;
	uint32_t upper;
	PS6000_PULSE_WIDTH_TYPE type;
}PWQ;

typedef struct
{
	int16_t handle;
	int8_t					modelString[8];
	int8_t					serial[10];
	int16_t					complete;
	int16_t					openStatus;
	int16_t					openProgress;
	PS6000_RANGE			firstRange;
	PS6000_RANGE			lastRange;
	int16_t					channelCount;
	BOOL					AWG;
	CHANNEL_SETTINGS		channelSettings [PS6000_MAX_CHANNELS];
	int32_t					awgBufferSize;
}UNIT;

/****************************************************************************
* Initialise unit' structure with Variant specific defaults
****************************************************************************/
void set_info(UNIT *unit);

/****************************************************************************
* SetDefaults - restore default settings
****************************************************************************/
void SetDefaults(UNIT *unit);

/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT *unit, int16_t ranges[4]);

std::vector<std::vector<int16_t*>> SetDataBuffers(UNIT *unit, std::bitset<4> activeChannels, 
	std::vector<uint16_t> samplesPostPerChannel, uint16_t samplesPreTrigger, 
	uint32_t numWaveforms, uint16_t maxPostSamples);

void SetTimebase(UNIT *unit, uint8_t timebase, uint16_t maxChSamples);

void SetSimpleTriggerSettings(UNIT *unit, int16_t threshold, 
		PS6000_THRESHOLD_DIRECTION dir, PS6000_CHANNEL ch);

void StartRapidBlock(UNIT *unit, uint16_t preTrigger, uint16_t postTriggerMax,
	uint8_t timebase, uint32_t numWaveforms);

void disableTrigger(UNIT *unit);

void SetMultiTriggerSettings(UNIT *unit, std::bitset<5> triggers, std::vector<int8_t> thresholds,
		int8_t auxThreshold);

void SetTriggers(UNIT *unit, std::bitset<5> triggers, std::vector<int16_t> chThreshold, int16_t auxThreshold);

PICO_STATUS OpenDevice(UNIT *unit, int8_t *serial);

PICO_STATUS HandleDevice(UNIT * unit);

void CloseDevice(UNIT *unit);

void findUnit(UNIT *unit);

void findUnit(UNIT *unit, int8_t *serial);

void enumUnits(int16_t *count, char* outSerials, int16_t *serialLth);

void PREF4 CallBackBlock(int16_t handle, PICO_STATUS status, void * pParameter);

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t mv_to_adc(int16_t mv, int16_t ch);

void ClearPulseGen(UNIT *unit);

void PicoSquarePulseGen(UNIT *unit, uint32_t PeakValue, double Width);

