// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//-----------------GetTuples (id list)------------------------------------------
struct GetTuplesFromListWorker
{
  vtkIdList* Ids;

  GetTuplesFromListWorker(vtkIdList* ids)
    : Ids(ids)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* src, Array2T* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange(src);
    auto dstTuples = vtk::DataArrayTupleRange(dst);

    vtkIdType* srcTupleId = this->Ids->GetPointer(0);
    vtkIdType* srcTupleIdEnd = this->Ids->GetPointer(Ids->GetNumberOfIds());

    auto dstTupleIter = dstTuples.begin();
    while (srcTupleId != srcTupleIdEnd)
    {
      *dstTupleIter++ = srcTuples[*srcTupleId++];
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdList* tupleIds, vtkAbstractArray* aa)
{
  vtkDataArray* da = vtkDataArray::FastDownCast(aa);
  if (!da)
  {
    vtkErrorMacro("Input is not a vtkDataArray, but " << aa->GetClassName());
    return;
  }

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
  {
    vtkErrorMacro("Number of components for input and output do not match.\n"
                  "Source: "
      << this->GetNumberOfComponents()
      << "\n"
         "Destination: "
      << da->GetNumberOfComponents());
    return;
  }

  GetTuplesFromListWorker worker(tupleIds);
  if (!vtkArrayDispatch::Dispatch2::Execute(this, da, worker))
  {
    // Use fallback if dispatch fails.
    worker(this, da);
  }
}
VTK_ABI_NAMESPACE_END
