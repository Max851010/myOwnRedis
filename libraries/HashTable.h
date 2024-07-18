#pragma once

#include <stddef.h>
#include <stdint.h>

// Hash table node
struct hashTableNode {
  hashTableNode *next = NULL;
  uint64_t hash_value = 0; // cached hash value
};

// Fixed-size hash table
struct hashTable {
  hashTableNode **table = NULL;
  size_t mask = 0;
  size_t size = 0;
};

// The real hash table interface
// Uses two hash tables for progressive resizing
struct hashMap {
  hashTable current_HT;
  hashTable previous_HT;
  size_t resizing_pos = 0;
};

hashTableNode *HMLookup(hashMap *HMap, hashTableNode *key,
                        bool (*eq)(hashTableNode *, hashTableNode *));
void HMInsert(hashMap *HMap, hashTableNode *HTNode);
hashTableNode *HMPop(hashMap *HMap, hashTableNode *key,
                     bool (*eq)(hashTableNode *, hashTableNode *));
size_t HMSize(hashMap *HMap);
void HMDestroy(hashMap *HMap);
