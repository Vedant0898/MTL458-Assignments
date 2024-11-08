#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

#define BUFFER_SIZE 100 // size of buffer

// circular queue struct definition
typedef struct queue
{
    uint32_t arr[BUFFER_SIZE];
    int size;
    int front;
    int rear;
} circularQueue;

/**
 * Initialize the circular queue
 * @param q: pointer to the circular queue
 *
 * @return void
 */
void init(circularQueue *q)
{
    q->size = 0;
    q->front = 0;
    q->rear = 0;
}

/**
 * Enqueue data into the circular queue
 * @param q: pointer to the circular queue
 * @param data: data to be enqueued
 *
 * @return void
 */
void enqueue(circularQueue *q, uint32_t data)
{
    assert(q->size < BUFFER_SIZE);
    q->arr[q->rear] = data;
    q->rear = (q->rear + 1) % BUFFER_SIZE;
    q->size++;
}

/**
 * Dequeue data from the circular queue
 * @param q: pointer to the circular queue
 *
 * @return dequeued data
 */
uint32_t dequeue(circularQueue *q)
{
    assert(q->size > 0);
    uint32_t data = q->arr[q->front];
    q->front = (q->front + 1) % BUFFER_SIZE;
    q->size--;
    return data;
}

// Global variables

circularQueue buffer;
int done = 0;
int error = 0;
pthread_cond_t bufferEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferFilled = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Wrapper function for pthread_create
 * @param thread: pointer to the thread
 * @param attr: pointer to the thread attributes
 * @param start_routine: pointer to the function to be executed
 * @param arg: pointer to the arguments to be passed to the function
 *
 * @return void
 */

void Pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg)
{
    int rc = pthread_create(thread, attr, start_routine, arg);
    assert(rc == 0);
}

/**
 * Wrapper function for pthread_join
 * @param thread: thread to be joined
 * @param value_ptr: pointer to the return value of the thread
 *
 * @return void
 */
void Pthread_join(pthread_t thread, void **value_ptr)
{
    int rc = pthread_join(thread, value_ptr);
    assert(rc == 0);
}

/**
 * Wrapper function for pthread_mutex_lock
 * @param mutex: pointer to the mutex
 *
 * @return void
 */
void Pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int rc = pthread_mutex_lock(mutex);
    assert(rc == 0);
}

/**
 * Wrapper function for pthread_mutex_unlock
 * @param mutex: pointer to the mutex
 *
 * @return void
 */
void Pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    int rc = pthread_mutex_unlock(mutex);
    assert(rc == 0);
}

/**
 * Wrapper function for pthread_cond_wait
 * @param cond: pointer to the condition variable
 * @param mutex: pointer to the mutex
 *
 * @return void
 */
void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    int rc = pthread_cond_wait(cond, mutex);
    assert(rc == 0);
}

/**
 * Wrapper function for pthread_cond_signal
 * @param cond: pointer to the condition variable
 *
 * @return void
 */
void Pthread_cond_signal(pthread_cond_t *cond)
{
    int rc = pthread_cond_signal(cond);
    assert(rc == 0);
}

/**
 * Producer function
 * @param arg: pointer to the arguments
 *
 * @return void
 */
void *producer(void *arg)
{
    // read from file
    FILE *file = fopen("input-part1.txt", "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        error = 1;
        Pthread_cond_signal(&bufferFilled); // to wake up consumer
        return NULL;
    }
    uint32_t data;
    // read till data is not 0
    fscanf(file, "%u", &data);
    while (1)
    {
        Pthread_mutex_lock(&lock);
        while (buffer.size == BUFFER_SIZE && error == 0)
        {
            Pthread_cond_wait(&bufferEmpty, &lock);
        }
        if (error == 1)
        {
            Pthread_mutex_unlock(&lock);
            break;
        }
        if (data == 0)
        {
            done = 1;
            Pthread_cond_signal(&bufferFilled);
            Pthread_mutex_unlock(&lock);
            break;
        }
        enqueue(&buffer, data);
        Pthread_cond_signal(&bufferFilled);
        Pthread_mutex_unlock(&lock);

        fscanf(file, "%u", &data);
    }
    fclose(file);
    return NULL;
}

/**
 * Consumer function
 * @param arg: pointer to the arguments
 *
 * @return void
 */
void *consumer(void *arg)
{
    FILE *file = fopen("output-part1.txt", "w");
    if (file == NULL)
    {
        printf("Error opening file\n");
        error = 1;
        Pthread_cond_signal(&bufferEmpty); // to wake up producer
        return NULL;
    }
    while (1)
    {
        Pthread_mutex_lock(&lock);
        while (buffer.size == 0 && done == 0 && error == 0)
        {
            Pthread_cond_wait(&bufferFilled, &lock);
        }
        if (error == 1)
        {
            Pthread_mutex_unlock(&lock);
            break;
        }
        if (done == 1 && buffer.size == 0)
        {
            Pthread_mutex_unlock(&lock);
            break;
        }
        uint32_t data = dequeue(&buffer);
        fprintf(file, "Consumed:[%u],Buffer-State:[", data);
        int rem = buffer.size;
        for (int i = buffer.front; i != buffer.rear; i = (i + 1) % BUFFER_SIZE)
        {
            fprintf(file, "%u", buffer.arr[i]);
            rem--;
            if (rem != 0)
            {
                fprintf(file, ",");
            }
        }
        fprintf(file, "]\n");
        Pthread_cond_signal(&bufferEmpty);
        Pthread_mutex_unlock(&lock);
    }
    fclose(file);
    return NULL;
}

int main()
{
    init(&buffer); // initialize buffer

    // create producer and consumer threads
    pthread_t p1, p2;
    Pthread_create(&p1, NULL, producer, NULL);
    Pthread_create(&p2, NULL, consumer, NULL);

    // wait for threads to complete
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);

    return 0;
}