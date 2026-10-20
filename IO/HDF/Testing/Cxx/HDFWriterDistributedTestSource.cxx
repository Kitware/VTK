// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <numeric>

namespace HDFWriterDistributedTestUtilities
{
namespace
{
class vtkHDFConfigurationSource : public vtkDataObjectAlgorithm
{
public:
  static vtkHDFConfigurationSource* New();
  vtkTypeMacro(vtkHDFConfigurationSource, vtkDataObjectAlgorithm);
  void SetConfiguration(const Configuration& config) { this->Config = config; }

protected:
  void AddAttributes(vtkDataSet* grid, uint32_t rank, uint32_t part, uint32_t step)
  {
    const double identity = 10000.0 * rank + 100.0 * part + step;
    vtkNew<vtkDoubleArray> pointValues;
    pointValues->SetName("PointValues");
    pointValues->SetNumberOfComponents(3);
    for (vtkIdType point = 0; point < grid->GetNumberOfPoints(); ++point)
    {
      const double values[] = { identity, static_cast<double>(point), step + 0.5 * point };
      pointValues->InsertNextTuple(values);
    }
    grid->GetPointData()->AddArray(pointValues);

    vtkNew<vtkDoubleArray> cellValues;
    cellValues->SetName("CellValues");
    for (vtkIdType cell = 0; cell < grid->GetNumberOfCells(); ++cell)
    {
      cellValues->InsertNextValue(identity + 0.25 * cell);
    }
    grid->GetCellData()->AddArray(cellValues);
  }

  vtkHDFConfigurationSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  vtkSmartPointer<vtkUnstructuredGrid> BuildUGPart(uint32_t rank, uint32_t part, uint32_t step)
  {
    vtkNew<vtkUnstructuredGrid> ug;
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;
    if (static_cast<int>(rank) == this->Config.EmptyRank)
    {
      ug->SetPoints(points);
      return ug;
    }
    const uint32_t geometryStep = this->Config.StaticMesh == StaticMeshType::Static ? 0 : step;
    const uint32_t numberOfCells = (part + 1) * (geometryStep + 1);

    if (this->Config.Polyhedrons == PolyhedronsType::Polyhedrons)
    {
      ug->SetPoints(points);
      ug->Allocate(numberOfCells);
      points->SetNumberOfPoints(numberOfCells * 8);
      for (uint32_t cellId = 0; cellId < numberOfCells; ++cellId)
      {
        const vtkIdType startingPt = cellId * 8;
        for (int z = 0; z < 2; ++z)
        {
          const vtkIdType faceStart = startingPt + z * 4;
          points->SetPoint(faceStart, cellId, part, rank + z);
          points->SetPoint(faceStart + 1, cellId + 1, part, rank + z);
          points->SetPoint(faceStart + 2, cellId + 1, part + 1, rank + z);
          points->SetPoint(faceStart + 3, cellId, part + 1, rank + z);
        }
        std::array<vtkIdType, 8> ids;
        std::iota(ids.begin(), ids.end(), startingPt);
        const vtkIdType faces[] = { 4, ids[3], ids[2], ids[1], ids[0], 4, ids[4], ids[5], ids[6],
          ids[7], 4, ids[0], ids[1], ids[5], ids[4], 4, ids[1], ids[2], ids[6], ids[5], 4, ids[2],
          ids[3], ids[7], ids[6], 4, ids[3], ids[0], ids[4], ids[7] };
        ug->InsertNextCell(VTK_POLYHEDRON, ids.size(), ids.data(), 6, faces);
      }
      this->AddAttributes(ug, rank, part, step);
      return ug;
    }

    // Generate a row of quads, larger at every step,
    // Except of course when we want a static mesh.
    for (uint32_t x = 0; x < numberOfCells; ++x)
    {
      points->InsertNextPoint(x, part, rank);
      points->InsertNextPoint(x + 1, part, rank);
      points->InsertNextPoint(x + 1, part + 1, rank);
      points->InsertNextPoint(x, part + 1, rank);

      std::array<vtkIdType, 4> quadConnect;
      std::iota(quadConnect.begin(), quadConnect.end(), 4 * x);
      cells->InsertNextCell(4, quadConnect.data());
    }

    ug->SetPoints(points);
    ug->SetCells(VTK_QUAD, cells);
    this->AddAttributes(ug, rank, part, step);
    return ug;
  }

  vtkSmartPointer<vtkPolyData> BuildPDPart(uint32_t rank, uint32_t part, uint32_t step)
  {
    vtkNew<vtkDataSetSurfaceFilter> surface;
    surface->SetInputData(this->BuildUGPart(rank, part, step));
    surface->Update();
    return surface->GetOutput();
  }

  vtkSmartPointer<vtkPartitionedDataSet> BuildPartitioned(
    uint32_t rank, uint32_t step, DataType type)
  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(this->Config.PartitionCounts[rank]);
    for (uint8_t partId = 0; partId < static_cast<uint8_t>(pds->GetNumberOfPartitions()); partId++)
    {
      if (type == DataType::UnstructuredGrid)
      {
        pds->SetPartition(partId, this->BuildUGPart(rank, partId, step));
      }
      else if (type == DataType::PolyData)
      {

        pds->SetPartition(partId, this->BuildPDPart(rank, partId, step));
      }
    }
    return pds;
  }

  vtkSmartPointer<vtkDataObjectTree> BuildComposite(uint32_t rank, uint32_t step)
  {
    if (this->Config.Composite == CompositeType::Multiblock)
    {
      vtkNew<vtkMultiBlockDataSet> mb;
      mb->SetNumberOfBlocks(static_cast<unsigned int>(this->Config.Types.size()));
      for (unsigned int blockId = 0; blockId < mb->GetNumberOfBlocks(); blockId++)
      {
        if (this->Config.Types[blockId] == DataType::UnstructuredGrid)
        {
          mb->SetBlock(blockId, this->BuildUGPart(rank, blockId, step));
        }
        else if (this->Config.Types[blockId] == DataType::PolyData)
        {
          mb->SetBlock(blockId, this->BuildPDPart(rank, blockId, step));
        }
      }

      return mb;
    }
    else if (this->Config.Composite == CompositeType::PDC)
    {
      vtkNew<vtkPartitionedDataSetCollection> pdc;
      for (unsigned int block = 0; block < this->Config.Types.size(); ++block)
      {
        pdc->SetPartitionedDataSet(
          block, this->BuildPartitioned(rank, step, this->Config.Types[block]));
      }
      return pdc;
    }
    return nullptr;
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    auto* outInfo = outputVector->GetInformationObject(0);
    if (this->Config.Temporal == TemporalType::Temporal)
    {
      std::array<double, NumberOfSteps> times;
      std::iota(times.begin(), times.end(), 0.0);
      const std::array<double, 2> timeRange = { times.front(), times.back() };
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times.data(),
        static_cast<int>(times.size()));
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(),
        static_cast<int>(timeRange.size()));
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    }
    outInfo->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
    return 1;
  }

  int RequestDataObject(vtkInformation*, vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector) override
  {
    vtkInformation* info = outputVector->GetInformationObject(0);

    vtkSmartPointer<vtkDataObject> dataObject;
    if (this->Config.Composite == CompositeType::Multiblock)
    {
      dataObject = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    }
    else if (this->Config.Composite == CompositeType::PDC)
    {
      dataObject = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    }
    else
    {
      if (!this->Config.PartitionCounts.empty())
      {
        dataObject = vtkSmartPointer<vtkPartitionedDataSet>::New();
      }
      else if (this->Config.Types[0] == DataType::UnstructuredGrid)
      {
        dataObject = vtkSmartPointer<vtkUnstructuredGrid>::New();
      }
      else if (this->Config.Types[0] == DataType::PolyData)
      {
        dataObject = vtkSmartPointer<vtkPolyData>::New();
      }
    }

    if (!dataObject)
    {
      vtkLog(ERROR, "Could not find data type to generate");
    }
    info->Set(vtkDataObject::DATA_OBJECT(), dataObject);
    return 1;
  }

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    auto controller = vtkMPIController::GetGlobalController();
    const int rank = controller->GetLocalProcessId();

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = vtkDataObject::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkSmartPointer<vtkDataObject> obj;

    if (!output)
    {
      vtkLog(ERROR, "Could not find output data structure");
      return 0;
    }

    double requestedTime = 0.0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      requestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
    const int timeIndex = static_cast<int>(requestedTime);

    if (this->Config.Composite == CompositeType::NoComposite)
    {
      if (!this->Config.PartitionCounts.empty())
      {
        obj = BuildPartitioned(rank, timeIndex, this->Config.Types[0]);
      }
      else if (this->Config.Types[0] == DataType::UnstructuredGrid)
      {
        obj = BuildUGPart(rank, 0, timeIndex);
      }
      else if (this->Config.Types[0] == DataType::PolyData)
      {
        obj = BuildPDPart(rank, 0, timeIndex);
      }
    }
    else
    {
      obj = BuildComposite(rank, timeIndex);
    }

    if (!obj)
    {
      vtkLog(ERROR, "Expected output to be set");
    }

    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), requestedTime);
    output->ShallowCopy(obj);
    return 1;
  }

private:
  vtkHDFConfigurationSource(const vtkHDFConfigurationSource&) = delete;
  void operator=(const vtkHDFConfigurationSource&) = delete;

  Configuration Config;
};

vtkStandardNewMacro(vtkHDFConfigurationSource);

}

vtkSmartPointer<vtkDataObjectAlgorithm> MakeConfigurationSource(const Configuration& config)
{
  vtkNew<vtkHDFConfigurationSource> source;
  source->SetConfiguration(config);
  return source;
}

}
