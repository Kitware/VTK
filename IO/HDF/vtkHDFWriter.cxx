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
#include "vtkErrorCode.h"
#include "vtkHDFUtilities.h"
#include "vtkHDFWriterImplementation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHDFWriter);

namespace
{
constexpr int NUM_POLY_DATA_TOPOS = 4;
constexpr hsize_t SINGLE_COLUMN = 1;

// Used for chunked arrays with 4 columns (polydata primitive topologies)
hsize_t PRIMITIVE_CHUNK[] = { 1, NUM_POLY_DATA_TOPOS };
hsize_t SMALL_CHUNK[] = { 1, 1 }; // Used for chunked arrays where values are read one by one

/**
 * Return the name of a partitioned dataset in a pdc given its index.
 * If not set, generate a name based on the id.
 */
std::string getBlockName(vtkPartitionedDataSetCollection* pdc, int datasetId)
{
  std::string name;
  if (pdc->GetMetaData(datasetId) && pdc->GetMetaData(datasetId)->Has(vtkCompositeDataSet::NAME()))
  {
    name = pdc->GetMetaData(datasetId)->Get(vtkCompositeDataSet::NAME());
  }
  if (name.empty())
  {
    name = "Block" + std::to_string(datasetId);
  }
  return name;
}

/**
 * Return the filename for an external file containing <blockname>, made from
 * the original <filename>.
 */
std::string getExternalBlockFileName(std::string&& filename, std::string& blockname)
{
  size_t lastDotPos = filename.find_last_of('.');
  std::string subfileName;
  if (lastDotPos != std::string::npos)
  {
    // <FileName_without_extension>_<BlockName>.<extension>
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
}

//------------------------------------------------------------------------------
vtkHDFWriter::~vtkHDFWriter()
{
  this->SetFileName(nullptr);
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
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->WriteAllTimeSteps && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->timeSteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
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

  this->WriteData();

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
    }
  }

  return 1;
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
void vtkHDFWriter::WriteData()
{
  // Root group only needs to be opened for the first timestep
  if (this->CurrentTimeIndex == 0 && !this->Impl->OpenFile(this->Overwrite))
  {
    vtkErrorMacro(<< "Could not open file : " << this->FileName);
    return;
  }

  vtkDataObject* input = vtkDataObject::SafeDownCast(this->GetInput());
  // First time step is considered static mesh
  if (this->CurrentTimeIndex == 0)
  {
    this->UpdatePreviousStepMeshMTime(input);
  }
  this->DispatchDataObject(this->Impl->GetRoot(), input);
  this->UpdatePreviousStepMeshMTime(input);
}

//------------------------------------------------------------------------------
void vtkHDFWriter::DispatchDataObject(hid_t group, vtkDataObject* input, unsigned int partId)
{
  if (!input)
  {
    vtkErrorMacro(<< "A vtkDataObject input is required.");
    return;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use.");
    return;
  }

  vtkPolyData* polydata = vtkPolyData::SafeDownCast(input);
  if (polydata)
  {
    if (!this->WriteDatasetToFile(group, polydata, partId))
    {
      vtkErrorMacro(<< "Can't write polydata to file:" << this->FileName);
      return;
    }
    return;
  }
  vtkUnstructuredGrid* unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (unstructuredGrid)
  {
    if (!this->WriteDatasetToFile(group, unstructuredGrid, partId))
    {
      vtkErrorMacro(<< "Can't write unstructuredGrid to file:" << this->FileName);
      return;
    }
    return;
  }
  vtkPartitionedDataSet* partitioned = vtkPartitionedDataSet::SafeDownCast(input);
  if (partitioned)
  {
    if (!this->WriteDatasetToFile(group, partitioned))
    {
      vtkErrorMacro(<< "Can't write partitionedDataSet to file:" << this->FileName);
      return;
    }
    return;
  }
  vtkDataObjectTree* tree = vtkDataObjectTree::SafeDownCast(input);
  if (tree)
  {
    if (!this->WriteDatasetToFile(group, tree))
    {
      vtkErrorMacro(<< "Can't write vtkDataObjectTree to file:" << this->FileName);
      return;
    }
    return;
  }

  vtkErrorMacro(<< "Dataset type not supported: " << input->GetClassName());
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkPolyData* input, unsigned int partId)
{
  if (partId == 0 && this->CurrentTimeIndex == 0 && !this->InitializeChunkedDatasets(group, input))
  {
    vtkErrorMacro(<< "Dataset initialization failed for Polydata " << this->FileName);
    return false;
  }
  if (this->CurrentTimeIndex == 0 && !this->InitializeTemporalPolyData())
  {
    vtkErrorMacro(<< "Temporal polydata initialization failed for PolyData " << this->FileName);
    return false;
  }
  if (!this->UpdateStepsGroup(input))
  {
    vtkErrorMacro(<< "Failed to update steps group for " << this->FileName);
    return false;
  }

  bool writeSuccess = true;
  if (this->CurrentTimeIndex == 0 && partId == 0)
  {
    writeSuccess &= this->Impl->WriteHeader(group, "PolyData");
  }
  writeSuccess &= this->AppendNumberOfPoints(group, input);
  if (this->HasGeometryChangedFromPreviousStep(input) || this->CurrentTimeIndex == 0)
  {
    writeSuccess &= this->AppendPoints(group, input);
  }
  writeSuccess &= this->AppendPrimitiveCells(group, input);
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
  if (this->CurrentTimeIndex == 0 && !this->InitializeTemporalUnstructuredGrid())
  {
    vtkErrorMacro(<< "Temporal initialization failed for Unstructured grid " << this->FileName);
    return false;
  }
  if (!this->UpdateStepsGroup(input))
  {
    vtkErrorMacro(<< "Failed to update steps group for timestep " << this->CurrentTimeIndex
                  << " for file " << this->FileName);
    return false;
  }

  vtkCellArray* cells = input->GetCells();

  bool writeSuccess = true;
  if (this->CurrentTimeIndex == 0 && partId == 0)
  {
    writeSuccess &= this->Impl->WriteHeader(group, "UnstructuredGrid");
  }
  writeSuccess &= this->AppendNumberOfPoints(group, input);
  writeSuccess &= this->AppendNumberOfCells(group, cells);
  writeSuccess &= this->AppendNumberOfConnectivityIds(group, cells);
  if (this->HasGeometryChangedFromPreviousStep(input) || this->CurrentTimeIndex == 0)
  {
    writeSuccess &= this->AppendPoints(group, input);
    writeSuccess &= this->AppendCellTypes(group, input);
    writeSuccess &= this->AppendConnectivity(group, cells);
    writeSuccess &= this->AppendOffsets(group, cells);
  }
  writeSuccess &= this->AppendDataArrays(group, input, partId);
  return writeSuccess;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkPartitionedDataSet* input)
{
  for (unsigned int partIndex = 0; partIndex < input->GetNumberOfPartitions(); partIndex++)
  {
    vtkDataSet* partition = input->GetPartition(partIndex);
    this->DispatchDataObject(group, partition, partIndex);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(hid_t group, vtkDataObjectTree* input)
{
  bool writeSuccess = true;

  auto* pdc = vtkPartitionedDataSetCollection::SafeDownCast(input);
  auto* mb = vtkMultiBlockDataSet::SafeDownCast(input);
  if (pdc)
  {
    writeSuccess &= this->Impl->WriteHeader(group, "PartitionedDataSetCollection");

    // Write vtkPartitionedDataSets, at the top level
    writeSuccess &= this->AppendBlocks(group, pdc);

    // For PDC, the assembly is stored in the separate vtkDataAssembly structure
    writeSuccess &=
      this->AppendAssembly(this->Impl->CreateHdfGroupWithLinkOrder(group, "Assembly"), pdc);
  }
  else if (mb)
  {
    writeSuccess &= this->Impl->WriteHeader(group, "MultiBlockDataSet");

    // For interoperability with PDC, we need to keep track of
    // the number of datasets (non-subtree) in the structure.
    writeSuccess &=
      this->AppendMultiblock(this->Impl->CreateHdfGroupWithLinkOrder(group, "Assembly"), mb);
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
bool vtkHDFWriter::UpdateStepsGroup(vtkUnstructuredGrid* input)
{
  if (!this->IsTemporal)
  {
    return true;
  }

  hid_t stepsGroup = this->Impl->GetStepsGroup();
  bool result = true;

  vtkIdType pointsOffset = 0;
  vtkIdType connectivitiesIdOffset = 0;

  if (this->HasGeometryChangedFromPreviousStep(input))
  {
    pointsOffset = input->GetNumberOfPoints();
    connectivitiesIdOffset = input->GetCells()->GetNumberOfConnectivityIds();
    result &= this->Impl->AddOrCreateSingleValueDataset(
      stepsGroup, "PointOffsets", pointsOffset, true, true);
    result &= this->Impl->AddOrCreateSingleValueDataset(
      stepsGroup, "ConnectivityIdOffsets", connectivitiesIdOffset, true, true);
  }
  // Don't write offsets for the last timestep
  if (this->CurrentTimeIndex < this->NumberOfTimeSteps - 1)
  {
    result &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PointOffsets", 0, true);
    result &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "CellOffsets", 0, true);
    result &=
      this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "ConnectivityIdOffsets", 0, true);
    result &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PartOffsets", 0, true);
  }

  return result;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::UpdateStepsGroup(vtkPolyData* input)
{
  if (!this->IsTemporal)
  {
    return true;
  }

  hid_t stepsGroup = this->Impl->GetStepsGroup();
  bool result = true;
  if (this->HasGeometryChangedFromPreviousStep(input))
  {
    result &= this->Impl->AddOrCreateSingleValueDataset(
      stepsGroup, "PointOffsets", input->GetNumberOfPoints(), true, true);
  }
  if (this->CurrentTimeIndex < this->NumberOfTimeSteps - 1)
  {
    result &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PointOffsets", 0, true);
    result &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PartOffsets", 0, true);
  }
  if (!result)
  {
    return false;
  }

  // Update connectivity and cell offsets for primitive types
  vtkHDF::ScopedH5DHandle connectivityOffsetsHandle =
    H5Dopen(stepsGroup, "ConnectivityIdOffsets", H5P_DEFAULT);

  // Get the connectivity offsets for the previous timestep
  std::vector<int> allValues;
  allValues.resize(NUM_POLY_DATA_TOPOS * (this->CurrentTimeIndex + 1));
  H5Dread(connectivityOffsetsHandle, H5T_NATIVE_INT, H5Dget_space(connectivityOffsetsHandle),
    H5S_ALL, H5P_DEFAULT, allValues.data());

  // Offset the offset by the previous timestep's offset
  std::vector<int> connectivityOffsetArray{ 0, 0, 0, 0 };
  auto cellArrayTopos = this->Impl->GetCellArraysForTopos(input);

  bool geometryUpdated = this->HasGeometryChangedFromPreviousStep(input);

  for (int i = 0; i < NUM_POLY_DATA_TOPOS; i++)
  {
    connectivityOffsetArray[i] += allValues[this->CurrentTimeIndex * NUM_POLY_DATA_TOPOS + i];
    if (geometryUpdated)
    {
      connectivityOffsetArray[i] += cellArrayTopos[i].cellArray->GetNumberOfConnectivityIds();
    }
  }
  vtkNew<vtkIntArray> connectivityOffsetvtkArray;
  connectivityOffsetvtkArray->SetNumberOfComponents(NUM_POLY_DATA_TOPOS);
  connectivityOffsetvtkArray->SetArray(connectivityOffsetArray.data(), NUM_POLY_DATA_TOPOS, 1);

  // When the geometry changes the previous offset needs to be overriden
  if (geometryUpdated)
  {
    // Need to deep copy the data since the pointer will be taken
    vtkNew<vtkIntArray> connectivityOffsetvtkArrayCopy;
    std::vector<int> connectivityOffsetArrayCopy = connectivityOffsetArray;
    connectivityOffsetvtkArrayCopy->SetNumberOfComponents(NUM_POLY_DATA_TOPOS);
    connectivityOffsetvtkArrayCopy->SetArray(
      connectivityOffsetArrayCopy.data(), NUM_POLY_DATA_TOPOS, 1);

    if (connectivityOffsetsHandle == H5I_INVALID_HID ||
      !this->Impl->AddArrayToDataset(connectivityOffsetsHandle, connectivityOffsetvtkArrayCopy, 1))
    {
      return false;
    }
  }

  // Add offset for next timestep except the last timestep
  if (this->CurrentTimeIndex < this->NumberOfTimeSteps - 1)
  {
    if (connectivityOffsetsHandle == H5I_INVALID_HID ||
      !this->Impl->AddArrayToDataset(connectivityOffsetsHandle, connectivityOffsetvtkArray))
    {
      return false;
    }
  }

  // Don't write offsets for the last timestep
  if (this->CurrentTimeIndex < this->NumberOfTimeSteps - 1)
  {
    // Cells are always numbered starting from 0 for each timestep,
    // so we don't have any offset
    int cellOffsetArray[] = { 0, 0, 0, 0 };
    vtkNew<vtkIntArray> cellOffsetvtkArray;
    cellOffsetvtkArray->SetNumberOfComponents(NUM_POLY_DATA_TOPOS);
    cellOffsetvtkArray->SetArray(cellOffsetArray, NUM_POLY_DATA_TOPOS, 1);
    vtkHDF::ScopedH5DHandle cellOffsetsHandle = H5Dopen(stepsGroup, "CellOffsets", H5P_DEFAULT);
    if ((this->CurrentTimeIndex < this->NumberOfTimeSteps - 1) &&
      (cellOffsetsHandle == H5I_INVALID_HID ||
        !this->Impl->AddArrayToDataset(cellOffsetsHandle, cellOffsetvtkArray)))
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializeTemporalUnstructuredGrid()
{
  if (!this->IsTemporal)
  {
    return true;
  }

  this->Impl->CreateStepsGroup();
  hid_t stepsGroup = this->Impl->GetStepsGroup();
  if (!this->AppendTimeValues(stepsGroup))
  {
    return false;
  }

  // Create empty offsets arrays, where a value is appended every step
  bool initResult = true;
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "PointOffsets", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "CellOffsets", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "ConnectivityIdOffsets", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "PartOffsets", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;

  // Add an initial 0 value in the offset arrays
  initResult &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PointOffsets", 0);
  initResult &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "CellOffsets", 0);
  initResult &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "ConnectivityIdOffsets", 0);
  initResult &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PartOffsets", 0);

  if (!initResult)
  {
    vtkWarningMacro(<< "Could not initialize steps offset arrays when creating: "
                    << this->FileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializeTemporalPolyData()
{
  if (!this->IsTemporal)
  {
    return true;
  }

  this->Impl->CreateStepsGroup();
  hid_t stepsGroup = this->Impl->GetStepsGroup();
  if (!this->AppendTimeValues(stepsGroup))
  {
    return false;
  }

  // Create empty offsets arrays, where a value is appended every step, and add and initial 0 value.
  bool initResult = true;
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "PointOffsets", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(stepsGroup, "PartOffsets", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  initResult &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PointOffsets", 0);
  initResult &= this->Impl->AddOrCreateSingleValueDataset(stepsGroup, "PartOffsets", 0);

  // Initialize datasets for primitive cells and connectivity. Fill with an empty 1*4 vector.
  vtkHDF::ScopedH5DHandle cellOffsetsHandle = this->Impl->InitDynamicDataset(
    stepsGroup, "CellOffsets", H5T_STD_I64LE, NUM_POLY_DATA_TOPOS, PRIMITIVE_CHUNK);
  vtkHDF::ScopedH5DHandle connectivityOffsetsHandle = this->Impl->InitDynamicDataset(
    stepsGroup, "ConnectivityIdOffsets", H5T_STD_I64LE, NUM_POLY_DATA_TOPOS, PRIMITIVE_CHUNK);
  if (!initResult || cellOffsetsHandle == H5I_INVALID_HID ||
    connectivityOffsetsHandle == H5I_INVALID_HID)
  {
    vtkWarningMacro(<< "Could not create transient offset datasets when creating: "
                    << this->FileName);
    return false;
  }

  vtkNew<vtkIntArray> emptyPrimitiveArray;
  emptyPrimitiveArray->SetNumberOfComponents(NUM_POLY_DATA_TOPOS);
  int emptyArray[] = { 0, 0, 0, 0 };
  emptyPrimitiveArray->SetArray(emptyArray, NUM_POLY_DATA_TOPOS, 1);
  initResult &= this->Impl->AddArrayToDataset(cellOffsetsHandle, emptyPrimitiveArray);
  initResult &= this->Impl->AddArrayToDataset(connectivityOffsetsHandle, emptyPrimitiveArray);
  if (!initResult)
  {
    vtkWarningMacro(<< "Could not initialize steps offset arrays when creating: "
                    << this->FileName);
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
  if (this->Impl->InitDynamicDataset(group, "Types", H5T_STD_U8LE, SINGLE_COLUMN, largeChunkSize,
        this->CompressionLevel) == H5I_INVALID_HID)
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
                    << " group during transient initialization when creating: " << this->FileName);
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
  initResult &= this->Impl->InitDynamicDataset(group, "Points", datatype, components,
                  pointChunkSize.data(), this->CompressionLevel) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(group, "NumberOfPoints", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  return initResult;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::InitializePrimitiveDataset(hid_t group)
{
  hsize_t largeChunkSize[] = { static_cast<hsize_t>(this->ChunkSize), 1 };
  bool initResult = true;
  initResult &= this->Impl->InitDynamicDataset(group, "Offsets", H5T_STD_I64LE, SINGLE_COLUMN,
                  largeChunkSize) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(group, "NumberOfCells", H5T_STD_I64LE, SINGLE_COLUMN,
                  SMALL_CHUNK) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(group, "Connectivity", H5T_STD_I64LE, SINGLE_COLUMN,
                  largeChunkSize, this->CompressionLevel) != H5I_INVALID_HID;
  initResult &= this->Impl->InitDynamicDataset(group, "NumberOfConnectivityIds", H5T_STD_I64LE,
                  SINGLE_COLUMN, SMALL_CHUNK) != H5I_INVALID_HID;
  return initResult;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfPoints(hid_t group, vtkPointSet* input)
{
  if (!this->Impl->AddOrCreateSingleValueDataset(
        group, "NumberOfPoints", input->GetNumberOfPoints()))
  {
    vtkErrorMacro(<< "Can not create NumberOfPoints dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfCells(hid_t group, vtkCellArray* input)
{
  if (!this->Impl->AddOrCreateSingleValueDataset(group, "NumberOfCells", input->GetNumberOfCells()))
  {
    vtkErrorMacro(<< "Can not create NumberOfCells dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfConnectivityIds(hid_t group, vtkCellArray* input)
{
  if (!this->Impl->AddOrCreateSingleValueDataset(
        group, "NumberOfConnectivityIds", input->GetNumberOfConnectivityIds()))
  {
    vtkErrorMacro(<< "Can not create NumberOfConnectivityIds dataset when creating: "
                  << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendCellTypes(hid_t group, vtkUnstructuredGrid* input)
{
  if (!this->Impl->AddOrCreateDataset(group, "Types", H5T_STD_U8LE, input->GetCellTypesArray()))
  {
    vtkErrorMacro(<< "Can not create Types dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendOffsets(hid_t group, vtkCellArray* input)
{
  if (!this->Impl->AddOrCreateDataset(group, "Offsets", H5T_STD_I64LE, input->GetOffsetsArray()))
  {
    vtkErrorMacro(<< "Can not create Offsets dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendConnectivity(hid_t group, vtkCellArray* input)
{
  if (!this->Impl->AddOrCreateDataset(
        group, "Connectivity", H5T_STD_I64LE, input->GetConnectivityArray()))
  {
    vtkErrorMacro(<< "Can not create Connectivity dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendPoints(hid_t group, vtkPointSet* input)
{
  if (input->GetPoints() != nullptr && input->GetPoints()->GetData() != nullptr)
  {
    if (!this->Impl->AddOrCreateDataset(
          group, "Points", H5T_IEEE_F32LE, input->GetPoints()->GetData()))
    {
      vtkErrorMacro(<< "Can not create points dataset when creating: " << this->FileName);
      return false;
    }
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
  constexpr std::array<const char*, 3> groupNames = { "PointData", "CellData", "FieldData" };
  for (int iAttribute = 0; iAttribute < vtkHDFUtilities::GetNumberOfAttributeTypes(); ++iAttribute)
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
    std::string offsetsGroupNameStr = std::string(groupName);
    offsetsGroupNameStr += "Offsets";
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

      // Create the offsets group in the steps group for transient data
      if (this->IsTemporal)
      {
        vtkHDF::ScopedH5GHandle offsetsGroup = H5Gcreate(
          this->Impl->GetStepsGroup(), offsetsGroupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (offsetsGroup == H5I_INVALID_HID)
        {
          vtkErrorMacro(<< "Could not create " << offsetsGroupName
                        << " group when creating: " << this->FileName);
          return false;
        }
      }
    }

    vtkHDF::ScopedH5GHandle group = H5Gopen(baseGroup, groupName, H5P_DEFAULT);

    // Add the arrays data in the group
    for (int iArray = 0; iArray < nArrays; ++iArray)
    {
      vtkAbstractArray* array = attributes->GetAbstractArray(iArray);
      const char* arrayName = array->GetName();
      hid_t dataType = vtkHDFUtilities::getH5TypeFromVtkType(array->GetDataType());
      if (dataType == H5I_INVALID_HID)
      {
        vtkWarningMacro(<< "Could not find HDF type for VTK type: " << array->GetDataType()
                        << " when creating: " << this->FileName);
        continue;
      }

      // For transient data, also add the offset in the steps group
      if (this->IsTemporal && !this->AppendDataArrayOffset(array, arrayName, offsetsGroupName))
      {
        return false;
      }

      // Create dynamic resizable dataset
      if (this->CurrentTimeIndex == 0 && partId == 0)
      {
        // Initialize empty dataset
        hsize_t ChunkSizeComponent[] = { static_cast<hsize_t>(this->ChunkSize),
          static_cast<unsigned long>(array->GetNumberOfComponents()) };
        if (this->Impl->InitDynamicDataset(group, arrayName, dataType,
              array->GetNumberOfComponents(), ChunkSizeComponent,
              this->CompressionLevel) == H5I_INVALID_HID)
        {
          vtkWarningMacro(<< "Could not initialize offset dataset for: " << arrayName
                          << " when creating: " << this->FileName);
          return false;
        }
      }

      // Add actual array in the dataset
      if (!this->Impl->AddOrCreateDataset(group, arrayName, dataType, array))
      {
        vtkErrorMacro(<< "Can not create array " << arrayName << " of attribute " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendBlocks(hid_t group, vtkPartitionedDataSetCollection* pdc)
{
  for (int datasetId = 0; datasetId < static_cast<int>(pdc->GetNumberOfPartitionedDataSets());
       datasetId++)
  {
    vtkHDF::ScopedH5GHandle datasetGroup;
    vtkPartitionedDataSet* currentBlock = pdc->GetPartitionedDataSet(datasetId);
    std::string currentName = ::getBlockName(pdc, datasetId);

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
      datasetGroup = this->Impl->CreateHdfGroup(group, currentName.c_str());
      this->DispatchDataObject(datasetGroup, currentBlock);
    }

    this->Impl->CreateScalarAttribute(datasetGroup, "Index", datasetId);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendExternalBlock(vtkDataObject* block, std::string& blockName)
{
  // Write the block data in an external file
  std::string subfileName = ::getExternalBlockFileName(std::string(this->FileName), blockName);
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(block);
  writer->SetFileName(subfileName.c_str());
  if (!writer->Write())
  {
    vtkErrorMacro(<< "Could not write block file " << subfileName);
    return false;
  }

  // Create external link
  if (this->Impl->CreateExternalLink(
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
      this->Impl->CreateSoftLink(this->Impl->GetRoot(), linkSource.c_str(), linkTarget.c_str());
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendMultiblock(hid_t assemblyGroup, vtkMultiBlockDataSet* mb)
{
  // Iterate over the children of the multiblock, recurse if needed.
  vtkSmartPointer<vtkDataObjectTreeIterator> treeIter;
  treeIter.TakeReference(mb->NewTreeIterator());
  treeIter->TraverseSubTreeOff(); // We use recursion on subtrees instead
  treeIter->SkipEmptyNodesOff();
  treeIter->VisitOnlyLeavesOff();

  for (treeIter->InitTraversal(); !treeIter->IsDoneWithTraversal(); treeIter->GoToNextItem())
  {
    // Retrieve name from metadata or create one
    std::string subTreeName;
    if (mb->HasMetaData(treeIter) && mb->GetMetaData(treeIter)->Has(vtkCompositeDataSet::NAME()))
    {
      subTreeName = mb->GetMetaData(treeIter)->Get(vtkCompositeDataSet::NAME());
    }
    if (subTreeName.empty())
    {
      subTreeName = "Block" + std::to_string(treeIter->GetCurrentFlatIndex());
    }

    if (treeIter->GetCurrentDataObject()->IsA("vtkMultiBlockDataSet"))
    {
      // Create a subgroup and recurse
      auto subTree = vtkMultiBlockDataSet::SafeDownCast(treeIter->GetCurrentDataObject());
      this->AppendMultiblock(
        this->Impl->CreateHdfGroupWithLinkOrder(assemblyGroup, subTreeName.c_str()), subTree);
    }
    else
    {
      if (this->UseExternalComposite)
      {
        // Create the block in a separate file and link it externally
        if (!this->AppendExternalBlock(treeIter->GetCurrentDataObject(), subTreeName))
        {
          return false;
        }
      }
      else
      {
        // Create a subgroup to root, write the data into it and softlink it to the assembly
        vtkHDF::ScopedH5GHandle datasetGroup =
          this->Impl->CreateHdfGroupWithLinkOrder(this->Impl->GetRoot(), subTreeName.c_str());
        this->DispatchDataObject(datasetGroup, treeIter->GetCurrentDataObject());
      }

      const std::string linkTarget = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" + subTreeName;
      const std::string linkSource = this->Impl->GetGroupName(assemblyGroup) + "/" + subTreeName;

      this->Impl->CreateSoftLink(this->Impl->GetRoot(), linkSource.c_str(), linkTarget.c_str());
      vtkHDF::ScopedH5GHandle linkedGroup =
        this->Impl->OpenExistingGroup(this->Impl->GetRoot(), linkTarget.c_str());
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendTimeValues(hid_t group)
{
  if (this->Impl->CreateScalarAttribute(group, "NSteps", this->NumberOfTimeSteps) ==
    H5I_INVALID_HID)
  {
    vtkWarningMacro(<< "Could not create steps group when creating: " << this->FileName);
    return false;
  }

  vtkNew<vtkDoubleArray> timeStepsArray;
  timeStepsArray->SetArray(this->timeSteps, this->NumberOfTimeSteps, 1);
  return this->Impl->CreateDatasetFromDataArray(group, "Values", H5T_IEEE_F32LE, timeStepsArray) !=
    H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataArrayOffset(
  vtkAbstractArray* array, const char* arrayName, const char* offsetsGroupName)
{
  vtkHDF::ScopedH5GHandle offsetsGroup =
    H5Gopen(this->Impl->GetStepsGroup(), offsetsGroupName, H5P_DEFAULT);

  if (this->CurrentTimeIndex == 0)
  {
    // Initialize offsets array
    hsize_t ChunkSize1D[] = { static_cast<hsize_t>(this->ChunkSize), 1 };
    if (this->Impl->InitDynamicDataset(offsetsGroup, arrayName, H5T_STD_I64LE, 1, ChunkSize1D) ==
      H5I_INVALID_HID)
    {
      vtkWarningMacro(<< "Could not initialize transient dataset for: " << arrayName
                      << " when creating: " << this->FileName);
      return false;
    }

    // Push a 0 value to the offsets array
    if (!this->Impl->AddOrCreateSingleValueDataset(offsetsGroup, arrayName, 0, false))
    {
      vtkWarningMacro(<< "Could not push a 0 value in the offsets array: " << arrayName
                      << " when creating: " << this->FileName);
      return false;
    }
  }
  else if (this->CurrentTimeIndex < this->NumberOfTimeSteps)
  {
    // Append offset to offset array
    if (!this->Impl->AddOrCreateSingleValueDataset(
          offsetsGroup, arrayName, array->GetNumberOfTuples(), true, false))
    {
      vtkWarningMacro(<< "Could not insert a value in the offsets array: " << arrayName
                      << " when creating: " << this->FileName);
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
/* TO IMPROVE
 * The template here could be replaced by a vtkDataSet once the GetMeshMTime
 * method will be implemented at its level.
 */
template <typename vtkStaticMeshDataSetT>
bool vtkHDFWriter::HasGeometryChangedFromPreviousStep(vtkStaticMeshDataSetT* input)
{
  return input->GetMeshMTime() != this->PreviousStepMeshMTime;
}

//------------------------------------------------------------------------------
/* TO IMPROVE
 * Here too we could avoid casting and use vtkDataSet when it'll support
 * the GetMeshMTime method.
 */
void vtkHDFWriter::UpdatePreviousStepMeshMTime(vtkDataObject* input)
{
  if (auto polyDataInput = vtkPolyData::SafeDownCast(input))
  {
    this->PreviousStepMeshMTime = polyDataInput->GetMeshMTime();
  }
  else if (auto unstructuredGridInput = vtkUnstructuredGrid::SafeDownCast(input))
  {
    this->PreviousStepMeshMTime = unstructuredGridInput->GetMeshMTime();
  }
}

VTK_ABI_NAMESPACE_END
