import numpy as np
from scipy.stats import norm
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import sys

###############################################################################
###                               Parameters                                ###
###############################################################################

### Data extraction
ps6000VRanges = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 
                 10000, 20000, 50000]
                 
### Analysis  
windowlow, windowhigh = 170, 250        
showing = 1
saving = 0
nplots = 200
inputDirectory = "../data-files/"
## tested files
## Monash setup / alternate LED on
#inputFileList = ["BothLED_81V_3.25V.dat", "RightLED_81V_3.25V.dat", "LeftLED_81V_3.25V.dat"]
## new setup / no LED on
#inputFileList = ["19Jun24_80V_Dark_TapedBox.dat"]
#inputFileList = ["19Jun24_80.5V_Dark_TapedBox.dat"]
#inputFileList = ["19Jun24_81V_Dark_TapedBox.dat"]
#inputFileList = ["19Jun24_81.5V_Dark_TapedBox.dat"]
#inputFileList = ["19Jun24_82V_Dark_TapedBox.dat"]
## new setup / LED at 1.41V
#inputFileList = ["19Jun24_80V_1.41V_TapedBox.dat", "19Jun24_81V_1.41V_TapedBox.dat", "19Jun24_81.5V_1.41V_TapedBox.dat", "19Jun24_82V_1.41V_TapedBox.dat"]
#inputFileList = ["19Jun24_81V_1.41V_TapedBox.dat"]
## new setup / no LED on / run 2
#inputFileList = ["19Jun24_81V_Dark_TapedBox_Round2.dat"]
## new setup / no LED on / rotated MPPC
#inputFileList = ["19Jun24_81V_Dark_TapedBox_RotatedMPPC.dat"]
## new setup / no LED on / other MPPC
#inputFileList = ["20Jun24_80V_Dark_TapedBox_MPPC3.dat"]
#inputFileList = ["20Jun24_80.5V_Dark_TapedBox_MPPC3.dat"]
#inputFileList = ["20Jun24_81V_Dark_TapedBox_MPPC3.dat"]
## new setup / no LED on / other MPPC / black sheet / with light
#inputFileList = ["20Jun24_80V_Dark_Sheet_MPPC3.dat"]
#inputFileList = ["20Jun24_81V_Dark_Sheet_MPPC3.dat"]
## new setup / no LED on / other MPPC / black sheet / without light
#inputFileList = ["20Jun24_80V_Dark_Sheet_MPPC3_LightsOff.dat"]
#inputFileList = ["20Jun24_81V_Dark_Sheet_MPPC3_LightsOff.dat"]
## new setup / LED at 1.41V / other MPPC / black sheet
#inputFileList = ["20Jun24_81V_1.41V_Sheet_MPPC3_LightsOn.dat"]
## new setup / LED at 1.41V / other MPPC / black sheet / diffuser added
#inputFileList = ["20Jun24_81V_1.41V_Diffuser_Sheet_MPPC3_LightsOn.dat"]
## new setup / LED at 1.44V / other MPPC / black sheet / diffuser added
inputFileList = ["20Jun24_81V_1.44V_Diffuser_Sheet_MPPC3_LightsOn.dat"]
## new setup / LED at 1.41V / other MPPC / black sheet / reverse MPPC
#inputFileList = ["20Jun24_80V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_80.5V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_81V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_80V_1.42V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_80.5V_1.42V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_81V_1.42V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_80V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_80.5V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat", "20Jun24_81V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_Rotated.dat"]
outputDirectory = "plots/"

###############################################################################
###                       Data extraction functions                         ###
###############################################################################

def byteBin(byte):
    return '{0:08b}'.format(ord(byte))

def byteHex(byte):
    return '{0:02x}'.format(ord(byte))

def bytesBin(f,n):
    s = ""
    for i in range(n):
        s += byteBin(f.read(1))
    return s

def bytesInt(f,n):
    s = ""
    for i in range(n):
        s += byteHex(f.read(1))
    return int(s,16)

def bytesHex(f,n):
    s = ""
    for i in range(n):
        s += byteHex(f.read(1))
    return s

def bytesTwos(f,n):
    hexStr = bytesHex(f,n)

    value = int(hexStr, 16)
    if value & (1 << (n * 8 - 1)):
        value -= 1 << n * 8
    return value

def bytesString(f, n = 0): # xxx: if n = 0, read until 0 byte, else read n bytes
    c = f.read(1)
    s = ''
    while c != b'\0':
        s += c.decode()
        c = f.read(1)
    return s

def adc2mv(value, range):
    return (value / 32512) * ps6000VRanges[range]

def readHeader(f):
    d = {}
    nCh = 4
    b = f.read(1)
    d['timebase'] = ord(b) >> 4
    d['activeChannels'] = byteBin(b)[4:]
    d['activeTriggers'] = byteBin(f.read(1))[3:]
    d['auxTriggerThreshold'] = adc2mv(bytesTwos(f,2),6)
    for i in range(nCh):
        d['ch' + chr(ord('A') + i) + 'TriggerThreshold'] = adc2mv(bytesTwos(f,2),6)
    vRanges = bytesBin(f,2)
    for i in range(nCh):
        d['ch' + chr(ord('A') + i) + 'VRange'] = int(vRanges[4*i:4*(i + 1)],2)
    for i in range(nCh):
        d['ch' + chr(ord('A') + i) + 'Samples'] = bytesInt(f,2)
    d['preTriggerSamples'] = bytesInt(f,2)
    d['numWaveforms'] = bytesInt(f,4)
    d['timestamp'] = bytesTwos(f,4)
    
    d['modelNumber'] = bytesString(f)
    d['serialNumber'] = bytesString(f)

    return d

def readData(f, d):

    data = []

    for ch in range(4):
        if d['activeChannels'][ch] == '0':
            continue
        nWf = d['numWaveforms']
        nSamples = d['ch' + chr(ord('A') + ch) + 'Samples']
        chADCData = np.fromfile(f, dtype='>i2', count=nWf * nSamples).reshape((nWf,nSamples))
        chData = adc2mv(chADCData, d['ch' + chr(ord('A') + ch) + 'VRange'])

        data.append(chData)

    return data
    
###############################################################################
###                 Pre-analysis and plot saving functions                  ###
###############################################################################  
 
def histogramming (d, numBins):
    
    binWidth = len(d) * 1.0 / numBins
    data = []
    xAxis = 0
    upperEdge = binWidth
    count = 0
    value = 0
    for ch in d:
        if count < upperEdge:
            value += ch
        else:
            data.append(value)
            value = ch
            upperEdge += binWidth
        count += 1
        #value /= binWidth
    data.append(value)
    
    return data
    
def plotting_plot (d, plotTitle, xLabel, yLabel, showing, ls='-'):
    fig = plt.figure()
    plt.plot(d, ls)
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    if showing ==  0:
    	plt.show()
    plt.close()
    return fig

def plotting_plots (D, plotTitle, xLabel, yLabel, showing, nplots, ls='.'):
    fig = plt.figure()
    plotcounter = 0
    for d in D:
        if plotcounter > nplots:
            break
        plt.plot(d, ls)
        plotcounter += 1
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    if showing ==  0:
        plt.show()
    plt.close()
    return fig
    
def plotting_hist (d, numBins, plotTitle, xLabel, yLabel, showing):
    fig = plt.figure()
    plt.hist(d, numBins)
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    plt.close()
    if showing ==  0:
    	plt.show()
    return fig
    
def moving_average0(d, n):
    newD = []
    for i in range(len(d)):
        D = d[i]
        if (i >= n and i <= len(d) - (n + 1)):
            for j in range(n):
                D += d[i+j] + d[i-j]
            newD.append(D / (2*n+1))
        elif i < n:
            for j in range(n):
                D += d[i+j]
            j = i - 1
            count = 0
            while j > 0:
                D += d[j]
                count += 1
                j -= 1
            newD.append(D / (n + 1 + count))
        else:
            for j in range(n):
                D += d[i-j]
            j = i + 1
            count = 0
            while j < len(d):
                D += d[j]
                count += 1
                j += 1
            newD.append(D / (n + 1 + count))           
    return newD

def moving_average1(d, n):
    return [np.mean(d[i-n:i+n]) for i in range(n, len(d)-n-1)]
  
def moving_average(d, n, i1, i2):
    return [np.mean(d[i-n:i+n]) for i in range(i1+n, i2-n-1)]  
  
def saving_plots(plots, outputFile):
    pdfFile = PdfPages(outputFile)
    for plot in plots:
        pdfFile.savefig(plot)
    pdfFile.close() 

###############################################################################
###                                 Main                                    ###
###############################################################################

name = sys.argv[1]
if name == "pre-analyse":
    print("\nStarting extraction and analysis...\n")
    for inputFile in inputFileList: 
        outputFile = outputDirectory + inputFile.removesuffix('.dat')
        print("### Extracting data from '" + inputFile + "'...")
        f = open(inputDirectory + inputFile, 'rb')
        header = readHeader(f)
        data = readData(f, header)
        ch = 0
        for chData in data:
            print("###### Analyzing channel " + chr(ord('A') + ch) + "...")
            chOutputFile = outputFile + "_ch" + chr(ord('A') + ch) + ".pdf"
            countMaWfPlots = 0
            countNewWfPlots = 0
            plots = []
            maChData = []
            newChData = []
            minimumMaChData = []
            minimumNewChData = []
            indexMinMaChData = []
            indexMinNewChData = []
            eventcounter = 0
            for wfData in chData:
                maWfData = moving_average(wfData, 10, windowlow, windowhigh)
                #maWfData = histogramming(maWfData, 500)
                maChData.append(maWfData)
                #newWfData = histogramming(wfData, 500)
                #newChData.append(newWfData)
                eventcounter += 1
                if eventcounter%5000==0:
            	    print("event {}".format(eventcounter))
            newChData = chData
            print("######### Moving average and histogramming done...")
            minimumAllNewWf = np.min(newChData) 
            plots.append(plotting_plots(newChData, 'First {} waveforms for ch {}'.format(nplots, ch), 'binned time', 'charge [mV]', showing, nplots))
            for newWfData in newChData:
                minimumNewWf = np.min(newWfData)
                minimumNewChData.append(minimumNewWf)
                minimumIndexNew = np.where(newWfData == minimumNewWf)[0]
                for indexNew in minimumIndexNew:
                    indexMinNewChData.append(indexNew)
                if (np.min(newWfData) == minimumAllNewWf and countNewWfPlots < 3):
                    plots.append(plotting_plot(newWfData, 'Largest charge waveform for ch' + chr(ord('A') + ch), 'binned time', 'charge [mV]', showing))
                    countNewWfPlots += 1
            plots.append(plotting_hist(indexMinNewChData, 500, 'Charge peak index for ch' + chr(ord('A') + ch), 'minimum charge bin index', 'frequency', showing))
            nBins = round((np.max(minimumNewChData) - np.min(minimumNewChData)) / 0.4)
            if nBins < 1:
                nBins = 1
            plots.append(plotting_hist(minimumNewChData, nBins, 
                         'Charge peak frequency for ch' + chr(ord('A') + ch), 
                         'minimum charge [mV]', 'frequency', showing))
            minimumAllMaWf = np.min(maChData) 
            for maWfData in maChData:
                minimumMaWf = np.min(maWfData)
                minimumMaChData.append(minimumMaWf)
                minimumIndexMa = np.where(maWfData == minimumMaWf)[0]
                for indexMa in minimumIndexMa:
                    indexMinMaChData.append(indexMa)
                if (np.min(maWfData) == minimumAllMaWf and countMaWfPlots < 3):
                    plots.append(plotting_plot(maWfData, 'Largest charge waveform for ch' + chr(ord('A') + ch), 'binned time', 'charge [mV]', showing))
                    countMaWfPlots += 1
            plots.append(plotting_hist(indexMinMaChData, 500, 'Charge peak index for ch' + chr(ord('A') + ch), 'minimum charge bin index', 'frequency', showing))
            nBins = round((np.max(minimumMaChData) - np.min(minimumMaChData)) / 0.1)
            if nBins < 1:
                nBins = 1
            plots.append(plotting_hist(minimumMaChData, nBins, 
                         'Charge peak frequency for ch' + chr(ord('A') + ch), 
                         'minimum charge [mV]', 'frequency', showing))
            if saving == 0:
                print("###### Saving...")
                chOutputFileNp = outputFile + "_ch" + chr(ord('A') + ch)
                np.save("{}_minimumMaChData.npy".format(chOutputFileNp), np.array(minimumMaChData))
                saving_plots(plots, chOutputFile)
            ch += 1
        print("")
    print("Extraction and analysis finished !\n")
elif name == "analyse":
    print("To be done...")
else:
    print("ERROR: You should either 'pre-analyse' or 'analyse' files...")
