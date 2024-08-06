#include "AVL.cpp"
#include <assert.h>
#include <iostream>
#include <set>
#include <stdio.h>
#include <stdlib.h>

#define container_of(ptr, type, member)                                        \
  reinterpret_cast<type *>(reinterpret_cast<char *>(ptr) -                     \
                           offsetof(type, member))

struct Data {
  AVLNode node;
  uint32_t val = 0;
};

struct Container {
  AVLNode *root = NULL;
};

static void add(Container &c, uint32_t val) {
  Data *data = new Data();
  AVLNodeInit(&data->node);
  data->val = val;

  AVLNode *cur = NULL;
  AVLNode **from = &c.root;

  while (*from) {
    cur = *from;
    uint32_t node_val = container_of(cur, Data, node)->val;
    from = (val < node_val) ? &cur->left : &cur->right;
  }
  *from = &data->node;
  data->node.parent = cur;
  c.root = AVLFix(&data->node);
}

static bool del(Container &c, uint32_t val) {
  AVLNode *cur = c.root;
  while (cur) {
    uint32_t node_val = container_of(cur, Data, node)->val;
    if (val == node_val) {
      break;
    }
    cur = val < node_val ? cur->left : cur->right;
  }
  if (!cur) {
    return false;
  }
  c.root = AVLDelete(cur);
  delete container_of(cur, Data, node);
  return true;
}

static void AVLVerify(AVLNode *parent, AVLNode *node) {
  if (!node) {
    return;
  }
  assert(node->parent == parent);
  AVLVerify(node, node->left);
  AVLVerify(node, node->right);
  assert(node->st_size == 1 + AVLSize(node->left) + AVLSize(node->right));
  uint32_t l_depth = AVLDepth(node->left);
  uint32_t r_depth = AVLDepth(node->right);
  assert(l_depth == r_depth || l_depth == 1 + r_depth ||
         r_depth == 1 + l_depth);
  assert(node->st_depth == 1 + max(l_depth, r_depth));

  uint32_t val = container_of(node, Data, node)->val;
  if (node->left) {
    assert(node->left->parent == node);
    assert(container_of(node->left, Data, node)->val <= val);
  }
  if (node->right) {
    assert(node->right->parent == node);
    assert(container_of(node->right, Data, node)->val >= val);
  }
}

static void extract(AVLNode *node, std::multiset<uint32_t> &extracted) {
  if (!node) {
    return;
  }
  extract(node->left, extracted);
  extracted.insert(container_of(node, Data, node)->val);
  extract(node->right, extracted);
}

static void containerVerify(Container &c, const std::multiset<uint32_t> &ref) {
  AVLVerify(NULL, c.root);
  assert(AVLSize(c.root) == ref.size());
  std::multiset<uint32_t> extracted;
  extract(c.root, extracted);
  assert(extracted == ref);
}

static void dispose(Container &c) {
  while (c.root) {
    AVLNode *node = c.root;
    c.root = AVLDelete(c.root);
    delete container_of(node, Data, node);
  }
}

static void testInsert(uint32_t size) {
  for (uint32_t val = 0; val < size; val++) {
    Container c;
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < size; i++) {
      if (i == val) {
        continue;
      }
      add(c, i);
      ref.insert(i);
    }
    containerVerify(c, ref);

    add(c, val);
    ref.insert(val);
    containerVerify(c, ref);
    dispose(c);
  }
}

static void testInsertDuplicate(uint32_t size) {
  for (uint32_t val = 0; val < size; val++) {
    Container c;
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < size; i++) {
      add(c, i);
      ref.insert(i);
    }
    add(c, val);
    ref.insert(val);
    containerVerify(c, ref);
    dispose(c);
  }
}

static void testRemove(uint32_t size) {
  for (uint32_t val = 0; val < size; val++) {
    Container c;
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < size; i++) {
      add(c, i);
      ref.insert(i);
    }
    containerVerify(c, ref);
    assert(del(c, val));
    ref.erase(val);
    containerVerify(c, ref);
    dispose(c);
  }
}

int main() {
  Container c;
  // Simple test
  containerVerify(c, {});
  add(c, 123);
  containerVerify(c, {123});
  assert(!del(c, 124));
  assert(del(c, 123));
  containerVerify(c, {});

  // Sequential insertion
  std::multiset<uint32_t> ref;
  for (uint32_t i = 0; i < 100; i = i + 3) {
    std::cout << i << std::endl;
    add(c, i);
    ref.insert(i);
    containerVerify(c, ref);
  }
  std::cout << "Here" << std::endl;

  // Random insertion
  for (uint32_t i = 0; i < 100; i++) {
    uint32_t val = (uint32_t)rand() % 1000;
    add(c, val);
    ref.insert(val);
    containerVerify(c, ref);
  }

  // random deletion
  for (uint32_t i = 0; i < 200; i++) {
    uint32_t val = (uint32_t)rand() % 1000;
    auto it = ref.find(val);
    if (it == ref.end()) {
      assert(!del(c, val));
    } else {
      assert(del(c, val));
      ref.erase(it);
    }
    containerVerify(c, ref);
  }

  for (uint32_t i = 0; i < 200; i++) {
    testInsert(i);
    testInsertDuplicate(i);
    testRemove(i);
  }
  dispose(c);
  return 0;
}
