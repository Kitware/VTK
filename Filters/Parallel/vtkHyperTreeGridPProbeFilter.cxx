/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridPProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridPProbeFilter.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
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
vtkStandardNewMacro(vtkHyperTreeGridPProbeFilter);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkHyperTreeGridPProbeFilter, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkHyperTreeGridPProbeFilter::vtkHyperTreeGridPProbeFilter()
  : vtkHyperTreeGridProbeFilter()
  , Controller(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkHyperTreeGridPProbeFilter::~vtkHyperTreeGridPProbeFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridPProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridPProbeFilter::Reduce(
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
    auto dealWithRemote = [](vtkIdList* remotePointIds, vtkDataSet* remoteOutput,
                            vtkHyperTreeGrid* htgSource, vtkDataSet* totOutput) {
      if (remotePointIds->GetNumberOfIds() > 0)
      {
        vtkNew<vtkIdList> iotaIds;
        iotaIds->SetNumberOfIds(remotePointIds->GetNumberOfIds());
        std::iota(iotaIds->begin(), iotaIds->end(), 0);
        unsigned int numArrays = htgSource->GetCellData()->GetNumberOfArrays();
        for (unsigned int iA = 0; iA < numArrays; iA++)
        {
          vtkDataArray* remoteArray = remoteOutput->GetPointData()->GetArray(
            htgSource->GetCellData()->GetArray(iA)->GetName());
          vtkDataArray* totArray =
            totOutput->GetPointData()->GetArray(htgSource->GetCellData()->GetArray(iA)->GetName());
          totArray->InsertTuples(remotePointIds, iotaIds, remoteArray);
        }
      }
    };
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
    dealWithRemote(localPointIds, remoteOutput, source, output);
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
          dealWithRemote(remotePointIds, remoteOutput, source, output);
          remoteOutput->Initialize();
        }
      }
    }
  }
  return true;
}
