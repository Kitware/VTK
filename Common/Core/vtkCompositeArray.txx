// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCompositeArray_txx
#define vtkCompositeArray_txx

#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCompositeArray.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkCompositeArray<ValueTypeT>* vtkCompositeArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkCompositeArray<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkCompositeArray<ValueTypeT>::ConstructBackend(vtkDataArrayCollection* arrays)
{
  this->Superclass::ConstructBackend(arrays);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkIdType vtkCompositeArray<ValueTypeT>::GetNumberOfArrays()
{
  auto backend = this->GetBackend();
  return backend ? backend->GetNumberOfArrays() : 0;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkDataArray* vtkCompositeArray<ValueTypeT>::GetArray(vtkIdType idx)
{
  auto backend = this->GetBackend();
  return backend ? backend->GetArray(idx) : nullptr;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkIdType vtkCompositeArray<ValueTypeT>::GetOffset(vtkIdType idx)
{
  auto backend = this->GetBackend();
  return backend ? backend->GetOffset(idx) : 0;
}

VTK_ABI_NAMESPACE_END

//-----------------------------------------------------------------------
namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueTypeT>
vtkSmartPointer<vtkCompositeArray<ValueTypeT>> ConcatenateDataArrays(
  const std::vector<vtkDataArray*>& arrays)
{
  if (arrays.empty())
  {
    return nullptr;
  }
  vtkIdType nComps = -1;
  for (auto arr : arrays)
  {
    if (arr == nullptr)
    {
      continue;
    }
    if (nComps == -1)
    {
      nComps = arr->GetNumberOfComponents();
    }
    else if (arr->GetNumberOfComponents() != nComps)
    {
      vtkErrorWithObjectMacro(nullptr, "Number of components of all the arrays are not equal");
      return nullptr;
    }
  }
  vtkNew<vtkCompositeArray<ValueTypeT>> composite;
  composite->SetBackend(std::make_shared<vtkCompositeImplicitBackend<ValueTypeT>>(arrays));
  composite->SetNumberOfComponents(nComps);
  int nTuples = 0;
  std::for_each(arrays.begin(), arrays.end(),
    [&nTuples](vtkDataArray* arr) { nTuples += arr ? arr->GetNumberOfTuples() : 0; });
  composite->SetNumberOfTuples(nTuples);
  return composite;
}
VTK_ABI_NAMESPACE_END
}
#endif // header guard
