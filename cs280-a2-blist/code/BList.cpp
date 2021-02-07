template <typename T, int Size>
size_t BList<T, Size>::nodesize(void)
{
  return sizeof(BNode);
}

template <typename T, int Size>
const typename BList<T, Size>::BNode* BList<T, Size>::GetHead() const
{
  return head_;
}
