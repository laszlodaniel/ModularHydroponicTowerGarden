# ModularHydroponicTowerGarden
A modular 3D-printable hydroponic tower garden with Arduino compatible water pump controller.

**3DPrint** ~~folder contains the STL-files to print tower parts.~~ [STL-files are hosted on Thingiverse](https://www.thingiverse.com/thing:3405964)

**Arduino** folder contains the source code and compiled binaries for the water pump controller written in Arduino IDE 1.8.12.

**GUI** folder contains the C# source code and compiled Windows binary for an external computer written in Visual Studio 2019.

**PCB** folder contains everything to manufacture and assemble the hardware. You can buy it on [Tindie.](https://www.tindie.com/products/19512/)

## Command Line Interface (CLI)

The water pump controller can be interfaced with the Arduino Serial Monitor, or any other Serial Monitor application, regardless of OS platform, to change its settings via text-based commands. The current GUI works on Windows OS only.

By default the Serial Monitor shows regular status updates:

```
...
-----------STATUS-----------
Service mode : disabled
  Water pump : on @ 80%
       Timer : 00:04:46 left
Temperatures
    external : 0.0°C
    internal : 0.0°C
----------------------------

-----------STATUS-----------
Service mode : disabled
  Water pump : on @ 80%
       Timer : 00:04:45 left
Temperatures
    external : 0.0°C
    internal : 0.0°C
----------------------------

-----------STATUS-----------
Service mode : disabled
  Water pump : on @ 80%
       Timer : 00:04:44 left
Temperatures
    external : 0.0°C
    internal : 0.0°C
----------------------------
...
```

To turn off regular status update display send this command:

```
status=0
```

The Serial Monitor reacts the following way to user inputs:

```
Input: status=0
Command: status
Value: 0
Result: regular status updates disabled.
```

With the command "help" a complete description is printed to the Serial Monitor on the available commands:

```
----------------------------------HELP---------------------------------
         help : print this description.
      restart : restart controller.
         info : print controller information.
servicemode=X : set service mode,
                X=0: disabled (default),
                X=1: enabled.
     status=X : print regular status updates,
                X=0: disabled,
                X=1: enabled (default).
  pumpspeed=X : set water pump speed in percents,
                X=0...100 [%].
     ontime=X : set water pump on-time in minutes,
                X=0...71582 [min],
                when set to 0 the water pump is permanently turned off.
    offtime=X : set water pump off-time in minutes,
                X=0...71582 [min],
                when set to 0 the water pump is permanently turned on.
tempsensors=X : set temperature sensor configuration,
                X=0: none (default),
                X=1: external,
                X=2: internal,
                X=3: both external and internal.
   tempunit=X : set temperature unit,
                X=1: Celsius (default),
                X=2: Fahrenheit,
                X=4: Kelvin.
    ntcbeta=X : set temperature sensor beta value in Kelvin,
                X=3950 [K] by default.
    pwmfreq=X : set water pump driver PWM frequency in Hertz,
                X=490 [Hz] by default.

Note: settings are automatically saved to the internal EEPROM.
-----------------------------------------------------------------------
```

To display controller information send the "info" command:

```
--------------------INFO---------------------
        AVR signature : 1E 95 0F (ATmega328P)
     Hardware version : V1.01
        Hardware date : 2019.11.18 18:07:36
        Assembly date : 2020.03.09 13:20:28
        Firmware date : 2021.04.20 14:02:43
         Service mode : disabled
     Water pump state : off
   Water pump on-time : 5 minute(s)
  Water pump off-time : 60 minute(s)
Temperature sensor(s) : external and internal
     Temperature unit : Celsius
       NTC beta value : 3950 K
        PWM frequency : 490 Hz
---------------------------------------------
```

## Pictures

![MHTG empty](https://thingiverse-production-new.s3.amazonaws.com/assets/93/72/7a/2c/28/IMG_20190417_153821_02.jpg)

![MHTG seedlings](https://thingiverse-production-new.s3.amazonaws.com/assets/ca/d1/2b/71/79/IMG_20190503_190128_02.jpg)

![MHTG halfway through](https://thingiverse-production-new.s3.amazonaws.com/assets/ac/ff/6e/5a/bc/IMG_20190516_110757.jpg)

![MHTG CAD drawing](https://thingiverse-production-new.s3.amazonaws.com/assets/13/7a/7d/81/b4/MHTG_V113_03.png)

![MHTG planting module](https://thingiverse-production-new.s3.amazonaws.com/assets/95/2c/2f/54/be/IMG_20190401_095039_02.jpg)

![V1.02 water pump controller](https://cdn.thingiverse.com/assets/e1/6c/c4/92/b1/IMG_20200112_160445_02.jpg)

![MHTG GUI 01](https://cdn.thingiverse.com/assets/28/b2/d1/ac/16/MHTG_GUI_01.png)