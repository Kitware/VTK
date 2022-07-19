/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPHyperTreeGridProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPHyperTreeGridProbeFilter.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <numeric>
#include <vector>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPHyperTreeGridProbeFilter);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPHyperTreeGridProbeFilter, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPHyperTreeGridProbeFilter::vtkPHyperTreeGridProbeFilter()
  : Controller(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPHyperTreeGridProbeFilter::~vtkPHyperTreeGridProbeFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPHyperTreeGridProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->GetController()->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
int vtkPHyperTreeGridProbeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  return 1;
}

//------------------------------------------------------------------------------
bool vtkPHyperTreeGridProbeFilter::Reduce(
  vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds)
{
  int procId = 0;
  int numProcs = 1;
  if (this->Controller)
  {
    procId = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
  }

  vtkIdType numPointsFound = localPointIds->GetNumberOfIds();
  if (procId != 0)
  {
    this->Controller->Send(&numPointsFound, 1, 0, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
    if (numPointsFound > 0)
    {
      this->Controller->Send(output, 0, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
      this->Controller->Send(
        localPointIds->GetPointer(0), numPointsFound, 0, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
    }
    output->ReleaseData();
    localPointIds->Initialize();
  }
  else
  {
    vtkIdType numRemotePoints = 0;
    vtkSmartPointer<vtkDataSet> remoteOutput = vtk::TakeSmartPointer(output->NewInstance());
    vtkNew<vtkIdList> remotePointIds;
    // deal with master process
    remoteOutput->CopyStructure(output);
    unsigned int numArrays = source->GetCellData()->GetNumberOfArrays();
    for (unsigned int iA = 0; iA < numArrays; iA++)
    {
      vtkDataArray* da =
        output->GetPointData()->GetArray(source->GetCellData()->GetArray(iA)->GetName());
      auto localInstance = vtk::TakeSmartPointer(da->NewInstance());
      localInstance->DeepCopy(da);
      remoteOutput->GetPointData()->AddArray(localInstance);
      da->SetNumberOfTuples(output->GetNumberOfPoints());
      this->FillDefaultArray(da);
    }
    this->DealWithRemote(localPointIds, remoteOutput, source, output);
    remoteOutput->Initialize();

    // deal with other processes
    if (numProcs > 1)
    {
      for (int iProc = 1; iProc < numProcs; iProc++)
      {
        this->Controller->Receive(
          &numRemotePoints, 1, iProc, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
        remotePointIds->SetNumberOfIds(numRemotePoints);
        if (numRemotePoints > 0)
        {
          this->Controller->Receive(remoteOutput, iProc, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
          remotePointIds->Initialize();
          remotePointIds->SetNumberOfIds(numRemotePoints);
          this->Controller->Receive(remotePointIds->GetPointer(0), numRemotePoints, iProc,
            HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
          this->DealWithRemote(remotePointIds, remoteOutput, source, output);
          remoteOutput->Initialize();
        }
      }
    }
  }
  return true;
}
