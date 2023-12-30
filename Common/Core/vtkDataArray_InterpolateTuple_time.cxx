// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

struct InterpolateTupleWorker
{
  vtkIdType SrcTuple1;
  vtkIdType SrcTuple2;
  vtkIdType DstTuple;
  double Weight;

  InterpolateTupleWorker(
    vtkIdType srcTuple1, vtkIdType srcTuple2, vtkIdType dstTuple, double weight)
    : SrcTuple1(srcTuple1)
    , SrcTuple2(srcTuple2)
    , DstTuple(dstTuple)
    , Weight(weight)
  {
  }

  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T* src1, Array2T* src2, Array3T* dst) const
  {
    // Use accessor here instead of ranges since we need to use Insert for
    // legacy compat
    vtkDataArrayAccessor<Array1T> s1(src1);
    vtkDataArrayAccessor<Array2T> s2(src2);
    vtkDataArrayAccessor<Array3T> d(dst);

    typedef typename vtkDataArrayAccessor<Array3T>::APIType DestType;

    const int numComps = dst->GetNumberOfComponents();
    const double oneMinusT = 1. - this->Weight;
    double val;
    DestType valT;

    for (int c = 0; c < numComps; ++c)
    {
      val = s1.Get(this->SrcTuple1, c) * oneMinusT + s2.Get(this->SrcTuple2, c) * this->Weight;
      vtkMath::RoundDoubleToIntegralIfNecessary(val, &valT);
      d.Insert(this->DstTuple, c, valT);
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Interpolate value from the two values, p1 and p2, and an
// interpolation factor, t. The interpolation factor ranges from (0,1),
// with t=0 located at p1. This method assumes that the three arrays are of
// the same type. p1 is value at index id1 in fromArray1, while, p2 is
// value at index id2 in fromArray2.
void vtkDataArray::InterpolateTuple(vtkIdType dstTuple, vtkIdType srcTuple1,
  vtkAbstractArray* source1, vtkIdType srcTuple2, vtkAbstractArray* source2, double t)
{
  int type = this->GetDataType();

  if (!vtkDataTypesCompare(type, source1->GetDataType()) ||
    !vtkDataTypesCompare(type, source2->GetDataType()))
  {
    vtkErrorMacro("All arrays to InterpolateValue must be of same type.");
    return;
  }

  if (srcTuple1 >= source1->GetNumberOfTuples())
  {
    vtkErrorMacro("Tuple 1 out of range for provided array. "
                  "Requested tuple: "
      << srcTuple1
      << " "
         "Tuples: "
      << source1->GetNumberOfTuples());
    return;
  }

  if (srcTuple2 >= source2->GetNumberOfTuples())
  {
    vtkErrorMacro("Tuple 2 out of range for provided array. "
                  "Requested tuple: "
      << srcTuple2
      << " "
         "Tuples: "
      << source2->GetNumberOfTuples());
    return;
  }

  vtkDataArray* src1DA = vtkDataArray::FastDownCast(source1);
  vtkDataArray* src2DA = vtkDataArray::FastDownCast(source2);
  if (!src1DA || !src2DA)
  {
    vtkErrorMacro("Both arrays must be vtkDataArray subclasses.");
    return;
  }

  bool fallback = type == VTK_BIT;

  if (!fallback)
  {
    InterpolateTupleWorker worker(srcTuple1, srcTuple2, dstTuple, t);
    // Use fallback if dispatch fails:
    fallback = !vtkArrayDispatch::Dispatch3SameValueType::Execute(src1DA, src2DA, this, worker);
  }

  // Fallback to a separate implementation that checks vtkDataArray::GetDataType
  // rather than relying on API types, since we'll need to round differently
  // depending on type, and the API type for vtkDataArray is always double.
  if (fallback)
  {
    bool doRound = !(this->GetDataType() == VTK_FLOAT || this->GetDataType() == VTK_DOUBLE);
    double typeMin = this->GetDataTypeMin();
    double typeMax = this->GetDataTypeMax();
    int numComp = source1->GetNumberOfComponents();
    double in1;
    double in2;
    double out;
    for (int c = 0; c < numComp; c++)
    {
      in1 = src1DA->GetComponent(srcTuple1, c);
      in2 = src2DA->GetComponent(srcTuple2, c);
      out = in1 + t * (in2 - in1);
      // Clamp to datatype range:
      out = std::max(out, typeMin);
      out = std::min(out, typeMax);
      // Round if needed:
      if (doRound)
      {
        out = std::floor((out >= 0.) ? (out + 0.5) : (out - 0.5));
      }
      this->InsertComponent(dstTuple, c, out);
    }
  }
}
VTK_ABI_NAMESPACE_END
