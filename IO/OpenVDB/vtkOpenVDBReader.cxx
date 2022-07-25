#include "vtkOpenVDBReader.h"

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkType.h"

#include <openvdb/Types.h>
#include <openvdb/openvdb.h>
#include <openvdb/points/PointCount.h>

vtkStandardNewMacro(vtkOpenVDBReader);

//------------------------------------------------------------------------
/*
This structure stores information about one grid, directly extracted
from its metadata.
*/
struct OpenVDBGridInformation
{
  // describes whether the grid is an image volume, a point cloud,
  // or an unsupported format
  enum class DataFormatType : int
  {
    UNKNOWN,
    IMAGE_DATA,
    POINT_CLOUD,
  };

  // index of the grid inside the file
  int GridIdx;
  // pointer to the abstract grid
  openvdb::GridBase::Ptr Grid;

  // both are in index space
  int BBoxMin[3];
  int BBoxMax[3];
  // world distance of a voxel cube
  double Spacing[3];
  double WorldOrigin[3];
  // name of the grid
  std::string Name;

  // number of points. only relevant for point clouds
  unsigned long PointsCount = 0;

  // vtk type of the data
  int ScalarType = VTK_FLOAT;
  // only 1 and 3 components are supported (by the standard types of openvdb)
  int NumComps = 1;

  // we only support uniform scales
  // (i.e. uniform and cubic voxels)
  bool UniformScale = true;

  DataFormatType DataFormat = DataFormatType::UNKNOWN;

  static std::string DataTypeToString(DataFormatType dataType)
  {
    constexpr const char* dataTypeDescription[3] = { "unsupported", "image", "point cloud" };

    return dataTypeDescription[static_cast<int>(dataType)];
  }
};

//------------------------------------------------------------------------
/*
As a single vtk block can correspond to several grids, we create another structure
to encapsulate the logic behind the merging of several grids, and the creation of
the vtk objects.
A vtkResDataLeafInformation corresponds directly to a block in the resulting vtkMultiBlockDataSet.
*/
struct vtkResDataLeafInformation
{
  // set during the initialization of the struct
  // -------------------------------------------

  // the grid indices requested for this block
  std::vector<unsigned int> GridIndices;
  float DownsamplingFactor = 1.0;

  // set during the data fetching from the grids
  // -------------------------------------------

  struct ArrayInfo
  {
    std::string Name;
    int vtkType;
    int numComps;
  };
  unsigned int NumberGrids = 0;
  // information about the different arrays of this block (one per grid)
  std::vector<ArrayInfo> Arrays;
  // the grids information
  std::vector<OpenVDBGridInformation*> GridsInfo;
  // the dataformat of the grids inside the block
  // (a block can not reference heterogeneous grids !)
  OpenVDBGridInformation::DataFormatType DataFormat;

  // computed internally with the grids information. correspond to the exported data set information
  // -----------------------------------------------------------------------------------------------

  // geometric information about the dataset
  // only relevant when it results in a vtkImageData
  int BBoxMin[3], BBoxMax[3], Dimensions[3];
  double Spacing[3], Origin[3];
  // number of points of the dataset
  // only relevant when it results in a vtkPolyData
  vtkIdType NumPoints;

  // this function fills the necessary information from the openvdb grid information
  bool FetchGridsInformation(std::vector<OpenVDBGridInformation>& gridsInfo);
  // this function does the geometrical part of computing the bbox, spacing, origins etc
  // from what was collected
  bool ComputeDatasetInformation();

  // functions to fill the different arrays by sampling the OpenVDB grids
  void PopulatePolyData(vtkPolyData* polydata) const;
  void PopulateImageData(vtkImageData* imagedata) const;
};

//------------------------------------------------------------------------
// Fills the vtkResDataLeafInformation information according to the
// grids metadata.
bool vtkResDataLeafInformation::FetchGridsInformation(
  std::vector<OpenVDBGridInformation>& gridsInfo)
{
  this->NumberGrids = this->GridIndices.size();
  if (this->GridIndices.empty() || gridsInfo.empty())
  {
    // shouldn't have to handle empty structs
    return false;
  }

  // take the type of the first grid
  this->DataFormat = gridsInfo[this->GridIndices[0]].DataFormat;

  for (int gridIdx : this->GridIndices)
  {
    ArrayInfo arrayInfo;
    OpenVDBGridInformation* gridInfo = &(gridsInfo[gridIdx]);

    arrayInfo.Name = gridInfo->Name;
    arrayInfo.numComps = gridInfo->NumComps;
    arrayInfo.vtkType = gridInfo->ScalarType;

    this->Arrays.emplace_back(arrayInfo);
    this->GridsInfo.emplace_back(gridInfo);

    if (gridInfo->DataFormat != this->DataFormat)
    {
      // heterogeneous collection of grids: shouldn't happen
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------
// Compute the information (dimension, spacing, number of points, etc)
// of the resulting vtk dataset
bool vtkResDataLeafInformation::ComputeDatasetInformation()
{
  if (this->DataFormat == OpenVDBGridInformation::DataFormatType::UNKNOWN)
  {
    return false;
  }

  for (int s = 0; s < 3; s++)
  {
    this->BBoxMin[s] = VTK_INT_MAX;
    this->BBoxMax[s] = VTK_INT_MIN;
    this->Spacing[s] = VTK_DOUBLE_MAX;
    this->Origin[s] = VTK_DOUBLE_MAX;
  }

  this->NumPoints = 0;

  for (const auto& gridInfo : this->GridsInfo)
  {
    for (int s = 0; s < 3; s++)
    {
      // the resulting bounding box is the union of all the requested bounding boxes
      this->BBoxMin[s] = std::min(this->BBoxMin[s], gridInfo->BBoxMin[s]);
      this->BBoxMax[s] = std::max(this->BBoxMax[s], gridInfo->BBoxMax[s]);
      // the spacing is the smallest spacing
      this->Spacing[s] = std::min(this->Spacing[s], gridInfo->Spacing[s]);
      // the origin is the minimal origin
      this->Origin[s] = std::min(this->Origin[s], gridInfo->WorldOrigin[s]);
    }
    // and we sum the numbers of points
    this->NumPoints += gridInfo->PointsCount;
  }

  for (int s = 0; s < 3; s++)
  {
    // apply the downsampling factor
    this->Dimensions[s] = (this->BBoxMax[s] - this->BBoxMin[s]) * this->DownsamplingFactor;
    this->Spacing[s] /= this->DownsamplingFactor;
  }

  return true;
}

namespace
{
//------------------------------------------------------------------------
template <vtkIdType NComps, typename GridType, typename ArrayType>
struct SamplerVdbGrid
{
  static inline void SampleVdbGrid(openvdb::Coord vtkNotUsed(ijk),
    typename GridType::Accessor vtkNotUsed(accessor), ArrayType* vtkNotUsed(dataArray),
    vtkIdType vtkNotUsed(idx)){};
};

//------------------------------------------------------------------------
template <typename GridType, typename ArrayType>
struct SamplerVdbGrid<1, GridType, ArrayType>
{
  static inline void SampleVdbGrid(
    openvdb::Coord ijk, typename GridType::Accessor accessor, ArrayType* dataArray, vtkIdType idx)
  {
    dataArray->SetTuple1(idx, accessor.getValue(ijk));
  }
};

//------------------------------------------------------------------------
template <typename GridType, typename ArrayType>
struct SamplerVdbGrid<3, GridType, ArrayType>
{
  static inline void SampleVdbGrid(
    openvdb::Coord ijk, typename GridType::Accessor accessor, ArrayType* dataArray, vtkIdType idx)
  {
    typename GridType::ValueType val = accessor.getValue(ijk);
    dataArray->SetTuple3(idx, val[0], val[1], val[2]);
  }
};

//------------------------------------------------------------------------
// fills the vtkPolyData points with the points of one OpenVDB grid
// firstIdx corresponds to the current point index inside the polydata
// it returns the index of the last added point
int PopulatePointSet(openvdb::points::PointDataGrid::Ptr grid, vtkPolyData* polydata, int firstIdx)
{
  vtkPoints* points = polydata->GetPoints();
  int currentIdx = firstIdx;

  for (auto leafIter = grid->tree().cbeginLeaf(); leafIter; ++leafIter)
  {
    // extract the voxel
    const openvdb::points::AttributeArray& array = leafIter->constAttributeArray("P");
    openvdb::points::AttributeHandle<openvdb::Vec3f> positionHandle(array);
    for (auto indexIter = leafIter->beginIndexOn(); indexIter; ++indexIter)
    {
      // Code extract from OpenVDB Cookbook
      // Extract the voxel-space position of the point.
      openvdb::Vec3f voxelPosition = positionHandle.get(*indexIter);
      // Extract the index-space position of the voxel.
      const openvdb::Vec3d xyz = indexIter.getCoord().asVec3d();
      // Compute the world-space position of the point.
      openvdb::Vec3f worldPosition = grid->transform().indexToWorld(voxelPosition + xyz);

      // add the point
      points->SetPoint(currentIdx, worldPosition[0], worldPosition[1], worldPosition[2]);
      currentIdx++;
    }
  }
  return currentIdx;
}

//------------------------------------------------------------------------
// functor to fill an array of a vtkImageData
// this additional logic is due to the template constraints of an OpenVDB grid.
struct PopulateImageDataArray
{
  // image data to fill
  vtkImageData* imagedata = nullptr;
  // dataset information of the imagedata
  const vtkResDataLeafInformation* dataInfo = nullptr;

  // grid is the OpenVDB grid to sample, dataArray is the vtk array to fill
  template <vtkIdType NComps, typename GridType, typename ArrayType>
  void operator()(typename GridType::Ptr grid, ArrayType* dataArray)
  {
    if (grid == nullptr || dataArray == nullptr || imagedata == nullptr || dataInfo == nullptr)
    {
      // we need the grid, the vtk array, the image data, and the block information
      // to populate the imagedata
      return;
    }

    // get the dimensions of the vtkImageData
    int imgDims[3];
    imagedata->GetDimensions(imgDims);

    if (imgDims[0] <= 0 || imgDims[1] <= 0 || imgDims[2] <= 0)
    {
      return;
    }

    const int maxSubIdx = imgDims[0] * imgDims[1];
    const int maxIdx = maxSubIdx * imgDims[2];

    vtkSMPTools::For(
      0, maxIdx, [this, maxSubIdx, &imgDims, grid, dataArray](vtkIdType idx, vtkIdType endIdx) {
        typename GridType::Accessor accessor = grid->getAccessor();
        float downsamplingFactor = this->dataInfo->DownsamplingFactor;

        for (; idx < endIdx; ++idx)
        {
          int i, j, k, t;
          openvdb::Coord ijk;
          k = idx / maxSubIdx;
          t = idx % maxSubIdx;
          j = t / imgDims[0];
          i = t % imgDims[0];
          // ijk is the sampling location in the OpenVDB grid
          ijk.reset(i / downsamplingFactor + dataInfo->BBoxMin[0],
            j / downsamplingFactor + dataInfo->BBoxMin[1],
            k / downsamplingFactor + dataInfo->BBoxMin[2]);
          ::SamplerVdbGrid<NComps, GridType, ArrayType>::SampleVdbGrid(
            ijk, accessor, dataArray, idx);
        }
      });
  }
};

//------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> InstanciateVtkArrayType(openvdb::GridBase::Ptr grid)
{
  // instanciate a vtkDataArray of the correct type according to the OpenVDB grid type
  if (grid->isType<openvdb::BoolGrid>())
  {
    return vtkSmartPointer<vtkCharArray>::New();
  }
  else if (grid->isType<openvdb::FloatGrid>())
  {
    return vtkSmartPointer<vtkFloatArray>::New();
  }
  else if (grid->isType<openvdb::DoubleGrid>())
  {
    return vtkSmartPointer<vtkDoubleArray>::New();
  }
  else if (grid->isType<openvdb::Int32Grid>())
  {
    return vtkSmartPointer<vtkIntArray>::New();
  }
  else if (grid->isType<openvdb::Int64Grid>())
  {
    return vtkSmartPointer<vtkLongArray>::New();
  }
  else if (grid->isType<openvdb::Vec3IGrid>())
  {
    return vtkSmartPointer<vtkIntArray>::New();
  }
  else if (grid->isType<openvdb::Vec3SGrid>())
  {
    return vtkSmartPointer<vtkFloatArray>::New();
  }
  else if (grid->isType<openvdb::Vec3DGrid>())
  {
    return vtkSmartPointer<vtkDoubleArray>::New();
  }

  return nullptr;
}

//------------------------------------------------------------------------
// This utility function, inspired by the OpenVDB Cookbook, calls the correct
// templated fill functor above according to the grid's type
template <typename OpType>
void processTypedGridArray(openvdb::GridBase::Ptr grid, vtkAbstractArray* dataArray, OpType& op)
{
#define CALL_OP(NComps, GridType, ArrayType)                                                       \
  op.template operator()<NComps, GridType, ArrayType>(                                             \
    openvdb::gridPtrCast<GridType>(grid), ArrayType::SafeDownCast(dataArray))

  if (grid->isType<openvdb::BoolGrid>())
  {
    CALL_OP(1, openvdb::BoolGrid, vtkCharArray);
  }
  else if (grid->isType<openvdb::FloatGrid>())
  {
    CALL_OP(1, openvdb::FloatGrid, vtkFloatArray);
  }
  else if (grid->isType<openvdb::DoubleGrid>())
  {
    CALL_OP(1, openvdb::DoubleGrid, vtkDoubleArray);
  }
  else if (grid->isType<openvdb::Int32Grid>())
  {
    CALL_OP(1, openvdb::Int32Grid, vtkIntArray);
  }
  else if (grid->isType<openvdb::Int64Grid>())
  {
    CALL_OP(1, openvdb::Int64Grid, vtkLongArray);
  }
  else if (grid->isType<openvdb::Vec3IGrid>())
  {
    CALL_OP(3, openvdb::Vec3IGrid, vtkIntArray);
  }
  else if (grid->isType<openvdb::Vec3SGrid>())
  {
    CALL_OP(3, openvdb::Vec3SGrid, vtkFloatArray);
  }
  else if (grid->isType<openvdb::Vec3DGrid>())
  {
    CALL_OP(3, openvdb::Vec3DGrid, vtkDoubleArray);
  }
#undef CALL_OP
}
};

//------------------------------------------------------------------------
// Populates the polydata according to the grids in the vtkResDataLeafInformation
// it essentially calls PopulatePointSet for each grid
void vtkResDataLeafInformation::PopulatePolyData(vtkPolyData* polydata) const
{
  int pointIdx = 0;
  for (const auto& gridInfo : this->GridsInfo)
  {
    openvdb::points::PointDataGrid::Ptr grid =
      openvdb::gridPtrCast<openvdb::points::PointDataGrid>(gridInfo->Grid);
    pointIdx = ::PopulatePointSet(grid, polydata, pointIdx);
  }
}

//------------------------------------------------------------------------
// Populates the vtkImageData for each grid in the vtkResDataLeafInformation
// It essentially calls PopulateImageDataArray1D/3D for each grid
void vtkResDataLeafInformation::PopulateImageData(vtkImageData* imagedata) const
{
  for (unsigned int arrayIdx = 0; arrayIdx < this->NumberGrids; arrayIdx++)
  {
    vtkAbstractArray* dataArray = imagedata->GetPointData()->GetAbstractArray(arrayIdx);

    if (dataArray == nullptr)
    {
      continue;
    }
    OpenVDBGridInformation* gridInfo = this->GridsInfo[arrayIdx];
    // take the OpenVDB grid
    openvdb::GridBase::Ptr grid = gridInfo->Grid;

    PopulateImageDataArray populateArray;
    populateArray.dataInfo = this;
    populateArray.imagedata = imagedata;
    ::processTypedGridArray(grid, dataArray, populateArray);
  }
}

//------------------------------------------------------------------------
class vtkOpenVDBReaderInternals
{
public:
  vtkOpenVDBReaderInternals(vtkOpenVDBReader* self) { this->Parent = self; };

  ~vtkOpenVDBReaderInternals() { delete CurrentlyOpenedFile.File; };

  vtkOpenVDBReader* Parent;

  struct VdbFileContext
  {
    openvdb::io::File* File = nullptr;
    std::string FileName;
  };

  struct VdbFileContext CurrentlyOpenedFile;
  // collection of the OpenVDB metadata pointers (one for each grid)
  openvdb::GridPtrVecPtr GridsVdbMetadata = nullptr;
  // collection of the OpenVDBGridInformation (one for each grid)
  std::vector<OpenVDBGridInformation> GridsInformation;

  bool ArraysNeedUpdate = true;

  OpenVDBGridInformation& GetGridInformation(int idx);
  VdbFileContext OpenFile(const char* fileName);
  void ResetCurrentlyOpenedFile(const char* newFileName);
  void ConstructGridsInformation();
  bool ValidateGridInformation(OpenVDBGridInformation& gridInfo);
  // this function updates the information of an OpenVDBGridInformation, given the
  // pointer to the corresponding abstract grid
  // it reads its metadata and interprets them
  void UpdateGridInformation(struct OpenVDBGridInformation*, openvdb::GridBase::Ptr grid);

  // this functions makes sure that all the information about a grid are up-to-date
  // it updates the information that can be missing, because they were not avaible in the
  // grid's metadata.
  // therefore, it should only be called when grid represents a grid that is FULLY loaded into
  // memory
  void UpdateMissingGridInformation(openvdb::GridBase::Ptr grid, OpenVDBGridInformation* gridInfo);

  // translates the requested grid names into grid indices
  std::vector<unsigned int> GetRequestedGridIdx();
};

//------------------------------------------------------------------------
vtkOpenVDBReaderInternals::VdbFileContext vtkOpenVDBReaderInternals::OpenFile(const char* fileName)
{
  struct VdbFileContext resCtx;

  resCtx.File = new openvdb::io::File(fileName);

  try
  {
    // Note that opening the file only loads the grids information, not the data itself
    resCtx.File->open();
    resCtx.FileName = fileName;
  }
  catch (openvdb::IoError& e)
  {
    // happens when the file doesn't exist or when it is not a VDB file
    vtkErrorWithObjectMacro(
      this->Parent, << "Error while opening file " << fileName << ": " << e.what());
    resCtx.File = nullptr;
    resCtx.FileName = "";
  }

  return resCtx;
}

//------------------------------------------------------------------------
void vtkOpenVDBReaderInternals::ResetCurrentlyOpenedFile(const char* newFileName)
{
  if (this->CurrentlyOpenedFile.File != nullptr && this->CurrentlyOpenedFile.File->isOpen() &&
    this->CurrentlyOpenedFile.FileName != newFileName)
  {
    // another already opened file, so we close the current one
    this->CurrentlyOpenedFile.File->close();
    this->ArraysNeedUpdate = true;
  }
  delete this->CurrentlyOpenedFile.File;
  this->CurrentlyOpenedFile.FileName.erase();
}

//------------------------------------------------------------------------
void vtkOpenVDBReaderInternals::ConstructGridsInformation()
{
  // GridsVdbMetadata contains pointer to each metadata pointer
  for (size_t gridIdx = 0; gridIdx < this->GridsVdbMetadata->size(); gridIdx++)
  {
    OpenVDBGridInformation gridInformation;
    openvdb::GridBase::Ptr gridBase = (*this->GridsVdbMetadata)[gridIdx];
    gridInformation.GridIdx = static_cast<int>(gridIdx);
    this->UpdateGridInformation(&gridInformation, gridBase);
    if (!this->ValidateGridInformation(gridInformation))
    {
      vtkWarningWithObjectMacro(
        this->Parent, << "Grid " << gridInformation.Name << " is unsupported. Discarding it.");
      continue;
    }
    this->GridsInformation.emplace_back(gridInformation);
  }
}

//------------------------------------------------------------------------
OpenVDBGridInformation& vtkOpenVDBReaderInternals::GetGridInformation(int numberGrid)
{
  // we shouldn't call this internal function with an out-of-range index anyway
  assert(this->Parent->NumberOfGrids() > 0);
  if (numberGrid < 0 || numberGrid >= this->Parent->NumberOfGrids())
  {
    vtkWarningWithObjectMacro(
      this->Parent, << "Implementation error, trying to access an out-of-range grid.");
    return this->GridsInformation[0];
  }
  return this->GridsInformation[numberGrid];
}

//------------------------------------------------------------------------
void vtkOpenVDBReaderInternals::UpdateGridInformation(
  struct OpenVDBGridInformation* gridInfo, openvdb::GridBase::Ptr grid)
{
  if (grid == nullptr)
  {
    return;
  }

  gridInfo->Name = grid->getName();
  // go through metadata
  openvdb::Vec3i bBoxMin;
  openvdb::Vec3i bBoxMax;
  openvdb::Vec3d worldOrig;
  try
  {
    // this is a standard convention, but we're not sure it is actually set
    bBoxMin = grid->template metaValue<openvdb::Vec3i>("file_bbox_min");
    bBoxMax = grid->template metaValue<openvdb::Vec3i>("file_bbox_max");

    openvdb::Coord bboxMinCoord;
    bboxMinCoord.reset(bBoxMin[0], bBoxMin[1], bBoxMin[2]);

    // origin in world coordinates
    worldOrig = grid->indexToWorld(bboxMinCoord);
  }
  catch (const openvdb::Exception& e)
  {
    // two different exceptions can happen: either the field is unknown,
    // either it has incorrect type. in both cases, we have this fallback
    // it will probably give [MAX_COORDS, MIN_COORDS] BBox, so we don't try
    // to compute the origin
    openvdb::CoordBBox defaultBBox = grid->evalActiveVoxelBoundingBox();
    openvdb::Coord defaultMin = defaultBBox.min();
    openvdb::Coord defaultMax = defaultBBox.max();
    bBoxMin.init(defaultMin[0], defaultMin[1], defaultMin[2]);
    bBoxMax.init(defaultMax[0], defaultMax[1], defaultMax[2]);
    worldOrig.init(); // 0, 0, 0
  }

  // spacing
  openvdb::Vec3d voxSpacing = grid->voxelSize();

  for (int s = 0; s < 3; s++)
  {
    gridInfo->BBoxMin[s] = bBoxMin[s];
    gridInfo->BBoxMax[s] = bBoxMax[s];
    gridInfo->Spacing[s] = voxSpacing[s];
    gridInfo->WorldOrigin[s] = worldOrig[s];
  }

  gridInfo->UniformScale = grid->hasUniformVoxels();

  // Get grid type
  if (grid->isType<openvdb::BoolGrid>())
  {
    gridInfo->ScalarType = VTK_CHAR;
    gridInfo->NumComps = 1;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::FloatGrid>())
  {
    gridInfo->ScalarType = VTK_FLOAT;
    gridInfo->NumComps = 1;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::DoubleGrid>())
  {
    gridInfo->ScalarType = VTK_DOUBLE;
    gridInfo->NumComps = 1;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::Int32Grid>())
  {
    gridInfo->ScalarType = VTK_INT;
    gridInfo->NumComps = 1;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::Int64Grid>())
  {
    gridInfo->ScalarType = VTK_LONG;
    gridInfo->NumComps = 1;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::Vec3IGrid>())
  {
    gridInfo->ScalarType = VTK_INT;
    gridInfo->NumComps = 3;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::Vec3SGrid>())
  {
    gridInfo->ScalarType = VTK_FLOAT;
    gridInfo->NumComps = 3;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::Vec3DGrid>())
  {
    gridInfo->ScalarType = VTK_DOUBLE;
    gridInfo->NumComps = 3;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::IMAGE_DATA;
  }
  else if (grid->isType<openvdb::points::PointDataGrid>())
  {
    // only one type for point clouds
    gridInfo->ScalarType = VTK_FLOAT;
    gridInfo->NumComps = 1;
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::POINT_CLOUD;
  }
  else
  {
    // unsupported grid format
    gridInfo->DataFormat = OpenVDBGridInformation::DataFormatType::UNKNOWN;
  }
}

//------------------------------------------------------------------------
void vtkOpenVDBReaderInternals::UpdateMissingGridInformation(
  openvdb::GridBase::Ptr grid, OpenVDBGridInformation* gridInfo)
{
  if (gridInfo->DataFormat == OpenVDBGridInformation::DataFormatType::POINT_CLOUD)
  {
    // we load the number of points
    // we are sure that they were not loaded
    openvdb::points::PointDataGrid::Ptr gridPoints =
      openvdb::gridPtrCast<openvdb::points::PointDataGrid>(gridInfo->Grid);
    gridInfo->PointsCount = openvdb::points::pointCount(gridPoints->tree());
  }
  else if (gridInfo->DataFormat == OpenVDBGridInformation::DataFormatType::IMAGE_DATA)
  {
    // check whether they were already loaded or not
    for (int s = 0; s < 3; s++)
    {
      if (gridInfo->BBoxMin[s] != 0 || gridInfo->BBoxMax[s] != 0)
      {
        // data was already loaded, we can leave
        return;
      }
    }

    // we have to handle the bbox and the origin
    openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();
    openvdb::Coord bBoxMin = bbox.min();
    openvdb::Coord bBoxMax = bbox.max();
    openvdb::Vec3d worldOrig = grid->indexToWorld(bBoxMin);

    for (int s = 0; s < 3; s++)
    {
      gridInfo->BBoxMin[s] = bBoxMin[s];
      gridInfo->BBoxMax[s] = bBoxMax[s];
      gridInfo->WorldOrigin[s] = worldOrig[s];
    }
  }
}

//------------------------------------------------------------------------
bool vtkOpenVDBReaderInternals::ValidateGridInformation(OpenVDBGridInformation& gridInfo)
{
  if (!gridInfo.UniformScale)
  {
    // It would be possible to have non-uniform but parallelepiped voxels, by using the ImageData
    // 'Direction' matrix and the identifying it with the grid's Transformation matrix
    vtkErrorWithObjectMacro(
      this->Parent, << "Grid '" << gridInfo.Name
                    << "' doesn't have a uniform scale. It is not supported for now.");
    // we store that we can't support it
    return false;
  }

  if (gridInfo.Spacing[0] <= 0 || gridInfo.Spacing[1] <= 0 || gridInfo.Spacing[2] <= 0)
  {
    vtkErrorWithObjectMacro(this->Parent,
      << "Grid '" << gridInfo.Name << "' has incorrect spacing: [" << gridInfo.Spacing[0] << " , "
      << gridInfo.Spacing[1] << ", " << gridInfo.Spacing[2] << "]");
    // this should be known at metadata time
    return false;
  }

  if (gridInfo.BBoxMax[0] <= gridInfo.BBoxMin[0] || gridInfo.BBoxMax[1] <= gridInfo.BBoxMin[1] ||
    gridInfo.BBoxMax[2] <= gridInfo.BBoxMin[2])
  {
    // can happen if the metadata doesn't contain the bbox information.
    // we have to wait until the tree is loaded to have those information
    for (int s = 0; s < 3; s++)
    {
      // reinit to 0, so that we know that it has standard value
      gridInfo.BBoxMin[s] = 0;
      gridInfo.BBoxMax[s] = 0;
    }
  }
  return true;
}

//------------------------------------------------------------------------
std::vector<unsigned int> vtkOpenVDBReaderInternals::GetRequestedGridIdx()
{
  // get the requested names and translate them into grid indices
  std::vector<unsigned int> reqIdx;
  vtkDataArraySelection* gridSelection = this->Parent->GetGridSelection();
  for (const auto& gridInfo : this->GridsInformation)
  {
    std::string gridName = gridInfo.Name;
    gridName += " (" + OpenVDBGridInformation::DataTypeToString(gridInfo.DataFormat) + ")";
    if (gridSelection->ArrayIsEnabled(gridName.c_str()))
    {
      reqIdx.emplace_back(gridInfo.GridIdx);
    }
  }
  std::sort(reqIdx.begin(), reqIdx.end());
  // ensure there's no duplicates
  reqIdx.erase(std::unique(reqIdx.begin(), reqIdx.end()), reqIdx.end());
  return reqIdx;
}

//------------------------------------------------------------------------
vtkOpenVDBReader::vtkOpenVDBReader()
  : Internals(new vtkOpenVDBReaderInternals(this))
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  openvdb::initialize();
}

//------------------------------------------------------------------------
bool vtkOpenVDBReader::LoadFile()
{
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "No file name has been set.");
    return false;
  }

  this->Internals->ResetCurrentlyOpenedFile(this->FileName);

  this->DataCorrect = true;

  vtkOpenVDBReaderInternals::VdbFileContext resCtx = this->Internals->OpenFile(this->FileName);

  if (resCtx.File == nullptr)
  {
    return false;
  }

  // then try to read the metadata
  try
  {
    // it creates the pointers to the metadata of the grids
    this->Internals->GridsVdbMetadata = resCtx.File->readAllGridMetadata();
  }
  catch (openvdb::IoError& e)
  {
    vtkErrorMacro(<< "Error while loading metadata from " << this->FileName << ": " << e.what());
    resCtx.File = nullptr;
    resCtx.FileName = "";
    return false;
  }

  // if everything went well, copy it into CurrentlyOpenedFile
  this->Internals->CurrentlyOpenedFile.File = resCtx.File;
  this->Internals->CurrentlyOpenedFile.FileName = resCtx.FileName;

  return true;
}

//------------------------------------------------------------------------
bool vtkOpenVDBReader::CanReadFile(const char* fileName)
{
  // try to open the file and look at the return code
  vtkOpenVDBReaderInternals::VdbFileContext resCtx = this->Internals->OpenFile(fileName);
  return resCtx.File != nullptr;
}

//------------------------------------------------------------------------
void vtkOpenVDBReader::InitializeData()
{
  // load the file
  if (!this->LoadFile())
  {
    this->DataCorrect = false;
    return;
  }

  // if everything went well, extract the grids information
  this->Internals->ConstructGridsInformation();
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // open the file, the metadata, construct the information we can already have
  this->InitializeData();

  if (!this->DataCorrect)
  {
    vtkErrorMacro(<< "An error occured while reading the file.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 0);

  if (this->Internals->ArraysNeedUpdate)
  {
    // reset the grid selection arrays
    this->GridSelection->RemoveAllArrays();

    for (int i = 0; i < this->NumberOfGrids(); i++)
    {
      // By default, every grid is requested
      OpenVDBGridInformation gridInfo = this->Internals->GetGridInformation(i);
      std::string tableName = gridInfo.Name;
      tableName += " (" + OpenVDBGridInformation::DataTypeToString(gridInfo.DataFormat) + ")";
      this->GridSelection->AddArray(tableName.c_str(), true);
    }

    this->Internals->ArraysNeedUpdate = false;
  }

  return 1;
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    output = vtkPartitionedDataSetCollection::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
  }
  return 1;
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->DataCorrect)
  {
    vtkErrorMacro(<< "An error occured while reading the file.");
    return 0;
  }

  auto output =
    vtkPartitionedDataSetCollection::SafeDownCast(vtkDataObject::GetData(outputVector, 0));

  std::vector<unsigned int> reqGridsImage;
  std::vector<unsigned int> reqGridsPoints;

  // we sort the requested grids indices into the image grids and the point grids
  // (and detect if there are unsupported grids)
  const std::vector<unsigned int>& reqGrids = this->Internals->GetRequestedGridIdx();
  for (const unsigned int& gridIdx : reqGrids)
  {
    OpenVDBGridInformation& gridInfo = this->Internals->GetGridInformation(gridIdx);
    // this is where the grid's data is actually loaded
    gridInfo.Grid = this->Internals->CurrentlyOpenedFile.File->readGrid(gridInfo.Name);
    if (gridInfo.Grid == nullptr)
    {
      vtkErrorMacro(<< "Unknown requested grid name: " << gridInfo.Name);
      return 0;
    }
    if (gridInfo.DataFormat == OpenVDBGridInformation::DataFormatType::IMAGE_DATA)
    {
      reqGridsImage.emplace_back(gridIdx);
    }
    else if (gridInfo.DataFormat == OpenVDBGridInformation::DataFormatType::POINT_CLOUD)
    {
      reqGridsPoints.emplace_back(gridIdx);
    }
    else
    {
      vtkErrorMacro(<< "Incompatible requested grid type: " << gridInfo.Name);
      return 0;
    }

    // ensure that the grid has all its information up-to-date
    this->Internals->UpdateMissingGridInformation(gridInfo.Grid, &gridInfo);
  }

  // now we construct the vtkResDataLeafInformation
  std::vector<vtkResDataLeafInformation> imgDatasetsInfo;
  std::vector<vtkResDataLeafInformation> pointsDatasetsInfo;

  if (!reqGridsImage.empty())
  {
    // if we merge the image grids, there is only one vtkImageData,
    // with every requested grid inside
    if (this->MergeImageVolumes)
    {
      vtkResDataLeafInformation mergedDataInfo;
      mergedDataInfo.GridIndices = reqGridsImage;
      mergedDataInfo.DownsamplingFactor = this->DownsamplingFactor;
      imgDatasetsInfo.emplace_back(mergedDataInfo);
    }
    // otherwise, there is one vtkImageData per requested grid
    else
    {
      for (const unsigned int& gridIdx : reqGridsImage)
      {
        vtkResDataLeafInformation dataInfo;
        dataInfo.GridIndices.emplace_back(gridIdx);
        dataInfo.DownsamplingFactor = this->DownsamplingFactor;
        imgDatasetsInfo.emplace_back(dataInfo);
      }
    }
  }

  if (!reqGridsPoints.empty())
  {
    // same for points grids
    if (this->MergePointSets)
    {
      vtkResDataLeafInformation mergedDataInfo;
      mergedDataInfo.GridIndices = reqGridsPoints;
      mergedDataInfo.DownsamplingFactor = this->DownsamplingFactor;
      pointsDatasetsInfo.emplace_back(mergedDataInfo);
    }
    else
    {
      for (const unsigned int& gridIdx : reqGridsPoints)
      {
        vtkResDataLeafInformation dataInfo;
        dataInfo.GridIndices.emplace_back(gridIdx);
        dataInfo.DownsamplingFactor = this->DownsamplingFactor;
        pointsDatasetsInfo.emplace_back(dataInfo);
      }
    }
  }

  // now we construct the information about the vtkResDataLeafInformation
  // about the images
  for (auto& dataInfo : imgDatasetsInfo)
  {
    if (!dataInfo.FetchGridsInformation(this->Internals->GridsInformation))
    {
      vtkErrorMacro(<< "Couldn't fetch the information from the data grids.");
      return 0;
    }
    if (!dataInfo.ComputeDatasetInformation())
    {
      vtkErrorMacro(<< "Could compute the datasets information.");
      return 0;
    }
  }
  // and about the point clouds
  for (auto& dataInfo : pointsDatasetsInfo)
  {
    if (!dataInfo.FetchGridsInformation(this->Internals->GridsInformation))
    {
      vtkErrorMacro(<< "Couldn't fetch the information from the data grids.");
      return 0;
    }
    if (!dataInfo.ComputeDatasetInformation())
    {
      vtkErrorMacro(<< "Couln't compute the datasets information.");
      return 0;
    }
  }

  // one block per vtkResDataLeafInformation
  output->SetNumberOfPartitionedDataSets(imgDatasetsInfo.size() + pointsDatasetsInfo.size());
  for (unsigned int blockidx = 0; blockidx < output->GetNumberOfPartitionedDataSets(); blockidx++)
  {
    output->SetNumberOfPartitions(blockidx, 1);
  }

  int leafIdx = 0;
  int numberImages = imgDatasetsInfo.size();
  // images first, points after
  for (const auto& imgDataInfo : imgDatasetsInfo)
  {
    vtkNew<vtkImageData> imgData;
    imgData->SetDimensions(imgDataInfo.Dimensions);
    imgData->SetSpacing(imgDataInfo.Spacing);
    imgData->SetOrigin(imgDataInfo.Origin);

    // inside each vtkResDataLeafInformation, there is one array per requested grid
    for (const auto& gridInfo : imgDataInfo.GridsInfo)
    {
      // instanciate the correct data array type (according to the OpenVDB grid type)
      vtkSmartPointer<vtkDataArray> dataArray = ::InstanciateVtkArrayType(gridInfo->Grid);
      if (!dataArray)
      {
        vtkErrorMacro(<< "Couldn't instanciate vtDataArray, unknown array type");
        return 0;
      }
      dataArray->SetName(gridInfo->Name.c_str());
      dataArray->SetNumberOfComponents(gridInfo->NumComps);
      dataArray->SetNumberOfTuples(imgData->GetNumberOfPoints());
      // add the array to the vtkImageData
      imgData->GetPointData()->AddArray(dataArray);
    }

    // add the block to the multi block
    output->SetPartition(leafIdx, 0, imgData);
    leafIdx++;
  }

  // same for the point grids
  for (const auto& pointsDataInfo : pointsDatasetsInfo)
  {
    vtkNew<vtkPolyData> polydata;
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;
    vtkNew<vtkIdTypeArray> vertices;
    unsigned int numVertices = pointsDataInfo.NumPoints;

    // we can already fill the topology of the polydata:
    // only vertices
    points->SetNumberOfPoints(numVertices);
    vtkIdType* rawVertices = new vtkIdType[2l * numVertices];

    // only vertices
    for (vtkIdType i = 0; i < numVertices; i++)
    {
      rawVertices[2 * i] = 1;
      rawVertices[2 * i + 1] = i;
    }

    vertices->SetArray(rawVertices, 2l * numVertices, 0, vtkDataArray::VTK_DATA_ARRAY_DELETE);
    cells->SetCells(numVertices, vertices);

    polydata->SetPoints(points);
    polydata->SetVerts(cells);

    output->SetPartition(leafIdx, 0, polydata);
    leafIdx++;
  }

  // now we populate the different datasets
  // first the image datas
  int imgdataIdx = 0;
  for (const auto& imgDataInfo : imgDatasetsInfo)
  {
    vtkImageData* imagedata = vtkImageData::SafeDownCast(output->GetPartition(imgdataIdx, 0));
    if (!imagedata)
    {
      imgdataIdx++;
      continue;
    }
    imgDataInfo.PopulateImageData(imagedata);
    imgdataIdx++;
  }

  // then the point sets
  int polydataIdx = 0;
  for (const auto& pointDataInfo : pointsDatasetsInfo)
  {
    vtkPolyData* polydata =
      vtkPolyData::SafeDownCast(output->GetPartition(numberImages + polydataIdx, 0));
    if (!polydata)
    {
      polydataIdx++;
      continue;
    }
    pointDataInfo.PopulatePolyData(polydata);
    polydataIdx++;
  }

  return 1;
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::NumberOfGrids()
{
  return this->Internals->GridsInformation.size();
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::GetNumberOfGridsSelectionArrays()
{
  return this->GridSelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------
const char* vtkOpenVDBReader::GetGridsSelectionArrayName(int index)
{
  return this->GridSelection->GetArrayName(index);
}

//------------------------------------------------------------------------
const char* vtkOpenVDBReader::GetGridArrayName(int index)
{
  if (index < 0 || index >= static_cast<int>(this->Internals->GridsInformation.size()))
  {
    return nullptr;
  }
  return this->Internals->GridsInformation[index].Name.c_str();
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::GetGridArrayType(int index)
{
  if (index < 0 || index >= static_cast<int>(this->Internals->GridsInformation.size()))
  {
    return -1;
  }
  OpenVDBGridInformation::DataFormatType gridType =
    this->Internals->GridsInformation[index].DataFormat;

  if (gridType == OpenVDBGridInformation::DataFormatType::IMAGE_DATA)
  {
    return VTK_IMAGE_DATA;
  }
  else if (gridType == OpenVDBGridInformation::DataFormatType::POINT_CLOUD)
  {
    return VTK_POLY_DATA;
  }
  return VTK_DATA_OBJECT;
}

//------------------------------------------------------------------------
int vtkOpenVDBReader::GetGridsSelectionArrayStatus(const char* name)
{
  return this->GridSelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------
void vtkOpenVDBReader::SetGridsSelectionArrayStatus(const char* name, int status)
{
  int oldStatus = this->GetGridsSelectionArrayStatus(name);
  if (status)
  {
    this->GridSelection->EnableArray(name);
  }
  else
  {
    this->GridSelection->DisableArray(name);
  }
  if (status != oldStatus)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------
const char* vtkOpenVDBReader::GetFileExtensions()
{
  return this->FILE_EXTENSIONS;
}

//------------------------------------------------------------------------
const char* vtkOpenVDBReader::GetDescriptiveName()
{
  return this->DESCRIPTIVE_NAME;
}

//------------------------------------------------------------------------
void vtkOpenVDBReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "DownsamplingFactor: " << this->DownsamplingFactor << endl;
  os << indent << "MergeImageVolumes: " << this->MergeImageVolumes << endl;
  os << indent << "MergePointSets: " << this->MergePointSets << endl;
  this->GridSelection->PrintSelf(os, indent.GetNextIndent());
}
