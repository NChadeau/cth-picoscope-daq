import os
import sys
from datetime import datetime
import time
import IV_Curve as vc
import sanityCheck as sc
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

def daqPerBias(bias, mppcStr, mv50List, mv200List, date, pmt, d, extra):
    """Runs DAQ for a range of LED Voltages for a given bias voltage"""
    s = r'%s'
    outFilePattern = d + r"%s_%sV_%s_%skV_%s%s" % \
                        (date, str(bias), s, pmt, mppcStr, extra)
    
    runDark(outFilePattern)

    runMvList(2, outFilePattern, mv50List, 1) # 20 mV PMT range for low light

    runMvList(4, outFilePattern, mv200List)

    return

def runDark(oFilePattern):
    """Runs DAQ for dark photos: no LED, very low voltage range"""
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

def runMvList(vRange, oFilePatternRaw, mvList, pmtVRange=2):
    """Runs DAQ for range of LED voltages for a given bias and voltage range"""
    daq.multiSeriesSetDaqSettings(
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    0, pmtVRange, 400, # 50mV is good for all, may want to reduce it for mv50List
                    100, 2, 20000, 0)

    oFilePattern = oFilePatternRaw % ("%imV")
    for mv in mvList:
        out = oFilePattern % mv
        gen.runFunctionGenerator(mv,38)
        print("\n\n\nNext DAQ: %s" % out)
        daq.multiSeriesCollectData(out)

    return

def sanityCheck(bias, mppcStr, ledV, date, pmt, d, extra, picoscopes):
    """
    Runs short DAQ with high LED and bias to check if the signals seem reasonable.

    Specific values are hard coded in sanityCheck.py. It will halt if the value
    is low, but can be continued or killed if it fails.
    """
    daq.multiSeriesSetDaqSettings(
                    0, 4, 400,
                    0, 4, 400,
                    0, 4, 400,
                    0, 2, 400,
                    100, 2, 1000, 0)
    
    out = d + r"Check_%s_%sV_%s_%skV_%s%s" % \
                (date, str(bias), ledV, pmt, mppcStr, extra)
    
    gen.runFunctionGenerator(ledV,38)
    print("\n\n\nNext DAQ: %s" % out)
    daq.multiSeriesCollectData(out)

    ex = False
    for ps in picoscopes:
        res = sc.sanityBool(out + "_%s.dat" % ps.replace("/","-"), output=True)
        if not res:
            print("\nERROR: Issues present in Picoscope %s" % ps)
            ex = True
    
    if ex:
        cont = input("Continue anyways? [y]/n")
        if cont != '':
            if cont[0].lower() == 'n':
                return False

    return True

def main(mppcList, reset, extra=''):

    start = time.time()

    date = datetime.today().strftime('%Y-%m-%d')

    directory = r'/media/cdc/MPPC-QC/QC-data/testing'
    mppcStr = "-".join(mppcList)
    if extra != '':
        extra = '_' + extra
    path = r"%s/%s%s/" % (directory, mppcStr, extra)
    os.makedirs(path, exist_ok=True) 

    picoscopes = ['IW098/0028','IW114/0004']
    fnGen = "GO024/040"

    pmt = "1.4"

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
        vc.rampDown(vs, jumpTarget, 0)
        vc.jumpVoltage(vs, 0)
        vs.instrument.write("*RST")
 
    vc.runSetup(vs, targetVoltage, "2.5e-4")

    vc.jumpVoltage(vs, jumpTarget)

    vc.rampVoltage(vs, targetVoltage)

    initPicoScopes(picoscopes, fnGen)

    input("Ramp PMT then press enter to begin...")

    res = sanityCheck(targetVoltage, mppcStr, 890, date, pmt, path, extra, picoscopes)
    if not res:
        vc.rampVoltage(vs, jumpTarget)
        vc.jumpVoltage(vs, 0)
        vs.instrument.write("*RST")
        exit()

    try:
        for bias in biasVoltageList:
            vc.rampVoltage(vs, bias)
            mvLists = ledVoltageMap[bias]
            daqPerBias(bias, mppcStr, mvLists[0], mvLists[1], date, pmt, path, extra)
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
    if "--crashed" in args:
        reset = False
        args.remove("--crashed")

    if len(args) < 4 | len(args) > 5:
        print("python3 fullDaq.py n1 n2 n3 [label] [--crashed]")
        print("Add each mppc number in separate argument!")
        print("  n1 = Channel A and is furthest in enclosure")
        print("  n2 = Channel B and is middle in enclosure")
        print("  n3 = Channel C and is closest in enclosure")
        print("Additional label string is optional\n")
    if len(args) == 4:
        main(args[1:4], reset)
    else:
        main(args[1:4], reset, args[4])
