/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAlgorithm.h"

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
#include "vtkTrivialProducer.h"

vtkCxxRevisionMacro(vtkImageAlgorithm, "1.1.2.2");

//----------------------------------------------------------------------------
vtkImageAlgorithm::vtkImageAlgorithm()
{
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
  this->InputScalarsSelection = NULL;
}

//----------------------------------------------------------------------------
vtkImageAlgorithm::~vtkImageAlgorithm()
{
  this->Threader->Delete();
  this->SetInputScalarsSelection(NULL);
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
}

struct vtkImageThreadStruct
{
  vtkImageAlgorithm *Filter;
  vtkInformation *Request;
  vtkInformationVector *InputsInfo;
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
int vtkImageAlgorithm::SplitExtent(int splitExt[6], int startExt[6], 
                                   int num, int total)
{
  int splitAxis;
  int min, max;

  vtkDebugMacro("SplitExtent: ( " << startExt[0] << ", " << startExt[1] << ", "
                << startExt[2] << ", " << startExt[3] << ", "
                << startExt[4] << ", " << startExt[5] << "), " 
                << num << " of " << total);

  // start with same extent
  memcpy(splitExt, startExt, 6 * sizeof(int));

  splitAxis = 2;
  min = startExt[4];
  max = startExt[5];
  while (min == max)
    {
    --splitAxis;
    if (splitAxis < 0)
      { // cannot split
      vtkDebugMacro("  Cannot Split");
      return 1;
      }
    min = startExt[splitAxis*2];
    max = startExt[splitAxis*2+1];
    }

  // determine the actual number of pieces that will be generated
  int range = max - min + 1;
  int valuesPerThread = (int)ceil(range/(double)total);
  int maxThreadIdUsed = (int)ceil(range/(double)valuesPerThread) - 1;
  if (num < maxThreadIdUsed)
    {
    splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
    splitExt[splitAxis*2+1] = splitExt[splitAxis*2] + valuesPerThread - 1;
    }
  if (num == maxThreadIdUsed)
    {
    splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
    }
  
  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
                << splitExt[2] << ", " << splitExt[3] << ", "
                << splitExt[4] << ", " << splitExt[5] << ")");

  return maxThreadIdUsed + 1;
}


// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
VTK_THREAD_RETURN_TYPE vtkImageAlgorithmThreadedExecute( void *arg )
{
  vtkImageThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;
  
  threadId = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  
  str = (vtkImageThreadStruct *)
    (((vtkMultiThreader::ThreadInfo *)(arg))->UserData);

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
    for (inPort = 0; inPort < str->Filter->GetNumberOfInputPorts(); ++inPort)
      {
      if (str->Filter->GetNumberOfInputConnections(inPort))
        {
        int updateExtent[6];
        str->InputsInfo->GetInformationObject(inPort)
          ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
          ->GetInformationObject(0)
          ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 
                updateExtent);
        memcpy(ext,updateExtent, sizeof(int)*6);
        break;
        }
      }
    if (inPort >= str->Filter->GetNumberOfInputPorts())
      {
      return VTK_THREAD_RETURN_VALUE;
      }
    }
  
  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);
    
  if (threadId < total)
    {
    str->Filter->ThreadedExecute(str->Inputs, str->Outputs, 
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
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
void vtkImageAlgorithm::ExecuteData(
  vtkInformation *request, 
  vtkInformationVector *inputVector, 
  vtkInformationVector *outputVector)
{
  int i;
  
  // setup the threasd structure
  vtkImageThreadStruct str;
  str.Filter = this;
  str.Request = request;
  str.InputsInfo = inputVector;
  str.OutputsInfo = outputVector;

  // now we must create the output array
  str.Outputs = new vtkImageData * [this->GetNumberOfOutputPorts()];
  for (i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkImageData *outData = static_cast<vtkImageData *>(
      info->Get(vtkDataObject::DATA_OBJECT()));
    str.Outputs[i] = outData;

    int updateExtent[6];
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);

    outData->SetScalarType( info->Get(vtkDataObject::SCALAR_TYPE()) );
    outData->SetNumberOfScalarComponents( info->Get(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS()) );

    // for image filters as a convinience we usually allocate the output data
    // in the superclass
    this->AllocateOutputData(outData, updateExtent);
    }
  
  // now create the inputs array
  str.Inputs = new vtkImageData ** [this->GetNumberOfInputPorts()];
  for (i = 0; i < this->GetNumberOfInputPorts(); ++i)
    {
    int j;
    vtkInformation* info = inputVector->GetInformationObject(i);
    str.Inputs[i] = new vtkImageData *[this->GetNumberOfInputConnections(i)];
    for (j = 0; j < this->GetNumberOfInputConnections(i); ++j)
      {
      vtkInformation *connInfo = 
        info->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
        ->GetInformationObject(j);
      str.Inputs[i][j] = static_cast<vtkImageData *>(
        connInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
    }

  // copy other arrays
  if (str.Inputs && str.Inputs[0] && str.Outputs)
    {
    this->CopyAttributeData(str.Inputs[0][0],str.Outputs[0]);
    }
    
  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  this->Threader->SetSingleMethod(vtkImageAlgorithmThreadedExecute, &str);  

  // always shut off debugging to avoid threading problems with GetMacros
  int debug = this->Debug;
  this->Debug = 0;
  this->Threader->SingleMethodExecute();
  this->Debug = debug;

  // free up the arrays
  for (i = 0; i < this->GetNumberOfInputPorts(); ++i)
    {
    delete [] str.Inputs[i];
    }
  delete [] str.Inputs;
  delete [] str.Outputs;  
}


int vtkImageAlgorithm::ProcessDownstreamRequest(
  vtkInformation *request, 
  vtkInformationVector *inputVector, 
  vtkInformationVector *outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    // get the output data object
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkImageData *output = 
      vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    
    output->PrepareForNewData();
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    this->AbortExecute = 0;
    this->Progress = 0.0;

    this->ExecuteData(request, inputVector, outputVector);

    if(!this->AbortExecute)
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    // Mark the data as up-to-date.
    output->DataHasBeenGenerated();
    return 1;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->ExecuteInformation(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessDownstreamRequest(request, inputVector,
                                                    outputVector);
}

void vtkImageAlgorithm::ExecuteInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector * vtkNotUsed(inputVector), 
  vtkInformationVector * vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
}

void vtkImageAlgorithm::ComputeInputUpdateExtent(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector * vtkNotUsed(inputVector), 
  vtkInformationVector * vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
}

int vtkImageAlgorithm::ProcessUpstreamRequest(
  vtkInformation *request, 
  vtkInformationVector *inputVector, 
  vtkInformationVector *outputVector)
{
  // execute information
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    this->ComputeInputUpdateExtent(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessUpstreamRequest(request, inputVector,
                                                  outputVector);
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageAlgorithm::ThreadedExecute(
  vtkImageData ***vtkNotUsed(inData), 
  vtkImageData **vtkNotUsed(outData),
  int extent[6], 
  int vtkNotUsed(threadId))
{
  extent = extent;
  vtkErrorMacro("subclase should override this method!!!");
}


//----------------------------------------------------------------------------
void vtkImageAlgorithm::AllocateOutputData(vtkImageData *output, 
                                           int *uExtent)
{ 
  // set the extent to be the update extent
  output->SetExtent(uExtent);
  output->AllocateScalars();
}


// by default copy the attr from the first input to the first output
void vtkImageAlgorithm::CopyAttributeData(vtkImageData *input,
                                          vtkImageData *output)
{
  if (!input || !output)
    {
    return;
    }
  
  int inExt[6];
  int outExt[6];
  vtkDataArray *inArray;
  vtkDataArray *outArray;

  input->GetExtent(inExt);
  output->GetExtent(outExt);

  // Do not copy the array we will be generating.
  inArray = input->GetPointData()->GetScalars(this->InputScalarsSelection);

  // Conditionally copy point and cell data.  Only copy if corresponding
  // indexes refer to identical points.
  double *oIn = input->GetOrigin();
  double *sIn = input->GetSpacing();
  double *oOut = output->GetOrigin();
  double *sOut = output->GetSpacing();
  if (oIn[0] == oOut[0] && oIn[1] == oOut[1] && oIn[2] == oOut[2] &&
      sIn[0] == sOut[0] && sIn[1] == sOut[1] && sIn[2] == sOut[2])   
    {
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();
    // Scalar copy flag trumps the array copy flag.
    if (inArray == input->GetPointData()->GetScalars())
      {
      output->GetPointData()->CopyScalarsOff();
      }
    else
      {
      output->GetPointData()->CopyFieldOff(this->InputScalarsSelection);
      }

    // If the extents are the same, then pass the attribute data for
    // efficiency.
    if (inExt[0] == outExt[0] && inExt[1] == outExt[1] &&
        inExt[2] == outExt[2] && inExt[3] == outExt[3] &&
        inExt[4] == outExt[4] && inExt[5] == outExt[5])
      {// Pass
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
      }
    else
      {// Copy
       // Since this can be expensive to copy all of these values,
       // lets make sure there are arrays to copy (other than the scalars)
      if (input->GetPointData()->GetNumberOfArrays() > 1)
        {
        // Copy the point data.
        // CopyAllocate frees all arrays.
        // Keep the old scalar array (not being copied).
        // This is a hack, but avoids reallocation ...
        vtkDataArray *tmp = NULL;
        if ( ! output->GetPointData()->GetCopyScalars() )
          {
          tmp = output->GetPointData()->GetScalars();
          }
        output->GetPointData()->CopyAllocate(input->GetPointData(), 
                                             output->GetNumberOfPoints());
        if (tmp)
          { // Restore the array.
          output->GetPointData()->SetScalars(tmp);
          }
        // Now Copy The point data, but only if output is a subextent of the
        // input.
        if (outExt[0] >= inExt[0] && outExt[1] <= inExt[1] &&
            outExt[2] >= inExt[2] && outExt[3] <= inExt[3] &&
            outExt[4] >= inExt[4] && outExt[5] <= inExt[5])
          {
          output->GetPointData()->CopyStructuredData(input->GetPointData(),
                                                     inExt, outExt);
          }
        }

      if (input->GetCellData()->GetNumberOfArrays() > 0)
        {
        output->GetCellData()->CopyAllocate(input->GetCellData(), 
                                            output->GetNumberOfCells());  
        // Cell extent is one less than point extent.
        // Conditional to handle a colapsed axis (lower dimensional cells).
        if (inExt[0] < inExt[1]) {--inExt[1];}
        if (inExt[2] < inExt[3]) {--inExt[3];}
        if (inExt[4] < inExt[5]) {--inExt[5];}
        // Cell extent is one less than point extent.
        if (outExt[0] < outExt[1]) {--outExt[1];}
        if (outExt[2] < outExt[3]) {--outExt[3];}
        if (outExt[4] < outExt[5]) {--outExt[5];}
        // Now Copy The cell data, but only if output is a subextent of the input.  
        if (outExt[0] >= inExt[0] && outExt[1] <= inExt[1] &&
            outExt[2] >= inExt[2] && outExt[3] <= inExt[3] &&
            outExt[4] >= inExt[4] && outExt[5] <= inExt[5])
          {
          output->GetCellData()->CopyStructuredData(input->GetCellData(),
                                                    inExt, outExt);
          }
        }
      }
    }

  // set the name of the output to match the input name
  outArray = output->GetPointData()->GetScalars();
  if (inArray)
    {
    outArray->SetName(inArray->GetName());
    }
}


//----------------------------------------------------------------------------
vtkImageData* vtkImageAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageAlgorithm::GetOutput(int port)
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::SetInput(vtkImageData* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::SetInput(int index, vtkImageData* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->SetInputConnection(index, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial producer.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->SetInputConnection(index, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
    }
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::AddInput(vtkImageData* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::AddInput(int index, vtkImageData* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->AddInputConnection(index, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial producer.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->AddInputConnection(index, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
}

int vtkImageAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  info->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  
  return 1;
}

int vtkImageAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
