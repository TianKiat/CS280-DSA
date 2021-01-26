/******************************************************************************/
/*!
\file   ObjectAllocator.cpp
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment 1
\date   21 January 2021
\brief  
  This file contains the implementation for the Object Allocator.
*/
/******************************************************************************/
#include "ObjectAllocator.h"
#include <cstring> //<! std::memset, std::strcpy, std::strlen

using BYTE = unsigned char;                   //!< Type of byte
constexpr size_t PTR_SIZE = sizeof(intptr_t); //!< Size of a pointer

/******************************************************************************/
/*!
\brief
  This function returns the closest \par n that is the multiple of \par alignment.

\par n The size to align.
\par alignment The quotient to align to.

\return The aligned size.
*/
/******************************************************************************/
inline size_t Align(size_t n, size_t alignment)
{
  if (!alignment)
    return n;

  size_t rem = n % alignment == 0 ? 0ULL : 1ULL;
  return alignment * ((n / alignment) + rem);
}

/******************************************************************************/
/*!
\brief
  This is the constructor of an Object Allocator.

\par ObjectSize The size of the object that this allocator stores.
\par config A client specified Allocator configuration
*/
/******************************************************************************/
ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig &config)
    : PageList_{nullptr}, FreeList_{nullptr}, Config_{config}, Stats_{}
{
  size_t leftHeaderSize = PTR_SIZE + config.HBlockInfo_.size_ + static_cast<size_t>(config.PadBytes_);
  HeaderSize_ = Align(leftHeaderSize, config.Alignment_);
  size_t midBlockSize = ObjectSize + (Config_.PadBytes_ * 2ULL) + config.HBlockInfo_.size_;
  MidBlockSize_ = Align(midBlockSize, config.Alignment_);

  Stats_.ObjectSize_ = ObjectSize;
  Config_.InterAlignSize_ = static_cast<unsigned int>(MidBlockSize_ - midBlockSize);
  Config_.LeftAlignSize_ = static_cast<unsigned int>(HeaderSize_ - leftHeaderSize);
  Stats_.PageSize_ = PTR_SIZE + Config_.LeftAlignSize_ + Config_.ObjectsPerPage_ * MidBlockSize_ - Config_.InterAlignSize_;

  AllocateNewPage(PageList_);
}

/******************************************************************************/
/*!
\brief
  This is the destructor of an Object Allocator.
*/
/******************************************************************************/
ObjectAllocator::~ObjectAllocator() noexcept
{
  GenericObject *page = PageList_;
  while (page)
  {
    GenericObject *next = page->Next;

    if (Config_.HBlockInfo_.type_ == OAConfig::hbExternal)
    {
      BYTE *objectAddress = reinterpret_cast<BYTE *>(page) + HeaderSize_;
      for (size_t i = 0; i < Config_.ObjectsPerPage_; ++i)
      {
        FreeHeader(reinterpret_cast<GenericObject *>(objectAddress), Config_.HBlockInfo_.type_);
      }
    }
    delete[] reinterpret_cast<BYTE *>(page);
    page = next;
  }
}

/******************************************************************************/
/*!
\brief
  This function provides block of memory to store an object.

\par label A label for the block of memory requested.
\return A pointer to an allocated block of memory.
*/
/******************************************************************************/
void *ObjectAllocator::Allocate(const char *label)
{
  if (Config_.UseCPPMemManager_)
  {
    try
    {
      BYTE *newObj = new BYTE[Stats_.ObjectSize_];

      ++Stats_.ObjectsInUse_;
      if (Stats_.ObjectsInUse_ > Stats_.MostObjects_)
        Stats_.MostObjects_ = Stats_.ObjectsInUse_;
      --Stats_.FreeObjects_;
      ++Stats_.Allocations_;

      return newObj;
    }
    catch (std::bad_alloc &)
    {
      throw OAException(OAException::E_NO_MEMORY, "Allocate: No system memory available!");
    }
  }

  if (nullptr == FreeList_)
  {
    AllocateNewPage(PageList_);
  }
  GenericObject *AllocatedObject = FreeList_;

  FreeList_ = FreeList_->Next;

  if (Config_.DebugOn_)
  {
    std::memset(AllocatedObject, ALLOCATED_PATTERN, Stats_.ObjectSize_);
  }

  ++Stats_.ObjectsInUse_;
  if (Stats_.ObjectsInUse_ > Stats_.MostObjects_)
    Stats_.MostObjects_ = Stats_.ObjectsInUse_;
  --Stats_.FreeObjects_;
  ++Stats_.Allocations_;

  InitHeader(AllocatedObject, Config_.HBlockInfo_.type_, label);

  return AllocatedObject;
}

/******************************************************************************/
/*!
\brief
  This function frees block of memory used by an object.

\par Object The objects address that needs to be freed.
*/
/******************************************************************************/
void ObjectAllocator::Free(void *Object)
{
  ++Stats_.Deallocations_;

  if (Config_.UseCPPMemManager_)
  {
    delete[] reinterpret_cast<BYTE *>(Object);
    return;
  }

  GenericObject *object = reinterpret_cast<GenericObject *>(Object);

  if (Config_.DebugOn_)
  {
    CheckBoundaries(reinterpret_cast<BYTE *>(Object));
    {
      if (!ValidatePadding(GetLeftPadAdrress(object), Config_.PadBytes_))
      {
        throw OAException{OAException::E_CORRUPTED_BLOCK, "Free: Corrupted left padding!"};
      }
      if (!ValidatePadding(GetRightPadAdrress(object), Config_.PadBytes_))
      {
        throw OAException{OAException::E_CORRUPTED_BLOCK, "Free: Corrupted right padding!"};
      }
    }
  }

  FreeHeader(object, Config_.HBlockInfo_.type_);

  if (Config_.DebugOn_)
  {
    std::memset(object, FREED_PATTERN, Stats_.ObjectSize_);
  }
  object->Next = nullptr;

  PushToFreeList(object);

  --Stats_.ObjectsInUse_;
}

/******************************************************************************/
/*!
\brief
  This function calls a callback \p fn on any active object in the allocator.

\par fn The callback function.
\return The amount of bytes currently in use by the allocator.
*/
/******************************************************************************/
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const
{
  if (!PageList_)
    return 0;
  else
  {
    unsigned bytesUsed = 0;
    GenericObject *page = PageList_;

    while (page)
    {
      BYTE *pageData = reinterpret_cast<BYTE *>(page) + HeaderSize_;
      for (size_t i = 0; i < Config_.ObjectsPerPage_; ++i)
      {
        GenericObject *objectData = reinterpret_cast<GenericObject *>(pageData + i * MidBlockSize_);

        if (IsObjectUsed(objectData))
        {
          fn(objectData, Stats_.ObjectSize_);
          ++bytesUsed;
        }
      }
      page = page->Next;
    }
    return bytesUsed;
  }
}

/******************************************************************************/
/*!
\brief
  This function calls a callback \p fn on any corrupted object in the allocator.

\par fn The callback function.
\par The number of blocks/objects that are corrupted.
*/
/******************************************************************************/
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const
{
  if (!Config_.DebugOn_ || Config_.PadBytes_ == 0)
    return 0;

  unsigned numBlocksCorrupted = 0;
  GenericObject *page = PageList_;

  while (page)
  {
    BYTE *pageData = reinterpret_cast<BYTE *>(page) + HeaderSize_;
    for (size_t i = 0; i < Config_.ObjectsPerPage_; ++i)
    {
      GenericObject *objectData = reinterpret_cast<GenericObject *>(pageData + i * MidBlockSize_);

      if (!ValidatePadding(GetLeftPadAdrress(objectData), Config_.PadBytes_) || !ValidatePadding(GetRightPadAdrress(objectData), Config_.PadBytes_))
      {
        fn(objectData, Stats_.ObjectSize_);
        ++numBlocksCorrupted;
        continue;
      }
    }
    page = page->Next;
  }
  return numBlocksCorrupted;
}

/******************************************************************************/
/*!
\brief
  This checks for empty pages and frees their memory (return to OS).

\return number of pages freed.
*/
/******************************************************************************/
unsigned ObjectAllocator::FreeEmptyPages()
{
  if (!PageList_)
    return 0;
  unsigned numEmptyPages = 0;
  GenericObject *tempHead = PageList_;
  GenericObject *prevHead = nullptr;

  while (tempHead && IsPageFree(tempHead))
  {
    PageList_ = tempHead->Next;
    FreePage(tempHead);
    tempHead = this->PageList_;
    ++numEmptyPages;
  }

  while (tempHead)
  {
    while (tempHead && !IsPageFree(tempHead))
    {
      prevHead = tempHead;
      tempHead = tempHead->Next;
    }

    if (!tempHead)
      return numEmptyPages;

    prevHead->Next = tempHead->Next;
    FreePage(tempHead);

    tempHead = prevHead->Next;
    ++numEmptyPages;
  }
  return numEmptyPages;
}

/******************************************************************************/
/*!
\brief
  This function sets the debug state of the object allocator

\par State Send \p true to turn on debugging, \p false to turn off debugging.
*/
/******************************************************************************/
void ObjectAllocator::SetDebugState(bool State)
{
  Config_.DebugOn_ = State;
}

/******************************************************************************/
/*!
\brief
  This function returns the free list of the object allocator.

\return Pointer to the free list.
*/
/******************************************************************************/
const void *ObjectAllocator::GetFreeList() const
{
  return FreeList_;
}

/******************************************************************************/
/*!
\brief
  This function returns the page list of the object allocator.

\return Pointer to the page list.
*/
/******************************************************************************/
const void *ObjectAllocator::GetPageList() const
{
  return PageList_;
}

/******************************************************************************/
/*!
\brief
  This function returns the configuration of the object allocator.

\return A copy of this allocator's configuration.
*/
/******************************************************************************/
OAConfig ObjectAllocator::GetConfig() const
{
  return Config_;
}

/******************************************************************************/
/*!
\brief
  This function returns the statistics of the object allocator.

\return A copy of this allocator's statistics.
*/
/******************************************************************************/
OAStats ObjectAllocator::GetStats() const
{
  return Stats_;
}

/******************************************************************************/
/*!
\brief
  This function allocates new memory for a page.

\par pageList A pointer to the previous page list.
*/
/******************************************************************************/
void ObjectAllocator::AllocateNewPage(GenericObject *&pageList)
{
  if (Stats_.PagesInUse_ == Config_.MaxPages_)
  {
    throw OAException(OAException::OA_EXCEPTION::E_NO_PAGES, "Out of pages!");
  }
  else
  {
    GenericObject *newPage = nullptr;
    try
    {
      newPage = reinterpret_cast<GenericObject *>(new BYTE[Stats_.PageSize_]());
      ++Stats_.PagesInUse_;
    }
    catch (std::bad_alloc &)
    {
      throw OAException{OAException::OA_EXCEPTION::E_NO_MEMORY, "AllocateNewPage: No system memory available!"};
    }

    if (Config_.DebugOn_)
    {
      std::memset(newPage, ALIGN_PATTERN, Stats_.PageSize_);
    }

    newPage->Next = pageList;
    pageList = newPage;

    BYTE *PageStartAddress = reinterpret_cast<BYTE *>(newPage);
    BYTE *DataStartAddress = PageStartAddress + HeaderSize_;

    for (; static_cast<unsigned>(abs(static_cast<int>(DataStartAddress - PageStartAddress))) < Stats_.PageSize_;
         DataStartAddress += MidBlockSize_)
    {
      GenericObject *dataAddress = reinterpret_cast<GenericObject *>(DataStartAddress);

      PushToFreeList(dataAddress);

      if (Config_.DebugOn_)
      {
        std::memset(reinterpret_cast<BYTE *>(dataAddress) + PTR_SIZE, UNALLOCATED_PATTERN, Stats_.ObjectSize_ - PTR_SIZE);
        std::memset(GetLeftPadAdrress(dataAddress), PAD_PATTERN, Config_.PadBytes_);
        std::memset(GetRightPadAdrress(dataAddress), PAD_PATTERN, Config_.PadBytes_);
      }
      std::memset(GetHeaderAddress(dataAddress), 0, Config_.HBlockInfo_.size_);
    }
  }
}

/******************************************************************************/
/*!
\brief
  This function puts an object at the front of the free list.

\par object The object to put on the free list.
*/
/******************************************************************************/
void ObjectAllocator::PushToFreeList(GenericObject *object)
{
  GenericObject *temp = FreeList_;
  FreeList_ = object;
  object->Next = temp;

  Stats_.FreeObjects_++;
}

/******************************************************************************/
/*!
\brief
  This function checks if a given address is on a valid boundary.

\par address The address to validate.
*/
/******************************************************************************/
void ObjectAllocator::CheckBoundaries(unsigned char *address) const
{
  GenericObject *pageList = PageList_;

  while (!IsObjectInPage(pageList, address))
  {
    pageList = pageList->Next;

    if (!pageList)
    {
      throw OAException{OAException::E_BAD_BOUNDARY, "CheckBoundaries: Address is not on boundary!"};
    }
  }

  BYTE *pageStart = reinterpret_cast<BYTE *>(pageList);

  if (static_cast<unsigned>(address - pageStart) < HeaderSize_)
    throw OAException{OAException::E_BAD_BOUNDARY, "CheckBoundaries: Address is not on boundary!"};

  pageStart += HeaderSize_;
  long displacement = address - pageStart;
  if (static_cast<size_t>(displacement) % MidBlockSize_ != 0)
    throw OAException{OAException::E_BAD_BOUNDARY, "CheckBoundaries: Address is not on boundary!"};
}

/******************************************************************************/
/*!
\brief
  This function checks if a given padding address is valid.

\par address The address to validate.
\return Returns \p true if padding is valid, else \p false if corrupted.
*/
/******************************************************************************/
bool ObjectAllocator::ValidatePadding(unsigned char *paddingAddress, size_t size) const
{
  for (size_t i = 0; i < size; ++i)
  {
    if (*(paddingAddress + i) != ObjectAllocator::PAD_PATTERN)
      return false;
  }
  return true;
}

/******************************************************************************/
/*!
\brief
  This function checks if a given address is in a page.

\par pageAddress The page to check.
\par address The address to check.
\return Returns \p true if address is in page, else \p false.
*/
/******************************************************************************/
bool ObjectAllocator::IsObjectInPage(GenericObject *pageAddress, unsigned char *address) const
{
  return (address >= reinterpret_cast<BYTE *>(pageAddress) &&
          address < reinterpret_cast<BYTE *>(pageAddress) + Stats_.PageSize_);
}

/******************************************************************************/
/*!
\brief
  This function checks if a given block address is currently used.

\par object The object to check.
\return Returns \p true if block is used, else \p false.
*/
/******************************************************************************/
bool ObjectAllocator::IsObjectUsed(GenericObject *object) const
{
  switch (Config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone:
  {
    GenericObject *freelist = FreeList_;
    while (freelist)
    {
      if (freelist == object)
        return true;
      freelist = freelist->Next;
    }
    return false;
  }
  case OAConfig::HBLOCK_TYPE::hbBasic:
  case OAConfig::HBLOCK_TYPE::hbExtended:
  {
    BYTE *flagByte = reinterpret_cast<BYTE *>(object) - Config_.PadBytes_ - 1;
    return *flagByte;
  }
  case OAConfig::HBLOCK_TYPE::hbExternal:
  {
    return *GetHeaderAddress(object);
  }
  default:
    return false;
    break;
  }
}

/******************************************************************************/
/*!
\brief
  This function checks if a given page is not used at all.

\par page The page to check.
\return Returns \p true if page is free, else \p false.
*/
/******************************************************************************/
bool ObjectAllocator::IsPageFree(GenericObject *page) const
{
  GenericObject *temp = FreeList_;
  unsigned freeObjectsInPage = 0;
  while (temp)
  {
    if (IsObjectInPage(page, reinterpret_cast<BYTE *>(temp)))
    {
      if (++freeObjectsInPage > Config_.ObjectsPerPage_ - 1)
        return true;
    }
    temp = temp->Next;
  }
  return false;
}

/******************************************************************************/
/*!
\brief
  This function returns memory used by a page back to the OS.

\par page The page to free.
*/
/******************************************************************************/
void ObjectAllocator::FreePage(GenericObject *page)
{
  GenericObject *temp = FreeList_;
  GenericObject *prev = nullptr;

  while (temp && IsObjectInPage(page, reinterpret_cast<BYTE *>(temp)))
  {
    FreeList_ = temp->Next;
    temp = this->FreeList_;
    --Stats_.FreeObjects_;
  }

  while (temp)
  {
    while (temp && !IsObjectInPage(page, reinterpret_cast<BYTE *>(temp)))
    {
      prev = temp;
      temp = temp->Next;
    }

    if (!temp)
      break;
    prev->Next = temp->Next;

    --Stats_.FreeObjects_;
    temp = prev->Next;
  }

  delete[] reinterpret_cast<BYTE *>(page);
  --Stats_.PagesInUse_;
}

/******************************************************************************/
/*!
\brief
  This function formats and initializes the header of a block.

\par object The block to initialize.
\par headerType The type of header to initialize.
\par label_ A cstring label for the block of memory.
*/
/******************************************************************************/
void ObjectAllocator::InitHeader(GenericObject *object, OAConfig::HBLOCK_TYPE headerType, const char *label_)
{
  switch (headerType)
  {
  case OAConfig::hbBasic:
  {
    BYTE *headerAddress = GetHeaderAddress(object);
    unsigned *allocationNumber = reinterpret_cast<unsigned *>(headerAddress);
    *allocationNumber = Stats_.Allocations_;

    BYTE *flag = reinterpret_cast<BYTE *>(allocationNumber + 1);
    *flag = true;
  }
  break;
  case OAConfig::hbExtended:
  {
    BYTE *headerAddress = GetHeaderAddress(object);
    unsigned short *counter = reinterpret_cast<unsigned short *>(headerAddress + Config_.HBlockInfo_.additional_);
    ++(*counter);

    unsigned *allocationNumber = reinterpret_cast<unsigned *>(counter + 1);
    *allocationNumber = Stats_.Allocations_;

    BYTE *flag = reinterpret_cast<BYTE *>(allocationNumber + 1);
    *flag = true;
  }
  break;
  case OAConfig::hbExternal:
  {
    BYTE *headerAddress = GetHeaderAddress(object);
    MemBlockInfo **memPtr = reinterpret_cast<MemBlockInfo **>(headerAddress);
    try
    {
      *memPtr = new MemBlockInfo{true, nullptr, Stats_.Allocations_};
      if (label_)
      {
        try
        {
          (*memPtr)->label = new char[std::strlen(label_) + 1];
        }
        catch (std::bad_alloc &)
        {
          throw OAException(OAException::E_NO_MEMORY, "InitHeader: No system memory available!");
        }
        std::strcpy((*memPtr)->label, label_);
      }
    }
    catch (std::bad_alloc &)
    {
      throw OAException(OAException::E_NO_MEMORY, "InitHeader: No system memory available!");
    }
  }
  break;
  default:
    break;
  }
}

/******************************************************************************/
/*!
\brief
  This function sets a block's header to free / unused.

\par object The block to free.
\par headerType The type of header to free.
*/
/******************************************************************************/
void ObjectAllocator::FreeHeader(GenericObject *object, OAConfig::HBLOCK_TYPE headerType)
{
  BYTE *headerAddress = GetHeaderAddress(object);
  switch (headerType)
  {
  case OAConfig::hbNone:
  {
    if (Config_.DebugOn_)
    {
      BYTE *lastByte = reinterpret_cast<BYTE *>(object) + Stats_.ObjectSize_ - 1;
      if (*lastByte == ObjectAllocator::FREED_PATTERN)
        throw OAException{OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed!"};
    }
  }
  break;
  case OAConfig::hbBasic:
  {
    if (Config_.DebugOn_)
    {
      if (0 == *(headerAddress + sizeof(unsigned)))
        throw OAException{OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed!"};
    }

    std::memset(headerAddress, 0, OAConfig::BASIC_HEADER_SIZE);
  }
  break;
  case OAConfig::hbExtended:
  {
    if (Config_.DebugOn_)
    {
      if (0 == *(headerAddress + sizeof(unsigned) + this->Config_.HBlockInfo_.additional_ + sizeof(unsigned short)))
        throw OAException(OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed!");
    }

    std::memset(headerAddress + this->Config_.HBlockInfo_.additional_ + sizeof(unsigned short), 0, OAConfig::BASIC_HEADER_SIZE);
  }
  break;
  case OAConfig::hbExternal:
  {
    MemBlockInfo **info = reinterpret_cast<MemBlockInfo **>(headerAddress);
    if (nullptr == *info && Config_.DebugOn_)
      return;

    if ((*info)->label)
      delete[](*info)->label;
    delete *info;
    *info = nullptr;
  }
  break;
  default:
    break;
  }
}

/******************************************************************************/
/*!
\brief
  This function returns the header address of a block.

\par object The block to get the header address from.
\return A pointer to the header of the block.
*/
/******************************************************************************/
unsigned char *ObjectAllocator::GetHeaderAddress(GenericObject *object) const
{
  return reinterpret_cast<BYTE *>(object) - Config_.HBlockInfo_.size_ - Config_.PadBytes_;
}

/******************************************************************************/
/*!
\brief
  This function returns the left padding address of a block.

\par object The block to get the left padding address from.
\return A pointer to the left padding of the block.
*/
/******************************************************************************/
unsigned char *ObjectAllocator::GetLeftPadAdrress(GenericObject *object) const
{
  return reinterpret_cast<BYTE *>(object) - Config_.PadBytes_;
}

/******************************************************************************/
/*!
\brief
  This function returns the right padding address of a block.

\par object The block to get the right padding address from.
\return A pointer to the right padding of the block.
*/
/******************************************************************************/
unsigned char *ObjectAllocator::GetRightPadAdrress(GenericObject *object) const
{
  return reinterpret_cast<BYTE *>(object) + Stats_.ObjectSize_;
}