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

#include "vtkAOSDataArrayTemplate.h"
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
  template <class Iterator>
  Internals(Iterator first, Iterator last)
  {
    this->Arrays.assign(first, last);
    if (this->Arrays.size() > 0)
    {
      this->Ranges.resize(this->Arrays.size());
      std::transform(first, last, this->Ranges.begin(),
        [](vtkDataArray* arr) { return vtk::DataArrayValueRange(arr); });
      this->Offsets.resize(this->Arrays.size() - 1);
      std::size_t runningSum = 0;
      std::transform(this->Ranges.begin(), this->Ranges.end() - 1, this->Offsets.begin(),
        [&runningSum](vtk::detail::SelectValueRange<vtkDataArray*,
          vtk::detail::DynamicTupleSize>::type& range) {
          runningSum += range.size();
          return runningSum;
        });
    }
  }

  std::vector<vtkSmartPointer<vtkDataArray>> Arrays;
  std::vector<vtk::detail::SelectValueRange<vtkDataArray*, vtk::detail::DynamicTupleSize>::type>
    Ranges;
  std::vector<std::size_t> Offsets;
};

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::vtkCompositeImplicitBackend(
  const std::vector<vtkDataArray*>& arrays)
  : Internal(std::unique_ptr<Internals>(new Internals(arrays.begin(), arrays.end())))
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkCompositeImplicitBackend<ValueType>::~vtkCompositeImplicitBackend() = default;

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkCompositeImplicitBackend<ValueType>::operator()(int idx) const
{
  auto itPos =
    std::upper_bound(this->Internal->Offsets.begin(), this->Internal->Offsets.end(), idx);
  int locIdx = itPos == this->Internal->Offsets.begin() ? idx : idx - *(itPos - 1);
  return this->Internal->Ranges[std::distance(this->Internal->Offsets.begin(), itPos)][locIdx];
}
VTK_ABI_NAMESPACE_END
