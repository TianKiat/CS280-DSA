/******************************************************************************/
/*!
\file   ObjectAllocator.cpp
\author Roland Shum
\par    email: roland.shum\@digipen.edu
\par    DigiPen login: roland.shum
\par    Course: CS280
\par    Assignment #1
\date   1/24/2020
\brief
  This is the interface for the Object Allocator and its accompanying classes.
  This includes OAConfig, MemBlockInfo, and ObjectAllocator.

  ObjectAllocator Functions
  - Constructor
  - Destructor
  - Allocate
  - Free
  - Debug functions:
    - DumpMemoryInUse
    - ValidePages
    - FreeEmptyPages
    - SetDebugState
    - GetFreeList
    - GetPageList
    - GetConfig
    - GetStats
  
  OAConfig
  - Constructor

  MemBlockInfo
  - Constructor
*/
/******************************************************************************/
#include "ObjectAllocator.h"
#include <cstdint>   // size_t 
#include <cstring>   // strlen, memset
#include <cstdlib>   // abs
#include <iostream>
#define PTR_SIZE sizeof(word_t)      //! Size of a pointer.
#define INCREMENT_PTR(ptr) (ptr + 1) //! Move the pointer by one

//! A word is a pointer
using word_t = intptr_t ;

/******************************************************************************/
/*!

  Pass in a literal to convert it to a size_t type. Example: 2_z; Similar
  to how 2.0f is a float.
   
  \param n
    A number to turn into size_t type in compile time.
  \return
    Returns n in size_t type.
*/
/******************************************************************************/
constexpr size_t operator "" _z(unsigned long long n)
{
    return static_cast<size_t>(n);
}

/******************************************************************************/
/*!

  Given a size and an alignment, returns the size after it has been aligned.
  Ex: Given a size of 7 and align of 8, returns 8. If size of 11, align of 8,
  returns 16.

  \param n
    The size to align
  \param align
    The power of size to align to.
  \return 
    Returns n, expanded so that it aligns with align
*/
/******************************************************************************/
inline size_t align(size_t n, size_t align)
{
    if (!align)
        return n;
    size_t remainder = n % align == 0 ? 0_z : 1_z;
    return align * ((n / align) + remainder);
}

/******************************************************************************/
/*!

  Constructor for the mem block info.

  \param alloc_num_
    The allocation number/ID.
  \param label
    C string to identify the header / data block.
*/
/******************************************************************************/
MemBlockInfo::MemBlockInfo(unsigned alloc_num_, const char* label_): in_use(true),
    label(nullptr), alloc_num(alloc_num_)
{
    if (label_)
    {
        try
        {
            // +1 to account for null.
            label = new char[strlen(label_) + 1];
        }
        catch (std::bad_alloc&)
        {
            throw OAException(OAException::E_NO_MEMORY, "Out of memory!");
        }
        strcpy(label, label_);
    }
}

/******************************************************************************/
/*!

  Destructor for MemBlockInfo. Deletes the C-string array

*/
/******************************************************************************/
MemBlockInfo::~MemBlockInfo()
{
    delete[] label;
}

/******************************************************************************/
/*!

  Constructor for Object Allocator. Determines stats and allocates inital page
  of memory.

  /param ObjectSize
    The size of each object that the allocator allocates.
  /param config
    An instance of OAConfig that determines the configuration of the Object
    Allocator

*/
/******************************************************************************/
ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig &config) : configuration(config)
{
    // TODO: Put this in constructor 
    // First we have to calculate all the stats
    this->headerSize = align(PTR_SIZE + config.HBlockInfo_.size_ + config.PadBytes_, config.Alignment_);
    this->dataSize = align(ObjectSize + config.PadBytes_ * 2_z + config.HBlockInfo_.size_, config.Alignment_);
    std::cout << dataSize << std::endl;
    this->stats.ObjectSize_ = ObjectSize;
    this->stats.PageSize_ = headerSize + dataSize * (config.ObjectsPerPage_ - 1) + ObjectSize + config.PadBytes_;
    this->totalDataSize = dataSize * (configuration.ObjectsPerPage_ - 1) + ObjectSize + config.PadBytes_;
    
    size_t interSize = ObjectSize + this->configuration.PadBytes_ * 2_z + static_cast<size_t>(this->configuration.HBlockInfo_.size_);
    this->configuration.InterAlignSize_ = static_cast<unsigned>(align(interSize, this->configuration.Alignment_) - interSize);
    allocate_new_page_safe(this->PageList_);
}

/******************************************************************************/
/*!

  Destructor for Object Allocator. Removes pages.

*/
/******************************************************************************/
ObjectAllocator::~ObjectAllocator()
{
    // We will need to call the destructor for every active object, and then
    // free the entire page
    // Walk the pages
    GenericObject* page = this->PageList_;
    while(page != nullptr)
    {
        GenericObject* next = page->Next;
        // Free external headers.
        if(this->configuration.HBlockInfo_.type_ == OAConfig::hbExternal)
        {
            unsigned char* headerAddr = reinterpret_cast<unsigned char*>(page) + this->headerSize;
            for(unsigned i = 0; i < configuration.ObjectsPerPage_; i++)
            {
                freeHeader(reinterpret_cast<GenericObject*>(headerAddr), OAConfig::hbExternal, true);
            }
        }
        delete[] reinterpret_cast<unsigned char*>(page);
        page = next;
    }
}

/******************************************************************************/
/*!

  Given a label, allocates memory from the page and returns it to the user.
  If UseCPPMemManager is true, bypasses OA and use new.
  
  
  \param label
    Label to pass to the external header to keep track of.

  \return
    Pointer to block of memory to use.
  
  \exception OAException
    Exception contains the type of exception it is. Possible types are
    -OAException::E_NO_MEMORY -- signifies no more memory from OS
    -OAException::E_NO_PAGES  -- There are no more memory in the pages to use,
                                 and the configuration also does not allow more
                                 memory.
*/
/******************************************************************************/
void* ObjectAllocator::Allocate(const char* label)
{
    //If we are not using the mem manager.
    if (this->configuration.UseCPPMemManager_)
    {
        try 
        {
            unsigned char* newObj = new unsigned char[this->stats.ObjectSize_];
            incrementStats();
            return newObj;
        }
        catch (std::bad_alloc&)
        {
            throw OAException(OAException::E_NO_MEMORY, "Out of memory!");
        }
    }

    // First we check if we can allocate from freelist.
    if (nullptr == FreeList_)
    {
        // Nothing left in freelist, allocate new page.
        allocate_new_page_safe(this->PageList_);
    }
    // Give them from the new page.
    GenericObject* objectToGive = this->FreeList_;
    
    // Update the free list
    this->FreeList_ = this->FreeList_->Next;

    // Update sig
    if (this->configuration.DebugOn_)
    {
        memset(objectToGive, ALLOCATED_PATTERN, stats.ObjectSize_);
    }
    incrementStats();
    // Update header
    updateHeader(objectToGive, configuration.HBlockInfo_.type_, label);
    
    
    // Return the object
    return objectToGive;
}

/******************************************************************************/
/*!

  Given a block of memory given by Allocate(), returns the memory back to the
  Object Allocator.  If UseCPPMemManager is true, bypasses OA and use delete.
  If debugOn is True, will perform checking to see if the block is corrupted 
  and within the same boundaries.

  \param Object
    Pointer to object allocated by Allocate().
  \exception OAException
    Exception contains the type of exception it is. Possible types are
    -OAException::E_CORRUPTED_BLOCK -- The memory block is corrupted.
    -OAException::E_BAD_BOUNDARY    -- The Object given is not aligned properly.
                                       Possibly wrong object.
*/
/******************************************************************************/
void ObjectAllocator::Free(void* Object)
{
    // Increment Deallocation count
    ++this->stats.Deallocations_;

    //If we are not using the mem manager.
    if (this->configuration.UseCPPMemManager_) 
    {
        delete[] reinterpret_cast<unsigned char*>(Object);
        return;
    }


    GenericObject* object = reinterpret_cast<GenericObject*>(Object);
    // Check if pad bytes are OK AKA boundary check
    // If there are no paddings...

    if (this->configuration.DebugOn_)
    {
        check_boundary_full(reinterpret_cast<unsigned char*>(Object));
        {
            if (!isPaddingCorrect(toLeftPad(object), this->configuration.PadBytes_))
            {
                throw OAException(OAException::E_CORRUPTED_BLOCK, "Bad boundary.");
            }
            if (!isPaddingCorrect(toRightPad(object), this->configuration.PadBytes_))
            {
                throw OAException(OAException::E_CORRUPTED_BLOCK, "Bad boundary.");
            }
        }
    }
    
    freeHeader(object, this->configuration.HBlockInfo_.type_);

    if (this->configuration.DebugOn_)
    {
        memset(object, FREED_PATTERN, stats.ObjectSize_);
    }
    object->Next = nullptr;


    

    put_on_freelist(object);
    // Update stats
    --this->stats.ObjectsInUse_;
}

/******************************************************************************/
/*!

  Debug Function: Walks through the pages in the OA and calls the given 
  function if the blocks of data are active

  \parem fn
    Function to call for each active object
  \return
    Number of active objects
*/
/******************************************************************************/
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const
{
    // Walk through each page, and dump it.
    if (!PageList_)
    {
        // empty list 
        return 0 ;
    }
    else 
    {
        unsigned memInUse = 0;
        // walk to the end, while calling the fn for each page.
        GenericObject* last = PageList_;
        while (last)
        {
            unsigned char* block = reinterpret_cast<unsigned char*>(last);
            // Move to the first data block.
            block += this->headerSize;

			// For each data
			for (unsigned i = 0; i < configuration.ObjectsPerPage_; ++i)
			{
				GenericObject* objectData = reinterpret_cast<GenericObject*>(block + i * dataSize);

                if (isObjectAllocated(objectData))
                {
					fn(objectData, stats.ObjectSize_);
					++memInUse;
                }
				// Check if mem is in use. By using slower checkData function
				//if (checkData(objectData, ALLOCATED_PATTERN))
				//{
				//	fn(objectData, stats.ObjectSize_);
				//	++memInUse;
				//}
			}

            last = last->Next;
        }
        return memInUse;
    }
}

/******************************************************************************/
/*!

  Debug Function: Walks through each page and verifies if each data block is
  corrupted or not.

  \param fn
    Function to call once a data block is identified to be corrupted.
  \return
    Number of corrupted data blocks.
*/
/******************************************************************************/
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const
{
    if (!configuration.DebugOn_ || !configuration.PadBytes_)
        return 0;
    unsigned corruptedBlocks = 0;

    //You need to walk each of the pages in the page list checking
    // the pad bytes of each block (free or not).
    GenericObject* last = PageList_;
    while (last)
    {
        unsigned char* block = reinterpret_cast<unsigned char*>(last);
        // Move to the first data block.
        block += this->headerSize;

        // For each data
        for(unsigned i = 0; i < configuration.ObjectsPerPage_; ++i)
        {
            GenericObject* objectData = reinterpret_cast<GenericObject*>(block + i * dataSize);
            // Validate the left and right pad
            if(!isPaddingCorrect(toLeftPad(objectData), configuration.PadBytes_) || 
                !isPaddingCorrect(toRightPad(objectData), configuration.PadBytes_))
            {
                fn(objectData, stats.ObjectSize_);
                ++corruptedBlocks;
                continue;
            }
        }
        last = last->Next;
    }
    
    return corruptedBlocks;
}

/******************************************************************************/
/*!

  Debug Function: Walks through each page and checks if the page is empty.
  If it is, returns the memory of the page back to the OS.

  \return
    The number of pages freed.
*/
/******************************************************************************/
unsigned ObjectAllocator::FreeEmptyPages()
{
    if (this->PageList_ == nullptr)
        return 0;
    // Return value
    unsigned emptyPages = 0;
    
    // Store head node 
    GenericObject* temp = this->PageList_, * prev = nullptr;

    // If head node itself holds the key or multiple occurrences of key 
    while (temp != nullptr && isPageEmpty(temp))
    {
        this->PageList_ = temp->Next;   // Changed head 
        freePage(temp);
        temp = this->PageList_;         // Change Temp 
        emptyPages++;
    }

    // Delete occurrences other than head 
    while (temp != nullptr)
    {
        // Search for the key to be deleted, keep track of the 
        // previous node as we need to change 'prev->next' 
        while (temp != nullptr && !isPageEmpty(temp))
        {
            prev = temp;
            temp = temp->Next;
        }

        // If key was not present in linked list 
        if (temp == nullptr) return emptyPages;

        // Unlink the node from linked list 
        prev->Next = temp->Next;
        freePage(temp);

        //Update Temp for next iteration of outer loop 
        temp = prev->Next;
        emptyPages++;
    }
    return emptyPages;
}

/******************************************************************************/
/*!

  Given a page, frees it from memory by returning it to the OS. It first
  removes all the datablocks from the freelist, and decrements the stat counter.
  
*/
/******************************************************************************/
void ObjectAllocator::freePage(GenericObject* temp)
{
    removePageObjs_from_freelist(temp);
    delete[] reinterpret_cast<unsigned char*>(temp);
    this->stats.PagesInUse_--;
    
}

/******************************************************************************/
/*!

  Does this OA implement extra credit? (Hint: Yes)

  \return
    Returns true if implemented. False otherwise.
*/
/******************************************************************************/
bool ObjectAllocator::ImplementedExtraCredit()
{
    return true;
}

/******************************************************************************/
/*!

  Given a debug state, sets the current OA to that debug state.
  
  \param State
    True to turn on Debug, False to turn off.
*/
/******************************************************************************/
void ObjectAllocator::SetDebugState(bool State)
{
    this->configuration.DebugOn_ = State;
}

/******************************************************************************/
/*!

  Debug Function: Returns the free list, a linked list determining what blocks
  of memory are free.

  \return
    The free list.
*/
/******************************************************************************/
const void* ObjectAllocator::GetFreeList() const
{
    return FreeList_;
}

/******************************************************************************/
/*!

  Debug Function: Returns the page list, a linked list that chains all the pages
  together.

  \return
    The page list.
*/
/******************************************************************************/
const void* ObjectAllocator::GetPageList() const
{
    return PageList_;
}

/******************************************************************************/
/*!

  Debug Function: Returns a copy of the current configuration

  \return
    The a copy of the current configuration
*/
/******************************************************************************/
OAConfig ObjectAllocator::GetConfig() const
{
    return this->configuration;
}

/******************************************************************************/
/*!

  Debug Function: Returns a copy of the current stats

  \return
    The a copy of the current stats
*/
/******************************************************************************/
OAStats ObjectAllocator::GetStats() const
{
    return this->stats;
}

/******************************************************************************/
/*!

  Allocates a new page after checks. Checks if we have hit the max number of 
  pages we are allowed to allocate.

  \param LPageList
    A reference to the head of the linked list of the pages.
  \exceptions OAException
    OAException::OA_EXCEPTION::E_NO_PAGES  - Ran of out pages to use.
    OAException::OA_EXCEPTION::E_NO_MEMORY - Signifies no more memory from OS
*/
/******************************************************************************/
void ObjectAllocator::allocate_new_page_safe(GenericObject *&LPageList)
{
    // If we have hit the max amount of pages...
    if (stats.PagesInUse_ == configuration.MaxPages_)
    {
        // Max pages have been reached.
        throw OAException(OAException::OA_EXCEPTION::E_NO_PAGES, "Out of pages!");
    }
    // If we can still allocate pages.
    else
    {
        // DEBUG TODO REMOVE LATER
        if (stats.PagesInUse_ >= configuration.MaxPages_)
            throw std::exception();
        // Allocate a new page.
        GenericObject* newPage = allocate_new_page(this->stats.PageSize_);

        if (this->configuration.DebugOn_)
        {
            memset(newPage, ALIGN_PATTERN, this->stats.PageSize_);
        }
        
        // Link it up to the page list.
        InsertHead(LPageList, newPage);


        // Putting objects on free list
        unsigned char* PageStartAddress = reinterpret_cast<unsigned char*>(newPage);
        unsigned char* DataStartAddress = PageStartAddress + this->headerSize;

        
        // For each start of the data...
        for (; static_cast<unsigned>(abs(static_cast<int>(DataStartAddress - PageStartAddress))) < this->stats.PageSize_;
         DataStartAddress += this->dataSize)
        {
            // We intepret it as a pointer.
            GenericObject* dataAddress = reinterpret_cast<GenericObject*>(DataStartAddress);

            // Add the pointer to the free list.
            put_on_freelist(dataAddress);

            if (this->configuration.DebugOn_)
            {
                // Update padding sig
                memset(reinterpret_cast<unsigned char*>(dataAddress) + PTR_SIZE, UNALLOCATED_PATTERN, this->stats.ObjectSize_ - PTR_SIZE);
                memset(toLeftPad(dataAddress), PAD_PATTERN, this->configuration.PadBytes_);
                memset(toRightPad(dataAddress), PAD_PATTERN, this->configuration.PadBytes_);
            }
            memset(toHeader(dataAddress), 0, configuration.HBlockInfo_.size_);
        }
    }
}

/******************************************************************************/
/*!

  Allocates a page according to the configuration given. No checks are done in
  regards to pages.

  \param pageSize
    The total size of the page
  \exceptions OAException
    OAException::OA_EXCEPTION::E_NO_MEMORY - Signifies no more memory from OS
  \return A pointer to the newly allocated page of memory from the OS
*/
/******************************************************************************/
GenericObject* ObjectAllocator::allocate_new_page(size_t pageSize)
{
    try
    {
        GenericObject* newObj = reinterpret_cast<GenericObject*>(new unsigned char[pageSize]());
        ++this->stats.PagesInUse_;
        return newObj;
    }
    catch (std::bad_alloc& exception)
    {
        throw OAException(OAException::OA_EXCEPTION::E_NO_MEMORY, "OA out of mem!");
    }
}

/******************************************************************************/
/*!

  Given an object, places it at the front of the freelist.

  \param Object
    Object to place on Free List

*/
/******************************************************************************/
void ObjectAllocator::put_on_freelist(GenericObject* Object)
{
    GenericObject* temp = this->FreeList_;
    this->FreeList_ = Object;
    Object->Next = temp;

    this->stats.FreeObjects_++;
}

/******************************************************************************/
/*!

  Given a page, removes all the data blocks in that page from the free list.

  \param pageAddr
    Pointer to the page that the user wants to remove all blocks from free list
    from.

*/
/******************************************************************************/
void ObjectAllocator::removePageObjs_from_freelist(GenericObject* pageAddr)
{
    // Store head node 
    GenericObject* temp = this->FreeList_, * prev = nullptr;

    // If head node itself holds the key or multiple occurrences of key 
    while (temp != nullptr && isInPage(pageAddr, reinterpret_cast<unsigned char*>(temp)))
    {
        this->FreeList_ = temp->Next;   // Changed head 
        temp = this->FreeList_;         // Change Temp 
        this->stats.FreeObjects_--;
    }

    // Delete occurrences other than head 
    while (temp != NULL)
    {
        // Search for the key to be deleted, keep track of the 
        // previous node as we need to change 'prev->next' 
        while (temp != nullptr && !isInPage(pageAddr, reinterpret_cast<unsigned char*>(temp)))
        {
            prev = temp;
            temp = temp->Next;
        }

        // If key was not present in linked list 
        if (temp == nullptr) return;

        // Unlink the node from linked list 
        prev->Next = temp->Next;

        this->stats.FreeObjects_--;
        //Update Temp for next iteration of outer loop 
        temp = prev->Next;
    }
}

/******************************************************************************/
/*!

  Increment the stats when an Object is allocted.

*/
/******************************************************************************/
void ObjectAllocator::incrementStats()
{
    // Update stats
    ++this->stats.ObjectsInUse_;
    if (this->stats.ObjectsInUse_ > this->stats.MostObjects_)
        this->stats.MostObjects_ = this->stats.ObjectsInUse_;
    --this->stats.FreeObjects_;
    ++this->stats.Allocations_;
}

/******************************************************************************/
/*!

  Given an object and the current header type configuration, frees the header.
  If debug is set to true, this function will check for multiple frees


  \param Object
    Object to place on Free List
  \param headerType
    Type of header.
  \param ignoreThrow
    If true, this function will not throw.
  \exception OAException
    OAException::E_MULTIPLE_FREE -- If given block is already freed.

*/
/******************************************************************************/
void ObjectAllocator::freeHeader(GenericObject* Object, OAConfig::HBLOCK_TYPE headerType,
  bool ignoreThrow)
{
    unsigned char* headerAddr = toHeader(Object);
    switch (headerType)
    {
    case OAConfig::hbNone:
    {
        // We check if it has been freed by checking the last byte of the object and comparing
        // to 0xCC
        if (this->configuration.DebugOn_ && !ignoreThrow)
        {
            unsigned char* lastChar = reinterpret_cast<unsigned char*>(Object) + stats.ObjectSize_ - 1;
            if (*lastChar == ObjectAllocator::FREED_PATTERN)
                throw OAException(OAException::E_MULTIPLE_FREE, "Multiple free!");
        }
        break;
    }
    case OAConfig::hbBasic:
    {
        // Check if the bit is already free
        if (this->configuration.DebugOn_ && !ignoreThrow)
        {
            if (0 == *(headerAddr + sizeof(unsigned)))
                throw OAException(OAException::E_MULTIPLE_FREE, "Multiple free!");
        }
        // Reset the basic header
        memset(headerAddr, 0, OAConfig::BASIC_HEADER_SIZE);
        break;
    }
    case OAConfig::hbExtended:
    {
        if (this->configuration.DebugOn_ && !ignoreThrow)
        {
            if (0 == *(headerAddr + sizeof(unsigned) + this->configuration.HBlockInfo_.additional_
                + sizeof(unsigned short)))
                throw OAException(OAException::E_MULTIPLE_FREE, "Multiple free!");
        }
        // Reset the basic header part of the extended to 0
        memset(headerAddr + this->configuration.HBlockInfo_.additional_ + sizeof(unsigned short), 0, OAConfig::BASIC_HEADER_SIZE);
        break;
    }
    case OAConfig::hbExternal:
    {
        // Free the external values
            
        MemBlockInfo** info = reinterpret_cast<MemBlockInfo**>(headerAddr);
        if(nullptr == *info	&& this->configuration.DebugOn_ && !ignoreThrow)
            throw OAException(OAException::E_MULTIPLE_FREE, "Multiple free!");
        delete *info;
        *info = nullptr;
    }
    default:
        break;
    }
}

/******************************************************************************/
/*!

  Given a pointer to a data block, builds the basic header for that data block.
  A basic header block consists of 5 bytes. 4 for allocation number, and one flag
  to determine if the block is allocated or not.
  \param addr
    Pointer to data block to build header for
*/
/******************************************************************************/
void ObjectAllocator::buildBasicHeader(GenericObject* addr)
{
    unsigned char* headerAddr = toHeader(addr);
    unsigned* allocationNumber = reinterpret_cast<unsigned*>(headerAddr);
    *allocationNumber = this->stats.Allocations_;
    // Now set the allocation flag
    unsigned char* flag = reinterpret_cast<unsigned char*>(INCREMENT_PTR(allocationNumber));
    *flag = true;
}

/******************************************************************************/
/*!

  Given a pointer to a data block, builds the external header for that data block.
  The external header is a MemBlockInfo

  \param Object
    Pointer to data block to build header for
  \param label
    The label for the external header to hold
  \exceptions OAException
    OAException::E_NO_MEMORY -- Thrown if OS is out of memory.
*/
/******************************************************************************/
void ObjectAllocator::buildExternalHeader(GenericObject* Object, const char* label)
{
    unsigned char* headerAddr = toHeader(Object);
    MemBlockInfo** memPtr = reinterpret_cast<MemBlockInfo**>(headerAddr);
    try
    {
        *memPtr = new MemBlockInfo(stats.Allocations_, label);
    }
    catch (std::bad_alloc&)
    {
        throw OAException(OAException::E_NO_MEMORY, "No memory");
    }
}

/******************************************************************************/
/*!

  Given a pointer to a data block, builds the extended header for that data block.
  The extended header adds on to the basic header two more things, a counter and
  a user specified field.

  \param addr
    Pointer to data block to build header for
*/
/******************************************************************************/
void ObjectAllocator::buildExtendedHeader(GenericObject* Object)
{
    unsigned char* headerAddr = toHeader(Object);
    // Set the 2 byte use-counter, 5 for 5 bytes of user defined stuff.
    unsigned short* counter = reinterpret_cast<unsigned short*>(headerAddr + this->configuration.HBlockInfo_.additional_);
    ++(*counter);
    
    unsigned* allocationNumber = reinterpret_cast<unsigned*>(INCREMENT_PTR(counter));
    *allocationNumber = this->stats.Allocations_;
    // Now set the allocation flag
    unsigned char* flag = reinterpret_cast<unsigned char*>(INCREMENT_PTR(allocationNumber));
    *flag = true;
}

/******************************************************************************/
/*!

  A slower but more throrough check to see if the given object is in a bad
  boundary. Used to check address from Free. If the object is in a bad boundary,
  an exception is thrown.

  \param addr
    The address to check if it is 1) within the page, and 2) a correct address.
  \exception OAException
    OAException::E_BAD_BOUNDARY -- Object is in a bad bounday.
*/
/******************************************************************************/
void ObjectAllocator::check_boundary_full(unsigned char* addr) const
{
        // Find the page the object rests in.
        GenericObject* pageList = this->PageList_;
        // While loop stops when addr resides within the page.
        while(!isInPage(pageList, addr))
        {
            pageList = pageList->Next;
            // If its not in our pages, its not our memory.
            if(!pageList)
            {
                throw OAException(OAException::E_BAD_BOUNDARY, "Bad boundary.");
            }
        }
        // We have found that is is in our pages. Check the boundary using %
        unsigned char* pageStart = reinterpret_cast<unsigned char*>(pageList);
        
        // Check if we are intruding on header.
        if(static_cast<unsigned>(addr - pageStart) < this->headerSize)
            throw OAException(OAException::E_BAD_BOUNDARY, "Bad boundary.");
        
        pageStart += this->headerSize;
        long distance = addr - pageStart;
        if(static_cast<size_t>(distance) % this->dataSize != 0)
            throw OAException(OAException::E_BAD_BOUNDARY, "Bad boundary.");

}

/******************************************************************************/
/*!

  A thorough check to see if the padding surrounding a block is correct.

  \param paddingAddr
    The address to the start of a padding.
  \size
    The size of the padding.
  \return
    True if padding is not corrupted, false if not.
*/
/******************************************************************************/
bool ObjectAllocator::isPaddingCorrect(unsigned char* paddingAddr, size_t size) const
{
    for(size_t i = 0; i < size; ++i)
    {
        if (*(paddingAddr + i) != ObjectAllocator::PAD_PATTERN)
            return false;
    }
    return true;
}

/******************************************************************************/
/*!

  Given a data block and a pattern, checks if the pattern exists on the data 
  block.

  \param objectData
    Address to the data block to check.
  \param pattern
    Pattern to check the data block for.
  \return true if the pattern is in the data, false if not.
*/
/******************************************************************************/
bool ObjectAllocator::checkData(GenericObject* objectdata, const unsigned char pattern) const
{
    unsigned char* data = reinterpret_cast<unsigned char*>(objectdata);
    for(size_t i = 0; i < stats.ObjectSize_; ++i)
    {
        if (data[i] == pattern)
            return true;
    }
    return false;
}

/******************************************************************************/
/*!

  Given a page address and any address, checks if the address is in the page.

  \param pageAddr - Pointer to page 
  \param addr - Address to check if it is within page.
  \return true if in page, false if not.
*/
/******************************************************************************/
bool ObjectAllocator::isInPage(GenericObject* pageAddr, unsigned char* addr) const
{
    return (addr >= reinterpret_cast<unsigned char*>(pageAddr) &&
        addr < reinterpret_cast<unsigned char*>(pageAddr) + stats.PageSize_);
}

/******************************************************************************/
/*!

  Given a page, checks if the page is empty by walking through the freelist
  and checking if there are $(ObjectsPerPage_) free items in a page.

  \param page
    Page to check if it is empty
  \return
    True if page is empty, else false
*/
/******************************************************************************/
bool ObjectAllocator::isPageEmpty(GenericObject* page) const
{
    // Walk though the linked list.
    GenericObject* freeList = this->FreeList_;
    unsigned freeInPage = 0;
    while (freeList)
    {
        if (isInPage(page, reinterpret_cast<unsigned char*>(freeList)))
        {
            if (++freeInPage >= configuration.ObjectsPerPage_)
                return true;
        }
        freeList = freeList->Next;
    }
    return false;
}

/******************************************************************************/
/*!

  Given an object, checks if it is allocated. If configuration has a header, it
  would do a header check. Else it would check if the data is in freelist.

  \param object
    The object to check if it is allocated
  \return
    True if allocated, false if not.
*/
/******************************************************************************/
bool ObjectAllocator::isObjectAllocated(GenericObject* object) const
{
    switch (this->configuration.HBlockInfo_.type_)
    {
    case OAConfig::HBLOCK_TYPE::hbNone:
    {
       // Checks if it is in free list.
      GenericObject* freelist = this->FreeList_;
      while (freelist != nullptr)
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
        // Checks the flag bit.
        unsigned char* flagByte = reinterpret_cast<unsigned char*>(object) - configuration.PadBytes_ - 1;
        return *flagByte;
    }
    case OAConfig::HBLOCK_TYPE::hbExternal:
    {
        // If we are a pointer, we are allocated. IF not, we aren't.
        unsigned char* header = toHeader(object);
        return *header;
    }
    default:
        return false;
        break;
    }
}

/******************************************************************************/
/*!

  Given a pointer to an object and its header type, creates the headers for 
  that object data.

  \param object
    The pointer to the object to create a header for.
  \param label
    Only used for external labels. Labels the header.
  \param headerType
    The type of header to create

*/
/******************************************************************************/
void ObjectAllocator::updateHeader(GenericObject* Object, OAConfig::HBLOCK_TYPE headerType, const char* label)
{
    switch (headerType)
    {
    case OAConfig::hbBasic:
    {
        buildBasicHeader(Object);
        break;
    }
    case OAConfig::hbExtended:
    {
        // one unsigned for allocation number, and one flag to determine on or off.
        buildExtendedHeader(Object);
        break;
    }
    case OAConfig::hbExternal:
        {
        buildExternalHeader(Object, label);
        }
    default:
        break;
    }
}

/******************************************************************************/
/*!

  Given an pointer to a data block, returns the header.

  \param obj
    Pointer to data block to return header for.
  \return
    Pointer to the header for the obj passed in.
*/
/******************************************************************************/
 unsigned char* ObjectAllocator::toHeader(GenericObject* obj) const
{
    return reinterpret_cast<unsigned char*>(obj) - this->configuration.PadBytes_ - this->configuration.HBlockInfo_.size_;
}

 /******************************************************************************/
/*!

  Given an pointer to a data block, returns the left padding address.

  \param obj
    Pointer to data block to return left padding
  \return
    Pointer to the header for the obj passed in.
*/
/******************************************************************************/
unsigned char* ObjectAllocator::toLeftPad(GenericObject* obj) const
{
    return reinterpret_cast<unsigned char*>(obj) - this->configuration.PadBytes_;
}

/******************************************************************************/
/*!

  Given an pointer to a data block, returns right padding

  \param obj
    Pointer to data block to return right padding
  \return
    Pointer to the header for the obj passed in.
*/
/******************************************************************************/
unsigned char* ObjectAllocator::toRightPad(GenericObject* obj) const
{
    return reinterpret_cast<unsigned char*>(obj) + this->stats.ObjectSize_;
}

/******************************************************************************/
/*!

  Given a head node and a node, inserts the node before the head node in the
  linked list.

  \param head
    Head of the linked list to insert the new node in.
  \param node
    Node to insert before the head of the linkedl list.
    
*/
/******************************************************************************/
void ObjectAllocator::InsertHead(GenericObject*& head, GenericObject* node)
{
    node->Next = head;
    head = node;
}

/******************************************************************************/
/*!

  Contructor for the configuration file.

  \param UseCPPMemManager
    Head of the linked list to insert the new node in.
  \param ObjectsPerPage
    Node to insert before the head of the linkedl list.
  \param MaxPages
    The maximum number of pages the OA is allowed to allocate
  \param DebugOn
    Turn on debug settings for the OA
  \param PadBytes
    The number of bytes used for padding
  \param HBInfo
    The information about the header to use
  \param Alignment
    The number of bytes to align to.

*/
/******************************************************************************/
OAConfig::OAConfig(bool UseCPPMemManager, unsigned int ObjectsPerPage, 
                   unsigned int MaxPages, bool DebugOn,
                   unsigned int PadBytes, const OAConfig::HeaderBlockInfo &HBInfo, 
                   unsigned int Alignment)
        : UseCPPMemManager_(UseCPPMemManager),
          ObjectsPerPage_(ObjectsPerPage),
          MaxPages_(MaxPages),
          DebugOn_(DebugOn),
          PadBytes_(PadBytes),
          HBlockInfo_(HBInfo),
          Alignment_(Alignment)
{
    HBlockInfo_ = HBInfo;


    // We need to calc what is left align and the interblock alignment
    unsigned leftHeaderSize = static_cast<unsigned>(PTR_SIZE + HBInfo.size_ + static_cast<size_t>(this->PadBytes_));
    LeftAlignSize_ = static_cast<unsigned>(align(leftHeaderSize, this->Alignment_) - leftHeaderSize);

    InterAlignSize_ = 0;
}
