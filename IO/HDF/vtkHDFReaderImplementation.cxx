// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReaderImplementation.h"
#include <stdexcept>
#include <type_traits>

#include "vtkAMRBox.h"
#include "vtkAMRUtilities.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFUtilities.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <sstream>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr vtkIdType BYTE_SIZE = 8;
auto bitSizeToBytes = [](vtkIdType numBits)
{
  return (numBits + BYTE_SIZE - 1) / BYTE_SIZE; // Integer 'ceil'
};
}

//------------------------------------------------------------------------------
vtkHDFReader::Implementation::Implementation(vtkHDFReader* reader)
  : File(-1)
  , VTKGroup(-1)
  , DataSetType(-1)
  , NumberOfPieces(-1)
  , Reader(reader)
{
  std::fill(this->AttributeDataGroup.begin(), this->AttributeDataGroup.end(), -1);
  std::fill(this->Version.begin(), this->Version.end(), 0);
}

//------------------------------------------------------------------------------
vtkHDFReader::Implementation::~Implementation()
{
  this->Close();
}

//------------------------------------------------------------------------------
std::vector<hsize_t> vtkHDFReader::Implementation::GetDimensions(const char* datasetName)
{
  return vtkHDFUtilities::GetDimensions(this->File, datasetName);
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::Open(const char* fileName)
{
  if (!fileName)
  {
    vtkErrorWithObjectMacro(this->Reader, "Invalid filename: " << fileName);
    return false;
  }

  if (this->FileName.empty() || this->FileName != fileName || this->File < 0)
  {
    this->FileName = fileName;
    if (this->File >= 0)
    {
      this->Close();
    }

    if (!vtkHDFUtilities::Open(fileName, this->File))
    {
      return false;
    }

    return this->RetrieveHDFInformation(vtkHDFUtilities::VTKHDF_ROOT_PATH);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::OpenGroupAsVTKGroup(const std::string& groupPath)
{
  if ((this->VTKGroup = H5Gopen(this->File, groupPath.c_str(), H5P_DEFAULT)) < 0)
  {
    // the file doesn't exist or we try to read a non-VTKHDF file
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::RetrieveHDFInformation(const std::string& rootName)
{
  return vtkHDFUtilities::RetrieveHDFInformation(this->File, this->VTKGroup, rootName,
    this->Version, this->DataSetType, this->NumberOfPieces, this->AttributeDataGroup);
}

//------------------------------------------------------------------------------
std::size_t vtkHDFReader::Implementation::GetNumberOfSteps()
{
  if (this->File < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Cannot get number of steps if the file is not open");
    return 0;
  }
  return vtkHDFUtilities::GetNumberOfSteps(this->VTKGroup);
}

//------------------------------------------------------------------------------
int vtkHDFReader::Implementation::GetNumberOfPieces(vtkIdType step)
{
  if (step < 0 || this->GetNumberOfSteps() == 1 ||
    H5Lexists(this->VTKGroup, "Steps/NumberOfParts", H5P_DEFAULT) <= 0)
  {
    return this->NumberOfPieces;
  }
  std::vector<vtkIdType> buffer = this->GetMetadata("Steps/NumberOfParts", 1, step);
  if (buffer.empty())
  {
    vtkErrorWithObjectMacro(
      nullptr, "Could not read step " << step << " in NumberOfParts data set.");
    return -1;
  }
  this->NumberOfPieces = buffer[0];
  return this->NumberOfPieces;
}

//------------------------------------------------------------------------------
void vtkHDFReader::Implementation::Close()
{
  this->DataSetType = -1;
  this->NumberOfPieces = 0;
  std::fill(this->Version.begin(), this->Version.end(), 0);
  for (size_t i = 0; i < this->AttributeDataGroup.size(); ++i)
  {
    if (this->AttributeDataGroup[i] >= 0)
    {
      H5Gclose(this->AttributeDataGroup[i]);
      this->AttributeDataGroup[i] = -1;
    }
  }
  if (this->VTKGroup >= 0)
  {
    H5Gclose(this->VTKGroup);
    this->VTKGroup = -1;
  }
  if (this->File >= 0)
  {
    H5Fclose(this->File);
    this->File = -1;
  }
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::GetPartitionExtent(hsize_t partitionIndex, int* extent)
{
  const int RANK = 2;
  const char* datasetName = "/VTKHDF/Extents";

  // create the memory space
  hsize_t dimsm[RANK];
  dimsm[0] = 1;
  dimsm[1] = 6;
  vtkHDF::ScopedH5SHandle memspace = H5Screate_simple(RANK, dimsm, nullptr);
  if (memspace < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, << "Error H5Screate_simple for memory space");
    return false;
  }

  // create the file dataspace + hyperslab
  vtkHDF::ScopedH5DHandle dataset = H5Dopen(this->File, datasetName, H5P_DEFAULT);
  if (dataset < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, << std::string("Cannot open ") + datasetName);
    return false;
  }

  hsize_t start[RANK] = { partitionIndex, 0 }, count[RANK] = { 1, 2 };
  vtkHDF::ScopedH5SHandle dataspace = H5Dget_space(dataset);
  if (dataspace < 0)
  {
    vtkErrorWithObjectMacro(
      this->Reader, << std::string("Cannot get space for dataset ") + datasetName);
    return false;
  }

  if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, nullptr, count, nullptr) < 0)
  {
    vtkErrorWithObjectMacro(
      this->Reader, << std::string("Error selecting hyperslab for ") + datasetName);
    return false;
  }

  // read hyperslab
  if (H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, extent) < 0)
  {
    vtkErrorWithObjectMacro(
      this->Reader, << std::string("Error reading hyperslab from ") + datasetName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
template <typename T>
bool vtkHDFReader::Implementation::GetAttribute(
  const char* attributeName, size_t numberOfElements, T* value)
{
  return vtkHDFUtilities::GetAttribute(this->VTKGroup, attributeName, numberOfElements, value);
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::HasAttribute(const char* groupName, const char* attributeName)
{
  vtkHDF::ScopedH5GHandle groupID = H5Gopen(this->File, groupName, H5P_DEFAULT);
  if (groupID < 0)
  {
    return false;
  }
  return H5Aexists(groupID, attributeName) > 0;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkHDFReader::Implementation::GetArrayNames(int attributeType)
{
  return vtkHDFUtilities::GetArrayNames(this->AttributeDataGroup, attributeType);
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkHDFReader::Implementation::GetOrderedChildrenOfGroup(
  const std::string& path)
{
  return vtkHDFUtilities::GetOrderedChildrenOfGroup(this->VTKGroup, path);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewArray(
  int attributeType, const char* name, const std::vector<hsize_t>& fileExtent)
{
  return vtkHDFUtilities::NewArrayForGroup(
    this->AttributeDataGroup[attributeType], name, fileExtent);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewArray(
  int attributeType, const char* name, hsize_t offset, hsize_t size)
{
  std::vector<hsize_t> fileExtent = { offset, offset + size };
  return vtkHDFUtilities::NewArrayForGroup(
    this->AttributeDataGroup[attributeType], name, fileExtent);
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkHDFReader::Implementation::NewFieldArray(
  const char* name, vtkIdType offset, vtkIdType size, vtkIdType dimMaxSize)
{
  return vtkHDFUtilities::NewFieldArray(this->AttributeDataGroup, name, offset, size, dimMaxSize);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewMetadataArray(
  const char* name, hsize_t offset, hsize_t size)
{
  std::vector<hsize_t> fileExtent = { offset, offset + size };
  return vtkHDFUtilities::NewArrayForGroup(this->VTKGroup, name, fileExtent);
}

//------------------------------------------------------------------------------
std::vector<vtkIdType> vtkHDFReader::Implementation::GetMetadata(
  const char* name, hsize_t size, hsize_t offset)
{
  return vtkHDFUtilities::GetMetadata(this->VTKGroup, name, size, offset);
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::IsPathSoftLink(const std::string& path)
{
  H5L_info_t object;
  auto err = H5Lget_info(this->File, path.c_str(), &object, H5P_DEFAULT);
  if (err < 0)
  {
    vtkWarningWithObjectMacro(this->Reader, "Can't open '" << path << "' link.");
    return false;
  }

  return object.type == H5L_TYPE_SOFT;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::FillAssembly(vtkDataAssembly* assembly)
{
  if (this->DataSetType != VTK_PARTITIONED_DATA_SET_COLLECTION &&
    this->DataSetType != VTK_MULTIBLOCK_DATA_SET)
  {
    vtkErrorWithObjectMacro(this->Reader,
      "Wrong data set type, expected " << VTK_PARTITIONED_DATA_SET_COLLECTION
                                       << ", but got: " << this->DataSetType);
    return false;
  }

  std::string assemblyPath = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/Assembly";
  vtkHDF::ScopedH5GHandle assemblyID = H5Gopen(this->VTKGroup, assemblyPath.c_str(), H5P_DEFAULT);
  if (assemblyID <= H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Can't open 'Assembly' group. A valid Composite vtkHDF file should have it.");
    return false;
  }

  return this->FillAssembly(assembly, this->VTKGroup, 0, assemblyPath);
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::FillAssembly(
  vtkDataAssembly* assembly, hid_t assemblyHandle, int assemblyID, std::string path)
{
  vtkHDF::ScopedH5GHandle currentHandle = H5Gopen(assemblyHandle, path.c_str(), H5P_DEFAULT);
  if (currentHandle < H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Reader,
      "Can't open '" << path << "' group. A valid Composite vtkHDF file should have it.");
    return false;
  }

  std::vector<std::string> childrenPath;
  H5Literate_by_name(currentHandle, path.c_str(), H5_INDEX_CRT_ORDER, H5_ITER_INC, nullptr,
    vtkHDFUtilities::FileInfoCallBack, &childrenPath, H5P_DEFAULT);

  if (childrenPath.empty())
  {
    return true;
  }

  for (auto& childPath : childrenPath)
  {
    std::string childFullPath = path + "/" + childPath;
    vtkHDF::ScopedH5GHandle childHandle =
      H5Gopen(currentHandle, childFullPath.c_str(), H5P_DEFAULT);
    if (childHandle <= H5I_INVALID_HID)
    {
      continue;
    }

    // Prevent to iterate recursively on the dataset itself by checking if it's a link
    H5L_info_t object;
    auto err = H5Lget_info(currentHandle, childFullPath.c_str(), &object, H5P_DEFAULT);
    if (err < 0)
    {
      vtkErrorWithObjectMacro(this->Reader, "Can't open '" << childFullPath << "' link.");
      return false;
    }

    if (object.type == H5L_TYPE_SOFT)
    {
      int index = 0;
      vtkHDFUtilities::GetAttribute(childHandle, "Index", 1, &index);
      assembly->AddDataSetIndex(assemblyID, index);
    }
    else
    {
      int groupIndex = assembly->AddNode(childPath.c_str(), assemblyID);
      this->FillAssembly(assembly, currentHandle, groupIndex, childFullPath);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ComputeAMRBlocksPerLevels(unsigned int maxLevel)
{
  this->AMRInformation.Clear();

  unsigned int level = 0;
  while (level < maxLevel)
  {
    std::stringstream levelGroupName;

    levelGroupName << "Level" << level;
    if (H5Lexists(this->VTKGroup, levelGroupName.str().c_str(), H5P_DEFAULT) <= 0)
    {
      // The level does not exist, just exit.
      return true;
    }

    vtkHDF::ScopedH5GHandle levelGroupID =
      H5Gopen(this->VTKGroup, levelGroupName.str().c_str(), H5P_DEFAULT);
    if (levelGroupID == H5I_INVALID_HID)
    {
      vtkErrorWithObjectMacro(this->Reader, "Can't open group Level" << level);
      return false;
    }

    if (H5Lexists(levelGroupID, "AMRBox", H5P_DEFAULT) <= 0)
    {
      vtkErrorWithObjectMacro(this->Reader, "No AMRBox dataset at Level" << level);
      return false;
    }

    vtkHDF::ScopedH5DHandle amrBoxDatasetID = H5Dopen(levelGroupID, "AMRBox", H5P_DEFAULT);
    if (amrBoxDatasetID == H5I_INVALID_HID)
    {
      vtkErrorWithObjectMacro(this->Reader, "Can't find AMRBox dataset at Level" << level);
      return false;
    }

    vtkHDF::ScopedH5SHandle spaceID = H5Dget_space(amrBoxDatasetID);
    if (spaceID == H5I_INVALID_HID)
    {
      vtkErrorWithObjectMacro(this->Reader, "Can't get space of AMRBox dataset at Level" << level);
      return false;
    }

    std::array<hsize_t, 2> dimensions;
    if (H5Sget_simple_extent_dims(spaceID, dimensions.data(), nullptr) <= 0)
    {
      vtkErrorWithObjectMacro(
        this->Reader, "Can't get space dimensions of AMRBox dataset at Level" << level);
      return false;
    }

    this->AMRInformation.BlocksPerLevel.push_back(dimensions[0]);

    ++level;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::GetImageAttributes(
  int WholeExtent[6], double Origin[3], double Spacing[3])
{

  if (!this->GetAttribute("WholeExtent", 6, WholeExtent))
  {
    vtkErrorWithObjectMacro(this->Reader, "Could not get WholeExtent attribute");
    return false;
  }
  if (!this->GetAttribute("Origin", 3, Origin))
  {
    vtkErrorWithObjectMacro(this->Reader, "Could not get Origin attribute");
    return false;
  }
  if (!this->GetAttribute("Spacing", 3, Spacing))
  {
    vtkErrorWithObjectMacro(this->Reader, "Could not get Spacing attribute");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ComputeAMROffsetsPerLevels(
  vtkDataArraySelection* dataArraySelection[3], vtkIdType step, unsigned int maxLevel)
{
  if (!dataArraySelection)
  {
    return false;
  }

  this->AMRInformation.Clear();

  if (this->DataSetType != VTK_OVERLAPPING_AMR)
  {
    vtkWarningWithObjectMacro(
      this->Reader, "Bad usage of this method. Should only be use for OverlappingAMR");
    return true;
  }

  vtkIdType numberOfSteps = this->GetNumberOfSteps();
  unsigned int level = 0;
  while (level < maxLevel)
  {
    std::stringstream levelGroupName;
    levelGroupName << "Steps/Level" << level;
    if (H5Lexists(this->VTKGroup, levelGroupName.str().c_str(), H5P_DEFAULT) <= 0)
    {
      // The level does not exist, just exit.
      return true;
    }

    vtkHDF::ScopedH5GHandle levelGroupID =
      H5Gopen(this->VTKGroup, levelGroupName.str().c_str(), H5P_DEFAULT);
    if (levelGroupID == H5I_INVALID_HID)
    {
      vtkErrorWithObjectMacro(this->Reader, "Can't open group Level" << level);
      return false;
    }

    std::vector<int> numberOfBox;
    numberOfBox.resize(numberOfSteps);

    std::vector<vtkIdType> boxOffsets;
    boxOffsets.resize(numberOfSteps);

    // Due to an error in the VTKHDF File Format 2.2, few array names miss an 's' at
    // the end. As we should support any VTKHDF version 2.X, we should handle it.
    // It concerns: NumberOfAMRBoxes, AMRBoxOffsets and Point/Cell/FieldDataOffsets
    std::string numberOfBoxesDatasetName = "NumberOfAMRBoxes";
    std::string amrboxOffsetsDatasetName = "AMRBoxOffsets";
    std::array<const char*, 3> groupNames = { "PointDataOffsets", "CellDataOffsets",
      "FieldDataOffsets" };

    if (this->Version[0] == 2 && this->Version[1] == 2)
    {
      numberOfBoxesDatasetName = "NumberOfAMRBox";
      amrboxOffsetsDatasetName = "AMRBoxOffset";
      groupNames = { "PointDataOffset", "CellDataOffset", "FieldDataOffset" };
    }

    if (H5Lexists(levelGroupID, numberOfBoxesDatasetName.c_str(), H5P_DEFAULT) <= 0)
    {
      vtkErrorWithObjectMacro(
        this->Reader, "No " << numberOfBoxesDatasetName << " dataset at Steps/Level" << level);
      return false;
    }

    vtkHDF::ScopedH5DHandle amrBoxDatasetID =
      H5Dopen(levelGroupID, numberOfBoxesDatasetName.c_str(), H5P_DEFAULT);
    if (amrBoxDatasetID == H5I_INVALID_HID)
    {
      vtkErrorWithObjectMacro(this->Reader,
        "Can't find " << numberOfBoxesDatasetName << " dataset at Steps/Level" << level);
      return false;
    }

    if (H5Dread(
          amrBoxDatasetID, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, numberOfBox.data()) < 0)
    {
      vtkErrorWithObjectMacro(this->Reader, << "Error reading hyperslab");
      return false;
    }

    if (H5Lexists(levelGroupID, amrboxOffsetsDatasetName.c_str(), H5P_DEFAULT) <= 0)
    {
      vtkErrorWithObjectMacro(
        this->Reader, "No " << amrboxOffsetsDatasetName << " dataset at Steps/Level" << level);
      return false;
    }

    vtkHDF::ScopedH5DHandle boxOffsetID =
      H5Dopen(levelGroupID, amrboxOffsetsDatasetName.c_str(), H5P_DEFAULT);
    if (boxOffsetID == H5I_INVALID_HID)
    {
      vtkErrorWithObjectMacro(
        this->Reader, "Can't " << amrboxOffsetsDatasetName << " dataset at Steps/Level" << level);
      return false;
    }

    if (H5Dread(boxOffsetID, VTK_ID_H5T, H5S_ALL, H5S_ALL, H5P_DEFAULT, boxOffsets.data()) < 0)
    {
      vtkErrorWithObjectMacro(this->Reader, << "Error reading hyperslab from ");
      return false;
    }

    this->AMRInformation.BlocksPerLevel.push_back(numberOfBox[step]);
    this->AMRInformation.BlockOffsetsPerLevel.push_back((boxOffsets[step]));

    for (int attributeType = vtkDataObject::AttributeTypes::POINT;
         attributeType <= vtkDataObject::AttributeTypes::FIELD; ++attributeType)
    {
      if (H5Lexists(levelGroupID, groupNames[attributeType], H5P_DEFAULT) <= 0)
      {
        // These groups are optional
        continue;
      }

      vtkHDF::ScopedH5GHandle cellOffsetID =
        H5Gopen(levelGroupID, groupNames[attributeType], H5P_DEFAULT);
      if (cellOffsetID == H5I_INVALID_HID)
      {
        vtkErrorWithObjectMacro(this->Reader,
          "Can't open " << groupNames[attributeType] << " group at Steps/Level" << level);
        return false;
      }

      const std::vector<std::string> arrayNames = this->GetArrayNames(attributeType);
      for (const std::string& name : arrayNames)
      {
        if (!dataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
        {
          continue;
        }

        if (H5Lexists(cellOffsetID, name.c_str(), H5P_DEFAULT) <= 0)
        {
          vtkErrorWithObjectMacro(
            this->Reader, "Can't open " << name << " group at Steps/Level" << level);
          return false;
        }

        vtkHDF::ScopedH5DHandle constantID = H5Dopen(cellOffsetID, name.c_str(), H5P_DEFAULT);
        if (constantID == H5I_INVALID_HID)
        {
          vtkErrorWithObjectMacro(
            this->Reader, "Can't open " << name << " dataset at Steps/Level" << level);
          return false;
        }

        std::vector<vtkIdType> cellOffsets;
        cellOffsets.resize(numberOfSteps);
        if (H5Dread(constantID, VTK_ID_H5T, H5S_ALL, H5S_ALL, H5P_DEFAULT, cellOffsets.data()) < 0)
        {
          vtkErrorWithObjectMacro(this->Reader, << "Error reading hyperslab from ");
          return false;
        }

        switch (attributeType)
        {
          case vtkDataObject::AttributeTypes::POINT:
            this->AMRInformation.PointOffsetsPerLevel[name].push_back(cellOffsets[step]);
            break;
          case vtkDataObject::AttributeTypes::CELL:
            this->AMRInformation.CellOffsetsPerLevel[name].push_back(cellOffsets[step]);
            break;
          case vtkDataObject::AttributeTypes::FIELD:
            this->AMRInformation.FieldOffsetsPerLevel[name].push_back(cellOffsets[step]);
            if (step + 1 < static_cast<int>(cellOffsets.size()))
            {
              vtkIdType fieldSize = cellOffsets[step + 1] - cellOffsets[step];
              this->AMRInformation.FieldSizesPerLevel[name].push_back(fieldSize);
            }
            else
            {
              this->AMRInformation.FieldSizesPerLevel[name].push_back(-1);
            }

            break;
        }
      }
    }

    ++level;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadLevelTopology(unsigned int level,
  const std::string& levelGroupName, vtkOverlappingAMR* data, double origin[3], bool isTemporalData)
{
  vtkHDF::ScopedH5GHandle levelGroupID =
    H5Gopen(this->VTKGroup, levelGroupName.c_str(), H5P_DEFAULT);
  if (levelGroupID == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't open group Level" << level);
    return false;
  }

  std::array<double, 3> spacing = { 0, 0, 0 };
  if (!this->ReadLevelSpacing(levelGroupID, spacing.data()))
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Error while reading spacing attribute at level " << level);
    return false;
  }
  data->SetSpacing(level, spacing.data());

  std::vector<int> amrBoxRawData;
  if (!this->ReadAMRBoxRawValues(levelGroupID, amrBoxRawData, level, isTemporalData))
  {
    vtkErrorWithObjectMacro(this->Reader, "Error while reading AMRBox at level " << level);
    return false;
  }

  if (amrBoxRawData.size() % 6 != 0)
  {
    vtkErrorWithObjectMacro(this->Reader,
      "The size of the \"AMRBox\" dataset at Level" << level << " is not a multiple of 6.");
    return false;
  }

  vtkIdType numberOfDatasets = static_cast<vtkIdType>(amrBoxRawData.size() / 6);
  for (vtkIdType dataSetIndex = 0; dataSetIndex < numberOfDatasets; ++dataSetIndex)
  {
    int* currentAMRBoxRawData = amrBoxRawData.data() + (6 * dataSetIndex);
    vtkAMRBox amrBox(currentAMRBoxRawData);

    data->SetAMRBox(level, dataSetIndex, amrBox);
    vtkNew<vtkUniformGrid> dataSet;
    dataSet->Initialize();

    const int* lowCorner = amrBox.GetLoCorner();
    double dataSetOrigin[3] = { origin[0] + lowCorner[0] * spacing[0],
      origin[1] + lowCorner[1] * spacing[1], origin[2] + lowCorner[2] * spacing[2] };
    dataSet->SetOrigin(dataSetOrigin);

    dataSet->SetSpacing(spacing.data());
    std::array<int, 3> numberOfNodes = { 0, 0, 0 };
    amrBox.GetNumberOfNodes(numberOfNodes.data());
    dataSet->SetDimensions(numberOfNodes.data());

    data->SetDataSet(level, dataSetIndex, dataSet);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadLevelData(unsigned int level,
  const std::string& levelGroupName, vtkOverlappingAMR* data,
  vtkDataArraySelection* dataArraySelection[3], bool isTemporalData)
{
  vtkHDF::ScopedH5GHandle levelGroupID =
    H5Gopen(this->VTKGroup, levelGroupName.c_str(), H5P_DEFAULT);
  if (levelGroupID == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't open group Level" << level);
    return false;
  }

  // Now read actual data - one array at a time
  std::array<const char*, 3> groupNames = { "PointData", "CellData", "FieldData" };
  for (int attributeType = vtkDataObject::AttributeTypes::POINT;
       attributeType <= vtkDataObject::AttributeTypes::FIELD; ++attributeType)
  {
    vtkHDF::ScopedH5GHandle groupID = H5Gopen(levelGroupID, groupNames[attributeType], H5P_DEFAULT);
    if (groupID == H5I_INVALID_HID)
    {
      // It's OK to not have groups in the file if there are no data arrays
      // for that attribute type.
      continue;
    }

    const std::vector<std::string> arrayNames = this->GetArrayNames(attributeType);
    for (const std::string& name : arrayNames)
    {
      if (!dataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
      {
        continue;
      }

      // Open dataset
      hid_t tempNativeType = H5I_INVALID_HID;
      std::vector<hsize_t> dims;
      vtkHDF::ScopedH5DHandle datasetID =
        vtkHDFUtilities::OpenDataSet(groupID, name.c_str(), &tempNativeType, dims);
      vtkHDF::ScopedH5THandle nativeType = tempNativeType;
      if (datasetID < 0)
      {
        continue;
      }

      // Iterate over all datasets, read data and assign attribute
      hsize_t dataOffset = 0;
      hsize_t dataSize = 0;
      unsigned int numberOfDatasets = data->GetNumberOfDataSets(level);
      for (unsigned int dataSetIndex = 0; dataSetIndex < numberOfDatasets; ++dataSetIndex)
      {
        const vtkAMRBox& amrBox = data->GetAMRBox(level, dataSetIndex);
        auto dataSet = data->GetDataSet(level, dataSetIndex);
        if (dataSet == nullptr)
        {
          vtkErrorWithObjectMacro(this->Reader,
            "Error fetching dataset at level " << level << " and dataSetIndex " << dataSetIndex);
          return false;
        }

        // Here dataSize is the size of the previous dataset read. Offset
        // is incremented, and a new size is specified after the increment.
        // This allows for reading AMR's where the size of the blocks vary
        // inside each level.
        dataOffset += dataSize;

        hsize_t cellOffset = 0;
        switch (attributeType)
        {
          case vtkDataObject::AttributeTypes::POINT:
            dataSize = amrBox.GetNumberOfNodes();
            if (isTemporalData)
            {
              if (this->AMRInformation.PointOffsetsPerLevel.count(name) == 1)
              {
                cellOffset = this->AMRInformation.PointOffsetsPerLevel[name][level];
              }
            }
            break;
          case vtkDataObject::AttributeTypes::CELL:
            dataSize = amrBox.GetNumberOfCells();
            if (isTemporalData)
            {
              if (this->AMRInformation.CellOffsetsPerLevel.count(name) == 1)
              {
                cellOffset = this->AMRInformation.CellOffsetsPerLevel[name][level];
              }
            }
            break;
          case vtkDataObject::AttributeTypes::FIELD:
            dataSize = dims[0] / numberOfDatasets;
            if (isTemporalData)
            {
              if (this->AMRInformation.FieldOffsetsPerLevel.count(name) == 1)
              {
                cellOffset =
                  this->AMRInformation.FieldOffsetsPerLevel[name][level] / numberOfDatasets;
                vtkIdType fieldSize = this->AMRInformation.FieldSizesPerLevel[name][level];
                if (fieldSize == -1)
                {
                  dataSize = (dataSize - cellOffset);
                }
                else
                {
                  dataSize = fieldSize;
                }
                dataSize /= numberOfDatasets;
              }
            }
        }

        std::vector<hsize_t> fileExtent = { cellOffset + dataOffset,
          cellOffset + dataOffset + dataSize };

        vtkSmartPointer<vtkDataArray> array;
        if ((array = vtk::TakeSmartPointer(vtkHDFUtilities::NewArrayForGroup(
               datasetID, nativeType, dims, fileExtent))) == nullptr)
        {
          vtkErrorWithObjectMacro(this->Reader, "Error reading array " << name);
          return false;
        }
        array->SetName(name.c_str());
        dataSet->GetAttributesAsFieldData(attributeType)->AddArray(array);
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadLevelSpacing(hid_t levelGroupID, double* spacing)
{
  if (!H5Aexists(levelGroupID, "Spacing"))
  {
    vtkErrorWithObjectMacro(this->Reader, "\"Spacing\" attribute does not exist.");
    return false;
  }
  vtkHDF::ScopedH5AHandle spacingAttributeID = H5Aopen_name(levelGroupID, "Spacing");
  if (spacingAttributeID < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't open \"Spacing\" attribute.");
    return false;
  }

  if (H5Aread(spacingAttributeID, H5T_NATIVE_DOUBLE, spacing) < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't read \"Spacing\" attribute.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadAMRBoxRawValues(
  hid_t levelGroupID, std::vector<int>& amrBoxRawData, int level, bool isTemporalData)
{
  hsize_t startBlock = 0;
  if (isTemporalData)
  {
    startBlock = static_cast<hsize_t>(this->AMRInformation.BlockOffsetsPerLevel[level]);
  }

  hsize_t numberOfBlock = static_cast<hsize_t>(this->AMRInformation.BlocksPerLevel[level]);

  if (H5Lexists(levelGroupID, "AMRBox", H5P_DEFAULT) <= 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "No AMRBox dataset");
    return false;
  }

  vtkHDF::ScopedH5DHandle amrBoxDatasetID = H5Dopen(levelGroupID, "AMRBox", H5P_DEFAULT);
  if (amrBoxDatasetID == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't open AMRBox dataset");
    return false;
  }

  vtkHDF::ScopedH5SHandle spaceID = H5Dget_space(amrBoxDatasetID);
  if (spaceID == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't get space of AMRBox dataset");
    return false;
  }

  std::array<hsize_t, 2> dimensions;
  if (H5Sget_simple_extent_dims(spaceID, dimensions.data(), nullptr) <= 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't get space dimensions of AMRBox dataset");
    return false;
  }

  if (dimensions[1] != 6)
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Wrong AMRBox dimension, expected 6, got: " << dimensions[1]);
    return false;
  }

  hsize_t startPosition[2] = { startBlock, 0 };
  hsize_t count[2] = { numberOfBlock, 6 };

  hid_t filSpace = H5Screate_simple(2, dimensions.data(), nullptr);
  H5Sselect_hyperslab(filSpace, H5S_SELECT_SET, startPosition, nullptr, count, nullptr);

  startPosition[0] = 0;
  hid_t memSpace = H5Screate_simple(2, dimensions.data(), nullptr);
  H5Sselect_hyperslab(memSpace, H5S_SELECT_SET, startPosition, nullptr, count, nullptr);

  hsize_t numberOfDatasets = numberOfBlock;
  amrBoxRawData.resize(numberOfDatasets * 6);
  if (H5Dread(
        amrBoxDatasetID, H5T_NATIVE_INT, memSpace, filSpace, H5P_DEFAULT, amrBoxRawData.data()) < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Can't read AMRBox dataset.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadAMRTopology(vtkOverlappingAMR* data, unsigned int level,
  unsigned int maxLevel, double origin[3], bool isTemporalData)
{
  if (this->AMRInformation.BlocksPerLevel.empty())
  {
    return false;
  }

  size_t numberOfLoadedLevels = 0;
  if (maxLevel == 0)
  {
    numberOfLoadedLevels = this->AMRInformation.BlocksPerLevel.size();
  }
  else
  {
    numberOfLoadedLevels =
      std::min(this->AMRInformation.BlocksPerLevel.size(), static_cast<size_t>(maxLevel));
  }

  data->Initialize(
    static_cast<int>(numberOfLoadedLevels), this->AMRInformation.BlocksPerLevel.data());
  data->SetOrigin(origin);
  data->SetGridDescription(VTK_XYZ_GRID);

  while (level < maxLevel)
  {
    std::string levelGroupName = "Level" + std::to_string(level);
    if (H5Lexists(this->VTKGroup, levelGroupName.c_str(), H5P_DEFAULT) <= 0)
    {
      break;
    }
    else
    {
      if (!this->ReadLevelTopology(level, levelGroupName, data, origin, isTemporalData))
      {
        vtkErrorWithObjectMacro(this->Reader, << "Can't read group Level" << level);
        return false;
      }
      ++level;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadAMRData(vtkOverlappingAMR* data, unsigned int level,
  unsigned int maxLevel, vtkDataArraySelection* dataArraySelection[3], bool isTemporalData)
{
  while (level < maxLevel)
  {
    std::string levelGroupName = "Level" + std::to_string(level);
    if (H5Lexists(this->VTKGroup, levelGroupName.c_str(), H5P_DEFAULT) <= 0)
    {
      break;
    }
    else
    {
      if (!this->ReadLevelData(level, levelGroupName, data, dataArraySelection, isTemporalData))
      {
        vtkErrorWithObjectMacro(this->Reader, << "Can't fill group Level" << level);
        return false;
      }
      ++level;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadHyperTreeGridData(vtkHyperTreeGrid* htg,
  const vtkDataArraySelection* arraySelection, const vtkIdType cellOffset,
  const vtkIdType treeIdsOffset, const vtkIdType depthOffset, const vtkIdType descriptorOffset,
  const vtkIdType maskOffset, const vtkIdType partOffset, const vtkIdType verticesPerDepthOffset,
  const vtkIdType depthLimit, const vtkIdType step)
{
  htg->Initialize();

  if (!this->ReadHyperTreeGridMetaInfo(htg))
  {
    return false;
  }

  if (!this->ReadHyperTreeGridDimensions(htg))
  {
    return false;
  }

  // Read meta-data for the current piece
  const vtkIdType treeCount = this->GetMetadata("NumberOfTrees", 1, partOffset)[0];
  const vtkIdType depthCount = this->GetMetadata("NumberOfDepths", 1, partOffset)[0];
  const vtkIdType cellCount = this->GetMetadata("NumberOfCells", 1, partOffset)[0];
  const vtkIdType descriptorCount = this->GetMetadata("DescriptorsSize", 1, partOffset)[0];

  // Read only the tree ids for the current piece
  const std::vector<vtkIdType> treeIds = this->GetMetadata("TreeIds", treeCount, treeIdsOffset);
  const std::vector<vtkIdType> depthPerTree =
    this->GetMetadata("DepthPerTree", treeCount, depthOffset);
  const std::vector<vtkIdType> numberOfCellsPerTreeDepth =
    this->GetMetadata("NumberOfCellsPerTreeDepth", depthCount, verticesPerDepthOffset);

  // Read Tree Descriptors
  const std::vector<hsize_t> descriptorExtent = { static_cast<hsize_t>(descriptorOffset),
    static_cast<hsize_t>(descriptorOffset + ::bitSizeToBytes(descriptorCount)) };
  vtkSmartPointer<vtkUnsignedCharArray> descriptorsByteArray =
    vtk::TakeSmartPointer(vtkArrayDownCast<vtkUnsignedCharArray>(
      vtkHDFUtilities::NewArrayForGroup(this->VTKGroup, "Descriptors", descriptorExtent)));

  // Build a bit array from the uint8 array, transferring the memory ownership to the bit array
  vtkNew<vtkBitArray> descriptor;
  descriptor->SetArray(descriptorsByteArray->GetPointer(0), descriptorCount, 0);
  descriptorsByteArray->SetArrayFreeFunction(nullptr);

  const bool hasMask = H5Lexists(this->VTKGroup, "Mask", H5P_DEFAULT) > 0;
  if (hasMask)
  {
    vtkNew<vtkBitArray> mask;
    mask->Allocate(cellCount);
    htg->SetMask(mask);
  }

  // Create cell arrays
  std::vector<vtkSmartPointer<vtkAbstractArray>> cellArrays;
  if (!this->CreateHyperTreeGridCellArrays(htg, cellArrays, arraySelection, cellCount))
  {
    return false;
  }

  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;
  vtkIdType treeDescriptorOffset = 0;
  vtkIdType inputCellOffset = 0;
  vtkIdType outputCellOffset = 0;
  vtkIdType numCellsPerDepthOffset = 0;

  // Iterate over trees, building them taking depth limiter into consideration.
  // Also appends masking and cell data
  for (const vtkIdType& treeId : treeIds)
  {
    // Compute both the total number of cells and the number of cells below the depth limit in the
    // current tree Number of cells in the current tree = sum of the number of cells in all depths
    vtkIdType lastDepthSize = 0;
    vtkIdType lastReadableDepthSize = 0; // Taking depth limit into account
    vtkIdType totalTreeSize = 0;
    vtkIdType readableTreeSize = 0; // Taking depth limit into account
    auto numCellsPerTreeDepthIt = numberOfCellsPerTreeDepth.begin() + numCellsPerDepthOffset;
    for (vtkIdType depth = 0; depth < depthPerTree[treeId]; depth++, numCellsPerTreeDepthIt++)
    {
      lastDepthSize = *numCellsPerTreeDepthIt;
      if (depth < depthLimit)
      {
        lastReadableDepthSize = lastDepthSize;
        readableTreeSize += lastDepthSize;
      }
      totalTreeSize += lastDepthSize;
    }

    // The descriptor has 1 bit entry for each cell in the tree, except for the last depth
    const vtkIdType descriptorSize = totalTreeSize - lastDepthSize;
    const vtkIdType readableDescriptorSize = readableTreeSize - lastReadableDepthSize;
    htg->InitializeNonOrientedCursor(treeCursor, treeId, true);
    treeCursor->SetGlobalIndexStart(outputCellOffset);

    if (!this->AppendCellDataForHyperTree(
          cellArrays, cellOffset, inputCellOffset, step, readableTreeSize))
    {
      return false;
    }

    if (hasMask &&
      !this->AppendMaskForHyperTree(htg, inputCellOffset, maskOffset, readableTreeSize))
    {
      return false;
    }

    // Build tree from descriptor
    treeCursor->GetTree()->BuildFromBreadthFirstOrderDescriptor(
      descriptor, readableDescriptorSize, treeDescriptorOffset);

    // Increment input & output offsets
    treeDescriptorOffset += descriptorSize;
    inputCellOffset += totalTreeSize;
    outputCellOffset += readableTreeSize;
    numCellsPerDepthOffset += depthPerTree[treeId];
  }

  // We pre-allocated `cellCount` items, but with depth limiter, it is possible than less than that
  // is used.
  for (auto& array : cellArrays)
  {
    array->Squeeze();
  }
  if (hasMask)
  {
    htg->GetMask()->Squeeze();
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadHyperTreeGridMetaInfo(vtkHyperTreeGrid* htg)
{
  // Read general info about this HTG
  int branchFactor = 0;
  if (!this->GetAttribute("BranchFactor", 1, &branchFactor))
  {
    vtkErrorWithObjectMacro(this->Reader, << "Cannot find BranchFactor attribute");
    return false;
  }
  htg->SetBranchFactor(branchFactor);

  // Read TransposedRootIndexing attribute
  int transposedRootIndexing = 0;
  if (H5Aexists(this->VTKGroup, "TransposedRootIndexing") > 0)
  {
    this->GetAttribute("TransposedRootIndexing", 1, &transposedRootIndexing);
  }
  htg->SetTransposedRootIndexing(transposedRootIndexing != 0);

  // Read Interface attributes if they exist
  bool hasInterfaceIntercepts = H5Aexists(this->VTKGroup, "InterfaceInterceptsName") > 0;
  bool hasInterfaceNormals = H5Aexists(this->VTKGroup, "InterfaceNormalsName") > 0;
  if (hasInterfaceIntercepts)
  {
    std::string interfaceInterceptsName;
    vtkHDFUtilities::GetStringAttribute(
      this->VTKGroup, "InterfaceInterceptsName", interfaceInterceptsName);
    htg->SetInterfaceInterceptsName(interfaceInterceptsName.c_str());
  }

  if (hasInterfaceNormals)
  {
    std::string interfaceNormalsName;
    vtkHDFUtilities::GetStringAttribute(
      this->VTKGroup, "InterfaceNormalsName", interfaceNormalsName);
    htg->SetInterfaceNormalsName(interfaceNormalsName.c_str());
  }

  if (hasInterfaceIntercepts && hasInterfaceNormals)
  {
    htg->SetHasInterface(true);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::ReadHyperTreeGridDimensions(vtkHyperTreeGrid* htg)
{
  std::array<int, 3> dimensions;
  if (!this->GetAttribute("Dimensions", 1, dimensions.data()))
  {
    vtkErrorWithObjectMacro(this->Reader, << "Missing dimensions attribute");
    return false;
  }

  // Read coordinate arrays
  std::vector<hsize_t> coordinates_extent{ 0, static_cast<hsize_t>(dimensions[0]) };
  auto XCoordinates = vtk::TakeSmartPointer(
    vtkHDFUtilities::NewArrayForGroup(this->VTKGroup, "XCoordinates", coordinates_extent));
  coordinates_extent[1] = static_cast<hsize_t>(dimensions[1]);
  auto YCoordinates = vtk::TakeSmartPointer(
    vtkHDFUtilities::NewArrayForGroup(this->VTKGroup, "YCoordinates", coordinates_extent));
  coordinates_extent[1] = static_cast<hsize_t>(dimensions[2]);
  auto ZCoordinates = vtk::TakeSmartPointer(
    vtkHDFUtilities::NewArrayForGroup(this->VTKGroup, "ZCoordinates", coordinates_extent));

  if (!XCoordinates || !YCoordinates || !ZCoordinates)
  {
    vtkErrorWithObjectMacro(this->Reader, << "Missing coordinates array");
    return false;
  }

  htg->SetDimensions(dimensions.data());
  htg->SetXCoordinates(XCoordinates);
  htg->SetYCoordinates(YCoordinates);
  htg->SetZCoordinates(ZCoordinates);

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::CreateHyperTreeGridCellArrays(vtkHyperTreeGrid* htg,
  std::vector<vtkSmartPointer<vtkAbstractArray>>& cellArrays,
  const vtkDataArraySelection* arraySelection, vtkIdType cellCount)
{
  constexpr int cellType = vtkDataObject::AttributeTypes::CELL;
  const std::vector<std::string> arrayNames = this->GetArrayNames(cellType);
  for (const std::string& arrayName : arrayNames)
  {
    if (arraySelection->ArrayIsEnabled(arrayName.c_str()))
    {
      // Read a null extent so we get the correct array type
      vtkSmartPointer<vtkDataArray> array;
      std::vector<hsize_t> nullExtent{ 0, 0 };
      if ((array = vtk::TakeSmartPointer(
             this->NewArray(cellType, arrayName.c_str(), nullExtent))) == nullptr)
      {
        vtkErrorWithObjectMacro(nullptr, "Error reading array " << arrayName);
        return false;
      }

      array->SetName(arrayName.c_str());
      array->Allocate(cellCount * array->GetNumberOfComponents());
      htg->GetCellData()->AddArray(array);
      cellArrays.emplace_back(array);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::AppendCellDataForHyperTree(
  std::vector<vtkSmartPointer<vtkAbstractArray>>& cellArrays, vtkIdType cellOffset,
  vtkIdType inputCellOffset, const vtkIdType step, const vtkIdType readableTreeSize)
{
  constexpr int cellType = vtkDataObject::AttributeTypes::CELL;

  for (auto& array : cellArrays)
  {
    std::string arrayName = array->GetName();
    vtkIdType startReadOffset = cellOffset +
      std::max<vtkIdType>(this->GetArrayOffset(step, cellType, arrayName), 0) + inputCellOffset;

    std::vector<hsize_t> arrayReadExtent{ static_cast<hsize_t>(startReadOffset),
      static_cast<hsize_t>(startReadOffset + readableTreeSize) };

    vtkSmartPointer<vtkDataArray> addedArray;
    if ((addedArray = vtk::TakeSmartPointer(
           this->NewArray(cellType, arrayName.c_str(), arrayReadExtent))) == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Error reading array " << arrayName);
      return false;
    }

    // Copy read array at the end of the data array
    array->InsertTuples(array->GetNumberOfTuples(), readableTreeSize, 0, addedArray);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::AppendMaskForHyperTree(vtkHyperTreeGrid* htg,
  vtkIdType inputCellOffset, const vtkIdType maskOffset, const vtkIdType readableTreeSize)
{
  // Read and append mask
  vtkIdType maskByteStartOffset = maskOffset + (inputCellOffset / BYTE_SIZE);
  vtkIdType offsetInStartingByte =
    inputCellOffset % BYTE_SIZE; // Mask for the tree may not start at the start of a byte
  vtkIdType maskByteSize = ::bitSizeToBytes(readableTreeSize + offsetInStartingByte);
  std::vector<hsize_t> maskExtent = { static_cast<hsize_t>(maskByteStartOffset),
    static_cast<hsize_t>(maskByteStartOffset + maskByteSize) };

  // Read only the relevant part of the mask, taking depth limiter into account
  vtkSmartPointer<vtkUnsignedCharArray> addedMaskByteArray =
    vtk::TakeSmartPointer(vtkArrayDownCast<vtkUnsignedCharArray>(
      vtkHDFUtilities::NewArrayForGroup(this->VTKGroup, "Mask", maskExtent)));

  vtkNew<vtkBitArray> addedMask;
  addedMask->SetArray(vtkArrayDownCast<vtkUnsignedCharArray>(addedMaskByteArray)->GetPointer(0),
    maskByteSize * BYTE_SIZE, 0);
  addedMaskByteArray->SetArrayFreeFunction(nullptr);
  if (htg->GetMask())
  {
    htg->GetMask()->InsertTuples(
      htg->GetMask()->GetNumberOfTuples(), readableTreeSize, offsetInStartingByte, addedMask);
  }

  return true;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::GetStepValues()
{
  if (this->File < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Cannot get step values if the file is not open");
  }
  return this->GetStepValues(this->VTKGroup);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::GetStepValues(hid_t group)
{
  if (group < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Cannot get step values from empty group");
  }

  if (H5Lexists(group, "Steps", H5P_DEFAULT) <= 0)
  {
    // Steps group does not exist
    return nullptr;
  }

  // Steps group does exist
  vtkHDF::ScopedH5GHandle steps = H5Gopen(group, "Steps", H5P_DEFAULT);
  if (steps < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, "Could not open steps group");
    return nullptr;
  }

  std::vector<hsize_t> fileExtent;
  return vtkHDFUtilities::NewArrayForGroup(steps, "Values", fileExtent);
}

//------------------------------------------------------------------------------
vtkIdType vtkHDFReader::Implementation::GetArrayOffset(
  vtkIdType step, int attributeType, std::string name)
{
  return vtkHDFUtilities::GetArrayOffset(this->VTKGroup, step, attributeType, name);
}

//------------------------------------------------------------------------------
std::array<vtkIdType, 2> vtkHDFReader::Implementation::GetFieldArraySize(
  vtkIdType step, std::string name)
{
  return vtkHDFUtilities::GetFieldArraySize(this->VTKGroup, step, name);
}

//------------------------------------------------------------------------------
// explicit template instantiation
template bool vtkHDFReader::Implementation::GetAttribute<int>(
  const char* attributeName, size_t dim, int* value);
template bool vtkHDFReader::Implementation::GetAttribute<double>(
  const char* attributeName, size_t dim, double* value);
VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkHDFReader::Implementation::GetNewDataSet(
  int dataSetType, int numPieces)
{
  const std::array partitionedTypes = { VTK_UNSTRUCTURED_GRID, VTK_POLY_DATA, VTK_HYPER_TREE_GRID };
  vtkSmartPointer<vtkDataObject> newOutput = nullptr;
  if (numPieces > 1 &&
    std::find(partitionedTypes.begin(), partitionedTypes.end(), dataSetType) !=
      partitionedTypes.end())
  {
    newOutput = vtkSmartPointer<vtkPartitionedDataSet>::New();
  }
  else if (dataSetType == VTK_IMAGE_DATA)
  {
    newOutput = vtkSmartPointer<vtkImageData>::New();
  }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID)
  {
    newOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
  }
  else if (dataSetType == VTK_POLY_DATA)
  {
    newOutput = vtkSmartPointer<vtkPolyData>::New();
  }
  else if (dataSetType == VTK_HYPER_TREE_GRID)
  {
    newOutput = vtkSmartPointer<vtkHyperTreeGrid>::New();
  }
  else if (dataSetType == VTK_OVERLAPPING_AMR)
  {
    newOutput = vtkSmartPointer<vtkOverlappingAMR>::New();
  }
  else if (dataSetType == VTK_PARTITIONED_DATA_SET_COLLECTION)
  {
    newOutput = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  }
  else if (dataSetType == VTK_MULTIBLOCK_DATA_SET)
  {
    newOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  }
  else
  {
    vtkErrorWithObjectMacro(this->Reader, "HDF dataset type unknown: " << dataSetType);
    return nullptr;
  }

  return newOutput;
}
