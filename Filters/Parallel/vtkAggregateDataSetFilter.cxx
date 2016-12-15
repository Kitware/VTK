/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAggregateDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

vtkStandardNewMacro(vtkAggregateDataSetFilter);

//-----------------------------------------------------------------------------
vtkAggregateDataSetFilter::vtkAggregateDataSetFilter()
{
  this->OutputDataType = VTK_POLY_DATA;
  this->NumberOfTargetProcesses = 1;
}

//-----------------------------------------------------------------------------
vtkAggregateDataSetFilter::~vtkAggregateDataSetFilter()
{
}

//-----------------------------------------------------------------------------
void vtkAggregateDataSetFilter::SetOutputDataType(int dt)
{
  if (dt != this->OutputDataType)
  {
    if (dt == VTK_POLY_DATA || dt == VTK_UNSTRUCTURED_GRID)
    {
      this->OutputDataType = dt;
      this->Modified();
    }
    else
    {
      vtkErrorMacro("Invalid output data type. Only VTK_POLY_DATA (0) "
                      << " or VTK_UNSTRUCTURED_GRID (4) supported");
    }
  }
}

//-----------------------------------------------------------------------------
void vtkAggregateDataSetFilter::SetNumberOfTargetProcesses(int tp)
{
  if (tp != this->NumberOfTargetProcesses)
  {
    int numProcs =
      vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();
    if (tp > 0 && tp <= numProcs)
    {
      this->NumberOfTargetProcesses = tp;
      this->Modified();
    }
    else if (tp <= 0 && this->NumberOfTargetProcesses != 1)
    {
      this->NumberOfTargetProcesses = 1;
      this->Modified();
    }
    else if (tp > numProcs && this->NumberOfTargetProcesses != numProcs)
    {
      this->NumberOfTargetProcesses = numProcs;
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
int vtkAggregateDataSetFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAggregateDataSetFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);

  vtkDataSet* outputCopy = 0;
  if (this->OutputDataType == VTK_POLY_DATA)
  {
    if (output && output->IsA("vtkPolyData"))
    {
      return 1;
    }
    outputCopy = vtkPolyData::New();
  }
  else if (this->OutputDataType == VTK_UNSTRUCTURED_GRID)
  {
    if (output && output->IsA("vtkUnstructuredGrid"))
    {
      return 1;
    }
    outputCopy = vtkUnstructuredGrid::New();
  }
  else
  {
    vtkErrorMacro("Invalid output type: " << this->OutputDataType
                  << ". Cannot create output.");
    return 0;
  }

  outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), outputCopy);
  outputCopy->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
// We should avoid marshalling more than once.
int vtkAggregateDataSetFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = 0;
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = vtkDataSet::GetData(inputVector[0], 0);
  }

  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();

  if (controller->GetNumberOfProcesses() == this->NumberOfTargetProcesses)
  {
    if (input)
    {
      output->ShallowCopy(input);
    }
    return 1;
  }

  // create a subcontroller to simplify communication between the processes
  // that are aggregating data
  vtkSmartPointer<vtkMultiProcessController> subController = NULL;
  if (this->NumberOfTargetProcesses == 1)
  {
    subController = controller;
  }
  else
  {
    int localColor = controller->GetLocalProcessId() / this->NumberOfTargetProcesses;
    int localKey = controller->GetLocalProcessId() % this->NumberOfTargetProcesses;
    subController.TakeReference(controller->PartitionController(localColor, localKey));
  }

  int subNumProcs = subController->GetNumberOfProcesses();
  int subRank = subController->GetLocalProcessId();

  std::vector<vtkIdType> pointCount(subNumProcs, 0);
  vtkIdType numPoints = input->GetNumberOfPoints();
  subController->AllGather(&numPoints, &pointCount[0], 1);

  // The first process in the subcontroller to have points is the one that data will
  // be aggregated to. All of the other processes send their data set to that process.
  int receiveProc = 0;
  vtkIdType maxVal = 0;
  for (int i=0;i<subNumProcs;i++)
  {
    if (pointCount[i] > maxVal)
    {
      maxVal = pointCount[i];
      receiveProc = i;
    }
  }

  std::vector<vtkSmartPointer<vtkDataObject> > recvBuffer;
  subController->Gather(input, recvBuffer, receiveProc);
  if (subRank == receiveProc)
  {
    if (recvBuffer.size() == 1)
    {
      output->ShallowCopy(input);
    }
    else if (this->OutputDataType == VTK_POLY_DATA)
    {
      vtkNew<vtkAppendPolyData> appendFilter;
      for (std::vector<vtkSmartPointer<vtkDataObject> >::iterator it=recvBuffer.begin();
           it!=recvBuffer.end();it++)
      {
        appendFilter->AddInputData(vtkPolyData::SafeDownCast(*it));
      }
      appendFilter->Update();
      output->ShallowCopy(appendFilter->GetOutput());
    }
    else if (this->OutputDataType == VTK_UNSTRUCTURED_GRID)
    {
      vtkNew<vtkAppendFilter> appendFilter;
      appendFilter->MergePointsOn();
      for (std::vector<vtkSmartPointer<vtkDataObject> >::iterator it=recvBuffer.begin();
           it!=recvBuffer.end();it++)
      {
        appendFilter->AddInputData(*it);
      }
      appendFilter->Update();
      output->ShallowCopy(appendFilter->GetOutput());
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkAggregateDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTargetProcesses: " << this->NumberOfTargetProcesses << endl;
  os << indent << "OutputDataType: ";
  if (this->OutputDataType == VTK_POLY_DATA)
  {
    os << "VTK_POLY_DATA";
  }
  else if (this->OutputDataType == VTK_UNSTRUCTURED_GRID)
  {
    os << "VTK_UNSTRUCTURED_GRID";
  }
  else
  {
    os << "Unrecognized output type " << this->OutputDataType;
  }
  os << endl;
}
