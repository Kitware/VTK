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
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkStreamingDemandDrivenPipeline);

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, CONTINUE_EXECUTING, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, EXACT_EXTENT, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_EXTENT, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_TIME, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_TIME_DEPENDENT_INFORMATION, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_EXTENT_INFORMATION, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_MANAGE_INFORMATION, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REQUEST_RESOLUTION_PROPAGATE, Request);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, MAXIMUM_NUMBER_OF_PIECES, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_EXTENT_INITIALIZED, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_PIECE_NUMBER, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_NUMBER_OF_PIECES, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_NUMBER_OF_GHOST_LEVELS, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_EXTENT_TRANSLATED, Integer);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, WHOLE_EXTENT, IntegerVector, 6);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, UPDATE_EXTENT, IntegerVector, 6);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, COMBINED_UPDATE_EXTENT, IntegerVector, 6);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UNRESTRICTED_UPDATE_EXTENT, Integer);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline,
                                 EXTENT_TRANSLATOR, ObjectBase,
                                 "vtkExtentTranslator");
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, WHOLE_BOUNDING_BOX, DoubleVector, 6);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_STEPS, DoubleVector);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_TIME_STEP, Double);

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, PREVIOUS_UPDATE_TIME_STEP, Double);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_RANGE, DoubleVector);

vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, PIECE_BOUNDING_BOX, DoubleVector, 6);
vtkInformationKeyRestrictedMacro(vtkStreamingDemandDrivenPipeline, PIECE_NORMAL, DoubleVector, 3);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, PRIORITY, Double);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, ORIGINAL_NUMBER_OF_CELLS, UnsignedLong);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_RESOLUTION, Double);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, REMOVE_ATTRIBUTE_INFORMATION, Integer);

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, FAST_PATH_FOR_TEMPORAL_DATA, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, FAST_PATH_OBJECT_TYPE, String);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, FAST_PATH_ID_TYPE, String);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, FAST_PATH_OBJECT_ID, IdType);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, PREVIOUS_FAST_PATH_OBJECT_ID, IdType);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, PREVIOUS_FAST_PATH_OBJECT_TYPE, String);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, PREVIOUS_FAST_PATH_ID_TYPE, String);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, UPDATE_AMR_LEVEL, Integer );

vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, BOUNDS, DoubleVector);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_DEPENDENT_INFORMATION, Integer);
vtkInformationKeyMacro(vtkStreamingDemandDrivenPipeline, TIME_LABEL_ANNOTATION, String);

//----------------------------------------------------------------------------
class vtkStreamingDemandDrivenPipelineToDataObjectFriendship
{
public:
  static void Crop(vtkDataObject* obj, const int* extent)
    {
    obj->Crop(extent);
    }
};

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
        ((updateExtent = outInfo->Get(UPDATE_EXTENT())) != 0) &&
        !outInfo->Has(UPDATE_RESOLUTION()))
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
          else
            {
            if (outInfo->Get(UPDATE_RESOLUTION()) !=
                inInfo->Get(UPDATE_RESOLUTION()))
              {
              N2E = 1;
              }
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
        // Remove update-related keys from the input information.
        this->ResetUpdateInformation(request, inInfoVec, outInfoVec);

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
    if (!this->NeedToExecuteData(outputPort,inInfoVec,outInfoVec))
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
    this->SetUpdateExtentToWholeExtent
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
        this->SetUpdateExtentToWholeExtent(inInfo);
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
      // Set default maximum request.
      if(data->GetExtentType() == VTK_PIECES_EXTENT)
        {
        if(!info->Has(MAXIMUM_NUMBER_OF_PIECES()))
          {
          info->Set(MAXIMUM_NUMBER_OF_PIECES(), -1);
          }
        }
      else if(data->GetExtentType() == VTK_3D_EXTENT)
        {
        if(!info->Has(WHOLE_EXTENT()))
          {
          int extent[6] = {0,-1,0,-1,0,-1};
          info->Set(WHOLE_EXTENT(), extent, 6);
          }
        }

      // Make sure an update request exists.
      if(!info->Has(UPDATE_EXTENT_INITIALIZED()) ||
         !info->Get(UPDATE_EXTENT_INITIALIZED()))
        {
        // Request all data by default.
        this->SetUpdateExtentToWholeExtent
          (outInfoVec->GetInformationObject(i));
        }
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
          outInfo->CopyEntry(inInfo, WHOLE_BOUNDING_BOX());
          outInfo->CopyEntry(inInfo, WHOLE_EXTENT());
          outInfo->CopyEntry(inInfo, MAXIMUM_NUMBER_OF_PIECES());
          outInfo->CopyEntry(inInfo, EXTENT_TRANSLATOR());
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

    // Setup default information for the outputs.
    for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
      {
      vtkInformation* outInfo = outInfoVec->GetInformationObject(i);

      // The data object will exist because UpdateDataObject has already
      // succeeded. Except when this method is called by a subclass
      // that does not provide this key in certain cases.
      vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
      if (!dataObject)
        {
        continue;
        }
      vtkInformation* dataInfo = dataObject->GetInformation();
      if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) ==
         VTK_PIECES_EXTENT)
        {
        if (!outInfo->Has(MAXIMUM_NUMBER_OF_PIECES()))
          {
          if (this->GetNumberOfInputPorts() > 0)
            {
            // must have structured input; MAXIMUM_NUMBER_OF_PIECES will
            // not be copied above (CopyEntry does nothing since key not set
            // in inInfo); set to -1
            outInfo->Set(MAXIMUM_NUMBER_OF_PIECES(), -1);
            }
          else
            {
            // Since most unstructured filters in VTK generate all their
            // data once, set the default maximum number of pieces to 1.
            outInfo->Set(MAXIMUM_NUMBER_OF_PIECES(), 1);
            }
          }
        }
      else if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
        {
        if(!outInfo->Has(EXTENT_TRANSLATOR()) ||
           !outInfo->Get(EXTENT_TRANSLATOR()))
          {
          // Create a default extent translator.
          vtkExtentTranslator* translator = vtkExtentTranslator::New();
          outInfo->Set(EXTENT_TRANSLATOR(), translator);
          translator->Delete();
          }
        }
      }
    }

  if(request->Has(REQUEST_UPDATE_TIME()))
    {
    //Copy requested resolution back
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
    //Copy requested resolution back
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
      vtkDataObject* outData = outInfo->Get(vtkDataObject::DATA_OBJECT());

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

          // Copy the fast-path-specific keys
          if ( outInfo->Has(FAST_PATH_OBJECT_ID()) )
            {
            inInfo->CopyEntry(outInfo, FAST_PATH_OBJECT_ID());
            }

          if ( outInfo->Has(FAST_PATH_OBJECT_TYPE()) )
            {
            inInfo->CopyEntry(outInfo, FAST_PATH_OBJECT_TYPE());
            }

          if ( outInfo->Has(FAST_PATH_ID_TYPE()) )
            {
            inInfo->CopyEntry(outInfo, FAST_PATH_ID_TYPE());
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

          //Copy requested resolution back
          inInfo->CopyEntry(outInfo, UPDATE_RESOLUTION());

          // Consider all combinations of extent types.
          if(inData->GetExtentType() == VTK_PIECES_EXTENT)
            {
            if(outData->GetExtentType() == VTK_PIECES_EXTENT)
              {
              if (outInfo->Get(UPDATE_PIECE_NUMBER()) < 0)
                {
                return;
                }
              inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
              inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
              inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
              inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
              }
            else if(outData->GetExtentType() == VTK_3D_EXTENT)
              {
              // The conversion from structrued requests to
              // unstrcutured requests is always to request the whole
              // extent.
              this->SetUpdateExtentToWholeExtent(inInfo);
              }
            }
          else if(inData->GetExtentType() == VTK_3D_EXTENT)
            {
            if (outInfo->Get(UPDATE_PIECE_NUMBER()) >= 0)
              {
              // Although only the extent is used when processing
              // structured datasets, this is still passed to let
              // algorithms know what the actual request was.
              inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
              inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
              inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
              }

            if(outData->GetExtentType() == VTK_PIECES_EXTENT)
              {
              int piece = outInfo->Get(UPDATE_PIECE_NUMBER());
              int numPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
              int ghostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
              if (piece >= 0)
                {
                this->SetUpdateExtent(inInfo, piece, numPieces, ghostLevel);
                }
              }
            else if(outData->GetExtentType() == VTK_3D_EXTENT)
              {
              inInfo->CopyEntry(outInfo, UPDATE_EXTENT());
              inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
              }
            }
          }
        }
      }
    }
  if(request->Has(REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    // Copy the meta information across that algorithm as long as
    // the algorithm doesn't change the information that the meta-information
    // is about.
    if(this->GetNumberOfInputPorts() > 0 &&
       inInfoVec[0]->GetNumberOfInformationObjects() > 0)
      {
      vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
      int oiobj = outInfoVec->GetNumberOfInformationObjects();
      for(int i=0; i < oiobj; ++i)
        {
        vtkInformation* outInfo = outInfoVec->GetInformationObject(i);

        // Copy these things across always, and let algorithms modify in RUEI
        // if they need to.
        outInfo->CopyEntry(inInfo, PRIORITY());
        outInfo->CopyEntry(inInfo, ORIGINAL_NUMBER_OF_CELLS());
        outInfo->CopyEntry(inInfo, PIECE_NORMAL());

        vtkInformation *algsProps = this->GetAlgorithm()->GetInformation();
        if (algsProps->Has(vtkAlgorithm::MANAGES_METAINFORMATION()))
          {
          //Let the algorithm decide what to do with the met informaiton
          this->InAlgorithm = 1;
          vtkInformation *newrequest = vtkInformation::New();
          newrequest->Set(REQUEST_MANAGE_INFORMATION());
          this->Algorithm->ProcessRequest(newrequest, inInfoVec, outInfoVec);
          this->InAlgorithm = 0;

          //if algorithm said to, remove the attribute range information downstream
          if(newrequest->Has(REMOVE_ATTRIBUTE_INFORMATION()))
            {
            request->Set(REMOVE_ATTRIBUTE_INFORMATION(), 1);

            //remove the attribute range information downstream
            vtkInformationVector *miv;
            miv = outInfo->Get(vtkDataObject::CELL_DATA_VECTOR());
            if (miv)
              {
              int nArrays = miv->GetNumberOfInformationObjects();
              for (int n = 0; n < nArrays; n++)
                {
                vtkInformation *oArray = miv->GetInformationObject(n);
                oArray->Remove(vtkDataObject::FIELD_ARRAY_NAME());
                oArray->Remove(vtkDataObject::PIECE_FIELD_RANGE());
                }
              }
            miv = outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
            if (miv)
              {
              int nArrays = miv->GetNumberOfInformationObjects();
              for (int n = 0; n < nArrays; n++)
                {
                vtkInformation *oArray = miv->GetInformationObject(n);
                oArray->Remove(vtkDataObject::FIELD_ARRAY_NAME());
                oArray->Remove(vtkDataObject::PIECE_FIELD_RANGE());
                }
              }
            }
          newrequest->Delete();
          }
        else
          {
          //Copy the attribute meta information when algorithm is known not to modify it
          if (
              algsProps->Has(vtkAlgorithm::PRESERVES_RANGES()) ||
              algsProps->Has(vtkAlgorithm::PRESERVES_ATTRIBUTES()) ||
              algsProps->Has(vtkAlgorithm::PRESERVES_DATASET())
              )
            {
            if (inInfo->Has(vtkDataObject::CELL_DATA_VECTOR()))
              {
              outInfo->CopyEntry(inInfo, vtkDataObject::CELL_DATA_VECTOR(), 1);
              }
            if (inInfo->Has(vtkDataObject::POINT_DATA_VECTOR()))
              {
              outInfo->CopyEntry(inInfo, vtkDataObject::POINT_DATA_VECTOR(), 1);
              }
            }
          else
            {
            //RI normally passes it on always, so this flag says remove it downstream
            request->Set(REMOVE_ATTRIBUTE_INFORMATION(), 1);
            }

          //remove the attribute range information downstream
          if(request->Has(REMOVE_ATTRIBUTE_INFORMATION()))
            {
            vtkInformationVector *miv;
            miv = outInfo->Get(vtkDataObject::CELL_DATA_VECTOR());
            if (miv)
              {
              int nArrays = miv->GetNumberOfInformationObjects();
              for (int n = 0; n < nArrays; n++)
                {
                vtkInformation *oArray = miv->GetInformationObject(n);
                oArray->Remove(vtkDataObject::FIELD_ARRAY_NAME());
                oArray->Remove(vtkDataObject::PIECE_FIELD_RANGE());
                }
              }
            miv = outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
            if (miv)
              {
              int nArrays = miv->GetNumberOfInformationObjects();
              for (int n = 0; n < nArrays; n++)
                {
                vtkInformation *oArray = miv->GetInformationObject(n);
                oArray->Remove(vtkDataObject::FIELD_ARRAY_NAME());
                oArray->Remove(vtkDataObject::PIECE_FIELD_RANGE());
                }
              }
            }

          // Copy the geometric meta information when algorithm is known not to modify it
          if (
              algsProps->Has(vtkAlgorithm::PRESERVES_BOUNDS()) ||
              algsProps->Has(vtkAlgorithm::PRESERVES_GEOMETRY()) ||
              algsProps->Has(vtkAlgorithm::PRESERVES_DATASET())
              )
            {
            outInfo->CopyEntry(inInfo, PIECE_BOUNDING_BOX());
            }

          // Copy the topological meta information when algorithm is known not to modify it
          if (
              algsProps->Has(vtkAlgorithm::PRESERVES_TOPOLOGY()) ||
              algsProps->Has(vtkAlgorithm::PRESERVES_DATASET())
              )
            {
            outInfo->CopyEntry(inInfo, vtkDataObject::DATA_GEOMETRY_UNMODIFIED());
            }
          }
        }
      }
    }

  if(request->Has(REQUEST_RESOLUTION_PROPAGATE()))
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

          //Copy requested resolution back
          inInfo->CopyEntry(outInfo, UPDATE_RESOLUTION());

          vtkDataObject *inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
          if (inData)
            {
            vtkInformation* dataInfo = inData->GetInformation();
            dataInfo->Set(vtkDataObject::DATA_RESOLUTION(), -1.0);
            }
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
  info->Remove(MAXIMUM_NUMBER_OF_PIECES());
  info->Remove(EXTENT_TRANSLATOR());
  info->Remove(EXACT_EXTENT());
  info->Remove(UPDATE_EXTENT_INITIALIZED());
  info->Remove(UPDATE_EXTENT());
  info->Remove(UPDATE_PIECE_NUMBER());
  info->Remove(UPDATE_RESOLUTION());
  info->Remove(UPDATE_NUMBER_OF_PIECES());
  info->Remove(UPDATE_NUMBER_OF_GHOST_LEVELS());
  info->Remove(UPDATE_EXTENT_TRANSLATED());
  info->Remove(TIME_STEPS());
  info->Remove(TIME_RANGE());
  info->Remove(UPDATE_TIME_STEP());
  info->Remove(PREVIOUS_UPDATE_TIME_STEP());
  info->Remove(FAST_PATH_OBJECT_ID());
  info->Remove(FAST_PATH_OBJECT_TYPE());
  info->Remove(FAST_PATH_ID_TYPE());
  info->Remove(PREVIOUS_FAST_PATH_OBJECT_ID());
  info->Remove(PREVIOUS_FAST_PATH_OBJECT_TYPE());
  info->Remove(PREVIOUS_FAST_PATH_ID_TYPE());
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
        if (!outInfo->Has(UPDATE_RESOLUTION()))
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
          }
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
}

//----------------------------------------------------------------------------
void
vtkStreamingDemandDrivenPipeline
::ExecuteDataEnd(vtkInformation* request,
                 vtkInformationVector** inInfoVec,
                 vtkInformationVector* outInfoVec)
{
  // Preserve the execution continuation flag in the request across
  // iterations of the algorithm.  Perform start operations only if
  // not in an execute continuation.
  if(request->Get(CONTINUE_EXECUTING()))
    {
    this->ContinueExecuting = 1;
    this->Update(request->Get(FROM_OUTPUT_PORT()));
    }
  else
    {
    this->ContinueExecuting = 0;
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
      if(vtkDataSet* ds = vtkDataSet::SafeDownCast(data))
        {
        // Generate ghost level arrays automatically only if the extent
        // was set through translation. Otherwise, 1. there is no need
        // for a ghost array 2. it may be wrong
        if (outInfo->Has(UPDATE_EXTENT_TRANSLATED()))
          {
          if (outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS()) > 0)
            {
            ds->GenerateGhostLevelArray(
              outInfo->Get(UPDATE_PIECE_NUMBER()),
              outInfo->Get(UPDATE_NUMBER_OF_PIECES()),
              outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS()),
              outInfo->Get(WHOLE_EXTENT()),
              vtkExtentTranslator::SafeDownCast(outInfo->Get(EXTENT_TRANSLATOR())));
            }
          }
        }

      // Copy the update piece information from the update port to
      // the data piece information of all output ports UNLESS the
      // algorithm already specified it.
      vtkInformation* dataInfo = data->GetInformation();
      if (!dataInfo->Has(vtkDataObject::DATA_PIECE_NUMBER()) ||
          dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER()) == - 1)
        {
        dataInfo->Set(vtkDataObject::DATA_PIECE_NUMBER(), piece);
        dataInfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), numPieces);
        dataInfo->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
        }

      if (!dataInfo->Has(vtkDataObject::DATA_RESOLUTION()) &&
          outInfo->Has(UPDATE_RESOLUTION()))
        {
        // It does not.
        // Does the input have it? If yes, copy it.
        vtkDataObject* input = 0;
        if (this->GetNumberOfInputPorts() > 0)
          {
          input = this->GetInputData(0, 0);
          }
        if (input &&
            input->GetInformation()->Has(vtkDataObject::DATA_RESOLUTION()))
          {
          dataInfo->CopyEntry(input->GetInformation(),
                              vtkDataObject::DATA_RESOLUTION(),
                              1);
          }
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

      // We are keeping track of the previous fast-path keys.
      if (outInfo->Has(FAST_PATH_OBJECT_ID()))
        {
        outInfo->Set(PREVIOUS_FAST_PATH_OBJECT_ID(),
                     outInfo->Get(FAST_PATH_OBJECT_ID()));
        }
      else
        {
        outInfo->Remove(PREVIOUS_FAST_PATH_OBJECT_ID());
        }
      if (outInfo->Has(FAST_PATH_OBJECT_TYPE()))
        {
        outInfo->Set(PREVIOUS_FAST_PATH_OBJECT_TYPE(),
                     outInfo->Get(FAST_PATH_OBJECT_TYPE()));
        }
      else
        {
        outInfo->Remove(PREVIOUS_FAST_PATH_OBJECT_TYPE());
        }
      if (outInfo->Has(FAST_PATH_ID_TYPE()))
        {
        outInfo->Set(PREVIOUS_FAST_PATH_ID_TYPE(),
                     outInfo->Get(FAST_PATH_ID_TYPE()));
        }
      else
        {
        outInfo->Remove(PREVIOUS_FAST_PATH_ID_TYPE());
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

  // Does the superclass want to execute?
  if(this->Superclass::NeedToExecuteData(outputPort,inInfoVec,outInfoVec))
    {
    return 1;
    }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* dataInfo = dataObject->GetInformation();
  double updateResolution = outInfo->Get(UPDATE_RESOLUTION());
  double dataResolution = dataInfo->Get(vtkDataObject::DATA_RESOLUTION());
  if (dataResolution == -1.0 || updateResolution > dataResolution)
    {
    return 1;
    }

  if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
    {
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
    }
  else if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
    {
    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int dataExtent[6];
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);
    dataInfo->Get(vtkDataObject::DATA_EXTENT(), dataExtent);
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

  if (this->NeedToExecuteBasedOnFastPathData(outInfo))
    {
    return 1;
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
int vtkStreamingDemandDrivenPipeline::NeedToExecuteBasedOnFastPathData(
  vtkInformation* outInfo)
{
  // If this algorithm does not provide a temporal fast-path, we do not
  // re-execute.
  if (!outInfo->Has(FAST_PATH_FOR_TEMPORAL_DATA()) ||
      (!outInfo->Has(FAST_PATH_OBJECT_ID()) &&
       !outInfo->Has(FAST_PATH_OBJECT_TYPE()) &&
       !outInfo->Has(FAST_PATH_ID_TYPE())) )
    {
    return 0;
    }

  // When all the fast-path keys are the same as all the previous ones,
  // don't re-execute.
  if (outInfo->Has(FAST_PATH_OBJECT_ID()) &&
      outInfo->Has(FAST_PATH_OBJECT_TYPE()) &&
      outInfo->Has(FAST_PATH_ID_TYPE()) &&
      outInfo->Has(PREVIOUS_FAST_PATH_OBJECT_ID()) &&
      outInfo->Has(PREVIOUS_FAST_PATH_OBJECT_TYPE()) &&
      outInfo->Has(PREVIOUS_FAST_PATH_ID_TYPE()))
    {
    if( (outInfo->Get(FAST_PATH_OBJECT_ID()) ==
            outInfo->Get(PREVIOUS_FAST_PATH_OBJECT_ID())) &&
        (strcmp(outInfo->Get(FAST_PATH_OBJECT_TYPE()),
                outInfo->Get(PREVIOUS_FAST_PATH_OBJECT_TYPE())) == 0) &&
        (strcmp(outInfo->Get(FAST_PATH_ID_TYPE()),
                outInfo->Get(PREVIOUS_FAST_PATH_ID_TYPE())) == 0) )
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetMaximumNumberOfPieces(int port, int n)
{
  return this->SetMaximumNumberOfPieces(this->GetOutputInformation(port), n);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetMaximumNumberOfPieces(vtkInformation *info, int n)
{
  if(!info)
    {
    vtkGenericWarningMacro("SetMaximumNumberOfPieces on invalid output");
    return 0;
    }
  if(vtkStreamingDemandDrivenPipeline::GetMaximumNumberOfPieces(info) != n)
    {
    info->Set(MAXIMUM_NUMBER_OF_PIECES(), n);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::GetMaximumNumberOfPieces(int port)
{
  return this->GetMaximumNumberOfPieces(this->GetOutputInformation(port));
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::GetMaximumNumberOfPieces(vtkInformation *info)
{
  if(!info)
    {
    vtkGenericWarningMacro("GetMaximumNumberOfPieces on invalid output");
    return 0;
    }
  if(!info->Has(MAXIMUM_NUMBER_OF_PIECES()))
    {
    info->Set(MAXIMUM_NUMBER_OF_PIECES(), -1);
    }
  return info->Get(MAXIMUM_NUMBER_OF_PIECES());
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
  if(vtkDataObject* data = info->Get(vtkDataObject::DATA_OBJECT()))
    {
    if(data->GetExtentType() == VTK_PIECES_EXTENT)
      {
      modified |=
        vtkStreamingDemandDrivenPipeline::SetUpdatePiece(info, 0);
      modified |=
        vtkStreamingDemandDrivenPipeline::SetUpdateNumberOfPieces(info, 1);
      modified |=
        vtkStreamingDemandDrivenPipeline::SetUpdateGhostLevel(info, 0);
      }
    else if(data->GetExtentType() == VTK_3D_EXTENT)
      {
      int extent[6] = {0,-1,0,-1,0,-1};
      info->Get(WHOLE_EXTENT(), extent);
      modified |=
        vtkStreamingDemandDrivenPipeline::SetUpdateExtent(info, extent);
      }
    }
  else
    {
    vtkGenericWarningMacro("SetUpdateExtentToWholeExtent called with no data object.");
    }

  // Make sure the update extent will remain the whole extent until
  // the update extent is explicitly set by the caller.
  info->Set(UPDATE_EXTENT_INITIALIZED(), 0);

  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(int port, int x0, int x1, int y0, int y1, int z0, int z1)
{
  int extent[6] = {x0, x1, y0, y1, z0, z1};
  return this->SetUpdateExtent(
    this->GetOutputInformation(port), extent);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(int port, int extent[6])
{
  return this->SetUpdateExtent(
    this->GetOutputInformation(port), extent);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(vtkInformation *info, int extent[6])
{
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
  info->Set(UPDATE_EXTENT_INITIALIZED(), 1);
  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(int port, int piece,int numPieces, int ghostLevel)
{
  return this->SetUpdateExtent(
    this->GetOutputInformation(port), piece, numPieces, ghostLevel);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateExtent(vtkInformation *info, int piece,
                  int numPieces,
                  int ghostLevel)
{
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
  if(vtkDataObject* data = info->Get(vtkDataObject::DATA_OBJECT()))
    {
    if(data->GetExtentType() == VTK_3D_EXTENT)
      {
      if(vtkExtentTranslator* translator =
         vtkStreamingDemandDrivenPipeline::GetExtentTranslator(info))
        {
        int wholeExtent[6];
        vtkStreamingDemandDrivenPipeline::GetWholeExtent(info, wholeExtent);
        translator->SetWholeExtent(wholeExtent);
        translator->SetPiece(piece);
        translator->SetNumberOfPieces(numPieces);
        translator->SetGhostLevel(ghostLevel);
        translator->PieceToExtent();
        modified |=
          vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
            info, translator->GetExtent());
        info->Set(UPDATE_EXTENT_TRANSLATED(), 1);
        }
      else
        {
        vtkGenericWarningMacro("Cannot translate unstructured extent to structured.");
        }
      }
    }
  return modified;
}

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
    info->Set(UPDATE_EXTENT_INITIALIZED(), 0);
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
    info->Set(UPDATE_EXTENT_INITIALIZED(), 0);
    }
  return info->Get(UPDATE_EXTENT());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdatePiece(vtkInformation *info, int piece)
{
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
  info->Set(UPDATE_EXTENT_INITIALIZED(), 1);
  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateResolution(int port, double resolution)
{
  return this->SetUpdateResolution(GetOutputInformation(port), resolution);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateResolution(vtkInformation *info, double resolution)
{
  if(!info)
    {
    vtkErrorMacro("SetUpdateResolution on invalid output");
    return 0;
    }
  int modified = 0;
  if(this->GetUpdateResolution(info) != resolution)
    {
    info->Set(UPDATE_RESOLUTION(), resolution);
    modified = 1;
    }
  return modified;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetUpdateTimeStep(int port, double time)
{
  return this->SetUpdateTimeStep(this->GetOutputInformation(port), time);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetUpdateTimeStep(vtkInformation *info, double time)
{
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
  info->Set(UPDATE_EXTENT_INITIALIZED(), 1);
  return modified;
}

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

//----------------------------------------------------------------------------
double vtkStreamingDemandDrivenPipeline
::GetUpdateResolution(vtkInformation *info)
{
  if(!info)
    {
    vtkErrorMacro("GetUpdateResolution on invalid output");
    return 0;
    }
  if(!info->Has(UPDATE_RESOLUTION()))
    {
    info->Set(UPDATE_RESOLUTION(), 1.0);
    }
  return info->Get(UPDATE_RESOLUTION());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateNumberOfPieces(vtkInformation *info, int n)
{
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
  info->Set(UPDATE_EXTENT_INITIALIZED(), 1);
  return modified;
}

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

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline
::SetUpdateGhostLevel(vtkInformation *info, int n)
{
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

//----------------------------------------------------------------------------
int
vtkStreamingDemandDrivenPipeline
::SetExtentTranslator(int port, vtkExtentTranslator* translator)
{
  return this->SetExtentTranslator(
    this->GetOutputInformation(port), translator);
}

//----------------------------------------------------------------------------
int
vtkStreamingDemandDrivenPipeline
::SetExtentTranslator(vtkInformation *info, vtkExtentTranslator* translator)
{
  if(!info)
    {
    vtkGenericWarningMacro("Attempt to set translator for invalid output");
    return 0;
    }
  vtkExtentTranslator* oldTranslator =
    vtkExtentTranslator::SafeDownCast(info->Get(EXTENT_TRANSLATOR()));
  if(translator != oldTranslator)
    {
    info->Set(EXTENT_TRANSLATOR(), translator);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkExtentTranslator*
vtkStreamingDemandDrivenPipeline::GetExtentTranslator(int port)
{
  return this->GetExtentTranslator(this->GetOutputInformation(port));
}

//----------------------------------------------------------------------------
vtkExtentTranslator*
vtkStreamingDemandDrivenPipeline::GetExtentTranslator(vtkInformation *info)
{
  if(!info)
    {
    vtkGenericWarningMacro("Attempt to get translator for invalid output");
    return 0;
    }
  vtkExtentTranslator* translator =
    vtkExtentTranslator::SafeDownCast(info->Get(EXTENT_TRANSLATOR()));
  if(!translator)
    {
    translator = vtkExtentTranslator::New();
    info->Set(EXTENT_TRANSLATOR(), translator);
    translator->Delete();
    }
  return translator;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetWholeBoundingBox(int port,
                                                          double extent[6])
{
  if(!this->OutputPortIndexInRange(port, "set whole bounding box on"))
    {
    return 0;
    }
  vtkInformation* info = this->GetOutputInformation(port);
  int modified = 0;
  double oldBoundingBox[6];
  this->GetWholeBoundingBox(port, oldBoundingBox);
  if(oldBoundingBox[0] != extent[0] || oldBoundingBox[1] != extent[1] ||
     oldBoundingBox[2] != extent[2] || oldBoundingBox[3] != extent[3] ||
     oldBoundingBox[4] != extent[4] || oldBoundingBox[5] != extent[5])
    {
    modified = 1;
    info->Set(WHOLE_BOUNDING_BOX(), extent, 6);
    }
  return modified;
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline::GetWholeBoundingBox(int port, double extent[6])
{
  double *bbox = this->GetWholeBoundingBox(port);
  memcpy(extent, bbox, 6*sizeof(double));
}

//----------------------------------------------------------------------------
double* vtkStreamingDemandDrivenPipeline::GetWholeBoundingBox(int port)
{
  static double emptyBoundingBox[6] = {0,-1,0,-1,0,-1};
  if(!this->OutputPortIndexInRange(port, "get whole bounding box from"))
    {
    return emptyBoundingBox;
    }
  vtkInformation* info = this->GetOutputInformation(port);
  if(!info->Has(WHOLE_BOUNDING_BOX()))
    {
    info->Set(WHOLE_BOUNDING_BOX(), emptyBoundingBox, 6);
    }
  return info->Get(WHOLE_BOUNDING_BOX());
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::SetPieceBoundingBox(int port,
                                                          double extent[6])
{
  if(!this->OutputPortIndexInRange(port, "set piece bounding box on"))
    {
    return 0;
    }
  vtkInformation* info = this->GetOutputInformation(port);
  int modified = 0;
  double oldBoundingBox[6];
  this->GetPieceBoundingBox(port, oldBoundingBox);
  if(oldBoundingBox[0] != extent[0] || oldBoundingBox[1] != extent[1] ||
     oldBoundingBox[2] != extent[2] || oldBoundingBox[3] != extent[3] ||
     oldBoundingBox[4] != extent[4] || oldBoundingBox[5] != extent[5])
    {
    modified = 1;
    info->Set(PIECE_BOUNDING_BOX(), extent, 6);
    }
  return modified;
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline::GetPieceBoundingBox(int port, double extent[6])
{
  double *bbox = this->GetPieceBoundingBox(port);
  memcpy(extent, bbox, 6*sizeof(double));
}

//----------------------------------------------------------------------------
double* vtkStreamingDemandDrivenPipeline::GetPieceBoundingBox(int port)
{
  static double emptyBoundingBox[6] = {0,-1,0,-1,0,-1};
  if(!this->OutputPortIndexInRange(port, "get piece bounding box from"))
    {
    return emptyBoundingBox;
    }
  vtkInformation* info = this->GetOutputInformation(port);
  if(!info->Has(PIECE_BOUNDING_BOX()))
    {
    info->Set(PIECE_BOUNDING_BOX(), emptyBoundingBox, 6);
    }
  return info->Get(PIECE_BOUNDING_BOX());
}

//----------------------------------------------------------------------------
double vtkStreamingDemandDrivenPipeline::ComputePriority(int port)
{
  vtkInformation* rqst;
  vtkInformationVector **inVec = this->GetInputInformation();
  vtkInformationVector *outVec = this->GetOutputInformation();

  //tell pipeline what piece to ask about
  rqst = vtkInformation::New();
  rqst->Set(REQUEST_RESOLUTION_PROPAGATE());
  rqst->Set(vtkExecutive::FORWARD_DIRECTION(),
            vtkExecutive::RequestUpstream);
  rqst->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  rqst->Set(vtkExecutive::FROM_OUTPUT_PORT(), port);
  this->ProcessRequest(rqst, inVec, outVec);
  rqst->Delete();

  //make sure global information is up to date
  rqst = vtkInformation::New();
  rqst->Set(REQUEST_DATA_OBJECT());
  rqst->Set(REQUEST_REGENERATE_INFORMATION(), 1);
  rqst->Set(vtkExecutive::FORWARD_DIRECTION(),
            vtkExecutive::RequestUpstream);
  rqst->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  rqst->Set(vtkExecutive::FROM_OUTPUT_PORT(), port);
  this->ProcessRequest(rqst, inVec, outVec);
  rqst->Delete();

  rqst = vtkInformation::New();
  rqst->Set(REQUEST_INFORMATION());
  rqst->Set(REQUEST_REGENERATE_INFORMATION(), 1);
  rqst->Set(vtkExecutive::FORWARD_DIRECTION(),
            vtkExecutive::RequestUpstream);
  rqst->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  rqst->Set(vtkExecutive::FROM_OUTPUT_PORT(), port);
  this->ProcessRequest(rqst, inVec, outVec);
  rqst->Delete();

  //tell pipeline what piece to ask about
  rqst = vtkInformation::New();
  rqst->Set(REQUEST_UPDATE_EXTENT());
  vtkInformation *oInfo = outVec->GetInformationObject(0);
  static int emptyExtent[6] = {0,-1,0,-1,0,-1};
  oInfo->Set(UPDATE_EXTENT(), emptyExtent, 6);
  oInfo->Set(UPDATE_EXTENT_INITIALIZED(), 0);
  rqst->Set(vtkExecutive::FORWARD_DIRECTION(),
            vtkExecutive::RequestUpstream);
  rqst->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  rqst->Set(vtkExecutive::FROM_OUTPUT_PORT(), port);
  this->ProcessRequest(rqst, inVec, outVec);
  rqst->Delete();

  //ask upstream filters to estimate priority for the piece
  rqst = vtkInformation::New();
  rqst->Set(REQUEST_UPDATE_EXTENT_INFORMATION());
  rqst->Set(vtkExecutive::FORWARD_DIRECTION(),
            vtkExecutive::RequestUpstream);
  rqst->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  rqst->Set(vtkExecutive::FROM_OUTPUT_PORT(), port);
  this->ProcessRequest(rqst, inVec, outVec);
  rqst->Delete();

  //obtain the priority returned
  double priority = 1.0;
  vtkInformation *info = outVec->GetInformationObject(port);
  if (info && info->Has(PRIORITY()))
    {
    priority = info->Get(PRIORITY());
    }
  return priority;
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline::ResetUpdateInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inInfoVec,
  vtkInformationVector* vtkNotUsed(outInfoVec))
{
  int num_ports = this->GetNumberOfInputPorts();

  for (int cc=0; cc < num_ports; cc++)
    {
    int num_conns = inInfoVec[cc]->GetNumberOfInformationObjects();
    for (int kk=0; kk < num_conns; kk++)
      {
      vtkInformation* inInfo = inInfoVec[cc]->GetInformationObject(kk);
      if (inInfo)
        {
        inInfo->Remove(FAST_PATH_OBJECT_ID());
        inInfo->Remove(FAST_PATH_OBJECT_TYPE());
        inInfo->Remove(FAST_PATH_ID_TYPE());
        inInfo->Remove(UPDATE_RESOLUTION());
        }
      }
    }
}
