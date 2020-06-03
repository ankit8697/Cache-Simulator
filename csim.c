// Name: Ankit Sanghi
// Carleton ID: 1988167

#define _GNU_SOURCE
#include "cachelab.h"
#include "unistd.h"
#include "stdbool.h"
#include <stdio.h>
#include "getopt.h"
#include <stdlib.h>
#include <string.h>

/* Command Line Argument Variables */
bool help = false;
bool verbose = false;
int s, e, b;
char *tracefile;

/* Structures required to simulate cache */
struct CacheLine
{
    int valid;
    // Least recently used state
    int LRUstate;
    int tag;
};

struct Cache
{
    int blockBit;
    int associativity;
    int setBits;
    int hits;
    int misses;
    int evictions;
    struct CacheLine **line;
} * cache;

/* Function to create a new cache (essentially a constructor) */
struct Cache *new(int setBits, int associativity, int blockBit)
{
    struct Cache *cache = (struct Cache *) malloc(sizeof(struct Cache));

    cache->setBits = setBits;
    cache->associativity = associativity;
    cache->blockBit = blockBit;

    int setSize = 1 << cache->setBits;

    cache->line = (struct CacheLine **) malloc(sizeof(struct CacheLine *) * setSize);

    // Create all lines in all sets
    for (int i = 0; i < setSize; ++i)
    {
        cache->line[i] = (struct CacheLine *) malloc(sizeof(struct CacheLine) * cache->associativity);
    }

    for (int i = 0; i < setSize; ++i)
        for (int j = 0; j < cache->associativity; ++j)
        {
            cache->line[i][j].valid = 0;
            cache->line[i][j].LRUstate = 0;
            cache->line[i][j].tag = 0;
        }
    return cache;
}

/* Deletes the cache, including all CacheLines */
void delete(struct Cache *cache)
{
    int setSize = 1 << cache->setBits;
    int i;
    for (i = 0; i < setSize; ++i)
    {
        free(cache->line[i]);
    }
    free(cache->line);
    free(cache);
}

/* Returns the position of the least recently used item in a cache line */
int getLRU(struct Cache *cache, int index)
{
    int i, position = 0;
    for (i = 0; i < cache->associativity; i++)
    {
        if (cache->line[index][i].LRUstate < cache->line[index][position].LRUstate) 
        {
            position = i;
        }
    }
    for (i = 0; i < cache->associativity; i++)
    {
        cache->line[index][i].LRUstate--;
    }
    cache->line[index][position].LRUstate = cache->associativity;
    return position;
}

void update(struct Cache *cache, int index, int position)
{
    for (int i = 0; i < cache->associativity; i++)
        cache->line[index][i].LRUstate--;
    cache->line[index][position].LRUstate = cache->associativity;
}

void accessCache(struct Cache *cache, long long address, bool verbose)
{
    int index = (int)((address >> cache->blockBit) & ((1LL << cache->setBits) - 1));
    int tag = (int)(address >> (cache->blockBit + cache->setBits));
    for (int i = 0; i < cache->associativity; i++)
    {
        if (cache->line[index][i].tag == tag && cache->line[index][i].valid)
        {
            // Hit
            cache->hits++;
            update(cache, index, i);
            if (verbose)
                printf("hit ");
            return;
        }
    }
    // Miss
    cache->misses++;
    if (verbose)
        printf("miss ");
    int LRU = getLRU(cache, index);
    if (cache->line[index][LRU].valid)
    {
        // Evict
        cache->evictions++;
        if (verbose)
            printf("eviction ");
    }
    cache->line[index][LRU].tag = tag;
    cache->line[index][LRU].valid = 1;
}

int setCommandLineArgs(int argc, char **argv) {
    int c;
    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (c)
        {

        case 'h':
            help = true;
            printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
            printf("Options:\n");
            printf("  -h         printf this help message.\n");
            printf("  -v         Optional verbose flag.\n");
            printf("  -s <num>   Number of set index bits.\n");
            printf("  -E <num>   Number of lines per set.\n");
            printf("  -b <num>   Number of block offset bits.\n");
            printf("  -t <file>  Trace file.\n\n");
            printf("Examples:\n");
            printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
            printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
            return 0;
            break;

        case 'v':
            verbose = true;
            break;

        case 's':
            s = atoi(optarg);
            break;

        case 'E':
            e = atoi(optarg);
            break;

        case 'b':
            b = atoi(optarg);
            break;

        case 't':
            tracefile = optarg;
            break;
        }
        cache = new(s, e, b);
    }
    return 1;
}

void simulateCacheOnFile(FILE *input)
{
    char instruction;

    while (fscanf(input, "%c", &instruction) != EOF)
    {
        // File reading code taken from StackOverflow. Unsure why it works but it does. Modified it to make it read traces
        while (instruction <= ' ' && fscanf(input, "%c", &instruction) != EOF) 
        {
            if (feof(input))
            {
                return;
            }
            long long address;
            int length;
            fscanf(input, "%llx,%d", &address, &length);
            if (verbose && instruction != 'I')
            {
                printf("%c %llx,%d ", instruction, address, length);
            }
            switch (instruction)
            {
            case 'L':
                accessCache(cache, address, verbose);
                break;
            case 'S':
                accessCache(cache, address, verbose);
                break;
            case 'M':
                accessCache(cache, address, verbose);
                accessCache(cache, address, verbose);
                break;
            }
            if (verbose && instruction != 'I')
            {
                printf("\n");
            } 
        }
    }
    fclose(input);
}

int main(int argc, char** argv) 
{
    setCommandLineArgs(argc, argv);

    FILE *fp = fopen(tracefile, "r");
    if (fp == NULL)
    {
        printf("File not found\n");
        exit(EXIT_FAILURE);
    }

    simulateCacheOnFile(fp);
    
    printSummary(cache->hits, cache->misses, cache->evictions);
    return 0;
}