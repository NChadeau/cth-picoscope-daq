#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <vector>
#include <bitset>
#include <string>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include "ps6000aApi.h"
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
#include <libps6000a/ps6000aApi.h>
#endif

#ifndef PICO_STATUS
#include <libps6000a/PicoStatus.h>
#endif

#define PS6000A_MAX_CHANNELS 8 //analog chs only

typedef enum enBOOL{FALSE,TRUE} BOOL;

typedef struct
{
	int16_t DCcoupled;
	int16_t range;
	int16_t enabled;
}CHANNEL_SETTINGS;

typedef enum
{
	MODEL_NONE = 0,//this is used
//models values not used in the code
} MODEL_TYPE;

typedef struct
{
	int16_t 					handle;
	MODEL_TYPE					model;
	int8_t						modelString[8];
	int8_t						serial[10];
	int16_t						complete;
	int16_t						openStatus;
	int16_t						openProgress;
	PICO_CONNECT_PROBE_RANGE	firstRange;
	PICO_CONNECT_PROBE_RANGE	lastRange;
	int16_t						channelCount;
	int16_t						maxADCValue;
	CHANNEL_SETTINGS			channelSettings [PS6000A_MAX_CHANNELS];
	PICO_DEVICE_RESOLUTION		resolution;
	int16_t						digitalPortCount;
}UNIT;

void set_info(UNIT *unit);

void SetDefaults(UNIT *unit);

void SetVoltages(UNIT *unit, int16_t ranges[4]);

std::vector<std::vector<int16_t*>> SetDataBuffers(UNIT *unit, std::bitset<4> activeChannels, 
	std::vector<uint16_t> samplesPostPerChannel, int16_t samplesPreTrigger, 
	uint32_t numWaveforms, uint16_t maxPostSamples);

void SetTimebase(UNIT *unit, uint8_t timebase, uint16_t maxChSamples);

void SetSimpleChannelTrigger(UNIT *unit, int16_t threshold, 
		PICO_THRESHOLD_DIRECTION dir, PICO_CHANNEL ch);

void SetSimpleAuxTrigger(UNIT *unit);

void SetDaqDelay(UNIT *unit, int16_t delay);

void StartRapidBlock(UNIT *unit, int16_t preTrigger, uint16_t postTriggerMax,
	uint8_t timebase, uint32_t numWaveforms);

void StartMultiRapidBlock(std::vector<UNIT *> vecUnit, std::vector<int16_t> vecPreTrigger,
	std::vector<uint16_t> vecPostTriggerMax, std::vector<uint8_t> vecTimebase,
	std::vector<uint32_t> vecNumWaveforms);

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

void PREF4 MultiCallBackBlock(int16_t handle, PICO_STATUS status, void *pParameter);

int16_t mv_to_adc(int16_t mv, int16_t rangeIndex, UNIT *unit);
