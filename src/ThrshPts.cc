/*=========================================================================

  Program:   Visualization Library
  Module:    ThrshPts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ThrshPts.hh"

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper.
vlThresholdPoints::vlThresholdPoints()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;

  this->ThresholdFunction = &vlThresholdPoints::Upper;
}

// Description:
// Criterion is cells whose scalars are less than lower threshold.
void vlThresholdPoints::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vlThresholdPoints::Lower;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are less than upper threshold.
void vlThresholdPoints::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vlThresholdPoints::Upper;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are between lower and upper thresholds.
void vlThresholdPoints::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = vlThresholdPoints::Between;
    this->Modified();
    }
}
  
void vlThresholdPoints::Execute()
{
  int cellId;
  vlScalars *inScalars;
  vlFloatPoints *newPoints;
  vlPointData *pd;
  vlCellArray *verts;
  int i, ptId, pts[1], numPts;
  float *x;

  vlDebugMacro(<< "Executing threshold points filter");
  this->Initialize();

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vlErrorMacro(<<"No scalar data to threshold");
    return;
    }
     
  numPts = this->Input->GetNumberOfPoints();
  newPoints = new vlFloatPoints(numPts);
  pd = this->Input->GetPointData();
  this->PointData.CopyAllocate(pd);
  verts = new vlCellArray();
  verts->Allocate(verts->EstimateSize(numPts,1));

  // Check that the scalars of each point satisfy the threshold criterion
  for (ptId=0; ptId < this->Input->GetNumberOfPoints(); ptId++)
    {
    if ( (this->*(this->ThresholdFunction))(inScalars->GetScalar(ptId)) ) 
      {
      x = this->Input->GetPoint(ptId);
      pts[0] = newPoints->InsertNextPoint(x);
      this->PointData.CopyData(pd,ptId,pts[0]);
      verts->InsertNextCell(1,pts);
      } // satisfied thresholding
    } // for all points

  vlDebugMacro(<< "Extracted " << this->GetNumberOfPoints() << " points.");

  // now clean up / update ourselves
  this->SetPoints(newPoints);
  this->SetVerts(verts);
  this->Squeeze();
}

void vlThresholdPoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
}
