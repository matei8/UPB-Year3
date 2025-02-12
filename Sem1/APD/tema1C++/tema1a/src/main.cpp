#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <pthread.h>
#include <algorithm>
#include <sstream>
#include <set>

#include "main.h"

using namespace std;

pthread_mutex_t mutex;
pthread_barrier_t barrier;
pthread_barrier_t barrierReducer;

void *mapper(void *arg) {
    auto *args = (struct mapperArgs *) arg;
    int filesNo = args->filesNo;
    int mapperID = args->mapperID;
    auto *maps = args->maps;
    int mapNo = args->mapNo;
    vector<File> inputFiles = *args->inputFiles;

    int filesPerMapper = filesNo / mapNo;
    int remainder = filesNo % mapNo;
    int start = mapperID * filesPerMapper + min(mapperID, remainder);
    int end = start + filesPerMapper + (mapperID < remainder ? 1 : 0);

    for (int i = start; i < end; i++) {
        if (inputFiles[i].status == 0) {
            ifstream currentFile(inputFiles[i].fileName);
            if (!currentFile.is_open()) {
                cerr << "Error opening file " << inputFiles[i].fileName << endl;
                continue;
            }

            map<int, string> localMap;
            set<string> uniqueWords;

            string line;
            while (getline(currentFile, line)) {
                istringstream lineStream(line);
                string token;

                while (lineStream >> token) {
                    token.erase(remove_if(token.begin(), token.end(), [](char c) {
                        return !isalpha(static_cast<unsigned char>(c));
                    }), token.end());

                    transform(token.begin(), token.end(), token.begin(), ::tolower);

                    if (!token.empty() && uniqueWords.find(token) == uniqueWords.end()) {
                        uniqueWords.insert(token);
                        localMap[localMap.size()] = token;
                    }
                }
            }

            pthread_mutex_lock(&mutex);
            (*maps)[i] = std::move(localMap);
            inputFiles[i].status = 1;
            pthread_mutex_unlock(&mutex);
            currentFile.close();
        }
    }

    return NULL;
}

void *reduce(void *arg) {
    auto *args = (struct reducerArgs *)arg;
    int reducerID = args->reducerID;
    int redNo = args->redNo;
    auto *maps = args->maps;

    int totalChars = 26;
    int charsPerReducer = totalChars / redNo;
    int remainder = totalChars % redNo;

    char startChar = 'a' + reducerID * charsPerReducer + min(reducerID, remainder);
    char endChar = startChar + charsPerReducer + (reducerID < remainder ? 1 : 0) - 1;

    map<char, map<string, vector<int>>> localReductions;

    // Process maps assigned to this reducer
    for (char currentChar = startChar; currentChar <= endChar; ++currentChar) {
        for (const auto &[mapID, currentMap] : *maps) {
            for (const auto &[_, word] : currentMap) {
                if (word[0] == currentChar) {
                    localReductions[currentChar][word].push_back(mapID);
                }
            }
        }
    }

    // Sort word lists locally
    for (auto &[currentChar, wordsMap] : localReductions) {
        for (auto &[word, mapIDs] : wordsMap) {
            sort(mapIDs.begin(), mapIDs.end());
        }
    }

    pthread_barrier_wait(&barrierReducer); // Ensure all reducers finish processing

    // Write results to output files
    for (char currentChar = startChar; currentChar <= endChar; ++currentChar) {
        string outputFileName = string(1, currentChar) + ".txt";
        ofstream outputFile(outputFileName);

        if (!outputFile.is_open()) {
            cerr << "Error opening file " << outputFileName << endl;
            continue;
        }

        if (localReductions.find(currentChar) != localReductions.end()) {
            vector<pair<string, vector<int>>> words(
                    localReductions[currentChar].begin(),
                    localReductions[currentChar].end()
            );

            // Sort words by frequency and lexicographically
            sort(words.begin(), words.end(), [](const auto &a, const auto &b) {
                if (a.second.size() != b.second.size()) {
                    return a.second.size() > b.second.size();
                }
                return a.first < b.first;
            });

            for (const auto &[word, mapIDs] : words) {
                outputFile << word << ":[";

                for (size_t i = 0; i < mapIDs.size(); ++i) {
                    outputFile << mapIDs[i] + 1;
                    if (i != mapIDs.size() - 1) {
                        outputFile << " ";
                    }
                }

                outputFile << "]\n";
            }
        }

        outputFile.close();
    }

    return NULL;
}

void *func(void *args) {
    auto *arg = (struct threadArgs *)args;
    int index = arg->index;
    int mapNo = arg->mapNo;
    if (index < mapNo) {
        auto *mapArgs = (struct mapperArgs *)malloc(sizeof(struct mapperArgs));
        mapArgs->filesNo = arg->filesNo;
        mapArgs->mapperID = index;
        mapArgs->maps = arg->maps;
        mapArgs->mapNo = arg->mapNo;
        mapArgs->inputFiles = arg->inputFiles;
        mapper(mapArgs);
        free(mapArgs);
    }

    pthread_barrier_wait(&barrier);

    if (index >= mapNo) {
        auto *redArgs = (struct reducerArgs *)malloc(sizeof(struct reducerArgs));
        redArgs->reducerID = index - mapNo;
        redArgs->redNo = arg->redNo;
        redArgs->maps = arg->maps;
        reduce(redArgs);
        free(redArgs);
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
///////////// Read arguments //////////////
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <mapNo> <redNo> <inputFileName>\n";
        return 1;
    }

    string inputFileName = argv[3];
    ifstream inputFile(inputFileName);

    if (!inputFile.is_open()) {
        cerr << "Error opening file\n";
        return 1;
    }

///////////// Shared variables //////////////
    vector<File> inputFiles;
    int mapNo = stoi(argv[1]);
    int redNo = stoi(argv[2]);
    auto *maps = new map<int, map<int, string>>();
    auto *reds = new map<char, map<string, vector<int>>>();

///////////// Read input files //////////////
    string line;
    int filesNo;
    getline(inputFile, line);
    filesNo = stoi(line);

    for (int i = 0; i < filesNo; i++) {
        File currentFile;
        string currentFileName;
        getline(inputFile, currentFileName);

        currentFileName.erase(currentFileName.find_last_not_of(" \n\r\t") + 1);
        currentFile.fileName = currentFileName;
        currentFile.fileID = i;
        currentFile.status = 0;

        inputFiles.push_back(std::move(currentFile));
    }

    inputFile.close();

///////////// Create threads //////////////
    pthread_t threads[mapNo + redNo];
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, mapNo + redNo);
    pthread_barrier_init(&barrierReducer, NULL, redNo);

    for (int i = 0; i < mapNo + redNo; i++) {
        auto *args = (struct threadArgs *)malloc(sizeof(struct threadArgs));
        args->index = i;
        args->filesNo = filesNo;
        args->inputFiles = &inputFiles;
        args->maps = maps;
        args->mapNo = mapNo;
        args->redNo = redNo;

        int r = pthread_create(&threads[i], NULL, func, args);
        if (r) {
            perror("Error creating thread");
            return 1;
        }
    }

///////////// Join threads //////////////
    for (int i = 0; i < mapNo + redNo; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    pthread_barrier_destroy(&barrierReducer);

    delete maps;
    delete reds;

    return 0;
}
