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
#include "vtkCompositeDataIterator.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTrivialProducer.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro(vtkCompositeDataPipeline);

vtkInformationKeyMacro(vtkCompositeDataPipeline, LOAD_REQUESTED_BLOCKS, Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline, COMPOSITE_DATA_META_DATA, ObjectBase);
vtkInformationKeyMacro(vtkCompositeDataPipeline, UPDATE_COMPOSITE_INDICES, IntegerVector);
vtkInformationKeyMacro(vtkCompositeDataPipeline, DATA_COMPOSITE_INDICES, IntegerVector);
vtkInformationKeyMacro(vtkCompositeDataPipeline, SUPPRESS_RESET_PI, Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline, BLOCK_AMOUNT_OF_DETAIL,Double);


//----------------------------------------------------------------------------
vtkCompositeDataPipeline::vtkCompositeDataPipeline()
{
  this->InLocalLoop = 0;
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
int vtkCompositeDataPipeline::ExecuteDataObject(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  vtkDebugMacro(<< "ExecuteDataObject");
  int result=1;

  // If the input is composite, allow algorithm to handle
  // REQUEST_DATA_OBJECT only if it can handle composite
  // datasets. Otherwise, the algorithm will get a chance to handle
  // REQUEST_DATA_OBJECT when it is being iterated over.
  int compositePort;
  bool shouldIterate = this->ShouldIterateOverInput(inInfoVec, compositePort);
  if (!shouldIterate)
  {
    // Invoke the request on the algorithm.
    result = this->CallAlgorithm(request, vtkExecutive::RequestDownstream,
                                 inInfoVec, outInfoVec);
    if (!result)
    {
      return result;
    }
  }

  int i;
  // Make sure a valid data object exists for all output ports.
  for(i=0; result && i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
  {
    vtkDebugMacro(<< "ExecuteDataObject calling CheckCompositeData");
    result = this->CheckCompositeData(request, i, inInfoVec, outInfoVec);
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::ExecuteDataStart(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::ExecuteDataStart(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
// Handle REQUEST_DATA
int vtkCompositeDataPipeline::ExecuteData(vtkInformation* request,
                                          vtkInformationVector** inInfoVec,
                                          vtkInformationVector* outInfoVec)
{
  vtkDebugMacro(<< "ExecuteData");
  int result = 1;

  int compositePort;
  bool composite = this->ShouldIterateOverInput(inInfoVec, compositePort);

  if ( composite)
  {
    if (this->GetNumberOfOutputPorts())
    {
      this->ExecuteSimpleAlgorithm(request, inInfoVec, outInfoVec, compositePort);
    }
    else
    {
       vtkErrorMacro("Can not execute simple alorithm without output ports");
       return 0;
    }
  }
  else
  {
    vtkDebugMacro(<< "  Superclass::ExecuteData");
    result = this->Superclass::ExecuteData(request,inInfoVec,outInfoVec);
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::InputTypeIsValid(
  int port, int index,vtkInformationVector **inInfoVec)
{
  if (this->InLocalLoop)
  {
    return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
  }
  if (!inInfoVec[port])
  {
    return 0;
  }

  // If we will be iterating over the input on this port, assume that we
  // can handle any input type. The input type will be checked again during
  // each step of the iteration.
  int compositePort;
  if (this->ShouldIterateOverInput(inInfoVec, compositePort))
  {
    if (compositePort == port)
    {
      return 1;
    }
  }

  // Otherwise, let superclass handle it.
  return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
}

//----------------------------------------------------------------------------
bool vtkCompositeDataPipeline::ShouldIterateOverInput(vtkInformationVector** inInfoVec,
                                                      int& compositePort)
{
  compositePort = -1;
  // Find the first input that has a composite data that does not match
  // the required input type. We assume that that port input has to
  // be iterated over. We also require that this port has only one
  // connection.
  int numInputPorts = this->Algorithm->GetNumberOfInputPorts();
  for(int i=0; i < numInputPorts; ++i)
  {
    int numInConnections = this->Algorithm->GetNumberOfInputConnections(i);
    // If there is 1 connection
    if (numInConnections == 1)
    {
      vtkInformation* inPortInfo =
        this->Algorithm->GetInputPortInformation(i);
      if (inPortInfo->Has(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE())
          && inPortInfo->Length(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) > 0)
      {
        const char* inputType = inPortInfo->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), 0);
        // the filter upstream will iterate

        if (strcmp(inputType, "vtkCompositeDataSet") == 0 ||
            strcmp(inputType, "vtkDataObjectTree") == 0 ||
            strcmp(inputType, "vtkHierarchicalBoxDataSet") == 0 ||
            strcmp(inputType, "vtkOverlappingAMR") == 0 ||
            strcmp(inputType, "vtkNonOverlappingAMR") == 0 ||
            strcmp(inputType, "vtkMultiBlockDataSet") == 0)
        {
          vtkDebugMacro(<< "ShouldIterateOverInput return 0 (Composite)");
          return false;
        }

        vtkInformation* inInfo = inInfoVec[i]->GetInformationObject(0);
        vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
        // If input does not match a required input type
        bool foundMatch = false;
        if(input)
        {
          int size = inPortInfo->Length(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
          for(int j = 0; j < size; ++j)
          {
            if(input->IsA(inPortInfo->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), j)))
            {
              foundMatch = true;
            }
          }
        }
        if (input && !foundMatch)
        {
          // If input is composite
          if (vtkCompositeDataSet::SafeDownCast(input))
          {
            // Assume that we have to iterate over input
            compositePort = i;
            vtkDebugMacro(<< "ShouldIterateOverInput returns 1 (input composite)");
            return true;
          }
        }
      }
    }
  }
  vtkDebugMacro(<< "ShouldIterateOverInput returns 0 (default)");
  return false;
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::ExecuteEach(vtkCompositeDataIterator* iter,
                                           vtkInformationVector** inInfoVec,
                                           vtkInformationVector* outInfoVec,
                                           int compositePort,
                                           int connection,
                                           vtkInformation* request,
                                           vtkCompositeDataSet* compositeOutput)
{
  vtkInformation* inInfo  =inInfoVec[compositePort]->GetInformationObject(connection);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0); //assumed to be 0

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    if (dobj)
    {
      // Note that since VisitOnlyLeaves is ON on the iterator,
      // this method is called only for leaves, hence, we are assured that
      // neither dobj nor outObj are vtkCompositeDataSet subclasses.
      vtkDataObject* outObj =
        this->ExecuteSimpleAlgorithmForBlock(inInfoVec,
                                             outInfoVec,
                                             inInfo,
                                             outInfo,
                                             request,
                                             dobj);
      if (outObj)
      {
        compositeOutput->SetDataSet(iter, outObj);
        outObj->FastDelete();
      }
    }
  }
}

//----------------------------------------------------------------------------
// Execute a simple (non-composite-aware) filter multiple times, once per
// block. Collect the result in a composite dataset that is of the same
// structure as the input.
void vtkCompositeDataPipeline::ExecuteSimpleAlgorithm(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec,
  int compositePort)
{
  vtkDebugMacro(<< "ExecuteSimpleAlgorithm");

  this->ExecuteDataStart(request,inInfoVec,outInfoVec);

  vtkInformation* outInfo = 0;

  if (this->GetNumberOfOutputPorts() > 0)
  {
    outInfo = outInfoVec->GetInformationObject(0);
  }
  if (!outInfo)
  {
    return;
  }

  // Make sure a valid composite data object exists for all output ports.
  for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
  {
    this->CheckCompositeData(request, i, inInfoVec, outInfoVec);
  }

  // if we have no composite inputs
  if (compositePort==-1)
  {
    return;
  }

  // Loop using the first input on the first port.
  // This might not be valid for all cases but it is a decent
  // assumption to start with.
  // TODO: Loop over all inputs
  vtkInformation* inInfo = this->GetInputInformation(compositePort, 0);
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkCompositeDataSet> compositeOutput =
    vtkCompositeDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input && compositeOutput)
  {
    compositeOutput->PrepareForNewData();
    compositeOutput->CopyStructure(input);
    if (input && input->GetFieldData())
    {
      compositeOutput->GetFieldData()->PassData(input->GetFieldData());
    }

    vtkSmartPointer<vtkInformation> r =
      vtkSmartPointer<vtkInformation>::New();

    r->Set(FROM_OUTPUT_PORT(), PRODUCER()->GetPort(outInfo));

    // The request is forwarded upstream through the pipeline.
    r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);

    // Algorithms process this request after it is forwarded.
    r->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);


    // Store the information (whole_extent)
    // before looping. Otherwise, executeinformation will cause
    // changes (because we pretend that the max. number of pieces is
    // one to process the whole block)
    this->PushInformation(inInfo);

    vtkDebugMacro(<< "EXECUTING " << this->Algorithm->GetClassName());;

    // True when the pipeline is iterating over the current (simple)
    // filter to produce composite output. In this case,
    // ExecuteDataStart() should NOT Initialize() the composite output.
    this->InLocalLoop = 1;

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());
    this->ExecuteEach(iter, inInfoVec, outInfoVec, compositePort, 0, r,compositeOutput);

    // True when the pipeline is iterating over the current (simple)
    // filter to produce composite output. In this case,
    // ExecuteDataStart() should NOT Initialize() the composite output.
    this->InLocalLoop = 0;
    // Restore the extent information and force it to be
    // copied to the output.
    this->PopInformation(inInfo);
    r->Set(REQUEST_INFORMATION());
    this->CopyDefaultInformation(r, vtkExecutive::RequestDownstream,
                                 this->GetInputInformation(),
                                 this->GetOutputInformation());

    vtkDataObject* curInput = inInfo->Get(vtkDataObject::DATA_OBJECT());
    if (curInput != input)
    {
      inInfo->Remove(vtkDataObject::DATA_OBJECT());
      inInfo->Set(vtkDataObject::DATA_OBJECT(), input);
    }
    vtkDataObject* curOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
    if (curOutput != compositeOutput.GetPointer())
    {
      outInfo->Set(vtkDataObject::DATA_OBJECT(), compositeOutput);
    }
  }
  this->ExecuteDataEnd(request,inInfoVec,outInfoVec);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataPipeline::ExecuteSimpleAlgorithmForBlock(
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec,
  vtkInformation* inInfo,
  vtkInformation* outInfo,
  vtkInformation* request,
  vtkDataObject* dobj)
{
  vtkDebugMacro(<< "ExecuteSimpleAlgorithmForBlock");

  if (dobj && dobj->IsA("vtkCompositeDataSet"))
  {
    vtkErrorMacro("ExecuteSimpleAlgorithmForBlock cannot be called "
      "for a vtkCompositeDataSet");
    return 0;
  }

  // There must be a bug somehwere. If this Remove()
  // is not called, the following Set() has the effect
  // of removing (!) the key.
  if (inInfo)
  {
    inInfo->Remove(vtkDataObject::DATA_OBJECT());
    inInfo->Set(vtkDataObject::DATA_OBJECT(), dobj);

    vtkTrivialProducer::FillOutputDataInformation(dobj, inInfo);
  }

  request->Set(REQUEST_DATA_OBJECT());
  outInfo->Set(SUPPRESS_RESET_PI(), 1);
  this->Superclass::ExecuteDataObject(request, inInfoVec, outInfoVec);
  outInfo->Remove(SUPPRESS_RESET_PI());
  request->Remove(REQUEST_DATA_OBJECT());

  request->Set(REQUEST_INFORMATION());

  this->Superclass::ExecuteInformation(request, inInfoVec, outInfoVec);
  request->Remove(REQUEST_INFORMATION());

  int storedPiece = -1;
  int storedNumPieces = -1;
  for(int m=0; m < this->Algorithm->GetNumberOfOutputPorts(); ++m)
  {
    vtkInformation* info = outInfoVec->GetInformationObject(m);
    // Update the whole thing
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
      storedPiece =
        info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
      storedNumPieces=
        info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
      info->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        1);
      vtkDebugMacro(<< "UPDATE_PIECE_NUMBER() 0"  << " " << info);
      info->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    }
  }

  request->Set(REQUEST_UPDATE_EXTENT());
  this->CallAlgorithm(request, vtkExecutive::RequestUpstream,
                      inInfoVec, outInfoVec);
  request->Remove(REQUEST_UPDATE_EXTENT());

  request->Set(REQUEST_DATA());
  this->Superclass::ExecuteData(request,inInfoVec,outInfoVec);
  request->Remove(REQUEST_DATA());

  for(int m=0; m < this->Algorithm->GetNumberOfOutputPorts(); ++m)
  {
    vtkInformation* info = outInfoVec->GetInformationObject(m);
    if (storedPiece!=-1)
    {
      info->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        storedNumPieces);
      vtkDebugMacro(<< "UPDATE_PIECE_NUMBER() 0"  << " " << info);
      info->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        storedPiece);
    }
  }

  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!output)
  {
    return 0;
  }
  vtkDataObject* outputCopy = output->NewInstance();
  outputCopy->ShallowCopy(output);
  return outputCopy;
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

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // If the output is not a composite dataset, let the superclass handle
  // NeedToExecuteData
  if (!vtkCompositeDataSet::SafeDownCast(dataObject))
  {
    return this->Superclass::NeedToExecuteData(outputPort,
                                               inInfoVec,outInfoVec);
  }

  // First do the basic checks.
  if(this->vtkDemandDrivenPipeline::NeedToExecuteData(
       outputPort,inInfoVec,outInfoVec))
  {
    return 1;
  }

  // Now handle composite stuff.

  vtkInformation* dataInfo = dataObject->GetInformation();

  // Check the unstructured extent.  If we do not have the requested
  // piece, we need to execute.
  int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
  int dataNumberOfPieces = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  if(dataNumberOfPieces != updateNumberOfPieces)
  {
    return 1;
  }
  int dataGhostLevel = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
  int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
  if(updateNumberOfPieces > 1 && dataGhostLevel < updateGhostLevel)
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

  if (this->NeedToExecuteBasedOnTime(outInfo, dataObject))
  {
    return 1;
  }

  if (this->NeedToExecuteBasedOnCompositeIndices(outInfo))
  {
    return 1;
  }

  // We do not need to execute.
  return 0;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::NeedToExecuteBasedOnCompositeIndices(vtkInformation* outInfo)
{
  if (outInfo->Has(UPDATE_COMPOSITE_INDICES()))
  {
    if (!outInfo->Has(DATA_COMPOSITE_INDICES()))
    {
      return 1;
    }
    unsigned int* requested_ids = reinterpret_cast<unsigned int*>(
      outInfo->Get(UPDATE_COMPOSITE_INDICES()));
    unsigned int* existing_ids = reinterpret_cast<unsigned int*>(
      outInfo->Get(DATA_COMPOSITE_INDICES()));
    int length_req = outInfo->Length(UPDATE_COMPOSITE_INDICES());
    int length_ex = outInfo->Length(DATA_COMPOSITE_INDICES());

    if (length_req > length_ex)
    {
      // we are requesting more blocks than those generated.
      return 1;
    }
    int ri=0, ei=0;
    // NOTE: We are relying on the fact that both these id lists are sorted to
    // do a more efficient comparison.
    for (; ri < length_req; ri++)
    {
      while (ei < length_ex &&
        existing_ids[ei] < requested_ids[ri])
      {
        ei++;
      }
      if (ei >= length_ex)
      {
        // we ran beyond the existing length.
        return 1;
      }
      if (existing_ids[ei] != requested_ids[ri])
      {
        return 1;
      }
    }
  }
  else
  {
    if (outInfo->Has(DATA_COMPOSITE_INDICES()))
    {
      // earlier request asked for a some blocks, but the new request is asking
      // for everything, so re-execute.
      return 1;
    }
  }
  return 0;
}


//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ForwardUpstream(vtkInformation* request)
{
  vtkDebugMacro(<< "ForwardUpstream");

  // Do not forward upstream if the input is shared with another
  // executive.
  if(this->SharedInputInformation)
  {
    return 1;
  }

  if (!this->Algorithm->ModifyRequest(request, BeforeForward))
  {
    return 0;
  }
  int port = request->Get(FROM_OUTPUT_PORT());

  // Forward the request upstream through all input connections.
  int result = 1;
  for(int i=0; i < this->GetNumberOfInputPorts(); ++i)
  {
    int nic = this->Algorithm->GetNumberOfInputConnections(i);
    vtkInformationVector* inVector = this->GetInputInformation()[i];
    for(int j=0; j < nic; ++j)
    {
      vtkInformation* info = inVector->GetInformationObject(j);
      // Get the executive producing this input.  If there is none, then
      // it is a NULL input.
      vtkExecutive* e;
      int producerPort;
      vtkExecutive::PRODUCER()->Get(info, e, producerPort);
      if(e)
      {
        request->Set(FROM_OUTPUT_PORT(), producerPort);
        if(!e->ProcessRequest(request,
                              e->GetInputInformation(),
                              e->GetOutputInformation()))
        {
          result = 0;
        }
        request->Set(FROM_OUTPUT_PORT(), port);
      }
    }
  }

  if (!this->Algorithm->ModifyRequest(request, AfterForward))
  {
    return 0;
  }

  return result;
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

  if (!this->Algorithm->ModifyRequest(request, BeforeForward))
  {
    return 0;
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

  if (!this->Algorithm->ModifyRequest(request, AfterForward))
  {
    return 0;
  }

  return result;
}
//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::CopyDefaultInformation(
  vtkInformation* request, int direction,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::CopyDefaultInformation(request, direction,
                                           inInfoVec, outInfoVec);

  if (request->Has(REQUEST_INFORMATION())||request->Has(REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    if (this->GetNumberOfInputPorts() > 0)
    {
      if (vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0))
      {
        // Copy information from the first input to all outputs.
        for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
        {
          vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
          outInfo->CopyEntry(inInfo, COMPOSITE_DATA_META_DATA());
        }
      }
    }
  }

  if(request->Has(REQUEST_UPDATE_EXTENT()))
  {
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

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
          inInfo->CopyEntry(outInfo, UPDATE_COMPOSITE_INDICES());
          inInfo->CopyEntry(outInfo, LOAD_REQUESTED_BLOCKS());
        }
      }
    }

    // Find the port that has a data that we will iterator over.
    // If there is one, make sure that we use piece extent for
    // that port. Composite data pipeline works with piece extents
    // only.
    int compositePort;
    if (this->ShouldIterateOverInput(inInfoVec, compositePort))
    {
      // Get the output port from which to copy the extent.
      outputPort = -1;
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

        // Loop over all connections on this input port.
        int numInConnections =
          inInfoVec[compositePort]->GetNumberOfInformationObjects();
        for (int j=0; j<numInConnections; j++)
        {
          // Get the pipeline information for this input connection.
          vtkInformation* inInfo =
            inInfoVec[compositePort]->GetInformationObject(j);

          vtkDebugMacro(<< "CopyEntry UPDATE_PIECE_NUMBER() " << outInfo->Get(UPDATE_PIECE_NUMBER()) << " " << outInfo);

          inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
          inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
          inInfo->CopyEntry(outInfo, LOAD_REQUESTED_BLOCKS());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::ResetPipelineInformation(int port,
                                                        vtkInformation* info)
{
  if (info->Has(SUPPRESS_RESET_PI()))
  {
    return;
  }

  this->Superclass::ResetPipelineInformation(port, info);
  info->Remove(COMPOSITE_DATA_META_DATA());
  info->Remove(UPDATE_COMPOSITE_INDICES());
  info->Remove(LOAD_REQUESTED_BLOCKS());
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PushInformation(vtkInformation* inInfo)
{
  vtkDebugMacro(<< "PushInformation " << inInfo);
  this->InformationCache->CopyEntry(inInfo, WHOLE_EXTENT());
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PopInformation(vtkInformation* inInfo)
{
  vtkDebugMacro(<< "PopInformation " << inInfo);
  inInfo->CopyEntry(this->InformationCache, WHOLE_EXTENT());
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::CheckCompositeData(
  vtkInformation *,
  int port,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  // Check that the given output port has a valid data object.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(port);

  // If this is a simple filter but has composite input,
  // create a composite output.
  int compositePort;

  if (this->ShouldIterateOverInput(inInfoVec, compositePort))
  {
    // This assumes that the first output of the filter is the one
    // that will have the composite data.
    vtkDataObject* doOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);
    if (!output)
    {
      output = this->CreateOutputCompositeDataSet(
        vtkCompositeDataSet::SafeDownCast(this->GetInputData(compositePort, 0, inInfoVec)),
        compositePort);
      vtkDebugMacro(<< "CheckCompositeData created " <<
                    output->GetClassName() << "output");

      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      // Copy extent type to the output port information because
      // CreateOutputCompositeDataSet() changes it and some algorithms need it.
      this->GetAlgorithm()->GetOutputPortInformation(port)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return 1;
  }
  // Otherwise, create a simple output
  else
  {
    return this->Superclass::CheckDataObject(port, outInfoVec);
  }
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
  return info->Get(vtkDataObject::DATA_OBJECT());
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataPipeline::GetCompositeOutputData(int port)
{
  if(!this->OutputPortIndexInRange(port, "get data for"))
  {
    return 0;
  }

  // Check that the given output port has a valid data object.
  vtkDebugMacro(<< "GetCompositeOutputData calling CheckCompositeData ");

  this->CheckCompositeData(0, port, this->GetInputInformation(), this->GetOutputInformation());

  // Return the data object.
  if(vtkInformation* info = this->GetOutputInformation(port))
  {
    return info->Get(vtkDataObject::DATA_OBJECT());
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataPipeline::CreateOutputCompositeDataSet(
  vtkCompositeDataSet* input, int compositePort)
{
  // pre: the algorithm is a non-composite algorithm.
  // pre: the question is
  //      whether to create vtkHierarchicalBoxDataSet or vtkMultiBlockDataSet.
  if (input->IsA("vtkHierarchicalBoxDataSet") ||
      input->IsA("vtkOverlappingAMR") ||
      input->IsA("vtkNonOverlappingAMR") )
  {
    vtkSmartPointer<vtkUniformGrid> tempInput = vtkSmartPointer<vtkUniformGrid>::New();

    // Check if the algorithm can accept UniformGrid on the input port.
    vtkInformation* inPortInfo =
      this->Algorithm->GetInputPortInformation(compositePort);
    const char* inputType =
      inPortInfo->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    if (!tempInput->IsA(inputType))
    {
      return vtkMultiBlockDataSet::New();
    }

    vtkInformation* inInfo = this->GetInputInformation(compositePort, 0);
    vtkSmartPointer<vtkDataObject> curInput = inInfo->Get(vtkDataObject::DATA_OBJECT());

    vtkInformation* outInfo = this->GetOutputInformation(0);

    vtkSmartPointer<vtkInformation> request =
      vtkSmartPointer<vtkInformation>::New();
    request->Set(FROM_OUTPUT_PORT(), PRODUCER()->GetPort(inInfo));

    // Set the input to be vtkUniformGrid.
    inInfo->Remove(vtkDataObject::DATA_OBJECT());
    inInfo->Set(vtkDataObject::DATA_OBJECT(), tempInput);
    // The request is forwarded upstream through the pipeline.
    request->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
    // Algorithms process this request after it is forwarded.
    request->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
    request->Set(REQUEST_DATA_OBJECT());
    outInfo->Set(SUPPRESS_RESET_PI(), 1);
    this->Superclass::ExecuteDataObject(
      request, this->GetInputInformation(),this->GetOutputInformation());
    outInfo->Remove(SUPPRESS_RESET_PI());
    request->Remove(REQUEST_DATA_OBJECT());

    // Restore input.
    inInfo->Remove(vtkDataObject::DATA_OBJECT());
    inInfo->Set(vtkDataObject::DATA_OBJECT(), curInput);

    // check the type of output data object created by the algorithm.
    vtkDataObject* curOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
    if (!curOutput->IsA("vtkUniformGrid"))
    {
      return vtkMultiBlockDataSet::New();
    }
  }

  return input->NewInstance();
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::MarkOutputsGenerated(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::MarkOutputsGenerated(request,inInfoVec,outInfoVec);

  for (int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
  {
    vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    vtkDataObject* data = outInfo->Get(vtkDataObject::DATA_OBJECT());
    if (data && !outInfo->Get(DATA_NOT_GENERATED()))
    {
      if (outInfo->Has(UPDATE_COMPOSITE_INDICES()))
      {
        size_t count = outInfo->Length(UPDATE_COMPOSITE_INDICES());
        int* indices = new int[count];
        // assume the source produced the blocks it was asked for:
        // the indices received are what was requested
        outInfo->Get(UPDATE_COMPOSITE_INDICES(),indices);
        outInfo->Set(DATA_COMPOSITE_INDICES(), indices,
          static_cast<int>(count));
        delete []indices;
      }
      else
      {
        outInfo->Remove(DATA_COMPOSITE_INDICES());
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

