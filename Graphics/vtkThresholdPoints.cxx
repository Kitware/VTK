/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdPoints.cxx
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
#include "vtkThresholdPoints.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkThresholdPoints, "1.32");
vtkStandardNewMacro(vtkThresholdPoints);

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper.
vtkThresholdPoints::vtkThresholdPoints()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;

  this->ThresholdFunction = &vtkThresholdPoints::Upper;
}

// Criterion is cells whose scalars are less than lower threshold.
void vtkThresholdPoints::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThresholdPoints::Lower;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are less than upper threshold.
void vtkThresholdPoints::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThresholdPoints::Upper;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThresholdPoints::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThresholdPoints::Between;
    this->Modified();
    }
}
  
void vtkThresholdPoints::Execute()
{
  vtkDataArray *inScalars;
  vtkPoints *newPoints;
  vtkPointData *pd, *outPD;
  vtkCellArray *verts;
  vtkIdType ptId, numPts, pts[1];
  float *x;
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  vtkDebugMacro(<< "Executing threshold points filter");

  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to threshold");
    return;
    }
     
  numPts = input->GetNumberOfPoints();
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd);
  verts = vtkCellArray::New();
  verts->Allocate(verts->EstimateSize(numPts,1));

  // Check that the scalars of each point satisfy the threshold criterion
  int abort=0;
  vtkIdType progressInterval = numPts/20+1;
  
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((float)ptId/numPts);
      abort = this->GetAbortExecute();
      }

    if ( (this->*(this->ThresholdFunction))(inScalars->GetComponent(ptId,0)) ) 
      {
      x = input->GetPoint(ptId);
      pts[0] = newPoints->InsertNextPoint(x);
      outPD->CopyData(pd,ptId,pts[0]);
      verts->InsertNextCell(1,pts);
      } // satisfied thresholding
    } // for all points

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points.");


  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(verts);
  verts->Delete();

  output->Squeeze();
}

void vtkThresholdPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
}
