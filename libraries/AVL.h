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

static AVLNode *AVLFix(AVLNode *node);
static AVLNode *AVLDelete(AVLNode *node);
