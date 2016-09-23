/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedImageAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreadedImageAlgorithm.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"

// If SMP backend is Sequential then fall back to vtkMultiThreader,
// else enable the newer vtkSMPTools code path by default.
#ifdef VTK_SMP_Sequential
bool vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP = false;
#else
bool vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP = true;
#endif

//----------------------------------------------------------------------------
vtkThreadedImageAlgorithm::vtkThreadedImageAlgorithm()
{
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();

  // SMP default settings
  this->EnableSMP = vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP;

  // Splitting method
  this->SplitMode = SLAB;
  this->SplitPath[0] = 2;
  this->SplitPath[1] = 1;
  this->SplitPath[2] = 0;
  this->SplitPathLength = 3;

  // Minumum block size
  this->MinimumPieceSize[0] = 16;
  this->MinimumPieceSize[1] = 1;
  this->MinimumPieceSize[2] = 1;

  // The desired block size in bytes
  this->DesiredBytesPerPiece = 65536;
}

//----------------------------------------------------------------------------
vtkThreadedImageAlgorithm::~vtkThreadedImageAlgorithm()
{
  this->Threader->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedImageAlgorithm::SetGlobalDefaultEnableSMP(bool enable)
{
  if (enable != vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP)
  {
    vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP = enable;
  }
}

//----------------------------------------------------------------------------
bool vtkThreadedImageAlgorithm::GetGlobalDefaultEnableSMP()
{
  return vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP;
}

//----------------------------------------------------------------------------
void vtkThreadedImageAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  os << indent << "EnableSMP: " << (this->EnableSMP ? "On\n" : "Off\n");
  os << indent << "GlobalDefaultEnableSMP: "
     << (vtkThreadedImageAlgorithm::GlobalDefaultEnableSMP ?
         "On\n" : "Off\n");
  os << indent << "MinimumPieceSize: "
     << this->MinimumPieceSize[0] << " "
     << this->MinimumPieceSize[1] << " "
     << this->MinimumPieceSize[2] << "\n";
  os << indent << "DesiredBytesPerPiece: "
     << this->DesiredBytesPerPiece << "\n";
  os << indent << "SplitMode: "
     << (this->SplitMode == SLAB ? "Slab\n" :
         (this->SplitMode == BEAM ? "Beam\n" :
          (this->SplitMode == BLOCK ? "Block\n" : "Unknown\n")));
}

//----------------------------------------------------------------------------
struct vtkImageThreadStruct
{
  vtkThreadedImageAlgorithm *Filter;
  vtkInformation *Request;
  vtkInformationVector **InputsInfo;
  vtkInformationVector *OutputsInfo;
  vtkImageData   ***Inputs;
  vtkImageData   **Outputs;
};

//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".
// If 1 is returned, the extent cannot be split.
int vtkThreadedImageAlgorithm::SplitExtent(int splitExt[6],
                                           int startExt[6],
                                           int num, int total)
{
  // split path (the order in which to split the axes)
  int pathlen = this->SplitPathLength;
  int mode = this->SplitMode;
  int axis0 = this->SplitPath[0];
  int axis1 = this->SplitPath[1];
  int axis2 = this->SplitPath[2];
  int path[3] = { axis0, axis1, axis2 };

  // divisions
  int divs[3] = { 1, 1, 1 };

  // this needs 64 bits to avoid overflow in the math below
  const vtkTypeInt64 size[3] = {
    startExt[1] - startExt[0] + 1,
    startExt[3] - startExt[2] + 1,
    startExt[5] - startExt[4] + 1 };

  // check for valid extent
  if (size[0] <= 0 || size[1] <= 0 || size[2] <= 0)
  {
    return 0;
  }

  // divide out the minimum block size
  int maxdivs[3] = { 1, 1, 1 };
  for (int i = 0; i < 3; i++)
  {
    if (size[i] > this->MinimumPieceSize[i] && this->MinimumPieceSize[i] > 0)
    {
      maxdivs[i] = size[i]/this->MinimumPieceSize[i];
    }
  }

  // make sure total is not greater than max number of pieces
  vtkTypeInt64 maxPieces = maxdivs[axis0];
  vtkTypeInt64 maxPieces2D = maxPieces;
  if (pathlen > 1)
  {
    maxPieces *= maxdivs[axis1];
    maxPieces2D = maxPieces;
    if (pathlen > 2)
    {
      maxPieces *= maxdivs[axis2];
    }
  }
  if (total > maxPieces)
  {
    total = maxPieces;
  }

  if (mode == SLAB || pathlen < 2)
  {
    // split the axes in the given order
    divs[axis0] = maxdivs[axis0];
    if (total < maxdivs[axis0])
    {
      divs[axis0] = total;
    }
    else if (pathlen > 1)
    {
      divs[axis1] = maxdivs[axis1];
      int q = total/divs[axis0];
      if (q < maxdivs[axis1])
      {
        divs[axis1] = q;
      }
      else if (pathlen > 2)
      {
        divs[axis2] = q/divs[axis1];
      }
    }
  }
  else if (mode == BEAM || pathlen < 3)
  {
    // split two of the axes first, leave third axis for last
    if (total < maxPieces2D)
    {
      // split until we get the desired number of pieces
      while (divs[axis0]*divs[axis1] < total)
      {
        axis0 = path[0];
        axis1 = path[1];

        // if necessary, swap axes to keep a good aspect ratio
        if (size[axis0]*divs[axis1] < size[axis1]*divs[axis0])
        {
          axis0 = path[1];
          axis1 = path[0];
        }

        // compute the new split for this axis
        divs[axis0] = divs[axis1]*size[axis0]/size[axis1] + 1;
      }

      // compute final division
      divs[axis0] = total/divs[axis1];
      if (divs[axis0] > maxdivs[axis0])
      {
        divs[axis0] = maxdivs[axis0];
      }
      divs[axis1] = total/divs[axis0];
      if (divs[axis1] > maxdivs[axis1])
      {
        divs[axis1] = maxdivs[axis1];
        divs[axis0] = total/divs[axis1];
      }
    }
    else
    {
      // maximum split for first two axes
      divs[axis0] = maxdivs[axis0];
      divs[axis1] = maxdivs[axis1];
      if (pathlen > 2)
      {
        // split the third axis
        divs[axis2] = total/(divs[axis0]*divs[axis1]);
      }
    }
  }
  else // block mode: keep blocks roughly cube shaped
  {
    // split until we get the desired number of pieces
    while (divs[0]*divs[1]*divs[2] < total)
    {
      axis0 = path[0];
      axis1 = path[1];
      axis2 = path[2];

      // check whether z or y is best candidate for splitting
      if (size[axis0]*divs[axis1] < size[axis1]*divs[axis0])
      {
        axis1 = axis0;
        axis0 = path[1];
      }

      if (pathlen > 2)
      {
        // check if x is the best candidate for splitting
        if (size[axis0]*divs[path[2]] < size[path[2]]*divs[axis0])
        {
          axis2 = axis1;
          axis1 = axis0;
          axis0 = path[2];
        }
        // now find the second best candidate
        if (size[axis1]*divs[axis2] < size[axis2]*divs[axis1])
        {
          int tmp = axis2;
          axis2 = axis1;
          axis1 = tmp;
        }
      }

      // compute the new split for this axis
      divs[axis0] = divs[axis1]*size[axis0]/size[axis1] + 1;

      // if axis0 reached maxdivs, remove it from the split path
      if (divs[axis0] >= maxdivs[axis0])
      {
        divs[axis0] = maxdivs[axis0];
        if (--pathlen == 1)
        {
          break;
        }
        if (axis0 != path[2])
        {
          if (axis0 != path[1])
          {
            path[0] = path[1];
          }
          path[1] = path[2];
          path[2] = axis0;
        }
      }
    }

    // compute the final division
    divs[axis0] = total/(divs[axis1]*divs[axis2]);
    if (divs[axis0] > maxdivs[axis0])
    {
      divs[axis0] = maxdivs[axis0];
    }
    divs[axis1] = total/(divs[axis0]*divs[axis2]);
    if (divs[axis1] > maxdivs[axis1])
    {
      divs[axis1] = maxdivs[axis1];
    }
    divs[axis2] = total/(divs[axis0]*divs[axis1]);
    if (divs[axis2] > maxdivs[axis2])
    {
      divs[axis2] = maxdivs[axis2];
    }
  }

  // compute new total from the chosen divisions
  total = divs[0]*divs[1]*divs[2];

  if (splitExt)
  {
    // compute increments
    int a = divs[0];
    int b = a*divs[1];

    // compute 3D block index
    int i = num;
    int index[3];
    index[2] = i/b;
    i -= index[2]*b;
    index[1] = i/a;
    i -= index[1]*a;
    index[0] = i;

    // compute the extent for the resulting block
    for (int j = 0; j < 3; j++)
    {
      splitExt[2*j] = index[j]*size[j]/divs[j];
      splitExt[2*j + 1] = (index[j] + 1)*size[j]/divs[j] - 1;
      splitExt[2*j] += startExt[2*j];
      splitExt[2*j + 1] += startExt[2*j];
    }
  }

  // return the number of blocks (may be fewer than requested)
  return total;
}

//----------------------------------------------------------------------------
// The old way to thread an image filter, before vtkSMPTools existed:
// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
static VTK_THREAD_RETURN_TYPE vtkThreadedImageAlgorithmThreadedExecute( void *arg )
{
  vtkImageThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;

  threadId = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->ThreadID;
  threadCount = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->NumberOfThreads;

  str = static_cast<vtkImageThreadStruct *>
    (static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);

  // if we have an output
  if (str->Filter->GetNumberOfOutputPorts())
  {
    // which output port did the request come from
    int outputPort =
      str->Request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

    // if output port is negative then that means this filter is calling the
    // update directly, for now an error
    if (outputPort == -1)
    {
      return VTK_THREAD_RETURN_VALUE;
    }

    // get the update extent from the output port
    vtkInformation *outInfo =
      str->OutputsInfo->GetInformationObject(outputPort);
    int updateExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                 updateExtent);
    memcpy(ext,updateExtent, sizeof(int)*6);
  }
  else
  {
    // if there is no output, then use UE from input, use the first input
    int inPort;
    bool found = false;
    for (inPort = 0; inPort < str->Filter->GetNumberOfInputPorts(); ++inPort)
    {
      if (str->Filter->GetNumberOfInputConnections(inPort))
      {
        int updateExtent[6];
        str->InputsInfo[inPort]
          ->GetInformationObject(0)
          ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                updateExtent);
        memcpy(ext,updateExtent, sizeof(int)*6);
        found = true;
        break;
      }
    }
    if (!found)
    {
      return VTK_THREAD_RETURN_VALUE;
    }
  }

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);

  if (threadId < total)
  {
    // return if nothing to do
    if (splitExt[1] < splitExt[0] ||
        splitExt[3] < splitExt[2] ||
        splitExt[5] < splitExt[4])
    {
      return VTK_THREAD_RETURN_VALUE;
    }
    str->Filter->ThreadedRequestData(str->Request,
                                     str->InputsInfo, str->OutputsInfo,
                                     str->Inputs, str->Outputs,
                                     splitExt, threadId);
  }
  // else
  //   {
  //   otherwise don't use this thread. Sometimes the threads dont
  //   break up very well and it is just as efficient to leave a
  //   few threads idle.
  //   }

  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
// This functor is used with vtkSMPTools to execute the algorithm in pieces
// split over the extent of the data.
class vtkThreadedImageAlgorithmFunctor
{
public:
  // Create the functor by providing all of the information that will be
  // needed by the ThreadedRequestData method that the functor will call.
  vtkThreadedImageAlgorithmFunctor(
    vtkThreadedImageAlgorithm *algo,
    vtkInformation *request,
    vtkInformationVector **inputsInfo,
    vtkInformationVector *outputsInfo,
    vtkImageData ***inputs,
    vtkImageData **outputs,
    const int extent[6],
    vtkIdType pieces)
    : Algorithm(algo), Request(request),
      InputsInfo(inputsInfo), OutputsInfo(outputsInfo),
      Inputs(inputs), Outputs(outputs),
      NumberOfPieces(pieces)
  {
    for (int i = 0; i < 6; i++)
    {
      this->Extent[i] = extent[i];
    }
  }

  // Called by vtkSMPTools to execute the algorithm over specific pieces.
  void operator()(vtkIdType begin, vtkIdType end)
  {
    this->Algorithm->SMPRequestData(
      this->Request, this->InputsInfo, this->OutputsInfo,
      this->Inputs, this->Outputs,
      begin, end, this->NumberOfPieces, this->Extent);
  }

private:
  vtkThreadedImageAlgorithmFunctor();

  vtkThreadedImageAlgorithm *Algorithm;
  vtkInformation *Request;
  vtkInformationVector **InputsInfo;
  vtkInformationVector *OutputsInfo;
  vtkImageData ***Inputs;
  vtkImageData **Outputs;
  int Extent[6];
  vtkIdType NumberOfPieces;
};

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkThreadedImageAlgorithm::SMPRequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector,
  vtkImageData ***inData,
  vtkImageData **outData,
  vtkIdType begin,
  vtkIdType end,
  vtkIdType numPieces,
  int extent[6])
{
  for (vtkIdType piece = begin; piece < end; piece++)
  {
    int splitExt[6] = { 0, -1, 0, -1, 0, -1 };

    vtkIdType total = this->SplitExtent(splitExt, extent, piece, numPieces);

    // check for valid piece and extent
    if (piece < total &&
        splitExt[0] <= splitExt[1] &&
        splitExt[2] <= splitExt[3] &&
        splitExt[4] <= splitExt[5])
    {
      this->ThreadedRequestData(
        request, inputVector, outputVector, inData, outData, splitExt, piece);
    }
  }
}

//----------------------------------------------------------------------------
void vtkThreadedImageAlgorithm::PrepareImageData(
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector,
  vtkImageData*** inDataObjects,
  vtkImageData** outDataObjects)
{
  vtkImageData* firstInput = 0;
  vtkImageData* firstOutput = 0;

  // now we must create the output array
  int numOutputPorts = this->GetNumberOfOutputPorts();
  for (int i = 0; i < numOutputPorts; i++)
  {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkImageData *outData = vtkImageData::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    if (i == 0)
    {
      firstOutput = outData;
    }
    if (outDataObjects)
    {
      outDataObjects[i] = outData;
    }
    if (outData)
    {
      int updateExtent[6];
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                updateExtent);

      // unlike geometry filters, for image filters data is pre-allocated
      // in the superclass (which means, in this class)
      this->AllocateOutputData(outData, info, updateExtent);
    }
  }

  // now create the inputs array
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; i++)
  {
    vtkInformationVector* portInfo = inputVector[i];
    int numConnections = portInfo->GetNumberOfInformationObjects();
    for (int j = 0; j < numConnections; j++)
    {
      vtkInformation* info = portInfo->GetInformationObject(j);
      vtkImageData *inData = vtkImageData::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
      if (i == 0 && j == 0)
      {
        firstInput = inData;
      }
      if (inDataObjects && inDataObjects[i])
      {
        inDataObjects[i][j] = inData;
      }
    }
  }

  // copy other arrays
  if (firstInput && firstOutput)
  {
    this->CopyAttributeData(firstInput, firstOutput, inputVector);
  }
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkThreadedImageAlgorithm::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // setup the thread structure
  vtkImageThreadStruct str;
  str.Filter = this;
  str.Request = request;
  str.InputsInfo = inputVector;
  str.OutputsInfo = outputVector;
  str.Inputs = 0;
  str.Outputs = 0;

  // create an array for input data objects
  int numInputPorts = this->GetNumberOfInputPorts();
  if (numInputPorts)
  {
    str.Inputs = new vtkImageData ** [numInputPorts];
    for (int i = 0; i < numInputPorts; i++)
    {
      int numConnections = inputVector[i]->GetNumberOfInformationObjects();
      str.Inputs[i] = new vtkImageData * [numConnections];
    }
  }

  // create an array for output data objects
  int numOutputPorts = this->GetNumberOfOutputPorts();
  if (numOutputPorts)
  {
    str.Outputs = new vtkImageData * [numOutputPorts];
  }

  // allocate the output data and call CopyAttributeData
  this->PrepareImageData(inputVector, outputVector, str.Inputs, str.Outputs);

  if (this->EnableSMP)
  {
    // SMP is enabled, use vtkSMPTools to thread the filter
    int updateExtent[6] = { 0, -1, 0, -1, 0, -1 };

    // need bytes per voxel to compute block size
    int bytesPerVoxel = 1;

    // get the update extent from the output, if there is an output
    if (numOutputPorts)
    {
      vtkImageData *outData = str.Outputs[0];
      if (outData)
      {
        bytesPerVoxel = (outData->GetScalarSize() *
                         outData->GetNumberOfScalarComponents());
        outData->GetExtent(updateExtent);
      }
    }
    else
    {
      // if no output, get update extent from the first input
      for (int inPort = 0; inPort < numInputPorts; inPort++)
      {
        if (this->GetNumberOfInputConnections(inPort))
        {
          vtkImageData *inData = str.Inputs[inPort][0];
          if (inData)
          {
            bytesPerVoxel = (inData->GetScalarSize() *
                             inData->GetNumberOfScalarComponents());
            inData->GetExtent(updateExtent);
            break;
          }
        }
      }
    }

    // verify that there is an extent for execution
    if (updateExtent[0] <= updateExtent[1] &&
        updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5])
    {
      // compute a reasonable number of pieces, this will be a multiple of
      // the number of available threads and relative to the data size
      vtkTypeInt64 bytesize = (
        static_cast<vtkTypeInt64>(updateExtent[1] - updateExtent[0] + 1)*
        static_cast<vtkTypeInt64>(updateExtent[3] - updateExtent[2] + 1)*
        static_cast<vtkTypeInt64>(updateExtent[5] - updateExtent[4] + 1)*
        bytesPerVoxel);
      vtkTypeInt64 bytesPerPiece = this->DesiredBytesPerPiece;
      vtkIdType pieces = vtkSMPTools::GetEstimatedNumberOfThreads();
      if (bytesPerPiece > 0 && bytesPerPiece < bytesize)
      {
        vtkTypeInt64 b = pieces*bytesPerPiece;
        pieces *= (bytesize + b - 1)/b;
      }
      // do a dummy execution of SplitExtent to compute the number of pieces
      int subExtent[6];
      pieces = this->SplitExtent(subExtent, updateExtent, 0, pieces);

      // always shut off debugging to avoid threading problems with GetMacros
      bool debug = this->Debug;
      this->Debug = false;

      vtkThreadedImageAlgorithmFunctor functor(
        this, request, inputVector, outputVector,
        str.Inputs, str.Outputs, updateExtent, pieces);

      vtkSMPTools::For(0, pieces, functor);

      this->Debug = debug;
    }
  }
  else
  {
    // if SMP is not enabled, use the vtkMultiThreader
    this->Threader->SetNumberOfThreads(this->NumberOfThreads);
    this->Threader->SetSingleMethod(vtkThreadedImageAlgorithmThreadedExecute, &str);
    // always shut off debugging to avoid threading problems with GetMacros
    bool debug = this->Debug;
    this->Debug = false;
    this->Threader->SingleMethodExecute();
    this->Debug = debug;
  }

  // free up the arrays
  for (int i = 0; i < numInputPorts; i++)
  {
    delete [] str.Inputs[i];
  }
  delete [] str.Inputs;
  delete [] str.Outputs;

  return 1;
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkThreadedImageAlgorithm::ThreadedRequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int extent[6],
  int threadId)
{
  this->ThreadedExecute(inData[0][0], outData[0], extent, threadId);
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkThreadedImageAlgorithm::ThreadedExecute(
  vtkImageData * inData,
  vtkImageData * outData,
  int extent[6],
  int threadId)
{
  (void)inData;
  (void)outData;
  (void)extent;
  (void)threadId;
  vtkErrorMacro("Subclass should override this method!!!");
}
