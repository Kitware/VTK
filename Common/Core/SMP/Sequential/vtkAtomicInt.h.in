 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAtomicInt - Provides support for atomic integers
// .SECTION Description
// Objects of atomic types are C++ objects that are free from data races;
// that is, if one thread writes to an atomic object while another thread
// reads from it, the behavior is well-defined. vtkAtomicInt provides
// a subset of the std::atomic API and implementation, mainly for 32 bit
// and 64 bit signed integers. For these types, vtkAtomicInt defines a
// number of operations that happen atomically - without interruption
// by another thread. Furthermore, these operations happen in a
// sequentially-consistent way and use full memory fences. This means
// that operations relating to atomic variables happen in the specified
// order and the results are made visible to other processing cores to
// guarantee proper sequential operation. Other memory access patterns
// supported by std::atomic are not currently supported.
//
// Note that when atomic operations are not available on a particular
// platform or compiler, mutexes, which are significantly slower, are used
// as a fallback.

#ifndef vtkAtomicInt_h
#define vtkAtomicInt_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"
#include "vtkConfigure.h"

#if defined(__APPLE__)
# include <libkern/OSAtomic.h>
#endif

#if (defined(_WIN32)  && !defined(__MINGW32__))
# define VTK_WINDOWS_ATOMIC
# if defined(_MSC_VER)
#  pragma warning(disable:4324) // disable warning about the padding due to alignment
# endif
#endif

#if defined(VTK_WINDOWS_ATOMIC) || defined(__APPLE__) || defined(VTK_HAVE_SYNC_BUILTINS)
# define VTK_HAS_ATOMIC32
#endif

// Overall, we assume that 64 bit atomic operations are not available on 32
// bit systems.
#if VTK_SIZEOF_VOID_P == 8
# if defined(VTK_WINDOWS_ATOMIC) || defined(__APPLE__) || defined(VTK_HAVE_SYNC_BUILTINS)
#  define VTK_HAS_ATOMIC64
# endif
#endif

#if !defined(VTK_HAS_ATOMIC64) || !defined(VTK_HAS_ATOMIC32)
class vtkSimpleCriticalSection;
#endif

// Below are the actual implementations of 32 and 64 bit atomic operations.
namespace detail
{
#if defined (VTK_WINDOWS_ATOMIC)
# define VTK__ALIGN32 __declspec(align(32))
#else
# define VTK__ALIGN32
#endif

template <typename T> class vtkAtomicIntImpl;

template <>
#if defined(VTK_HAS_ATOMIC32) && !defined(VTK_WINDOWS_ATOMIC)
class vtkAtomicIntImpl<vtkTypeInt32>
#else
class VTKCOMMONCORE_EXPORT vtkAtomicIntImpl<vtkTypeInt32>
#endif
{
public:

  // Description:
  // Atomic pre-increment.
#if defined(VTK_HAS_ATOMIC32) && !defined(VTK_WINDOWS_ATOMIC)
  vtkTypeInt32 operator++()
    {
# if defined(__APPLE__)
    return OSAtomicIncrement32Barrier(&this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    return __sync_add_and_fetch(&this->Value, 1);

# endif
    }

  // Description:
  // Atomic pre-decrement.
  vtkTypeInt32 operator--()
    {
# if defined(__APPLE__)
    return OSAtomicDecrement32Barrier(&this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    return __sync_sub_and_fetch(&this->Value, 1);

# endif
    }

  // Description:
  // Atomic add. Returns value after addition.
  vtkTypeInt32 operator+=(vtkTypeInt32 val)
    {
# if defined(__APPLE__)
    return OSAtomicAdd32Barrier(val, &this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    return __sync_add_and_fetch(&this->Value, val);

# endif
    }

  // Description:
  // Atomic fetch.
  vtkTypeInt32 load() const
    {
# if defined(__APPLE__)
    vtkTypeInt32 retval = 0;
    OSAtomicCompareAndSwap32Barrier(retval, this->Value, &retval);
    return retval;

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    vtkTypeInt32 retval = 0;
    __sync_val_compare_and_swap(&retval, retval, this->Value);
    return retval;

# endif
    }

  // Description:
  // Atomic save.
  void store(vtkTypeInt32 val)
    {
# if defined(__APPLE__)
    OSAtomicCompareAndSwap32Barrier(this->Value, val, &this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  __sync_val_compare_and_swap(&this->Value, this->Value, val);

#endif
    }

#else // defined(VTK_HAS_ATOMIC32) && !defined(VTK_WINDOWS_ATOMIC)

  // These methods are for when using a mutex. Same as above.
  // A virtual descructor is used becase the mutex is constructed
  // with new to avoid including windows header in the .h file.
  vtkAtomicIntImpl<vtkTypeInt32>();
  virtual ~vtkAtomicIntImpl<vtkTypeInt32>();
  vtkTypeInt32 operator++();
  vtkTypeInt32 operator--();
  vtkTypeInt32 operator+=(vtkTypeInt32 val);
  vtkTypeInt32 load() const;
  void store(vtkTypeInt32 val);

#endif // defined(VTK_HAS_ATOMIC32) && !defined(VTK_WINDOWS_ATOMIC)

protected:
  // Explicitely aligning Value on Windows is probably not necessary
  // since the compiler should automatically do it. Just being extra
  // cautious since the InterlockedXXX() functions require alignment.
  VTK__ALIGN32 vtkTypeInt32 Value;

#if !defined(VTK_HAS_ATOMIC32)
  vtkSimpleCriticalSection* AtomicInt32CritSec;
#endif
};

#if defined (VTK_WINDOWS_ATOMIC)
# define VTK__ALIGN64 __declspec(align(64))
#else
# define VTK__ALIGN64
#endif

template <>
#if defined(VTK_HAS_ATOMIC64) && !defined(VTK_WINDOWS_ATOMIC)
class vtkAtomicIntImpl<vtkTypeInt64>
#else
class VTKCOMMONCORE_EXPORT vtkAtomicIntImpl<vtkTypeInt64>
#endif
{
public:

  // Description:
  // Atomic pre-increment.
#if defined(VTK_HAS_ATOMIC64) && !defined(VTK_WINDOWS_ATOMIC)
  vtkTypeInt64 operator++()
    {
# if defined(__APPLE__)
    return OSAtomicIncrement64Barrier(&this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    return __sync_add_and_fetch(&this->Value, 1);

# endif
   }

  // Description:
  // Atomic pre-decrement.
  vtkTypeInt64 operator--()
    {
# if defined(__APPLE__)
    return OSAtomicDecrement64Barrier(&this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    return __sync_sub_and_fetch(&this->Value, 1);

# endif
    }

  // Description:
  // Atomic add. Returns value after addition.
  vtkTypeInt64 operator+=(vtkTypeInt64 val)
    {
# if defined(__APPLE__)
    return OSAtomicAdd64Barrier(val, &this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
    return __sync_add_and_fetch(&this->Value, val);

# endif
    }

  // Description:
  // Atomic fetch.
  vtkTypeInt64 load() const
    {
# if defined(__APPLE__)
  vtkTypeInt64 retval = 0;
  OSAtomicCompareAndSwap64Barrier(retval, this->Value, &retval);
  return retval;

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  vtkTypeInt64 retval = 0;
  __sync_val_compare_and_swap(&retval, retval, this->Value);
  return retval;

# endif
    }

  // Description:
  // Atomic store.
  void store(vtkTypeInt64 val)
    {
# if defined(__APPLE__)
  OSAtomicCompareAndSwap64Barrier(this->Value, val, &this->Value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  __sync_val_compare_and_swap(&this->Value, this->Value, val);

# endif
    }

#else // defined(VTK_HAS_ATOMIC64) && !defined(VTK_WINDOWS_ATOMIC)

  // These methods are for when using a mutex. Same as above.
  // A virtual descructor is used becase the mutex is constructed
  // with new to avoid including windows header in the .h file.
  vtkAtomicIntImpl<vtkTypeInt64>();
  virtual ~vtkAtomicIntImpl<vtkTypeInt64>();
  vtkTypeInt64 operator++();
  vtkTypeInt64 operator--();
  vtkTypeInt64 operator+=(vtkTypeInt64 val);
  vtkTypeInt64 load() const;
  void store(vtkTypeInt64 val);

#endif // defined(VTK_HAS_ATOMIC64) && !defined(VTK_WINDOWS_ATOMIC)

protected:
 // Explicitely aligning Value on Windows is probably not necessary
  // since the compiler should automatically do it. Just being extra
  // cautious since the InterlockedXXX() functions require alignment.
  VTK__ALIGN64 vtkTypeInt64 Value;

#if !defined(VTK_HAS_ATOMIC64)
  vtkSimpleCriticalSection* AtomicInt64CritSec;
#endif
};
}

template <typename T>
class vtkAtomicInt: public detail::vtkAtomicIntImpl<T>
{
  typedef detail::vtkAtomicIntImpl<T> Superclass;

public:
  // Description:
  // Default constructor. Not atomic.
  vtkAtomicInt()
    {
    this->Value = 0;
    }

  // Description:
  // Constructor with initialization. Not atomic.
  vtkAtomicInt(T val)
    {
    this->Value = val;
    }

  // Description:
  // Atomic pre-increment.
  T operator++()
    {
    return this->Superclass::operator++();
    }

  // Description:
  // Atomic post-increment.
  T operator++(int)
    {
    return this->operator++() - 1;
    }

  // Description:
  // Atomic pre-decrement.
  T operator--()
    {
    return this->Superclass::operator--();
    }

  // Description:
  // Atomic post-decrement.
  T operator--(int)
    {
    return this->operator--() + 1;
    }

  // Description:
  // Atomic subtraction. Returns value after.
  T operator-=(T val)
    {
    return this->operator+=(-val);
    }

  // Description:
  // Atomic load.
  operator T() const
    {
    return this->load();
    }

  // Description:
  // Atomic save.
  T operator=(T val)
    {
    this->store(val);
    return val;
    }
};


#endif
// VTK-HeaderTest-Exclude: vtkAtomicInt.h
