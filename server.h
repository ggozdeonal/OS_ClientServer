//
// Created by ubuntu on 12.06.2021.
//

#ifndef BIL461_HW1_SERVER_H
#define BIL461_HW1_SERVER_H

#include <pthread.h>
#include "customDefinitions.h"

#define MAX_FILE_NAME 100
#define MAX_THREAD_COUNT 50

typedef struct threadMap {
    pthread_t pThreadID;
    messageQueueMessageData message;
    int iReadyToWrite;
    int iActiveThread;
} threadMapStruct ;

typedef struct fileCache {
    char strFileName[MAX_FILE_NAME];
    char strData[MAX_FILE_SIZE];
    int iFileSize;
} fileCacheStruct ;

static fileCacheStruct fileCache[MAX_FILE_COUNT] = {
        {"file1", { 0 }, 0},
        {"file2", { 0 }, 0},
        {"file3", { 0 }, 0},
        {"file4", { 0 }, 0},
        {"file5", { 0 }, 0}
};

#endif //BIL461_HW1_SERVER_H
