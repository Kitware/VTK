/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImpTC.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ImpTC.hh"

// Description:
// Create object with texture dimension=2 and no r-s-t implicit functions defined.
vtkImplicitTextureCoords::vtkImplicitTextureCoords()
{
  this->RFunction = NULL;
  this->SFunction = NULL;
  this->TFunction = NULL;
}

void vtkImplicitTextureCoords::Execute()
{
  int ptId, numPts, tcoordDim;
  vtkFloatTCoords *newTCoords;
  float min[3], max[3], scale[3];
  float tCoord[3], *tc, *x;
  int i;
  vtkDataSet *input=this->Input;
//
// Initialize
//
  vtkDebugMacro(<<"Generating texture coordinates from implicit functions...");
  this->Initialize();

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    vtkErrorMacro(<< "No input points!");
    return;
    }

  if ( this->RFunction == NULL )
    {
    vtkErrorMacro(<< "No implicit functions defined!");
    return;
    }

  tcoordDim = 1;
  if ( this->SFunction != NULL )
    {
    tcoordDim++;
    if ( this->TFunction != NULL )
      {
      tcoordDim++;
      }
    }
//
// Allocate
//
  tCoord[0] = tCoord[1] = tCoord[2] = 0.0;

  if ( tcoordDim == 1 ) //force 2D map to be created
    newTCoords = new vtkFloatTCoords(numPts,2);
  else
    newTCoords = new vtkFloatTCoords(numPts,tcoordDim);
//
// Compute implicit function values -> insert as initial texture coordinate
//
  for (i=0; i<3; i++) //initialize min/max values array
    {
    min[i] = LARGE_FLOAT;
    max[i] = -LARGE_FLOAT;
    }
  for (ptId=0; ptId<numPts; ptId++) //compute texture coordinates
    {
    x = input->GetPoint(ptId);
    tCoord[0] = this->RFunction->EvaluateFunction(x);
    if ( this->SFunction ) tCoord[1] = this->SFunction->EvaluateFunction(x);
    if ( this->TFunction ) tCoord[2] = this->TFunction->EvaluateFunction(x);

    for (i=0; i<tcoordDim; i++)
      {
      if (tCoord[i] < min[i]) min[i] = tCoord[i];
      if (tCoord[i] > max[i]) max[i] = tCoord[i];
      }

    newTCoords->InsertTCoord(ptId,tCoord);
    }
//
// Scale and shift texture coordinates into (0,1) range, with 0.0 implicit 
// function value equal to texture coordinate value of 0.5
//
  for (i=0; i<tcoordDim; i++)
    {
    scale[i] = 1.0;
    if ( max[i] > 0.0 && min[i] < 0.0 ) //have positive & negative numbers
      {
      if ( max[i] > (-min[i]) ) scale[i] = 0.5 / max[i]; //scale into 0.5->1
      else scale[i] = -0.5 / min[i]; //scale into 0->0.5
      }
    else if ( max[i] > 0.0 ) //have positive numbers only
      {
      scale[i] = 0.5 / max[i]; //scale into 0.5->1.0
      }
    else if ( min[i] < 0.0 ) //have negative numbers only
      {
      scale[i] = -0.5 / min[i]; //scale into 0.0->0.5
      }
    }

  for (ptId=0; ptId<numPts; ptId++)
    {
    tc = newTCoords->GetTCoord(ptId);
    for (i=0; i<tcoordDim; i++) tCoord[i] = 0.5 + scale[i] * tc[i];
    newTCoords->InsertTCoord(ptId,tCoord);
    }
//
// Update self
//
  this->PointData.CopyTCoordsOff();
  this->PointData.PassData(input->GetPointData());

  this->PointData.SetTCoords(newTCoords);
}

void vtkImplicitTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  if ( this->RFunction != NULL )
    {
    if ( this->SFunction != NULL )
      {
      if ( this->TFunction != NULL )
        {
        os << indent << "R, S, and T Functions defined\n";
        }
      }
    else
      {
      os << indent << "R and S Functions defined\n";
      }
    }
  else
    {
    os << indent << "R Function defined\n";
    }
}
