import serial
from pynput.keyboard import Key, Listener

PORT = "/dev/ttyUSB0"
BAUDRATE = 9600

the_key = None
stopped = False

def show(key):

    global the_key, stopped

    #print('\nYou Entered {0}'.format( key))
    the_key = key

    if key == Key.delete:
        # Stop listener
        stopped = True
        return False

if __name__ == "__main__":
    listener = Listener(on_press = show)
    listener.start()
    with serial.Serial(PORT, BAUDRATE, timeout=1) as ser:
        while not stopped:
            rd = ser.readall()
            msg = None
            if rd is not None and len(rd) > 0:
                print(f"RX: {rd.decode('ascii')}")
            if the_key is not None:
                try:
                    if the_key.char in ('l', 'L'):
                        msg = b"ohm: TST LIGHT"
                    if the_key.char in ('u', 'U'):
                        msg = b"ohm: TST UNLOCK"
                    if the_key.char in ('?', ',', '<'):
                        msg = b"ohm: TST ?"
                except AttributeError:
                    pass 
                #print(str(the_key))
                the_key = None
            if msg is not None:
                print(f"TX: {msg.decode('ascii')}")
                ser.write(msg)
    listener.join()
