ARG SHAIRPORT_SYNC_BRANCH=.

FROM alpine:3.20 AS builder

# Classic (aka AirPlay 1) Build

# Check required arguments exist. These will be provided by the Github Action
# Workflow and are required to ensure the correct branches are being used.
ARG SHAIRPORT_SYNC_BRANCH
RUN test -n "$SHAIRPORT_SYNC_BRANCH"

RUN apk -U add \
        alsa-lib-dev \
        autoconf \
        automake \
        avahi-dev \
        build-base \
        dbus \
        git \
        libconfig-dev \
        libsndfile-dev \
        libtool \
        openssl-dev \
        mosquitto-dev \
        popt-dev \
        pulseaudio-dev \
        soxr-dev

##### ALAC #####
RUN git clone https://github.com/mikebrady/alac
WORKDIR /alac
RUN autoreconf -i
RUN ./configure
RUN make -j $(nproc)
RUN make install
WORKDIR /
##### ALAC END #####

##### SPS #####
WORKDIR /shairport-sync
COPY . .
RUN git checkout "$SHAIRPORT_SYNC_BRANCH"
WORKDIR /shairport-sync/build
RUN autoreconf -i ../
RUN CFLAGS="-O3" CXXFLAGS="-O3" ../configure --sysconfdir=/etc --with-alsa --with-pa --with-soxr --with-avahi --with-ssl=openssl \
        --with-metadata --with-dummy --with-pipe --with-dbus-interface \
        --with-stdout --with-mpris-interface --with-mqtt-client \
        --with-apple-alac --with-convolution
RUN make -j $(nproc)
RUN DESTDIR=install make install
WORKDIR /
##### SPS END #####

# Shairport Sync Runtime System
FROM crazymax/alpine-s6:3.20-3.2.0.2

ENV S6_CMD_WAIT_FOR_SERVICES=1
ENV S6_CMD_WAIT_FOR_SERVICES_MAXTIME=0

RUN apk -U add \
        alsa-lib \
        avahi \
        avahi-tools \
        dbus \
        glib \
        less \
        less-doc \
        libconfig \
        libsndfile-dev \
        libpulse \
        man-pages \
        mandoc \
        libcrypto3 \
        mosquitto \
        popt \
        soxr \
        curl

# Copy build files.
COPY --from=builder /shairport-sync/build/install/usr/local/bin/shairport-sync /usr/local/bin/shairport-sync
COPY --from=builder /shairport-sync/build/install/usr/local/share/man/man1 /usr/share/man/man1
COPY --from=builder /usr/local/lib/libalac.* /usr/local/lib/
COPY --from=builder /shairport-sync/build/install/etc/shairport-sync.conf /etc/
COPY --from=builder /shairport-sync/build/install/etc/shairport-sync.conf.sample /etc/
COPY --from=builder /shairport-sync/build/install/etc/dbus-1/system.d/shairport-sync-dbus.conf /etc/dbus-1/system.d/
COPY --from=builder /shairport-sync/build/install/etc/dbus-1/system.d/shairport-sync-mpris.conf /etc/dbus-1/system.d/

COPY ./docker/classic/etc/s6-overlay/s6-rc.d /etc/s6-overlay/s6-rc.d
RUN chmod +x /etc/s6-overlay/s6-rc.d/01-startup/script.sh

# Create non-root user for running the container -- running as the user 'shairport-sync' also allows
# Shairport Sync to provide the D-Bus and MPRIS interfaces within the container

RUN addgroup shairport-sync
RUN adduser -D shairport-sync -G shairport-sync

# Add the shairport-sync user to the pre-existing audio group, which has ID 29, for access to the ALSA stuff
RUN addgroup -g 29 docker_audio && addgroup shairport-sync docker_audio && addgroup shairport-sync audio

# Remove anything we don't need.
RUN rm -rf /lib/apk/db/*

# Remove any statically-defined Avahi services, e.g. SSH and SFTP
RUN rm -rf /etc/avahi/services/*.service

# Add run script that will start SPS
COPY ./docker/run.sh ./run.sh
RUN chmod +x /run.sh

# D-Bus might need this directory
RUN mkdir -p /run/dbus

ENTRYPOINT ["/init","./run.sh"]
