/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXDataArray.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXDataArray.cxx
 *
 *  Created on: Jun 4, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXDataArray.h"

#include <algorithm> //std::copy

#include "vtkDoubleArray.h"

namespace vtx
{
namespace types
{

bool DataArray::IsScalar() const noexcept
{
  if (this->VectorVariables.empty())
  {
    return true;
  }
  return false;
}

void DataArray::ConvertTo3DVTK(const std::vector<double>& fillValues)
{
  const int components = this->Data->GetNumberOfComponents();
  if (this->Data->GetDataType() != VTK_DOUBLE)
  {
    return;
  }

  if (components == 2 || components == 1)
  {
    // copy contents to a temporary
    double* data = vtkDoubleArray::SafeDownCast(this->Data.GetPointer())->GetPointer(0);
    const size_t size = static_cast<size_t>(this->Data->GetSize());
    // using raw memory to just use new and copy without initializing
    double* temporary = new double[size];
    std::copy(data, data + size, temporary);

    // reallocate
    const size_t tuples = static_cast<size_t>(this->Data->GetNumberOfTuples());
    const size_t newSize = 3 * tuples;

    const double fillValue = (fillValues.size() == 1) ? fillValues.front() : 0.;

    // set vtkDataArray to 3D
    this->Data->Reset();
    this->Data->Allocate(static_cast<vtkIdType>(newSize));
    this->Data->SetNumberOfComponents(3);
    this->Data->SetNumberOfTuples(static_cast<vtkIdType>(tuples));

    for (size_t t = 0; t < tuples; ++t)
    {
      const vtkIdType tVTK = static_cast<vtkIdType>(t);
      this->Data->SetComponent(static_cast<vtkIdType>(tVTK), 0, temporary[2 * tVTK]);

      if (components == 2)
      {
        this->Data->SetComponent(tVTK, 1, temporary[2 * tVTK + 1]);
      }
      else if (components == 1)
      {
        this->Data->SetComponent(tVTK, 1, fillValue);
      }

      this->Data->SetComponent(tVTK, 2, fillValue);
    }

    delete[] temporary;
  }

  // swap tuples with components and swap data
  if (IsSOA)
  {
    double* data = vtkDoubleArray::SafeDownCast(this->Data.GetPointer())->GetPointer(0);
    const size_t size = static_cast<size_t>(this->Data->GetSize());
    // using raw memory to just use new and copy without initializing
    double* temporary = new double[size];
    std::copy(data, data + size, temporary);

    // set vtkDataArray to 3D
    const size_t tuples = static_cast<size_t>(this->Data->GetNumberOfComponents());
    this->Data->SetNumberOfComponents(3);
    this->Data->SetNumberOfTuples(tuples);

    for (size_t t = 0; t < tuples; ++t)
    {
      const vtkIdType tVTK = static_cast<vtkIdType>(t);
      this->Data->SetComponent(tVTK, 0, temporary[3 * tVTK]);
      this->Data->SetComponent(tVTK, 1, temporary[3 * tVTK + 1]);
      this->Data->SetComponent(tVTK, 2, temporary[3 * tVTK + 2]);
    }

    delete[] temporary;
  }
}

} // end namespace types
} // end namespace vtx
