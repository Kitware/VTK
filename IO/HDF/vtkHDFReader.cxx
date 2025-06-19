// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#define VTK_DEPRECATION_LEVEL 0

#include "vtkHDFReader.h"
#include "vtkAMRUtilities.h"
#include "vtkAffineArray.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkHDFReaderImplementation.h"
#include "vtkHDFUtilities.h"
#include "vtkHDFVersion.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix3x3.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/Encoding.hxx"
#include "vtksys/FStream.hxx"
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <locale>
#include <numeric>
#include <sstream>
#include <vector>

#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHDFReader);

namespace
{
//----------------------------------------------------------------------------
int GetNDims(int* extent)
{
  int ndims = 3;
  if (extent[5] - extent[4] == 0)
  {
    --ndims;
  }
  if (extent[3] - extent[2] == 0)
  {
    --ndims;
  }
  return ndims;
}

//----------------------------------------------------------------------------
std::vector<hsize_t> ReduceDimension(int* updateExtent, int* wholeExtent)
{
  int dims = ::GetNDims(wholeExtent);
  std::vector<hsize_t> v(2 * dims);
  for (int i = 0; i < dims; ++i)
  {
    int j = 2 * i;
    v[j] = updateExtent[j];
    v[j + 1] = updateExtent[j + 1];
  }
  return v;
}

//----------------------------------------------------------------------------
template <typename ImplT, typename CacheT>
vtkSmartPointer<vtkDataArray> ReadFromFileOrCache(ImplT* impl, std::shared_ptr<CacheT> cache,
  int tag, std::string name, std::string name_modifier, vtkIdType offset, vtkIdType size,
  bool mData = true)
{
  vtkSmartPointer<vtkDataArray> array;
  std::string cacheName = name + name_modifier;
  if (cache && cache->CheckExistsAndEqual(tag, cacheName, offset, size))
  {
    array = vtkDataArray::SafeDownCast(cache->Get(tag, cacheName));
    if (!array)
    {
      vtkErrorWithObjectMacro(nullptr, "Cannot read the " << cacheName << " array from cache");
      return nullptr;
    }
  }
  else
  {
    array = vtk::TakeSmartPointer(mData ? impl->NewMetadataArray(name.c_str(), offset, size)
                                        : impl->NewArray(tag, name.c_str(), offset, size));
    if (!array)
    {
      vtkErrorWithObjectMacro(nullptr, "Cannot read the " + cacheName + " array from file");
      return nullptr;
    }
    if (cache)
    {
      cache->Set(tag, cacheName, offset, size, array);
    }
  }
  return array;
}

//----------------------------------------------------------------------------
template <typename T, typename CacheT>
bool ReadPolyDataPiece(T* impl, std::shared_ptr<CacheT> cache, vtkIdType pointOffset,
  vtkIdType numberOfPoints, std::vector<vtkIdType>& cellOffsets,
  std::vector<vtkIdType>& numberOfCells, std::vector<vtkIdType>& connectivityOffsets,
  std::vector<vtkIdType>& numberOfConnectivityIds, int filePiece, vtkPolyData* pieceData,
  const std::string& compositePath)
{
  auto readFromFileOrCache = [&](int tag, std::string name, vtkIdType offset, vtkIdType size)
  {
    std::string modifier = "_" + std::to_string(filePiece) + "_" + compositePath;
    return ReadFromFileOrCache(impl, cache, tag, name, modifier, offset, size);
  };
  vtkSmartPointer<vtkDataArray> pointArray;
  if ((pointArray = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG, "Points",
         pointOffset, numberOfPoints)) == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, "Cannot read the Points array");
    return false;
  }

  vtkNew<vtkPoints> points;
  pieceData->SetPoints(points);

  /* If cache is up to date with the geometry, avoid geometry load
   * which would cause the MTime of the geometry to update.
   * SetData would prevent us from using the MeshMTime correctly.
   */
  if (cache != nullptr && !cache->HasBeenUpdated && compositePath.empty())
  {
    return true;
  }
  points->SetData(pointArray);

  std::vector<vtkSmartPointer<vtkCellArray>> cArrays;
  for (std::size_t iTopo = 0; iTopo < vtkHDFUtilities::NUM_POLY_DATA_TOPOS; ++iTopo)
  {
    const auto& name = vtkHDFUtilities::POLY_DATA_TOPOS[iTopo];
    vtkSmartPointer<vtkDataArray> offsetsArray;
    if ((offsetsArray = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG,
           (name + "/Offsets"), cellOffsets[iTopo], numberOfCells[iTopo] + 1)) == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Cannot read the Offsets array for " + name);
      return false;
    }
    vtkSmartPointer<vtkDataArray> connectivityArray;
    if ((connectivityArray = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG,
           (name + "/Connectivity"), connectivityOffsets[iTopo], numberOfConnectivityIds[iTopo])) ==
      nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Cannot read the Connectivity array for " + name);
      return false;
    }
    vtkNew<vtkCellArray> cellArray;
    cellArray->SetData(offsetsArray, connectivityArray);
    cArrays.emplace_back(cellArray);
  }
  pieceData->SetVerts(cArrays[0]);
  pieceData->SetLines(cArrays[1]);
  pieceData->SetPolys(cArrays[2]);
  pieceData->SetStrips(cArrays[3]);
  return true;
}

//----------------------------------------------------------------------------
/**
 * Update the MeshCache if the geometry changed from previous last step,
 * else it loads the geometry data from the cache.
 */
template <typename vtkDataObjT>
void UpdateGeometryIfRequired(vtkDataObjT* data, vtkCompositeDataSet* compositeData, bool useCache,
  bool meshGeometryChanged, vtkDataObjectMeshCache* meshCache)
{
  if (useCache)
  {
    if (!meshGeometryChanged)
    {
      if (compositeData)
      {
        meshCache->CopyCacheToDataObject(compositeData);
      }
      else
      {
        meshCache->CopyCacheToDataObject(data);
      }
    }
    else
    {
      if (compositeData)
      {
        meshCache->UpdateCache(compositeData);
      }
      else
      {
        meshCache->UpdateCache(data);
      }
    }
  }
}

}

//----------------------------------------------------------------------------
/*
 * A data cache for avoiding supplemental read of data that doesn't change from
 * one time step to the next
 *
 * Comment: The cache could be improved to also conserve the MeshMTime of the
 * DataSets by adding supplemental storage for the intermediate geometrical containers
 * (i.e. vtkPoints and vtkCellArray). By also taking them from the cache and avoiding
 * their reinitialization the MeshMTime of the data sets can be conserved and this
 * reader could work better with static mesh mechanisms in VTK.
 */
struct vtkHDFReader::DataCache
{
  /*
   * The key is a pair of:
   * - first: an int flag referring to the attribute type
   * - second: a unique name associated to the array for that attribute type
   */
  using KeyT = std::pair<int, std::string>;
  /*
   * The values of the map are also pairs with:
   * - first: the extent of the last read array in the file
   * - second: a vtkSmartPointer to the array itself for quick access
   */
  using ValueT = std::pair<std::vector<vtkIdType>, vtkSmartPointer<vtkAbstractArray>>;
  bool Has(int attribute, const std::string& key)
  {
    return (this->Map.find(KeyT{ attribute, key }) != this->Map.end());
  }

  template <typename T>
  bool CheckExistsAndEqual(int attribute, const std::string& name, const T& currentOffset)
  {
    if (!this->Has(attribute, name))
    {
      return false;
    }
    const auto& lastOffsets = this->Map.find(KeyT{ attribute, name })->second;
    if (lastOffsets.first.size() != currentOffset.size())
    {
      return false;
    }
    for (std::size_t iO = 0; iO < lastOffsets.first.size(); ++iO)
    {
      if (lastOffsets.first[iO] != static_cast<vtkIdType>(currentOffset[iO]))
      {
        return false;
      }
    }
    return true;
  }

  template <typename T, typename ArrayT>
  void Set(int attribute, const std::string& name, const T& offset, vtkSmartPointer<ArrayT> array)
  {
    std::vector<vtkIdType> buff(offset.size());
    std::copy(offset.begin(), offset.end(), buff.begin());
    this->Map.emplace(KeyT{ attribute, name },
      ValueT{ std::move(buff), static_cast<vtkSmartPointer<vtkAbstractArray>>(array) });
    this->HasBeenUpdated = true;
  }

  template <typename OffT>
  bool CheckExistsAndEqual(
    int attribute, const std::string& name, const OffT& currentOffset, const OffT& currentSize)
  {
    std::vector<vtkIdType> buff{ static_cast<vtkIdType>(currentOffset),
      static_cast<vtkIdType>(currentSize) };
    return this->CheckExistsAndEqual(attribute, name, buff);
  }

  template <typename OffT, typename ArrayT>
  void Set(int attribute, const std::string& name, const OffT& offset, const OffT& size,
    vtkSmartPointer<ArrayT> array)
  {
    auto key = KeyT{ attribute, name };
    std::vector<vtkIdType> buff{ static_cast<vtkIdType>(offset), static_cast<vtkIdType>(size) };
    this->Map.emplace(
      key, ValueT{ std::move(buff), static_cast<vtkSmartPointer<vtkAbstractArray>>(array) });
    this->HasBeenUpdated = true;
  }

  vtkSmartPointer<vtkAbstractArray> Get(int attribute, const std::string& name)
  {
    auto it = this->Map.find(KeyT{ attribute, name });
    if (it == this->Map.end())
    {
      return nullptr;
    }
    return it->second.second;
  }

  void ResetCacheUpdatedStatus() { this->HasBeenUpdated = false; }

  /*
   * Returns the cache updated status and resets it afterwards
   */
  bool CheckCacheUpdatedStatus()
  {
    bool result = this->HasBeenUpdated;
    ResetCacheUpdatedStatus();
    return result;
  }
  bool HasBeenUpdated = false;

private:
  std::map<KeyT, ValueT> Map;
};

//----------------------------------------------------------------------------
vtkHDFReader::vtkHDFReader()
  : Cache(std::make_shared<DataCache>())
{
  this->FileName = nullptr;
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkHDFReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  for (int i = 0; i < vtkHDFUtilities::GetNumberOfAttributeTypes(); ++i)
  {
    this->DataArraySelection[i] = vtkDataArraySelection::New();
    this->DataArraySelection[i]->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  }
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Impl = new vtkHDFReader::Implementation(this);
  this->TimeRange[0] = this->TimeRange[1] = 0.0;
  this->MeshCache->SetConsumer(this);
  this->MeshCache->AddOriginalIds(
    vtkDataObject::POINT, this->GetAttributeOriginalIdName(vtkDataObject::POINT));
  this->MeshCache->AddOriginalIds(
    vtkDataObject::CELL, this->GetAttributeOriginalIdName(vtkDataObject::CELL));
}

//----------------------------------------------------------------------------
vtkHDFReader::~vtkHDFReader()
{
  delete this->Impl;
  this->SetFileName(nullptr);
  for (int i = 0; i < vtkHDFUtilities::GetNumberOfAttributeTypes(); ++i)
  {
    this->DataArraySelection[i]->RemoveObserver(this->SelectionObserver);
    this->DataArraySelection[i]->Delete();
  }
  this->SelectionObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkHDFReader::MergePartsOn()
{
  this->SetMergeParts(true);
}

//----------------------------------------------------------------------------
void vtkHDFReader::MergePartsOff()
{
  this->SetMergeParts(false);
}

//----------------------------------------------------------------------------
void vtkHDFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "CellDataArraySelection: " << this->DataArraySelection[vtkDataObject::CELL]
     << "\n";
  os << indent << "PointDataArraySelection: " << this->DataArraySelection[vtkDataObject::POINT]
     << "\n";
  os << indent << "HasTemporalData: " << (this->HasTemporalData ? "true" : "false") << "\n";
  os << indent << "NumberOfSteps: " << this->NumberOfSteps << "\n";
  os << indent << "Step: " << this->Step << "\n";
  os << indent << "TimeValue: " << this->TimeValue << "\n";
  os << indent << "TimeRange: " << this->TimeRange[0] << " - " << this->TimeRange[1] << "\n";
}

//----------------------------------------------------------------------------
vtkDataSet* vtkHDFReader::GetOutputAsDataSet()
{
  return this->GetOutputAsDataSet(0);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkHDFReader::GetOutputAsDataSet(int index)
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(index));
}

//----------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_4_0()
bool vtkHDFReader::GetHasTransientData()
{
  return this->GetHasTemporalData();
}

//----------------------------------------------------------------------------
bool vtkHDFReader::GetHasTemporalData()
{
  return this->HasTemporalData || this->HasTransientData;
}

//----------------------------------------------------------------------------
void vtkHDFReader::SetHasTemporalData(bool hasTemporalData)
{
  this->HasTemporalData = hasTemporalData;
  this->HasTransientData = hasTemporalData;
}

//----------------------------------------------------------------------------
// Major version should be incremented when older readers can no longer
// read files written for this reader. Minor versions are for added
// functionality that can be safely ignored by older readers.
int vtkHDFReader::CanReadFileVersion(int major, int vtkNotUsed(minor))
{
  return (major > vtkHDFMajorVersion) ? 0 : 1;
}

//----------------------------------------------------------------------------
int vtkHDFReader::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(name, &fs) != 0)
  {
    vtkErrorMacro("File does not exist: " << name);
    return 0;
  }
  if (!this->Impl->Open(name))
  {
    return 0;
  }
  this->Impl->Close();
  return 1;
}

//----------------------------------------------------------------------------
void vtkHDFReader::SelectionModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkHDFReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int vtkHDFReader::GetNumberOfPointArrays()
{
  return this->DataArraySelection[vtkDataObject::POINT]->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkHDFReader::GetPointArrayName(int index)
{
  return this->DataArraySelection[vtkDataObject::POINT]->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkHDFReader::GetPointDataArraySelection()
{
  return this->DataArraySelection[vtkDataObject::POINT];
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkHDFReader::GetFieldDataArraySelection()
{
  return this->DataArraySelection[vtkDataObject::FIELD];
}

//----------------------------------------------------------------------------
int vtkHDFReader::GetNumberOfCellArrays()
{
  return this->DataArraySelection[vtkDataObject::CELL]->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkHDFReader::GetCellDataArraySelection()
{
  return this->DataArraySelection[vtkDataObject::CELL];
}

//----------------------------------------------------------------------------
const char* vtkHDFReader::GetCellArrayName(int index)
{
  return this->DataArraySelection[vtkDataObject::CELL]->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkHDFReader::RequestDataObject(vtkInformation*, vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  std::map<int, std::string> typeNameMap = { { VTK_IMAGE_DATA, "vtkImageData" },
    { VTK_UNSTRUCTURED_GRID, "vtkUnstructuredGrid" }, { VTK_POLY_DATA, "vtkPolyData" },
    { VTK_OVERLAPPING_AMR, "vtkOverlappingAMR" }, { VTK_HYPER_TREE_GRID, "vtkHyperTreeGrid" },
    { VTK_PARTITIONED_DATA_SET_COLLECTION, "vtkPartitionedDataSetCollection" },
    { VTK_MULTIBLOCK_DATA_SET, "vtkMultiBlockDataSet" } };

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }

  if (!this->Impl->Open(this->FileName))
  {
    return 0;
  }
  auto version = this->Impl->GetVersion();
  if (!CanReadFileVersion(version[0], version[1]))
  {
    vtkWarningMacro("File version: " << version[0] << "." << version[1]
                                     << " is higher than "
                                        "this reader supports "
                                     << vtkHDFMajorVersion << "." << vtkHDFMinorVersion);
  }

  if (this->MergeParts)
  {
    vtkWarningMacro("MergeParts option will be ignored. Please use vtkMergeBlocks instead.");
  }

  this->NumberOfSteps = this->Impl->GetNumberOfSteps();
  const int numPieces = this->Impl->GetNumberOfPieces(this->Step);
  this->SetHasTemporalData(this->NumberOfSteps > 1);
  const int dataSetType = this->Impl->GetDataSetType();
  if (!output || !output->IsA(typeNameMap[dataSetType].c_str()))
  {
    this->Assembly = vtkSmartPointer<vtkDataAssembly>::New();
    info->Set(vtkDataObject::DATA_OBJECT(), this->Impl->GetNewDataSet(dataSetType, numPieces));
    for (int i = 0; i < vtkHDFUtilities::GetNumberOfAttributeTypes(); ++i)
    {
      const std::vector<std::string> arrayNames = this->Impl->GetArrayNames(i);
      // Remove obsolete arrays from selection
      vtkIdType arrId = 0;
      while (arrId < this->DataArraySelection[i]->GetNumberOfArrays())
      {
        auto arrName = this->DataArraySelection[i]->GetArrayName(arrId);
        if (std::find(arrayNames.cbegin(), arrayNames.cend(), arrName) == arrayNames.cend())
        {
          // Selected array is not available anymore
          this->DataArraySelection[i]->RemoveArrayByName(arrName);
        }
        else
        {
          arrId++;
        }
      }
      // Add new arrays to selection
      for (const std::string& arrayName : arrayNames)
      {
        if (!this->DataArraySelection[i]->ArrayExists(arrayName.c_str()))
        {
          this->DataArraySelection[i]->AddArray(arrayName.c_str());
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }
  // Ensures a new file is open. This happen for vtkFileSeriesReader
  // which does not call RequestDataObject for every time step.
  if (!this->Impl->Open(this->FileName))
  {
    vtkErrorMacro("Could not open file " << this->FileName);
    return 0;
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Invalid output information object");
    return 0;
  }

  return this->SetupInformation(outInfo);
}

//------------------------------------------------------------------------------
int vtkHDFReader::SetupInformation(vtkInformation* outInfo)
{
  int dataSetType = this->Impl->GetDataSetType();
  if (dataSetType == VTK_IMAGE_DATA)
  {
    int WholeExtent[6];
    double Origin[3];
    double Spacing[3];

    if (!this->Impl->GetImageAttributes(WholeExtent, Origin, Spacing))
    {
      return 0;
    }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), WholeExtent, 6);
    outInfo->Set(vtkDataObject::ORIGIN(), Origin, 3);
    outInfo->Set(vtkDataObject::SPACING(), Spacing, 3);
    outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID || dataSetType == VTK_POLY_DATA)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
  else if (dataSetType == VTK_OVERLAPPING_AMR)
  {
    double Origin[3];
    if (!this->Impl->GetAttribute("Origin", 3, Origin))
    {
      vtkErrorMacro("Could not get Origin attribute");
      return 0;
    }
    outInfo->Set(vtkDataObject::ORIGIN(), Origin, 3);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 0);
  }
  else if (dataSetType == VTK_HYPER_TREE_GRID)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
  else if (dataSetType == VTK_PARTITIONED_DATA_SET_COLLECTION ||
    dataSetType == VTK_MULTIBLOCK_DATA_SET)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
    if (dataSetType == VTK_PARTITIONED_DATA_SET_COLLECTION)
    {
      this->GenerateAssembly();
    }
    if (!this->RetrieveDataArraysFromAssembly())
    {
      return 0;
    }
    if (!this->Impl->RetrieveHDFInformation(vtkHDFUtilities::VTKHDF_ROOT_PATH))
    {
      return 0;
    }
    if (!this->RetrieveStepsFromAssembly())
    {
      return 0;
    }
  }
  else
  {
    vtkErrorMacro("Invalid dataset type: " << dataSetType);
    return 0;
  }

  // Recover temporal data information
  this->SetHasTemporalData(this->NumberOfSteps > 1);
  if (this->GetHasTemporalData())
  {
    std::vector<double> values(this->NumberOfSteps, 0.0);
    {
      vtkSmartPointer<vtkDataArray> stepValues = vtk::TakeSmartPointer(this->Impl->GetStepValues());
      if (stepValues)
      {
        auto container = vtk::DataArrayValueRange<1>(stepValues);
        std::copy(container.begin(), container.end(), values.begin());

        this->TimeRange[0] = *std::min_element(values.begin(), values.end());
        this->TimeRange[1] = *std::max_element(values.begin(), values.end());
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), values.data(),
          static_cast<int>(values.size()));
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), this->TimeRange.data(), 2);
      }
    }
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkHDFReader::PrintPieceInformation(vtkInformation* outInfo)
{
  std::array<int, 6> updateExtent;
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent.data());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numGhosts = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  std::cout << "Piece:" << piece << " " << numPieces << " " << numGhosts;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    std::cout << " Extent: " << updateExtent[0] << " " << updateExtent[1] << " " << updateExtent[2]
              << " " << updateExtent[3] << " " << updateExtent[4] << " " << updateExtent[5];
  }
  std::cout << std::endl;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkImageData* data)
{
  int WholeExtent[6];
  double Origin[3];
  double Spacing[3];

  if (!this->Impl->GetImageAttributes(WholeExtent, Origin, Spacing))
  {
    return 0;
  }

  std::vector<int> updateExtent(WholeExtent, WholeExtent + 6);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent.data());
  }

  data->SetOrigin(Origin);
  data->SetSpacing(Spacing);
  data->SetExtent(updateExtent.data());
  if (!this->Impl->GetAttribute("Direction", 9, data->GetDirectionMatrix()->GetData()))
  {
    return 0;
  }

  // in the same order as vtkDataObject::AttributeTypes: POINT, CELL
  for (int attributeType = 0; attributeType < vtkDataObject::FIELD; ++attributeType)
  {
    const hsize_t pointModifier = (attributeType == vtkDataObject::POINT) ? 1 : 0;
    std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
    for (const std::string& name : names)
    {
      if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
      {
        vtkSmartPointer<vtkDataArray> array;
        std::vector<hsize_t> fileExtent = ::ReduceDimension(updateExtent.data(), WholeExtent);
        std::vector<int> extentBuffer(fileExtent.size(), 0);
        std::copy(
          updateExtent.begin(), updateExtent.begin() + extentBuffer.size(), extentBuffer.begin());
        if (this->GetHasTemporalData())
        {
          vtkIdType offset = this->Impl->GetArrayOffset(this->Step, attributeType, name);
          if (offset >= 0)
          {
            extentBuffer.emplace_back(offset);
            extentBuffer.emplace_back(offset);
          }
          else
          {
            extentBuffer.emplace_back(this->Step);
            extentBuffer.emplace_back(this->Step);
          }
          fileExtent.resize(extentBuffer.size(), 0);
        }
        // Create the memory space, reverse axis order for VTK fortran order,
        // because VTK stores 2D/3D arrays in memory along columns (fortran order) rather
        // than along rows (C order)
        for (std::size_t iDim = 0; iDim < fileExtent.size() / 2; ++iDim)
        {
          std::size_t rIDim = (fileExtent.size() / 2) - 1 - iDim;
          // if an extent value is negative it won't go into an hsize_t
          if (extentBuffer[rIDim * 2] < 0)
          {
            extentBuffer[rIDim * 2 + 1] -= extentBuffer[rIDim * 2];
            extentBuffer[rIDim * 2] = 0;
          }
          fileExtent[iDim * 2] = extentBuffer[rIDim * 2];
          fileExtent[iDim * 2 + 1] = extentBuffer[rIDim * 2 + 1] + pointModifier;
        }
        if (this->GetHasTemporalData() && !pointModifier)
        {
          // Add one to the extent for the time dimension if needed
          fileExtent[1] += 1;
        }
        if (this->UseCache && this->Cache->CheckExistsAndEqual(attributeType, name, fileExtent))
        {
          array = vtkDataArray::SafeDownCast(this->Cache->Get(attributeType, name));
          if (!array)
          {
            vtkErrorMacro("Error retrieving array " + name + " from cache.");
            return 0;
          }
        }
        else
        {
          if ((array = vtk::TakeSmartPointer(
                 this->Impl->NewArray(attributeType, name.c_str(), fileExtent))) == nullptr)
          {
            vtkErrorMacro("Error reading array " << name);
            return 0;
          }
        }
        array->SetName(name.c_str());
        data->GetAttributesAsFieldData(attributeType)->AddArray(array);
        if (this->UseCache)
        {
          this->Cache->Set(attributeType, name, fileExtent, array);
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::AddFieldArrays(vtkDataObject* data)
{
  const std::vector<std::string> names = this->Impl->GetArrayNames(vtkDataObject::FIELD);
  for (const std::string& name : names)
  {
    vtkSmartPointer<vtkAbstractArray> array;
    vtkIdType offset = -1;
    std::array<vtkIdType, 2> size = { -1, -1 };
    if (this->Impl->GetDataSetType() != VTK_OVERLAPPING_AMR && this->GetHasTemporalData())
    {
      size = this->Impl->GetFieldArraySize(this->Step, name);
      offset = this->Impl->GetArrayOffset(this->Step, vtkDataObject::FIELD, name);
      if (size[0] == 0 && size[1] == 0)
      {
        continue;
      }
    }
    if (this->UseCache &&
      this->Cache->CheckExistsAndEqual(vtkDataObject::FIELD, name, offset, size[1]))
    {
      array = this->Cache->Get(vtkDataObject::FIELD, name);
      if (!array)
      {
        vtkErrorMacro("Error retrieving array " + name + " from cache.");
        return 0;
      }
    }
    else
    {
      if ((array = vtk::TakeSmartPointer(
             this->Impl->NewFieldArray(name.c_str(), offset, size[1], size[0]))) == nullptr)
      {
        vtkErrorMacro("Error reading array " << name);
        return 0;
      }
      array->SetName(name.c_str());
    }
    data->GetAttributesAsFieldData(vtkDataObject::FIELD)->AddArray(array);
    if (this->UseCache)
    {
      this->Cache->Set(vtkDataObject::FIELD, name, offset, size[1], array);
    }
  }
  if (this->GetHasTemporalData())
  {
    vtkNew<vtkDoubleArray> time;
    time->SetName("Time");
    time->SetNumberOfComponents(1);
    time->SetNumberOfTuples(1);
    time->SetValue(0, this->TimeValue);
    data->GetAttributesAsFieldData(vtkDataObject::FIELD)->AddArray(time);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(const std::vector<vtkIdType>& numberOfPoints,
  const std::vector<vtkIdType>& numberOfCells,
  const std::vector<vtkIdType>& numberOfConnectivityIds, vtkIdType partOffset,
  vtkIdType startingPointOffset, vtkIdType startingCellOffset,
  vtkIdType startingConnectivityIdOffset, int filePiece, vtkUnstructuredGrid* pieceData)
{
  auto readFromFileOrCache =
    [&](int tag, std::string name, vtkIdType offset, vtkIdType size, bool mData)
  {
    std::string modifier = "_" + std::to_string(filePiece) + "_" + this->CompositeCachePath;
    return ::ReadFromFileOrCache(
      this->Impl, this->UseCache ? this->Cache : nullptr, tag, name, modifier, offset, size, mData);
  };
  // Prepare to check if geometry of the piece is updated
  this->Cache->ResetCacheUpdatedStatus();
  // read the piece and add it to data
  vtkSmartPointer<vtkDataArray> pointArray;
  vtkIdType pointOffset =
    std::accumulate(numberOfPoints.data(), &numberOfPoints[filePiece], startingPointOffset);
  if ((pointArray = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG, "Points",
         pointOffset, numberOfPoints[filePiece], true)) == nullptr)
  {
    vtkErrorMacro("Cannot read the Points array");
    return 0;
  }

  vtkNew<vtkPoints> points;

  /* If cache is up to date with the geometry, avoid geometry load
   * which would cause the MTime of the geometry to update.
   * SetData would prevent us from using the MeshMTime correctly.
   */
  if (!this->UseCache || this->Cache->CheckCacheUpdatedStatus() ||
    !this->CompositeCachePath.empty())
  {
    points->SetData(pointArray);
    this->MeshGeometryChangedFromPreviousTimeStep = true;
  }
  pieceData->SetPoints(points);
  vtkNew<vtkCellArray> cellArray;
  vtkSmartPointer<vtkDataArray> offsetsArray;
  vtkSmartPointer<vtkDataArray> connectivityArray;
  vtkSmartPointer<vtkDataArray> p;
  vtkUnsignedCharArray* typesArray;
  // the offsets array has (numberOfCells[part] + 1) elements per part.
  vtkIdType offset = std::accumulate(
    numberOfCells.data(), &numberOfCells[filePiece], startingCellOffset + partOffset + filePiece);
  if ((offsetsArray = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG, "Offsets",
         offset, numberOfCells[filePiece] ? numberOfCells[filePiece] + 1 : 0, true)) == nullptr)
  {
    vtkErrorMacro("Cannot read the Offsets array");
    return 0;
  }
  offset = std::accumulate(numberOfConnectivityIds.data(), &numberOfConnectivityIds[filePiece],
    startingConnectivityIdOffset);
  if ((connectivityArray = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG,
         "Connectivity", offset, numberOfConnectivityIds[filePiece], true)) == nullptr)
  {
    vtkErrorMacro("Cannot read the Connectivity array");
    return 0;
  }
  cellArray->SetData(offsetsArray, connectivityArray);

  vtkIdType cellOffset =
    std::accumulate(numberOfCells.data(), &numberOfCells[filePiece], startingCellOffset);
  if ((p = readFromFileOrCache(vtkHDFUtilities::GEOMETRY_ATTRIBUTE_TAG, "Types", cellOffset,
         numberOfCells[filePiece], true)) == nullptr)
  {
    vtkErrorMacro("Cannot read the Types array");
    return 0;
  }
  if ((typesArray = vtkUnsignedCharArray::SafeDownCast(p)) == nullptr)
  {
    vtkErrorMacro("Error: The Types array element is not unsigned char.");
    return 0;
  }
  pieceData->SetCells(typesArray, cellArray);

  std::vector<vtkIdType> offsets = { pointOffset, cellOffset };
  std::vector<vtkIdType> startingOffsets = { startingPointOffset, startingCellOffset };
  std::vector<const std::vector<vtkIdType>*> numberOf = { &numberOfPoints, &numberOfCells };
  // Specify if Geometry changed
  if (this->Cache->CheckCacheUpdatedStatus())
  {
    this->MeshGeometryChangedFromPreviousTimeStep = true;
  }

  for (int attributeType = vtkDataObject::AttributeTypes::POINT;
       attributeType <= vtkDataObject::AttributeTypes::CELL; ++attributeType)
  {
    const std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
    for (const std::string& name : names)
    {
      if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
      {
        vtkIdType arrayOffset = offsets[attributeType];
        if (this->GetHasTemporalData())
        {
          // Read offset for the array values is the temporal offset in "Steps/XDataOffsets/Array"
          // added to the number of X in previous parts of the time step.
          vtkIdType buff = this->Impl->GetArrayOffset(this->Step, attributeType, name);
          if (buff >= 0)
          {
            arrayOffset += buff - startingOffsets[attributeType];
          }
        }
        vtkSmartPointer<vtkDataArray> array;
        if ((array = readFromFileOrCache(attributeType, name, arrayOffset,
               (*numberOf[attributeType])[filePiece], false)) == nullptr)
        {
          vtkErrorMacro("Cannot read the " << name << " array");
          return 0;
        }
        array->SetName(name.c_str());
        pieceData->GetAttributesAsFieldData(attributeType)->AddArray(array);
        if (this->MeshGeometryChangedFromPreviousTimeStep && this->UseCache)
        {
          this->AddOriginalIds(pieceData->GetAttributes(attributeType), array->GetNumberOfTuples(),
            this->GetAttributeOriginalIdName(attributeType));
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(
  vtkInformation* outInfo, vtkUnstructuredGrid* data, vtkPartitionedDataSet* pData)
{
  int filePieceCount = this->Impl->GetNumberOfPieces();
  if (this->GetHasTemporalData())
  {
    filePieceCount = this->Impl->GetNumberOfPieces(this->Step);
  }
  vtkIdType partOffset = 0;
  vtkIdType startingPointOffset = 0;
  vtkIdType startingCellOffset = 0;
  vtkIdType startingConnectivityIdOffset = 0;
  if (this->GetHasTemporalData())
  {
    vtkHDFUtilities::TemporalGeometryOffsets geoOffs(this->Impl, this->Step);
    if (!geoOffs.Success)
    {
      vtkErrorMacro("Error in reading temporal geometry offsets");
      return 0;
    }
    partOffset = geoOffs.PartOffset;
    startingPointOffset = geoOffs.PointOffset;
    startingCellOffset = geoOffs.CellOffsets[0];
    startingConnectivityIdOffset = geoOffs.ConnectivityOffsets[0];
  }
  std::vector<vtkIdType> numberOfPoints =
    this->Impl->GetMetadata("NumberOfPoints", filePieceCount, partOffset);
  if (numberOfPoints.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfCells =
    this->Impl->GetMetadata("NumberOfCells", filePieceCount, partOffset);
  if (numberOfCells.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfConnectivityIds =
    this->Impl->GetMetadata("NumberOfConnectivityIds", filePieceCount, partOffset);
  if (numberOfConnectivityIds.empty())
  {
    return 0;
  }
  int memoryPieceCount = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (memoryPieceCount == 0)
  {
    vtkErrorMacro("Number of pieces per process was set to 0");
    return 0;
  }
  std::vector<vtkSmartPointer<vtkUnstructuredGrid>> pieces;
  pieces.reserve(filePieceCount / memoryPieceCount);
  for (int filePiece = piece; filePiece < filePieceCount; filePiece += memoryPieceCount)
  {
    vtkNew<vtkUnstructuredGrid> pieceData;
    pieceData->Initialize();
    if (!this->Read(numberOfPoints, numberOfCells, numberOfConnectivityIds, partOffset,
          startingPointOffset, startingCellOffset, startingConnectivityIdOffset, filePiece,
          pieceData))
    {
      return 0;
    }
    pieces.emplace_back(pieceData);
  }
  std::reverse(pieces.begin(), pieces.end());
  unsigned int nPieces = static_cast<unsigned int>(pieces.size());
  if (pData)
  {
    pData->Initialize();
    pData->SetNumberOfPartitions(nPieces);
    for (unsigned int iPiece = 0; iPiece < nPieces; ++iPiece)
    {
      pData->SetPartition(iPiece, pieces.back());
      pieces.pop_back();
    }
  }
  else if (data)
  {
    // Only single piece datasets should have a non-partitioned output structure,
    // Although all ranks may not have a non-null piece
    assert(pieces.size() <= 1);
    if (pieces.size() == 1)
    {
      data->ShallowCopy(pieces.back());
    }
  }
  else
  {
    vtkErrorMacro("Both proposed outputs are nullptr.");
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkPolyData* data, vtkPartitionedDataSet* pData)
{
  // The number of pieces in this step
  int filePieceCount = this->Impl->GetNumberOfPieces();
  if (this->GetHasTemporalData())
  {
    filePieceCount = this->Impl->GetNumberOfPieces(this->Step);
  }

  // The initial offsetting with which to read the step in particular
  vtkIdType partOffset = 0;
  vtkIdType startingPointOffset = 0;
  std::vector<vtkIdType> startingCellOffsets(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
  std::vector<vtkIdType> startingConnectivityIdOffsets(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);

  if (this->GetHasTemporalData())
  {
    // Read the time offsets for this step
    vtkHDFUtilities::TemporalGeometryOffsets geoOffs(this->Impl, this->Step);
    if (!geoOffs.Success)
    {
      vtkErrorMacro("Error in reading temporal geometry offsets");
      return 0;
    }
    // bring these offsets up in scope
    partOffset = geoOffs.PartOffset;
    startingPointOffset = geoOffs.PointOffset;
    std::swap(startingCellOffsets, geoOffs.CellOffsets);
    std::swap(startingConnectivityIdOffsets, geoOffs.ConnectivityOffsets);
  }

  // extract the array containing the number of points for this step
  std::vector<vtkIdType> numberOfPoints =
    this->Impl->GetMetadata("NumberOfPoints", filePieceCount, partOffset);
  if (numberOfPoints.empty())
  {
    vtkErrorMacro("Error in reading NumberOfPoints");
    return 0;
  }

  std::map<std::string, std::vector<vtkIdType>> numberOfCells;
  std::map<std::string, std::vector<vtkIdType>> numberOfCellsBefore;
  std::map<std::string, std::vector<vtkIdType>> numberOfConnectivityIds;
  for (const auto& name : vtkHDFUtilities::POLY_DATA_TOPOS)
  {
    // extract the array containing the number of cells of this topology for this step
    numberOfCells[name] =
      this->Impl->GetMetadata((name + "/NumberOfCells").c_str(), filePieceCount, partOffset);
    numberOfCellsBefore[name] =
      this->Impl->GetMetadata((name + "/NumberOfCells").c_str(), partOffset, 0);
    if (numberOfCells[name].empty())
    {
      vtkErrorMacro("Error in reading NumberOfCells for " + name);
      return 0;
    }
    // extract the array containing the number of connectivity ids of this topology for this step
    numberOfConnectivityIds[name] = this->Impl->GetMetadata(
      (name + "/NumberOfConnectivityIds").c_str(), filePieceCount, partOffset);
    if (numberOfConnectivityIds[name].empty())
    {
      vtkErrorMacro("Error in reading NumberOfConnectivityIds for " + name);
      return 0;
    }
  }
  // determine the stride to use when updating pieces
  int memoryPieceCount = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  // determine the initial piece number to update
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  if (memoryPieceCount == 0)
  {
    vtkErrorMacro("Number of pieces per process was set to 0");
    return 0;
  }
  std::vector<vtkSmartPointer<vtkPolyData>> pieces;
  pieces.reserve(filePieceCount / memoryPieceCount);
  vtkIdType startingCellOffset =
    std::accumulate(startingCellOffsets.begin(), startingCellOffsets.end(), 0);
  for (int filePiece = piece; filePiece < filePieceCount; filePiece += memoryPieceCount)
  {
    // determine the exact offsetting for the piece that needs to be read
    vtkIdType pointOffset =
      std::accumulate(numberOfPoints.data(), &numberOfPoints[filePiece], startingPointOffset);
    std::vector<vtkIdType> cellOffsets(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
    std::vector<vtkIdType> pieceNumberOfCells(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
    std::vector<vtkIdType> connectivityOffsets(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
    std::vector<vtkIdType> pieceNumberOfConnectivityIds(vtkHDFUtilities::NUM_POLY_DATA_TOPOS, 0);
    for (std::size_t iTopo = 0; iTopo < vtkHDFUtilities::NUM_POLY_DATA_TOPOS; ++iTopo)
    {
      const auto& nCells = numberOfCells[vtkHDFUtilities::POLY_DATA_TOPOS[iTopo]];
      vtkIdType connectivityPartOffset = 0;
      vtkIdType numCellSum = 0;
      for (const auto& numCell : numberOfCellsBefore[vtkHDFUtilities::POLY_DATA_TOPOS[iTopo]])
      {
        // No need to iterate if there is no offsetting on the connectivity. Otherwise, we
        // accumulate the number of part until we reach the current offset, it's useful to retrieve
        // the real cell offset
        if (numCellSum >= startingCellOffsets[iTopo])
        {
          break;
        }
        else
        {
          connectivityPartOffset += 1;
        }
        numCellSum += numCell;
      }
      cellOffsets[iTopo] = std::accumulate(nCells.begin(), nCells.begin() + filePiece,
        startingCellOffsets[iTopo] + connectivityPartOffset + filePiece);
      pieceNumberOfCells[iTopo] = nCells[filePiece];
      const auto& nConnectivity = numberOfConnectivityIds[vtkHDFUtilities::POLY_DATA_TOPOS[iTopo]];
      connectivityOffsets[iTopo] = std::accumulate(nConnectivity.begin(),
        nConnectivity.begin() + filePiece, startingConnectivityIdOffsets[iTopo]);
      pieceNumberOfConnectivityIds[iTopo] = nConnectivity[filePiece];
    }

    // populate the poly data piece
    vtkNew<vtkPolyData> pieceData;
    pieceData->Initialize();

    // Read geometry
    this->Cache->ResetCacheUpdatedStatus();
    if (!::ReadPolyDataPiece(this->Impl, this->UseCache ? this->Cache : nullptr, pointOffset,
          numberOfPoints[filePiece], cellOffsets, pieceNumberOfCells, connectivityOffsets,
          pieceNumberOfConnectivityIds, filePiece, pieceData, this->CompositeCachePath))
    {
      vtkErrorMacro(
        "There was an error in reading the " << filePiece << " piece of the poly data file.");
      return 0;
    }

    if (this->Cache->CheckCacheUpdatedStatus())
    {
      this->MeshGeometryChangedFromPreviousTimeStep = true;
    }

    // sum over topologies to get total offsets for fields
    vtkIdType cellOffset = startingCellOffset;
    for (const auto& name : vtkHDFUtilities::POLY_DATA_TOPOS)
    {
      const auto& nCells = numberOfCells[name];
      cellOffset = std::accumulate(nCells.begin(), nCells.begin() + filePiece, cellOffset);
    }
    vtkIdType accumulatedNumberOfCells =
      std::accumulate(pieceNumberOfCells.begin(), pieceNumberOfCells.end(), 0);

    // read point and cell data arrays
    std::vector<vtkIdType> offsets = { pointOffset, cellOffset };
    std::vector<vtkIdType> startingOffsets = { startingPointOffset, startingCellOffset };
    std::vector<vtkIdType> numberOf = { numberOfPoints[filePiece], accumulatedNumberOfCells };
    for (int attributeType = vtkDataObject::AttributeTypes::POINT;
         attributeType <= vtkDataObject::AttributeTypes::CELL; ++attributeType)
    {
      const std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
      for (const std::string& name : names)
      {
        if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
        {
          vtkIdType arrayOffset = offsets[attributeType];
          if (this->GetHasTemporalData())
          {
            vtkIdType buff = this->Impl->GetArrayOffset(this->Step, attributeType, name);
            if (buff >= 0)
            {
              arrayOffset += buff - startingOffsets[attributeType];
            }
          }
          vtkSmartPointer<vtkDataArray> array;
          if ((array = ::ReadFromFileOrCache(this->Impl, this->UseCache ? this->Cache : nullptr,
                 attributeType, name, "_" + std::to_string(filePiece), arrayOffset,
                 numberOf[attributeType], false)) == nullptr)
          {
            vtkErrorMacro("Error reading array " << name);
            return 0;
          }
          array->SetName(name.c_str());
          pieceData->GetAttributesAsFieldData(attributeType)->AddArray(array);
          if (this->MeshGeometryChangedFromPreviousTimeStep && this->UseCache)
          {
            this->AddOriginalIds(pieceData->GetAttributes(attributeType),
              array->GetNumberOfTuples(), this->GetAttributeOriginalIdName(attributeType));
          }
        }
      }
    }
    pieces.emplace_back(pieceData);
  }
  std::reverse(pieces.begin(), pieces.end());
  unsigned int nPieces = static_cast<unsigned int>(pieces.size());
  if (pData)
  {
    pData->Initialize();
    pData->SetNumberOfPartitions(nPieces);
    for (unsigned int iPiece = 0; iPiece < nPieces; ++iPiece)
    {
      pData->SetPartition(iPiece, pieces.back());
      pieces.pop_back();
    }
  }
  else if (data)
  {
    // Only single piece datasets should have a non-partitioned output structure
    assert(pieces.size() == 1);
    data->ShallowCopy(pieces.back());
  }
  else
  {
    vtkErrorMacro("Both proposed outputs are nullptr.");
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkPartitionedDataSetCollection* pdc)
{
  this->Impl->OpenGroupAsVTKGroup("VTKHDF/");
  // Save temporal information, that can be overridden when changing root dataset
  bool isPDCTemporal = this->GetHasTemporalData();
  vtkIdType pdcSteps = this->NumberOfSteps;

  const std::vector<std::string> datasets =
    this->Impl->GetOrderedChildrenOfGroup(vtkHDFUtilities::VTKHDF_ROOT_PATH);

  pdc->SetNumberOfPartitionedDataSets(
    static_cast<unsigned int>(datasets.size() - 1)); // One child is the assembly
  pdc->SetDataAssembly(this->Assembly);
  for (const auto& datasetName : datasets)
  {
    if (datasetName == "Assembly")
    {
      continue;
    }
    std::string hdfPathName = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" + datasetName;
    if (!this->Impl->RetrieveHDFInformation(hdfPathName))
    {
      return 0;
    }
    this->Impl->OpenGroupAsVTKGroup(hdfPathName); // Change root

    int dsIndex = -1;
    this->Impl->GetAttribute("Index", 1, &dsIndex);
    if (dsIndex == -1)
    {
      vtkErrorMacro("Could not get 'Index' attribute for dataset " << hdfPathName);
      return 0;
    }

    const int numPieces = this->Impl->GetNumberOfPieces(this->Step);
    const int datatype = this->Impl->GetDataSetType();

    vtkSmartPointer<vtkDataObject> dataObject = this->Impl->GetNewDataSet(datatype, numPieces);
    this->CompositeCachePath = datasetName;
    if (!this->ReadData(outInfo, dataObject))
    {
      return 0;
    }

    vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(dataObject);
    if (pds)
    {
      pdc->SetPartitionedDataSet(dsIndex, pds);
    }
    else
    {
      // Craft a PDS from the single-part data object received
      vtkNew<vtkPartitionedDataSet> newPDS;
      newPDS->SetNumberOfPartitions(1);
      newPDS->SetPartition(0, dataObject);
      pdc->SetPartitionedDataSet(dsIndex, newPDS);
    }

    vtkPartitionedDataSet* pData = pdc->GetPartitionedDataSet(dsIndex);
    for (unsigned int idx = 0; idx < pData->GetNumberOfPartitions(); ++idx)
    {
      this->AddFieldArrays(pData->GetPartitionAsDataObject(idx));
    }
  }

  // Implementation can point to a subset due to the previous method instead of the root, reset it
  // to avoid any conflict for temporal dataset.
  this->Impl->RetrieveHDFInformation(vtkHDFUtilities::VTKHDF_ROOT_PATH);
  this->SetHasTemporalData(isPDCTemporal);
  this->NumberOfSteps = pdcSteps;

  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkMultiBlockDataSet* mb)
{
  // Save temporal information, that can be overridden when changing root dataset
  bool isPDCTemporal = this->GetHasTemporalData();
  vtkIdType pdcSteps = this->NumberOfSteps;

  int result = this->ReadRecursively(outInfo, mb, vtkHDFUtilities::VTKHDF_ROOT_PATH + "/Assembly");

  if (!this->Impl->RetrieveHDFInformation(vtkHDFUtilities::VTKHDF_ROOT_PATH))
  {
    return 0;
  }
  this->SetHasTemporalData(isPDCTemporal);
  this->NumberOfSteps = pdcSteps;

  return result;
}

//------------------------------------------------------------------------------
void vtkHDFReader::GenerateAssembly()
{
  this->Assembly->Initialize();
  this->Impl->FillAssembly(this->Assembly);
}

//------------------------------------------------------------------------------
bool vtkHDFReader::RetrieveStepsFromAssembly()
{
  const std::vector<std::string> datasets =
    this->Impl->GetOrderedChildrenOfGroup(vtkHDFUtilities::VTKHDF_ROOT_PATH);
  for (const auto& datasetName : datasets)
  {
    if (datasetName == "Assembly")
    {
      continue;
    }
    std::string hdfPathName = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" + datasetName;
    if (!this->Impl->HasAttribute(hdfPathName.c_str(), "Type"))
    {
      // Do not read the (null) block if type is not set
      continue;
    }
    this->Impl->OpenGroupAsVTKGroup(hdfPathName);
    std::size_t nStep = this->Impl->GetNumberOfSteps();

    if (nStep > 1)
    {
      if (this->NumberOfSteps > 1 && this->NumberOfSteps != static_cast<vtkIdType>(nStep))
      {
        vtkErrorMacro("This composite file has mismatching number of steps between datasets : "
          << this->NumberOfSteps << " and " << nStep
          << ". Number of steps need to be the same across composite components.");
        return false;
      }
      this->NumberOfSteps = nStep;
      this->SetHasTemporalData(true);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::RetrieveDataArraysFromAssembly()
{
  const std::vector<std::string> datasets =
    this->Impl->GetOrderedChildrenOfGroup(vtkHDFUtilities::VTKHDF_ROOT_PATH);
  for (const auto& datasetName : datasets)
  {
    if (datasetName == "Assembly")
    {
      continue;
    }
    std::string hdfPathName = vtkHDFUtilities::VTKHDF_ROOT_PATH + "/" + datasetName;

    if (!this->Impl->HasAttribute(hdfPathName.c_str(), "Type"))
    {
      continue; // Allow empty datasets in assembly
    }
    if (!this->Impl->RetrieveHDFInformation(hdfPathName))
    {
      return false;
    }

    // Fill DataArray
    this->Impl->RetrieveHDFInformation(hdfPathName);
    for (int attrIdx = vtkDataObject::AttributeTypes::POINT;
         attrIdx <= vtkDataObject::AttributeTypes::CELL; ++attrIdx)
    {
      const std::vector<std::string> arrayNames = this->Impl->GetArrayNames(attrIdx);
      for (const std::string& arrayName : arrayNames)
      {
        this->DataArraySelection[attrIdx]->AddArray(arrayName.c_str());
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkHDFReader::ReadRecursively(
  vtkInformation* outInfo, vtkMultiBlockDataSet* dataMB, const std::string& path)
{
  this->Impl->OpenGroupAsVTKGroup(path);

  const std::vector<std::string> datasets = this->Impl->GetOrderedChildrenOfGroup(path);
  dataMB->SetNumberOfBlocks(static_cast<unsigned int>(datasets.size()));
  for (int i = 0; i < static_cast<int>(datasets.size()); i++)
  {
    const std::string& nodeName = datasets.at(i);
    const std::string hdfPath = path + "/" + nodeName;

    dataMB->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), nodeName);
    if (this->Impl->IsPathSoftLink(hdfPath))
    {
      if (!this->Impl->HasAttribute(hdfPath.c_str(), "Type"))
      {
        dataMB->SetBlock(i, nullptr);
        continue;
      }
      if (!this->Impl->RetrieveHDFInformation(hdfPath))
      {
        return 0;
      }
      this->Impl->OpenGroupAsVTKGroup(hdfPath); // Set current path as HDF5 root

      const int numPieces = this->Impl->GetNumberOfPieces(this->Step);
      const int datatype = this->Impl->GetDataSetType();

      vtkSmartPointer<vtkDataObject> dataObject = this->Impl->GetNewDataSet(datatype, numPieces);
      if (vtkPartitionedDataSet::SafeDownCast(dataObject))
      {
        dataObject.TakeReference(vtkMultiPieceDataSet::New());
      }
      this->CompositeCachePath = hdfPath;
      if (!this->ReadData(outInfo, dataObject))
      {
        return 0;
      }
      dataMB->SetBlock(i, dataObject);
      this->AddFieldArrays(dataMB->GetBlock(i));
    }
    else
    {
      // Node is not a leaf, recurse
      vtkNew<vtkMultiBlockDataSet> childGroup;
      dataMB->SetBlock(i, childGroup);
      this->ReadRecursively(outInfo, childGroup, hdfPath);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* vtkNotUsed(outInfo), vtkOverlappingAMR* data)
{
  double Origin[3];
  if (!this->Impl->GetAttribute("Origin", 3, Origin))
  {
    vtkErrorMacro("Could not get Origin attribute");
    return 0;
  }
  data->SetOrigin(Origin);

  unsigned int maxLevel = this->MaximumLevelsToReadByDefaultForAMR > 0
    ? this->MaximumLevelsToReadByDefaultForAMR
    : std::numeric_limits<unsigned int>::max();

  if (this->GetHasTemporalData())
  {
    if (!this->Impl->ComputeAMROffsetsPerLevels(this->DataArraySelection, this->Step, maxLevel))
    {
      return 0;
    }
  }
  else
  {
    if (!this->Impl->ComputeAMRBlocksPerLevels(maxLevel))
    {
      return 0;
    }
  }

  unsigned int level = 0;

  if (!this->Impl->ReadAMRTopology(data, level, maxLevel, Origin, this->GetHasTemporalData()))
  {
    return 1;
  }

  if (!this->Impl->ReadAMRData(
        data, level, maxLevel, this->DataArraySelection, this->GetHasTemporalData()))
  {
    return 1;
  }

  vtkAMRUtilities::BlankCells(data);

  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(
  vtkInformation* outInfo, vtkHyperTreeGrid* data, vtkPartitionedDataSet* pData)
{
  int filePieceCount = this->Impl->GetNumberOfPieces();
  if (this->GetHasTemporalData())
  {
    filePieceCount = this->Impl->GetNumberOfPieces(this->Step);
  }

  vtkIdType step = this->GetHasTemporalData() ? this->Step : -1;
  vtkHDFUtilities::TemporalHyperTreeGridOffsets htgTemporalOffsets(this->Impl, step);
  if (!htgTemporalOffsets.Success)
  {
    vtkErrorMacro("Error in reading temporal hyper tree grid offsets");
    return 0;
  }

  // Read NumberOfTrees, Cells and Depths
  std::vector<vtkIdType> numberOfTrees =
    this->Impl->GetMetadata("NumberOfTrees", filePieceCount, htgTemporalOffsets.PartOffset);
  if (numberOfTrees.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfCells =
    this->Impl->GetMetadata("NumberOfCells", filePieceCount, htgTemporalOffsets.PartOffset);
  if (numberOfCells.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfDepths =
    this->Impl->GetMetadata("NumberOfDepths", filePieceCount, htgTemporalOffsets.PartOffset);
  if (numberOfDepths.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> descriptorSizes =
    this->Impl->GetMetadata("DescriptorsSize", filePieceCount, htgTemporalOffsets.PartOffset);
  if (numberOfDepths.empty())
  {
    return 0;
  }

  int memoryPieceCount = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (memoryPieceCount == 0)
  {
    vtkErrorMacro("Number of pieces per process was set to 0");
    return 0;
  }
  std::vector<vtkSmartPointer<vtkHyperTreeGrid>> pieces;
  pieces.reserve(filePieceCount / memoryPieceCount);
  for (int filePiece = piece; filePiece < filePieceCount; filePiece += memoryPieceCount)
  {
    vtkNew<vtkHyperTreeGrid> pieceData;
    pieceData->Initialize();
    if (!this->Read(numberOfTrees, numberOfCells, numberOfDepths, descriptorSizes,
          htgTemporalOffsets, filePiece, pieceData))
    {
      return 0;
    }
    pieces.emplace_back(pieceData);
  }
  std::reverse(pieces.begin(), pieces.end());
  unsigned int nPieces = static_cast<unsigned int>(pieces.size());
  if (pData)
  {
    pData->Initialize();
    pData->SetNumberOfPartitions(nPieces);
    for (unsigned int iPiece = 0; iPiece < nPieces; ++iPiece)
    {
      pData->SetPartition(iPiece, pieces.back());
      pieces.pop_back();
    }
  }
  else if (data)
  {
    data->ShallowCopy(pieces[0]);
  }
  else
  {
    vtkErrorMacro("Both proposed outputs are nullptr.");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(const std::vector<vtkIdType>& numberOfTrees,
  const std::vector<vtkIdType>& numberOfCells, const std::vector<vtkIdType>& numberOfDepths,
  const std::vector<vtkIdType>& descriptorSizes,
  const vtkHDFUtilities::TemporalHyperTreeGridOffsets& htgTemporalOffsets, int filePiece,
  vtkHyperTreeGrid* pieceData)
{
  // Offsets are in bytes but sizes are in bits. New pieces always start
  auto sumByteSizes = [](vtkIdType startOffsetInBytes, vtkIdType addedBits)
  {
    return startOffsetInBytes + ((addedBits + 8 - 1) / 8); // Integer 'ceil'
  };

  // Get read offsets for the piece we are reading for the current time step
  // Add the offset for the time step to the number of cells/trees/etc. in previous partitions of
  // the current time step
  const vtkIdType cellOffset = std::accumulate(numberOfCells.data(), &numberOfCells[filePiece], 0);
  const vtkIdType treeIdsOffset = std::accumulate(
    numberOfTrees.data(), &numberOfTrees[filePiece], htgTemporalOffsets.TreeIdsOffset);
  const vtkIdType verticesPerDepthOffset = std::accumulate(numberOfDepths.data(),
    &numberOfDepths[filePiece], htgTemporalOffsets.NumberOfCellsPerTreeDepthOffset);
  const vtkIdType depthOffset = std::accumulate(
    numberOfTrees.data(), &numberOfTrees[filePiece], htgTemporalOffsets.DepthPerTreeOffset);
  const vtkIdType descriptorOffset = std::accumulate(descriptorSizes.data(),
    &descriptorSizes[filePiece], htgTemporalOffsets.DescriptorsOffset, sumByteSizes);
  const vtkIdType maskOffset = std::accumulate(
    numberOfCells.data(), &numberOfCells[filePiece], htgTemporalOffsets.MaskOffset, sumByteSizes);
  const vtkIdType partOffset = filePiece + htgTemporalOffsets.PartOffset;

  const vtkIdType depthLimit = this->MaximumLevelsToReadByDefaultForAMR > 0
    ? static_cast<vtkIdType>(this->MaximumLevelsToReadByDefaultForAMR)
    : std::numeric_limits<unsigned int>::max();

  // Build trees from descriptors
  if (!this->Impl->ReadHyperTreeGridData(pieceData,
        this->DataArraySelection[vtkDataObject::AttributeTypes::CELL], cellOffset, treeIdsOffset,
        depthOffset, descriptorOffset, maskOffset, partOffset, verticesPerDepthOffset, depthLimit,
        this->Step))
  {
    vtkErrorMacro("Failed to read HyperTreeGrid file");
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->Impl->Open(this->FileName))
  {
    return 0;
  }
  this->CompositeCachePath.clear();
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    return 0;
  }
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!output)
  {
    return 0;
  }

  bool result = ReadData(outInfo, output);

  if (this->GetHasTemporalData())
  {
    // do this at the end because using cache may override this.
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->TimeValue);
  }
  this->Impl->Close();
  return result ? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkHDFReader::ReadData(vtkInformation* outInfo, vtkDataObject* data)
{
  int ok = 1;
  this->MeshGeometryChangedFromPreviousTimeStep = false;

  if (this->GetHasTemporalData())
  {
    double* values = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (!values)
    {
      vtkErrorMacro("Expected TIME_STEPS key for temporal data");
      return false;
    }
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      double requestedValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      this->Step = std::distance(values,
                     std::upper_bound(values, values + this->NumberOfSteps, requestedValue)) -
        1;
      this->Step = this->Step >= this->NumberOfSteps ? this->NumberOfSteps - 1
                                                     : (this->Step < 0 ? 0 : this->Step);
      data->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->TimeValue);
    }
    this->TimeValue = values[this->Step];
  }

  int dataSetType = this->Impl->GetDataSetType();
  if (dataSetType == VTK_IMAGE_DATA)
  {
    vtkImageData* imageData = vtkImageData::SafeDownCast(data);
    ok = this->Read(outInfo, imageData);
  }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID)
  {
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(data);
    vtkPartitionedDataSet* pData = vtkPartitionedDataSet::SafeDownCast(data);
    ok = this->Read(outInfo, ug, pData);
    if (this->UseCache && this->CompositeCachePath.empty())
    {
      ::UpdateGeometryIfRequired(
        ug, pData, this->UseCache, this->MeshGeometryChangedFromPreviousTimeStep, this->MeshCache);
    }
    // data cleanup after using mesh cache
    if (pData && this->UseCache && this->MeshGeometryChangedFromPreviousTimeStep)
    {
      this->CleanOriginalIds(pData);
    }
  }
  else if (dataSetType == VTK_POLY_DATA)
  {
    vtkPolyData* polydata = vtkPolyData::SafeDownCast(data);
    vtkPartitionedDataSet* pData = vtkPartitionedDataSet::SafeDownCast(data);
    ok = this->Read(outInfo, polydata, pData);
    if (this->UseCache && this->CompositeCachePath.empty())
    {
      ::UpdateGeometryIfRequired(polydata, pData, this->UseCache,
        this->MeshGeometryChangedFromPreviousTimeStep, this->MeshCache);
    }
    // data cleanup after using mesh cache
    if (pData && this->UseCache && this->MeshGeometryChangedFromPreviousTimeStep)
    {
      this->CleanOriginalIds(pData);
    }
  }
  else if (dataSetType == VTK_OVERLAPPING_AMR)
  {
    vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(data);
    ok = this->Read(outInfo, amr);
  }
  else if (dataSetType == VTK_HYPER_TREE_GRID)
  {
    vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(data);
    vtkPartitionedDataSet* pData = vtkPartitionedDataSet::SafeDownCast(data);
    ok = this->Read(outInfo, htg, pData);
  }
  else if (dataSetType == VTK_PARTITIONED_DATA_SET_COLLECTION)
  {
    vtkPartitionedDataSetCollection* pdc = vtkPartitionedDataSetCollection::SafeDownCast(data);
    ok = this->Read(outInfo, pdc);
  }
  else if (dataSetType == VTK_MULTIBLOCK_DATA_SET)
  {
    vtkMultiBlockDataSet* mbds = vtkMultiBlockDataSet::SafeDownCast(data);
    ok = this->Read(outInfo, mbds);
  }
  else
  {
    vtkErrorMacro("HDF dataset type unknown: " << dataSetType);
    return false;
  }

  return ok && this->AddFieldArrays(data);
}

//----------------------------------------------------------------------------
bool vtkHDFReader::AddOriginalIds(
  vtkDataSetAttributes* attributes, vtkIdType size, const std::string& name)
{
  if (attributes->GetAbstractArray(name.c_str()) != nullptr)
  {
    return false; // An array with original ids (or at least the same name) shouldn't exist already.
  }
  vtkNew<vtkAffineArray<vtkIdType>> ids;
  ids->SetBackend(std::make_shared<vtkAffineImplicitBackend<vtkIdType>>(1, 0));
  ids->SetNumberOfTuples(size);
  ids->SetName(name.c_str());
  attributes->AddArray(ids);
  return true;
}

//----------------------------------------------------------------------------
std::string vtkHDFReader::GetAttributeOriginalIdName(vtkIdType attribute)
{
  return this->AttributesOriginalIdName.at(attribute);
}

//----------------------------------------------------------------------------
void vtkHDFReader::SetAttributeOriginalIdName(vtkIdType attribute, const std::string& name)
{
  this->AttributesOriginalIdName.emplace(attribute, name);
}

//----------------------------------------------------------------------------
void vtkHDFReader::CleanOriginalIds(vtkPartitionedDataSet* output)
{
  int attributesToClean[3] = { vtkDataObject::POINT, vtkDataObject::CELL, vtkDataObject::FIELD };

  for (unsigned int i = 0; i < output->GetNumberOfPartitions(); ++i)
  {
    vtkDataObject* partition = output->GetPartitionAsDataObject(i);

    for (int attributeType : attributesToClean)
    {
      std::string arrayName = this->GetAttributeOriginalIdName(attributeType);
      vtkDataSetAttributes* attributes = partition->GetAttributes(attributeType);
      if (attributes && attributes->GetArray(arrayName.c_str()))
      {
        attributes->RemoveArray(arrayName.c_str());
      }
    }
  }
}

VTK_ABI_NAMESPACE_END
