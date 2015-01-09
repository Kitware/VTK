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

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"


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
  delete [] this->Shifts;
}

//----------------------------------------------------------------------------
void vtkImageAppend::ReplaceNthInputConnection(int idx,
                                               vtkAlgorithmOutput *input)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(0))
    {
    vtkErrorMacro("Attempt to replace connection idx " << idx
                  << " of input port " << 0 << ", which has only "
                  << this->GetNumberOfInputConnections(0)
                  << " connections.");
    return;
    }

  if (!input || !input->GetProducer())
    {
    vtkErrorMacro("Attempt to replace connection index " << idx
                  << " for input port " << 0 << " with " <<
                  (!input ? "a null input." : "an input with no producer."));
    return;
    }

  this->SetNthInputConnection(0, idx, input);
}

//----------------------------------------------------------------------------
// The default vtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void vtkImageAppend::SetInputData(int idx, vtkDataObject *input)
{
  this->SetInputDataInternal(idx, input);
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
// This method tells the output it will have more components
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
  unionExt[0] = unionExt[2] = unionExt[4] = VTK_INT_MAX;
  unionExt[1] = unionExt[3] = unionExt[5] = -VTK_INT_MAX;

  // Initialize the shifts.
  delete [] this->Shifts;
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
      // Compute union for preserving extents.
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

//----------------------------------------------------------------------------
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
  // get the outInfo object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // default input extent will be that of output extent
  int inExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt);
  int *outExt =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  for (int whichInput = 0; whichInput < this->GetNumberOfInputConnections(0);
       whichInput++)
    {
    int *inWextent;

    // Find the outMin/max of the appended axis for this input.
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(whichInput);
    inWextent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    this->InternalComputeInputUpdateExtent(inExt, outExt,
                                           inWextent, whichInput);

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);
    }

  return 1;
}

//----------------------------------------------------------------------------
static void vtkImageAppendGetContinuousIncrements
  (int wExtent[6], int sExtent[6], vtkIdType nComp, bool forCells,
   vtkIdType &incX,
   vtkIdType &incY,
   vtkIdType &incZ)
{
  //I can not use the one in vtkImageData, since that assumes point scalars
  //and I need it to work for any point or cell array

  int e0, e1, e2, e3;
  incX = 0;
  e0 = sExtent[0];
  if (e0 < wExtent[0])
    {
    e0 = wExtent[0];
    }
  e1 = sExtent[1];
  if (e1 > wExtent[1])
    {
    e1 = wExtent[1];
    }
  e2 = sExtent[2];
  if (e2 < wExtent[2])
    {
    e2 = wExtent[2];
    }
  e3 = sExtent[3];
  if (e3 > wExtent[3])
    {
    e3 = wExtent[3];
    }

  int ptAdjust = (forCells?0:1);
  int idx;
  vtkIdType increments[3];
  int wholeJump;
  for (idx = 0; idx < 3; ++idx)
    {
    increments[idx] = nComp;
    wholeJump = wExtent[idx*2+1] - wExtent[idx*2] + ptAdjust;
    if (wholeJump == 0)
      {
      wholeJump = 1;
      }
    nComp *= wholeJump;
    }

  //cerr << "INCS "
  // << increments[0] << " " << increments[1] << " " << increments[2] << endl;
  int dx = (e1-e0 + ptAdjust);
  if (dx == 0) dx = 1;
  int dy = (e3-e2 + ptAdjust);
  if (dy == 0) dy = 1;

  incY = increments[1] - dx*increments[0];
  incZ = increments[2] - dy*increments[1];

  //cerr << "RETURN " << incX << " " << incY << " " << incZ << endl;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageAppendExecute(vtkImageAppend *self, int id,
                           int inExt[6], vtkImageData *inData, T *inPtr,
                           int outExt[6], vtkImageData *outData, T *outPtr,
                           vtkIdType numComp,
                           bool forCells,
                           int nArrays)
{
  int idxR, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  double dnArrays = (double)nArrays;

  vtkImageAppendGetContinuousIncrements(
    inData->GetExtent(), inExt, numComp, forCells, inIncX, inIncY, inIncZ);

  //cerr << "IN INCS " << inIncX << " " << inIncY << " " << inIncZ << endl;
  vtkImageAppendGetContinuousIncrements(
    outData->GetExtent(), outExt, numComp, forCells, outIncX, outIncY, outIncZ);
  //cerr << "OUT INCS " << outIncX << " " << outIncY << " " << outIncZ << endl;

  int ptAdjust = (forCells?0:1);
  // find the region to loop over
  maxX = inExt[1]-inExt[0]+ptAdjust;
  if (maxX == 0)
    {
    maxX = 1;
    }
  rowLength = maxX*numComp;
  maxY = inExt[3] - inExt[2] + ptAdjust;
  if (maxY == 0)
    {
    maxY = 1;
    }
  maxZ = inExt[5] - inExt[4] + ptAdjust;
  if (maxZ == 0)
    {
    maxZ = 1;
    }
  //cerr << "SETUP " << endl;
  //cerr << "IE0:" << inExt[0] << " IE1:" << inExt[1] << endl;
  //cerr << "IE2:" << inExt[2] << " IE2:" << inExt[3] << endl;
  //cerr << "IE4:" << inExt[4] << " IE5:" << inExt[5] << endl;
  //cerr << "PTS:" << ptAdjust << " NCOMP:" << numComp << " RL:" << rowLength << endl;

  target = static_cast<unsigned long>((maxZ+ptAdjust)*(maxY+ptAdjust)/50.0/dnArrays);
  target++;

  // Loop through input pixels
  for (idxZ = 0; idxZ < maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY < maxY; idxY++)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      //cerr << "PTRS " << inPtr << " " << outPtr << endl;
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
        //cerr << idxZ << "," << idxY << "," << idxR << " " << *inPtr << endl;
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
  vtkIdType outIncY, outIncZ;
  int rowLength;
  int typeSize;
  unsigned char *outPtrZ, *outPtrY;

  // This method needs to clear all point-data for the update-extent.

  vtkPointData* pd = outData->GetPointData();
  for (int arrayIdx=0; arrayIdx < pd->GetNumberOfArrays(); arrayIdx++)
    {
    vtkDataArray* array = pd->GetArray(arrayIdx);
    if (!array)
      {
      continue;
      }

    typeSize = vtkDataArray::GetDataTypeSize(array->GetDataType());
    outPtrZ = static_cast<unsigned char *>(
      outData->GetArrayPointerForExtent(array, outExt));

    // Get increments to march through data
    vtkIdType increments[3];
    outData->GetArrayIncrements(array, increments);
    outIncY = increments[1];
    outIncZ = increments[2];

    outIncY *= typeSize;
    outIncZ *= typeSize;

    // Find the region to loop over
    rowLength = (outExt[1] - outExt[0]+1)* array->GetNumberOfComponents();
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
  int c_in[3], c_out[3];
  void *inPtr;
  void *outPtr;
  int nArrays;

  this->InitOutput(outExt, outData[0]);

  for (idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
    {
    //cerr << "INPUT " << idx1 << endl;

    if (inData[0][idx1] != NULL)
      {
      nArrays = inData[0][idx1]->GetPointData()->GetNumberOfArrays() +
        inData[0][idx1]->GetCellData()->GetNumberOfArrays();

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

      c_in[0] = inExt[0];
      c_in[1] = inExt[2];
      c_in[2] = inExt[4];

      c_out[0] = cOutExt[0];
      c_out[1] = cOutExt[2];
      c_out[2] = cOutExt[4];

      // do a quick check to see if the input is used at all.
      if (inExt[0] <= inExt[1] &&
          inExt[2] <= inExt[3] &&
          inExt[4] <= inExt[5])
        {
        vtkIdType ai;
        vtkDataArray *inArray;
        vtkDataArray *outArray;
        vtkIdType numComp;

        //do point associated arrays
        for (ai = 0;
             ai < inData[0][idx1]->GetPointData()->GetNumberOfArrays();
             ai++)
          {
          //cerr << "POINT ARRAY " << ai << endl;

          inArray = inData[0][idx1]->GetPointData()->GetArray(ai);
          outArray = outData[0]->GetPointData()->GetArray(ai);

          numComp = inArray->GetNumberOfComponents();
          if (numComp != outArray->GetNumberOfComponents())
            {
            vtkErrorMacro("Components of the inputs do not match");
            return;
            }

          // this filter expects that input is the same type as output.
          if (inArray->GetDataType() != outArray->GetDataType())
            {
            vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType ("
                          << inArray->GetDataType()
                          << "), must match output ScalarType ("
                          << outArray->GetDataType() << ")");
            return;
            }

          inPtr = inData[0][idx1]->GetArrayPointerForExtent(inArray, inExt);
          outPtr = outData[0]->GetArrayPointerForExtent(outArray, cOutExt);

          //cerr << "INITIAL PTRS " << inPtr << " " << outPtr << endl;
          switch (inArray->GetDataType())
            {
            vtkTemplateMacro(
                             vtkImageAppendExecute(this, id,
                                                   inExt, inData[0][idx1],
                                                   static_cast<VTK_TT *>(inPtr),
                                                   cOutExt, outData[0],
                                                   static_cast<VTK_TT *>(outPtr),
                                                   numComp,
                                                   false,
                                                   nArrays));
            default:
              vtkErrorMacro(<< "Execute: Unknown ScalarType");
              return;
            }
          }

        //do cell associated arrays
        for (ai = 0;
             ai < inData[0][idx1]->GetCellData()->GetNumberOfArrays();
             ai++)
          {
          //cerr << "CELL ARRAY " << ai << endl;

          inArray = inData[0][idx1]->GetCellData()->GetArray(ai);
          outArray = outData[0]->GetCellData()->GetArray(ai);

          numComp = inArray->GetNumberOfComponents();
          if (numComp != outArray->GetNumberOfComponents())
            {
            vtkErrorMacro("Components of the inputs do not match");
            return;
            }

          // this filter expects that input is the same type as output.
          if (inArray->GetDataType() != outArray->GetDataType())
            {
            vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType ("
                          << inArray->GetDataType()
                          << "), must match output ScalarType ("
                          << outArray->GetDataType() << ")");
            return;
            }

          vtkIdType cellId;
          cellId = vtkStructuredData::ComputeCellIdForExtent(inExt, c_in);
          inPtr = inArray->GetVoidPointer(cellId*numComp);
          cellId = vtkStructuredData::ComputeCellIdForExtent(outExt, c_out);
          outPtr = outArray->GetVoidPointer(cellId*numComp);
          //cerr << "INITIAL PTRS " << inPtr << " " << outPtr << " "
          //     << c_out[0] << "," << c_out[1] << "," << c_out[2] << ":"
          //     << outExt[0] << " " << outExt[1] << ", "
          //     << outExt[2] << " " << outExt[3] << ", "
          //     << outExt[4] << " " << outExt[5] << endl;

          switch (inArray->GetDataType())
            {
            vtkTemplateMacro(
                             vtkImageAppendExecute(this, id,
                                                   inExt, inData[0][idx1],
                                                   static_cast<VTK_TT *>(inPtr),
                                                   cOutExt, outData[0],
                                                   static_cast<VTK_TT *>(outPtr),
                                                   numComp,
                                                   true,
                                                   nArrays));
            default:
              vtkErrorMacro(<< "Execute: Unknown ScalarType");
              return;
            }
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

//----------------------------------------------------------------------------
void vtkImageAppend::AllocateOutputData(vtkImageData *output,
                                        vtkInformation*,
                                        int *uExtent)
{
  output->SetExtent(uExtent);

  //compute number of cells and points in the uExtent
  vtkIdType numpts = 1;
  vtkIdType numcells = 1;
  for (int i = 0; i < 3; i++)
    {
    if (uExtent[i*2+1] >= uExtent[i*2])
      {
      vtkIdType dim = uExtent[i*2+1]-uExtent[i*2];
      numpts = numpts*(dim+1);
      if (dim != 0)
        {
        numcells = numcells*dim;
        }
      }
    }

  //get a hold of any of my inputs to get arrays
  vtkImageData *in = vtkImageData::SafeDownCast(this->GetInputDataObject(0,0));

  vtkDataSetAttributes *ifd, *ofd;
  ifd = in->GetPointData();
  ofd = output->GetPointData();
  if (ifd && ofd)
    {
    ofd->CopyAllOn();
    ofd->CopyAllocate(ifd, numpts);
    ofd->SetNumberOfTuples(numpts);
    }
  ifd = in->GetCellData();
  ofd = output->GetCellData();
  if (ifd && ofd)
    {
    ofd->CopyAllOn();
    ofd->CopyAllocate(ifd, numcells);
    ofd->SetNumberOfTuples(numcells);
    }
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageAppend::AllocateOutputData(vtkDataObject *output,
                                                 vtkInformation* outInfo)
{
  // set the extent to be the update extent
  vtkImageData *out = vtkImageData::SafeDownCast(output);
  if (out)
    {
    int* uExtent = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    this->AllocateOutputData(out, outInfo, uExtent);
    }
  return out;
}

//----------------------------------------------------------------------------
void vtkImageAppend::CopyAttributeData(vtkImageData *vtkNotUsed(input),
                                       vtkImageData *vtkNotUsed(output),
                                       vtkInformationVector **vtkNotUsed(inputVector))
{
  //Do not simply shallow copy forward the data as other imaging filters do.
  //We have to append instead.
}
