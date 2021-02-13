/******************************************************************************/
/*!
\file   BList.cpp
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment 2
\date   12 February 2021
\brief  
  This file contains the implementation for the BList.
*/
/******************************************************************************/
template <typename T, int Size>
size_t BList<T, Size>::nodesize(void)
{
  return sizeof(BNode);
}

template <typename T, int Size>
const typename BList<T, Size>::BNode *BList<T, Size>::GetHead() const
{
  return head_;
}

template <typename T, int Size>
BList<T, Size>::BList() : head_{nullptr}, tail_{nullptr}
{
  stats_.NodeSize = nodesize();
  stats_.ArraySize = Size;
}

template <typename T, int Size>
BList<T, Size>::BList(const BList &rhs) : stats_{rhs.stats_}
{
  auto rhs_current = rhs.GetHead();
  BNode * current = nullptr;
  BNode * prev = nullptr;

  while (rhs_current)
  {
    current = CreateNode(rhs_current);

    // link up prev node and current node
    if (prev)
    {
      prev->next = current;
      current->prev = prev;
    }
    else
    {
      head_ = current; // update head if it's first node
    }

    //traverse down list
    prev = current;
    rhs_current = rhs_current->next;
  }

  tail_ = prev;
}

template <typename T, int Size>
BList<T, Size>::~BList()
{
  clear();
}

template <typename T, int Size>
BList<T, Size> &BList<T, Size>::operator=(const BList &rhs)
{
  if (this == &rhs)
    return *this;

  clear();

  auto rhs_current = rhs.GetHead();
  BNode * current = nullptr;
  BNode * prev = nullptr;

  while (rhs_current)
  {
    current = CreateNode(rhs_current);

    // link up prev node and current node
    if (prev)
    {
      prev->next = current;
      current->prev = prev;
    }
    else
    {
      head_ = current; // update head if it's first node
    }

    //traverse down list
    prev = current;
    rhs_current = rhs_current->next;
  }

  tail_ = prev;
  stats_ = rhs.GetStats();
  return *this;
}

template <typename T, int Size>
void BList<T, Size>::push_back(const T &value)
{
  //add to tail node if tail node has available space
  if (tail_ && tail_->count < Size)
  {
    tail_->values[tail_->count] = value;
    IncrementNodeCount(tail_);
  }
  else
  {
    //create a new node
    auto new_node = CreateNode(nullptr);
    new_node->values[0] = value;
    IncrementNodeCount(new_node);

    if (stats_.NodeCount == 0)
      tail_ = head_ = new_node;
    else
    {
      new_node->prev = tail_;
      tail_->next = new_node;
      tail_ = new_node;
    }
    ++stats_.NodeCount;
  }
  ++stats_.ItemCount;
}

template <typename T, int Size>
void BList<T, Size>::push_front(const T &value)
{
  //add to head node if head node has available space
  if (head_ && head_->count < Size)
  {
    for (auto i = head_->count; i > 0; --i)
      head_->values[i] = head_->values[i - 1];

    head_->values[0] = value;
    IncrementNodeCount(head_);
  }
  else
  {
    //create a new node
    auto new_node = CreateNode(nullptr);
    new_node->values[0] = value;
    IncrementNodeCount(new_node);

    if (stats_.NodeCount == 0)
      tail_ = head_ = new_node;
    else
    {
      head_->prev = new_node;
      new_node->next = head_;
      head_ = new_node;
    }
    ++stats_.NodeCount;
  }
  ++stats_.ItemCount;
}

template <typename T, int Size>
void BList<T, Size>::insert(const T &value)
{
  if (!head_)
  {
    push_front(value);
    return;
  }

  auto current = head_;
  auto i = 0;
  // Find node to insert value in
  while (current)
  {
    while (i < Size && current->values[i] < value)
      ++i;

    if (i < current->count)
      break;

    i = 0;
    current = current->next;
  }

  if (current) // we have found a suitable node to insert the value
  {
    if (i == 0)
    {
      if (current->prev && current->prev->count < Size)
      {
        InsertValueAtIndex(current->prev, current->prev->count, value);
      }
      else if (current->count < Size)
      {
        InsertValueAtIndex(current, i, value);
      }
      else if (current->prev)
      {
        SplitNode(current->prev, Size, value);
      }
      else
      {
        SplitNode(current, i, value);
      }
    }
    else
    {
      if (current->count < Size)
      {
        InsertValueAtIndex(current, i, value);
      }
      else
      {
        SplitNode(current, i, value);
      }
    }
  }
  else //this means we are at the tail (current = nullptr)
  {
    if (tail_->count < Size)
    {
      InsertValueAtIndex(tail_, tail_->count, value);
    }
    else
    {
      SplitNode(tail_, tail_->count, value);
    }
  }
}

template <typename T, int Size>
void BList<T, Size>::remove(int index)
{
  auto node = GetNodeAtIndex(index);
  auto i = index % Size;

  RemoveValueAtIndex(node, i);

  if (node->count == 0)
    FreeNode(node);
}

template <typename T, int Size>
void BList<T, Size>::remove_by_value(const T &value)
{
  auto current = head_;
  auto index = 0;
  bool found = false;

  while (current)
  {
    for (auto i = 0; i < current->count; ++i)
    {
      if (current->values[i] == value)
      {
        index = i;
        found = true;
        break;
      }
    }
    if (found)
      break;
    current = current->next;
  }

  if (current)
  {
    RemoveValueAtIndex(current, index);
    if (current->count == 0)
      FreeNode(current);
  }
}

template <typename T, int Size>
int BList<T, Size>::find(const T &value) const
{
  BNode *current = head_;
  auto total_index = 0;
  while (current)
  {
    for (auto i = 0; i < current->count; ++i)
    {
      if (current->values[i] == value)
        return total_index + i;
    }
    total_index += current->count;
    current = current->next;
  }
  return -1;
}

template <typename T, int Size>
T &BList<T, Size>::operator[](int index)
{
  return GetValueAtIndex(index);
}

template <typename T, int Size>
const T &BList<T, Size>::operator[](int index) const
{
  return GetValueAtIndex(index);
}

template <typename T, int Size>
size_t BList<T, Size>::size() const
{
  return stats_.ItemCount;
}

template <typename T, int Size>
void BList<T, Size>::clear()
{
  while (stats_.ItemCount > 0)
    remove(0);
}

template <typename T, int Size>
BListStats BList<T, Size>::GetStats() const
{
  return stats_;
}

template <typename T, int Size>
typename BList<T, Size>::BNode *BList<T, Size>::CreateNode(const BNode *rhs)
{
  BNode *new_node = nullptr;
  try
  {
    new_node = new BNode();
    if (rhs)
    {
      new_node->count = rhs->count;
      for (auto i = 0; i < rhs->count; ++i)
        new_node->values[i] = rhs->values[i];
    }
  }
  catch (const std::exception &e)
  {
    throw(BListException(BListException::E_NO_MEMORY, e.what()));
  }
  return new_node;
}

template <typename T, int Size>
typename BList<T, Size>::BNode *BList<T, Size>::GetNodeAtIndex(int index) const
{
  auto total_count = 0;
  auto current = head_;

  while (total_count <= index)
  {
    total_count += current->count;
    current = current->next;
  }

  if (current == head_)
    return current;
  else if (current)
    return current->prev;
  else
    return tail_;
}

template <typename T, int Size>
void BList<T, Size>::FreeNode(BNode *node)
{
  if (node->prev)
    node->prev->next = node->next;
  else
    head_ = node->next;

  if (node->next)
    node->next->prev = node->prev;
  else
    tail_ = node->prev;

  node->~BNode();
  delete node;
  --stats_.NodeCount;
}

template <typename T, int Size>
void BList<T, Size>::IncrementNodeCount(BNode *node)
{
  ++node->count;
  if (node->count > Size)
    node->count = Size;
}

template <typename T, int Size>
void BList<T, Size>::SplitNode(BNode *node, int index, const T &value)
{
  auto new_node = CreateNode(nullptr);
  new_node->prev = node;
  if (node->next)
  {
    node->next->prev = new_node;
    new_node->next = node->next;
  }
  node->next = new_node;

  if (Size == 1)
  {
    if (index == 0)
    {
      new_node->values[0] = node->values[0];
      node->values[0] = value;
    }
    else
    {
      new_node->values[0] = value;
    }
    IncrementNodeCount(new_node);
  }
  else
  {
    auto middle = Size / 2;

    //Copy upper half values to new node
    auto j = 0;
    for (auto i = middle; i < Size; ++i)
    {
      new_node->values[j++] = node->values[i];
      IncrementNodeCount(new_node);
    }

    node->count = middle;

    //insert value
    //if index is less than middle insert in given node
    if (index <= middle)
    {
      for (auto i = node->count; i > index; --i)
        node->values[i] = node->values[i - 1];
      node->values[index] = value;
      IncrementNodeCount(node);
    }
    // else insert it into the new node
    else if (index == Size)
    {
      new_node->values[new_node->count] = value;
      IncrementNodeCount(new_node);
    }
    else
    {
      index -= middle;

      for (auto i = new_node->count; i > index; --i)
        new_node->values[i] = new_node->values[i - 1];
      new_node->values[index] = value;
      IncrementNodeCount(new_node);
    }
  }
  if (node == tail_)
    tail_ = new_node;
  ++stats_.ItemCount;
  ++stats_.NodeCount;
}

template <typename T, int Size>
T &BList<T, Size>::GetValueAtIndex(int index) const
{
  auto current = head_;
  auto total_index = current->count;
  auto i = index;

  while (total_index <= index)
  {
    i -= current->count;
    current = current->next;
    if (current)
      total_index += current->count;
  }

  return current->values[i];
}

template <typename T, int Size>
void BList<T, Size>::InsertValueAtIndex(BNode *node, int index, const T &value)
{
  auto i = node->count;
  while (i > index)
  {
    node->values[i] = node->values[i - 1];
    --i;
  }

  node->values[index] = value;
  IncrementNodeCount(node);
  ++stats_.ItemCount;
}

template <typename T, int Size>
void BList<T, Size>::RemoveValueAtIndex(BNode *node, int index)
{
  for (auto i = index; i < node->count; ++i)
    node->values[i] = node->values[i + 1];
  --node->count;
  --stats_.ItemCount;
}