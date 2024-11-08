#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

// struct for read-write lock with reader preference
typedef struct rwLock
{
    sem_t mutex;
    sem_t writeLock;
    int readCount;
} rwLockReaderPref;

/**
 * Initialize the reader-writer lock
 * @param rw the reader-writer lock
 *
 * @return void
 */
void rwLockReaderPrefInit(rwLockReaderPref *rw)
{
    rw->readCount = 0;
    sem_init(&rw->mutex, 0, 1);
    sem_init(&rw->writeLock, 0, 1);
}

/**
 * Destroy the reader-writer lock
 * @param rw the reader-writer lock
 *
 * @return void
 */
void rwLockReaderPrefDestroy(rwLockReaderPref *rw)
{
    sem_destroy(&rw->mutex);
    sem_destroy(&rw->writeLock);
}

/**
 * Lock the reader-writer lock for reading
 * @param rw the reader-writer lock
 *
 * @return void
 */
void rwLockReaderPrefReadLock(rwLockReaderPref *rw)
{
    sem_wait(&rw->mutex);
    if (rw->readCount == 0)
        sem_wait(&rw->writeLock);
    rw->readCount++;
    sem_post(&rw->mutex);
}

/**
 * Unlock the reader-writer lock after reading
 * @param rw the reader-writer lock
 *
 * @return void
 */
void rwLockReaderPrefReadUnlock(rwLockReaderPref *rw)
{
    sem_wait(&rw->mutex);
    rw->readCount--;
    if (rw->readCount == 0)
        sem_post(&rw->writeLock);
    sem_post(&rw->mutex);
}

/**
 * Lock the reader-writer lock for writing
 * @param rw the reader-writer lock
 *
 * @return void
 */
void rwLockReaderPrefWriteLock(rwLockReaderPref *rw)
{
    sem_wait(&rw->writeLock);
}

/**
 * Unlock the reader-writer lock after writing
 * @param rw the reader-writer lock
 *
 * @return void
 */
void rwLockReaderPrefWriteUnlock(rwLockReaderPref *rw)
{
    sem_post(&rw->writeLock);
}

// Global variables

rwLockReaderPref rw;
FILE *file = NULL;

/**
 * Reader thread function
 * @param arg the argument to the reader
 *
 * @return void
 */
void *reader(void *arg)
{
    // your code here
    rwLockReaderPrefReadLock(&rw);
    fprintf(file, "Reading,Number-of-readers-present:[%d]\n", rw.readCount);
    FILE *sharedFile = fopen("shared-file.txt", "r");
    char c;
    while ((c = fgetc(sharedFile)) != EOF)
    {
        // reading file
    }
    fclose(sharedFile);
    rwLockReaderPrefReadUnlock(&rw);
    return NULL;
}

/**
 * Writer thread function
 * @param arg the argument to the writer
 *
 * @return void
 */
void *writer(void *arg)
{
    // your code here
    rwLockReaderPrefWriteLock(&rw);
    fprintf(file, "Writing,Number-of-readers-present:[%d]\n", rw.readCount);
    FILE *sharedFile = fopen("shared-file.txt", "a");
    fprintf(sharedFile, "Hello World!\n");
    fclose(sharedFile);
    rwLockReaderPrefWriteUnlock(&rw);
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

    // initialize reader-writer lock and output file
    rwLockReaderPrefInit(&rw);
    file = fopen("output-reader-pref.txt", "w");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return 1;
    }

    // Create reader and writer threads
    for (int i = 0; i < n; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (int i = 0; i < m; i++)
        pthread_create(&writers[i], NULL, writer, NULL);

    // Wait for all threads to complete
    for (int i = 0; i < n; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++)
        pthread_join(writers[i], NULL);

    // clean up
    fclose(file);
    rwLockReaderPrefDestroy(&rw);
    return 0;
}