#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

// read write lock with writer preference
typedef struct rwLock
{
    int activeReaders;
    int waitingReaders;
    int waitingWriters;
    sem_t activeReadersMutex;
    sem_t waitingReadersMutex;
    sem_t waitingWritersMutex;
    sem_t writeLock;
    sem_t readersQueue;
} rwLockWriterPref;

/**
 * Initialize the read write lock
 * @param rw The read write lock
 *
 * @return void
 */
void rwLockWriterPrefInit(rwLockWriterPref *rw)
{
    rw->activeReaders = 0;
    rw->waitingReaders = 0;
    rw->waitingWriters = 0;
    sem_init(&rw->activeReadersMutex, 0, 1);
    sem_init(&rw->waitingReadersMutex, 0, 1);
    sem_init(&rw->waitingWritersMutex, 0, 1);
    sem_init(&rw->writeLock, 0, 1);
    sem_init(&rw->readersQueue, 0, 1);
}

/**
 * Destroy the read write lock
 * @param rw The read write lock
 *
 * @return void
 */
void rwLockWriterPrefDestroy(rwLockWriterPref *rw)
{
    sem_destroy(&rw->activeReadersMutex);
    sem_destroy(&rw->waitingReadersMutex);
    sem_destroy(&rw->waitingWritersMutex);
    sem_destroy(&rw->writeLock);
    sem_destroy(&rw->readersQueue);
}

/**
 * Read lock the read write lock
 * @param rw The read write lock
 *
 * @return void
 */
void rwLockWriterPrefReadLock(rwLockWriterPref *rw)
{
    sem_wait(&rw->waitingReadersMutex);
    rw->waitingReaders++;
    sem_post(&rw->waitingReadersMutex);

    sem_wait(&rw->readersQueue);
    sem_wait(&rw->waitingWritersMutex);
    while (rw->waitingWriters > 0)
    {
        sem_post(&rw->waitingWritersMutex);
        sem_post(&rw->readersQueue);
        sleep(1);
        sem_wait(&rw->readersQueue);
        sem_wait(&rw->waitingWritersMutex);
    }

    sem_wait(&rw->activeReadersMutex);
    rw->activeReaders++;
    if (rw->activeReaders == 1)
    {
        sem_wait(&rw->writeLock);
    }
    sem_post(&rw->activeReadersMutex);

    sem_wait(&rw->waitingReadersMutex);
    rw->waitingReaders--;
    sem_post(&rw->waitingReadersMutex);

    sem_post(&rw->waitingWritersMutex);
    sem_post(&rw->readersQueue);
}

/**
 * Read unlock the read write lock
 * @param rw The read write lock
 *
 * @return void
 */
void rwLockWriterPrefReadUnlock(rwLockWriterPref *rw)
{
    sem_wait(&rw->activeReadersMutex);
    rw->activeReaders--;
    if (rw->activeReaders == 0)
    {
        sem_post(&rw->writeLock);
    }
    sem_post(&rw->activeReadersMutex);
}

/**
 * Write lock the read write lock
 * @param rw The read write lock
 *
 * @return void
 */
void rwLockWriterPrefWriteLock(rwLockWriterPref *rw)
{
    sem_wait(&rw->waitingWritersMutex);
    rw->waitingWriters++;
    if (rw->waitingWriters == 1)
    {
        sem_wait(&rw->readersQueue);
    }
    sem_post(&rw->waitingWritersMutex);

    sem_wait(&rw->writeLock);
}

/**
 * Write unlock the read write lock
 * @param rw The read write lock
 *
 * @return void
 */
void rwLockWriterPrefWriteUnlock(rwLockWriterPref *rw)
{
    sem_wait(&rw->waitingWritersMutex);
    rw->waitingWriters--;
    if (rw->waitingWriters == 0)
    {
        sem_post(&rw->readersQueue);
    }
    sem_post(&rw->waitingWritersMutex);

    sem_post(&rw->writeLock);
}

rwLockWriterPref rw;
FILE *file = NULL;

void *reader(void *arg)
{
    rwLockWriterPrefReadLock(&rw);
    fprintf(file, "Reading,Number-of-readers-present:[%d]\n", rw.activeReaders);
    FILE *sharedFile = fopen("shared-file.txt", "r");
    char c;
    while ((c = fgetc(sharedFile)) != EOF)
    {
        // reading file
    }
    fclose(sharedFile);
    rwLockWriterPrefReadUnlock(&rw);
    return NULL;
}

void *writer(void *arg)
{
    // your code here
    rwLockWriterPrefWriteLock(&rw);
    fprintf(file, "Writing,Number-of-readers-present:[%d]\n", rw.activeReaders);
    FILE *sharedFile = fopen("shared-file.txt", "a");
    fprintf(sharedFile, "Hello World!\n");
    fclose(sharedFile);
    rwLockWriterPrefWriteUnlock(&rw);
    return NULL;
}

int main(int argc, char **argv)
{

    // Do not change the code below to spawn threads
    if (argc != 3)
        return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    // Initialize read write lock and open file
    rwLockWriterPrefInit(&rw);
    file = fopen("output-writer-pref.txt", "w");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return 1;
    }

    // Create reader and writer threads
    for (long i = 0; i < n; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (long i = 0; i < m; i++)
        pthread_create(&writers[i], NULL, writer, NULL);

    // Wait for all threads to complete
    for (int i = 0; i < n; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++)
        pthread_join(writers[i], NULL);

    // Clean up
    fclose(file);
    rwLockWriterPrefDestroy(&rw);

    return 0;
}