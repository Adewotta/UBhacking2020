import serial
from pynput import keyboard

key_w = keyboard.KeyCode(char='w')
key_a = keyboard.KeyCode(char='a')
key_s = keyboard.KeyCode(char='s')
key_d = keyboard.KeyCode(char='d')
key_z = keyboard.KeyCode(char='z')
key_x = keyboard.KeyCode(char='x')

controllerDict = {
    key_w: 0,
    key_a: 0,
    key_s: 0,
    key_d: 0,
    key_z: 0,
    key_x: 0
}

def on_press(key):
    if key == key_x:
        controllerDict[key] |= 0b1;

    elif key == key_z:
        controllerDict[key] |= 0b10;

    else:
        controllerDict[key] = 80

def on_release(key):
    if key == key_x:
        controllerDict[key] ^= 0b1;

    elif key == key_z:
        controllerDict[key] ^= 0b10;

    else:
        controllerDict[key] = 0

    if key == keyboard.Key.esc:
        sys.exit(0)

listener = keyboard.Listener(
    on_press=on_press,
    on_release=on_release)

listener.start()

# These values should be further refined.
ser = serial.Serial("COM3", 1000000)
ser.timeout = 0.25
ser.write_timeout = 1;
print("Using serial port: {0}".format(ser.name))

while True:
    nextByte = ser.read()
    if len(nextByte) == 0:
        print("Read timed out!")
        continue

    if ord(nextByte) == 1: # DATA_REQUEST
        print("Received data request; sending values.")
        try:
            ser.write(bytes([0 + controllerDict[key_x] + controllerDict[key_z]])) #byte 0
            ser.write(bytes([128])) #byte 1
            ser.write(bytes([128 + controllerDict[key_d] - controllerDict[key_a]])) #byte 2
            ser.write(bytes([128 + controllerDict[key_w] - controllerDict[key_s]])) #byte 3
            ser.write(bytes([128])) #byte 4
            ser.write(bytes([128])) #byte 5
            ser.write(bytes([128])) #byte 6
            ser.write(bytes([128])) #byte 7
            ser.flush()

            print("Finished sending controller data.")

        except:
            print("ser.write() timed out!")

    elif ord(nextByte) == 2: # LOG
        print("[LOG] ", end='')

        while True:
            charRead = ser.read()
            if ord(charRead) == 0:
                break
            else:
                print(charRead.decode("utf-8"), end='')
        print("")

    else:
        print("Received unknown command: {0}".format(nextByte))

ser.close()
