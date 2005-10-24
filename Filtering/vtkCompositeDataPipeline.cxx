/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCompositeDataPipeline.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"

vtkCxxRevisionMacro(vtkCompositeDataPipeline, "1.31");
vtkStandardNewMacro(vtkCompositeDataPipeline);

vtkInformationKeyMacro(vtkCompositeDataPipeline,BEGIN_LOOP,Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline,END_LOOP,Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline,COMPOSITE_DATA_TYPE_NAME,String);
vtkInformationKeyMacro(vtkCompositeDataPipeline,COMPOSITE_DATA_INFORMATION,ObjectBase);
vtkInformationKeyMacro(vtkCompositeDataPipeline,MARKED_FOR_UPDATE,Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline,INPUT_REQUIRED_COMPOSITE_DATA_TYPE, String);
vtkInformationKeyMacro(vtkCompositeDataPipeline,UPDATE_BLOCKS, ObjectBase);

//----------------------------------------------------------------------------
vtkCompositeDataPipeline::vtkCompositeDataPipeline()
{
  this->InLocalLoop = 0;
  this->InSubPass = 0;
  this->InformationCache = vtkInformation::New();

  this->GenericRequest = vtkInformation::New();

  this->DataObjectRequest = vtkInformation::New();
  this->DataObjectRequest->Set(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT());
  // The request is forwarded upstream through the pipeline.
  this->DataObjectRequest->Set(
    vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
      // Algorithms process this request after it is forwarded.
  this->DataObjectRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

  this->InformationRequest = vtkInformation::New();
  this->InformationRequest->Set(vtkDemandDrivenPipeline::REQUEST_INFORMATION());
  // The request is forwarded upstream through the pipeline.
  this->InformationRequest->Set(
    vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  this->InformationRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

  this->UpdateExtentRequest = vtkInformation::New();
  this->UpdateExtentRequest->Set(
    vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT());
  // The request is forwarded upstream through the pipeline.
  this->UpdateExtentRequest->Set(
    vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  // Algorithms process this request before it is forwarded.
  this->UpdateExtentRequest->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
                            
  this->DataRequest = vtkInformation::New();
  this->DataRequest->Set(REQUEST_DATA());
  // The request is forwarded upstream through the pipeline.
  this->DataRequest->Set(
    vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  this->DataRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
              
              
}

//----------------------------------------------------------------------------
vtkCompositeDataPipeline::~vtkCompositeDataPipeline()
{
  this->InformationCache->Delete();

  this->GenericRequest->Delete();
  this->DataObjectRequest->Delete();
  this->InformationRequest->Delete();
  this->UpdateExtentRequest->Delete();
  this->DataRequest->Delete();
}

//----------------------------------------------------------------------------
int
vtkCompositeDataPipeline::ComputePipelineMTime(vtkInformation* request,
                                               vtkInformationVector** inInfoVec,
                                               vtkInformationVector* outInfoVec,
                                               int requestFromOutputPort,
                                               unsigned long* mtime)
{
  // if not a subpass then just do normal stuff
  if (!this->InSubPass)
    {
    return this->Superclass::ComputePipelineMTime(request,
                                                  inInfoVec, outInfoVec,
                                                  requestFromOutputPort,
                                                  mtime);
    }

  // if in subpass then Update the time only if it is the beginning of the
  // next pass in the loop.
  if (request->Has(vtkCompositeDataSet::INDEX()))
    {
    this->SubPassTime.Modified();
    }
  *mtime = this->SubPassTime;
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ProcessRequest(vtkInformation* request,
                                             vtkInformationVector** inInfoVec,
                                             vtkInformationVector* outInfoVec)
{

  if(this->Algorithm && request->Has(BEGIN_LOOP()))
    {
    this->InSubPass = 1;
    return 1;
    }

  if(this->Algorithm && request->Has(END_LOOP()))
    {
    this->InSubPass = 0;
    return 1;
    }


  if(this->Algorithm && request->Has(REQUEST_DATA_OBJECT()))
    {
    if (this->InSubPass)
      {
      int result = 1;
      if (this->SubPassTime > this->DataObjectTime.GetMTime())
        {
        // Request information from the algorithm.
        result = this->ExecuteDataObjectForBlock(request);
        }
      return result;
      }
    }

  if(this->Algorithm && request->Has(REQUEST_INFORMATION()))
    {
    // Make sure our output information is up-to-date.
    int result = 1;
    if (this->InSubPass)
      {
      if(this->SubPassTime > this->InformationTime.GetMTime())
        {
        result = this->ExecuteInformationForBlock(request);
        
        // Information is now up to date.
        this->InformationTime.Modified();
        }
      }
    else
      {
      int appendKey = 1;
      vtkInformationKey** keys = request->Get(vtkExecutive::KEYS_TO_COPY());
      if (keys)
        {
        int len = request->Length(vtkExecutive::KEYS_TO_COPY());
        for (int i=0; i<len; i++)
          {
          if (keys[i] == vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION())
            {
            appendKey = 0;
            break;
            }
          }
        }
      if (appendKey)
        {
        request->Append(vtkExecutive::KEYS_TO_COPY(), 
                        vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION());
        }
      
      result = this->Superclass::ProcessRequest(request, inInfoVec,outInfoVec);
      }
    return result;
    }

  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    if (this->InSubPass)
      {
      return 1;
      }
    }

  if(this->Algorithm && request->Has(REQUEST_DATA()))
    {
    if (this->InSubPass)
      {
      return this->ExecuteDataForBlock(request);
      }

    // Get the output port from which the request was made.
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
      {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      }


    // UPDATE_BLOCKS() is the key that tells filters downstream which
    // blocks of a composite dataset is available. This consumers will then
    // ask for these blocks if they have to loop (simple filters in the
    // middle)
    int appendKey = 1;
    vtkInformationKey** keys = request->Get(vtkExecutive::KEYS_TO_COPY());
    if (keys)
      {
      int len = request->Length(vtkExecutive::KEYS_TO_COPY());
      for (int i=0; i<len; i++)
        {
        if (keys[i] == vtkCompositeDataPipeline::UPDATE_BLOCKS())
          {
          appendKey = 0;
          break;
          }
        }
      }
    if (appendKey)
      {
      request->Append(vtkExecutive::KEYS_TO_COPY(), 
                      vtkCompositeDataPipeline::UPDATE_BLOCKS());
      }
    
    if(this->NeedToExecuteData(outputPort,inInfoVec,outInfoVec))
      {
      // We have to check whether an input is marked for update before
      // ExecuteData() is entered. When looping over blocks, the
      // composite data producer up the pipeline will trick the
      // intermediate simple filters to execute by sending an update
      // mtime. However, if none of those filters are modified, this
      // would cause unnecessary execution of the whole intermediate
      // pipeline. Here, NeedToExecuteData() is called and the result
      // cached so that the blocks that are up to date can be skipped
      // later.
      
      // Loop over all input ports.
      for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
        {
        // Loop over all connections on this input port.
        int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
        for (int j=0; j<numInConnections; j++)
          {
          vtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
          vtkDemandDrivenPipeline* ddp = 
            vtkDemandDrivenPipeline::SafeDownCast(
              inInfo->GetExecutive(vtkExecutive::PRODUCER()));
          inInfo->Remove(MARKED_FOR_UPDATE());
          if (ddp)
            {
            if (ddp->NeedToExecuteData(-1,ddp->GetInputInformation(),
                                       ddp->GetOutputInformation()))
              {
              inInfo->Set(MARKED_FOR_UPDATE(), 1);
              }
            }
          }
        }
      }
    }

  // Let the superclass handle other requests.
  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::VerifyOutputInformation(
  int outputPort,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)

{
  if (outputPort < 0)
    {
    return this->Superclass::VerifyOutputInformation(
      outputPort, inInfoVec, outInfoVec);
    }

  vtkInformation* portInfo = 
    this->Algorithm->GetOutputPortInformation(outputPort);
  if (portInfo->Has(COMPOSITE_DATA_TYPE_NAME()))
    {
    vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
    // For an unstructured extent, make sure the update request
    // exists.  We do not need to check if it is valid because
    // out-of-range requests produce empty data.
    if(!outInfo->Has(MAXIMUM_NUMBER_OF_PIECES()))
      {
      vtkErrorMacro("No maximum number of pieces has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_PIECE_NUMBER()))
      {
      vtkErrorMacro("No update piece number has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_NUMBER_OF_PIECES()))
      {
      vtkErrorMacro("No update number of pieces has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
      {
      // Use zero ghost levels by default.
      outInfo->Set(UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
      }
    return 1;
    }
  return this->Superclass::VerifyOutputInformation(
    outputPort, inInfoVec, outInfoVec);
}


//----------------------------------------------------------------------------
// Handle REQUEST_DATA_OBJECT
int vtkCompositeDataPipeline::ExecuteDataObject(
  vtkInformation* request, 
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  // Invoke the request on the algorithm.
  int result = this->CallAlgorithm(request, vtkExecutive::RequestDownstream,
                                   inInfoVec, outInfoVec);
  if (!result)
    {
    return result;
    }

  result = this->ExecuteDataObjectForBlock(request);
  if (!result)
    {
    return result;
    }

  int i; 
  // Make sure a valid data object exists for all output ports.
  for(i=0; result && i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    result = this->CheckDataObject(i, outInfoVec);
    }

  // Make sure a valid composite data object exists for all output ports.
  for(i=0; result && i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    result = this->CheckCompositeData(i, outInfoVec);
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ExecuteDataObjectForBlock(vtkInformation* request)
{
  int result = 1;

  vtkInformationVector* outputVector = this->GetOutputInformation();
  int numOut = outputVector->GetNumberOfInformationObjects();
  for (int i=0; i<numOut; i++)
    {
    vtkInformation* portInfo = 
      this->Algorithm->GetOutputPortInformation(0);
    if (portInfo->Has(COMPOSITE_DATA_TYPE_NAME()))
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
    
      vtkDataObject* doOutput = 
        info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
      vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);

      vtkDataObject* simpleOutput = 0;

      if (output)
        {
        vtkDataObject* dobj;

        // If the index is specified, use it. Otherwise, use (0,0)
        if (request->Has(vtkCompositeDataSet::INDEX()))
          {
          dobj = output->GetDataSet(request);
          }
        else
          {
          vtkSmartPointer<vtkInformation> r = 
            vtkSmartPointer<vtkInformation>::New();
          r->Set(vtkMultiGroupDataSet::GROUP(), 0);
          r->Set(vtkCompositeDataSet::INDEX(),    0);
          dobj = output->GetDataSet(r);
          }
        
        if (dobj)
          {
          this->DataObjectTime.Modified();

          vtkDataObject* currDObj = info->Get(vtkDataObject::DATA_OBJECT());

          if (currDObj && 
              strcmp(currDObj->GetClassName(), dobj->GetClassName()) == 0)
            {
            continue;
            }
          
          simpleOutput = dobj->NewInstance();
          if (!simpleOutput)
            {
            vtkErrorMacro("Could not create a copy of " << dobj->GetClassName()
                          << ".");
            return 0;
            }
          }
        }

      if (!simpleOutput)
        {
        // If there was no current data object in the composite data object
        // and there is already any data object in the output, move to the
        // next port
        if(info->Get(vtkDataObject::DATA_OBJECT()))
          {
          continue;
          }
        // Otherwise, simply create a polydata so that filters that
        // must have a data object (such as datasettodataset filters)
        // have a dummy dataset
        simpleOutput = vtkPolyData::New();
        }
      
      simpleOutput->SetPipelineInformation(info);
      simpleOutput->Delete();
      }
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ExecuteInformationForBlock(vtkInformation* request)
{
  vtkInformationVector* outputVector = this->GetOutputInformation();
  int numOut = outputVector->GetNumberOfInformationObjects();
  for (int i=0; i<numOut; i++)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    
    vtkDataObject* doOutput = 
      info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);

    if (output)
      {
      vtkDataObject* dobj = output->GetDataSet(request);
      vtkDataObject* dobjCopy = 
        info->Get(vtkDataObject::DATA_OBJECT());
      if (dobj && dobjCopy)
        {
        dobjCopy->ShallowCopy(dobj);
        dobjCopy->CopyInformation(dobj);
        }
      }
    }

  return 1;
}


//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::ExecuteDataStart(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::ExecuteDataStart(request, inInfoVec, outInfoVec);

  // True when the pipeline is iterating over the current (simple) filter
  // to produce composite output. In this case, ExecuteDataStart() should
  // NOT Initialize() the composite output.
  if (!this->InLocalLoop)
    {
    // Prepare outputs that will be generated to receive new data.
    for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
      {
      vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
      vtkDataObject* data = 
        outInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
      if(data && !outInfo->Get(DATA_NOT_GENERATED()))
        {
        data->PrepareForNewData();
        data->CopyInformationFromPipeline(request);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Handle REQUEST_DATA
int vtkCompositeDataPipeline::ExecuteData(vtkInformation* request,
                                          vtkInformationVector** inInfoVec,
                                          vtkInformationVector* outInfoVec)
{
  int result = 1;
  int outputPort = request->Get(FROM_OUTPUT_PORT());
  int i, j;

  for(i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outInfoVec->GetInformationObject(i);
    info->Remove(UPDATE_BLOCKS());
    }

  // Loop over all input ports.
  for(i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
    // Loop over all connections on this input port.
    int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
    for (j=0; j<numInConnections; j++)
      {
      // If the algorithm connected to the input port did not provide
      // a composite dataset, it is a "simple" algorithm. Every block
      // that consists of a group of simple algorithms has to be executed
      // in a loop to process composite datasets. This is done by a
      // collaboration of the composite source and algorithm that surround
      // this simple algorithm block.
      vtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
      vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
        inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));

      // There is a composite input, no need to loop
      if (input)
        {
        continue;
        }

      vtkMultiGroupDataSet* updateInfo = 
        vtkMultiGroupDataSet::SafeDownCast(
          inInfo->Get(vtkCompositeDataPipeline::UPDATE_BLOCKS()));

      // Tell the producer upstream that looping is about to start
      int retVal = this->SendBeginLoop(i, j, inInfo, updateInfo);
      if (retVal == vtkCompositeDataPipeline::EXECUTE_BLOCK_CONTINUE)
        {
        continue;
        }
      if (retVal == vtkCompositeDataPipeline::EXECUTE_BLOCK_ERROR)
        {
        return 0;
        }

      input = this->CreateInputCompositeData(i, inInfo);

      this->UpdateBlocks(i, j, outputPort, updateInfo, input, inInfo);

      if (!this->SendEndLoop(i, j))
        {
        return 0;
        }
      }
    }

  int inputPortIsComposite;
  int inputIsComposite;
  int compositePort;
  this->CheckInputPorts(inputPortIsComposite, inputIsComposite, compositePort);
  if (inputIsComposite && !inputPortIsComposite)
    {
    this->ExecuteSimpleAlgorithm(
      request, inInfoVec, outInfoVec, compositePort);
    }
  else
    {
    result = this->Superclass::ExecuteData(request,inInfoVec,outInfoVec);
    }

  for(int jj=0; jj < this->Algorithm->GetNumberOfOutputPorts(); ++jj)
    {
    vtkInformation* info = this->GetOutputInformation(jj);
    vtkObject* dobj= info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
    if (dobj)
      {
      info->Set(UPDATE_BLOCKS(), dobj);
      }
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::InputTypeIsValid(
  int port, int index,vtkInformationVector **inInfoVec)
{
  if (this->InLocalLoop || this->InSubPass)
    {
    return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
    }
  if (!inInfoVec[port])
    {
    return 0;
    }  
  vtkDataObject* input = this->GetCompositeInputData(port, index, inInfoVec);
  if (!input)
    {
    return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
    }
  vtkInformation* info = this->Algorithm->GetInputPortInformation(port);

  // Enforce required type, if any.
  if(const char* dt = info->Get(
       vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE()))
    {
    if (!input->IsA(dt))
      {
      vtkErrorMacro("Input for connection index " << index
                    << " on input port index " << port
                    << " for algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ") is of type "
                    << input->GetClassName() << ", but a " << dt
                    << " is required.");
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::CheckInputPorts(int& inputPortIsComposite,
                                               int& inputIsComposite,
                                               int& compositePort)
{
  inputPortIsComposite = 0;
  inputIsComposite = 0;
  compositePort = -1;
  // Loop over all input ports and check if any of them asks
  // for composite data and has composite data. 
  // Find the first port that asks for composite data and that
  // has composite data and break.
  // TODO: This should be changed to handle multiple ports that are
  // a mix of composite and simple data.
  for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
    vtkInformation* inPortInfo = 
      this->Algorithm->GetInputPortInformation(i);
    if (inPortInfo->Has(INPUT_REQUIRED_COMPOSITE_DATA_TYPE()))
      {
      inputPortIsComposite = 1;
      }
    int numInConnections = this->Algorithm->GetNumberOfInputConnections(i);
    if (numInConnections > 0)
      {
      vtkInformation* inInfo = this->GetInputInformation(i, 0);
      if (inInfo->Has(vtkCompositeDataSet::COMPOSITE_DATA_SET()))
        {
        inputIsComposite = 1;
        compositePort = i;
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::UpdateBlocks(
  int i, int j, int outputPort, 
  vtkMultiGroupDataSet* updateInfo, 
  vtkCompositeDataSet* input,
  vtkInformation* inInfo)
{
  // Get the producer and its output port for input connection i,j.
  vtkExecutive* producer;
  int producerPort;
  if(vtkInformation* info = this->GetInputInformation(i, j))
    {
    // Get the executive producing this input.  If there is none, then
    // it is a NULL input.
    info->Get(vtkExecutive::PRODUCER(), producer, producerPort);
    }
  if(!producer)
    {
    this->SendEndLoop(i,j);
    return EXECUTE_BLOCK_ERROR;
    }

  // Execute the streaming demand driven pipeline for each block
  unsigned int numGroups = updateInfo->GetNumberOfGroups();
  for (unsigned int k=0; k<numGroups; k++)
    {
    unsigned int numDataSets = updateInfo->GetNumberOfDataSets(k);
    for (unsigned l=0; l<numDataSets; l++)
      {
      if (!updateInfo->GetDataSet(k,l))
        {
        vtkDebugMacro(<< k << "," << l << "  not marked for update");
        continue;
        }
      // First pipeline mtime
              
      // Setup the request for pipeline modification time.
      this->GenericRequest->Set(vtkMultiGroupDataSet::GROUP(), k);
      this->GenericRequest->Set(vtkCompositeDataSet::INDEX(), l);
      unsigned long mtime;
      producer->ComputePipelineMTime(this->GenericRequest,
                                     producer->GetInputInformation(),
                                     producer->GetOutputInformation(),
                                     producerPort, &mtime);

      // Do the data-object creation pass before the information pass.
              
      // Send the request for data object creation.
      this->DataObjectRequest->Set(vtkMultiGroupDataSet::GROUP(), k);
      this->DataObjectRequest->Set(vtkCompositeDataSet::INDEX(), l);
      if (!this->ForwardUpstream(i, j, this->DataObjectRequest))
        {
        this->SendEndLoop(i,j);
        return EXECUTE_BLOCK_ERROR;
        }
              
      // Send the request for information.
      this->InformationRequest->Set(vtkMultiGroupDataSet::GROUP(), k);
      this->InformationRequest->Set(vtkCompositeDataSet::INDEX(), l);
      if (!this->ForwardUpstream(i, j, this->InformationRequest))
        {
        this->SendEndLoop(i,j);
        return EXECUTE_BLOCK_ERROR;
        }
            
      // Update the whole thing
      // TODO: This might change
      if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
        {
        int extent[6] = {0,-1,0,-1,0,-1};
        inInfo->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);
        inInfo->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
        inInfo->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
        inInfo->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
        inInfo->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
        }
            
      // Send the request for update extent propagation.
      this->UpdateExtentRequest->Set(vtkMultiGroupDataSet::GROUP(), k);
      this->UpdateExtentRequest->Set(vtkCompositeDataSet::INDEX(), l);
      this->UpdateExtentRequest->Set(
        vtkExecutive::FROM_OUTPUT_PORT(), outputPort);
      if (!this->ForwardUpstream(i, j, this->UpdateExtentRequest))
        {
        this->SendEndLoop(i,j);
        return EXECUTE_BLOCK_ERROR;
        }
            
      // Send the request for data.
      this->DataRequest->Set(vtkMultiGroupDataSet::GROUP(), k);
      this->DataRequest->Set(vtkCompositeDataSet::INDEX(), l);
      this->DataRequest->Set(FROM_OUTPUT_PORT(), outputPort);
              
      // Send the request.
      if (!this->ForwardUpstream(i, j, this->DataRequest))
        {
        this->SendEndLoop(i,j);
        return EXECUTE_BLOCK_ERROR;
        }
              
      vtkDataObject* block = inInfo->Get(vtkDataObject::DATA_OBJECT());
      if (block && input)
        {
        vtkDataObject* blockCopy = block->NewInstance();
        blockCopy->ShallowCopy(block);
        input->AddDataSet(this->DataRequest, blockCopy);
        blockCopy->Delete();
        }
      }
    }
  return EXECUTE_BLOCK_OK;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::SendBeginLoop(
  int i, int j, vtkInformation* inInfo, vtkMultiGroupDataSet* updateInfo)
{

  // Tell the producer upstream that looping is about to start
  this->GenericRequest->Set(BEGIN_LOOP(), 1);
        
  // The request is forwarded upstream through the pipeline.
  this->GenericRequest->Set(
    vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  
  this->GenericRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
        
  // Send the request.
  if (!this->ForwardUpstream(i, j, this->GenericRequest))
    {
    this->GenericRequest->Remove(BEGIN_LOOP());
    return vtkCompositeDataPipeline::EXECUTE_BLOCK_ERROR;
    }

  if (!updateInfo)
    {
    // This means no composite data producer was found upstream.
    vtkDebugMacro(<< "No UPDATE_BLOCKS() for input " << i << "," << j
                  << " was provided. Skipping.");
    this->SendEndLoop(i,j);
    this->GenericRequest->Remove(BEGIN_LOOP());
    return vtkCompositeDataPipeline::EXECUTE_BLOCK_CONTINUE;
    }

  // If the input is up-to-date (not MARKED_FOR_UPDATE() by ProcessRequest ->
  // REQUEST_DATA()), there is no need to loop the input. If the input is
  // looped, it will cause unnecessary execution(s)
  if (!inInfo->Has(MARKED_FOR_UPDATE()))
    {
    this->SendEndLoop(i,j);
    this->GenericRequest->Remove(BEGIN_LOOP());
    return vtkCompositeDataPipeline::EXECUTE_BLOCK_CONTINUE;
    }
  this->GenericRequest->Remove(BEGIN_LOOP());

  return vtkCompositeDataPipeline::EXECUTE_BLOCK_OK;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::SendEndLoop(int i, int j)
{
  // Tell the producer upstream that looping is over
  vtkSmartPointer<vtkInformation> re = 
    vtkSmartPointer<vtkInformation>::New();
  re->Set(END_LOOP(), 1);
  
  // The request is forwarded upstream through the pipeline.
  re->Set(vtkExecutive::FORWARD_DIRECTION(), 
          vtkExecutive::RequestUpstream);
  
  // Send the request.
  if (!this->ForwardUpstream(i, j, re))
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::ExecuteSimpleAlgorithm(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec,
  int compositePort)
{
  int outputInitialized = 0;

  this->ExecuteDataStart(request,inInfoVec,outInfoVec);

  vtkInformation* outInfo = 0;
  vtkSmartPointer<vtkDataObject> prevOutput;

  if (this->GetNumberOfOutputPorts() > 0)
    {
    outInfo = outInfoVec->GetInformationObject(0);
    }

  // Make sure a valid composite data object exists for all output ports.
  for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    this->CheckCompositeData(i, outInfoVec);
    }

  // Loop using the first input on the first port.
  // This might not be valid for all cases but it is a decent
  // assumption to start with.
  // TODO: Loop over all inputs
  vtkInformation* inInfo = this->GetInputInformation(compositePort, 0);
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));

  vtkMultiGroupDataSet* updateInfo = 
    vtkMultiGroupDataSet::SafeDownCast(
      inInfo->Get(vtkCompositeDataPipeline::UPDATE_BLOCKS()));

  vtkCompositeDataSet* output = 
    vtkCompositeDataSet::SafeDownCast(
      outInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));

  prevOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input && updateInfo)
    {
    vtkSmartPointer<vtkInformation> r = 
      vtkSmartPointer<vtkInformation>::New();

    r->Set(FROM_OUTPUT_PORT(), inInfo->GetPort(PRODUCER()));

    // The request is forwarded upstream through the pipeline.
    r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
        
    // Algorithms process this request after it is forwarded.
    r->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

    unsigned int numGroups = updateInfo->GetNumberOfGroups();
    vtkSmartPointer<vtkDataObject> prevInput = 
      inInfo->Get(vtkDataObject::DATA_OBJECT());

    vtkDebugMacro("EXECUTING: " << this->Algorithm->GetClassName());

    // True when the pipeline is iterating over the current (simple)
    // filter to produce composite output. In this case,
    // ExecuteDataStart() should NOT Initialize() the composite output.
    this->InLocalLoop = 1;

    // Store the information (whole_extent and maximum_number_of_pieces)
    // before looping. Otherwise, executeinformation will cause
    // changes (because we pretend that the max. number of pieces is
    // one to process the whole block)
    this->PushInformation(inInfo);
    for (unsigned int k=0; k<numGroups; k++)
      {
      unsigned int numDataSets = updateInfo->GetNumberOfDataSets(k);
      for (unsigned l=0; l<numDataSets; l++)
        {
        if (updateInfo->GetDataSet(k,l))
          {
          r->Set(vtkMultiGroupDataSet::GROUP(), k);
          r->Set(vtkCompositeDataSet::INDEX(),    l);
          vtkDataObject* dobj = input->GetDataSet(r);
          // There must be a bug somehwere. If this Remove()
          // is not called, the following Set() has the effect
          // of removing (!) the key.
          inInfo->Remove(vtkDataObject::DATA_OBJECT());
          inInfo->Set(vtkDataObject::DATA_OBJECT(), dobj);

          // Process the whole dataset
          this->CopyFromDataToInformation(dobj, inInfo);

          r->Set(REQUEST_DATA_OBJECT());
          this->Superclass::ExecuteDataObject
            (r,this->GetInputInformation(),this->GetOutputInformation());
          r->Remove(REQUEST_DATA_OBJECT());

          r->Set(REQUEST_INFORMATION());
          // Make sure that pipeline informations is in sync with the data
          dobj->CopyInformationToPipeline(r, 0, inInfo, 1);
          // This should not be needed but since a lot of image filters do:
          // img->GetScalarType(), it is necessary.
          dobj->GetProducerPort(); // make sure there is pipeline info.
          dobj->CopyInformationToPipeline(
            r, 0, dobj->GetPipelineInformation(), 1);
          this->Superclass::ExecuteInformation(r,inInfoVec,outInfoVec);
          r->Remove(REQUEST_INFORMATION());

          for(int m=0; m < this->Algorithm->GetNumberOfOutputPorts(); ++m)
            {
            vtkInformation* info = this->GetOutputInformation(m);

            // Update the whole thing
            // TODO: This might change
            if (info->Has(
                  vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
              {
              int extent[6] = {0,-1,0,-1,0,-1};
              info->Get(
                vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                extent);
              info->Set(
                vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 
                extent, 
                6);
              info->Set(
                vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 
                1);
              info->Set(
                vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 
                1);
              info->Set(
                vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
              }
            }

          r->Set(REQUEST_UPDATE_EXTENT());
          this->CallAlgorithm(r, vtkExecutive::RequestUpstream,
                              inInfoVec, outInfoVec);
          this->ForwardUpstream(r);
          r->Remove(REQUEST_UPDATE_EXTENT());

          r->Set(REQUEST_DATA());
          this->Superclass::ExecuteData(r,inInfoVec,outInfoVec);
          r->Remove(REQUEST_DATA());

          if (output && outInfo)
            {
            if (!outputInitialized)
              {
              output->PrepareForNewData();
              outputInitialized = 1;
              }
            vtkDataObject* tmpOutput = 
              outInfo->Get(vtkDataObject::DATA_OBJECT());
            vtkDataObject* outputCopy = tmpOutput->NewInstance();
            outputCopy->ShallowCopy(tmpOutput);
            output->AddDataSet(r, outputCopy);
            outputCopy->Delete();
            }
          }
        }
      }
    // True when the pipeline is iterating over the current (simple)
    // filter to produce composite output. In this case,
    // ExecuteDataStart() should NOT Initialize() the composite output.
    this->InLocalLoop = 0;
    // Restore the extent information and force it to be
    // copied to the output. Composite sources should set
    // MAXIMUM_NUMBER_OF_PIECES to -1 anyway (and handle
    // piece requests properly).
    this->PopInformation(inInfo);
    r->Set(REQUEST_INFORMATION());
    this->CopyDefaultInformation(r, vtkExecutive::RequestDownstream,
                                 this->GetInputInformation(),
                                 this->GetOutputInformation());
        
    vtkDataObject* curInput = inInfo->Get(vtkDataObject::DATA_OBJECT());
    if (curInput != prevInput)
      {
      inInfo->Remove(vtkDataObject::DATA_OBJECT());
      inInfo->Set(vtkDataObject::DATA_OBJECT(), prevInput);
      }
    vtkDataObject* curOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
    if (curOutput != prevOutput)
      {
      prevOutput->SetPipelineInformation(outInfo);
      }
    }
  this->ExecuteDataEnd(request,inInfoVec,outInfoVec);
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ExecuteDataForBlock(vtkInformation* request)
{
  vtkInformationVector* outputVector = this->GetOutputInformation();
  int numOut = outputVector->GetNumberOfInformationObjects();
  for (int i=0; i<numOut; i++)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    
    vtkDataObject* doOutput = 
      info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);
    
    if (output)
      {
      vtkDataObject* dobj = output->GetDataSet(request);
      if (dobj)
        {
        vtkDataObject* dobjCopy = 
          info->Get(vtkDataObject::DATA_OBJECT());
        
        if (dobj && dobjCopy)
          {
          dobjCopy->ShallowCopy(dobj);
          }
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::MarkOutputsGenerated
(vtkInformation* request,
 vtkInformationVector** inInfoVec,
 vtkInformationVector* outputs)                    
{
  this->Superclass::MarkOutputsGenerated(request, inInfoVec, outputs);

  // Tell all generated outputs that they have been generated.
  for(int i=0; i < outputs->GetNumberOfInformationObjects(); ++i)
    {
    vtkInformation* outInfo = outputs->GetInformationObject(i);
    vtkDataObject* data = outInfo->Get(
      vtkCompositeDataSet::COMPOSITE_DATA_SET());
    if(data && !outInfo->Get(DATA_NOT_GENERATED()))
      {
      data->DataHasBeenGenerated();
      }
    }
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::NeedToExecuteData(
  int outputPort,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  // Has the algorithm asked to be executed again?
  if(this->ContinueExecuting)
    {
    return 1;
    }

  // If no port is specified, check all ports.  This behavior is
  // implemented by the superclass.
  if(outputPort < 0)
    {
    return this->Superclass::NeedToExecuteData(outputPort,
                                               inInfoVec,outInfoVec);
    }

  // Does the superclass want to execute?
  if(this->vtkDemandDrivenPipeline::NeedToExecuteData(
       outputPort,inInfoVec,outInfoVec))
    {
    return 1;
    }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  vtkDataObject* dataObject = outInfo->Get(
    vtkCompositeDataSet::COMPOSITE_DATA_SET());
  if (!dataObject)
    {
    return this->Superclass::NeedToExecuteData(outputPort,
                                               inInfoVec,outInfoVec);
    }
  vtkInformation* dataInfo = dataObject->GetInformation();

  // Check the unstructured extent.  If we do not have the requested
  // piece, we need to execute.
  int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
  int dataNumberOfPieces = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  if(dataNumberOfPieces != updateNumberOfPieces)
    {
    return 1;
    }
  if (dataNumberOfPieces != 1)
    {
    int dataPiece = dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER());
    int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());
    if (dataPiece != updatePiece)
      {
      return 1;
      }
    }

  // if we are requesting a particular update time index, check
  // if we have the desired time index
  if ( outInfo->Has(UPDATE_TIME_INDEX()) )
    {
    if (!dataInfo->Has(vtkDataObject::DATA_TIME_INDEX()) ||
      dataInfo->Get(vtkDataObject::DATA_TIME_INDEX()) !=
      outInfo->Get(UPDATE_TIME_INDEX()))
      {
      return 1;
      }
    }

  // We do not need to execute.
  return 0;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ForwardUpstream(vtkInformation* request)
{
  return this->Superclass::ForwardUpstream(request);
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ForwardUpstream(
  int i, int j, vtkInformation* request)
{
  // Do not forward upstream if input information is shared.
  if(this->SharedInputInformation)
    {
    return 1;
    }
  int result = 1;
  if(vtkExecutive* e = this->GetInputExecutive(i, j))
    {
    vtkAlgorithmOutput* input = this->Algorithm->GetInputConnection(i, j);
    int port = request->Get(FROM_OUTPUT_PORT());
    request->Set(FROM_OUTPUT_PORT(), input->GetIndex());
    if(!e->ProcessRequest(request,
                          e->GetInputInformation(),
                          e->GetOutputInformation()))
      {
      result = 0;
      }
    request->Set(FROM_OUTPUT_PORT(), port);
    }
  return result;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::CopyDefaultInformation(
  vtkInformation* request, int direction,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  int hasUpdateBlocks = 0;
  if(direction == vtkExecutive::RequestDownstream)
    {
    vtkInformationKey** keys = request->Get(vtkExecutive::KEYS_TO_COPY());
    if (keys)
      {
      int len = request->Length(vtkExecutive::KEYS_TO_COPY());
      for (int i=0; i<len; i++)
        {
        if (keys[i] == vtkCompositeDataPipeline::UPDATE_BLOCKS())
          {
          hasUpdateBlocks = 1;
          break;
          }
        }
      if (hasUpdateBlocks)
        {
        request->Remove(vtkExecutive::KEYS_TO_COPY(),
                        vtkCompositeDataPipeline::UPDATE_BLOCKS());
        }
      }
    }

  this->Superclass::CopyDefaultInformation(request, direction, 
                                           inInfoVec, outInfoVec);

  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    int inputPortIsComposite;
    int inputIsComposite;
    int compositePort;
    this->CheckInputPorts(
      inputPortIsComposite, inputIsComposite, compositePort);
    if (inputPortIsComposite)
      {
      // Get the output port from which to copy the extent.
      int outputPort = -1;
      if(request->Has(FROM_OUTPUT_PORT()))
        {
        outputPort = request->Get(FROM_OUTPUT_PORT());
        }
      
      // Setup default information for the inputs.
      if(outInfoVec->GetNumberOfInformationObjects() > 0)
        {
        // Copy information from the output port that made the request.
        // Since VerifyOutputInformation has already been called we know
        // there is output information with a data object.
        vtkInformation* outInfo =
          outInfoVec->GetInformationObject((outputPort >= 0)? outputPort : 0);
        // Loop over all input ports.
        for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
          {
          // Loop over all connections on this input port.
          int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
          for (int j=0; j<numInConnections; j++)
            {
            // Get the pipeline information for this input connection.
            vtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
            
            inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
            inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
            inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
            inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
            }
          }
        }
      }
    }

  if (hasUpdateBlocks)
    {
    request->Append(vtkExecutive::KEYS_TO_COPY(), 
                    vtkCompositeDataPipeline::UPDATE_BLOCKS());
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::CopyFromDataToInformation(
  vtkDataObject* dobj, vtkInformation* inInfo)
{
  if (dobj->IsA("vtkImageData"))
    {
    inInfo->Set(
      WHOLE_EXTENT(), static_cast<vtkImageData*>(dobj)->GetExtent(), 6);
    }
  else if (dobj->IsA("vtkStructuredGrid"))
    {
    inInfo->Set(
      WHOLE_EXTENT(), static_cast<vtkStructuredGrid*>(dobj)->GetExtent(), 6);
    }
  else if (dobj->IsA("vtkRectilinearGrid"))
    {
    inInfo->Set(
      WHOLE_EXTENT(), static_cast<vtkRectilinearGrid*>(dobj)->GetExtent(), 6);
    }
  else if (dobj->IsA("vtkUniformGrid"))
    {
    inInfo->Set(
      WHOLE_EXTENT(), static_cast<vtkUniformGrid*>(dobj)->GetExtent(), 6);
    }
  else
    {
    inInfo->Set(MAXIMUM_NUMBER_OF_PIECES(), 1);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PushInformation(vtkInformation* inInfo)
{
  this->InformationCache->CopyEntry(inInfo, WHOLE_EXTENT());
  this->InformationCache->CopyEntry(inInfo, MAXIMUM_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PopInformation(vtkInformation* inInfo)
{
  inInfo->CopyEntry(this->InformationCache, WHOLE_EXTENT());
  inInfo->CopyEntry(this->InformationCache, MAXIMUM_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataPipeline::CreateInputCompositeData(
  int i, vtkInformation* inInfo)
{
  vtkCompositeDataSet* input = 0;

  // If the input requires a composite dataset and one is not available,
  // create it. This means that there are "simple" filters between the
  // composite data producer and this filter
  vtkInformation* inPortInfo = 
    this->Algorithm->GetInputPortInformation(i);
  const char* dt = 
    inPortInfo->Get(INPUT_REQUIRED_COMPOSITE_DATA_TYPE());
  if (dt)
    {
    if (strcmp(dt, "vtkCompositeDataSet") == 0)
      {
      // If vtkCompositeDataSet is specified, the algorithm
      // will work with all sub-classes. Create a vtkMultiGroupDataSet
      dt = "vtkMultiGroupDataSet";
      }
    // If the composite data input to the algorithm is not
    // set, create and assign it. This happens when the producer
    // of the input data actually produces a simple data object
    // (in a loop)
    vtkDataObject* dobj = this->NewDataObject(dt);
    if (dobj)
      {
      dobj->SetPipelineInformation(inInfo);
      input = vtkCompositeDataSet::SafeDownCast(dobj);
      dobj->Delete();
      }
    else
      {
      vtkErrorMacro("Cannot instantiate " << dt
                    << ". The INPUT_REQUIRED_COMPOSITE_DATA_TYPE() of "
                    << this->Algorithm->GetClassName()
                    << " is not set properly.");
      
      }
    }
  return input;
}
            

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::CheckDataObject(
  int port, vtkInformationVector *outInfoVec)
{
  // Check that the given output port has a valid data object.
  vtkInformation* outInfo =
    outInfoVec->GetInformationObject(port);
  vtkDataObject* data = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* portInfo = this->Algorithm->GetOutputPortInformation(port);
  if(const char* dt = portInfo->Get(vtkDataObject::DATA_TYPE_NAME()))
    {
    // The output port specifies a data type.  Make sure the data
    // object exists and is of the right type.
    if(!data || !data->IsA(dt))
      {
      // Try to create an instance of the correct type.
      data = this->NewDataObject(dt);
      this->SetOutputData(port, data, outInfo);
      if(data)
        {
        data->Delete();
        }
      }
    if(!data)
      {
      // The algorithm has a bug and did not create the data object.
      vtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "("
                    << this->Algorithm
                    << ") did not create output for port " << port
                    << " when asked by REQUEST_DATA_OBJECT and does not"
                    << " specify a concrete DATA_TYPE_NAME.");
      return 0;
      }
    return 1;
    }
  else if(data || outInfo->Has(vtkCompositeDataSet::COMPOSITE_DATA_SET()))
    {
    // The algorithm did not specify its output data type.  Just assume
    // the data object is of the correct type.
    return 1;
    }
  else
    {
    // The algorithm did not specify its output data type and no
    // object exists.
    vtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "("
                  << this->Algorithm
                  << ") did not create output for port " << port
                  << " when asked by REQUEST_DATA_OBJECT and does not"
                  << " specify any DATA_TYPE_NAME.");
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::CheckCompositeData(
  int port, vtkInformationVector* outInfoVec)
{
  // Check that the given output port has a valid data object.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(port);
  vtkDataObject* data = 
    outInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkInformation* portInfo = this->Algorithm->GetOutputPortInformation(port);
  if (const char* dt = portInfo->Get(COMPOSITE_DATA_TYPE_NAME()))
    {
    if(!data || !data->IsA(dt))
      {
      // Try to create an instance of the correct type.
      data = this->NewDataObject(dt);
      data->SetPipelineInformation(outInfo);
      if(data)
        {
        data->Delete();
        }
      }
    }

  // If this is a simple filter but has composite input, create a composite
  // output.
  int inputPortIsComposite;
  int inputIsComposite;
  int compositePort;
  this->CheckInputPorts(inputPortIsComposite, inputIsComposite, compositePort);
  if (inputIsComposite && !inputPortIsComposite)
    {
    // This assumes that the filter has one output.
    vtkDataObject* doOutput = 
      outInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);
    if (!output)
      {
      output = vtkMultiGroupDataSet::New();
      output->SetPipelineInformation(outInfo);
      output->Delete();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataPipeline::GetCompositeInputData(
  int port, int index, vtkInformationVector **inInfoVec)
{
  if (!inInfoVec[port])
    {
    return 0;
    }
  vtkInformation *info = inInfoVec[port]->GetInformationObject(index);
  if (!info)
    {
    return 0;
    }
  return info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataPipeline::GetCompositeOutputData(int port)
{
  if(!this->OutputPortIndexInRange(port, "get data for"))
    {
    return 0;
    }

  // Check that the given output port has a valid data object.
  this->CheckCompositeData(port, this->GetOutputInformation());

  // Return the data object.
  if(vtkInformation* info = this->GetOutputInformation(port))
    {
    return info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

