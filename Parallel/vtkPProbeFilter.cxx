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

#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkPProbeFilter, "1.10");
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
int vtkPProbeFilter::RequestInformation(vtkInformation *,
                                        vtkInformationVector **,
                                        vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPProbeFilter::RequestData(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  this->vtkProbeFilter::RequestData(request, inputVector, outputVector);
  int procid = 0;
  int numProcs = 1;
  if ( this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  vtkIdType numPoints = this->GetValidPoints()->GetMaxId() + 1;
  if ( procid )
    {
    // Satellite node
    this->Controller->Send(&numPoints, 1, 0, 1970);
    if ( numPoints > 0 )
      {
      this->Controller->Send(this->GetValidPoints(), 0, 1971);
      this->Controller->Send(output, 0, 1972);      
      }
    output->ReleaseData();
    }
  else if ( numProcs > 1 )
    {
    vtkIdType numRemotePoints = 0;
    vtkIdTypeArray *validPoints = vtkIdTypeArray::New();
    vtkDataSet *remoteProbeOutput = output->NewInstance();
    vtkPointData *remotePointData;
    vtkPointData *pointData = output->GetPointData();
    vtkIdType i;
    vtkIdType j;
    vtkIdType k;
    vtkIdType pointId;
    vtkIdType numComponents = pointData->GetNumberOfComponents();
    double *tuple = new double[numComponents];
    for (i = 1; i < numProcs; i++)
      {
      this->Controller->Receive(&numRemotePoints, 1, i, 1970);
      if (numRemotePoints > 0)
        {
        this->Controller->Receive(validPoints, i, 1971);
        this->Controller->Receive(remoteProbeOutput, i, 1972);
      
        remotePointData = remoteProbeOutput->GetPointData();
        for (j = 0; j < numRemotePoints; j++)
          {
          pointId = validPoints->GetValue(j);
        
          remotePointData->GetTuple(pointId, tuple);
        
          for (k = 0; k < numComponents; k++)
            {
            output->GetPointData()->SetComponent(pointId, k, tuple[k]);
            }
          }
        }
      }
    validPoints->Delete();
    remoteProbeOutput->Delete();
    delete [] tuple;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPProbeFilter::RequestUpdateExtent(vtkInformation *,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              0);
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
void vtkPProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller " << this->Controller << endl;
}
