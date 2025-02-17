/*
 * This file is part of Shairport Sync.
 * Copyright (c) Mike Brady 2020 -- 2023
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
#ifndef __PTP_UTILITIES_H
#define __PTP_UTILITIES_H

#include "config.h"
#include "nqptp-shm-structures.h"
#include <stdint.h>

int ptp_get_clock_info(uint64_t *actual_clock_id, uint64_t *time_of_sample, uint64_t *raw_offset,
                       uint64_t *mastership_start_time);

void ptp_send_control_message_string(const char *msg);

void ptp_shm_interface_init();
int ptp_shm_interface_open();
int ptp_get_clock_version();
int ptp_shm_interface_close();

#endif /* __PTP_UTILITIES_H */
