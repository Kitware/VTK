/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomic.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAtomic - Provides support for atomic integers
// .SECTION Description
// Objects of atomic types are C++ objects that are free from data races;
// that is, if one thread writes to an atomic object while another thread
// reads from it, the behavior is well-defined. vtkAtomic provides
// a subset of the std::atomic API and implementation for the following types,
// 32 bit signed and unsigned integers, 64 bit signed and unsigned integers,
// and pointers. For these types, vtkAtomic defines a
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

#ifndef vtkAtomic_h
#define vtkAtomic_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkAtomicTypeConcepts.h"
#include "vtkConfigure.h"
#include "vtkSystemIncludes.h"
#include "vtkType.h"

#include <cstddef>


// Assume 64-bit atomic operations are not available on 32 bit platforms
#if defined(VTK_HAVE_SYNC_BUILTINS)
# define VTK_GCC_ATOMICS_32
# if VTK_SIZEOF_VOID_P == 8
#   define VTK_GCC_ATOMICS_64
# endif
#elif defined(__APPLE__)
# include <libkern/OSAtomic.h>
# define VTK_APPLE_ATOMICS_32
# if VTK_SIZEOF_VOID_P == 8
#   define VTK_APPLE_ATOMICS_64
# endif
#elif defined(_WIN32) && defined(_MSC_VER)
# define VTK_WINDOWS_ATOMICS_32
# if VTK_SIZEOF_VOID_P == 8
#   define VTK_WINDOWS_ATOMICS_64
# endif
#endif


#if defined(_WIN32) && defined(_MSC_VER)
//  disable warning about the padding due to alignment
# pragma warning(disable:4324)
# define VTK_ALIGN(X) __declspec(align(X))
#elif defined(__GNUC__) // gcc compatible compiler
# define VTK_ALIGN(X) __attribute__ ((aligned (X)))
#else
# define VTK_ALIGN(X)
#endif


class vtkSimpleCriticalSection;


#ifndef __VTK_WRAP__
namespace detail
{

template <size_t size> class AtomicOps;

#if defined(VTK_GCC_ATOMICS_64)
template <> class AtomicOps<8>
{
public:
  typedef vtkTypeInt64 VTK_ALIGN(8) atomic_type;
  typedef vtkTypeInt64 value_type;

  static value_type AddAndFetch(value_type *ref, value_type val)
  {
    return __sync_add_and_fetch(ref, val);
  }

  static value_type SubAndFetch(value_type *ref, value_type val)
  {
    return __sync_sub_and_fetch(ref, val);
  }

  static value_type PreIncrement(value_type *ref)
  {
    return __sync_add_and_fetch(ref, 1);
  }

  static value_type PreDecrement(value_type *ref)
  {
    return __sync_sub_and_fetch(ref, 1);
  }

  static value_type PostIncrement(value_type *ref)
  {
    return __sync_fetch_and_add(ref, 1);
  }

  static value_type PostDecrement(value_type *ref)
  {
    return __sync_fetch_and_sub(ref, 1);
  }

  static value_type Load(const value_type *ref)
  {
    __sync_synchronize();
    return *static_cast<const volatile value_type *>(ref);
  }

  static void Store(value_type *ref, value_type val)
  {
    *static_cast<volatile value_type*>(ref) = val;
    __sync_synchronize();
  }
};

#elif defined(VTK_APPLE_ATOMICS_64)
template <> class AtomicOps<8>
{
public:
  typedef vtkTypeInt64 VTK_ALIGN(8) atomic_type;
  typedef vtkTypeInt64 value_type;

  static vtkTypeInt64 AddAndFetch(vtkTypeInt64 *ref, vtkTypeInt64 val)
  {
    return OSAtomicAdd64Barrier(val, ref);
  }

  static vtkTypeInt64 SubAndFetch(vtkTypeInt64 *ref, vtkTypeInt64 val)
  {
    return OSAtomicAdd64Barrier(-val, ref);
  }

  static vtkTypeInt64 PreIncrement(vtkTypeInt64 *ref)
  {
    return OSAtomicIncrement64Barrier(ref);
  }

  static vtkTypeInt64 PreDecrement(vtkTypeInt64 *ref)
  {
    return OSAtomicDecrement64Barrier(ref);
  }

  static vtkTypeInt64 PostIncrement(vtkTypeInt64 *ref)
  {
    vtkTypeInt64 val = OSAtomicIncrement64Barrier(ref);
    return --val;
  }

  static vtkTypeInt64 PostDecrement(vtkTypeInt64 *ref)
  {
    vtkTypeInt64 val = OSAtomicDecrement64Barrier(ref);
    return ++val;
  }

  static vtkTypeInt64 Load(const vtkTypeInt64 *ref);
  {
    OSMemoryBarrier();
    return *static_cast<const volatile vtkTypeInt64*>(ref);
  }

  static void Store(vtkTypeInt64 *ref, vtkTypeInt64 val);
  {
    *static_cast<volatile vtkTypeInt64*>(ref) = val;
    OSMemoryBarrier();
  }
};

#else

template <> class VTKCOMMONCORE_EXPORT AtomicOps<8>
{
public:
#if defined(VTK_WINDOWS_ATOMICS_64)
  typedef vtkTypeInt64 VTK_ALIGN(8) atomic_type;
#else
  struct VTKCOMMONCORE_EXPORT atomic_type
  {
    vtkTypeInt64 var;
    vtkSimpleCriticalSection *csec;

    atomic_type(vtkTypeInt64 init);
    ~atomic_type();
  };
#endif
  typedef vtkTypeInt64 value_type;

  static vtkTypeInt64 AddAndFetch(atomic_type *ref, vtkTypeInt64 val);
  static vtkTypeInt64 SubAndFetch(atomic_type *ref, vtkTypeInt64 val);
  static vtkTypeInt64 PreIncrement(atomic_type *ref);
  static vtkTypeInt64 PreDecrement(atomic_type *ref);
  static vtkTypeInt64 PostIncrement(atomic_type *ref);
  static vtkTypeInt64 PostDecrement(atomic_type *ref);
  static vtkTypeInt64 Load(const atomic_type *ref);
  static void Store(atomic_type *ref, vtkTypeInt64 val);
};

#endif

#if defined(VTK_GCC_ATOMICS_32)
template <> class AtomicOps<4>
{
public:
  typedef vtkTypeInt32 VTK_ALIGN(4) atomic_type;
  typedef vtkTypeInt32 value_type;

  static value_type AddAndFetch(value_type *ref, value_type val)
  {
    return __sync_add_and_fetch(ref, val);
  }

  static value_type SubAndFetch(value_type *ref, value_type val)
  {
    return __sync_sub_and_fetch(ref, val);
  }

  static value_type PreIncrement(value_type *ref)
  {
    return __sync_add_and_fetch(ref, 1);
  }

  static value_type PreDecrement(value_type *ref)
  {
    return __sync_sub_and_fetch(ref, 1);
  }

  static value_type PostIncrement(value_type *ref)
  {
    return __sync_fetch_and_add(ref, 1);
  }

  static value_type PostDecrement(value_type *ref)
  {
    return __sync_fetch_and_sub(ref, 1);
  }

  static value_type Load(const value_type *ref)
  {
    __sync_synchronize();
    return *static_cast<const volatile value_type *>(ref);
  }

  static void Store(value_type *ref, value_type val)
  {
    *static_cast<volatile value_type*>(ref) = val;
    __sync_synchronize();
  }
};

#elif defined(VTK_APPLE_ATOMICS_32)
template <> class AtomicOps<4>
{
public:
  typedef vtkTypeInt32 VTK_ALIGN(4) atomic_type;
  typedef vtkTypeInt32 value_type;

  static vtkTypeInt32 AddAndFetch(vtkTypeInt32 *ref, vtkTypeInt32 val)
  {
    return OSAtomicAdd32Barrier(val, ref);
  }

  static vtkTypeInt32 SubAndFetch(vtkTypeInt32 *ref, vtkTypeInt32 val)
  {
    return OSAtomicAdd32Barrier(-val, ref);
  }

  static vtkTypeInt32 PreIncrement(vtkTypeInt32 *ref)
  {
    return OSAtomicIncrement32Barrier(ref);
  }

  static vtkTypeInt32 PreDecrement(vtkTypeInt32 *ref)
  {
    return OSAtomicDecrement32Barrier(ref);
  }

  static vtkTypeInt32 PostIncrement(vtkTypeInt32 *ref)
  {
    vtkTypeInt32 val = OSAtomicIncrement32Barrier(ref);
    return --val;
  }

  static vtkTypeInt32 PostDecrement(vtkTypeInt32 *ref)
  {
    vtkTypeInt32 val = OSAtomicDecrement32Barrier(ref);
    return ++val;
  }

  static vtkTypeInt32 Load(const vtkTypeInt32 *ref);
  {
    OSMemoryBarrier();
    return *static_cast<const volatile vtkTypeInt32*>(ref);
  }

  static void Store(vtkTypeInt32 *ref, vtkTypeInt32 val);
  {
    *static_cast<volatile vtkTypeInt32*>(ref) = val;
    OSMemoryBarrier();
  }
};

#else

template <> class VTKCOMMONCORE_EXPORT AtomicOps<4>
{
public:
#if defined(VTK_WINDOWS_ATOMICS_32)
  typedef vtkTypeInt32 VTK_ALIGN(4) atomic_type;
#else
  struct VTKCOMMONCORE_EXPORT atomic_type
  {
    vtkTypeInt32 var;
    vtkSimpleCriticalSection *csec;

    atomic_type(vtkTypeInt32 init);
    ~atomic_type();
  };
#endif
  typedef vtkTypeInt32 value_type;

  static vtkTypeInt32 AddAndFetch(atomic_type *ref, vtkTypeInt32 val);
  static vtkTypeInt32 SubAndFetch(atomic_type *ref, vtkTypeInt32 val);
  static vtkTypeInt32 PreIncrement(atomic_type *ref);
  static vtkTypeInt32 PreDecrement(atomic_type *ref);
  static vtkTypeInt32 PostIncrement(atomic_type *ref);
  static vtkTypeInt32 PostDecrement(atomic_type *ref);
  static vtkTypeInt32 Load(const atomic_type *ref);
  static void Store(atomic_type *ref, vtkTypeInt32 val);
};

#endif
}
#endif // __VTK_WRAP__

template <typename T> class vtkAtomic : private vtk::atomic::detail::IntegralType<T>
{
private:
  typedef detail::AtomicOps<sizeof(T)> Impl;

public:
  vtkAtomic() : Atomic(0)
  {
  }

  vtkAtomic(T val) : Atomic(static_cast<typename Impl::value_type>(val))
  {
  }

  vtkAtomic(const vtkAtomic<T> &atomic)
    : Atomic(static_cast<typename Impl::value_type>(atomic.load()))
  {
  }

  T operator++()
  {
    return static_cast<T>(Impl::PreIncrement(&this->Atomic));
  }

  T operator++(int)
  {
    return static_cast<T>(Impl::PostIncrement(&this->Atomic));
  }

  T operator--()
  {
    return static_cast<T>(Impl::PreDecrement(&this->Atomic));
  }

  T operator--(int)
  {
    return static_cast<T>(Impl::PostDecrement(&this->Atomic));
  }

  T operator+=(T val)
  {
    return static_cast<T>(Impl::AddAndFetch(&this->Atomic,
      static_cast<typename Impl::value_type>(val)));
  }

  T operator-=(T val)
  {
    return static_cast<T>(Impl::SubAndFetch(&this->Atomic,
      static_cast<typename Impl::value_type>(val)));
  }

  operator T() const
  {
    return static_cast<T>(Impl::Load(&this->Atomic));
  }

  T operator=(T val)
  {
    Impl::Store(&this->Atomic, static_cast<typename Impl::value_type>(val));
    return val;
  }

  vtkAtomic<T>& operator=(const vtkAtomic<T> &atomic)
  {
    this->store(atomic.load());
    return *this;
  }

  T load() const
  {
    return static_cast<T>(Impl::Load(&this->Atomic));
  }

  void store(T val)
  {
    Impl::Store(&this->Atomic, static_cast<typename Impl::value_type>(val));
  }

private:
  typename Impl::atomic_type Atomic;
};


template <typename T> class vtkAtomic<T*>
{
private:
  typedef detail::AtomicOps<sizeof(T*)> Impl;

public:
  vtkAtomic() : Atomic(0)
  {
  }

  vtkAtomic(T* val)
    : Atomic(reinterpret_cast<typename Impl::value_type>(val))
  {
  }

  vtkAtomic(const vtkAtomic<T*> &atomic)
    : Atomic(reinterpret_cast<typename Impl::value_type>(atomic.load()))
  {
  }

  T* operator++()
  {
    return reinterpret_cast<T*>(Impl::AddAndFetch(&this->Atomic, sizeof(T)));
  }

  T* operator++(int)
  {
    T* val = reinterpret_cast<T*>(Impl::AddAndFetch(&this->Atomic, sizeof(T)));
    return --val;
  }

  T* operator--()
  {
    return reinterpret_cast<T*>(Impl::SubAndFetch(&this->Atomic, sizeof(T)));
  }

  T* operator--(int)
  {
    T* val = reinterpret_cast<T*>(Impl::AddAndFetch(&this->Atomic, sizeof(T)));
    return ++val;
  }

  T* operator+=(std::ptrdiff_t val)
  {
    return reinterpret_cast<T*>(Impl::AddAndFetch(&this->Atomic,
                                                  val * sizeof(T)));
  }

  T* operator-=(std::ptrdiff_t val)
  {
    return reinterpret_cast<T*>(Impl::SubAndFetch(&this->Atomic,
                                                  val * sizeof(T)));
  }

  operator T*() const
  {
    return reinterpret_cast<T*>(Impl::Load(&this->Atomic));
  }

  T* operator=(T* val)
  {
    Impl::Store(&this->Atomic,
                reinterpret_cast<typename Impl::value_type>(val));
    return val;
  }

  vtkAtomic<T*>& operator=(const vtkAtomic<T*> &atomic)
  {
    this->store(atomic.load());
    return *this;
  }

  T* load() const
  {
    return reinterpret_cast<T*>(Impl::Load(&this->Atomic));
  }

  void store(T* val)
  {
    Impl::Store(&this->Atomic,
                reinterpret_cast<typename Impl::value_type>(val));
  }

private:
  typename Impl::atomic_type Atomic;
};


template <> class vtkAtomic<void*>
{
private:
  typedef detail::AtomicOps<sizeof(void*)> Impl;

public:
  vtkAtomic() : Atomic(0)
  {
  }

  vtkAtomic(void* val)
    : Atomic(reinterpret_cast<Impl::value_type>(val))
  {
  }

  vtkAtomic(const vtkAtomic<void*> &atomic)
    : Atomic(reinterpret_cast<Impl::value_type>(atomic.load()))
  {
  }

  operator void*() const
  {
    return reinterpret_cast<void*>(Impl::Load(&this->Atomic));
  }

  void* operator=(void* val)
  {
    Impl::Store(&this->Atomic,
                reinterpret_cast<Impl::value_type>(val));
    return val;
  }

  vtkAtomic<void*>& operator=(const vtkAtomic<void*> &atomic)
  {
    this->store(atomic.load());
    return *this;
  }

  void* load() const
  {
    return reinterpret_cast<void*>(Impl::Load(&this->Atomic));
  }

  void store(void* val)
  {
    Impl::Store(&this->Atomic,
                reinterpret_cast<Impl::value_type>(val));
  }

private:
  Impl::atomic_type Atomic;
};

#endif
// VTK-HeaderTest-Exclude: vtkAtomic.h
