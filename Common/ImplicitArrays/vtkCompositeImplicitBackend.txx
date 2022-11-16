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

//-----------------------------------------------------------------------
namespace
{

template <typename ValueType>
struct GetValueDispatch
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, int& idx, ValueType& val) const
  {
    val = static_cast<ValueType>(arr->GetValue(idx));
  }
};

}

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::vtkCompositeImplicitBackend(
  vtkDataArray* leftArr, vtkDataArray* rightArr)
  : Left(leftArr)
  , Right(rightArr)
{
  this->Offset = this->Left->GetDataSize();
}

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkCompositeImplicitBackend<ValueType>::operator()(int idx) const
{
  int nxtIdx = idx;
  vtkDataArray* branch;
  if (idx < this->Offset)
  {
    branch = this->Left;
  }
  else
  {
    nxtIdx -= this->Offset;
    branch = this->Right;
  }

  using DisArrays = vtkArrayDispatch::AllArrays;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<DisArrays>;

  ValueType val;
  ::GetValueDispatch<ValueType> worklet;
  if (!Dispatcher::Execute(branch, worklet, nxtIdx, val))
  {
    int iTup = nxtIdx / branch->GetNumberOfComponents();
    int iComp = iTup * branch->GetNumberOfComponents() - nxtIdx;
    val = static_cast<ValueType>(branch->GetComponent(iTup, iComp));
  }
  return val;
}
VTK_ABI_NAMESPACE_END
