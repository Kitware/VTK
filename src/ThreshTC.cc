/*=========================================================================

  Program:   Visualization Library
  Module:    ThreshTC.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ThreshTC.hh"

// Construct with lower threshold=0, upper threshold=1, threshold 
// function=upper, and texture dimension = 2.
vlThresholdTextureCoords::vlThresholdTextureCoords()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->TextureDimension = 2;

  this->ThresholdFunction = &vlThresholdTextureCoords::Upper;
}

// Description:
// Criterion is cells whose scalars are less than lower threshold.
void vlThresholdTextureCoords::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vlThresholdTextureCoords::Lower;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are less than upper threshold.
void vlThresholdTextureCoords::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vlThresholdTextureCoords::Upper;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are between lower and upper thresholds.
void vlThresholdTextureCoords::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = vlThresholdTextureCoords::Between;
    this->Modified();
    }
}
  
void vlThresholdTextureCoords::Execute()
{
  vlDataSet *input=this->Input;
  int numPts;
  vlFloatTCoords *newTCoords;
  int ptId;
  float inTC[3], outTC[3];
  vlScalars *inScalars;

  vlDebugMacro(<< "Executing texture threshold filter");
  this->Initialize();

  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vlErrorMacro(<<"No scalar data to texture threshold");
    return;
    }
     
  numPts = input->GetNumberOfPoints();
  newTCoords = new vlFloatTCoords(this->TextureDimension);
  inTC[0] = inTC[1] = inTC[2] = 1.0;
  outTC[0] = outTC[1] = outTC[2] = 0.0;

  // Check that the scalars of each cell satisfy the threshold criterion
  for (ptId=0; ptId < input->GetNumberOfCells(); ptId++)
    {
    if ( ((this->*(this->ThresholdFunction))(inScalars->GetScalar(ptId))) )
      {
      newTCoords->InsertTCoord(ptId,inTC);
      }
    else //doesn't satisfy criterion
      {
      newTCoords->InsertTCoord(ptId,outTC);
      }

    } //for all points

  this->GetPointData()->SetTCoords(newTCoords);
}

void vlThresholdTextureCoords::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToDataSetFilter::PrintSelf(os,indent);

  if ( this->ThresholdFunction == &vlThresholdTextureCoords::Upper )
    os << indent << "Threshold By Upper\n";

  else if ( this->ThresholdFunction == &vlThresholdTextureCoords::Lower )
    os << indent << "Threshold By Lower\n";

  else if ( this->ThresholdFunction == &vlThresholdTextureCoords::Between )
    os << indent << "Threshold Between\n";

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
  os << indent << "Texture Dimension: " << this->TextureDimension << "\n";;
}
