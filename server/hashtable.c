#include "hashtable.h"
#include "fnv.c"
#include <stdint.h>

#define TABLE_SIZE 10000

size_t getIndex(const uint8_t *key)
{
    uint32_t hash = fnv1a_32(key, strlen(key));
    return hash % TABLE_SIZE;
}
uint8_t *getFromHahTable(uint8_t *key, HashTable *table)
{
    return table->values[getIndex(key)];
}
int insertIntoHashTable(Node *node, HashTable *table)
{
    size_t index = getIndex(node->key);
    table->values[index] = node->value;

    return 0;
}
int deleteFromHashTable(uint8_t *key, HashTable *table)
{
    size_t index = getIndex(key);
    table->values[0] = NULL;

    return 0;
}

bool init_hash_table(HashTable *hashTable)
{
    Node node;
    node.value = (uint8_t *)malloc(MAX_MSG_SIZE * sizeof(uint8_t));
    hashTable->size = TABLE_SIZE;
    hashTable->values = (Node *)malloc(TABLE_SIZE * sizeof(node));

    return true;
}
