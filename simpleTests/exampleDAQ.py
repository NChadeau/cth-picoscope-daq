import daq6000a as daq
import daq6000 as gen

def runMvList(vRange, oFilePattern, mvList):

    daq.multiSeriesSetDaqSettings(
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    100, 2, 50000, 0)

    for mv in mvList:
        out = oFilePattern % mv
        gen.runFunctionGenerator(mv,38)
        print("\n\n\nNext DAQ: %s" % out)
        daq.multiSeriesCollectData(out)

    return

outFile1 = r"./data/example"

outFileList = [r"./data/08Jul24_81V_Dark_0kV_3-6-8",
               r"./data/08Jul24_81V_800mV_0kV_3-6-8",
               r"./data/08Jul24_81V_700mV_0kV_3-6-8"
               ]

outFilePattern = r"./data/08Jul24_81V_%imV_0kV_3-6-8"

outFileDark = r"./data/08Jul24_81V_Dark_0kV_3-6-8"

mv50List  = [805, 810, 820, 830, 840, 850]
mv100List = [860, 870, 880]
mv200List = [890, 900]
mv500List = [905, 920]

darkDaq = [ 0, 2, 2000,
            0, 2, 2000,
            0, 2, 2000,
            0, 2, 2000,
            100, 2, 50000, 0]

picoscopes = ['IW098/0028','IW114/0004']
print(picoscopes)

for ps in picoscopes:
    status = daq.multiSeriesInitDaq(ps)
    if status == 0:
        print("Python - Device not opened")
        exit()

daq.multiSeriesSetDaqSettings(*darkDaq)
daq.multiSeriesCollectData(outFileDark)

gen.initFunctionGenerator("GO024/040")

runMvList(2, outFilePattern, mv50List)

runMvList(3, outFilePattern, mv100List)

runMvList(4, outFilePattern, mv200List)

runMvList(5, outFilePattern, mv500List)

daq.multiSeriesCloseDaq()
gen.closeFunctionGenerator()
