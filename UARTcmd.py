import serial

def sendCMD(cmd:str):
    with serial.Serial(/dev/ttyAMA0, 115200, timeout=1) as myser:
        myser.write(cmd.encode())
        asw = myser.read(20)
        return asw