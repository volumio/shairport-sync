/*
 * libao output driver. This file is part of Shairport.
 * Copyright (c) James Laird 2013
 * Copyright (c) Mike Brady 2014 -- 2022
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "audio.h"
#include "common.h"
#include <ao/ao.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>

ao_device *dev = NULL;
ao_option *ao_opts = NULL;
ao_sample_format fmt;
int driver = 0;

static void help(void) {
  printf("    -d driver           set the output driver\n"
         "    -o name=value       set an arbitrary ao option\n"
         "    -i id               shorthand for -o id=<id>\n"
         "    -n name             shorthand for -o dev=<name> -o dsp=<name>\n");
  // get a list of drivers available
  ao_initialize();
  int defaultDriver = ao_default_driver_id();
  if (defaultDriver == -1) {
    printf("    No usable drivers available.\n");
  } else {
    ao_info *defaultDriverInfo = ao_driver_info(defaultDriver);
    int driver_count;
    ao_info **driver_list = ao_driver_info_list(&driver_count);
    int i = 0;
    if (driver_count == 0) {
      printf("    Driver list unavailable.\n");
    } else {
      printf("    Drivers:\n");
      for (i = 0; i < driver_count; i++) {
        ao_info *the_driver = driver_list[i];
        if (strcmp(the_driver->short_name, defaultDriverInfo->short_name) == 0)
          printf("        \"%s\" (default)\n", the_driver->short_name);
        else
          printf("        \"%s\"\n", the_driver->short_name);
      }
    }
  }
  ao_shutdown();
}

static int init(int argc, char **argv) {
  int oldState;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState); // make this un-cancellable
  ao_initialize();
  driver = ao_default_driver_id();
  if (driver == -1) {
    warn("libao can not find a usable driver!");
  } else {

    // set up default values first

    config.audio_backend_buffer_desired_length = 1.0;
    config.audio_backend_latency_offset = 0;

    // get settings from settings file first, allow them to be overridden by
    // command line options

    // do the "general" audio  options. Note, these options are in the "general" stanza!
    parse_general_audio_options();

    optind = 1; // optind=0 is equivalent to optind=1 plus special behaviour
    argv--;     // so we shift the arguments to satisfy getopt()
    argc++;

    // some platforms apparently require optreset = 1; - which?
    int opt;
    char *mid;
    while ((opt = getopt(argc, argv, "d:i:n:o:")) > 0) {
      switch (opt) {
      case 'd':
        driver = ao_driver_id(optarg);
        if (driver < 0)
          die("could not find ao driver %s", optarg);
        break;
      case 'i':
        ao_append_option(&ao_opts, "id", optarg);
        break;
      case 'n':
        ao_append_option(&ao_opts, "dev", optarg);
        // Old libao versions (for example, 0.8.8) only support
        // "dsp" instead of "dev".
        ao_append_option(&ao_opts, "dsp", optarg);
        break;
      case 'o':
        mid = strchr(optarg, '=');
        if (!mid)
          die("Expected an = in audio option %s", optarg);
        *mid = 0;
        ao_append_option(&ao_opts, optarg, mid + 1);
        break;
      default:
        help();
        die("Invalid audio option -%c specified", opt);
      }
    }

    if (optind < argc)
      die("Invalid audio argument: %s", argv[optind]);

    memset(&fmt, 0, sizeof(fmt));

    fmt.bits = 16;
    fmt.rate = 44100;
    fmt.channels = 2;
    fmt.byte_format = AO_FMT_NATIVE;
    fmt.matrix = strdup("L,R");
  }
  pthread_setcancelstate(oldState, NULL);
  return 0;
}

static void deinit(void) {
  int oldState;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState); // make this un-cancellable
  if (dev != NULL)
    ao_close(dev);
  dev = NULL;
  ao_shutdown();
  pthread_setcancelstate(oldState, NULL);
}

static void start(__attribute__((unused)) int sample_rate,
                  __attribute__((unused)) int sample_format) {
  // debug(1,"libao start");
}

static int play(void *buf, int samples, __attribute__((unused)) int sample_type,
                __attribute__((unused)) uint32_t timestamp,
                __attribute__((unused)) uint64_t playtime) {
  int response = 0;
  int oldState;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState); // make this un-cancellable
  if (driver != -1) {
    if (dev == NULL)
      dev = ao_open_live(driver, &fmt, ao_opts);
    if (dev != NULL)
      response = ao_play(dev, buf, samples * 4);
  }
  pthread_setcancelstate(oldState, NULL);
  return response;
}

static void stop(void) {
  // debug(1,"libao stop");
  int oldState;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState); // make this un-cancellable
  if (dev != NULL) {
    ao_close(dev);
    dev = NULL;
  }
  pthread_setcancelstate(oldState, NULL);
}

audio_output audio_ao = {.name = "ao",
                         .help = &help,
                         .init = &init,
                         .deinit = &deinit,
                         .prepare = NULL,
                         .start = &start,
                         .stop = &stop,
                         .is_running = NULL,
                         .flush = NULL,
                         .delay = NULL,
                         .stats = NULL,
                         .play = &play,
                         .volume = NULL,
                         .parameters = NULL,
                         .mute = NULL};
