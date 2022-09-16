# esphome-somfy
Control Somfy Blinds with ESPHome


<img alt="Lines of code" src="https://img.shields.io/tokei/lines/github/LeoCal/esphome-somfy">
<img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/LeoCal/esphome-somfy">




This project allows to control Somfy Blinds via ESP microcontroller.
It is forked from the [dmslabsbr/esphome-somfy](https://github.com/dmslabsbr/esphome-somfy) project and it has gone through a massive code restructioning to get it working for my use case.

__Materials:__
* 1 - [Wemos D1](https://s.click.aliexpress.com/e/_d8jADk8);
* 1 - [FS1000A 433,42 Mhz](https://s.click.aliexpress.com/e/_dZrWjOC) (*);
* 1 - 220 ohms resistor (optional, currently disabled in the code);
* 1 - Led, any color (optional, currently disabled in the code);
* 1 - Breadboard (optional);

(*) You need to buy a FS1000A 433,92 Mhz and change the crystal for a [433,42 Mhz](http://rover.ebay.com/rover/1/711-53200-19255-0/1?ff3=4&pub=5575522659&toolid=10001&campid=5338569169&customid=somfy&mpre=https%3A%2F%2Fwww.ebay.com%2Fitm%2F5PCS-433-42M-433-42MHz-R433-F433-SAW-Resonator-Crystals-TO-39-NEW%2F232574365405%3FssPageName%3DSTRK%253AMEBIDX%253AIT%26_trksid%3Dp2057872.m2749.l2649).

__Software:__
* ESPhome component [https://esphome.io/]

Here's the wiring schema:

![scheme](/img/esquema.png)

Here some instructions on how to make an antenna for 433.42Mhz transmission:

![Antenna](/img/Antenna.png)


**1st Prototype**
![1st Prototype](/img/20200402_111304.jpg)



## Install:

1. Copy the files from this repository to the `/config/esphome/` directory.
* RFsomfy.h
* SomfyRts.cpp
* SomfyRts.h

2. Create a new ESPHome device and use `esp_somfy.yaml` from this repository.

3. Customize `RFsomfy.h` file, as you need.

````
#define STATUS_LED_PIN D1
#define REMOTE_TX_PIN D0
#define REMOTE_FIRST_ADDR 0x121311   // <- Change remote name and remote code here!
#define REMOTE_COUNT 5   // <- Number of somfy blinds.
````

4. Customize your ESPHome device, changing `esp_somfy.yaml` as you need.

`````
wifi:
  ssid: "REDACTED"
  password: "REDACTED"
`````
Change here according to the amount of blinds you have.
```
cover:
- platform: custom
  lambda: |-
    std::vector<Cover *> covers;
    auto rfSomfy0 = new RFsomfy(0);
    App.register_component(rfSomfy0);
    auto rfSomfy1 = new RFsomfy(1);
    App.register_component(rfSomfy1);
    auto rfSomfy2 = new RFsomfy(2);
    App.register_component(rfSomfy2);
    auto rfSomfy3 = new RFsomfy(3);
    App.register_component(rfSomfy3);
    auto rfSomfy4 = new RFsomfy(4);
    App.register_component(rfSomfy4);
    covers.push_back(rfSomfy0);
    covers.push_back(rfSomfy1);
    covers.push_back(rfSomfy2);
    covers.push_back(rfSomfy3);
    covers.push_back(rfSomfy4);
    return {covers};

  covers:
    - name: "Blind1"
      device_class: shutter
      id: somfy0
    - name: "Blind2"
      device_class: shutter
      id: somfy1
    - name: "Blind3"
      device_class: shutter
      id: somfy2
    - name: "Blind4"
      device_class: shutter
      id: somfy3
    - name: "Blind5"
      device_class: shutter
      id: somfy4
```

**ATTENTION**

You do not need to use this line `rfSomfy0->set_code(1);` , only if you need to manually set the first code.

5. Compile and Upload customized ESPhome to your device.

## How to configure

1. Once connected to wifi, the device can be operated via its dedicated web page at device_ip:80.
   The device will also expose APIs in case you want to integrated in your home automation platform of choice.

## How to program

1. For programming, place your ESPHome device as close as possible to the blind to be controlled.

   Once programmed, you can place it further away, in a position that is close to all the blinds to be controlled.

2. Choose one of the entities and open it in full mode.

3. Put your blind in programming mode. If necessary, consult the blind manual or the manufacturer.

4. Slide the bar that controls the tilt position to the value 11.

   a) This causes your ESP device to enter programming mode. As if it were an additional remote control.
  
   b) If the programming without problems, your blind will move immediately.

   c) In case of problems, check your device's log.

## Blind configuration commands

Some commands were created, accessed by tilting the blind to try to facilitate debugging and configuration.

```
// cmd 11 - program mode
// cmd 16 - program mode for grail devices
// cmd 21 - delete rolling code file
// cmd 41 - List files
// cmd 51 - Test filesystem.
// cmd 61 - Format filesystem and test.
// cmd 71 - Show actual rolling code
// cmd 81 - Get all rolling code
// cmd 85 - Write new rolling codes
```
