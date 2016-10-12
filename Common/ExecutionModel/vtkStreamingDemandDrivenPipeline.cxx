/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingDemandDrivenPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerRequestKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkNew.h"

vtkStandardNewMacro(vtkStreamingDemandDrivenPipeline);

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, CONTINUE_EXECUTING, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, EXACT_EXTENT, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_EXTENT, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_TIME, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_TIME_DEPENDENT_INFORMATION, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_EXTENT_INITIALIZED, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_PIECE_NUMBER, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_NUMBER_OF_PIECES, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_NUMBER_OF_GHOST_LEVELS, Integer);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, WHOLE_EXTENT, IntegerVector, 6);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, UPDATE_EXTENT, IntegerVector, 6);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, COMBINED_UPDATE_EXTENT, IntegerVector, 6);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UNRESTRICTED_UPDATE_EXTENT, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_STEPS, DoubleVector);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_TIME_STEP, Double);

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, PREVIOUS_UPDATE_TIME_STEP, Double);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_RANGE, DoubleVector);

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, BOUNDS, DoubleVector);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_DEPENDENT_INFORMATION, Integer);

//----------------------------------------------------------------------------
class vtkStreamingDemandDrivenPipelineToDataObjectFriendship
{
public:
  static void Crop(vtkDataObject* obj, const int* extent)
  {
    obj->Crop(extent);
  }
};

namespace
{
void vtkSDDPSetUpdateExtentToWholeExtent(vtkInformation *info)
{
  typedef vtkStreamingDemandDrivenPipeline vtkSDDP;
  info->Set(vtkSDDP::UPDATE_PIECE_NUMBER(), 0);
  info->Set(vtkSDDP::UPDATE_NUMBER_OF_PIECES(), 1);
  info->Set(vtkSDDP::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  if(info->Has(vtkSDDP::WHOLE_EXTENT()))
  {
    int extent[6] = {0,-1,0,-1,0,-1};
    info->Get(vtkSDDP::WHOLE_EXTENT(), extent);
    info->Set(vtkSDDP::UPDATE_EXTENT(), extent, 6);
  }
}
}

//----------------------------------------------------------------------------
vtkStreamingDemandDrivenPipeline::vtkStreamingDemandDrivenPipeline()
{
  this->ContinueExecuting = 0;
  this->UpdateExtentRequest = 0;
  this->LastPropogateUpdateExtentShortCircuited = 0;
}

//----------------------------------------------------------------------------
vtkStreamingDemandDrivenPipeline::~vtkStreamingDemandDrivenPipeline()
{
  if (this->UpdateExtentRequest)
  {
    this->UpdateExtentRequest->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::ProcessRequest(vtkInformation* request,
                 vtkInformationVector** inInfoVec,
                 vtkInformationVector* outInfoVec)
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("ProcessRequest", request))
  {
    return 0;
  }

  // Look for specially supported requests.
  if(request->Has(REQUEST_UPDATE_TIME()))
  {
    int result = 1;
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    int N2E =  this->Superclass::NeedToExecuteData(outputPort, inInfoVec,outInfoVec);
    if(!N2E && outputPort>=0)
    {
      vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
      vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
      if (outInfo->Has(TIME_DEPENDENT_INFORMATION()))
      {
        N2E  = this->NeedToExecuteBasedOnTime(outInfo,dataObject);
      }
      else
      {
        N2E = 0;
      }
    }
    if(N2E)
    {
      result = this->CallAlgorithm(request, vtkExecutive::RequestUpstream,
                                   inInfoVec, outInfoVec);
      // Propagate the update extent to all inputs.
      if(result)
      {
        result = this->ForwardUpstream(request);
      }
      result = 1;
    }
    return result;
  }

 // Look for specially supported requests.
  if(request->Has(REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }
    int N2E = 1;
    if(outputPort>=0)
    {
      vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
      if(!outInfo->Has(TIME_DEPENDENT_INFORMATION()))
      {
        N2E = 0;
      }
    }
    if(!N2E)
    {
      return 1;
    }
  }

  if(request->Has(REQUEST_UPDATE_EXTENT()))
  {
    // Get the output port from which the request was made.
    this->LastPropogateUpdateExtentShortCircuited = 1;
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    // Make sure the information on the output port is valid.
    if(!this->VerifyOutputInformation(outputPort,inInfoVec,outInfoVec))
    {
      return 0;
    }

    // Get the output info
    vtkInformation* outInfo = 0;
    if (outputPort > -1)
    {
      outInfo = outInfoVec->GetInformationObject(outputPort);
    }

    // Combine the requested extent into COMBINED_UPDATE_EXTENT,
    // but only do so if the UPDATE_EXTENT key exists and if the
    // UPDATE_EXTENT is not an empty extent
    int *updateExtent = 0;
    if (outInfo &&
        (updateExtent = outInfo->Get(UPDATE_EXTENT())) != 0)
    {
      // Downstream algorithms can set UPDATE_EXTENT_INITIALIZED to
      // REPLACE if they do not want to combine with previous extents
      if (outInfo->Get(UPDATE_EXTENT_INITIALIZED()) !=
          VTK_UPDATE_EXTENT_REPLACE)
      {
        int *combinedExtent = outInfo->Get(COMBINED_UPDATE_EXTENT());
        if (combinedExtent &&
            combinedExtent[0] <= combinedExtent[1] &&
            combinedExtent[2] <= combinedExtent[3] &&
            combinedExtent[4] <= combinedExtent[5])
        {
          if (updateExtent[0] <= updateExtent[1] &&
              updateExtent[2] <= updateExtent[3] &&
              updateExtent[4] <= updateExtent[5])
          {
            int newExtent[6];
            for (int ii = 0; ii < 6; ii += 2)
            {
              newExtent[ii] = combinedExtent[ii];
              if (updateExtent[ii] < newExtent[ii])
              {
                newExtent[ii] = updateExtent[ii];
              }
              newExtent[ii+1] = combinedExtent[ii+1];
              if (updateExtent[ii+1] > newExtent[ii+1])
              {
                newExtent[ii+1] = updateExtent[ii+1];
              }
            }
            outInfo->Set(COMBINED_UPDATE_EXTENT(), newExtent, 6);
            outInfo->Set(UPDATE_EXTENT(), newExtent, 6);
          }
          else
          {
            outInfo->Set(UPDATE_EXTENT(), combinedExtent, 6);
          }
        }
        else
        {
          outInfo->Set(COMBINED_UPDATE_EXTENT(), updateExtent, 6);
        }
      }
    }

    // If we need to execute, propagate the update extent.
    int result = 1;
    int N2E = this->NeedToExecuteData(outputPort,inInfoVec,outInfoVec);
    if (!N2E &&
        outInfo &&
        this->GetNumberOfInputPorts() &&
        inInfoVec[0]->GetNumberOfInformationObjects () > 0)
    {
      vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
      int outNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
      int inNumberOfPieces = inInfo->Get(UPDATE_NUMBER_OF_PIECES());
      if(inNumberOfPieces != outNumberOfPieces)
      {
        N2E = 1;
      }
      else
      {
        if (outNumberOfPieces != 1)
        {
          int outPiece = outInfo->Get(UPDATE_PIECE_NUMBER());
          int inPiece = inInfo->Get(UPDATE_PIECE_NUMBER());
          if (inPiece != outPiece)
          {
            N2E = 1;
          }
        }
      }
    }
    if(N2E)
    {
      // Make sure input types are valid before algorithm does anything.
      if(!this->InputCountIsValid(inInfoVec) ||
         !this->InputTypeIsValid(inInfoVec))
      {
        result = 0;
      }
      else
      {
        // Invoke the request on the algorithm.
        this->LastPropogateUpdateExtentShortCircuited = 0;
        result = this->CallAlgorithm(request, vtkExecutive::RequestUpstream,
                                     inInfoVec, outInfoVec);

        // Propagate the update extent to all inputs.
        if(result)
        {
          result = this->ForwardUpstream(request);
        }
        result = 1;
      }
    }
    if (!N2E)
    {
      if(outInfo && outInfo->Has(COMBINED_UPDATE_EXTENT()))
      {
        static int emptyExt[6] = { 0, -1, 0, -1, 0, -1 };
        outInfo->Set(COMBINED_UPDATE_EXTENT(), emptyExt, 6);
      }
    }
    return result;
  }

  if(request->Has(REQUEST_DATA()))
  {
    // Let the superclass handle the request first.
    if(this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec))
    {
      for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
      {
        vtkInformation* info = outInfoVec->GetInformationObject(i);
        // Crop the output if the exact extent flag is set.
        if(info->Has(EXACT_EXTENT()) && info->Get(EXACT_EXTENT()))
        {
          vtkDataObject* data = info->Get(vtkDataObject::DATA_OBJECT());
          vtkStreamingDemandDrivenPipelineToDataObjectFriendship::Crop(
            data, info->Get(UPDATE_EXTENT()));
        }
        // Clear combined update extent, since the update cycle has completed
        if (info->Has(COMBINED_UPDATE_EXTENT()))
        {
          static int emptyExt[6] = { 0, -1, 0, -1, 0, -1 };
          info->Set(COMBINED_UPDATE_EXTENT(), emptyExt, 6);
        }
      }
      return 1;
    }
    return 0;
  }

  // Let the superclass handle other requests.
  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update()
{
  return this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(int port)
{
  return this->Update(port, 0);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(int port,
                                             vtkInformationVector* requests)
{
  if(!this->UpdateInformation())
  {
    return 0;
  }
  int numPorts = this->Algorithm->GetNumberOfOutputPorts();
  if (requests)
  {
    vtkInformationVector* outInfoVec = this->GetOutputInformation();
    for (int i=0; i<numPorts; i++)
    {
      vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
      vtkInformation* req = requests->GetInformationObject(i);
      if (outInfo && req)
      {
        outInfo->Append(req);
      }
    }
  }

  if(port >= -1 && port < numPorts)
  {
    int retval = 1;
    // some streaming filters can request that the pipeline execute multiple
    // times for a single update
    do
    {
      this->PropagateTime(port);
      this->UpdateTimeDependentInformation(port);
      retval = retval && this->PropagateUpdateExtent(port);
      if (retval && !this->LastPropogateUpdateExtentShortCircuited)
      {
        retval = retval && this->UpdateData(port);
      }
    }
    while (this->ContinueExecuting);
    return retval;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::UpdateWholeExtent()
{
  this->UpdateInformation();
  // if we have an output then set the UE to WE for it
  if (this->Algorithm->GetNumberOfOutputPorts())
  {
    vtkSDDPSetUpdateExtentToWholeExtent
      (this->GetOutputInformation()->GetInformationObject(0));
  }
  // otherwise do it for the inputs
  else
  {
    // Loop over all input ports.
    for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
      // Loop over all connections on this input port.
      int numInConnections = this->Algorithm->GetNumberOfInputConnections(i);
      for (int j=0; j<numInConnections; j++)
      {
        // Get the pipeline information for this input connection.
        vtkInformation* inInfo = this->GetInputInformation(i, j);
        vtkSDDPSetUpdateExtentToWholeExtent(inInfo);
      }
    }
  }
  return this->Update();
}

//----------------------------------------------------------------------------
int
vtkStreamingDemandDrivenPipeline
::ExecuteInformation(vtkInformation* request,
                     vtkInformationVector** inInfoVec,
                     vtkInformationVector* outInfoVec)
{
  // Let the superclass make the request to the algorithm.
  if(this->Superclass::ExecuteInformation(request,inInfoVec,outInfoVec))
  {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outInfoVec->GetInformationObject(i);
      vtkDataObject* data = info->Get(vtkDataObject::DATA_OBJECT());

      if (!data)
      {
        return 0;
      }

      if(data->GetExtentType() == VTK_3D_EXTENT)
      {
        if(!info->Has(WHOLE_EXTENT()))
        {
          int extent[6] = {0,-1,0,-1,0,-1};
          info->Set(WHOLE_EXTENT(), extent, 6);
        }
      }

      // Make sure an update request exists.
      // Request all data by default.
      vtkSDDPSetUpdateExtentToWholeExtent
        (outInfoVec->GetInformationObject(i));
    }
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
void
vtkStreamingDemandDrivenPipeline
::CopyDefaultInformation(vtkInformation* request, int direction,
                         vtkInformationVector** inInfoVec,
                         vtkInformationVector* outInfoVec)
{
  // Let the superclass copy first.
  this->Superclass::CopyDefaultInformation(request, direction,
                                           inInfoVec, outInfoVec);

  if(request->Has(REQUEST_INFORMATION()))
  {
    if(this->GetNumberOfInputPorts() > 0)
    {
      if(vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0))
      {
        vtkInformation* scalarInfo =
          vtkDataObject::GetActiveFieldInformation(
            inInfo,
            vtkDataObject::FIELD_ASSOCIATION_POINTS,
            vtkDataSetAttributes::SCALARS);
        // Copy information from the first input to all outputs.
        for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
        {
          vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
          outInfo->CopyEntry(inInfo, WHOLE_EXTENT());
          outInfo->CopyEntry(inInfo, TIME_STEPS());
          outInfo->CopyEntry(inInfo, TIME_RANGE());
          outInfo->CopyEntry(inInfo, vtkDataObject::ORIGIN());
          outInfo->CopyEntry(inInfo, vtkDataObject::SPACING());
          outInfo->CopyEntry(inInfo, TIME_DEPENDENT_INFORMATION());
          if (scalarInfo)
          {
            int scalarType = VTK_DOUBLE;
            if (scalarInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()))
            {
              scalarType = scalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
            }
            int numComp = 1;
            if (scalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
            {
              numComp = scalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
            }
            vtkDataObject::SetPointDataActiveScalarInfo(
              outInfo, scalarType, numComp);
          }
        }
      }
    }
  }

  if(request->Has(REQUEST_UPDATE_TIME()))
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

          // Copy the time request
          if ( outInfo->Has(UPDATE_TIME_STEP()) )
          {
            inInfo->CopyEntry(outInfo, UPDATE_TIME_STEP());
          }
        }
      }
    }
  }
  if(request->Has(REQUEST_UPDATE_EXTENT()))
  {
    // Get the output port from which to copy the extent.
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    // Initialize input extent to whole extent if it is not
    // already initialized.
    // This may be overwritten by the default code below as
    // well as what that an algorith may do.
    for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
      // Loop over all connections on this input port.
      int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
      for (int j=0; j<numInConnections; j++)
      {
        vtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
        vtkSDDPSetUpdateExtentToWholeExtent(inInfo);
      }
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

          // Copy the time request
          if ( outInfo->Has(UPDATE_TIME_STEP()) )
          {
            inInfo->CopyEntry(outInfo, UPDATE_TIME_STEP());
          }

          // If an algorithm wants an exact extent it must explicitly
          // add it to the request.  We do not want to get the setting
          // from another consumer of the same input.
          inInfo->Remove(EXACT_EXTENT());

          // Get the input data object for this connection.  It should
          // have already been created by the UpdateDataObject pass.
          vtkDataObject* inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
          if(!inData)
          {
            vtkErrorMacro("Cannot copy default update request from output port "
                          << outputPort << " on algorithm "
                          << this->Algorithm->GetClassName()
                          << "(" << this->Algorithm << ") to input connection "
                          << j << " on input port " << i
                          << " because there is no data object.");
            continue;
          }


          if (outInfo->Has(UPDATE_EXTENT()))
          {
            inInfo->CopyEntry(outInfo, UPDATE_EXTENT());
          }

          inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());

          inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void
vtkStreamingDemandDrivenPipeline
::ResetPipelineInformation(int port, vtkInformation* info)
{
  this->Superclass::ResetPipelineInformation(port, info);
  info->Remove(WHOLE_EXTENT());
  info->Remove(EXACT_EXTENT());
  info->Remove(UPDATE_EXTENT_INITIALIZED());
  info->Remove(UPDATE_EXTENT());
  info->Remove(UPDATE_PIECE_NUMBER());
  info->Remove(UPDATE_NUMBER_OF_PIECES());
  info->Remove(UPDATE_NUMBER_OF_GHOST_LEVELS());
  info->Remove(TIME_STEPS());
  info->Remove(TIME_RANGE());
  info->Remove(UPDATE_TIME_STEP());
  info->Remove(PREVIOUS_UPDATE_TIME_STEP());
  info->Remove(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST());
  info->Remove(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::PropagateUpdateExtent(int outputPort)
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("PropagateUpdateExtent", 0))
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
  if (!this->UpdateExtentRequest)
  {
    this->UpdateExtentRequest = vtkInformation::New();
    this->UpdateExtentRequest->Set(REQUEST_UPDATE_EXTENT());
    // The request is forwarded upstream through the pipeline.
    this->UpdateExtentRequest->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
    // Algorithms process this request before it is forwarded.
    this->UpdateExtentRequest->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  }

  this->UpdateExtentRequest->Set(FROM_OUTPUT_PORT(), outputPort);

  // Send the request.
  return this->ProcessRequest(this->UpdateExtentRequest,
                              this->GetInputInformation(),
                              this->GetOutputInformation());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::PropagateTime(int outputPort)
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("PropagateTime", 0))
  {
    return 0;
  }

  // Range check.
  if(outputPort < -1 ||
     outputPort >= this->Algorithm->GetNumberOfOutputPorts())
  {
    vtkErrorMacro("PropagateUpdateTime given output port index "
                  << outputPort << " on an algorithm with "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
  }

  // Setup the request for update extent propagation.
  vtkSmartPointer<vtkInformation> updateTimeRequest = vtkSmartPointer<vtkInformation>::New();

  //if (!this->UpdateExtentRequest)
  {
    updateTimeRequest->Set(REQUEST_UPDATE_TIME());
    // The request is forwarded upstream through the pipeline.
    updateTimeRequest->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
    // Algorithms process this request before it is forwarded.
    updateTimeRequest->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  }

  updateTimeRequest->Set(FROM_OUTPUT_PORT(), outputPort);

  // Send the request.
  return this->ProcessRequest(updateTimeRequest,
                              this->GetInputInformation(),
                              this->GetOutputInformation());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::UpdateTimeDependentInformation(int port)
{
  // The algorithm should not invoke anything on the executive.
  if(!this->CheckAlgorithm("UpdateMetaInformation", 0))
  {
    return 0;
  }
  // Setup the request for information.
  vtkSmartPointer<vtkInformation> timeRequest = vtkSmartPointer<vtkInformation>::New();
  timeRequest->Set(REQUEST_TIME_DEPENDENT_INFORMATION());
  // The request is forwarded upstream through the pipeline.
  timeRequest->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  timeRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

  timeRequest->Set(FROM_OUTPUT_PORT(), port);

  // Send the request.
  return this->ProcessRequest(timeRequest,
                              this->GetInputInformation(),
                              this->GetOutputInformation());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::VerifyOutputInformation(int outputPort,
                          vtkInformationVector** inInfoVec,
                          vtkInformationVector* outInfoVec)
{
  // If no port is specified, check all ports.
  if(outputPort < 0)
  {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
      if(!this->VerifyOutputInformation(i,inInfoVec,outInfoVec))
      {
        return 0;
      }
    }
    return 1;
  }

  // Get the information object to check.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);

  // Make sure there is a data object.  It is supposed to be created
  // by the UpdateDataObject step.
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!dataObject)
  {
    vtkErrorMacro("No data object has been set in the information for "
                  "output port " << outputPort << ".");
    return 0;
  }

  // Check extents.
  vtkInformation* dataInfo = dataObject->GetInformation();
  if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
  {
    // For an unstructured extent, make sure the update request
    // exists.  We do not need to check if it is valid because
    // out-of-range requests produce empty data.
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
  }
  else if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
  {
    // For a structured extent, make sure the update request
    // exists.
    if(!outInfo->Has(WHOLE_EXTENT()))
    {
      vtkErrorMacro("No whole extent has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
    }
    if(!outInfo->Has(UPDATE_EXTENT()))
    {
      vtkErrorMacro("No update extent has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
    }
    // Make sure the update request is inside the whole extent.
    int wholeExtent[6];
    int updateExtent[6];
    outInfo->Get(WHOLE_EXTENT(), wholeExtent);
    outInfo->Get(UPDATE_EXTENT(), updateExtent);
    if((updateExtent[0] < wholeExtent[0] ||
        updateExtent[1] > wholeExtent[1] ||
        updateExtent[2] < wholeExtent[2] ||
        updateExtent[3] > wholeExtent[3] ||
        updateExtent[4] < wholeExtent[4] ||
        updateExtent[5] > wholeExtent[5]) &&
       (updateExtent[0] <= updateExtent[1] &&
        updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5]))
    {
      if (!outInfo->Has(UNRESTRICTED_UPDATE_EXTENT()))
      {
        // Update extent is outside the whole extent and is not empty.
        vtkErrorMacro("The update extent specified in the "
                      "information for output port " << outputPort
                      << " on algorithm " << this->Algorithm->GetClassName()
                      << "(" << this->Algorithm << ") is "
                      << updateExtent[0] << " " << updateExtent[1] << " "
                      << updateExtent[2] << " " << updateExtent[3] << " "
                      << updateExtent[4] << " " << updateExtent[5]
                      << ", which is outside the whole extent "
                      << wholeExtent[0] << " " << wholeExtent[1] << " "
                      << wholeExtent[2] << " " << wholeExtent[3] << " "
                      << wholeExtent[4] << " " << wholeExtent[5] << ".");
        return 0;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void
vtkStreamingDemandDrivenPipeline
::ExecuteDataStart(vtkInformation* request,
                   vtkInformationVector** inInfoVec,
                   vtkInformationVector* outInfoVec)
{
  // Preserve the execution continuation flag in the request across
  // iterations of the algorithm.  Perform start operations only if
  // not in an execute continuation.
  if(this->ContinueExecuting)
  {
    request->Set(CONTINUE_EXECUTING(), 1);
  }
  else
  {
    request->Remove(CONTINUE_EXECUTING());
    this->Superclass::ExecuteDataStart(request,inInfoVec,outInfoVec);
  }

  int numInfo = outInfoVec->GetNumberOfInformationObjects();
  for(int i=0; i < numInfo ; ++i)
  {
    vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    int numPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    if (numPieces > 1)
    {
      int* uExt = outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
      if (uExt)
      {
        // Store the total requested extent in ALL_PIECES_EXTENT.
        // This can be different than DATA_EXTENT if the algorithm
        // produces multiple pieces.
        // NOTE: we store this in outInfo because data info gets
        // wiped during execute. We move this to data info in
        // ExecuteDataEnd.
        outInfo->Set(vtkDataObject::ALL_PIECES_EXTENT(), uExt, 6);
      }

      // If the algorithm is capable of producing sub-extents, use
      // an extent translator to break update extent request into
      // pieces.
      if (outInfo->Has(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT()))
      {
        int piece = outInfo->Get(UPDATE_PIECE_NUMBER());
        int ghost = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());

        int splitMode = vtkExtentTranslator::BLOCK_MODE;
        if (outInfo->Has(vtkExtentTranslator::UPDATE_SPLIT_MODE()))
        {
          splitMode = outInfo->Get(vtkExtentTranslator::UPDATE_SPLIT_MODE());
        }

        vtkExtentTranslator* et = vtkExtentTranslator::New();
        int execExt[6];
        et->PieceToExtentThreadSafe(piece, numPieces, ghost,
                                    uExt, execExt,
                                    splitMode, 0);
        et->Delete();
        outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                     execExt, 6);
      }
    }
  }
}

//----------------------------------------------------------------------------
void
vtkStreamingDemandDrivenPipeline
::ExecuteDataEnd(vtkInformation* request,
                 vtkInformationVector** inInfoVec,
                 vtkInformationVector* outInfoVec)
{
  int numInfo = outInfoVec->GetNumberOfInformationObjects();
  for(int i=0; i < numInfo ; ++i)
  {
    vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    int numPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    if (numPieces > 1)
    {
      vtkDataObject* dobj = outInfo->Get(vtkDataObject::DATA_OBJECT());

      // See ExecuteDataStart for an explanation of this key and
      // why we move it from outInfo to data info.
      if (outInfo->Has(vtkDataObject::ALL_PIECES_EXTENT()))
      {
        dobj->GetInformation()->Set(vtkDataObject::ALL_PIECES_EXTENT(),
                                    outInfo->Get(vtkDataObject::ALL_PIECES_EXTENT()),
                                    6);
      }

      if (outInfo->Has(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT()))
      {
        int ghost = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
        if (ghost > 0)
        {
          vtkDataSet* data = vtkDataSet::SafeDownCast(dobj);
          if (data)
          {
            int* uExt = data->GetInformation()->Get(
              vtkDataObject::ALL_PIECES_EXTENT());

            int piece = outInfo->Get(UPDATE_PIECE_NUMBER());

            vtkExtentTranslator* et = vtkExtentTranslator::New();
            int zeroExt[6];
            et->PieceToExtentThreadSafe(piece, numPieces, 0,
                                        uExt, zeroExt,
                                        vtkExtentTranslator::BLOCK_MODE, 0);
            et->Delete();

            data->GenerateGhostArray(zeroExt);
          }
        }

        // Restore the full update extent, as the subextent handling will
        // clobber it
        if (outInfo->Has(vtkDataObject::ALL_PIECES_EXTENT()))
        {
          outInfo->Set(UPDATE_EXTENT(),
                       outInfo->Get(vtkDataObject::ALL_PIECES_EXTENT()), 6);
        }
      }
      // Remove ALL_PIECES_EXTENT from outInfo (it was moved to the data obj
      // earlier).
      if (outInfo->Has(vtkDataObject::ALL_PIECES_EXTENT()))
      {
        outInfo->Remove(vtkDataObject::ALL_PIECES_EXTENT());
      }
    }
  }

  // Preserve the execution continuation flag in the request across
  // iterations of the algorithm.  Perform start operations only if
  // not in an execute continuation.
  if(request->Get(CONTINUE_EXECUTING()))
  {
    if (!this->ContinueExecuting)
    {
      this->ContinueExecuting = 1;
      this->Update(request->Get(FROM_OUTPUT_PORT()));
    }
  }
  else
  {
    if (this->ContinueExecuting)
    {
      this->ContinueExecuting = 0;
    }
    this->Superclass::ExecuteDataEnd(request,inInfoVec,outInfoVec);
  }
}

//----------------------------------------------------------------------------
void
vtkStreamingDemandDrivenPipeline
::MarkOutputsGenerated(vtkInformation* request,
                       vtkInformationVector** inInfoVec,
                       vtkInformationVector* outInfoVec)
{
  // Tell outputs they have been generated.
  this->Superclass::MarkOutputsGenerated(request,inInfoVec,outInfoVec);

  int outputPort = 0;
  if(request->Has(FROM_OUTPUT_PORT()))
  {
    outputPort = request->Get(FROM_OUTPUT_PORT());
    outputPort = (outputPort >= 0 ? outputPort : 0);
  }

  // Get the piece request from the update port (port 0 if none)
  // The defaults are:
  int piece = 0;
  int numPieces = 1;
  int ghostLevel = 0;
  vtkInformation* fromInfo = 0;
  if (outputPort < outInfoVec->GetNumberOfInformationObjects())
  {
    fromInfo = outInfoVec->GetInformationObject(outputPort);
    if (fromInfo->Has(UPDATE_PIECE_NUMBER()))
    {
      piece = fromInfo->Get(UPDATE_PIECE_NUMBER());
    }
    if (fromInfo->Has(UPDATE_NUMBER_OF_PIECES()))
    {
      numPieces = fromInfo->Get(UPDATE_NUMBER_OF_PIECES());
    }
    if (fromInfo->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
      ghostLevel = fromInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
  }

  for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
  {
    vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    vtkDataObject* data = outInfo->Get(vtkDataObject::DATA_OBJECT());
    // Compute ghost level arrays for generated outputs.
    if(data && !outInfo->Get(DATA_NOT_GENERATED()))
    {
      // Copy the update piece information from the update port to
      // the data piece information of all output ports UNLESS the
      // algorithm already specified it.
      vtkInformation* dataInfo = data->GetInformation();
      if (!dataInfo->Has(vtkDataObject::DATA_PIECE_NUMBER()) ||
          dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER()) == - 1)
      {
        dataInfo->Set(vtkDataObject::DATA_PIECE_NUMBER(), piece);
        dataInfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), numPieces);
        // If the source or filter produced a different number of ghost
        // levels, honor it.
        int dataGhostLevel = 0;
        if (dataInfo->Has(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS()))
        {
          dataGhostLevel =
            dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
        }
        // If the ghost level generated by the algorithm is larger than
        // requested, we keep it. Otherwise, we store the requested one.
        // We do this because there is no point in the algorithm re-executing
        // if the downstream asks for the same level even though the
        // algorithm cannot produce it.
        dataInfo->Set(
          vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(),
          ghostLevel > dataGhostLevel ? ghostLevel : dataGhostLevel);
      }

      // In this block, we make sure that DATA_TIME_STEP() is set if:
      // * There was someone upstream that supports time (TIME_RANGE() key
      //   is present)
      // * Someone downstream requested a timestep (UPDATE_TIME_STEP())
      //
      // A common situation in which the DATA_TIME_STEP() would not be
      // present even if the two conditions above are satisfied is when
      // a filter that is not time-aware is processing a dataset produced
      // by a time-aware source. In this case, DATA_TIME_STEP() should
      // be copied from input to output.
      //
      // Check if the output has DATA_TIME_STEP().
      if (!dataInfo->Has(vtkDataObject::DATA_TIME_STEP()) &&
          outInfo->Has(TIME_RANGE()))
      {
        // It does not.
        // Does the input have it? If yes, copy it.
        vtkDataObject* input = 0;
        if (this->GetNumberOfInputPorts() > 0)
        {
          input = this->GetInputData(0, 0);
        }
          if (input &&
              input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
          {
          dataInfo->CopyEntry(input->GetInformation(),
                              vtkDataObject::DATA_TIME_STEP(),
                              1);
          }
        // Does the update request have it? If yes, copy it. This
        // should not normally happen.
        else if (outInfo->Has(UPDATE_TIME_STEP()))
        {
          dataInfo->Set(vtkDataObject::DATA_TIME_STEP(),outInfo->Get(UPDATE_TIME_STEP()));
        }
      }

      // We are keeping track of the previous time request.
      if (fromInfo->Has(UPDATE_TIME_STEP()))
      {
        outInfo->Set(PREVIOUS_UPDATE_TIME_STEP(),fromInfo->Get(UPDATE_TIME_STEP()));
      }
      else
      {
        outInfo->Remove(PREVIOUS_UPDATE_TIME_STEP());
      }

      // Give the keys an opportunity to store meta-data in
      // the data object about what update request lead to
      // the last execution. This information can later be
      // used to decide whether an execution is necessary.
      vtkSmartPointer<vtkInformationIterator> infoIter =
        vtkSmartPointer<vtkInformationIterator>::New();
      infoIter->SetInformationWeak(outInfo);
      infoIter->InitTraversal();
      while(!infoIter->IsDoneWithTraversal())
      {
        vtkInformationKey* key = infoIter->GetCurrentKey();
        key->StoreMetaData(request, outInfo, dataInfo);
        infoIter->GoToNextItem();
      }

    }
  }
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::NeedToExecuteData(int outputPort,
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

  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
  int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());

  if (updateNumberOfPieces > 1 && updatePiece > 0)
  {
    // This is a source.
    if (this->Algorithm->GetNumberOfInputPorts() == 0)
    {
      // And cannot handle piece request (i.e. not parallel)
      // and is not a structured source that can produce sub-extents.
      if (!outInfo->Get(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST()) &&
          !outInfo->Get(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT()))
      {
        // Then don't execute it.
        return 0;
      }
    }
  }

  // Does the superclass want to execute?
  if(this->Superclass::NeedToExecuteData(outputPort,inInfoVec,outInfoVec))
  {
    return 1;
  }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* dataInfo = dataObject->GetInformation();

  // Check the unstructured extent.  If we do not have the requested
  // piece, we need to execute.
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
    if (dataPiece != updatePiece)
    {
      return 1;
    }
  }

  if (outInfo->Has(UPDATE_EXTENT())
      &&
      dataInfo->Has(vtkDataObject::DATA_EXTENT_TYPE()) &&
      dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT
      )
  {
    if (!dataInfo->Has(vtkDataObject::DATA_EXTENT()) &&
        !dataInfo->Has(vtkDataObject::ALL_PIECES_EXTENT()))
    {
      return 1;
    }

    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);

    int dataExtent[6];
    if (dataInfo->Has(vtkDataObject::ALL_PIECES_EXTENT()))
    {
      dataInfo->Get(vtkDataObject::ALL_PIECES_EXTENT(), dataExtent);
    }
    else
    {
      dataInfo->Get(vtkDataObject::DATA_EXTENT(), dataExtent);
    }

    // if the ue is out side the de
    if((updateExtent[0] < dataExtent[0] ||
        updateExtent[1] > dataExtent[1] ||
        updateExtent[2] < dataExtent[2] ||
        updateExtent[3] > dataExtent[3] ||
        updateExtent[4] < dataExtent[4] ||
        updateExtent[5] > dataExtent[5]) &&
       // and the ue is set
       (updateExtent[0] <= updateExtent[1] &&
        updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5]))
    {
      return 1;
    }
  }

  if (this->NeedToExecuteBasedOnTime(outInfo, dataObject))
  {
    return 1;
  }

  // Ask the keys if we need to execute. Keys can overwrite
  // NeedToExecute() to make their own decision about whether
  // what they are asking for is different than what is in the
  // data and whether the filter should execute.
  vtkSmartPointer<vtkInformationIterator> infoIter =
    vtkSmartPointer<vtkInformationIterator>::New();
  infoIter->SetInformationWeak(outInfo);

  infoIter->InitTraversal();
  while(!infoIter->IsDoneWithTraversal())
  {
    vtkInformationKey* key = infoIter->GetCurrentKey();
    if (key->NeedToExecute(outInfo, dataInfo))
    {
      return 1;
    }
    infoIter->GoToNextItem();
  }

  // We do not need to execute.
  return 0;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::NeedToExecuteBasedOnTime(
  vtkInformation* outInfo, vtkDataObject* dataObject)
{
  // If this algorithm does not provide time information and another
  // algorithm upstream did not provide time information, we do not
  // re-execute even if the time request changed.
  if (!outInfo->Has(TIME_RANGE()))
  {
    return 0;
  }

  vtkInformation *dataInfo = dataObject->GetInformation();
  // if we are requesting a particular update time index, check
  // if we have the desired time index.
  if ( outInfo->Has(UPDATE_TIME_STEP()) )
  {
    if (!dataInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      return 1;
    }

    double ustep = outInfo->Get(UPDATE_TIME_STEP());

    // First check if time request is the same as previous time request.
    // If the previous update request did not correspond to an existing
    // time step and the reader chose a time step with it's own logic, the
    // data time step will be different than the request. If the same time
    // step is requested again, there is no need to re-execute the
    // algorithm.  We know that it does not have this time step.
    if ( outInfo->Has(PREVIOUS_UPDATE_TIME_STEP()) )
    {
      if (outInfo->Has(UPDATE_TIME_STEP()))
      {
        bool match = true;
        double pstep = outInfo->Get(PREVIOUS_UPDATE_TIME_STEP());
        if (pstep != ustep)
        {
          match = false;
        }
        if (match)
        {
          return 0;
        }
      }
    }

    int hasdsteps = dataInfo->Has(vtkDataObject::DATA_TIME_STEP());
    int hasusteps = dataInfo->Has(UPDATE_TIME_STEP());

    double dstep = dataInfo->Get(vtkDataObject::DATA_TIME_STEP());
    if ( (hasdsteps && !hasusteps) || (!hasdsteps && hasusteps))
    {
      return 1;
    }
    if (dstep != ustep)
    {
      return 1;
    }

  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetWholeExtent(vtkInformation *info, int extent[6])
{
  if(!info)
  {
    vtkGenericWarningMacro("SetWholeExtent on invalid output");
    return 0;
  }
  int modified = 0;
  int oldExtent[6];
  vtkStreamingDemandDrivenPipeline::GetWholeExtent(info, oldExtent);
  if(oldExtent[0] != extent[0] || oldExtent[1] != extent[1] ||
     oldExtent[2] != extent[2] || oldExtent[3] != extent[3] ||
     oldExtent[4] != extent[4] || oldExtent[5] != extent[5])
  {
    modified = 1;
    info->Set(WHOLE_EXTENT(), extent, 6);
  }
  return modified;
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline
::GetWholeExtent(vtkInformation *info, int extent[6])
{
  static int emptyExtent[6] = {0,-1,0,-1,0,-1};
  if(!info)
  {
    memcpy(extent, emptyExtent, sizeof(int)*6);
    return;
  }
  if(!info->Has(WHOLE_EXTENT()))
  {
    info->Set(WHOLE_EXTENT(), emptyExtent, 6);
  }
  info->Get(WHOLE_EXTENT(), extent);
}

//----------------------------------------------------------------------------
int* vtkStreamingDemandDrivenPipeline::GetWholeExtent(vtkInformation* info)
{
  static int emptyExtent[6] = {0,-1,0,-1,0,-1};
  if(!info)
  {
    return emptyExtent;
  }
  if(!info->Has(WHOLE_EXTENT()))
  {
    info->Set(WHOLE_EXTENT(), emptyExtent, 6);
  }
  return info->Get(WHOLE_EXTENT());
}

// This is here to shut off warning about deprecated calls being
// made from other deprecated functions.
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef _MSC_VER
# pragma warning (disable: 4996)
#endif

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtentToWholeExtent(int port)
{
  return this->SetUpdateExtentToWholeExtent(this->GetOutputInformation(port));
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtentToWholeExtent(vtkInformation *info)
{
  if (!info)
  {
    vtkGenericWarningMacro("SetUpdateExtentToWholeExtent on invalid output");
    return 0;
  }

  // Request all data.
  int modified = 0;
  modified |=
    vtkStreamingDemandDrivenPipeline::SetUpdatePiece(info, 0);
  modified |=
    vtkStreamingDemandDrivenPipeline::SetUpdateNumberOfPieces(info, 1);
  modified |=
    vtkStreamingDemandDrivenPipeline::SetUpdateGhostLevel(info, 0);

  if(info->Has(WHOLE_EXTENT()))
  {
    int extent[6] = {0,-1,0,-1,0,-1};
    info->Get(WHOLE_EXTENT(), extent);
    modified |=
      vtkStreamingDemandDrivenPipeline::SetUpdateExtent(info, extent);
  }

  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(int port, int x0, int x1, int y0, int y1, int z0, int z1)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateExtent, "VTK 7.1");
  int extent[6] = {x0, x1, y0, y1, z0, z1};
  return this->SetUpdateExtent(
    this->GetOutputInformation(port), extent);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(int port, int extent[6])
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateExtent, "VTK 7.1");
  return this->SetUpdateExtent(
    this->GetOutputInformation(port), extent);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(vtkInformation *info, int extent[6])
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateExtent, "VTK 7.1");
  if(!info)
  {
    vtkGenericWarningMacro("SetUpdateExtent on invalid output");
    return 0;
  }
  int modified = 0;
  int oldExtent[6];
  vtkStreamingDemandDrivenPipeline::GetUpdateExtent(info, oldExtent);
  if(oldExtent[0] != extent[0] || oldExtent[1] != extent[1] ||
     oldExtent[2] != extent[2] || oldExtent[3] != extent[3] ||
     oldExtent[4] != extent[4] || oldExtent[5] != extent[5])
  {
    modified = 1;
    info->Set(UPDATE_EXTENT(), extent, 6);
  }
  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(int port, int piece,int numPieces, int ghostLevel)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateExtent, "VTK 7.1");
  return this->SetUpdateExtent(
    this->GetOutputInformation(port), piece, numPieces, ghostLevel);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(vtkInformation *info, int piece,
                  int numPieces,
                  int ghostLevel)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateExtent, "VTK 7.1");
  if(!info)
  {
    vtkGenericWarningMacro("SetUpdateExtent on invalid output");
    return 0;
  }
  int modified = 0;
  modified |= vtkStreamingDemandDrivenPipeline::SetUpdatePiece(
    info, piece);
  modified |= vtkStreamingDemandDrivenPipeline::SetUpdateNumberOfPieces(
    info, numPieces);
  modified |= vtkStreamingDemandDrivenPipeline::SetUpdateGhostLevel(
    info, ghostLevel);

  return modified;
}
#endif // VTK_LEGACY_REMOVE

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline
::GetUpdateExtent(vtkInformation *info, int extent[6])
{
  static int emptyExtent[6] = {0,-1,0,-1,0,-1};
  if(!info)
  {
    vtkGenericWarningMacro("GetUpdateExtent on invalid output");
    memcpy(extent, emptyExtent, sizeof(int)*6);
    return;
  }
  if(!info->Has(UPDATE_EXTENT()))
  {
    info->Set(UPDATE_EXTENT(), emptyExtent, 6);
  }
  info->Get(UPDATE_EXTENT(), extent);
}

//----------------------------------------------------------------------------
int* vtkStreamingDemandDrivenPipeline
::GetUpdateExtent(vtkInformation *info)
{
  static int emptyExtent[6] = {0,-1,0,-1,0,-1};
  if(!info)
  {
    vtkGenericWarningMacro("GetUpdateExtent on invalid output");
    return emptyExtent;
  }
  if(!info->Has(UPDATE_EXTENT()))
  {
    info->Set(UPDATE_EXTENT(), emptyExtent, 6);
  }
  return info->Get(UPDATE_EXTENT());
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdatePiece(vtkInformation *info, int piece)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdatePiece, "VTK 7.1");
  if(!info)
  {
    vtkGenericWarningMacro("SetUpdatePiece on invalid output");
    return 0;
  }
  int modified = 0;
  if(vtkStreamingDemandDrivenPipeline::GetUpdatePiece(info) != piece)
  {
    info->Set(UPDATE_PIECE_NUMBER(), piece);
    modified = 1;
  }
  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetUpdateTimeStep(int port, double time)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateTimeStep, "VTK 7.1");
  return this->SetUpdateTimeStep(this->GetOutputInformation(port), time);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetUpdateTimeStep(vtkInformation *info, double time)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateTimeStep, "VTK 7.1");
  if(!info)
  {
    vtkGenericWarningMacro("SetUpdateTimeSteps on invalid output");
    return 0;
  }
  int modified = 0;
  if (info->Has(UPDATE_TIME_STEP()))
  {
    double oldStep = info->Get(UPDATE_TIME_STEP());
    if (oldStep != time)
    {
      modified = 1;
    }
  }
  else
  {
    modified = 1;
  }
  if (modified)
  {
    info->Set(UPDATE_TIME_STEP(),time);
  }
  return modified;
}
#endif // VTK_LEGACY_REMOVE

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::GetUpdatePiece(vtkInformation *info)
{
  if(!info)
  {
    vtkGenericWarningMacro("GetUpdatePiece on invalid output");
    return 0;
  }
  if(!info->Has(UPDATE_PIECE_NUMBER()))
  {
    info->Set(UPDATE_PIECE_NUMBER(), 0);
  }
  return info->Get(UPDATE_PIECE_NUMBER());
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateNumberOfPieces(vtkInformation *info, int n)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateNumberOfPieces,
    "VTK 7.1");
  if(!info)
  {
    vtkGenericWarningMacro("SetUpdateNumberOfPieces on invalid output");
    return 0;
  }
  int modified = 0;
  if(vtkStreamingDemandDrivenPipeline::GetUpdateNumberOfPieces(info) != n)
  {
    info->Set(UPDATE_NUMBER_OF_PIECES(), n);
    modified = 1;
  }
  return modified;
}
#endif
//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::GetUpdateNumberOfPieces(vtkInformation *info)
{
  if(!info)
  {
    vtkGenericWarningMacro("GetUpdateNumberOfPieces on invalid output");
    return 1;
  }
  if(!info->Has(UPDATE_NUMBER_OF_PIECES()))
  {
    info->Set(UPDATE_NUMBER_OF_PIECES(), 1);
  }
  return info->Get(UPDATE_NUMBER_OF_PIECES());
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateGhostLevel(vtkInformation *info, int n)
{
  VTK_LEGACY_BODY(vtkStreamingDemandDrivenPipeline::SetUpdateGhostLevel,
    "VTK 7.1");
  if(!info)
  {
    vtkGenericWarningMacro("SetUpdateGhostLevel on invalid output");
    return 0;
  }
  if(vtkStreamingDemandDrivenPipeline::GetUpdateGhostLevel(info) != n)
  {
    info->Set(UPDATE_NUMBER_OF_GHOST_LEVELS(), n);
    return 1;
  }
  return 0;
}
#endif

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::GetUpdateGhostLevel(vtkInformation *info)
{
  if(!info)
  {
    vtkGenericWarningMacro("GetUpdateGhostLevel on invalid output");
    return 0;
  }
  if(!info->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
  {
    info->Set(UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  return info->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetRequestExactExtent(int port, int flag)
{
  if(!this->OutputPortIndexInRange(port, "set request exact extent flag on"))
  {
    return 0;
  }
  vtkInformation* info = this->GetOutputInformation(port);
  if(this->GetRequestExactExtent(port) != flag)
  {
    info->Set(EXACT_EXTENT(), flag);
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::GetRequestExactExtent(int port)
{
  if(!this->OutputPortIndexInRange(port, "get request exact extent flag from"))
  {
    return 0;
  }
  vtkInformation* info = this->GetOutputInformation(port);
  if(!info->Has(EXACT_EXTENT()))
  {
    info->Set(EXACT_EXTENT(), 0);
  }
  return info->Get(EXACT_EXTENT());
}
