/**
 * @file offline_schedulers.h
 * @brief Offline scheduling algorithms for process execution
 *
 * This file contains the implementation of offline scheduling algorithms for process execution.
 * The following algorithms are implemented:
 * 1. First Come First Serve (FCFS)
 * 2. Round Robin (RR)
 * 3. Multi Level Feedback Queue (MLFQ)
 *
 * @author Tamhane Vedant
 * @entrynumber 2021MT10898
 * @date 2021-08-31
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// Process struct
typedef struct
{
    char *command;

    bool finished;
    bool error;
    bool started;
    uint64_t arrival_time;
    uint64_t start_time;
    uint64_t context_start_time;
    uint64_t context_end_time;
    uint64_t completion_time;
    uint64_t burst_time;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    int process_id;

} Process;

// Queue node struct
typedef struct queue_node
{
    Process *process;
    struct queue_node *next;
} QueueNode;

// Queue struct
typedef struct
{
    QueueNode *head;
    QueueNode *tail;
    int size;
} Queue;

// Function prototypes
// scheduling algorithms
void FCFS(Process p[], int n);
void RoundRobin(Process p[], int n, int quantum);
void MultiLevelFeedbackQueue(Process p[], int n, int quantum0, int quantum1, int quantum2, int boostTime);
// utils
void initializeProcess(Process *p, int n);
uint64_t getCurrentTimeInMilliseconds();
void writeHeaderToCSV(char *filename);
void writeProcessToCSV(Process p, char *filename);
// process exec utils
char **parseCommand(char *input);
void executeProcess(Process *p, uint64_t t0);
void pauseProcess(Process *p, uint64_t t0);
void resumeProcess(Process *p, uint64_t t0);
// dsa utils
void enqueue(Queue *q, Process *p);
Process *dequeue(Queue *q);

/**
 * First Come First Serve (FCFS) Scheduling Algorithm
 * @param p: Array of Process structs
 * @param n: Number of processes
 *
 * @return void
 */
void FCFS(Process p[], int n)
{
    writeHeaderToCSV("result_offline_FCFS.csv");
    uint64_t t0 = getCurrentTimeInMilliseconds();
    initializeProcess(p, n);
    for (int i = 0; i < n; i++)
    {
        executeProcess(&p[i], t0);
        int status;
        int result = waitpid(p[i].process_id, &status, 0);
        if (result < 0)
        {
            printf("[ERROR]: Waitpid failed\n");
            continue;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            p[i].finished = true;
            p[i].completion_time = getCurrentTimeInMilliseconds() - t0;
            p[i].context_end_time = p[i].completion_time;
            p[i].burst_time = p[i].completion_time - p[i].start_time;
            p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
            p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
            p[i].response_time = p[i].start_time - p[i].arrival_time;
        }
        else
        { // handle error
            p[i].error = true;
            p[i].context_end_time = getCurrentTimeInMilliseconds() - t0;
            p[i].burst_time = p[i].context_end_time - p[i].context_start_time;
            p[i].response_time = p[i].start_time - p[i].arrival_time;
            printf("Error in process %d\n", i);
        }
        writeProcessToCSV(p[i], "result_offline_FCFS.csv");
        printf("%s|%lu|%lu\n", p[i].command, p[i].context_start_time, p[i].context_end_time);
    }
}

/**
 * Round Robin Scheduling Algorithm
 * @param p: Array of Process structs
 * @param n: Number of processes
 * @param quantum: Time slice for each process
 *
 * @return void
 */
void RoundRobin(Process p[], int n, int quantum)
{
    writeHeaderToCSV("result_offline_RR.csv");
    uint64_t t0 = getCurrentTimeInMilliseconds();
    initializeProcess(p, n);
    int i = 0;
    int remaining_processes = n;
    while (remaining_processes > 0)
    {
        if (p[i].finished == true || p[i].error == true)
        { // skip finished or errored processes
            i = (i + 1) % n;
            continue;
        }
        if (p[i].started == false)
        {
            executeProcess(&p[i], t0);
        }
        else
        {
            resumeProcess(&p[i], t0);
        }

        // sleep scheduler for quantum milliseconds
        usleep(quantum * 1000);

        // check if process has finished
        int status;
        pid_t result = waitpid(p[i].process_id, &status, WNOHANG);
        if (result < 0)
        {
            printf("[ERROR]: Waitpid failed");
            break;
        }
        else if (result == 0)
        { // process is still running
            pauseProcess(&p[i], t0);
        }
        else
        { // process has finished
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            { // no error
                p[i].finished = true;
                p[i].completion_time = getCurrentTimeInMilliseconds() - t0;
                p[i].context_end_time = p[i].completion_time;
                p[i].burst_time += p[i].context_end_time - p[i].context_start_time;
                p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
                p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
                p[i].response_time = p[i].start_time - p[i].arrival_time;
            }
            else
            { // handle error case
                p[i].error = true;
                p[i].context_end_time = getCurrentTimeInMilliseconds() - t0;
                p[i].response_time = p[i].start_time - p[i].arrival_time;
                printf("Error in process %s\n", p[i].command);
            }
            remaining_processes--;
            writeProcessToCSV(p[i], "result_offline_RR.csv");
        }
        // print context switch
        printf("%s|%lu|%lu\n", p[i].command, p[i].context_start_time, p[i].context_end_time);
        i = (i + 1) % n;
    }
}

/**
 * Multi Level Feedback Queue Scheduling Algorithm
 * @param p: Array of Process structs
 * @param n: Number of processes
 * @param quantum0: Time slice for high priority queue
 * @param quantum1: Time slice for medium priority queue
 * @param quantum2: Time slice for low priority queue
 * @param boostTime: Time after which all processes are promoted to the high priority queue
 *
 * @return void
 */
void MultiLevelFeedbackQueue(Process p[], int n, int quantum0, int quantum1, int quantum2, int boostTime)
{
    writeHeaderToCSV("result_offline_MLFQ.csv");
    uint64_t t0 = getCurrentTimeInMilliseconds();
    Queue MLFQ[3];
    for (int i = 0; i < 3; i++)
    { // initialize queues
        MLFQ[i].head = NULL;
        MLFQ[i].tail = NULL;
        MLFQ[i].size = 0;
    }

    initializeProcess(p, n);
    for (int i = 0; i < n; i++)
    { // enqueue all processes to high priority queue
        enqueue(&MLFQ[0], &p[i]);
    }

    int Quantum[3] = {quantum0, quantum1, quantum2};
    int remaining_processes = n;
    int current_queue = 0;
    uint64_t last_boost_time = t0;
    while (remaining_processes > 0)
    {
        Process *p = dequeue(&MLFQ[current_queue]);
        if (p == NULL)
        {
            current_queue = (current_queue + 1) % 3;
            continue;
        }
        if (p->finished == true || p->error == true)
        { // skip finished or errored processes
            continue;
        }
        if (p->started == false)
        {
            executeProcess(p, t0);
        }
        else
        {
            resumeProcess(p, t0);
        }
        // sleep scheduler for quantum milliseconds
        usleep(Quantum[current_queue] * 1000);

        // check if process has finished
        int status;
        pid_t result = waitpid(p->process_id, &status, WNOHANG);
        if (result < 0)
        {
            printf("[ERROR]: Waitpid failed");
            break;
        }
        else if (result == 0)
        { // process is still running
            pauseProcess(p, t0);
            // demote process to next queue
            if (current_queue < 2)
            {
                enqueue(&MLFQ[current_queue + 1], p);
            }
            else
            {
                enqueue(&MLFQ[current_queue], p);
            }
        }
        else
        { // process has finished
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            { // no error
                p->finished = true;
                p->completion_time = getCurrentTimeInMilliseconds() - t0;
                p->context_end_time = p->completion_time;
                p->burst_time += p->context_end_time - p->context_start_time;
                p->turnaround_time = p->completion_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                p->response_time = p->start_time - p->arrival_time;
            }
            else
            { // handle error case
                p->error = true;
                p->context_end_time = getCurrentTimeInMilliseconds() - t0;
                p->response_time = p->start_time - p->arrival_time;
                printf("Error in process - %s\n", p->command);
            }
            remaining_processes--;
            writeProcessToCSV(*p, "result_offline_MLFQ.csv");
        }
        // print context switch
        printf("%s|%lu|%lu\n", p->command, p->context_start_time, p->context_end_time);
        // check for boost
        if (getCurrentTimeInMilliseconds() - last_boost_time >= boostTime)
        {
            last_boost_time = getCurrentTimeInMilliseconds();
            for (int i = 1; i < 3; i++)
            {
                Process *p = dequeue(&MLFQ[i]);
                while (p != NULL)
                {
                    enqueue(&MLFQ[0], p);
                    p = dequeue(&MLFQ[i]);
                }
            }
        }
    }
}

/**
 * Initialize process parameters to default values
 * @param p: Process struct
 * @param n: Number of processes
 *
 * @return void
 */
void initializeProcess(Process *p, int n)
{
    for (int i = 0; i < n; i++)
    {
        p[i].finished = false;
        p[i].error = false;
        p[i].started = false;
        p[i].arrival_time = 0;
        p[i].start_time = 0;
        p[i].context_start_time = 0;
        p[i].context_end_time = 0;
        p[i].completion_time = 0;
        p[i].burst_time = 0;
        p[i].turnaround_time = 0;
        p[i].waiting_time = 0;
        p[i].response_time = 0;
        p[i].process_id = -1;
    }
}

/**
 * Get current time in milliseconds
 *
 * @return uint64_t: Current time in milliseconds
 */
uint64_t getCurrentTimeInMilliseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    return time_in_mill;
}

/**
 * Write header to CSV file
 * @param filename: Name of the CSV file
 *
 * @return void
 */
void writeHeaderToCSV(char *filename)
{
    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        printf("[ERROR]: File opening failed\n");
        return;
    }
    // header line
    fprintf(f, "Command, Finished, Error, Burst Time (in ms), Turnaround Time (in ms), Waiting Time (in ms), Response Time (in ms)\n");
    fclose(f);
}

/**
 * Write process parameters to CSV file
 * @param p: Process struct
 * @param filename: Name of the CSV file
 *
 * @return void
 */
void writeProcessToCSV(Process p, char *filename)
{
    FILE *f = fopen(filename, "a");
    if (f == NULL)
    {
        printf("[ERROR]: File opening failed\n");
        return;
    }
    fprintf(f, "%s, %s, %s, %lu, %lu, %lu, %lu\n",
            p.command,
            p.finished ? "Yes" : "No",
            p.error ? "Yes" : "No",
            p.burst_time,
            p.turnaround_time,
            p.waiting_time,
            p.response_time);
    fclose(f);
}

/**
 * Execute process by fork and execvp
 * @param p: Process struct
 * @param t0: Start time of the scheduler
 *
 * @return void
 */
void executeProcess(Process *p, uint64_t t0)
{
    int pid = fork();
    if (pid < 0)
    {
        printf("[ERROR]: Fork failed\n");
    }
    else if (pid == 0)
    {
        // Child process
        char **tokens = parseCommand(p->command);
        if (tokens == NULL)
        { // kill the child process if command is invalid
            exit(0);
        }
        if (execvp(tokens[0], tokens))
        {
            printf("[ERROR]: Command execution failed\n");
            exit(1);
        }
    }
    else
    {
        // Parent process
        p->process_id = pid;
        p->started = true;
        p->start_time = getCurrentTimeInMilliseconds() - t0;
        p->context_start_time = p->start_time;
    }
}

/**
 * Pause process
 * @param p: Process struct
 * @param t0: Start time of the scheduler
 *
 * @return void
 */
void pauseProcess(Process *p, uint64_t t0)
{
    if (p->started == false || p->finished == true)
    {
        return;
    }
    if (kill(p->process_id, SIGSTOP) < 0)
    {
        printf("[ERROR]: Pausing process failed\n");
    }
    p->context_end_time = getCurrentTimeInMilliseconds() - t0;
    p->burst_time += p->context_end_time - p->context_start_time;
}

/**
 * Resume process
 * @param p: Process struct
 * @param t0: Start time of the scheduler
 *
 * @return void
 */
void resumeProcess(Process *p, uint64_t t0)
{
    if (p->started == false || p->finished == true)
    {
        return;
    }
    if (kill(p->process_id, SIGCONT) < 0)
    {
        printf("[ERROR]: Resuming process failed\n");
    }
    p->context_start_time = getCurrentTimeInMilliseconds() - t0;
}

/**
 * Parse command string into tokens for execvp
 * @param input: Command string
 *
 * @return char**: Array of tokens
 */
char **parseCommand(char *input)
{
    int tokensSize = 8;
    char **tokens = calloc(tokensSize, sizeof(char *));
    if (tokens == NULL)
    {
        printf("Invalid Command\n");
        return NULL;
    }
    int defaultTokenSize = 16, curTokenSize = defaultTokenSize;
    char *token = malloc(curTokenSize * sizeof(char));
    int posInput = 0, posToken = 0, posTokens = 0;
    while (input[posInput] != '\0')
    {
        if (posToken >= curTokenSize)
        { // realloc token
            curTokenSize *= 2;
            token = realloc(token, curTokenSize);
            if (token == NULL)
            {
                printf("Invalid Command\n");
                return NULL;
            }
        }
        if (posTokens >= tokensSize)
        { // realloc tokens
            tokensSize *= 2;
            tokens = realloc(tokens, tokensSize * sizeof(char *));
            if (tokens == NULL)
            {
                printf("Invalid Command\n");
                return NULL;
            }
        }
        if (input[posInput] == ' ' || input[posInput] == '\t')
        { // if space, null terminate the token and add to tokens
            if (posToken == 0)
            { // handle multiple spaces
                posInput++;
                continue;
            }
            token[posToken] = '\0';
            tokens[posTokens++] = token;
            posToken = 0;
            curTokenSize = defaultTokenSize;
            token = malloc(sizeof(char) * curTokenSize);
            posInput++;
        }
        else
        {
            token[posToken++] = input[posInput++]; // add character to token
        }
    }
    if (posToken == 0)
    { // check for empty token
        free(token);
    }
    if (posToken > 0)
    { // check for last token
        if (posToken >= curTokenSize)
        {
            curTokenSize++;
            token = realloc(token, curTokenSize);
            if (token == NULL)
            {
                printf("Invalid Command\n");
                return NULL;
            }
        }
        token[posToken] = '\0';
        tokens[posTokens++] = token;
    }
    // null terminate the tokens array
    if (posTokens >= tokensSize)
    {
        tokensSize++;
        tokens = realloc(tokens, tokensSize * sizeof(char *));
        if (tokens == NULL)
        {
            printf("Invalid Command\n");
            return NULL;
        }
    }
    tokens[posTokens] = NULL;
    return tokens;
}

/**
 * Enqueue process to end of queue
 * @param q: Queue struct
 * @param p: Process struct
 *
 * @return void
 */
void enqueue(Queue *q, Process *p)
{
    QueueNode *newNode = (QueueNode *)malloc(sizeof(QueueNode));
    newNode->process = p;
    newNode->next = NULL;
    if (q->head == NULL)
    {
        q->head = newNode;
        q->tail = newNode;
    }
    else
    {
        q->tail->next = newNode;
        q->tail = newNode;
    }
    q->size++;
}

/**
 * Dequeue process from front of queue
 * @param q: Queue struct
 *
 * @return Process*: Process struct
 */
Process *dequeue(Queue *q)
{
    if (q->head == NULL)
    {
        return NULL;
    }
    QueueNode *temp = q->head;
    Process *p = temp->process;
    q->head = q->head->next;
    if (q->head == NULL)
    {
        q->tail = NULL;
    }
    free(temp);
    q->size--;
    return p;
}
