#!/bin/bash -e
sudo apt install build-essential mbrola mbrola-pl1 alsa-utils
make -f Makefile_rpi
umask 0022
sudo make -f Makefile_rpi install
echo "Milena została zainstalowana"
