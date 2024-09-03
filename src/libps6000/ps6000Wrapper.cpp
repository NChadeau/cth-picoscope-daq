#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <assert.h>
#include <bitset>
#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <chrono>

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

#ifndef PS6000WRAPPER
#include "libps6000/ps6000Wrapper.h"
#endif

using namespace std;


uint16_t inputRanges [PS6000_MAX_RANGES] = {	10,
												20,
												50,
												100,
												200,
												500,
												1000,
												2000,
												5000,
												10000,
												20000,
												50000};
BOOL        g_ready = FALSE;

void set_info(UNIT *unit)
{
	int16_t i = 0;
	int16_t r = 20;
	int8_t line [20];
    char* charLine[20];
	int32_t variant;

	int8_t description [11][25]= { "Driver Version",
		"USB Version",
		"Hardware Version",
		"Variant Info",
		"Serial",
		"Cal Date",
		"Kernel",
		"Digital H/W",
		"Analogue H/W",
		"Firmware 1",
		"Firmware 2"};

	if (unit->handle) 
	{
		for (i = 0; i < 11; i++) 
		{
			ps6000GetUnitInfo(unit->handle, line, sizeof (line), &r, i);
		
			if (i == 3) 
			{
				// info = 3 - PICO_VARIANT_INFO
		
				variant = atoi((char *)line);
				memcpy(&(unit->modelString),line,sizeof(unit->modelString)==7?7:sizeof(unit->modelString));
		
				//To identify A or B model variants.....
				if (strlen((char *)line) == 4)							// standard, not A, B, C or D, convert model number into hex i.e 6402 -> 0x6402
				{
					variant += 0x4B00;
				}
				else
				{
					if (strlen((char *) line) == 5)						// A, B, C or D variant unit 
					{
						line[4] = toupper(line[4]);

						switch(line[4])
						{
							case 65: // i.e 6402A -> 0xA402
								variant += 0x8B00;
								break;
							case 66: // i.e 6402B -> 0xB402
								variant += 0x9B00;
								break;
							case 67: // i.e 6402C -> 0xC402
								variant += 0xAB00;
								break;
							case 68: // i.e 6402D -> 0xD402
								variant += 0xBB00;
								break;
							default:
								break;
						}
					}
				}
			}

			if (i == 4) 
			{
				// info = 4 - PICO_BATCH_AND_SERIAL
				ps6000GetUnitInfo(unit->handle, unit->serial, sizeof (unit->serial), &r, PICO_BATCH_AND_SERIAL);
			}

			printf("%s: %s\n", description[i], line);
		}

		switch (variant)
		{
			case MODEL_PS6402:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402A:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402B:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402C:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402D:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = PS640X_C_D_MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403A:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403B:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403C:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403D:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = PS640X_C_D_MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404A:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404B:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404C:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404D:
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = PS640X_C_D_MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6407:
				unit->firstRange = PS6000_100MV;
				unit->lastRange = PS6000_100MV;
				unit->channelCount = 4;
				unit->AWG = TRUE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_100MV;
					unit->channelSettings[i].DCcoupled = PS6000_DC_50R;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			default:
				break;
		}
	}
}

/****************************************************************************
* SetDefaults - restore default settings
****************************************************************************/
void SetDefaults(UNIT *unit)
{
	PICO_STATUS status;
	int32_t i;

	status = ps6000SetEts(unit->handle, PS6000_ETS_OFF, 0, 0, NULL); // Turn off ETS

	for (i = 0; i < unit->channelCount; i++) // reset channels to most recent settings
	{
		status = ps6000SetChannel(unit->handle, (PS6000_CHANNEL) (PS6000_CHANNEL_A + i),
			unit->channelSettings[PS6000_CHANNEL_A + i].enabled,
			PS6000_AC,
			// (PS6000_COUPLING)unit->channelSettings[PS6000_CHANNEL_A + i].DCcoupled,
			(PS6000_RANGE)unit->channelSettings[PS6000_CHANNEL_A + i].range, 0, PS6000_BW_FULL);
	}

}

/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT *unit, int16_t ranges[4])
{
	int32_t ch;
	int32_t count = 0;

	for (ch = 0; ch < unit->channelCount; ch++) 
	{
		unit->channelSettings[ch].range = ranges[ch];

		if (unit->channelSettings[ch].range != 99) 
		{
			unit->channelSettings[ch].enabled = TRUE;
			count++;
		} 
		else 
		{
			unit->channelSettings[ch].enabled = FALSE;
		}
	}

	if (count == 0) {throw invalid_argument("No active channels");}


	SetDefaults(unit);	// Put these changes into effect
}


void SetTimebase(UNIT *unit, uint8_t timebase, uint16_t maxChSamples) // Don't need this???
{
    // Not foolproof, should check vs range of selected picoscope
    assert(timebase <= 5 && timebase >= 0);

    float timeInterval = 0.00f;
	uint32_t maxSamples;
	PICO_STATUS status;

	
    status = ps6000GetTimebase2(unit->handle, timebase, maxChSamples, &timeInterval, 1, &maxSamples, 0);

    if(status != PICO_OK)
    {
        throw invalid_argument("Invalid timebase given for this unit");
    }
	

	// bool oversample = TRUE;

	return;

}

vector<vector<int16_t*>> SetDataBuffers(UNIT *unit, bitset<4> activeChannels, 
	vector<uint16_t> samplesPostPerChannel, uint16_t samplesPreTrigger, 
	uint32_t numWaveforms, uint16_t maxPostSamples)
{//Using rapid block mode only for now
	vector<vector<int16_t*>> outBuffers(activeChannels.count());

	enPS6000Channel ch = PS6000_CHANNEL_A;

	uint32_t nMaxSamples = activeChannels.count() * maxPostSamples;
	PICO_STATUS status = ps6000MemorySegments(unit->handle, numWaveforms, &nMaxSamples);
	if (status != PICO_OK)
	{
		throw "Improperly set pico memory segments";
	}
	ps6000SetNoOfCaptures(unit->handle, numWaveforms);

	for (int i = 0; i < 4; i++)
	{
		if (!activeChannels.test(i)) {continue;}

		uint16_t chSamples = samplesPreTrigger + samplesPostPerChannel.at(i);
		outBuffers.at(i) = vector<int16_t*>(numWaveforms);

		for (int j = 0; j < numWaveforms; j++)
		{
			outBuffers.at(i).at(j) = (int16_t*) calloc(chSamples, sizeof(int16_t));
			if (outBuffers.at(i).at(j) == NULL)
			{
				printf("Memory allocation failed: Ch %d, Wf %d\n", i, j);
			}
			ps6000SetDataBufferBulk(unit->handle, (PS6000_CHANNEL) (ch + i),
				outBuffers.at(i).at(j), chSamples, j, PS6000_RATIO_MODE_NONE);
		}
	}

	return outBuffers;
}

void SetSimpleTriggerSettings(UNIT *unit, int16_t threshold, 
		PS6000_THRESHOLD_DIRECTION dir, PS6000_CHANNEL ch)
{
	disableTrigger(unit);

	ps6000SetSimpleTrigger(unit->handle, 1, ch, threshold, dir, 0, 0);
	return;
}

void disableTrigger(UNIT *unit)
{
	ps6000SetTriggerChannelProperties(unit->handle, NULL, 0, 0, 0);
	ps6000SetTriggerChannelConditions(unit->handle,	NULL, 0);
	ps6000SetTriggerChannelDirections(  unit->handle, 
										PS6000_ABOVE,
										PS6000_ABOVE,
										PS6000_ABOVE,
										PS6000_ABOVE,
										PS6000_ABOVE,
										PS6000_ABOVE);
	ps6000SetTriggerDelay(unit->handle, 0);

	struct tPwq pwq;
	memset(&pwq, 0, sizeof(struct tPwq));

	ps6000SetPulseWidthQualifier(   unit->handle, 
								    pwq.conditions,
								    pwq.nConditions, 
								    pwq.direction,
								    pwq.lower, 
								    pwq.upper, 
								    pwq.type);

	return;
}

void SetMultiTriggerSettings(UNIT *unit, bitset<5> triggers, vector<int8_t> thresholds,
		int8_t auxThreshold)
{   // NOTE: Ignores external channel
	assert(thresholds.size() == 4);

	throw "I haven't made this function yet: SetMultiTriggerSettings";

	// RECAP FROM LAST TIME: WRITING SIMPLE ONE TRIGGER AND COMBINED SIMPLE TRIGGERS
	// USING ARRAY OF PS6000_TRIGGER_CONDITIONS STRCTURES
	return;
}

void SetTriggers(UNIT *unit, bitset<5> triggers, vector<int16_t> chThreshold, int16_t auxThreshold)
{
	assert(chThreshold.size() == 4);
	
	if (triggers.count() == 0)
	{
		throw "No set triggers";
	}
	if (triggers.count() == 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (triggers.test(i + 1))
			{
				SetSimpleTriggerSettings(unit, chThreshold.at(i), (PS6000_THRESHOLD_DIRECTION)
					((chThreshold.at(i) >= 0) ? PS6000_RISING : PS6000_FALLING), (PS6000_CHANNEL) (PS6000_CHANNEL_A + i));
				break;
			}
		}
		if (triggers.test(0))
		{
			SetSimpleTriggerSettings(unit, auxThreshold, (PS6000_THRESHOLD_DIRECTION)
					((auxThreshold >= 0) ? PS6000_RISING : PS6000_FALLING), (PS6000_CHANNEL) (PS6000_TRIGGER_AUX));
		}
	}
	else
	{
		printf("multi trig (will fail since unimplemented)\n");
		// SetMultiTriggerSettings(unit, triggers, chThreshold, auxThreshold);
	}
	return;
}

int32_t _getch()
{
        struct termios oldt, newt;
        int32_t ch;
        int32_t bytesWaiting;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        setbuf(stdin, NULL);
        do {
                ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
                if (bytesWaiting)
                        getchar();
        } while (bytesWaiting);

        ch = getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
}

int32_t _kbhit()
{
        struct termios oldt, newt;
        int32_t bytesWaiting;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        setbuf(stdin, NULL);
        ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return bytesWaiting;
}


void StartRapidBlock(UNIT *unit, uint16_t preTrigger, uint16_t postTriggerMax,
	uint8_t timebase, uint32_t numWaveforms)
{
	PICO_STATUS status;
	int32_t timeIndisposed;
	uint32_t nCompletedCaptures;

	printf("\n\nStarting DAQ\n\n");

	chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	ps6000RunBlock(unit->handle, preTrigger, postTriggerMax, timebase, 0, 
			&timeIndisposed, 0, CallBackBlock, NULL);
	g_ready = FALSE;

	while (!g_ready && !_kbhit()) // XXX: Should change to only cancel if getch == ctrl+c
	{
		usleep(0);
	}

	if (!g_ready)
	{
		_getch();
		status = ps6000Stop(unit->handle);
		status = ps6000GetNoOfCaptures(unit->handle, &nCompletedCaptures);

		printf("Rapid capture aborted. %d complete blocks were captured\n", nCompletedCaptures);
		printf("Early abort writeout not yet supported\n");

		throw "aborted, need to implement early cancellation writeout";

	}

	chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	int time = chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

	printf("Time taken: %d ms\n", time);
	printf("Trigger rate: %f Hz\n", (double) numWaveforms / time * 1.0e3);

	status = ps6000GetNoOfCaptures(unit->handle, &nCompletedCaptures);


	uint32_t nSamples = preTrigger + postTriggerMax;

	// Get data
	status = ps6000GetValuesBulk(unit->handle, &nSamples, 0, numWaveforms - 1, 
			1, PS6000_RATIO_MODE_NONE, NULL);
	
	// ps6000GetValuesTriggerTimeOffsetBulk64

	// Stop
	status = ps6000Stop(unit->handle);

	status = ps6000MemorySegments(unit->handle, 1, &nSamples);
	status = ps6000SetNoOfCaptures(unit->handle, 1);

	return;
}

PICO_STATUS OpenDevice(UNIT *unit, int8_t *serial)
{
	PICO_STATUS status;

	if (serial == NULL)
	{
		status = ps6000OpenUnit(&unit->handle, NULL);
	}
	else
	{
		status = ps6000OpenUnit(&unit->handle, serial);
	}

	unit->openStatus = status;
	unit->complete = 1;

	return status;
}

PICO_STATUS HandleDevice(UNIT * unit)
{
	printf("Handle: %d\n", unit->handle);
	if (unit->openStatus != PICO_OK)
	{
		printf("Unable to open device\n");
		printf("Error code : 0x%08x\n", (uint32_t)unit->openStatus);
		throw;
	}

	printf("Device opened successfully\n\n");
	set_info(unit);

	SetDefaults(unit);

	disableTrigger(unit);

	return unit->openStatus;
}

void CloseDevice(UNIT *unit)
{
	ps6000CloseUnit(unit->handle);
}

void findUnit(UNIT *unit, int8_t *serial)
{
	if (serial == NULL)
	{
		findUnit(unit);
		return;
	}

	PICO_STATUS status = OpenDevice(unit, serial);

	if (status == PICO_OK || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT)
	{
		status = HandleDevice(unit);

		if (status != PICO_OK)
		{
			printf("Picoscope devices open failed, error code 0x%x\n",(uint32_t)status);
		}

		return;
	}
}

void findUnit(UNIT *unit)
{
#define MAX_PICO_DEVICES 64
#define TIMED_LOOP_STEP 500

	int8_t ch;
	uint16_t devCount = 0, listIter = 0,	openIter = 0;
	// Device indexer -  64 chars - 64 is maximum number of picoscope devices handled by driver
	int8_t devChars[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#";
	PICO_STATUS status = PICO_OK;
	UNIT allUnits[MAX_PICO_DEVICES];


	do
	{
		memset(&(allUnits[devCount]),0,sizeof (UNIT));
		status = OpenDevice(&(allUnits[devCount]),NULL);
		if(status == PICO_OK || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT)
		{
			allUnits[devCount++].openStatus = status;
		}

	} while(status != PICO_NOT_FOUND);

	if (devCount == 0)
	{
		printf("Picoscope devices not found\n");
		return;
	}
	// If there is only one device, open and handle it here
	if (devCount == 1)
	{
		printf("Found one device, opening...\n\n");
		status = allUnits[0].openStatus;

		if (status == PICO_OK || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT)
		{
			status = HandleDevice(&allUnits[0]);
		}

		if (status != PICO_OK)
		{
			printf("Picoscope devices open failed, error code 0x%x\n",(uint32_t)status);
			return;
		}

		unit = &allUnits[0];
		return;
	} 
	else
	{
		printf("Too many picoscope devices found\n");
	}
}

void enumUnits(int16_t *count, char* outSerials, int16_t *serialLth)
{
	PICO_STATUS ps = ps6000EnumerateUnits(count, (int8_t*) outSerials, serialLth);

	return;
}

/****************************************************************************
* Callback
* used by PS6000 data block collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void PREF4 CallBackBlock(int16_t handle, PICO_STATUS status, void * pParameter)
{
	if (status != PICO_CANCELLED)
	{
		g_ready = TRUE;
	}
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t mv_to_adc(int16_t mv, int16_t ch)
{
	return (mv * PS6000_MAX_VALUE) / inputRanges[ch];
}

void ClearPulseGen(UNIT *unit)
{
	PICO_STATUS ps = ps6000SetSigGenBuiltInV2(
		unit->handle, // Handle
		0,  // Voltage offset
		0,	 // PkToPk voltage
		PS6000_SINE, // Squarewave signal
		0, // StartFrequency
		0, // StopFrequency (sweep)
		0, // Sweep Increment
		0, // Sweep Time
		PS6000_UP, // Sweep Mode
		PS6000_ES_OFF, //Operation mode
		0, // Cycle number
		0, // Sweep number
		PS6000_SIGGEN_RISING, // Trigger type (rising edge)
		PS6000_SIGGEN_NONE, // Trigger source (AUX)
		0 // AuxTrigger Voltage threshold (500 mV)
	);
}

void PicoSquarePulseGen(UNIT *unit, uint32_t PeakValue, double Width)
{
	PeakValue=PeakValue*1e3;
	Width=Width*1e-9;
	double Frequency = 1.0/(2*Width);
	if ((Frequency<=20000000)&&(PeakValue<=2000000))
	{
		PICO_STATUS ps = ps6000SetSigGenBuiltInV2(
			unit->handle, // Handle
			0,  // Voltage offset
			PeakValue*2,	 // PkToPk voltage
			PS6000_SQUARE, // Squarewave signal
			Frequency, // StartFrequency
			Frequency, // StopFrequency (sweep)
			0, // Sweep Increment
			0, // Sweep Time
			PS6000_UP, // Sweep Mode
			PS6000_ES_OFF, //Operation mode
			1, // Cycle number
			0, // Sweep number
			PS6000_SIGGEN_RISING, // Trigger type (rising edge)
			PS6000_SIGGEN_AUX_IN, // Trigger source (AUX)
			3277 // AuxTrigger Voltage threshold (500 mV)
		);
		
		if (ps != PICO_OK)
		{
		printf("error: %x\n",ps);
			throw "Error in the PicoSquarePulseGen setup.";
		}
		else 
		{
			printf("PicoSquarePulseGen is setup:\n");
			printf("PeakValue value = %f mV, Frequency = %f kHz, Width = %f ns\n", PeakValue*1e-3 , Frequency*1e-3, Width*1e9);
		}
	}
	else
	{
		printf("Either PeakValue or Width input for PicoSquarePulseGen is too high!\n");
		printf("PeakValue value = %f mV, Frequency = %f kHz, Width = %f ns\n", PeakValue*1e-3 , Frequency*1e-3, Width*1e9);
	}

}
