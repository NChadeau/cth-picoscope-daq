#ifndef constants_h
#define constants_h

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <ctime>
// #include <direct.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <variant>
#include <vector>

// ROOT dependencies
#include "TAttFill.h"
#include "TAttLine.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "THStack.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TLine.h"
#include "TList.h"
#include "TNamed.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TText.h"
#include "TTree.h"

struct sample
{
	double voltage; // [mV]
	double time;	// [ns]
};

struct points
{
	double value;
	std::vector<int> index;
};

struct gaussParams
{
	double mean;
	double sigma;
};

struct plot2D
{
	TH2D *histo;
	std::string name;
};

struct plot1D
{
	TH1D *histo;
	std::string name;
};

class dataHeader
{
public:
	int timebase;
	std::string activeChannels;
	std::string activeTriggers;
	float auxTriggerThreshold;
	std::vector<float> chTriggerThreshold;
	std::vector<int> chVRanges;
	std::vector<int> chSamples;
	int preTriggerSamples;
	int numWaveforms;
	int timestamp;
	std::string modelNumber;
	std::string serialNumber;
};

const std::vector<int>
	ps6000VRanges{10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
const bool showHeader(false);
const bool doMovingAverage(false);
const std::vector<std::string> MPPCsHV{/*"78V", "78.5V", "79V", "79.5V","80V", "80.5V", */ "81V" /*, "81.5V", "82V", "82.5V", "83V"*/};
const std::vector<std::string> LEDV{"Dark", "805mV", "810mV", "820mV", "840mV", "870mV", "890mV"};
const std::vector<std::string> PMTV{"1.2kV" /*, "1.25kV", "1.3kV", "1.35kV", "1.4kV", "1.45kV", "1.5kV"*/};
const std::vector<std::string> picoscopeNames{"IW098-0028", "IW114-0004"};

// ROOT file branches variables
TTree *tree;
int channel = 0;
double pulseMinimum = 0;
int pulseMinimumSampleNumber = 0;
double pulseMinimumTime = 0;
double pulseMinimumMovingAverage = 0;
double integratedCharge = 0;

// Titles and naming should be modified if adding an histo
const std::vector<std::vector<std::string>> titles{{"First 100 waveforms for ch", "waveforms"},
												   {"Pulse time for ch", "pulseTime"},
												   {"Pulse minimal value for ch", "pulseMinimum"},
												   {"Integrated charge for ch", "integratedCharge"}};

#endif // constants_h
