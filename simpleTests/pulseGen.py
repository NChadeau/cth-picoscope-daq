import daq6000 as daq

daq.initFunctionGenerator("GO024/040")
daq.runFunctionGenerator(900,38)
mv_str = input()
while mv_str != '':
    mv = int(mv_str)
    daq.runFunctionGenerator(mv,38)
    mv_str = input()
daq.closeFunctionGenerator()

