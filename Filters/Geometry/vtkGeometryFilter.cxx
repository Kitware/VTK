/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkGeometryFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkAtomicMutex.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPentagonalPrism.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkTetra.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <memory>
#include <mutex>

vtkStandardNewMacro(vtkGeometryFilter);
vtkCxxSetObjectMacro(vtkGeometryFilter, Locator, vtkIncrementalPointLocator);

static constexpr unsigned char MASKED_CELL_VALUE = vtkDataSetAttributes::HIDDENCELL |
  vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::REFINEDCELL;

static constexpr unsigned char MASKED_CELL_VALUE_NOT_VISIBLE =
  vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;

static constexpr unsigned char MASKED_POINT_VALUE = vtkDataSetAttributes::HIDDENPOINT;

//------------------------------------------------------------------------------
// Construct with all types of clipping turned off.
vtkGeometryFilter::vtkGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = VTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = VTK_ID_MAX;

  this->Extent[0] = -VTK_DOUBLE_MAX;
  this->Extent[1] = VTK_DOUBLE_MAX;
  this->Extent[2] = -VTK_DOUBLE_MAX;
  this->Extent[3] = VTK_DOUBLE_MAX;
  this->Extent[4] = -VTK_DOUBLE_MAX;
  this->Extent[5] = VTK_DOUBLE_MAX;

  this->PointClipping = false;
  this->CellClipping = false;
  this->ExtentClipping = false;

  this->Merging = true;
  this->Locator = nullptr;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->FastMode = false;
  this->RemoveGhostInterfaces = true;

  this->PieceInvariant = 0;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
  this->OriginalCellIdsName = nullptr;
  this->OriginalPointIdsName = nullptr;

  // optional 2nd input
  this->SetNumberOfInputPorts(2);

  // Compatibility with vtkDataSetSurfaceFilter
  this->NonlinearSubdivisionLevel = 1;

  // Enable delegation to an internal vtkDataSetSurfaceFilter.
  this->Delegation = true;
}

//------------------------------------------------------------------------------
vtkGeometryFilter::~vtkGeometryFilter()
{
  this->SetLocator(nullptr);
  this->SetOriginalCellIdsName(nullptr);
  this->SetOriginalPointIdsName(nullptr);
}

//------------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//------------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(double extent[6])
{
  int i;

  if (extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
    extent[2] != this->Extent[2] || extent[3] != this->Extent[3] || extent[4] != this->Extent[4] ||
    extent[5] != this->Extent[5])
  {
    this->Modified();
    for (i = 0; i < 3; i++)
    {
      if (extent[2 * i + 1] < extent[2 * i])
      {
        extent[2 * i + 1] = extent[2 * i];
      }
      this->Extent[2 * i] = extent[2 * i];
      this->Extent[2 * i + 1] = extent[2 * i + 1];
    }
  }
}

//------------------------------------------------------------------------------
void vtkGeometryFilter::SetOutputPointsPrecision(int precision)
{
  if (this->OutputPointsPrecision != precision)
  {
    this->OutputPointsPrecision = precision;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//------------------------------------------------------------------------------
// Excluded faces are defined here.
template <typename TInputIdType>
struct vtkExcludedFaces
{
  vtkStaticCellLinksTemplate<TInputIdType>* Links;
  vtkExcludedFaces()
    : Links(nullptr)
  {
  }
  ~vtkExcludedFaces() { delete this->Links; }
};

//----------------------------------------------------------------------------
int vtkGeometryFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* excInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (numPts == 0 || numCells == 0)
  {
    return 1;
  }

  // Check to see if excluded faces have been provided, and is so prepare the data
  // for use.
  vtkPolyData* excFaces = nullptr;
  if (excInfo)
  {
    excFaces = vtkPolyData::SafeDownCast(excInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  int wholeExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (input->GetExtentType() == VTK_3D_EXTENT)
  {
    const int* wholeExt32;
    wholeExt32 = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    std::copy(wholeExt32, wholeExt32 + 6, wholeExtent);
  }

  // Prepare to delegate based on dataset type and characteristics.
  if (vtkPolyData::SafeDownCast(input))
  {
    return this->PolyDataExecute(input, output, excFaces);
  }
  else if (vtkUnstructuredGridBase::SafeDownCast(input))
  {
    return this->UnstructuredGridExecute(input, output, nullptr, excFaces);
  }
  else if (vtkImageData::SafeDownCast(input) || vtkRectilinearGrid::SafeDownCast(input) ||
    vtkStructuredGrid::SafeDownCast(input))
  {
    return this->StructuredExecute(input, output, wholeExtent, excFaces);
  }
  else
  {
    // Use the general case
    return this->DataSetExecute(input, output, excFaces);
  }
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. This method is now deprecated.
void vtkGeometryFilter::CreateDefaultLocator() {}

//------------------------------------------------------------------------------
void vtkGeometryFilter::SetExcludedFacesData(vtkPolyData* input)
{
  this->Superclass::SetInputData(1, input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter.
void vtkGeometryFilter::SetExcludedFacesConnection(vtkAlgorithmOutput* algOutput)
{
  this->Superclass::SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
// Return the input data or filter.
vtkPolyData* vtkGeometryFilter::GetExcludedFaces()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");

  os << indent << "Fast Mode: " << (this->FastMode ? "On\n" : "Off\n");
  os << indent << "Remove Ghost Interfaces: " << (this->RemoveGhostInterfaces ? "On\n" : "Off\n")
     << "\n";

  os << indent << "PieceInvariant: " << this->GetPieceInvariant() << endl;
  os << indent << "PassThroughCellIds: " << (this->GetPassThroughCellIds() ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: " << (this->GetPassThroughPointIds() ? "On\n" : "Off\n");

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;

  os << indent << "NonlinearSubdivisionLevel: " << this->GetNonlinearSubdivisionLevel() << endl;
}

//------------------------------------------------------------------------------
// Acceleration methods and classes for unstructured grid geometry extraction.
namespace // anonymous
{
/**
 * A face class for defining faces
 */
template <typename TInputIdType>
class Face
{
public:
  Face* Next = nullptr;
  TInputIdType OriginalCellId;
  TInputIdType* PointIds;
  int NumberOfPoints;
  bool IsGhost;

  Face() = default;
  Face(const vtkIdType& originalCellId, const vtkIdType& numberOfPoints, const bool& isGhost)
    : OriginalCellId(static_cast<TInputIdType>(originalCellId))
    , NumberOfPoints(numberOfPoints)
    , IsGhost(isGhost)
  {
  }

  bool operator==(const Face& other) const
  {
    if (this->NumberOfPoints != other.NumberOfPoints)
    {
      return false;
    }
    switch (this->NumberOfPoints)
    {
      case 3:
      {
        return this->PointIds[0] == other.PointIds[0] &&
          ((this->PointIds[1] == other.PointIds[2] && this->PointIds[2] == other.PointIds[1]) ||
            (this->PointIds[1] == other.PointIds[1] && this->PointIds[2] == other.PointIds[2]));
      }
      case 4:
      {
        return this->PointIds[0] == other.PointIds[0] && this->PointIds[2] == other.PointIds[2] &&
          ((this->PointIds[1] == other.PointIds[3] && this->PointIds[3] == other.PointIds[1]) ||
            (this->PointIds[1] == other.PointIds[1] && this->PointIds[3] == other.PointIds[3]));
      }
      default:
      {
        bool match = true;
        if (this->PointIds[0] == other.PointIds[0])
        {
          // if the first two points match loop through forwards
          // checking all points
          if (this->NumberOfPoints > 1 && this->PointIds[1] == other.PointIds[1])
          {
            for (auto i = 2; i < this->NumberOfPoints; ++i)
            {
              if (this->PointIds[i] != other.PointIds[i])
              {
                match = false;
                break;
              }
            }
          }
          else
          {
            // check if the points go in the opposite direction
            for (auto i = 1; i < this->NumberOfPoints; ++i)
            {
              if (this->PointIds[this->NumberOfPoints - i] != other.PointIds[i])
              {
                match = false;
                break;
              }
            }
          }
        }
        else
        {
          match = false;
        }
        return match;
      }
    }
  }
  bool operator!=(const Face& other) const { return !(*this == other); }
};

/**
 * A subclass of face to define Faces with a static number of points
 */
template <int TSize, typename TInputIdType>
class StaticFace : public Face<TInputIdType>
{
private:
  std::array<TInputIdType, TSize> PointIdsContainer{};

public:
  StaticFace(const vtkIdType& originalCellId, const vtkIdType* pointIds, const bool& isGhost)
    : Face<TInputIdType>(originalCellId, TSize, isGhost)
  {
    this->PointIds = this->PointIdsContainer.data();
    this->Initialize(pointIds);
  }

  inline static constexpr int GetSize() { return TSize; }

  void Initialize(const vtkIdType* pointIds)
  {
    // find the index to the smallest id
    int offset = 0;
    int index;
    for (index = 1; index < TSize; ++index)
    {
      if (pointIds[index] < pointIds[offset])
      {
        offset = index;
      }
    }
    // copy ids into ordered array with the smallest id first
    for (index = 0; index < TSize; ++index)
    {
      this->PointIds[index] = static_cast<TInputIdType>(pointIds[(offset + index) % TSize]);
    }
  }
};

/**
 * A subclass of face to define Faces with a dynamic number of points
 */
template <typename TInputIdType>
class DynamicFace : public Face<TInputIdType>
{
private:
  std::vector<TInputIdType> PointIdsContainer;

public:
  DynamicFace(const vtkIdType& originalCellId, const vtkIdType& numberOfPoints,
    const vtkIdType* pointIds, const bool& isGhost)
    : Face<TInputIdType>(originalCellId, numberOfPoints, isGhost)
  {
    assert(this->NumberOfPoints != 0);
    this->PointIdsContainer.resize(static_cast<size_t>(this->NumberOfPoints));
    this->PointIds = this->PointIdsContainer.data();
    this->Initialize(pointIds);
  }

  inline int GetSize() const { return this->NumberOfPoints; }

  void Initialize(const vtkIdType* pointIds)
  {
    // find the index to the smallest id
    int offset = 0;
    int index;
    for (index = 1; index < this->NumberOfPoints; ++index)
    {
      if (pointIds[index] < pointIds[offset])
      {
        offset = index;
      }
    }
    // copy ids into ordered array with the smallest id first
    for (index = 0; index < this->NumberOfPoints; ++index)
    {
      this->PointIds[index] =
        static_cast<TInputIdType>(pointIds[(offset + index) % this->NumberOfPoints]);
    }
  }
};

template <typename TInputIdType>
using Triangle = StaticFace<3, TInputIdType>;
template <typename TInputIdType>
using Quad = StaticFace<4, TInputIdType>;
template <typename TInputIdType>
using Pentagon = StaticFace<5, TInputIdType>;
template <typename TInputIdType>
using Hexagon = StaticFace<6, TInputIdType>;
template <typename TInputIdType>
using Heptagon = StaticFace<7, TInputIdType>;
template <typename TInputIdType>
using Octagon = StaticFace<8, TInputIdType>;
template <typename TInputIdType>
using Nonagon = StaticFace<9, TInputIdType>;
template <typename TInputIdType>
using Decagon = StaticFace<10, TInputIdType>;
template <typename TInputIdType>
using Polygon = DynamicFace<TInputIdType>;

/**
 * Memory pool for faces.
 * Code was aggregated from vtkDataSetSurfaceFilter
 */
template <typename TInputIdType>
class FaceMemoryPool
{
private:
  using TFace = Face<TInputIdType>;

  vtkIdType NumberOfArrays;
  vtkIdType ArrayLength;
  vtkIdType NextArrayIndex;
  vtkIdType NextFaceIndex;
  unsigned char** Arrays;

  inline static int SizeofFace(const int& numberOfPoints)
  {
    static constexpr int fSize = sizeof(TFace);
    static constexpr int sizeId = sizeof(TInputIdType);
    if (fSize % sizeId == 0)
    {
      return static_cast<int>(fSize + numberOfPoints * sizeId);
    }
    else
    {
      return static_cast<int>((fSize / sizeId + 1 + numberOfPoints) * sizeId);
    }
  }

public:
  FaceMemoryPool()
    : NumberOfArrays(0)
    , ArrayLength(0)
    , NextArrayIndex(0)
    , NextFaceIndex(0)
    , Arrays(nullptr)
  {
  }

  // Copy ctor.
  FaceMemoryPool(const FaceMemoryPool&) = default;

  // Copy assignment operator.
  FaceMemoryPool& operator=(const FaceMemoryPool&) = default;

  ~FaceMemoryPool() { this->Destroy(); }

  void Initialize(const vtkIdType& numberOfPoints)
  {
    this->Destroy();
    this->NumberOfArrays = 100;
    this->NextArrayIndex = 0;
    this->NextFaceIndex = 0;
    this->Arrays = new unsigned char*[this->NumberOfArrays];
    for (auto i = 0; i < this->NumberOfArrays; i++)
    {
      this->Arrays[i] = nullptr;
    }
    // size the chunks based on the size of a quadrilateral
    int quadSize = SizeofFace(4);
    if (numberOfPoints < this->NumberOfArrays)
    {
      this->ArrayLength = 50 * quadSize;
    }
    else
    {
      this->ArrayLength = (numberOfPoints / 2) * quadSize;
    }
  }

  void Destroy()
  {
    for (auto i = 0; i < this->NumberOfArrays; i++)
    {
      delete[] this->Arrays[i];
      this->Arrays[i] = nullptr;
    }
    delete[] this->Arrays;
    this->Arrays = nullptr;
    this->ArrayLength = 0;
    this->NumberOfArrays = 0;
    this->NextArrayIndex = 0;
    this->NextFaceIndex = 0;
  }

  TFace* Allocate(const int& numberOfPoints)
  {
    // see if there's room for this one
    const int polySize = SizeofFace(numberOfPoints);
    if (this->NextFaceIndex + polySize > this->ArrayLength)
    {
      ++this->NextArrayIndex;
      this->NextFaceIndex = 0;
    }

    // Although this should not happen often, check first.
    if (this->NextArrayIndex >= this->NumberOfArrays)
    {
      int idx, num;
      unsigned char** newArrays;
      num = this->NumberOfArrays * 2;
      newArrays = new unsigned char*[num];
      for (idx = 0; idx < num; ++idx)
      {
        newArrays[idx] = nullptr;
        if (idx < this->NumberOfArrays)
        {
          newArrays[idx] = this->Arrays[idx];
        }
      }
      delete[] this->Arrays;
      this->Arrays = newArrays;
      this->NumberOfArrays = num;
    }

    // Next: allocate a new array if necessary.
    if (this->Arrays[this->NextArrayIndex] == nullptr)
    {
      this->Arrays[this->NextArrayIndex] = new unsigned char[this->ArrayLength];
    }

    TFace* face =
      reinterpret_cast<TFace*>(this->Arrays[this->NextArrayIndex] + this->NextFaceIndex);
    face->NumberOfPoints = numberOfPoints;

    static constexpr int fSize = sizeof(TFace);
    static constexpr int sizeId = sizeof(TInputIdType);
    // If necessary, we create padding after TFace such that
    // the beginning of ids aligns evenly with sizeof(TInputIdType).
    if (fSize % sizeId == 0)
    {
      face->PointIds = (TInputIdType*)face + fSize / sizeId;
    }
    else
    {
      face->PointIds = (TInputIdType*)face + fSize / sizeId + 1;
    }

    this->NextFaceIndex += polySize;

    return face;
  }
};

// This class accumulates cell array-related information. Also marks points
// as used if a point map is provided.
template <typename TInputIdType>
class CellArrayType
{
private:
  TInputIdType* PointMap;
  vtkStaticCellLinksTemplate<TInputIdType>* ExcFaces;
  const unsigned char* PointGhost;

public:
  // Make things a little more expressive
  using IdListType = std::vector<TInputIdType>;
  IdListType Cells;
  IdListType OrigCellIds;

  CellArrayType()
    : PointMap(nullptr)
    , ExcFaces(nullptr)
    , PointGhost(nullptr)
  {
  }

  void SetPointsGhost(const unsigned char* pointGhost) { this->PointGhost = pointGhost; }
  void SetPointMap(TInputIdType* ptMap) { this->PointMap = ptMap; }
  void SetExcludedFaces(vtkStaticCellLinksTemplate<TInputIdType>* exc) { this->ExcFaces = exc; }
  vtkIdType GetNumberOfCells() { return static_cast<vtkIdType>(this->OrigCellIds.size()); }
  vtkIdType GetNumberOfConnEntries() { return static_cast<vtkIdType>(this->Cells.size()); }

  template <typename TGivenIds>
  void InsertNextCell(TGivenIds npts, const TGivenIds* pts, TGivenIds cellId)
  {
    // Only insert the face cell if it's not excluded
    if (this->ExcFaces && this->ExcFaces->MatchesCell(npts, pts))
    {
      return;
    }
    else if (this->PointGhost)
    {
      for (auto i = 0; i < npts; ++i)
      {
        if (this->PointGhost[pts[i]] & MASKED_POINT_VALUE)
        {
          return;
        }
      }
    }

    // Okay insert the boundary face cell
    this->Cells.emplace_back(npts);
    if (!this->PointMap)
    {
      for (auto i = 0; i < npts; ++i)
      {
        this->Cells.emplace_back(static_cast<TInputIdType>(pts[i]));
      }
    }
    else
    {
      for (auto i = 0; i < npts; ++i)
      {
        this->Cells.emplace_back(static_cast<TInputIdType>(pts[i]));
        this->PointMap[pts[i]] = 1;
      }
    }
    this->OrigCellIds.emplace_back(static_cast<TInputIdType>(cellId));
  }
};

/**
 * Hash map for faces
 */
template <typename TInputIdType>
class FaceHashMap
{
private:
  using TFace = Face<TInputIdType>;
  using TCellArrayType = CellArrayType<TInputIdType>;
  using TFaceMemoryPool = FaceMemoryPool<TInputIdType>;
  struct Bucket
  {
    TFace* Head;
    vtkAtomicMutex Lock;
    Bucket()
      : Head(nullptr)
    {
    }
  };
  size_t Size;
  std::vector<Bucket> Buckets;

public:
  FaceHashMap(const size_t& size)
    : Size(size)
  {
    this->Buckets.resize(this->Size);
  }

  template <typename FaceType>
  void Insert(const FaceType& f, TFaceMemoryPool& pool)
  {
    const size_t key = static_cast<size_t>(f.PointIds[0]) % this->Size;
    auto& bucket = this->Buckets[key];
    auto& bucketHead = bucket.Head;
    auto& bucketLock = bucket.Lock;

    std::lock_guard<vtkAtomicMutex> lock(bucketLock);
    auto current = bucketHead;
    auto previous = current;
    while (current != nullptr)
    {
      if (*current == f)
      {
        // delete the duplicate
        if (bucketHead == current)
        {
          bucketHead = current->Next;
        }
        else
        {
          previous->Next = current->Next;
        }
        return;
      }
      previous = current;
      current = current->Next;
    }
    // not found
    TFace* newF = pool.Allocate(f.GetSize());
    newF->Next = nullptr;
    newF->OriginalCellId = f.OriginalCellId;
    newF->IsGhost = f.IsGhost;
    for (int i = 0; i < f.GetSize(); ++i)
    {
      newF->PointIds[i] = f.PointIds[i];
    }
    if (bucketHead == nullptr)
    {
      bucketHead = newF;
    }
    else
    {
      previous->Next = newF;
    }
  }

  void PopulateCellArrays(std::vector<TCellArrayType*>& threadedPolys)
  {
    std::vector<TFace*> faces;
    for (auto& bucket : this->Buckets)
    {
      if (bucket.Head != nullptr)
      {
        auto current = bucket.Head;
        while (current != nullptr)
        {
          if (!current->IsGhost)
          {
            faces.push_back(current);
          }
          current = current->Next;
        }
      }
    }
    const vtkIdType numberOfThreads = static_cast<vtkIdType>(threadedPolys.size());
    const vtkIdType numberOfFaces = static_cast<vtkIdType>(faces.size());
    vtkSMPTools::For(0, numberOfThreads, [&](vtkIdType beginThreadId, vtkIdType endThreadId) {
      for (vtkIdType threadId = beginThreadId; threadId < endThreadId; ++threadId)
      {
        vtkIdType begin = threadId * numberOfFaces / numberOfThreads;
        vtkIdType end = (threadId + 1) * numberOfFaces / numberOfThreads;
        for (vtkIdType i = begin; i < end; ++i)
        {
          auto& f = faces[i];
          threadedPolys[threadId]->template InsertNextCell<TInputIdType>(
            f->NumberOfPoints, f->PointIds, f->OriginalCellId);
        }
      }
    });
  }
};

//--------------------------------------------------------------------------
// Functor/worklet interfaces VTK -> SMPTools threading. This class enables
// compositing the output threads into a final VTK output. The actual work
// is performed by by subclasses of ExtractCellBoundaries which implement
// their own operator() method (i.e., the subclasses specialize
// to a particular dataset type).
template <typename TInputIdType>
struct LocalDataType
{
  using TCellArrayType = CellArrayType<TInputIdType>;
  using TFaceMemoryPool = FaceMemoryPool<TInputIdType>;
  // Later on (in Reduce()), a thread id is assigned to the thread.
  int ThreadId;
  bool BaseThread;

  // If point merging is specified, then a non-null point map is provided.
  TInputIdType* PointMap;

  // These collect the boundary entities from geometry extraction. Note also
  // that these implicitly keep track of the number of cells inserted.
  TCellArrayType Verts;
  TCellArrayType Lines;
  TCellArrayType Polys;
  TCellArrayType Strips;

  // Later (in the Reduce() method) build an offset structure to support
  // threaded compositing of output geometric entities.
  vtkIdType VertsConnOffset;  // this thread's offset into the output vert connectivity
  vtkIdType VertsOffset;      // this thread's offset into the output offsets
  vtkIdType LinesConnOffset;  // this thread's offset into the output line connectivity
  vtkIdType LinesOffset;      // offset into the output line cells
  vtkIdType PolysConnOffset;  // this thread's offset into the output poly connectivity
  vtkIdType PolysOffset;      // offset into the output poly cells
  vtkIdType StripsConnOffset; // this thread's offset into the output strip connectivity
  vtkIdType StripsOffset;     // offset into the output triangle strip cells

  // These are scratch arrays to avoid repeated allocations
  vtkSmartPointer<vtkGenericCell> Cell;
  vtkSmartPointer<vtkIdList> CellIds;
  vtkSmartPointer<vtkIdList> IPts;
  vtkSmartPointer<vtkIdList> ICellIds;
  vtkSmartPointer<vtkIdList> CellPointIds;
  vtkSmartPointer<vtkPoints> Coords;

  TFaceMemoryPool FacePool;

  LocalDataType()
  {
    this->PointMap = nullptr;
    this->Cell.TakeReference(vtkGenericCell::New());
    this->CellIds.TakeReference(vtkIdList::New());
    this->IPts.TakeReference(vtkIdList::New());
    this->ICellIds.TakeReference(vtkIdList::New());
    this->CellPointIds.TakeReference(vtkIdList::New());
    this->Coords.TakeReference(vtkPoints::New());
  }

  LocalDataType(const LocalDataType& other)
  {
    this->ThreadId = other.ThreadId;

    this->Verts = other.Verts;
    this->Lines = other.Lines;
    this->Polys = other.Polys;
    this->Strips = other.Strips;

    this->VertsConnOffset = other.VertsConnOffset;
    this->VertsOffset = other.VertsOffset;
    this->LinesConnOffset = other.LinesConnOffset;
    this->LinesOffset = other.LinesOffset;
    this->PolysConnOffset = other.PolysConnOffset;
    this->PolysOffset = other.PolysOffset;
    this->StripsConnOffset = other.StripsConnOffset;
    this->StripsOffset = other.StripsOffset;

    this->PointMap = other.PointMap;
    // These are here to have a different allocation for each threads
    this->Cell.TakeReference(vtkGenericCell::New());
    this->CellIds.TakeReference(vtkIdList::New());
    this->IPts.TakeReference(vtkIdList::New());
    this->ICellIds.TakeReference(vtkIdList::New());
    this->CellPointIds.TakeReference(vtkIdList::New());
    this->Coords.TakeReference(vtkPoints::New());

    this->FacePool = other.FacePool;
  }

  LocalDataType& operator=(const LocalDataType& other)
  {
    if (this != &other)
    {
      LocalDataType tmp = LocalDataType(other);
      this->Swap(tmp);
    }
    return *this;
  }

  void Swap(LocalDataType& other)
  {
    using std::swap; // the compiler will use custom swap for members if it exists

    swap(this->Verts, other.Verts);
    swap(this->Lines, other.Lines);
    swap(this->Polys, other.Polys);
    swap(this->Strips, other.Strips);

    swap(this->VertsConnOffset, other.VertsConnOffset);
    swap(this->VertsOffset, other.VertsOffset);
    swap(this->LinesConnOffset, other.LinesConnOffset);
    swap(this->LinesOffset, other.LinesOffset);
    swap(this->PolysConnOffset, other.PolysConnOffset);
    swap(this->PolysOffset, other.PolysOffset);
    swap(this->StripsConnOffset, other.StripsConnOffset);
    swap(this->StripsOffset, other.StripsOffset);

    swap(this->PointMap, other.PointMap);
    swap(this->Cell, other.Cell);
    swap(this->CellIds, other.CellIds);
    swap(this->IPts, other.IPts);
    swap(this->ICellIds, other.ICellIds);
    swap(this->CellPointIds, other.CellPointIds);
    swap(this->Coords, other.Coords);

    swap(this->FacePool, other.FacePool);
  }

  void SetPointMap(TInputIdType* ptMap)
  {
    this->PointMap = ptMap;
    this->Verts.SetPointMap(ptMap);
    this->Lines.SetPointMap(ptMap);
    this->Polys.SetPointMap(ptMap);
    this->Strips.SetPointMap(ptMap);
  }

  void SetExcludedFaces(vtkStaticCellLinksTemplate<TInputIdType>* exc)
  {
    this->Verts.SetExcludedFaces(exc);
    this->Lines.SetExcludedFaces(exc);
    this->Polys.SetExcludedFaces(exc);
    this->Strips.SetExcludedFaces(exc);
  }

  void InitializeFacePool(const vtkIdType& numberOfPoints)
  {
    this->FacePool.Initialize(numberOfPoints);
  }
};

template <typename TInputIdType>
using ThreadIterType = typename vtkSMPThreadLocal<LocalDataType<TInputIdType>>::iterator;
template <typename TInputIdType>
using ThreadOutputType = std::vector<ThreadIterType<TInputIdType>>;

//--------------------------------------------------------------------------
// Given a cell and a bunch of supporting objects (to support computing and
// minimize allocation/deallocation), extract boundary features from the cell.
// This method works with arbitrary datasets.
template <typename TInputIdType>
void ExtractDSCellGeometry(
  vtkDataSet* input, vtkIdType cellId, const char* cellVis, LocalDataType<TInputIdType>* localData)
{
  using TCellArrayType = CellArrayType<TInputIdType>;
  static constexpr int pixelConvert[4] = { 0, 1, 3, 2 };
  vtkGenericCell* cell = localData->Cell;
  input->GetCell(cellId, cell);
  int cellType = cell->GetCellType();

  if (cellType != VTK_EMPTY_CELL)
  {
    TCellArrayType& verts = localData->Verts;
    TCellArrayType& lines = localData->Lines;
    TCellArrayType& polys = localData->Polys;
    TCellArrayType& strips = localData->Strips;
    vtkIdList* cellIds = localData->CellIds.Get();
    vtkIdList* ptIds = localData->IPts.Get();
    ptIds->SetNumberOfIds(4);

    int cellDim = cell->GetCellDimension();
    vtkIdType npts = cell->PointIds->GetNumberOfIds();
    vtkIdType* pts = cell->PointIds->GetPointer(0);

    switch (cellDim)
    {
      // create new points and then cell
      case 0:
        verts.InsertNextCell(npts, pts, cellId);
        break;

      case 1:
        lines.InsertNextCell(npts, pts, cellId);
        break;

      case 2:
        if (cellType == VTK_TRIANGLE_STRIP)
        {
          strips.InsertNextCell(npts, pts, cellId);
        }
        else if (cellType == VTK_PIXEL)
        {
          ptIds->SetId(0, pts[pixelConvert[0]]);
          ptIds->SetId(1, pts[pixelConvert[1]]);
          ptIds->SetId(2, pts[pixelConvert[2]]);
          ptIds->SetId(3, pts[pixelConvert[3]]);
          polys.InsertNextCell(npts, ptIds->GetPointer(0), cellId);
        }
        else
        {
          polys.InsertNextCell(npts, pts, cellId);
        }
        break;

      case 3:
        int numFaces = cell->GetNumberOfFaces();
        for (auto j = 0; j < numFaces; j++)
        {
          vtkCell* face = cell->GetFace(j);
          input->GetCellNeighbors(cellId, face->PointIds, cellIds);
          if (cellIds->GetNumberOfIds() <= 0 || (cellVis && !cellVis[cellIds->GetId(0)]))
          {
            vtkIdType numFacePts = face->GetNumberOfPoints();
            polys.InsertNextCell(numFacePts, face->PointIds->GetPointer(0), cellId);
          }
        }
        break;
    } // switch
  }   // non-empty cell
} // extract dataset geometry

//--------------------------------------------------------------------------
// Given a cell and a bunch of supporting objects (to support computing and
// minimize allocation/deallocation), extract boundary features from the cell.
// This method works with unstructured grids.
template <typename TInputIdType>
void ExtractCellGeometry(vtkUnstructuredGridBase* input, vtkIdType cellId, int cellType,
  vtkIdType npts, const vtkIdType* pts, LocalDataType<TInputIdType>* localData,
  FaceHashMap<TInputIdType>* faceMap, const bool& isGhost)
{
  using TCellArrayType = CellArrayType<TInputIdType>;
  TCellArrayType& verts = localData->Verts;
  TCellArrayType& lines = localData->Lines;
  TCellArrayType& polys = localData->Polys;
  TCellArrayType& strips = localData->Strips;
  vtkGenericCell* cell = localData->Cell.Get();

  int faceId, numFaces, numFacePts;
  static constexpr int MAX_FACE_POINTS = 32;
  vtkIdType ptIds[MAX_FACE_POINTS]; // cell face point ids
  const vtkIdType* faceVerts;
  static constexpr int pixelConvert[4] = { 0, 1, 3, 2 };

  switch (cellType)
  {
    case VTK_EMPTY_CELL:
      break;

    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      verts.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_LINE:
    case VTK_POLY_LINE:
      lines.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_POLYGON:
      polys.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_TRIANGLE_STRIP:
      strips.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_PIXEL:
      // pixelConvert (in the following loop) is an int[4]. GCC 5.1.1
      // warns about pixelConvert[4] being uninitialized due to loop
      // unrolling -- forcibly restricting npts <= 4 prevents this warning.
      ptIds[0] = pts[pixelConvert[0]];
      ptIds[1] = pts[pixelConvert[1]];
      ptIds[2] = pts[pixelConvert[2]];
      ptIds[3] = pts[pixelConvert[3]];
      polys.InsertNextCell(npts, ptIds, cellId);
      break;

    case VTK_TETRA:
      for (faceId = 0; faceId < 4; faceId++)
      {
        faceVerts = vtkTetra::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        faceMap->Insert(Triangle<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
      }
      break;

    case VTK_VOXEL:
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkVoxel::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[pixelConvert[0]]];
        ptIds[1] = pts[faceVerts[pixelConvert[1]]];
        ptIds[2] = pts[faceVerts[pixelConvert[2]]];
        ptIds[3] = pts[faceVerts[pixelConvert[3]]];
        faceMap->Insert(Quad<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
      }
      break;

    case VTK_HEXAHEDRON:
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkHexahedron::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        faceMap->Insert(Quad<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
      }
      break;

    case VTK_WEDGE:
      for (faceId = 0; faceId < 5; faceId++)
      {
        faceVerts = vtkWedge::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        if (faceVerts[3] < 0)
        {
          faceMap->Insert(Triangle<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
        else
        {
          ptIds[3] = pts[faceVerts[3]];
          faceMap->Insert(Quad<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
      }
      break;

    case VTK_PYRAMID:
      for (faceId = 0; faceId < 5; faceId++)
      {
        faceVerts = vtkPyramid::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        if (faceVerts[3] < 0)
        {
          faceMap->Insert(Triangle<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
        else
        {
          ptIds[3] = pts[faceVerts[3]];
          faceMap->Insert(Quad<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
      }
      break;

    case VTK_HEXAGONAL_PRISM:
      for (faceId = 0; faceId < 8; faceId++)
      {
        faceVerts = vtkHexagonalPrism::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        if (faceVerts[4] < 0)
        {
          faceMap->Insert(Quad<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
        else
        {
          ptIds[4] = pts[faceVerts[4]];
          ptIds[5] = pts[faceVerts[5]];
          faceMap->Insert(Hexagon<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
      }
      break;

    case VTK_PENTAGONAL_PRISM:
      for (faceId = 0; faceId < 7; faceId++)
      {
        faceVerts = vtkPentagonalPrism::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        if (faceVerts[4] < 0)
        {
          faceMap->Insert(Quad<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
        else
        {
          ptIds[4] = pts[faceVerts[4]];
          faceMap->Insert(Pentagon<TInputIdType>(cellId, ptIds, isGhost), localData->FacePool);
        }
      }
      break;

    default:
      // Other types of 3D linear cells handled by vtkGeometryFilter. Exactly what
      // is a linear cell is defined by vtkCellTypes::IsLinear().
      input->GetCell(cellId, cell);
      if (cell->GetCellDimension() == 3)
      {
        for (faceId = 0, numFaces = cell->GetNumberOfFaces(); faceId < numFaces; faceId++)
        {
          vtkCell* face = cell->GetFace(faceId);
          numFacePts = static_cast<int>(face->PointIds->GetNumberOfIds());
          switch (numFacePts)
          {
            case 3:
              faceMap->Insert(
                Triangle<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 4:
              faceMap->Insert(Quad<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 5:
              faceMap->Insert(
                Pentagon<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 6:
              faceMap->Insert(Hexagon<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 7:
              faceMap->Insert(
                Heptagon<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 8:
              faceMap->Insert(Octagon<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 9:
              faceMap->Insert(Nonagon<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            case 10:
              faceMap->Insert(Decagon<TInputIdType>(cellId, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
            default:
              faceMap->Insert(
                Polygon<TInputIdType>(cellId, numFacePts, face->PointIds->GetPointer(0), isGhost),
                localData->FacePool);
              break;
          }
        } // for all cell faces
      }   // if 3D
      else
      {
        vtkLog(ERROR, "Unknown cell type.");
      }
  } // switch
} // ExtractCellGeometry()

// Base class to extract boundary entities. Derived by all dataset extraction
// types -- the operator() method needs to be implemented by subclasses.
template <typename TInputIdType>
struct ExtractCellBoundaries
{
  vtkGeometryFilter* Self;
  // If point merging is specified, then a point map is created.
  TInputIdType* PointMap;

  // Cell visibility and cell ghost levels
  const char* CellVis;
  const unsigned char* CellGhosts;
  const unsigned char* PointGhost;

  // Thread-related output data
  vtkSMPThreadLocal<LocalDataType<TInputIdType>> LocalData;
  vtkIdType VertsNumPts, VertsNumCells;
  vtkIdType LinesNumPts, LinesNumCells;
  vtkIdType PolysNumPts, PolysNumCells;
  vtkIdType StripsNumPts, StripsNumCells;
  vtkIdType VertsCellIdOffset;
  vtkIdType LinesCellIdOffset;
  vtkIdType PolysCellIdOffset;
  vtkIdType StripsCellIdOffset;
  vtkIdType NumPts;
  vtkIdType NumCells;
  ExtractCellBoundaries* Extract;
  vtkStaticCellLinksTemplate<TInputIdType>* ExcFaces;
  using TThreadOutputType = ThreadOutputType<TInputIdType>;
  TThreadOutputType* Threads;

  ExtractCellBoundaries(vtkGeometryFilter* self, const char* cellVis,
    const unsigned char* cellGhosts, const unsigned char* pointGhost,
    vtkExcludedFaces<TInputIdType>* exc, TThreadOutputType* threads)
    : Self(self)
    , PointMap(nullptr)
    , CellVis(cellVis)
    , CellGhosts(cellGhosts)
    , PointGhost(pointGhost)
    , Threads(threads)
  {
    this->ExcFaces = (exc == nullptr ? nullptr : exc->Links);
  }

  virtual ~ExtractCellBoundaries() { delete[] this->PointMap; }

  // If point merging is needed, create the point map (map from old points
  // to new points).
  void CreatePointMap(vtkIdType numPts)
  {
    this->PointMap = new TInputIdType[numPts];
    vtkSMPTools::Fill(this->PointMap, this->PointMap + numPts, -1);
  }

  // Initialize thread data
  virtual void Initialize()
  {
    // Make sure cells have been built
    auto& localData = this->LocalData.Local();
    localData.BaseThread = false;
    localData.SetPointMap(this->PointMap);
    localData.SetExcludedFaces(this->ExcFaces);
    localData.Verts.SetPointsGhost(this->PointGhost);
    localData.Lines.SetPointsGhost(this->PointGhost);
    localData.Polys.SetPointsGhost(this->PointGhost);
    localData.Strips.SetPointsGhost(this->PointGhost);
  }

  // operator() implemented by dataset-specific subclasses

  // Composite local thread data; i.e., rather than linearly appending data from each
  // thread into the filter's output, this performs a parallel append.
  virtual void Reduce()
  {
    // Determine offsets to partition work and perform memory allocations.
    vtkIdType numCells, numConnEntries;
    this->VertsNumPts = this->VertsNumCells = 0;
    this->LinesNumPts = this->LinesNumCells = 0;
    this->PolysNumPts = this->PolysNumCells = 0;
    this->StripsNumPts = this->StripsNumCells = 0;
    int threadId = 0;

    // Loop over the local data generated by each thread. Setup the
    // offsets and such to insert into the output cell arrays.
    auto tItr = this->LocalData.begin();
    auto tEnd = this->LocalData.end();
    for (; tItr != tEnd; ++tItr)
    {
      tItr->ThreadId = threadId++;
      this->Threads->emplace_back(tItr); // need pointers to local thread data

      tItr->VertsConnOffset = this->VertsNumPts;
      tItr->VertsOffset = this->VertsNumCells;
      numCells = tItr->Verts.GetNumberOfCells();
      numConnEntries = tItr->Verts.GetNumberOfConnEntries() - numCells;
      this->VertsNumCells += numCells;
      this->VertsNumPts += numConnEntries;

      tItr->LinesConnOffset = this->LinesNumPts;
      tItr->LinesOffset = this->LinesNumCells;
      numCells = tItr->Lines.GetNumberOfCells();
      numConnEntries = tItr->Lines.GetNumberOfConnEntries() - numCells;
      this->LinesNumCells += numCells;
      this->LinesNumPts += numConnEntries;

      tItr->PolysConnOffset = this->PolysNumPts;
      tItr->PolysOffset = this->PolysNumCells;
      numCells = tItr->Polys.GetNumberOfCells();
      numConnEntries = tItr->Polys.GetNumberOfConnEntries() - numCells;
      this->PolysNumCells += numCells;
      this->PolysNumPts += numConnEntries;

      tItr->StripsConnOffset = this->StripsNumPts;
      tItr->StripsOffset = this->StripsNumCells;
      numCells = tItr->Strips.GetNumberOfCells();
      numConnEntries = tItr->Strips.GetNumberOfConnEntries() - numCells;
      this->StripsNumCells += numCells;
      this->StripsNumPts += numConnEntries;
    }
    this->VertsCellIdOffset = 0;
    this->LinesCellIdOffset = this->VertsNumCells;
    this->PolysCellIdOffset = this->VertsNumCells + this->LinesNumCells;
    this->StripsCellIdOffset = this->VertsNumCells + this->LinesNumCells + this->PolysNumCells;
    this->NumCells =
      this->VertsNumCells + this->LinesNumCells + this->PolysNumCells + this->StripsNumCells;
    this->NumPts = this->VertsNumPts + this->LinesNumPts + this->PolysNumPts + this->StripsNumPts;
  }
};

// Extract unstructured grid boundary by visiting each cell and examining
// cell features.
template <typename TInputIdType>
struct ExtractUG : public ExtractCellBoundaries<TInputIdType>
{
  // The unstructured grid to process
  vtkUnstructuredGridBase* Grid;
  using TFaceHashMap = FaceHashMap<TInputIdType>;
  std::shared_ptr<TFaceHashMap> FaceMap;
  bool RemoveGhostInterfaces;

  vtkIdType NumberOfCells;
  const unsigned char MASKED_CELL;

  ExtractUG(vtkGeometryFilter* self, vtkUnstructuredGridBase* grid, const char* cellVis,
    const unsigned char* cellGhost, const unsigned char* pointGhost,
    vtkExcludedFaces<TInputIdType>* exc, ThreadOutputType<TInputIdType>* t)
    : ExtractCellBoundaries<TInputIdType>(self, cellVis, cellGhost, pointGhost, exc, t)
    , Grid(grid)
    , RemoveGhostInterfaces(self->GetRemoveGhostInterfaces())
    , NumberOfCells(grid->GetNumberOfCells())
    , MASKED_CELL(
        self->GetRemoveGhostInterfaces() ? MASKED_CELL_VALUE : MASKED_CELL_VALUE_NOT_VISIBLE)
  {
    if (self->GetMerging())
    {
      this->CreatePointMap(grid->GetNumberOfPoints());
    }
    this->FaceMap = std::make_shared<TFaceHashMap>(static_cast<size_t>(grid->GetNumberOfPoints()));
  }

  // Initialize thread data
  void Initialize() override
  {
    this->ExtractCellBoundaries<TInputIdType>::Initialize();
    this->LocalData.Local().InitializeFacePool(this->Grid->GetNumberOfPoints());
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    auto faceMap = this->FaceMap.get();
    auto& localData = this->LocalData.Local();
    auto& cellPointIds = localData.CellPointIds;
    if (beginCellId == 0)
    {
      localData.BaseThread = true;
    }

    vtkIdType npts;
    const vtkIdType* pts;
    unsigned char type;
    bool isGhost;
    for (vtkIdType cellId = beginCellId; cellId < endCellId && !this->Self->GetAbortExecute();
         ++cellId)
    {
      // -------------------------------------Ghost explanation-------------------------------------
      // Note for both cell dimension cases: MASKED_CELL is computed based on RemoveGhostInterfaces.
      //
      // For 0d-1d-2d cells, we have 2 cases:
      // 1) When RemoveGhostInterfaces is on, we want to remove all types of ghosts.
      // Since isGhost is always true for every type of ghost, and dim < 3, they will be skipped.
      // 2) When RemoveGhostInterfaces is off, we want to keep only the duplicate ghosts.
      // Since isGhost will be false for duplicates, the duplicates cells will be kept
      // the rest will be skipped.
      //
      // For 3d cells, we have 2 cases:
      // 1) When RemoveGhostInterfaces is on, we want to remove all types of ghosts.
      // Since isGhost is always true for every type of ghost, these cells will be processed
      // and their faces will be removed in the PopulateCellArrays step of the FaceHashMap.
      // 2) When RemoveGhostInterfaces is off, we want to keep only the duplicate ghosts.
      // Since isGhost is always false for duplicates, the duplicate cells will be kept and the
      // the rest will be skipped.
      type = static_cast<unsigned char>(this->Grid->GetCellType(cellId));
      isGhost = this->CellGhosts && this->CellGhosts[cellId] & this->MASKED_CELL;
      if (isGhost && (vtkCellTypes::GetDimension(type) < 3 || !this->RemoveGhostInterfaces))
      {
        continue;
      }
      // If the cell is visible process it
      if (!this->CellVis || this->CellVis[cellId])
      {
        this->Grid->GetCellPoints(cellId, npts, pts, cellPointIds);
        ExtractCellGeometry(this->Grid, cellId, type, npts, pts, &localData, faceMap, isGhost);
      } // if cell visible
    }   // for all cells in this batch
    if (localData.BaseThread)
    {
      this->Self->UpdateProgress(static_cast<double>(0.8 * endCellId / this->NumberOfCells));
    }
  } // operator()

  // Composite local thread data
  void Reduce() override
  {
    std::vector<CellArrayType<TInputIdType>*> threadedPolys;
    for (auto& localData : this->LocalData)
    {
      threadedPolys.push_back(&localData.Polys);
    }
    this->FaceMap->PopulateCellArrays(threadedPolys);
    // Deallocate threaded face memory pools since CellArrays are now populated
    for (auto& localData : this->LocalData)
    {
      localData.FacePool.Destroy();
    }
    this->ExtractCellBoundaries<TInputIdType>::Reduce();
  }
};

// Extract structured 3D grid boundary with and without visible cells
template <typename TGrid, typename TInputIdType>
struct ExtractStructured : public ExtractCellBoundaries<TInputIdType>
{
  TGrid* Input;       // Input data
  bool FastMode;      // Whether to use fast mode or not
  bool* ExtractFaces; // Whether to extract faces or not
  int* Extent;        // Data extent
  int* WholeExtent;   // Whole extent
  int Dimension;      // Dimension of the input
  int Dims[3];        // Grid dimensions

  bool ForceSimpleVisibilityCheck; // Whether to force simple visibility check
  bool AllCellsVisible;            // Whether all cells are visible

  int CurrentAxis;  // Current Axis
  int IAxis, JAxis; // Axis indices
  int MinFace;      // Whether to extract max face or min face

  vtkIdType NumberOfFaces;

  ExtractStructured(vtkGeometryFilter* self, TGrid* ds, int* wholeExtent, bool* extractFaces,
    bool merging, vtkExcludedFaces<TInputIdType>* exc, ThreadOutputType<TInputIdType>* t)
    : ExtractCellBoundaries<TInputIdType>(self, nullptr, nullptr, nullptr, exc, t)
    , Input(ds)
    , FastMode(self->GetFastMode())
    , ExtractFaces(extractFaces)
    , Extent(ds->GetExtent())
    , WholeExtent(wholeExtent)
    , Dimension(ds->GetDataDimension())
  {
    this->Dims[0] = this->Extent[1] - this->Extent[0] + 1;
    this->Dims[1] = this->Extent[3] - this->Extent[2] + 1;
    this->Dims[2] = this->Extent[5] - this->Extent[4] + 1;
    if (merging)
    {
      this->CreatePointMap(ds->GetNumberOfPoints());
    }
    this->ForceSimpleVisibilityCheck = extractFaces != nullptr;
    this->AllCellsVisible = !this->Input->GetCellData()->HasAnyGhostBitSet(
      self->GetRemoveGhostInterfaces() ? MASKED_CELL_VALUE : MASKED_CELL_VALUE_NOT_VISIBLE);
  }

  // Initialize thread data
  void Initialize() override { this->ExtractCellBoundaries<TInputIdType>::Initialize(); }

  /**
   * This check is needed to understand of a side of the structure data needs to be extracted
   * when running in distributed mode.
   */
  bool CheckIfFaceShouldBeProcessed()
  {
    auto& aAxis = this->CurrentAxis;
    auto& bAxis = this->IAxis;
    auto& cAxis = this->JAxis;

    int aA2 = aAxis * 2;
    int bA2 = bAxis * 2;
    int cA2 = cAxis * 2;

    // We might as well put the test for this face here.
    if (this->Extent[bA2] == this->Extent[bA2 + 1] || this->Extent[cA2] == this->Extent[cA2 + 1])
    {
      return false;
    }
    if (!this->MinFace)
    {
      if (this->Extent[aA2 + 1] < this->WholeExtent[aA2 + 1])
      {
        return false;
      }
    }
    else
    { // min faces have a slightly different condition to avoid coincident faces.
      if (this->Extent[aA2] == this->Extent[aA2 + 1] || this->Extent[aA2] > this->WholeExtent[aA2])
      {
        return false;
      }
    }
    return true;
  }

  enum FaceMode
  {
    WHOLE_FACE = 0,     // Used by the FaceOperator
    SHRINKING_FACES = 1 // Used by ShrinkingFacesOperator
  };

  std::array<TInputIdType, 4> GetFace(const int ijk[3], bool minFace, FaceMode faceMode)
  {
    const int& axis = this->CurrentAxis;
    const int& iAxis = this->IAxis;
    const int& jAxis = this->JAxis;

    int ptIjk[3] = { ijk[0], ijk[1], ijk[2] };
    if (!minFace)
    {
      ++ptIjk[axis];
    }

    std::array<TInputIdType, 4> face;
    face[0] =
      static_cast<TInputIdType>(vtkStructuredData::ComputePointIdForExtent(this->Extent, ptIjk));

    ++ptIjk[iAxis];
    face[1] =
      static_cast<TInputIdType>(vtkStructuredData::ComputePointIdForExtent(this->Extent, ptIjk));

    ++ptIjk[jAxis];
    face[2] =
      static_cast<TInputIdType>(vtkStructuredData::ComputePointIdForExtent(this->Extent, ptIjk));

    --ptIjk[iAxis];
    face[3] =
      static_cast<TInputIdType>(vtkStructuredData::ComputePointIdForExtent(this->Extent, ptIjk));

    if (minFace || faceMode == FaceMode::WHOLE_FACE)
    {
      // invert face order to get an outside pointing normal.
      std::swap(face[1], face[3]);
    }
    return face;
  }

  void FaceOperator(vtkIdType faceBeginCellId, vtkIdType faceEndCellId)
  {
    auto& polys = this->LocalData.Local().Polys;
    const auto& extent = this->Extent;
    const auto& dims = this->Dims;
    const int& axis = this->CurrentAxis;
    const int& iAxis = this->IAxis;
    const int& jAxis = this->JAxis;

    const int axis2 = this->CurrentAxis * 2;
    const int iAxis2 = this->IAxis * 2;
    const int jAxis2 = this->JAxis * 2;

    TInputIdType cellId;
    int ijk[3];
    ijk[axis] = this->MinFace ? extent[axis2] : extent[axis2 + 1] - 1;
    const int faceWidth_1 = dims[iAxis] - 1;
    for (vtkIdType faceCellId = faceBeginCellId;
         faceCellId < faceEndCellId && !this->Self->GetAbortExecute(); ++faceCellId)
    {
      ijk[iAxis] = extent[iAxis2] + static_cast<int>(faceCellId % faceWidth_1);
      ijk[jAxis] = extent[jAxis2] + static_cast<int>(faceCellId / faceWidth_1);
      cellId = static_cast<TInputIdType>(vtkStructuredData::ComputeCellIdForExtent(extent, ijk));
      if (!this->ForceSimpleVisibilityCheck || this->Input->IsCellVisible(cellId))
      {
        const auto face = this->GetFace(ijk, this->MinFace, FaceMode::WHOLE_FACE);
        polys.template InsertNextCell<TInputIdType>(4, face.data(), cellId);
      }
    }
  }

  /**
   * Implementation to compute the external polydata for a structured grid with
   * blanking. The algorithm, which we call "Shrinking Faces",
   * takes the min and max face along each axis and then for each cell on the
   * face, keep on advancing the cell in the direction of the axis till a visible
   * cell is found and then extracts the face long the chosen axis. For min face,
   * this advancing is done in the positive direction of the axis while it's in
   * reverse for the max face. This works well for generating an outer shell and
   * is quite fast too. However we miss internal faces. So in non-fast mode, we
   * don't reverse the direction instead continue along the axis while
   * flip-flopping between detecting visible or invisible cells and then picking
   * the appropriate face to extract.
   *
   * This implementation only supports 3D grids. For 2D/1D grids, the standard
   * algorithm for extracting surface is adequate.
   *
   * This function returns false if data is not appropriate in which case the
   * caller should simply fall back to the default case without blanking.
   */
  void ShrinkingFacesOperator(vtkIdType faceBeginCellId, vtkIdType faceEndCellId)
  {
    auto& polys = this->LocalData.Local().Polys;
    const auto& extent = this->Extent;
    const auto& dims = this->Dims;
    const int& axis = this->CurrentAxis;
    const int& iAxis = this->IAxis;
    const int& jAxis = this->JAxis;

    const int axis2 = this->CurrentAxis * 2;
    const int iAxis2 = this->IAxis * 2;
    const int jAxis2 = this->JAxis * 2;

    TInputIdType cellId, reverseCellId;
    bool minFace;
    int ijk[3];
    const int faceWidth_1 = dims[iAxis] - 1;
    for (vtkIdType faceCellId = faceBeginCellId;
         faceCellId < faceEndCellId && !this->Self->GetAbortExecute(); ++faceCellId)
    {
      ijk[iAxis] = extent[iAxis2] + static_cast<int>(faceCellId % faceWidth_1);
      ijk[jAxis] = extent[jAxis2] + static_cast<int>(faceCellId / faceWidth_1);
      minFace = true;
      for (int k = extent[axis2]; k < extent[axis2 + 1]; ++k)
      {
        ijk[axis] = k;
        cellId = static_cast<TInputIdType>(vtkStructuredData::ComputeCellIdForExtent(extent, ijk));
        const bool cellVisible = this->Input->IsCellVisible(cellId);
        if ((minFace && cellVisible) || (!minFace && !cellVisible))
        {
          // this ensures correct cell-data is picked for the face
          ijk[axis] = minFace ? k : (k - 1);
          const auto face = this->GetFace(ijk, minFace, FaceMode::SHRINKING_FACES);
          cellId =
            static_cast<TInputIdType>(vtkStructuredData::ComputeCellIdForExtent(extent, ijk));
          polys.template InsertNextCell<TInputIdType>(4, face.data(), cellId);
          if (this->FastMode)
          {
            // in fast mode, we immediately start iterating from the other
            // side instead to find the capping surface. we can ignore
            // interior surfaces for speed.

            // find max-face (reverse order)
            for (int reverseK = extent[axis2 + 1] - 1; reverseK >= k; --reverseK)
            {
              ijk[axis] = reverseK;
              reverseCellId =
                static_cast<TInputIdType>(vtkStructuredData::ComputeCellIdForExtent(extent, ijk));
              if (this->Input->IsCellVisible(reverseCellId))
              {
                const auto face2 = this->GetFace(ijk, /*minFace=*/false, FaceMode::SHRINKING_FACES);
                polys.template InsertNextCell<TInputIdType>(4, face2.data(), reverseCellId);
                break;
              }
            }
            break;
          }
          minFace = !minFace;
        }
      }
      // If not in fast mode, and we've stepped out of the volume without a
      // capping-surface, add the capping surface.
      if (!minFace && !this->FastMode)
      {
        cellId = static_cast<TInputIdType>(vtkStructuredData::ComputeCellIdForExtent(extent, ijk));
        ijk[axis] = extent[5] - 1;
        const auto face = this->GetFace(ijk, /*minFace=*/false, FaceMode::SHRINKING_FACES);
        polys.template InsertNextCell<TInputIdType>(4, face.data(), cellId);
      }
    }
  }

  void operator()(vtkIdType faceBeginCellId, vtkIdType faceEndCellId)
  {
    auto& localData = this->LocalData.Local();
    if (faceBeginCellId == 0)
    {
      localData.BaseThread = true;
    }
    if (this->AllCellsVisible || this->ForceSimpleVisibilityCheck)
    {
      this->FaceOperator(faceBeginCellId, faceEndCellId);
      if (localData.BaseThread)
      {
        this->Self->UpdateProgress(static_cast<double>(0.05 * (this->CurrentAxis + !this->MinFace) +
          (0.05 * faceEndCellId / this->NumberOfFaces)));
      }
    }
    else
    {
      this->ShrinkingFacesOperator(faceBeginCellId, faceEndCellId);
      if (localData.BaseThread)
      {
        this->Self->UpdateProgress(static_cast<double>(
          0.1 * this->CurrentAxis + (0.1 * faceEndCellId / this->NumberOfFaces)));
      }
    }
  } // operator()

  // Composite local thread data
  void Reduce() override {}

  static ExtractCellBoundaries<TInputIdType>* Execute(vtkGeometryFilter* self, TGrid* ds,
    int* wholeExtent, bool* extractFaces, bool merging, vtkExcludedFaces<TInputIdType>* exc,
    ThreadOutputType<TInputIdType>* t)
  {
    auto extract = new ExtractStructured<TGrid, TInputIdType>(
      self, ds, wholeExtent, extractFaces, merging, exc, t);
    if (extract->AllCellsVisible || extract->ForceSimpleVisibilityCheck)
    {
      const auto& extent = extract->Extent;
      auto& axis = extract->CurrentAxis;
      auto& iAxis = extract->IAxis;
      auto& jAxis = extract->JAxis;
      for (axis = 0; axis < 3; ++axis)
      {
        // axisMin-face
        iAxis = (axis + 1) % 3;
        jAxis = (axis + 2) % 3;
        extract->MinFace = true;
        bool processFace = !extract->ForceSimpleVisibilityCheck || extract->ExtractFaces[2 * axis];
        if (processFace && extract->CheckIfFaceShouldBeProcessed())
        {
          extract->NumberOfFaces = std::abs(extent[2 * iAxis + 1] - extent[2 * iAxis]) *
            std::abs(extent[2 * jAxis] - extent[2 * jAxis + 1]);
          vtkSMPTools::For(0, extract->NumberOfFaces, *extract);
        }
        // axisMax-face
        std::swap(iAxis, jAxis);
        extract->MinFace = false;
        processFace = !extract->ForceSimpleVisibilityCheck || extract->ExtractFaces[2 * axis + 1];
        if (processFace && extract->CheckIfFaceShouldBeProcessed())
        {
          extract->NumberOfFaces = std::abs(extent[2 * iAxis + 1] - extent[2 * iAxis]) *
            std::abs(extent[2 * jAxis] - extent[2 * jAxis + 1]);
          vtkSMPTools::For(0, extract->NumberOfFaces, *extract);
        }
      }
    }
    else
    {
      auto& extent = extract->Extent;
      auto& axis = extract->CurrentAxis;
      auto& iAxis = extract->IAxis;
      auto& jAxis = extract->JAxis;
      for (axis = 0; axis < 3; ++axis)
      {
        iAxis = (axis + 1) % 3;
        jAxis = (axis + 2) % 3;
        extract->NumberOfFaces = std::abs(extent[2 * iAxis + 1] - extent[2 * iAxis]) *
          std::abs(extent[2 * jAxis] - extent[2 * jAxis + 1]);
        vtkSMPTools::For(0, extract->NumberOfFaces, *extract);
      }
    }
    extract->ExtractCellBoundaries<TInputIdType>::Reduce();
    return extract;
  }
};

// Extract the boundaries of a general vtkDataSet by visiting each cell and
// examining cell features. This is slower than specialized types and should be
// avoided if possible.
template <typename TInputIdType>
struct ExtractDS : public ExtractCellBoundaries<TInputIdType>
{
  // The unstructured grid to process
  vtkDataSet* DataSet;
  vtkIdType NumberOfCells;
  const unsigned char MASKED_CELL;

  ExtractDS(vtkGeometryFilter* self, vtkDataSet* ds, const char* cellVis,
    const unsigned char* cellGhost, const unsigned char* pointGhost,
    vtkExcludedFaces<TInputIdType>* exc, ThreadOutputType<TInputIdType>* t)
    : ExtractCellBoundaries<TInputIdType>(self, cellVis, cellGhost, pointGhost, exc, t)
    , DataSet(ds)
    , NumberOfCells(ds->GetNumberOfCells())
    , MASKED_CELL(
        self->GetRemoveGhostInterfaces() ? MASKED_CELL_VALUE : MASKED_CELL_VALUE_NOT_VISIBLE)
  {
    // Point merging is always required since points are not explicitly
    // represented and cannot be passed through to the output.
    this->CreatePointMap(ds->GetNumberOfPoints());
    // Make sure any internal initialization methods which may not be thread
    // safe are built.
    this->DataSet->GetCell(0);
  }

  // Initialize thread data
  void Initialize() override { this->ExtractCellBoundaries<TInputIdType>::Initialize(); }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    if (beginCellId == 0)
    {
      localData.BaseThread = true;
    }

    for (vtkIdType cellId = beginCellId; cellId < endCellId && !this->Self->GetAbortExecute();
         ++cellId)
    {
      // Handle ghost cells here.  Another option was used cellVis array.
      if (this->CellGhosts && this->CellGhosts[cellId] & this->MASKED_CELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // If the cell is visible process it
      if (!this->CellVis || this->CellVis[cellId])
      {
        ExtractDSCellGeometry(this->DataSet, cellId, this->CellVis, &localData);
      } // if cell visible

    } // for all cells in this batch
    if (localData.BaseThread)
    {
      this->Self->UpdateProgress(static_cast<double>(0.8 * endCellId / this->NumberOfCells));
    }
  } // operator()

  // Composite local thread data
  void Reduce() override { this->ExtractCellBoundaries<TInputIdType>::Reduce(); }
};

// Helper class to record original point and cell ids. This is for copying
// cell data, and also to produce output arrays indicating where output
// cells originated from (typically used in picking).
struct IdRecorder
{
  vtkSmartPointer<vtkIdTypeArray> Ids;

  IdRecorder(
    vtkTypeBool passThru, const char* name, vtkDataSetAttributes* attrD, vtkIdType allocSize)
  {
    if (passThru)
    {
      this->Ids.TakeReference(vtkIdTypeArray::New());
      this->Ids->SetName(name);
      this->Ids->SetNumberOfComponents(1);
      this->Ids->Allocate(allocSize);
      attrD->AddArray(this->Ids.Get());
    }
  }
  IdRecorder(vtkTypeBool passThru, const char* name, vtkDataSetAttributes* attrD)
  {
    if (passThru)
    {
      this->Ids.TakeReference(vtkIdTypeArray::New());
      this->Ids->SetName(name);
      this->Ids->SetNumberOfComponents(1);
      attrD->AddArray(this->Ids.Get());
    }
    else
    {
      this->Ids = nullptr;
    }
  }
  void Insert(vtkIdType destId, vtkIdType origId)
  {
    if (this->Ids.Get() != nullptr)
    {
      this->Ids->InsertValue(destId, origId);
    }
  }
  vtkIdType* GetPointer() { return this->Ids->GetPointer(0); }
  vtkTypeBool PassThru() { return this->Ids.Get() != nullptr; }
  void Allocate(vtkIdType num)
  {
    if (this->Ids.Get() != nullptr)
    {
      this->Ids->Allocate(num);
    }
  }
  void SetNumberOfValues(vtkIdType num)
  {
    if (this->Ids.Get() != nullptr)
    {
      this->Ids->SetNumberOfValues(num);
    }
  }
}; // id recorder

// Generate point map for explicit point representations.
template <typename TIP, typename TOP, typename TInputIdType>
struct GenerateExpPoints
{
  TIP* InPts;
  TOP* OutPts;
  TInputIdType* PointMap;
  ArrayList* PtArrays;

  GenerateExpPoints(TIP* inPts, TOP* outPts, TInputIdType* ptMap, ArrayList* ptArrays)
    : InPts(inPts)
    , OutPts(outPts)
    , PointMap(ptMap)
    , PtArrays(ptArrays)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto inPts = vtk::DataArrayTupleRange<3>(this->InPts);
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPts);
    vtkIdType mapId;

    for (; ptId < endPtId; ++ptId)
    {
      if ((mapId = this->PointMap[ptId]) >= 0)
      {
        auto xIn = inPts[ptId];
        auto xOut = outPts[mapId];
        xOut[0] = xIn[0];
        xOut[1] = xIn[1];
        xOut[2] = xIn[2];
        this->PtArrays->Copy(ptId, mapId);
      }
    }
  }
};

// Generate point map for implicit point representations.
template <typename TOP, typename TInputIdType>
struct GenerateImpPoints
{
  vtkDataSet* InPts;
  TOP* OutPts;
  TInputIdType* PointMap;
  ArrayList* PtArrays;

  GenerateImpPoints(vtkDataSet* inPts, TOP* outPts, TInputIdType* ptMap, ArrayList* ptArrays)
    : InPts(inPts)
    , OutPts(outPts)
    , PointMap(ptMap)
    , PtArrays(ptArrays)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPts);
    double xIn[3];
    vtkIdType mapId;

    for (; ptId < endPtId; ++ptId)
    {
      if ((mapId = this->PointMap[ptId]) >= 0)
      {
        this->InPts->GetPoint(ptId, xIn);
        auto xOut = outPts[mapId];
        xOut[0] = xIn[0];
        xOut[1] = xIn[1];
        xOut[2] = xIn[2];
        this->PtArrays->Copy(ptId, mapId);
      }
    }
  }
};

// Base class for point generation workers.
template <typename TInputIdType>
struct GeneratePtsWorker
{
  vtkIdType NumOutputPoints;

  GeneratePtsWorker()
    : NumOutputPoints(0)
  {
  }

  // Create the final point map. This could be threaded (prefix_sum) but
  // performance gains are minimal.
  TInputIdType* GeneratePointMap(
    vtkIdType numInputPts, ExtractCellBoundaries<TInputIdType>* extract)
  {
    // The PointMap has been marked as to which points are being used.
    // This needs to be updated to indicate the output point ids.
    TInputIdType* ptMap = extract->PointMap;
    for (auto ptId = 0; ptId < numInputPts; ++ptId)
    {
      if (ptMap[ptId] == 1)
      {
        ptMap[ptId] = this->NumOutputPoints++;
      }
    }
    return ptMap;
  }
};

// Dispatch to explicit, templated point types
template <typename TInputIdType>
struct ExpPtsWorker : public GeneratePtsWorker<TInputIdType>
{
  template <typename TIP, typename TOP>
  void operator()(TIP* inPts, TOP* outPts, vtkIdType numInputPts, vtkPointData* inPD,
    vtkPointData* outPD, ExtractCellBoundaries<TInputIdType>* extract)
  {
    // Finalize the point map
    TInputIdType* ptMap = this->GeneratePointMap(numInputPts, extract);

    // Now generate all of the points and point attribute data
    ArrayList ptArrays;
    outPD->CopyAllocate(inPD, this->NumOutputPoints);
    ptArrays.AddArrays(this->NumOutputPoints, inPD, outPD, 0.0, false);

    outPts->SetNumberOfTuples(this->NumOutputPoints);
    GenerateExpPoints<TIP, TOP, TInputIdType> genPts(inPts, outPts, ptMap, &ptArrays);
    vtkSMPTools::For(0, numInputPts, genPts);
  }
};

// Dispatch to implicit input points, explicit output points
template <typename TInputIdType>
struct ImpPtsWorker : public GeneratePtsWorker<TInputIdType>
{
  template <typename TOP>
  void operator()(TOP* outPts, vtkDataSet* inPts, vtkIdType numInputPts, vtkPointData* inPD,
    vtkPointData* outPD, ExtractCellBoundaries<TInputIdType>* extract)
  {
    // Finalize the point map
    TInputIdType* ptMap = this->GeneratePointMap(numInputPts, extract);

    // Now generate all of the points and point attribute data
    ArrayList ptArrays;
    outPD->CopyAllocate(inPD, this->NumOutputPoints);
    ptArrays.AddArrays(this->NumOutputPoints, inPD, outPD, 0.0, false);

    outPts->SetNumberOfTuples(this->NumOutputPoints);
    GenerateImpPoints<TOP, TInputIdType> genPts(inPts, outPts, ptMap, &ptArrays);
    vtkSMPTools::For(0, numInputPts, genPts);
  }
};

// Composite threads to produce output cell topology
template <typename TInputIdType, typename TOutputIdType>
struct CompositeCells
{
  const TInputIdType* PointMap;
  ArrayList* CellArrays;
  ExtractCellBoundaries<TInputIdType>* Extractor;
  ThreadOutputType<TInputIdType>* Threads;

  // These are the final composited output cell arrays
  vtkCellArray* Verts;           // output verts
  TOutputIdType* VertsConnPtr;   // output connectivity array
  TOutputIdType* VertsOffsetPtr; // output offsets array

  vtkCellArray* Lines; // output lines
  TOutputIdType* LinesConnPtr;
  TOutputIdType* LinesOffsetPtr;

  vtkCellArray* Polys; // output polys
  TOutputIdType* PolysConnPtr;
  TOutputIdType* PolysOffsetPtr;

  vtkCellArray* Strips; // output triangle strips
  TOutputIdType* StripsConnPtr;
  TOutputIdType* StripsOffsetPtr;

  CompositeCells(TInputIdType* ptMap, ArrayList* cellArrays,
    ExtractCellBoundaries<TInputIdType>* extract, ThreadOutputType<TInputIdType>* threads,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkCellArray* strips)
    : PointMap(ptMap)
    , CellArrays(cellArrays)
    , Extractor(extract)
    , Threads(threads)
    , Verts(verts)
    , VertsConnPtr(nullptr)
    , VertsOffsetPtr(nullptr)
    , Lines(lines)
    , LinesConnPtr(nullptr)
    , LinesOffsetPtr(nullptr)
    , Polys(polys)
    , PolysConnPtr(nullptr)
    , PolysOffsetPtr(nullptr)
    , Strips(strips)
    , StripsConnPtr(nullptr)
    , StripsOffsetPtr(nullptr)
  {
    // Allocate data for the output cell arrays: connectivity and
    // offsets are required to construct a cell array.
    if (extract->VertsNumPts > 0)
    {
      this->AllocateCellArray(extract->VertsNumPts, extract->VertsNumCells, this->Verts,
        this->VertsConnPtr, this->VertsOffsetPtr);
    }
    if (extract->LinesNumPts > 0)
    {
      this->AllocateCellArray(extract->LinesNumPts, extract->LinesNumCells, this->Lines,
        this->LinesConnPtr, this->LinesOffsetPtr);
    }
    if (extract->PolysNumPts > 0)
    {
      this->AllocateCellArray(extract->PolysNumPts, extract->PolysNumCells, this->Polys,
        this->PolysConnPtr, this->PolysOffsetPtr);
    }
    if (extract->StripsNumPts > 0)
    {
      this->AllocateCellArray(extract->StripsNumPts, extract->StripsNumCells, this->Strips,
        this->StripsConnPtr, this->StripsOffsetPtr);
    }
  }

  // Helper function to allocate and construct output cell arrays.
  // Also keep local information to facilitate compositing.
  void AllocateCellArray(vtkIdType connSize, vtkIdType numCells, vtkCellArray* ca,
    TOutputIdType*& connPtr, TOutputIdType*& offsetPtr)
  {
    vtkNew<vtkAOSDataArrayTemplate<TOutputIdType>> outConn;
    outConn->SetNumberOfValues(connSize);
    connPtr = outConn->GetPointer(0);
    vtkNew<vtkAOSDataArrayTemplate<TOutputIdType>> outOffsets;
    outOffsets->SetNumberOfValues(numCells + 1);
    offsetPtr = outOffsets->GetPointer(0);
    offsetPtr[numCells] = connSize;
    ca->SetData(outOffsets, outConn);
  }

  void CompositeCellArray(CellArrayType<TInputIdType>* cat, vtkIdType connOffset, vtkIdType offset,
    vtkIdType cellIdOffset, TOutputIdType* connPtr, TOutputIdType* offsetPtr)
  {
    TInputIdType* cells = cat->Cells.data();
    vtkIdType numCells = cat->GetNumberOfCells();
    connPtr += connOffset;
    offsetPtr += offset;
    vtkIdType offsetVal = connOffset;
    vtkIdType globalCellId = cellIdOffset + offset;

    // If not merging points, we reuse input points and so do not need to
    // produce new points nor point data.
    if (!this->PointMap)
    {
      for (auto cellId = 0; cellId < numCells; ++cellId)
      {
        *offsetPtr++ = static_cast<TOutputIdType>(offsetVal);
        TInputIdType npts = *cells++;
        for (auto i = 0; i < npts; ++i)
        {
          *connPtr++ = static_cast<TOutputIdType>(*cells++);
        }
        offsetVal += npts;
        this->CellArrays->Copy(cat->OrigCellIds[cellId], globalCellId++);
      }
    }
    else // Merging - i.e., using a point map
    {
      for (auto cellId = 0; cellId < numCells; ++cellId)
      {
        *offsetPtr++ = static_cast<TOutputIdType>(offsetVal);
        TInputIdType npts = *cells++;
        for (auto i = 0; i < npts; ++i)
        {
          *connPtr++ = static_cast<TOutputIdType>(this->PointMap[*cells++]);
        }
        offsetVal += npts;
        this->CellArrays->Copy(cat->OrigCellIds[cellId], globalCellId++);
      }
    }
  }

  void operator()(vtkIdType thread, vtkIdType threadEnd)
  {
    auto* extract = this->Extractor;

    for (; thread < threadEnd; ++thread)
    {
      auto tItr = (*this->Threads)[thread];

      if (this->VertsConnPtr)
      {
        this->CompositeCellArray(&tItr->Verts, tItr->VertsConnOffset, tItr->VertsOffset,
          extract->VertsCellIdOffset, this->VertsConnPtr, this->VertsOffsetPtr);
      }
      if (this->LinesConnPtr)
      {
        this->CompositeCellArray(&tItr->Lines, tItr->LinesConnOffset, tItr->LinesOffset,
          extract->LinesCellIdOffset, this->LinesConnPtr, this->LinesOffsetPtr);
      }
      if (this->PolysConnPtr)
      {
        this->CompositeCellArray(&tItr->Polys, tItr->PolysConnOffset, tItr->PolysOffset,
          extract->PolysCellIdOffset, this->PolysConnPtr, this->PolysOffsetPtr);
      }
      if (this->StripsConnPtr)
      {
        this->CompositeCellArray(&tItr->Strips, tItr->StripsConnOffset, tItr->StripsOffset,
          extract->StripsCellIdOffset, this->StripsConnPtr, this->StripsOffsetPtr);
      }
    }
  }
}; // CompositeCells

// Composite threads to produce originating cell ids
template <typename TInputIdType, typename TOutputIdType>
struct CompositeCellIds
{
  ExtractCellBoundaries<TInputIdType>* Extractor;
  ::CompositeCells<TInputIdType, TOutputIdType>* CompositeCells;
  ThreadOutputType<TInputIdType>* Threads;
  vtkIdType* OrigIds;

  CompositeCellIds(ExtractCellBoundaries<TInputIdType>* extract,
    ::CompositeCells<TInputIdType, TOutputIdType>* compositeCells,
    ThreadOutputType<TInputIdType>* threads, vtkIdType* origIds)
    : Extractor(extract)
    , CompositeCells(compositeCells)
    , Threads(threads)
    , OrigIds(origIds)
  {
  }

  void CompositeIds(CellArrayType<TInputIdType>* cat, vtkIdType offset, vtkIdType cellIdOffset)
  {
    vtkIdType numCells = cat->GetNumberOfCells();
    vtkIdType globalCellId = cellIdOffset + offset;

    for (auto cellId = 0; cellId < numCells; ++cellId)
    {
      this->OrigIds[globalCellId++] = cat->OrigCellIds[cellId];
    }
  }

  void operator()(vtkIdType thread, vtkIdType threadEnd)
  {
    auto* extract = this->Extractor;
    auto* compositeCells = this->CompositeCells;

    for (; thread < threadEnd; ++thread)
    {
      auto tItr = (*this->Threads)[thread];

      if (compositeCells->VertsConnPtr)
      {
        this->CompositeIds(&tItr->Verts, tItr->VertsOffset, extract->VertsCellIdOffset);
      }
      if (compositeCells->LinesConnPtr)
      {
        this->CompositeIds(&tItr->Lines, tItr->LinesOffset, extract->LinesCellIdOffset);
      }
      if (compositeCells->PolysConnPtr)
      {
        this->CompositeIds(&tItr->Polys, tItr->PolysOffset, extract->PolysCellIdOffset);
      }
      if (compositeCells->StripsConnPtr)
      {
        this->CompositeIds(&tItr->Strips, tItr->StripsOffset, extract->StripsCellIdOffset);
      }
    }
  }
}; // CompositeCellIds

} // anonymous namespace

//------------------------------------------------------------------------------
// This is currently not threaded. Usually polydata extraction is only used to
// setup originating cell or point ids - this part is threaded.
namespace
{
template <typename TInputIdType>
int ExecutePolyData(vtkGeometryFilter* self, vtkDataSet* dataSetInput, vtkPolyData* output,
  vtkExcludedFaces<TInputIdType>* exc)
{
  vtkPolyData* input = static_cast<vtkPolyData*>(dataSetInput);
  vtkIdType cellId;
  int i;
  int allVisible;
  vtkIdType npts;
  const vtkIdType* pts;
  vtkPoints* p = input->GetPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  vtkIdType newCellId, ptId;
  int visible, type;
  double x[3];
  unsigned char* cellGhosts = nullptr;
  vtkStaticCellLinksTemplate<TInputIdType>* links = (exc == nullptr ? nullptr : exc->Links);

  vtkDebugWithObjectMacro(self, << "Executing geometry filter for poly data input");

  if (cd)
  {
    if (auto ghosts = cd->GetGhostArray())
    {
      cellGhosts = ghosts->GetPointer(0);
    }
  }

  auto cellClipping = self->GetCellClipping();
  auto cellMinimum = self->GetCellMinimum();
  auto cellMaximum = self->GetCellMaximum();
  auto pointClipping = self->GetPointClipping();
  auto pointMinimum = self->GetPointMinimum();
  auto pointMaximum = self->GetPointMaximum();
  auto extentClipping = self->GetExtentClipping();
  auto extent = self->GetExtent();
  if ((!cellClipping) && (!pointClipping) && (!extentClipping))
  {
    allVisible = 1;
  }
  else
  {
    allVisible = 0;
  }

  IdRecorder origCellIds(
    self->GetPassThroughCellIds(), self->GetOriginalCellIdsName(), output->GetCellData());
  IdRecorder origPointIds(
    self->GetPassThroughPointIds(), self->GetOriginalPointIdsName(), output->GetPointData());

  // vtkPolyData points are not culled
  if (origPointIds.PassThru())
  {
    origPointIds.SetNumberOfValues(numPts);
    vtkIdType* origPointIdsPtr = origPointIds.GetPointer();
    vtkSMPTools::For(0, numPts, [&origPointIdsPtr](vtkIdType pId, vtkIdType endPId) {
      for (; pId < endPId; ++pId)
      {
        origPointIdsPtr[pId] = pId;
      }
    });
  }

  // Special case when data is just passed through
  if (allVisible && links == nullptr)
  {
    output->CopyStructure(input);
    outputPD->PassData(pd);
    outputCD->PassData(cd);

    if (origCellIds.PassThru())
    {
      origCellIds.SetNumberOfValues(numCells);
      vtkIdType* origCellIdsPtr = origCellIds.GetPointer();
      vtkSMPTools::For(0, numCells, [&origCellIdsPtr](vtkIdType cId, vtkIdType endCId) {
        for (; cId < endCId; ++cId)
        {
          origCellIdsPtr[cId] = cId;
        }
      });
    }

    return 1;
  }

  // Okay slower path, clipping by cells and/or point ids, or excluding
  // faces. Cells may be culled. Always pass point data (points are not
  // culled).
  output->SetPoints(p);
  outputPD->PassData(pd);

  // Allocate
  //
  origCellIds.Allocate(numCells);
  origPointIds.Allocate(numPts);

  output->AllocateEstimate(numCells, 1);
  outputCD->CopyAllocate(cd, numCells, numCells / 2);
  input->BuildCells(); // needed for GetCellPoints()

  const unsigned char MASKED_CELL =
    self->GetRemoveGhostInterfaces() ? MASKED_CELL_VALUE : MASKED_CELL_VALUE_NOT_VISIBLE;
  vtkIdType progressInterval = numCells / 20 + 1;
  for (cellId = 0; cellId < numCells; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      vtkDebugWithObjectMacro(self, << "Process cell #" << cellId);
      self->UpdateProgress(static_cast<double>(cellId) / numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts && cellGhosts[cellId] & MASKED_CELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    input->GetCellPoints(cellId, npts, pts);

    visible = 1;
    if (!allVisible)
    {
      if (cellClipping && (cellId < cellMinimum || cellId > cellMaximum))
      {
        visible = 0;
      }
      else
      {
        for (i = 0; i < npts; i++)
        {
          ptId = pts[i];
          input->GetPoint(ptId, x);

          if ((pointClipping && (pts[i] < pointMinimum || pts[i] > pointMaximum)) ||
            (extentClipping &&
              (x[0] < extent[0] || x[0] > extent[1] || x[1] < extent[2] || x[1] > extent[3] ||
                x[2] < extent[4] || x[2] > extent[5])))
          {
            visible = 0;
            break;
          }
        }
      }
    }

    // now if visible extract geometry - i.e., cells may be culled
    if ((allVisible || visible) && (!links || !links->MatchesCell(npts, pts)))
    {
      type = input->GetCellType(cellId);
      newCellId = output->InsertNextCell(type, npts, pts);
      outputCD->CopyData(cd, cellId, newCellId);
      origCellIds.Insert(cellId, newCellId);
    } // if visible
  }   // for all cells

  // Update ourselves and release memory
  //
  output->Squeeze();

  vtkDebugWithObjectMacro(self, << "Extracted " << output->GetNumberOfPoints() << " points,"
                                << output->GetNumberOfCells() << " cells.");

  return 1;
}
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::PolyDataExecute(
  vtkDataSet* dataSetInput, vtkPolyData* output, vtkPolyData* excludedFaces)
{
#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds = (dataSetInput->GetNumberOfPoints() > VTK_TYPE_INT32_MAX ||
    dataSetInput->GetNumberOfCells() > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkTypeInt64;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          dataSetInput->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecutePolyData<TInputIdType>(this, dataSetInput, output, &exc);
  }
  else
#endif
  {
    using TInputIdType = vtkTypeInt32;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          dataSetInput->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecutePolyData<TInputIdType>(this, dataSetInput, output, &exc);
  }
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::PolyDataExecute(vtkDataSet* dataSetInput, vtkPolyData* output)
{
  return this->PolyDataExecute(dataSetInput, output, nullptr);
}

namespace
{
//------------------------------------------------------------------------------
struct CharacterizeGrid
{
  vtkUnstructuredGridBase* Grid;

  using CellTypesInformation = vtkGeometryFilterHelper::CellTypesInformation;
  using CellType = vtkGeometryFilterHelper::CellType;
  vtkSMPThreadLocal<CellTypesInformation> TLCellTypesInfo;

  CellTypesInformation CellTypesInfo;
  unsigned char IsLinear;

  CharacterizeGrid(vtkUnstructuredGridBase* grid)
    : Grid(grid)
  {
  }

  void Initialize()
  {
    CellTypesInformation& cellTypesInfo = this->TLCellTypesInfo.Local();
    std::fill(cellTypesInfo.begin(), cellTypesInfo.end(), false);
  }

  static void AssignCellTypeInfo(const unsigned char& cellType, CellTypesInformation& cellTypesInfo)
  {
    switch (cellType)
    {
      case VTK_VERTEX:
      case VTK_POLY_VERTEX:
        if (!cellTypesInfo[CellType::VERTS])
        {
          cellTypesInfo[CellType::VERTS] = true;
        }
        break;
      case VTK_LINE:
      case VTK_POLY_LINE:
        if (!cellTypesInfo[CellType::LINES])
        {
          cellTypesInfo[CellType::LINES] = true;
        }
        break;
      case VTK_TRIANGLE:
      case VTK_QUAD:
      case VTK_POLYGON:
        if (!cellTypesInfo[CellType::POLYS])
        {
          cellTypesInfo[CellType::POLYS] = true;
        }
        break;
      case VTK_TRIANGLE_STRIP:
        if (!cellTypesInfo[CellType::STRIPS])
        {
          cellTypesInfo[CellType::STRIPS] = true;
        }
        break;
      case VTK_EMPTY_CELL:
      case VTK_PIXEL:
      case VTK_TETRA:
      case VTK_VOXEL:
      case VTK_HEXAHEDRON:
      case VTK_WEDGE:
      case VTK_PYRAMID:
      case VTK_PENTAGONAL_PRISM:
      case VTK_HEXAGONAL_PRISM:
      case VTK_CONVEX_POINT_SET:
      case VTK_POLYHEDRON:
        if (!cellTypesInfo[CellType::OTHER_LINEAR_CELLS])
        {
          cellTypesInfo[CellType::OTHER_LINEAR_CELLS] = true;
        }
        break;
      default:
        if (!cellTypesInfo[CellType::NON_LINEAR_CELLS])
        {
          cellTypesInfo[CellType::NON_LINEAR_CELLS] = true;
        }
    }
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    CellTypesInformation& cellTypesInfo = this->TLCellTypesInfo.Local();
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      CharacterizeGrid::AssignCellTypeInfo(
        static_cast<unsigned char>(this->Grid->GetCellType(cellId)), cellTypesInfo);
    }
  }

  void Reduce()
  {
    // initialize
    std::fill(this->CellTypesInfo.begin(), this->CellTypesInfo.end(), false);
    for (const auto& cellTypesInfo : this->TLCellTypesInfo)
    {
      for (auto i = 0; i < CellType::NUM_CELL_TYPES; ++i)
      {
        if (cellTypesInfo[i] && !this->CellTypesInfo[i])
        {
          this->CellTypesInfo[i] = true;
        }
      }
    }
    this->IsLinear = static_cast<unsigned char>(!this->CellTypesInfo[CellType::NON_LINEAR_CELLS]);
  }
};

//------------------------------------------------------------------------------
// Threaded creation to generate array of originating point ids.
template <typename TInputIdType>
void PassPointIds(const char* name, vtkIdType numInputPts, vtkIdType numOutputPts,
  TInputIdType* ptMap, vtkPointData* outPD)
{
  vtkNew<vtkIdTypeArray> origPtIds;
  origPtIds->SetName(name);
  origPtIds->SetNumberOfComponents(1);
  origPtIds->SetNumberOfTuples(numOutputPts);
  outPD->AddArray(origPtIds);
  vtkIdType* origIds = origPtIds->GetPointer(0);

  // Now threaded populate the array
  vtkSMPTools::For(0, numInputPts, [&origIds, &ptMap](vtkIdType ptId, vtkIdType endPtId) {
    for (; ptId < endPtId; ++ptId)
    {
      if (ptMap[ptId] >= 0)
      {
        origIds[ptMap[ptId]] = ptId;
      }
    }
  });
}

//------------------------------------------------------------------------------
// Threaded compositing of originating cell ids.
template <typename TInputIdType, typename TOutputIdType>
void PassCellIds(const char* name, ExtractCellBoundaries<TInputIdType>* extract,
  CompositeCells<TInputIdType, TOutputIdType>* compositeCells,
  ThreadOutputType<TInputIdType>* threads, vtkCellData* outCD)
{
  vtkIdType numOutputCells = extract->NumCells;
  vtkNew<vtkIdTypeArray> origCellIds;
  origCellIds->SetName(name);
  origCellIds->SetNumberOfComponents(1);
  origCellIds->SetNumberOfTuples(numOutputCells);
  outCD->AddArray(origCellIds);
  vtkIdType* origIds = origCellIds->GetPointer(0);

  // Now populate the original cell ids
  CompositeCellIds<TInputIdType, TOutputIdType> compIds(extract, compositeCells, threads, origIds);
  vtkSMPTools::For(0, static_cast<vtkIdType>(threads->size()), compIds);
}

} // anonymous

//----------------------------------------------------------------------------
vtkGeometryFilterHelper* vtkGeometryFilterHelper::CharacterizeUnstructuredGrid(
  vtkUnstructuredGridBase* input)
{
  vtkGeometryFilterHelper* info = new vtkGeometryFilterHelper;

  // Check to see if the data actually has nonlinear cells.  Handling
  // nonlinear cells requires delegation to the appropriate filter.
  CharacterizeGrid characterize(input);
  vtkSMPTools::For(0, input->GetNumberOfCells(), characterize);

  info->CellTypesInfo = characterize.CellTypesInfo;
  info->IsLinear = characterize.IsLinear;

  return info;
}

//----------------------------------------------------------------------------
void vtkGeometryFilterHelper::CopyFilterParams(vtkGeometryFilter* gf, vtkDataSetSurfaceFilter* dssf)
{
  // Helper method to copy key parameters from this filter to an instance of
  // vtkDataSetSurfaceFilter. This is for delegation.
  dssf->SetPieceInvariant(gf->GetPieceInvariant());
  dssf->SetPassThroughCellIds(gf->GetPassThroughCellIds());
  dssf->SetPassThroughPointIds(gf->GetPassThroughPointIds());
  dssf->SetOriginalCellIdsName(gf->GetOriginalCellIdsName());
  dssf->SetOriginalPointIdsName(gf->GetOriginalPointIdsName());
  dssf->SetNonlinearSubdivisionLevel(gf->GetNonlinearSubdivisionLevel());
  dssf->SetFastMode(gf->GetFastMode());
}

//----------------------------------------------------------------------------
void vtkGeometryFilterHelper::CopyFilterParams(vtkDataSetSurfaceFilter* dssf, vtkGeometryFilter* gf)
{
  // Helper method to copy key parameters from an instance of
  // vtkDataSetSurfaceFilter to vtkGeometryFilter. This is for delegation.
  gf->SetPieceInvariant(dssf->GetPieceInvariant());
  gf->SetPassThroughCellIds(dssf->GetPassThroughCellIds());
  gf->SetPassThroughPointIds(dssf->GetPassThroughPointIds());
  gf->SetOriginalCellIdsName(dssf->GetOriginalCellIdsName());
  gf->SetOriginalPointIdsName(dssf->GetOriginalPointIdsName());
  gf->SetNonlinearSubdivisionLevel(dssf->GetNonlinearSubdivisionLevel());
  gf->SetFastMode(dssf->GetFastMode());
}

namespace
{
//----------------------------------------------------------------------------
template <typename TInputIdType>
int ExecuteUnstructuredGrid(vtkGeometryFilter* self, vtkDataSet* dataSetInput, vtkPolyData* output,
  vtkGeometryFilterHelper* info, vtkExcludedFaces<TInputIdType>* exc)
{
  vtkUnstructuredGridBase* uGridBase = vtkUnstructuredGridBase::SafeDownCast(dataSetInput);
  if (uGridBase->GetNumberOfCells() == 0)
  {
    vtkDebugWithObjectMacro(self, << "Nothing to extract");
    return 0;
  }
  vtkUnstructuredGrid* uGrid = vtkUnstructuredGrid::SafeDownCast(uGridBase);

  // If no info, then compute information about the unstructured grid.
  // Depending on the outcome, we may process the data ourselves, or send over
  // to the faster vtkGeometryFilter.
  bool mayDelegate = (info == nullptr && self->GetDelegation());
  bool info_owned = false;
  if (info == nullptr)
  {
    info = vtkGeometryFilterHelper::CharacterizeUnstructuredGrid(uGridBase);
    info_owned = true;
  }

  // Nonlinear cells are handled by vtkDataSetSurfaceFilter
  // non-linear cells using sub-division.
  if (!info->IsLinear && mayDelegate)
  {
    vtkNew<vtkDataSetSurfaceFilter> dssf;
    vtkGeometryFilterHelper::CopyFilterParams(self, dssf.Get());
    dssf->UnstructuredGridExecute(dataSetInput, output, info);
    delete info;
    return 1;
  }
  // fast conversion when input is actually polydata with one cell array
  if (uGrid &&
    (info->HasOnlyVerts() || info->HasOnlyLines() || info->HasOnlyPolys() || info->HasOnlyStrips()))
  {
    vtkNew<vtkPolyData> polyDataInput;
    polyDataInput->SetPoints(uGrid->GetPoints());
    polyDataInput->GetPointData()->ShallowCopy(uGrid->GetPointData());
    if (info->HasOnlyVerts())
    {
      polyDataInput->SetVerts(uGrid->GetCells());
    }
    else if (info->HasOnlyLines())
    {
      polyDataInput->SetLines(uGrid->GetCells());
    }
    else if (info->HasOnlyPolys())
    {
      polyDataInput->SetPolys(uGrid->GetCells());
    }
    else if (info->HasOnlyStrips())
    {
      polyDataInput->SetStrips(uGrid->GetCells());
    }
    polyDataInput->GetCellData()->ShallowCopy(uGrid->GetCellData());
    if (info_owned)
    {
      delete info;
    }
    return ExecutePolyData<TInputIdType>(self, polyDataInput, output, exc);
  }
  if (info_owned)
  {
    delete info;
  }

  vtkIdType cellId;
  vtkIdType npts = 0;
  vtkNew<vtkIdList> pointIdList;
  const vtkIdType* pts = nullptr;
  vtkPoints* inPts = uGridBase->GetPoints();
  vtkIdType numInputPts = uGridBase->GetNumberOfPoints(), numOutputPts;
  vtkIdType numCells = uGridBase->GetNumberOfCells();
  vtkPointData* inPD = uGridBase->GetPointData();
  vtkCellData* inCD = uGridBase->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  vtkNew<vtkAOSDataArrayTemplate<char>> cellVisArray;
  char* cellVis;
  unsigned char* cellGhosts = nullptr;
  unsigned char* pointGhosts = nullptr;

  vtkDebugWithObjectMacro(self, << "Executing geometry filter for unstructured grid input");

  if (inCD)
  {
    if (auto ghosts = inCD->GetGhostArray())
    {
      cellGhosts = ghosts->GetPointer(0);
    }
  }
  if (inPD)
  {
    if (auto ghosts = inPD->GetGhostArray())
    {
      pointGhosts = ghosts->GetPointer(0);
    }
  }

  auto cellClipping = self->GetCellClipping();
  auto cellMinimum = self->GetCellMinimum();
  auto cellMaximum = self->GetCellMaximum();
  auto pointClipping = self->GetPointClipping();
  auto pointMinimum = self->GetPointMinimum();
  auto pointMaximum = self->GetPointMaximum();
  auto extentClipping = self->GetExtentClipping();
  auto extent = self->GetExtent();
  // Determine nature of what we have to do
  if ((!cellClipping) && (!pointClipping) && (!extentClipping))
  {
    cellVis = nullptr;
  }
  else
  {
    cellVisArray->SetNumberOfValues(numCells);
    cellVis = cellVisArray->GetPointer(0);
  }

  outPD->CopyGlobalIdsOn();
  outCD->CopyGlobalIdsOn();

  // Loop over the cells determining what's visible. This could be threaded
  // if necessary - for now it's not used very often so serial.
  if (cellVis)
  {
    double x[3];
    for (cellId = 0; cellId < numCells; ++cellId)
    {
      uGridBase->GetCellPoints(cellId, npts, pts, pointIdList);
      cellVis[cellId] = 1;
      if (cellClipping && (cellId < cellMinimum || cellId > cellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        for (int i = 0; i < npts; i++)
        {
          inPts->GetPoint(pts[i], x);
          if ((pointClipping && (pts[i] < pointMinimum || pts[i] > pointMaximum)) ||
            (extentClipping &&
              (x[0] < extent[0] || x[0] > extent[1] || x[1] < extent[2] || x[1] > extent[3] ||
                x[2] < extent[4] || x[2] > extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          } // point/extent clipping
        }   // for each point
      }     // if point clipping needs checking
    }       // for all cells
  }         // if not all visible

  // Prepare to generate the output. The cell arrays are of course the output vertex,
  // line, polygon, and triangle strip output. The four IdListType's capture the
  // generating cell ids (used later to copy cell attributes).
  vtkNew<vtkPoints> outPts;
  if (self->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  if (!self->GetMerging()) // no merging, just use input points
  {
    output->SetPoints(inPts);
    outPD->PassData(inPD);
  }
  else
  {
    output->SetPoints(outPts);
  }

  vtkNew<vtkCellArray> verts;
  vtkNew<vtkCellArray> lines;
  vtkNew<vtkCellArray> polys;
  vtkNew<vtkCellArray> strips;

  output->SetVerts(verts);
  output->SetLines(lines);
  output->SetPolys(polys);
  output->SetStrips(strips);

  // Threaded visit of each cell to extract boundary features. Each thread gathers
  // output which is then composited into the final vtkPolyData.
  // Keep track of each thread's output, we'll need this later for compositing.
  ThreadOutputType<TInputIdType> threads;

  // Perform the threaded boundary cell extraction. This performs some
  // initial reduction and allocation of the output. It also computes offsets
  // and sizes for allocation and writing of data.
  auto* extract =
    new ExtractUG<TInputIdType>(self, uGridBase, cellVis, cellGhosts, pointGhosts, exc, &threads);
  vtkSMPTools::For(0, numCells, *extract);
  numCells = extract->NumCells;
  self->UpdateProgress(0.8);

  // If merging points, then it's necessary to allocate the points array,
  // configure the point map, and generate the new points. Here we are using
  // an explicit point dispatch (i.e., the point representation is explicitly
  // represented by a data array as we are processing an unstructured grid).
  TInputIdType* ptMap = extract->PointMap;
  if (self->GetMerging())
  {
    using vtkArrayDispatch::Reals;
    using ExpPtsDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
    ExpPtsWorker<TInputIdType> compWorker;
    if (!ExpPtsDispatch::Execute(
          inPts->GetData(), outPts->GetData(), compWorker, numInputPts, inPD, outPD, extract))
    { // Fallback to slowpath for other point types
      compWorker(inPts->GetData(), outPts->GetData(), numInputPts, inPD, outPD, extract);
    }
    numOutputPts = compWorker.NumOutputPoints;

    // Generate originating point ids if requested and merging is
    // on. (Generating these originating point ids only makes sense if the
    // points are merged.)
    if (self->GetPassThroughPointIds())
    {
      PassPointIds(self->GetOriginalPointIdsName(), numInputPts, numOutputPts, ptMap, outPD);
    }
  }
  self->UpdateProgress(0.9);

  // Finally we can composite the output topology.
  ArrayList cellArrays;
  outCD->CopyAllocate(inCD, numCells);
  cellArrays.AddArrays(numCells, inCD, outCD, 0.0, false);

  vtkIdType connectivitySize =
    extract->VertsNumPts + extract->LinesNumPts + extract->PolysNumPts + extract->StripsNumPts;

#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds = connectivitySize > VTK_TYPE_INT32_MAX;
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    CompositeCells<TInputIdType, TOutputIdType> compCells(
      ptMap, &cellArrays, extract, &threads, verts, lines, polys, strips);
    vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

    // Generate originating cell ids if requested.
    if (self->GetPassThroughCellIds())
    {
      PassCellIds<TInputIdType, TOutputIdType>(
        self->GetOriginalCellIdsName(), extract, &compCells, &threads, outCD);
    }
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    CompositeCells<TInputIdType, TOutputIdType> compCells(
      ptMap, &cellArrays, extract, &threads, verts, lines, polys, strips);
    vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

    // Generate originating cell ids if requested.
    if (self->GetPassThroughCellIds())
    {
      PassCellIds<TInputIdType, TOutputIdType>(
        self->GetOriginalCellIdsName(), extract, &compCells, &threads, outCD);
    }
  }
  self->UpdateProgress(1.0);

  vtkDebugWithObjectMacro(self, << "Extracted " << output->GetNumberOfPoints() << " points,"
                                << output->GetNumberOfCells() << " cells.");

  // Clean up and get out
  delete extract;
  return 1;
}
}

//----------------------------------------------------------------------------
int vtkGeometryFilter::UnstructuredGridExecute(vtkDataSet* dataSetInput, vtkPolyData* output,
  vtkGeometryFilterHelper* info, vtkPolyData* excludedFaces)
{
#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds = (dataSetInput->GetNumberOfPoints() > VTK_TYPE_INT32_MAX ||
    dataSetInput->GetNumberOfCells() > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkTypeInt64;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          dataSetInput->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecuteUnstructuredGrid<TInputIdType>(this, dataSetInput, output, info, &exc);
  }
  else
#endif
  {
    using TInputIdType = vtkTypeInt32;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          dataSetInput->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecuteUnstructuredGrid<TInputIdType>(this, dataSetInput, output, info, &exc);
  }
}

//----------------------------------------------------------------------------
int vtkGeometryFilter::UnstructuredGridExecute(vtkDataSet* dataSetInput, vtkPolyData* output)
{
  return this->UnstructuredGridExecute(dataSetInput, output, nullptr, nullptr);
}

namespace
{
//------------------------------------------------------------------------------
template <typename TInputIdType>
int ExecuteStructured(vtkGeometryFilter* self, vtkDataSet* input, vtkPolyData* output,
  int* wholeExtent, vtkExcludedFaces<TInputIdType>* exc, bool* extractFace)
{
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();

  // Setup processing
  bool mergePts = true; // implicit point representations require merging
  vtkPoints* inPts = nullptr;
  if (auto sgrid = vtkStructuredGrid::SafeDownCast(input))
  {
    inPts = sgrid->GetPoints();
    mergePts = self->GetMerging(); // may not be required for explicit
  }

  // We can now extract the boundary topology. This works for all structured
  // types. Here we are only dealing with 3D structured datasets. The 2D cells
  // are handled as a general dataset.
  vtkNew<vtkCellArray> polys;
  output->SetPolys(polys);
  ThreadOutputType<TInputIdType> threads;

  outPD->CopyGlobalIdsOn();
  outCD->CopyGlobalIdsOn();

  ExtractCellBoundaries<TInputIdType>* extStr;
  // if extractFace are defined, it's an AMR block
  if (auto image = vtkImageData::SafeDownCast(input))
  {
    extStr = ExtractStructured<vtkImageData, TInputIdType>::Execute(
      self, image, wholeExtent, extractFace, mergePts, exc, &threads);
  }
  else if (auto sGrid = vtkStructuredGrid::SafeDownCast(input))
  {
    extStr = ExtractStructured<vtkStructuredGrid, TInputIdType>::Execute(
      self, sGrid, wholeExtent, extractFace, mergePts, exc, &threads);
  }
  else if (auto rGrid = vtkRectilinearGrid::SafeDownCast(input))
  {
    extStr = ExtractStructured<vtkRectilinearGrid, TInputIdType>::Execute(
      self, rGrid, wholeExtent, extractFace, mergePts, exc, &threads);
  }
  else
  {
    vtkErrorWithObjectMacro(self, "Data type " << input->GetClassName() << "is not supported.");
    return 0;
  }
  numCells = extStr->NumCells;
  self->UpdateProgress(0.3);

  // Generate the output points
  vtkIdType numInputPts = input->GetNumberOfPoints(), numOutputPts;
  vtkNew<vtkPoints> outPts;
  if (self->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && inPts != nullptr)
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  if (!mergePts && inPts != nullptr) // no merging, just use input points
  {
    output->SetPoints(inPts);
    outPD->PassData(inPD);
  }
  else
  {
    output->SetPoints(outPts);
  }

  if (mergePts && inPts != nullptr) // are these explicit points with merging on?
  {
    using vtkArrayDispatch::Reals;
    using ExpPtsDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
    ExpPtsWorker<TInputIdType> compWorker;
    if (!ExpPtsDispatch::Execute(
          inPts->GetData(), outPts->GetData(), compWorker, numInputPts, inPD, outPD, extStr))
    { // Fallback to slowpath for other point types
      compWorker(inPts->GetData(), outPts->GetData(), numInputPts, inPD, outPD, extStr);
    }
    numOutputPts = compWorker.NumOutputPoints;
  }
  else // implicit point representation
  {
    // Some of these datasets have explicit point representations, we'll generate
    // the geometry (i.e., points) now.
    using vtkArrayDispatch::Reals;
    using ImpPtsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    ImpPtsWorker<TInputIdType> compWorker;
    if (!ImpPtsDispatch::Execute(
          outPts->GetData(), compWorker, input, numInputPts, inPD, outPD, extStr))
    { // Fallback to slowpath for other point types
      compWorker(outPts->GetData(), input, numInputPts, inPD, outPD, extStr);
    }
    numOutputPts = compWorker.NumOutputPoints;
  }

  // Generate originating point ids if requested and merging is
  // on. (Generating these originating point ids only makes sense if the
  // points are merged.)
  TInputIdType* ptMap = extStr->PointMap;
  if (self->GetPassThroughPointIds() && (inPts == nullptr || mergePts))
  {
    PassPointIds(self->GetOriginalPointIdsName(), numInputPts, numOutputPts, ptMap, outPD);
  }
  self->UpdateProgress(0.75);

  // Finally we can composite the output topology.
  ArrayList cellArrays;
  outCD->CopyAllocate(inCD, numCells);
  cellArrays.AddArrays(numCells, inCD, outCD, 0.0, false);

#ifdef VTK_USE_64BIT_IDS
  vtkIdType connectivitySize =
    extStr->VertsNumPts + extStr->LinesNumPts + extStr->PolysNumPts + extStr->StripsNumPts;

  bool use64BitsIds = connectivitySize > VTK_TYPE_INT32_MAX;
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    CompositeCells<TInputIdType, TOutputIdType> compCells(
      ptMap, &cellArrays, extStr, &threads, nullptr, nullptr, polys, nullptr);
    vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

    // Generate originating cell ids if requested.
    if (self->GetPassThroughCellIds())
    {
      PassCellIds<TInputIdType, TOutputIdType>(
        self->GetOriginalCellIdsName(), extStr, &compCells, &threads, outCD);
    }
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    CompositeCells<TInputIdType, TOutputIdType> compCells(
      ptMap, &cellArrays, extStr, &threads, nullptr, nullptr, polys, nullptr);
    vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

    // Generate originating cell ids if requested.
    if (self->GetPassThroughCellIds())
    {
      PassCellIds<TInputIdType, TOutputIdType>(
        self->GetOriginalCellIdsName(), extStr, &compCells, &threads, outCD);
    }
  }
  self->UpdateProgress(1.0);

  vtkDebugWithObjectMacro(self, << "Extracted " << output->GetNumberOfPoints() << " points,"
                                << output->GetNumberOfCells() << " cells.");

  delete extStr;
  return 1;
}
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::StructuredExecute(vtkDataSet* input, vtkPolyData* output,
  vtkInformation* inInfo, vtkPolyData* exc, bool* extractFace)
{
  int wholeExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (input->GetExtentType() == VTK_3D_EXTENT)
  {
    const int* wholeExt32;
    if (inInfo)
    {
      wholeExt32 = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      std::copy(wholeExt32, wholeExt32 + 6, wholeExtent);
    }
  }
  return this->StructuredExecute(input, output, wholeExtent, exc, extractFace);
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::StructuredExecute(
  vtkDataSet* input, vtkPolyData* output, vtkInformation* inInfo, bool* extractFace)
{
  int wholeExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (input->GetExtentType() == VTK_3D_EXTENT)
  {
    const int* wholeExt32;
    if (inInfo)
    {
      wholeExt32 = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      std::copy(wholeExt32, wholeExt32 + 6, wholeExtent);
    }
  }
  return this->StructuredExecute(input, output, wholeExtent, extractFace);
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::StructuredExecute(vtkDataSet* input, vtkPolyData* output, int* wholeExtent,
  vtkPolyData* excludedFaces, bool* extractFace)
{
  int dataDim = -1;
  if (auto imageData = vtkImageData::SafeDownCast(input))
  {
    dataDim = imageData->GetDataDimension();
  }
  else if (auto structured = vtkStructuredGrid::SafeDownCast(input))
  {
    dataDim = structured->GetDataDimension();
  }
  else if (auto rectilinear = vtkRectilinearGrid::SafeDownCast(input))
  {
    dataDim = rectilinear->GetDataDimension();
  }
  assert(dataDim != -1);

  // Delegate to the generic dataset processing if structuredGrid is not 3d or cell/point/extent
  // clipping is requested. Otherwise. use the fast structured algorithms. This is done for
  // simplification purposes.
  if (dataDim != 3 || this->GetCellClipping() || this->GetPointClipping() ||
    this->GetExtentClipping())
  {
    return this->DataSetExecute(input, output, excludedFaces);
  }

#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds = (input->GetNumberOfPoints() > VTK_TYPE_INT32_MAX ||
    input->GetNumberOfCells() > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkTypeInt64;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          input->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecuteStructured<TInputIdType>(this, input, output, wholeExtent, &exc, extractFace);
  }
  else
#endif
  {
    using TInputIdType = vtkTypeInt32;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          input->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecuteStructured<TInputIdType>(this, input, output, wholeExtent, &exc, extractFace);
  }
}

//------------------------------------------------------------------------------
// Process various types of structured datasets.
int vtkGeometryFilter::StructuredExecute(
  vtkDataSet* input, vtkPolyData* output, int* wholeExtent, bool* extractFace)
{
  return this->StructuredExecute(input, output, wholeExtent, nullptr, extractFace);
}

//------------------------------------------------------------------------------
namespace
{
template <typename TInputIdType>
int ExecuteDataSet(vtkGeometryFilter* self, vtkDataSet* input, vtkPolyData* output,
  vtkExcludedFaces<TInputIdType>* exc)
{
  vtkIdType cellId;
  int i;
  vtkIdType numCells = input->GetNumberOfCells();
  double x[3];
  vtkIdType ptId;
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  vtkNew<vtkAOSDataArrayTemplate<char>> cellVisArray;
  char* cellVis;
  unsigned char* cellGhosts = nullptr;
  unsigned char* pointGhosts = nullptr;

  vtkDebugWithObjectMacro(self, << "Executing geometry filter");

  if (inCD)
  {
    if (auto ghosts = inCD->GetGhostArray())
    {
      cellGhosts = ghosts->GetPointer(0);
    }
  }
  if (inPD)
  {
    if (auto ghosts = inPD->GetGhostArray())
    {
      pointGhosts = ghosts->GetPointer(0);
    }
  }

  auto cellClipping = self->GetCellClipping();
  auto cellMinimum = self->GetCellMinimum();
  auto cellMaximum = self->GetCellMaximum();
  auto pointClipping = self->GetPointClipping();
  auto pointMinimum = self->GetPointMinimum();
  auto pointMaximum = self->GetPointMaximum();
  auto extentClipping = self->GetExtentClipping();
  auto extent = self->GetExtent();
  // Determine nature of what we have to do
  if ((!cellClipping) && (!pointClipping) && (!extentClipping))
  {
    cellVis = nullptr;
  }
  else
  {
    cellVisArray->SetNumberOfValues(numCells);
    cellVis = cellVisArray->GetPointer(0);
  }

  // Mark cells as being visible or not
  //
  if (cellVis)
  {
    vtkNew<vtkGenericCell> cell;
    vtkIdList* ptIds;
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (cellClipping && (cellId < cellMinimum || cellId > cellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        input->GetCell(cellId, cell);
        ptIds = cell->GetPointIds();
        vtkIdType ncells = ptIds->GetNumberOfIds();
        for (i = 0; i < ncells; i++)
        {
          ptId = ptIds->GetId(i);
          input->GetPoint(ptId, x);

          if ((pointClipping && (ptId < pointMinimum || ptId > pointMaximum)) ||
            (extentClipping &&
              (x[0] < extent[0] || x[0] > extent[1] || x[1] < extent[2] || x[1] > extent[3] ||
                x[2] < extent[4] || x[2] > extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }
        if (i >= ncells)
        {
          cellVis[cellId] = 1;
        }
      }
    }
  }

  // Create new output points. In a dataset, points are assumed to be
  // implicitly represented, so merging must occur,
  vtkNew<vtkPoints> outPts;
  if (self->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION ||
    self->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  output->SetPoints(outPts);

  vtkNew<vtkCellArray> verts;
  vtkNew<vtkCellArray> lines;
  vtkNew<vtkCellArray> polys;
  vtkNew<vtkCellArray> strips;

  output->SetVerts(verts);
  output->SetLines(lines);
  output->SetPolys(polys);
  output->SetStrips(strips);

  outPD->CopyGlobalIdsOn();
  outCD->CopyGlobalIdsOn();

  // The extraction process for vtkDataSet
  ThreadOutputType<TInputIdType> threads;
  ExtractDS<TInputIdType> extract(self, input, cellVis, cellGhosts, pointGhosts, exc, &threads);
  vtkSMPTools::For(0, numCells, extract);
  numCells = extract.NumCells;
  self->UpdateProgress(0.8);

  // If merging points, then it's necessary to allocate the points
  // array. This will be populated later when the final compositing
  // occurs.
  vtkIdType numInputPts = input->GetNumberOfPoints(), numOutputPts;

  // Generate the new points
  using vtkArrayDispatch::Reals;
  using ImpPtsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  ImpPtsWorker<TInputIdType> compWorker;
  if (!ImpPtsDispatch::Execute(
        outPts->GetData(), compWorker, input, numInputPts, inPD, outPD, &extract))
  { // Fallback to slowpath for other point types
    compWorker(outPts->GetData(), input, numInputPts, inPD, outPD, &extract);
  }
  numOutputPts = compWorker.NumOutputPoints;

  // Generate originating point ids if requested and merging is
  // on. (Generating these originating point ids only makes sense if the
  // points are merged.)
  TInputIdType* ptMap = extract.PointMap;
  if (self->GetPassThroughPointIds())
  {
    PassPointIds(self->GetOriginalPointIdsName(), numInputPts, numOutputPts, ptMap, outPD);
  }
  self->UpdateProgress(0.9);

  // Finally we can composite the output topology.
  ArrayList cellArrays;
  outCD->CopyAllocate(inCD, numCells);
  cellArrays.AddArrays(numCells, inCD, outCD, 0.0, false);

  vtkIdType connectivitySize =
    extract.VertsNumPts + extract.LinesNumPts + extract.PolysNumPts + extract.StripsNumPts;

#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds = connectivitySize > VTK_TYPE_INT32_MAX;
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    CompositeCells<TInputIdType, TOutputIdType> compCells(
      ptMap, &cellArrays, &extract, &threads, verts, lines, polys, strips);
    vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

    // Generate originating cell ids if requested.
    if (self->GetPassThroughCellIds())
    {
      PassCellIds<TInputIdType, TOutputIdType>(
        self->GetOriginalCellIdsName(), &extract, &compCells, &threads, outCD);
    }
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    CompositeCells<TInputIdType, TOutputIdType> compCells(
      ptMap, &cellArrays, &extract, &threads, verts, lines, polys, strips);
    vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

    // Generate originating cell ids if requested.
    if (self->GetPassThroughCellIds())
    {
      PassCellIds<TInputIdType, TOutputIdType>(
        self->GetOriginalCellIdsName(), &extract, &compCells, &threads, outCD);
    }
  }
  self->UpdateProgress(1.0);

  vtkDebugWithObjectMacro(self, << "Extracted " << output->GetNumberOfPoints() << " points,"
                                << output->GetNumberOfCells() << " cells.");

  return 1;
}
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::DataSetExecute(
  vtkDataSet* input, vtkPolyData* output, vtkPolyData* excludedFaces)
{
  bool use64BitsIds =
    (input->GetNumberOfPoints() > VTK_INT_MAX || input->GetNumberOfCells() > VTK_INT_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkIdType;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          input->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecuteDataSet<TInputIdType>(this, input, output, &exc);
  }
  else
  {
    using TInputIdType = int;
    vtkExcludedFaces<TInputIdType> exc;
    if (excludedFaces)
    {
      vtkCellArray* excPolys = excludedFaces->GetPolys();
      if (excPolys->GetNumberOfCells() > 0)
      {
        exc.Links = new vtkStaticCellLinksTemplate<TInputIdType>;
        exc.Links->ThreadedBuildLinks(
          input->GetNumberOfPoints(), excPolys->GetNumberOfCells(), excPolys);
      }
    }
    return ExecuteDataSet<TInputIdType>(this, input, output, &exc);
  }
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::DataSetExecute(vtkDataSet* input, vtkPolyData* output)
{
  return this->DataSetExecute(input, output, nullptr);
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1 && this->PieceInvariant)
  {
    // The special execute for structured data handle boundaries internally.
    // PolyData does not need any ghost levels.
    vtkDataObject* dobj = inInfo->Get(vtkDataObject::DATA_OBJECT());
    if (dobj && !strcmp(dobj->GetClassName(), "vtkUnstructuredGrid"))
    { // Processing does nothing for ghost levels yet so ...
      // Be careful to set output ghost level value one less than default
      // when they are implemented.  I had trouble with multiple executes.
      ++ghostLevels;
    }
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}
