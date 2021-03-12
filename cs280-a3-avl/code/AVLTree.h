/******************************************************************************/
/*!
\file   AVLTree.h
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment 3
\date   12 March 2021
\brief  
  This file contains the declarations for the AVLTree.
*/
/******************************************************************************/
//---------------------------------------------------------------------------
#ifndef AVLTREE_H
#define AVLTREE_H
//---------------------------------------------------------------------------
#include <stack>
#include "BSTree.h"

/*!
  Definition for the AVL Tree
*/
template <typename T>
class AVLTree : public BSTree<T>
{
  public:
    AVLTree(ObjectAllocator *oa = 0, bool ShareOA = false);
    virtual ~AVLTree() = default; // DO NOT IMPLEMENT
    virtual void insert(const T& value) override;
    virtual void remove(const T& value) override;

      // Returns true if efficiency implemented
    static bool ImplementedBalanceFactor(void);

  private:
    using BinTree = typename BSTree<T>::BinTree;
    using Stack = std::stack<BinTree *>;
    // private stuff
    void InsertAVL(BinTree& tree, const T& value, Stack& visited);
    void RemoveAVL(BinTree& tree, const T& value, Stack& visited);
    void BalanceAVL(Stack& visited);
    
    void RotateLeft(BinTree& tree);
    void RotateRight(BinTree& tree);
    unsigned int CountTree(BinTree& tree);
    void RecountAVL(BinTree& tree);
};

#include "AVLTree.cpp"

#endif
//---------------------------------------------------------------------------
