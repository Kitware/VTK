// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataSetSurfaceFilter.h"

#include "vtkBezierCurve.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTriangle.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLogger.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkTetra.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridBase.h"
#include "vtkUnstructuredGridGeometryFilter.h"
#include "vtkVector.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

namespace
{
constexpr int FSize = sizeof(vtkFastGeomQuad);
constexpr int SizeId = sizeof(vtkIdType);
constexpr int PointerSize = sizeof(void*);
constexpr bool Is64BitsSystem = PointerSize == 8;
constexpr bool IsId64Bits = SizeId == 8;
constexpr bool EasyToComputeSize = !Is64BitsSystem || IsId64Bits;
constexpr int FSizeDivSizeId = FSize / SizeId;
inline int sizeofFastQuad(int numPts)
{
  return FSize +
    (EasyToComputeSize ? numPts * SizeId : (numPts + (numPts & 1 /*fast %2*/)) * SizeId);
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
template <typename DataSetT>
bool StructuredExecuteWithBlanking(
  DataSetT* input, vtkPolyData* output, vtkDataSetSurfaceFilter* self)
{
  if (input == nullptr)
  {
    return false;
  }

  int inExtent[6];
  input->GetExtent(inExtent);
  if (vtkStructuredData::GetDataDimension(inExtent) != 3 || !input->HasAnyBlankCells())
  {
    // no need to use this logic for non 3D cells or if no blanking is provided.
    return false;
  }

  vtkLogScopeF(TRACE, "StructuredExecuteWithBlanking (fastMode=%d)", (int)self->GetFastMode());
  vtkNew<vtkPoints> points;
  points->Allocate(input->GetNumberOfPoints() / 2);
  output->AllocateEstimate(input->GetNumberOfCells(), 4);
  output->SetPoints(points);

  // Extracts a either the min (or max) face along the `axis` for the cell
  // identified by `cellId` in the input dataset.
  auto getFace = [&inExtent](const int ijk[3], const int axis, bool minFace) {
    const int iAxis = (axis + 1) % 3;
    const int jAxis = (axis + 2) % 3;

    int ptIjk[3] = { ijk[0], ijk[1], ijk[2] };
    if (!minFace)
    {
      ++ptIjk[axis];
    }

    std::array<vtkIdType, 4> face;
    face[0] = vtkStructuredData::ComputePointIdForExtent(inExtent, ptIjk);

    ++ptIjk[iAxis];
    face[1] = vtkStructuredData::ComputePointIdForExtent(inExtent, ptIjk);

    ++ptIjk[jAxis];
    face[2] = vtkStructuredData::ComputePointIdForExtent(inExtent, ptIjk);

    --ptIjk[iAxis];
    face[3] = vtkStructuredData::ComputePointIdForExtent(inExtent, ptIjk);

    if (minFace)
    {
      // invert face order to get an outside pointing normal.
      return std::array<vtkIdType, 4>({ face[0], face[3], face[2], face[1] });
    }

    return face;
  };

  // Passes data arrays. Also adds `originalIds` the output if `arrayName`
  // non-null.
  auto passData = [](vtkIdTypeArray* originalIds, vtkDataSetAttributes* inputDSA,
                    vtkDataSetAttributes* outputDSA, const char* arrayName) {
    const auto numValues = originalIds->GetNumberOfTuples();
    outputDSA->CopyGlobalIdsOn();
    outputDSA->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());
    outputDSA->CopyAllocate(inputDSA, numValues);

    vtkNew<vtkIdList> fromIds;
    fromIds->SetArray(originalIds->GetPointer(0), numValues); // don't forget to call `Release`

    vtkNew<vtkIdList> toIds;
    toIds->SetNumberOfIds(numValues);
    std::iota(toIds->begin(), toIds->end(), 0);
    outputDSA->CopyData(inputDSA, fromIds, toIds);
    fromIds->Release(); // necessary to avoid double delete.

    // unmark global ids, if any since we don't really preserve input global
    // ids.
    outputDSA->SetActiveAttribute(-1, vtkDataSetAttributes::GLOBALIDS);

    if (arrayName)
    {
      originalIds->SetName(arrayName);
      outputDSA->AddArray(originalIds);
    }
    outputDSA->Squeeze();
  };

  // This map is used to avoid inserting same point multiple times in the
  // output. Since points are looked up using their ids, we simply use that to
  // uniquify points and don't need any locator.
  // key: input point id, value: output point id.
  std::unordered_map<vtkIdType, vtkIdType> pointMap;

  vtkNew<vtkIdTypeArray> originalPtIds;
  originalPtIds->Allocate(input->GetNumberOfPoints());

  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->Allocate(input->GetNumberOfCells());

  auto addFaceToOutput = [&](const std::array<vtkIdType, 4>& ptIds, vtkIdType inCellId) {
    vtkIdType outPtIds[5];
    for (int cc = 0; cc < 4; ++cc)
    {
      auto iter = pointMap.find(ptIds[cc]);
      if (iter != pointMap.end())
      {
        outPtIds[cc] = iter->second;
      }
      else
      {
        double pt[3];
        input->GetPoint(ptIds[cc], pt);
        outPtIds[cc] = points->InsertNextPoint(pt);
        pointMap.insert(std::make_pair(ptIds[cc], outPtIds[cc]));
        originalPtIds->InsertNextValue(ptIds[cc]);
      }
    }
    outPtIds[4] = outPtIds[0];
    output->InsertNextCell(VTK_POLYGON, 5, outPtIds);
    originalCellIds->InsertNextValue(inCellId);
  };

  for (int axis = 0; axis < 3; ++axis)
  {
    const int iAxis = (axis + 1) % 3;
    const int jAxis = (axis + 2) % 3;

    const int extent[6] = { inExtent[2 * iAxis], inExtent[2 * iAxis + 1], inExtent[2 * jAxis],
      inExtent[2 * jAxis + 1], inExtent[2 * axis], inExtent[2 * axis + 1] };

    // iterate over cells
    for (int i = extent[0]; i < extent[1]; ++i)
    {
      int ijk[3];
      ijk[iAxis] = i;
      for (int j = extent[2]; j < extent[3]; ++j)
      {
        ijk[jAxis] = j;

        bool minFace = true;
        for (int k = extent[4]; k < extent[5]; ++k)
        {
          ijk[axis] = k;
          const auto cellId = vtkStructuredData::ComputeCellIdForExtent(inExtent, ijk);
          const bool cellVisible = input->IsCellVisible(cellId);
          if ((minFace && cellVisible) || (!minFace && !cellVisible))
          {
            ijk[axis] =
              minFace ? k : (k - 1); // this ensure correct cell-data is picked for the face.
            addFaceToOutput(getFace(ijk, axis, /*minFace=*/minFace),
              vtkStructuredData::ComputeCellIdForExtent(inExtent, ijk));
            if (self->GetFastMode())
            {
              // in fast mode, we immediately start iterating from the other
              // side instead to find the capping surface. we can ignore
              // interior surfaces for speed.

              // find max-face (reverse order)
              for (int reverseK = extent[5] - 1; reverseK >= k; --reverseK)
              {
                ijk[axis] = reverseK;
                const auto reverseCellId = vtkStructuredData::ComputeCellIdForExtent(inExtent, ijk);
                if (input->IsCellVisible(reverseCellId))
                {
                  addFaceToOutput(getFace(ijk, axis, /*minFace=*/false), reverseCellId);
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
        if (!minFace && !self->GetFastMode())
        {
          const auto cellId = vtkStructuredData::ComputeCellIdForExtent(inExtent, ijk);
          ijk[axis] = extent[5] - 1;
          addFaceToOutput(getFace(ijk, axis, false), cellId);
        }
      }
    }
  }

  // Now copy cell and point data. We want to copy global ids, however we don't
  // want them to be flagged as global ids. So we do this.
  passData(originalPtIds, input->GetPointData(), output->GetPointData(),
    self->GetPassThroughPointIds() ? self->GetOriginalPointIdsName() : nullptr);
  passData(originalCellIds, input->GetCellData(), output->GetCellData(),
    self->GetPassThroughCellIds() ? self->GetOriginalCellIdsName() : nullptr);
  output->Squeeze();
  return true;
}

}

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSetSurfaceFilter::vtkEdgeInterpolationMap
{
public:
  void AddEdge(vtkIdType endpoint1, vtkIdType endpoint2, vtkIdType midpoint)
  {
    if (endpoint1 > endpoint2)
      std::swap(endpoint1, endpoint2);
    Map.insert(std::make_pair(std::make_pair(endpoint1, endpoint2), midpoint));
  }
  vtkIdType FindEdge(vtkIdType endpoint1, vtkIdType endpoint2)
  {
    if (endpoint1 == endpoint2)
    {
      return endpoint1;
    }
    if (endpoint1 > endpoint2)
      std::swap(endpoint1, endpoint2);
    MapType::iterator iter = Map.find(std::make_pair(endpoint1, endpoint2));
    if (iter != Map.end())
    {
      return iter->second;
    }
    else
    {
      return -1;
    }
  }

  void clear() { Map.clear(); }

protected:
  struct HashFunction
  {
  public:
    size_t operator()(std::pair<vtkIdType, vtkIdType> edge) const
    {
      return static_cast<size_t>(edge.first + edge.second);
    }
  };
  typedef std::unordered_map<std::pair<vtkIdType, vtkIdType>, vtkIdType, HashFunction> MapType;
  MapType Map;
};

vtkObjectFactoryNewMacro(vtkDataSetSurfaceFilter);

//------------------------------------------------------------------------------
vtkDataSetSurfaceFilter::vtkDataSetSurfaceFilter()
{
  this->QuadHash = nullptr;
  this->PointMap = nullptr;
  this->EdgeMap = nullptr;
  this->QuadHashLength = 0;
  this->NumberOfNewCells = 0;

  // Quad allocation stuff.
  this->FastGeomQuadArrayLength = 0;
  this->NumberOfFastGeomQuadArrays = 0;
  this->FastGeomQuadArrays = nullptr;
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;
  this->FastMode = false;
  this->PieceInvariant = 0;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
  this->OriginalCellIds = nullptr;
  this->OriginalPointIds = nullptr;
  this->OriginalCellIdsName = nullptr;
  this->OriginalPointIdsName = nullptr;

  this->NonlinearSubdivisionLevel = 1;
  this->MatchBoundariesIgnoringCellOrder = 0;

  this->AllowInterpolation = true;
  this->Delegation = false;
}

//------------------------------------------------------------------------------
vtkDataSetSurfaceFilter::~vtkDataSetSurfaceFilter()
{
  this->SetOriginalCellIdsName(nullptr);
  this->SetOriginalPointIdsName(nullptr);
  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
}

//------------------------------------------------------------------------------
int vtkDataSetSurfaceFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numCells = input->GetNumberOfCells();
  int wholeExt[6] = { 0, -1, 0, -1, 0, -1 };
  if (input->CheckAttributes())
  {
    return 1;
  }

  if (numCells == 0)
  {
    vtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  if (input->GetExtentType() == VTK_3D_EXTENT)
  {
    const int* wholeExt32;
    wholeExt32 = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    std::copy(wholeExt32, wholeExt32 + 6, wholeExt);
  }

  switch (input->GetDataObjectType())
  {
    case VTK_UNSTRUCTURED_GRID:
    case VTK_UNSTRUCTURED_GRID_BASE:
    {
      this->UnstructuredGridExecute(input, output);
      output->CheckAttributes();
      return 1;
    }
    case VTK_RECTILINEAR_GRID:
    {
      auto rg = vtkRectilinearGrid::SafeDownCast(input);
      return this->StructuredExecute(input, output, rg->GetExtent(), wholeExt);
    }
    case VTK_STRUCTURED_GRID:
    {
      auto sg = vtkStructuredGrid::SafeDownCast(input);
      return this->StructuredExecute(input, output, sg->GetExtent(), wholeExt);
    }
    case VTK_UNIFORM_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
    {
      auto img = vtkImageData::SafeDownCast(input);
      return this->StructuredExecute(input, output, img->GetExtent(), wholeExt);
    }
    case VTK_POLY_DATA:
    {
      vtkPolyData* inPd = vtkPolyData::SafeDownCast(input);
      output->ShallowCopy(inPd);
      if (this->PassThroughCellIds)
      {
        // make a 1:1 mapping
        this->OriginalCellIds = vtkIdTypeArray::New();
        this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
        this->OriginalCellIds->SetNumberOfComponents(1);
        vtkCellData* outputCD = output->GetCellData();
        outputCD->AddArray(this->OriginalCellIds);
        vtkIdType numTup = output->GetNumberOfCells();
        this->OriginalCellIds->SetNumberOfValues(numTup);
        for (vtkIdType cId = 0; cId < numTup; cId++)
        {
          if (this->CheckAbort())
          {
            break;
          }
          this->OriginalCellIds->SetValue(cId, cId);
        }
        this->OriginalCellIds->Delete();
        this->OriginalCellIds = nullptr;
      }
      if (this->PassThroughPointIds)
      {
        // make a 1:1 mapping
        this->OriginalPointIds = vtkIdTypeArray::New();
        this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
        this->OriginalPointIds->SetNumberOfComponents(1);
        vtkPointData* outputPD = output->GetPointData();
        outputPD->AddArray(this->OriginalPointIds);
        vtkIdType numTup = output->GetNumberOfPoints();
        this->OriginalPointIds->SetNumberOfValues(numTup);
        for (vtkIdType cId = 0; cId < numTup; cId++)
        {
          if (this->CheckAbort())
          {
            break;
          }
          this->OriginalPointIds->SetValue(cId, cId);
        }
        this->OriginalPointIds->Delete();
        this->OriginalPointIds = nullptr;
      }

      return 1;
    }
    default:
      return this->DataSetExecute(input, output);
  }
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::EstimateStructuredDataArraySizes(
  vtkIdType* ext, vtkIdType* wholeExt, vtkIdType& numPoints, vtkIdType& numCells)
{
  // Sanity Checks
  assert(ext != nullptr);
  assert(wholeExt != nullptr);

  numPoints = numCells = 0;

  // xMin face
  if (ext[0] == wholeExt[0] && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
  {
    numCells += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // xMax face
  if (ext[1] == wholeExt[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    numCells += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMin face
  if (ext[2] == wholeExt[2] && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
  {
    numCells += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMax face
  if (ext[3] == wholeExt[3] && ext[0] != ext[1] && ext[4] != ext[5])
  {
    numCells += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // zMin face
  if (ext[4] == wholeExt[4] && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    numCells += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }
  // zMax face
  if (ext[5] == wholeExt[5] && ext[0] != ext[1] && ext[2] != ext[3])
  {
    numCells += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }
}

//------------------------------------------------------------------------------
int vtkDataSetSurfaceFilter::UniformGridExecute(
  vtkDataSet* input, vtkPolyData* output, vtkIdType* ext, vtkIdType* wholeExt, bool extractface[6])
{
  vtkIdType numPoints, numCells;
  vtkPoints* gridPnts = vtkPoints::New();
  vtkCellArray* gridCells = vtkCellArray::New();

  int originalPassThroughCellIds = this->PassThroughCellIds;

  // Lets figure out the max number of cells and points we are going to have
  numPoints = numCells = 0;
  this->EstimateStructuredDataArraySizes(ext, wholeExt, numPoints, numCells);
  gridPnts->Allocate(numPoints);
  gridCells->AllocateEstimate(numCells, 1);
  output->SetPoints(gridPnts);
  gridPnts->Delete();
  output->SetPolys(gridCells);
  gridCells->Delete();

  // Allocate attributes for copying.
  output->GetPointData()->CopyGlobalIdsOn();
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPoints);
  output->GetCellData()->CopyGlobalIdsOn();
  output->GetCellData()->CopyAllocate(input->GetCellData(), numCells);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(numCells);
    output->GetCellData()->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = vtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPoints);
    output->GetPointData()->AddArray(this->OriginalPointIds);
  }

  // xMin face
  if (extractface[0])
    this->ExecuteFaceQuads(input, output, 0, ext, 0, 1, 2, wholeExt, true);

  // xMax face
  if (extractface[1])
    this->ExecuteFaceQuads(input, output, 1, ext, 0, 2, 1, wholeExt, true);

  // yMin face
  if (extractface[2])
    this->ExecuteFaceQuads(input, output, 0, ext, 1, 2, 0, wholeExt, true);

  // yMax face
  if (extractface[3])
    this->ExecuteFaceQuads(input, output, 1, ext, 1, 0, 2, wholeExt, true);

  // zMin face
  if (extractface[4])
    this->ExecuteFaceQuads(input, output, 0, ext, 2, 0, 1, wholeExt, true);

  // zMax face
  if (extractface[5])
    this->ExecuteFaceQuads(input, output, 1, ext, 2, 1, 0, wholeExt, true);

  output->Squeeze();
  this->PassThroughCellIds = originalPassThroughCellIds;

  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkDataSetSurfaceFilter::StructuredExecute(
  vtkDataSet* input, vtkPolyData* output, vtkIdType* ext, vtkIdType* wholeExt)
{
  if (this->Delegation)
  {
    vtkLogScopeF(
      TRACE, "StructuredExecute Using GeometryFilter (fastMode=%d)", (int)this->GetFastMode());
    vtkNew<vtkGeometryFilter> geometryFilter;
    vtkGeometryFilterHelper::CopyFilterParams(this, geometryFilter);
    int wholeExtent[6];
    std::copy(wholeExt, wholeExt + 6, wholeExtent);
    return geometryFilter->StructuredExecute(input, output, wholeExtent, nullptr, nullptr);
  }

  if (::StructuredExecuteWithBlanking(vtkImageData::SafeDownCast(input), output, this) ||
    ::StructuredExecuteWithBlanking(vtkStructuredGrid::SafeDownCast(input), output, this) ||
    ::StructuredExecuteWithBlanking(vtkRectilinearGrid::SafeDownCast(input), output, this))
  {
    return 1;
  }

  return this->StructuredExecuteNoBlanking(input, output, ext, wholeExt);
}

//------------------------------------------------------------------------------
// It is a pain that structured data sets do not share a common super class
// other than data set, and data set does not allow access to extent!
int vtkDataSetSurfaceFilter::StructuredExecuteNoBlanking(
  vtkDataSet* input, vtkPolyData* output, vtkIdType* ext, vtkIdType* wholeExt)
{
  vtkRectilinearGrid* rgrid = vtkRectilinearGrid::SafeDownCast(input);
  vtkStructuredGrid* sgrid = vtkStructuredGrid::SafeDownCast(input);
  if (rgrid || sgrid)
  {
    // Fetch the grid dimension
    int iext[6];
    std::copy(ext, ext + 6, iext);
    int dimension = vtkStructuredData::GetDataDimension(iext);

    if (dimension == 1)
    {
      // Use specialized filter in case of 1D grid
      if (rgrid)
      {
        vtkNew<vtkRectilinearGridGeometryFilter> filter;
        filter->SetInputData(input);
        filter->SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        filter->SetContainerAlgorithm(this);
        filter->Update();
        output->ShallowCopy(filter->GetOutput());
        return 1;
      }
      else if (sgrid)
      {
        vtkNew<vtkStructuredGridGeometryFilter> filter;
        filter->SetInputData(input);
        filter->SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        filter->SetContainerAlgorithm(this);
        filter->Update();
        output->ShallowCopy(filter->GetOutput());
        return 1;
      }
    }
  }

  vtkIdType numPoints, cellArraySize;
  vtkCellArray* outPolys;
  vtkPoints* outPoints;

  // Cell Array Size is a pretty good estimate.

  // Lets figure out how many cells and points we are going to have.
  // It may be overkill comptuing the exact amount, but we can do it, so ...
  cellArraySize = numPoints = 0;
  // xMin face
  if (ext[0] == wholeExt[0] && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
  {
    cellArraySize += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // xMax face
  if (ext[1] == wholeExt[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    cellArraySize += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMin face
  if (ext[2] == wholeExt[2] && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMax face
  if (ext[3] == wholeExt[3] && ext[0] != ext[1] && ext[4] != ext[5])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // zMin face
  if (ext[4] == wholeExt[4] && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }
  // zMax face
  if (ext[5] == wholeExt[5] && ext[0] != ext[1] && ext[2] != ext[3])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }

  int originalPassThroughCellIds = this->PassThroughCellIds;
  outPolys = vtkCellArray::New();
  outPolys->AllocateEstimate(cellArraySize, 4);
  output->SetPolys(outPolys);
  outPolys->Delete();
  outPoints = vtkPoints::New();
  int dataType;
  switch (input->GetDataObjectType())
  {
    case VTK_RECTILINEAR_GRID:
    {
      dataType = rgrid->GetXCoordinates()->GetDataType();
      break;
    }
    case VTK_STRUCTURED_GRID:
    {
      dataType = sgrid->GetPoints()->GetDataType();
      break;
    }
    case VTK_UNIFORM_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
    {
      dataType = VTK_DOUBLE;
      break;
    }
    default:
      dataType = VTK_DOUBLE;
      vtkErrorMacro("Invalid data set type: " << input->GetDataObjectType());
      outPoints->Delete();
      return 1;
  }

  outPoints->SetDataType(dataType);
  outPoints->Allocate(numPoints);
  output->SetPoints(outPoints);
  outPoints->Delete();

  // Allocate attributes for copying.
  output->GetPointData()->CopyGlobalIdsOn();
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPoints);
  output->GetCellData()->CopyGlobalIdsOn();
  output->GetCellData()->CopyAllocate(input->GetCellData(), cellArraySize);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(cellArraySize);
    output->GetCellData()->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = vtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPoints);
    output->GetPointData()->AddArray(this->OriginalPointIds);
  }

  // xMin face
  this->ExecuteFaceQuads(input, output, 0, ext, 0, 1, 2, wholeExt);
  // xMax face
  this->ExecuteFaceQuads(input, output, 1, ext, 0, 2, 1, wholeExt);
  // yMin face
  this->ExecuteFaceQuads(input, output, 0, ext, 1, 2, 0, wholeExt);
  // yMax face
  this->ExecuteFaceQuads(input, output, 1, ext, 1, 0, 2, wholeExt);
  // zMin face
  this->ExecuteFaceQuads(input, output, 0, ext, 2, 0, 1, wholeExt);
  // zMax face
  this->ExecuteFaceQuads(input, output, 1, ext, 2, 1, 0, wholeExt);

  output->Squeeze();
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  this->PassThroughCellIds = originalPassThroughCellIds;

  this->CheckAbort();

  return 1;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::ExecuteFaceQuads(vtkDataSet* input, vtkPolyData* output, int maxFlag,
  vtkIdType* ext, int aAxis, int bAxis, int cAxis, vtkIdType* wholeExt, bool checkVisibility)
{
  vtkPoints* outPts;
  vtkCellArray* outPolys;
  vtkPointData *inPD, *outPD;
  vtkCellData *inCD, *outCD;
  vtkIdType pInc[3];
  vtkIdType qInc[3];
  vtkIdType cOutInc;
  double pt[3];
  vtkIdType inStartPtId;
  vtkIdType inStartCellId;
  vtkIdType outStartPtId;
  vtkIdType outPtId;
  vtkIdType inId, outId;
  vtkIdType ib, ic;
  int aA2, bA2, cA2;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  pInc[0] = 1;
  pInc[1] = (ext[1] - ext[0] + 1);
  pInc[2] = (ext[3] - ext[2] + 1) * pInc[1];
  // quad increments (cell increments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = ext[1] - ext[0];
  // The conditions are for when we have one or more degenerate axes (2d or 1d cells).
  if (qInc[1] == 0)
  {
    qInc[1] = 1;
  }
  qInc[2] = (ext[3] - ext[2]) * qInc[1];
  if (qInc[2] == 0)
  {
    qInc[2] = qInc[1];
  }

  // Temporary variables to avoid many multiplications.
  aA2 = aAxis * 2;
  bA2 = bAxis * 2;
  cA2 = cAxis * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2 + 1] || ext[cA2] == ext[cA2 + 1])
  {
    return;
  }
  if (maxFlag)
  {
    if (ext[aA2 + 1] < wholeExt[aA2 + 1])
    {
      return;
    }
  }
  else
  { // min faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2 + 1] || ext[aA2] > wholeExt[aA2])
    {
      return;
    }
  }

  // Assuming no ghost cells ...
  inStartPtId = inStartCellId = 0;
  // I put this confusing conditional to fix a regression test.
  // If we are creating a maximum face, then we indeed have to offset
  // the input cell Ids.  However, vtkGeometryFilter created a 2d
  // image as a max face, but the cells are copied as a min face (no
  // offset).  Hence maxFlag = 1 and there should be no offset.
  if (maxFlag && ext[aA2] < ext[1 + aA2])
  {
    inStartPtId = pInc[aAxis] * (ext[aA2 + 1] - ext[aA2]);
    inStartCellId = qInc[aAxis] * (ext[aA2 + 1] - ext[aA2] - 1);
  }

  vtkUniformGrid* grid = static_cast<vtkUniformGrid*>(input);
  assert(grid != nullptr);

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
    {
      inId = inStartPtId + (ib - ext[bA2]) * pInc[bAxis] + (ic - ext[cA2]) * pInc[cAxis];
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD, inId, outId);
      this->RecordOrigPointId(outId, inId);
    }
  }

  // Do the cells.
  cOutInc = ext[bA2 + 1] - ext[bA2] + 1;

  outPolys = output->GetPolys();

  // Old method for creating quads (needed for cell data.).
  for (ic = ext[cA2]; ic < ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib < ext[bA2 + 1]; ++ib)
    {
      outPtId = outStartPtId + (ib - ext[bA2]) + (ic - ext[cA2]) * cOutInc;
      inId = inStartCellId + (ib - ext[bA2]) * qInc[bAxis] + (ic - ext[cA2]) * qInc[cAxis];

      if (checkVisibility && grid->IsCellVisible(inId))
      {
        outId = outPolys->InsertNextCell(4);
        outPolys->InsertCellPoint(outPtId);
        outPolys->InsertCellPoint(outPtId + cOutInc);
        outPolys->InsertCellPoint(outPtId + cOutInc + 1);
        outPolys->InsertCellPoint(outPtId + 1);
        // Copy cell data.
        outCD->CopyData(inCD, inId, outId);
        this->RecordOrigCellId(outId, inId);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::ExecuteFaceQuads(vtkDataSet* input, vtkPolyData* output, int maxFlag,
  vtkIdType* ext, int aAxis, int bAxis, int cAxis, vtkIdType* wholeExt)
{
  vtkPoints* outPts;
  vtkCellArray* outPolys;
  vtkPointData *inPD, *outPD;
  vtkCellData *inCD, *outCD;
  vtkIdType pInc[3];
  vtkIdType qInc[3];
  vtkIdType cOutInc;
  double pt[3];
  vtkIdType inStartPtId;
  vtkIdType inStartCellId;
  vtkIdType outStartPtId;
  vtkIdType outPtId;
  vtkIdType inId, outId;
  vtkIdType ib, ic;
  int aA2, bA2, cA2;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  pInc[0] = 1;
  pInc[1] = (ext[1] - ext[0] + 1);
  pInc[2] = (ext[3] - ext[2] + 1) * pInc[1];
  // quad increments (cell increments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = ext[1] - ext[0];
  // The conditions are for when we have one or more degenerate axes (2d or 1d cells).
  if (qInc[1] == 0)
  {
    qInc[1] = 1;
  }
  qInc[2] = (ext[3] - ext[2]) * qInc[1];
  if (qInc[2] == 0)
  {
    qInc[2] = qInc[1];
  }

  // Temporary variables to avoid many multiplications.
  aA2 = aAxis * 2;
  bA2 = bAxis * 2;
  cA2 = cAxis * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2 + 1] || ext[cA2] == ext[cA2 + 1])
  {
    return;
  }
  if (maxFlag)
  {
    if (ext[aA2 + 1] < wholeExt[aA2 + 1])
    {
      return;
    }
  }
  else
  { // min faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2 + 1] || ext[aA2] > wholeExt[aA2])
    {
      return;
    }
  }

  // Assuming no ghost cells ...
  inStartPtId = inStartCellId = 0;
  // I put this confusing conditional to fix a regression test.
  // If we are creating a maximum face, then we indeed have to offset
  // the input cell Ids.  However, vtkGeometryFilter created a 2d
  // image as a max face, but the cells are copied as a min face (no
  // offset).  Hence maxFlag = 1 and there should be no offset.
  if (maxFlag && ext[aA2] < ext[1 + aA2])
  {
    inStartPtId = pInc[aAxis] * (ext[aA2 + 1] - ext[aA2]);
    inStartCellId = qInc[aAxis] * (ext[aA2 + 1] - ext[aA2] - 1);
  }

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
    {
      inId = inStartPtId + (ib - ext[bA2]) * pInc[bAxis] + (ic - ext[cA2]) * pInc[cAxis];
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD, inId, outId);
      this->RecordOrigPointId(outId, inId);
    }
  }

  // Do the cells.
  cOutInc = ext[bA2 + 1] - ext[bA2] + 1;

  outPolys = output->GetPolys();

  // Old method for creating quads (needed for cell data.).
  for (ic = ext[cA2]; ic < ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib < ext[bA2 + 1]; ++ib)
    {
      outPtId = outStartPtId + (ib - ext[bA2]) + (ic - ext[cA2]) * cOutInc;
      inId = inStartCellId + (ib - ext[bA2]) * qInc[bAxis] + (ic - ext[cA2]) * qInc[cAxis];

      outId = outPolys->InsertNextCell(4);
      outPolys->InsertCellPoint(outPtId);
      outPolys->InsertCellPoint(outPtId + cOutInc);
      outPolys->InsertCellPoint(outPtId + cOutInc + 1);
      outPolys->InsertCellPoint(outPtId + 1);
      // Copy cell data.
      outCD->CopyData(inCD, inId, outId);
      this->RecordOrigCellId(outId, inId);
    }
  }
}

//------------------------------------------------------------------------------
int vtkDataSetSurfaceFilter::DataSetExecute(vtkDataSet* input, vtkPolyData* output)
{
  vtkIdType cellId, newCellId;
  int i, j;
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkCell* face;
  double x[3];
  vtkIdList* cellIds;
  vtkIdList* pts;
  vtkPoints* newPts;
  vtkIdType ptId, pt;
  int npts;
  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  if (numCells == 0)
  {
    vtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(numCells);
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = vtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPts);
    outputPD->AddArray(this->OriginalPointIds);
  }

  cellIds = vtkIdList::New();
  pts = vtkIdList::New();

  vtkDebugMacro(<< "Executing geometry filter");

  // Allocate
  //
  newPts = vtkPoints::New();
  // we don't know what type of data the input points are so
  // we keep the output points to have the default type (float)
  newPts->Allocate(numPts, numPts / 2);
  output->AllocateEstimate(numCells, 3);
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  // Traverse cells to extract geometry
  //
  bool abort = false;
  vtkIdType progressInterval = numCells / 20 + 1;

  for (cellId = 0; cellId < numCells && !abort; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      vtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->CheckAbort();
    }
    vtkCell* cell = input->GetCell(cellId);
    switch (cell->GetCellDimension())
    {
      // create new points and then cell
      case 0:
      case 1:
      case 2:
      {
        int type = cell->GetCellType();
        if (type == VTK_EMPTY_CELL)
        {
          // Empty cells are not supported by vtkPolyData
          break;
        }

        npts = cell->GetNumberOfPoints();
        pts->Reset();
        for (i = 0; i < npts; i++)
        {
          ptId = cell->GetPointId(i);
          input->GetPoint(ptId, x);
          pt = newPts->InsertNextPoint(x);
          outputPD->CopyData(pd, ptId, pt);
          this->RecordOrigPointId(pt, ptId);
          pts->InsertId(i, pt);
        }
        newCellId = output->InsertNextCell(type, pts);
        if (newCellId > 0)
        {
          outputCD->CopyData(cd, cellId, newCellId);
          this->RecordOrigCellId(newCellId, cellId);
        }
        break;
      }
      case 3:
        for (j = 0; j < cell->GetNumberOfFaces(); j++)
        {
          face = cell->GetFace(j);
          input->GetCellNeighbors(cellId, face->PointIds, cellIds);
          bool noNeighbors = cellIds->GetNumberOfIds() <= 0;
          if (noNeighbors)
          {
            npts = face->GetNumberOfPoints();
            pts->Reset();
            for (i = 0; i < npts; i++)
            {
              ptId = face->GetPointId(i);
              input->GetPoint(ptId, x);
              pt = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd, ptId, pt);
              this->RecordOrigPointId(pt, ptId);
              pts->InsertId(i, pt);
            }
            newCellId = output->InsertNextCell(face->GetCellType(), pts);
            if (newCellId > 0)
            {
              outputCD->CopyData(cd, cellId, newCellId);
              this->RecordOrigCellId(newCellId, cellId);
            }
          }
        }
        break;
    } // switch
  }   // for all cells

  vtkDebugMacro(<< "Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  // free storage
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int vtkDataSetSurfaceFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
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

//------------------------------------------------------------------------------
int vtkDataSetSurfaceFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PieceInvariant: " << this->GetPieceInvariant() << endl;
  os << indent << "PassThroughCellIds: " << (this->GetPassThroughCellIds() ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: " << (this->GetPassThroughPointIds() ? "On\n" : "Off\n");
  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;
  os << indent << "NonlinearSubdivisionLevel: " << this->GetNonlinearSubdivisionLevel() << endl;
  os << indent
     << "MatchBoundariesIgnoringCellOrder: " << this->GetMatchBoundariesIgnoringCellOrder() << endl;
  os << indent << "FastMode: " << this->GetFastMode() << endl;
  os << indent << "AllowInterpolation: " << this->GetAllowInterpolation() << endl;
  os << indent << "Delegation: " << this->GetDelegation() << endl;
}

//========================================================================
// Coordinate the delegation process.
int vtkDataSetSurfaceFilter::UnstructuredGridExecute(vtkDataSet* dataSetInput, vtkPolyData* output)
{
  switch (dataSetInput->GetDataObjectType())
  {
    case VTK_UNSTRUCTURED_GRID:
      return this->UnstructuredGridExecute(dataSetInput, output, nullptr);
    case VTK_UNSTRUCTURED_GRID_BASE:
      return this->UnstructuredGridBaseExecute(dataSetInput, output);
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
// This method may delegate to vtkGeometryFilter. The "info", if passed in,
// provides information about the unstructured grid. This avoids the possibility of
// repeated evaluations, and back and forth delegation, as vtkGeometryFilter and
// vtkDataSetSurfaceFilter coordinate their efforts.
int vtkDataSetSurfaceFilter::UnstructuredGridExecute(
  vtkDataSet* dataSetInput, vtkPolyData* output, vtkGeometryFilterHelper* info)
{
  vtkUnstructuredGrid* input = vtkUnstructuredGrid::SafeDownCast(dataSetInput);

  // If no info, then compute information about the unstructured grid.
  // Depending on the outcome, we may process the data ourselves, or send over
  // to the faster vtkGeometryFilter.
  bool mayDelegate = (info == nullptr && this->Delegation);
  bool info_owned = false;
  if (info == nullptr)
  {
    info = vtkGeometryFilterHelper::CharacterizeUnstructuredGrid(input);
    info_owned = true;
  }
  bool handleSubdivision = (!info->IsLinear);

  // Before we start doing anything interesting, check if we need handle
  // non-linear cells using sub-division.
  if (info->IsLinear && mayDelegate)
  {
    vtkNew<vtkGeometryFilter> gf;
    vtkGeometryFilterHelper::CopyFilterParams(this, gf.Get());
    gf->UnstructuredGridExecute(dataSetInput, output, info, nullptr);
    delete info;
    return 1;
  }
  if (info_owned)
  {
    delete info;
  }

  // If here, the data is gnarly and this filter will process it.
  return this->UnstructuredGridExecuteInternal(input, output, handleSubdivision);
}

//------------------------------------------------------------------------------
// Unoptimized version of UnstructuredGridExecute for non vtkUnstructuredGrid instances
int vtkDataSetSurfaceFilter::UnstructuredGridBaseExecute(
  vtkDataSet* dataSetInput, vtkPolyData* output)
{
  vtkUnstructuredGridBase* input = vtkUnstructuredGridBase::SafeDownCast(dataSetInput);

  // Before we start doing anything interesting, check if we need handle
  // non-linear cells using sub-division.
  bool handleSubdivision = false;
  if (this->NonlinearSubdivisionLevel >= 1)
  {
    // Check to see if the data actually has nonlinear cells.  Handling
    // nonlinear cells adds unnecessary work if we only have linear cells.
    vtkIdType numCells = input->GetNumberOfCells();
    if (input->IsHomogeneous())
    {
      if (numCells >= 1)
      {
        handleSubdivision = !vtkCellTypes::IsLinear(input->GetCellType(0));
      }
    }
    else
    {
      for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
      {
        if (!vtkCellTypes::IsLinear(input->GetCellType(cellId)))
        {
          handleSubdivision = true;
          break;
        }
      }
    }
  }

  return this->UnstructuredGridExecuteInternal(input, output, handleSubdivision);
}

//========================================================================
// Tris are now degenerate quads so we only need one hash table.
// We might want to change the method names from QuadHash to just Hash.
int vtkDataSetSurfaceFilter::UnstructuredGridExecuteInternal(
  vtkUnstructuredGridBase* input, vtkPolyData* output, bool handleSubdivision)
{
  vtkSmartPointer<vtkUnstructuredGrid> tempInput;
  if (handleSubdivision)
  {
    // Since this filter only properly subdivides 2D cells past
    // level 1, we convert 3D cells to 2D by using
    // vtkUnstructuredGridGeometryFilter.
    vtkNew<vtkUnstructuredGridGeometryFilter> uggf;
    vtkNew<vtkUnstructuredGrid> clone;
    clone->ShallowCopy(input);
    uggf->SetInputData(clone);
    uggf->SetPassThroughCellIds(this->PassThroughCellIds);
    uggf->SetOriginalCellIdsName(this->GetOriginalCellIdsName());
    uggf->SetPassThroughPointIds(this->PassThroughPointIds);
    uggf->SetMatchBoundariesIgnoringCellOrder(this->MatchBoundariesIgnoringCellOrder);
    uggf->SetOriginalPointIdsName(this->GetOriginalPointIdsName());
    uggf->DuplicateGhostCellClippingOff();
    uggf->SetContainerAlgorithm(this);
    // Disable point merging as it may prevent the correct visualization
    // of non-continuous attributes.
    uggf->MergingOff();
    uggf->Update();

    tempInput = vtkSmartPointer<vtkUnstructuredGrid>::New();
    tempInput->ShallowCopy(uggf->GetOutputDataObject(0));
    input = tempInput;

    if (this->CheckAbort())
    {
      return 1;
    }
  }

  vtkUnsignedCharArray* ghosts = input->GetPointGhostArray();
  vtkUnsignedCharArray* ghostCells = input->GetCellGhostArray();
  vtkCellArray* newVerts;
  vtkCellArray* newLines;
  vtkCellArray* newPolys;
  vtkPoints* newPts;
  int progressCount;
  vtkIdType i, j, k;
  int cellType;
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkGenericCell* cell;
  vtkNew<vtkIdList> pointIdList;
  const vtkIdType* ids;
  vtkIdType numFacePts;
  vtkIdType inPtId, outPtId, numCellPts;
  vtkPointData* inputPD = input->GetPointData();
  vtkCellData* inputCD = input->GetCellData();
  vtkFieldData* inputFD = input->GetFieldData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  vtkFieldData* outputFD = output->GetFieldData();
  vtkFastGeomQuad* q;

  // Shallow copy field data not associated with points or cells
  outputFD->ShallowCopy(inputFD);

  // These are for the default case/
  vtkIdList* pts;
  vtkCell* face;
  int flag2D = 0;

  // These are for subdividing quadratic cells
  std::vector<double> parametricCoords;
  std::unique_ptr<vtkEdgeInterpolationMap> localEdgeMap(new vtkEdgeInterpolationMap());
  vtkIdList* outPts;
  vtkIdList* pts2;

  pts = vtkIdList::New();
  outPts = vtkIdList::New();
  pts2 = vtkIdList::New();
  cell = vtkGenericCell::New();
  std::vector<double> weights;

  this->NumberOfNewCells = 0;
  this->InitializeQuadHash(numPts);

  // Allocate
  //
  newPts = vtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetData()->GetDataType());
  newPts->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->AllocateEstimate(numCells, 3);
  newVerts = vtkCellArray::New();
  newLines = vtkCellArray::New();

  if (this->NonlinearSubdivisionLevel < 2)
  {
    outputPD->CopyGlobalIdsOn();
    outputPD->CopyAllocate(inputPD, numPts, numPts / 2);
  }
  else
  {
    outputPD->InterpolateAllocate(inputPD, numPts, numPts / 2);
  }
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(inputCD, numCells, numCells / 2);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = vtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
  }

  // First insert all points.  Points have to come first in poly data.
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    cellType = input->GetCellType(cellId);

    // A couple of common cases to see if things go faster.
    if (cellType == VTK_VERTEX || cellType == VTK_POLY_VERTEX)
    {
      input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
      newVerts->InsertNextCell(numCellPts);
      for (i = 0; i < numCellPts; i++)
      {
        outPtId = this->GetOutputPointId(ids[i], input, newPts, outputPD);
        newVerts->InsertCellPoint(outPtId);
      }
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
  }

  // Traverse cells to extract geometry
  //
  progressCount = 0;
  bool abort = false;
  vtkIdType progressInterval = numCells / 20 + 1;

  // First insert all points lines in output and 3D geometry in hash.
  // Save 2D geometry for second pass.
  for (vtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    // We skip cells marked as hidden
    if (ghostCells &&
      (ghostCells->GetValue(cellId) & vtkDataSetAttributes::CellGhostTypes::HIDDENCELL))
    {
      continue;
    }

    // Progress and abort method support
    if (progressCount >= progressInterval)
    {
      vtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->CheckAbort();
      progressCount = 0;
    }
    progressCount++;

    cellType = input->GetCellType(cellId);

    switch (cellType)
    {
      case VTK_VERTEX:
      case VTK_POLY_VERTEX:
      case VTK_EMPTY_CELL:
        // Do nothing -- these were handled previously.
        break;

      case VTK_LINE:
      case VTK_POLY_LINE:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        newLines->InsertNextCell(numCellPts);
        for (i = 0; i < numCellPts; i++)
        {
          outPtId = this->GetOutputPointId(ids[i], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }

        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
        break;
      case VTK_LAGRANGE_CURVE:
      case VTK_QUADRATIC_EDGE:
      case VTK_CUBIC_LINE:
      {
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);

        if (this->NonlinearSubdivisionLevel <= 1)
        {
          int numCellPtsAfterSubdivision = this->NonlinearSubdivisionLevel == 0 ? 2 : numCellPts;
          newLines->InsertNextCell(numCellPtsAfterSubdivision);
          outPtId = this->GetOutputPointId(ids[0], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
          for (i = 2; i < numCellPtsAfterSubdivision; i++)
          {
            outPtId = this->GetOutputPointId(ids[i], input, newPts, outputPD);
            newLines->InsertCellPoint(outPtId);
          }
          outPtId = this->GetOutputPointId(ids[1], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }
        else
        {
          int numDeltaPtsAfterSubdivision = std::pow(2, this->NonlinearSubdivisionLevel - 1);
          int numCellPtsAfterSubdivision = numDeltaPtsAfterSubdivision * (numCellPts - 1) + 1;
          newLines->InsertNextCell(numCellPtsAfterSubdivision);
          outPtId = this->GetOutputPointId(ids[0], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
          double paramCoordDelta = 1. / (numCellPtsAfterSubdivision - 1);
          input->GetCell(cellId, cell);
          weights.resize(cell->GetNumberOfPoints());
          double inParamCoords[3];
          inParamCoords[1] = inParamCoords[2] = 0.;
          for (i = 0; i < (numCellPts - 1); i++)
          {
            for (j = 0; j < numDeltaPtsAfterSubdivision - 1; j++)
            {
              inParamCoords[0] = paramCoordDelta * (numDeltaPtsAfterSubdivision * i + j + 1);
              outPtId = GetInterpolatedPointId(
                input, cell, inParamCoords, weights.data(), newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
            }
            if (i < numCellPts - 2)
            {
              outPtId = this->GetOutputPointId(ids[i + 2], input, newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
            }
          }
          outPtId = this->GetOutputPointId(ids[1], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }
        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
        break;
      }
      case VTK_BEZIER_CURVE:
      {
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        if (this->NonlinearSubdivisionLevel == 0 || !AllowInterpolation)
        {
          int numCellPtsAfterSubdivision = this->NonlinearSubdivisionLevel == 0 ? 2 : numCellPts;
          newLines->InsertNextCell(numCellPtsAfterSubdivision);
          outPtId = this->GetOutputPointId(ids[0], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
          for (i = 2; i < numCellPtsAfterSubdivision; i++)
          {
            outPtId = this->GetOutputPointId(ids[i], input, newPts, outputPD);
            newLines->InsertCellPoint(outPtId);
          }
          outPtId = this->GetOutputPointId(ids[1], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }
        else
        {
          int numDeltaPtsAfterSubdivision = std::pow(2, this->NonlinearSubdivisionLevel - 1);
          int numCellPtsAfterSubdivision = numDeltaPtsAfterSubdivision * (numCellPts - 1) + 1;
          newLines->InsertNextCell(numCellPtsAfterSubdivision);
          input->GetCell(cellId, cell);
          input->SetCellOrderAndRationalWeights(cellId, cell);
          weights.resize(cell->GetNumberOfPoints());
          double* pc = cell->GetParametricCoords();

          outPtId = this->GetOutputPointId(ids[0], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
          if (this->NonlinearSubdivisionLevel == 1)
          {
            for (i = 2; i < numCellPts; i++)
            {
              outPtId = this->GetOutputPointIdAndInterpolate(
                i, input, cell, pc, weights.data(), newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
            }
          }
          else
          {
            double paramCoordDelta = 1. / (numCellPtsAfterSubdivision - 1);
            double inParamCoords[3];
            inParamCoords[1] = inParamCoords[2] = 0.;
            for (i = 0; i < (numCellPts - 1); i++)
            {
              for (j = 0; j < numDeltaPtsAfterSubdivision - 1; j++)
              {
                inParamCoords[0] = paramCoordDelta * (numDeltaPtsAfterSubdivision * i + j + 1);
                outPtId = GetInterpolatedPointId(
                  input, cell, inParamCoords, weights.data(), newPts, outputPD);
                newLines->InsertCellPoint(outPtId);
              }
              if (i < numCellPts - 2)
              {
                outPtId = this->GetOutputPointIdAndInterpolate(
                  i + 2, input, cell, pc, weights.data(), newPts, outputPD);
                newLines->InsertCellPoint(outPtId);
              }
            }
          }
          outPtId = this->GetOutputPointId(ids[1], input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }

        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
        break;
      }
      case VTK_HEXAHEDRON:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId);
        this->InsertQuadInHash(ids[0], ids[3], ids[2], ids[1], cellId);
        this->InsertQuadInHash(ids[0], ids[4], ids[7], ids[3], cellId);
        this->InsertQuadInHash(ids[1], ids[2], ids[6], ids[5], cellId);
        this->InsertQuadInHash(ids[2], ids[3], ids[7], ids[6], cellId);
        this->InsertQuadInHash(ids[4], ids[5], ids[6], ids[7], cellId);
        break;

      case VTK_VOXEL:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId);
        this->InsertQuadInHash(ids[0], ids[2], ids[3], ids[1], cellId);
        this->InsertQuadInHash(ids[0], ids[4], ids[6], ids[2], cellId);
        this->InsertQuadInHash(ids[1], ids[3], ids[7], ids[5], cellId);
        this->InsertQuadInHash(ids[2], ids[6], ids[7], ids[3], cellId);
        this->InsertQuadInHash(ids[4], ids[5], ids[7], ids[6], cellId);
        break;

      case VTK_TETRA:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertTriInHash(ids[0], ids[1], ids[3], cellId, 2);
        this->InsertTriInHash(ids[0], ids[2], ids[1], cellId, 3);
        this->InsertTriInHash(ids[0], ids[3], ids[2], cellId, 1);
        this->InsertTriInHash(ids[1], ids[2], ids[3], cellId, 0);
        break;

      case VTK_PENTAGONAL_PRISM:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertQuadInHash(ids[0], ids[1], ids[6], ids[5], cellId);
        this->InsertQuadInHash(ids[1], ids[2], ids[7], ids[6], cellId);
        this->InsertQuadInHash(ids[2], ids[3], ids[8], ids[7], cellId);
        this->InsertQuadInHash(ids[3], ids[4], ids[9], ids[8], cellId);
        this->InsertQuadInHash(ids[4], ids[0], ids[5], ids[9], cellId);
        this->InsertPolygonInHash(ids, 5, cellId);
        this->InsertPolygonInHash(&ids[5], 5, cellId);
        break;

      case VTK_HEXAGONAL_PRISM:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertQuadInHash(ids[0], ids[1], ids[7], ids[6], cellId);
        this->InsertQuadInHash(ids[1], ids[2], ids[8], ids[7], cellId);
        this->InsertQuadInHash(ids[2], ids[3], ids[9], ids[8], cellId);
        this->InsertQuadInHash(ids[3], ids[4], ids[10], ids[9], cellId);
        this->InsertQuadInHash(ids[4], ids[5], ids[11], ids[10], cellId);
        this->InsertQuadInHash(ids[5], ids[0], ids[6], ids[11], cellId);
        this->InsertPolygonInHash(ids, 6, cellId);
        this->InsertPolygonInHash(&ids[6], 6, cellId);
        break;

      case VTK_PYRAMID:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertQuadInHash(ids[3], ids[2], ids[1], ids[0], cellId);
        this->InsertTriInHash(ids[0], ids[1], ids[4], cellId);
        this->InsertTriInHash(ids[1], ids[2], ids[4], cellId);
        this->InsertTriInHash(ids[2], ids[3], ids[4], cellId);
        this->InsertTriInHash(ids[3], ids[0], ids[4], cellId);
        break;

      case VTK_WEDGE:
        input->GetCellPoints(cellId, numCellPts, ids, pointIdList);
        this->InsertQuadInHash(ids[0], ids[2], ids[5], ids[3], cellId);
        this->InsertQuadInHash(ids[1], ids[0], ids[3], ids[4], cellId);
        this->InsertQuadInHash(ids[2], ids[1], ids[4], ids[5], cellId);
        this->InsertTriInHash(ids[0], ids[1], ids[2], cellId);
        this->InsertTriInHash(ids[3], ids[5], ids[4], cellId);
        break;

      case VTK_PIXEL:
      case VTK_QUAD:
      case VTK_TRIANGLE:
      case VTK_POLYGON:
      case VTK_TRIANGLE_STRIP:
      case VTK_QUADRATIC_TRIANGLE:
      case VTK_BIQUADRATIC_TRIANGLE:
      case VTK_QUADRATIC_QUAD:
      case VTK_QUADRATIC_LINEAR_QUAD:
      case VTK_BIQUADRATIC_QUAD:
      case VTK_QUADRATIC_POLYGON:
      case VTK_LAGRANGE_TRIANGLE:
      case VTK_LAGRANGE_QUADRILATERAL:
      case VTK_BEZIER_TRIANGLE:
      case VTK_BEZIER_QUADRILATERAL:
        // save 2D cells for third pass
        flag2D = 1;
        break;

      default:
      {
        // Default way of getting faces. Differentiates between linear
        // and higher order cells.
        input->GetCell(cellId, cell);
        if (cell->IsLinear())
        {
          if (cell->GetCellDimension() == 3)
          {
            int numFaces = cell->GetNumberOfFaces();
            for (j = 0; j < numFaces; j++)
            {
              face = cell->GetFace(j);
              numFacePts = face->GetNumberOfPoints();
              if (numFacePts == 4)
              {
                this->InsertQuadInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                  face->PointIds->GetId(2), face->PointIds->GetId(3), cellId);
              }
              else if (numFacePts == 3)
              {
                this->InsertTriInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                  face->PointIds->GetId(2), cellId);
              }
              else
              {
                this->InsertPolygonInHash(
                  face->PointIds->GetPointer(0), face->PointIds->GetNumberOfIds(), cellId);
              }
            } // for all cell faces
          }   // if 3D
          else
          {
            vtkDebugMacro("Missing cell type.");
          }
        }    // a linear cell type
        else // process nonlinear cells via triangulation
        {
          input->SetCellOrderAndRationalWeights(cellId, cell);
          if (cell->GetCellDimension() == 1)
          {
            cell->TriangulateIds(0, pts);
            for (i = 0; i < pts->GetNumberOfIds(); i += 2)
            {
              newLines->InsertNextCell(2);
              inPtId = pts->GetId(i);
              this->RecordOrigCellId(this->NumberOfNewCells, cellId);
              outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
              outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
              inPtId = pts->GetId(i + 1);
              outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
            }
          }
          else if (cell->GetCellDimension() == 2)
          {
            vtkWarningMacro(<< "2-D nonlinear cells must be processed with all other 2-D cells.");
          }
          else // 3D nonlinear cell
          {
            vtkIdList* cellIds = vtkIdList::New();
            int numFaces = cell->GetNumberOfFaces();
            for (j = 0; j < numFaces; j++)
            {
              face = cell->GetFace(j);
              input->GetCellNeighbors(cellId, face->PointIds, cellIds);
              if (cellIds->GetNumberOfIds() <= 0)
              {
                // FIXME: Face could not be consistent. vtkOrderedTriangulator is a better option
                if (this->NonlinearSubdivisionLevel >= 1)
                {
                  // TODO: Handle NonlinearSubdivisionLevel > 1 correctly.
                  face->TriangulateIds(0, pts);
                  for (i = 0; i < pts->GetNumberOfIds(); i += 3)
                  {
                    this->InsertTriInHash(
                      pts->GetId(i), pts->GetId(i + 1), pts->GetId(i + 2), cellId);
                  }
                }
                else
                {
                  switch (face->GetCellType())
                  {
                    case VTK_QUADRATIC_TRIANGLE:
                    case VTK_LAGRANGE_TRIANGLE:
                    case VTK_BEZIER_TRIANGLE:
                      this->InsertTriInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                        face->PointIds->GetId(2), cellId);
                      break;
                    case VTK_QUADRATIC_QUAD:
                    case VTK_BIQUADRATIC_QUAD:
                    case VTK_QUADRATIC_LINEAR_QUAD:
                    case VTK_LAGRANGE_QUADRILATERAL:
                    case VTK_BEZIER_QUADRILATERAL:
                      this->InsertQuadInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                        face->PointIds->GetId(2), face->PointIds->GetId(3), cellId);
                      break;
                    default:
                      vtkWarningMacro(<< "Encountered unknown nonlinear face.");
                      break;
                  } // switch cell type
                }   // subdivision level
              }     // cell has ids
            }       // for faces
            cellIds->Delete();
          } // 3d cell
        }   // nonlinear cell
      }     // default switch case
    }       // switch(cellType)
  }         // for all cells.

  // It would be possible to add these (except for polygons with 5+ sides)
  // to the hashes.  Alternatively, the higher order 2d cells could be handled
  // in the following loop.

  // Now insert 2DCells.  Because of poly datas (cell data) ordering,
  // the 2D cells have to come after points and lines.
  for (vtkIdType cellId = 0; cellId < numCells && !abort && flag2D; ++cellId)
  {
    // We skip cells marked as hidden
    if (ghostCells &&
      (ghostCells->GetValue(cellId) & vtkDataSetAttributes::CellGhostTypes::HIDDENCELL))
    {
      continue;
    }

    cellType = input->GetCellType(cellId);
    input->GetCellPoints(cellId, numCellPts, ids, pointIdList);

    // If we have a quadratic face and our subdivision level is zero, just treat
    // it as a linear cell.  This should work so long as the first points of the
    // quadratic cell correspond to all those of the equivalent linear cell
    // (which all the current definitions do).
    if (this->NonlinearSubdivisionLevel < 1)
    {
      switch (cellType)
      {
        case VTK_QUADRATIC_TRIANGLE:
        case VTK_LAGRANGE_TRIANGLE:
        case VTK_BEZIER_TRIANGLE:
          cellType = VTK_TRIANGLE;
          numCellPts = 3;
          break;
        case VTK_QUADRATIC_QUAD:
        case VTK_BIQUADRATIC_QUAD:
        case VTK_QUADRATIC_LINEAR_QUAD:
        case VTK_LAGRANGE_QUADRILATERAL:
        case VTK_BEZIER_QUADRILATERAL:
          cellType = VTK_QUAD;
          numCellPts = 4;
          break;
      }
    }

    // A couple of common cases to see if things go faster.
    if (cellType == VTK_PIXEL)
    { // Do we really want to insert the 2D cells into a hash?
      pts->Reset();
      pts->InsertId(0, this->GetOutputPointId(ids[0], input, newPts, outputPD));
      pts->InsertId(1, this->GetOutputPointId(ids[1], input, newPts, outputPD));
      pts->InsertId(2, this->GetOutputPointId(ids[3], input, newPts, outputPD));
      pts->InsertId(3, this->GetOutputPointId(ids[2], input, newPts, outputPD));
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == VTK_POLYGON || cellType == VTK_TRIANGLE || cellType == VTK_QUAD)
    {
      pts->Reset();
      for (i = 0; i < numCellPts; i++)
      {
        outPtId = this->GetOutputPointId(ids[i], input, newPts, outputPD);
        pts->InsertId(i, outPtId);
      }
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == VTK_TRIANGLE_STRIP)
    {
      // Change strips to triangles so we do not have to worry about order.
      int toggle = 0;
      vtkIdType ptIds[3];
      // This check is not really necessary.  It was put here because of another (now fixed) bug.
      if (numCellPts > 1)
      {
        ptIds[0] = this->GetOutputPointId(ids[0], input, newPts, outputPD);
        ptIds[1] = this->GetOutputPointId(ids[1], input, newPts, outputPD);
        for (i = 2; i < numCellPts; ++i)
        {
          ptIds[2] = this->GetOutputPointId(ids[i], input, newPts, outputPD);
          newPolys->InsertNextCell(3, ptIds);
          this->RecordOrigCellId(this->NumberOfNewCells, cellId);
          outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
          ptIds[toggle] = ptIds[2];
          toggle = !toggle;
        }
      }
    }
    else if (cellType == VTK_QUADRATIC_TRIANGLE || cellType == VTK_BIQUADRATIC_TRIANGLE ||
      cellType == VTK_QUADRATIC_QUAD || cellType == VTK_BIQUADRATIC_QUAD ||
      cellType == VTK_QUADRATIC_LINEAR_QUAD || cellType == VTK_QUADRATIC_POLYGON ||
      cellType == VTK_LAGRANGE_TRIANGLE || cellType == VTK_LAGRANGE_QUADRILATERAL ||
      cellType == VTK_BEZIER_TRIANGLE || cellType == VTK_BEZIER_QUADRILATERAL)
    {
      // If one of the points is hidden (meaning invalid), do not
      // extract surface cell.
      // Removed checking for whether all points are ghost, because that's an
      // incorrect assumption.
      bool oneHidden = false;
      if (ghosts)
      {
        for (i = 0; i < numCellPts; i++)
        {
          unsigned char val = ghosts->GetValue(ids[i]);
          if (val & vtkDataSetAttributes::HIDDENPOINT)
          {
            oneHidden = true;
            break;
          }
        }
      }
      if (oneHidden)
      {
        continue;
      }

      // Note: we should not be here if this->NonlinearSubdivisionLevel is less
      // than 1.  See the check above.
      input->GetCell(cellId, cell);
      double* pc = cell->GetParametricCoords();

      // If the cell is of Bezier type, the weights might be rational and the degree nonuniform.
      // This need to be initiated.
      input->SetCellOrderAndRationalWeights(cellId, cell);

      // Get the triangulation of the first subdivision level.
      // Note that the output of TriangulateLocalIds records triangles in pts where each 3 points
      // defines a triangle. The returned ids are local ids with respect to the cell.
      cell->TriangulateLocalIds(0, pts);
      assert(pts->GetNumberOfIds() % 3 == 0);

      // Start to fill outPts with the cell points
      numFacePts = cell->GetNumberOfPoints();
      outPts->Reset();
      weights.resize(numFacePts);
      // For Bezier cells, the points that are not at the corners are overload to get the
      // projection of the non-interpolate points. numFacePtsToCopy is the number of points to be
      // copied, and numFacePts - numFacePtsToCopy will be the number of points that are
      // interpolated.
      vtkIdType numFacePtsToCopy = !AllowInterpolation ||
          (cellType != VTK_BEZIER_QUADRILATERAL && cellType != VTK_BEZIER_TRIANGLE)
        ? numFacePts
        : (cellType == VTK_BEZIER_QUADRILATERAL ? 4 : 3);
      // Points that are copied:
      for (i = 0; i < numFacePtsToCopy; i++)
      {
        outPts->InsertNextId(this->GetOutputPointId(cell->GetPointId(i), input, newPts, outputPD));
      }
      // Points that are interpolated (only for Bezier cells when AllowInterpolation is true )
      for (i = numFacePtsToCopy; i < numFacePts; i++)
      {
        outPts->InsertNextId(this->GetOutputPointIdAndInterpolate(
          i, input, cell, pc, weights.data(), newPts, outputPD));
      }

      bool isDegenerateCell = false;
      auto isDegeneratedSubTriangle = [&](vtkIdType ii) {
        return outPts->GetId(pts->GetId(ii)) == outPts->GetId(pts->GetId(ii + 1)) ||
          outPts->GetId(pts->GetId(ii)) == outPts->GetId(pts->GetId(ii + 2)) ||
          outPts->GetId(pts->GetId(ii + 1)) == outPts->GetId(pts->GetId(ii + 2));
      };

      // Do any further subdivision if necessary.
      if (this->NonlinearSubdivisionLevel > 1 && pc)
      {
        for (i = 0; i < pts->GetNumberOfIds(); i += 3)
        {
          if (isDegeneratedSubTriangle(i))
          {
            isDegenerateCell = true;
            break;
          }
        }

        vtkIdType maxNumberOfIds =
          std::pow(4, this->NonlinearSubdivisionLevel - 1) * pts->GetNumberOfIds();
        pts2->Allocate(maxNumberOfIds);
        // We are going to need parametric coordinates to further subdivide.
        parametricCoords.resize(maxNumberOfIds * 3);
        std::copy(&pc[0], &pc[0] + numFacePts * 3, parametricCoords.begin());

        // localEdgeMap is simular to this->EdgeMap, but only stores local ids
        localEdgeMap->clear();

        auto isEqualTo1Or0 = [](double a, double e = 1e-10) {
          return (std::abs(a) <= e) || (std::abs(a - 1) <= e);
        };

        vtkIdType localIdCpt = numFacePts;
        vtkIdType pt1, pt2, id;
        vtkIdType inPts[6];
        // Subdivide these triangles as many more times as necessary.  Remember
        // that we have already done the first subdivision.
        for (j = 1; j < this->NonlinearSubdivisionLevel; j++)
        {
          pts2->Reset();
          if (isDegenerateCell)
          {
            // For degenerate cells, we can have multiple parametric points linked to the same
            // output point. But we need to select a single one. The rule is to give priority to
            // the points that are on the contour of the parametric space. This is necessary for
            // connecting adjacent cells. The way we give this priority is by calling
            // this->EdgeMap->FindEgde/AddEdge for those points first. So a first iteration over pts
            // is performed to add those points. During the second iteration (the one not specific
            // to degenerate cells), when trying to add a duplicate point, the edge map will return
            // the output id of the already existing point.
            double coords[3];
            for (i = 0; i < pts->GetNumberOfIds(); i += 3)
            {
              for (k = 0; k < 3; k++)
              {
                pt1 = pts->GetId(i + k);
                pt2 = pts->GetId(i + ((k < 2) ? (k + 1) : 0));
                {
                  coords[0] = 0.5 * (parametricCoords[pt1 * 3] + parametricCoords[pt2 * 3]);
                  coords[1] = 0.5 * (parametricCoords[pt1 * 3 + 1] + parametricCoords[pt2 * 3 + 1]);
                  coords[2] = 0.5 * (parametricCoords[pt1 * 3 + 2] + parametricCoords[pt2 * 3 + 2]);
                  if (isEqualTo1Or0(coords[0]) || isEqualTo1Or0(coords[1]))
                  {
                    this->GetInterpolatedPointId(outPts->GetId(pt1), outPts->GetId(pt2), input,
                      cell, coords, weights.data(), newPts, outputPD);
                  }
                }
              }
            }
          }

          // Each triangle will be split into 4 triangles.
          for (i = 0; i < pts->GetNumberOfIds(); i += 3)
          {
            // Hold the input point ids and parametric coordinates.  First 3
            // indices are the original points.  Second three are the midpoints
            // in the edges (0,1), (1,2) and (2,0), respectively (see comment
            // below).
            for (k = 0; k < 3; k++)
            {
              inPts[k] = pts->GetId(i + k);
              pt1 = inPts[k];
              pt2 = pts->GetId(i + ((k < 2) ? (k + 1) : 0));
              id = localEdgeMap->FindEdge(pt1, pt2);
              if (id == -1)
              {
                id = localIdCpt;
                parametricCoords[id * 3] =
                  0.5 * (parametricCoords[pt1 * 3] + parametricCoords[pt2 * 3]);
                parametricCoords[id * 3 + 1] =
                  0.5 * (parametricCoords[pt1 * 3 + 1] + parametricCoords[pt2 * 3 + 1]);
                parametricCoords[id * 3 + 2] =
                  0.5 * (parametricCoords[pt1 * 3 + 2] + parametricCoords[pt2 * 3 + 2]);

                localEdgeMap->AddEdge(pt1, pt2, id);
                outPts->InsertNextId(
                  this->GetInterpolatedPointId(outPts->GetId(pt1), outPts->GetId(pt2), input, cell,
                    &parametricCoords[id * 3], weights.data(), newPts, outputPD));
                localIdCpt++;
              }
              inPts[k + 3] = id;
            }
            //       * 0
            //      / \        Use the 6 points recorded
            //     /   \       in inPts and paramCoords
            //  3 *-----* 5    to create the 4 triangles
            //   / \   / \     shown here.
            //  /   \ /   \    .
            // *-----*-----*
            // 1     4     2
            static const int subtriangles[12] = { 0, 3, 5, 3, 1, 4, 3, 4, 5, 5, 4, 2 };
            for (int subId : subtriangles)
            {
              pts2->InsertNextId(inPts[subId]);
            }
          } // Iterate over triangles
          // Now that we have recorded the subdivided triangles in pts2 , swap them with pts to
          // make them the current ones.
          std::swap(pts, pts2);
        } // Iterate over subdivision levels
      }
      for (i = 0; i < pts->GetNumberOfIds(); i += 3)
      {
        if (isDegenerateCell && isDegeneratedSubTriangle(i))
        {
          continue; // Do not record the degenerate triangle
        }
        newPolys->InsertNextCell(3);
        newPolys->InsertCellPoint(outPts->GetId(pts->GetId(i)));
        newPolys->InsertCellPoint(outPts->GetId(pts->GetId(i + 1)));
        newPolys->InsertCellPoint(outPts->GetId(pts->GetId(i + 2)));
        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    }

  } // for all cells.

  // Now transfer geometry from hash to output (only triangles and quads).
  this->InitQuadHashTraversal();
  while ((q = this->GetNextVisibleQuadFromHash()))
  {
    // If one of the points is hidden (meaning invalid), do not
    // extract surface cell.
    // Removed checking for whether all points are ghost, because that's an
    // incorrect assumption.
    bool oneHidden = false;
    // handle all polys
    for (i = 0; i < q->numPts; i++)
    {
      if (ghosts)
      {
        unsigned char val = ghosts->GetValue(q->ptArray[i]);
        if (val & vtkDataSetAttributes::HIDDENPOINT)
        {
          oneHidden = true;
        }
      }

      q->ptArray[i] = this->GetOutputPointId(q->ptArray[i], input, newPts, outputPD);
    }

    if (oneHidden)
    {
      continue;
    }
    newPolys->InsertNextCell(q->numPts, q->ptArray);
    this->RecordOrigCellId(this->NumberOfNewCells, q);
    outputCD->CopyData(inputCD, q->SourceId, this->NumberOfNewCells++);
  }

  if (this->PassThroughCellIds)
  {
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    outputPD->AddArray(this->OriginalPointIds);
  }

  // Update ourselves and release memory
  //
  cell->Delete();
  pts->Delete();
  outPts->Delete();
  pts2->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();
  if (newVerts->GetNumberOfCells() > 0)
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();
  newVerts = nullptr;
  if (newLines->GetNumberOfCells() > 0)
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  // free storage
  output->Squeeze();
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  this->DeleteQuadHash();

  return 1;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::InitializeQuadHash(vtkIdType numPoints)
{
  vtkIdType i;

  if (this->QuadHash)
  {
    this->DeleteQuadHash();
  }

  // Prepare our special quad allocator (for efficiency).
  this->InitFastGeomQuadAllocation(numPoints);

  this->QuadHash = new vtkFastGeomQuad*[numPoints];
  this->QuadHashLength = numPoints;
  this->PointMap = new vtkIdType[numPoints];
  for (i = 0; i < numPoints; ++i)
  {
    this->QuadHash[i] = nullptr;
    this->PointMap[i] = -1;
  }
  this->EdgeMap = new vtkEdgeInterpolationMap;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::DeleteQuadHash()
{
  vtkIdType i;

  this->DeleteAllFastGeomQuads();

  for (i = 0; i < this->QuadHashLength; ++i)
  {
    this->QuadHash[i] = nullptr;
  }

  delete[] this->QuadHash;
  this->QuadHash = nullptr;
  this->QuadHashLength = 0;
  delete[] this->PointMap;
  this->PointMap = nullptr;
  delete this->EdgeMap;
  this->EdgeMap = nullptr;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::InsertQuadInHash(
  vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d, vtkIdType sourceId)
{
  vtkIdType tmp;
  vtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c && b < d)
  {
    tmp = a;
    a = b;
    b = c;
    c = d;
    d = tmp;
  }
  else if (c < a && c < b && c < d)
  {
    tmp = a;
    a = c;
    c = tmp;
    tmp = b;
    b = d;
    d = tmp;
  }
  else if (d < a && d < b && d < c)
  {
    tmp = a;
    a = d;
    d = c;
    c = b;
    b = tmp;
  }

  // Look for existing quad in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
  {
    end = &(quad->Next);
    // a has to match in this bin.
    // c should be independent of point order.
    if (quad->numPts == 4 && c == quad->ptArray[2])
    {
      // Check both orders for b and d.
      if ((b == quad->ptArray[1] && d == quad->ptArray[3]) ||
        (b == quad->ptArray[3] && d == quad->ptArray[1]))
      {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do.  Hide any quad shared by two or more cells.
        return;
      }
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(4);
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  quad->ptArray[3] = d;
  *end = quad;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::InsertTriInHash(
  vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType sourceId, vtkIdType vtkNotUsed(faceId) /*= -1*/)
{
  vtkIdType tmp;
  vtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c)
  {
    tmp = a;
    a = b;
    b = c;
    c = tmp;
  }
  else if (c < a && c < b)
  {
    tmp = a;
    a = c;
    c = b;
    b = tmp;
  }
  // We can't put the second smallest in b because it might change the order
  // of the vertices in the final triangle.

  // Look for existing tri in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
  {
    end = &(quad->Next);
    // a has to match in this bin.
    if (quad->numPts == 3)
    {
      if ((b == quad->ptArray[1] && c == quad->ptArray[2]) ||
        (b == quad->ptArray[2] && c == quad->ptArray[1]))
      {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do. Hide any tri shared by two or more cells.
        return;
      }
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(3);
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  *end = quad;
}

// Insert a polygon into the hash.
// Input: an array of vertex ids
//        the start index of the polygon in the array
//        the end index of the polygon in the array
//        the cellId of the polygon
//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::InsertPolygonInHash(
  const vtkIdType* ids, int numPts, vtkIdType sourceId)
{
  // sanity check
  if (numPts == 0)
  {
    return;
  }
  vtkFastGeomQuad *quad, **end;

  // find the index to the smallest id
  vtkIdType offset = 0;
  for (int i = 0; i < numPts; i++)
  {
    if (ids[i] < ids[offset])
    {
      offset = i;
    }
  }

  // copy ids into ordered array with smallest id first
  vtkIdType* tab = new vtkIdType[numPts];
  for (int i = 0; i < numPts; i++)
  {
    tab[i] = ids[(offset + i) % numPts];
  }

  // Look for existing hex in the hash;
  end = this->QuadHash + tab[0];
  quad = *end;
  while (quad)
  {
    end = &(quad->Next);
    // a has to match in this bin.
    // first just check the polygon size.
    bool match = true;
    if (numPts == quad->numPts)
    {
      if (tab[0] == quad->ptArray[0])
      {
        // if the first two points match loop through forwards
        // checking all points
        if (numPts > 1 && tab[1] == quad->ptArray[1])
        {
          for (int i = 2; i < numPts; ++i)
          {
            if (tab[i] != quad->ptArray[i])
            {
              match = false;
              break;
            }
          }
        }
        else
        {
          // check if the points go in the opposite direction
          for (int i = 1; i < numPts; ++i)
          {
            if (tab[numPts - i] != quad->ptArray[i])
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
    }
    else
    {
      match = false;
    }

    if (match)
    {
      // We have a match.
      quad->SourceId = -1;
      // That is all we need to do. Hide any tri shared by two or more cells.
      delete[] tab;
      return;
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(numPts);
  // mark the structure as a polygon
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  for (int i = 0; i < numPts; i++)
  {
    quad->ptArray[i] = tab[i];
  }
  *end = quad;

  delete[] tab;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::InitFastGeomQuadAllocation(vtkIdType numberOfCells)
{
  int idx;

  this->DeleteAllFastGeomQuads();
  // Allocate 100 pointers to arrays.
  // This should be plenty (unless we have triangle strips) ...
  this->NumberOfFastGeomQuadArrays = 100;
  this->FastGeomQuadArrays = new unsigned char*[this->NumberOfFastGeomQuadArrays];
  // Initialize all to nullptr;
  for (idx = 0; idx < this->NumberOfFastGeomQuadArrays; ++idx)
  {
    this->FastGeomQuadArrays[idx] = nullptr;
  }
  // Set pointer to the beginning.
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;

  // size the chunks based on the size of a quadrilateral
  int quadSize = sizeofFastQuad(4);

  // Lets keep the chunk size relatively small.
  if (numberOfCells < 100)
  {
    this->FastGeomQuadArrayLength = 50 * quadSize;
  }
  else
  {
    this->FastGeomQuadArrayLength = (numberOfCells / 2) * quadSize;
  }
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::DeleteAllFastGeomQuads()
{
  for (int idx = 0; idx < this->NumberOfFastGeomQuadArrays; ++idx)
  {
    delete[] this->FastGeomQuadArrays[idx];
    this->FastGeomQuadArrays[idx] = nullptr;
  }
  delete[] this->FastGeomQuadArrays;
  this->FastGeomQuadArrays = nullptr;
  this->FastGeomQuadArrayLength = 0;
  this->NumberOfFastGeomQuadArrays = 0;
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;
}

//------------------------------------------------------------------------------
vtkFastGeomQuad* vtkDataSetSurfaceFilter::NewFastGeomQuad(int numPts)
{
  if (this->FastGeomQuadArrayLength == 0)
  {
    vtkErrorMacro("Face hash allocation has not been initialized.");
    return nullptr;
  }

  // see if there's room for this one
  int polySize = sizeofFastQuad(numPts);
  if (this->NextQuadIndex + polySize > this->FastGeomQuadArrayLength)
  {
    ++(this->NextArrayIndex);
    this->NextQuadIndex = 0;
  }

  // Although this should not happen often, check first.
  if (this->NextArrayIndex >= this->NumberOfFastGeomQuadArrays)
  {
    int idx, num;
    unsigned char** newArrays;
    num = this->NumberOfFastGeomQuadArrays * 2;
    newArrays = new unsigned char*[num];
    for (idx = 0; idx < num; ++idx)
    {
      newArrays[idx] = nullptr;
      if (idx < this->NumberOfFastGeomQuadArrays)
      {
        newArrays[idx] = this->FastGeomQuadArrays[idx];
      }
    }
    delete[] this->FastGeomQuadArrays;
    this->FastGeomQuadArrays = newArrays;
    this->NumberOfFastGeomQuadArrays = num;
  }

  // Next: allocate a new array if necessary.
  if (this->FastGeomQuadArrays[this->NextArrayIndex] == nullptr)
  {
    this->FastGeomQuadArrays[this->NextArrayIndex] =
      new unsigned char[this->FastGeomQuadArrayLength];
  }

  vtkFastGeomQuad* q = reinterpret_cast<vtkFastGeomQuad*>(
    this->FastGeomQuadArrays[this->NextArrayIndex] + this->NextQuadIndex);
  q->numPts = numPts;
  q->ptArray = (vtkIdType*)q + FSizeDivSizeId;

  this->NextQuadIndex += polySize;

  return q;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::InitQuadHashTraversal()
{
  this->QuadHashTraversalIndex = 0;
  if (this->QuadHashLength == 0)
  {
    this->QuadHashTraversal = nullptr;
  }
  else
  {
    this->QuadHashTraversal = this->QuadHash[0];
  }
}

//------------------------------------------------------------------------------
vtkFastGeomQuad* vtkDataSetSurfaceFilter::GetNextVisibleQuadFromHash()
{
  vtkFastGeomQuad* quad;

  quad = this->QuadHashTraversal;

  // Move till traversal until we have a quad to return.
  // Note: the current traversal has not been returned yet.
  while (quad == nullptr || quad->SourceId == -1)
  {
    if (quad)
    { // The quad must be hidden.  Move to the next.
      quad = quad->Next;
    }
    else
    { // must be the end of the linked list.  Move to the next bin.
      this->QuadHashTraversalIndex += 1;
      if (this->QuadHashTraversalIndex >= this->QuadHashLength)
      { // There are no more bins.
        this->QuadHashTraversal = nullptr;
        return nullptr;
      }
      quad = this->QuadHash[this->QuadHashTraversalIndex];
    }
  }

  // Now we have a quad to return.  Set the traversal to the next entry.
  this->QuadHashTraversal = quad->Next;

  return quad;
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSetSurfaceFilter::GetOutputPointId(
  vtkIdType inPtId, vtkDataSet* input, vtkPoints* outPts, vtkPointData* outPD)
{
  vtkIdType outPtId;

  outPtId = this->PointMap[inPtId];
  if (outPtId == -1)
  {
    outPtId = outPts->InsertNextPoint(input->GetPoint(inPtId));
    outPD->CopyData(input->GetPointData(), inPtId, outPtId);
    this->PointMap[inPtId] = outPtId;
    this->RecordOrigPointId(outPtId, inPtId);
  }

  return outPtId;
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSetSurfaceFilter::GetOutputPointIdAndInterpolate(vtkIdType cellPtId,
  vtkDataSet* input, vtkCell* cell, double* weights, vtkPoints* outPts, vtkPointData* outPD)
{
  double* pc = cell->GetParametricCoords();
  return this->GetOutputPointIdAndInterpolate(cellPtId, input, cell, pc, weights, outPts, outPD);
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSetSurfaceFilter::GetOutputPointIdAndInterpolate(vtkIdType cellPtId,
  vtkDataSet* input, vtkCell* cell, double* pc, double* weights, vtkPoints* outPts,
  vtkPointData* outPD)
{
  vtkIdType outPtId;
  vtkIdType inPtId = cell->GetPointId(cellPtId);
  outPtId = this->PointMap[inPtId];
  if (outPtId == -1)
  {
    int subId = -1;
    double wcoords[3];
    cell->EvaluateLocation(subId, pc + 3 * cellPtId, wcoords, weights);
    outPtId = outPts->InsertNextPoint(wcoords);
    outPD->InterpolatePoint(input->GetPointData(), outPtId, cell->GetPointIds(), weights);
    this->PointMap[inPtId] = outPtId;
    this->RecordOrigPointId(outPtId, inPtId);
  }
  return outPtId;
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSetSurfaceFilter::GetInterpolatedPointId(vtkIdType edgePtA, vtkIdType edgePtB,
  vtkDataSet* input, vtkCell* cell, double* pcoords, double* weights, vtkPoints* outPts,
  vtkPointData* outPD)
{
  vtkIdType outPtId = this->EdgeMap->FindEdge(edgePtA, edgePtB);
  if (outPtId == -1)
  {
    int subId = -1;
    double wcoords[3];
    cell->EvaluateLocation(subId, pcoords, wcoords, weights);
    outPtId = outPts->InsertNextPoint(wcoords);
    outPD->InterpolatePoint(input->GetPointData(), outPtId, cell->GetPointIds(), weights);
    this->RecordOrigPointId(outPtId, -1);
    this->EdgeMap->AddEdge(edgePtA, edgePtB, outPtId);
  }
  return outPtId;
}

vtkIdType vtkDataSetSurfaceFilter::GetInterpolatedPointId(vtkDataSet* input, vtkCell* cell,
  double pcoords[3], double* weights, vtkPoints* outPts, vtkPointData* outPD)
{
  int subId = -1;
  double wcoords[3];
  cell->EvaluateLocation(subId, pcoords, wcoords, weights);
  vtkIdType outPtId = outPts->InsertNextPoint(wcoords);
  outPD->InterpolatePoint(input->GetPointData(), outPtId, cell->GetPointIds(), weights);
  this->RecordOrigPointId(outPtId, -1);
  return outPtId;
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::RecordOrigCellId(vtkIdType destIndex, vtkIdType originalId)
{
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->InsertValue(destIndex, originalId);
  }
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::RecordOrigCellId(vtkIdType destIndex, vtkFastGeomQuad* quad)
{
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->InsertValue(destIndex, quad->SourceId);
  }
}

//------------------------------------------------------------------------------
void vtkDataSetSurfaceFilter::RecordOrigPointId(vtkIdType destIndex, vtkIdType originalId)
{
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->InsertValue(destIndex, originalId);
  }
}
VTK_ABI_NAMESPACE_END
