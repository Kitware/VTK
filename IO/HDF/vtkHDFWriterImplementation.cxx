// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFWriterImplementation.h"

#include "H5public.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFUtilities.h"
#include "vtkHDFVersion.h"
#include "vtkMultiProcessController.h"
#include "vtkPartitionedDataSet.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <sstream>
#include <string_view>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkHDFWriter::Implementation::SetUseDistributedWriting(bool distributedFile)
{
#ifdef H5_HAVE_PARALLEL
  this->UseDistributedWriting = distributedFile;
#else
  this->UseDistributedWriting = false;
  if (distributedFile)
  {
    vtkErrorWithObjectMacro(
      this->Writer, "Distributed single-file writing requires HDF5 built with MPI support");
  }
#endif
}

namespace PATH
{
// VTKHDF Group & Dataset paths definitions, used to create virtual datasets properly in meta-files.

// vtkPointSet
constexpr std::string_view POINTS = "Points";
constexpr std::string_view OFFSETS = "Offsets";
constexpr std::string_view TYPES = "Types";
constexpr std::string_view CONNECTIVITY = "Connectivity";
constexpr std::string_view NUMBER_OF_POINTS = "NumberOfPoints";
constexpr std::string_view NUMBER_OF_CELLS = "NumberOfCells";
constexpr std::string_view NUMBER_OF_CONNECTIVITY_IDS = "NumberOfConnectivityIds";

// vtkHyperTreeGrid
constexpr std::string_view DEPTH_PER_TREE = "DepthPerTree";
constexpr std::string_view NUMBER_OF_CELLS_PER_TREE_DEPTH = "NumberOfCellsPerTreeDepth";
constexpr std::string_view DESCRIPTORS = "Descriptors";
constexpr std::string_view TREE_IDS = "TreeIds";
constexpr std::string_view MASK = "Mask";
constexpr std::string_view NUMBER_OF_TREES = "NumberOfTrees";
constexpr std::string_view NUMBER_OF_DEPTHS = "NumberOfDepths";
constexpr std::string_view DESCRIPTORS_SIZE = "DescriptorsSize";
constexpr std::string_view XCOORDS = "XCoordinates";
constexpr std::string_view YCOORDS = "YCoordinates";
constexpr std::string_view ZCOORDS = "ZCoordinates";

constexpr std::string_view CELL_DATA = "CellData";
constexpr std::string_view POINT_DATA = "PointData";
constexpr std::string_view FIELD_DATA = "FieldData";

constexpr std::string_view STEPS = "Steps";
constexpr std::string_view STEPS_PART_OFFSETS = "Steps/PartOffsets";
constexpr std::string_view STEPS_NUMBER_OF_PARTS = "Steps/NumberOfParts";

using namespace std::literals::string_view_literals;

constexpr std::initializer_list<std::string_view> COUNT_VALUES = {
  NUMBER_OF_POINTS,
  NUMBER_OF_CELLS,
  NUMBER_OF_CONNECTIVITY_IDS,
  NUMBER_OF_TREES,
  NUMBER_OF_DEPTHS,
  DESCRIPTORS_SIZE,
};
constexpr std::initializer_list<std::string_view> PRIMITIVE_TYPES = {
  "Strips"sv,
  "Polygons"sv,
  "Vertices"sv,
  "Lines"sv,
};

/**
 * Return true if `path` contains one of the given `subpath`.
 */
bool ContainsAny(const std::string& path, std::initializer_list<std::string_view> subpath)
{
  for (auto& sub : subpath)
  {
    if (path.find(sub) != std::string::npos)
    {
      return true;
    }
  }
  return false;
}

/**
 * Get base path for the dataset: either /VTKHDF/ or /VTKHDF/XXX/YYY... for composite types
 */
std::string GetBasePath(const std::string& fullPath)
{
  auto removePrefix = [](std::string& path, const std::string_view& suffix)
  {
    if (path.find(suffix) != std::string::npos)
    {
      path = path.substr(0, path.find(suffix) - 1);
    }
  };

  std::string basePath = fullPath;
  for (const std::string_view& suffix : PATH::PRIMITIVE_TYPES)
  {
    removePrefix(basePath, suffix);
  }

  for (const std::string_view& suffix :
    { PATH::STEPS, PATH::CELL_DATA, PATH::POINT_DATA, PATH::FIELD_DATA })
  {
    removePrefix(basePath, suffix);
  }
  return basePath;
}

}

namespace
{
constexpr int SMALL_CHUNK = 100;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CollectPartitionOffsets(vtkDataObject* input)
{
  UnstructuredGridOffset temporalOffsets{}; // TODO: temporal collection
  return this->CollectPartitionOffsets(input, temporalOffsets);
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CollectPartitionOffsets(
  vtkDataObject* input, const UnstructuredGridOffset& temporalOffsets)
{
  this->UgOffsets = {};

  // Collect local partitions
  std::vector<vtkUnstructuredGrid*> localPartitions;
  auto pds = vtkPartitionedDataSet::SafeDownCast(input);
  if (pds)
  {
    localPartitions.reserve(pds->GetNumberOfPartitions());
    for (unsigned int partition = 0; partition < pds->GetNumberOfPartitions(); ++partition)
    {
      if (auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartitionAsDataObject(partition)))
      {
        localPartitions.emplace_back(ug);
      }
    }
  }
  else if (auto ug = vtkUnstructuredGrid::SafeDownCast(input))
  {
    localPartitions.emplace_back(ug);
  }

  // Share number of partitions in each rank
  vtkIdType localPartCount = localPartitions.size();
  if (pds)
  {
    localPartCount = 0;
    for (unsigned int part = 0; part < pds->GetNumberOfPartitions(); ++part)
    {
      localPartCount += pds->GetPartitionAsDataObject(part) ? 1 : 0;
    }
  }
  vtkMultiProcessController* controller = this->Writer->GetController();
  const int numRanks = controller->GetNumberOfProcesses();
  const vtkIdType localNumberOfPartitions = localPartCount;
  std::vector<vtkIdType> numberOfPartitions(numRanks);
  controller->AllGather(&localNumberOfPartitions, numberOfPartitions.data(), 1);

  PartitionedOffsets result;
  result.PartOffsets.assign(numRanks + 1, 0);
  for (int rank = 0; rank < numRanks; ++rank)
  {
    result.PartOffsets[rank + 1] = result.PartOffsets[rank] + numberOfPartitions[rank];
  }

  // Share size attributes using the UnstructuredGridOffset structure to store sizes
  std::vector<UnstructuredGridOffset> localSizes(localNumberOfPartitions);
  for (vtkIdType partition = 0; partition < localNumberOfPartitions; ++partition)
  {
    localSizes[partition].PartOffset = 1;
    auto* grid = localPartitions[partition];
    vtkCellArray* cells = grid->GetCells();
    localSizes[partition].PointOffset = localPartitions[partition]->GetNumberOfPoints();
    localSizes[partition].CellOffset = localPartitions[partition]->GetNumberOfCells();
    localSizes[partition].PointDataOffset = localSizes[partition].PointOffset;
    localSizes[partition].CellDataOffset = localSizes[partition].CellOffset;
    localSizes[partition].ConnectivityOffset = cells ? cells->GetNumberOfConnectivityIds() : 0;
    auto* faces = grid->GetPolyhedronFaces();
    auto* polyhedra = grid->GetPolyhedronFaceLocations();
    localSizes[partition].FaceOffset = faces ? faces->GetNumberOfCells() : 0;
    localSizes[partition].FacePartOffset = 1;
    localSizes[partition].FaceConnectivityOffset = faces ? faces->GetNumberOfConnectivityIds() : 0;
    localSizes[partition].PolyhedronOffset = localSizes[partition].CellOffset;
    localSizes[partition].PolyhedronToFaceOffset =
      polyhedra ? polyhedra->GetNumberOfConnectivityIds() : 0;
  }

  result.UgOffsets.resize(result.PartOffsets.back() + 1);
  const vtkIdType recordSize = static_cast<vtkIdType>(sizeof(UnstructuredGridOffset));
  std::vector<vtkIdType> receiveLengths(numRanks);
  std::vector<vtkIdType> receiveOffsets(numRanks);
  for (int rank = 0; rank < numRanks; ++rank)
  {
    receiveLengths[rank] = recordSize * numberOfPartitions[rank];
    receiveOffsets[rank] = recordSize * result.PartOffsets[rank];
  }
  controller->GetCommunicator()->AllGatherVVoidArray(localSizes.data(), result.UgOffsets.data(),
    recordSize * localNumberOfPartitions, receiveLengths.data(), receiveOffsets.data(),
    VTK_UNSIGNED_CHAR);

  // Add partition sizes to the temporal base
  UnstructuredGridOffset current = temporalOffsets;
  for (std::size_t partition = 0; partition + 1 < result.UgOffsets.size(); ++partition)
  {
    const auto sizes = result.UgOffsets[partition];
    result.UgOffsets[partition] = current;
    current.PartOffset += sizes.PartOffset;
    current.PointOffset += sizes.PointOffset;
    current.CellOffset += sizes.CellOffset;
    current.PointDataOffset += sizes.PointDataOffset;
    current.CellDataOffset += sizes.CellDataOffset;
    current.ConnectivityOffset += sizes.ConnectivityOffset;
    current.FaceOffset += sizes.FaceOffset;
    current.FacePartOffset += sizes.FacePartOffset;
    current.FaceConnectivityOffset += sizes.FaceConnectivityOffset;
    current.PolyhedronOffset += sizes.PolyhedronOffset;
    current.PolyhedronToFaceOffset += sizes.PolyhedronToFaceOffset;
  }
  result.UgOffsets.back() = current;
  this->UgOffsets = std::move(result);

  this->PrintOffsetInformation(this->UgOffsets);

  return true;
}

void vtkHDFWriter::Implementation::PrintOffsetInformation(PartitionedOffsets& offsets)
{
  if (this->Writer->GetController() && this->Writer->GetController()->GetLocalProcessId() != 0)
  {
    return;
  }
  for (std::size_t i = 0; i < offsets.PartOffsets.size(); ++i)
  {
    vtkDebugWithObjectMacro(
      this->Writer, "PartOffset[" << i << "]" << " =" << offsets.PartOffsets[i]);
  }
  for (std::size_t i = 0; i < offsets.UgOffsets.size(); ++i)
  {
    [[maybe_unused]] const auto& o = offsets.UgOffsets[i];
    vtkDebugWithObjectMacro(this->Writer,
      "UnstructuredGridOffset[" << i << "]"
                                << " Part=" << o.PartOffset << " Point=" << o.PointOffset
                                << " Cell=" << o.CellOffset
                                << " Connectivity=" << o.ConnectivityOffset
                                << " Face=" << o.FaceOffset << " FacePart=" << o.FacePartOffset
                                << " FaceConnectivity=" << o.FaceConnectivityOffset
                                << " Polyhedron=" << o.PolyhedronOffset
                                << " PolyhedronToFace=" << o.PolyhedronToFaceOffset);
  }
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteHeader(hid_t group, const char* hdfType)
{
  if (H5Aexists(group, "Type") > 0)
  {
    // Header has already been written
    return true;
  }

  // Write type attribute to root
  if (this->CreateStringAttribute(group, "Type", hdfType) <= 0)
  {
    return false;
  }

  // Write version attribute to root
  hsize_t versionDimensions[1] = { 2 };
  vtkIdType versionBuffer[2] = { vtkHDFMajorVersion, vtkHDFMinorVersion };
  vtkHDF::ScopedH5SHandle versionDataspace{ H5Screate(H5S_SIMPLE) };
  if (versionDataspace == H5I_INVALID_HID)
  {
    return false;
  }
  if (H5Sset_extent_simple(versionDataspace, 1, versionDimensions, versionDimensions) < 0)
  {
    return false;
  }
  vtkHDF::ScopedH5AHandle versionAttribute =
    H5Acreate(group, "Version", H5T_STD_I64LE, versionDataspace, H5P_DEFAULT, H5P_DEFAULT);
  if (versionAttribute == H5I_INVALID_HID)
  {
    return false;
  }
  if (H5Awrite(versionAttribute, H5T_STD_I64LE, versionBuffer) < 0)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateFile(bool overwrite, const std::string& filename)
{
  // Create file
  vtkDebugWithObjectMacro(
    this->Writer, << "Creating file " << this->Writer->CurrentPiece << ": " << filename);

  vtkHDF::ScopedH5PHandle fileAccessProperties;
  hid_t fileAccessPropertiesId = H5P_DEFAULT;
  if (this->GetUseDistributedWriting())
  {
#ifdef H5_HAVE_PARALLEL
    fileAccessProperties = H5Pcreate(H5P_FILE_ACCESS);
    if (fileAccessProperties == H5I_INVALID_HID ||
      H5Pset_fapl_mpio(fileAccessProperties, MPI_COMM_WORLD, MPI_INFO_NULL) < 0 ||
      H5Pset_coll_metadata_write(fileAccessProperties, true) < 0)
    {
      vtkErrorWithObjectMacro(this->Writer, "Could not configure parallel HDF5 file access");
      return false;
    }
    fileAccessPropertiesId = fileAccessProperties;
#else
    vtkErrorWithObjectMacro(
      this->Writer, "Parallel HDF5 is required for distributed single-file output");
    return false;
#endif
  }

  vtkHDF::ScopedH5FHandle file{ H5Fcreate(filename.c_str(),
    overwrite ? H5F_ACC_TRUNC : H5F_ACC_EXCL, H5P_DEFAULT, fileAccessPropertiesId) };
  if (file == H5I_INVALID_HID)
  {
    return false;
  }

  // Create the root group
  vtkHDF::ScopedH5GHandle root = this->CreateHdfGroupWithLinkOrder(file, "VTKHDF");
  if (root == H5I_INVALID_HID)
  {
    return false;
  }

  this->File = std::move(file);
  this->Root = std::move(root);

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::OpenFile(const std::string& filename)
{
  vtkDebugWithObjectMacro(
    this->Writer, << "Opening file on rank" << this->Writer->CurrentPiece << ": " << filename);

  vtkHDF::ScopedH5PHandle fileAccessProperties;
  hid_t fileAccessPropertiesId = H5P_DEFAULT;
  if (this->GetUseDistributedWriting())
  {
#ifdef H5_HAVE_PARALLEL
    fileAccessProperties = H5Pcreate(H5P_FILE_ACCESS);
    if (fileAccessProperties == H5I_INVALID_HID ||
      H5Pset_fapl_mpio(fileAccessProperties, MPI_COMM_WORLD, MPI_INFO_NULL) < 0 ||
      H5Pset_coll_metadata_write(fileAccessProperties, true) < 0)
    {
      vtkErrorWithObjectMacro(this->Writer, "Could not configure parallel HDF5 file access");
      return false;
    }
    fileAccessPropertiesId = fileAccessProperties;
#else
    vtkErrorWithObjectMacro(
      this->Writer, "Parallel HDF5 is required for distributed single-file output");
    return false;
#endif
  }

  vtkHDF::ScopedH5FHandle file{ H5Fopen(filename.c_str(), H5F_ACC_RDWR, fileAccessPropertiesId) };
  if (file == H5I_INVALID_HID)
  {
    return false;
  }

  this->File = std::move(file);
  this->Root = this->OpenExistingGroup(this->File, "VTKHDF");

  return this->Root != H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
hid_t vtkHDFWriter::Implementation::GetTransferPropertyList()
{
  // Only the distributed single-file case requires collective I/O. In serial or external-file
  // mode, the default (independent) transfer is used.
  if (!this->GetUseDistributedWriting())
  {
    return H5P_DEFAULT;
  }

  if (this->TransferPropertyList == H5I_INVALID_HID)
  {
#ifdef H5_HAVE_PARALLEL
    this->TransferPropertyList = H5Pcreate(H5P_DATASET_XFER);
    if (this->TransferPropertyList == H5I_INVALID_HID ||
      H5Pset_dxpl_mpio(this->TransferPropertyList, H5FD_MPIO_COLLECTIVE) < 0)
    {
      vtkErrorWithObjectMacro(
        this->Writer, "Could not create collective data transfer property list");
      this->TransferPropertyList = H5I_INVALID_HID;
      return H5P_DEFAULT;
    }
#else
    vtkErrorWithObjectMacro(
      this->Writer, "Parallel HDF5 is required for distributed single-file output");
    return H5P_DEFAULT;
#endif
  }
  return this->TransferPropertyList;
}

//------------------------------------------------------------------------------
void vtkHDFWriter::Implementation::CloseFile()
{
  vtkDebugWithObjectMacro(this->Writer,
    "Closing current file " << this->File << " " << this->Writer->FileName << " on rank "
                            << this->Writer->CurrentPiece);

  // Setting to H5I_INVALID_HID closes the group/file using RAII
  this->StepsGroup = H5I_INVALID_HID;
  this->Root = H5I_INVALID_HID;
  this->File = H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::OpenSubfile(const std::string& filename)
{
  vtkDebugWithObjectMacro(
    this->Writer, << "Opening sub file on rank " << this->Writer->CurrentPiece << ": " << filename);

  vtkHDF::ScopedH5FHandle file{ H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT) };
  if (file == H5I_INVALID_HID)
  {
    return false;
  }

  this->Subfiles.emplace_back(std::move(file));
  this->SubfileNames.emplace_back(filename);

  return true;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::OpenExistingGroup(
  hid_t group, const char* name)
{
  vtkDebugWithObjectMacro(this->Writer, << "Opening group " << name);
  return H5Gopen(group, name, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::OpenDataset(hid_t group, const char* name)
{
  return H5Dopen(group, name, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
hsize_t vtkHDFWriter::Implementation::GetDataSetSize(hid_t group, const char* name)
{
  if (H5Lexists(group, name, H5P_DEFAULT) <= 0)
  {
    return 0;
  }

  vtkHDF::ScopedH5DHandle ds = this->OpenDataset(group, name);
  const auto dimensions = vtkHDFUtilities::GetDimensions(ds);
  return !dimensions.empty() ? dimensions[0] : 0;
}

//------------------------------------------------------------------------------
std::string vtkHDFWriter::Implementation::GetGroupName(hid_t group)
{
  size_t len = H5Iget_name(group, nullptr, 0);
  if (len == 0)
  {
    return std::string("");
  }
  std::string buffer;
  buffer.resize(len);
  H5Iget_name(group, &buffer.front(), len + 1);
  return buffer;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateStepsGroup(hid_t group)
{
  // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): static data is NUL-terminated
  if (H5Lexists(group, PATH::STEPS.data(), H5P_DEFAULT) <= 0)
  {
    vtkHDF::ScopedH5GHandle stepsGroup{ H5Gcreate(
      // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): static data is NUL-terminated
      group, PATH::STEPS.data(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
    return stepsGroup != H5I_INVALID_HID;
  }
  return this->StepsGroup != H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
hid_t vtkHDFWriter::Implementation::GetStepsGroup(hid_t currentGroup)
{
  // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): static data is NUL-terminated
  if (H5Lexists(currentGroup, PATH::STEPS.data(), H5P_DEFAULT))
  {
    // Store the last steps group accessed. There can be multiple for a given file
    // if it composite. This Steps group is only used internally.
    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): static data is NUL-terminated
    this->StepsGroup = H5Gopen(currentGroup, PATH::STEPS.data(), H5P_DEFAULT);
    return this->StepsGroup;
  }
  return H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::CreateSimpleDataspace(
  const std::vector<hsize_t>& dimensions)
{
  vtkHDF::ScopedH5SHandle dataspace{ H5Screate(H5S_SIMPLE) };
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  auto res = H5Sset_extent_simple(
    dataspace, static_cast<int>(dimensions.size()), dimensions.data(), dimensions.data());
  if (res < 0)
  {
    return H5I_INVALID_HID;
  }
  return dataspace;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5AHandle vtkHDFWriter::Implementation::CreateVectorAttribute(
  hid_t group, const char* name, hid_t type, hsize_t size, const void* data)
{
  if (H5Aexists(group, name))
  {
    return H5Aopen_name(group, name);
  }

  vtkHDF::ScopedH5SHandle vectorSpaceAttribute = H5Screate_simple(1, &size, nullptr);
  if (vectorSpaceAttribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  vtkHDF::ScopedH5AHandle attribute{ H5Acreate(
    group, name, type, vectorSpaceAttribute, H5P_DEFAULT, H5P_DEFAULT) };
  if (attribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  if (H5Awrite(attribute, type, data) < 0)
  {
    return H5I_INVALID_HID;
  }

  return attribute;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5AHandle vtkHDFWriter::Implementation::CreateScalarAttribute(
  hid_t group, const char* name, int value)
{
  if (H5Aexists(group, name))
  {
    return H5Aopen_name(group, name);
  }

  vtkHDF::ScopedH5SHandle scalarSpaceAttribute{ H5Screate(H5S_SCALAR) };
  if (scalarSpaceAttribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  vtkHDF::ScopedH5AHandle attribute{ H5Acreate(
    group, name, H5T_STD_I64LE, scalarSpaceAttribute, H5P_DEFAULT, H5P_DEFAULT) };
  if (attribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  if (H5Awrite(attribute, H5T_NATIVE_INT, &value) < 0)
  {
    return H5I_INVALID_HID;
  }

  return attribute;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5AHandle vtkHDFWriter::Implementation::CreateStringAttribute(
  hid_t group, const char* name, const std::string& value)
{
  if (H5Aexists(group, name))
  {
    return H5Aopen_name(group, name);
  }

  vtkHDF::ScopedH5SHandle scalarSpaceAttribute{ H5Screate(H5S_SCALAR) };
  if (scalarSpaceAttribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  vtkHDF::ScopedH5PHandle utf8PropertyList{ H5Pcreate(H5P_ATTRIBUTE_CREATE) };
  if (utf8PropertyList == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  if (H5Pset_char_encoding(utf8PropertyList, H5T_CSET_UTF8) < 0)
  {
    return H5I_INVALID_HID;
  }
  vtkHDF::ScopedH5THandle typeOfTypeAttr = H5Tcreate(H5T_STRING, value.length());
  if (typeOfTypeAttr == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  vtkHDF::ScopedH5AHandle attribute{ H5Acreate(
    group, name, typeOfTypeAttr, scalarSpaceAttribute, utf8PropertyList, H5P_DEFAULT) };
  if (attribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  if (H5Awrite(attribute, typeOfTypeAttr, value.c_str()) < 0)
  {
    return H5I_INVALID_HID;
  }

  return attribute;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::CreateHdfGroup(hid_t group, const char* name)
{
  if (H5Lexists(group, name, H5P_DEFAULT) <= 0)
  {
    return H5Gcreate(group, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
  return this->OpenExistingGroup(group, name);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::CreateHdfGroupWithLinkOrder(
  hid_t group, const char* name)
{
  if (H5Lexists(group, name, H5P_DEFAULT) <= 0)
  {
    vtkHDF::ScopedH5PHandle plist = H5Pcreate(H5P_GROUP_CREATE);
    if (plist == H5I_INVALID_HID)
    {
      return H5I_INVALID_HID;
    }
    H5Pset_link_creation_order(plist, H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED);
    return H5Gcreate(group, name, H5P_DEFAULT, plist, H5P_DEFAULT);
  }
  return this->OpenExistingGroup(group, name);
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateSoftLink(
  hid_t group, const char* groupName, const char* targetLink)
{
  if (H5Lexists(group, groupName, H5P_DEFAULT) <= 0)
  {
    return H5Lcreate_soft(targetLink, group, groupName, H5P_DEFAULT, H5P_DEFAULT) >= 0;
  }
  return true; // Ignore if already exists
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateExternalLink(
  hid_t group, const char* filename, const char* source, const char* targetLink)
{
  if (H5Lexists(group, targetLink, H5P_DEFAULT) <= 0)
  {
    return H5Lcreate_external(filename, source, group, targetLink, H5P_DEFAULT, H5P_DEFAULT) >= 0;
  }
  return true;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateHdfDataset(
  hid_t group, const char* name, hid_t type, hid_t dataspace)
{
  return H5Dcreate(group, name, type, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateHdfDataset(
  hid_t group, const char* name, hid_t type, const std::vector<hsize_t>& dimensions)
{
  vtkHDF::ScopedH5SHandle dataspace = this->CreateSimpleDataspace(dimensions);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  return this->CreateHdfDataset(group, name, type, dataspace);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::CreateDataspaceFromArray(
  vtkAbstractArray* dataArray, const std::vector<hsize_t>& dims)
{
  const int nComp = dataArray ? dataArray->GetNumberOfComponents() : 0;
  std::vector<hsize_t> dspaceDims;
  if (dims.empty())
  {
    const hsize_t nTuples = dataArray ? dataArray->GetNumberOfTuples() : 0;
    dspaceDims.emplace_back(nTuples);
  }
  else
  {
    dspaceDims = dims;
  }

  if (nComp > 1)
  {
    dspaceDims.emplace_back(static_cast<hsize_t>(nComp));
  }

  return this->CreateSimpleDataspace(dspaceDims);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateHdfDatasetFromDataArray(hid_t group,
  const char* name, hid_t type, vtkAbstractArray* dataArray, const std::vector<hsize_t>& dims)
{
  // Create dataspace from array
  vtkHDF::ScopedH5SHandle dataspace = this->CreateDataspaceFromArray(dataArray, dims);
  if (dataspace == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Writer, "Could not create dataspace for array " << name);
    return H5I_INVALID_HID;
  }
  // Create dataset from dataspace and other arguments
  vtkHDF::ScopedH5DHandle dataset = this->CreateHdfDataset(group, name, type, dataspace);
  if (dataset == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Writer, "Could not create Dataset");
    return H5I_INVALID_HID;
  }

  auto da = vtkDataArray::FastDownCast(dataArray);
  // If it is not a vtkDataArray, return either an invalid id or the dataset depending on the number
  // of values in the dataArray
  if (da == nullptr)
  {
    if (dataArray->GetNumberOfValues() == 0)
    {
      return dataset;
    }
    else
    {
      vtkErrorWithObjectMacro(this->Writer, "Dataset " << name << " is null");
      return H5I_INVALID_HID;
    }
  }
  // Find which HDF type corresponds to the dataArray type
  // It is different from the `type` in the argument list which defines which type should be used to
  // store the data in the HDF file
  hid_t source_type = vtkHDFUtilities::getH5TypeFromVtkType(dataArray->GetDataType());
  if (source_type == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Writer, "Source type " << source_type << " is invalid");
    return H5I_INVALID_HID;
  }
  // Write vtkAbstractArray data to the HDF dataset
  auto aos = da->ToAOSDataArray();
  if (H5Dwrite(dataset, source_type, H5S_ALL, dataspace, this->GetTransferPropertyList(),
        aos->GetVoidPointer(0)) < 0) // NOLINT(bugprone-unsafe-functions)
  {
    vtkErrorWithObjectMacro(this->Writer, "Could not write dataset " << name);
    return H5I_INVALID_HID;
  }
  return dataset;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::AllocateDataset(
  hid_t group, const char* name, std::vector<hsize_t> dimensions)
{
  vtkHDF::ScopedH5DHandle dataset = this->OpenDataset(group, name);
  if (dataset == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  const auto current = vtkHDFUtilities::GetDimensions(dataset);
  if (current.empty())
  {
    return H5I_INVALID_HID;
  }
  const std::string path = this->GetGroupName(group) + "/" + name;
  hsize_t rows = current[0] + dimensions[0];
  if (dimensions.size() <= 2 && this->UsesDistributedOffsets(path))
  {
    const hsize_t scale = PATH::ContainsAny(path, { PATH::FIELD_DATA }) ? dimensions[0] : 1;
    rows = this->GetDistributedDatasetSize(path) * scale;
  }
  dimensions[0] = rows;
  vtkDebugWithObjectMacro(this->Writer, "Resizing dataset " << name << " to " << rows << " rows");
  if (H5Dset_extent(dataset, dimensions.data()) < 0)
  {
    return H5I_INVALID_HID;
  }
  return dataset;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateAndAllocateDataSet(hid_t group,
  const char* name, hid_t type, unsigned int partId, const std::vector<hsize_t>& dimensions,
  const std::vector<hsize_t>& chunkSize, hsize_t& rowOffset, bool trim)
{
  auto initialDimensions = dimensions;
  initialDimensions[0] = 0;
  if (!this->InitDynamicDataset(group, name, type, initialDimensions, chunkSize))
  {
    return H5I_INVALID_HID;
  }
  auto dataset =
    trim ? this->OpenDataset(group, name) : this->AllocateDataset(group, name, dimensions);
  if (dataset == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  rowOffset = this->GetDatasetWriteOffset(dataset, partId, dimensions[0], trim);
  if (rowOffset == H5S_UNLIMITED)
  {
    return H5I_INVALID_HID;
  }
  return dataset;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::UsesDistributedOffsets(const std::string& path)
{
  const auto& parts = this->UgOffsets.PartOffsets;
  if (!this->GetUseDistributedWriting() || PATH::ContainsAny(path, { PATH::STEPS }) ||
    parts.empty() || parts.back() == parts.front())
  {
    return false;
  }
  const auto mode = this->GetDatasetIndexationMode(path);
  return mode == IndexingMode::Points || mode == IndexingMode::Cells ||
    mode == IndexingMode::Connectivity || mode == IndexingMode::MetaData;
}

//------------------------------------------------------------------------------
hsize_t vtkHDFWriter::Implementation::GetDatasetWriteOffset(
  hid_t dataset, unsigned int partId, hsize_t localRows, bool trim)
{
  const std::string path = this->GetGroupName(dataset);
  const auto extent = vtkHDFUtilities::GetDimensions(dataset);
  if (extent.empty())
  {
    return H5S_UNLIMITED;
  }
  if (trim || extent.size() > 2 || !this->UsesDistributedOffsets(path))
  {
    const hsize_t rows = trim ? 1 : localRows;
    return extent[0] >= rows ? extent[0] - rows : H5S_UNLIMITED;
  }
  const auto& parts = this->UgOffsets.PartOffsets;
  const int rank = this->Writer->CurrentPiece;
  if (rank < 0 || static_cast<std::size_t>(rank) + 1 >= parts.size() ||
    static_cast<vtkIdType>(partId) >= parts[rank + 1] - parts[rank])
  {
    return H5S_UNLIMITED;
  }
  const auto partition = parts[rank] - parts.front() + partId;
  return this->GetDatasetValueOffset(partition, path, this->GetDatasetIndexationMode(path));
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteDataset(hid_t dataset, hid_t memoryType,
  const std::vector<hsize_t>& start, const std::vector<hsize_t>& count, const void* data)
{
  vtkHDF::ScopedH5SHandle fileSpace = H5Dget_space(dataset);
  if (fileSpace == H5I_INVALID_HID || start.size() != count.size() ||
    static_cast<int>(count.size()) != H5Sget_simple_extent_ndims(fileSpace))
  {
    return false;
  }
  const bool empty = std::find(count.begin(), count.end(), hsize_t{ 0 }) != count.end();
  vtkHDF::ScopedH5SHandle memorySpace = empty
    ? H5Screate(H5S_NULL)
    : H5Screate_simple(static_cast<int>(count.size()), count.data(), nullptr);
  if (memorySpace == H5I_INVALID_HID)
  {
    return false;
  }
  if (empty ? H5Sselect_none(fileSpace) < 0
            : H5Sselect_hyperslab(
                fileSpace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr) < 0)
  {
    return false;
  }
  return H5Dwrite(dataset, memoryType, memorySpace, fileSpace, this->GetTransferPropertyList(),
           empty ? nullptr : data) >= 0;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteOneRowDataset(
  hid_t dataset, const std::vector<vtkIdType>& values, hsize_t rowOffset, bool offset, bool trim)
{
  vtkDebugWithObjectMacro(this->Writer,
    "Writing 1 row to " << this->GetGroupName(dataset) << " at row " << rowOffset
                        << (trim ? " with trimming" : ""));
  if (values.empty())
  {
    return false;
  }
  const hsize_t rowSize = values.size();
  const auto dimensions = vtkHDFUtilities::GetDimensions(dataset);
  const int rank = static_cast<int>(dimensions.size());
  std::vector<hsize_t> startPos(rank, 0);
  std::vector<hsize_t> count(rank, 1);
  if (rank == 2)
  {
    count[1] = rowSize;
  }
  std::vector<vtkIdType> addedValues(values);
  if (offset && (trim || rowOffset > 0))
  {
    vtkHDF::ScopedH5SHandle newDataspace = H5Screate_simple(1, &rowSize, nullptr);
    vtkHDF::ScopedH5SHandle currentDataspace = H5Dget_space(dataset);
    if (newDataspace == H5I_INVALID_HID || currentDataspace == H5I_INVALID_HID)
    {
      return false;
    }
    startPos[0] = trim ? rowOffset : rowOffset - 1;
    std::vector<vtkIdType> previousValues(rowSize);
    if (H5Sselect_hyperslab(
          currentDataspace, H5S_SELECT_SET, startPos.data(), nullptr, count.data(), nullptr) < 0 ||
      H5Dread(dataset, H5T_STD_I64LE, newDataspace, currentDataspace, H5P_DEFAULT,
        previousValues.data()) < 0)
    {
      return false;
    }
    for (hsize_t i = 0; i < rowSize; ++i)
    {
      addedValues[i] += previousValues[i];
    }
  }
  startPos[0] = rowOffset;
  return this->WriteDataset(dataset, H5T_STD_I64LE, startPos, count, addedValues.data());
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddOrCreateSingleRowDataset(hid_t group, unsigned int partId,
  const char* name, const std::vector<vtkIdType>& values, bool offset, bool trim)
{
  vtkDebugWithObjectMacro(this->Writer, "Preparing " << name << " for partition " << partId);
  // Assume that when subfiles are set, we don't need to write data unless
  // SubFilesReady is set, which means all subfiles have been written.
  if (!this->Subfiles.empty() && (group != this->StepsGroup || this->Writer->NbPieces > 1))
  {
    if (this->SubFilesReady)
    {
      return this->CreateVirtualDataset(
        group, name, H5T_STD_I64LE, static_cast<int>(values.size()));
    }
    return true;
  }

  std::vector<hsize_t> small_chunk = { SMALL_CHUNK, values.size() };
  std::vector<hsize_t> dimensions{ 1 };
  if (values.size() > 1)
  {
    dimensions.push_back(values.size());
  }
  hsize_t rowOffset = 0;
  auto dataset = this->CreateAndAllocateDataSet(
    group, name, H5T_STD_I64LE, partId, dimensions, small_chunk, rowOffset, trim);
  if (dataset == H5I_INVALID_HID)
  {
    return false;
  }
  return this->WriteOneRowDataset(dataset, values, rowOffset, offset, trim);
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddArrayToDataset(
  hid_t dataset, vtkAbstractArray* dataArray, hsize_t rowOffset, const std::vector<hsize_t>& dims)
{
  if (dataArray == nullptr)
  {
    return true;
  }

  if (dataset == -1)
  {
    return false;
  }

  hid_t source_type = vtkHDFUtilities::getH5TypeFromVtkType(dataArray->GetDataType());
  if (source_type == H5I_INVALID_HID)
  {
    return false;
  }

  const bool isStructured = !dims.empty();

  // Determine dimensions of the data to be added
  const int nComp = dataArray->GetNumberOfComponents();
  std::vector<hsize_t> addedDims;
  if (!isStructured)
  {
    const vtkIdType nTuples = dataArray->GetNumberOfTuples();
    addedDims.emplace_back(static_cast<hsize_t>(nTuples));
  }
  else
  {
    addedDims = dims;
  }
  if (nComp > 1)
  {
    addedDims.emplace_back(static_cast<hsize_t>(nComp));
  }
  const int rank = static_cast<int>(addedDims.size()) + (isStructured ? 1 : 0);

  // Debug output
  std::ostringstream addedDimsStr;
  addedDimsStr << addedDims[0];
  for (std::size_t i = 1; i < addedDims.size(); ++i)
  {
    addedDimsStr << ", " << addedDims[i];
  }
  vtkDebugWithObjectMacro(this->Writer, << "Adding dims [" << addedDimsStr.str() << "] with "
                                        << nComp << " components"
                                        << " at row " << rowOffset);

  // Retrieve current dataspace dimensions
  const auto currentDims = vtkHDFUtilities::GetDimensions(dataset);
  if (currentDims.size() != static_cast<std::size_t>(rank))
  {
    return false;
  }
  for (int i = 1; i < rank; ++i)
  {
    const int j = isStructured ? (i - 1) : i;
    if (currentDims[i] != addedDims[j])
    {
      vtkErrorWithObjectMacro(this->Writer, << "Number of dimensions or components mismatched: "
                                            << currentDims[i] << " != " << addedDims[j]);
      return false;
    }
  }

  const hsize_t rows = isStructured ? 1 : addedDims[0];
  if (rowOffset > currentDims[0] || rows > currentDims[0] - rowOffset)
  {
    return false;
  }

  std::vector<hsize_t> start(rank, 0);
  start[0] = rowOffset;
  std::vector<hsize_t> count;
  if (!isStructured)
  {
    count = addedDims;
  }
  else
  {
    count = { 1 };
    count.insert(count.end(), addedDims.begin(), addedDims.end());
  }

  if (auto sa = vtkStringArray::SafeDownCast(dataArray))
  {
    vtkHDF::ScopedH5THandle datatype = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype, H5T_VARIABLE);

    std::vector<const char*> strArray;
    strArray.resize(sa->GetNumberOfValues());
    for (vtkIdType i = 0; i < sa->GetNumberOfValues(); i++)
    {
      strArray[i] = sa->GetPointer(i)->c_str();
    }

    // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
    return this->WriteDataset(dataset, datatype, start, count, strArray.data());
  }
  else if (auto da = vtkDataArray::SafeDownCast(dataArray))
  {
    auto aos = da->ToAOSDataArray();
    // NOLINTNEXTLINE(bugprone-unsafe-functions)
    const void* data = dataArray->GetNumberOfValues() == 0 ? nullptr : aos->GetVoidPointer(0);
    return this->WriteDataset(dataset, source_type, start, count, data);
  }
  else
  {
    vtkErrorWithObjectMacro(this->Writer, << "Unsupported array type" << dataArray->GetClassName());
    return false;
  }
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddOrCreateDataset(hid_t group, unsigned int partId,
  const char* name, hid_t type, vtkAbstractArray* dataArray, const std::vector<hsize_t>& dims)
{
  vtkDebugWithObjectMacro(this->Writer, "Preparing " << name << " for partition " << partId);
  if (!dataArray)
  {
    vtkWarningWithObjectMacro(this->Writer, "Skipping null array " << name);
    return true;
  }

  if (!this->Subfiles.empty())
  {
    if (this->SubFilesReady)
    {
      return this->CreateVirtualDataset(group, name, type, dataArray->GetNumberOfComponents());
    }
    return true;
  }

  if (!dims.empty() && H5Lexists(group, name, H5P_DEFAULT) <= 0)
  {
    // Dataset needs to be created
    return this->CreateHdfDatasetFromDataArray(group, name, type, dataArray, dims) !=
      H5I_INVALID_HID;
  }
  std::vector<hsize_t> dimensions;
  if (dims.empty())
  {
    dimensions.push_back(dataArray->GetNumberOfTuples());
  }
  else
  {
    dimensions.push_back(1);
    dimensions.insert(dimensions.end(), dims.begin(), dims.end());
  }
  if (dataArray->GetNumberOfComponents() > 1)
  {
    dimensions.push_back(dataArray->GetNumberOfComponents());
  }
  const std::vector<hsize_t> chunkSize = {
    this->GetUseDistributedWriting() ? static_cast<hsize_t>(this->ChunkSize)
                                     : std::clamp<hsize_t>(dataArray->GetNumberOfTuples(),
                                         std::min(SMALL_CHUNK, this->ChunkSize), this->ChunkSize),
    static_cast<hsize_t>(dataArray->GetNumberOfComponents()),
  };
  hsize_t rowOffset = 0;
  auto prepared =
    this->CreateAndAllocateDataSet(group, name, type, partId, dimensions, chunkSize, rowOffset);
  if (prepared == H5I_INVALID_HID)
  {
    return false;
  }
  return this->AddArrayToDataset(prepared, dataArray, rowOffset, dims);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5THandle vtkHDFWriter::Implementation::GetSubfileDataType(
  const std::string& datasetPath)
{
  for (auto& subfile : this->Subfiles)
  {
    if (this->DatasetAndGroupExist(datasetPath, subfile))
    {
      vtkHDF::ScopedH5DHandle source = H5Dopen(subfile, datasetPath.c_str(), H5P_DEFAULT);
      if (source == H5I_INVALID_HID)
      {
        return H5I_INVALID_HID;
      }
      return H5Dget_type(source);
    }
  }
  return H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateVirtualDataset(
  hid_t group, const char* name, hid_t type, int numComp)
{
  if (group == this->StepsGroup)
  {
    return this->WriteSumSteps(group, name);
  }

  if (H5Lexists(group, name, H5P_DEFAULT))
  {
    // Virtual dataset has already been created
    return true;
  }

  vtkDebugWithObjectMacro(
    this->Writer, "Creating virtual dataset " << name << " in " << this->GetGroupName(group));

  // Initialize Virtual Dataset property
  vtkHDF::ScopedH5PHandle virtualSourceP = H5Pcreate(H5P_DATASET_CREATE);
  if (virtualSourceP == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(
      this->Writer, << "Could not create virtual source property for " << name);
    return false;
  }

  // Collect total dataset size
  const std::string groupPath = this->GetGroupName(group);
  const std::string datasetPath = groupPath + "/" + name;
  hsize_t totalSize = 0;
  if (!this->GetSubFilesDatasetSize(datasetPath, groupPath, totalSize))
  {
    vtkDebugWithObjectMacro(
      this->Writer, << "Ignoring dataset " << datasetPath << " not present in every sub-file.");
    if (groupPath.find(PATH::CELL_DATA) != std::string::npos ||
      groupPath.find(PATH::POINT_DATA) != std::string::npos)
    {
      return group; // Partial field, no error
    }
    return false;
  }

  // Copy stored data type
  auto sourceType = this->GetSubfileDataType(datasetPath);
  if (sourceType == H5I_INVALID_HID)
  {
    return false;
  }
  type = sourceType;

  hsize_t numberOfSteps = 1;
  if (this->Writer->IsTemporal && this->Writer->NbPieces != 1)
  {
    numberOfSteps = this->Writer->NumberOfTimeSteps;
  }

  vtkDebugWithObjectMacro(
    this->Writer, "Total Virtual Dataset Size: " << totalSize << "x" << numComp);

  // Create destination dataspace with the final size
  std::vector<hsize_t> dspaceDims{ totalSize };
  const int numDim = numComp == 1 ? 1 : 2;
  if (numDim == 2)
  {
    dspaceDims.emplace_back(numComp);
  }
  vtkHDF::ScopedH5SHandle destSpace = H5Screate_simple(numDim, dspaceDims.data(), nullptr);
  if (destSpace == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(
      this->Writer, << "Could not create virtual destination dataspace for " << name);
    return false;
  }

  // Find if dataset is indexed on points, cells, or connectivity, or is a meta-data array
  IndexingMode indexMode = this->GetDatasetIndexationMode(datasetPath);
  std::string basePath = PATH::GetBasePath(groupPath);

  // Find primitive type for PolyData
  bool isPolyData = H5Lexists(this->OpenExistingGroup(this->Root, basePath.c_str()),
                      // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): NUL-terminated
                      PATH::PRIMITIVE_TYPES.begin()->data(), H5P_DEFAULT) > 0;

  // Keep track of offsets in the destination, and in each of the source datasets.
  hsize_t mappingOffset = 0;
  std::vector<hsize_t> sourceOffsets(this->Subfiles.size(), 0);

  // Build virtual dataset mappings from sub-files, based on time steps and parts
  for (hsize_t step = 0; step < numberOfSteps; step++)
  {
    for (hsize_t file = 0; file < this->Subfiles.size(); file++)
    {
      std::stringstream ss;
      ss << " for file " << file << " for step " << step << " for group " << name;
      const std::string debugString = ss.str();

      // Determine the number of parts in the file for the current step
      // Using either NumberOfParts (when temporal) or the size of NumberOfPoints otherwise.
      std::string numPointsName = basePath + "/" + std::string(PATH::NUMBER_OF_POINTS);
      std::string numCellsName = basePath + "/" + std::string(PATH::NUMBER_OF_CELLS);
      bool hasStepsGroup =
        H5Lexists(this->OpenExistingGroup(this->Subfiles[file], basePath.c_str()),
          // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): NUL-terminated
          PATH::STEPS.data(), H5P_DEFAULT) > 0;
      if (!hasStepsGroup && step > 0)
      {
        continue;
      }

      hsize_t numParts = 1;
      if (hasStepsGroup)
      {
        numParts = std::max<hsize_t>(
          numParts, this->GetSubfileNumberOf(basePath, PATH::STEPS_NUMBER_OF_PARTS, file, step));
      }
      else if (H5Lexists(this->Subfiles[file], numPointsName.c_str(), H5P_DEFAULT) > 0)
      {
        numParts = vtkHDFUtilities::GetDimensions(this->Subfiles[file], numPointsName.c_str())[0];
      }
      else if (H5Lexists(this->Subfiles[file], numCellsName.c_str(), H5P_DEFAULT) > 0)
      {
        // HTG does not have points, so use numCells instead
        numParts = vtkHDFUtilities::GetDimensions(this->Subfiles[file], numCellsName.c_str())[0];
      }
      else
      {
        vtkDebugWithObjectMacro(this->Writer, << "Parts not found, skipping" << debugString);
        continue;
      }

      vtkDebugWithObjectMacro(this->Writer, << "Using " << numParts << " parts" << debugString);

      hsize_t partOffset = 0;
      if (hasStepsGroup)
      {
        partOffset = this->GetSubfileNumberOf(basePath, PATH::STEPS_PART_OFFSETS, file, step);
      }

      // Handle static mesh
      bool isStaticMesh = false;
      if (step > 0)
      {
        hsize_t currentPartOffset =
          this->GetSubfileNumberOf(basePath, PATH::STEPS_PART_OFFSETS, file, step);
        hsize_t previousPartOffset =
          this->GetSubfileNumberOf(basePath, PATH::STEPS_PART_OFFSETS, file, step - 1);
        vtkDebugWithObjectMacro(this->Writer, << "Current Part offset:  " << currentPartOffset
                                              << " Previous part offset: " << previousPartOffset);
        isStaticMesh = currentPartOffset == previousPartOffset;
      }

      for (hsize_t part = 0; part < numParts; part++)
      {
        // Skip datasets not present everywhere
        if (!this->DatasetAndGroupExist(datasetPath, this->Subfiles[file]))
        {
          if (name == PATH::OFFSETS)
          {
            mappingOffset++; // Offset by 1 the next offset, because even for 0 cells, we need 1
                             // offset value.
            vtkDebugWithObjectMacro(
              this->Writer, "Adding 1 to mapping offset for Offsets : " << mappingOffset);
          }
          continue;
        }

        // Open the source dataset/dataspace
        vtkHDF::ScopedH5DHandle sourceDataset =
          H5Dopen(this->Subfiles[file], datasetPath.c_str(), H5P_DEFAULT);
        if (sourceDataset == H5I_INVALID_HID)
        {
          vtkErrorWithObjectMacro(this->Writer, << "Could not find source dataset " << debugString);
          return false;
        }
        vtkHDF::ScopedH5SHandle sourceDataSpace = H5Dget_space(sourceDataset);
        if (sourceDataSpace == H5I_INVALID_HID)
        {
          vtkErrorWithObjectMacro(
            this->Writer, << "Could not find source dataspace " << debugString);
          return false;
        }
        auto sourceDims = vtkHDFUtilities::GetDimensions(sourceDataset);
        sourceDims.resize(std::max<std::size_t>(3, sourceDims.size()), 0);

        vtkDebugWithObjectMacro(this->Writer,
          << "Extent source " << sourceDims[0] << " " << sourceDims[1] << " " << sourceDims[2]);

        std::vector<hsize_t> mappingSize{
          sourceDims[0]
        }; // By default, select the whole source dataset

        // Correctly set mappingSize given the type of dataset&
        switch (indexMode)
        {
          case IndexingMode::MetaData:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on metadata");

            // Handle static mesh
            if (isStaticMesh)
            {
              vtkDebugWithObjectMacro(
                this->Writer, << "Static mesh, not writing count value again");
              continue;
            }

            // Select only one value in the source dataspace
            mappingSize[0] = 1;

            break;
          }
          case IndexingMode::Points:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on points");

            hsize_t nbPointsPart =
              this->GetSubfileNumberOf(basePath, PATH::NUMBER_OF_POINTS, file, part + partOffset);

            // Handle static mesh
            if (name == PATH::POINTS && isStaticMesh)
            {
              vtkDebugWithObjectMacro(
                this->Writer, << "Static mesh, not writing points virtual dataset again");
              continue;
            }

            mappingSize[0] = nbPointsPart;

            break;
          }
          case IndexingMode::Cells:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on cells");
            hsize_t partNbCells = this->GetNumberOfCellsSubfile(
              basePath, file, part + partOffset, isPolyData, this->GetGroupName(group));

            // Handle static mesh: don't write offsets if cells have not changed
            if ((name == PATH::OFFSETS || name == PATH::TYPES) && isStaticMesh)
            {
              vtkDebugWithObjectMacro(
                this->Writer, << "Static mesh, not writing virtual offsets/types again");
              continue;
            }

            mappingSize[0] = partNbCells;

            // For N cells, store N+1 cell offsets
            if (name == PATH::OFFSETS && partNbCells != 0)
            {
              mappingSize[0]++;
            }

            // Find the number of bytes corresponding to the mask size
            if (name == PATH::MASK)
            {
              mappingSize[0] = (mappingSize[0] + (8 - mappingSize[0]) % 8) / 8;
            }

            vtkDebugWithObjectMacro(this->Writer, << "Mapping size is " << mappingSize[0]);
            break;
          }
          case IndexingMode::Connectivity:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on connectivity");

            hsize_t nbConnectivityIdPart = this->GetSubfileNumberOf(
              groupPath, PATH::NUMBER_OF_CONNECTIVITY_IDS, file, part + partOffset);

            // Handle static mesh
            if (name == PATH::CONNECTIVITY && isStaticMesh)
            {
              vtkDebugWithObjectMacro(
                this->Writer, << "Static mesh, not writing virtual connectivity Ids again");
              continue;
            }

            mappingSize[0] = nbConnectivityIdPart;
            break;
          }
          case IndexingMode::Trees:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on trees");
            hsize_t partNbTrees =
              this->GetSubfileNumberOf(basePath, PATH::NUMBER_OF_TREES, file, part + partOffset);
            mappingSize[0] = partNbTrees;
            break;
          }
          case IndexingMode::TreeDepths:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on tree depths");
            hsize_t partNbDepths =
              this->GetSubfileNumberOf(basePath, PATH::NUMBER_OF_DEPTHS, file, part + partOffset);
            mappingSize[0] = partNbDepths;
            break;
          }
          case IndexingMode::Descriptor:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on descriptor sizes");
            hsize_t partDescrSize =
              this->GetSubfileNumberOf(basePath, PATH::DESCRIPTORS_SIZE, file, part + partOffset);
            mappingSize[0] = (partDescrSize + (8 - partDescrSize) % 8) / 8;
            break;
          }
          case IndexingMode::HTGCoords:
          {
            vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on HTG Coords");
            std::vector<unsigned int> dimensions(3);
            vtkHDFUtilities::GetAttribute(
              this->OpenExistingGroup(this->Subfiles[file], basePath.c_str()), "Dimensions",
              dimensions.size(), dimensions.data());
            if (name == PATH::XCOORDS)
            {
              mappingSize[0] = dimensions[0];
            }
            else if (name == PATH::YCOORDS)
            {
              mappingSize[0] = dimensions[1];
            }
            else if (name == PATH::ZCOORDS)
            {
              mappingSize[0] = dimensions[2];
            }
            break;
          }
          default:
            vtkErrorWithObjectMacro(this->Writer, "Unknown indexing mode for " << datasetPath);
            break;
        }

        if (mappingSize[0] == 0)
        {
          continue;
        }

        // Select hyperslab in source space of size 1
        std::vector<hsize_t> sourceOffset{ sourceOffsets[file] };

        if (numDim == 2)
        {
          sourceOffset.emplace_back(0);
          mappingSize.emplace_back(sourceDims[1]); // All components
        }

        // Select hyperslab in destination space
        std::vector<hsize_t> destinationOffset{ mappingOffset };
        if (numDim == 2)
        {
          destinationOffset.emplace_back(0);
        }
        if (H5Sselect_hyperslab(destSpace, H5S_SELECT_SET, destinationOffset.data(), nullptr,
              mappingSize.data(), nullptr) < 0)
        {
          return false;
        }

        vtkDebugWithObjectMacro(this->Writer,
          << "Build mapping of " << name << " from [" << sourceOffset[0] << "+" << mappingSize[0]
          << "] to [" << destinationOffset[0] << "+" << mappingSize[0] << "]");

        // Create mapping H5S and select Hyperslab
        vtkHDF::ScopedH5SHandle mappedDataSpace =
          H5Screate_simple(numDim, mappingSize.data(), nullptr);
        if (mappedDataSpace == H5I_INVALID_HID)
        {
          return false;
        }
        if (H5Sselect_hyperslab(mappedDataSpace, H5S_SELECT_SET, sourceOffset.data(), nullptr,
              mappingSize.data(), nullptr) < 0)
        {
          return false;
        }

        // Build the mapping
        if (H5Pset_virtual(virtualSourceP, destSpace, this->SubfileNames[file].c_str(),
              datasetPath.c_str(), mappedDataSpace) < 0)
        {
          return false;
        }

        mappingOffset += mappingSize[0];
        sourceOffsets[file] += mappingSize[0];
      }
    }
  }

  // Create the virtual dataset using all the mappings
  vtkHDF::ScopedH5DHandle vdset =
    H5Dcreate(group, name, type, destSpace, H5P_DEFAULT, virtualSourceP, H5P_DEFAULT);
  return vdset != H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
hsize_t vtkHDFWriter::Implementation::GetNumberOfCellsSubfile(const std::string& basePath,
  std::size_t subfileId, hsize_t part, bool isPolyData, const std::string& groupName)
{
  if (!isPolyData)
  {
    return this->GetSubfileNumberOf(basePath, PATH::NUMBER_OF_CELLS, subfileId, part);
  }

  if (isPolyData && groupName.find(PATH::CELL_DATA) != std::string::npos)
  {
    // Sum up the number of cells for each primitive type
    return std::accumulate(PATH::PRIMITIVE_TYPES.begin(), PATH::PRIMITIVE_TYPES.end(),
      vtkIdType{ 0 },
      [this, subfileId, part, basePath](vtkIdType sum, std::string_view prim)
      {
        return sum +
          this->GetSubfileNumberOf(basePath,
            std::string(prim) + "/" + std::string(PATH::NUMBER_OF_CELLS), subfileId, part);
      });
  }
  else
  {
    return this->GetSubfileNumberOf(groupName, PATH::NUMBER_OF_CELLS, subfileId, part);
  }
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteSumSteps(hid_t group, const char* name)
{
  vtkDebugWithObjectMacro(this->Writer,
    "Creating steps sum " << name << " in " << this->GetGroupName(group) << "with base "
                          << PATH::GetBasePath(this->GetGroupName(group)));

  if (!this->InitDynamicDataset(group, name, H5T_STD_I64LE, 1, { SMALL_CHUNK }))
  {
    return false;
  }
  vtkHDF::ScopedH5DHandle dataset = this->OpenDataset(group, name);
  if (dataset == H5I_INVALID_HID)
  {
    return false;
  }

  // For each timestep, collect the sum of values in subfiles,
  // and append it to the meta-file array.
  const hsize_t firstRow = this->GetDataSetSize(group, name);
  const hsize_t rows = firstRow + this->Writer->NumberOfTimeSteps;
  if (H5Dset_extent(dataset, &rows) < 0)
  {
    return false;
  }
  for (int step = 0; step < this->Writer->NumberOfTimeSteps; step++)
  {
    vtkIdType totalForTimeStep = 0;
    for (std::size_t file = 0; file < this->Subfiles.size(); file++)
    {
      totalForTimeStep += this->GetSubfileNumberOf(PATH::GetBasePath(this->GetGroupName(group)),
        std::string(PATH::STEPS) + "/" + name, file, step);
    }

    if (!this->WriteOneRowDataset(dataset, { totalForTimeStep }, firstRow + step, false, false))
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteSumStepsPolyData(hid_t group, const char* name)
{
  vtkDebugWithObjectMacro(
    this->Writer, "Creating polydata steps sum " << name << " in " << this->GetGroupName(group));

  const std::vector<hsize_t> primitive_chunk = { SMALL_CHUNK,
    vtkHDFUtilities::NUM_POLY_DATA_TOPOS };
  this->InitDynamicDataset(
    group, name, H5T_STD_I64LE, vtkHDFUtilities::NUM_POLY_DATA_TOPOS, primitive_chunk);

  vtkHDF::ScopedH5DHandle dataset = this->OpenDataset(group, name);
  if (dataset == H5I_INVALID_HID)
  {
    return false;
  }

  // Create VTK Array of size nbPrimitives * nbTimeSteps
  vtkNew<vtkIdTypeArray> totalsArray;
  totalsArray->SetNumberOfComponents(vtkHDFUtilities::NUM_POLY_DATA_TOPOS);
  totalsArray->SetNumberOfTuples(this->Writer->NumberOfTimeSteps);

  // For each timestep, sum each primitive from all pieces
  for (int step = 0; step < this->Writer->NumberOfTimeSteps; step++)
  {
    totalsArray->SetTuple4(step, 0, 0, 0, 0);
    vtkDebugWithObjectMacro(this->Writer, "timestep " << step);
    for (int prim = 0; prim < static_cast<int>(vtkHDFUtilities::NUM_POLY_DATA_TOPOS); prim++)
    {
      vtkDebugWithObjectMacro(this->Writer, "primitive " << prim);
      // Collect size for the current time step in each subfile for each primitive
      for (std::size_t part = 0; part < this->Subfiles.size(); part++)
      {
        auto current = totalsArray->GetComponent(step, prim);
        vtkDebugWithObjectMacro(this->Writer, "part " << part << " value " << current);
        totalsArray->SetComponent(step, prim,
          current + this->GetSubfileNumberOf(this->GetGroupName(group), name, part, step, prim));
      }
    }
  }

  const hsize_t firstRow = this->GetDataSetSize(group, name);
  const hsize_t extent[] = { firstRow + static_cast<hsize_t>(totalsArray->GetNumberOfTuples()),
    static_cast<hsize_t>(totalsArray->GetNumberOfComponents()) };
  if (H5Dset_extent(dataset, extent) < 0 ||
    !this->AddArrayToDataset(dataset, totalsArray, firstRow))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
hsize_t vtkHDFWriter::Implementation::GetSubfileNumberOf(const std::string& base,
  const std::string_view& qualifier, std::size_t subfileId, hsize_t part, char primitive)
{
  const std::string fullPath = base + "/" + std::string(qualifier);
  bool isPolyData = H5Lexists(this->OpenExistingGroup(this->Root, PATH::GetBasePath(base).c_str()),
                      "Lines", H5P_DEFAULT) > 0;
  std::stringstream ss;
  ss << fullPath << " for subfile " << subfileId << " for part " << part << " with primitive "
     << static_cast<int>(primitive);
  const std::string debugInfo = ss.str();

  vtkDebugWithObjectMacro(this->Writer, << "Fetching " << debugInfo);

  if (!this->DatasetAndGroupExist(fullPath, this->Subfiles[subfileId]))
  {
    // In composite structure, not all subfiles may have a valid block
    return 0;
  }
  vtkHDF::ScopedH5DHandle sourceDataset =
    H5Dopen(this->Subfiles[subfileId], fullPath.c_str(), H5P_DEFAULT);
  if (sourceDataset == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Writer, << "Could not open dataset " << debugInfo);
    return 0;
  }

  std::vector<hsize_t> start{ static_cast<unsigned long>(part) }, count{ 1 }, result{ 0 };
  int dimension = 1;
  constexpr char INVALID_PRIMITIVE = 0xff;

  if (isPolyData && primitive != INVALID_PRIMITIVE)
  {
    start.emplace_back(static_cast<hsize_t>(primitive));
    count.emplace_back(1);
    dimension++;
  }

  vtkHDF::ScopedH5SHandle sourceSpace = H5Dget_space(sourceDataset);

  vtkHDF::ScopedH5SHandle destSpace = H5Screate_simple(dimension, count.data(), nullptr);
  if (sourceSpace == H5I_INVALID_HID || sourceSpace == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(this->Writer, << "Could not create destination space " << debugInfo);
    return 0;
  }

  if (H5Sselect_hyperslab(
        sourceSpace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr) < 0)
  {
    vtkErrorWithObjectMacro(this->Writer, << "Could not select hyperslab " << debugInfo);
    return 0;
  }

  if (H5Dread(sourceDataset, H5T_STD_I64LE, destSpace, sourceSpace, H5P_DEFAULT, result.data()) < 0)
  {
    vtkErrorWithObjectMacro(this->Writer, << "Could not read dataset " << debugInfo);
    return 0;
  }

  return result[0];
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::DatasetAndGroupExist(const std::string& dataset, hid_t group)
{
  std::string datasetPath = dataset;
  // Remove trailing '/'
  if (datasetPath.back() == '/')
  {
    datasetPath = dataset.substr(0, dataset.size() - 1);
  }
  vtkDebugWithObjectMacro(this->Writer, "datasetPath is  " << datasetPath);

  std::size_t currentPos = datasetPath.find_first_of('/', 1);
  while (currentPos != std::string::npos)
  {
    std::string groupName = datasetPath.substr(0, currentPos);
    if (H5Lexists(group, groupName.c_str(), H5P_DEFAULT) <= 0)
    {
      return false;
    }
    currentPos = datasetPath.find_first_of('/', currentPos + 1);
  }

  vtkDebugWithObjectMacro(this->Writer, "Testing existence of " << datasetPath);
  return H5Lexists(group, datasetPath.c_str(), H5P_DEFAULT) > 0;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::GetSubFilesDatasetSize(
  const std::string& datasetPath, const std::string& groupName, hsize_t& totalSize)
{
  totalSize = 0;
  for (auto& fileRoot : this->Subfiles)
  {
    if (H5Lexists(fileRoot, groupName.c_str(), H5P_DEFAULT) <= 0 ||
      H5Lexists(fileRoot, datasetPath.c_str(), H5P_DEFAULT) <= 0)
    {
      if (PATH::ContainsAny(datasetPath, { PATH::OFFSETS }))
      {
        totalSize++; // For 0 cells, Offset is still [0]
      }
      continue;
    }
    vtkHDF::ScopedH5DHandle sourceDataset = H5Dopen(fileRoot, datasetPath.c_str(), H5P_DEFAULT);
    const auto sourceDims = vtkHDFUtilities::GetDimensions(sourceDataset);
    if (sourceDims.empty())
    {
      return false;
    }
    totalSize += sourceDims[0];
  }
  return true;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::GetSubfileNonNullPart(
  const std::string& blockPath, int& type)
{
  for (auto& fileRoot : this->Subfiles)
  {
    vtkHDF::ScopedH5GHandle blockG = this->OpenExistingGroup(fileRoot, blockPath.c_str());
    if (blockG == H5I_INVALID_HID)
    {
      vtkWarningWithObjectMacro(
        this->Writer, "Could not find group " << blockPath << " in subfile " << blockG);
    }

    if (H5Aexists(blockG, "Type") >= 0 && vtkHDFUtilities::ReadDataSetType(blockG, type))
    {
      return blockG;
    }
  }

  // Not found
  return H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
hsize_t vtkHDFWriter::Implementation::GetDatasetValueOffset(
  std::size_t partition, const std::string& datasetPath, IndexingMode mode) const
{
  vtkIdType offset = 0;
  switch (mode)
  {
    case IndexingMode::Points:
      offset = this->UgOffsets.UgOffsets[partition].PointOffset;
      break;
    case IndexingMode::Cells:
    {
      offset = this->UgOffsets.UgOffsets[partition].CellOffset;
      // 'Offset' dataset has size (nCells + nPart)
      const std::size_t lastSlash = datasetPath.find_last_of('/');
      if (lastSlash != std::string::npos && datasetPath.substr(lastSlash + 1) == PATH::OFFSETS)
      {
        offset += partition;
      }
      break;
    }
    case IndexingMode::Connectivity:
      offset = this->UgOffsets.UgOffsets[partition].ConnectivityOffset;
      break;
    case IndexingMode::MetaData:
      offset = partition;
      break;
    default:
      vtkErrorWithObjectMacro(this->Writer,
        "Cannot get offset for " << datasetPath << " with mode " << static_cast<uint8_t>(mode)
                                 << " partition " << partition);
      return 0;
  }

  vtkDebugWithObjectMacro(this->Writer,
    "Offset for " << datasetPath << " with mode " << static_cast<uint8_t>(mode) << " partition "
                  << partition << " is " << offset);
  return static_cast<hsize_t>(offset);
}

//------------------------------------------------------------------------------
hsize_t vtkHDFWriter::Implementation::GetDistributedDatasetSize(const std::string& datasetPath)
{
  const IndexingMode indexMode = this->GetDatasetIndexationMode(datasetPath);
  size_t partition = this->UgOffsets.PartOffsets.back() - this->UgOffsets.PartOffsets.front();
  const hsize_t datasetSize = this->GetDatasetValueOffset(partition, datasetPath, indexMode);
  vtkDebugWithObjectMacro(
    this->Writer, << "Dataset " << datasetPath << " has size: " << datasetSize);
  return datasetSize;
}

//------------------------------------------------------------------------------
vtkHDFWriter::Implementation::IndexingMode vtkHDFWriter::Implementation::GetDatasetIndexationMode(
  const std::string& path)
{
  if (PATH::ContainsAny(path, { PATH::NUMBER_OF_CELLS_PER_TREE_DEPTH }))
  {
    return IndexingMode::TreeDepths;
  }
  if (PATH::ContainsAny(path, PATH::COUNT_VALUES) || PATH::ContainsAny(path, { PATH::FIELD_DATA }))
  {
    return IndexingMode::MetaData;
  }
  if (PATH::ContainsAny(path, { PATH::POINT_DATA, PATH::POINTS }))
  {
    return IndexingMode::Points;
  }
  if (PATH::ContainsAny(path, { PATH::CELL_DATA, PATH::OFFSETS, PATH::TYPES, PATH::MASK }))
  {
    return IndexingMode::Cells;
  }
  if (PATH::ContainsAny(path, { PATH::CONNECTIVITY }))
  {
    return IndexingMode::Connectivity;
  }
  if (PATH::ContainsAny(path, { PATH::TREE_IDS, PATH::DEPTH_PER_TREE }))
  {
    return IndexingMode::Trees;
  }
  if (PATH::ContainsAny(path, { PATH::DESCRIPTORS }))
  {
    return IndexingMode::Descriptor;
  }
  if (PATH::ContainsAny(path, { PATH::XCOORDS, PATH::YCOORDS, PATH::ZCOORDS }))
  {
    return IndexingMode::HTGCoords;
  }

  return IndexingMode::Undefined;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::InitDynamicDataset(
  hid_t group, const char* name, hid_t type, hsize_t cols, const std::vector<hsize_t>& chunkSize)
{
  std::vector<hsize_t> dims = { 0 };
  if (cols > 1)
  {
    dims.push_back(cols);
  }

  return this->InitDynamicDataset(group, name, type, dims, chunkSize);
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::InitDynamicDataset(hid_t group, const char* name, hid_t type,
  const std::vector<hsize_t>& dims, const std::vector<hsize_t>& chunkSize)
{
  // When writing data externally, don't create a dynamic dataset,
  // But create a virtual one based on the subfiles on the last step or partition.
  if (!this->Subfiles.empty() && group != this->StepsGroup)
  {
    return true;
  }

  // Skip if dataset already exists
  if (H5Lexists(group, name, H5P_DEFAULT) > 0)
  {
    return true;
  }

  if (dims.empty())
  {
    vtkErrorWithObjectMacro(this->Writer, "dims must not be empty for InitDynamicDataset");
    return false;
  }

  std::vector<hsize_t> maxDims = dims;
  maxDims[0] = H5S_UNLIMITED;

  vtkHDF::ScopedH5SHandle emptyDataspace =
    H5Screate_simple(static_cast<int>(dims.size()), dims.data(), maxDims.data());
  if (emptyDataspace == H5I_INVALID_HID)
  {
    return false;
  }

  vtkHDF::ScopedH5PHandle plist = H5Pcreate(H5P_DATASET_CREATE);
  if (plist == H5I_INVALID_HID)
  {
    return false;
  }
  H5Pset_layout(plist, H5D_CHUNKED);
  H5Pset_chunk(plist, static_cast<int>(dims.size()), chunkSize.data());
  if (this->CompressionLevel != 0)
  {
    H5Pset_deflate(plist, this->CompressionLevel);
  }

  vtkHDF::ScopedH5DHandle dataset =
    H5Dcreate(group, name, type, emptyDataspace, H5P_DEFAULT, plist, H5P_DEFAULT);
  return dataset != H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
void vtkHDFWriter::Implementation::CreateArraysFromNonNullPart(hid_t group, vtkDataObject* data)
{
  std::map<int, hid_t> attributeDataGroup;
  attributeDataGroup[vtkDataObject::POINT] = H5I_INVALID_HID;
  attributeDataGroup[vtkDataObject::CELL] = H5I_INVALID_HID;
  attributeDataGroup[vtkDataObject::FIELD] = H5I_INVALID_HID;
  std::array attributeGroupNames = { PATH::POINT_DATA, PATH::CELL_DATA, PATH::FIELD_DATA };

  for (int i = 0; i < 3; i++)
  {
    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): static data is NUL-terminated
    if (H5Lexists(group, attributeGroupNames[i].data(), H5P_DEFAULT) > 0)
    {
      vtkHDF::ScopedH5GHandle pointDataGroup =
        // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): static data is NUL-terminated
        this->OpenExistingGroup(group, attributeGroupNames[i].data());
      attributeDataGroup[i] = pointDataGroup;
      for (const std::string& name : vtkHDFUtilities::GetArrayNames(attributeDataGroup, i))
      {
        std::vector<hsize_t> extent = { 0, 0 };
        vtkSmartPointer<vtkDataArray> array = vtk::TakeSmartPointer(
          vtkHDFUtilities::NewArrayForGroup(attributeDataGroup[i], name.c_str(), extent));
        array->SetName(name.c_str());
        if (i == vtkDataObject::AttributeTypes::FIELD)
        {
          data->GetAttributesAsFieldData(i)->AddArray(array);
        }
        else
        {
          data->GetAttributes(i)->AddArray(array);
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
vtkHDFWriter::Implementation::Implementation(vtkHDFWriter* writer)
  : Writer(writer)
  , File(H5I_INVALID_HID)
  , Root(H5I_INVALID_HID)
{
}

//------------------------------------------------------------------------------
vtkHDFWriter::Implementation::~Implementation() = default;

VTK_ABI_NAMESPACE_END
