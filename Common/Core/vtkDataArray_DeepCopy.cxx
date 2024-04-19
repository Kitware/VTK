// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkGenericDataArray.h"
#include "vtkLookupTable.h"
#include "vtkSMPTools.h"

namespace
{

template <typename ValueType>
struct threadedCopyFunctor
{
  ValueType* src;
  ValueType* dst;
  int nComp;
  void operator()(vtkIdType begin, vtkIdType end) const
  {
    // std::copy(src+begin, src+end, dst+begin); //slower
    memcpy(dst + begin * nComp, src + begin * nComp, (end - begin) * nComp * sizeof(ValueType));
  }
};

//--------Copy tuples from src to dest------------------------------------------
struct DeepCopyWorker
{
  // AoS --> AoS same-type specialization:
  template <typename ValueType>
  void operator()(
    vtkAOSDataArrayTemplate<ValueType>* src, vtkAOSDataArrayTemplate<ValueType>* dst) const
  {
    vtkIdType len = src->GetNumberOfTuples();
    if (len < 1024 * 1024)
    {
      // With less than a megabyte or so threading is likely to hurt performance. so don't
      std::copy(src->Begin(), src->End(), dst->Begin());
    }
    else
    {
      threadedCopyFunctor<ValueType> worker;
      worker.src = src->GetPointer(0);
      worker.dst = dst->GetPointer(0);
      worker.nComp = src->GetNumberOfComponents();
      // High granularity is likely to hurt performance too, so limit calls. 16 is about maximal.
      int numThreads = std::min(vtkSMPTools::GetEstimatedNumberOfThreads(), 16);
      vtkSMPTools::For(0, len, len / numThreads, worker);
    }
  }

#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wunused-template")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"
#endif
#endif

  // SoA --> SoA same-type specialization:
  template <typename ValueType>
  void operator()(
    vtkSOADataArrayTemplate<ValueType>* src, vtkSOADataArrayTemplate<ValueType>* dst) const
  {
    dst->CopyData(src);
  }

#ifdef VTK_USE_SCALED_SOA_ARRAYS
  // ScaleSoA --> ScaleSoA same-type specialization:
  template <typename ValueType>
  void operator()(
    vtkScaledSOADataArrayTemplate<ValueType>* src, vtkScaledSOADataArrayTemplate<ValueType>* dst)
  {
    vtkIdType numTuples = src->GetNumberOfTuples();
    for (int comp = 0; comp < src->GetNumberOfComponents(); ++comp)
    {
      ValueType* srcBegin = src->GetComponentArrayPointer(comp);
      ValueType* srcEnd = srcBegin + numTuples;
      ValueType* dstBegin = dst->GetComponentArrayPointer(comp);

      std::copy(srcBegin, srcEnd, dstBegin);
    }
    dst->SetScale(src->GetScale());
  }
#endif

// Undo warning suppression.
#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wunused-template")
#pragma clang diagnostic pop
#endif
#endif

  // Generic implementation:
  template <typename SrcArrayT, typename DstArrayT>
  void DoGenericCopy(SrcArrayT* src, DstArrayT* dst) const
  {
    const auto srcRange = vtk::DataArrayValueRange(src);
    auto dstRange = vtk::DataArrayValueRange(dst);

    using DstT = typename decltype(dstRange)::ValueType;
    auto destIter = dstRange.begin();
    // use for loop instead of copy to avoid -Wconversion warnings
    for (auto v = srcRange.cbegin(); v != srcRange.cend(); ++v, ++destIter)
    {
      *destIter = static_cast<DstT>(*v);
    }
  }

  // These overloads are split so that the above specializations will be
  // used properly.
  template <typename Array1DerivedT, typename Array1ValueT, typename Array2DerivedT,
    typename Array2ValueT>
  void operator()(vtkGenericDataArray<Array1DerivedT, Array1ValueT>* src,
    vtkGenericDataArray<Array2DerivedT, Array2ValueT>* dst) const
  {
    this->DoGenericCopy(src, dst);
  }

  void operator()(vtkDataArray* src, vtkDataArray* dst) const { this->DoGenericCopy(src, dst); }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Normally subclasses will do this when the input and output type of the
// DeepCopy are the same. When they are not the same, then we use the
// templated code below.
void vtkDataArray::DeepCopy(vtkDataArray* da)
{
  // Match the behavior of the old AttributeData
  if (da == nullptr)
  {
    return;
  }

  if (this != da)
  {
    this->Superclass::DeepCopy(da); // copy Information object

    vtkIdType numTuples = da->GetNumberOfTuples();
    int numComps = da->NumberOfComponents;

    this->SetNumberOfComponents(numComps);
    this->SetNumberOfTuples(numTuples);

    if (numTuples != 0)
    {
      DeepCopyWorker worker;
      if (!vtkArrayDispatch::Dispatch2::Execute(da, this, worker))
      {
        // If dispatch fails, use fallback:
        worker(da, this);
      }
    }

    this->SetLookupTable(nullptr);
    if (da->LookupTable)
    {
      this->LookupTable = da->LookupTable->NewInstance();
      this->LookupTable->DeepCopy(da->LookupTable);
    }
  }

  this->Squeeze();
}
VTK_ABI_NAMESPACE_END
