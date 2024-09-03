#include "constants.h"

///////////////////////////////////////////////////////////////////////////////
///                            General functions                            ///
///////////////////////////////////////////////////////////////////////////////

bool isExisting(const std::string &name)
{
	return (access(name.c_str(), F_OK) != -1);
}

std::vector<std::string> stringComponents(const std::string &name)
{
	std::string stringToDecompose(name), delimiter("_");
	std::vector<std::string> components;
	size_t pos(0);
	while ((pos = stringToDecompose.find(delimiter)) != std::string::npos)
	{
		std::string stringPart(stringToDecompose.substr(0, pos));
		components.push_back(stringPart);
		stringToDecompose.erase(0, pos + delimiter.length());
	}
	return components;
}

///////////////////////////////////////////////////////////////////////////////
///                          Extraction functions                           ///
///////////////////////////////////////////////////////////////////////////////

std::string byteBin(const char byte)
{
	return std::bitset<8>(static_cast<unsigned char>(byte)).to_string();
}

std::string byteHex(const char byte)
{
	std::stringstream ss;
	ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(byte));
	return ss.str();
}

std::string bytesBin(std::ifstream &f,
					 const int n)
{
	std::string s;
	for (int i0(0); i0 < n; ++i0)
	{
		char byte;
		f.read(&byte, 1);
		s += byteBin(byte);
	}
	return s;
}

int bytesInt(std::ifstream &f,
			 const int n)
{
	std::string s;
	for (int i0(0); i0 < n; ++i0)
	{
		char byte;
		f.read(&byte, 1);
		s += byteHex(byte);
	}
	return std::stoi(s, nullptr, 16);
}

std::string bytesHex(std::ifstream &f,
					 const int n)
{
	std::string s;
	for (int i0(0); i0 < n; ++i0)
	{
		char byte;
		f.read(&byte, 1);
		s += byteHex(byte);
	}
	return s;
}

int bytesTwos(std::ifstream &f,
			  const int n)
{
	std::string hexStr(bytesHex(f, n));
	int value(std::stoi(hexStr, nullptr, 16));
	if (value & (1 << (n * 8 - 1)))
	{
		value -= 1 << (n * 8);
	}
	return value;
}

std::string bytesString(std::ifstream &f,
						const int n = 0)
{
	char c;
	std::string s;
	f.read(&c, 1);
	while (c != '\0')
	{
		s += c;
		f.read(&c, 1);
	}
	return s;
}

float adc2mv(const int16_t value, const int range)
{
	return (value / 32512.0f) * ps6000VRanges[range];
}

dataHeader readHeader(std::ifstream &f)
{
	dataHeader d;
	int nCh(4);
	char b;
	f.read(&b, 1);
	d.timebase = b >> 4;
	d.activeChannels = byteBin(b).substr(4);
	d.activeTriggers = byteBin(f.get()).substr(3);
	d.auxTriggerThreshold = adc2mv(bytesTwos(f, 2), 6);
	for (int i0(0); i0 < nCh; ++i0)
	{
		d.chTriggerThreshold.push_back(adc2mv(bytesTwos(f, 2), 6));
	}
	std::string vRanges = bytesBin(f, 2);
	for (int i0(0); i0 < nCh; ++i0)
	{
		d.chVRanges.push_back(std::stoi(vRanges.substr(4 * i0, 4), nullptr, 2));
	}
	for (int i0(0); i0 < nCh; ++i0)
	{
		d.chSamples.push_back(bytesInt(f, 2));
	}
	d.preTriggerSamples = bytesInt(f, 2);
	d.numWaveforms = bytesInt(f, 4);
	d.timestamp = bytesTwos(f, 4);
	d.modelNumber = bytesString(f);
	d.serialNumber = bytesString(f);
	return d;
}

void printHeader(const dataHeader &header)
{
	int nCh(4);
	std::cout << "Header Information:\n";
	std::cout << "timebase:             " << header.timebase << std::endl;
	std::cout << "activeChannels:       " << header.activeChannels << std::endl;
	std::cout << "activeTriggers:       " << header.activeTriggers << std::endl;
	std::cout << "auxTriggerThreshold:  " << header.auxTriggerThreshold << std::endl;
	std::cout << "preTriggerSamples:    " << header.preTriggerSamples  << std::endl;
	std::cout << "numWaveforms:         " << header.numWaveforms  << std::endl;
	std::cout << "timestamp:            " << header.timestamp  << std::endl;
	std::cout << "modelNumber:          " << header.modelNumber  << std::endl;
	std::cout << "serialNumber:         " << header.serialNumber  << std::endl;
	for (int i0(0) ; i0 < nCh ; ++i0)
	{
	std::cout << "\nChannel " << 'A' + i0 << std::endl;
	std::cout << "chTriggerThreshold:   " << header.chTriggerThreshold.at(i0) << std::endl;
	std::cout << "chVRanges:            " << header.chVRanges.at(i0) << std::endl;
	std::cout << "chSamples:            " << header.chSamples.at(i0) << std::endl;
	}
}

bool isLittleEndian()
{
	uint32_t i(1);
	char *c = (char *)&i;
	return bool(*c);
}

std::vector<std::vector<std::vector<sample>>> readData(std::ifstream &f, dataHeader &d)
{
	std::vector<std::vector<std::vector<sample>>> data;
	bool little(isLittleEndian());
	int nWf = d.numWaveforms;
	double timeBase = pow(2, d.timebase) * 0.2;
	for (int ch(0); ch < 4; ++ch)
	{
		if (d.activeChannels[ch] == '0')
		{
			continue;
		}
		int nSamples = d.chSamples.at(ch);
		std::vector<int16_t> chADCData(nWf * nSamples);
		int16_t *chADCDataPter = chADCData.data();
		if (little)
		{
			for (int i0(0); i0 < nSamples * nWf; ++i0)
			{
				int16_t tmp;
				f.read(reinterpret_cast<char *>(&tmp), sizeof(int16_t));
				*chADCDataPter = __builtin_bswap16(tmp);
				chADCDataPter++;
			}
		}
		else
		{
			f.read(reinterpret_cast<char *>(chADCData.data()), nWf * nSamples * sizeof(int));
		}
		std::vector<std::vector<sample>> chData(nWf);
		for (int i0(0); i0 < nWf; ++i0)
		{
			std::vector<sample> wfChData(nSamples);
			for (int i1(0); i1 < nSamples; ++i1)
			{
				sample newSample{adc2mv(chADCData[i0 * nSamples + i1], d.chVRanges.at(ch)), timeBase * i1};
				wfChData.at(i1) = (newSample);
				// std::cout << newSample.time << " / " << newSample.voltage << std::endl;
			}
			chData.at(i0) = (wfChData);
		}
		data.push_back(chData);
	}
	return data;
}

int getNumSamples(dataHeader &d)
{
	int numSamples(d.chSamples.at(0));
	for (int ch(0); ch < 4; ++ch)
	{
		if (d.chSamples.at(ch) != numSamples && d.chSamples.at(ch) != 0)
		{
			std::cout << "WARNING: all channels do not have the same number of samples, default value of '1' retuned...";
			return 1;
		}
	}
	return numSamples;
}

double getTimeBase(dataHeader &d)
{
	double timebase(pow(2, d.timebase) * 0.2);
	return timebase;
}

///////////////////////////////////////////////////////////////////////////////
///                         Pre-analysis functions                          ///
///////////////////////////////////////////////////////////////////////////////

points getMinData(const std::vector<sample>& data)
{
    std::vector<sample> sortedData(data);
    std::sort(sortedData.begin(), sortedData.end(), [](sample a, sample b)
			{return a.voltage < b.voltage;});
    double minimum = sortedData[0].voltage;
    sortedData.clear();
    std::vector<int> minimumIndexes;
    for(int i0(0); i0<(int)data.size(); ++i0){
    	if(data[i0].voltage == minimum){
    		minimumIndexes.push_back(i0);
    	}
    }
	points result{minimum, minimumIndexes};
	return result;
}

points getMaxData(const std::vector<sample> &data)
{
	std::vector<sample> sortedData(data);
	std::sort(sortedData.begin(), sortedData.end(), [](sample a, sample b)
			  { return a.voltage > b.voltage; });
	double maximum = sortedData[0].voltage;
	sortedData.clear();
	std::vector<int> maximumIndexes;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		if (data[i0].voltage == maximum)
		{
			maximumIndexes.push_back(i0);
		}
	}
	points result{maximum, maximumIndexes};
	return result;
}

void plottingFirstWaveforms(const std::string fileNameBase,
							const std::vector<std::vector<std::vector<sample>>> &data,
							std::vector<plot2D> &plots2D,
							const int nBinsT,
							const int nBinsV,
							const int nSamples,
							const double timeBase)
{
	int nBins[2] = {nBinsT, nBinsV};
	double lower[2] = {0, 0};
	double upper[2] = {nSamples * timeBase, 0};
	int numWf(100);
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		if (numWf > (int)data[i0].size())
		{
			numWf = data[i0].size();
		}
	}
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		for (int i1(0); i1 < numWf; ++i1)
		{
			points minimum(getMinData(data.at(i0).at(i1)));
			points maximum(getMaxData(data.at(i0).at(i1)));
			if (minimum.value < lower[1])
			{
				lower[1] = minimum.value;
			}
			if (maximum.value > upper[1])
			{
				upper[1] = maximum.value;
			}
		}
	}
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		TString ch(std::string(1, 'A' + i0)), nWf(std::to_string(numWf));
		TH2D *h = new TH2D(fileNameBase + "_" + nWf + "Wf_ch" + ch, ";time [ns];charge [mV]", nBins[0], lower[0], upper[0], nBins[1], lower[1], upper[1]);
		for (int i1(0); i1 < numWf; ++i1)
		{
			for (int i2(0); i2 < (int)data[i0][i1].size(); ++i2)
			{
				h->Fill(data[i0][i1][i2].time, data[i0][i1][i2].voltage);
			}
		}
		plot2D newPlot{h, "waveforms"};
		plots2D.push_back(newPlot);
	}
}

void plottingMinimum(const std::string fileNameBase,
					 const std::vector<std::vector<std::vector<sample>>> &data,
					 std::vector<plot1D> &plots1D,
					 std::vector<std::vector<double>> &pulseMinimumMatrix,
					 std::vector<std::vector<int>> &pulseMinimumSampleNumberMatrix,
					 std::vector<std::vector<double>> &pulseMinimumTimeMatrix,
					 const int nBinsMinimumTime,
					 const int nBinsPulseMinimum,
					 const int nSamples,
					 const double timeBase)
{
	int nBinsTime(nBinsMinimumTime);
	double lowerTime(0), upperTime(nSamples * timeBase);
	int nBinsMini(nBinsPulseMinimum);
	double lowerMini(0), upperMini(0);
	std::vector<std::vector<points>> matrixMinimum;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		std::vector<points> vecMinimum;
		for (int i1(0); i1 < (int)data[i0].size(); ++i1)
		{
			points minimum(getMinData(data[i0][i1]));
			points maximum(getMaxData(data[i0][i1]));
			if (minimum.value < lowerMini)
			{
				lowerMini = minimum.value;
			}
			if (maximum.value > upperMini)
			{
				upperMini = maximum.value;
			}
			vecMinimum.push_back(minimum);
		}
		matrixMinimum.push_back(vecMinimum);
	}
	std::vector<TH1D *> plotsMini;
	std::vector<TH1D *> plotsTime;
	for (int i0(0); i0 < (int)matrixMinimum.size(); ++i0)
	{
		TString ch(std::string(1, 'A' + i0));
		TH1D *h = new TH1D(fileNameBase + "_pulseMinimum_ch" + ch, ";charge [mV]", nBinsMini, lowerMini, upperMini);
		TH1D *h1 = new TH1D(fileNameBase + "_pulseMinimumTime_ch" + ch, ";time [ns]", nBinsTime, lowerTime, upperTime);
		for (int i1(0); i1 < (int)matrixMinimum[i0].size(); ++i1)
		{
			h->Fill(matrixMinimum[i0][i1].value);
			for (int i2(0); i2 < (int)matrixMinimum[i0][i1].index.size(); ++i2)
			{
				h1->Fill(matrixMinimum[i0][i1].index[i2] * timeBase);
				pulseMinimumMatrix[i0].push_back(matrixMinimum[i0][i1].value);
				pulseMinimumSampleNumberMatrix[i0].push_back(matrixMinimum[i0][i1].index[i2]);
				pulseMinimumTimeMatrix[i0].push_back(matrixMinimum[i0][i1].index[i2] * timeBase);
			}
		}
		plotsMini.push_back(h);
		plotsTime.push_back(h1);
	}
	for (int i0(0); i0 < (int)plotsMini.size(); ++i0)
	{
		plot1D newPlot{plotsMini[i0], "pulseMinimum"};
		plots1D.push_back(newPlot);
	}
	for (int i0(0); i0 < (int)plotsTime.size(); ++i0)
	{
		plot1D newPlot{plotsTime[i0], "pulseTime"};
		plots1D.push_back(newPlot);
	}
	matrixMinimum.clear();
}

std::vector<sample> movingAverage(const std::vector<sample> &data,
								  const int windowLowerEdge,
								  const int windowUpperEdge,
								  const int nHalfAverage)
{
	std::vector<sample> newData;
	int lowerEdge, upperEdge;
	if (windowLowerEdge - nHalfAverage < 0 && windowUpperEdge + nHalfAverage > (int)data.size() - 1)
	{
		lowerEdge = nHalfAverage;
		upperEdge = data.size() - 1 - nHalfAverage;
	}
	else if (windowLowerEdge - nHalfAverage < 0)
	{
		lowerEdge = nHalfAverage;
		upperEdge = windowUpperEdge;
	}
	else if (windowUpperEdge + nHalfAverage > (int)data.size() - 1)
	{
		lowerEdge = windowLowerEdge;
		upperEdge = data.size() - 1 - nHalfAverage;
	}
	else
	{
		lowerEdge = windowLowerEdge;
		upperEdge = windowUpperEdge;
	}
	for (int i0(lowerEdge); i0 <= upperEdge; ++i0)
	{
		newData.push_back(data[i0]);
		for (int i1(1); i1 <= nHalfAverage; ++i1)
		{
			newData[i0].voltage += newData[i0 - i1].voltage + newData[i0 + i1].voltage;
		}
		newData[i0].voltage /= 2 * nHalfAverage + 1;
	}
	return newData;
}

// TODO: Do we really need it???
void plottingMinimumMovingAverage(const std::string fileNameBase,
								  const std::vector<std::vector<std::vector<sample>>> &data,
								  std::vector<plot1D> &plots1D,
								  std::vector<std::vector<double>> pulseMinimumMovingAverageMatrix,
								  const int nBins,
								  const int windowLowerEdge,
								  const int windowUpperEdge,
								  const int nHalfAverage,
								  const int nSamples,
								  const double timeBase)
{
}

gaussParams baseLine(const std::vector<sample> &data,
					 const int windowLowerEdge,
					 const int windowUpperEdge)
{
	double x(0), x2(0);
	for (int i0(windowLowerEdge); i0 <= windowUpperEdge; ++i0)
	{
		x += data[i0].voltage / (windowUpperEdge - windowLowerEdge + 1);
		x2 += data[i0].voltage * data[i0].voltage / (windowUpperEdge - windowLowerEdge + 1);
	}
	gaussParams parameters{x, sqrt(((windowUpperEdge - windowLowerEdge + 1) / (windowUpperEdge - windowLowerEdge)) * (x2 - x * x))};
	return parameters;
}

double chargeIntegration(const std::vector<sample> &data,
						 const int sampleNumber,
						 const int windowLowerEdge,
						 const int windowUpperEdge,
						 const double timeBase,
						 const double baseLineValue)
{
	double integratedCharge(0);
	int lowerEdge(0), upperEdge(0);
	if (sampleNumber - windowLowerEdge < 0 && sampleNumber + windowUpperEdge > (int)data.size())
	{
		lowerEdge = 0;
		upperEdge = data.size() - 1;
	}
	else if (sampleNumber - windowLowerEdge < 0)
	{
		lowerEdge = 0;
		upperEdge = sampleNumber + windowUpperEdge;
	}
	else if (sampleNumber + windowUpperEdge > (int)data.size())
	{
		lowerEdge = sampleNumber - windowLowerEdge;
		upperEdge = data.size() - 1;
	}
	else
	{
		lowerEdge = sampleNumber - windowLowerEdge;
		upperEdge = sampleNumber + windowUpperEdge;
	}
	for (int i0(lowerEdge); i0 <= upperEdge; ++i0)
	{
		integratedCharge += (data[i0].voltage - baseLineValue) * timeBase;
	}
	return integratedCharge;
}

void plottingIntegratedCharge(const std::string fileNameBase,
							  const std::vector<std::vector<std::vector<sample>>> &data,
							  std::vector<std::vector<double>> &integratedChargeMatrix,
							  std::vector<plot1D> &plots1D,
							  const int nBinsIntegrated,
							  const int windowLowerEdge,
							  const int windowUpperEdge,
							  const double timeBase)
{
	int nBins(nBinsIntegrated);
	double lower(0), upper(0);
	std::vector<std::vector<double>> matrixInteQ;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		std::vector<double> vecInteQ;
		for (int i1(0); i1 < (int)data[i0].size(); ++i1)
		{
			gaussParams baseLineValue(baseLine(data[i0][i1], 0, 50));
			points minimums(getMinData(data[i0][i1]));
			for (int i2(0); i2 < (int)minimums.index.size(); ++i2)
			{
				double inteQ(chargeIntegration(data[i0][i1], minimums.index[i2], windowLowerEdge, windowUpperEdge, timeBase, baseLineValue.mean));
				if (inteQ < -1000000)
				{
					std::cout << "ERROR: index " << minimums.index[i2] << "/ pulse value " << data[i0][i1][minimums.index[i2]].voltage << " / lower and upper edge " << windowLowerEdge << " " << windowUpperEdge << " / baseline " << baseLineValue.mean << " mV / integrated charge " << inteQ << " mV" << std::endl;
					inteQ = 0;
				}
				integratedChargeMatrix[i0].push_back(inteQ);
				vecInteQ.push_back(inteQ);
				if (inteQ < lower)
				{
					lower = inteQ;
				}
				if (inteQ > upper)
				{
					upper = inteQ;
				}
			}
		}
		matrixInteQ.push_back(vecInteQ);
	}
	for (int i0(0); i0 < (int)matrixInteQ.size(); ++i0)
	{
		TString ch(std::string(1, 'A' + i0));
		TH1D *h = new TH1D(fileNameBase + "_integratedCharge_ch" + ch, ";integrated charge [mV.ns]", nBins, lower, upper);
		for (int i1(0); i1 < (int)matrixInteQ[i0].size(); ++i1)
		{
			h->Fill(matrixInteQ[i0][i1]);
		}
		plot1D newPlot{h, "integratedCharge"};
		plots1D.push_back(newPlot);
	}
	matrixInteQ.clear();
}

///////////////////////////////////////////////////////////////////////////////
///                            Saving functions                             ///
///////////////////////////////////////////////////////////////////////////////

Double_t maxValueTH2D(TH2D *histo)
{
	Double_t max_value = 0;
	Int_t x_bins = histo->GetNbinsX();
	Int_t y_bins = histo->GetNbinsY();
	for (Int_t i = 0; i < x_bins; ++i)
	{
		for (Int_t j = 0; j < y_bins; ++j)
		{
			Double_t test_value = histo->GetBinContent(i + 1, j + 1);
			if (test_value > max_value)
			{
				max_value = test_value;
			}
		}
	}
	return max_value;
}

Double_t minValueTH2D(TH2D *histo)
{
	Double_t min_value = maxValueTH2D(histo);
	Int_t x_bins = histo->GetNbinsX();
	Int_t y_bins = histo->GetNbinsY();
	for (Int_t i = 0; i < x_bins; ++i)
	{
		for (Int_t j = 0; j < y_bins; ++j)
		{
			Double_t test_value = histo->GetBinContent(i + 1, j + 1);
			if (test_value < min_value && test_value > 0)
			{
				min_value = test_value;
			}
		}
	}
	return min_value;
}

void setStyle(TCanvas *canvas)
{
	canvas->SetTicks();
	canvas->SetTopMargin(0.05f);
	canvas->SetRightMargin(0.15f);
	canvas->SetLeftMargin(0.15f);
}

void addTitle(TCanvas *canvas,
			  const TString title)
{
	canvas->cd();
	TLatex Tl;
	Tl.SetNDC();
	Tl.SetTextColor(kBlack);
	Tl.SetTextAlign(31);
	Tl.SetTextSize(0.03f);
	Tl.DrawLatex(0.85, 0.96, title);
}

TCanvas *drawCanvas(TString canvasName,
					TH1D *histo,
					const TString &title)
{
	TCanvas *canvas = new TCanvas(canvasName, canvasName, 1000, 1000);
	canvas->cd();
	setStyle(canvas);
	histo->Draw();
	addTitle(canvas, title);
	return canvas;
}

TCanvas *drawColorCanvas(TString canvasName,
						 TH2D *histo,
						 const TString &title,
						 const Double_t maxValue,
						 const Double_t minValue)
{
	TCanvas *canvas = new TCanvas(canvasName, canvasName, 1238, 1000);
	canvas->cd();
	setStyle(canvas);
	histo->SetMaximum(maxValue);
	histo->SetMinimum(minValue);
	histo->Draw("colz");
	gPad->Update();
	addTitle(canvas, title);
	return canvas;
}

TCanvas *drawStackCanvas(TString canvasName,
						 THStack *histos,
						 const TString &title)
{
	TCanvas *canvas = new TCanvas(canvasName, canvasName, 1000, 1000);
	canvas->cd();
	setStyle(canvas);
	histos->Draw("nostack");
	addTitle(canvas, title);
	return canvas;
}

void saveInPDF(const TString directory,
			   const TString pdfName,
			   TCanvas *canvas,
			   const TString logScale)
{
	gStyle->SetOptStat(0);
	TCanvas *C = canvas;
	if ("logX" == logScale)
	{
		C->SetLogx();
	}
	else if ("logY" == logScale)
	{
		C->SetLogy();
	}
	else if ("logZ" == logScale)
	{
		C->SetLogz();
	}
	else if ("logXY" == logScale)
	{
		C->SetLogx();
		C->SetLogy();
	}
	else if ("logXZ" == logScale)
	{
		C->SetLogx();
		C->SetLogz();
	}
	else if ("logYZ" == logScale)
	{
		C->SetLogy();
		C->SetLogz();
	}
	else if ("logXYZ" == logScale)
	{
		C->SetLogx();
		C->SetLogy();
		C->SetLogz();
	}
	C->SaveAs(directory + pdfName);
	delete C;
}

void savingPlots(const std::vector<plot2D> &plots2D,
				 const std::vector<plot1D> &plots1D,
				 const std::string pdfFileNameBase,
				 const std::string outputDirectory)
{
	int numberPlots2DPerChannel(plots2D.size() / 4), numberPlots1DPerChannel(plots1D.size() / 4);
	for (int i0(0); i0 < 4; ++i0)
	{
		TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);
		TString ch(std::string(1, 'A' + i0));
		TString pdfFile(pdfFileNameBase + "_ch" + ch + ".pdf");
		Ctmp->SaveAs(outputDirectory + pdfFile + "[");
		for (int i1(0); i1 < numberPlots2DPerChannel; ++i1)
		{
			for (int i2(0); i2 < (int)titles.size(); ++i2)
			{
				if (plots2D[i1 * 4 + i0].name == titles[i2][1])
				{
					saveInPDF(outputDirectory, pdfFile,
							  drawColorCanvas(plots2D[i1 * 4 + i0].histo->GetName(), plots2D[i1 * 4 + i0].histo,
											  titles[i2][0] + ch,
											  maxValueTH2D(plots2D[i1 * 4 + i0].histo), minValueTH2D(plots2D[i1 * 4 + i0].histo)),
							  "logNot");
				}
			}
		}
		for (int i1(0); i1 < numberPlots1DPerChannel; ++i1)
		{
			for (int i2(0); i2 < (int)titles.size(); ++i2)
			{
				if (plots1D[i1 * 4 + i0].name == titles[i2][1])
				{
					std::string histoName(plots1D[i1 * 4 + i0].histo->GetName());
					saveInPDF(outputDirectory, pdfFile,
							  drawCanvas(plots1D[i1 * 4 + i0].histo->GetName(), plots1D[i1 * 4 + i0].histo,
										 titles[i2][0] + ch),
							  "logNot");
				}
			}
		}
		Ctmp->SaveAs(outputDirectory + pdfFile + "]");
		delete Ctmp;
	}
}

void fillingAndSavingTree(const std::string fileNameBase,
						  const std::string outputDirectory,
						  const std::vector<std::vector<double>> &pulseMinimumMatrix,
						  const std::vector<std::vector<int>> &pulseMinimumSampleNumberMatrix,
						  const std::vector<std::vector<double>> &pulseMinimumTimeMatrix,
						  const std::vector<std::vector<double>> &pulseMinimumMovingAverageMatrix,
						  const std::vector<std::vector<double>> &integratedChargeMatrix)
{
	TFile *file = TFile::Open((TString)(outputDirectory + fileNameBase) + ".root", "RECREATE");
	tree = new TTree("tree", "Charateristics of MPPC waveforms");
	tree->Branch("channel", &channel, "channel/I");
	tree->Branch("pulseMinimum", &pulseMinimum, "pulseMinimum/D");
	tree->Branch("pulseMinimumSampleNumber", &pulseMinimumSampleNumber, "pulseMinimumSampleNumber/I");
	tree->Branch("pulseMinimumTime", &pulseMinimumTime, "pulseMinimumTime/D");
	if (doMovingAverage == true)
	{
		tree->Branch("pulseMinimumMovingAverage", &pulseMinimumMovingAverage, "pulseMinimumMovingAverage/D");
	}
	tree->Branch("integratedCharge", &integratedCharge, "integratedCharge/D");
	for (int i0(0); i0 < 4; ++i0)
	{
		int n0(pulseMinimumMatrix[i0].size());
		int n1(pulseMinimumSampleNumberMatrix[i0].size());
		int n2(pulseMinimumTimeMatrix[i0].size());
		int n3(pulseMinimumMovingAverageMatrix[i0].size());
		int n4(integratedChargeMatrix[i0].size());
		if (doMovingAverage == true)
		{
			if (n0 != n1 || n0 != n2 || n0 != n3 || n0 != n4)
			{
				std::cout << "ERROR: vectors should all have the same size..." << std::endl;
				continue;
			}
		}
		else
		{
			if (n0 != n1 || n0 != n2 || n0 != n4)
			{
				std::cout << "ERROR: vectors should all have the same size (except for moving average)..." << std::endl;
				continue;
			}
		}
		for (int i1(0); i1 < n0; ++i1)
		{
			channel = i0;
			pulseMinimum = pulseMinimumMatrix[i0][i1];
			pulseMinimumSampleNumber = pulseMinimumSampleNumberMatrix[i0][i1];
			pulseMinimumTime = pulseMinimumTimeMatrix[i0][i1];
			if (doMovingAverage == true)
			{
				pulseMinimumMovingAverage = pulseMinimumMovingAverageMatrix[i0][i1];
			}
			else
			{
				pulseMinimumMovingAverage = 0;
			}
			integratedCharge = integratedChargeMatrix[i0][i1];
			tree->Fill();
		}
	}
	tree->Write();
	file->Close();
}

int preAnalyseFile(std::string directory, std::string filePath)
{
	std::string fileBaseExt = filePath.substr(filePath.find_last_of("/") + 1);
	std::string::size_type const p(fileBaseExt.find_last_of('.'));
	std::string fileBasename = fileBaseExt.substr(0, p);

	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "ERROR: can not open file." << std::endl;
		return 1;
	}
	clock_t startTime, endTime;
	std::cout << "\n######### Starting pre-analysis: " << filePath << std::endl;
	startTime = clock();
	std::cout << "###### Starting extraction..." << std::endl;
	dataHeader header = readHeader(file);
	if (showHeader == true)
	{
		printHeader(header);
	}
	std::vector<std::vector<std::vector<sample>>> data = readData(file, header);
	file.close();
	std::cout << "###### Finished extraction..." << std::endl;
	std::cout << "###### Starting plotting..." << std::endl;

	std::vector<plot2D> plots2D;
	std::vector<plot1D> plots1D;
	std::vector<double> emptyVecDouble(0);
	std::vector<int> emptyVecInt(0);
	std::vector<std::vector<double>> pulseMinimumMatrix(4, emptyVecDouble);
	std::vector<std::vector<int>> pulseMinimumSampleNumberMatrix(4, emptyVecInt);
	std::vector<std::vector<double>> pulseMinimumTimeMatrix(4, emptyVecDouble);
	std::vector<std::vector<double>> pulseMinimumMovingAverageMatrix(4, emptyVecDouble);
	std::vector<std::vector<double>> integratedChargeMatrix(4, emptyVecDouble);

	int nSamples(getNumSamples(header));
	double timeBase(getTimeBase(header));
	int nBinsT(nSamples), nBinsV(50), nBinsMinimumTime(nSamples), nBinsPulseMinimum(200), nBinsIntegrated(200), nHalfAverage(10), windowLowerEdge(10), windowUpperEdge(50);

	plottingFirstWaveforms(fileBasename, data, plots2D, nBinsT, nBinsV, nSamples, timeBase);
	std::cout << "### Plotted first waveforms..." << std::endl;
	plottingMinimum(fileBasename, data, plots1D, pulseMinimumMatrix, pulseMinimumSampleNumberMatrix, pulseMinimumTimeMatrix, nBinsMinimumTime, nBinsPulseMinimum, nSamples, timeBase);
	std::cout << "### Plotted pulse minimum and time..." << std::endl;
	if (doMovingAverage == true)
	{
		plottingMinimumMovingAverage(fileBasename, data, plots1D, pulseMinimumMovingAverageMatrix, nBinsPulseMinimum, windowLowerEdge, windowUpperEdge, nHalfAverage, nSamples, timeBase);
		std::cout << "### Plotted moving averaged minimum peaks value..." << std::endl;
	}
	plottingIntegratedCharge(fileBasename, data, integratedChargeMatrix, plots1D, nBinsIntegrated, windowLowerEdge, windowUpperEdge, timeBase);
	std::cout << "### Plotted integrated charge..." << std::endl;
	std::cout << "###### Finished plotting..." << std::endl;
	std::cout << "###### Starting saving plots..." << std::endl;
	std::string outputDirectory = directory + "plots/";
	savingPlots(plots2D, plots1D, fileBasename, outputDirectory);
	std::cout << "### Plots saved..." << std::endl;
	outputDirectory = directory + "root-files/";
	fillingAndSavingTree(fileBasename, outputDirectory, pulseMinimumMatrix, pulseMinimumSampleNumberMatrix, pulseMinimumTimeMatrix, pulseMinimumMovingAverageMatrix, integratedChargeMatrix);
	std::cout << "### ROOT file saved..." << std::endl;
	std::cout << "###### Finished saving..." << std::endl;
	endTime = clock();
	std::cout << "###### Finished pre-analysis: " << (float)(endTime - startTime) / CLOCKS_PER_SEC << "s\n" << std::endl;
	return 0;
}

void batchPreAnalysis(std::string directory, std::vector<std::string> files)
{
	for (int i0(0); i0 < (int)files.size(); ++i0)
	{
		std::string filePath = files.at(i0);
		if (isExisting(filePath) != true)
		{
			std::cerr << "WARNING: the file '" + filePath + "' does not exist..." << std::endl;
			continue;
		}
		preAnalyseFile(directory, filePath);
	}
}

///////////////////////////////////////////////////////////////////////////////
///                              Main function                              ///
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "ERROR: you need at least 1 argument: 'pre-analyse' or 'analyse'..." << std::endl;
		return 1;
	}
	std::string analysisType(argv[1]);
	if (analysisType == "pre-analyse")
	{
		std::string outputDir(argv[2]);
		std::vector<std::string> datFiles(argv + 3, argv + argc);
		batchPreAnalysis(outputDir, datFiles);
	}
	else if (analysisType == "analyse")
	{
		if (argc != 7)
		{
			std::cerr << "ERROR: you should have 7 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string analysisSubType(argv[2]);
		if (analysisSubType == "maxPMT-maxMPPC")
		{
			std::string date(argv[3]);
			std::string MPPCsNumbers(argv[4]);
			std::string directory0(argv[5]);
			std::string directory1(argv[6]);
			for (int i0(0); i0 < (int)MPPCsHV.size(); ++i0)
			{
				for (int i1(0); i1 < (int)picoscopeNames.size(); ++i1)
				{
					clock_t startTime, endTime;
					std::cout << "\n######### Starting analysis: PMT maximum VS PMT voltage & MPPCs maximum VS LED voltage" << std::endl;
					startTime = clock();
					std::cout << "###### Starting setting up histograms..." << std::endl;
					std::vector<std::vector<std::vector<TH1D *>>> integratedChargeTensor;
					for (int i2(0); i2 < (int)LEDV.size(); ++i2)
					{
						std::vector<std::vector<TH1D *>> integratedChargeMatrix;
						for (int i3(0); i3 < (int)PMTV.size(); ++i3)
						{
							std::vector<TH1D *> integratedChargeVec;
							for (int i4(0); i4 < 4; ++i4)
							{
								std::string fileNameBase(date + "_" + MPPCsHV[i0] + "_" + LEDV[i2] + "_" + PMTV[i3] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
								TH1D *integratedChargeHisto = new TH1D();
								TString ch(std::string(1, 'A' + i4));
								integratedChargeHisto->SetName(fileNameBase + "_ch" + ch);
								integratedChargeHisto->SetTitle(";Q [mV.ns];count");
								integratedChargeHisto->SetBins(100, 0, 100);
								integratedChargeVec.push_back(integratedChargeHisto);
							}
							integratedChargeMatrix.push_back(integratedChargeVec);
						}
						integratedChargeTensor.push_back(integratedChargeMatrix);
					}
					std::vector<TH1D *> maxMPPCvsLEDVVec;
					std::vector<TH1D *> maxMPPCvsLEDVVecNorm;
					std::vector<TH1D *> maxPMTvsPMTVVec;
					for (int i2(0); i2 < 4; ++i2)
					{
						std::string ch(std::string(1, 'A' + i2));
						TString name("maxMPPCvsLEDV_" + date + "_" + MPPCsHV[i0] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1] + "_ch" + ch);
						TH1D *maxMPPCvsLEDV = new TH1D();
						maxMPPCvsLEDV->SetName(name);
						maxMPPCvsLEDV->SetTitle(";U_{LED} [mV];max_{MPPC} [mV]");
						maxMPPCvsLEDV->SetBins(100, 800, 900);
						maxMPPCvsLEDVVec.push_back(maxMPPCvsLEDV);
						name = "maxMPPCvsLEDVNorm_" + date + "_" + MPPCsHV[i0] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1] + "_ch" + ch;
						TH1D *maxMPPCvsLEDVNorm = new TH1D();
						maxMPPCvsLEDVNorm->SetName(name);
						maxMPPCvsLEDVNorm->SetTitle(";U_{LED} [mV];count");
						maxMPPCvsLEDVNorm->SetBins(100, 800, 900);
						maxMPPCvsLEDVVecNorm.push_back(maxMPPCvsLEDVNorm);
						if (i2 == 3)
						{
							for (int i3(0); i3 < (int)LEDV.size(); ++i3)
							{
								TString name("maxPMTvsPMTV_" + date + "_" + MPPCsHV[i0] + "_" + LEDV[i3] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
								TH1D *maxPMTvsPMTV = new TH1D();
								maxPMTvsPMTV->SetName(name);
								maxPMTvsPMTV->SetTitle(";U_{PMT} [kV];max_{PMT} [mV]");
								maxPMTvsPMTV->SetBins(300, 1.2, 1.5);
								maxPMTvsPMTVVec.push_back(maxPMTvsPMTV);
							}
						}
					}
					std::cout << "###### Finished setting up histograms..." << std::endl;
					std::cout << "###### Starting formating pre-analysed data..." << std::endl;
					bool canBeAnalysed(false);
					for (int i2(0); i2 < (int)LEDV.size(); ++i2)
					{
						for (int i3(0); i3 < (int)PMTV.size(); ++i3)
						{
							std::string fileNameBase(date + "_" + MPPCsHV[i0] + "_" + LEDV[i2] + "_" + PMTV[i3] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
							std::string fileToProcess(directory0 + "root-files/" + fileNameBase + ".root");
							if (isExisting(fileToProcess) != true)
							{
								std::cout << "WARNING: the file '" + fileToProcess + "' does not exist..." << std::endl;
								continue;
							}
							else
							{
								canBeAnalysed = true;
								std::cout << "### Formating file: " + fileToProcess << std::endl;
								TFile *file = TFile::Open((TString)fileToProcess);
								if (!file)
								{
									std::cout << "ERROR: could not open '" << fileToProcess << "'..." << std::endl;
									return 1;
								}
								tree = dynamic_cast<TTree *>(file->Get("tree"));
								if (!tree)
								{
									std::cout << "ERROR: could not find 'tree'..." << std::endl;
									return 1;
								}
								tree->SetBranchAddress("channel", &channel);
								tree->SetBranchAddress("integratedCharge", &integratedCharge);
								Long64_t nRows(tree->GetEntries());
								double maxIntegratedCharge(0), minIntegratedCharge(0);
								for (int i4(0); i4 < nRows; ++i4)
								{
									tree->GetEntry(i4);
									if (integratedCharge > maxIntegratedCharge)
									{
										maxIntegratedCharge = integratedCharge;
									}
									if (integratedCharge < minIntegratedCharge)
									{
										minIntegratedCharge = integratedCharge;
									}
								}
								for (int i4(0); i4 < 4; ++i4)
								{
									integratedChargeTensor[i2][i3][i4]->SetBins(300, minIntegratedCharge, maxIntegratedCharge);
								}
								for (int i4(0); i4 < nRows; ++i4)
								{
									tree->GetEntry(i4);
									integratedChargeTensor[i2][i3][channel]->Fill(integratedCharge);
								}
							}
						}
					}
					std::cout << "###### Finished formating pre-analysed data..." << std::endl;
					if (canBeAnalysed == true)
					{
						std::cout << "###### Starting plotting analysis histograms..." << std::endl;
						for (int i2(0); i2 < (int)LEDV.size(); ++i2)
						{
							for (int i3(0); i3 < (int)PMTV.size(); ++i3)
							{
								for (int i4(0); i4 < 4; ++i4)
								{
									// double maximumY(0);
									double maximumX(0);
									/*for(int i5(1); i5<=integratedChargeTensor[i2][i3][i4]->GetNbinsX(); ++i5){
										if(maximumY < integratedChargeTensor[i2][i3][i4]->GetBinContent(i5)){
											maximumY = integratedChargeTensor[i2][i3][i4]->GetBinContent(i5);
											maximumX = integratedChargeTensor[i2][i3][i4]->GetXaxis()->GetBinCenter(i5);
										}
									}*/
									maximumX = integratedChargeTensor[i2][i3][i4]->GetMean();
									if (LEDV[i2] != "Dark")
									{
										std::string ledVoltage(LEDV[i2]);
										ledVoltage.erase(ledVoltage.end() - 2, ledVoltage.end());
										maxMPPCvsLEDVVec[i4]->Fill(std::stod(ledVoltage), maximumX);
										maxMPPCvsLEDVVecNorm[i4]->Fill(std::stod(ledVoltage));
									}
									if (LEDV[i2] != "Dark" && i4 == 3)
									{
										std::string pmtVoltage(PMTV[i3]);
										pmtVoltage.erase(pmtVoltage.end() - 2, pmtVoltage.end());
										maxPMTvsPMTVVec[i2]->Fill(std::stod(pmtVoltage), maximumX);
									} /*else{
										 std::cout<<"WARNING: no maximum found in the integrated charge distribution for file '"<<date+"_"+MPPCsHV[i0]+"_"+LEDV[i2]+"_"+PMTV[i3]+"_"+MPPCsNumbers+"_"+picoscopeNames[i1]<<".root'..."<<std::endl;
									 }*/
								}
							}
						}
						std::cout << "###### Finished plotting analysis histograms..." << std::endl;
						std::cout << "###### Starting saving..." << std::endl;
						for (int i2(0); i2 < (int)maxMPPCvsLEDVVec.size(); ++i2)
						{
							maxMPPCvsLEDVVec[i2]->Divide(maxMPPCvsLEDVVec[i2], maxMPPCvsLEDVVecNorm[i2], 1, 1);
							for (int i3(1); i3 <= maxMPPCvsLEDVVec[i2]->GetNbinsX(); ++i3)
							{
								maxMPPCvsLEDVVec[i2]->SetBinError(i3, 0.01 * maxMPPCvsLEDVVec[i2]->GetBinContent(i3));
							}
							TString outputDirectory(directory1);
							TString name(maxMPPCvsLEDVVec[i2]->GetName());
							TString pdfFile(name + ".pdf");
							TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);
							Ctmp->SaveAs(outputDirectory + pdfFile + "[");
							saveInPDF(outputDirectory, pdfFile,
									  drawCanvas(maxMPPCvsLEDVVec[i2]->GetName(), maxMPPCvsLEDVVec[i2],
												 maxMPPCvsLEDVVec[i2]->GetName()),
									  "logNot");
							Ctmp->SaveAs(outputDirectory + pdfFile + "]");
							delete Ctmp;
						}
						THStack *maxPMTvsPMTV = new THStack();
						TString name("maxPMTvsPMTV_" + date + "_" + MPPCsHV[i0] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
						maxPMTvsPMTV->SetName(name);
						maxPMTvsPMTV->SetTitle(";U_{PMT} [kV];max_{PMT}");
						for (int i2(0); i2 < (int)maxPMTvsPMTVVec.size(); ++i2)
						{
							for (int i3(1); i3 <= maxPMTvsPMTVVec[i2]->GetNbinsX(); ++i3)
							{
								maxPMTvsPMTVVec[i2]->SetBinError(i3, 0.05 * maxPMTvsPMTVVec[i2]->GetBinContent(i3));
								if (maxPMTvsPMTVVec[i2]->GetBinContent(i3) != 0)
								{
									std::cout << maxPMTvsPMTVVec[i2]->GetBinContent(i3) << " / ";
								}
							}
							std::cout << std::endl;
							maxPMTvsPMTV->Add(maxPMTvsPMTVVec[i2]);
						}
						TString outputDirectory(directory1);
						TString pdfFile(name + ".pdf");
						TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);
						Ctmp->SaveAs(outputDirectory + pdfFile + "[");
						saveInPDF(outputDirectory, pdfFile,
								  drawStackCanvas(maxPMTvsPMTV->GetName(), maxPMTvsPMTV,
												  maxPMTvsPMTV->GetName()),
								  "logNot");
						Ctmp->SaveAs(outputDirectory + pdfFile + "]");
						delete Ctmp;
						std::cout << "###### Finished saving..." << std::endl;
					}
					else
					{
						std::cout << "###### No analysis needed..." << std::endl;
					}
					endTime = clock();
					std::cout << "###### Finished analysis: " << (float)(endTime - startTime) / CLOCKS_PER_SEC << "s\n"
							  << std::endl;
				}
			}
		}
		else if (analysisSubType == "reproducibility")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
		}
		else if (analysisSubType == "position_dependency")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
		}
		else if (analysisSubType == "temperature_dependency")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
		}
		else if (analysisSubType == "QC")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
			std::string datesList(argv[3]);
			std::string MPPCsNumbersList(argv[4]);
			std::string directory0(argv[5]);
			std::string directory1(argv[6]);
			std::vector<std::string> dates;
			std::vector<std::string> mppcs;
			std::vector<TH2D *> maxMPPCvsmaxPMT;
			for (int i0(0); i0 < (int)dates.size(); ++i0)
			{
				for (int i1(0); i1 < (int)MPPCsHV.size(); ++i1)
				{
					for (int i2(0); i2 < (int)LEDV.size(); ++i2)
					{
						for (int i3(0); i3 < (int)PMTV.size(); ++i3)
						{
							for (int i4(0); i4 < (int)mppcs.size(); ++i4)
							{
								for (int i5(0); i5 < (int)picoscopeNames.size(); ++i5)
								{
								}
							}
						}
					}
				}
			}
		}
		else
		{
			std::cout << "Analysis type: " << analysisSubType << " is not a valid type" << std::endl;
			std::cout << "Valid analysis types: analyse <maxPMT-maxMPPC | reproducibility"
					  << " | position_dependency | temperature_dependency | QC>" << std::endl;
		}
	}
	return 0;
}
