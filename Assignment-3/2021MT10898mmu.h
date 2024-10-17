#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// Constants
#define MMAP_THRESHOLD 8 * 1024     // 8KB
#define MUNMAP_THRESHOLD 128 * 1024 // 128KB
#define BLOCK_SIZE sizeof(BlockHeader)
#define HASH_CONST 0x9EA759B9

/**
 * Struct for Block Header
 */
typedef struct BlockHeader
{
    size_t size;
    bool isFree;
    size_t hashCode;
    struct BlockHeader *next;

} BlockHeader;

// Global free list
BlockHeader *freeList = NULL;

/**
 * Get Hash Value for a pointer
 * @param ptr void pointer
 *
 * @return size_t hash value
 */
size_t getHashValue(void *ptr)
{
    return (size_t)((uintptr_t)ptr ^ HASH_CONST);
}

/**
 * Split block into two blocks
 * @param block BlockHeader pointer
 * @param size size_t size
 *
 * @return void
 */
void splitBlock(BlockHeader *block, size_t size)
{
    BlockHeader *newBlock = (BlockHeader *)((char *)block + size + BLOCK_SIZE);
    newBlock->size = block->size - size - BLOCK_SIZE;
    newBlock->isFree = true;
    newBlock->hashCode = getHashValue(newBlock);
    newBlock->next = block->next;
    block->size = size;
    block->next = newBlock;
}

/**
 * Coalesce adjacent free blocks in free list
 *
 * @return void
 */
void coalesceBlocks()
{
    BlockHeader *temp = freeList;
    while (temp != NULL && temp->next != NULL)
    {
        if ((char *)temp + temp->size + BLOCK_SIZE == (char *)temp->next)
        {
            temp->size += temp->next->size + BLOCK_SIZE;
            temp->next = temp->next->next;
            continue;
        }
        temp = temp->next;
    }
}

/**
 * Insert block in free list in sorted order of address
 * @param block BlockHeader pointer to be inserted
 *
 * @return void
 */
void insertBlock(BlockHeader *block)
{
    if (freeList == NULL)
    {
        freeList = block;
        return;
    }
    if ((char *)block < (char *)freeList)
    {
        block->next = freeList;
        freeList = block;
        return;
    }
    BlockHeader *temp = freeList;
    while (temp->next != NULL && (char *)temp->next < (char *)block)
    {
        temp = temp->next;
    }
    block->next = temp->next;
    temp->next = block;
}

/**
 * Request large memory using mmap
 * @param size size_t size
 *
 * @return void pointer
 */
void *requestLargeMemory(size_t size)
{
    // get memory from mmap
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        return NULL;
    }
    BlockHeader *block = (BlockHeader *)ptr;
    block->size = size - BLOCK_SIZE;
    block->isFree = false;
    block->hashCode = getHashValue(block);
    block->next = NULL;
    return (void *)((char *)block + BLOCK_SIZE);
}

/**
 * Request small memory using sbrk
 * @param size size_t size
 *
 * @return void pointer
 */
void *requestSmallMemory(size_t size)
{
    // get memory from sbrk
    void *ptr = sbrk(0);
    void *request = sbrk(size);
    if (request == (void *)-1)
    {
        return NULL;
    }
    BlockHeader *block = (BlockHeader *)ptr;
    block->size = size - BLOCK_SIZE;
    block->isFree = false;
    block->hashCode = getHashValue(block);
    block->next = NULL;
    return (void *)((char *)block + BLOCK_SIZE);
}

/**
 * My Malloc Implementation
 * @param size size_t size
 *
 * @return void pointer
 */
void *my_malloc(size_t size)
{

    // check if size is 0
    if (size == 0)
    {
        return NULL;
    }

    // align size to a multiple of 8
    size = (size + 7) & ~7;

    BlockHeader *temp = freeList;
    BlockHeader *prev = NULL;

    // search the free list for first block that is large enough (First Fit)
    while (temp != NULL && temp->size < size)
    {
        prev = temp;
        temp = temp->next;
    }
    if (temp != NULL)
    {
        if (temp->size >= size + BLOCK_SIZE)
        {
            splitBlock(temp, size);
        }
        if (prev == NULL)
        {
            freeList = temp->next;
        }
        else
        {
            prev->next = temp->next;
        }
        temp->isFree = false;
        return (void *)((char *)temp + BLOCK_SIZE);
    }

    // if no block is found in free list, request memory from mmap or sbrk
    if (size + BLOCK_SIZE >= MMAP_THRESHOLD)
    {
        return requestLargeMemory(size + BLOCK_SIZE);
    }
    return requestSmallMemory(size + BLOCK_SIZE);
}

/**
 * My Calloc Implementation
 * @param nelem size_t nelem
 * @param size size_t size
 *
 * @return void pointer
 */
void *my_calloc(size_t nelem, size_t size)
{
    void *ptr = my_malloc(nelem * size); // get memory using my_malloc
    if (ptr == NULL)
    {
        return NULL;
    }
    memset(ptr, 0, nelem * size); // zero out the memory
    return ptr;
}

/**
 * My Free Implementation
 * @param ptr void pointer
 *
 * @return void
 */
void my_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    BlockHeader *block = (BlockHeader *)((char *)ptr - BLOCK_SIZE);
    if (block->isFree)
    {
        return;
    }
    block->isFree = true;
    // check hashCode
    if (block->hashCode != getHashValue(block))
    {
        perror("Invalid memory passed to free\n");
        return;
    }

    // check if block is large enough to be munmapped
    if (block->size + BLOCK_SIZE >= MUNMAP_THRESHOLD)
    { // release large chunk of memory using munmap
        if (munmap(block, block->size + BLOCK_SIZE) == -1)
        {
            perror("munmap failed\n");
        }
        return;
    }

    insertBlock(block);
    coalesceBlocks();
}
