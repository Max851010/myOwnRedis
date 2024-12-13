#pragma once

#include "AVL.h"
#include "HashTable.h"

struct ZSet {
  AVLNode *tree = NULL;
  HashMap HMap;
};

struct ZNode {
  AVLNode tree;
  hashTableNode HTNode;
  double score = 0;
  size_t len = 0;
  char name[0];
}
