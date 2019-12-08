/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointData.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

#include <vector>

vtkStandardNewMacro(vtkPointData);

void vtkPointData::NullPoint(vtkIdType ptId)
{
  vtkFieldData::Iterator it(this);
  vtkDataArray* da;
  std::vector<float> tuple(32, 0.f);
  for (da = it.Begin(); !it.End(); da = it.Next())
  {
    if (da)
    {
      const size_t numComps = static_cast<size_t>(da->GetNumberOfComponents());
      if (numComps > tuple.size())
      {
        tuple.resize(numComps, 0.f);
      }
      da->InsertTuple(ptId, tuple.data());
    }
  }
}

void vtkPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
