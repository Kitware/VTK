/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeImplicitBackend.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeImplicitBackend.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchImplicitArrayList.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
struct vtkCompositeImplicitBackend<ValueType>::Internals
{
  Internals(vtkDataArray* leftArr, vtkDataArray* rightArr)
    : Left(leftArr)
    , Right(rightArr)
  {
    if (this->Left == nullptr || this->Right == nullptr)
    {
      vtkWarningWithObjectMacro(nullptr, "Creating composite array with nullptr");
      return;
    }
    this->LeftRange = vtk::DataArrayValueRange(this->Left);
    this->RightRange = vtk::DataArrayValueRange(this->Right);
    this->Offset = LeftRange.size();
  }

  vtkSmartPointer<vtkDataArray> Left;
  vtk::detail::SelectValueRange<vtkDataArray*, vtk::detail::DynamicTupleSize>::type LeftRange;
  vtkSmartPointer<vtkDataArray> Right;
  vtk::detail::SelectValueRange<vtkDataArray*, vtk::detail::DynamicTupleSize>::type RightRange;
  int Offset = -1;
};

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::vtkCompositeImplicitBackend(
  vtkDataArray* leftArr, vtkDataArray* rightArr)
  : Internal(std::unique_ptr<Internals>(new Internals(leftArr, rightArr)))
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::~vtkCompositeImplicitBackend()
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkCompositeImplicitBackend<ValueType>::operator()(int idx) const
{
  return static_cast<ValueType>((idx < this->Internal->Offset)
      ? this->Internal->LeftRange[idx]
      : this->Internal->RightRange[idx - this->Internal->Offset]);
}
VTK_ABI_NAMESPACE_END
