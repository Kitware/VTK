/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoolText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BoolText.hh"
#include "AGraymap.hh"

vtkBooleanTexture::vtkBooleanTexture()
{
  this->Thickness = 0;

  this->XSize = this->YSize = 12;

  this->InIn[0] = this->InIn[1] = 255;
  this->InOut[0] = this->InOut[1] = 255;
  this->OutIn[0] = this->OutIn[1] = 255;
  this->OutOut[0] = this->OutOut[1] = 255;
  this->OnOn[0] = this->OnOn[1] = 255;
  this->OnIn[0] = this->OnIn[1] = 255;
  this->OnOut[0] = this->OnOut[1] = 255;
  this->InOn[0] = this->InOn[1] = 255;
  this->OutOn[0] = this->OutOn[1] = 255;
}

void vtkBooleanTexture::Execute()
{
  int numPts, i, j;
  vtkAGraymap *newScalars;
  int midILower, midJLower, midIUpper, midJUpper;

  this->Initialize();

  if ( (numPts = this->XSize * this->YSize) < 1 )
    {
    vtkErrorMacro(<<"Bad texture (xsize,ysize) specification!");
    return;
    }

  this->SetDimensions(this->XSize,this->YSize,1);
  newScalars = new vtkAGraymap(numPts);
//
// Compute size of various regions
//
  midILower = (int) ((float)(this->XSize - 1) / 2.0 - this->Thickness / 2.0);
  midJLower = (int) ((float)(this->YSize - 1) / 2.0 - this->Thickness / 2.0);
  midIUpper = (int) ((float)(this->XSize - 1) / 2.0 + this->Thickness / 2.0);
  midJUpper = (int) ((float)(this->YSize - 1) / 2.0 + this->Thickness / 2.0);
//
// Create texture map
//
  for (j = 0; j < this->YSize; j++) 
    {
    for (i = 0; i < this->XSize; i++) 
      {
      if (i < midILower && j < midJLower) 
        newScalars->InsertNextColor(this->InIn);
      else if (i > midIUpper && j < midJLower) 
        newScalars->InsertNextColor(this->OutIn);
      else if (i < midILower && j > midJUpper) 
        newScalars->InsertNextColor(this->InOut);
      else if (i > midIUpper && j > midJUpper) 
        newScalars->InsertNextColor(this->OutOut);
      else if ((i >= midILower && i <= midIUpper) && (j >= midJLower && j <= midJUpper)) 
        newScalars->InsertNextColor(this->OnOn);
      else if ((i >= midILower && i <= midIUpper) && j < midJLower) 
        newScalars->InsertNextColor(this->OnIn);
      else if ((i >= midILower && i <= midIUpper) && j > midJUpper) 
        newScalars->InsertNextColor(this->OnOut);
      else if (i < midILower && (j >= midJLower && j <= midJUpper)) 
        newScalars->InsertNextColor(this->InOn);
      else if (i > midIUpper && (j >= midJLower && j <= midJUpper)) 
        newScalars->InsertNextColor(this->OutOn);
      }
    }

//
// Update ourselves
//
  this->PointData.SetScalars(newScalars);
}

void vtkBooleanTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "In/In: (" << this->InIn[0] << "," << this->InIn[1] << ")\n";
  os << indent << "In/Out: (" << this->InOut[0] << "," << this->InOut[1] << ")\n";
  os << indent << "Out/In: (" << this->OutIn[0] << "," << this->OutIn[1] << ")\n";
  os << indent << "Out/Out: (" << this->OutOut[0] << "," << this->OutOut[1] << ")\n";
  os << indent << "On/On: (" << this->OnOn[0] << "," << this->OnOn[1] << ")\n";
  os << indent << "On/In: (" << this->OnIn[0] << "," << this->OnIn[1] << ")\n";
  os << indent << "On/Out: (" << this->OnOut[0] << "," << this->OnOut[1] << ")\n";
  os << indent << "In/On: (" << this->InOn[0] << "," << this->InOn[1] << ")\n";
  os << indent << "Out/On: (" << this->OutOn[0] << "," << this->OutOn[1] << ")\n";
}

