#include <stddef.h>
#include <stdint.h>

struct AVLNode {
  uint32_t st_depth = 0; // subtree depth
  uint32_t st_size = 0;  // subtree size
  AVLNode *left = NULL;
  AVLNode *right = NULL;
  AVLNode *parent = NULL;
};

static void AVLNodeInit(AVLNode *node) {
  node->st_depth = 1;
  node->st_size = 1;
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;
};

static uint32_t max(uint32_t lhs, uint32_t rhs) {
  return lhs < rhs ? rhs : lhs;
}

static uint32_t AVLDepth(AVLNode *node) { return node ? node->st_depth : 0; }

static uint32_t AVLSize(AVLNode *node) { return node ? node->st_size : 0; }

static uint32_t AVLDepth(AVLNode *node);
static uint32_t AVLSize(AVLNode *node);
static uint32_t max(uint32_t lhs, uint32_t rhs);
static void AVLNodeUpdate(AVLNode *node) {
  node->st_depth = 1 + max(AVLDepth(node->left), AVLDepth(node->right));
  node->st_size = 1 + AVLSize(node->left) + AVLSize(node->right);
}

/*
 * RR case, only need to perform this function once
 * Example:
 *
 *   10               20             5                      10
 *    \              /  \             \                    /  \
 *    20     ->     10  30            10                  5   15
 *     \                             /  \         ->       \    \
 *     30                           8   15                  8   20
 *                                        \
 *                                        20
 */
static void AVLNodeUpdate(AVLNode *node);
static AVLNode *rotateLeft(AVLNode *node) {
  AVLNode *new_node = node->right;
  if (new_node->left) {
    new_node->left->parent = node;
  }
  node->right = new_node->left;
  new_node->left = node;
  new_node->parent = node->parent;
  node->parent = new_node;
  AVLNodeUpdate(node);
  AVLNodeUpdate(new_node);
  return new_node;
}

// LL case
static void AVLNodeUpdate(AVLNode *node);
static AVLNode *rotateRight(AVLNode *node) {
  AVLNode *new_node = node->left;
  if (new_node->right) {
    new_node->right->parent = node;
  }
  node->left = new_node->right;
  new_node->right = node;
  new_node->parent = node->parent;
  node->parent = new_node;
  AVLNodeUpdate(node);
  AVLNodeUpdate(new_node);
  return new_node;
}

// RL case
static AVLNode *rotateRight(AVLNode *node);
static AVLNode *rotateLeft(AVLNode *node);
static uint32_t AVLDepth(AVLNode *node);
static AVLNode *AVLFixRight(AVLNode *root) {
  if (AVLDepth(root->right->right) < AVLDepth(root->right->left)) {
    root->right = rotateRight(root->right);
  }
  return rotateLeft(root);
}

// LR case
static AVLNode *rotateRight(AVLNode *node);
static AVLNode *rotateLeft(AVLNode *node);
static uint32_t AVLDepth(AVLNode *node);
static AVLNode *AVLFixLeft(AVLNode *root) {
  if (AVLDepth(root->left->left) < AVLDepth(root->left->right)) {
    root->left = rotateLeft(root->left);
  }
  return rotateRight(root);
}

// Maintain the balance of the AVL tree after insertion and deletion.
// Bottom up.
static void AVLNodeUpdate(AVLNode *node);
static uint32_t AVLDepth(AVLNode *node);
static AVLNode *AVLFix(AVLNode *node) {
  while (true) {
    AVLNodeUpdate(node);
    uint32_t left_depth = AVLDepth(node->left);
    uint32_t right_depth = AVLDepth(node->right);
    // Determines whether the current node is the left or right child of it's
    // parent and sets the from pointer accrodingly
    // The from pointer is determined by checking if the current node is the
    // left or right child of its parent.
    AVLNode **from = NULL;
    if (node->parent) {
      from = (node->parent->left == node) ? &node->parent->left
                                          : &node->parent->right;
    }
    if (left_depth == right_depth + 2) {
      node = AVLFixLeft(node);
    } else if (right_depth == left_depth + 2) {
      node = AVLFixRight(node);
    }
    if (!from) {
      return node;
    }
    // Pointer is pointed from parent to the current node (move up)
    // Also move up node
    *from = node;
    node = node->parent;
  }
}

static AVLNode *AVLDelete(AVLNode *node) {
  if (node->right == NULL) {
    // no right subtree, replaced with left subtree
    AVLNode *parent = node->parent;
    if (node->left) {
      node->left->parent = parent;
    }
    if (parent) {
      (parent->left == node ? parent->left : parent->right) = node->left;
      return AVLFix(parent);
    } else {
      return node->left;
    }
  } else {
    // The leftmost of the right subtree
    AVLNode *successor = node->right;
    while (successor->left) {
      successor = successor->left;
    }
    // because we move the smallest node of right subtree to the successor,
    // now we need to recursively delete every node in the right
    // (just like move all them up)
    AVLNode *root = AVLDelete(successor);
    // Copies the data from the node to be deleted into the successor node.
    // This effectively replaces the node with its successor.
    *successor = *node;
    if (successor->left) {
      successor->left->parent = successor;
    }
    if (successor->right) {
      successor->right->parent = successor;
    }
    AVLNode *parent = node->parent;
    if (parent) {
      (parent->left == node ? parent->left : parent->right) = successor;
      return root;
    } else {
      return successor;
    }
  }
}
