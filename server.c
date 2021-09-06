//
// Created by ubuntu on 12.06.2021.
//

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include "customDefinitions.h"
#include <signal.h>

static threadMapStruct threadMap[MAX_THREAD_COUNT];

static void cacheFiles(void)
{
    int iFileIndex;
    FILE *fp;

    for (iFileIndex = 0; iFileIndex < MAX_FILE_COUNT; iFileIndex++)
    {
        memset(fileCache[iFileIndex].strData, 0, sizeof(char) * MAX_FILE_SIZE);
        fp = fopen(fileCache[iFileIndex].strFileName, "r");
        if (fp == NULL)
        {
            printf("File %s cannot be opened\n", fileCache[iFileIndex].strFileName);
        }
        else
        {
            // get file size
            fseek(fp, 0, SEEK_END);
            fileCache[iFileIndex].iFileSize = ftell(fp);

            // read file
            rewind(fp);
            fread(fileCache[iFileIndex].strData, fileCache[iFileIndex].iFileSize, 1, fp);
            fclose(fp);
        }
    }

}

static int getThreadIndex(int iProcessID)
{
    int iRet = -1;
    int iThreadIndex;

    for (iThreadIndex = 0; iThreadIndex < MAX_THREAD_COUNT; iThreadIndex++)
    {
        if ((threadMap[iThreadIndex].message.iProcessID == iProcessID) && (threadMap[iThreadIndex].iActiveThread == 1))
        {
            // thread created before for this process
            iRet = iThreadIndex;
        }
    }

    return iRet;
}

void *handleWorkerTask(void *piTreadIndex)
{
    int iFileID;
    int iThreadIndex;
    sharedMemoryStruct *sharedMemory = NULL;

    iThreadIndex = *(int *)(&piTreadIndex);

    // attach to shared memory segment
    sharedMemory = shmat(threadMap[iThreadIndex].message.iSharedMemoryAddr, NULL, 0);
    if (sharedMemory < 0) {
        printf("Shared Memory segment cannot be attached\n");
        exit(1);
    }

    while (threadMap[iThreadIndex].iActiveThread == 1)
    {
        // wait for client to set read to write flag of shared memory
        while ((threadMap[iThreadIndex].iActiveThread == 1) && (threadMap[iThreadIndex].iReadyToWrite == 0))
        {
            sleep(1);
        }

        iFileID = threadMap[iThreadIndex].message.iFileID;
        memcpy(sharedMemory->strBuffer, fileCache[iFileID].strData, fileCache[iFileID].iFileSize);
        sharedMemory->iWrittenBytes = fileCache[iFileID].iFileSize;
        threadMap[iThreadIndex].iReadyToWrite = 0;

        // send finish signal
        kill(threadMap[iThreadIndex].message.iProcessID, SIGUSR1);

        sleep(1);
    }

    pthread_exit(NULL);
}

static void createNewThread(messageQueueMessageData *message)
{
    pthread_t pThreadID;
    int iThreadIndex;

    for (iThreadIndex = 0; iThreadIndex < MAX_THREAD_COUNT; iThreadIndex++)
    {
        if (threadMap[iThreadIndex].iActiveThread == 0)
        {
            threadMap[iThreadIndex].message.iSharedMemoryAddr = message->iSharedMemoryAddr;
            threadMap[iThreadIndex].message.iProcessID = message->iProcessID;
            threadMap[iThreadIndex].message.iFileID = message->iFileID;
            threadMap[iThreadIndex].iReadyToWrite = 1;
            threadMap[iThreadIndex].iActiveThread = 1;

            pthread_create(&pThreadID, NULL, handleWorkerTask, (void *)iThreadIndex);
            threadMap[iThreadIndex].pThreadID = pThreadID;

            printf("A new worker thread %ld created for client %d\n", pThreadID, message->iProcessID);
            break;
        }
    }
}

static void terminateClientThread(pid_t iProcessID)
{
    int iThreadIndex = getThreadIndex(iProcessID);

    if (iThreadIndex >= 0)
    {
        threadMap[iThreadIndex].iActiveThread = 0;
        printf("Client process %d terminated. Thread %ld is terminating.. \n",
               threadMap[iThreadIndex].message.iProcessID, threadMap[iThreadIndex].pThreadID);
    }
}

void handleServerSignals(int sig, siginfo_t *info, void *context)
{
    if (sig == SIGUSR2)
    {
        terminateClientThread(info->si_pid);
    }
}

int main() {
    key_t key;
    int iMessageID = -1;
    int iThreadIndex;
    int iRCVMessageSize;
    messageQueueMessage message;
    struct sigaction sa;

    printf("Greetings From Server!\n");

    // store files into memory
    cacheFiles();
    // generate unique key for shared memory and message queue operations
    key = ftok(KEY_PATH, KEY_ID);
    // get message queue identifier
    iMessageID = msgget(key, 0666 | IPC_CREAT);

    // listen SIGUSR2 signal which will be generated by client when client process is about to terminate
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handleServerSignals;
    sigaction(SIGUSR2, &sa, NULL);

    while (1)
    {
        iRCVMessageSize = msgrcv(iMessageID, &message, sizeof(message), MESSAGE_TYPE, 0);
        if (iRCVMessageSize > 0)
        {
            printf("File %d requested from client %d\n", message.message.iFileID, message.message.iProcessID);

            // check if thread created before for this process
            iThreadIndex = getThreadIndex(message.message.iProcessID);
            if (iThreadIndex < 0)
            {
                // no thread created before for this process. create now.
                createNewThread(&message.message);
            }
            else
            {
                // thread created before, update file id
                threadMap[iThreadIndex].message.iFileID = message.message.iFileID;
                threadMap[iThreadIndex].iReadyToWrite = 1;
            }

            // clear message buffer
            memset(&message, 0, sizeof(message));
        }

        sleep(0.5);
    }

    msgctl(iMessageID, IPC_RMID, NULL);
    return 0;
}
