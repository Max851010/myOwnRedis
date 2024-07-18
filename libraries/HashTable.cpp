#include "HashTable.h"
#include <assert.h>
#include <stdlib.h>

static void HTInit(hashTable *HTable, size_t n) {
  // n must be power of 2
  // number of buckets being a power of two for efficient index computation
  // using bitwise operations.
  assert(n > 0 && ((n & (n - 1)) == 0));
  // The value of this memory block will be initialized as NULL or 0
  // Dynamically allocate a space for the pointers of hashTableNode, so we can
  // see it as an array of pointers of hashTableNode
  /**
    table (hashTableNode **):
    +-----------+-----------+-----------+-----------+------------
    | table[0]  | table[1]  | table[2]  |    ...    |  table[n-1]
    +-----------+-----------+-----------+-----------+------------
  **/
  HTable->table = (hashTableNode **)calloc(sizeof(hashTableNode *), n);
  HTable->mask = n - 1;
  HTable->size = 0;
}

// Hash table insertion
static void HTInsert(hashTable *HTable, hashTableNode *HTNode) {
  size_t pos = HTNode->hash_value & HTable->mask;
  // Current head of the bucket
  hashTableNode *currentHead = HTable->table[pos];
  HTNode->next = currentHead;
  HTable->table[pos] = HTNode;
  HTable->size++;
}

// Lookup a node in the hash table
static hashTableNode **HTLookup(hashTable *HTable, hashTableNode *HTNode,
                                bool (*eq)(hashTableNode *, hashTableNode *)) {
  if (!HTable->table) {
    return NULL;
  }
  size_t pos = HTNode->hash_value & HTable->mask;
  // Incoming pointer to the result
  hashTableNode **from = &(HTable->table[pos]);
  // Iterate through the linked list
  for (hashTableNode *cur; (cur = *from) != NULL; from = &cur->next) {
    // Why check both? It's efficient, if the first one is false, we don't need
    // to check the second one!
    if (cur->hash_value == HTNode->hash_value && eq(cur, HTNode)) {
      return from;
    }
  }
  return NULL;
}

// Remove a node from the hash table
static hashTableNode *HTRemove(hashTable *HTable, hashTableNode **from) {
  hashTableNode *node = *from;
  *from = node->next;
  HTable->size--;
  return node;
}

// Average size of buckets in a table
const size_t k_max_load_factor = 8;

// Hashmap insertion
static void HTInit(hashTable *HTable, size_t n);
static void HTInsert(hashTable *HTable, hashTableNode *HTNode);
static void HMResizeMove(hashMap *HMap);
static void HMResizeCreate(hashMap *HMap);
void HMInsert(hashMap *HMap, hashTableNode *HTNode) {
  if (!(HMap->current_HT.table)) {
    HTInit(&HMap->current_HT, 4);
  }
  HTInsert(&HMap->current_HT, HTNode);
  if (!(HMap->previous_HT.table)) {
    size_t load_factor = (HMap->current_HT.size) / (HMap->current_HT.mask + 1);
    if (load_factor > k_max_load_factor) {
      // Create a new larger table
      HMResizeCreate(HMap);
    }
    // Move some key to the newer table
    HMResizeMove(HMap);
  }
}

static void HTInit(hashTable *HTable, size_t n);
static void HMResizeCreate(hashMap *HMap) {
  // Ensures that the previous_HT (previous hash table) is not already in use
  // before initiating a new resize operation because we are using progressive
  // resizing.
  assert(HMap->previous_HT.table == NULL);
  HMap->previous_HT = HMap->current_HT;
  HTInit(&HMap->current_HT, (HMap->current_HT.mask + 1) * 2);
  HMap->resizing_pos = 0;
}

// k_resizing_work ensures that hash table resizing operations are controlled
// and incremental, thereby balancing performance with system responsiveness. It
// allows the hash table to resize efficiently without monopolizing resources or
// causing excessive delays in other system operations.
const size_t k_resizing_work = 128;

static void HTInsert(hashTable *HTable, hashTableNode *HTNode);
static hashTableNode *HTRemove(hashTable *HTable, hashTableNode **from);
static void HMResizeMove(hashMap *HMap) {
  size_t n = 0;
  while (n < k_resizing_work && HMap->previous_HT.size > 0) {
    hashTableNode **from = &(HMap->previous_HT.table[HMap->resizing_pos]);
    // If the bucket is empty move to the next
    if (!*from) {
      HMap->resizing_pos++;
      continue;
    }
    HTInsert(&HMap->current_HT, HTRemove(&HMap->previous_HT, from));
    n++;
  }
  if (HMap->previous_HT.size == 0 && HMap->previous_HT.table) {
    free(HMap->previous_HT.table);
    HMap->previous_HT = hashTable();
  }
}

/*
 * The HTLookup function is designed to look up a key in a hash map (hashMap).
 * It takes care of progressive resizing and searches through both the new and
 * old hash tables.
 */

static hashTableNode **HTLookup(hashTable *HTable, hashTableNode *HTNode,
                                bool (*eq)(hashTableNode *, hashTableNode *));
static void HMResizeMove(hashMap *HMap);
hashTableNode *HMLookup(hashMap *HMap, hashTableNode *key,
                        bool (*eq)(hashTableNode *, hashTableNode *)) {
  // If there are still some nodes in old hashmap, move it to the newer one
  HMResizeMove(HMap);
  hashTableNode **from = HTLookup(&HMap->current_HT, key, eq);
  from = from ? from : HTLookup(&HMap->previous_HT, key, eq);
  return from ? *from : NULL;
}

static void HMResizeMove(hashMap *HMap);
static hashTableNode *HTRemove(hashTable *HTable, hashTableNode **from);
hashTableNode *HMPop(hashMap *HMap, hashTableNode *key,
                     bool (*eq)(hashTableNode *, hashTableNode *)) {
  HMResizeMove(HMap);
  if (hashTableNode **from = HTLookup(&HMap->current_HT, key, eq)) {
    return HTRemove(&HMap->current_HT, from);
  }
  if (hashTableNode **from = HTLookup(&HMap->previous_HT, key, eq)) {
    return HTRemove(&HMap->previous_HT, from);
  }
  return NULL;
}

size_t HMSize(hashMap *HMap) {
  return HMap->current_HT.size + HMap->previous_HT.size;
}

void HMDestroy(hashMap *HMap) {
  free(HMap->current_HT.table);
  free(HMap->previous_HT.table);
  *HMap = hashMap();
}
