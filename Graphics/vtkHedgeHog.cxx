/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHedgeHog.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHedgeHog, "1.38");
vtkStandardNewMacro(vtkHedgeHog);

vtkHedgeHog::vtkHedgeHog()
{
  this->ScaleFactor = 1.0;
  this->VectorMode = VTK_USE_VECTOR;
}

void vtkHedgeHog::Execute()
{
  vtkDataSet *input= this->GetInput();
  vtkIdType numPts;
  vtkPoints *newPts;
  vtkPointData *pd;
  vtkDataArray *inVectors;
  vtkDataArray *inNormals;
  vtkIdType ptId;
  int i;
  vtkIdType pts[2];
  vtkCellArray *newLines;
  float *x, *v;
  float newX[3];
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  // Initialize
  //
  numPts = input->GetNumberOfPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();
  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }
  if ( !inVectors && this->VectorMode == VTK_USE_VECTOR)
    {
    vtkErrorMacro(<<"No vectors in input data");
    return;
    }

  inNormals = pd->GetNormals();
  if ( !inNormals && this->VectorMode == VTK_USE_NORMAL)
    {
    vtkErrorMacro(<<"No normals in input data");
    return;
    }
  outputPD->CopyAllocate(pd, 2*numPts);

  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(2*numPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numPts,2));

  // Loop over all points, creating oriented line
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( ! (ptId % 10000) ) //abort/progress
      {
      this->UpdateProgress ((float)ptId/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }
    
    x = input->GetPoint(ptId);
    if (this->VectorMode == VTK_USE_VECTOR)
      {
      v = inVectors->GetTuple(ptId);
      }
    else
      {
      v = inNormals->GetTuple(ptId);
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
      }

    pts[0] = ptId;
    pts[1] = ptId + numPts;;

    newPts->SetPoint(pts[0], x);
    newPts->SetPoint(pts[1], newX);

    newLines->InsertNextCell(2,pts);

    outputPD->CopyData(pd,ptId,pts[0]);
    outputPD->CopyData(pd,ptId,pts[1]);
    }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}

void vtkHedgeHog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ? 
                                       "Orient by vector\n" : "Orient by normal\n");
}
