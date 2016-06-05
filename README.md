## AMDCOVC

This is program to control AMD Overdrive settings that is working if graphics card's
are in console-mode. This program has been written to replace `amdconfig` utility
and allow to overclock Radeon GPU's if no running X11 server. An `amdconfig`
utility requires a running X11 server and X11 kept configuration to be working
correctly. Thus, this program ease overclocking of Radeon GPU's under console environment
(in SSH or Linux console). Nevertheless, this program can works under X11 environment.

### WARNING

This program changes graphics card's clocks and voltages. This program can easily
DAMAGE your hardware if it will be used UNCAREFULLY.

Before any setting of AMD Overdrive parameters (clocks, voltages, fan speeds),
please STOP ANY the GPU computations and the GPU renderings.

YOU USE THIS PROGRAM AT YOUR OWN RISK! WE DO NOT GIVE ANY WARRANTY THAT PROGRAM
WILL BE WORKING CORRECTLY IN ANY CASE! Read license before using this program.

### Preliminary requirements

Program to work requires following things:

* C++ environment compliant with C++11 standard (libraries)
* OpenCL environment (to force initializing of devices)
* libadlxx.so library (AMD ADL library)
* pciutils library

To build program you need:

* compiler compliant with C++11 standard
* OpenCL headers
* AMD ADL SDK (in developer.amd.com site)
* pciutils developer package (includes)

### Building program

To build program, just type:

```
make
```

### Invoking program

To run program, just type:

```
./amdcovc
```

This command prints current status of all adapters (graphics cards).

```
./amdcovc -v
```

Prints more informations about graphics cards.

To print help, type:

```
./amdcovc -?
```

To set core clock and memory clock for first adapter, just type:

```
./amdcovc coreclk:0=1000 memclk:0=1200
```

Sets core clock to 1000 MHz and memory clock to 1200 MHz.

### Understanding info printed by program

The AMDCOVC by default prints following informations about graphics card:

```
Adapter 1: Pitcairn PRO [Radeon HD 7850 / R7 265 / R9 270 1024SP]
  Core: 1010 MHz, Mem: 1235 MHz, Vddc: 1.21 V, Load: 98%, Temp: 67 C, Fan: 54%
  Max Ranges: Core: 150 - 1050 MHz, Mem: 75 - 1450 MHz, Vddc: 0.8 - 1.225 V
  PerfLevels: Core: 300 - 1010 MHz, Mem: 150 - 1235 MHz, Vddc: 0.825 - 1.21 V
```

The first line below adapter is current state of graphics card (core clock,
memory clock, voltage, load, temperature and fan speed).

The `PerfLevels` is current performance level settings from lowest to highest.
Ofcourse, the highest performance levels is used while some computations/rendering.
First level will be used in idle mode (if no work).

The `Max Ranges` is minimal and maximal possible settings for graphics card.

The verbose informations contains:

* current state of graphics (Current CoreClock,...)
* minimal and maximal fan speed settings
* minimal and maximal clocks and voltages
* current performance level settings
* default performance level settings

### List of parameters

List of parameters that can be set:

* coreclk[:[ADAPTER][:LEVEL]]=CLOCK - set core clock in MHz
* memclk[:[ADAPTER][:LEVEL]]=CLOCK - set memory clock in MHz
* vcore[:[ADAPTER][:LEVEL]]=VOLTAGE - set Vddc voltage in Volts
* icoreclk[:ADAPTER]=CLOCK - set core clock in MHz for idle level
* imemclk[:ADAPTER]=CLOCK - set memory clock in MHz for idle level
* ivcore[:ADAPTER]=VOLTAGE - set Vddc voltage  in Volts for idle level
* fanspeed[:[ADAPTER][:THID]]=PERCENT -  set fanspeed in percents

Extra specifiers in parameters:

* ADAPTER - adapter (device) index (default is 0)
* LEVEL - performance level (typically 0 or 1, default is last)
* THID - thermal controller index (must be 0)

You can use 'default' in value place to set default value.
For fanspeed 'default' value force automatic speed setup.

### List of options

List of options:

* -a, --adapters=LIST - print informations only for these adapters
* -v, --verbose - print verbose informations
* --version - print version
* -?, --help - print help