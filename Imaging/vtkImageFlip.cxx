/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.cxx
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
#include "vtkImageFlip.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageFlip, "1.33");
vtkStandardNewMacro(vtkImageFlip);

//----------------------------------------------------------------------------
vtkImageFlip::vtkImageFlip()
{
  this->PreserveImageExtent = 1;
  this->FlipAboutOrigin = 0;
  this->FilteredAxis = 0;

  if (!this->ResliceAxes)
    {
    this->ResliceAxes = vtkMatrix4x4::New();
    }
}

//----------------------------------------------------------------------------
void vtkImageFlip::ExecuteInformation(vtkImageData *input, 
                                      vtkImageData *output) 
{
  float spacing[3];
  float origin[3];
  int wholeExt[6];
   
  input->GetWholeExtent(wholeExt);
  input->GetSpacing(spacing);
  input->GetOrigin(origin);

  int iflip = this->FilteredAxis;

  // changing the matrix elements directly is ugly, but if the matrix is
  // Modified then the MTime of the filter also changes, which would screw
  // up the pipeline
  if (this->ResliceAxes)
    {
    // set to identity
    for (int i = 0; i < 4; i++)
      {
      for (int j = 0; j < 4; j++)
        {
        this->ResliceAxes->Element[i][j] = 0.0;
        }
      this->ResliceAxes->Element[i][i] = 1.0;
      }
    // but with a iflip along one axis
    this->ResliceAxes->Element[iflip][iflip] = -1.0;
    }

  if (!this->FlipAboutOrigin)
    {
    // set ResliceAxesOrigin so the flip occurs around the correct axis such that
    // the Origin of the output data can be left the same as the Origin of the
    // input data
    if (this->ResliceAxes)
      {
      this->ResliceAxes->Element[iflip][3] = 2*origin[iflip] +
        spacing[iflip]*(wholeExt[2*iflip] + wholeExt[2*iflip+1]);
      }
    }
  else
    {
    // set the output Origin such that when the image flips about its origin
    // (meaning the real origin, not what vtkImageData calls "Origin") the
    // transformed output bounds exactly overlay the input bounds. 
    origin[iflip] = - origin[iflip]
      - spacing[iflip]*(wholeExt[2*iflip] + wholeExt[2*iflip+1]);
    }

  output->SetWholeExtent(wholeExt);
  output->SetSpacing(spacing);
  output->SetOrigin(origin);
  output->SetScalarType(input->GetScalarType());
  output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());
}

//----------------------------------------------------------------------------
void vtkImageFlip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FilteredAxis: " << this->FilteredAxis << "\n";
  os << indent << "FlipAboutOrigin: " << (this->FlipAboutOrigin ? "On\n" : "Off\n");
  os << indent << "PreserveImageExtent: " << (this->PreserveImageExtent ? "On\n" : "Off\n");
}
