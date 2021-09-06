//
// Created by ubuntu on 12.06.2021.
//

#ifndef BIL461_HW1_CUSTOMDEFINITIONS_H
#define BIL461_HW1_CUSTOMDEFINITIONS_H

#include <sys/types.h>
#include <unistd.h>

#define MAX_FILE_COUNT  5
#define MAX_FILE_SIZE   250
#define KEY_PATH        "."
#define KEY_ID          42
#define MESSAGE_TYPE    1

typedef struct sharedMemoryStruct
{
    int iWrittenBytes;
    char strBuffer[MAX_FILE_SIZE];
} sharedMemoryStruct;

typedef struct messageQueueMessageData {
    int iFileID;                // requested file number
    int iSharedMemoryAddr;      // shared memory address
    pid_t iProcessID;           // pid for signal generation
} messageQueueMessageData;

typedef struct messageQueueMessage {
    long mtype;
    messageQueueMessageData message;
} messageQueueMessage;

#endif //BIL461_HW1_CUSTOMDEFINITIONS_H
