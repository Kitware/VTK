/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkElevationFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkElevationFilter);

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkElevationFilter::~vtkElevationFilter()
{
}

//----------------------------------------------------------------------------
void vtkElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Low Point: ("
     << this->LowPoint[0] << ", "
     << this->LowPoint[1] << ", "
     << this->LowPoint[2] << ")\n";
  os << indent << "High Point: ("
     << this->HighPoint[0] << ", "
     << this->HighPoint[1] << ", "
     << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: ("
     << this->ScalarRange[0] << ", "
     << this->ScalarRange[1] << ")\n";
}

//----------------------------------------------------------------------------
int vtkElevationFilter::RequestData(vtkInformation*,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  // Check the size of the input.
  vtkIdType numPts = input->GetNumberOfPoints();
  if(numPts < 1)
    {
    vtkDebugMacro("No input!");
    return 1;
    }

  // Allocate space for the elevation scalar data.
  vtkSmartPointer<vtkFloatArray> newScalars =
    vtkSmartPointer<vtkFloatArray>::New();
  newScalars->SetNumberOfTuples(numPts);

  // Set up 1D parametric system and make sure it is valid.
  double diffVector[3] =
    { this->HighPoint[0] - this->LowPoint[0],
      this->HighPoint[1] - this->LowPoint[1],
      this->HighPoint[2] - this->LowPoint[2] };
  double length2 = vtkMath::Dot(diffVector, diffVector);
  if(length2 <= 0)
    {
    vtkErrorMacro("Bad vector, using (0,0,1).");
    diffVector[0] = 0;
    diffVector[1] = 0;
    diffVector[2] = 1;
    length2 = 1.0;
    }

  // Support progress and abort.
  vtkIdType tenth = (numPts >= 10? numPts/10 : 1);
  double numPtsInv = 1.0/numPts;
  int abort = 0;

  // Compute parametric coordinate and map into scalar range.
  double diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
  vtkDebugMacro("Generating elevation scalars!");
  for(vtkIdType i=0; i < numPts && !abort; ++i)
    {
    // Periodically update progress and check for an abort request.
    if(i % tenth == 0)
      {
      this->UpdateProgress((i+1)*numPtsInv);
      abort = this->GetAbortExecute();
      }

    // Project this input point into the 1D system.
    double x[3];
    input->GetPoint(i, x);
    double v[3] = { x[0] - this->LowPoint[0],
                    x[1] - this->LowPoint[1],
                    x[2] - this->LowPoint[2] };
    double s = vtkMath::Dot(v, diffVector) / length2;
    s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);

    // Store the resulting scalar value.
    newScalars->SetValue(i, this->ScalarRange[0] + s*diffScalar);
    }

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  newScalars->SetName("Elevation");
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars("Elevation");

  return 1;
}
