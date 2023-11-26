// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//------------InterpolateTuple workers------------------------------------------
struct InterpolateMultiTupleWorker
{
  vtkIdType DestTuple;
  vtkIdType* TupleIds;
  vtkIdType NumTuples;
  double* Weights;

  InterpolateMultiTupleWorker(
    vtkIdType destTuple, vtkIdType* tupleIds, vtkIdType numTuples, double* weights)
    : DestTuple(destTuple)
    , TupleIds(tupleIds)
    , NumTuples(numTuples)
    , Weights(weights)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* src, Array2T* dst) const
  {
    // Use vtkDataArrayAccessor here instead of a range, since we need to use
    // Insert for legacy compat
    vtkDataArrayAccessor<Array1T> s(src);
    vtkDataArrayAccessor<Array2T> d(dst);

    typedef typename vtkDataArrayAccessor<Array2T>::APIType DestType;

    int numComp = src->GetNumberOfComponents();

    for (int c = 0; c < numComp; ++c)
    {
      double val = 0.;
      for (vtkIdType tupleId = 0; tupleId < this->NumTuples; ++tupleId)
      {
        vtkIdType t = this->TupleIds[tupleId];
        double weight = this->Weights[tupleId];
        val += weight * static_cast<double>(s.Get(t, c));
      }
      DestType valT;
      vtkMath::RoundDoubleToIntegralIfNecessary(val, &valT);
      d.Insert(this->DestTuple, c, valT);
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Interpolate array value from other array value given the
// indices and associated interpolation weights.
// This method assumes that the two arrays are of the same time.
void vtkDataArray::InterpolateTuple(
  vtkIdType dstTupleIdx, vtkIdList* tupleIds, vtkAbstractArray* source, double* weights)
{
  if (!vtkDataTypesCompare(this->GetDataType(), source->GetDataType()))
  {
    vtkErrorMacro("Cannot interpolate arrays of different type.");
    return;
  }

  vtkDataArray* da = vtkDataArray::FastDownCast(source);
  if (!da)
  {
    vtkErrorMacro(<< "Source array is not a vtkDataArray.");
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (da->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << source->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  vtkIdType numIds = tupleIds->GetNumberOfIds();
  vtkIdType* ids = tupleIds->GetPointer(0);

  bool fallback = da->GetDataType() == VTK_BIT || this->GetDataType() == VTK_BIT;

  if (!fallback)
  {
    InterpolateMultiTupleWorker worker(dstTupleIdx, ids, numIds, weights);
    // Use fallback if dispatch fails.
    fallback = !vtkArrayDispatch::Dispatch2SameValueType::Execute(da, this, worker);
  }

  // Fallback to a separate implementation that checks vtkDataArray::GetDataType
  // rather than relying on API types, since we'll need to round differently
  // depending on type, and the API type for vtkDataArray is always double.
  if (fallback)
  {
    bool doRound = !(this->GetDataType() == VTK_FLOAT || this->GetDataType() == VTK_DOUBLE);
    double typeMin = this->GetDataTypeMin();
    double typeMax = this->GetDataTypeMax();

    for (int c = 0; c < numComps; ++c)
    {
      double val = 0.;
      for (vtkIdType j = 0; j < numIds; ++j)
      {
        val += weights[j] * da->GetComponent(ids[j], c);
      }

      // Clamp to data type range:
      val = std::max(val, typeMin);
      val = std::min(val, typeMax);

      // Round for floating point types:
      if (doRound)
      {
        val = std::floor((val >= 0.) ? (val + 0.5) : (val - 0.5));
      }

      this->InsertComponent(dstTupleIdx, c, val);
    }
  }
}
VTK_ABI_NAMESPACE_END
