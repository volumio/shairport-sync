/*
 * sndio output driver. This file is part of Shairport Sync.
 * Copyright (c) 2013 Dimitri Sokolyuk <demon@dim13.org>
 * Copyright (c) 2017 Tobias Kortkamp <t@tobik.me>
 *
 * Modifications for audio synchronisation
 * and related work, copyright (c) Mike Brady 2014 -- 2024
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "audio.h"
#include "common.h"
#include <pthread.h>
#include <sndio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static pthread_mutex_t sndio_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct sio_hdl *hdl;
static int is_running;
static int framesize;
static size_t played;
static size_t written;
uint64_t time_of_last_onmove_cb;
int at_least_one_onmove_cb_seen;

struct sio_par par;

struct sndio_formats {
  const char *name;
  sps_format_t fmt;
  unsigned int rate;
  unsigned int bits;
  unsigned int bps;
  unsigned int sig;
  unsigned int le;
};

static struct sndio_formats formats[] = {{"S8", SPS_FORMAT_S8, 44100, 8, 1, 1, SIO_LE_NATIVE},
                                         {"U8", SPS_FORMAT_U8, 44100, 8, 1, 0, SIO_LE_NATIVE},
                                         {"S16", SPS_FORMAT_S16, 44100, 16, 2, 1, SIO_LE_NATIVE},
                                         {"AUTOMATIC", SPS_FORMAT_S16, 44100, 16, 2, 1,
                                          SIO_LE_NATIVE}, // TODO: make this really automatic?
                                         {"S24", SPS_FORMAT_S24, 44100, 24, 4, 1, SIO_LE_NATIVE},
                                         {"S24_3LE", SPS_FORMAT_S24_3LE, 44100, 24, 3, 1, 1},
                                         {"S24_3BE", SPS_FORMAT_S24_3BE, 44100, 24, 3, 1, 0},
                                         {"S32", SPS_FORMAT_S32, 44100, 32, 4, 1, SIO_LE_NATIVE}};

static void help() {
  printf("    -d output-device    set the output device [default|rsnd/0|rsnd/1...]\n");
}

void onmove_cb(__attribute__((unused)) void *arg, int delta) {
  time_of_last_onmove_cb = get_absolute_time_in_ns();
  at_least_one_onmove_cb_seen = 1;
  played += delta;
}

static int init(int argc, char **argv) {
  int found, opt, round, rate, bufsz;
  unsigned int i;
  const char *devname, *tmp;

  // set up default values first

  sio_initpar(&par);
  par.rate = 44100;
  par.pchan = 2;
  par.bits = 16;
  par.bps = SIO_BPS(par.bits);
  par.le = 1;
  par.sig = 1;
  devname = SIO_DEVANY;

  config.audio_backend_buffer_desired_length = 1.0;
  config.audio_backend_buffer_interpolation_threshold_in_seconds =
      0.25; // below this, soxr interpolation will not occur -- it'll be basic interpolation
            // instead.
  config.audio_backend_latency_offset = 0;

  // get settings from settings file

  // do the "general" audio  options. Note, these options are in the "general" stanza!
  parse_general_audio_options();

  // get the specific settings

  if (config.cfg != NULL) {
    if (!config_lookup_string(config.cfg, "sndio.device", &devname))
      devname = SIO_DEVANY;
    if (config_lookup_int(config.cfg, "sndio.rate", &rate)) {
      if (rate % 44100 == 0 && rate >= 44100 && rate <= 352800) {
        par.rate = rate;
      } else {
        die("sndio: output rate must be a multiple of 44100 and 44100 <= rate <= "
            "352800");
      }
    }
    if (config_lookup_int(config.cfg, "sndio.bufsz", &bufsz)) {
      if (bufsz > 0) {
        par.appbufsz = bufsz;
      } else {
        die("sndio: bufsz must be > 0");
      }
    }
    if (config_lookup_int(config.cfg, "sndio.round", &round)) {
      if (round > 0) {
        par.round = round;
      } else {
        die("sndio: round must be > 0");
      }
    }
    if (config_lookup_string(config.cfg, "sndio.format", &tmp)) {
      for (i = 0, found = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
        if (strcasecmp(formats[i].name, tmp) == 0) {
          config.output_format = formats[i].fmt;
          found = 1;
          break;
        }
      }
      if (!found)
        die("Invalid output format \"%s\". Should be one of: S8, U8, S16, S24, "
            "S24_3LE, S24_3BE, S32, Automatic",
            tmp);
    }
  }
  optind = 1; // optind=0 is equivalent to optind=1 plus special behaviour
  argv--;     // so we shift the arguments to satisfy getopt()
  argc++;
  while ((opt = getopt(argc, argv, "d:")) > 0) {
    switch (opt) {
    case 'd':
      devname = optarg;
      break;
    default:
      help();
      die("Invalid audio option -%c specified", opt);
    }
  }
  if (optind < argc)
    die("Invalid audio argument: %s", argv[optind]);
  pthread_cleanup_debug_mutex_lock(&sndio_mutex, 1000, 1);
  // pthread_mutex_lock(&sndio_mutex);
  debug(1, "sndio: output device name is \"%s\".", devname);
  debug(1, "sndio: rate: %u.", par.rate);
  debug(1, "sndio: bits: %u.", par.bits);

  is_running = 0;
  hdl = sio_open(devname, SIO_PLAY, 0);
  if (!hdl)
    die("sndio: cannot open audio device");

  written = played = 0;
  time_of_last_onmove_cb = 0;
  at_least_one_onmove_cb_seen = 0;

  for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
    if (formats[i].fmt == config.output_format) {
      par.bits = formats[i].bits;
      par.bps = formats[i].bps;
      par.sig = formats[i].sig;
      par.le = formats[i].le;
      break;
    }
  }

  if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par))
    die("sndio: failed to set audio parameters");
  for (i = 0, found = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
    if (formats[i].bits == par.bits && formats[i].bps == par.bps && formats[i].sig == par.sig &&
        formats[i].le == par.le && formats[i].rate == par.rate) {
      config.output_format = formats[i].fmt;
      found = 1;
      break;
    }
  }
  if (!found)
    die("sndio: could not set output device to the required format and rate.");

  framesize = par.bps * par.pchan;
  config.output_rate = par.rate;
  if (par.rate == 0) {
    die("sndio: par.rate set to zero.");
  }

  config.audio_backend_buffer_desired_length = 1.0 * par.bufsz / par.rate;
  config.audio_backend_latency_offset = 0;

  sio_onmove(hdl, onmove_cb, NULL);

  // pthread_mutex_unlock(&sndio_mutex);
  pthread_cleanup_pop(1); // unlock the mutex
  if (framesize == 0) {
    die("sndio: framesize set to zero.");
  }
  return 0;
}

static void deinit() {
  pthread_cleanup_debug_mutex_lock(&sndio_mutex, 1000, 1);
  if (hdl != NULL) {
    if (is_running != 0) {
      sio_flush(hdl);
      is_running = 0;
    }
    sio_close(hdl);
    hdl = NULL;
  }
  pthread_cleanup_pop(1); // unlock the mutex
}

static int play(void *buf, int frames, __attribute__((unused)) int sample_type,
                __attribute__((unused)) uint32_t timestamp,
                __attribute__((unused)) uint64_t playtime) {
  if (frames > 0) {
    pthread_cleanup_debug_mutex_lock(&sndio_mutex, 1000, 1);
    if (is_running == 0) {
      if (hdl != NULL) {
        if (sio_start(hdl) != 1)
          debug(1, "sndio: unable to start");
        is_running = 1;
        written = played = 0;
        time_of_last_onmove_cb = 0;
        at_least_one_onmove_cb_seen = 0;
      } else {
        debug(1, "sndio: output device is not open for play!");
      }
    }
    written += sio_write(hdl, buf, frames * framesize);
    pthread_cleanup_pop(1); // unlock the mutex
  }
  return 0;
}

static void stop() {
  pthread_cleanup_debug_mutex_lock(&sndio_mutex, 1000, 1);
  if (hdl != NULL) {
    if (is_running != 0) {
      if (sio_flush(hdl) != 1)
        debug(1, "sndio: unable to stop");
      written = played = is_running = 0;
    } else {
      debug(1, "sndio: stop: not running.");
    }
  } else {
    debug(1, "sndio: output device is not open for stop!");
  }
  pthread_cleanup_pop(1); // unlock the mutex
}

int get_delay(long *delay) {
  int response = 0;
  size_t estimated_extra_frames_output = 0;
  if (at_least_one_onmove_cb_seen) { // when output starts, the onmove_cb callback will be made
    // calculate the difference in time between now and when the last callback occurred,
    // and use it to estimate the frames that would have been output
    uint64_t time_difference = get_absolute_time_in_ns() - time_of_last_onmove_cb;
    uint64_t frame_difference = (time_difference * par.rate) / 1000000000;
    estimated_extra_frames_output = frame_difference;
    // sanity check -- total estimate can not exceed frames written.
    if ((estimated_extra_frames_output + played) > written / framesize) {
      // debug(1,"play estimate fails sanity check, possibly due to running on a VM");
      estimated_extra_frames_output = 0; // can't make any sensible guess
    }
    // debug(1,"Frames played to last cb: %d, estimated to current time:
    // %d.",played,estimated_extra_frames_output);
  }
  if (delay != NULL)
    *delay = (written / framesize) - (played + estimated_extra_frames_output);
  return response;
}

static int delay(long *delay) {
  int result = 0;
  pthread_cleanup_debug_mutex_lock(&sndio_mutex, 1000, 1);
  if (hdl != NULL) {
    if (is_running != 0) {
      result = get_delay(delay);
    } else {
      debug(1, "sndio: output device is not open for delay!");
      if (delay != NULL)
        *delay = 0;
    }
  } else {
    debug(1, "sndio: output device is not open for delay!");
  }
  pthread_cleanup_pop(1); // unlock the mutex
  return result;
}

static void flush() {
  pthread_cleanup_debug_mutex_lock(&sndio_mutex, 1000, 1);
  if (hdl != NULL) {
    if (is_running != 0) {
      if (sio_flush(hdl) != 1)
        debug(1, "sndio: unable to flush");
      written = played = is_running = 0;
    } else {
      debug(1, "sndio: flush: not running.");
    }
  } else {
    debug(1, "sndio: output device is not open for flush!");
  }
  pthread_cleanup_pop(1); // unlock the mutex
}

audio_output audio_sndio = {.name = "sndio",
                            .help = &help,
                            .init = &init,
                            .deinit = &deinit,
                            .prepare = NULL,
                            .start = NULL,
                            .stop = &stop,
                            .is_running = NULL,
                            .flush = &flush,
                            .delay = &delay,
                            .stats = NULL,
                            .play = &play,
                            .volume = NULL,
                            .parameters = NULL,
                            .mute = NULL};
