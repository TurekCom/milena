#!/bin/bash
sudo apt-get install build-essential sox libsox-fmt-all antiword odt2txt libenca-dev libao-dev lame faac poppler-utils vorbis-tools || exit 1
sudo apt-get install mbrola mbrola-pl1 </dev/null >/dev/null 2>/dev/null

umask 0022

make prefix=/usr distro=debian && sudo make prefix=/usr distro=debian install
echo "Milena została zainstalowana"
