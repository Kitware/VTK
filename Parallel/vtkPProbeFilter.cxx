/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPProbeFilter.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPProbeFilter);

vtkCxxSetObjectMacro(vtkPProbeFilter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPProbeFilter::vtkPProbeFilter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPProbeFilter::~vtkPProbeFilter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkPProbeFilter::RequestInformation(vtkInformation *request,
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by this filter.
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (strcmp(
      sddp->GetExtentTranslator(outInfo)->GetClassName(),
      "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    sddp->SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPProbeFilter::RequestData(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int procid = 0;
  int numProcs = 1;
  if ( this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  vtkIdType numPoints = this->NumberOfValidPoints;
  if ( procid )
    {
    // Satellite node
    this->Controller->Send(&numPoints, 1, 0, PROBE_COMMUNICATION_TAG);
    if ( numPoints > 0 )
      {
      this->Controller->Send(output, 0, PROBE_COMMUNICATION_TAG);
      }
    output->ReleaseData();
    }
  else if ( numProcs > 1 )
    {
    vtkIdType numRemoteValidPoints = 0;
    vtkDataSet *remoteProbeOutput = output->NewInstance();
    vtkPointData *remotePointData;
    vtkPointData *pointData = output->GetPointData();
    vtkIdType i;
    vtkIdType k;
    vtkIdType pointId;
    for (i = 1; i < numProcs; i++)
      {
      this->Controller->Receive(&numRemoteValidPoints, 1, i, PROBE_COMMUNICATION_TAG);
      if (numRemoteValidPoints > 0)
        {
        this->Controller->Receive(remoteProbeOutput, i, PROBE_COMMUNICATION_TAG);
      
        remotePointData = remoteProbeOutput->GetPointData();

        vtkCharArray* maskArray = vtkCharArray::SafeDownCast(
          remotePointData->GetArray(this->ValidPointMaskArrayName));

        // Iterate over all point data in the output gathered from the remove
        // and copy array values from all the pointIds which have the mask array
        // bit set to 1.
        vtkIdType numRemotePoints = remoteProbeOutput->GetNumberOfPoints();
        for (pointId=0; (pointId < numRemotePoints) && maskArray; pointId++)
          {
          if (maskArray->GetValue(pointId) == 1)
            {
            for (k = 0; k < pointData->GetNumberOfArrays(); k++)
              {
              vtkAbstractArray *oaa = pointData->GetArray(k);
              vtkAbstractArray *raa = remotePointData->GetArray(oaa->GetName());
              if (raa != NULL)
                {
                oaa->SetTuple(pointId, pointId, raa);
                }            
              }
            }
          }
        }
      }
    remoteProbeOutput->Delete();
    }

  return 1;
}

#include "vtkInformationIntegerVectorKey.h"
//----------------------------------------------------------------------------
int vtkPProbeFilter::RequestUpdateExtent(vtkInformation *,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (sddp)
    {
    sddp->SetUpdateExtentToWholeExtent(inInfo);
    }

//inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
//inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
//inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
//            0);
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkPProbeFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }

  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller " << this->Controller << endl;
}
