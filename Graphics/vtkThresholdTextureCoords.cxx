/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTextureCoords.cxx
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
#include "vtkThresholdTextureCoords.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkThresholdTextureCoords, "1.34");
vtkStandardNewMacro(vtkThresholdTextureCoords);

// Construct with lower threshold=0, upper threshold=1, threshold 
// function=upper, and texture dimension = 2. The "out" texture coordinate
// is (0.25,0,0); the "in" texture coordinate is (0.75,0,0).
vtkThresholdTextureCoords::vtkThresholdTextureCoords()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->TextureDimension = 2;

  this->ThresholdFunction = &vtkThresholdTextureCoords::Upper;

  this->OutTextureCoord[0] = 0.25;
  this->OutTextureCoord[1] = 0.0;
  this->OutTextureCoord[2] = 0.0;

  this->InTextureCoord[0] = 0.75;
  this->InTextureCoord[1] = 0.0;
  this->InTextureCoord[2] = 0.0;
}

// Criterion is cells whose scalars are less than lower threshold.
void vtkThresholdTextureCoords::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThresholdTextureCoords::Lower;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are less than upper threshold.
void vtkThresholdTextureCoords::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThresholdTextureCoords::Upper;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThresholdTextureCoords::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThresholdTextureCoords::Between;
    this->Modified();
    }
}
  
void vtkThresholdTextureCoords::Execute()
{
  vtkIdType numPts;
  vtkFloatArray *newTCoords;
  vtkIdType ptId;
  vtkDataArray *inScalars;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  vtkDebugMacro(<< "Executing texture threshold filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to texture threshold");
    return;
    }
     
  numPts = input->GetNumberOfPoints();
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*this->TextureDimension);

  // Check that the scalars of each point satisfy the threshold criterion
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( (this->*(this->ThresholdFunction))(inScalars->GetComponent(ptId,0)) )
      {
      newTCoords->InsertTuple(ptId,this->InTextureCoord);
      }
    else //doesn't satisfy criterion
      {
      newTCoords->InsertTuple(ptId,this->OutTextureCoord);
      }

    } //for all points

  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkThresholdTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->ThresholdFunction == &vtkThresholdTextureCoords::Upper )
    {
    os << indent << "Threshold By Upper\n";
    }

  else if ( this->ThresholdFunction == &vtkThresholdTextureCoords::Lower )
    {
    os << indent << "Threshold By Lower\n";
    }

  else if ( this->ThresholdFunction == &vtkThresholdTextureCoords::Between )
    {
    os << indent << "Threshold Between\n";
    }

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
  os << indent << "Texture Dimension: " << this->TextureDimension << "\n";;

  os << indent << "Out Texture Coordinate: (" << this->OutTextureCoord[0] 
     << ", " << this->OutTextureCoord[1]
     << ", " << this->OutTextureCoord[2] << ")\n";

  os << indent << "In Texture Coordinate: (" << this->InTextureCoord[0] 
     << ", " << this->InTextureCoord[1]
     << ", " << this->InTextureCoord[2] << ")\n";
}
