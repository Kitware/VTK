/*=========================================================================

  Program:   Visualization Library
  Module:    BrownPts.cc
  Language:  C++
  Date:      9/14/94
  Version:   1.1

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BrownPts.hh"
#include "FVectors.hh"
#include "vlMath.hh"

vlBrownianPoints::vlBrownianPoints()
{
  this->MinimumSpeed = 0.0;
  this->MaximumSpeed = 1.0;
}

void vlBrownianPoints::Execute()
{
  int i, numPts;
  vlFloatVectors *newVectors;
  vlMath math;
  float v[3], norm, speed;

  vlDebugMacro(<< "Executing Brownian filter");

  this->Initialize();

  if ( ((numPts=this->Input->GetNumberOfPoints()) < 1) )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }

  newVectors = new vlFloatVectors(numPts);
//
// Check consistency of minumum and maximum speed
//  
  if ( this->MinimumSpeed > this->MaximumSpeed )
    {
    vlErrorMacro(<< " Minimum speed > maximum speed; reset to (0,1).");
    this->MinimumSpeed = 0.0;
    this->MaximumSpeed = 1.0;
    }

  for (i=0; i<numPts; i++)
    {
    speed = math.Random(this->MinimumSpeed,this->MaximumSpeed);
    if ( speed != 0.0 )
      {
      for (i=0; i<3; i++) v[i] = math.Random(0,speed);
      norm = math.Norm(v);
      for (i=0; i<3; i++) v[i] *= (speed / norm);
      }
    else
      {
      for (i=0; i<3; i++) v[i] = 0.0;
      }

    newVectors->InsertNextVector(v);
    }
//
// Update ourselves
//
  this->PointData.CopyVectorsOff();
  this->PointData.PassData(this->Input->GetPointData());

  this->GetPointData()->SetVectors(newVectors);
}

void vlBrownianPoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Minimum Speed: " << this->MinimumSpeed << "\n";
  os << indent << "Maximum Speed: " << this->MaximumSpeed << "\n";
}
