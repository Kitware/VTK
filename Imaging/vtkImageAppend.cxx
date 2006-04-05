/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAppend.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkImageAppend, "1.31");
vtkStandardNewMacro(vtkImageAppend);

//----------------------------------------------------------------------------
vtkImageAppend::vtkImageAppend()
{
  this->AppendAxis = 0;
  this->Shifts = NULL;
  this->PreserveExtents = 0;
}

//----------------------------------------------------------------------------
vtkImageAppend::~vtkImageAppend()
{
  if (this->Shifts != NULL)
    {
    delete [] this->Shifts;
    }
}

//----------------------------------------------------------------------------
// The default vtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void vtkImageAppend::SetInput(int idx, vtkDataObject *input)
{
  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, idx, (input ? input->GetProducerPort() : 0));
}

//----------------------------------------------------------------------------
vtkDataObject *vtkImageAppend::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// This method tells the ouput it will have more components
int vtkImageAppend::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo;

  int idx;
  int min, max, size, tmp;
  int *inExt, outExt[6];
  int unionExt[6];

  // Initialize the union.
  unionExt[0] = unionExt[2] = unionExt[4] = VTK_LARGE_INTEGER;
  unionExt[1] = unionExt[3] = unionExt[5] = -VTK_LARGE_INTEGER;

  // Initialize the shifts.
  if (this->Shifts)
    {
    delete [] this->Shifts;
    }
  this->Shifts = new int [this->GetNumberOfInputConnections(0)];
  
  // Find the outMin/max of the appended axis for this input.
  inInfo = inputVector[0]->GetInformationObject(0);
  inExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  min = tmp = inExt[this->AppendAxis * 2];
  for (idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
    {
    inInfo = inputVector[0]->GetInformationObject(idx);
    inExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    
    if (this->PreserveExtents)
      {
      // Compute union for preseving extents.
      if (inExt[0] < unionExt[0])
        {
        unionExt[0] = inExt[0];
        }
      if (inExt[1] > unionExt[1])
        {
        unionExt[1] = inExt[1];
        }
      if (inExt[2] < unionExt[2])
        {
        unionExt[2] = inExt[2];
        }
      if (inExt[3] > unionExt[3])
        {
        unionExt[3] = inExt[3];
        }
      if (inExt[4] < unionExt[4])
        {
        unionExt[4] = inExt[4];
        }
      if (inExt[5] > unionExt[5])
        {
        unionExt[5] = inExt[5];
        }
      this->Shifts[idx] = 0;
      }
    else
      {
      // Compute shifts if we are not preserving extents.
      this->Shifts[idx] = tmp - inExt[this->AppendAxis*2];
      size = inExt[this->AppendAxis*2 + 1] - inExt[this->AppendAxis*2] + 1;
      tmp += size;
      }
    }
  
  if (this->PreserveExtents)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),unionExt,6);
    }
  else
    {
    inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),outExt);
    max = tmp - 1;
    outExt[this->AppendAxis*2] = min;
    outExt[this->AppendAxis*2 + 1] = max;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),outExt,6);
    }

  return 1;
}

void vtkImageAppend::InternalComputeInputUpdateExtent(
  int *inExt, int *outExt, int *inWextent, int whichInput)
{
  int min, max, shift, tmp, idx;

  // default input extent will be that of output extent
  memcpy(inExt,outExt,sizeof(int)*6);

  shift = 0;
  if ( ! this->PreserveExtents)
    {
    shift = this->Shifts[whichInput];
    }
  min = inWextent[this->AppendAxis*2] + shift;
  max = inWextent[this->AppendAxis*2 + 1] + shift;
  
  // now clip the outExtent against the outExtent for this input (intersect)
  tmp = outExt[this->AppendAxis*2];
  if (min < tmp) 
    {
    min = tmp;
    }
  tmp = outExt[this->AppendAxis*2 + 1];
  if (max > tmp) 
    {
    max = tmp;
    }
  
  // now if min > max, we do not need the input at all.  I assume
  // the pipeline will interpret this extent this way.
  
  // convert back into input coordinates.
  inExt[this->AppendAxis*2] = min - shift;
  inExt[this->AppendAxis*2 + 1] = max - shift;
  
  // for robustness (in the execute method), 
  // do not ask for more than the whole extent of the other axes.
  for (idx = 0; idx < 3; ++idx)
    {
    if (inExt[idx*2] < inWextent[idx*2])
      {
      inExt[idx*2] = inWextent[idx*2];
      }
    if (inExt[idx*2 + 1] > inWextent[idx*2 + 1])
      {
      inExt[idx*2 + 1] = inWextent[idx*2 + 1];
      }
    }
}

//----------------------------------------------------------------------------
int vtkImageAppend::RequestUpdateExtent(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  
  // default input extent will be that of output extent
  int inExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt);
  int *outExt = 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  int whichInput;
  for (whichInput = 0; whichInput < this->GetNumberOfInputConnections(0); 
       whichInput++)
    {
    int *inWextent;
    
    // Find the outMin/max of the appended axis for this input.
    inInfo = inputVector[0]->GetInformationObject(whichInput);
    inWextent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    this->InternalComputeInputUpdateExtent(inExt, outExt, 
                                           inWextent, whichInput);
    
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);
    }

  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageAppendExecute(vtkImageAppend *self, int id, 
                           int inExt[6], vtkImageData *inData, T *inPtr,
                           int outExt[6], vtkImageData *outData, T *outPtr)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;

  // Get increments to march through data 
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // find the region to loop over
  rowLength = (inExt[1] - inExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = inExt[3] - inExt[2]; 
  maxZ = inExt[5] - inExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  

  // Loop through input pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
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
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
        *outPtr = *inPtr;
        outPtr++;
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
void vtkImageAppend::InitOutput(int outExt[6], vtkImageData *outData)
{
  int idxY, idxZ;
  int maxY, maxZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  int typeSize;
  unsigned char *outPtrZ, *outPtrY;
  

  typeSize = outData->GetScalarSize();  
  outPtrZ = (unsigned char *)(outData->GetScalarPointerForExtent(outExt));

  // Get increments to march through data 
  outData->GetIncrements(outIncX, outIncY, outIncZ);
  outIncX *= typeSize;
  outIncY *= typeSize;
  outIncZ *= typeSize;

  // Find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*outData->GetNumberOfScalarComponents();
  rowLength *= typeSize;
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];

  // Loop through input pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    outPtrY = outPtrZ;
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memset(outPtrY, 0, rowLength);
      outPtrY += outIncY;
      }
    outPtrZ += outIncZ;
    }
}
//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppend::ThreadedRequestData (  
  vtkInformation * vtkNotUsed( request ), 
  vtkInformationVector** inputVector, 
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData, 
  vtkImageData **outData,
  int outExt[6], int id)
{
  int idx1;
  int inExt[6], cOutExt[6];
  void *inPtr;
  void *outPtr;
  
  this->InitOutput(outExt, outData[0]);

  for (idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
    {
    if (inData[0][idx1] != NULL)
      {
      // Get the input extent and output extent
      // the real out extent for this input may be clipped.
      vtkInformation *inInfo = 
        inputVector[0]->GetInformationObject(idx1);
      int *inWextent = 
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      this->InternalComputeInputUpdateExtent(inExt, outExt, inWextent, idx1);
      memcpy(cOutExt, inExt, 6*sizeof(int));
      cOutExt[this->AppendAxis*2] = 
        inExt[this->AppendAxis*2] + this->Shifts[idx1];
      cOutExt[this->AppendAxis*2 + 1] = 
        inExt[this->AppendAxis*2 + 1] + this->Shifts[idx1];
      
      // do a quick check to see if the input is used at all.
      if (inExt[this->AppendAxis*2] <= inExt[this->AppendAxis*2 + 1])
        {
        inPtr = inData[0][idx1]->GetScalarPointerForExtent(inExt);
        outPtr = outData[0]->GetScalarPointerForExtent(cOutExt);
        
        if (inData[0][idx1]->GetNumberOfScalarComponents() !=
            outData[0]->GetNumberOfScalarComponents())
          {
          vtkErrorMacro("Components of the inputs do not match");
          return;
          }
        
        // this filter expects that input is the same type as output.
        if (inData[0][idx1]->GetScalarType() != outData[0]->GetScalarType())
          {
          vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" 
                        << inData[0][idx1]->GetScalarType() 
                        << "), must match output ScalarType (" 
                        << outData[0]->GetScalarType() << ")");
          return;
          }
        
        switch (inData[0][idx1]->GetScalarType())
          {
          vtkTemplateMacro(
            vtkImageAppendExecute(this, id, 
                                  inExt, inData[0][idx1], (VTK_TT *)(inPtr),
                                  cOutExt, outData[0], (VTK_TT *)(outPtr)));
          default:
            vtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
          }
        }
      }
    }
}


//----------------------------------------------------------------------------
int vtkImageAppend::FillInputPortInformation(int i, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i,info);
}

//----------------------------------------------------------------------------
void vtkImageAppend::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AppendAxis: " << this->AppendAxis << endl;
  os << indent << "PreserveExtents: " << this->PreserveExtents << endl;
}
