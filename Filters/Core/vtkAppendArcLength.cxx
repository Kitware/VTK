/*=========================================================================

  Program:   ParaView
  Module:    vtkAppendArcLength.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendArcLength.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkAppendArcLength);
//----------------------------------------------------------------------------
vtkAppendArcLength::vtkAppendArcLength()
{
}

//----------------------------------------------------------------------------
vtkAppendArcLength::~vtkAppendArcLength()
{
}

//----------------------------------------------------------------------------
int vtkAppendArcLength::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
  if (input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  output->ShallowCopy(input);

  // Now add "arc_length" array.
  vtkDataArray* arc_length = 0;
  vtkPoints* points = output->GetPoints();
  vtkIdType numPoints = points->GetNumberOfPoints();
  if (points->GetDataType() == VTK_DOUBLE)
  {
    arc_length = vtkDoubleArray::New();
  }
  else
  {
    arc_length = vtkFloatArray::New();
  }
  arc_length->SetName("arc_length");
  arc_length->SetNumberOfComponents(1);
  arc_length->SetNumberOfTuples(numPoints);
  arc_length->FillComponent(0, 0.0);

  vtkCellArray* lines = output->GetLines();
  vtkIdType numCellPoints;
  vtkIdType* cellPoints;
  lines->InitTraversal();
  while (lines->GetNextCell(numCellPoints, cellPoints))
  {
    if (numCellPoints == 0)
    {
      continue;
    }
    double arc_distance = 0.0;
    double prevPoint[3];
    points->GetPoint(cellPoints[0], prevPoint);
    for (vtkIdType cc = 1; cc < numCellPoints; cc++)
    {
      double curPoint[3];
      points->GetPoint(cellPoints[cc], curPoint);
      double distance = sqrt(vtkMath::Distance2BetweenPoints(curPoint, prevPoint));
      arc_distance += distance;
      arc_length->SetTuple1(cellPoints[cc], arc_distance);
      memcpy(prevPoint, curPoint, 3 * sizeof(double));
    }
  }
  output->GetPointData()->AddArray(arc_length);
  arc_length->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendArcLength::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
