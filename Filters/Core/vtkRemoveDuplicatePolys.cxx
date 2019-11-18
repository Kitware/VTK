/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveDuplicatePolys.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemoveDuplicatePolys.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

vtkStandardNewMacro(vtkRemoveDuplicatePolys);

//----------------------------------------------------------------------------
vtkRemoveDuplicatePolys::vtkRemoveDuplicatePolys() {}

//----------------------------------------------------------------------------
vtkRemoveDuplicatePolys::~vtkRemoveDuplicatePolys() {}

//----------------------------------------------------------------------------
void vtkRemoveDuplicatePolys::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkRemoveDuplicatePolys::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPolys() == 0)
  {
    // set up a polyData with same data arrays as input, but
    // no points, polys or data.
    output->ShallowCopy(input);
    return 1;
  }

  // Copy over the original points. Assume there are no degenerate points.
  output->SetPoints(input->GetPoints());

  // Remove duplicate polys.
  std::map<std::set<int>, vtkIdType> polySet;
  std::map<std::set<int>, vtkIdType>::iterator polyIter;

  // Now copy the polys.
  vtkIdList* polyPoints = vtkIdList::New();
  const vtkIdType numberOfPolys = input->GetNumberOfPolys();
  vtkIdType progressStep = numberOfPolys / 100;
  if (progressStep == 0)
  {
    progressStep = 1;
  }

  output->AllocateCopy(input);
  int ndup = 0;

  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyAllocate(input->GetCellData(), numberOfPolys);

  for (int id = 0; id < numberOfPolys; ++id)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.8 + 0.2 * (static_cast<float>(id) / numberOfPolys));
    }

    // duplicate points do not make poly vertices or triangles
    // strips degenerate so don't remove them
    int polyType = input->GetCellType(id);
    if (polyType == VTK_POLY_VERTEX || polyType == VTK_TRIANGLE_STRIP)
    {
      input->GetCellPoints(id, polyPoints);
      vtkIdType newId = output->InsertNextCell(polyType, polyPoints);
      output->GetCellData()->CopyData(input->GetCellData(), id, newId);
      continue;
    }

    input->GetCellPoints(id, polyPoints);
    std::set<int> nn;
    std::vector<int> ptIds;
    for (int i = 0; i < polyPoints->GetNumberOfIds(); ++i)
    {
      int polyPtId = polyPoints->GetId(i);
      nn.insert(polyPtId);
      ptIds.push_back(polyPtId);
    }

    // this conditional may generate non-referenced nodes
    polyIter = polySet.find(nn);

    // only copy a cell to the output if it is neither degenerate nor duplicate
    if (nn.size() == static_cast<unsigned int>(polyPoints->GetNumberOfIds()) &&
      polyIter == polySet.end())
    {
      vtkIdType newId = output->InsertNextCell(input->GetCellType(id), polyPoints);
      output->GetCellData()->CopyData(input->GetCellData(), id, newId);
      polySet[nn] = newId;
    }
    else if (polyIter != polySet.end())
    {
      ++ndup; // cell has duplicate(s)
    }
  }

  if (ndup)
  {
    vtkDebugMacro(<< "vtkRemoveDuplicatePolys : " << ndup
                  << " duplicate polys (multiple instances of a polygon) have been"
                  << " removed." << endl);

    polyPoints->Delete();
    output->Squeeze();
  }

  return 1;
}
