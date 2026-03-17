// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAggregateDataSetFilter.h"

#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkAggregateDataSetFilter);

//------------------------------------------------------------------------------
vtkAggregateDataSetFilter::vtkAggregateDataSetFilter()
{
  this->NumberOfTargetProcesses = 1;
  this->TargetProcess = vtkAggregateDataSetFilter::PROCESS_WITH_MOST_POINTS;
}

//------------------------------------------------------------------------------
vtkAggregateDataSetFilter::~vtkAggregateDataSetFilter() = default;

//------------------------------------------------------------------------------
void vtkAggregateDataSetFilter::SetNumberOfTargetProcesses(int tp)
{
  if (tp != this->NumberOfTargetProcesses)
  {
    const int numberOfProcesses =
      vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();
    if (tp > 0 && tp <= numberOfProcesses)
    {
      this->NumberOfTargetProcesses = tp;
      this->Modified();
    }
    else if (tp <= 0 && this->NumberOfTargetProcesses != 1)
    {
      this->NumberOfTargetProcesses = 1;
      this->Modified();
    }
    else if (tp > numberOfProcesses && this->NumberOfTargetProcesses != numberOfProcesses)
    {
      this->NumberOfTargetProcesses = numberOfProcesses;
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
int vtkAggregateDataSetFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//------------------------------------------------------------------------------
// We should avoid marshalling more than once.
int vtkAggregateDataSetFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = nullptr;
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = vtkDataSet::GetData(inputVector[0], 0);
  }
  if (!input)
  {
    return 1;
  }

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  const int numberOfProcesses = controller->GetNumberOfProcesses();
  if (numberOfProcesses == this->NumberOfTargetProcesses)
  {
    output->ShallowCopy(input);
    return 1;
  }

  if (input->IsA("vtkImageData") || input->IsA("vtkRectilinearGrid") ||
    input->IsA("vtkStructuredGrid"))
  {
    vtkErrorMacro("Must build with the vtkFiltersParallelDIY2 module enabled to "
      << "aggregate topologically regular grids with MPI");

    return 0;
  }

  // create a subcontroller to simplify communication between the processes
  // that are aggregating data
  vtkSmartPointer<vtkMultiProcessController> subController = nullptr;
  if (this->NumberOfTargetProcesses == 1)
  {
    subController = controller;
  }
  else
  {
    // group processes in round robin-fashion
    const int localProcessId = controller->GetLocalProcessId();
    const auto divResult = std::div(numberOfProcesses, this->NumberOfTargetProcesses);
    const int numberOfProcessesPerGroup = divResult.quot;
    int localColor = localProcessId / numberOfProcessesPerGroup;
    if (divResult.rem)
    {
      localColor = static_cast<int>(localProcessId / (1.0 * numberOfProcessesPerGroup));
    }
    subController.TakeReference(controller->PartitionController(localColor, 0));
  }

  int subNumProcs = subController->GetNumberOfProcesses();
  int subRank = subController->GetLocalProcessId();

  std::vector<vtkIdType> pointCount(subNumProcs, 0);
  vtkIdType numPoints = input->GetNumberOfPoints();
  subController->AllGather(&numPoints, pointCount.data(), 1);

  int receiveProc = 0;
  if (this->TargetProcess == vtkAggregateDataSetFilter::PROCESS_WITH_MOST_POINTS)
  {
    // The first process in the subcontroller to have points is the one that data will
    // be aggregated to. All the other processes send their data set to that process.
    vtkIdType maxVal = 0;
    for (int i = 0; i < subNumProcs; i++)
    {
      if (pointCount[i] > maxVal)
      {
        maxVal = pointCount[i];
        receiveProc = i;
      }
    }
  }

  std::vector<vtkSmartPointer<vtkDataObject>> recvBuffer;
#ifdef VTKAGGREGATEDATASETFILTER_USE_GATHER
  subController->Gather(input, recvBuffer, receiveProc);
#else
  // by default, we don't use gather to avoid paraview/paraview#19937.
  if (subRank == receiveProc)
  {
    recvBuffer.emplace_back(input);
    for (int cc = 0; cc < (subNumProcs - 1); ++cc)
    {
      recvBuffer.push_back(vtkSmartPointer<vtkDataObject>::Take(
        subController->ReceiveDataObject(vtkMultiProcessController::ANY_SOURCE, 909911)));
    }
  }
  else
  {
    subController->Send(input, receiveProc, 909911);
  }
#endif

  if (subRank == receiveProc)
  {
    if (recvBuffer.size() == 1)
    {
      output->ShallowCopy(input);
    }
    else if (input->IsA("vtkPolyData"))
    {
      vtkNew<vtkAppendPolyData> appendFilter;
      for (const vtkSmartPointer<vtkDataObject>& dObj : recvBuffer)
      {
        appendFilter->AddInputData(vtkPolyData::SafeDownCast(dObj));
      }
      appendFilter->Update();
      output->ShallowCopy(appendFilter->GetOutput());
    }
    else if (input->IsA("vtkUnstructuredGrid"))
    {
      vtkNew<vtkAppendFilter> appendFilter;
      appendFilter->SetMergePoints(this->MergePoints);
      for (const vtkSmartPointer<vtkDataObject>& dObj : recvBuffer)
      {
        appendFilter->AddInputData(dObj);
      }
      appendFilter->Update();
      output->ShallowCopy(appendFilter->GetOutput());
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkAggregateDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTargetProcesses: " << this->NumberOfTargetProcesses << endl;
}
VTK_ABI_NAMESPACE_END
