# Teensy Stuff

An accumulation of headers, libraries, make files, and so on for writing software for the Teensy 3.2 microcontroller dev board.

## Prerequisites

libusb-dev is needed by `third_party/teensy_loader_cli`:
```
sudo apt-get install libusb-dev
```

**[VERIFY]** User must be in the dialout group:
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

ARM toolchain should be added to the path:
```
echo export PATH=/opt/gcc-arm-none-eabi-5_2-2015q4/bin:$PATH >> ~/.bashrc
source ~/.bashrc
```

## Included Software

`third_party/teensy_loader_cli`, the command line downloader for hex files, comes from https://github.com/PaulStoffregen/teensy_loader_cli

`third_party/Teensy3x`, which contains the basis makefile, headers, and libraries needed to build code for the Teensy 3.x without requiring the Arduino development environment, comes from http://www.seanet.com/~karllunt/bareteensy31.html
