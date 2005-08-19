/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageGradient.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageGradient, "1.54");
vtkStandardNewMacro(vtkImageGradient);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageGradient fitler.
vtkImageGradient::vtkImageGradient()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->HandleBoundaries = 0;
  this->HandleBoundariesOn();
  this->Dimensionality = 2;
}


//----------------------------------------------------------------------------
void vtkImageGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HandleBoundaries: " << this->HandleBoundaries << "\n";
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}


//----------------------------------------------------------------------------
int vtkImageGradient::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int extent[6];
  int idx;

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // invalid setting, it has not been set, so default to whole Extent
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
              extent);

  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->Dimensionality; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2 + 1] -= 1;
      }
    }
  
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
               extent, 6);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, this->Dimensionality);
  return 1;
}

//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
int vtkImageGradient::RequestUpdateExtent (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int wholeExtent[6];
  int idx;

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // invalid setting, it has not been set, so default to whole Extent
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
              wholeExtent);
  int inUExt[6]; 
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inUExt);

  // grow input whole extent.
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    inUExt[idx*2] -= 1;
    inUExt[idx*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with whole extent is we hanlde boundaries.
      if (inUExt[idx*2] < wholeExtent[idx*2])
        {
        inUExt[idx*2] = wholeExtent[idx*2];
        }
      if (inUExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
        {
        inUExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
        }
      }
    }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inUExt, 6);  

  return 1;
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageGradientExecute(vtkImageGradient *self,
                             vtkImageData *inData, T *inPtr,
                             vtkImageData *outData, double *outPtr,
                             int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int axesNum;
  int *inExt = inData->GetExtent();
  int *wholeExtent;
  vtkIdType *inIncs;
  double r[3], d;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // The data spacing is important for computing the gradient.
  // central differences (2 * ratio).
  // Negative because below we have (min - max) for dx ...
  inData->GetSpacing(r);
  r[0] = -0.5 / r[0];
  r[1] = -0.5 / r[1];
  r[2] = -0.5 / r[2];

  // get some other info we need
  inIncs = inData->GetIncrements(); 
  wholeExtent = inData->GetExtent(); 

  // Move the pointer to the correct starting position.
  inPtr += (outExt[0]-inExt[0])*inIncs[0] + 
           (outExt[2]-inExt[2])*inIncs[1] + 
           (outExt[4]-inExt[4])*inIncs[2]; 

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useZMin = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useZMax = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      useYMin = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useYMax = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        useXMin = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
        useXMax = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];

        // do X axis
        d = (double)(inPtr[useXMin]);
        d -= (double)(inPtr[useXMax]);
        d *= r[0]; // multiply by the data spacing
        *outPtr = d;
        outPtr++;
        
        // do y axis
        d = (double)(inPtr[useYMin]);
        d -= (double)(inPtr[useYMax]);
        d *= r[1]; // multiply by the data spacing
        *outPtr = d;
        outPtr++;
        if (axesNum == 3)
          {
          // do z axis
          d = (double)(inPtr[useZMin]);
          d -= (double)(inPtr[useZMax]);
          d *= r[2]; // multiply by the data spacing
          *outPtr = d;
          outPtr++;
          }
        inPtr++;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageGradient::ThreadedExecute (vtkImageData *inData, 
                                       vtkImageData *outData,
                                       int outExt[6], int id)
{
  void *inPtr;
  double *outPtr = (double *)(outData->GetScalarPointerForExtent(outExt));
  inPtr = inData->GetScalarPointer();

  // this filter expects that input is the same type as output.
  if (outData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, " << outData->GetScalarType()
                  << ", must be double\n");
    return;
    }
  
  if (inData->GetNumberOfScalarComponents() != 1)
    {
    vtkErrorMacro(<< "Execute: input has more than one components. The input to gradient should be a single component image. Think about it. If you insist on using a color image then run it though RGBToHSV then ExtractComponents to get the V components. That's probably what you want anyhow.");
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageGradientExecute(this, inData, 
                              (VTK_TT *)(inPtr), outData, outPtr, 
                              outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}




