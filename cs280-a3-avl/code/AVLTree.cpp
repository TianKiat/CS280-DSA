#include "AVLTree.h"

template <typename T>
AVLTree<T>::AVLTree(ObjectAllocator *OA, bool ShareOA)
    : BSTree<T>{OA, ShareOA}
{
}

template <typename T>
void AVLTree<T>::insert(const T &value)
{
  Stack visited_nodes;
  InsertAVL(BSTree<T>::get_root(), value, visited_nodes);
  RecountAVL(BSTree<T>::get_root());
}

template <typename T>
void AVLTree<T>::remove(const T &value)
{
  Stack visited_nodes;
  RemoveAVL(BSTree<T>::get_root(), value, visited_nodes);
  RecountAVL(BSTree<T>::get_root());
}

template <typename T>
bool AVLTree<T>::ImplementedBalanceFactor(void)
{
  return false;
}

template <typename T>
void AVLTree<T>::InsertAVL(BinTree &tree, const T &value, Stack &visited)
{
  if (tree == nullptr)
  {
    tree = BSTree<T>::make_node(value);
    ++this->size_;
    BalanceAVL(visited);
  }
  else if (value < tree->data)
  {
    visited.push(&tree);
    InsertAVL(tree->left, value, visited);
  }
  else if (value > tree->data)
  {
    visited.push(&tree);
    InsertAVL(tree->right, value, visited);
  }
}

template <typename T>
void AVLTree<T>::RemoveAVL(BinTree &tree, const T &value, Stack &visited)
{
  if (tree == nullptr)
    return;
  else if (value < tree->data)
  {
    visited.push(&tree);
    RemoveAVL(tree->left, value, visited);
  }
  else if (value > tree->data)
  {
    visited.push(&tree);
    RemoveAVL(tree->right, value, visited);
  }
  else
  {
    if (tree->left == nullptr)
    {
      AVLTree<T>::BinTree temp = tree;
      tree = tree->right;
      BSTree<T>::free_node(temp);
      --this->size_;
    }
    else if (tree->right == nullptr)
    {
      AVLTree<T>::BinTree temp = tree;
      tree = tree->left;
      BSTree<T>::free_node(temp);
      --this->size_;
    }
    else
    {
      BinTree pred = nullptr;
      BSTree<T>::find_predecessor(tree, pred);
      tree->data = pred->data;
      RemoveAVL(tree->left, tree->data, visited);
    }
    BalanceAVL(visited);
  }
}

template <typename T>
void AVLTree<T>::BalanceAVL(Stack &visited)
{
  while (!visited.empty())
  {
    BinTree *node = visited.top();
    visited.pop();

    if (std::abs(BSTree<T>::tree_height((*node)->left) - BSTree<T>::tree_height((*node)->right)) < 2)
      continue;

    if(BSTree<T>::tree_height((*node)->right) > BSTree<T>::tree_height((*node)->left))
    {
      if ((*node)->right)
      {

        if (BSTree<T>::tree_height((*node)->right->left) > BSTree<T>::tree_height((*node)->right->right))
          RotateRight((*node)->right);
        RotateLeft((*node));
      }
    }

    if(BSTree<T>::tree_height((*node)->right) < BSTree<T>::tree_height((*node)->left))
    {
      if ((*node)->left)
      {
        if (BSTree<T>::tree_height((*node)->left->left) < BSTree<T>::tree_height((*node)->left->right))
          RotateLeft((*node)->left);
        RotateRight((*node));
      }
    }
  }
}

template <typename T>
void AVLTree<T>::RotateLeft(BinTree &tree)
{
  BinTree temp = tree;
  tree = tree->right;
  temp->right = tree->left;
  tree->left = temp;
}

template <typename T>
void AVLTree<T>::RotateRight(BinTree &tree)
{
  BinTree temp = tree;
  tree = tree->left;
  temp->left = tree->right;       
  tree->right = temp;
}

template <typename T>
unsigned int AVLTree<T>::CountTree(BinTree &tree)
{
  if (tree == nullptr)
    return 0;
  
  return 1 + CountTree(tree->left) + CountTree(tree->right);
}

template <typename T>
void AVLTree<T>::RecountAVL(BinTree &tree)
{
  if (tree == nullptr)
    return;
  tree->count = CountTree(tree);
  RecountAVL(tree->left);
  RecountAVL(tree->right);
}
