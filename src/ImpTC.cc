/*=========================================================================

  Program:   Visualization Library
  Module:    ImpTC.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ImpTC.hh"

vlImplicitTextureCoords::vlImplicitTextureCoords()
{
  this->RFunction = NULL;
  this->SFunction = NULL;
  this->TFunction = NULL;

  this->ScaleFactor = 1.0;

  this->Clamp = 0;
  this->RRange[0] = -100.0;
  this->RRange[1] =  100.0;
  this->SRange[0] = -100.0;
  this->SRange[1] =  100.0;
  this->TRange[0] = -100.0;
  this->TRange[1] =  100.0;

  this->Transform = NULL;
  this->SelfCreatedTransform = 0;
}

void vlImplicitTextureCoords::Execute()
{
  int ptId, numPts, tcoordDim;
  vlFloatTCoords *newTCoords;
//
// Initialize
//
  vlDebugMacro(<<"Generating texture coordinates!");
  this->Initialize();

  if ( ((numPts=this->Input->GetNumberOfPoints()) < 1) )
    {
    vlErrorMacro(<< "No input!");
    return;
    }

  if ( this->RFunction == NULL )
    {
    vlErrorMacro(<< "No input!");
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
  newTCoords = new vlFloatTCoords(numPts,tcoordDim);
//
// Compute texture coordinate and map into appropriate range
//
  for (ptId=0; ptId<numPts; ptId++)
    {
    }
//
// Update self
//
  this->PointData.CopyTCoordsOff();
  this->PointData.PassData(this->Input->GetPointData());

  this->PointData.SetTCoords(newTCoords);
}

// Description:
// Specify a transform to position implicit function.
void vlImplicitTextureCoords::SetTransform(vlTransform *transform)
{
  if ( this->Transform != transform ) 
    {
    if ( this->SelfCreatedTransform ) delete this->Transform;
    this->SelfCreatedTransform = 0;
    this->Transform = transform;
    this->Modified();
    }
}

// Description:
// Create default transform. Used to create one when none is specified.  
// Default transform is identity matrix.
void vlImplicitTextureCoords::CreateDefaultTransform()
{
  if ( this->SelfCreatedTransform ) delete this->Transform;
  this->Transform = new vlTransform;
  this->SelfCreatedTransform = 1;
}

void vlImplicitTextureCoords::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Clamp: " << (this->Clamp ? "On\n" : "Off\n");

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "RRange: (" << this->RRange[0] << ", " << this->RRange[1] << ")\n";
  os << indent << "SRange: (" << this->SRange[0] << ", " << this->SRange[1] << ")\n";
  os << indent << "TRange: (" << this->TRange[0] << ", " << this->TRange[1] << ")\n";

}
