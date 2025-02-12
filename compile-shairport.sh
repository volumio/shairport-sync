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
    --with-dbus-interface \
    --with-dbus-test-client \
    --with-mpris-interface \
    --with-mpris-test-client \
    --with-libdaemon \
    --sysconfdir=/etc

# Build
make


# Automating checkinstall does not work, proceed with manual entries
sudo checkinstall
# In this case, press 4 and enter 1 as release version

# Create debian package with checkinstall
#sudo checkinstall --pkgname=shairport-sync \
#    --pkgversion="$(date +%Y%m%d)" \
#    --pkgrelease="1" \
#    --backup=no \
#    --deldoc=yes \
#    --fstrans=no \
#    --default \
#    --pakdir="../"

# Clean up build directory
cd ..
echo "DEB package has been created in the parent directory"
ls -l shairport-sync*.deb
