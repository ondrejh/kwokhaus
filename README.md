# Kwokhaus

Attempt on some eggish automation.

# ToDo

- [ ] remote controlled kwakhaus lock (status, unlock)
- [ ] kwokhaus door motor controll


## Kwakhaus

### Lock

Simple lock controll with serial connection to Meshtastic.

Source directory fw/src_lock

- [x] clear project (remove display and rtc)
- [x] check lock / unlock response
- [x] create unlock trigger (remote only)
- [ ] repeat status message every half an hour

- [ ] connect meshtastic message to home assistant

#### Communication protocol

function | command | answer | example
--- | --- | --- | ---
get status | NAME ? | NAME LODKED/UNLOCKED | RX: ``KWAK ?```
| | | | TX: ```KWAK: LOCKED/UNLOCKED```
unlock | NAME UNLOCK | | RX: ```KWAK UNLOCK```
| | | |
status change | | NAME LOCKED/UNLOCKED | TX: ```KWAK LOCKED/UNLOCKED```

#### Building, flashing

From source directory ```fw``` run ```./rebuild_lock.sh```. Reset the board to bootloader by ```./utils/reset.sh```, flash it by ```./utils/flash.sh```.


### Idea (not to forget)

- rubber duck splash screen

### Siple tasks

- [x] trigger by pushbutton (test)
- [ ] trigger on time
- [ ] send status on idle (every 15 minutes)
- [ ] send status on event
- [ ] draw display function chart
- [x] create and test function to store data in flash (or find lib)
  - improve nvdata.c/h
- [x] parse incoming messages
  - [x] reply on ?
  - [x] T to set time
  - [x] Z to set zone
  - [x] U to set unlock time
- [x] simple pushbutton (any bool input) filtering
- [ ] measure trigger charge voltage

### Hardware needed

- [x] rtc module
- [x] display
- [x] communication port (uart)
- [ ] charge pump
  - to charge trigger from lower voltage (single lipo)

### Functionality and features

- [x] filter incoming messages
- [x] show time
- [x] set time
- [x] set trigger time
- [ ] set time, trigger and zone localy (button, display)
- [ ] trigger on time
- [ ] showing status on display (unlocked, locked, next unlock in ..)
- [x] remote addministration by com. port

### Communication protocol (so far)

function | command | answer | example
--- | --- | --- | ---
get status | NAME ? | NAME LODKED/UNLOCKED Uhh:mm Thh:mm(z) | RX: ```KWAK ?```
| | | | TX: ```KWAK: LOCKED U11:03 T10:08(+2)```
set local time | NAME Thh:mm | NAME: Thh:mm | RX: ```KWAK T12:00```
| | | | TX: ```KWAK: T12:00```
set unlock time (local) | NAME Uhh:mm | NAME: Uhh:mm | RX: ```KWAK U12:01```
| | | | TX: ```KWAK: U12:01```
set zone | NAME Z+/-z | NAME: Z+/-z | RX: ```KWAK Z+1```
| | | | TX: ```KWAK: Z+1```

