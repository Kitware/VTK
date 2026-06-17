// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRContourFilter.h"

#include "vtkAMRInterfaceFilter.h"
#include "vtkCallbackCommand.h"
#include "vtkCartesianGrid.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataNormals.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"

#include <set>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkAMRContourFilter);

//------------------------------------------------------------------------------
vtkAMRContourFilter::vtkAMRContourFilter()
{
  // setup a callback to report progress
  this->InternalProgressObserver->SetCallback(
    &vtkAMRContourFilter::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);
  this->InternalInterface->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressObserver);
  this->InternalContour->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressObserver);

  // Forced by image implementation of contour
  this->InternalContour->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
}

//------------------------------------------------------------------------------
vtkAMRContourFilter::~vtkAMRContourFilter() = default;

//------------------------------------------------------------------------------
void vtkAMRContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->InternalInterface->PrintSelf(os, indent.GetNextIndent());
  this->InternalContour->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::SetValue(int i, double value)
{
  this->InternalContour->SetValue(i, value);
}

//------------------------------------------------------------------------------
double vtkAMRContourFilter::GetValue(int i)
{
  return this->InternalContour->GetValue(i);
}

//------------------------------------------------------------------------------
double* vtkAMRContourFilter::GetValues()
{
  return this->InternalContour->GetValues();
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::GetValues(double* contourValues)
{
  this->InternalContour->GetValues(contourValues);
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::SetNumberOfContours(int number)
{
  this->InternalContour->SetNumberOfContours(number);
}

//------------------------------------------------------------------------------
int vtkAMRContourFilter::GetNumberOfContours()
{
  return this->InternalContour->GetNumberOfContours();
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::GenerateValues(int numContours, double range[2])
{
  this->InternalContour->GenerateValues(numContours, range);
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->InternalContour->GenerateValues(numContours, rangeStart, rangeEnd);
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::SetContourValues(const std::vector<double>& values)
{
  this->InternalContour->SetContourValues(values);
}

//------------------------------------------------------------------------------
std::vector<double> vtkAMRContourFilter::GetContourValues()
{
  return this->InternalContour->GetContourValues();
}

//------------------------------------------------------------------------------
vtkMTimeType vtkAMRContourFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType interfaceTime = this->InternalInterface->GetMTime();
  vtkMTimeType contourTime = this->InternalContour->GetMTime();
  mTime = std::max(mTime, interfaceTime);
  mTime = std::max(mTime, contourTime);
  return mTime;
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::SetComputeNormals(bool val)
{
  this->InternalContour->SetComputeNormals(val);
}

//------------------------------------------------------------------------------
bool vtkAMRContourFilter::GetComputeNormals()
{
  return this->InternalContour->GetComputeNormals();
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::SetComputeScalars(bool val)
{
  this->InternalContour->SetComputeScalars(val);
}

//------------------------------------------------------------------------------
bool vtkAMRContourFilter::GetComputeScalars()
{
  return this->InternalContour->GetComputeScalars();
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::SetGenerateTriangles(bool val)
{
  this->InternalContour->SetGenerateTriangles(val);
}

//------------------------------------------------------------------------------
bool vtkAMRContourFilter::GetGenerateTriangles()
{
  return this->InternalContour->GetGenerateTriangles();
}

//------------------------------------------------------------------------------
int vtkAMRContourFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::InternalProgressCallbackFunction(
  vtkObject* arg, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkAMRContourFilter*>(clientdata)
    ->InternalProgressCallback(static_cast<vtkAlgorithm*>(arg));
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::InternalProgressCallback(vtkAlgorithm* algorithm)
{
  double progress = algorithm->GetProgress();
  this->UpdateProgress(
    this->ProgressFloor + progress * (this->ProgressCeiling - this->ProgressFloor));
}

//------------------------------------------------------------------------------
int vtkAMRContourFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkOverlappingAMR* input = vtkOverlappingAMR::GetData(inputVector[0]);
  if (!input)
  {
    vtkErrorMacro("Input AMR dataset is nullptr!");
    return 0;
  }

  auto output = vtkPartitionedDataSet::GetData(outputVector);
  if (!output)
  {
    vtkErrorMacro("Output partitioned dataset is nullptr!");
    return 0;
  }

  if (input->GetMTime() > this->InterfaceTime.GetMTime())
  {
    this->ProgressCeiling = 0.5;
    this->InternalInterface->SetInputData(input);
    if (!this->InternalInterface->Update())
    {
      vtkErrorMacro("AMRInterface could not be computed, aborting");
      return 0;
    }

    this->InterfaceCache->ShallowCopy(this->InternalInterface->GetOutput());
    this->InterfaceTime.Modified();
  }
  output->SetNumberOfPartitions(this->InterfaceCache->GetNumberOfPartitions());

  // Report progress
  this->ProgressFloor = 0.5;
  this->UpdateProgress(this->ProgressFloor);
  if (this->CheckAbort())
  {
    return 0;
  }

  // Finish contour configuration and compute contour
  this->InternalContour->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));

  auto iter = vtk::TakeSmartPointer(this->InterfaceCache->NewTreeIterator());
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();
  unsigned int idx = 0;
  double progressPart = 0.5 / this->InterfaceCache->GetNumberOfPartitions();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), idx++)
  {
    this->ProgressCeiling = 0.5 + idx * progressPart;
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    assert(ds);
    if (!this->ContourDataSet(this->InternalContour, ds, idx, output))
    {
      vtkErrorMacro("Contour could not be computed, aborting");
      return 0;
    }

    // Report progress
    this->ProgressFloor = this->ProgressCeiling;
    this->UpdateProgress(this->ProgressFloor);
    if (this->CheckAbort())
    {
      return 0;
    }
  }

  this->CleanupOutput(output);
  this->UpdateProgress(1.0);

  return 1;
}

//------------------------------------------------------------------------------
bool vtkAMRContourFilter::ContourDataSet(
  vtkContourFilter* contour, vtkDataSet* ds, unsigned int idx, vtkPartitionedDataSet* output)
{
  // Run the provided contour filter
  contour->SetInputData(ds);
  if (!contour->Update())
  {
    return false;
  }

  // Shallow copy output
  vtkNew<vtkPolyData> localOutput;
  localOutput->ShallowCopy(contour->GetOutput());

  // The underlying contour filter no longer removes ghost cells (the convention
  // is to leave them in place until the very end of the pipeline). The AMR
  // contour, however, must drop them here: in the coarse blocks the interface
  // filter marks the refined/interface regions as ghost cells, and contouring
  // them would produce geometry that overlaps the finer levels.
  localOutput->RemoveGhostCells();

  // RemoveGhostCells() early-returns without dropping the ghost array when a block
  // has no masked cells, so remove it explicitly to keep the output clean. This
  // also works around https://gitlab.kitware.com/vtk/vtk/-/issues/20001
  localOutput->GetCellData()->RemoveArray(localOutput->GetCellData()->GhostArrayName());

  // Add contour to the output
  output->SetPartition(idx, localOutput);
  return true;
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::CleanupOutput(vtkPartitionedDataSet* output)
{
  std::vector<int> nonEmptyIndices;
  for (unsigned int i = 0; i < output->GetNumberOfPartitions(); i++)
  {
    vtkDataSet* ds = output->GetPartition(i);
    if (ds && ds->GetNumberOfPoints() > 0)
    {
      nonEmptyIndices.emplace_back(i);
    }
  }

  for (std::size_t i = 0; i < nonEmptyIndices.size(); i++)
  {
    output->SetPartition(static_cast<unsigned int>(i), output->GetPartition(nonEmptyIndices[i]));
  }
  output->SetNumberOfPartitions(static_cast<unsigned int>(nonEmptyIndices.size()));
}
VTK_ABI_NAMESPACE_END
