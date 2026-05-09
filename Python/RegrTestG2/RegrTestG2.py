#!/usr/bin/env python
#
#===============================================================================
#
#                         OOOO
#                       OOOOOOOO
#      PPPPPPPPPPPPP   OOO    OOO   PPPPPPPPPPPPP
#    PPPPPPPPPPPPPP   OOO      OOO   PPPPPPPPPPPPPP
#   PPP         PPP   OOO      OOO   PPP         PPP
#  PPP          PPP   OOO      OOO   PPP          PPP
#  PPP          PPP   OOO      OOO   PPP          PPP
#  PPP          PPP   OOO      OOO   PPP          PPP
#   PPP         PPP   OOO      OOO   PPP         PPP
#    PPPPPPPPPPPPPP   OOO      OOO   PPPPPPPPPPPPPP
#     PPPPPPPPPPPPP   OOO      OOO   PPP
#               PPP   OOO      OOO   PPP
#               PPP   OOO      OOO   PPP
#               PPP   OOO      OOO   PPP
#               PPP    OOO    OOO    PPP
#               PPP     OOOOOOOO     PPP
#              PPPPP      OOOO      PPPPP
#
# @file:   RegrTestG2.py
# @author: Hugh Spahr
# @date:   12/12/2015
#
# @note:   Open Pinball Project
#          Copyright 2015, Hugh Spahr
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#===============================================================================
#
# Tests for Gen2 cards.
#
#===============================================================================

# Support Python 3 print(,end="") functionality so py2.7 and py3.x work
from __future__ import print_function

testVers = '00.00.08'

import sys
import serial
import array
import time
import re
import rs232BIntf
import subprocess
import os

port = 'COM1'
vers = ""
testNum = 255
data = ""
NUM_MSGS = 1
currInpData = []
matrixInpData = []
numGen2Brd = 0
gen2AddrArr = []
currWingCfg = []
hasMatrix = []
versInt = 0

CRC8ByteLookup = \
    [ 0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, \
      0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, \
      0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, \
      0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, \
      0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, \
      0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, \
      0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a, \
      0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, \
      0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, \
      0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, \
      0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, \
      0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, \
      0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, \
      0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, \
      0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, \
      0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3 ]

# Config inputs as all state inputs
wingCfg = [ [ rs232BIntf.WING_NEO, rs232BIntf.WING_SOL, rs232BIntf.WING_INP, rs232BIntf.WING_INCAND ] ]

# Config inputs as all state inputs
inpCfg = [ [ rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE ], \
           [ rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, \
             rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE, rs232BIntf.CFG_INP_STATE ] ]

# Config for solenoid wing board in second position, first two config'd as flippers, second two config'd as one-shots
solCfg =  [ [ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              rs232BIntf.CFG_SOL_USE_SWITCH, 0x30, 0x04, rs232BIntf.CFG_SOL_USE_SWITCH, 0x30, 0x04, \
              rs232BIntf.CFG_SOL_USE_SWITCH, 0x10, 0x00, rs232BIntf.CFG_SOL_USE_SWITCH, 0x10, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ], \
            [ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              rs232BIntf.CFG_SOL_USE_SWITCH, 0x01, 0x01, rs232BIntf.CFG_SOL_USE_SWITCH, 0xff, 0x0f, \
              rs232BIntf.CFG_SOL_USE_SWITCH, 0x01, 0x00, rs232BIntf.CFG_SOL_USE_SWITCH, 0xff, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ] ]

# Config color table
#              Entry 0                 Entry 1                 Entry 2                 Entry 3 */
colorCfg = [ [ 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, \
               0xff, 0x00, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
               0x10, \
            ] ]

#calculate a crc8
def calcCrc8(msgInts):
    crc8Byte = 0xff
    for indInt in msgInts:
        crc8Byte = CRC8ByteLookup[crc8Byte ^ indInt];
    return (crc8Byte)

def getChar():
    global windows
    if windows:
        return msvcrt.getch()
    else:
        fd = sys.stdin.fileno()
        oldSettings = termios.tcgetattr(fd)

        try:
            tty.setcbreak(fd)
            answer = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, oldSettings)

        return answer

def kbHit():
    global windows
    if windows:
        return msvcrt.kbhit()
    else:
        return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

def getAsynchChar():
    global windows
    if windows:
        return msvcrt.getch()
    else:
        return sys.stdin.read(1)

def runTest(test):
    global windows
    if windows:
        retVal = test()
    else:
        old_settings = termios.tcgetattr(sys.stdin)
        try:
            tty.setcbreak(sys.stdin.fileno())
            retVal = test()

        finally:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
    return retVal

def writeNoCR(text):
    global windows
    sys.stdout.write(text)
    if windows == False:
        sys.stdout.flush()

#grab data from serial port
def getSerialData():
    global ser
    resp = ser.read(32)
    return (resp)

#send inventory cmd
def sendInvCmd():
    global ser
    cmdArr = []
    cmdArr.append(rs232BIntf.INV_CMD)
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)

#rcv inventory resp
def rcvInvResp(append = True):
    global numGen2Brd
    global gen2AddrArr
    global currInpData
    global matrixInpData

    global hasMatrix
    global currWingCfg
    data = getSerialData();
    #First byte should be inventory cmd
    index = 1
    if (len(data) == 0):
        print ("No data received.  Are the Tx/Rx jumpers installed?")
        return (100)
    if (data[0] != rs232BIntf.INV_CMD):
        return (101)
    if (len(data) < index + 1):
        print ("Could not find EOM.")
        return (102)
    numGen2Brd = 0
    gen2AddrArr = []
    while (data[index] != rs232BIntf.EOM_CMD):
        if ((data[index] & rs232BIntf.CARD_ID_TYPE_MASK) == rs232BIntf.CARD_ID_GEN2_CARD):
            numGen2Brd = numGen2Brd + 1
            gen2AddrArr.append(data[index])
            if (append):
                currInpData.append(0)
                currWingCfg.append(0)
                hasMatrix.append(False)
                matrixInpData.append([0,0,0,0,0,0,0,0])
        index = index + 1
        if (len(data) < index + 1):
            print ("Could not find EOM.")
            return (103)
    print ("Found {} Gen2 brds.".format(numGen2Brd))
    print("Addr = {}".format(''.join(["0x%02x " % byte for byte in gen2AddrArr])))
    return (0)

#send standard 4 byte command
def sendStd4ByteCmd(cmd):
    global ser
    global gen2AddrArr
    cmdArr = []
    
    # First address for Gen2 cards is always 0x20
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(cmd)
    cmdArr.append(0x00)
    cmdArr.append(0x00)
    cmdArr.append(0x00)
    cmdArr.append(0x00)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)

#send get version cmd
def sendGetVersCmd():
    sendStd4ByteCmd(rs232BIntf.GET_VERS_CMD)

#rcv get version resp
def rcvGetVersResp(version):
    global numGen2Brd
    global gen2AddrArr
    global currInpData
    global currWingCfg
    global versInt
    data = getSerialData();
    if (data[0] != gen2AddrArr[0]):
        print("\nData = {0}, expected = {1}".format(data[0],gen2AddrArr[0]))
        print(repr(data))
        return (200)
    if (data[1] != rs232BIntf.GET_VERS_CMD):
        print("\nData = {0}, expected = {1}".format(data[1],rs232BIntf.READ_GEN2_INP_CMD))
        print(repr(data))
        return (201)
    tmpData = [ data[0], data[1], data[2], data[3], data[4], data[5] ]
    crc8 = calcCrc8(tmpData)
    if (data[6] != crc8):
        print("\nBad CRC, Data = {0}, expected = {1}".format(data[6],crc8))
        return (202)
    if (data[7] != rs232BIntf.EOM_CMD):
        print("\nData = {0}, expected = {1}".format(data[7],rs232BIntf.EOM_CMD))
        return (203)
    versResp = "%d.%d.%d.%d" % (data[2], data[3], data[4], data[5])
    print("Found version " + versResp)
    if version:
        # Verify the version number if it was passed in on the command line
        if version != versResp:
            print("\n!!! Fail !!! Version does not match expected version.\n")
            print("\nData = {0}, expected = {1}".format(versResp, version))
            return (204)
    versInt = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5]
    return (0)

#send get serial number cmd
def sendSetSerNumCmd(serialNum):
    global ser
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.SET_SER_NUM_CMD)
    cmdArr.append((serialNum >> 24) & 0xff)
    cmdArr.append((serialNum >> 16) & 0xff)
    cmdArr.append((serialNum >> 8) & 0xff)
    cmdArr.append(serialNum & 0xff)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)

#send get serial number cmd
def sendGetSerNumCmd():
    sendStd4ByteCmd(rs232BIntf.GET_SER_NUM_CMD)

#rcv get serial number resp
def rcvGetSerNumResp():
    global numGen2Brd
    global gen2AddrArr
    global currInpData
    global currWingCfg
    expectedSerialNum = 123456789
    data = getSerialData();
    if (data[0] != gen2AddrArr[0]):
        print("\nData = {0}, expected = {1}".format(data[0],gen2AddrArr[0]))
        print(repr(data))
        return (300)
    if (data[1] != rs232BIntf.GET_SER_NUM_CMD):
        print("\nData = {0}, expected = {1}".format(data[1],rs232BIntf.READ_GEN2_INP_CMD))
        print(repr(data))
        return (301)
    tmpData = [ data[0], data[1], data[2], data[3], data[4], data[5] ]
    crc8 = calcCrc8(tmpData)
    if (data[6] != crc8):
        print("\nBad CRC, Data = {0}, expected = {1}".format(data[6],crc8))
        return (302)
    if (data[7] != rs232BIntf.EOM_CMD):
        print("\nData = {0}, expected = {1}".format(data[7],rs232BIntf.EOM_CMD))
        return (303)
    serialNum = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5]
    print("Found serial num: {0}".format(serialNum))
    if (serialNum == 0):
        sendSetSerNumCmd(expectedSerialNum)
        retCode = rcvEomResp()
        if (retCode != 0):
            print("\n!!! Fail !!! Could not program serial number into card.\n")
            return (304)
    elif (serialNum != expectedSerialNum):
        print("\n!!! Fail !!! Serial number not the expected value.\n")
        return (305)
    return (0)

#send reset cmd
def sendResetCmd():
    global ser
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.RESET_CMD)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)

#Update code, tests go bootloader command
def TestGoBoot():
    global ser
    global gen2AddrArr
    
    print("\nForcing board into the boot loader.  Sent Go Boot command.")
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.GO_BOOT_CMD)
    cmdArr.append(calcCrc8(cmdArr))
    ser.write(cmdArr)
    time.sleep(1)
    ser.close()

    command = "cd ..\cyflash &" \
       "c:\Python27\python.exe -m cyflash.__main__" + \
       " --serial " + port + " --serial_baudrate 115200" + \
       " ..\..\Creator\Gen2\Gen2.cydsn\CortexM0\ARM_GCC_541\Debug\Gen2.cyacd"
    print("Updating code to latest version.")
    process = subprocess.Popen(command,stdout=subprocess.PIPE, shell=True)
    proc_stdout = process.communicate()[0].strip()
    if not "Device checksum verifies OK." in proc_stdout:
        print("!!! Fail !!! Update code failed.\n")
        return 400
    # Send inventory command so card "relearns" its address.
    try:
        ser=serial.Serial(port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=.1)
    except serial.SerialException:
        print("\nCould not open " + port + " after upgrading firmware.")
        return 401
    sendInvCmd()
    rcvInvResp()
    print("GoBoot tested successfully.")
    return 0

def TestGetVersion(version):
    print("\nTesting Get Version command.")
    sendGetVersCmd()
    retCode = rcvGetVersResp(version)
    return retCode

def TestGetSerialNum():
    print("\nTesting Get Serial Number command.")
    sendGetSerNumCmd()
    retCode = rcvGetSerNumResp()
    return retCode

def ResetBoard():
    global ser
    sendResetCmd()
    ser.close()
    time.sleep(1)
    # Re-open the serial port.
    try:
        ser=serial.Serial(port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=.1)
    except serial.SerialException:
        print("\nCould not open " + port + " when resetting card.")
        return 550
    sendInvCmd()
    rcvInvResp()
    return 0

#Update board with standard configuration
def TestStoreStdCfg():
    global ser
    global windows
    
    print("\nStoring standard configuration on the card.")
    ser.close()

    if windows:
        command = "cd ..\Gen2Test & " \
           "c:\Python27\python.exe Gen2Test.py -port=" + port + " -eraseCfg & " \
           "c:\Python27\python.exe Gen2Test.py -port=" + port + " -saveCfg -loadCfg=regrCfg"
    else:
        command = "{}; {}; {}; {}".format("cd ../Gen2Test", "pwd", \
            "sudo python Gen2Test.py -port=" + port + " -eraseCfg", \
            "sudo python Gen2Test.py -port=" + port + " -saveCfg -loadCfg=regrCfg")
    process = subprocess.Popen(command,stdout=subprocess.PIPE, shell=True)
    proc_stdout = process.communicate()[0].strip()
    # Re-open the serial port.
    try:
        ser=serial.Serial(port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=.1)
    except serial.SerialException:
        print("\nCould not open " + port + " updating configuration.")
        return 500
    retCode = ResetBoard()
    if retCode: return retCode
    print("Store standard configuration successful.")
    return 0

#Verify standard configuration, tests read input cmd, verifies
#solenoid configuration working for one-shots and PWMs
def VerifyStdCfg():
    print("\nVerify standard configuration.")
    print("\nTest input switches for solenoids.")
    print("Verify 0 and 1 act like flippers.")
    print("Verify 2 and 3 act like one-shots.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nStandard configuration for solenoids failed.")
        return 601
    print("\nTest read input cmd for input switches.")
    print("Note:  PSOC4200:  First input switch on right is output for Neopixel, so not an input.")
    print("Note:  STM32F103:  Fifth input switch from right is output for Neopixel, so not an input.")
    print("All other inputs are configured in state mode.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        sendReadInpBrdCmd()
        retCode = rcvReadInpResp()
        if retCode != 0:
            print("\nRead input response failed.")
            return (retCode)
        outArr = []
        outArr.append('\r')
        for loop in range(rs232BIntf.NUM_G2_INP_PER_BRD):
            if (currInpData[0] & (1 << (rs232BIntf.NUM_G2_INP_PER_BRD - loop - 1))):
                outArr.append('1')
            else:
                outArr.append('0')
        writeNoCR(''.join(outArr))
        
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nStandard configuration for inputs failed.")
                retCode = 602
            else:
                retCode = 0
            exitReq = True
    print("")
    return retCode

#Test processor can kick solenoids (with no auto clear)
#Verify on both one-shots and flipper configurations
def TestProcNoAutoClr():
    print("\nVerify main processor driving solenoids (no auto clear).")
    print("\nTurning on flipper 1 'on' for 1 second/'off' for 1 second.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        sendKickSolCmd(0x0020, 0x0020)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(1)
        sendKickSolCmd(0x0000, 0x0020)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nSolenoid kick command failed for flippers.")
                retCode = 1401
            else:
                retCode = 0
            exitReq = True
        if not exitReq:
            time.sleep(1)
    print("\nOne shot solenoid 3 firing continuously for 1 second/off for 1 second.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        sendKickSolCmd(0x0080, 0x0080)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(1)
        sendKickSolCmd(0x0000, 0x0080)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nSolenoid kick command failed for one shots.")
                retCode = 1402
            else:
                retCode = 0
            exitReq = True
        if not exitReq:
            time.sleep(1)
    return retCode

#Test processor can kick solenoids (with auto clear)
#Verify on both one-shots and flipper configurations
def TestProcAutoClr():
    # Reconfigure solenoid with auto clear bit set
    sendCfgIndSolCmd(0x04, 0x03, 0x30, 0x04)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify main processor driving solenoids (with auto clear).")
    print("\nTurning on flipper 0 'on', wait 1 second then repeat.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        sendKickSolCmd(0x0010, 0x0010)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(1)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nSolenoid kick command failed for flippers.")
                retCode = 1501
            else:
                retCode = 0
            exitReq = True
        if not exitReq:
            time.sleep(1)
            
    # Reconfigure solenoid with auto clear bit set
    sendCfgIndSolCmd(0x06, 0x03, 0x10, 0x00)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nOne shot solenoid 2 firing once, wait 1 second then repeat.")
    print("Press (y) if working, (n) if not working.")
    
    exitReq = False
    while (not exitReq):
        sendKickSolCmd(0x0040, 0x0040)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(1)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nSolenoid kick command failed for one shots.")
                retCode = 1502
            else:
                retCode = 0
            exitReq = True
        if not exitReq:
            time.sleep(1)
    return retCode

#Test solenoid configs, verifies configure all solenoids command works.
#Two solenoids are configured as flippers, one with minimum init kick/PWM,
#one with maximum init kick/PWM.  Two solenoids are configured as one shots
#with one having minimum init kick, and one having maximum init kick.
def TestSolenoidConfig():
    # Reconfigure all solenoids with single command
    sendSolCfgCmd(1)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify solenoid 0 is flipper with minimum (1 ms init kick/min PWM)")
    print("Verify solenoid 1 is flipper with maximum (255 ms init kick/max PWM)")
    print("Verify solenoid 2 is one shot with minimum (1 ms) init kick")
    print("Verify solenoid 3 is one shot with maximum (255 ms) init kick")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nStandard configuration for solenoids failed.")
        return 1601
    return 0

#Test on/off solenoid configs, verifies on/off configuration bit works.
def TestOnOffSolConfig():
    # Reconfigure first solenoid as on/off
    sendCfgIndSolCmd(0x04, 0x05, 0x30, 0x00)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify solenoid 0 is fully on when the button is pressed (no flicker)")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nStandard configuration for solenoids failed.")
        return 1701
    # Verify processor can drive on/off solenoid
    print("\nVerify main processor driving on/off solenoid.")
    print("\nTurning solenoid 0 'on' for 1 second/'off' for 1 second.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        sendKickSolCmd(0x0010, 0x0010)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(1)
        sendKickSolCmd(0x0000, 0x0010)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nSolenoid kick command failed for on/off solenoid.")
                retCode = 1702
            else:
                retCode = 0
            exitReq = True
        if not exitReq:
            time.sleep(1)
    return 0

#Test delay solenoid config, verifies delay configuration bit works.
def TestDelaySolConfig():
    # Reconfigure second solenoid with the maximum delay for comparing
    sendCfgIndSolCmd(0x04, 0x03, 0x30, 0x00)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendCfgIndSolCmd(0x05, 0x0b, 0x30, 0x0f)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify delay solenoid cmd.")
    print("\nSolenoid 0 has a 0 ms delay and solenoid 1 has a 60 ms delay.")
    print("Solenoids are triggered every second.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        sendKickSolCmd(0x0030, 0x0030)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(1)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nSolenoid delay command failed.")
                retCode = 1801
            else:
                retCode = 0
            exitReq = True
        if not exitReq:
            time.sleep(1)
    return 0

#Test solenoid input commands.
def TestSolInputConfig():
    # Verify configuring first solenoid as not using the input switch works
    sendCfgIndSolCmd(0x04, 0x02, 0x30, 0x04)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify first solenoid is not triggered using its input switch.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid switch configuration failed.")
        return 1901
    # Verify configuring third solenoid as using the input switch, then remove
    # switch by sending set solenoid input command
    sendCfgIndSolCmd(0x06, 0x03, 0x30, 0x04)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendSetSolInputCmd(0x0a, 0x86)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify third solenoid is not triggered using its input switch.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid switch configuration failed.")
        return 1902
    # Verify third solenoid can be triggered using first solenoid's input switch
    sendSetSolInputCmd(0x08, 0x06)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify third solenoid is triggered using the first solenoid input switch.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid switch configuration failed.")
        return 1903
    # Verify third solenoid can be triggered using second input wing switch on wing 2
    sendSetSolInputCmd(0x11, 0x06)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify third solenoid is triggered using the second input switch on wing 2.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid switch configuration failed.")
        return 1904
    # Verify third solenoid triggers can be removed
    sendSetSolInputCmd(0x08, 0x86)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendSetSolInputCmd(0x11, 0x86)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify third solenoid is not triggered with first solenoid input switch")
    print("or second input switch on wing 2.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid switch configuration failed.")
        return 1905
    # Verify two solenoids can be triggered by one input.  Dual wound flipper test
    sendSetSolInputCmd(0x08, 0x04)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendSetSolInputCmd(0x08, 0x06)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendCfgIndSolCmd(0x06, 0x01, 0x30, 0x00)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendCfgIndSolCmd(0x04, 0x05, 0x00, 0x00)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify first solenoid (flipper) and third solenoid is triggered with")
    print("the first flipper input switch.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid switch configuration failed.")
        return 1906
    sendSetSolInputCmd(0x08, 0x86)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify flipper is disabled when input is removed.")
    print("Press and hold first flipper input switch, verify flipper is held.")
    print("Press (y) while holding the flipper button")
    ch = getChar()
    sendSetSolInputCmd(0x08, 0x84)
    retCode = rcvEomResp()
    print("\nVerify first solenoid (flipper) is now off.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDisable solenoid when switch configuration removed failed.")
        return 1907
    return 0

#Test cancel solenoid config.
def TestCancelSolConfig():
    # Reconfigure first solenoid having maximum initial kick but able to be canceled
    sendCfgIndSolCmd(0x04, 0x21, 0xff, 0x00)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify solenoid 0 can be canceled by tapping input switch")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nCancel initial kick of solenoid failed.")
        return 2101
    return 0

#Test solenoid pwm config.
def TestSolPwmConfig():
    # Reconfigure first solenoid having maximum initial kick but 12.5% PWM
    sendCfgIndSolCmd(0x04, 0x01, 0xff, 0x02)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    # Change initial kick PWM to match hold PWM
    sendSolKickPwmCmd(0x03, 0x04)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify solenoid 0 initial kick and hold look the same")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nInitial kick PWM set failed.")
        return 2201
    # Change initial kick PWM back to 100%
    sendSolKickPwmCmd(0x1f, 0x04)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    
    print("\nVerify solenoid 0 initial kick is 100% on and hold looks dimmer")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nInitial kick PWM set failed.")
        return 2202
    return 0

#Test incandescent commands.
def TestIncandCmds():
    global versInt
    print("\nVerify all LEDS are blinking slowly on the incandescent board.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nDefault incandescent state failed.")
        return 2001
    # Verify "on" bulbs override blinking.  Test incand "on" cmd
    sendIncandCmd(0x02, 0x0f000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify first four LEDS are on, rest are blinking slowly.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent cmd override blinking failed.")
        return 2002
    # Verify bulbs turned "off" go back to blinking.  Test incand "off" cmd
    sendIncandCmd(0x03, 0x0f000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify all LEDs are blinking slowly.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent cmd off goes back to blinking failed.")
        return 2003
    # Verify blink off command works.
    sendIncandCmd(0x06, 0xf0000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify last four LEDs are off.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent cmd blink off failed.")
        return 2004
    # Verify on command for non-blinking bulbs
    sendIncandCmd(0x02, 0xc0000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify last two LEDs are on.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent cmd turn on failed.")
        return 2005
    # Send blink off command for all bulbs.  Test fast blink
    # to every other bulb
    sendIncandCmd(0x03, 0xc0000000)
    retCode = rcvEomResp()
    sendIncandCmd(0x06, 0xff000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    sendIncandCmd(0x05, 0x55000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify every other bulb starting at first bulb is blinking fast.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent cmd blink fast failed.")
        return 2006
    # Verify on command overides off and fast blink.  Turn on first
    # four bulbs
    sendIncandCmd(0x02, 0x0f000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify first four bulbs are on.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent cmd turn on failed.")
        return 2007
    # Verify incand set state command works.  Turn off all bulbs
    sendIncandCmd(0x80, 0xff000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify all bulbs are off.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent set state off failed.")
        return 2008
    # Verify incand set state to slow blink works for first four bulbs
    sendIncandCmd(0x82, 0x0f000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify first four bulbs are blinking slowly.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent set state slow blink failed.")
        return 2009
    # Verify incand set state to fast blink works for last four bulbs
    sendIncandCmd(0x84, 0xf0000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify last four bulbs are blinking fast.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent set state fast blink failed.")
        return 2010
    # Verify incand on command over rides blinking
    sendIncandCmd(0x02, 0xff000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify all bulbs are on.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent on over-riding blinking failed.")
        return 2011
    # Verify incand on/off all off command clears the blinking state
    sendIncandCmd(0x07, 0x00000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify all bulbs are off.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent on/off comand clears blinking states.")
        return 2012
    # Verify incand on/off masked on bulbs works
    sendIncandCmd(0x07, 0xaa000000)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify every other bulb is on including last bulb.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nIncandescent on/off comand forces bulbs on/off failed.")
        return 2013
    # If version supports incand fades
    if (versInt >= 0x02010000):
        print("\n\nTesting incand fade cmds.")
        # Turn off all the bulbs
        sendIncandCmd(0x07, 0x00000000)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        # Verify turn bulbs on using fade cmd
        neoData = [0xff, 0xff, 0xff, 0xff, \
            0xff, 0xff, 0xff, 0xff]
        sendNeoCmd(4096 + 24, 0, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        print("\nVerify all incand bulbs are on.")
        print("Press (y) if working, (n) if not working.")
        ch = getChar()
        if (ch != 'y') and (ch != 'Y'):
            print("\nIncandescent On using fade failed.")
            return 2014
        # Verify turn bulbs off using fade cmd
        neoData = [0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00]
        sendNeoCmd(4096 + 24, 0, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        print("\nVerify all incand bulbs are off.")
        print("Press (y) if working, (n) if not working.")
        ch = getChar()
        if (ch != 'y') and (ch != 'Y'):
            print("\nIncandescent Off using fade failed.")
            return 2015
        # Verify intensities work
        neoData = [0x20, 0x40, 0x60, 0x80, \
            0xa0, 0xc0, 0xe0, 0xff]
        sendNeoCmd(4096 + 24, 0, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        print("\nVerify incand bulbs are on from 12.5%, 25%...to 100% On.")
        print("Press (y) if working, (n) if not working.")
        ch = getChar()
        if (ch != 'y') and (ch != 'Y'):
            print("Incandescent Intensity support using fade failed.")
            return 2016
        # Verify fade on/fade off works
        print("\nVerify Incand fade on/fade off working.")
        print("Fading from low intensity to high intensity")
        print("in 2s, wait .1s, fade to off in 2s.")
        print("Press (y) if working, (n) if not working.")
        exitReq = False
        while (not exitReq):
            neoData = [0x20, 0x40, 0x60, 0x80, \
                0xa0, 0xc0, 0xe0, 0xff]
            sendNeoCmd(4096 + 24, 2000, neoData)
            retCode = rcvEomResp()
            if retCode: return (retCode)
            time.sleep(2.1)
            neoData = [0x00, 0x00, 0x00, 0x00, \
                0x00, 0x00, 0x00, 0x00]
            sendNeoCmd(4096 + 24, 2000, neoData)
            retCode = rcvEomResp()
            if retCode: return (retCode)
            #Check if exit is requested
            while kbHit():
                char = getAsynchChar()
                if (char != 'y') and (char != 'Y'):
                    print("\nIncand fade on/off failed.")
                    retCode = 2017
                else:
                    retCode = 0
                exitReq = True
            time.sleep(2.1)
        if retCode: return (retCode)
    return 0

#Test Neopixel commands.
def TestNeopixelCmds():
    print("\nVerify Neopixels display green, red, blue, green, red, blue, green, red.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()

    if (ch != 'y') and (ch != 'Y'):
        print("\nDefault Neopixel state failed.")
        return 2101
    # Verify set color
    neoData = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    sendNeoCmd(0, 0, neoData)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    print("\nVerify Neopixels are all off.")
    print("Press (y) if working, (n) if not working.")
    ch = getChar()
    if (ch != 'y') and (ch != 'Y'):
        print("\nNeopixel set color failed.")
        return 2102
    # Verify fade on/fade off works
    print("\nVerify Neopixel fade on/fade off working.")
    print("Fading to 100% on in 2s, wait .1s, fade to off in 2s.")
    print("Colors are green, red, blue, yellow,")
    print("magenta, cyan, white, 1/2 bright white.")
    print("Press (y) if working, (n) if not working.")
    exitReq = False
    while (not exitReq):
        neoData = [0xff, 0x00, 0x00, 0x00, 0xff, 0x00, \
            0x00, 0x00, 0xff, 0xff, 0xff, 0x00, \
            0x00, 0xff, 0xff, 0xff, 0x00, 0xff, \
            0xff, 0xff, 0xff, 0x80, 0x80, 0x80]
        sendNeoCmd(0, 2000, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(2.1)
        neoData = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
        sendNeoCmd(0, 2000, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nNeopixel fade on/off failed.")
                retCode = 2103
            else:
                retCode = 0
            exitReq = True
        time.sleep(2.1)
    if retCode: return (retCode)

    # Verify offset and multiple fade rates
    print("\nVerify offset and multiple fade rates working.")
    print("Fade green LEDs in 250ms,500ms, 750ms, 1s, 1.25s, 1.5s, 1.75s and 2s")
    print("Fade off at same rates (fastest to slowest)")
    print("Press (y) if working, (n) if not working.")
    numNeopixels = 8
    exitReq = False
    while (not exitReq):
        offset = 0
        fadeTime = 0
        neoData = [0xff]
        for i in range(numNeopixels):
            sendNeoCmd(offset, fadeTime, neoData, False)
            offset = offset + 3
            fadeTime = fadeTime + 250
        sendEom()
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(2.1)
        offset = 0
        fadeTime = 0
        neoData = [0x00]
        for i in range(numNeopixels):
            sendNeoCmd(offset, fadeTime, neoData, False)
            offset = offset + 3
            fadeTime = fadeTime + 250
        sendEom()
        retCode = rcvEomResp()
        if retCode: return (retCode)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nNeopixel offset and multiple fade rates failed.")
                retCode = 2104
            else:
                retCode = 0
            exitReq = True
        time.sleep(2.1)
    if retCode: return (retCode)

    # Verify partial brightness fades
    print("\nVerify partial brightness fades working.")
    print("Fade 0%-25%, 25%-50%, 50%-75%, 75%-100%,")
    print("25%-0%, 50%-25%, 75%-50%, 100%-75% and fade back")
    print("Press (y) if working, (n) if not working.")
    neoData = [0x00, 0x00, 0x00, 0x40, 0x40, 0x40, \
        0x80, 0x80, 0x80, 0xc0, 0xc0, 0xc0, \
        0x40, 0x40, 0x40, 0x80, 0x80, 0x80, \
        0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xff]
    sendNeoCmd(0, 0, neoData)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    exitReq = False
    while (not exitReq):
        neoData = [0x40, 0x40, 0x40, 0x80, 0x80, 0x80, \
            0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xff, \
            0x00, 0x00, 0x00, 0x40, 0x40, 0x40, \
            0x80, 0x80, 0x80, 0xc0, 0xc0, 0xc0]
        sendNeoCmd(0, 2000, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        time.sleep(2.1)
        neoData = [0x00, 0x00, 0x00, 0x40, 0x40, 0x40, \
            0x80, 0x80, 0x80, 0xc0, 0xc0, 0xc0, \
            0x40, 0x40, 0x40, 0x80, 0x80, 0x80, \
            0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xff]
        sendNeoCmd(0, 2000, neoData)
        retCode = rcvEomResp()
        if retCode: return (retCode)
        #Check if exit is requested
        while kbHit():
            char = getAsynchChar()
            if (char != 'y') and (char != 'Y'):
                print("\nNeopixel fade on/off failed.")
                retCode = 2105
            else:
                retCode = 0
            exitReq = True
        time.sleep(2.1)
    if retCode: return (retCode)

    # Turn all LEDs off
    neoData = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    sendNeoCmd(0, 0, neoData)
    retCode = rcvEomResp()
    if retCode: return (retCode)
    return 0

#send input cfg cmd
def sendInpCfgCmd(cfgNum):
    global ser
    global numGen2Brd
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.CFG_INP_CMD)
    for loop in range(rs232BIntf.NUM_G2_INP_PER_BRD):
        if loadCfg:
            cmdArr.append(cfgFile.inpCfg[cfgNum][loop])
        else:
            cmdArr.append(inpCfg[cfgNum][loop])
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#rcv end of message resp
def sendEom():
    cmdArr = []
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#rcv end of message resp
def rcvEomResp():
    data = getSerialData();
    if (data[0] != rs232BIntf.EOM_CMD):
        return (800)
    return (0)

#send read input board
def sendReadInpBrdCmd():
    global ser
    global numGen2Brd
    global gen2AddrArr
    sendStd4ByteCmd(rs232BIntf.READ_GEN2_INP_CMD)
    return (0)

#rcv read input cmd
def rcvReadInpResp():
    global ser
    global numGen2Brd
    global gen2AddrArr
    global currInpData
    data = getSerialData();
    if (data[0] != gen2AddrArr[0]):
        print("\nData = {0}, expected = {1}".format(data[0],gen2AddrArr[0]))
        print(repr(data))
        return (1000)
    if (data[1] != rs232BIntf.READ_GEN2_INP_CMD):
        print("\nData = {0}, expected = {1}".format(data[1],rs232BIntf.READ_GEN2_INP_CMD))
        print(repr(data))
        return (1001)
    tmpData = [ data[0], data[1], data[2], data[3], data[4], data[5] ]
    crc8 = calcCrc8(tmpData)
    if (data[6] != crc8):
        print("\nBad CRC, Data = {0}, expected = {1}".format(data[6],crc8))
        return (1002)
    if (data[7] != rs232BIntf.EOM_CMD):
        print("\nData = {0}, expected = {1}".format(data[7],rs232BIntf.EOM_CMD))
        return (1003)
    currInpData[0] = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5]
    return (0)

#send sol cfg cmd
def sendSolCfgCmd(cfgNum):
    global ser
    global numGen2Brd
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.CFG_SOL_CMD)
    for loop in range(rs232BIntf.NUM_G2_SOL_PER_BRD):
        cmdArr.append(solCfg[cfgNum][loop * 3])
        cmdArr.append(solCfg[cfgNum][(loop * 3) + 1])
        cmdArr.append(solCfg[cfgNum][(loop * 3) + 2])
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send read wing cfg board
def sendReadWingCfgCmd():
    sendStd4ByteCmd(rs232BIntf.GET_GEN2_CFG)
    return (0)

#rcv read wing cfg resp
def rcvReadWingCfgResp():
    global ser
    global gen2AddrArr
    global currWingCfg
    global hasMatrix
    data = getSerialData();
    if (data[0] != gen2AddrArr[0]):
        print ("\nData = {0}, expected = {1}".format(data[0],gen2AddrArr[0]))
        print (repr(data))
        return (700)
    if (data[1] != rs232BIntf.GET_GEN2_CFG):
        print ("\nData = {0}, expected = {1}".format(data[1],rs232BIntf.GET_GEN2_CFG))
        print (repr(data))
        return (701)
    tmpData = [ data[0], data[1], data[2], data[3], data[4], data[5] ]
    crc8 = calcCrc8(tmpData)
    if (data[6] != crc8):
        print ("\nBad CRC, Data = {0}, expected = {1}".format(data[6],crc8))
        return (702)
    if (data[7] != rs232BIntf.EOM_CMD):
        print ("\nData = {0}, expected = {1}".format(data[7],rs232BIntf.EOM_CMD))
        return (703)
    currWingCfg[0] = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5]
    print ("{0} WingCfg = {1}".format("0x%02x" % gen2AddrArr[0], "0x%08x" % currWingCfg[0]))
    print ("{}".format("0x%02x" % gen2AddrArr[0]), end=" ")
    for index in range(rs232BIntf.NUM_G2_WING_PER_BRD):
        outStr = "W[%d]:" % index
        if data[index + 2] == rs232BIntf.WING_SOL:
            outStr += "SOL_WING"
        elif data[index + 2] == rs232BIntf.WING_INP:
            outStr += "INP_WING"
        elif data[index + 2] == rs232BIntf.WING_INCAND:
            outStr += "INCAND_WING"
        elif data[index + 2] == rs232BIntf.WING_SW_MATRIX_OUT:
            outStr += "SW_MATRIX_OUT_WING"
        elif data[index + 2] == rs232BIntf.WING_SW_MATRIX_IN:
            outStr += "SW_MATRIX_IN_WING"
            hasMatrix[0] = True
        elif data[index + 2] == rs232BIntf.WING_NEO:
            outStr += "NEO_WING"
        elif data[index + 2] == rs232BIntf.WING_HI_SIDE_INCAND:
            outStr += "INCAND_HI_WING"
        elif data[index + 2] == rs232BIntf.WING_NEO_SOL:
            outStr += "NEO_SOL_WING"
        elif data[index + 2] == rs232BIntf.WING_SPI:
            outStr += "SPI_WING"
        elif data[index + 2] == rs232BIntf.WING_SW_MATRIX_OUT_LOW:
            outStr += "SW_MATRIX_OUT_LOW_WING"
        elif data[index + 2] == rs232BIntf.WING_LAMP_MATRIX_COL:
            outStr += "LAMP_MATRIX_COL_WING"
        elif data[index + 2] == rs232BIntf.WING_LAMP_MATRIX_ROW:
            outStr += "LAMP_MATRIX_ROW_WING"
        else:
            outStr += "Error"
        if index < rs232BIntf.NUM_G2_WING_PER_BRD - 1:
            outStr += ","
            print (outStr, end=" ")
        else:
            print (outStr)
    return (0)

#send wing cfg cmd
def sendWingCfgCmd():
    global ser
    global numGen2Brd
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.SET_GEN2_CFG)
    for loop in range(rs232BIntf.NUM_G2_WING_PER_BRD):
        if loadCfg:
            cmdArr.append(cfgFile.wingCfg[0][loop])
        else:
            cmdArr.append(wingCfg[0][loop])
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send color table cfg cmd
def sendColorCfgCmd():
    global ser
    global numGen2Brd
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.SET_NEO_COLOR_TBL)
    for loop in range((rs232BIntf.NUM_COLOR_TBL * 3) + 1):
        if loadCfg:
            cmdArr.append(cfgFile.colorCfg[0][loop])
        else:
            cmdArr.append(colorCfg[0][loop])
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send kick solenoid cmd
def sendKickSolCmd(solenoids, mask):
    global ser
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.KICK_SOL_CMD)
    cmdArr.append((solenoids >> 8) & 0xff)
    cmdArr.append(solenoids & 0xff)
    cmdArr.append((mask >> 8) & 0xff)
    cmdArr.append(mask & 0xff)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send cfg individual solenoid cmd
def sendCfgIndSolCmd(solIndex, cmd, initKick, offHold):
    global ser
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.CFG_IND_SOL_CMD)
    cmdArr.append(solIndex)
    cmdArr.append(cmd)
    cmdArr.append(initKick)
    cmdArr.append(offHold)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send set solenoid input cmd
def sendSetSolInputCmd(inpIndex, solIndex):
    global ser
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.SET_SOL_INPUT_CMD)
    cmdArr.append(inpIndex)
    cmdArr.append(solIndex)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send solenoid kick PWM cmd
def sendSolKickPwmCmd(pwmVal, solIndex):
    global ser
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.SOL_KICK_PWM)
    cmdArr.append(pwmVal)
    cmdArr.append(solIndex)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send incandescent cmd
def sendIncandCmd(subCmd, mask):
    global ser
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.INCAND_CMD)
    cmdArr.append(subCmd)
    cmdArr.append((mask >> 24) & 0xff)
    cmdArr.append((mask >> 16) & 0xff)
    cmdArr.append((mask >> 8) & 0xff)
    cmdArr.append(mask & 0xff)
    cmdArr.append(calcCrc8(cmdArr))
    cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

#send Neopixel cmd
def sendNeoCmd(offset, fadeTime, data, sendEom=True):
    global ser
    global gen2AddrArr
    cmdArr = []
    cmdArr.append(gen2AddrArr[0])
    cmdArr.append(rs232BIntf.RS232I_NEO_FADE_CMD)
    cmdArr.append((offset >> 8) & 0xff)
    cmdArr.append(offset & 0xff)
    dataLen = len(data)
    cmdArr.append((dataLen >> 8) & 0xff)
    cmdArr.append(dataLen & 0xff)
    cmdArr.append((fadeTime >> 8) & 0xff)
    cmdArr.append(fadeTime & 0xff)
    for tmp in data:
        cmdArr.append(tmp)
    cmdArr.append(calcCrc8(cmdArr))
    if (sendEom):
        cmdArr.append(rs232BIntf.EOM_CMD)
    ser.write(cmdArr)
    return (0)

def endTest(error):
    global ser
    global errMsg
    print("\nError code = {0}".format(error))
    ser.close()
    print("\nPress any key to close window")
    ch = getChar()
    sys.exit(error)
 
#Main code
try:
    # for Windows-based systems
    import msvcrt # If successful, we are on Windows
    windows = True
except ImportError:
    # for POSIX-based systems (with termios & tty support)
    import tty, sys, termios, select  # raises ImportError if unsupported
    windows = False
end = False
skipProg = False
skipSaveStdCfg = False
skipSolTests = False
skipIncandTests = False
skipNeopixelTests = False
skipSerNumTests = False
stm32 = False
for arg in sys.argv:
  if arg.startswith('-port='):
    port = arg.replace('-port=','',1)
  elif arg.startswith('-vers='):
    vers = arg.replace('-vers=','',1)
  elif arg.startswith('-skipProg'):
    skipProg = True
  elif arg.startswith('-skipSaveStdCfg'):
    skipSaveStdCfg = True
  elif arg.startswith('-skipSolTests'):
    skipSolTests = True
  elif arg.startswith('-skipIncandTests'):
    skipIncandTests = True
  elif arg.startswith('-skipSerNumTests'):
    skipSerNumTests = True
  elif arg.startswith('-skipNeopixelTests'):
    skipNeopixelTests = True
  elif arg.startswith('-psoc4200'):
    skipNeopixelTests = True
  elif arg.startswith('-stm32'):
    skipProg = True
    skipSaveStdCfg = True
    stm32 = True
  elif arg.startswith('-?'):
    print("python RegrTestG2.py [OPTIONS]")
    print("    -?                 Options Help")
    print("    -port=portName     COM port number, defaults to COM1")
    print("    -vers=version num  Ex. 0.1.1.0")
    print("    -skipProg          Skip programming (used for debugging tests)")
    print("    -skipSaveStdCfg    Skip saving standard config (used for debugging tests)")
    print("    -skipSolTests      Skip solenoid tests")
    print("    -skipIncandTests   Skip incandescent tests")
    print("    -skipSerNumTests   Skip serial number tests")
    print("    -skipNeopixelTests Skip Neopixel tests")
    print("    -psoc4200          Test PSOC4200")
    print("    -stm32             Test STM32 (skips programming and saving standard config)")
    end = True

if end:
    print("\nPress any key to close window")
    ch = getChar()
    sys.exit(0)
try:
    ser=serial.Serial(port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=.1)
except serial.SerialException:
    print("\nCould not open {0}".format(port))
    print("\nPress any key to close window")
    ch = getChar()
    sys.exit(1)
print("Sending inventory cmd")
sendInvCmd()
rcvInvResp()

# Verify only a single board is attached
if (numGen2Brd != 1):
    print("Only one board should be attached.  Exiting regression tests.")
    sys.exit(2)
    
#Get the current configuration
sendReadWingCfgCmd()
rcvReadWingCfgResp()
    
# Verify the board is configured as Neopixel, solenoid, input and incandescent
if (currWingCfg[0] != 0x06010203):
    print("Regression testing board must have wing board configuration of:")
    print("Neopixel, solenoid, input, incandescent.  Exiting regression tests.")
    sys.exit(3)

# Update code to newest version, also verifies go to bootloader command works
if not skipProg:
    retCode = TestGoBoot()
    if retCode != 0: sys.exit(retCode)

# Verify Get Version command
retCode = TestGetVersion(vers)
if retCode != 0: sys.exit(retCode)

# Verify Get Serial Number command
if not skipSerNumTests:
    retCode = TestGetSerialNum()
    if retCode != 0: sys.exit(retCode)

# Program the standard configuration
# Erases the configuration and stores the standard configuration
# to the card, and resets the card.  (Verification will happen when
# a second configuration is stored and saved.)
if not skipSaveStdCfg:
    retCode = TestStoreStdCfg()
    if retCode != 0: sys.exit(retCode)
else:
    #Reset the board to get back to the stored configuration so
    #rerunning regression tests will work
    if not stm32:
        ResetBoard()

if not skipSolTests:
    retCode = runTest(VerifyStdCfg)
    if retCode != 0: sys.exit(retCode)

    retCode = runTest(TestProcNoAutoClr)
    if retCode != 0: sys.exit(retCode)

    retCode = runTest(TestProcAutoClr)
    if retCode != 0: sys.exit(retCode)

    retCode = TestSolenoidConfig()
    if retCode != 0: sys.exit(retCode)

    retCode = runTest(TestOnOffSolConfig)
    if retCode != 0: sys.exit(retCode)

    retCode = runTest(TestDelaySolConfig)
    if retCode != 0: sys.exit(retCode)

    retCode = TestSolInputConfig()
    if retCode != 0: sys.exit(retCode)

    retCode = TestCancelSolConfig()
    if retCode != 0: sys.exit(retCode)

    retCode = TestSolPwmConfig()
    if retCode != 0: sys.exit(retCode)

if not skipIncandTests:
    retCode = runTest(TestIncandCmds)
    if retCode != 0: sys.exit(retCode)

if not skipNeopixelTests:
    retCode = runTest(TestNeopixelCmds)
    if retCode != 0: sys.exit(retCode)

print("\nSuccessful completion.")
print("\nPress any key to close window")
ch = getChar()
sys.exit(0)
