#ifndef _RTSP_H
#define _RTSP_H

#include "player.h"

extern pthread_rwlock_t principal_conn_lock;
extern rtsp_conn_info *principal_conn;
extern rtsp_conn_info **conns;

void *rtsp_listen_loop(__attribute((unused)) void *arg);

void lock_player();
void unlock_player();

// this can be used to forcibly stop a play session
int get_play_lock(rtsp_conn_info *conn, int allow_session_interruption);

// initialise and completely delete the metadata stuff

void metadata_init(void);
void metadata_stop(void);

// sends metadata out to the metadata pipe, if enabled.
// It is sent with the type 'ssnc' the given code, data and length
// The handler at the other end must know what to do with the data
// e.g. if it's malloced, to free it, etc.
// nothing is done automatically

int send_ssnc_metadata(uint32_t code, char *data, uint32_t length, int block);

#endif // _RTSP_H
