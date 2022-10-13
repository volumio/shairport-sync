#!/bin/bash

#Shairport-sync Volumio compile script 

## FIRST STEP, DOWNLOAD THE APPROPRIATE VERSION OF CHECKINSTALL FROM 
# https://packages.debian.org/buster-backports/armhf/checkinstall/download

echo "Installing dependencies"

sudo apt-get update
apt-get install -y --no-install-recommends build-essential git xmltoman autoconf automake libtool libpopt-dev libconfig-dev libasound2-dev avahi-daemon libavahi-client-dev libssl-dev libsoxr-dev
apt-get install -y --no-install-recommends libplist-dev libsodium-dev libavutil-dev libavcodec-dev libavformat-dev uuid-dev libgcrypt-dev xxd

echo "Installing NQPTP"

cd ..
git clone https://github.com/mikebrady/nqptp.git
cd nqptp
autoreconf -fi
./configure --with-systemd-startup
make
checkinstall

echo "Compiling Shairport-Sync"
cd ..
cd shairport-sync
autoreconf -fi
./configure --sysconfdir=/etc --with-alsa --with-soxr --with-avahi --with-ssl=openssl --with-systemd --with-airplay-2 --with-metadata
make
checkinstall

echo "Done"
