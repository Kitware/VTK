/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkElevationFilter.cxx
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
#include "vtkElevationFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkElevationFilter, "1.49");
vtkStandardNewMacro(vtkElevationFilter);

// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vtkElevationFilter::vtkElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;
 
  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//
// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vtkElevationFilter::Execute()
{
  vtkIdType numPts, i;
  int j;
  vtkFloatArray *newScalars;
  float l, *x, s, v[3];
  float diffVector[3], diffScalar;
  vtkDataSet *input = this->GetInput();
  int abort=0;

  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

  // First, copy the input to the output as a starting point
  this->GetOutput()->CopyStructure( input );

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    //vtkErrorMacro(<< "No input!");
    return;
    }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // Set up 1D parametric system
  //
  for (i=0; i<3; i++)
    {
    diffVector[i] = this->HighPoint[i] - this->LowPoint[i];
    }
  if ( (l = vtkMath::Dot(diffVector,diffVector)) == 0.0)
    {
    vtkErrorMacro(<< this << ": Bad vector, using (0,0,1)\n");
    diffVector[0] = diffVector[1] = 0.0; diffVector[2] = 1.0;
    l = 1.0;
    }

  // Compute parametric coordinate and map into scalar range
  //
  int tenth = numPts/10 + 1;
  diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
  for (i=0; i<numPts && !abort; i++)
    {
    if ( ! (i % tenth) ) 
      {
      this->UpdateProgress ((float)i/numPts);
      abort = this->GetAbortExecute();
      }

    x = input->GetPoint(i);
    for (j=0; j<3; j++)
      {
      v[j] = x[j] - this->LowPoint[j];
      }
    s = vtkMath::Dot(v,diffVector) / l;
    s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);
    newScalars->SetValue(i,this->ScalarRange[0]+s*diffScalar);
    }

  // Update self
  //
  this->GetInput()->GetPointData()->CopyScalarsOff();
  this->GetOutput()->GetPointData()->PassData(input->GetPointData());

  this->GetOutput()->GetCellData()->PassData(input->GetCellData());

  newScalars->SetName("Elevation");
  this->GetOutput()->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Low Point: (" << this->LowPoint[0] << ", "
                                << this->LowPoint[1] << ", "
                                << this->LowPoint[2] << ")\n";
  os << indent << "High Point: (" << this->HighPoint[0] << ", "
                                << this->HighPoint[1] << ", "
                                << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                << this->ScalarRange[1] << ")\n";
}
