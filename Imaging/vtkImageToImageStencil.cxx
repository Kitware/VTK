/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageStencil.cxx
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
#include "vtkImageToImageStencil.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageToImageStencil, "1.7");
vtkStandardNewMacro(vtkImageToImageStencil);

//----------------------------------------------------------------------------
vtkImageToImageStencil::vtkImageToImageStencil()
{
  this->UpperThreshold = VTK_LARGE_FLOAT;
  this->LowerThreshold = -VTK_LARGE_FLOAT;
}

//----------------------------------------------------------------------------
vtkImageToImageStencil::~vtkImageToImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToImageStencil::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void vtkImageToImageStencil::ThresholdByUpper(float thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < VTK_LARGE_FLOAT)
    {
    this->LowerThreshold = thresh;
    this->UpperThreshold = VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void vtkImageToImageStencil::ThresholdByLower(float thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -VTK_LARGE_FLOAT)
    {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void vtkImageToImageStencil::ThresholdBetween(float lower, float upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::ThreadedExecute(vtkImageStencilData *data,
                                             int outExt[6], int id)
{
  vtkImageData *inData = this->GetInput();
  if (!inData)
    {
    return;
    }

  int *inExt = inData->GetExtent();
  int *inWholeExt = inData->GetWholeExtent();
  vtkDataArray *inScalars = inData->GetPointData()->GetScalars();
  float upperThreshold = this->UpperThreshold;
  float lowerThreshold = this->LowerThreshold;

  // clip the extent with the image data extent
  int extent[6];
  for (int i = 0; i < 3; i++)
    {
    int lo = 2*i;
    extent[lo] = outExt[lo];
    if (extent[lo] < inWholeExt[lo])
      {
      extent[lo] = inWholeExt[lo];
      }
    int hi = 2*i + 1;
    extent[hi] = outExt[hi];
    if (extent[hi] > inWholeExt[hi])
      {
      extent[hi] = inWholeExt[hi];
      }
    if (extent[lo] > extent[hi])
      {
      return;
      }
    }

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = (unsigned long)
    ((extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)/50.0);
  target++;

  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
    {
    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      if (id == 0)
        { // update progress if we're the main thread
        if (count%target == 0) 
          {
          this->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      int state = 1; // inside or outside, start outside
      int r1 = extent[0];
      int r2 = extent[1];

      // index into scalar array
      int idS = ((inExt[1] - inExt[0] + 1)*
                 ((inExt[3] - inExt[2] + 1)*(idZ - inExt[4]) +
                  (idY - inExt[2])) + (extent[0] - inExt[0]));

      for (int idX = extent[0]; idX <= extent[1]; idX++)
        {
        int newstate = 1;
        float value = inScalars->GetComponent(idS++,0);
        if (value >= lowerThreshold && value <= upperThreshold)
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
      if (state < 0)
        { // if inside at end, cap off the sub extent
        data->InsertNextExtent(r1, extent[1], idY, idZ);
        }
      } // for idY
    } // for idZ
}
