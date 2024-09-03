
# -*- coding: utf-8 -*-
"""
Created on Mon 29/06/2020
Author: Sam Dekkers
Last Update by Sam Dekkers: Mon 04/07/22

Script for ramping up voltage on Keithley 6487 Picoammeter via an RS232 connection

Arguments
[1] = Reset Connection
[2] = Voltage Level Required
[3] = Voltage Increment Before Threshold
[4] = Threshold Voltage
[5] = Voltage Increment After Threshold
[6] InitDAQ = 1, else use 0'

"""
import sys
print(sys.path)
import pyvisa as visa
import time
import numpy as np
import daq6000a as daq

args = sys.argv
numberargs = 7
minArgs = 6
ExitFlag=0
if(len(args) > numberargs | len(args) < minArgs):
    print('Use this file to set voltage on Keithley 6487 Picoammeter + Voltage Source: ')
    print(' ')
    print('sudo python DAQ_VoltageControl_RS232.py [1] [2] [3] [4] [5] [6]')
    print(' ')
    print(' [1] Reset Connection = 1, else use 0')
    print(' [2] Voltage Level Required (.1f)')
    print(' [3] Voltage Increment Before Threshold Voltage (.1f)')
    print(' [4] Threshold Voltage (above which voltage ramp is in steps of [5]) (.1f)')
    print(' [5] Voltage Increment After Threshold Voltage (.1f)')
    print(' [6] InitDAQ = 1, else use 0')
    print(' ')
    print('If not working make sure permissions have been given to communicate via USB port')
    print('E.g. sudo chmod 666 /dev/{Correct USB port}')
    print('Also check that the USB port that RS232 is connected to matches the one below:')
    print('I.e instrument = rm.open_resource(u''ASRL/dev/{Correct USB port}::INSTR'')')
    sys.exit()
    ExitFlag=1
if(len(args)>1):
    if (args[1]=='-help' or args[1]=='-h'):
        print('Use this file to set voltage on Keithley 6487 Picoammeter + Voltage Source: ')
        print(' ')
        print('sudo python DAQ_VoltageControl_RS232.py [1] [2] [3] [4] [5] [6]')
        print(' ')
        print(' [1] Reset Connection = 1, else use 0')
        print(' [2] Voltage Level Required (.1f)')
        print(' [3] Voltage Increment Before Threshold Voltage (.1f)')
        print(' [4] Threshold Voltage (above which voltage ramp is in steps of [5]) (.1f)')
        print(' [5] Voltage Increment After Threshold Voltage (.1f)')
        print(' [6] InitDAQ = 1, else use 0')
        print(' ')
        print('If not working make sure permissions have been given to communicate via USB port')
        print('E.g. sudo chmod 666 /dev/{Correct USB port}')
        print('Also check that the USB port that RS232 is connected to matches the one below:')
        print('I.e instrument = rm.open_resource(u''ASRL/dev/{Correct USB port}::INSTR'')')
        sys.exit()
if(ExitFlag==1): sys.exit()

########################SETUP######################################################################
rm = visa.ResourceManager() 
print(rm.list_resources()) #shows available connections (good to see if wrong USB port is being used - pick whichever the RS232 is being listened to on)
#If no ports are listed, might require a reset of the picoammeter
time.sleep(0.5)
instrument=rm.open_resource(u'ASRL/dev/ttyUSB0::INSTR')
#instrument=rm.open_resource(u'ASRL19::INSTR') #for use with windows pc

ResetFlag = int(args[1]) #reset flag helps for rectifying some communication issues - don't use if voltage already high in case you damage whatever is connected

#Instrument communication parameters

instrument.baud_rate=9600 #how many bits per second are coommunicated - 9600 is used for RS232 communication on the 6487

#Setup command character terminations - basically the settings so that the PC and instrument
#recognise the end of a command - this is instrument dependent unfortunately and not always
#clear as to what it should be set to.
instrument.write_termination = '\n'
instrument.read_termination = '\n'

instrument.timeout=5000 #timeout occurs if nothing is heard back in this many ms

#First need to send a query command for instrument identification to test both read and write commands are
#functioning at both ends - if everything is okay then instrument.write('*IDN?') will return the instrument
#details when we print instrument.read('\n')

#Display settings
print("%%%%%%%%%RS232-USB Configuration Settings%%%%%%%%%%%")
print("Write Termination Character: \\n")
print("Read Termination Character: \\n")
print("Instrument Address: 22")

#Reset before starting if we want - not really necessary if everything is working well
if(ResetFlag==1):
    print(instrument.write("*RST"))
    print("RESET")

instrument.write('*IDN?')
print(instrument.read('\n'))

##########################################################################################################
##########################VOLTAGE CONDITIONS#################################################################
VoltageLevel = float(args[2])#How high voltage is to be set (V)

NormIncrement = float(args[3])#Voltage increment below the threshold

ThresholdVoltage = float(args[4])#Voltage level after which it will increment in ThreshIncrement volts

ThreshIncrement = float(args[5]) #Voltage increment after the threshold is reached

RangeVoltage = 0.0

CompFlag = 0 #flag for checking current limiting error

EndFlag = 0 #flag for checking whether still operating or ramping down to finish script
###################################Set Voltage Range Function######################################################
def SetRange(VoltageLevel,instrument):
    global RangeVoltage
    RangeVoltage = 0.0
    if(VoltageLevel<10.0):
        instrument.write("SOUR:VOLT:RANG 10") #Set voltage range to 10 V range
        print("Voltage Range: 10 V")
        RangeVoltage = 10.0
    elif(VoltageLevel>=10.0 and VoltageLevel<50.0):
        instrument.write("SOUR:VOLT:RANG 50") #Set voltage range to 50 V range
        print("Voltage Range: 50 V")
        RangeVoltage = 50.0
    elif(VoltageLevel>=50.0 and VoltageLevel<500.0):
        instrument.write("SOUR:VOLT:RANG 500") #Set voltage range to 500 V range
        print("Voltage Range: 500 V")
        RangeVoltage = 500.0
    else:
        print("Error! Voltage must be between 0.0 and 500.0 V!")
        sys.exit()
##################################Read Voltage Function##########################################################################
def ReadVoltage(instrument):
    instrument.write("INIT")
    instrument.write("FORM:ELEM VSO")
    instrument.write("READ?")
    VoltageRead = str(instrument.read()).split(',')
    VoltageRead = ((np.array(VoltageRead)).astype(float)[0]) #Convert results to float array
    VoltageRead = np.round(VoltageRead,2)
    return VoltageRead
#################################Ramp Down Function##############################################################################
def ZeroVoltage(VoltageRead,instrument):
    while(VoltageRead>0.0):
        if(VoltageRead>ThresholdVoltage):
            VoltageRead-=ThreshIncrement
        else:
            VoltageRead-=NormIncrement
        
        if(VoltageRead<2.0):
            VoltageRead=0.0
        
        voltagecommand='SOUR:VOLT '+str(VoltageRead)
        
        sys.stdout.write("\r Voltage: %.2f V" %VoltageRead)
        sys.stdout.flush()
        
        instrument.write(voltagecommand)
        time.sleep(1)
        
        if(CompFlag==0): VoltageRead = ReadVoltage(instrument) #Need to check whether comp error occured or not otherwise can't ramp down with checking read voltage
    instrument.write("*RST") #Reset instrument settings on instrument now that voltage safely at 0 V
        

######################################Start Voltage Ramp###############################################
#Check whether the 6487 is at 0 V before beginning ramp up
VoltageRead = ReadVoltage(instrument)
print(VoltageRead)
#Turn off voltage if VoltageRead is not 0 V at the beginning...
if(VoltageRead>0.0): ZeroVoltage(VoltageRead,instrument)
VoltageRead = ReadVoltage(instrument)
SetRange(VoltageLevel,instrument)
instrument.write("SOUR:VOLT:ILIM 2.5e-4") #Limit current?
instrument.write("SOUR:VOLT:STAT ON") #Turn voltage source on
EndFlag=0 #Check whether we are still operating or want to ramp down
CompFlag=0 #Error flag
    
#Here we track both the actual reading and what this reading should be
#CurrentVoltage = what the voltage should be
#VoltageRead = what the voltage read from the instrument is

if len(args < numberargs):
    DAQ_Init = "1"
else:
    DAQ_Init = str(args[6])
DAQ_ParamsLoaded = 0
DAQ_End = 0
picoscopes = ['IW098/0028','IW114/0004']
if (DAQ_Init == "1"):
    for ps in picoscopes:
        print("Picoscope:", ps)
        status = 0
        status = daq.multiSeriesInitDaq(ps)
        if status == 0:
            print("Python - DAQ device not opened correctly: ",ps)
            print(status)
else:
    print("DAQ not initialized at launch because of arg [6]")
    DAQ_End = 1
while(EndFlag==0):

    CurrentVoltage = VoltageRead #Keep track of voltage as increase it

    if(CurrentVoltage<VoltageLevel):
        while(CurrentVoltage<VoltageLevel):
            if((CurrentVoltage+NormIncrement)<=ThresholdVoltage):
                CurrentVoltage+=NormIncrement
            else: CurrentVoltage+=ThreshIncrement

            if(CurrentVoltage>VoltageLevel):
                CurrentVoltage=VoltageLevel
            CurrentVoltage = np.round(CurrentVoltage,2)
            voltagestr=str(CurrentVoltage)
            voltagecommand='SOUR:VOLT '+str(voltagestr)
            instrument.write(voltagecommand)
            
            #Read whether it actually is this voltage or some other error has occured...
            VoltageRead = ReadVoltage(instrument)
    
            #Displays what voltage read from instrument is after sending most recent voltage increment
            sys.stdout.write("\r Voltage: %.2f V" % VoltageRead)    
            sys.stdout.flush()
                
            #If the voltage desired doesn't match the voltage read (e.g. instrument sends an error code back) this will
            #set the voltage level target to 0.0 essentially breaking out immediately to ramp down   
            if(VoltageRead!=CurrentVoltage and CompFlag==0):
                print("Error!")
                if(VoltageRead==-999.0): print("Current COMPL Error!") #specifically if too much current is drawn - wrong connection somewhere!
                VoltageLevel=0.0
                CompFlag=1

            time.sleep(2)

    elif(CurrentVoltage>VoltageLevel):
        while(CurrentVoltage>VoltageLevel):
            if(CurrentVoltage>ThresholdVoltage):
                CurrentVoltage-=ThreshIncrement
            else:CurrentVoltage-=NormIncrement

            if(CurrentVoltage<VoltageLevel):
                CurrentVoltage=VoltageLevel
            CurrentVoltage = np.round(CurrentVoltage,2)
            voltagestr=str(CurrentVoltage)
            voltagecommand='SOUR:VOLT '+str(voltagestr)
             
            instrument.write(voltagecommand)
            
            VoltageRead = ReadVoltage(instrument)    
                
            sys.stdout.write("\r Voltage: %.2f V" % CurrentVoltage)
            sys.stdout.flush()
                
            if(VoltageRead!=CurrentVoltage and CompFlag==0):
                print("Error!")
                VoltageLevel=0.0
                if(VoltageRead==-999.0): print("Current COMPL Error!")
                CompFlag=1
                
            time.sleep(2)

    print("")
    print("Voltage now set to %f V"%(CurrentVoltage))

    if(CompFlag==0):
        CommandFlag=0
        while(CommandFlag==0):
            print("==============================================================================")
            print("Enter next command (enter ""L"" for options): ")
            VComm = str(input())
            if(VComm=="L"):
                print("")
                print("Available options: ")
                print("")
                print("SetDAQ => set the data acquisition system parameters")
                print("StartDAQ => start the data acquisition system")
                print("StopDAQ => stop the data acquisition system")
                print("ResetDAQ => reset the data acquisition system")
                print("NI => set new voltage increment for below the threshold voltage")
                print("TI => set new voltage increment for above the threshold voltage")
                print("TV => set new threshold voltage")
                print("")
                print("Just enter a float value to continue and set to a new voltage")
                print("Else enter 0 to ramp down and finish script!")
                print("")
                     
            elif(VComm=="SetDAQ"):
                if(DAQ_Init!="1"):
                    print("DAQ not initialized yet, do ResetDAQ first")
                    continue
                if(DAQ_End==1):
                    print("DAQ already stopped!")
                    continue
                print("Setup DAQ with one of the default setups? (y/n): ")
                DAQ_default = str(input())
                if (DAQ_default=="n" or DAQ_default=="no" or DAQ_default=="N" or DAQ_default=="No"):
                    print("Input DAQ settings as:")
                    print("chATrigger,chAVRange,chAWfSamples,")
                    print("chBTrigger,chBVRange,chBWfSamples,")
                    print("chCTrigger,chCVRange,chCWfSamples,")
                    print("chDTrigger,chDVRange,chDWfSamples,")
                    print("auxTrigger,timebase,numWaveforms,samplesPreTrigger")
                    try:
                        DAQ_args = input().replace(" ","").split(",")
                        int_DAQ_args = [int(p) for p in DAQ_args]
                        if (len(int_DAQ_args)!=16):
                            print(int_DAQ_args, "is not a valid input")
                        else:
                            daq.multiSeriesSetDaqSettings(*int_DAQ_args)
                            DAQ_ParamsLoaded=1
                    except:
                        print("Input is not valid")
                elif (DAQ_default=="y" or DAQ_default=="yes" or DAQ_default=="Y" or DAQ_default=="Yes"):
                    print("----------------------------------------------------")
                    print("Setup 1: Generic pulse catching, 4 channels")
                    print("ChA: 0, 2, 1000")
                    print("ChB: 0, 2, 1000,")
                    print("ChC: 0, 2, 1000,")
                    print("ChD: 0, 2, 1000,")
                    print("AuxTrigger: 100")
                    print("Timebase: 2 (0.8ns)")
                    print("NumWaveforms: 50000")
                    print("SamplesPreTrigger: 0")
                    print("----------------------------------------------------")
                    print("Setup 2: Faster pulse catching, 2 channels")
                    print("ChA: 0, 2, 1000")
                    print("ChB: 0, 99, 0,")
                    print("ChC: 0, 2, 1000,")
                    print("ChD: 0, 99, 0,")
                    print("AuxTrigger: 100")
                    print("Timebase: 1 (0.4ns)")
                    print("NumWaveforms: 50000")
                    print("SamplesPreTrigger: 0")
                    print("----------------------------------------------------")
                    print("Setup 3: Fastest pulse catching, 1 channel")
                    print("ChA: 0, 2, 1000")
                    print("ChB: 0, 99, 0,")
                    print("ChC: 0, 99, 0,")
                    print("ChD: 0, 99, 0,")
                    print("AuxTrigger: 100")
                    print("Timebase: 0 (0.2ns)")
                    print("NumWaveforms: 50000")
                    print("SamplesPreTrigger: 0")
                    print("----------------------------------------------------")
                    print("Setup 4: 3 MPPC + 1 PMT, LED")
                    print("Note that this does not work for the time being")
                    print("ChA: 0, 2, 1000")
                    print("ChB: 0, 2, 1000,")
                    print("ChC: 0, 2, 1000,")
                    print("ChD: 0, 2, 5000,")
                    print("AuxTrigger: 100")
                    print("Timebase: 2 (0.8ns)")
                    print("NumWaveforms: 50000")
                    print("SamplesPreTrigger: 0")
                    print("----------------------------------------------------")
                    print("Setup 5: 3 MPPC + 1 PMT, Dark photons")
                    print("ChA: 0, 2, 3000")
                    print("ChB: 0, 2, 3000,")
                    print("ChC: 0, 2, 3000,")
                    print("ChD: 0, 2, 3000,")
                    print("AuxTrigger: 100")
                    print("Timebase: 2 (0.8ns)")
                    print("NumWaveforms: 50000")
                    print("SamplesPreTrigger: 0")
                    print("----------------------------------------------------")
                    print("Choose a default setup: (1, 2, 3, 4 or 5)")
                    try:
                        DAQ_setup = str(input())
                        if (DAQ_setup == "1"):
                            daq.multiSeriesSetDaqSettings(
                                0, 2, 1000,
                                0, 2, 1000,
                                0, 2, 1000,
                                0, 2, 1000,
                                100, 2, 50000, 0)
                            DAQ_ParamsLoaded=1
                        elif (DAQ_setup == "2"):
                            daq.multiSeriesSetDaqSettings(
                                0, 2, 1000,
                                0, 99, 0,
                                0, 2, 1000,
                                0, 99, 0,
                                100, 1, 50000, 0)
                            DAQ_ParamsLoaded=1
                        elif (DAQ_setup == "3"):
                            daq.multiSeriesSetDaqSettings(
                                0, 2, 1200,
                                0, 99, 0,
                                0, 99, 0,
                                0, 99, 0,
                                100, 0, 50000, 0)
                            DAQ_ParamsLoaded=1
                        elif (DAQ_setup == "4"):
                            print("This setup has an error that I haven't fixed, please choose another setup")
                            continue
                            daq.multiSeriesSetDaqSettings(
                                0, 2, 1000,
                                0, 2, 1000,
                                0, 2, 1000,
                                0, 2, 5000,
                                100, 2, 50000, 0)
                            DAQ_ParamsLoaded=1
                        elif (DAQ_setup == "5"):
                            daq.multiSeriesSetDaqSettings(
                                0, 2, 3000,
                                0, 2, 3000,
                                0, 2, 3000,
                                0, 2, 3000,
                                100, 2, 50000, 0)
                            DAQ_ParamsLoaded=1
                        else:
                            print(DAQ_setup, "Error occured during setup setting")
                    except:
                        print("Error occured during setup setting")
                    
                else:
                    print(DAQ_default, " is not a valid input")
                if (DAQ_ParamsLoaded==1):
                    print("DAQ settings loaded.")
                
                
            elif(VComm=="StartDAQ"):
                if(DAQ_Init!="1"):
                    print("DAQ not initialized yet, do ResetDAQ first")
                    continue
                if(DAQ_End==1):
                    print("DAQ already stopped!")
                    continue
                if (DAQ_ParamsLoaded != 1):
                    print("Set some DAQ parameters first!")
                else:           
                    try:
                        print("Input name for the DAQ output file:")
                        DAQ_outFile = r"./data/"+str(input())
                        print("DAQ output file:", DAQ_outFile) 
                        daq.multiSeriesCollectData(DAQ_outFile)
                    except:
                        print("An error occured during DAQ.")
                        CommandFlag=1
                        EndFlag=1
                        print("Ramping voltage down now!") 

            elif(VComm=="StopDAQ"):
                if(DAQ_Init!="1"):
                    print("DAQ not initialized yet, do ResetDAQ first")
                    continue
                if (DAQ_End != 0):
                    print("DAQ already stopped.")
                else:
                    DAQ_End = 1 
                    DAQ_ParamsLoaded = 0
                    DAQ_Init = "0"
                    daq.multiSeriesCloseDaq()
                    print("DAQ stopped.")

            elif(VComm=="ResetDAQ"):
                if (DAQ_End == 1):
                    for ps in picoscopes:
                        status = 0
                        status = daq.multiSeriesInitDaq(ps)
                        if status == 0:
                            print("Python - DAQ device not opened correctly:",ps)
                            print(status)
                    DAQ_End = 0
                    DAQ_Init = "1"
                else:
                    print("The DAQ system needs to be stopped to reset!")
                
            elif(VComm=="NI"):
                print("Enter new below threshold increment voltage (0.1f): ")
                NormIncrement = float(input())
            elif(VComm=="TI"):
                print("Enter new above threshold increment voltage (0.1f): ")
                ThreshIncrement = float(input())
            elif(VComm=="TV"):
                print("Enter new threshold voltage (0.1f): ")
                ThresholdVoltage = float(input())
            else:
                try:
                    PrevVoltageLevel = VoltageLevel
                    VoltageLevel = float(VComm)
                    
                    if(VoltageLevel>500 or VoltageLevel<0): 
                        VoltageLevel = PrevVoltageLevel
                        print("Voltage level must be between 0 V and 500 V!")
                    else: print("Now setting to %f V"%(VoltageLevel))
                    
                    CommandFlag=1
            
                    if(VoltageLevel>RangeVoltage):
                        print("New voltage above current voltage range...")
                        SetRange(VoltageLevel,instrument)

                except ValueError:
                    print("Need to enter a valid voltage input!!!")
            
    if(VoltageLevel==0):
        EndFlag=1
        print("Ramping voltage down now!")
        print("==============================================================================")

###################################Safely Ramp Down (if not 0 V)############################################
if(DAQ_End==0): daq.multiSeriesCloseDaq()
VoltageRead = ReadVoltage(instrument)
if(CompFlag==1): VoltageRead=CurrentVoltage #if an error code is being read, cant use the readout to ramp down so use what it should have been to start ramping down   
if(VoltageRead>0.0):ZeroVoltage(VoltageRead,instrument)
print("")
print("Voltage ramp complete!")
print("==============================================================================")
##########################################################################################################
instrument.clear()
