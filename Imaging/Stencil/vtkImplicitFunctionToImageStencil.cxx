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

//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil::vtkImplicitFunctionToImageStencil()
{
  this->SetNumberOfInputPorts(0);
  this->Threshold = 0;

  this->Input = NULL;
}

//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil::~vtkImplicitFunctionToImageStencil()
{
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkImplicitFunctionToImageStencil::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
}

//----------------------------------------------------------------------------
unsigned long vtkImplicitFunctionToImageStencil::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();

  if ( this->Input != NULL )
    {
    unsigned long nTime = this->Input->GetMTime();
    mTime = ( nTime > mTime ? nTime : mTime );
    }

  return mTime;
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
