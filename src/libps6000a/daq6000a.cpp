#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <bitset>
#include <vector>
#include <assert.h>
#include <memory>

#ifndef PS6000WRAPPER
#include "libps6000a/ps6000aWrapper.h"
#endif

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

using namespace std;
namespace py = pybind11;

class dataCollectionConfig 
{
public:
    UNIT unit;
    bitset<4> activeChannels;
    bitset<5> activeTriggers;
    bitset<4> timebase;
    bitset<16> chVoltageRanges;
    vector<int16_t> chTriggerThresholdADC;
    int16_t auxTriggerThresholdADC;
    vector<uint16_t> chPostSamplesPerWaveform; // NOTE: Excludes pre trigger samples
    uint16_t maxPostSamples;
    int16_t samplesPreTrigger;
    uint32_t numWaveforms;
    vector<vector<int16_t*>> dataBuffers;

    BOOL dataConfigured = FALSE;
    BOOL unitInitialised = FALSE;

    char serial[32];
    dataCollectionConfig(UNIT unit, char *serial)
    {
        this->unit = unit;
        strcpy(this->serial, serial);
    }
    void print()
    {
        printf("Data Collection Config Info:\n");
        printf("Serial Number: %s\n", this->serial);
        printf("Active Channels: %u\n", (uint) this->activeChannels.to_ulong());
        printf("Active Triggers: %u\n", (uint) this->activeTriggers.to_ulong());
        printf("Timebase: %u\n", (uint) this->timebase.to_ulong());
        printf("Channel V Ranges: %u\n", (uint) this->chVoltageRanges.to_ulong());
        for (int i = 0; i < this->chTriggerThresholdADC.size(); i++)
        {
            printf("%c Trigger Threshold: %i\n", 'A' + i, this->chTriggerThresholdADC.at(i));
        }
        printf("Aux Trigger Threshold: %i\n", this->auxTriggerThresholdADC);
        for (int i = 0; i < this->chPostSamplesPerWaveform.size(); i++)
        {
            printf("%c Post Trigger Samples: %i\n", 'A' + i, this->chPostSamplesPerWaveform.at(i));
        }
        printf("Max Post Trigger Samples: %i\n", this->maxPostSamples);
        printf("Samples Pre Trigger: %i\n", this->samplesPreTrigger);
        printf("Number of Waveforms: %i\n", this->numWaveforms);
        printf("Data Configured: %s\n", this->dataConfigured ? "true" : "false");
        printf("Unit Initialised: %s\n\n", this->unitInitialised ? "true" : "false");
    }
};

bool isLittleEndian()
{
    uint32_t i = 1;
    char *c = (char*)&i;
    return bool(*c);
}

bool g_littleEndian = isLittleEndian();

UNIT g_unit;
dataCollectionConfig g_dcc(g_unit, (char*) "");
vector<dataCollectionConfig> g_vecDcc;

void setActiveChannels(dataCollectionConfig &dcc, 
                       int16_t aChVoltage,
                       int16_t bChVoltage,
                       int16_t cChVoltage,
                       int16_t dChVoltage)
{
    int16_t chVoltage[4] = {aChVoltage, bChVoltage, cChVoltage, dChVoltage};
    uint8_t chVRangeBits[4];
    uint16_t allVRange;

    for (int i = 0; i < 4; i++)
    {
        if (chVoltage[i] != 99) 
        {
            dcc.activeChannels.set(i);
        }
        chVRangeBits[i] = chVoltage[i] * dcc.activeChannels.test(i);
    }
    dcc.chVoltageRanges = chVoltage[0] * dcc.activeChannels.test(0) << 12 |
                          chVoltage[1] * dcc.activeChannels.test(1) <<  8 |
                          chVoltage[2] * dcc.activeChannels.test(2) <<  4 |
                          chVoltage[3] * dcc.activeChannels.test(3);

    SetVoltages(&dcc.unit, chVoltage);
    return;
}

void setTriggerConfig(dataCollectionConfig &dcc, 
                      int16_t aTrigVoltageMv,
                      int16_t bTrigVoltageMv,
                      int16_t cTrigVoltageMv,
                      int16_t dTrigVoltageMv,
                      int16_t auxTrigVoltageMv)
{

    int16_t trigVoltageMv[4] = {aTrigVoltageMv,
                                bTrigVoltageMv,
                                cTrigVoltageMv,
                                dTrigVoltageMv};

    vector<int16_t> chTriggerThresholdADC;

    for (int i = 0; i < 4; i++)
    {
        if (trigVoltageMv[i] != 0 && dcc.activeChannels.test(i))
        {
            dcc.activeTriggers.set(i + 1);
            chTriggerThresholdADC.push_back(mv_to_adc(trigVoltageMv[i], 
            dcc.unit.channelSettings[PICO_CHANNEL_A + i].range, &dcc.unit));
        }
        else
        {
            chTriggerThresholdADC.push_back(0);
        }
    }
    if (auxTrigVoltageMv != 0)
    {
        dcc.activeTriggers.set(0);
        dcc.auxTriggerThresholdADC = mv_to_adc(auxTrigVoltageMv, PICO_X1_PROBE_1V, &dcc.unit);
    }
    else
    {
        dcc.auxTriggerThresholdADC = 0;
    }

    dcc.chTriggerThresholdADC = chTriggerThresholdADC;
    SetTriggers(&dcc.unit, dcc.activeTriggers, dcc.chTriggerThresholdADC, dcc.auxTriggerThresholdADC);

    return;
}

void setDataConfig(dataCollectionConfig &dcc, uint8_t timebase,
    uint32_t numWaveforms, int16_t samplesPreTrigger, uint16_t chAWfSamples,
    uint16_t chBWfSamples , uint16_t chCWfSamples , uint16_t chDWfSamples)
{
    dcc.timebase = timebase;
    dcc.numWaveforms = numWaveforms;
    dcc.samplesPreTrigger = samplesPreTrigger;

    vector<uint16_t> samplesPostPerWaveform = {chAWfSamples, chBWfSamples, chCWfSamples, chDWfSamples};
    dcc.chPostSamplesPerWaveform = samplesPostPerWaveform;
    dcc.maxPostSamples = *max_element(  dcc.chPostSamplesPerWaveform.begin(),
                                        dcc.chPostSamplesPerWaveform.end());

    if (dcc.samplesPreTrigger < 0)
    {
        printf("Setting post-trigger delay of %i samples\n", -1 * dcc.samplesPreTrigger);
        SetDaqDelay(&dcc.unit, dcc.samplesPreTrigger);
    }

    dcc.dataBuffers = SetDataBuffers(&dcc.unit, dcc.activeChannels, dcc.chPostSamplesPerWaveform, 
            dcc.samplesPreTrigger, dcc.numWaveforms, dcc.maxPostSamples);

    return;
}

void freeDataBuffers(dataCollectionConfig &dcc)
{
    for (int i = 0; i < dcc.activeChannels.count(); i++)
    {
        for (int j = 0; j < dcc.numWaveforms; j++)
        {
            free(dcc.dataBuffers.at(i).at(j));
        }
    }
}

void resetDataBuffers(dataCollectionConfig &dcc)
{
    freeDataBuffers(dcc);
    dcc.dataBuffers = SetDataBuffers(&dcc.unit, dcc.activeChannels, dcc.chPostSamplesPerWaveform, 
            dcc.samplesPreTrigger, dcc.numWaveforms, dcc.maxPostSamples);
}

void collectRapidBlockData(dataCollectionConfig &dcc)
{
    uint16_t maxPostTrigger = *max_element( dcc.chPostSamplesPerWaveform.begin(),
                                            dcc.chPostSamplesPerWaveform.end());
    StartRapidBlock(&dcc.unit, dcc.samplesPreTrigger, maxPostTrigger, 
            dcc.timebase.to_ulong(), dcc.numWaveforms);
    return;
}

void collectMultiRapidBlockData(vector<dataCollectionConfig> &vecDcc)
{
    int32_t vecLength = vecDcc.size();
    vector<UNIT *> vecUnit;
    vector<int16_t> vecSamplesPreTrigger;
    vector<uint16_t> vecMaxPostTrigger;
    vector<uint8_t> vecTimebase;
    vector<uint32_t> vecNumWaveforms;

    for (int i = 0; i < vecLength; i++)
    {
        if (vecDcc.at(i).dataConfigured == FALSE) // unit should always be initialised in vector
        {
            printf("Unit %s has not been given daq settings",vecDcc.at(i).serial);
            continue;
        }

        uint16_t maxPostTrigger = *max_element( 
                vecDcc.at(i).chPostSamplesPerWaveform.begin(),
                vecDcc.at(i).chPostSamplesPerWaveform.end());

        vecUnit.push_back(&vecDcc.at(i).unit);
        vecSamplesPreTrigger.push_back(vecDcc.at(i).samplesPreTrigger);
        vecMaxPostTrigger.push_back(maxPostTrigger);
        vecTimebase.push_back(vecDcc.at(i).timebase.to_ulong());
        vecNumWaveforms.push_back(vecDcc.at(i).numWaveforms);
    }
    if (vecUnit.size() == 0)
    {
        printf("No units with daq settings enabled\n");
        return;
    }
    StartMultiRapidBlock(vecUnit, vecSamplesPreTrigger, vecMaxPostTrigger,
            vecTimebase, vecNumWaveforms);
}

void setDataOutput(char *outputFileName, ofstream &of)
{
    of.open(outputFileName, ios::out | ios::binary);
    return;
}

void closeDataOutput(ofstream &of)
{
    of.close();
    return;
}

int16_t bswap16(int16_t n)
{
    if (g_littleEndian) {return __builtin_bswap16(n);}
    else {return n;}
}

uint16_t bswapu16(uint16_t n)
{
    if (g_littleEndian) {return __builtin_bswap16(n);}
    else {return n;}
}

int32_t bswap32(int32_t n)
{
    if (g_littleEndian) {return __builtin_bswap32(n);}
    else {return n;}
}

uint32_t bswapu32(uint32_t n)
{
    if (g_littleEndian) {return __builtin_bswap32(n);}
    else {return n;}
}

template<std::size_t N>
void bitset_reverse(std::bitset<N> &b) 
{
    for(std::size_t i = 0; i < N/2; ++i) 
    {
        bool t = b[i];
        b[i] = b[N-i-1];
        b[N-i-1] = t;
    }
}

char *concatTwoChar(char *line1, char *line2)
{
    char *totalLine;
    int len = asprintf(&totalLine, "%s%s", line1, line2);
    if (len < 0) abort();
    return totalLine;
}

// Replaces '/' with '-' and adds '_' to the start
char *formatSerial(char *serial)
{
    int max = 31;
    char outputSerial[max];
    strcpy(outputSerial, serial);

    for (int i = 0; i < max; i++)
    {
        if (outputSerial[i] == '/')
        {
            outputSerial[i] = '-';
        }
    }

    return concatTwoChar((char *) "_", outputSerial); 
}

char *createFileName(char *serial, char *outputFileBasename)
{
    char *formattedSerial = formatSerial(serial);
    return concatTwoChar(concatTwoChar(outputFileBasename, formattedSerial), (char *) ".dat");
}

void writeDataHeader(dataCollectionConfig &dcc, ofstream &of)
{
    /*
     * Bit layout, in order
     * 4 bits: timebase (from 0-4 for ps6000)
     * 4 bits: ch1-4 active
     * 3 bits: padding
     * 5 bits: ch1-4, aux trigger active
     * 16 bits: aux trigger threshold
     * 64 (16*4) bits: trigger threshold (ch1-4)
     * 16 (4*4): ch1-4 voltage ranges (aux is always +-1V range)
     * 64 (4*16) bits: number of TOTAL samples per waveform (including pretrigger)
     * 16 bits: number of samples before trigger
     * 32 bits: number of waveforms
     * 32 bits: unix timestamp (signed integer)
     * total above bits: 232 (29 bytes)
     * Flexible length, 0 terminated: model string
     * Flexible length, 0 terminated: serial number
    */

    int16_t o16;
    uint16_t ou16;
    int32_t o32;
    uint32_t ou32;

    bitset_reverse(dcc.activeChannels);
    bitset_reverse(dcc.activeTriggers);

    uint8_t timebaseActiveCh =  (uint8_t) dcc.timebase.to_ullong() << 4 |
                                (uint8_t) dcc.activeChannels.to_ullong();
    of.write((const char *) &timebaseActiveCh, sizeof(uint8_t));

    uint8_t activeTriggers = (uint8_t) dcc.activeTriggers.to_ullong();
    of.write((const char *) &activeTriggers, sizeof(uint8_t));

    bitset_reverse(dcc.activeChannels);
    bitset_reverse(dcc.activeTriggers);

    o16 = bswap16(dcc.auxTriggerThresholdADC);
    of.write((const char *) &o16, sizeof(int16_t));

    for (int i = 0; i < 4; i++)
    {
        o16 = bswap16(dcc.chTriggerThresholdADC.at(i));
        of.write((const char *) &o16, sizeof(int16_t));
    }

    o16 = bswap16((int16_t) dcc.chVoltageRanges.to_ullong());
    of.write((const char *) &o16, sizeof(int16_t));

    for (int i = 0; i < 4; i++)
    {
        ou16 = bswapu16(dcc.chPostSamplesPerWaveform.at(i) + dcc.samplesPreTrigger);
        of.write((const char *) &ou16, sizeof(uint16_t));
    }

    o16 = bswap16(dcc.samplesPreTrigger);
    of.write((const char *) &o16, sizeof(int16_t));

    o32 = bswap32(dcc.numWaveforms);
    of.write((const char *) &o32, sizeof(int32_t));

    time_t t = time(nullptr);
    o32 = bswap32((int32_t) t);
    of.write((const char *) &o32, sizeof(int32_t));

    for (int i = 0; i < sizeof(dcc.unit.modelString); i++)
    {
        of.write((const char *) &dcc.unit.modelString[i], 1L);
        if (dcc.unit.modelString[i] == '\0') {break;}
    }

    for (int i = 0; i < sizeof(dcc.serial); i++)
    {
        of.write((const char *) &dcc.serial[i], 1L);
        if (dcc.unit.serial[i] == '\0') {break;}
    }

    return;
}

void writeDataOut(dataCollectionConfig &dcc, ofstream &of)
{
    uint16_t maxSamples = *max_element( dcc.chPostSamplesPerWaveform.begin(), 
                                        dcc.chPostSamplesPerWaveform.end());
    
    int16_t o16;
    uint32_t s = sizeof(int16_t);

    for (int i = 0; i < dcc.activeChannels.count(); i++)
    {
        uint64_t nSamples = (dcc.chPostSamplesPerWaveform.at(i) 
                           + dcc.samplesPreTrigger);
        for (int j = 0; j < dcc.numWaveforms; j++)
        {
            for (int k = 0; k < nSamples; k++)
            {
                o16 = bswap16((dcc.dataBuffers.at(i).at(j))[k]);
                of.write((const char*) &o16, s);
            }
        }
    }
}

int seriesInitDaq(char *serial)
{
    if (serial == "") {serial = NULL;}
    findUnit(&g_dcc.unit, (int8_t*) serial);
    try
    {
        strcpy(g_dcc.serial, serial);
        g_dcc.unitInitialised = TRUE;
    }
    catch (exception e)
    {
        printf("Final Catch\n");
        printf("Caught: %s\n", e.what());
        CloseDevice(&g_dcc.unit);
        g_dcc.unitInitialised = FALSE;
        g_dcc.dataConfigured = FALSE;
        throw e;
    }
    return 1;
}

int seriesSetDaqSettings(
            int16_t chATrigger, int16_t chAVRange, uint16_t chAWfSamples,
            int16_t chBTrigger, int16_t chBVRange, uint16_t chBWfSamples,
            int16_t chCTrigger, int16_t chCVRange, uint16_t chCWfSamples,
            int16_t chDTrigger, int16_t chDVRange, uint16_t chDWfSamples,
            int16_t auxTrigger, uint8_t timebase,
            uint32_t numWaveforms, int16_t samplesPreTrigger)
{
    if (g_dcc.unitInitialised == FALSE)
    {
        return 0;
    }
    if (g_dcc.dataConfigured == TRUE)
    {
        freeDataBuffers(g_dcc);
    }
    try
    {
        setActiveChannels(g_dcc, chAVRange, chBVRange, chCVRange, chDVRange);
        printf("Active channels set\n");
        setTriggerConfig(g_dcc, chATrigger, chBTrigger, chCTrigger, chDTrigger, auxTrigger);
        printf("Trigger channels set\n");
        setDataConfig(g_dcc, timebase, numWaveforms, samplesPreTrigger, 
                chAWfSamples, chBWfSamples, chCWfSamples, chDWfSamples);
        printf("All settings configured\n\n");
        g_dcc.dataConfigured = TRUE;
        printf("Data settings updated\n");
    }
    catch (exception e)
    {
        printf("Final Catch\n");
        printf("Caught: %s\n", e.what());
        CloseDevice(&g_dcc.unit);
        g_dcc.unitInitialised = FALSE;
        g_dcc.dataConfigured = FALSE;
        throw e;
    }
    return 1;
}

int seriesCollectData(char *outputFileBasename)
{
    if ((g_dcc.unitInitialised == FALSE) || (g_dcc.dataConfigured = FALSE))
    {
        return 0;
    }
    try
    {
        collectRapidBlockData(g_dcc);

        ofstream of;
        char *outputFile = concatTwoChar(outputFileBasename, (char *) ".dat");
        setDataOutput(outputFile, of);
        writeDataHeader(g_dcc, of);
        writeDataOut(g_dcc, of);
        closeDataOutput(of);
        printf("Written to file: %s\n", outputFile);
        resetDataBuffers(g_dcc);
        printf("Daq finished\n\n");
        return 1;
    }
    catch (exception e)
    {
        printf("Final Catch\n");
        printf("Caught: %s\n", e.what());
        CloseDevice(&g_dcc.unit);
        g_dcc.unitInitialised = FALSE;
        g_dcc.dataConfigured = FALSE;
        throw e;
    }
}

int seriesCloseDaq()
{
    if (g_dcc.dataConfigured)
    {
        freeDataBuffers(g_dcc);
        g_dcc.dataConfigured = FALSE;
    }
    if (g_dcc.unitInitialised)
    {
        CloseDevice(&g_dcc.unit);
        g_dcc.unitInitialised = FALSE;
    }
    return 1;
}

int multiSeriesInitDaq(char *serial)
{
    for (int i = 0; i < g_vecDcc.size(); i++)
    {
        if (0 == strcmp(serial, (char *) g_vecDcc.at(i).unit.serial))
        {
            printf("This unit is already intialised!!\n");
            return 0;
        }
    }

    UNIT unit;
    findUnit(&unit, (int8_t*) serial);

    dataCollectionConfig dcc(unit, serial);
    dcc.unitInitialised = TRUE;

    g_vecDcc.push_back(dcc);

    return 1;
}

int multiSeriesSetDaqSettings(
            int16_t chATrigger, int16_t chAVRange, uint16_t chAWfSamples,
            int16_t chBTrigger, int16_t chBVRange, uint16_t chBWfSamples,
            int16_t chCTrigger, int16_t chCVRange, uint16_t chCWfSamples,
            int16_t chDTrigger, int16_t chDVRange, uint16_t chDWfSamples,
            int16_t auxTrigger, uint8_t timebase,
            uint32_t numWaveforms, int16_t samplesPreTrigger)
{
    for (int i = 0; i < g_vecDcc.size(); i++)
    {
        try {
            if (g_vecDcc.at(i).unitInitialised == FALSE)
            {
                printf("Unit uninitialised in multiDcc\n");
                return 0;
            }
            if (g_vecDcc.at(i).dataConfigured == TRUE)
            {
                freeDataBuffers(g_vecDcc.at(i));
            }
            setActiveChannels(g_vecDcc.at(i), chAVRange, chBVRange, chCVRange, chDVRange);
            printf("%s: Active channel(s) configured\n", g_vecDcc.at(i).serial);
            setTriggerConfig(g_vecDcc.at(i), chATrigger, chBTrigger, chCTrigger, chDTrigger, auxTrigger);
            printf("%s: Trigger channel(s) configured\n", g_vecDcc.at(i).serial);
            setDataConfig(g_vecDcc.at(i), timebase, numWaveforms, samplesPreTrigger, 
                    chAWfSamples, chBWfSamples, chCWfSamples, chDWfSamples);
            g_vecDcc.at(i).dataConfigured = TRUE;
            printf("%s: Settings configured\n\n", g_vecDcc.at(i).serial);

        }
        catch (exception e)
        {
            printf("Final Catch\n");
            printf("Caught: %s\n", e.what());
            CloseDevice(&g_vecDcc.at(i).unit);
            g_vecDcc.at(i).unitInitialised = FALSE;
            g_vecDcc.at(i).dataConfigured = FALSE;
            // throw e;
        }
    }

    bool anyActive = FALSE;

    for (int i = 0; i < g_vecDcc.size(); i++)
    {
        if (g_vecDcc.at(i).unitInitialised && g_vecDcc.at(i).dataConfigured)
        {
            anyActive = TRUE;
        }
    }
    if (!anyActive)
    {
        return 0;
    }

    return 1;
}

int multiSeriesCollectData(char *outputFileBasename)
{
    // Idk how to decide outputfile names, add a random number? Add serial number?
    // Issue is that the serial number has a / so it won't work as is
    // Will need to replace with a - or something
    // Need to add functions to daq6ka and ps6kawrapper to accommodate this

    bool anyActive = FALSE;

    for (int i = 0; i < g_vecDcc.size(); i++)
    {
        if (g_vecDcc.at(i).unitInitialised && g_vecDcc.at(i).dataConfigured)
        {
            anyActive = TRUE;
        }
    }
    if (!anyActive)
    {
        return 0;
    }

    collectMultiRapidBlockData(g_vecDcc);
    for (int i = 0; i < g_vecDcc.size(); i++)
    {
        if (!(g_dcc.unitInitialised && g_dcc.dataConfigured))
        {
            
        }
        ofstream of;
        char *outputFile = createFileName(g_vecDcc.at(i).serial, outputFileBasename);
        setDataOutput(outputFile, of);
        writeDataHeader(g_vecDcc.at(i), of);
        writeDataOut(g_vecDcc.at(i), of);
        closeDataOutput(of);
        printf("Written to file: %s\n", outputFile);
        resetDataBuffers(g_vecDcc.at(i));
    }
    printf("Daq finished\n\n");
    return 1;
}

int multiSeriesCloseDaq()
{
    for (int i = 0; i < g_vecDcc.size(); i++)
    {
        if (g_vecDcc.at(i).dataConfigured)
        {
            freeDataBuffers(g_vecDcc.at(i));
            g_vecDcc.at(i).dataConfigured = FALSE;
        }
        if (g_vecDcc.at(i).unitInitialised)
        {
            CloseDevice(&g_vecDcc.at(i).unit);
            g_vecDcc.at(i).unitInitialised = FALSE;
        }
    }
    return 1;
}

// to be run from python side
int runFullDAQ(char *outputFileBasename,
            int16_t chATrigger, int16_t chAVRange, uint16_t chAWfSamples,
            int16_t chBTrigger, int16_t chBVRange, uint16_t chBWfSamples,
            int16_t chCTrigger, int16_t chCVRange, uint16_t chCWfSamples,
            int16_t chDTrigger, int16_t chDVRange, uint16_t chDWfSamples,
            int16_t auxTrigger, uint8_t timebase,
            uint32_t numWaveforms, int16_t samplesPreTrigger, char *serial)
{
    UNIT *unit;
    if (serial == "") {serial = NULL;}
    findUnit(unit, (int8_t*) serial);
    try
    {
        dataCollectionConfig dcc(*unit, serial);
        setActiveChannels(dcc, chAVRange, chBVRange, chCVRange, chDVRange);
        setTriggerConfig(dcc, chATrigger, chBTrigger, chCTrigger, chDTrigger, auxTrigger);
        setDataConfig(dcc, timebase, numWaveforms, samplesPreTrigger, 
                    chAWfSamples, chBWfSamples, chCWfSamples, chDWfSamples);

        collectRapidBlockData(dcc);

        ofstream of;
        char *outputFile = concatTwoChar(outputFileBasename, (char *) ".dat");
        setDataOutput(outputFile, of);
        writeDataHeader(dcc, of);
        writeDataOut(dcc, of);
        closeDataOutput(of);
        printf("Data written to %s\n", outputFile);
        CloseDevice(unit);
        printf("Device closed\n");
    }
    catch (exception e)
    {
        printf("Final Catch\n");
        printf("Caught: %s\n", e.what());
        CloseDevice(unit);
        throw e;
    }
    return 1;
}

char* getSerials()
{
    int16_t *serialLth;
    *serialLth = 128;
    char* out = new char[*serialLth];
    int16_t *count;

    enumUnits(count, out, serialLth);

    return out;
}

PYBIND11_MODULE(daq6000a, m)
{
    m.doc() = "Picoscope 6000a DAQ System";

    m.def("runFullDAQ", &runFullDAQ, py::return_value_policy::copy);
    m.def("seriesInitDaq", &seriesInitDaq, py::return_value_policy::copy);
    m.def("seriesSetDaqSettings", &seriesSetDaqSettings, py::return_value_policy::copy);
    m.def("seriesCollectData", &seriesCollectData, py::return_value_policy::copy);
    m.def("seriesCloseDaq", &seriesCloseDaq, py::return_value_policy::copy);
    m.def("multiSeriesInitDaq", &multiSeriesInitDaq, py::return_value_policy::copy);
    m.def("multiSeriesSetDaqSettings", &multiSeriesSetDaqSettings, py::return_value_policy::copy);
    m.def("multiSeriesCollectData", &multiSeriesCollectData, py::return_value_policy::copy);
    m.def("multiSeriesCloseDaq", &multiSeriesCloseDaq, py::return_value_policy::copy);
    m.def("getSerials", &getSerials, py::return_value_policy::copy);
}

