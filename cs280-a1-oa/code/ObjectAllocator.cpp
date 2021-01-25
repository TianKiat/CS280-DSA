
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

///TODO: Everything after Free(void *Object); https://github.com/ShumWengSang/CS280/blob/master/Assignment1/src/ObjectAllocator.cpp

using BYTE = unsigned char;                 //!< Type of byte
constexpr size_t PTR_SIZE = sizeof(void *); //!< Size of a pointer

inline size_t Align(size_t n, size_t alignment)
{
  if (!alignment)
    return n;

  size_t rem = n % alignment == 0 ? 0 : 1;
  return alignment * ((n / alignment) + rem);
}

ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig &config)
    : PageList_{nullptr}, FreeList_{nullptr}, Config_{config}, Stats_{}
{
  HeaderSize_ = Align(PTR_SIZE + config.HBlockInfo_.size_ + config.PadBytes_, config.Alignment_);
  size_t midBlockSize = static_cast<size_t>(config.PadBytes_ * 2) + ObjectSize + config.HBlockInfo_.size_;
  MidBlockSize_ = Align(midBlockSize, config.Alignment_);
  Stats_.ObjectSize_ = ObjectSize;
  Config_.InterAlignSize_ = MidBlockSize_ - midBlockSize;

  Stats_.PageSize_ = HeaderSize_ + (config.ObjectsPerPage_ - 1) * MidBlockSize_ + ObjectSize + config.PadBytes_;

  unsigned int leftHeaderSize = static_cast<unsigned int>(PTR_SIZE + config.HBlockInfo_.size_ + static_cast<size_t>(config.PadBytes_));
  Config_.LeftAlignSize_ = static_cast<unsigned int>(Align(leftHeaderSize, config.Alignment_) - leftHeaderSize);

  AllocateNewPage_s(PageList_);
}

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

void *ObjectAllocator::Allocate(const char *label = 0)
{
  if (Config_.UseCPPMemManager_)
  {
    try
    {
      BYTE *newObject = new BYTE[Stats_.ObjectSize_];
      UpdateStats();
      return newObject;
    }
    catch (const std::bad_alloc &)
    {
      throw OAException{OAException::E_NO_MEMORY, "Allocate: No system memory available."};
    }
  }

  if (!FreeList_)
  {
    AllocateNewPage_s(PageList_);
  }

  GenericObject *AllocatedObject = FreeList_;
  FreeList_ = FreeList_->Next;

  if (Config_.DebugOn_)
    std::memset(AllocatedObject, ALLOCATED_PATTERN, Stats_.ObjectSize_);

  UpdateStats();
  InitHeader(AllocatedObject, Config_.HBlockInfo_.type_, label);

  return AllocatedObject;
}

void ObjectAllocator::Free(void *Object)
{
  ++Stats_.Deallocations_;

  //If we are not using the mem manager.
  if (Config_.UseCPPMemManager_)
  {
    delete[] reinterpret_cast<BYTE *>(Object);
    return;
  }

  GenericObject *object = reinterpret_cast<GenericObject *>(Object);

  if (Config_.DebugOn_)
  {
    CheckBoundaries(reinterpret_cast<unsigned char *>(Object));
    {
      if (!ValidatePadding(GetLeftPadAdrress(object), Config_.PadBytes_))
      {
        throw OAException{OAException::E_CORRUPTED_BLOCK, "Free: Object not on a boundary."};
      }
      if (!ValidatePadding(GetRightPadAdrress(object), Config_.PadBytes_))
      {
        throw OAException{OAException::E_CORRUPTED_BLOCK, "Free: Object not on a boundary."};
      }
    }
  }

  FreeHeader(object, Config_.HBlockInfo_.type_);

  if (Config_.DebugOn_)
  {
    memset(object, FREED_PATTERN, Stats_.ObjectSize_);
  }
  object->Next = nullptr;

  PushToFreeList(object);
  // Update stats
  Stats_.ObjectsInUse_;
}

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

unsigned ObjectAllocator::FreeEmptyPages()
{
  if (!PageList_)
    return 0;
  unsigned numEmptyPages = 0;
  GenericObject *tempHead = PageList_;
  GenericObject *prevHead = nullptr;

  while (tempHead && IsPageEmpty(tempHead))
  {
    PageList_ = tempHead->Next;
    FreePage(tempHead);
    tempHead = this->PageList_;
    ++numEmptyPages;
  }

  while (tempHead)
  {
    while (tempHead && !IsPageEmpty(tempHead))
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

void ObjectAllocator::SetDebugState(bool State)
{
  Config_.DebugOn_ = State;
}

const void *ObjectAllocator::GetFreeList() const
{
  return FreeList_;
}

const void *ObjectAllocator::GetPageList() const
{
  return PageList_;
}

OAConfig ObjectAllocator::GetConfig() const
{
  return Config_;
}

OAStats ObjectAllocator::GetStats() const
{
  return Stats_;
}

GenericObject *ObjectAllocator::AllocateNewPage(size_t pageSize)
{
  try
  {
    GenericObject *newPage = reinterpret_cast<GenericObject *>(new unsigned char[pageSize]());
    ++Stats_.PagesInUse_;
    return newPage;
  }
  catch (std::bad_alloc &)
  {
    throw OAException{OAException::OA_EXCEPTION::E_NO_MEMORY, "AllocateNewPage: No system memory available."};
  }
}

void ObjectAllocator::AllocateNewPage_s(GenericObject *&pageList)
{
  if (Stats_.PagesInUse_ == Config_.MaxPages_)
  {
    throw OAException{OAException::OA_EXCEPTION::E_NO_PAGES, "AllocateNewPage_s: No logical memory available"};
  }
  else
  {
    GenericObject *newPage = AllocateNewPage(Stats_.PageSize_);

    if (Config_.DebugOn_)
    {
      std::memset(newPage, ALIGN_PATTERN, Stats_.PageSize_);
    }
    // Link new page to page list
    newPage->Next = pageList;
    pageList = newPage;

    //TODO: CAST BLOCKS INTO OBJECTS AND PUT THEM ON THE FREE LIST REFERENCE LINE: 612
    BYTE *PageStartAddress = reinterpret_cast<BYTE *>(newPage);
    BYTE *ObjectStartAddress = PageStartAddress + HeaderSize_;

    for (; static_cast<unsigned>(abs(static_cast<int>(ObjectStartAddress - PageStartAddress))) < Stats_.PageSize_;
         ObjectStartAddress += MidBlockSize_)
    {
      GenericObject *ObjectAddress = reinterpret_cast<GenericObject *>(ObjectStartAddress);
      PushToFreeList(ObjectAddress);

      if (Config_.DebugOn_)
      {
        memset(reinterpret_cast<BYTE *>(ObjectAddress) + PTR_SIZE, UNALLOCATED_PATTERN, Stats_.ObjectSize_ - PTR_SIZE);
        memset(GetLeftPadAdrress(ObjectAddress), PAD_PATTERN, Config_.PadBytes_);
        memset(GetRightPadAdrress(ObjectAddress), PAD_PATTERN, Config_.PadBytes_);
      }
      memset(GetHeaderAddress(ObjectAddress), 0, Config_.HBlockInfo_.size_);
    }
  }
}

void ObjectAllocator::PushToFreeList(GenericObject *object)
{
  GenericObject *temp = FreeList_;
  FreeList_ = object;
  object->Next = temp;

  ++Stats_.FreeObjects_;
}

void ObjectAllocator::UpdateStats()
{
  ++Stats_.ObjectsInUse_;
  if (Stats_.ObjectsInUse_ > Stats_.MostObjects_)
    Stats_.MostObjects_ = Stats_.ObjectsInUse_;
  --Stats_.FreeObjects_;
  ++Stats_.Allocations_;
}

void ObjectAllocator::CheckBoundaries(unsigned char *address) const
{
  // Find the page the object rests in.
  GenericObject *pageList = PageList_;
  // While loop stops when addr resides within the page.
  while (!IsObjectInPage(pageList, address))
  {
    pageList = pageList->Next;
    // If its not in our pages, its not our memory.
    if (!pageList)
    {
      throw OAException{OAException::E_BAD_BOUNDARY, "CheckBoundaries: Object not on a boundary."};
    }
  }
  // We have found that is is in our pages. Check the boundary using %
  BYTE *pageStart = reinterpret_cast<BYTE *>(pageList);

  // Check if we are intruding on header.
  if (static_cast<unsigned>(address - pageStart) < HeaderSize_)
    throw OAException{OAException::E_BAD_BOUNDARY, "CheckBoundaries: Object not on a boundary."};

  pageStart += HeaderSize_;
  long displacement = address - pageStart;
  if (static_cast<size_t>(displacement) % MidBlockSize_ != 0)
    throw OAException{OAException::E_BAD_BOUNDARY, "CheckBoundaries: Object not on a boundary."};
}

bool ObjectAllocator::ValidatePadding(unsigned char *paddingAddress, size_t size) const
{
  for (size_t i = 0; i < size; ++i)
  {
    if (*(paddingAddress + i) != ObjectAllocator::PAD_PATTERN)
      return false;
  }
  return true;
}

bool ObjectAllocator::IsObjectInPage(GenericObject *pageAddress, unsigned char *address) const
{
  return (address >= reinterpret_cast<BYTE *>(pageAddress) &&
          address < reinterpret_cast<BYTE *>(pageAddress) + Stats_.PageSize_);
}

bool ObjectAllocator::IsObjectUsed(GenericObject *object) const
{
  switch (Config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone:
  {
    // Checks if it is in free list.
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

bool ObjectAllocator::IsPageEmpty(GenericObject *page) const
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

bool ObjectAllocator::FreePage(GenericObject *page)
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
      return;
    prev->Next = temp->Next;

    --Stats_.FreeObjects_;
    temp = prev->Next;
  }

  delete[] reinterpret_cast<BYTE *>(page);
  --Stats_.PagesInUse_;
}

void ObjectAllocator::InitHeader(GenericObject *object, OAConfig::HBLOCK_TYPE headerType, const char *label)
{
  switch (headerType)
  {
  case OAConfig::hbBasic:
  {
    BYTE *headerAddress = GetHeaderAddress(object);
    unsigned *allocationNumber = reinterpret_cast<unsigned *>(headerAddress);
    *allocationNumber = Stats_.Allocations_;
    // Now set the allocation flag
    BYTE *flag = reinterpret_cast<BYTE *>(allocationNumber + 1);
    *flag = true;
  }
  break;
  case OAConfig::hbExtended:
  {
    BYTE *headerAddress = GetHeaderAddress(object);
    // Set the 2 byte use-counter, 5 for 5 bytes of user defined stuff.
    unsigned short *counter = reinterpret_cast<unsigned short *>(headerAddress + Config_.HBlockInfo_.additional_);
    ++(*counter);

    unsigned *allocationNumber = reinterpret_cast<unsigned *>(counter + 1);
    *allocationNumber = Stats_.Allocations_;
    // Now set the allocation flag
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
      *memPtr = new MemBlockInfo{};
      (*memPtr)->in_use = true;
      (*memPtr)->alloc_num = Stats_.Allocations_;
      if (label)
      {
        try
        {
          (*memPtr)->label = new char[strlen(label) + 1];
        }
        catch (std::bad_alloc &)
        {
          throw OAException(OAException::E_NO_MEMORY, "InitHeader: No system memory available.");
        }
        strcpy((*memPtr)->label, label);
      }
    }
    catch (std::bad_alloc &)
    {
      throw OAException(OAException::E_NO_MEMORY, "InitHeader: No system memory available.");
    }
  }
  break;
  default:
    break;
  }
}

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
        throw OAException{OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed."};
    }
  }
  break;
  case OAConfig::hbBasic:
  {
    // Check if the bit is already free
    if (Config_.DebugOn_)
    {
      if (0 == *(headerAddress + sizeof(unsigned)))
        throw OAException{OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed."};
    }
    // Reset the basic header
    std::memset(headerAddress, 0, OAConfig::BASIC_HEADER_SIZE);
  }
  break;
  case OAConfig::hbExtended:
  {
    if (Config_.DebugOn_)
    {
      if (0 == *(headerAddress + sizeof(unsigned) + this->Config_.HBlockInfo_.additional_ + sizeof(unsigned short)))
        throw OAException(OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed.");
    }
    // Reset the basic header part of the extended to 0
    memset(headerAddress + this->Config_.HBlockInfo_.additional_ + sizeof(unsigned short), 0, OAConfig::BASIC_HEADER_SIZE);
  }
  break;
  case OAConfig::hbExternal:
  {
    // Free the external values

    MemBlockInfo **info = reinterpret_cast<MemBlockInfo **>(headerAddress);
    if (nullptr == *info && Config_.DebugOn_)
      throw OAException(OAException::E_MULTIPLE_FREE, "FreeHeader: Object has already been freed.");

    delete *info;
    *info = nullptr;
  }
  break;
  default:
    break;
  }
}

unsigned char *ObjectAllocator::GetHeaderAddress(GenericObject *object) const
{
  return reinterpret_cast<BYTE *>(object) - Config_.HBlockInfo_.size_ - Config_.PadBytes_;
}
unsigned char *ObjectAllocator::GetLeftPadAdrress(GenericObject *object) const
{
  return reinterpret_cast<BYTE *>(object) - Config_.PadBytes_;
}
unsigned char *ObjectAllocator::GetRightPadAdrress(GenericObject *object) const
{
  return reinterpret_cast<BYTE *>(object) + Config_.PadBytes_;
}