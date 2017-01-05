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
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>
#include <sstream>

vtkStandardNewMacro(vtkImageGradient);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageGradient fitler.
vtkImageGradient::vtkImageGradient()
{
  this->HandleBoundaries = 1;
  this->Dimensionality = 2;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
void vtkImageGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HandleBoundaries: " << this->HandleBoundaries << "\n";
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

//----------------------------------------------------------------------------
int vtkImageGradient::RequestInformation(vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // Get input and output pipeline information.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // Get the input whole extent.
  int extent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // Shrink output image extent by one pixel if not handling boundaries.
  if(!this->HandleBoundaries)
  {
    for(int idx = 0; idx < this->Dimensionality; ++idx)
    {
      extent[idx*2] += 1;
      extent[idx*2 + 1] -= 1;
    }
  }

  // Store the new whole extent for the output.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  // Set the number of point data componets to the number of
  // components in the gradient vector.
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE,
                                              this->Dimensionality);

  return 1;
}

//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
int vtkImageGradient::RequestUpdateExtent(vtkInformation*,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  // Get input and output pipeline information.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // We need one extra ghost level
  int ugl = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ugl + 1);

  // Get the input whole extent.
  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  // Get the requested update extent from the output.
  int inUExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inUExt);

  // In order to do central differencing we need one more layer of
  // input pixels than we are producing output pixels.
  for(int idx = 0; idx < this->Dimensionality; ++idx)
  {
    inUExt[idx*2] -= 1;
    inUExt[idx*2+1] += 1;

    // If handling boundaries instead of shrinking the image then we
    // must clip the needed extent within the whole extent of the
    // input.
    if (this->HandleBoundaries)
    {
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

  // Store the update extent needed from the intput.
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
  vtkIdType inIncs[3];
  double r[3], d;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;

  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
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
  inData->GetIncrements(inIncs);
  wholeExtent = inData->GetExtent();

  // Move the pointer to the correct starting position.
  inPtr += (outExt[0]-inExt[0])*inIncs[0] +
           (outExt[2]-inExt[2])*inIncs[1] +
           (outExt[4]-inExt[4])*inIncs[2];

  // Loop through output pixels
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
        d = static_cast<double>(inPtr[useXMin]);
        d -= static_cast<double>(inPtr[useXMax]);
        d *= r[0]; // multiply by the data spacing
        *outPtr = d;
        outPtr++;

        // do y axis
        d = static_cast<double>(inPtr[useYMin]);
        d -= static_cast<double>(inPtr[useYMax]);
        d *= r[1]; // multiply by the data spacing
        *outPtr = d;
        outPtr++;
        if (axesNum == 3)
        {
          // do z axis
          d = static_cast<double>(inPtr[useZMin]);
          d -= static_cast<double>(inPtr[useZMax]);
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

int vtkImageGradient::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Shrink the update extent to the input extent. Input extent
  // can be smaller than update extent when there is an piece
  // request (UPDATE_NUMBER_OF_PIECES() > 1).
  // Since the superclass and this class make decisions based
  // on UPDATE_EXTENT(), this is the quickest way of making this
  // filter in distributed parallel mode.
  // In the future, this logic should move up the hierarchy so
  // other imaging classes can benefit from it.
  vtkImageData* input = vtkImageData::GetData(inputVector[0]);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int ue[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ue);
  int ue2[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ue2);
  int* ie =  input->GetExtent();
  for (int i=0; i<3; i++)
  {
    if (ue[2*i] < ie[2*i])
    {
      ue2[2*i] = ie[2*i];
    }
    if (ue[2*i+1] > ie[2*i+1])
    {
      ue2[2*i+1] = ie[2*i+1];
    }
  }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ue2, 6);

  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }
  vtkImageData* output = vtkImageData::GetData(outputVector);
  vtkDataArray* outArray = output->GetPointData()->GetScalars();
  std::ostringstream newname;
  newname << (outArray->GetName()?outArray->GetName():"")
    << "Gradient";
  outArray->SetName(newname.str().c_str());
  // Why not pass the original array?
  if (this->GetInputArrayToProcess(0, inputVector))
  {
    output->GetPointData()->AddArray(
        this->GetInputArrayToProcess(0, inputVector));
  }
  // Restore the previous update extent. See code above for details.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ue, 6);
  return 1;
}

//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  This method does handle
// boundary conditions.
void vtkImageGradient::ThreadedRequestData(vtkInformation*,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector*,
                                           vtkImageData*** inData,
                                           vtkImageData** outData,
                                           int outExt[6],
                                           int threadId)
{
  // Get the input and output data objects.
  vtkImageData* input = inData[0][0];
  vtkImageData* output = outData[0];

  // The ouptut scalar type must be double to store proper gradients.
  if(output->GetScalarType() != VTK_DOUBLE)
  {
    vtkErrorMacro("Execute: output ScalarType is "
                  << output->GetScalarType() << "but must be double.");
    return;
  }

  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (!inputArray)
  {
    vtkErrorMacro("No input array was found. Cannot execute");
    return;
  }

  // Gradient makes sense only with one input component.  This is not
  // a Jacobian filter.
  if(inputArray->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(
      "Execute: input has more than one component. "
      "The input to gradient should be a single component image. "
      "Think about it. If you insist on using a color image then "
      "run it though RGBToHSV then ExtractComponents to get the V "
      "components. That's probably what you want anyhow.");
    return;
  }

  void* inPtr = inputArray->GetVoidPointer(0);
  double* outPtr = static_cast<double *>(
    output->GetScalarPointerForExtent(outExt));
  switch(inputArray->GetDataType())
  {
    vtkTemplateMacro(
      vtkImageGradientExecute(this, input, static_cast<VTK_TT*>(inPtr),
                              output, outPtr, outExt, threadId)
      );
    default:
      vtkErrorMacro("Execute: Unknown ScalarType " << input->GetScalarType());
      return;
  }
}
