/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleElevationFilter.cxx
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
#include "vtkSimpleElevationFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkSimpleElevationFilter, "1.13");
vtkStandardNewMacro(vtkSimpleElevationFilter);

// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vtkSimpleElevationFilter::vtkSimpleElevationFilter()
{
  this->Vector[0] = 0.0;
  this->Vector[1] = 0.0;
  this->Vector[2] = 1.0;
}

// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vtkSimpleElevationFilter::Execute()
{
  vtkIdType i, numPts;
  vtkFloatArray *newScalars;
  float s, x[3];
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

 // First, copy the input to the output as a starting point
  output->CopyStructure( input );

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
  if ( vtkMath::Dot(this->Vector,this->Vector) == 0.0)
    {
    vtkErrorMacro(<< "Bad vector, using (0,0,1)");
    this->Vector[0] = this->Vector[1] = 0.0; this->Vector[2] = 1.0;
    }

  // Compute dot product
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (i=0; i<numPts && !abort; i++)
    {
    if ( ! (i % progressInterval) ) 
      {
      this->UpdateProgress ((float)i/numPts);
      abort = this->GetAbortExecute();
      }

    input->GetPoint(i,x);
    s = vtkMath::Dot(this->Vector,x);
    newScalars->SetComponent(i,0,s);
    }

  // Update self
  //
  output->GetPointData()->CopyScalarsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  newScalars->SetName("Elevation");
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkSimpleElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vector: (" << this->Vector[0] << ", "
     << this->Vector[1] << ", " << this->Vector[2] << ")\n";
}
