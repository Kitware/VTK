/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionToImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitFunctionToImageStencil.h"

#include "vtkImageStencilData.h"
#include "vtkImplicitFunction.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImplicitFunctionToImageStencil);
vtkCxxSetObjectMacro(vtkImplicitFunctionToImageStencil, Input,
                     vtkImplicitFunction);
vtkCxxSetObjectMacro(vtkImplicitFunctionToImageStencil, InformationInput,
                     vtkImageData);

//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil::vtkImplicitFunctionToImageStencil()
{
  this->SetNumberOfInputPorts(0);
  this->Threshold = 0;

  this->Input = NULL;
  // InformationInput is an input used only for its information.
  // The vtkImageStencilSource produces a structured data
  // set with a specific "Spacing" and "Origin", and the
  // InformationInput is a source of this information.
  this->InformationInput = NULL;

  // If no InformationInput is set, then the Spacing and Origin
  // for the output must be set directly.
  this->OutputOrigin[0] = 0;
  this->OutputOrigin[1] = 0;
  this->OutputOrigin[2] = 0;
  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  // The default output extent is essentially infinite, which allows
  // this filter to produce any requested size.  This would not be a
  // great source to connect to some sort of writer or viewer, it
  // should only be connected to multiple-input filters that take
  // compute their output extent from one of the other inputs.
  this->OutputWholeExtent[0] = 0;
  this->OutputWholeExtent[1] = VTK_LARGE_INTEGER >> 2;
  this->OutputWholeExtent[2] = 0;
  this->OutputWholeExtent[3] = VTK_LARGE_INTEGER >> 2;
  this->OutputWholeExtent[4] = 0;
  this->OutputWholeExtent[5] = VTK_LARGE_INTEGER >> 2;
}

//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil::~vtkImplicitFunctionToImageStencil()
{
  this->SetInformationInput(NULL);
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkImplicitFunctionToImageStencil::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

    os << indent << "InformationInput: " << this->InformationInput << "\n";
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputWholeExtent: " << this->OutputWholeExtent[0] << " " <<
    this->OutputWholeExtent[1] << " " << this->OutputWholeExtent[2] << " " <<
    this->OutputWholeExtent[3] << " " << this->OutputWholeExtent[4] << " " <<
    this->OutputWholeExtent[5] << "\n";
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
}

//----------------------------------------------------------------------------
int vtkImplicitFunctionToImageStencil::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  int wholeExtent[6];
  double spacing[3];
  double origin[3];

  for (int i = 0; i < 3; i++)
    {
    wholeExtent[2*i] = this->OutputWholeExtent[2*i];
    wholeExtent[2*i+1] = this->OutputWholeExtent[2*i+1];
    spacing[i] = this->OutputSpacing[i];
    origin[i] = this->OutputOrigin[i];
    }

  // If InformationInput is set, then get the spacing,
  // origin, and whole extent from it.
  if (this->InformationInput)
    {
    this->InformationInput->UpdateInformation();
    this->InformationInput->GetWholeExtent(wholeExtent);
    this->InformationInput->GetSpacing(spacing);
    this->InformationInput->GetOrigin(origin);
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  return 1;
}

//----------------------------------------------------------------------------
// set up the clipping extents from an implicit function by brute force
// (i.e. by evaluating the function at each and every voxel)
int vtkImplicitFunctionToImageStencil::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::RequestData(request, inputVector, outputVector);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImplicitFunction *function = this->Input;
  double *spacing = data->GetSpacing();
  double *origin = data->GetOrigin();
  double threshold = this->Threshold;

  // if the input is not set then punt
  if (!function)
    {
    return 1;
    }

  // for conversion of (idX,idY,idZ) into (x,y,z)
  double point[3];

  // for keeping track of progress
  unsigned long count = 0;
  int extent[6];
  data->GetExtent(extent);
  unsigned long target = static_cast<unsigned long>(
    (extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)/50.0);
  target++;

  // loop through all voxels
  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
    {
    point[2] = idZ*spacing[2] + origin[2];

    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      point[1] = idY*spacing[1] + origin[1];
      int state = 1; // inside or outside, start outside
      int r1 = extent[0];
      int r2 = extent[1];

      if (count%target == 0) 
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;

      for (int idX = extent[0]; idX <= extent[1]; idX++)
        {
        point[0] = idX*spacing[0] + origin[0];
        int newstate = 1;
        if (function->FunctionValue(point) < threshold)
          {
          newstate = -1;
          if (newstate != state)
            { // sub extent starts
            r1 = idX;
            }
          }
        else if (newstate != state)
          { // sub extent ends
          r2 = idX - 1;
          data->InsertNextExtent(r1, r2, idY, idZ);
          }
        state = newstate;
        } // for idX
      if (state == -1)
        { // if inside at end, cap off the sub extent
        data->InsertNextExtent(r1, extent[1], idY, idZ);
        }
      } // for idY    
    } // for idZ

  return 1;
}
