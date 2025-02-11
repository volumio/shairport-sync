#!/bin/bash

# Exit on error
set -e

# Update package lists
sudo apt-get update

# Install build dependencies
sudo apt-get install -y \
    build-essential \
    git \
    autoconf \
    automake \
    libtool \
    libdaemon-dev \
    libasound2-dev \
    libpopt-dev \
    libconfig-dev \
    libsoxr-dev \
    libavahi-client-dev \
    libssl-dev \
    libsndfile1-dev \
    libpulse-dev \
    avahi-daemon \
    checkinstall \
    xxd \
    libplist-dev \
    libsodium-dev \
    libgcrypt-dev \
    libavutil-dev \
    libavcodec-dev \
    libavformat-dev


# Clone Shairport Sync
#git clone https://github.com/mikebrady/shairport-sync.git
cd shairport-sync

# Configure and prepare for build
autoreconf -fi

# Configure with common options
./configure \
    --with-alsa \
    --with-avahi \
    --with-ssl=openssl \
    --with-soxr \
    --with-metadata \
    --with-systemd \
    --with-airplay-2 \
    --with-dbus-interface

# Build
make

# Create debian package with checkinstall
sudo checkinstall --pkgname=shairport-sync \
    --pkgversion="$(date +%Y%m%d)" \
    --backup=no \
    --deldoc=yes \
    --fstrans=no \
    --default \
    --pakdir="../"

# Clean up build directory
cd ..
echo "DEB package has been created in the parent directory"
ls -l shairport-sync*.deb
