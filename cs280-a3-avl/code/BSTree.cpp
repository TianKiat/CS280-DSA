/******************************************************************************/
/*!
\file   BSTree.cpp
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment 3
\date   12 March 2021
\brief  
  This file contains the implmentation for the BSTree.
*/
/******************************************************************************/
#include "BSTree.h"

/******************************************************************************/
/*!
\brief
  This is the constructor for a BSTree.
\param oa, the object allocator(OA) to use.
\param ShareOA, this boolean decides whether this BST will share it's OA.
*/
/******************************************************************************/
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
    OAConfig config(true);
    OA = new ObjectAllocator(sizeof(BinTreeNode), config);
    free_OA = true;
  }
  share_OA = ShareOA;
}

/******************************************************************************/
/*!
\brief
  This is the copy constructor for a BSTree.
\param rhs, the other BST to copy.
*/
/******************************************************************************/
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
    OAConfig config(true);
    OA = new ObjectAllocator(sizeof(BinTreeNode), config);
    free_OA = true;
    share_OA = false;
  }

  DeepCopyTree(rhs.root_node, root_node);
  size_ = rhs.size_;
  height_ = rhs.height_;
}

/******************************************************************************/
/*!
\brief
  This is the destructor for a BSTree.
*/
/******************************************************************************/
template <typename T>
BSTree<T>::~BSTree()
{
  clear();
  if (free_OA)
    delete OA;
}

/******************************************************************************/
/*!
\brief
  This is the copy assignment operator for a BSTree.
\param rhs, the other BST to copy from.
\return a reference to this BST.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  This subcript operator returns the node at the given index.
\param index, the index of the node to return.
\return the node if given valid index, else returns nullptr.
*/
/******************************************************************************/
template <typename T>
const typename BSTree<T>::BinTreeNode *BSTree<T>::operator[](int index) const
{
  if (static_cast<unsigned>(index) > size_)
    return nullptr;
  else
    return FindNodeAtIndex(root_node, index);
}

/******************************************************************************/
/*!
\brief
  This function inserts a value into the BST.
\param value, the data to insert.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  This function removes a value from the BST.
\param value, the data to remove.
*/
/******************************************************************************/
template <typename T>
void BSTree<T>::remove(const T &value)
{
  DeleteNode(root_node, const_cast<T &>(value));
  height_ = tree_height(root_node);
}

/******************************************************************************/
/*!
\brief
  This function clears the BST.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  This function finds a value in the BST.
\param value, the data to find.
\param compares, the number of comparisons needed to find the value.
*/
/******************************************************************************/
template <typename T>
bool BSTree<T>::find(const T &value, unsigned &compares) const
{
  return FindNode(root_node, value, compares);
}

/******************************************************************************/
/*!
\brief
  This function returns true if BST is empty.
\return true if BST is empty, else false.
*/
/******************************************************************************/
template <typename T>
bool BSTree<T>::empty() const
{
  return size_ == 0 ? true : false;
}

/******************************************************************************/
/*!
\brief
  This function returns size of BST.
\return size of BST.
*/
/******************************************************************************/
template <typename T>
unsigned int BSTree<T>::size() const
{
  return size_;
}

/******************************************************************************/
/*!
\brief
  This function returns height of BST.
\return height of BST.
*/
/******************************************************************************/
template <typename T>
int BSTree<T>::height() const
{
  return tree_height(root_node);
}

/******************************************************************************/
/*!
\brief
  This function returns copy of root node of BST.
\return root node of BST.
*/
/******************************************************************************/
template <typename T>
typename BSTree<T>::BinTree BSTree<T>::root() const
{
  return root_node;
}

/******************************************************************************/
/*!
\brief
  This function returns reference of root node of BST.
\return root node of BST.
*/
/******************************************************************************/
template <typename T>
typename BSTree<T>::BinTree &BSTree<T>::get_root()
{
  return root_node;
}

/******************************************************************************/
/*!
\brief
  Given a value, this function creates a new node and returns it.
\param value, to insert.
\return new node of BST.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  Given a node, this function frees it.
\param node, to free.
*/
/******************************************************************************/
template <typename T>
void BSTree<T>::free_node(BinTree node)
{
  node->~BinTreeNode();
  OA->Free(node);
}

/******************************************************************************/
/*!
\brief
  This function finds the height of a tree.
\param tree, to find the height of.
\return the height.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  This function finds the predecessor of a given node.
\param tree, the given node.
\param predecessor, the predecessor node(nullptr if there is no node).
*/
/******************************************************************************/
template <typename T>
void BSTree<T>::find_predecessor(BinTree tree, BinTree &predecessor) const
{
  predecessor = tree->left;

  while (predecessor->right != nullptr)
    predecessor = predecessor->right;
}

/******************************************************************************/
/*!
\brief
  This function performs per node copying.
\param source, source tree.
\param dest, destination tree.
*/
/******************************************************************************/
template <typename T>
void BSTree<T>::DeepCopyTree(const BinTree &source, BinTree &dest)
{
  if (source == nullptr)
    dest = nullptr;

  else
  {
    dest = make_node(source->data);
    dest->count = source->count;
    dest->balance_factor = source->balance_factor;
    DeepCopyTree(source->left, dest->left);
    DeepCopyTree(source->right, dest->right);
  }
}

/******************************************************************************/
/*!
\brief
  This function recursively frees a tree.
\param tree to free.
*/
/******************************************************************************/
template <typename T>
void BSTree<T>::FreeTree(BinTree tree)
{
  if (tree == nullptr)
    return;

  FreeTree(tree->left);
  FreeTree(tree->right);

  free_node(tree);
}

/******************************************************************************/
/*!
\brief
  This function inserts a node.
\param node, node to insert.
\param value, value of the node.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  This function deletes a node.
\param node, node to delete.
\param value, value of the node.
*/
/******************************************************************************/
template <typename T>
void BSTree<T>::DeleteNode(BinTree &node, const T &value)
{
  if (node == nullptr)
    return;
  else if (value < node->data)
  {
    --node->count;
    DeleteNode(node->left, value);
  }
  else if (value > node->data)
  {
    --node->count;
    DeleteNode(node->right, value);
  }
  else
  {
    --node->count;
    if (node->left == nullptr)
    {
      BinTree tmp = node;
      node = node->right;
      free_node(tmp);
      --size_;
    }
    else if (node->right == nullptr)
    {
      BinTree tmp = node;
      node = node->left;
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

/******************************************************************************/
/*!
\brief
  This function finds a node.
\param node, node to find.
\param value, value of the node to find.
\param compares, number of comparisons needed to find this node.
*/
/******************************************************************************/
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

/******************************************************************************/
/*!
\brief
  This function finds a node at a given index.
\param tree, node to find.
\param index, index of the node.
*/
/******************************************************************************/
template <typename T>
typename BSTree<T>::BinTree BSTree<T>::FindNodeAtIndex(BinTree tree, unsigned index) const
{
  if (tree == nullptr)
    return nullptr;
  else
  {
    unsigned int left_count = 0;
    if (tree->left)
      left_count = tree->left->count;
    
    if (left_count > index)
      return FindNodeAtIndex(tree->left, index);
    else if (left_count < index)
      return FindNodeAtIndex(tree->right, index - left_count - 1);
    else
      return tree;
  }
}
