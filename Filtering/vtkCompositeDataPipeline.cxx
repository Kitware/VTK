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
#include "vtkHierarchicalDataInformation.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkCompositeDataPipeline, "1.4");
vtkStandardNewMacro(vtkCompositeDataPipeline);

vtkInformationKeyMacro(vtkCompositeDataPipeline,REQUEST_COMPOSITE_DATA,Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline,REQUEST_COMPOSITE_INFORMATION,Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline,REQUEST_COMPOSITE_UPDATE_EXTENT,Integer);
vtkInformationKeyMacro(vtkCompositeDataPipeline,COMPOSITE_DATA_TYPE_NAME,String);
vtkInformationKeyMacro(vtkCompositeDataPipeline,COMPOSITE_DATA_SET,DataObject);
vtkInformationKeyMacro(vtkCompositeDataPipeline,COMPOSITE_DATA_INFORMATION,ObjectBase);
vtkInformationKeyMacro(vtkCompositeDataPipeline,UPDATE_COST,Double);
vtkInformationKeyMacro(vtkCompositeDataPipeline,MARKED_FOR_UPDATE,Integer);

//----------------------------------------------------------------------------
vtkCompositeDataPipeline::vtkCompositeDataPipeline()
{
  this->InSubPass = 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataPipeline::~vtkCompositeDataPipeline()
{
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ProcessRequest(vtkInformation* request)
{
  // Handle StreamingDemandDrivenPipeline passes

  if(this->Algorithm && request->Has(REQUEST_PIPELINE_MODIFIED_TIME()))
    {
    // Update inputs first.
    if(!this->ForwardUpstream(request))
      {
      return 0;
      }

    unsigned int outMTime;;

    // First pipeline mtime request, update internal mtime to force
    // execution
    if (request->Has(vtkCompositeDataSet::INDEX()))
      {
      this->SubPassTime.Modified();
      }

    if (this->InSubPass)
      {
      outMTime = this->SubPassTime;
      }
    else
      {
      // The pipeline's MTime starts with this algorithm's MTime.
      this->PipelineMTime = this->Algorithm->GetMTime();
      
      // We want the maximum PipelineMTime of all inputs.
      for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
        {
        for(int j=0; j < this->Algorithm->GetNumberOfInputConnections(i); ++j)
          {
          vtkInformation* info = this->GetInputInformation(i, j);
          unsigned long mtime = info->Get(PIPELINE_MODIFIED_TIME());
          if(mtime > this->PipelineMTime)
            {
            this->PipelineMTime = mtime;
            }
          }
        }
      outMTime = this->PipelineMTime;
      }

    // Set the pipeline mtime for all outputs.
    for(int j=0; j < this->Algorithm->GetNumberOfOutputPorts(); ++j)
      {
      vtkInformation* info = this->GetOutputInformation(j);
      info->Set(PIPELINE_MODIFIED_TIME(), outMTime);
      }

    return 1;
    }

  if(this->Algorithm && request->Has(REQUEST_DATA_OBJECT()))
    {
    // Make sure our output information is up-to-date.
    int result = 1;
    int executeDataObject = 0;
    if (this->InSubPass)
      {
      if (this->SubPassTime > this->DataObjectTime.GetMTime())
        {
        executeDataObject = 1;
        }
      }
    else
      {
      if (this->PipelineMTime > this->DataObjectTime.GetMTime())
        {
        executeDataObject = 1;
        }
      }

    if(executeDataObject)
      {
      // Request information from the algorithm.
      result = this->ExecuteDataObject(request);

      // Information is now up to date.
      this->DataObjectTime.Modified();
      }

    return result;
    }

  if(this->Algorithm && request->Has(REQUEST_INFORMATION()))
    {
    // Make sure our output information is up-to-date.
    int result = 1;
    if (this->InSubPass)
      {
      if(this->SubPassTime > this->InformationTime.GetMTime())
        {
        // Make sure input types are valid before algorithm does anything.
        if(!this->InputCountIsValid() /* || !this->InputTypeIsValid() */)
          {
          return 0;
          }
        
        int currentBlock[2] = {-1, -1};
        if (request->Has(vtkHierarchicalDataSet::LEVEL()))
          {
          currentBlock[0] = request->Get(vtkHierarchicalDataSet::LEVEL());
          }
        if (request->Has(vtkCompositeDataSet::INDEX()))
          {
          currentBlock[1] = request->Get(vtkCompositeDataSet::INDEX());
          }
        vtkInformation* outInfo = 
          this->GetOutputInformation()->GetInformationObject(0);
        if (outInfo)
          {
          outInfo->Set(vtkHierarchicalDataSet::LEVEL(), currentBlock[0]);
          outInfo->Set(vtkCompositeDataSet::INDEX(),    currentBlock[1]);
          }
        
        // Request information from the algorithm.
        result = this->ExecuteInformation(request);
        
        // Information is now up to date.
        this->InformationTime.Modified();
        }
      }
    // There are two ways InSubPass may not be set when this pass is invoked:
    // 1. The output is a simple data object (i.e. polydata) and the consumer
    // is a simple data algorithm (i.e. mapper)
    // 2. The output is complex data object but is connected to a simple
    // data algorithm
    // (2) is not supported yet
    else
      {
      // Translate the request to a composite request

      // Setup the request for information.
      vtkSmartPointer<vtkInformation> r = vtkSmartPointer<vtkInformation>::New();
      r->Set(REQUEST_COMPOSITE_INFORMATION(), 1);
      
      // The request is forwarded upstream through the pipeline.
      r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
      
      // Algorithms process this request after it is forwarded.
      r->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

      result = this->ProcessRequest(r);
      }
    return result;
    }

  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    return 1;
    }

  if(this->Algorithm && request->Has(REQUEST_DATA()))
    {
    int result = 1;
    // Request data from the algorithm.
    if (this->InSubPass)
      {
      result = this->ExecuteData(request);
      }
    // There are two ways InSubPass may not be set when this pass is invoked:
    // 1. The output is a simple data object (i.e. polydata) and the consumer
    // is a simple data algorithm (i.e. mapper)
    // 2. The output is complex data object but is connected to a simple
    // data algorithm
    // (2) is not supported yet
    else
      {
      // Translate the request to a composite request

      // Setup the request for update extent propagation.
      vtkSmartPointer<vtkInformation> r = vtkSmartPointer<vtkInformation>::New();
      r->Set(REQUEST_COMPOSITE_DATA(), 1);
      
      r->Set(FROM_OUTPUT_PORT(), request->Get(FROM_OUTPUT_PORT()));

      // The request is forwarded upstream through the pipeline.
      r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);

      result = this->ProcessRequest(r);
      }
    return result;
    }

  if(this->Algorithm && request->Has(REQUEST_COMPOSITE_INFORMATION()))
    {
    this->InSubPass = 0;

    // Update inputs first.
    if(!this->ForwardUpstream(request))
      {
      return 0;
      }

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
          }
        }
      }
    if (appendKey)
      {
      request->Append(vtkExecutive::KEYS_TO_COPY(), 
                      vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION());
      }
    
    return  this->ExecuteCompositeInformation(request);
    }

  if(request->Has(REQUEST_COMPOSITE_UPDATE_EXTENT()))
    {
    // Get the output port from which the request was made.
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
      {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      }

    // If we need to execute, propagate the update extent.
    int result = 1;
    if(this->vtkDemandDrivenPipeline::NeedToExecuteData(outputPort))
      {
      // Make sure input types are valid before algorithm does anything.
      if(!this->InputCountIsValid() /* || !this->InputTypeIsValid() */ )
        {
        return 0;
        }

      // Invoke the request on the algorithm.
      result = this->CallAlgorithm(request, vtkExecutive::RequestUpstream);

      // Propagate the update extent to all inputs.
      if(result)
        {
        result = this->ForwardUpstream(request);
        }
      }
    return result;
    }

  if(this->Algorithm && request->Has(REQUEST_COMPOSITE_DATA()))
    {
    this->InSubPass = 1;

    // Update inputs first.
    if(!this->ForwardUpstream(request))
      {
      return 0;
      }
    return this->ExecuteCompositeData(request);
    }

  // Let the superclass handle other requests.
  return this->Superclass::ProcessRequest(request);
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::Update()
{
  return this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::UpdateData(int outputPort)
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("UpdateData"))
    {
    return 0;
    }

  // Range check.
  if(outputPort < -1 ||
     outputPort >= this->Algorithm->GetNumberOfOutputPorts())
    {
    vtkErrorMacro("UpdateData given output port index "
                  << outputPort << " on an algorithm with "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
    }

  // Setup the request for update extent propagation.
  vtkSmartPointer<vtkInformation> r = vtkSmartPointer<vtkInformation>::New();
  r->Set(REQUEST_COMPOSITE_DATA(), 1);

  r->Set(FROM_OUTPUT_PORT(), outputPort);

  // The request is forwarded upstream through the pipeline.
  r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);

  // Send the request.
  return this->ProcessRequest(r);
}

//----------------------------------------------------------------------------
// Overwrite Update() with the composite data passes. UpdateData() potentially
// invokes superclass' passes in a loop
int vtkCompositeDataPipeline::Update(int port)
{
  this->ExecuteDataObject(0);

  // Next update the information
  if(!this->UpdateInformation())
    {
    return 0;
    }

  if(port >= -1 && port < this->Algorithm->GetNumberOfOutputPorts())
    {
    int retval = 1;
    // some streaming filters can request that the pipeline execute multiple
    // times for a single update
    do 
      {
      retval =  
        this->PropagateUpdateExtent(port) && 
        this->UpdateData(port) && 
        retval;
      }
    while (this->Algorithm->GetInformation()->Get(CONTINUE_EXECUTING()));
    return retval;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::UpdateInformation()
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("UpdateInformation"))
    {
    return 0;
    }

  // Update the pipeline mtime first.
  if(!this->UpdatePipelineMTime())
    {
    return 0;
    }

  // Setup the request for information.
  vtkSmartPointer<vtkInformation> r = vtkSmartPointer<vtkInformation>::New();
  r->Set(REQUEST_COMPOSITE_INFORMATION(), 1);

  // The request is forwarded upstream through the pipeline.
  r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);

  // Algorithms process this request after it is forwarded.
  r->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

  // Send the request.
  return this->ProcessRequest(r);
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::PropagateUpdateExtent(int outputPort)
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("PropagateUpdateExtent"))
    {
    return 0;
    }

  // Range check.
  if(outputPort < -1 ||
     outputPort >= this->Algorithm->GetNumberOfOutputPorts())
    {
    vtkErrorMacro("PropagateUpdateExtent given output port index "
                  << outputPort << " on an algorithm with "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
    }

  // Setup the request for update extent propagation.
  vtkSmartPointer<vtkInformation> r = vtkSmartPointer<vtkInformation>::New();
  r->Set(REQUEST_COMPOSITE_UPDATE_EXTENT(), 1);
  r->Set(FROM_OUTPUT_PORT(), outputPort);

  // The request is forwarded upstream through the pipeline.
  r->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);

  // Algorithms process this request before it is forwarded.
  r->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);

  // Send the request.
  return this->ProcessRequest(r);
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ExecuteCompositeInformation(
  vtkInformation* request)
{
  // Make sure our output information is up-to-date.
  int result = 1;
  if(this->PipelineMTime > this->CompositeDataInformationTime.GetMTime())
    {
    // Make sure input types are valid before algorithm does anything.
    if(!this->InputCountIsValid() /* || !this->InputTypeIsValid() */)
      {
      return 0;
      }

    // Invoke the request on the algorithm.
    int numInputPorts = this->Algorithm->GetNumberOfInputPorts();
    for (int i=0; i<numInputPorts; i++)
      {
      int numInputCons = this->Algorithm->GetNumberOfInputConnections(i);
      for (int j=0; j<numInputCons; j++)
        {
        vtkInformation* inInfo = this->GetInputInformation(i, j);
        vtkDataObject* input = inInfo->Get(COMPOSITE_DATA_SET());
        if (!input)
          {
          vtkInformation* inPortInfo = 
            this->Algorithm->GetInputPortInformation(i);
          const char* dt = 
            inPortInfo->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
          if (dt)
            {
            // If the composite data input to the algorithm is not
            // set, create and assign it. This happens when the producer
            // of the input data actually produces a simple data object
            // (in a loop)
            vtkDataObject* dobj = this->NewDataObject(dt);
            inInfo->Set(COMPOSITE_DATA_SET(), dobj);
            dobj->Delete();
            }
          }
        }
      }

    result =  this->CallAlgorithm(request, vtkExecutive::RequestDownstream);

    // Information is now up to date.
    this->CompositeDataInformationTime.Modified();
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkCompositeDataPipeline::ExecuteCompositeData(vtkInformation* request)
{
  int retval = 1;

  if(this->PipelineMTime > this->CompositeDataTime.GetMTime())
    {
    int outputPort = request->Get(FROM_OUTPUT_PORT());

    if (this->GetNumberOfInputPorts() > 0)
      {
      // TODO: This should work with all inputs
      vtkInformation* inInfo = this->GetInputInformation(0, 0);
      vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
        inInfo->Get(COMPOSITE_DATA_SET()));
    
      vtkHierarchicalDataInformation* dataInf = 
        vtkHierarchicalDataInformation::SafeDownCast(
          inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
      if (dataInf)
        {
        // Execute the streaming demand driven pipeline for each block
        // TODO: This should only update the blocks that are MARKED_FOR_UPDATE
        unsigned int numLevels = dataInf->GetNumberOfLevels();
        for (unsigned int i=0; i<numLevels; i++)
          {
          unsigned int numDataSets = dataInf->GetNumberOfDataSets(i);
          for (unsigned j=0; j<numDataSets; j++)
            {
            // First pipeline mtime
          
            // Setup the request for pipeline modification time.
            vtkSmartPointer<vtkInformation> r1 = 
              vtkSmartPointer<vtkInformation>::New();
            r1->Set(REQUEST_PIPELINE_MODIFIED_TIME(), 1);
          
            r1->Set(vtkHierarchicalDataSet::LEVEL(), i);
            r1->Set(vtkCompositeDataSet::INDEX(), j);
          
            // The request is forwarded upstream through the pipeline.
            r1->Set(vtkExecutive::FORWARD_DIRECTION(), 
                    vtkExecutive::RequestUpstream);
          
            // Send the request.
            if (!this->ForwardUpstream(r1))
              {
              return 0;
              }
          
            // Do the data-object creation pass before the information pass.

            // Setup the request for data object creation.
            vtkSmartPointer<vtkInformation> r1_5 = 
              vtkSmartPointer<vtkInformation>::New();
            r1_5->Set(REQUEST_DATA_OBJECT(), 1);

            // The request is forwarded upstream through the pipeline.
            r1_5->Set(vtkExecutive::FORWARD_DIRECTION(), 
                      vtkExecutive::RequestUpstream);

            // Algorithms process this request after it is forwarded.
            r1_5->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

            r1_5->Set(vtkHierarchicalDataSet::LEVEL(), i);
            r1_5->Set(vtkCompositeDataSet::INDEX(), j);

            // Send the request.
            if (!this->ForwardUpstream(r1_5))
              {
              return 0;
              }
          
            // Setup the request for information.
            vtkSmartPointer<vtkInformation> r2 = 
              vtkSmartPointer<vtkInformation>::New();
            r2->Set(REQUEST_INFORMATION(), 1);
          
            r2->Set(vtkHierarchicalDataSet::LEVEL(), i);
            r2->Set(vtkCompositeDataSet::INDEX(), j);
          
            // The request is forwarded upstream through the pipeline.
            r2->Set(
              vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
          
            // Algorithms process this request after it is forwarded.
            r2->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
          
            // Send the request.
            if (!this->ForwardUpstream(r2))
              {
              return 0;
              }
          
            // Setup the request for update extent propagation.
            vtkSmartPointer<vtkInformation> r3 = 
              vtkSmartPointer<vtkInformation>::New();
            r3->Set(REQUEST_UPDATE_EXTENT(), 1);
            r3->Set(FROM_OUTPUT_PORT(), outputPort);

            r3->Set(vtkHierarchicalDataSet::LEVEL(), i);
            r3->Set(vtkCompositeDataSet::INDEX(), j);
          
            // The request is forwarded upstream through the pipeline.
            r3->Set(
              vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
          
            // Algorithms process this request before it is forwarded.
            r3->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
          
            // Send the request.
            if (!this->ForwardUpstream(r3))
              {
              return 0;
              }
          
            // Setup the request for data.
            vtkSmartPointer<vtkInformation> r4 = 
              vtkSmartPointer<vtkInformation>::New();
            r4->Set(REQUEST_DATA(), 1);
            r4->Set(FROM_OUTPUT_PORT(), outputPort);
          
            r4->Set(vtkHierarchicalDataSet::LEVEL(), i);
            r4->Set(vtkCompositeDataSet::INDEX(), j);
          
            // The request is forwarded upstream through the pipeline.
            r4->Set(
              vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
          
            // Algorithms process this request after it is forwarded.
            r4->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
          
            // Send the request.
            if (!this->ForwardUpstream(r4))
              {
              return 0;
              }

            vtkDataObject* block = inInfo->Get(vtkDataObject::DATA_OBJECT());
            if (block)
              {
              vtkDataObject* blockCopy = block->NewInstance();
              blockCopy->ShallowCopy(block);
              input->AddDataSet(r4, blockCopy);
              blockCopy->Delete();
              }
            }
          }
        }
      }

    int result = 1;
    // Make sure a valid data object exists for all output ports.  This
    // will create the output if necessary. Note that this means that
    // output is guaranteed to exist only when REQUEST_COMPOSITE_DATA is
    // process (not during REQUEST_COMPOSITE_INFORMATION)
    for(int i=0; result && i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      result = this->CheckCompositeData(i);
      }

    if (retval)
      {
      // execute the algorithm
      retval = this->CallAlgorithm(request, vtkExecutive::RequestDownstream);
      }

    // Data is now up to date.
    this->CompositeDataTime.Modified();
    }

  return retval;
}

//----------------------------------------------------------------------------
// Handle REQUEST_DATA_OBJECT
int vtkCompositeDataPipeline::ExecuteDataObject(vtkInformation* request)
{
  vtkInformationVector* outputVector = this->GetOutputInformation();
  int numOut = outputVector->GetNumberOfInformationObjects();
  for (int i=0; i<numOut; i++)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    
    vtkDataObject* doOutput = 
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_SET());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);

    if (output && request)
      {
      vtkDataObject* dobj = output->GetDataSet(request);
      if (dobj)
        {
        vtkDataObject* dobjCopy = dobj->NewInstance();
        dobjCopy->SetPipelineInformation(info);
        dobjCopy->Delete();
        }
      else
        {
        vtkDataObject* dobjCopy = vtkDataObject::New();
        dobjCopy->SetPipelineInformation(info);
        dobjCopy->Delete();
        }
      }
    else
      {
      this->CheckDataObject(i);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Handle REQUEST_INFORMATION
int vtkCompositeDataPipeline::ExecuteInformation(vtkInformation* request)
{
  vtkInformationVector* outputVector = this->GetOutputInformation();
  int numOut = outputVector->GetNumberOfInformationObjects();
  for (int i=0; i<numOut; i++)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    
    vtkDataObject* doOutput = 
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_SET());
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(doOutput);

    if (output)
      {
      vtkDataObject* dobj = output->GetDataSet(request);
      vtkDataObject* dobjCopy = 
        info->Get(vtkDataObject::DATA_OBJECT());
      if (dobj && dobjCopy)
        {
        dobjCopy->ShallowCopy(dobj);
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Handle REQUEST_DATA
int vtkCompositeDataPipeline::ExecuteData(vtkInformation* request)
{
  vtkInformationVector* outputVector = this->GetOutputInformation();
  int numOut = outputVector->GetNumberOfInformationObjects();
  for (int i=0; i<numOut; i++)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    
    vtkDataObject* doOutput = 
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_SET());
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
int vtkCompositeDataPipeline::CheckCompositeData(int port)
{
  // Check that the given output port has a valid data object.
  vtkInformation* outInfo =
    this->GetOutputInformation()->GetInformationObject(port);
  vtkDataObject* data = 
    outInfo->Get(COMPOSITE_DATA_SET());
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

  return 1;
}


//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataPipeline::GetCompositeOutputData(int port)
{
  if(!this->OutputPortIndexInRange(port, "get data for"))
    {
    return 0;
    }

  this->CheckCompositeData(port);

  // Return the data object.
  if(vtkInformation* info = this->GetOutputInformation(port))
    {
    return info->Get(COMPOSITE_DATA_SET());
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

