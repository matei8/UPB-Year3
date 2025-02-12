#include <pthread.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "utils.h"

void write_to_files(PeerData peer_data) {
    for (int i = 0; i < peer_data.num_wanted_files; i++) {
        for (int j = 0; j < peer_data.num_owned_files; j++) {
            if (strcmp(peer_data.wanted_files[i], peer_data.owned_files[j].filename) == 0) {
                char outputName[50];
                sprintf(outputName, "client%d_%s", peer_data.rank, peer_data.owned_files[j].filename);
                FILE *output = fopen(outputName, "w");

                if (!output) {
                    printf("Error: Output file %s not found for rank %d\n", outputName, peer_data.rank);
                    exit(-1);
                }

                for (int k = 0; k < peer_data.owned_files[j].num_chunks; k++) {
                    fprintf(output, "%s\n", peer_data.owned_files[j].chunks[k].hash);
                }

                fclose(output);
            }
        }
    }
}

void update_tracker(PeerData peer_data) {
    MPI_Send("UPDATE", MAX_MSG_SIZE, MPI_CHAR, TRACKER_RANK, MSG_TO_TRACKER, MPI_COMM_WORLD);

    MPI_Send(&peer_data.rank, 1, MPI_INT, TRACKER_RANK, MSG_UPDATE_SWARM, MPI_COMM_WORLD);
    MPI_Send(&peer_data.num_owned_files, 1, MPI_INT, TRACKER_RANK, MSG_UPDATE_SWARM, MPI_COMM_WORLD);

    for (int i = 0; i < peer_data.num_owned_files; i++) {
        MPI_Send(peer_data.owned_files[i].filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_UPDATE_SWARM, MPI_COMM_WORLD);
        MPI_Send(&peer_data.owned_files[i].num_chunks, 1, MPI_INT, TRACKER_RANK, MSG_UPDATE_SWARM, MPI_COMM_WORLD);
    }
}

void send_init_data_to_tracker(PeerData peer_data) {
    MPI_Send(&peer_data.num_owned_files, 1, MPI_INT, TRACKER_RANK, MSG_INIT, MPI_COMM_WORLD);

    for (int i = 0; i < peer_data.num_owned_files; i++) {
        MPI_Send(peer_data.owned_files[i].filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_INIT, MPI_COMM_WORLD);
        MPI_Send(&(peer_data.owned_files[i].num_chunks), 1, MPI_INT, TRACKER_RANK, MSG_INIT, MPI_COMM_WORLD);

        for (int j = 0; j < peer_data.owned_files[i].num_chunks; j++) {
            MPI_Send(&(peer_data.owned_files[i].chunks[j].hash), HASH_SIZE + 1, MPI_CHAR, TRACKER_RANK, MSG_INIT, MPI_COMM_WORLD);
        }
    }
}

TrackerData receive_init_data(TrackerData tracker_data, int numtasks) {
    tracker_data.num_swarms = 0;
    tracker_data.completed_peers = 0;

    for (int rank = 1; rank < numtasks; rank++) {
        PeerData client_data;
        MPI_Status status;

        int num_owned_files;
        MPI_Recv(&num_owned_files, 1, MPI_INT, rank, MSG_INIT, MPI_COMM_WORLD, &status);
        if (rank != status.MPI_SOURCE) {
            printf("Error: Rank mismatch at tracker init\n");
            continue;
        }
        client_data.num_owned_files = num_owned_files;

        client_data.rank = rank;

        for (int i = 0; i < num_owned_files; i++) {
            MPI_Recv(client_data.owned_files[i].filename, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, MSG_INIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&(client_data.owned_files[i].num_chunks), 1, MPI_INT, status.MPI_SOURCE, MSG_INIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Check if a swarm for this file already exists
            bool swarm_exists = false;
            for (int j = 0; j < tracker_data.num_swarms; j++) {
                if (strcmp(tracker_data.swarms[j].file.filename, client_data.owned_files[i].filename) == 0) {
                    swarm_exists = true;

                    // Add the peer to the swarm
                    tracker_data.swarms[j].peers[tracker_data.swarms[j].num_peers] = rank;
                    tracker_data.swarms[j].num_peers++;

                    // Add the chunks to the swarm
                    for (int k = 0; k < client_data.owned_files[i].num_chunks; k++) {
                        MPI_Recv(&(client_data.owned_files[i].chunks[k].hash), HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, MSG_INIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        strcpy(tracker_data.swarms[j].file.chunks[k].hash, client_data.owned_files[i].chunks[k].hash);
                    }

                    tracker_data.swarms[j].file.num_chunks += client_data.owned_files[i].num_chunks;
                    break;
                }
            }

            if (!swarm_exists) {
                // Create a new swarm and add it to the tracker
                SwarmInfo swarm;

                strcpy(swarm.file.filename,client_data.owned_files[i].filename);
                swarm.file.num_chunks = client_data.owned_files[i].num_chunks;
                swarm.num_seeds = 1;
                swarm.seeds[0] = rank;
                swarm.num_peers = 0;

                for (int j = 0; j < client_data.owned_files[i].num_chunks; j++) {
                    MPI_Recv(&(client_data.owned_files[i].chunks[j].hash), HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, MSG_INIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    strcpy(swarm.file.chunks[j].hash, client_data.owned_files[i].chunks[j].hash);
                }

                tracker_data.swarms[tracker_data.num_swarms] = swarm;
                tracker_data.num_swarms++;
            } else {
                continue;
            }
        }

        MPI_Send("Start", 6, MPI_CHAR, rank, MSG_INIT, MPI_COMM_WORLD);
    }

    return tracker_data;
}

PeerData read_peer_data(FILE *f) {
    PeerData peer_data;
    fscanf(f, "%d", &peer_data.num_owned_files);
    for (int i = 0; i < peer_data.num_owned_files; i++) {
        fscanf(f, "%s %d", peer_data.owned_files[i].filename, &peer_data.owned_files[i].num_chunks);
        for (int j = 0; j < peer_data.owned_files[i].num_chunks; j++) {
            fscanf(f, "%s", peer_data.owned_files[i].chunks[j].hash);
        }
    }

    fscanf(f, "%d", &peer_data.num_wanted_files);
    for (int i = 0; i < peer_data.num_wanted_files; i++) {
        fscanf(f, "%s", peer_data.wanted_files[i]);
    }

    return peer_data;
}

void init_peer_data(PeerData *peer_data, int rank) {
    peer_data->num_owned_files = 0;
    peer_data->num_wanted_files = 0;

    char inputName[20];
    sprintf(inputName, "in%d.txt", rank);
    FILE *input = fopen(inputName, "r");

    if (!input) {
        printf("Error: Input file %s not found for rank %d\n", inputName, rank);
        exit(-1);
    }

    *peer_data = read_peer_data(input);
    // Since the values are assigned statically, the rank is not set in the read function so it becomes 0
    peer_data->rank = rank;
    send_init_data_to_tracker(*peer_data);

    char start[6];
    MPI_Recv(start, sizeof(start), MPI_CHAR, TRACKER_RANK, MSG_INIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (strcmp(start, "Start") != 0) {
        printf("Error starting peer %d\n", rank);
        exit(-1);
    }

    fclose(input);
}

bool has_file(PeerData *peer, const char *filename) {
    for (int i = 0; i < peer->num_owned_files; i++) {
        if (strcmp(peer->owned_files[i].filename, filename) == 0) {
            return true;
        }
    }
    return false;
}

int get_file_index(PeerData *peer, const char *filename) {
    for (int i = 0; i < peer->num_owned_files; i++) {
        if (strcmp(peer->owned_files[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

void *download_thread_func(void *arg) {
    PeerData *thisPeer = (PeerData *) arg;

    int completed_files = 0;
    int wanted_files_no = thisPeer->num_wanted_files;
    char wanted_files[MAX_FILES][MAX_FILENAME] = {0};

    for (int i = 0; i < wanted_files_no; i++) {
        strcpy(wanted_files[i], thisPeer->wanted_files[i]);
    }

    MPI_Status status;

    // Receive the swarm info from the tracker
    for (int i = 0; i < wanted_files_no; i++) {
        MPI_Send("REQUEST_SWARM", MAX_MSG_SIZE, MPI_CHAR, TRACKER_RANK, MSG_TO_TRACKER, MPI_COMM_WORLD);
        MPI_Send(wanted_files[i], MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_REQUEST_SWARM, MPI_COMM_WORLD);

        SwarmInfo currentSwarm;
        MPI_Recv(&currentSwarm.file.num_chunks, 1, MPI_INT, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(currentSwarm.file.filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int j = 0; j < currentSwarm.file.num_chunks; j++) {
            MPI_Recv(&(currentSwarm.file.chunks[j].hash), HASH_SIZE + 1, MPI_CHAR, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Recv(&currentSwarm.num_seeds, 1, MPI_INT, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(currentSwarm.seeds, MAX_CLIENTS, MPI_INT, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Recv(&currentSwarm.num_peers, 1, MPI_INT, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(currentSwarm.peers, MAX_CLIENTS, MPI_INT, TRACKER_RANK, MSG_GET_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int downloaded_chunks = 0;

        /** Download chunks either from the seeds or the peers until all chunks are downloaded */

        while(downloaded_chunks < currentSwarm.file.num_chunks) {
            if (currentSwarm.num_peers == 0) {
                /** When there are no peers available, then it means that the file
                is fully owned by the seed of the swarm,
                so for this case we only request the chunks from the seeder. */

                // Get random seed rank from the available seeds
                // Collect seeds that are not equal to thisPeer->rank
                int available_seeds[MAX_CLIENTS];
                int num_available_seeds = 0;
                for (int j = 0; j < currentSwarm.num_seeds; j++) {
                    if (currentSwarm.seeds[j] != thisPeer->rank) {
                        available_seeds[num_available_seeds++] = currentSwarm.seeds[j];
                    }
                }

                srand(time(NULL));
                int seedRank = available_seeds[rand() % num_available_seeds];

                if (seedRank != thisPeer->rank && seedRank != 0) {
                    char chunk_hash[HASH_SIZE + 1];
                    strcpy(chunk_hash, currentSwarm.file.chunks[downloaded_chunks].hash);

                    MPI_Send("REQUEST_CHUNK", MAX_MSG_SIZE, MPI_CHAR, seedRank, MSG_REQUEST_CHUNK_TYPE, MPI_COMM_WORLD);
                    MPI_Send(&chunk_hash, HASH_SIZE + 1, MPI_CHAR, seedRank, MSG_REQUEST_CHUNK_DATA, MPI_COMM_WORLD);

                    char response[MAX_MSG_SIZE];
                    MPI_Recv(response, MAX_MSG_SIZE, MPI_CHAR, seedRank, MSG_SEND_CHUNK, MPI_COMM_WORLD, &status);
                    if (seedRank != status.MPI_SOURCE) {
                        printf("Error: Seed rank mismatch\n");
                        continue;
                    }

                    // After receiving a chunk and verifying the response
                    if (strcmp(response, "OK") == 0) {
                        char hash[HASH_SIZE + 1];
                        strcpy(hash, currentSwarm.file.chunks[downloaded_chunks].hash);

                        if (!has_file(thisPeer, currentSwarm.file.filename)) {
                            // Add new file to owned_files
                            if (thisPeer->num_owned_files < MAX_FILES) {
                                strcpy(thisPeer->owned_files[thisPeer->num_owned_files].filename, currentSwarm.file.filename);
                                thisPeer->owned_files[thisPeer->num_owned_files].num_chunks = 1;
                                strcpy(thisPeer->owned_files[thisPeer->num_owned_files].chunks[0].hash, hash);
                                thisPeer->num_owned_files++;
                            }
                        } else {
                            int file_index = get_file_index(thisPeer, currentSwarm.file.filename);
                            if (file_index != -1 && thisPeer->owned_files[file_index].num_chunks < MAX_CHUNKS) {
                                thisPeer->owned_files[file_index].num_chunks++;
                                strcpy(thisPeer->owned_files[file_index].chunks[thisPeer->owned_files[file_index].num_chunks - 1].hash, hash);
                            }
                        }

                        downloaded_chunks++;
                        if (downloaded_chunks % 10 == 0 && downloaded_chunks != 0) {
                            MPI_Send("REQUEST_PEERS", MAX_MSG_SIZE, MPI_CHAR, TRACKER_RANK, MSG_TO_TRACKER, MPI_COMM_WORLD);
                            MPI_Send(currentSwarm.file.filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_REQUEST_PEERS, MPI_COMM_WORLD);

                            MPI_Recv(&currentSwarm.num_peers, 1, MPI_INT, TRACKER_RANK, MSG_SEND_PEERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            MPI_Recv(currentSwarm.peers, MAX_CLIENTS, MPI_INT, TRACKER_RANK, MSG_SEND_PEERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        }

                        // Update the tracker with the new chunk with "UPDATE" message
                        update_tracker(*thisPeer);
                    }
                }
            } else {
                /** Get random peer rank from the available peers.
                We set the peer rank to -1 in order to check if the client is active as peer and not as seed
                (in the tracker function, when the peer becomes a seed and is removed from the list of peers,
                the rank is set to -1). We generate a random number until we find a rank that is active.
                For the case when there is only one peer in the swarm we set the peer rank to the only peer available. */
                int available_peers[MAX_CLIENTS];
                int num_available_peers = 0;
                for (int j = 0; j < currentSwarm.num_seeds; j++) {
                    if (currentSwarm.seeds[j] != thisPeer->rank) {
                        available_peers[num_available_peers++] = currentSwarm.seeds[j];
                    }
                }

                if (num_available_peers == 0) {
                    continue;
                }

                srand(time(NULL));
                int peerRank = available_peers[rand() % num_available_peers];

                if (peerRank != thisPeer->rank && peerRank != 0) {
                    char chunk_hash[HASH_SIZE + 1];
                    strcpy(chunk_hash, currentSwarm.file.chunks[downloaded_chunks].hash);

                    MPI_Send("REQUEST_CHUNK", MAX_MSG_SIZE, MPI_CHAR, peerRank, MSG_REQUEST_CHUNK_TYPE, MPI_COMM_WORLD);
                    MPI_Send(&chunk_hash, HASH_SIZE + 1, MPI_CHAR, peerRank, MSG_REQUEST_CHUNK_DATA, MPI_COMM_WORLD);

                    char response[MAX_MSG_SIZE];
                    MPI_Recv(response, MAX_MSG_SIZE, MPI_CHAR, peerRank, MSG_SEND_CHUNK, MPI_COMM_WORLD, &status);
                    if (peerRank != status.MPI_SOURCE) {
                        printf("Error: Peer rank mismatch\n");
                        continue;
                    }

                    // After receiving a chunk and verifying the response
                    if (strcmp(response, "OK") == 0) {
                        char hash[HASH_SIZE + 1];
                        strcpy(hash, currentSwarm.file.chunks[downloaded_chunks].hash);

                        if (!has_file(thisPeer, currentSwarm.file.filename)) {
                            // Add new file to owned_files
                            if (thisPeer->num_owned_files < MAX_FILES) {
                                strcpy(thisPeer->owned_files[thisPeer->num_owned_files].filename, currentSwarm.file.filename);
                                thisPeer->owned_files[thisPeer->num_owned_files].num_chunks = 1;
                                strcpy(thisPeer->owned_files[thisPeer->num_owned_files].chunks[0].hash, hash);
                                thisPeer->num_owned_files++;
                            }
                        } else {
                            int file_index = get_file_index(thisPeer, currentSwarm.file.filename);
                            if (file_index != -1 && thisPeer->owned_files[file_index].num_chunks < MAX_CHUNKS) {
                                thisPeer->owned_files[file_index].num_chunks++;
                                strcpy(thisPeer->owned_files[file_index].chunks[thisPeer->owned_files[file_index].num_chunks - 1].hash, hash);
                            }
                        }

                        downloaded_chunks++;
                        if (downloaded_chunks % 10 == 0 && downloaded_chunks != 0) {
                            MPI_Send("REQUEST_PEERS", MAX_MSG_SIZE, MPI_CHAR, TRACKER_RANK, MSG_TO_TRACKER, MPI_COMM_WORLD);
                            MPI_Send(currentSwarm.file.filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_REQUEST_PEERS, MPI_COMM_WORLD);

                            MPI_Recv(&currentSwarm.num_peers, 1, MPI_INT, TRACKER_RANK, MSG_SEND_PEERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            MPI_Recv(currentSwarm.peers, MAX_CLIENTS, MPI_INT, TRACKER_RANK, MSG_SEND_PEERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        }

                        // Update the tracker with the new chunk with "UPDATE" message
                        update_tracker(*thisPeer);
                    }
                }
            }
        }

        // Notify the tracker that the download is complete
        if (downloaded_chunks == currentSwarm.file.num_chunks) {
            MPI_Send("FILE_DONE", MAX_MSG_SIZE, MPI_CHAR, TRACKER_RANK, MSG_TO_TRACKER, MPI_COMM_WORLD);
            MPI_Send(currentSwarm.file.filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, MSG_COMPLETED, MPI_COMM_WORLD);
            thisPeer->num_owned_files++;
            write_to_files(*thisPeer);
            completed_files++;
        } else {
            printf("Error downloading file %s\n", currentSwarm.file.filename);
        }

        // Check if all wanted files have been downloaded
        if (completed_files == wanted_files_no) {
            MPI_Send("DONE", MAX_MSG_SIZE, MPI_CHAR, TRACKER_RANK, MSG_TO_TRACKER, MPI_COMM_WORLD);
            break;
        }
    }

    pthread_exit(NULL);
}

void *upload_thread_func(void *arg) {
    PeerData *thisPeer = (PeerData*) arg;

    while (1) {
        // Listen for segment requests from other clients
        char requested_segment[HASH_SIZE + 1];
        char msg_type[MAX_MSG_SIZE];
        MPI_Status status;
        MPI_Recv(msg_type, MAX_MSG_SIZE, MPI_CHAR, MPI_ANY_SOURCE, MSG_REQUEST_CHUNK_TYPE, MPI_COMM_WORLD, &status);
        if (strcmp(msg_type, "SHUTDOWN") == 0 && status.MPI_SOURCE == TRACKER_RANK) {
            break;
        }

        MPI_Recv(requested_segment, HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, MSG_REQUEST_CHUNK_DATA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Check if the segment is owned
        bool segment_found = false;
        for (int i = 0; i < thisPeer->num_owned_files; i++) {
            for (int j = 0; j < thisPeer->owned_files[i].num_chunks; j++) {
                if (strcmp(thisPeer->owned_files[i].chunks[j].hash, requested_segment) == 0) {
                    segment_found = true;
                    break;
                }
            }
            if (segment_found) break;
        }

        // Respond to the request
        if (segment_found) {
            char response[MAX_MSG_SIZE];
            strcpy(response, "OK");
            MPI_Send(response, MAX_MSG_SIZE, MPI_CHAR, status.MPI_SOURCE, MSG_SEND_CHUNK, MPI_COMM_WORLD);
        } else {
            char response[MAX_MSG_SIZE];
            strcpy(response, "NOT_FOUND");
            MPI_Send(response, MAX_MSG_SIZE, MPI_CHAR, status.MPI_SOURCE, MSG_SEND_CHUNK, MPI_COMM_WORLD);
        }
    }

    pthread_exit(NULL);
}

void tracker(int numtasks, int rank) {
    TrackerData tracker_data;
    tracker_data.num_swarms = 0;
    tracker_data = receive_init_data(tracker_data, numtasks);
    int completed_peers = 0;

    // Get updates and requests from the clients
    while (1) {
        char message[MAX_MSG_SIZE];
        MPI_Status status;
        MPI_Recv(message, MAX_MSG_SIZE, MPI_CHAR, MPI_ANY_SOURCE, MSG_TO_TRACKER, MPI_COMM_WORLD, &status);

/** ~~~~~~~~~~~~~~~~~~~~~~~~~Peer has completed a file~~~~~~~~~~~~~~~~~~~~~~~~~ */

        if (strcmp(message, "FILE_DONE") == 0) {
            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, MSG_COMPLETED, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Add peer to the list of seeds of the swarm
            for (int i = 0; i < tracker_data.num_swarms; i++) {
                if (strcmp(tracker_data.swarms[i].file.filename, filename) == 0) {
                    tracker_data.swarms[i].seeds[tracker_data.swarms[i].num_seeds] = status.MPI_SOURCE;
                    tracker_data.swarms[i].num_seeds++;
                    tracker_data.swarms[i].num_peers--;

                    for (int j = 0; j < tracker_data.swarms[i].num_peers; j++) {
                        if (tracker_data.swarms[i].peers[j] == status.MPI_SOURCE) {
                            for (int k = j; k < tracker_data.swarms[i].num_peers - 1; k++) {
                                tracker_data.swarms[i].peers[k] = tracker_data.swarms[i].peers[k + 1];
                            }
                            tracker_data.swarms[i].num_peers--;
                            break;
                        }
                    }
                    break;
                }
            }

/** ~~~~~~~~~~~~~~~~~~~~~~~~Peer has completed downloading~~~~~~~~~~~~~~~~~~~~~ */

        } else if (strcmp(message, "DONE") == 0) {
            completed_peers++;

            if (completed_peers == numtasks - 1) {
                // Notify all peers to shut down
                for (int i = 1; i < numtasks; i++) {
                    MPI_Send("SHUTDOWN", MAX_MSG_SIZE, MPI_CHAR, i, MSG_REQUEST_CHUNK_TYPE, MPI_COMM_WORLD);
                }
                break;
            }

        } else if (strcmp(message, "UPDATE") == 0) {

/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~Update the swarm info~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

            int num_owned_files;
            int recv_rank;
            PeerData client_data;

            MPI_Recv(&recv_rank, 1, MPI_INT, status.MPI_SOURCE, MSG_UPDATE_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&num_owned_files, 1, MPI_INT, status.MPI_SOURCE, MSG_UPDATE_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            client_data.num_owned_files = num_owned_files;
            client_data.rank = recv_rank;

            for (int i = 0; i < num_owned_files; i++) {
                MPI_Recv(client_data.owned_files[i].filename, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, MSG_UPDATE_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&(client_data.owned_files[i].num_chunks), 1, MPI_INT, status.MPI_SOURCE, MSG_UPDATE_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Check if this client is already a peer in the swarm
                bool peer_exists = false;
                for (int j = 0; j < tracker_data.num_swarms; j++) {
                    if (strcmp(tracker_data.swarms[j].file.filename, client_data.owned_files[i].filename) == 0) {
                        for (int k = 0; k < tracker_data.swarms[j].num_peers; k++) {
                            if (tracker_data.swarms[j].peers[k] == recv_rank) {
                                peer_exists = true;

                                if (tracker_data.swarms[j].num_seeds == 0 || client_data.owned_files[i].num_chunks == tracker_data.swarms[j].file.num_chunks) {
                                    tracker_data.swarms[j].seeds[tracker_data.swarms[j].num_seeds] = recv_rank;
                                    tracker_data.swarms[j].num_seeds++;
                                    tracker_data.swarms[j].num_peers--;

                                    for (int l = k; l < tracker_data.swarms[j].num_peers; l++) {
                                        tracker_data.swarms[j].peers[l] = tracker_data.swarms[j].peers[l + 1];
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }

                if (!peer_exists) {
                    for (int j = 0; j < tracker_data.num_swarms; j++) {
                        if (strcmp(tracker_data.swarms[j].file.filename, client_data.owned_files[i].filename) == 0) {
                            tracker_data.swarms[j].peers[tracker_data.swarms[j].num_peers] = recv_rank;
                            tracker_data.swarms[j].num_peers++;
                            break;
                        }
                    }
                }
            }
        }  else if (strcmp(message, "REQUEST_SWARM") == 0) {

/** ~~~~~~~~~~~~~~~~~~~~~~~~~Client requests swarm info~~~~~~~~~~~~~~~~~~~~~~~~~~ */

            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, MSG_REQUEST_SWARM, MPI_COMM_WORLD, &status);

            for (int i = 0; i < tracker_data.num_swarms; i++) {
                if (strcmp(tracker_data.swarms[i].file.filename, filename) == 0) {
                    SwarmInfo currentSwarm = tracker_data.swarms[i];
                    MPI_Send(&currentSwarm.file.num_chunks, 1, MPI_INT, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);
                    MPI_Send(currentSwarm.file.filename, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);

                    for (int j = 0; j < currentSwarm.file.num_chunks; j++) {
                        MPI_Send(&(currentSwarm.file.chunks[j].hash), HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);
                    }

                    MPI_Send(&currentSwarm.num_seeds, 1, MPI_INT, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);
                    MPI_Send(currentSwarm.seeds, MAX_CLIENTS, MPI_INT, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);

                    MPI_Send(&currentSwarm.num_peers, 1, MPI_INT, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);
                    MPI_Send(currentSwarm.peers, MAX_CLIENTS, MPI_INT, status.MPI_SOURCE, MSG_GET_SWARM, MPI_COMM_WORLD);
                    break;
                }
            }
        } else if (strcmp(message, "REQUEST_PEERS") == 0) {

/** ~~~~~~~~~~~~~~~~~~~~~~~~~Client requests peers info~~~~~~~~~~~~~~~~~~~~~~~~~~ */

            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, MSG_REQUEST_PEERS, MPI_COMM_WORLD, &status);

            for (int i = 0; i < tracker_data.num_swarms; i++) {
                if (strcmp(tracker_data.swarms[i].file.filename, filename) == 0) {
                    SwarmInfo currentSwarm = tracker_data.swarms[i];
                    MPI_Send(&currentSwarm.num_peers, 1, MPI_INT, status.MPI_SOURCE, MSG_SEND_PEERS, MPI_COMM_WORLD);
                    MPI_Send(currentSwarm.peers, MAX_CLIENTS, MPI_INT, status.MPI_SOURCE, MSG_SEND_PEERS, MPI_COMM_WORLD);
                    break;
                }
            }
        }
    }
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    PeerData *peer_data = malloc(sizeof(PeerData));
    if (peer_data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(-1);
    }
    init_peer_data(peer_data, rank);

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) peer_data);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) peer_data);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }

    free(peer_data);
    peer_data = NULL;
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
