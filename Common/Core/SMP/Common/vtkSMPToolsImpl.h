// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMPToolsImpl_h
#define vtkSMPToolsImpl_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSMP.h"

#include <atomic>

#define VTK_SMP_MAX_BACKENDS_NB 4

#define VTK_SMP_BACKEND_SEQUENTIAL 0
#define VTK_SMP_BACKEND_STDTHREAD 1
#define VTK_SMP_BACKEND_TBB 2
#define VTK_SMP_BACKEND_OPENMP 3

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN
enum class BackendType
{
  Sequential = VTK_SMP_BACKEND_SEQUENTIAL,
  STDThread = VTK_SMP_BACKEND_STDTHREAD,
  TBB = VTK_SMP_BACKEND_TBB,
  OpenMP = VTK_SMP_BACKEND_OPENMP
};

#if VTK_SMP_DEFAULT_IMPLEMENTATION_SEQUENTIAL
const BackendType DefaultBackend = BackendType::Sequential;
#elif VTK_SMP_DEFAULT_IMPLEMENTATION_STDTHREAD
const BackendType DefaultBackend = BackendType::STDThread;
#elif VTK_SMP_DEFAULT_IMPLEMENTATION_TBB
const BackendType DefaultBackend = BackendType::TBB;
#elif VTK_SMP_DEFAULT_IMPLEMENTATION_OPENMP
const BackendType DefaultBackend = BackendType::OpenMP;
#endif

template <BackendType Backend>
class vtkSMPToolsImpl
{
public:
  //--------------------------------------------------------------------------------
  void Initialize(int numThreads = 0);

  //--------------------------------------------------------------------------------
  int GetEstimatedNumberOfThreads();

  //--------------------------------------------------------------------------------
  int GetEstimatedDefaultNumberOfThreads();

  //--------------------------------------------------------------------------------
  void SetNestedParallelism(bool isNested);

  //--------------------------------------------------------------------------------
  bool GetNestedParallelism();

  //--------------------------------------------------------------------------------
  bool IsParallelScope();

  //--------------------------------------------------------------------------------
  bool GetSingleThread();

  //--------------------------------------------------------------------------------
  template <typename FunctorInternal>
  void For(vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi);

  //--------------------------------------------------------------------------------
  template <typename InputIt, typename OutputIt, typename Functor>
  void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform);

  //--------------------------------------------------------------------------------
  template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
  void Transform(
    InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform);

  //--------------------------------------------------------------------------------
  template <typename Iterator, typename T>
  void Fill(Iterator begin, Iterator end, const T& value);

  //--------------------------------------------------------------------------------
  template <typename RandomAccessIterator>
  void Sort(RandomAccessIterator begin, RandomAccessIterator end);

  //--------------------------------------------------------------------------------
  template <typename RandomAccessIterator, typename Compare>
  void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp);

  //--------------------------------------------------------------------------------
  vtkSMPToolsImpl();

  //--------------------------------------------------------------------------------
  vtkSMPToolsImpl(const vtkSMPToolsImpl& other);

  //--------------------------------------------------------------------------------
  void operator=(const vtkSMPToolsImpl& other);

private:
  bool NestedActivated = false;
  std::atomic<bool> IsParallel{ false };
};

template <BackendType Backend>
void vtkSMPToolsImpl<Backend>::SetNestedParallelism(bool isNested)
{
  this->NestedActivated = isNested;
}

template <BackendType Backend>
bool vtkSMPToolsImpl<Backend>::GetNestedParallelism()
{
  return this->NestedActivated;
}

template <BackendType Backend>
bool vtkSMPToolsImpl<Backend>::IsParallelScope()
{
  return this->IsParallel;
}

template <BackendType Backend>
vtkSMPToolsImpl<Backend>::vtkSMPToolsImpl()
  : NestedActivated(true)
  , IsParallel(false)
{
}

template <BackendType Backend>
vtkSMPToolsImpl<Backend>::vtkSMPToolsImpl(const vtkSMPToolsImpl& other)
  : NestedActivated(other.NestedActivated)
  , IsParallel(other.IsParallel.load())
{
}

template <BackendType Backend>
void vtkSMPToolsImpl<Backend>::operator=(const vtkSMPToolsImpl& other)
{
  this->NestedActivated = other.NestedActivated;
  this->IsParallel = other.IsParallel.load();
}

using ExecuteFunctorPtrType = void (*)(void*, vtkIdType, vtkIdType, vtkIdType);

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: vtkSMPToolsImpl.h */
