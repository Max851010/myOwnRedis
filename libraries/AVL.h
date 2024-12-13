#include <cstdint>
struct AVLNode {
  uint32_t st_depth = 0; // subtree depth
  uint32_t st_size = 0;  // subtree size
  AVLNode *left = nullptr;
  AVLNode *right = nullptr;
  AVLNode *parent = nullptr;
};

static void AVLNodeInit(AVLNode *node) {
  node->st_depth = 1;
  node->st_size = 1;
  node->left = nullptr;
  node->right = nullptr;
  node->parent = nullptr;
};

static AVLNode *AVLFix(AVLNode *node);
static AVLNode *AVLDelete(AVLNode *node);
