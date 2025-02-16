#ifndef HASH_TABLE
#define HASH_TABLE

#include <stdlib.h>
#include <stdbool.h>

#define MAX_MSG_SIZE 2048

typedef struct
{
    uint8_t *key;
    uint8_t *value;
} Node;

typedef struct
{
    size_t size;
    uint8_t *values;
} HashTable;

Node getFromHahTable(Node* node);
int insertIntoHashTable(Node* node);
int deleteFromHashTable(Node* node);
int editFromHashTable(Node* node);
int hash(uint8_t key);
bool init_hash_table(Node *nodes, size_t size);

#endif