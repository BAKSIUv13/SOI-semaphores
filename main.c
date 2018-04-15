// Semaphores SOI
// Bartlomiej Kulik
// 15.04.2018

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <limits.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

// VARIABLES
int N; // number of bolids
int K; // number of services
int PRIORITY; // priority vaule
int L; // number of visits in services

// shared memory below
int *inService;
int *readyToEntry;
int *readyToEscape;

int *freeServiceTrack;
// end of shared memory
// semaphores
int semId;
struct sembuf semOpArg = { 0, 0, 0 };
enum { acces, queueEntry, queueEscape };
// key to semaphores and shared memory
key_t key;

// PROTOTYPES
int areValid(int argc, char **argv);
void startWork();
int runTasks();
void prepareSharedMemory();
void endWork();
void P(int semaphore);
void V(int semaphore);

void bolid(int id);
void leaveServiceTrack();
void drivingOnTheRoad(unsigned int timeSec, int id);
void entryToService(unsigned int timeSec, int id);
void serviceWorks(unsigned int timeSec, int id);
void exitService(unsigned int timeSec, int id);

// MAIN FUNCTION
int main(int argc, char **argv)
{
    // set N,K,P,L
    if (!areValid(argc, argv))
    {
        return 1;
    }

    startWork();
    if (runTasks())
    {
        return 0;
    }
    endWork();

    return 0;
}

void startWork()
{
    int shmId;

    // MEMORY
    shmId = shmget(key, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("startWork(): shmId < 0\n");
        exit(0);
    }
    inService = (int *)shmat(shmId, NULL, 0);
    if (inService < 0)
    {
        printf("startWork(): inService < 0\n");
        exit(0);
    }
    *inService = 0;
    shmdt(inService);

    shmId = shmget(key + 1, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("startWork(): shmId < 0\n");
        exit(0);
    }
    readyToEntry = (int *)shmat(shmId, NULL, 0);
    if (readyToEntry < 0)
    {
        printf("startWork(): readyToEntry < 0\n");
        exit(0);
    }
    *readyToEntry = 0;
    shmdt(readyToEntry);

    shmId = shmget(key + 2, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("startWork(): shmId < 0\n");
        exit(0);
    }
    readyToEscape = (int *)shmat(shmId, NULL, 0);
    if (readyToEscape < 0)
    {
        printf("startWork(): readyToEscape < 0\n");
        exit(0);
    }
    *readyToEscape = 0;
    shmdt(readyToEscape);

    shmId = shmget(key + 3, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("startWork(): shmId < 0\n");
        exit(0);
    }    
    freeServiceTrack = (int *)shmat(shmId, NULL, 0);
    if (freeServiceTrack < 0)
    {
        printf("startWork(): freeServiceTrack < 0\n");
        exit(0);
    }
    *freeServiceTrack = TRUE;
    shmdt(freeServiceTrack);

    // SEMAPHORES
    semId = semget(key, 3, 0600 | IPC_CREAT);
    semctl(semId, acces, SETVAL, 1);
    semctl(semId, queueEscape, SETVAL, 0);
    semctl(semId, queueEntry, SETVAL, 0);

}

void prepareSharedMemory()
{
    int shmId;

    shmId = shmget(key, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("prepareSharedMemory(): shmId < 0\n");
        exit(0);
    }    
    inService = (int *)shmat(shmId, NULL, 0);
    if (inService < 0)
    {
        printf("prepareSharedMemory(): inService < 0\n");
        exit(0);
    }

    shmId = shmget(key + 1, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("prepareSharedMemory(): shmId < 0\n");
        exit(0);
    }    
    readyToEntry = (int *)shmat(shmId, NULL, 0);
    if (readyToEntry < 0)
    {
        printf("prepareSharedMemory(): readyToEntry < 0\n");
        exit(0);
    }

    shmId = shmget(key + 2, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("prepareSharedMemory(): shmId < 0\n");
        exit(0);
    }    
    readyToEscape = (int *)shmat(shmId, NULL, 0);
    if (readyToEscape < 0)
    {
        printf("prepareSharedMemory(): readyToEscape < 0\n");
        exit(0);
    }

    shmId = shmget(key + 3, sizeof(int), 0600 | IPC_CREAT);
    if (shmId < 0)
    {
        printf("prepareSharedMemory(): shmId < 0\n");
        exit(0);
    }    
    freeServiceTrack = (int *)shmat(shmId, NULL, 0);
    if (freeServiceTrack < 0)
    {
        printf("prepareSharedMemory(): freeServiceTrack < 0\n");
        exit(0);
    }

}

int runTasks()
{
    int pid;
    int i;
    for (i = 0; i < N; ++i)
    {
        pid = fork();
        printf("fork!\n");
        if (pid == 0) // child
        {
            prepareSharedMemory();
            // go!
            printf("go: %d!\n", i);
            bolid(i);
            printf("finish: %d!\n", i);
            // finish!
            return 1;
        }
    }
    return 0;
}

void endWork()
{
    int processes;
    for (processes = N; processes > 0; --processes) // one pass
    {
        wait(NULL);
    }

    semctl(semId, 0, IPC_RMID);

    shmctl(inService, IPC_RMID, NULL);
    shmctl(readyToEntry, IPC_RMID, NULL);
    shmctl(readyToEscape, IPC_RMID, NULL);
    shmctl(freeServiceTrack, IPC_RMID, NULL);
}

void P(int semaphore)
{
    semOpArg.sem_num = semaphore;
    semOpArg.sem_op = -1;
    semop(semId, &semOpArg, 1);
}

void V(int semaphore)
{
    semOpArg.sem_num = semaphore;
    semOpArg.sem_op = 1;
    semop(semId, &semOpArg, 1);
}

int areValid(int argc, char **argv)
{
    if (argc != 5)
    {
        return 0;
    }
    N = atoi(argv[1]);
    K = atoi(argv[2]);
    PRIORITY = atoi(argv[3]);
    L = atoi(argv[4]);
    if (N <= 0 || K <= 0 || PRIORITY <= 0 || L <= 0)
    {
        return 0;
    }
    // key set
    key_t key = ftok(argv[0], 1);
    return 1;
}
// BOLID ALGORITHM
void bolid(int id)
{
    int refueling = 0;
    while (refueling++ < L) // every bolid is serviced L times
    {
        drivingOnTheRoad(8, id);

        P(acces);
        if (*freeServiceTrack && (*inService < K))
        {
            *freeServiceTrack = FALSE;
            V(acces);
        }
        else
        {
            ++*readyToEntry;
            V(acces);
            P(queueEntry);
        }

        entryToService(2, id);


        P(acces);
        ++*inService;
        printf("inService: %d bolids\n", *inService);
        leaveServiceTrack();
        V(acces);

        serviceWorks(5, id);
        printf("Finish service: %d\n", id);

        P(acces);
        if (*freeServiceTrack)
        {
            *freeServiceTrack = FALSE;
            V(acces);
        }
        else
        {
            ++*readyToEscape;
            V(acces);
            P(queueEscape); // ERROR 1 - V -> P
        }

        exitService(2, id);

        P(acces);
        --*inService;
        printf("inService: %d bolids\n", *inService);
        leaveServiceTrack();
        V(acces);
    } // main while

} // bolid

void leaveServiceTrack()
{

    if (*inService < PRIORITY)
    {
        if (*readyToEntry > 0)
        {
            --*readyToEntry;
            V(queueEntry);
        }
        else if (*readyToEscape > 0)
        {
            --*readyToEscape;
            V(queueEscape); // ERROR 2 - was queueEntry
        }
        else
        {
            *freeServiceTrack = TRUE;
        }
    }
    else
    {
        if (*readyToEscape > 0)
        {
            --*readyToEscape;
            V(queueEscape); // ERROR 3
        }
        else if (*readyToEntry > 0)
        {
            --*readyToEntry;
            V(queueEntry);
        }
        else
        {
            *freeServiceTrack = TRUE;
        }
    } // else

} // zwolnij Pas serwisowy

void drivingOnTheRoad(unsigned int timeSec, int id)
{
    printf("drivingOnTheRoad: %d\n", id);
    sleep(timeSec);
}

void entryToService(unsigned int timeSec, int id)
{
    printf("entryToService: %d\n", id);
    sleep(timeSec);
}

void serviceWorks(unsigned int timeSec, int id)
{
    printf("serviceWorks: %d\n", id);
    sleep(timeSec);
}

void exitService(unsigned int timeSec, int id)
{
    printf("exitService: %d\n", id);
    sleep(timeSec);
}
