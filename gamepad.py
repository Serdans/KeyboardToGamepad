from evdev import InputDevice, ecodes, categorize

dev = InputDevice('/dev/input/event4')

for event in dev.read_loop():
    if event.type == ecodes.EV_KEY:
        print(categorize(event))