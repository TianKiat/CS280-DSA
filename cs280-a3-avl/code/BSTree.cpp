#include "BSTree.h"

template <typename T>
BSTree<T>::BSTree(ObjectAllocator *oa, bool ShareOA)
    : root_node{nullptr}, size_{0}, height_{-1}
{
  if (oa)
  {
    OA = oa;
    free_OA = false;
  }
  else
  {
    OA = new ObjectAllocator{sizeof(BinTreeNode), OAConfig{true}};
    free_OA = true;
  }
  share_OA = ShareOA;
}

template <typename T>
BSTree<T>::BSTree(const BSTree &rhs)
{
  if (rhs.share_OA)
  {
    OA = rhs.OA;
    free_OA = false;
    share_OA = true;
  }
  else
  {
    OA = new ObjectAllocator{sizeof(BinTreeNode), OAConfig{true}};
    free_OA = true;
    share_OA = false;
  }

  DeepCopyTree(rhs.root_node, root_node);
  size_ = rhs.size_;
  height_ = rhs.height_;
}

template <typename T>
BSTree<T>::~BSTree()
{
  clear();
  if (free_OA)
    delete OA;
}

template <typename T>
BSTree<T> &BSTree<T>::operator=(const BSTree &rhs)
{
  if (this == &rhs)
    return *this;

  if (rhs.share_OA)
  {
    if (free_OA)
    {
      clear();
      delete OA;
    }

    OA = rhs.OA;
    free_OA = false;
    share_OA = true;
  }

  clear();
  DeepCopyTree(rhs.root_node, root_node);
  size_ = rhs.size_;
  height_ = rhs.height_;

  return *this;
}

template <typename T>
const typename BSTree<T>::BinTreeNode *BSTree<T>::operator[](int index) const
{
  if (static_cast<unsigned>(index) > size_) 
    return nullptr;
  else
    return FindNodeAtIndex(root_node, index);
}

template <typename T>
void BSTree<T>::insert(const T &value)
{
  try
  {
    InsertNode(root_node, value, 0);
  }
  catch (const BSTException &except)
  {
    throw except;
  }
}

template <typename T>
void BSTree<T>::remove(const T &value)
{
  DeleteNode(root_node, const_cast<T &>(value));

  height_ = tree_height(root_node);
}

template <typename T>
void BSTree<T>::clear()
{
  if (root_node)
  {
    FreeTree(root_node);

    root_node = nullptr;
    size_ = 0;
    height_ = -1;
  }
}

template <typename T>
bool BSTree<T>::find(const T &value, unsigned &compares) const
{
  return FindNode(root_node, value, compares);
}

template <typename T>
bool BSTree<T>::empty() const
{
  return height_ < 0 ? true : false;
}

template <typename T>
unsigned int BSTree<T>::size() const
{
  return size_;
}

template <typename T>
int BSTree<T>::height() const
{
  return height_;
}

template <typename T>
typename BSTree<T>::BinTree BSTree<T>::root() const
{
  return root_node;
}

template <typename T>
typename BSTree<T>::BinTree &BSTree<T>::get_root()
{
  return root_node;
}

template <typename T>
typename BSTree<T>::BinTree BSTree<T>::make_node(const T &value) const
{
  try
  {
    BinTree alloc = reinterpret_cast<BinTree>(OA->Allocate());

    BinTree node = new (alloc) BinTreeNode(value);
    return node;
  }
  catch (const OAException &except)
  {
    throw(BSTException(BSTException::E_NO_MEMORY, except.what()));
  }
}

template <typename T>
void BSTree<T>::free_node(BinTree node)
{
  node->~BinTreeNode();
  OA->Free(node);
}

template <typename T>
int BSTree<T>::tree_height(BinTree tree) const
{
  if (tree == nullptr)
    return -1;

  else
  {
    int height_left = tree_height(tree->left);
    int height_right = tree_height(tree->right);
    return std::max(height_left, height_right) + 1;
  }
}

template <typename T>
void BSTree<T>::find_predecessor(BinTree tree, BinTree &predecessor) const
{
  predecessor = tree->left;

  while (predecessor->right != nullptr)
    predecessor = predecessor->right;
}

template <typename T>
void BSTree<T>::DeepCopyTree(const BinTree &source, BinTree &dest)
{
  if (source == nullptr)
    dest = nullptr;

  else
  {
    dest = make_node(source->data);
    DeepCopyTree(source->left, dest->left);
    DeepCopyTree(source->right, dest->right);
  }
}

template <typename T>
void BSTree<T>::FreeTree(BinTree tree)
{
  if (tree == nullptr)
    return;

  FreeTree(tree->left);
  FreeTree(tree->right);

  free_node(tree);
}

template <typename T>
void BSTree<T>::InsertNode(BinTree &node, const T &value, int depth)
{
  try
  {
    if (node == nullptr)
    {
      if (depth > height_)
        ++height_;

      node = make_node(value);
      ++size_;

      return;
    }

    if (value < node->data)
    {
      ++node->count;
      InsertNode(node->left, value, depth + 1);
    }
    else
    {
      ++node->count;
      InsertNode(node->right, value, depth + 1);
    }
  }
  catch (const BSTException &except)
  {
    throw except;
  }
}

template <typename T>
void BSTree<T>::DeleteNode(BinTree &node, const T &value)
{
  if (node == nullptr)
    return;
  else if (value < node->data)
    DeleteNode(node->left, value);
  else if (value > node->data)
    DeleteNode(node->right, value);
  else
  {
    if (node->left == nullptr)
    {
      BinTree tmp = node;
      node = node->right;
      --node->count;
      free_node(tmp);
      --size_;
    }
    else if (node->right == nullptr)
    {
      BinTree tmp = node;
      node = node->left;
      --node->count;
      free_node(tmp);
      --size_;
    }
    else
    {
      BinTree pred = nullptr;
      find_predecessor(node, pred);
      node->data = pred->data;
      DeleteNode(node->left, node->data);
    }
  }
}

template <typename T>
bool BSTree<T>::FindNode(BinTree node, const T &value, unsigned &compares) const
{
  ++compares;

  if (node == nullptr)
    return false;
  else if (value == node->data)
    return true;
  else if (value < node->data)
    return FindNode(node->left, value, compares);
  else
    return FindNode(node->right, value, compares);
}

template <typename T>
typename BSTree<T>::BinTree BSTree<T>::FindNodeAtIndex(BinTree tree, unsigned index) const
{
  if (tree == nullptr)
    return nullptr;
  else
  {
    const unsigned int left_count = tree->left->count;
    if (left_count > index)
      return FindNodeAtIndex(tree->left, index);
    else if (left_count < index)
      return FindNodeAtIndex(tree->right, index - left_count - 1);
    else
      return tree;
  }
}
