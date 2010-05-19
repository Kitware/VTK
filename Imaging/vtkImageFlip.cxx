/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageFlip.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSetAttributes.h"

vtkStandardNewMacro(vtkImageFlip);

//----------------------------------------------------------------------------
vtkImageFlip::vtkImageFlip()
{
  this->PreserveImageExtent = 1;
  this->FlipAboutOrigin = 0;
  this->FilteredAxis = 0;

  if (!this->ResliceAxes)
    {
    // for consistent register/unregister
    this->SetResliceAxes(vtkMatrix4x4::New());
    this->ResliceAxes->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkImageFlip::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  double spacing[3];
  double origin[3];
  int wholeExt[6];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  inInfo->Get(vtkDataObject::ORIGIN(), origin);

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

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  // This information already copied from input to output in CopyInformationToPipeline?
  vtkInformation *inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo, 
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!inScalarInfo)
    {
    vtkErrorMacro("Missing scalar field on input information!");
    return 0;
    }
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, 
    inScalarInfo->Get( vtkDataObject::FIELD_ARRAY_TYPE() ),
    inScalarInfo->Get( vtkDataObject::FIELD_NUMBER_OF_COMPONENTS() ) );
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageFlip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FilteredAxis: " << this->FilteredAxis << "\n";
  os << indent << "FlipAboutOrigin: " << (this->FlipAboutOrigin ? "On\n" : "Off\n");
  os << indent << "PreserveImageExtent: " << (this->PreserveImageExtent ? "On\n" : "Off\n");
}
