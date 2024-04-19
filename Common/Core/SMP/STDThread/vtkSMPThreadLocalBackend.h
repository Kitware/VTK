// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thread Specific Storage is implemented as a Hash Table, with the Thread Id
// as the key and a Pointer to the data as the value. The Hash Table implements
// Open Addressing with Linear Probing. A fixed-size array (HashTableArray) is
// used as the hash table. The size of this array is allocated to be large
// enough to store thread specific data for all the threads with a Load Factor
// of 0.5. In case the number of threads changes dynamically and the current
// array is not able to accommodate more entries, a new array is allocated that
// is twice the size of the current array. To avoid rehashing and blocking the
// threads, a rehash is not performed immediately. Instead, a linked list of
// hash table arrays is maintained with the current array at the root and older
// arrays along the list. All lookups are sequentially performed along the
// linked list. If the root array does not have an entry, it is created for
// faster lookup next time. The ThreadSpecific::GetStorage() function is thread
// safe and only blocks when a new array needs to be allocated, which should be
// rare.
//
// This implementation is the same as the OpenMP equivalent but with std::mutex
// and std::lock_guard instead of omp_lock_t and custom lock guard.

#ifndef STDThreadvtkSMPThreadLocalBackend_h
#define STDThreadvtkSMPThreadLocalBackend_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include <atomic>
#include <cstdint> // For uint_fast32_t
#include <mutex>   // std::mutex, std::lock_guard
#include <thread>

namespace vtk
{
namespace detail
{
namespace smp
{
namespace STDThread
{
VTK_ABI_NAMESPACE_BEGIN

typedef size_t ThreadIdType;
typedef uint_fast32_t HashType;
typedef void* StoragePointerType;

struct Slot
{
  std::atomic<ThreadIdType> ThreadId;
  std::mutex Mutex;
  StoragePointerType Storage;

  Slot();
  ~Slot() = default;

private:
  // not copyable
  Slot(const Slot&);
  void operator=(const Slot&);
};

struct HashTableArray
{
  size_t Size, SizeLg;
  std::atomic<size_t> NumberOfEntries;
  Slot* Slots;
  HashTableArray* Prev;

  explicit HashTableArray(size_t sizeLg);
  ~HashTableArray();

private:
  // disallow copying
  HashTableArray(const HashTableArray&);
  void operator=(const HashTableArray&);
};

class VTKCOMMONCORE_EXPORT ThreadSpecific final
{
public:
  explicit ThreadSpecific(unsigned numThreads);
  ~ThreadSpecific();

  StoragePointerType& GetStorage();
  size_t GetSize() const;

private:
  std::atomic<HashTableArray*> Root;
  std::atomic<size_t> Size;
  std::mutex Mutex;

  friend class ThreadSpecificStorageIterator;
};

class ThreadSpecificStorageIterator
{
public:
  ThreadSpecificStorageIterator()
    : ThreadSpecificStorage(nullptr)
    , CurrentArray(nullptr)
    , CurrentSlot(0)
  {
  }

  void SetThreadSpecificStorage(ThreadSpecific& threadSpecifc)
  {
    this->ThreadSpecificStorage = &threadSpecifc;
  }

  void SetToBegin()
  {
    this->CurrentArray = this->ThreadSpecificStorage->Root;
    this->CurrentSlot = 0;
    if (!this->CurrentArray->Slots->Storage)
    {
      this->Forward();
    }
  }

  void SetToEnd()
  {
    this->CurrentArray = nullptr;
    this->CurrentSlot = 0;
  }

  bool GetInitialized() const { return this->ThreadSpecificStorage != nullptr; }

  bool GetAtEnd() const { return this->CurrentArray == nullptr; }

  void Forward()
  {
    while (true)
    {
      if (++this->CurrentSlot >= this->CurrentArray->Size)
      {
        this->CurrentArray = this->CurrentArray->Prev;
        this->CurrentSlot = 0;
        if (!this->CurrentArray)
        {
          break;
        }
      }
      Slot* slot = this->CurrentArray->Slots + this->CurrentSlot;
      if (slot->Storage)
      {
        break;
      }
    }
  }

  StoragePointerType& GetStorage() const
  {
    Slot* slot = this->CurrentArray->Slots + this->CurrentSlot;
    return slot->Storage;
  }

  bool operator==(const ThreadSpecificStorageIterator& it) const
  {
    return (this->ThreadSpecificStorage == it.ThreadSpecificStorage) &&
      (this->CurrentArray == it.CurrentArray) && (this->CurrentSlot == it.CurrentSlot);
  }

private:
  ThreadSpecific* ThreadSpecificStorage;
  HashTableArray* CurrentArray;
  size_t CurrentSlot;
};

VTK_ABI_NAMESPACE_END
} // STDThread;
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: INCLUDES:CLASSES */
