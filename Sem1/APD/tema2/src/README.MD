# BitTorrent Protocol Simulation Using MPI

## Introduction

This project simulates the BitTorrent protocol using Message Passing Interface (MPI) for inter-process communication 
and POSIX threads (pthreads) for concurrent operations. The simulation involves a tracker and multiple peers that 
communicate to download and share files in a decentralized manner.

---

## Architecture Overview

### Tracker

**Overview**

The `tracker` function serves as the central coordinator in the BitTorrent protocol simulation. It manages the swarms 
of files, tracks which peers have which files, and facilitates communication between peers by providing necessary
information and updates. It is also responsible for managing the overall operation of the BitTorrent network simulation. It handles 
messages from peers, updates swarm information, and coordinates the shutdown of the simulation once all peers have completed 
their downloads.

- Initializes `tracker_data` and sets `num_swarms` to 0.

- Calls `receive_init_data` to gather initial data from all peers about the files they own. After receiving the data, it sends
to each peer the `Start` message to begin the actual transfer of files. In this function every peer sends to the tracker
the files that it owns with the hash chunks. The tracker will store this information in the `tracker_data` structure.

- Initializes `completed_peers` to 0.

- **Message Handling Loop:**

- Runs indefinitely to handle messages from peers. It breaks out of the loop when the variable `completed_peers` is equal 
to the number of peers, meaning that all peers have completed their downloads. Whe  this happens the tracker sends a
`SHUTDOWN` message to all tasks to terminate the simulation.
    ```c
    if (completed_peers == numtasks - 1) {
        // Notify all peers to shut down
        for (int i = 1; i < numtasks; i++) {
            MPI_Send("SHUTDOWN", MAX_MSG_SIZE, MPI_CHAR, i, MSG_REQUEST_CHUNK_TYPE, MPI_COMM_WORLD);
        }
        break;
    }
            

- Uses `MPI_Recv` to receive messages of type `char` with tag `MSG_TO_TRACKER` from any source, which describes that a
client has sent a message to the tracker. After getting the message it checks the type of message in order to fulfill the
requests of the peers. The types of messages are:
    - `FILE_DONE`: Indicates that a peer has completed downloading a file. The tracker updates the swarm by adding the 
    peer to the seeds list and removing it from the peers list.
    - `DONE`: Indicates that a peer has finished downloading all its desired files. The tracker keeps a count of completed 
    peers and, once all peers have completed, sends a `SHUTDOWN` message to all peers to terminate the simulation.
    - `UPDATE`: Sent by a peer to update the tracker about the files it currently owns. The tracker updates the swarm 
    information accordingly, moving peers to seeds if they have all chunks of a file.
    - `REQUEST_SWARM`: A peer requests information about a specific swarm (file). The tracker sends back the details of
    the swarm, including chunk hashes, seeds, and peers.
    - `REQUEST_PEERS`: A peer requests the list of peers for a specific swarm. The tracker sends back the list of peers for that swarm.

---

### Peers

**Overview**

The `peer` function simulates a peer in the BitTorrent network. Each peer is responsible for downloading desired files
by requesting chunks from seeds or other peers and uploading chunks to requesting peers. The peer function initializes
peer data, sends initial data to the tracker, and waits for the start signal to begin the download process.

**Download Process**

- The `void *download_thread_func(void *arg)` function initializes some variables like completed_files and wanted_files_no, 
and copies the wanted files into a local array. Then it enters a loop for each wanted file, until all the segments of the
file are received from seeds or peers.

- Inside the loop, it sends a `REQUEST_SWARM` message to the tracker to get information about the swarm for that file. 
It receives the swarm info, including the number of chunks, chunk hashes, seeds, and peers.

- The while loop goes on until the number of the owned chunks of the current file are equal to the number of chunks of the file
in the swarm info received from the tracker. Inside this loop, it checks if there are any peers available. If not, it tries 
to get chunks from seeds. It selects a random seed that isn't itself and requests a chunk.

- If there are peers, it selects a random peer and requests a chunk from them. It sends a `REQUEST_CHUNK` message with 
the hash of the chunk it wants. It then waits for a response. If the response is `OK`, it adds the chunk to its owned_files 
and updates the tracker. If not, it continues to the next iteration.

- After receiving a chunk, it checks if it already has the file. If not, it adds the file to its owned_files array. If 
it does have the file, it adds the new chunk to the existing file's chunk list.

- After downloading 10 chunks, it requests an updated list of peers from the tracker. Once all chunks are downloaded, it 
sends a `FILE_DONE` message to the tracker and writes the file to an output file. It increments the completed_files counter 
and checks if all wanted files are downloaded. If so, it sends a `DONE` message to the tracker and exits.

**Upload Process**

- The `void *upload_thread_func(void *arg)` is responsible for handling incoming chunk requests from other peers. It 
listens for `REQUEST_CHUNK_TYPE` messages and, upon receiving a request, checks if it has the requested chunk. If it 
does, it sends back an `OK` response; otherwise, it sends `NOT_FOUND`. It continues this process until it receives a 
`SHUTDOWN` message from the tracker, at which point it exits.

---

## Message Tags and Constants

- **Message Tags**: Defined in `utils.h` for MPI communications.
Depending on the message type, the tags are used to differentiate between messages. They are designed in pairs for sending
to the tracker and receiving from the tracker for example:
- MSG_REQUEST_SWARM with MSG_GET_SWARM
- MSG_REQUEST_CHUNK_DATA with MSG_SEND_CHUNK

- **Constants**: Include maximum sizes for files, filenames, chunks, etc.
  ```c
  #define MAX_FILES 10
  #define MAX_FILENAME 20
  #define HASH_SIZE 32
  #define MAX_CHUNKS 100
  #define MAX_CLIENTS 10
  #define MAX_MSG_SIZE 1024

---
