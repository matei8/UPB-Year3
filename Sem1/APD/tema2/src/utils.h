#include <stdio.h>

#ifndef TEMA2_UTILS_H
#define TEMA2_UTILS_H

#define MSG_INIT 1
#define MSG_REQUEST_SWARM 2
#define MSG_SEND_CHUNK 5
#define MSG_UPDATE_SWARM 6
#define MSG_COMPLETED 7

#define MSG_GET_SWARM 9
#define MSG_REQUEST_PEERS 10
#define MSG_SEND_PEERS 11
#define MSG_TO_TRACKER 12

#define MAX_FILES 10
#define MAX_FILENAME 20
#define HASH_SIZE 32
#define MAX_CHUNKS 100
#define MAX_CLIENTS 10

#define TRACKER_RANK 0
#define MAX_MSG_SIZE 1024
#define MSG_REQUEST_CHUNK_TYPE 4
#define MSG_REQUEST_CHUNK_DATA 13

typedef struct {
    char hash[HASH_SIZE + 1];
} Chunk;

typedef struct {
    char filename[MAX_FILENAME];
    int num_chunks;
    Chunk chunks[MAX_CHUNKS];
} File;

typedef struct {
    int rank;
    int num_owned_files;
    File owned_files[MAX_FILES];
    int num_wanted_files;
    char wanted_files[MAX_FILES][MAX_FILENAME];
} PeerData;

typedef struct {
    File file;
    int seeds[MAX_CLIENTS];
    int peers[MAX_CLIENTS];
    int num_peers;
    int num_seeds;
} SwarmInfo;

typedef struct {
    SwarmInfo swarms[MAX_FILES];
    int num_swarms;
    int completed_peers;
} TrackerData;

#endif //TEMA2_UTILS_H
