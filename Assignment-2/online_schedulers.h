/**
 * @file online_schedulers.h
 * @brief Header file for online schedulers implementation
 *
 * This file contains the implementation of online scheduling algorithms along with utility functions
 * for process execution and data structures.
 * The online scheduling algorithms implemented are:
 * 1. Shortest Job First (SJF)
 * 2. Multi-Level Feedback Queue (MLFQ)
 *
 * @author Tamhane Vedant
 * @entrynumber 2021MT10898
 * @date 2021-08-31
 *
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

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
typedef struct queueNode
{
    Process *process;
    struct queueNode *next;
} QueueNode;

// Queue struct
typedef struct
{
    QueueNode *head;
    QueueNode *tail;
    int size;
} Queue;

// Hash table node struct
typedef struct hashTableNode
{
    char *command;
    int hash;
    int sum_burst_time;
    int num_bursts;
    struct hashTableNode *next;
} HashTableNode;

// Cell struct
typedef struct
{
    HashTableNode *head;
    int size;
} Cell;

// Constants
int MOD = 1e9 + 7;
int HASH_PRIME = 131;
int NUM_CELLS = 3;

// Function prototypes
// scheduling algorithms
void ShortestJobFirst();
void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime);
// utils
uint64_t getCurrentTimeInMilliseconds();
void writeHeaderToCSV(char *filename);
void writeProcessToCSV(Process p, char *filename);
bool readInput(char *inputBuffer, int bufferSize);
// process exec utils
char **parseCommand(char *inputBuffer);
void executeProcess(Process *p, uint64_t t0);
void pauseProcess(Process *p, uint64_t t0);
void resumeProcess(Process *p, uint64_t t0);
// dsa utils
uint64_t hashFunction(char *command);
int getMLFQIndex(int *Quantum, Process *p, Cell *hashTable);
HashTableNode *findNode(Cell *hashTable, char *command);
uint64_t getExpectedBurstTime(Cell *hashTable, Process *p);
void updateBurstTime(Cell *hashTable, int burst_time, char *command);
void enqueue(Queue *q, Process *p);
Process *dequeue(Queue *q);

/**
 * Shortest Job First (SJF) Scheduling Algorithm
 *
 * @return void
 */
void ShortestJobFirst()
{
    writeHeaderToCSV("result_online_SJF.csv");
    // initialize hash table
    Cell hashTable[NUM_CELLS];
    for (int i = 0; i < NUM_CELLS; i++)
    {
        hashTable[i].head = NULL;
        hashTable[i].size = 0;
    }
    uint64_t t0 = getCurrentTimeInMilliseconds();
    int PROCESS_LIST_CAPACITY = 32;
    int numProcesses = 0;
    Process *processList = (Process *)malloc(sizeof(Process) * PROCESS_LIST_CAPACITY);
    if (processList == NULL)
    {
        printf("[ERROR]: Memory allocation failed\n");
        return;
    }
    int remainingProcesses = 0;
    char inputBuffer[1024];
    while (true)
    { // start the scheduler
        while (readInput(inputBuffer, 1024))
        { // check if input is available
            Process *p = (Process *)malloc(sizeof(Process));
            if (p == NULL)
            {
                printf("[ERROR]: Memory allocation failed\n");
                return;
            }
            // copy the command and remove the newline character
            char *cmd = (char *)malloc(sizeof(char) * strlen(inputBuffer));
            if (cmd == NULL)
            {
                printf("[ERROR]: Memory allocation failed\n");
                return;
            }
            for (int i = 0; i < strlen(inputBuffer); i++)
            {
                cmd[i] = inputBuffer[i];
            }
            cmd[strlen(inputBuffer) - 1] = '\0';
            p->command = cmd;
            p->arrival_time = getCurrentTimeInMilliseconds() - t0;
            p->finished = false;
            p->error = false;
            p->started = false;
            p->process_id = -1;
            p->burst_time = 0;
            p->turnaround_time = 0;
            p->waiting_time = 0;
            p->response_time = 0;
            if (numProcesses == PROCESS_LIST_CAPACITY)
            {
                PROCESS_LIST_CAPACITY *= 2;
                processList = realloc(processList, sizeof(Process) * PROCESS_LIST_CAPACITY);
                if (processList == NULL)
                {
                    printf("[ERROR]: Memory allocation failed\n");
                    return;
                }
            }
            processList[numProcesses] = *p;
            numProcesses++;
            remainingProcesses++;
        }
        if (remainingProcesses == 0)
        { // wait for 10ms for new processes to arrive
            usleep(10000);
            continue;
        }
        int minIndex = -1;
        uint64_t minBurstTime = 1e9;
        for (int i = 0; i < numProcesses; i++)
        {
            if (processList[i].finished || processList[i].error)
            {
                continue;
            }
            uint64_t expectedBurstTime = getExpectedBurstTime(hashTable, &processList[i]);
            if (expectedBurstTime < minBurstTime)
            {
                minBurstTime = expectedBurstTime;
                minIndex = i;
            }
        }
        if (minIndex == -1)
        { // should not happen
            printf("No process found\n");
            continue;
        }
        Process *p = &processList[minIndex];
        executeProcess(p, t0);
        int status;
        int result = waitpid(p->process_id, &status, 0);
        if (result < 0)
        {
            printf("[ERROR]: Waitpid failed\n");
        }
        else
        {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            {
                p->finished = true;
                p->completion_time = getCurrentTimeInMilliseconds() - t0;
                p->context_end_time = p->completion_time;
                p->burst_time = p->context_end_time - p->context_start_time;
                p->turnaround_time = p->completion_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                p->response_time = p->start_time - p->arrival_time;
                updateBurstTime(hashTable, p->burst_time, p->command);
            }
            else
            {
                p->error = true;
                p->context_end_time = getCurrentTimeInMilliseconds() - t0;
                p->burst_time = p->context_end_time - p->context_start_time;
                p->response_time = p->start_time - p->arrival_time;
            }
            remainingProcesses--;
            // write to csv
            writeProcessToCSV(*p, "result_online_SJF.csv");
            // print context switch
            printf("%s|%lu|%lu\n", p->command, p->context_start_time, p->context_end_time);
            fflush(stdout); // flush stdout for online scheduler
        }
    }
}

/**
 * Multi-Level Feedback Queue (MLFQ) Scheduling Algorithm
 * @param quantum0 Quantum for the first queue
 * @param quantum1 Quantum for the second queue
 * @param quantum2 Quantum for the third queue
 * @param boostTime Time after which all processes are moved to the first queue
 *
 * @return void
 */
void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime)
{
    writeHeaderToCSV("result_online_MLFQ.csv");
    // initialize hash table
    Cell hashTable[NUM_CELLS];
    for (int i = 0; i < NUM_CELLS; i++)
    {
        hashTable[i].head = NULL;
        hashTable[i].size = 0;
    }
    uint64_t t0 = getCurrentTimeInMilliseconds();
    // initialize MLFQ queues
    Queue MLFQ[3];
    for (int i = 0; i < 3; i++)
    {
        MLFQ[i].head = NULL;
        MLFQ[i].tail = NULL;
        MLFQ[i].size = 0;
    }
    int Quantum[3] = {quantum0, quantum1, quantum2};
    int remainingProcesses = 0;
    uint64_t lastBoostTime = t0;
    char inputBuffer[1024];
    while (true)
    { // start the scheduler
        while (readInput(inputBuffer, 1024))
        { // check if input is available
            Process *p = (Process *)malloc(sizeof(Process));
            if (p == NULL)
            {
                printf("[ERROR]: Memory allocation failed\n");
                return;
            }
            // copy the command manually and remove the newline character
            char *cmd = (char *)malloc(sizeof(char) * strlen(inputBuffer));
            if (cmd == NULL)
            {
                printf("[ERROR]: Memory allocation failed\n");
                return;
            }
            for (int i = 0; i < strlen(inputBuffer); i++)
            {
                cmd[i] = inputBuffer[i];
            }
            cmd[strlen(inputBuffer) - 1] = '\0';
            p->command = cmd;
            p->arrival_time = getCurrentTimeInMilliseconds() - t0;
            p->finished = false;
            p->error = false;
            p->started = false;
            p->process_id = -1;
            p->burst_time = 0;
            p->turnaround_time = 0;
            p->waiting_time = 0;
            p->response_time = 0;

            int index = getMLFQIndex(Quantum, p, hashTable);
            enqueue(&MLFQ[index], p);
            remainingProcesses++;
        }
        if (remainingProcesses == 0)
        { // wait for new processes to arrive
            usleep(1000);
            continue;
        }
        for (int i = 0; i < 3; i++)
        {
            if (MLFQ[i].size == 0)
            {
                continue;
            }
            Process *p = dequeue(&MLFQ[i]);
            if (p->started == false)
            {
                executeProcess(p, t0);
            }
            else
            {
                resumeProcess(p, t0);
            }

            // sleep scheduler for quantum milliseconds
            usleep(Quantum[i] * 1000);

            // check if process has finished
            int status;
            pid_t result = waitpid(p->process_id, &status, WNOHANG);
            if (result < 0)
            {
                printf("[ERROR]: Waitpid failed\n");
            }
            else if (result == 0)
            {
                // process is still running
                pauseProcess(p, t0);
                // demote process to lower queue
                if (i < 2)
                {
                    enqueue(&MLFQ[i + 1], p);
                }
                else
                {
                    enqueue(&MLFQ[i], p);
                }
            }
            else
            {
                // process has finished
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                { // no error
                    p->finished = true;
                    p->completion_time = getCurrentTimeInMilliseconds() - t0;
                    p->context_end_time = p->completion_time;
                    p->burst_time += p->context_end_time - p->context_start_time;
                    p->turnaround_time = p->completion_time - p->arrival_time;
                    p->waiting_time = p->turnaround_time - p->burst_time;
                    p->response_time = p->start_time - p->arrival_time;
                    updateBurstTime(hashTable, p->burst_time, p->command);
                }
                else
                { // handle error case
                    p->error = true;
                    p->context_end_time = getCurrentTimeInMilliseconds() - t0;
                    p->response_time = p->start_time - p->arrival_time;
                }
                remainingProcesses--;
                // write to csv
                writeProcessToCSV(*p, "result_online_MLFQ.csv");
            }
            // print context switch
            printf("%s|%lu|%lu\n", p->command, p->context_start_time, p->context_end_time);
            fflush(stdout); // flush stdout for online scheduler
            break;
        }

        // check for boost
        if (getCurrentTimeInMilliseconds() - lastBoostTime >= boostTime)
        {
            for (int i = 1; i < 3; i++)
            {
                while (MLFQ[i].size > 0)
                {
                    Process *p = dequeue(&MLFQ[i]);
                    enqueue(&MLFQ[0], p);
                }
            }
            lastBoostTime = getCurrentTimeInMilliseconds();
        }
    }
}

/**
 * Get the current time in milliseconds
 *
 * @return uint64_t Current time in milliseconds
 */
uint64_t getCurrentTimeInMilliseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    return time_in_mill;
}

/**
 * Write the header to the CSV file
 * @param filename Name of the CSV file
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
 * Write the process to the CSV file
 * @param p Process to be written to the CSV file
 * @param filename Name of the CSV file
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
 * Read input from the stdin
 * @param inputBuffer Buffer to store the input
 * @param bufferSize Size of the buffer
 *
 * @return bool True if input is available, False otherwise
 */
bool readInput(char *inputBuffer, int bufferSize)
{
    fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(STDIN_FILENO, &readFDs);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(STDIN_FILENO + 1, &readFDs, NULL, NULL, &timeout);
    if (ready == -1)
    {
        printf("[ERROR]: Select failed\n");
        return false;
    }
    else if (ready == 0)
    {
        return false;
    }
    else
    {
        if (FD_ISSET(STDIN_FILENO, &readFDs))
        {
            ssize_t bytesRead = read(STDIN_FILENO, inputBuffer, bufferSize - 1);
            if (bytesRead > 0)
            {
                inputBuffer[bytesRead] = '\0';
            }
            return true;
        }
        return false;
    }
}

/**
 * Parse command string into tokens for execvp
 * @param input: Command string
 *
 * @return char**: Array of tokens
 */
char **parseCommand(char *inputBuffer)
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
    while (inputBuffer[posInput] != '\0')
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
        if (inputBuffer[posInput] == ' ' || inputBuffer[posInput] == '\t')
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
            token[posToken++] = inputBuffer[posInput++]; // add character to token
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
 * Hash function for string
 * @param command Command for which the hash is to be found
 *
 * @return uint64_t Hash value
 */
uint64_t hashFunction(char *command)
{
    uint64_t sum = 5147;
    for (int i = strlen(command) - 1; i >= 0; i--)
    {
        sum = (sum * HASH_PRIME + command[i]) % MOD;
    }
    return sum;
}

/**
 * Get the index of the MLFQ queue for the given process based on history and return 1 if not found
 * @param Quantum Array of quantum values for the queues
 * @param p Process for which the index is to be found
 * @param hashTable Hash table to store the burst times
 *
 * @return int Index of the MLFQ queue
 */
int getMLFQIndex(int *Quantum, Process *p, Cell *hashTable)
{
    HashTableNode *node = findNode(hashTable, p->command);
    if (node == NULL)
    {
        return 1;
    }
    int avgBurstTime = node->sum_burst_time / node->num_bursts;
    if (avgBurstTime <= Quantum[0])
    {
        return 0;
    }
    if (avgBurstTime <= Quantum[1])
    {
        return 1;
    }
    return 2;
}

/**
 * Find the node in the hash table for the given command and return NULL if not found
 * @param hashTable Hash table to store the burst times
 * @param command Command for which the node is to be found
 *
 * @return HashTableNode* Node corresponding to the given command
 */
HashTableNode *findNode(Cell *hashTable, char *command)
{
    int hash = hashFunction(command);
    Cell *cell = &hashTable[hash % NUM_CELLS];
    if (cell->size == 0)
    {
        return NULL;
    }
    HashTableNode *current = cell->head;
    while (current != NULL)
    {
        if (strcmp(current->command, command) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Get the expected burst time for the given process from history and return 1000 if not found
 * @param hashTable Hash table to store the burst times
 * @param p Process for which the expected burst time is to be found
 *
 * @return uint64_t Expected burst time
 */
uint64_t getExpectedBurstTime(Cell *hashTable, Process *p)
{
    HashTableNode *node = findNode(hashTable, p->command);
    if (node == NULL)
    {
        return 1000;
    }
    return node->sum_burst_time / node->num_bursts;
}

/**
 * Update the burst time in the hash table
 * @param hashTable Hash table to store the burst times
 * @param burst_time Burst time to be updated
 * @param command Command for which the burst time is to be updated
 *
 * @return void
 */
void updateBurstTime(Cell *hashTable, int burst_time, char *command)
{
    int hash = hashFunction(command);
    Cell *cell = &hashTable[hash % NUM_CELLS];
    HashTableNode *node = findNode(hashTable, command);
    if (node == NULL)
    {
        HashTableNode *newNode = (HashTableNode *)malloc(sizeof(HashTableNode));
        newNode->command = strdup(command);
        newNode->sum_burst_time = burst_time;
        newNode->num_bursts = 1;
        newNode->next = cell->head;
        cell->head = newNode;
        cell->size++;
    }
    else
    {
        node->sum_burst_time += burst_time;
        node->num_bursts++;
    }
}

/**
 * Enqueue the process in the given queue
 * @param q Queue in which the process is to be enqueued
 * @param p Process to be enqueued
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
 * Dequeue the process from the given queue
 * @param q Queue from which the process is to be dequeued
 *
 * @return Process* Dequeued process
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
