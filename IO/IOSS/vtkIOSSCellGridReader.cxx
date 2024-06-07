// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIOSSCellGridReader.h"
#include "vtkIOSSCellGridReaderInternal.h"
#include "vtkIOSSFilesScanner.h"
#include "vtkIOSSReaderCommunication.h"
#include "vtkIOSSUtilities.h"

#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellGrid.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkExtractGrid.h"
#include "vtkFiltersCellGrid.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkMultiProcessStreamSerialization.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkQuad.h"
#include "vtkRemoveUnusedPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ionit_Initializer.h)
#include VTK_IOSS(Ioss_Assembly.h)
#include VTK_IOSS(Ioss_DatabaseIO.h)
#include VTK_IOSS(Ioss_EdgeBlock.h)
#include VTK_IOSS(Ioss_EdgeSet.h)
#include VTK_IOSS(Ioss_ElementBlock.h)
#include VTK_IOSS(Ioss_ElementSet.h)
#include VTK_IOSS(Ioss_ElementTopology.h)
#include VTK_IOSS(Ioss_FaceBlock.h)
#include VTK_IOSS(Ioss_FaceSet.h)
#include VTK_IOSS(Ioss_IOFactory.h)
#include VTK_IOSS(Ioss_NodeBlock.h)
#include VTK_IOSS(Ioss_NodeSet.h)
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_SideBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_StructuredBlock.h)
// clang-format on

#include <array>
#include <cassert>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkIOSSCellGridReader);

vtkIOSSCellGridReader::vtkIOSSCellGridReader()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();

  delete this->Internals;
  this->Internals = new vtkIOSSCellGridReaderInternal(this);
}

vtkIOSSCellGridReader::~vtkIOSSCellGridReader() = default;

int vtkIOSSCellGridReader::ReadMetaData(vtkInformation* metadata)
{
  vtkLogScopeF(TRACE, "ReadMetaData");
  vtkIOSSUtilities::CaptureNonErrorMessages captureMessagesRAII;

  auto& internals = *dynamic_cast<vtkIOSSCellGridReaderInternal*>(this->Internals);
  if (!internals.UpdateDatabaseNames(this))
  {
    return 0;
  }

  // read time information and generate that.
  if (!internals.UpdateTimeInformation(this))
  {
    return 0;
  }
  else
  {
    // add timesteps to metadata
    const auto& timesteps = internals.GetTimeSteps();
    if (!timesteps.empty())
    {
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timesteps.data(),
        static_cast<int>(timesteps.size()));
      double time_range[2] = { timesteps.front(), timesteps.back() };
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), time_range, 2);
    }
    else
    {
      metadata->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      metadata->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }
  }

  // read field/entity selection meta-data. i.e. update vtkDataArraySelection
  // instances for all available entity-blocks, entity-sets, and their
  // corresponding data arrays.
  if (!internals.UpdateEntityAndFieldSelections(this))
  {
    return 0;
  }

  // read assembly information.
  if (!internals.UpdateAssembly(this, &this->AssemblyTag))
  {
    return 0;
  }

  metadata->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  if (internals.HaveRestartFiles())
  {
    // All meta-data have been read successfully, so we can release all the regions.
    // Subsequent ReadMesh calls create only the requested regions (if needed) and release previous
    // regions (if no longer needed).
    internals.ReleaseRegions();
  }
  return 1;
}

int vtkIOSSCellGridReader::ReadMesh(
  int piece, int npieces, int vtkNotUsed(nghosts), int timestep, vtkDataObject* output)
{
  auto& internals = *dynamic_cast<vtkIOSSCellGridReaderInternal*>(this->Internals);
  vtkIOSSUtilities::CaptureNonErrorMessages captureMessagesRAII;

  if (!internals.UpdateDatabaseNames(this))
  {
    // This should not be necessary. ReadMetaData returns false when
    // `UpdateDatabaseNames` fails. At which point vtkReaderAlgorithm should
    // never call `RequestData` leading to a call to this method. However, it
    // does, for some reason. Hence adding this check here.
    // ref: paraview/paraview#19951.
    return 0;
  }

  // This is the first method that gets called when generating data.
  // Reset internal cache counters so we can flush fields not accessed.
  internals.ResetCacheAccessCounts();

  auto collection = vtkPartitionedDataSetCollection::SafeDownCast(output);

  // setup output based on the block/set selections (and those available in the
  // database).
  if (!internals.GenerateOutput(collection, this))
  {
    vtkErrorMacro("Failed to generate output.");
    return 0;
  }

  std::set<unsigned int> selectedAssemblyIndices;
  if (!internals.Selectors.empty() && internals.GetAssembly() != nullptr)
  {
    std::vector<std::string> selectors(internals.Selectors.size());
    std::copy(internals.Selectors.begin(), internals.Selectors.end(), selectors.begin());
    auto assembly = internals.GetAssembly();
    auto nodes = assembly->SelectNodes(selectors);
    auto dsindices = assembly->GetDataSetIndices(nodes);
    selectedAssemblyIndices.insert(dsindices.begin(), dsindices.end());
  }

  // dbaseHandles are handles for individual files this instance will to read to
  // satisfy the request. Can be >= 0.
  const auto dbaseHandles = internals.GetDatabaseHandles(piece, npieces, timestep);
  // If we have restart files, and the previously read regions are no longer needed.
  if (internals.HaveRestartFiles() && !internals.HaveCreatedRegions(dbaseHandles))
  {
    // … then we need to release them and, if requested, clear their cached information
    internals.ReleaseRegions();
    if (!this->GetCaching())
    {
      internals.ClearCache();
    }
  }

  // Read global data. Since this should be same on all ranks, we only read on
  // root node and broadcast it to all. This helps us easily handle the case
  // where the number of reading-ranks is more than writing-ranks.
  auto controller = this->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;
  if (rank == 0)
  {
    if (!dbaseHandles.empty())
    {
      // Read global data. Since global data is expected to be identical on all
      // files in a partitioned collection, we can read it from the first
      // dbaseHandle alone.
      if (this->GetReadGlobalFields())
      {
        internals.GetGlobalFields(collection->GetFieldData(), dbaseHandles[0], timestep);
      }

      if (this->GetReadQAAndInformationRecords())
      {
        internals.GetQAAndInformationRecords(collection->GetFieldData(), dbaseHandles[0]);
      }

      // Handle assemblies.
      internals.ReadAssemblies(collection, dbaseHandles[0]);
    }
  }
  // Transmit assembly and QA records to all ranks.
  if (numRanks > 1)
  {
    vtkNew<vtkUnstructuredGrid> temp;
    vtkMultiProcessStream stream;
    if (rank == 0)
    {
      temp->GetFieldData()->ShallowCopy(collection->GetFieldData());
      stream << collection->GetDataAssembly()->SerializeToXML(vtkIndent());
    }
    controller->Broadcast(temp, 0);
    controller->Broadcast(stream, 0);
    if (rank > 0)
    {
      collection->GetFieldData()->ShallowCopy(temp->GetFieldData());

      std::string xml;
      stream >> xml;
      collection->GetDataAssembly()->InitializeFromXML(xml.c_str());
    }
  }
  // All ranks now have assembly and QA records; extract field-glomming information.
  internals.Annotations->FetchAnnotations(
    collection->GetFieldData(), collection->GetDataAssembly());

  for (unsigned int pdsIdx = 0; pdsIdx < collection->GetNumberOfPartitionedDataSets(); ++pdsIdx)
  {
    const std::string blockName(collection->GetMetaData(pdsIdx)->Get(vtkCompositeDataSet::NAME()));
    const auto entity_type = collection->GetMetaData(pdsIdx)->Get(ENTITY_TYPE());
    const auto vtk_entity_type = static_cast<vtkIOSSCellGridReader::EntityType>(entity_type);

    auto selection = this->GetEntitySelection(vtk_entity_type);
    if (!selection->ArrayIsEnabled(blockName.c_str()) &&
      selectedAssemblyIndices.find(pdsIdx) == selectedAssemblyIndices.end())
    {
      // skip disabled blocks.
      continue;
    }

    auto pds = collection->GetPartitionedDataSet(pdsIdx);
    assert(pds != nullptr);
    for (const auto& handle : dbaseHandles)
    {
      try
      {
        auto cellgrids = internals.GetCellGrids(blockName, vtk_entity_type, handle, timestep, this);
        for (auto& cg : cellgrids)
        {
          pds->SetPartition(pds->GetNumberOfPartitions(), cg);
        }
      }
      catch (const std::runtime_error& e)
      {
        vtkLogF(ERROR,
          "Error reading entity block (or set) named '%s' from '%s'; skipping. Details: %s",
          blockName.c_str(), internals.GetRawFileName(handle).c_str(), e.what());
      }
      // Note: Consider using the inner ReleaseHandles (and not the outer) for debugging purposes
      // internals.ReleaseHandles();
    }
  }
  internals.ReleaseHandles();
  internals.ClearCacheUnused();
  return 1;
}

void vtkIOSSCellGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
