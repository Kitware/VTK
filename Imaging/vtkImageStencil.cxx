/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStencil.h"

#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageStencil);

//----------------------------------------------------------------------------
vtkImageStencil::vtkImageStencil()
{
  this->ReverseStencil = 0;

  this->BackgroundColor[0] = 1;
  this->BackgroundColor[1] = 1;
  this->BackgroundColor[2] = 1;
  this->BackgroundColor[3] = 1;
  this->SetNumberOfInputPorts(3);
}

//----------------------------------------------------------------------------
vtkImageStencil::~vtkImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetStencil(vtkImageStencilData *stencil)
{
  this->SetInput(2, stencil); 
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageStencil::GetStencil()
{
  if (this->GetNumberOfInputConnections(2) < 1)
    {
    return NULL;
    }
  else
    {
    return vtkImageStencilData::SafeDownCast(
      this->GetExecutive()->GetInputData(2, 0));
    }
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetBackgroundInput(vtkImageData *data)
{
  this->SetInput(1, data); 
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageStencil::GetBackgroundInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  else
    {
    return vtkImageData::SafeDownCast(
      this->GetExecutive()->GetInputData(1, 0));
    }
}

//----------------------------------------------------------------------------
int vtkImageStencil::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // need to set the spacing and origin of the stencil to match the output
  vtkImageStencilData *stencil = this->GetStencil();
  if (stencil)
    {
    stencil->SetSpacing(inInfo->Get(vtkDataObject::SPACING()));
    stencil->SetOrigin(inInfo->Get(vtkDataObject::ORIGIN()));
    }

  return 1;
}

//----------------------------------------------------------------------------
// Some helper functions for 'ThreadedRequestData'
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// copy a pixel, advance the output pointer but not the input pointer

template<class T>
inline void vtkCopyPixel(T *&out, const T *in, int numscalars)
{
  do
    {
    *out++ = *in++;
    }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Convert background color from double to appropriate type

template <class T>
void vtkAllocBackground(vtkImageStencil *self, T *&background,
                        vtkInformation *outInfo)
{
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int numComponents = output->GetNumberOfScalarComponents();
  int scalarType = output->GetScalarType();

  background = new T[numComponents];

  for (int i = 0; i < numComponents; i++)
    {
    if (i < 4)
      {
      if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
        {
        background[i] = static_cast<T>(self->GetBackgroundColor()[i]);
        }
      else
        { // round float to nearest int
        background[i] =
          static_cast<T>(floor(self->GetBackgroundColor()[i] + 0.5));
        }
      }
    else
      { // all values past 4 are set to zero
      background[i] = 0;
      }
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkFreeBackground(vtkImageStencil *vtkNotUsed(self), T *&background)
{
  delete [] background;
  background = NULL;
}

//----------------------------------------------------------------------------
template <class T>
void vtkImageStencilExecute(vtkImageStencil *self,
                            vtkImageData *inData, T *inPtr,
                            vtkImageData *in2Data,T *in2Ptr,
                            vtkImageData *outData, T *outPtr,
                            int outExt[6], int id,
                            vtkInformation *outInfo)
{
  int numscalars, inIncX;
  int idX, idY, idZ;
  int r1, r2, cr1, cr2, iter, rval;
  vtkIdType outIncX, outIncY, outIncZ;
  int inExt[6];
  vtkIdType inInc[3];
  int in2Ext[6];
  vtkIdType in2Inc[3];
  unsigned long count = 0;
  unsigned long target;
  T *background, *tempPtr;

  // get the clipping extents
  vtkImageStencilData *stencil = self->GetStencil();

  // find maximum input range
  inData->GetExtent(inExt);
  inData->GetIncrements(inInc);
  if (in2Data)
    {
    in2Data->GetExtent(in2Ext);
    in2Data->GetIncrements(in2Inc);
    }

  // Get Increments to march through data 
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();

  target = static_cast<unsigned long>(
    (outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;  
  
  // set color for area outside of input volume extent
  vtkAllocBackground(self, background, outInfo);

  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (!id) 
        {
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      iter = 0;
      if (self->GetReverseStencil())
        { // flag that we want the complementary extents
        iter = -1;
        }

      cr1 = outExt[0];
      for (;;)
        {
        rval = 0;
        r1 = outExt[1] + 1;
        r2 = outExt[1];
        if (stencil)
          {
          rval = stencil->GetNextExtent(r1, r2, outExt[0], outExt[1],
                                        idY, idZ, iter);
          }
        else if (iter < 0)
          {
          r1 = outExt[0];
          r2 = outExt[1];
          rval = 1;
          iter = 1;
          }

        tempPtr = background;
        inIncX = 0; 
        if (in2Ptr)
          {
          tempPtr = in2Ptr + (in2Inc[2]*(idZ - in2Ext[4]) +
                              in2Inc[1]*(idY - in2Ext[2]) +
                              numscalars*(cr1 - in2Ext[0]));
          inIncX = numscalars;
          }

        cr2 = r1 - 1;
        for (idX = cr1; idX <= cr2; idX++)
          {
          vtkCopyPixel(outPtr, tempPtr, numscalars);
          tempPtr += inIncX;
          }
        cr1 = r2 + 1; // for next time 'round

        // break if no foreground extents left
        if (rval == 0)
          {
          break;
          }

        tempPtr = inPtr + (inInc[2]*(idZ - inExt[4]) +
                           inInc[1]*(idY - inExt[2]) +
                           numscalars*(r1 - inExt[0]));

        for (idX = r1; idX <= r2; idX++)
          {
          vtkCopyPixel(outPtr, tempPtr, numscalars);
          tempPtr += numscalars;
          }
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  vtkFreeBackground(self, background);
}

//----------------------------------------------------------------------------
void vtkImageStencil::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector,
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr, *inPtr2;
  void *outPtr;
  vtkImageData *inData2 = this->GetBackgroundInput();
  
  inPtr = inData[0][0]->GetScalarPointer();
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inPtr2 = NULL;
  if (inData2)
    {
    inPtr2 = inData2->GetScalarPointer();
    if (inData2->GetScalarType() != inData[0][0]->GetScalarType())
      {
      if (id == 0)
        {
        vtkErrorMacro("Execute: BackgroundInput ScalarType " 
                      << inData2->GetScalarType()
                      << ", must match Input ScalarType "
                      << inData[0][0]->GetScalarType());
        }
      return;
      }
    else if (inData2->GetNumberOfScalarComponents() 
             != inData[0][0]->GetNumberOfScalarComponents())
      {
      if (id == 0)
        {
        vtkErrorMacro("Execute: BackgroundInput NumberOfScalarComponents " 
                      << inData2->GetNumberOfScalarComponents()
                      << ", must match Input NumberOfScalarComponents "
                      << inData[0][0]->GetNumberOfScalarComponents());
        }
      return;
      }
    int wholeExt1[6], wholeExt2[6];
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *inInfo2 = inputVector[1]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt1);
    inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt2);

    for (int i = 0; i < 6; i++)
      {
      if (wholeExt1[i] != wholeExt2[i])
        {
        if (id == 0)
          {
          vtkErrorMacro("Execute: BackgroundInput must have the same "
                        "WholeExtent as the Input");
          }
        return;
        }
      }
    }
  
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageStencilExecute(this, 
                             inData[0][0],
                             static_cast<VTK_TT *>(inPtr), 
                             inData2, 
                             static_cast<VTK_TT *>(inPtr2), 
                             outData[0], 
                             static_cast<VTK_TT *>(outPtr), 
                             outExt, 
                             id, 
                             outInfo));
    default:
      vtkErrorMacro("Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
int vtkImageStencil::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 2)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    if (port == 1)
      {
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
      }
    }
  return 1;
}

void vtkImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ?
                                         "On\n" : "Off\n");

  os << indent << "BackgroundInput: " << this->GetBackgroundInput() << "\n";
  os << indent << "BackgroundValue: " << this->BackgroundColor[0] << "\n";

  os << indent << "BackgroundColor: (" << this->BackgroundColor[0] << ", "
                                    << this->BackgroundColor[1] << ", "
                                    << this->BackgroundColor[2] << ", "
                                    << this->BackgroundColor[3] << ")\n";
}
