#ifndef TEMA1A_MAIN_H
#define TEMA1A_MAIN_H

#include <memory>
#include <map>

using namespace std;

struct File {
    string fileName;
    int fileID;
    int status;
};

struct mapperArgs {
    int filesNo;
    vector<File> *inputFiles;
    int mapperID;
    int mapNo;
    map<int, map<int, string>> *maps;
};

struct reducerArgs {
    int reducerID;
    int redNo;
    map<int, map<int, string>> *maps;
};

struct threadArgs {
    int index;
    int filesNo;
    vector<File> *inputFiles;
    map<int, map<int, std::string>> *maps;
    int mapNo;
    int redNo;
};

void *mapper(void *arg);
void *reduce(void *arg);

#endif //TEMA1A_MAIN_H
