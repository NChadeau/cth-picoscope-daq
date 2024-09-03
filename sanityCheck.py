import sys
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

ps6000VRanges = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 
                 10000, 20000, 50000]

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

def integrate(chData, chBaseline):
    dim = chData.shape
    argMax = np.argmin(chData, axis=1, keepdims=True)

    ind = np.indices(dim)[1]

    charge = np.sum(chData - chBaseline[:, np.newaxis], axis=1, 
                    where=(ind > argMax - 10) & (ind < argMax + 40))

    return charge

def baseline(chData):
    return np.mean(chData[:,:100], axis=1)

def sanityBool(fileName, output=False, plot=False, show=False):
    mean, std = main(fileName, plot=plot, output=output, show=show)

    return not (np.any(np.abs(mean[:3]) < 2000) or (np.abs(mean[3]) < 50))


def main(fileName, plot=True, output=True, show=True):

    print("\n%s" % fileName)

    with open(fileName, 'rb') as f:
        header = readHeader(f)
        data = readData(f, header)

    dims = (len(data), len(data[0]))

    chIntData = np.zeros(dims)
    chBaseData = np.zeros(dims)
    for i, chData in enumerate(data):
        chBaseData[i] = baseline(chData)
        chIntData[i] = integrate(chData, chBaseData[i])
    
    mean = np.mean(chIntData, axis=1)
    std = np.std(chIntData, axis=1)

    if output:
        for i in range(dims[0]):
            print("Ch %s:" % chr(ord("A") + i), mean[i], chr(177), std[i])
    
    if plot:
        fig, axes = plt.subplots(2,2)
        fig.suptitle('/'.join(fileName.split("/")[-2:]))
        for i in range(dims[0]):
            axes[i // 2][i % 2].hist(chIntData[i], bins=100)
        if show: plt.show()

    return mean, std

if __name__ == "__main__":
    args = sys.argv
    if len(args) < 2:
        print("python3 sanityCheck.py <file> ...")
        exit()
    for f in args[1:]:
        main(f, show=False)
    plt.show()
