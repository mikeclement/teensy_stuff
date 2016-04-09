# Teensy Stuff

An accumulation of headers, libraries, make files, and so on for writing software for the Teensy 3.2 microcontroller dev board *without* requiring a Teensyduino dev environment.

## Prerequisites

libusb-dev is needed by `third_party/teensy_loader_cli`, and libc6-i386 is needed by the ARM toolchain:
```
sudo apt-get install libusb-dev libc6-i386
```

**[VERIFY IF NEEDED]** User must be in the dialout group:
```
sudo adduser $USER dialout
```

Teensy requires a udev rule to be writable:
```
cat <<EOF | sudo tee /etc/udev/rules.d/49-teensy.rules > /dev/null
ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789]?", ENV{ID_MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789]?", ENV{MTP_NO_PROBE}="1"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789]?", MODE:="0666"
KERNEL=="ttyACM*", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789]?", MODE:="0666"
EOF
```

ARM toolchain is needed for building hex files (if not already installed):
```
pushd /opt
ARM_TARBALL=gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2
sudo wget https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/$ARM_TARBALL
sudo tar jxf $ARM_TARBALL
sudo rm $ARM_TARBALL
popd
```

## Setting up and building a project

The folder `projects/blinky` provides a minimal example, copied and derived from `third_party/Teensy3x`. An includeable makefile lives in `mk`, making it easy to write a `Makefile` for each project. Simply specify the name of the project (which will also be the basename of the hex file generated) and project-specific objects (some standard objects are included automatically), then include the base makefile:

```
PROJECT = blinky
OBJECTS = blinky.o

include ../../mk/makefile.inc
```

The `all` target builds the hex, bin, and other key files. The `upload` target additionally builds `teensy_loader_cli` and uses it to upload the built hex file to a board:

```
cd projects/blinky
make upload
```

## Included software

`third_party/Teensy3x`, which contains the basis for the makefiles, headers, and libraries used here, comes from http://www.seanet.com/~karllunt/bareteensy31.html

`third_party/teensy-oscilloscope`, which contains bare metal code for initializing the USB interface, comes from https://github.com/kcuzner/teensy-oscilloscope/ (and more detail is available here: http://kevincuzner.com/2014/12/12/teensy-3-1-bare-metal-writing-a-usb-driver/)

`third_party/teensy_loader_cli`, which contains the command line downloader for hex files, comes from https://github.com/PaulStoffregen/teensy_loader_cli

`third_party/usb_debug_only`, which contains some *Arduino* example code for using the USB port, comes from https://www.prjc.com/teensy/usb_debug_only.html
