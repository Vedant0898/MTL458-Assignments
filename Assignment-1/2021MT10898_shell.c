/**
 * @file 2021MT10898_shell.c
 * @brief This file contains the implementation of a shell program.
 *
 * This program allows the user to execute shell commands and provides a basic shell environment.
 * It supports various features such as command execution, history and piping.
 *
 * @author Tamhane Vedant
 * @entryNumber 2021MT10898
 * @date 1st August 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Function declarations

char *getInput();
char **splitInput(char *input);
void freeStringArray(char **arr);
void addToHistory(char *input);
void displayHistory();
int handleCDCommand(char **tokens);
int handleHistoryCommand(char **tokens);
int getPipeIndex(char **tokens);
int handlePipeCommand(char **tokens);
void executeCommand(char **tokens);

// Global variables
char **HISTORY;
int HISTORY_SIZE = 0;
int HISTORY_CAPACITY = 16;

const char *HOME = "HOME";
char *OLD_PWD = NULL;

int main(int argc, char *argv[])
{
    HISTORY = calloc(HISTORY_CAPACITY, sizeof(char *)); // Initialize history array
    while (1)
    {
        printf("MTL458 >");
        char *input = getInput();
        if (input == NULL)
        { // Invalid input
            continue;
        }
        char **tokens = splitInput(input);
        if (tokens == NULL)
        { // Invalid tokens
            free(input);
            continue;
        }

        if (tokens[0] == NULL)
        { // empty command
            free(input);
            free(tokens);
            continue;
        }
        addToHistory(input); // Add command to history

        if (strcmp(tokens[0], "exit") == 0)
        { // exit command
            if (tokens[1] != NULL)
            {
                printf("Invalid Command\n");
                freeStringArray(tokens); // Free tokens
                continue;
            }
            freeStringArray(tokens); // Free tokens
            break;
        }

        executeCommand(tokens);  // Execute command
        freeStringArray(tokens); // Free tokens
    }
    // Free history
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        free(HISTORY[i]);
    }
    free(HISTORY);
    if (OLD_PWD != NULL)
    {
        free(OLD_PWD);
    }

    return 0;
}

/**
 * Get input from user
 *
 * @return input string
 */
char *getInput()
{
    int inputBufferSize = 16; // Initial size of input buffer
    char *buffer = malloc(sizeof(char) * inputBufferSize);
    if (buffer == NULL)
    { // Check if buffer is allocated
        printf("Invalid Command\n");
        return NULL;
    }
    char c;
    int posInput = 0;
    while (1)
    {
        c = getchar();
        if (c == '\n' || c == EOF)
        {
            buffer[posInput] = '\0'; // Null terminate the string
            return buffer;
        }
        buffer[posInput++] = c;
        if (posInput >= inputBufferSize)
        { // Reallocate buffer if input size exceeds buffer size
            inputBufferSize *= 2;
            buffer = realloc(buffer, inputBufferSize * sizeof(char));
            if (buffer == NULL)
            { // Check if buffer is allocated
                printf("Invalid Command\n");
                return NULL;
            }
        }
    }
}

/**
 * Split input into tokens
 *
 * @param input: command input received by the shell
 * @return array of tokens
 */
char **splitInput(char *input)
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
 * Free string array
 *
 * @param arr: array of strings
 * @return void
 */
void freeStringArray(char **arr)
{
    for (int i = 0; arr[i] != NULL; i++)
    {
        free(arr[i]);
    }
    free(arr);
}

/**
 * Add command to history
 *
 * @param input: command input received by the shell
 * @return void
 */
void addToHistory(char *input)
{
    if (HISTORY_SIZE >= HISTORY_CAPACITY)
    {
        HISTORY_CAPACITY *= 2;
        HISTORY = realloc(HISTORY, sizeof(char *) * HISTORY_CAPACITY);
        if (HISTORY == NULL)
        {
            printf("Invalid Command\n");
            exit(EXIT_FAILURE);
        }
    }
    HISTORY[HISTORY_SIZE++] = input; // don't free input as it is added to history
}

/**
 * Display history
 *
 * @return void
 */
void displayHistory()
{
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        printf("%s\n", HISTORY[i]);
    }
}

/**
 * Handle builtin command cd
 *
 * @param tokens: array of tokens
 * @return 1 if it is a cd command, 0 otherwise
 */
int handleCDCommand(char **tokens)
{
    if (strcmp(tokens[0], "cd") == 0)
    {
        char *newDir = NULL;
        char *curDir = getcwd(NULL, 0);

        if (tokens[1] == NULL)
        {
            newDir = getenv(HOME);
        }
        else if (tokens[2] == NULL)
        {
            if (strcmp(tokens[1], "-") == 0)
            {
                newDir = OLD_PWD;
            }
            else if (strcmp(tokens[1], "~") == 0)
            {
                newDir = getenv(HOME);
            }
            else
            {
                newDir = tokens[1];
            }
        }

        // Change directory
        if (newDir == NULL)
        {
            printf("Invalid Command\n");
            free(curDir);
            return 1;
        }

        if (newDir[0] == '\"')
        { // string is enclosed in double quotes

            int len = 0; // calculate length of string
            while (newDir[len] != '\0')
            {
                len++;
            }
            if (newDir[len - 1] == '\"')
            {
                newDir[len - 1] = '\0';
                newDir++;
            }
        }

        if (chdir(newDir) != 0)
        {
            printf("Invalid Command\n");
            free(curDir);
        }
        else
        {
            if (OLD_PWD != NULL)
            {
                free(OLD_PWD);
            }
            OLD_PWD = curDir;
        }
        return 1;
    }
    return 0;
}

/**
 * Handle builtin command history
 *
 * @param tokens: array of tokens
 * @return 1 if it is a history command, 0 otherwise
 */
int handleHistoryCommand(char **tokens)
{
    if (strcmp(tokens[0], "history") == 0)
    {
        if (tokens[1] != NULL)
        { // multiple arguments provided
            printf("Invalid Command\n");
        }
        else
        {
            displayHistory();
        }
        return 1;
    }
    return 0;
}

/**
 * Get index of pipe in tokens
 *
 * @param tokens: array of tokens
 * @return index of pipe in tokens
 */
int getPipeIndex(char **tokens)
{
    for (int i = 0; tokens[i] != NULL; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            return i;
        }
    }
    return -1;
}

/**
 * Handle single pipe command
 *
 * @param tokens: array of tokens
 * @param pipeIndex: index of pipe in tokens
 * @return 1 if it is a pipe command, 0 otherwise
 */
int handlePipeCommand(char **tokens)
{
    int pipeIndex = getPipeIndex(tokens);
    if (pipeIndex == -1)
    {
        return 0;
    }
    // split tokens into two parts
    char *pipeString = tokens[pipeIndex];
    tokens[pipeIndex] = NULL;
    char **tokens1 = tokens;
    char **tokens2 = tokens + pipeIndex + 1;

    // handle pipe command
    int pipefd[2];
    pid_t pid1, pid2;
    if (pipe(pipefd) < 0)
    {
        printf("Invalid Command\n");
        return 1;
    }
    pid1 = fork();
    if (pid1 < 0)
    { // Fork failed
        printf("Invalid Command\n");
        return 1;
    }
    if (pid1 == 0)
    { // Child 1
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        if (handleHistoryCommand(tokens1))
        { // Check if it is a history command
            exit(EXIT_SUCCESS);
        }
        if (execvp(tokens1[0], tokens1) < 0)
        { // execvp returns -1 if failed
            printf("Invalid Command\n");
            exit(EXIT_FAILURE); // Exit child process
        }
    }
    else
    {
        pid2 = fork();
        if (pid2 < 0)
        { // Fork failed
            printf("Invalid Command\n");
            return 1;
        }

        if (pid2 == 0)
        { // Child 2
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            if (handleHistoryCommand(tokens2))
            { // Check if it is a history command
                exit(EXIT_SUCCESS);
            }
            if (execvp(tokens2[0], tokens2) < 0)
            { // execvp returns -1 if failed
                printf("Invalid Command\n");
                exit(EXIT_FAILURE); // Exit child process
            }
        }
        else
        { // Parent
            // Close both ends of the pipe
            close(pipefd[0]);
            close(pipefd[1]);
            // Wait for children to finish
            wait(NULL);
            wait(NULL);
            tokens[pipeIndex] = pipeString; // restore pipe string to assist in freeing tokens
            return 1;
        }
    }
}

/**
 * Execute command
 *
 * @param tokens: array of tokens
 * @return void
 */
void executeCommand(char **tokens)
{
    // check for builtin command
    if (handleCDCommand(tokens))
    {
        return;
    }
    // check for pipe command
    if (handlePipeCommand(tokens))
    {
        return;
    }

    // fork and exec for standard commands
    pid_t pid = fork();
    if (pid < 0)
    { // Fork failed
        printf("Invalid Command\n");
        return;
    }
    else if (pid == 0)
    { // Child process
        if (handleHistoryCommand(tokens))
        { // Check if it is a history command
            exit(EXIT_SUCCESS);
        }
        if (execvp(tokens[0], tokens))
        { // execvp returns -1 if failed
            printf("Invalid Command\n");
            exit(EXIT_FAILURE); // Exit child process
        }
    }
    else
    {               // Parent process
        wait(NULL); // Wait for child process to finish
    }
}
