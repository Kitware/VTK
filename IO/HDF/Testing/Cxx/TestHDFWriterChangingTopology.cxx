// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file TestHDFWriterChangingTopology.cxx
 * @brief Test for HDF writer's ability to handle time-varying topology
 *
 * This test verifies that the vtkHDFWriter can correctly write and read back
 * unstructured grids & polydata with changing topology across multiple time steps.
 * The test creates a custom source that generates different meshes at different time steps.
 *
 * The test validates:
 * - Successful writing of time-dependent data with varying topology
 * - Correct reading back of the written HDF file
 * - Proper handling of different numbers of points and cells at each time step
 * - Accurate time step information preservation
 *
 * @class vtkChangingTopologyUGSource
 * A custom VTK algorithm that generates unstructured grids with time-dependent
 * topology. Inherits from vtkUnstructuredGridAlgorithm and produces different
 * tetrahedral meshes based on the requested time step.
 *
 * @function TestChangingTopologyUG
 * Core test function that creates a time-varying topology source, writes it
 * to an HDF file using vtkHDFWriter, then reads it back with vtkHDFReader
 * to verify data integrity and correct handling of changing topology.
 *
 * @function TestHDFWriterChangingTopology
 * Main test entry point that sets up the temporary directory and executes
 * the changing topology test.
 */

#include "vtkAlgorithm.h"
#include "vtkCellArray.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include <string>

namespace
{
class vtkChangingTopologyUGSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkChangingTopologyUGSource* New();
  vtkTypeMacro(vtkChangingTopologyUGSource, vtkUnstructuredGridAlgorithm);

protected:
  vtkChangingTopologyUGSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double timeSteps[2] = { 0.0, 1.0 };
    double timeRange[2] = { 0.0, 1.0 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps, 2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    return 1;
  }

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkUnstructuredGrid* output =
      vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!output)
    {
      return 0;
    }

    double requestedTime = 0.0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      requestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
    const int timeIndex = (requestedTime >= 1.0) ? 1 : 0;

    vtkNew<vtkPoints> points;
    points->InsertNextPoint(0.0, 0.0, 0.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    points->InsertNextPoint(0.0, 0.0, 1.0);
    if (timeIndex == 1)
    {
      points->InsertNextPoint(1.0, 1.0, 1.0);
    }

    vtkNew<vtkCellArray> cells;
    vtkIdType tetra0[4] = { 0, 1, 2, 3 };
    cells->InsertNextCell(4, tetra0);
    if (timeIndex == 1)
    {
      vtkIdType tetra1[4] = { 1, 2, 3, 4 };
      cells->InsertNextCell(4, tetra1);
    }

    output->Initialize();
    output->SetPoints(points);
    output->SetCells(VTK_TETRA, cells);
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), static_cast<double>(timeIndex));

    return 1;
  }

private:
  vtkChangingTopologyUGSource(const vtkChangingTopologyUGSource&) = delete;
  void operator=(const vtkChangingTopologyUGSource&) = delete;
};

class vtkChangingTopologyPDSource : public vtkPolyDataAlgorithm
{
public:
  static vtkChangingTopologyPDSource* New();
  vtkTypeMacro(vtkChangingTopologyPDSource, vtkPolyDataAlgorithm);

protected:
  vtkChangingTopologyPDSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double timeSteps[3] = { 0.0, 1.0, 2.0 };
    double timeRange[3] = { 0.0, 1.0, 2.0 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps, 3);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 3);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    return 1;
  }

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!output)
    {
      return 0;
    }

    double requestedTime = 0.0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      requestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
    const int timeIndex = static_cast<int>(requestedTime);

    vtkNew<vtkPoints> points;
    points->InsertNextPoint(0.0, 0.0, 0.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    if (timeIndex >= 1)
    {
      points->InsertNextPoint(1.0, 1.0, 0.0);
    }
    if (timeIndex == 2)
    {
      points->InsertNextPoint(3.0, 0.0, 0.0);
      points->InsertNextPoint(3.0, 3.0, 0.0);
    }

    vtkNew<vtkCellArray> stripCells;
    vtkNew<vtkCellArray> lineCells;
    if (timeIndex == 0)
    {
      vtkIdType strip[4] = { 0, 1, 2 };
      stripCells->InsertNextCell(3, strip);
    }
    else
    {
      vtkIdType strips[4] = { 0, 1, 2, 3 };
      stripCells->InsertNextCell(4, strips);
    }
    if (timeIndex == 2)
    {
      vtkIdType line[2] = { 4, 5 };
      lineCells->InsertNextCell(2, line);
    }

    output->Initialize();
    output->SetPoints(points);
    output->SetStrips(stripCells);
    output->SetLines(lineCells);
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), static_cast<double>(timeIndex));

    return 1;
  }

private:
  vtkChangingTopologyPDSource(const vtkChangingTopologyPDSource&) = delete;
  void operator=(const vtkChangingTopologyPDSource&) = delete;
};

vtkStandardNewMacro(vtkChangingTopologyUGSource);
vtkStandardNewMacro(vtkChangingTopologyPDSource);
}

//----------------------------------------------------------------------------
bool TestChangingTopologyUG(const std::string& tempDir)
{
  std::string filePath = tempDir + "/HDFWriterChangingGeometryUG.vtkhdf";

  vtkNew<vtkChangingTopologyUGSource> source;
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(source->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->SetWriteAllTimeSteps(true);
  if (!writer->Write())
  {
    vtkLog(ERROR, "Failed to write file: " << filePath);
    return false;
  }

  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(filePath.c_str()))
  {
    vtkLog(ERROR, "vtkHDFReader can not read file: " << filePath);
    return false;
  }
  reader->SetFileName(filePath.c_str());
  reader->Update();

  if (reader->GetNumberOfSteps() != 2)
  {
    vtkLog(ERROR, "Unexpected number of steps: " << reader->GetNumberOfSteps());
    return false;
  }

  reader->SetStep(0);
  reader->Update();
  vtkUnstructuredGrid* poly0 = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());
  if (!poly0)
  {
    vtkLog(ERROR, "Failed to read time step 0");
    return false;
  }
  if (poly0->GetNumberOfCells() != 1 || poly0->GetNumberOfPoints() != 4)
  {
    vtkLog(ERROR, "Unexpected topology at time step 0");
    return false;
  }

  reader->SetStep(1);
  reader->Update();
  vtkUnstructuredGrid* poly1 = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());
  if (!poly1)
  {
    vtkLog(ERROR, "Failed to read time step 1");
    return false;
  }
  if (poly1->GetNumberOfCells() != 2 || poly1->GetNumberOfPoints() != 5)
  {
    vtkLog(ERROR, "Unexpected topology at time step 1");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestChangingTopologyPD(const std::string& tempDir)
{
  std::string filePath = tempDir + "/HDFWriterChangingGeometryPD.vtkhdf";

  vtkNew<vtkChangingTopologyPDSource> source;
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(source->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->SetWriteAllTimeSteps(true);
  if (!writer->Write())
  {
    vtkLog(ERROR, "Failed to write file: " << filePath);
    return false;
  }

  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(filePath.c_str()))
  {
    vtkLog(ERROR, "vtkHDFReader can not read file: " << filePath);
    return false;
  }
  reader->SetFileName(filePath.c_str());
  reader->Update();

  if (reader->GetNumberOfSteps() != 3)
  {
    vtkLog(ERROR, "Unexpected number of steps: " << reader->GetNumberOfSteps());
    return false;
  }

  reader->SetStep(0);
  reader->Update();
  vtkPolyData* grid0 = vtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());
  if (!grid0)
  {
    vtkLog(ERROR, "Failed to read time step 0");
    return false;
  }
  if (grid0->GetNumberOfCells() != 1 || grid0->GetNumberOfPoints() != 3)
  {
    vtkLog(ERROR, "Unexpected topology at time step 0");
    return false;
  }

  reader->SetStep(1);
  reader->Update();
  vtkPolyData* grid1 = vtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());
  if (!grid1)
  {
    vtkLog(ERROR, "Failed to read time step 1");
    return false;
  }
  if (grid1->GetNumberOfCells() != 1 || grid1->GetNumberOfPoints() != 4)
  {
    vtkLog(ERROR, "Unexpected topology at time step 1");
    return false;
  }

  reader->SetStep(2);
  reader->Update();
  vtkPolyData* grid2 = vtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());
  if (!grid2)
  {
    vtkLog(ERROR, "Failed to read time step 1");
    return false;
  }
  if (grid2->GetNumberOfCells() != 2 || grid2->GetNumberOfPoints() != 6)
  {
    vtkLog(ERROR, "Unexpected topology at time step 1");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
int TestHDFWriterChangingTopology(int argc, char* argv[])
{
  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;

  bool testPasses = true;
  testPasses &= TestChangingTopologyUG(tempDir);
  testPasses &= TestChangingTopologyPD(tempDir);
  return testPasses ? EXIT_SUCCESS : EXIT_FAILURE;
}
