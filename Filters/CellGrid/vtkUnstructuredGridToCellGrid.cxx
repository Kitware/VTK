// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridToCellGrid.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataAssembly.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

#define VTK_DBG_UGRID_TO_CGRID 1

#define VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM 0
#define VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT 1

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals; // for ""_token

vtkStandardNewMacro(vtkUnstructuredGridToCellGrid);
vtkStandardNewMacro(vtkUnstructuredGridToCellGrid::TranscribeQuery);

bool vtkUnstructuredGridToCellGrid::TranscribeQuery::Initialize()
{
  bool ok = this->Superclass::Initialize();
  if (!this->Input)
  {
    return false;
  }
  switch (this->Phase)
  {
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM:
    {
      // Reset all claims (erasing the number of cells, but preserving any
      // preferred cell-type and priority value.
      for (auto& entry : this->CellTypeMap)
      {
        entry.second.NumberOfCells =
          -1; // TODO: If the input dataset has not been modified, do nothing?
      }
      // Populate the CellTypeMap with numbers of cells of each cell type present.
      auto it = vtk::TakeSmartPointer(this->Input->NewCellIterator());
      for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
      {
        int cellType = it->GetCellType();
        ++(this->CellTypeMap[cellType].NumberOfCells);
      }
    }
    break;
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT:
      break;
  }
  return ok;
}

bool vtkUnstructuredGridToCellGrid::TranscribeQuery::Finalize()
{
  switch (this->Phase)
  {
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM:
    {
      // TODO: Identify whether any cell types are unclaimed and warn or fail as configured.

      // Create cell-attribute instances on output to match input point- and cell-data arrays.
      // Also, create a "shape" cell-attribute instance for the geometry.
      vtkNew<vtkCellAttribute> shape;
      // NB: These values are hardwired for now. In the future, we should examine the
      //     claimed cell types and choose something appropriate.
      shape->Initialize("shape", "ℝ³", 3);
      this->Output->SetShapeAttribute(shape);
      this->Coordinates = this->Input->GetPoints()->GetData();
      if (this->Coordinates)
      {
        if (!this->Coordinates->GetName() || !this->Coordinates->GetName()[0])
        {
          this->Coordinates->SetName("points");
        }
        this->Output->GetAttributes("coordinates"_token)->SetVectors(this->Coordinates);
      }
    }
    break;
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT:
      this->Input = nullptr;
      this->Output = nullptr;
      break;
  }
  return true;
}

bool vtkUnstructuredGridToCellGrid::TranscribeQuery::SumOutputCounts()
{
  bool allCellsClaimed = true;
  this->OutputAllocations.clear();
  for (const auto& entry : this->CellTypeMap)
  {
    if (entry.second.NumberOfCells > 0)
    {
      if (entry.second.CellType.IsValid())
      {
        this->OutputAllocations[entry.second.CellType] += entry.second.NumberOfCells;
        // clang-format off
        vtkLogF(TRACE, "Entry %zu += %lld for '%s' (%x)",
          this->OutputAllocations.size(),
          static_cast<long long>(entry.second.NumberOfCells),
          entry.second.CellType.Data().c_str(),
          entry.second.CellType.GetId());
        // clang-format on
      }
      else
      {
        // clang-format off
        vtkLogF(TRACE, "No allocations for %llu cells of type %lu",
          static_cast<unsigned long long>(entry.second.NumberOfCells),
          static_cast<unsigned long>(entry.first));
        // clang-format on
        allCellsClaimed = false;
      }
    }
  }
  vtkLogF(TRACE, "%zu types with allocations", this->OutputAllocations.size());
  return allCellsClaimed;
}

void vtkUnstructuredGridToCellGrid::TranscribeQuery::AddCellAttributes(
  vtkDataSetAttributes* attributes)
{
  int numberOfArrays = attributes->GetNumberOfArrays();
  for (int aa = 0; aa < numberOfArrays; ++aa)
  {
    auto* arrayIn = attributes->GetAbstractArray(aa);
    if (!arrayIn || !arrayIn->GetName())
    {
      vtkWarningMacro("Empty or unnamed array " << aa << ".");
      continue;
    }
    int nc = arrayIn->GetNumberOfComponents();
    vtkNew<vtkCellAttribute> attribOut;
    std::string fieldSpace = vtkCellAttribute::EncodeSpace("ℝ", nc);
    attribOut->Initialize(arrayIn->GetName(), fieldSpace, nc);
    this->Output->AddCellAttribute(attribOut);
  }
}

vtkUnstructuredGridToCellGrid::vtkUnstructuredGridToCellGrid()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

void vtkUnstructuredGridToCellGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkUnstructuredGridToCellGrid::Reset()
{
  // this->Request->CellTypeMap.clear();
  // this->Request->OutputAllocations.clear();
}

void vtkUnstructuredGridToCellGrid::AddPreferredOutputType(
  int inputCellType, vtkStringToken preferredOutputType, int priority)
{
  this->Request->CellTypeMap[inputCellType] =
    TranscribeQuery::Claim{ 0, priority, preferredOutputType };
}

int vtkUnstructuredGridToCellGrid::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port != 0)
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

int vtkUnstructuredGridToCellGrid::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> inputPDC;
  inputPDC = vtkPartitionedDataSetCollection::GetData(inInfo[0]);
  if (!inputPDC)
  {
    auto* input = vtkUnstructuredGrid::GetData(inInfo[0]);
    if (input)
    {
      inputPDC = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
      inputPDC->SetNumberOfPartitionedDataSets(1);
      inputPDC->SetPartition(0, 0, input);
    }
  }
  auto* output = vtkPartitionedDataSetCollection::GetData(ouInfo);
  if (!inputPDC)
  {
    vtkWarningMacro("Empty input or input of wrong type.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  // Copy the input's hierarchical block arrangement if it exists:
  if (inputPDC->GetDataAssembly())
  {
    vtkNew<vtkDataAssembly> dataAssembly;
    dataAssembly->DeepCopy(inputPDC->GetDataAssembly());
    output->SetDataAssembly(dataAssembly);
  }

  // Look for annotations specifying DG cell-attributes.
  // this->AddAnnotatedAttributes(inputPDC);
  // Reset any annotations from the ustructured grid and re-ingest.
  this->Request->Annotations->Reset();
  this->Request->Annotations->FetchAnnotations(
    inputPDC->GetFieldData(), inputPDC->GetDataAssembly());

  // Iterate over partitioned datasets and turn unstructured grids
  // into cell grids.
  //
  // NB: We cannot use vtkPartitionedDataSetCollection::NewIterator()
  // to fetch an iterator because there is no mapping between that
  // iterator's flat index and the flat index of the parent node ID
  // in the vtkDataAssembly.
  // For instance, given a data assembly like this:
  //   + root 1
  //     + node 2 : dataset ids 0, 3
  //     + node 3 : dataset ids 1
  //       + node 4 : dataset ids 2
  //  When a composite iterator points to a dataset held inside
  //  dataset id 1 (itself a partitioned-dataset), there is no
  //  way to discover the flat index of dataset id 1 from the
  //  (non-partitioned) child dataset or the iterator.
  //
  // Instead, we manually iterate over the collection's top-level
  // vector of partitioned-datasets in order to track the dataset ids.
  output->CopyStructure(inputPDC);
  auto numPartitionedDataSets = inputPDC->GetNumberOfPartitionedDataSets();
  for (unsigned int ii = 0; ii < numPartitionedDataSets; ++ii)
  {
    auto* pd = inputPDC->GetPartitionedDataSet(ii);
    this->Request->FlatIndex = ii;
    auto numPartitions = pd->GetNumberOfPartitions();
    for (unsigned int jj = 0; jj < numPartitions; ++jj)
    {
      if (auto* ugrid = vtkUnstructuredGrid::SafeDownCast(pd->GetPartition(jj)))
      {
        vtkNew<vtkCellGrid> cellGrid;
        if (!this->ProcessUnstructuredGrid(ugrid, cellGrid))
        {
          return 0;
        }
        output->GetPartitionedDataSet(ii)->SetPartition(jj, cellGrid);
      }
    }
  }
  this->Request->FlatIndex = static_cast<unsigned int>(-1); // Invalidate until the next run.

  return 1;
}

bool vtkUnstructuredGridToCellGrid::ProcessUnstructuredGrid(
  vtkUnstructuredGrid* input, vtkCellGrid* output)
{
  // Add every type of cell to the output (so the query
  // asks each one which input cells it can claim).
  output->Initialize();
  output->AddAllCellMetadata();

  // Now claim cells:
  this->Request->Input = input;
  this->Request->Output = output;
  this->Request->Phase = VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM;
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Cell-grid failed to claim input cells.");
    return false;
  }
  this->Request->Input = input;
  this->Request->Output = output;
  if (!this->Request->SumOutputCounts())
  {
    // TODO: Warn or error or ignore when unhandled input cells exist? Should be configurable.
    vtkWarningMacro("One or more unhandled input cells exist.");
  }
  this->Request->Phase = VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT;
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Cell-grid failed to transcribe some claimed input cells.");
    return false;
  }
  output->RemoveUnusedCellMetadata();

  // TODO: Will we ever copy schema/content information from the unstructured grid?
  output->SetSchema("dg leaf", 1);
  output->SetContentVersion(1);
  return true;
}

VTK_ABI_NAMESPACE_END
