#include "ChHashTable.h"
#include <cmath>

template <typename T>
ChHashTable<T>::ChHashTable(const HTConfig &Config, ObjectAllocator *allocator)
    : oa{allocator}, config{Config}, stats{}
{
  head = new ChHTHeadNode[config.InitialTableSize_];

  stats.TableSize_ = config.InitialTableSize_;
  stats.HashFunc_ = config.HashFunc_;
  stats.Allocator_ = allocator;
}

template <typename T>
ChHashTable<T>::~ChHashTable()
{
  clear();
  delete[] head;
}

template <typename T>
void ChHashTable<T>::insert(const char *Key, const T &Data)
{
  try
  {
    if (((stats.Count_ + 1) / static_cast<double>(stats.TableSize_)) > config.MaxLoadFactor_)
      grow_table();

    unsigned index = config.HashFunc_(Key, stats.TableSize_);
    ChHTHeadNode *table_head = &head[index];

    ChHTNode *list = table_head->Nodes;

    ++stats.Probes_;

    while (list)
    {
      ++stats.Probes_;

      if (strncmp(Key, list->Key, MAX_KEYLEN) == 0)
        throw(HashTableException(HashTableException::E_DUPLICATE, "Item being inserted is a duplicate"));

      list = list->Next;
    }

    ChHTNode *new_node = make_node(Data);
    strncpy(new_node->Key, Key, MAX_KEYLEN);

    new_node->Next = table_head->Nodes;
    table_head->Nodes = new_node;

    ++stats.Count_;
    ++table_head->Count;
  }
  catch (HashTableException &e)
  {
    throw e;
  }
}

template <typename T>
void ChHashTable<T>::remove(const char *Key)
{
  unsigned index = stats.HashFunc_(Key, stats.TableSize_);
  ChHTHeadNode *table_head = &head[index];
  ChHTNode *current = table_head->Nodes;
  ChHTNode *previous = nullptr;

  while (current)
  {
    ++stats.Probes_;

    if (strncmp(Key, current->Key, MAX_KEYLEN) == 0)
    {
      if (previous)
        previous->Next = current->Next;
      else
        table_head->Nodes = current->Next;

      --table_head->Count;
      --stats.Count_;

      remove_node(current);
      return;
    }

    previous = current;
    current = current->Next;
  }
}

template <typename T>
const T &ChHashTable<T>::find(const char *Key) const
{
  unsigned index = stats.HashFunc_(Key, stats.TableSize_);
  ChHTNode *list = head[index].Nodes;

  while (list)
  {
    ++stats.Probes_;

    if (strncmp(Key, list->Key, MAX_KEYLEN) == 0)
      return list->Data;

    list = list->Next;
  }

  throw(HashTableException(HashTableException::E_ITEM_NOT_FOUND,
                           "Item requested to be found does not exist"));
}

template <typename T>
void ChHashTable<T>::clear()
{
  for (unsigned i = 0; i < stats.TableSize_; ++i)
  {
    ChHTNode *list = head[i].Nodes;

    while (list)
    {
      ChHTNode *temp = list->Next;
      remove_node(list);
      list = temp;
    }

    head[i].Nodes = nullptr;
  }

  stats.Count_ = 0;
}

template <typename T>
HTStats ChHashTable<T>::GetStats() const
{
  return stats;
}

template <typename T>
const typename ChHashTable<T>::ChHTHeadNode *ChHashTable<T>::GetTable() const
{
  return head;
}

template <typename T>
typename ChHashTable<T>::ChHTNode *ChHashTable<T>::make_node(const T &data)
{
  try
  {
    if (oa)
    {
      ChHTNode *mem = reinterpret_cast<ChHTNode *>(oa->Allocate());
      ChHTNode *node = new (mem) ChHTNode(data);
      return node;
    }

    ChHTNode *node = new ChHTNode(data);
    return node;
  }
  catch (std::bad_alloc &e)
  {
    throw(HashTableException(HashTableException::E_NO_MEMORY,
                             "No memory is available"));
  }
}

template <typename T>
void ChHashTable<T>::remove_node(ChHTNode *&node)
{

  if (config.FreeProc_)
    config.FreeProc_(node->Data);

  if (oa)
    oa->Free(node);
  else
    delete node;
}

template <typename T>
void ChHashTable<T>::grow_table()
{
  try
  {
    unsigned old_table_size = stats.TableSize_;

    double factor = std::ceil(stats.TableSize_ * config.GrowthFactor_);
    unsigned new_table_size = GetClosestPrime(static_cast<unsigned>(factor));
    stats.TableSize_ = new_table_size;

    ChHTHeadNode *new_table = new ChHTHeadNode[stats.TableSize_];
    for (unsigned i = 0; i < old_table_size; ++i)
    {

      ChHTNode *list = head[i].Nodes;
      while (list)
      {
        ++stats.Probes_;
        ChHTNode *temp = list->Next;
        unsigned index = stats.HashFunc_(list->Key, stats.TableSize_);

        if (new_table[index].Nodes)
        {
          ChHTNode *new_list = new_table[index].Nodes;

          while (new_list)
          {
            ++stats.Probes_;

            if (strncmp(new_list->Key, list->Key, MAX_KEYLEN) == 0)
              break;
            new_list = new_list->Next;
          }
        }
          list->Next = new_table[index].Nodes;
        new_table[index].Nodes = list;

        list = temp;
      }
    }
    delete[] head;
    head = new_table;
    ++stats.Expansions_;
  }
  catch (std::bad_alloc &ba)
  {
    throw(HashTableException(HashTableException::E_NO_MEMORY,
                             "No memory is available"));
  }
}
