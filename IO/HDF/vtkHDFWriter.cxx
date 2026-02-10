// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFWriter.h"

#include "vtkAbstractArray.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFieldData.h"
#include "vtkHDFUtilities.h"
#include "vtkHDFWriterImplementation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringFormatter.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHDFWriter);
vtkCxxSetObjectMacro(vtkHDFWriter, Controller, vtkMultiProcessController);

namespace
{
constexpr hsize_t SINGLE_COLUMN = 1;

// Used for chunked arrays with 4 columns (polydata primitive topologies)
hsize_t PRIMITIVE_CHUNK[] = { 1, vtkHDFUtilities::NUM_POLY_DATA_TOPOS };
hsize_t SMALL_CHUNK[] = { 1, 1 }; // Used for chunked arrays where values are read one by one

/**
 * Return the name of a partitioned dataset in a pdc given its index.
 * If not set, generate a name based on the id.
 */
std::string getBlockName(vtkPartitionedDataSetCollection* pdc, unsigned int datasetId)
{
  std::string name;
  if (pdc->GetMetaData(datasetId) && pdc->GetMetaData(datasetId)->Has(vtkCompositeDataSet::NAME()))
  {
    name = pdc->GetMetaData(datasetId)->Get(vtkCompositeDataSet::NAME());
  }
  if (name.empty())
  {
    name = "Block" + vtk::to_string(datasetId);
  }
  return name;
}

/**
 * Return the filename for an external file containing <blockname>, made from
 * the original <filename>.
 */
std::string GetExternalBlockFileName(const std::string&& filename, const std::string& blockname)
{
  size_t lastDotPos = filename.find_last_of('.');
  if (lastDotPos != std::string::npos)
  {
    // <FileStem>_<BlockName>.<extension>
    const std::string rawName = filename.substr(0, lastDotPos);
    const std::string extension = filename.substr(lastDotPos);
    return rawName + "_" + blockname + extension;
  }
  // <FileName>_<BlockName>.vtkhdf
  return filename + "_" + blockname + ".vtkhdf";
}
}

//------------------------------------------------------------------------------
vtkHDFWriter::vtkHDFWriter()
  : Impl(new Implementation(this))
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller == nullptr)
  {
    // No multi-process controller has been set, use a dummy one.
    // Mark that it has been created by this process so we can destroy it
    // After the filter execution.
    this->UsesDummyController = true;
    this->SetController(vtkDummyController::New());
  }

  this->NbPieces = this->Controller->GetNumberOfProcesses();
  this->CurrentPiece = this->Controller->GetLocalProcessId();
}

//------------------------------------------------------------------------------
vtkHDFWriter::~vtkHDFWriter()
{
  this->SetFileName(nullptr);
  if (this->UsesDummyController)
  {
    this->Controller->Delete();
    this->SetController(nullptr);
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkHDFWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkHDFWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->timeSteps.resize(this->NumberOfTimeSteps);

    if (this->WriteAllTimeSteps)
    {
      this->IsTemporal = true;
    }
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->Controller)
  {
    vtkInformation* info = inputVector[0]->GetInformationObject(0);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->CurrentPiece);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NbPieces);
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->WriteAllTimeSteps && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->timeSteps.data());
    double timeReq = this->timeSteps[this->CurrentTimeIndex];

    inputVector[0]->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFWriter::RequestData(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    return 1;
  }

  bool ret = this->WriteDataAndReturn();

  if (this->IsTemporal)
  {
    if (this->CurrentTimeIndex == 0)
    {
      // Tell the pipeline to start looping in order to write all the timesteps
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

    this->CurrentTimeIndex++;

    if (this->CurrentTimeIndex >= this->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
      this->CurrentTimeIndex = 0;
      this->Impl->CloseFile();
    }
  }
  else
  {
    this->Impl->CloseFile();
  }

  return ret ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkHDFWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkHDFWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Overwrite: " << (this->Overwrite ? "yes" : "no") << "\n";
  os << indent << "WriteAllTimeSteps: " << (this->WriteAllTimeSteps ? "yes" : "no") << "\n";
  os << indent << "ChunkSize: " << this->ChunkSize << "\n";
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDataAndReturn()
{
  this->Impl->SetSubFilesReady(false);

  // Root file group only needs to be opened for the first timestep
  if (this->CurrentTimeIndex == 0)
  {
    // Write all pieces concurrently
    if (this->NbPieces > 1)
    {
      const std::string partitionSuffix = "part" + vtk::to_string(this->CurrentPiece);
      const std::string filePath =
        ::GetExternalBlockFileName(std::string(this->FileName), partitionSuffix);
      this->Impl->CreateFile(this->Overwrite, filePath);
    }
    else
    {
      if (!this->Impl->CreateFile(this->Overwrite, this->FileName))
      {
        vtkErrorMacro(<< "Could not create file : " << this->FileName);
        return false;
      }
    }
  }

  // Wait for the file to be created
  this->Controller->Barrier();

  vtkDataObject* input = vtkDataObject::SafeDownCast(this->GetInput());

  // Write the time step data in an external file
  if (this->NbPieces == 1 && this->IsTemporal && this->UseExternalTimeSteps)
  {
    const std::string timestepSuffix = vtk::to_string(this->CurrentTimeIndex);
    const std::string subFilePath =
      ::GetExternalBlockFileName(std::string(this->FileName), timestepSuffix);
    vtkNew<vtkHDFWriter> writer;
    writer->SetInputData(input);
    writer->SetFileName(subFilePath.c_str());
    writer->SetCompressionLevel(this->CompressionLevel);
    writer->SetChunkSize(this->ChunkSize);
    writer->SetUseExternalComposite(this->UseExternalComposite);
    writer->SetUseExternalPartitions(this->UseExternalPartitions);
    if (!writer->Write())
    {
      vtkErrorMacro(<< "Could not write timestep file " << subFilePath);
      return false;
    }
    if (!this->Impl->OpenSubfile(subFilePath))
    {
      vtkErrorMacro(<< "Could not open subfile" << subFilePath);
      return false;
    }
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps - 1)
    {
      // On the last timestep, the implementation creates virtual datasets referencing all
      // Subfiles. This can only be done once we know the size of all sub-datasets.
      this->Impl->SetSubFilesReady(true);
    }
  }

  bool ret = this->DispatchDataObject(this->Impl->GetRoot(), input);

  this->UpdatePreviousStepMeshMTime(input);

  // Write the metafile for distributed datasets, gathering information from all timesteps
  if (this->NbPieces > 1)
  {
    ret &= this->WriteDistributedMetafile(input);
  }
  return ret;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDistributedMetafile(vtkDataObject* input)
{
  // Only relevant on the last time step
  if (this->IsTemporal && this->CurrentTimeIndex != this->NumberOfTimeSteps - 1)
  {
    return true;
  }

  this->Impl->CloseFile();

  // Make sure all processes have written and closed their associated subfile
  this->Controller->Barrier();

  bool ret = true;
  if (this->CurrentPiece == 0)
  {
    this->Impl->CreateFile(this->Overwrite, this->FileName);
    for (int i = 0; i < this->NbPieces; i++)
    {
      const std::string partitionSuffix = "part" + vtk::to_string(i);
      const std::string subFilePath =
        ::GetExternalBlockFileName(std::string(this->FileName), partitionSuffix);
      if (!this->Impl->OpenSubfile(subFilePath))
      {
        vtkErrorMacro(<< "Could not open subfile" << subFilePath);
      }
    }
    this->Impl->SetSubFilesReady(true);
    this->CurrentTimeIndex = 0; // Reset time so that datasets are initialized properly

    /* This is a special writing pass. The dataset will be processed and go through writing
    all datasets for its type, except that write operations will be different:
    instead of writing the data actually associated to the input data object,
    write commands will instead gather information from all previously written distributed
    pieces, and create virtual datasets referencing them. */
    ret = this->DispatchDataObject(this->Impl->GetRoot(), input);
  }

  // Set the time value back to where it was, to stop executing
  this->CurrentTimeIndex = this->NumberOfTimeSteps - 1;
  return ret;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::DispatchDataObject(hid_t group, vtkDataObject* input, unsigned int partId)
{
  if (!input)
  {
    vtkErrorMacro(<< "A vtkDataObject input is required.");
    return false;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use.");
    return false;
  }

  vtkPolyData* polydata = vtkPolyData::SafeDownCast(input);
  if (polydata)
  {
    if (!this->WriteDatasetToFile(group, polydata, partId))
    {
      vtkErrorMacro(<< "Can't write polydata to file:" << this->FileName);
      return false;
    }
    return true;
  }
  vtkUnstructuredGrid* unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (unstructuredGrid)
  {
    if (!this->WriteDatasetToFile(group, unstructuredGrid, partId))
    {
      vtkErrorMacro(<< "Can't write unstructuredGrid to file:" << this->FileName);
      return false;
    }
    return true;
  }
  vtkPartitionedDataSet* partitioned = vtkPartitionedDataSet::SafeDownCast(input);
  if (partitioned)
  {
    if (!this->WriteDatasetToFile(group, partitioned))
    {
      vtkErrorMacro(<< "Can't write partitionedDataSet to file:" << this->FileName);
      return false;
    }
    return true;
  }
  vtkDataObjectTree* tree = vtkDataObjectTree::SafeDownCast(input);
  if (tree)
  {
    if (!this->WriteDatasetToFile(group, tree))
    {
      vtkErrorMacro(<< "Can't write vtkDataObjectTree to file:" << this->FileName);
      return false;
    }
    return true;
  }

  vtkErrorMacro(<< "Dataset type not supported: " << input->GetClassName());
  return false;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkPolyData* input, unsigned int partId)
{
  if (partId == 0 && this->CurrentTimeIndex == 0 && !this->InitializeChunkedDatasets(group, input))
  {
    vtkErrorMacro(<< "Dataset initialization failed for Polydata " << this->FileName);
    return false;
  }
  if (this->CurrentTimeIndex == 0 && !this->InitializeTemporalPolyData(group))
  {
    vtkErrorMacro(<< "Temporal polydata initialization failed for PolyData " << this->FileName);
    return false;
  }
  if (!this->UpdateStepsGroup(group, input))
  {
    vtkErrorMacro(<< "Failed to update steps group for " << this->FileName);
    return false;
  }

  bool writeSuccess = true;
  if (this->CurrentTimeIndex == 0 && partId == 0)
  {
    writeSuccess &= this->Impl->WriteHeader(group, "PolyData");
  }
  if (this->HasGeometryChangedFromPreviousStep(input) || this->CurrentTimeIndex == 0)
  {
    writeSuccess &= this->AppendNumberOfPoints(group, input);
    writeSuccess &= this->AppendPoints(group, input);
    writeSuccess &= this->AppendPrimitiveCells(group, input);
  }
  writeSuccess &= this->AppendDataArrays(group, input, partId);
  return writeSuccess;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkUnstructuredGrid* input, unsigned int partId)
{
  if (partId == 0 && this->CurrentTimeIndex == 0 && !this->InitializeChunkedDatasets(group, input))
  {
    vtkErrorMacro(<< "Dataset initialization failed for Unstructured grid " << this->FileName);
    return false;
  }

  if ((this->CurrentTimeIndex == 0 || (this->Impl->GetSubFilesReady() && this->NbPieces > 1)) &&
    !this->InitializeTemporalUnstructuredGrid(group))
  {
    vtkErrorMacro(<< "Temporal initialization failed for Unstructured grid " << this->FileName);
    return false;
  }

  vtkCellArray* cells = input->GetCells();

  bool writeSuccess = true;
  if (this->CurrentTimeIndex == 0 && partId == 0)
  {
    writeSuccess &= this->Impl->WriteHeader(group, "UnstructuredGrid");
  }
  if (this->HasGeometryChangedFromPreviousStep(input) || this->CurrentTimeIndex == 0)
  {
    writeSuccess &= this->AppendNumberOfPoints(group, input);
    writeSuccess &= this->AppendNumberOfCells(group, cells);
    writeSuccess &= this->AppendNumberOfConnectivityIds(group, cells);
    writeSuccess &= this->AppendPoints(group, input);
    writeSuccess &= this->AppendCellTypes(group, input);
    writeSuccess &= this->AppendConnectivity(group, cells);
    writeSuccess &= this->AppendOffsets(group, cells);
  }

  writeSuccess &= this->AppendDataArrays(group, input, partId);

  if (!this->UpdateStepsGroup(group, input))
  {
    vtkErrorMacro(<< "Failed to update steps group for timestep " << this->CurrentTimeIndex
                  << " for file " << this->FileName);
    return false;
  }

  return writeSuccess;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkPartitionedDataSet* input)
{
  bool ret = true;
  for (unsigned int partIndex = 0; partIndex < input->GetNumberOfPartitions(); partIndex++)
  {
    // Write individual partitions in different files
    if (this->UseExternalPartitions)
    {
      const std::string partitionSuffix = "part" + vtk::to_string(partIndex);
      const std::string subFilePath =
        ::GetExternalBlockFileName(std::string(this->FileName), partitionSuffix);
      vtkNew<vtkHDFWriter> writer;
      writer->SetInputData(input->GetPartition(partIndex));
      writer->SetFileName(subFilePath.c_str());
      writer->SetCompressionLevel(this->CompressionLevel);
      writer->SetChunkSize(this->ChunkSize);
      writer->SetUseExternalComposite(this->UseExternalComposite);
      writer->SetUseExternalPartitions(this->UseExternalPartitions);
      writer->SetUseExternalTimeSteps(this->UseExternalTimeSteps);
      writer->SetWriteAllTimeSteps(this->WriteAllTimeSteps);
      if (!writer->Write())
      {
        vtkErrorMacro(<< "Could not write partition file " << subFilePath);
        return false;
      }
      if (!this->Impl->OpenSubfile(subFilePath))
      {
        vtkErrorMacro(<< "Could not open subfile" << subFilePath);
      }

      if (partIndex == input->GetNumberOfPartitions() - 1)
      {
        // On the last partition, the implementation creates virtual datasets referencing all
        // Subfiles. This can only be done once we know the size of all sub-datasets.
        this->Impl->SetSubFilesReady(true);
      }
    }

    vtkDataSet* partition = input->GetPartition(partIndex);
    ret &= this->DispatchDataObject(group, partition, partIndex);
  }
  return ret;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkDataObjectTree* input)
{
  bool writeSuccess = true;

  if (this->GetUseExternalPartitions())
  {
    // When writing partitions in individual files,
    // force writing each vtkPartitionedDataset in a different file.
    this->SetUseExternalComposite(true);
  }

  if (this->IsTemporal)
  {
    // Temporal + composite writing can currently only be done in a single file.
    // The current writer implementation makes External<X> difficult when time is involved,
    // because we rely on writers outside of the current pipeline that simply write a data object.
    // Supporting these cases would require to give the writer the ability to add timesteps to an
    // existing file.
    this->SetUseExternalTimeSteps(false);
    this->SetUseExternalComposite(false);
    this->SetUseExternalPartitions(false);
  }

  auto* pdc = vtkPartitionedDataSetCollection::SafeDownCast(input);
  auto* mb = vtkMultiBlockDataSet::SafeDownCast(input);
  if (pdc)
  {
    // Write vtkPartitionedDataSets, at the top level
    writeSuccess &= this->AppendBlocks(group, pdc);

    // For PDC, the assembly is stored in the separate vtkDataAssembly structure
    if (this->CurrentTimeIndex == 0)
    {
      writeSuccess &= this->Impl->WriteHeader(group, "PartitionedDataSetCollection");
      writeSuccess &=
        this->AppendAssembly(this->Impl->CreateHdfGroupWithLinkOrder(group, "Assembly"), pdc);
    }
  }
  else if (mb)
  {
    if (this->CurrentTimeIndex == 0)
    {
      writeSuccess &= this->Impl->WriteHeader(group, "MultiBlockDataSet");
    }

    if (this->CurrentTimeIndex == 0)
    {
      this->Impl->CreateHdfGroupWithLinkOrder(group, "Assembly");
    }
    int leafIndex = 0;
    writeSuccess &=
      this->AppendMultiblock(this->Impl->OpenExistingGroup(group, "Assembly"), mb, leafIndex);
  }
  else
  {
    vtkErrorMacro("Unsupported vtkDataObjectTree subclass. This writer only supports "
                  "vtkPartitionedDataSetCollection and vtkMultiBlockDataSet.");
    return false;
  }

  return writeSuccess;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::UpdateStepsGroup(hid_t group, vtkUnstructuredGrid* input)
{
  if (!this->IsTemporal)
  {
    return true;
  }

  vtkDebugMacro("Update UG Steps group for file " << this->GetFileName());

  hid_t stepsGroup = this->Impl->GetStepsGroup(group);
  bool result = true;

  if (this->CurrentTimeIndex > 0 && !this->HasGeometryChangedFromPreviousStep(input))
  {
    // Subtract current number of points, cells and connectivity ids from last value to nullify the
    // offset
    result &= this->Impl->AddOrCreateSingleRowDataset(
      stepsGroup, "CellOffsets", { -input->GetNumberOfCells() }, true, true);
    result &= this->Impl->AddOrCreateSingleRowDataset(
      stepsGroup, "PointOffsets", { -input->GetNumberOfPoints() }, true, true);
    result &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "ConnectivityIdOffsets",
      { -input->GetCells()->GetNumberOfConnectivityIds() }, true, true);
    result &=
      this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PartOffsets", { -1 }, true, true);
  }

  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "NumberOfParts", { 1 }, false); // !12714: fix for multi-part

  // Don't write offsets for the last timestep
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps - 1)
  {
    return result;
  }

  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "CellOffsets", { input->GetNumberOfCells() }, true);
  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "PointOffsets", { input->GetNumberOfPoints() }, true);
  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "ConnectivityIdOffsets", { input->GetCells()->GetNumberOfConnectivityIds() }, true);
  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "PartOffsets", { 1 }, true); // !12714: fix for multi-part

  return result;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::UpdateStepsGroup(hid_t group, vtkPolyData* input)
{
  if (!this->IsTemporal)
  {
    return true;
  }

  vtkDebugMacro("Update PD Steps group");

  hid_t stepsGroup = this->Impl->GetStepsGroup(group);
  bool result = true;

  // Retrieve current # of connectivity values and cells
  std::vector<vtkIdType> numConnectivityIdsByTopo(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
  auto cellArrayTopos = this->Impl->GetCellArraysForTopos(input);
  for (std::size_t i = 0; i < vtkHDFUtilities::NUM_POLY_DATA_TOPOS; i++)
  {
    numConnectivityIdsByTopo[i] = cellArrayTopos[i].cellArray->GetNumberOfConnectivityIds();
  }

  const std::vector<vtkIdType> numCellsByTopo = { input->GetNumberOfVerts(),
    input->GetNumberOfLines(), input->GetNumberOfPolys(), input->GetNumberOfStrips() };

  if (this->CurrentTimeIndex > 0 && !this->HasGeometryChangedFromPreviousStep(input))
  {
    // When dealing with a static mesh, points & cells from current step have not been written,
    // so we subtract current number of points/cells/etc. from last offset value to nullify the
    // offset difference compared to the previous step.
    result &= this->Impl->AddOrCreateSingleRowDataset(
      stepsGroup, "PointOffsets", { -input->GetNumberOfPoints() }, true, true);
    result &=
      this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PartOffsets", { -1 }, true, true);

    std::vector<vtkIdType> negateNumConn(vtkHDFUtilities::NUM_POLY_DATA_TOPOS);
    std::transform(numConnectivityIdsByTopo.begin(), numConnectivityIdsByTopo.end(),
      negateNumConn.begin(), std::negate<>());
    result &= this->Impl->AddOrCreateSingleRowDataset(
      stepsGroup, "ConnectivityIdOffsets", negateNumConn, true, true);

    std::vector<vtkIdType> negateNumCells(vtkHDFUtilities::NUM_POLY_DATA_TOPOS);
    std::transform(
      numCellsByTopo.begin(), numCellsByTopo.end(), negateNumCells.begin(), std::negate<>());

    result &= this->Impl->AddOrCreateSingleRowDataset(
      stepsGroup, "CellOffsets", negateNumCells, true, true);
  }

  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "NumberOfParts", { 1 }, false); // !12714: fix for multi-part

  // Don't write offsets for the last time step
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps - 1)
  {
    return result;
  }

  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "PointOffsets", { input->GetNumberOfPoints() }, true);
  result &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PartOffsets", { 1 }, true);
  result &=
    this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "CellOffsets", numCellsByTopo, true);
  result &= this->Impl->AddOrCreateSingleRowDataset(
    stepsGroup, "ConnectivityIdOffsets", numConnectivityIdsByTopo, true);

  // Special code path when writing meta-file
  if (this->Impl->GetSubFilesReady() && this->NbPieces > 1)
  {
    result &= this->Impl->WriteSumStepsPolyData(stepsGroup, "ConnectivityIdOffsets");
    result &= this->Impl->WriteSumStepsPolyData(stepsGroup, "CellOffsets");
  }

  return result;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializeTemporalUnstructuredGrid(hid_t group)
{
  if (!this->IsTemporal)
  {
    return true;
  }

  vtkDebugMacro("Initialize Temporal UG for file " << this->FileName);

  if (!this->Impl->CreateStepsGroup(group))
  {
    vtkErrorMacro("Could not create steps group");
    return false;
  }
  hid_t stepsGroup = this->Impl->GetStepsGroup(group);
  if (!this->AppendTimeValues(stepsGroup))
  {
    return false;
  }

  if (this->Impl->GetSubFilesReady())
  {
    return true;
  }

  // Create empty offsets arrays, where a value is appended every step
  bool initResult = true;
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "PointOffsets", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "PartOffsets", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "NumberOfParts", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "CellOffsets", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "ConnectivityIdOffsets", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);

  // Add an initial 0 value in the offset arrays
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PointOffsets", { 0 });
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "CellOffsets", { 0 });
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "ConnectivityIdOffsets", { 0 });
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PartOffsets", { 0 });

  if (!initResult)
  {
    vtkErrorMacro(<< "Could not initialize steps offset arrays when creating: " << this->FileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializeTemporalPolyData(hid_t group)
{
  if (!this->IsTemporal)
  {
    return true;
  }
  vtkDebugMacro("Initialize Temporal PD");

  if (!this->Impl->CreateStepsGroup(group))
  {
    vtkErrorMacro("Could not create steps group");
    return false;
  }
  hid_t stepsGroup = this->Impl->GetStepsGroup(group);
  if (!this->AppendTimeValues(stepsGroup))
  {
    return false;
  }

  if (this->Impl->GetSubFilesReady())
  {
    return true;
  }

  // Create empty offsets arrays, where a value is appended every step, and add and initial 0 value.
  bool initResult = true;
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "PointOffsets", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "PartOffsets", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    stepsGroup, "NumberOfParts", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);

  // Initialize datasets for primitive cells and connectivity. Fill with an empty 1*4 vector.
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "CellOffsets", H5T_STD_I64LE,
    vtkHDFUtilities::NUM_POLY_DATA_TOPOS, PRIMITIVE_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "ConnectivityIdOffsets", H5T_STD_I64LE,
    vtkHDFUtilities::NUM_POLY_DATA_TOPOS, PRIMITIVE_CHUNK);

  // Add an initial 0 value in the offset arrays
  std::vector<vtkIdType> emptyTopoArray(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PointOffsets", { 0 });
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "PartOffsets", { 0 });
  initResult &= this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "CellOffsets", emptyTopoArray);
  initResult &=
    this->Impl->AddOrCreateSingleRowDataset(stepsGroup, "ConnectivityIdOffsets", emptyTopoArray);

  if (!initResult)
  {
    vtkErrorMacro(<< "Could not create temporal offset datasets when creating: " << this->FileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializeChunkedDatasets(hid_t group, vtkUnstructuredGrid* input)
{
  if (!this->InitializePointDatasets(group, input->GetPoints()) ||
    !this->InitializePrimitiveDataset(group))
  {
    vtkErrorMacro(<< "Could not initialize datasets when creating: " << this->FileName);
    return false;
  }

  // Cell types array is specific to UG
  hsize_t largeChunkSize[] = { static_cast<hsize_t>(this->ChunkSize), 1 };
  if (!this->Impl->InitDynamicDataset(
        group, "Types", H5T_STD_U8LE, SINGLE_COLUMN, largeChunkSize, this->CompressionLevel))
  {
    vtkErrorMacro(<< "Could not initialize types dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializeChunkedDatasets(hid_t group, vtkPolyData* input)
{
  if (!this->InitializePointDatasets(group, input->GetPoints()))
  {
    vtkErrorMacro(<< "Could not initialize point datasets when creating: " << this->FileName);
    return false;
  }

  // For each primitive type, create a group and datasets/dataspaces
  auto cellArrayTopos = this->Impl->GetCellArraysForTopos(input);
  for (const auto& cellArrayTopo : cellArrayTopos)
  {
    const char* groupName = cellArrayTopo.hdfGroupName;
    vtkHDF::ScopedH5GHandle topoGroup{ H5Gcreate(
      group, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
    if (topoGroup == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Can not create " << groupName
                    << " group during temporal initialization when creating: " << this->FileName);
      return false;
    }

    if (!this->InitializePrimitiveDataset(topoGroup))
    {
      vtkErrorMacro(<< "Could not initialize topology " << groupName
                    << " datasets when creating: " << this->FileName);
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializePointDatasets(hid_t group, vtkPoints* points)
{
  int components = 3;
  hid_t datatype = vtkHDFUtilities::getH5TypeFromVtkType(VTK_DOUBLE);
  if (points)
  {
    vtkAbstractArray* pointArray = points->GetData();
    datatype = vtkHDFUtilities::getH5TypeFromVtkType(pointArray->GetDataType());
    components = pointArray->GetNumberOfComponents();
  }

  // Create resizeable datasets for Points and NumberOfPoints
  std::vector<hsize_t> pointChunkSize{ static_cast<hsize_t>(this->ChunkSize),
    static_cast<hsize_t>(components) };
  bool initResult = true;
  initResult &= this->Impl->InitDynamicDataset(
    group, "Points", datatype, components, pointChunkSize.data(), this->CompressionLevel);
  initResult &= this->Impl->InitDynamicDataset(
    group, "NumberOfPoints", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  return initResult;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializePrimitiveDataset(hid_t group)
{
  hsize_t largeChunkSize[] = { static_cast<hsize_t>(this->ChunkSize), 1 };
  bool initResult = true;
  initResult &=
    this->Impl->InitDynamicDataset(group, "Offsets", H5T_STD_I64LE, SINGLE_COLUMN, largeChunkSize);
  initResult &= this->Impl->InitDynamicDataset(
    group, "NumberOfCells", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  initResult &= this->Impl->InitDynamicDataset(
    group, "Connectivity", H5T_STD_I64LE, SINGLE_COLUMN, largeChunkSize, this->CompressionLevel);
  initResult &= this->Impl->InitDynamicDataset(
    group, "NumberOfConnectivityIds", H5T_STD_I64LE, SINGLE_COLUMN, SMALL_CHUNK);
  return initResult;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfPoints(hid_t group, vtkPointSet* input)
{
  if (!this->Impl->AddOrCreateSingleRowDataset(
        group, "NumberOfPoints", { input->GetNumberOfPoints() }))
  {
    vtkErrorMacro(<< "Cannot create NumberOfPoints dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfCells(hid_t group, vtkCellArray* input)
{
  vtkIdType nbCells = input ? input->GetNumberOfCells() : 0;
  if (!this->Impl->AddOrCreateSingleRowDataset(group, "NumberOfCells", { nbCells }))
  {
    vtkErrorMacro(<< "Cannot create NumberOfCells dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfConnectivityIds(hid_t group, vtkCellArray* input)
{
  vtkIdType nbConn = input ? input->GetNumberOfConnectivityIds() : 0;
  if (!this->Impl->AddOrCreateSingleRowDataset(group, "NumberOfConnectivityIds", { nbConn }))
  {
    vtkErrorMacro(<< "Cannot create NumberOfConnectivityIds dataset when creating: "
                  << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendCellTypes(hid_t group, vtkUnstructuredGrid* input)
{
  if (!this->Impl->AddOrCreateDataset(group, "Types", H5T_STD_U8LE, input->GetCellTypes()))
  {
    vtkErrorMacro(<< "Can not create Types dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendOffsets(hid_t group, vtkCellArray* input)
{
  vtkSmartPointer<vtkDataArray> offsetsArray = nullptr;
  if (input && input->GetOffsetsArray())
  {
    offsetsArray = input->GetOffsetsArray();
  }
  else
  {
    offsetsArray = vtkSmartPointer<vtkIntArray>::New();
    offsetsArray->SetNumberOfValues(0);
  }
  if (!this->Impl->AddOrCreateDataset(group, "Offsets", H5T_STD_I64LE, offsetsArray))
  {
    vtkErrorMacro(<< "Can not create Offsets dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendConnectivity(hid_t group, vtkCellArray* input)
{
  vtkSmartPointer<vtkDataArray> connArray = nullptr;
  if (input && input->GetConnectivityArray())
  {
    connArray = input->GetConnectivityArray();
  }
  else
  {
    connArray = vtkSmartPointer<vtkIntArray>::New();
    connArray->SetNumberOfValues(0);
  }
  if (!this->Impl->AddOrCreateDataset(group, "Connectivity", H5T_STD_I64LE, connArray))
  {
    vtkErrorMacro(<< "Can not create Connectivity dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendPoints(hid_t group, vtkPointSet* input)
{
  vtkSmartPointer<vtkPoints> points = nullptr;
  if (input && input->GetPoints())
  {
    points = input->GetPoints();
  }
  else
  {
    points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(0);
  }
  if (!this->Impl->AddOrCreateDataset(group, "Points", H5T_IEEE_F64LE, points->GetData()))
  {
    vtkErrorMacro(<< "Can not create points dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendPrimitiveCells(hid_t baseGroup, vtkPolyData* input)
{
  // On group per primitive: Polygons, Strips, Vertices, Lines
  auto cellArrayTopos = this->Impl->GetCellArraysForTopos(input);
  for (const auto& cellArrayTopo : cellArrayTopos)
  {
    const char* groupName = cellArrayTopo.hdfGroupName;
    vtkCellArray* cells = cellArrayTopo.cellArray;

    vtkHDF::ScopedH5GHandle group = H5Gopen(baseGroup, groupName, H5P_DEFAULT);
    if (group == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Could not find or create " << groupName
                    << " group when creating: " << this->FileName);
      return false;
    }

    if (!this->AppendNumberOfCells(group, cells))
    {
      vtkErrorMacro(<< "Could not create NumberOfCells dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }

    if (!this->AppendNumberOfConnectivityIds(group, cells))
    {
      vtkErrorMacro(<< "Could not create NumberOfConnectivityIds dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }

    if (this->HasGeometryChangedFromPreviousStep(input) || this->CurrentTimeIndex == 0)
    {
      if (!this->AppendOffsets(group, cells))
      {
        vtkErrorMacro(<< "Could not create Offsets dataset in group " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }
      if (!this->AppendConnectivity(group, cells))
      {
        vtkErrorMacro(<< "Could not create Connectivity dataset in group " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataArrays(hid_t baseGroup, vtkDataObject* input, unsigned int partId)
{
  if (!this->AppendDataSetAttributes(baseGroup, input, partId))
  {
    vtkErrorMacro("Could not append dataset attributes to file");
    return false;
  }
  if (!this->AppendFieldDataArrays(baseGroup, input, partId))
  {
    vtkErrorMacro("Could not append field arrays to file");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataSetAttributes(
  hid_t baseGroup, vtkDataObject* input, unsigned int partId)
{
  constexpr std::array<const char*, 2> groupNames = { "PointData", "CellData" };
  for (int iAttribute = 0; iAttribute < vtkHDFUtilities::GetNumberOfDataArrayTypes(); ++iAttribute)
  {
    vtkDataSetAttributes* attributes = input->GetAttributes(iAttribute);
    if (attributes == nullptr)
    {
      continue;
    }

    int nArrays = attributes->GetNumberOfArrays();
    if (nArrays <= 0)
    {
      continue;
    }

    // Create the group corresponding to point, cell or field data
    const char* groupName = groupNames[iAttribute];
    const std::string offsetsGroupNameStr = std::string(groupName) + "Offsets";
    const char* offsetsGroupName = offsetsGroupNameStr.c_str();

    if (this->CurrentTimeIndex == 0 && partId == 0)
    {
      vtkHDF::ScopedH5GHandle group{ H5Gcreate(
        baseGroup, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
      if (group == H5I_INVALID_HID)
      {
        vtkErrorMacro(<< "Could not create " << groupName
                      << " group when creating: " << this->FileName);
        return false;
      }

      // Create the offsets group in the steps group for temporal data
      if (this->IsTemporal)
      {
        vtkHDF::ScopedH5GHandle offsetsGroup = H5Gcreate(this->Impl->GetStepsGroup(baseGroup),
          offsetsGroupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (offsetsGroup == H5I_INVALID_HID)
        {
          vtkErrorMacro(<< "Could not create " << offsetsGroupName
                        << " group when creating: " << this->FileName);
          return false;
        }
      }
    }

    vtkHDF::ScopedH5GHandle attributeGroup = H5Gopen(baseGroup, groupName, H5P_DEFAULT);

    // Add the arrays data in the group
    for (int iArray = 0; iArray < nArrays; ++iArray)
    {
      vtkAbstractArray* array = attributes->GetAbstractArray(iArray);
      std::string arrayName{ array->GetName() };

      vtkHDFUtilities::MakeObjectNameValid(arrayName);

      hid_t dataType = vtkHDFUtilities::getH5TypeFromVtkType(array->GetDataType());
      if (dataType == H5I_INVALID_HID)
      {
        vtkWarningMacro(<< "Could not find HDF type for VTK type: " << array->GetDataType()
                        << " when creating: " << this->FileName);
        continue;
      }

      // For temporal data, also add the offset in the steps group
      if (this->IsTemporal &&
        !this->AppendDataArrayOffset(baseGroup, array, arrayName, offsetsGroupName))
      {
        return false;
      }

      // Create dynamic resizable dataset
      if (this->CurrentTimeIndex == 0 && partId == 0)
      {
        // Initialize empty dataset
        hsize_t ChunkSizeComponent[] = { static_cast<hsize_t>(this->ChunkSize),
          static_cast<unsigned long>(array->GetNumberOfComponents()) };
        if (!this->Impl->InitDynamicDataset(attributeGroup, arrayName.c_str(), dataType,
              array->GetNumberOfComponents(), ChunkSizeComponent, this->CompressionLevel))
        {
          vtkErrorMacro(<< "Could not initialize offset dataset for: " << arrayName
                        << " when creating: " << this->FileName);
          return false;
        }
      }

      // Add actual array in the dataset
      if (!this->Impl->AddOrCreateDataset(attributeGroup, arrayName.c_str(), dataType, array))
      {
        vtkErrorMacro(<< "Can not create array " << arrayName << " of attribute " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }

      if (this->CurrentTimeIndex == 0 && partId == 0)
      {
        // Write attribute if the array is a special one
        int attrId = attributes->IsArrayAnAttribute(iArray);
        if (attrId >= 0)
        {
          const char* attrName = attributes->GetAttributeTypeAsString(attrId);
          vtkHDF::ScopedH5DHandle dataset =
            this->Impl->OpenDataset(attributeGroup, arrayName.c_str());
          this->Impl->CreateStringAttribute(dataset, "Attribute", attrName);
        }
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendFieldDataArrays(hid_t baseGroup, vtkDataObject* input, unsigned int partId)
{
  vtkFieldData* attributes = input->GetFieldData();
  if (attributes == nullptr)
  {
    return true;
  }

  int nArrays = attributes->GetNumberOfArrays();
  if (nArrays <= 0)
  {
    return true;
  }

  // Create the group corresponding to field data
  std::string groupName = "FieldData";
  const std::string offsetsGroupName = groupName + "Offsets";
  std::string fieldDataSizeName = "FieldDataSizes";

  if (this->CurrentTimeIndex == 0 && partId == 0)
  {
    vtkHDFUtilities::MakeObjectNameValid(groupName);
    vtkHDF::ScopedH5GHandle group{ H5Gcreate(
      baseGroup, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
    if (group == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Could not create " << groupName
                    << " group when creating: " << this->FileName);
      return false;
    }

    // Create the offsets and the sizes group in the steps group for temporal data
    if (this->IsTemporal)
    {
      vtkHDF::ScopedH5GHandle offsetsGroup = H5Gcreate(this->Impl->GetStepsGroup(baseGroup),
        offsetsGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      if (offsetsGroup == H5I_INVALID_HID)
      {
        vtkErrorMacro(<< "Could not create " << offsetsGroupName
                      << " group when creating: " << this->FileName);
        return false;
      }

      vtkHDF::ScopedH5GHandle sizesGroup = H5Gcreate(this->Impl->GetStepsGroup(baseGroup),
        fieldDataSizeName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      if (offsetsGroup == H5I_INVALID_HID)
      {
        vtkErrorMacro(<< "Could not create " << fieldDataSizeName
                      << " group when creating: " << this->FileName);
        return false;
      }
    }
  }

  vtkHDF::ScopedH5GHandle fieldDataGroup = H5Gopen(baseGroup, groupName.c_str(), H5P_DEFAULT);

  // Add the arrays data in the group
  for (int iArray = 0; iArray < nArrays; ++iArray)
  {
    vtkAbstractArray* array = attributes->GetAbstractArray(iArray);
    std::string arrayName = array->GetName();

    hid_t dataType = vtkHDFUtilities::getH5TypeFromVtkType(array->GetDataType());
    if (dataType == H5I_INVALID_HID)
    {
      vtkWarningMacro(<< "Could not find HDF type for VTK type: " << array->GetDataType()
                      << " when creating: " << this->FileName);
      return true;
    }

    // For temporal data, also add the offset in the steps group
    if (this->IsTemporal &&
      !this->AppendDataArrayOffset(baseGroup, array, arrayName, offsetsGroupName))
    {
      vtkErrorMacro(<< "Could not append data array offset for : " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }
    if (this->IsTemporal &&
      !this->AppendDataArraySizeOffset(baseGroup, array, arrayName, fieldDataSizeName))
    {
      vtkErrorMacro(<< "Could not append data array size offset for : " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }

    if (dataType == H5T_C_S1)
    {
      dataType = H5Tcopy(H5T_C_S1);
      if (H5Tset_size(dataType, H5T_VARIABLE) < 0)
      {
        vtkErrorMacro(<< "Could not set the size for : " << arrayName << ".");
        return false;
      }
    }

    // Create dynamic resizable dataset
    if (this->CurrentTimeIndex == 0 && partId == 0)
    {
      // Initialize empty dataset
      hsize_t ChunkSizeComponent[] = { static_cast<hsize_t>(this->ChunkSize),
        static_cast<unsigned long>(array->GetNumberOfComponents()) };
      if (!this->Impl->InitDynamicDataset(fieldDataGroup, arrayName.c_str(), dataType,
            array->GetNumberOfComponents(), ChunkSizeComponent, this->CompressionLevel))
      {
        vtkErrorMacro(<< "Could not initialize offset dataset for: " << arrayName
                      << " when creating: " << this->FileName);
        return false;
      }
    }

    // Add actual array in the dataset
    if (!this->Impl->AddOrCreateDataset(fieldDataGroup, arrayName.c_str(), dataType, array))
    {
      vtkErrorMacro(<< "Can not create array " << arrayName << " of attribute " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendBlocks(hid_t group, vtkPartitionedDataSetCollection* pdc)
{
  bool ret = true;
  for (unsigned int datasetId = 0; datasetId < pdc->GetNumberOfPartitionedDataSets(); datasetId++)
  {
    vtkHDF::ScopedH5GHandle datasetGroup;
    vtkPartitionedDataSet* currentBlock = pdc->GetPartitionedDataSet(datasetId);
    const std::string currentName = ::getBlockName(pdc, datasetId);

    if (this->UseExternalComposite)
    {
      if (!this->AppendExternalBlock(currentBlock, currentName))
      {
        return false;
      }
      datasetGroup = this->Impl->OpenExistingGroup(group, currentName.c_str());
    }
    else
    {
      if (this->CurrentTimeIndex == 0)
      {
        datasetGroup = this->Impl->CreateHdfGroup(group, currentName.c_str());
      }
      else
      {
        datasetGroup = this->Impl->OpenExistingGroup(group, currentName.c_str());
      }
      this->PreviousStepMeshMTime = this->CompositeMeshMTime[datasetId];
      ret &= this->DispatchDataObject(datasetGroup, currentBlock);
      if (auto ds = vtkDataSet::SafeDownCast(currentBlock->GetPartition(0)))
      {
        this->CompositeMeshMTime[datasetId] = ds->GetMeshMTime();
      }
      else
      {
        this->CompositeMeshMTime[datasetId] = this->CurrentTimeIndex + 1;
      }
    }

    if (this->CurrentTimeIndex == 0)
    {
      this->Impl->CreateScalarAttribute(datasetGroup, "Index", datasetId);
    }
  }

  return ret;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendExternalBlock(vtkDataObject* block, const std::string& blockName)
{
  // Write the block data in an external file. Append data if it already exists
  const std::string subfileName =
    ::GetExternalBlockFileName(std::string(this->FileName), blockName);
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(block);
  writer->SetFileName(subfileName.c_str());
  writer->SetCompressionLevel(this->CompressionLevel);
  writer->SetChunkSize(this->ChunkSize);
  writer->SetUseExternalComposite(this->UseExternalComposite);
  writer->SetUseExternalPartitions(this->UseExternalPartitions);
  if (!writer->Write())
  {
    vtkErrorMacro(<< "Could not write block file " << subfileName);
    return false;
  }

  // Create external link, only done once
  if (this->CurrentTimeIndex == 0 &&
    !this->Impl->CreateExternalLink(
      this->Impl->GetRoot(), subfileName.c_str(), "VTKHDF", blockName.c_str()))
  {
    vtkErrorMacro(<< "Could not create external link to file " << subfileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendAssembly(hid_t assemblyGroup, vtkPartitionedDataSetCollection* pdc)
{
  vtkDataAssembly* assembly = pdc->GetDataAssembly();
  if (!assembly)
  {
    vtkErrorMacro(<< "Could not retrieve assembly from composite vtkPartitionedDataSetCollection");
    return false;
  }

  std::vector<int> assemblyIndices = assembly->GetChildNodes(
    assembly->GetRootNode(), true, vtkDataAssembly::TraversalOrder::DepthFirst);

  for (auto& nodeIndex : assemblyIndices)
  {
    std::string nodePath = assembly->GetNodePath(nodeIndex);
    const std::string rootPrefix = "/" + std::string(assembly->GetRootNodeName()) + "/";
    nodePath = nodePath.substr(rootPrefix.length());

    // Keep track of link creation order because children order matters
    vtkHDF::ScopedH5GHandle nodeGroup =
      this->Impl->CreateHdfGroupWithLinkOrder(assemblyGroup, nodePath.c_str());

    // Softlink all datasets associated with this node.
    for (auto& datasetId : assembly->GetDataSetIndices(nodeIndex, false))
    {
      const std::string datasetName = ::getBlockName(pdc, datasetId);
      const std::string linkTarget = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" + datasetName;
      const std::string linkSource =
        vtkHDFUtilities::VTKHDF_ROOT_PATH + "/Assembly/" + nodePath + "/" + datasetName;
      if (!this->Impl->CreateSoftLink(
            this->Impl->GetRoot(), linkSource.c_str(), linkTarget.c_str()))
      {
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendMultiblock(hid_t assemblyGroup, vtkMultiBlockDataSet* mb, int& leafIndex)
{
  // Iterate over the children of the multiblock, recurse if needed.
  vtkSmartPointer<vtkDataObjectTreeIterator> treeIter;
  treeIter.TakeReference(mb->NewTreeIterator());
  treeIter->TraverseSubTreeOff(); // We use recursion on subtrees instead
  treeIter->SkipEmptyNodesOff();
  treeIter->VisitOnlyLeavesOff();

  for (treeIter->InitTraversal(); !treeIter->IsDoneWithTraversal(); treeIter->GoToNextItem())
  {
    leafIndex++;

    // Retrieve name from metadata or create one
    std::string uniqueSubTreeName = "Block_" + vtk::to_string(leafIndex);
    std::string originalSubTreeName;
    if (mb->HasMetaData(treeIter) && mb->GetMetaData(treeIter)->Has(vtkCompositeDataSet::NAME()))
    {
      originalSubTreeName =
        std::string(mb->GetMetaData(treeIter)->Get(vtkCompositeDataSet::NAME()));
    }
    else
    {
      originalSubTreeName = uniqueSubTreeName;
    }

    if (treeIter->GetCurrentDataObject() &&
      treeIter->GetCurrentDataObject()->IsA("vtkMultiBlockDataSet"))
    {
      // Create a subgroup and recurse
      auto subTree = vtkMultiBlockDataSet::SafeDownCast(treeIter->GetCurrentDataObject());
      if (this->CurrentTimeIndex == 0)
      {
        this->Impl->CreateHdfGroupWithLinkOrder(assemblyGroup, originalSubTreeName.c_str());
      }
      this->AppendMultiblock(
        this->Impl->OpenExistingGroup(assemblyGroup, originalSubTreeName.c_str()), subTree,
        leafIndex);
    }
    else
    {
      if (this->UseExternalComposite)
      {
        // Create the block in a separate file and link it externally
        if (!this->AppendExternalBlock(treeIter->GetCurrentDataObject(), uniqueSubTreeName))
        {
          return false;
        }
      }
      else
      {
        // Create a subgroup in root, write the data into it and softlink it to the assembly
        if (this->CurrentTimeIndex == 0)
        {
          vtkHDF::ScopedH5GHandle datasetGroup = this->Impl->CreateHdfGroupWithLinkOrder(
            this->Impl->GetRoot(), uniqueSubTreeName.c_str());
        }
        if (treeIter->GetCurrentDataObject())
        {
          this->AppendIterDataObject(treeIter, leafIndex, uniqueSubTreeName);
        }
        else if (this->Impl->GetSubFilesReady())
        {
          this->AppendCompositeSubfilesDataObject(uniqueSubTreeName);
        }
      }

      // Create a soft-link from the dataset on root group to the hierarchy positions where it
      // belongs
      if (this->CurrentTimeIndex == 0)
      {
        const std::string linkTarget = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" + uniqueSubTreeName;
        const std::string linkSource =
          this->Impl->GetGroupName(assemblyGroup) + "/" + originalSubTreeName;

        if (!this->Impl->CreateSoftLink(
              this->Impl->GetRoot(), linkSource.c_str(), linkTarget.c_str()))
        {
          return false;
        }
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendIterDataObject(
  vtkDataObjectTreeIterator* treeIter, const int& leafIndex, const std::string& uniqueSubTreeName)
{
  this->PreviousStepMeshMTime = this->CompositeMeshMTime[leafIndex];

  if (!this->DispatchDataObject(
        this->Impl->OpenExistingGroup(this->Impl->GetRoot(), uniqueSubTreeName.c_str()),
        treeIter->GetCurrentDataObject()))
  {
    return false;
  }

  auto ds = vtkDataSet::SafeDownCast(treeIter->GetCurrentDataObject());
  auto pds = vtkPartitionedDataSet::SafeDownCast(treeIter->GetCurrentDataObject());
  if (ds)
  {
    this->CompositeMeshMTime[leafIndex] = ds->GetMeshMTime();
  }
  else if (pds && pds->GetNumberOfPartitions() > 0)
  {
    vtkDataSet* part0 = pds->GetPartition(0);
    if (!part0)
    {
      vtkWarningMacro("No partition available when recovering MeshMTime, skipping");
    }
    else
    {
      this->CompositeMeshMTime[leafIndex] =
        vtkDataSet::SafeDownCast(pds->GetPartition(0))->GetMeshMTime();
    }
  }
  else
  {
    this->CompositeMeshMTime[leafIndex] = this->CurrentTimeIndex + 1;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendCompositeSubfilesDataObject(const std::string& uniqueSubTreeName)
{
  // In multi-piece/distributed, it is possible that one piece is null for the rank 0
  // writing the virtual structure. We try to infer the actual type of the current
  // non-composite dataset, create array structures, and write all non-null pieces to the
  // main file.

  const std::string blockPath = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" +
    uniqueSubTreeName; // All blocks are located on root group and have the same name
                       // for all subfiles
  int type = -1;

  vtkHDF::ScopedH5GHandle nonNullPart = this->Impl->GetSubfileNonNullPart(blockPath, type);
  if (nonNullPart == H5I_INVALID_HID)
  {
    return true; // Leaf is null for every subfile
  }

  bool ret = false;
  if (type == VTK_UNSTRUCTURED_GRID)
  {
    // Get all arrays from the non null part
    vtkNew<vtkUnstructuredGrid> ug;
    this->Impl->CreateArraysFromNonNullPart(nonNullPart, ug);
    ret = this->DispatchDataObject(
      this->Impl->OpenExistingGroup(this->Impl->GetRoot(), uniqueSubTreeName.c_str()), ug);
  }
  else if (type == VTK_POLY_DATA)
  {
    vtkNew<vtkPolyData> pd;
    this->Impl->CreateArraysFromNonNullPart(nonNullPart, pd);
    ret = this->DispatchDataObject(
      this->Impl->OpenExistingGroup(this->Impl->GetRoot(), uniqueSubTreeName.c_str()), pd);
  }
  return ret;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendTimeValues(hid_t group)
{
  if (this->Impl->CreateScalarAttribute(group, "NSteps", this->NumberOfTimeSteps) ==
    H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Could not create steps group when creating: " << this->FileName);
    return false;
  }

  vtkNew<vtkDoubleArray> timeStepsArray;
  timeStepsArray->SetArray(this->timeSteps.data(), this->NumberOfTimeSteps, 1);
  return this->Impl->CreateDatasetFromDataArray(group, "Values", H5T_IEEE_F32LE, timeStepsArray) !=
    H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataArrayOffset(hid_t baseGroup, vtkAbstractArray* array,
  const std::string& arrayName, const std::string& offsetsGroupName)
{
  std::string datasetName{ offsetsGroupName + "/" + arrayName };

  if (this->CurrentTimeIndex == 0 || (this->Impl->GetSubFilesReady() && this->NbPieces > 1))
  {
    // Initialize offsets array
    hsize_t ChunkSize1D[] = { static_cast<hsize_t>(this->ChunkSize), 1 };
    if (!this->Impl->InitDynamicDataset(
          this->Impl->GetStepsGroup(baseGroup), datasetName.c_str(), H5T_STD_I64LE, 1, ChunkSize1D))
    {
      vtkErrorMacro(<< "Could not initialize temporal dataset for: " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }

    // Push a 0 value to the offsets array
    if (!this->Impl->AddOrCreateSingleRowDataset(
          this->Impl->GetStepsGroup(baseGroup), datasetName.c_str(), { 0 }, false))
    {
      vtkErrorMacro(<< "Could not push a 0 value in the offsets array: " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }
  }
  else if (this->CurrentTimeIndex < this->NumberOfTimeSteps)
  {
    // Append offset to offset array
    if (!this->Impl->AddOrCreateSingleRowDataset(this->Impl->GetStepsGroup(baseGroup),
          datasetName.c_str(), { array->GetNumberOfTuples() }, true, false))
    {
      vtkErrorMacro(<< "Could not insert a value in the offsets array: " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataArraySizeOffset(hid_t baseGroup, vtkAbstractArray* array,
  const std::string& arrayName, const std::string& offsetsGroupName)
{
  std::string datasetName{ offsetsGroupName + "/" + arrayName };

  if (this->CurrentTimeIndex < 0 || (this->Impl->GetSubFilesReady() && this->NbPieces > 1))
  {
    // silently do nothing as it could mean that there is no temporal data to write
    return true;
  }

  std::vector<vtkIdType> value;
  if (this->CurrentTimeIndex == 0)
  {
    value.resize(2);
    value[0] = array->GetNumberOfComponents();
    value[1] = array->GetNumberOfTuples();

    // FieldData size always represented by a pair of value per timestep
    hsize_t ChunkSize1D[] = { 1, 2 };
    if (!this->Impl->InitDynamicDataset(this->Impl->GetStepsGroup(baseGroup), datasetName.c_str(),
          H5T_STD_I64LE, value.size(), ChunkSize1D))
    {
      vtkErrorMacro(<< "Could not initialize temporal dataset for: " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }

    // Push a 0 value to the offsets array
    if (!this->Impl->AddOrCreateFieldDataSizeValueDataset(this->Impl->GetStepsGroup(baseGroup),
          datasetName.c_str(), value.data(), static_cast<vtkIdType>(value.size())))
    {
      vtkErrorMacro(<< "Could not push a 0 value in the offsets array: " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }
  }
  else if (this->CurrentTimeIndex < this->NumberOfTimeSteps)
  {
    value.resize(2);
    value[0] = array->GetNumberOfComponents();
    value[1] = array->GetNumberOfTuples();

    // Append offset to offset array
    if (!this->Impl->AddOrCreateFieldDataSizeValueDataset(this->Impl->GetStepsGroup(baseGroup),
          datasetName.c_str(), value.data(), static_cast<vtkIdType>(value.size()), false))
    {
      vtkErrorMacro(<< "Could not insert a value in the offsets array: " << arrayName
                    << " when creating: " << this->FileName);
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::HasGeometryChangedFromPreviousStep(vtkDataSet* input)
{
  return this->CurrentTimeIndex != 0 && input->GetMeshMTime() != this->PreviousStepMeshMTime;
}

//------------------------------------------------------------------------------
void vtkHDFWriter::UpdatePreviousStepMeshMTime(vtkDataObject* input)
{
  if (auto dsInput = vtkDataSet::SafeDownCast(input))
  {
    this->PreviousStepMeshMTime = dsInput->GetMeshMTime();
  }
}

VTK_ABI_NAMESPACE_END
