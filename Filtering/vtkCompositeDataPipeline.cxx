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
#include "vtkTemporalDataSet.h"
#include "vtkUniformGrid.h"

//----------------------------------------------------------------------------
#if defined (JB_DEBUG1)
  #ifndef WIN32
  #else
    #define OUTPUTTEXT(a) vtkOutputWindowDisplayText(a);
  #endif

  #undef vtkDebugMacro
  #define vtkDebugMacro(a)  \
  { \
    vtkOStreamWrapper::EndlType endl; \
    vtkOStreamWrapper::UseEndl(endl); \
    vtkOStrStreamWrapper vtkmsg; \
    const char *name = this->Algorithm->GetClassName(); \
    if (!strcmp(name, "vtkTemporalDataSetCache") || \
        !strcmp(name, "vtkContourFilter") || \
        !strcmp(name, "vtkOpenDXStructuredGridReader") || \
        !strcmp(name, "vtkPCellDataToPointData") || \
        !strcmp(name, "vtkProcessIdScalars") || \
        !strcmp(name, "vtkTemporalFractal") || \
        !strcmp(name, "vtkTemporalSphereSource") || \
        !strcmp(name, "vtkTemporalInterpolator") || \
        !strcmp(name, "vtkTemporalStreamTracer")) \
      { \
      vtkmsg << name << " : " a << endl; \
      OUTPUTTEXT(vtkmsg.str()); \
      vtkmsg.rdbuf()->freeze(0); \
      } \
  }
#endif
//----------------------------------------------------------------------------
/*
    if (!strcmp(name, "vtkTemporalDataSetCache") || \
        !strcmp(name, "vtkContourFilter")) \
        !strcmp(name, "vtkOpenDXStructuredGridReader") || \
        !strcmp(name, "vtkPCellDataToPointData") || \
        !strcmp(name, "vtkProcessIdScalars") || \
        !strcmp(name, "vtkTemporalFractal") || \
        !strcmp(name, "vtkTemporalSphereSource") || \
        !strcmp(name, "vtkTemporalInterpolator") || \
        !strcmp(name, "vtkTemporalStreamTracer")) \
      { \
*/
vtkStandardNewMacro(vtkCompositeDataPipeline);

vtkInformationKeyMacro(vtkCompositeDataPipeline,REQUIRES_TIME_DOWNSTREAM, Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline, COMPOSITE_DATA_META_DATA, ObjectBase);
vtkInformationKeyMacro(vtkCompositeDataPipeline, UPDATE_COMPOSITE_INDICES, IntegerVector);
vtkInformationKeyMacro(vtkCompositeDataPipeline, COMPOSITE_INDICES, IntegerVector);

//----------------------------------------------------------------------------
vtkCompositeDataPipeline::vtkCompositeDataPipeline()
{
  this->InLocalLoop = 0;
  this->SuppressResetPipelineInformation = 0;
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

  // Check if REQUIRES_TIME_DOWNSTREAM() key is in the output. If yes,
  // pass it to inputs.
  bool hasRTD = false;
  int port = request->Get(FROM_OUTPUT_PORT());
  if ( port <  0 )
    {
    for (int i=0; i<this->GetNumberOfOutputPorts(); i++)
      {
      if (this->GetOutputInformation(i) && 
          this->GetOutputInformation(i)->Has(REQUIRES_TIME_DOWNSTREAM()))
        {
        hasRTD = true;
        break;
        }
      }
    }
  else
    {
    if (this->GetOutputInformation(port) && 
        this->GetOutputInformation(port)->Has(REQUIRES_TIME_DOWNSTREAM()))
      {
      hasRTD = true;
      }
    }
  
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
        
        // if the input requires time them mark that
        vtkInformation* ipi = this->Algorithm->GetInputPortInformation(i);
        const char* rdt = ipi->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
        if ((rdt && !strcmp("vtkTemporalDataSet", rdt)) || hasRTD)
          {
          info->Set(REQUIRES_TIME_DOWNSTREAM(),1);
          vtkDebugMacro(<< "Set REQUIRES_TIME_DOWNSTREAM");
          }
        if(!e->ProcessRequest(request,
                              e->GetInputInformation(),
                              e->GetOutputInformation()))
          {
          result = 0;
          }
        info->Remove(REQUIRES_TIME_DOWNSTREAM());
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
int vtkCompositeDataPipeline::ProcessRequest(vtkInformation* request,
                                             vtkInformationVector** inInfoVec,
                                             vtkInformationVector* outInfoVec)
{
  if(this->Algorithm && request->Has(REQUEST_DATA_OBJECT()))
    {
    vtkDebugMacro(<< "REQUEST_DATA_OBJECT()");
    // if we are up to date then short circuit
    if (this->PipelineMTime < this->DataObjectTime.GetMTime()
        && ! request->Has(REQUEST_REGENERATE_INFORMATION()))
      {
      return 1;
      }

    // request Update inputs first if they are out of date
    if(!this->ForwardUpstream(request))
      {
      return 0;
      }
    
    // Make sure our output data type is up-to-date.
    int result = 1;
    if(this->PipelineMTime > this->DataObjectTime.GetMTime()
        || request->Has(REQUEST_REGENERATE_INFORMATION()))
      {
      // Request data type from the algorithm.
      result = this->ExecuteDataObject(request,inInfoVec,outInfoVec);

      // Make sure the data object exists for all output ports.
      for(int i=0;
          result && i < outInfoVec->GetNumberOfInformationObjects(); ++i)
        {
        vtkInformation* info = outInfoVec->GetInformationObject(i);
        if(!info->Get(vtkDataObject::DATA_OBJECT()))
          {
          result = 0;
          }
        }

      if(result)
        {
        // Data object is now up to date.
        this->DataObjectTime.Modified();
        }
      }

    return result;
    }

  if(this->Algorithm && request->Has(REQUEST_INFORMATION()))
    {
    vtkDebugMacro(<< "REQUEST_INFORMATION()");
    return this->Superclass::ProcessRequest(request, inInfoVec,outInfoVec);
    }

  // Let the superclass handle other requests.
  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
// Handle REQUEST_DATA_OBJECT
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
  bool shouldIterate = this->ShouldIterateOverInput(compositePort) ||
    this->ShouldIterateTemporalData(request, inInfoVec, outInfoVec);
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
  // If the last iteration of ExecuteData had to iterate over time values, but
  // the current does not, then we may need to replace the output.
  bool hasIteratedTemporalData = false;
  bool isIteratingTemporalData = false;
  for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); i++)
    {
    vtkInformation* info = outInfoVec->GetInformationObject(i);
    if (info->Has(REQUIRES_TIME_DOWNSTREAM()))
      {
      isIteratingTemporalData = true;
      }
    vtkInformation* opi = this->Algorithm->GetOutputPortInformation(i);
    const char* providedDataType = opi->Get(vtkDataObject::DATA_TYPE_NAME());
    if (strcmp(providedDataType, "vtkTemporalDataSet") != 0 &&
        info->Get(vtkDataObject::DATA_OBJECT())->IsA("vtkTemporalDataSet"))
      {
      hasIteratedTemporalData = true;
      }
    }
  if (hasIteratedTemporalData && !isIteratingTemporalData)
    {
    this->SuppressResetPipelineInformation = 1;
    this->ExecuteDataObject(this->DataObjectRequest, inInfoVec, outInfoVec);
    this->SuppressResetPipelineInformation = 0;
    }

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
  bool composite = this->ShouldIterateOverInput(compositePort);
  bool temporal = 
    this->ShouldIterateTemporalData(request, inInfoVec, outInfoVec);
  // This is stupid.
  //compositePort = temporal ? -1 : compositePort;  
  if (temporal || composite)
    {
    this->ExecuteSimpleAlgorithm(request, inInfoVec, outInfoVec, compositePort);
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
  if (this->ShouldIterateOverInput(compositePort))
    {
    if (compositePort == port)
      {
      return 1;
      }
    }

  // If the algorithm is requesting a vtkTemporalDataSet, then assume that the
  // upstream pipeline will be run multiple times and a vtkTemporalDataSet will
  // be created from the multiple results.
  vtkInformation *info = this->Algorithm->GetInputPortInformation(port);
  const char *requiredType
    = info->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  if (requiredType && (strcmp(requiredType, "vtkTemporalDataSet") == 0))
    {
    return 1;
    }

  // Otherwise, let superclass handle it.
  return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
}

//----------------------------------------------------------------------------
bool vtkCompositeDataPipeline::ShouldIterateOverInput(int& compositePort)
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
        if (strcmp(inputType, "vtkTemporalDataSet") == 0)
          {
          vtkDebugMacro(<< "ShouldIterateOverInput returns 0 (Temporal)");
          return false;
          }

        if (strcmp(inputType, "vtkCompositeDataSet") == 0 ||
          strcmp(inputType, "vtkHierarchicalBoxDataSet") == 0 ||
          strcmp(inputType, "vtkMultiBlockDataSet") == 0)
          {
          vtkDebugMacro(<< "ShouldIterateOverInput return 0 (Composite)");
          return false;
          }

        vtkInformation* inInfo = this->GetInputInformation(i, 0);
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
bool vtkCompositeDataPipeline::ShouldIterateTemporalData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inInfoVec),
  vtkInformationVector* outInfoVec)
{
  // Exit fast if no outputs exist
  if (!this->Algorithm->GetNumberOfOutputPorts())
    {
    vtkDebugMacro(<< "ShouldIterateTemporalData returns 0 (no outputs)");
    return false;
    }

  // if the filter is a subclass of vtkTemporalDataSetAlgorithm
  // we do not need to loop, because it will input and output temporal data
  if (this->Algorithm->IsA("vtkTemporalDataSetAlgorithm"))
    {
    vtkDebugMacro(<< "ShouldIterateTemporalData returns 0 (vtkTemporalDataSetAlgorithm)");
    return false;
    }

  // If the input to this is Temporal, the upstream will loop
  // we do not have to.
  int i, numInputPorts = this->Algorithm->GetNumberOfInputPorts();
  for (i=0; i<numInputPorts; ++i) 
    {
    vtkInformation* inPortInfo = this->Algorithm->GetInputPortInformation(i);
    const char* inputType = 
      inPortInfo->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    if (inputType && (strcmp(inputType, "vtkTemporalDataSet") == 0))
      {
      vtkDebugMacro(<< "ShouldIterateTemporalData returns 0 (vtkTemporalDataSet input)");
      return false;
      }
    }

  int numOut = outInfoVec->GetNumberOfInformationObjects();
  for (int out = 0; out < numOut; out++)
    {
    if (outInfoVec->GetInformationObject(out)->Has(REQUIRES_TIME_DOWNSTREAM()))
      {
      // Time was requested so answer yes.
      vtkDebugMacro(<< "ShouldIterateTemporalData returns 1 (REQUIRES_TIME_DOWNSTREAM)");
      return true;
      }
    }

  return false;
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

  // Make sure a valid composite data object exists for all output ports.
  for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    this->CheckCompositeData(request, i, inInfoVec, outInfoVec);
    }

  // if we have no composite inputs then we are looping over time on a source
  if (compositePort==-1)
    {
    this->ExecuteSimpleAlgorithmTime(request, inInfoVec, outInfoVec);
    return;
    }

  // Loop using the first input on the first port.
  // This might not be valid for all cases but it is a decent
  // assumption to start with.
  // TODO: Loop over all inputs
  vtkInformation* inInfo = 0;
  inInfo = this->GetInputInformation(compositePort, 0);
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkSmartPointer<vtkCompositeDataSet> compositeOutput = 
    vtkCompositeDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  // do we have a request for multiple time steps?
  int numTimeSteps = 0;
  double *times = 0;
  numTimeSteps = outInfo->Length(UPDATE_TIME_STEPS());
  if (numTimeSteps)
    {
    times = new double [numTimeSteps];
    memcpy(times,outInfo->Get(UPDATE_TIME_STEPS()),
           sizeof(double)*numTimeSteps);
    }

  if (input && compositeOutput)
    {
    compositeOutput->PrepareForNewData();
    compositeOutput->CopyStructure(input);

    vtkSmartPointer<vtkInformation> r = 
      vtkSmartPointer<vtkInformation>::New();

    r->Set(FROM_OUTPUT_PORT(), PRODUCER()->GetPort(outInfo));

    // The request is forwarded upstream through the pipeline.
    r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
        
    // Algorithms process this request after it is forwarded.
    r->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);


    // Store the information (whole_extent and maximum_number_of_pieces)
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
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
      {
      // if it is a temporal input, set the time for each piece
      if (times)
        {
        outInfo->Set(UPDATE_TIME_STEPS(), times, numTimeSteps);
        }
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
                                               r,
                                               dobj);
        if (outObj)
          {
          compositeOutput->SetDataSet(iter, outObj);
          outObj->Delete();
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
    if (times)
      {
      outInfo->Set(UPDATE_TIME_STEPS(), times, numTimeSteps);
      compositeOutput->GetInformation()->Set(
        vtkDataObject::DATA_TIME_STEPS(), times, numTimeSteps);
      delete [] times;
      }
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
      compositeOutput->SetPipelineInformation(outInfo);
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

  double time = 0;
  int hasTime = outInfo->Length(UPDATE_TIME_STEPS());
  if (hasTime)
    {
    time = outInfo->Get(UPDATE_TIME_STEPS())[0];
    }

  // There must be a bug somehwere. If this Remove()
  // is not called, the following Set() has the effect
  // of removing (!) the key.
  if (inInfo)
    {
    inInfo->Remove(vtkDataObject::DATA_OBJECT());
    inInfo->Set(vtkDataObject::DATA_OBJECT(), dobj);
    
    // Process the whole dataset
    this->CopyFromDataToInformation(dobj, inInfo);
    }

  request->Set(REQUEST_DATA_OBJECT());
  this->SuppressResetPipelineInformation = 1;
  this->Superclass::ExecuteDataObject(
    request, this->GetInputInformation(),this->GetOutputInformation());
  this->SuppressResetPipelineInformation = 0;
  request->Remove(REQUEST_DATA_OBJECT());
  
  request->Set(REQUEST_INFORMATION());

  // Make sure that pipeline informations is in sync with the data
  if (dobj)
    {
    dobj->CopyInformationToPipeline(request, 0, inInfo, 1);

    // This should not be needed but since a lot of image filters do:
    // img->GetScalarType(), it is necessary.
    dobj->GetProducerPort(); // make sure there is pipeline info.
    dobj->CopyInformationToPipeline
      (request, 0, dobj->GetPipelineInformation(), 1);
    }

  this->Superclass::ExecuteInformation(request,inInfoVec,outInfoVec);
  request->Remove(REQUEST_INFORMATION());
  
  int storedPiece = -1;
  int storedNumPieces = -1;
  for(int m=0; m < this->Algorithm->GetNumberOfOutputPorts(); ++m)
    {
    vtkInformation* info = this->GetOutputInformation(m);
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
      info->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 
        1);
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

  // if there was a time make sure that gets set in the RUE
  if (hasTime)
    {
    outInfo->Set(UPDATE_TIME_STEPS(),&time,1);
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
    vtkInformation* info = this->GetOutputInformation(m);
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
// Execute a simple (non-composite-aware) filter multiple times, once per
// block. Collect the result in a composite dataset that is of the same
// structure as the input.
// Note that if this method is called we are assured that the input is not
// composite.
void vtkCompositeDataPipeline::ExecuteSimpleAlgorithmTime(
  vtkInformation* request,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  vtkDebugMacro(<< "ExecuteSimpleAlgorithmTime");

  vtkInformation* outInfo = 0;
  vtkSmartPointer<vtkInformation> originalInformation = 
    vtkSmartPointer<vtkInformation>::New();

  if (this->GetNumberOfOutputPorts() > 0)
    {
    outInfo = outInfoVec->GetInformationObject(0);
    originalInformation->CopyEntry(outInfo,TIME_STEPS(), 0);
    originalInformation->CopyEntry(outInfo,TIME_RANGE(), 0);
    }

  vtkSmartPointer<vtkTemporalDataSet> temporalOutput = 
    vtkTemporalDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  // do we have a request for multiple time steps?
  int numTimeSteps = 0;
  double *times = 0;
  numTimeSteps = outInfo->Length(UPDATE_TIME_STEPS());
  times = new double [numTimeSteps];
  memcpy(times, 
         outInfo->Get(UPDATE_TIME_STEPS()),
         sizeof(double)*numTimeSteps);

  int outputInitialized = 0;
  
  vtkSmartPointer<vtkInformation> r = 
    vtkSmartPointer<vtkInformation>::New();
  
  r->Set(FROM_OUTPUT_PORT(), request->Get(FROM_OUTPUT_PORT()));

  // The request is forwarded upstream through the pipeline.
  r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  
  // Algorithms process this request after it is forwarded.
  r->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  
  vtkDebugMacro(<<"EXECUTING: " << this->Algorithm->GetClassName());
  
  // Store the information (whole_extent and maximum_number_of_pieces)
  // before looping. Otherwise, executeinformation will cause
  // changes (because we pretend that the max. number of pieces is
  // one to process the whole block)
//  vtkInformation* inInfo = this->GetInputInformation(compositePort, 0);
//  this->PushInformation(inInfo);

  // True when the pipeline is iterating over the current (simple)
  // filter to produce composite output. In this case,
  // ExecuteDataStart() should NOT Initialize() the composite output.
  this->InLocalLoop = 1;
  
  for (unsigned int k=0; k< static_cast<unsigned int>(numTimeSteps); k++)
    {
    // if it is a temporal input, set the time for each piece
    outInfo->Set(UPDATE_TIME_STEPS(), times+k, 1);

    vtkDataObject* dobj = 0;
    
    vtkDataObject* outCopy = 
      this->ExecuteSimpleAlgorithmForBlock(inInfoVec, outInfoVec,
                                           0,         outInfo,
                                           r,
                                           dobj);
    if (outCopy)
      {
      vtkDebugMacro(<<"Got Data from Block");
      if (!outputInitialized)
        {
        temporalOutput->PrepareForNewData();
        outputInitialized = 1;
        }
      temporalOutput->SetTimeStep(k, outCopy);
      outCopy->Delete();
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
//  this->PopInformation(inInfo);

  outInfo->Set(UPDATE_TIME_STEPS(), times, numTimeSteps);
  temporalOutput->GetInformation()->Set(
    vtkDataObject::DATA_TIME_STEPS(), times, numTimeSteps);
  delete [] times;
  
  r->Set(REQUEST_INFORMATION());
  this->CopyDefaultInformation(r, 
                               vtkExecutive::RequestDownstream,
                               this->GetInputInformation(),
                               this->GetOutputInformation());

  outInfo->CopyEntry(originalInformation, TIME_STEPS(), 0);
  outInfo->CopyEntry(originalInformation, TIME_RANGE(), 0);
  
  vtkDataObject* curOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (curOutput != temporalOutput.GetPointer())
    {
    temporalOutput->SetPipelineInformation(outInfo);
    }
  this->ExecuteDataEnd(request,inInfoVec,outInfoVec);
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
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!vtkCompositeDataSet::SafeDownCast(dataObject))
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
  int dataGhostLevel = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
  int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
  if(dataGhostLevel < updateGhostLevel)
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

  if (this->NeedToExecuteBasedOnFastPathData(outInfo))
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
int vtkCompositeDataPipeline::NeedToExecuteBasedOnTime(
                                                      vtkInformation* outInfo,
                                                      vtkDataObject* dataObject)
{
  if (this->Superclass::NeedToExecuteBasedOnTime(outInfo, dataObject))
    {
    vtkDebugMacro(<<"NeedToExecuteBasedOnTime returns 1");
    return 1;
    }

  if (outInfo->Has(REQUIRES_TIME_DOWNSTREAM()))
    {
    // Check to see if there is anything new to do to make a proper
    // vtkTemporalDataSet.
    if (!dataObject->IsA("vtkTemporalDataSet"))
      {
      // Re-run the algorithm to generate a vtkTemporalDataSet.  This is not
      // very efficient, as we could just take the current data set and stuff it
      // into a vtkTemporalDataSet without re-running the algorithm.  In fact,
      // it would be nice if vtkCompositeDataPipeline would always use existing
      // data in its output to fill a vtkTemporalDataSet with a set at the same
      // time step.  However, that would require a lot of logic about whether
      // something else changed, and it is not a straightforward change.
      vtkDebugMacro(<<"NeedToExecuteBasedOnTime returns 1 (!vtkTemporalDataSet)");
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::NeedToExecuteBasedOnCompositeIndices(vtkInformation* outInfo)
{
  if (outInfo->Has(UPDATE_COMPOSITE_INDICES()))
    {
    if (!outInfo->Has(COMPOSITE_INDICES()))
      {
      return 1;
      }
    unsigned int* requested_ids = reinterpret_cast<unsigned int*>(
      outInfo->Get(UPDATE_COMPOSITE_INDICES()));
    unsigned int* existing_ids = reinterpret_cast<unsigned int*>(
      outInfo->Get(COMPOSITE_INDICES()));
    int length_req = outInfo->Length(UPDATE_COMPOSITE_INDICES());
    int length_ex = outInfo->Length(COMPOSITE_INDICES());
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
    if (outInfo->Has(COMPOSITE_INDICES()))
      {
      // earlier request asked for a some blocks, but the new request is asking
      // for everything, so re-execute.
      return 1;
      }
    }
  return 0;
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

  if (request->Has(REQUEST_INFORMATION()))
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
    // Find the port that has a data that we will iterator over.
    // If there is one, make sure that we use piece extent for
    // that port. Composite data pipeline works with piece extents
    // only.
    int compositePort;
    if (this->ShouldIterateOverInput(compositePort))
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

        // Loop over all connections on this input port.
        int numInConnections = 
          inInfoVec[compositePort]->GetNumberOfInformationObjects();
        for (int j=0; j<numInConnections; j++)
          {
          // Get the pipeline information for this input connection.
          vtkInformation* inInfo = 
            inInfoVec[compositePort]->GetInformationObject(j);
            
          inInfo->CopyEntry(outInfo, UPDATE_TIME_STEPS());
          inInfo->CopyEntry(outInfo, FAST_PATH_OBJECT_ID());
          inInfo->CopyEntry(outInfo, FAST_PATH_ID_TYPE());
          inInfo->CopyEntry(outInfo, FAST_PATH_OBJECT_TYPE());
          vtkDebugMacro(<< "CopyEntry UPDATE_PIECE_NUMBER() " << outInfo->Get(UPDATE_PIECE_NUMBER()) << " " << outInfo);

          inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
          inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
          inInfo->CopyEntry(outInfo, UPDATE_COMPOSITE_INDICES());
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::ResetPipelineInformation(int port,
                                                        vtkInformation* info)
{
  if (this->SuppressResetPipelineInformation)
    {
    return;
    }

  this->Superclass::ResetPipelineInformation(port, info);
  info->Remove(REQUIRES_TIME_DOWNSTREAM());
  info->Remove(COMPOSITE_DATA_META_DATA());
  info->Remove(UPDATE_COMPOSITE_INDICES());
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
  vtkDebugMacro(<< "PushInformation " << inInfo);
  this->InformationCache->CopyEntry(inInfo, WHOLE_EXTENT());
  this->InformationCache->CopyEntry(inInfo, MAXIMUM_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PopInformation(vtkInformation* inInfo)
{
  vtkDebugMacro(<< "PopInformation " << inInfo);
  inInfo->CopyEntry(this->InformationCache, WHOLE_EXTENT());
  inInfo->CopyEntry(this->InformationCache, MAXIMUM_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::CheckCompositeData(
  vtkInformation *request,
  int port, 
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  // Check that the given output port has a valid data object.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(port);

  // If this is a simple filter but has composite input, 
  // create a composite output.
  int compositePort;
  bool temporaldownstream = request != 0 && 
    this->ShouldIterateTemporalData(request, inInfoVec, outInfoVec);
  if (this->ShouldIterateOverInput(compositePort) || temporaldownstream)
    {
    // This assumes that the first output of the filter is the one
    // that will have the composite data.
    vtkDataObject* doOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);
    vtkTemporalDataSet* temporal = vtkTemporalDataSet::SafeDownCast(doOutput);
    if (!output || (temporaldownstream && !temporal))
      {
      if (temporaldownstream)
        {
        vtkDebugMacro(<< "CheckCompositeData created vtkTemporalDataSet output");
        output = vtkTemporalDataSet::New();
        }
      else
        {
        output = this->CreateOutputCompositeDataSet(
          vtkCompositeDataSet::SafeDownCast(this->GetInputData(compositePort, 0, inInfoVec)),
            compositePort);
        vtkDebugMacro(<< "CheckCompositeData created " <<
          output->GetClassName() << "output");
        }
      output->SetPipelineInformation(outInfo);
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
  // pre: we are not create vtkTemporalDataSet for the output, the question is
  //      whether to create vtkHierarchicalBoxDataSet or vtkMultiBlockDataSet.
  if (input->IsA("vtkHierarchicalBoxDataSet"))
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


    vtkSmartPointer<vtkInformation> request = 
      vtkSmartPointer<vtkInformation>::New();
    request->Set(FROM_OUTPUT_PORT(), PRODUCER()->GetPort(inInfo));

    // Set the input to be vtkUniformGrid.
    inInfo->Remove(vtkDataObject::DATA_OBJECT());
    inInfo->Set(vtkDataObject::DATA_OBJECT(), tempInput);
    // Process the whole dataset
    this->CopyFromDataToInformation(tempInput, inInfo);
    // The request is forwarded upstream through the pipeline.
    request->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
    // Algorithms process this request after it is forwarded.
    request->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1); 
    request->Set(REQUEST_DATA_OBJECT());
    this->SuppressResetPipelineInformation = 1;
    this->Superclass::ExecuteDataObject(
      request, this->GetInputInformation(),this->GetOutputInformation());
    this->SuppressResetPipelineInformation = 0;
    request->Remove(REQUEST_DATA_OBJECT());

    // Restore input.
    inInfo->Remove(vtkDataObject::DATA_OBJECT());
    inInfo->Set(vtkDataObject::DATA_OBJECT(), curInput);
    
    // check the type of output data object created by the algorithm.
    vtkInformation* outInfo = this->GetOutputInformation(0);
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

  // Save the information about COMPOSITE_INDICES() as needed in the data
  // object.
  int outputPort = 0;
  if(request->Has(FROM_OUTPUT_PORT()))
    {
    outputPort = request->Get(FROM_OUTPUT_PORT());
    outputPort = (outputPort >= 0 ? outputPort : 0);
    }

  for (int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
    {
    vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    vtkDataObject* data = outInfo->Get(vtkDataObject::DATA_OBJECT());
    if (data && !outInfo->Get(DATA_NOT_GENERATED()))
      {
      vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(data);
      if (outInfo->Has(UPDATE_COMPOSITE_INDICES()) && cd)
        {
        vtkCompositeDataIterator* iter = cd->NewIterator();
        size_t count = 0;
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
          iter->GoToNextItem())
          {
          count++;
          }
        int *indices = new int[count+1];
        int index=0;
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
          iter->GoToNextItem(), index++)
          {
          indices[index] = static_cast<int>(iter->GetCurrentFlatIndex());
          }
        iter->Delete();
        outInfo->Set(COMPOSITE_INDICES(), indices,
          static_cast<int>(count));
        delete []indices;
        }
      else
        {
        outInfo->Remove(COMPOSITE_INDICES());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

