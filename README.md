# PSP Sine Demo

## Overview 
A PSP demo using some traditional Amiga influenced demo effects.

- Sine scroller
- Swing logo
- Text block print
- Future composer music

![](https://github.com/optixx/psp-sine-demo/raw/master/screenshots/sine-demo1.png)

[![](https://img.youtube.com/vi/GUyV878LCI4/0.jpg)](https://www.youtube.com/watch?v=GUyV878LCI4)

## Usage
1. Install docker image with psp toolchain
2. Create pspdev-docker.sh script into your path
3. Compile
```
pspdev-docker.sh make clean all 
```
4. Run elf in the emulator 
```
/Applications/PPSSPPSDL.app/Contents/MacOS/PPSSPPSDL sine-deme.elf
```

## Links
* [pspdev-docker](https://github.com/optixx/pspdev-docker) - Bbuild a docker image with the pspdev toolchain
* [ppsspp](https://www.ppsspp.org/) - A PSP emulator
