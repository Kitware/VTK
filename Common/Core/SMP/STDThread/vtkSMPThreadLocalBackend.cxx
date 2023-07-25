// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "SMP/STDThread/vtkSMPThreadLocalBackend.h"

#include "SMP/STDThread/vtkSMPThreadPool.h"

#include <algorithm>
#include <cmath>      // For std::floor & std::log2
#include <functional> // For std::hash
#include <thread>     // For std::thread

namespace vtk
{
namespace detail
{
namespace smp
{
namespace STDThread
{
VTK_ABI_NAMESPACE_BEGIN

static ThreadIdType GetThreadId()
{
  return vtkSMPThreadPool::GetInstance().GetThreadId();
}

// 32 bit FNV-1a hash function
inline HashType GetHash(ThreadIdType id)
{
  const HashType offset_basis = 2166136261u;
  const HashType FNV_prime = 16777619u;

  unsigned char* bp = reinterpret_cast<unsigned char*>(&id);
  unsigned char* be = bp + sizeof(id);
  HashType hval = offset_basis;
  while (bp < be)
  {
    hval ^= static_cast<HashType>(*bp++);
    hval *= FNV_prime;
  }

  return hval;
}

Slot::Slot()
  : ThreadId(0)
  , Storage(nullptr)
{
}

HashTableArray::HashTableArray(size_t sizeLg)
  : Size(1ULL << sizeLg)
  , SizeLg(sizeLg)
  , NumberOfEntries(0)
  , Prev(nullptr)
{
  this->Slots = new Slot[this->Size];
}

HashTableArray::~HashTableArray()
{
  delete[] this->Slots;
}

// Recursively lookup the slot containing threadId in the HashTableArray
// linked list -- array
static Slot* LookupSlot(HashTableArray* array, ThreadIdType threadId, size_t hash)
{
  if (!array)
  {
    return nullptr;
  }

  size_t mask = array->Size - 1u;
  Slot* slot = nullptr;

  // since load factor is maintained below 0.5, this loop should hit an
  // empty slot if the queried slot does not exist in this array
  for (size_t idx = hash & mask;; idx = (idx + 1) & mask) // linear probing
  {
    slot = array->Slots + idx;
    ThreadIdType slotThreadId = slot->ThreadId.load();
    if (!slotThreadId) // empty slot means threadId doesn't exist in this array
    {
      slot = LookupSlot(array->Prev, threadId, hash);
      break;
    }
    else if (slotThreadId == threadId)
    {
      break;
    }
  }

  return slot;
}

// Lookup threadId. Try to acquire a slot if it doesn't already exist.
// Does not block. Returns nullptr if acquire fails due to high load factor.
// Returns true in 'firstAccess' if threadID did not exist previously.
static Slot* AcquireSlot(
  HashTableArray* array, ThreadIdType threadId, size_t hash, bool& firstAccess)
{
  size_t mask = array->Size - 1u;
  Slot* slot = nullptr;
  firstAccess = false;

  for (size_t idx = hash & mask;; idx = (idx + 1) & mask)
  {
    slot = array->Slots + idx;
    ThreadIdType slotThreadId = slot->ThreadId.load();
    if (!slotThreadId) // unused?
    {
      std::lock_guard<std::mutex> lguard(slot->Mutex);

      size_t size = array->NumberOfEntries++;
      if ((size * 2) > array->Size) // load factor is above threshold
      {
        --array->NumberOfEntries;
        return nullptr; // indicate need for resizing
      }

      if (!slot->ThreadId.load()) // not acquired in the meantime?
      {
        slot->ThreadId.store(threadId);
        // check previous arrays for the entry
        Slot* prevSlot = LookupSlot(array->Prev, threadId, hash);
        if (prevSlot)
        {
          slot->Storage = prevSlot->Storage;
          // Do not clear PrevSlot's ThreadId as our technique of stopping
          // linear probing at empty slots relies on slots not being
          // "freed". Instead, clear previous slot's storage pointer as
          // ThreadSpecificStorageIterator relies on this information to
          // ensure that it doesn't iterate over the same thread's storage
          // more than once.
          prevSlot->Storage = nullptr;
        }
        else // first time access
        {
          slot->Storage = nullptr;
          firstAccess = true;
        }
        break;
      }
    }
    else if (slotThreadId == threadId)
    {
      break;
    }
  }

  return slot;
}

ThreadSpecific::ThreadSpecific(unsigned numThreads)
  : Size(0)
{
  const int lastSetBit = (numThreads != 0 ? std::floor(std::log2(numThreads)) : 0);
  const size_t initSizeLg = (lastSetBit + 2);
  this->Root = new HashTableArray(initSizeLg);
}

ThreadSpecific::~ThreadSpecific()
{
  HashTableArray* array = this->Root;
  while (array)
  {
    HashTableArray* tofree = array;
    array = array->Prev;
    delete tofree;
  }
}

StoragePointerType& ThreadSpecific::GetStorage()
{
  ThreadIdType threadId = GetThreadId();
  size_t hash = GetHash(threadId);

  Slot* slot = nullptr;
  while (!slot)
  {
    bool firstAccess = false;
    HashTableArray* array = this->Root.load();
    slot = AcquireSlot(array, threadId, hash, firstAccess);
    if (!slot) // not enough room, resize
    {
      std::lock_guard<std::mutex> lguard(this->Mutex);

      if (this->Root == array)
      {
        HashTableArray* newArray = new HashTableArray(array->SizeLg + 1);
        newArray->Prev = array;
        this->Root.store(newArray);
      }
    }
    else if (firstAccess)
    {
      this->Size++;
    }
  }
  return slot->Storage;
}

size_t ThreadSpecific::GetSize() const
{
  return this->Size;
}

VTK_ABI_NAMESPACE_END
} // STDThread
} // namespace smp
} // namespace detail
} // namespace vtk
