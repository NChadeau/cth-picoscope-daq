import os
import sys
from datetime import datetime
import time
import IV_Curve as vc
import daq6000 as gen
import daq6000a as daq

def initPicoScopes(picoList, fnGen):
    for ps in picoList:
        status = daq.multiSeriesInitDaq(ps)
        if status == 0:
            print("Python - Device not opened")
            exit()

    status = gen.initFunctionGenerator(fnGen)
    if status == 0:
        print("Python - Device not opened")
        exit()

def daqPerBias(bias, mppcStr, mv50List, mv200List, date, pmt='1.2', extra='', d='./data/'):

    s = r'%s'

    if extra != '':
        extra = '_' + extra

    outFilePattern = d + r"%s_%sV_%s_%skV_%s%s" % \
                        (date, str(bias), s, pmt, mppcStr, extra)
    
    runDark(outFilePattern)

    runMvList(2, outFilePattern, mv50List)

    runMvList(4, outFilePattern, mv200List)

    return

def runDark(oFilePattern):
    daq.multiSeriesSetDaqSettings(
                    0, 1, 2000,
                    0, 1, 2000,
                    0, 1, 2000,
                    0, 1, 2000,
                    100, 2, 5000, 0)
    

    gen.runFunctionGenerator(0,38)
    out = oFilePattern % "Dark"
    print("\n\n\nNext DAQ: %s" % out)
    daq.multiSeriesCollectData(out)
    return

def runMvList(vRange, oFilePatternRaw, mvList):

    daq.multiSeriesSetDaqSettings(
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    100, 2, 20000, 0)

    oFilePattern = oFilePatternRaw % ("%imV")
    for mv in mvList:
        out = oFilePattern % mv
        gen.runFunctionGenerator(mv,38)
        print("\n\n\nNext DAQ: %s" % out)
        daq.multiSeriesCollectData(out)

    return

def main(mppcList, reset, extra=''):

    start = time.time()

    date = datetime.today().strftime('%Y-%m-%d')

    directory = r'/media/cdc/MPPC-QC/QC-data'
    mppcStr = "-".join(mppcList)
    path = r"%s/%s/" % (directory, mppcStr)
    os.makedirs(path, exist_ok=True) 

    picoscopes = ['IW098/0028','IW114/0004']
    fnGen = "GO024/040"

    pmt = "1.2"

    biasVoltageList = [83, 82.5, 82, 81.5, 81, 80.5, 80, 79.5, 79, 78.5, 78]

    # bias voltage is key, first list if 50mV range, second list is 200mV
    ledVoltageMap = {83  : [[805, 810, 820],[840,870,890]],
                     82.5: [[805, 810, 820],[840,870,890]],
                     82  : [[805, 810, 820],[840,870,890]],
                     81.5: [[805, 810, 820],[840,870,890]],
                     81  : [[805, 810, 820],[840,870,890]],
                     80.5: [[805, 810, 820],[840,870,890]],
                     80  : [[805, 810, 820],[840,870,890]],
                     79.5: [[805, 810, 820],[840,870,890]],
                     79  : [[805, 810, 820],[840,870,890]],
                     78.5: [[805, 810, 820],[840,870,890]],
                     78  : [[805, 810, 820],[840,870,890]],
    }

    targetVoltage   = biasVoltageList[0]
    normIncrement   = 2
    threshVoltage   = 76
    threshIncrement = 0.5

    jumpTarget = 70

    vs = vc.voltageSettings(reset, threshVoltage, normIncrement, 
                            threshIncrement)

    if not reset:
        vc.rampVoltage(vs, 0)
        vs.instrument.write("*RST")
 
    vc.runSetup(vs, targetVoltage, "2.5e-4")

    vc.jumpVoltage(vs, jumpTarget)

    vc.rampVoltage(vs, targetVoltage)

    initPicoScopes(picoscopes, fnGen)

    input("Ramp PMT then press enter to begin...")

    try:
        for bias in biasVoltageList:
            vc.rampVoltage(vs, bias)
            mvLists = ledVoltageMap[bias]
            daqPerBias(bias, mppcStr, mvLists[0], mvLists[1], date, d=path, pmt=pmt, extra=extra)
    except:
        vc.rampVoltage(vs, 0)

    try:
        vc.rampVoltage(vs, jumpTarget)
        vc.jumpVoltage(vs, 0)

        vs.instrument.write("*RST") # Reset all parameters now that safely ramped down
    except:
        print("\nUnexpected error occurred while ramping to 0!!")
        print("Please ramp down the device manually")
        print("    To do so, press the \"Config/Local\" button and then use the")
        print("    up and down arrow buttons in the \"V-SOURCE\" box, with gray")
        print("    body colour and a white triangular arrow.")
        print("")
        print("    Change which digit to increment with the left and right arrow")
        print("    buttons directly below them, with white body colour and gray")
        print("    arrows.")
        print("")
        print("    Once safely ramped down, press the \"OPER\" button to switch")
        print("    the voltage off.")
        print("")
        print("    DO NOT USE THE PURE WHITE \"RANGE\" BUTTONS!!!")
        exit()
    
    print("Total elapsed time:", int(time.time() - start), "s")

if __name__ == '__main__':
    args = sys.argv

    reset = True
    if "--no-reset" in args:
        reset = False
        args.remove("--no-reset")

    if len(args) < 4 | len(args) > 5:
        print("python3 fullDaq.py n1 n2 n3 [label]")
        print("Add each mppc number in separate argument!")
        print("Additional label string is optional\n")
    if len(args) == 4:
        main(args[1:4], reset)
    else:
        main(args[1:4], reset, args[4])