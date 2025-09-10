// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_6_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkUnstructuredGrid.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkCellTypeUtilities.h"
#include "vtkCellTypes.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyhedron.h"
#include "vtkStaticCellLinks.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGridCellIterator.h"

#include <algorithm>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnstructuredGrid);
vtkStandardExtendedNewMacro(vtkUnstructuredGrid);

namespace
{
constexpr unsigned char MASKED_CELL_VALUE = vtkDataSetAttributes::HIDDENCELL |
  vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::REFINEDCELL;

//==============================================================================
struct RemoveGhostCellsWorker
{
  vtkNew<vtkIdList> NewPointIdMap;
  vtkNew<vtkIdList> NewCellIdMap;

  template <class ArrayT1, class ArrayT2, class ArrayT3>
  void operator()(ArrayT1* inputOffsets, ArrayT2* inputFacesOffsets,
    ArrayT3* inputFaceLocationsOffsets, vtkDataArray* inputConnectivityDA,
    vtkDataArray* inputFacesDA, vtkDataArray* inputFaceLocationsDA, vtkUnsignedCharArray* types,
    vtkUnsignedCharArray* ghostCells, vtkIdType numPoints, vtkDataArray* outputOffsetsDA,
    vtkDataArray* outputConnectivityDA, vtkDataArray* outputFacesOffsetsDA,
    vtkDataArray* outputFacesDA, vtkDataArray* outputFaceLocationsOffsetsDA,
    vtkDataArray* outputFaceLocationsDA)
  {
    if (!inputOffsets->GetNumberOfValues())
    {
      return;
    }

    auto outputOffsets = vtkArrayDownCast<ArrayT1>(outputOffsetsDA);
    auto inputConnectivity = vtkArrayDownCast<ArrayT1>(inputConnectivityDA);
    auto outputConnectivity = vtkArrayDownCast<ArrayT1>(outputConnectivityDA);

    outputOffsets->SetNumberOfValues(inputOffsets->GetNumberOfValues());
    outputConnectivity->SetNumberOfValues(inputConnectivity->GetNumberOfValues());

    auto inputOffsetsRange = vtk::DataArrayValueRange<1>(inputOffsets);
    auto inputConnectivityRange = vtk::DataArrayValueRange<1>(inputConnectivity);
    using ValueType = typename decltype(inputOffsetsRange)::ValueType;

    auto outputOffsetsRange = vtk::DataArrayValueRange<1>(outputOffsets);
    auto outputConnectivityRange = vtk::DataArrayValueRange<1>(outputConnectivity);

    auto typesRange = vtk::DataArrayValueRange<1>(types);
    auto ghostCellsRange = vtk::DataArrayValueRange<1>(ghostCells);

    //
    auto inputFaces = vtkArrayDownCast<ArrayT2>(inputFacesDA);
    auto inputFaceLocations = vtkArrayDownCast<ArrayT3>(inputFaceLocationsDA);

    auto outputFaces = vtkArrayDownCast<ArrayT2>(outputFacesDA);
    auto outputFacesOffsets = vtkArrayDownCast<ArrayT2>(outputFacesOffsetsDA);
    auto outputFaceLocations = vtkArrayDownCast<ArrayT3>(outputFaceLocationsDA);
    auto outputFaceLocationsOffsets = vtkArrayDownCast<ArrayT3>(outputFaceLocationsOffsetsDA);

    using RangeFaces = typename vtk::detail::SelectValueRange<ArrayT2, 1>::type;
    using RangeFacesLocations = typename vtk::detail::SelectValueRange<ArrayT3, 1>::type;

    RangeFaces inputFacesOffsetsRange;
    RangeFaces inputFacesRange;
    RangeFacesLocations inputFaceLocsOffsetRange;
    RangeFacesLocations inputFaceLocsRange;

    RangeFaces outputFacesOffsetRange;
    RangeFaces outputFacesRange;
    RangeFacesLocations outputFaceLocsOffsetRange;
    RangeFacesLocations outputFaceLocsRange;

    if (inputFacesOffsets && inputFaces)
    {
      outputFacesOffsets->SetNumberOfValues(inputFacesOffsets->GetNumberOfValues());
      outputFaces->SetNumberOfValues(inputFaces->GetNumberOfValues());
      outputFacesOffsets->Fill(0);

      outputFaceLocationsOffsets->SetNumberOfValues(inputFaceLocationsOffsets->GetNumberOfValues());
      outputFaceLocationsOffsets->Fill(-1);
      outputFaceLocations->SetNumberOfValues(inputFaceLocations->GetNumberOfValues());

      inputFacesOffsetsRange = vtk::DataArrayValueRange<1>(inputFacesOffsets);
      inputFacesRange = vtk::DataArrayValueRange<1>(inputFaces);

      inputFaceLocsOffsetRange = vtk::DataArrayValueRange<1>(inputFaceLocationsOffsets);
      inputFaceLocsRange = vtk::DataArrayValueRange<1>(inputFaceLocations);

      outputFacesOffsetRange = vtk::DataArrayValueRange<1>(outputFacesOffsets);
      outputFacesRange = vtk::DataArrayValueRange<1>(outputFaces);

      outputFaceLocsOffsetRange = vtk::DataArrayValueRange<1>(outputFaceLocationsOffsets);
      outputFaceLocsRange = vtk::DataArrayValueRange<1>(outputFaceLocations);
    }

    std::vector<vtkIdType> pointIdRedirectionMap(numPoints, -1);

    this->NewPointIdMap->Allocate(numPoints);
    this->NewCellIdMap->Allocate(types->GetNumberOfValues());

    vtkIdType newPointsMaxId = -1;
    ValueType startId = inputOffsetsRange[0];
    vtkIdType newCellsMaxId = -1;
    ValueType currentOutputOffset = 0;
    ValueType currentOutFacesOffset = 0;
    ValueType currentOutFaceLocsOffset = 0;

    for (vtkIdType cellId = 0; cellId < inputOffsets->GetNumberOfValues() - 1; ++cellId)
    {
      if (ghostCellsRange[cellId] & MASKED_CELL_VALUE)
      {
        startId = inputOffsetsRange[cellId + 1];
        continue;
      }

      this->NewCellIdMap->InsertNextId(cellId);

      ValueType endId = inputOffsetsRange[cellId + 1];
      ValueType size = endId - startId;

      outputOffsetsRange[++newCellsMaxId] = currentOutputOffset;
      outputOffsetsRange[newCellsMaxId + 1] = currentOutputOffset + size;

      for (ValueType cellPointId = 0; cellPointId < size; ++cellPointId)
      {
        vtkIdType pointId = inputConnectivityRange[startId + cellPointId];
        if (pointIdRedirectionMap[pointId] == -1)
        {
          pointIdRedirectionMap[pointId] = ++newPointsMaxId;
          this->NewPointIdMap->InsertNextId(pointId);
        }
        outputConnectivityRange[currentOutputOffset + cellPointId] = pointIdRedirectionMap[pointId];
      }

      if (typesRange[cellId] == VTK_POLYHEDRON)
      {
        ValueType startFaceId = inputFaceLocsOffsetRange[cellId];
        ValueType endFaceId = inputFaceLocsOffsetRange[cellId + 1];
        ValueType numberOfFaces = endFaceId - startFaceId;

        outputFaceLocsOffsetRange[newCellsMaxId] = currentOutFaceLocsOffset;
        outputFaceLocsOffsetRange[newCellsMaxId + 1] = currentOutFaceLocsOffset + numberOfFaces;

        for (ValueType faceLoc = 0; faceLoc < numberOfFaces; ++faceLoc)
        {
          ValueType faceId = inputFaceLocsRange[startFaceId + faceLoc];
          ValueType startFace = inputFacesOffsetsRange[faceId];
          ValueType endFace = inputFacesOffsetsRange[faceId + 1];
          ValueType faceSize = endFace - startFace;

          outputFaceLocsRange[currentOutFaceLocsOffset + faceLoc] =
            currentOutFaceLocsOffset + faceLoc;
          outputFacesOffsetRange[currentOutFaceLocsOffset + faceLoc] = currentOutFacesOffset;
          outputFacesOffsetRange[currentOutFaceLocsOffset + faceLoc + 1] =
            currentOutFacesOffset + faceSize;

          for (ValueType pointLoc = 0; pointLoc < faceSize; ++pointLoc)
          {
            vtkIdType pointId = inputFacesRange[startFace + pointLoc];
            outputFacesRange[currentOutFacesOffset + pointLoc] = pointIdRedirectionMap[pointId];
          }
          currentOutFacesOffset += faceSize;
        }
        currentOutFaceLocsOffset += numberOfFaces;
      }

      currentOutputOffset += size;
      startId = endId;
    }

    if (currentOutFacesOffset > 0)
    {
      // Fix cells not polyhedron in the face locations offset
      outputFaceLocsOffsetRange[0] = 0;
      for (vtkIdType loc = 1; loc < newCellsMaxId + 2; ++loc)
      {
        if (outputFaceLocsOffsetRange[loc] == -1)
        {
          outputFaceLocsOffsetRange[loc] = outputFaceLocsOffsetRange[loc - 1];
        }
      }
      outputFaceLocationsOffsets->Resize(newCellsMaxId + 2);
      outputFaceLocations->Resize(currentOutFaceLocsOffset);
      outputFacesOffsets->Resize(currentOutFaceLocsOffset + 1);
      outputFaces->Resize(currentOutFacesOffset);
    }
    outputOffsets->Resize(newCellsMaxId + 2);
    outputConnectivity->Resize(currentOutputOffset + 1);
  }
};
} // anonymous namespace

// VTK_DEPRECATED_IN_9_6_0()
//------------------------------------------------------------------------------
vtkIdTypeArray* vtkUnstructuredGrid::GetCellLocationsArray()
{
  if (!this->CellLocations)
  {
    this->CellLocations = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  this->CellLocations->DeepCopy(this->Connectivity->GetOffsetsArray());
  this->CellLocations->SetNumberOfValues(this->GetNumberOfCells());

  return this->CellLocations;
}

// VTK_DEPRECATED_IN_9_6_0()
//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(
  vtkUnsignedCharArray* cellTypes, vtkIdTypeArray*, vtkCellArray* cells)
{
  this->SetCells(cellTypes, cells);
}

// VTK_DEPRECATED_IN_9_6_0()
//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray* cellTypes, vtkIdTypeArray*,
  vtkCellArray* cells, vtkIdTypeArray* faceLocations, vtkIdTypeArray* faces)
{
  this->SetCells(cellTypes, cells, faceLocations, faces);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid::vtkUnstructuredGrid()
{
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);

  this->DistinctCellTypesUpdateMTime = 0;
  this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
  this->Types = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Connectivity = vtkSmartPointer<vtkCellArray>::New();
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid::~vtkUnstructuredGrid() = default;

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetPiece()
{
  return this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetNumberOfPieces()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetGhostLevel()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
}

//------------------------------------------------------------------------------
// Copy the geometric and topological structure of an input unstructured grid.
void vtkUnstructuredGrid::CopyStructure(vtkDataSet* ds)
{
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(ds);
  if (!ug)
  {
    vtkErrorMacro("Input dataset is not a " << this->GetClassName());
    return;
  }
  this->Superclass::CopyStructure(ug);
  // If ds is a vtkUnstructuredGrid, do a shallow copy of the cell data.
  this->Connectivity = ug->Connectivity;
  this->Types = ug->Types;
  this->DistinctCellTypes = nullptr;
  this->DistinctCellTypesUpdateMTime = 0;
  this->Faces = ug->Faces;
  this->FaceLocations = ug->FaceLocations;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::Cleanup()
{
  this->Connectivity = nullptr;
  this->Links = nullptr;
  this->Types = nullptr;
  this->DistinctCellTypes = nullptr;
  this->DistinctCellTypesUpdateMTime = 0;
  this->Faces = nullptr;
  this->FaceLocations = nullptr;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::Initialize()
{
  vtkPointSet::Initialize();

  this->Cleanup();

  if (this->Information)
  {
    this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 0);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
  }
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetCellType(vtkIdType cellId)
{
  vtkDebugMacro(<< "Returning cell type " << static_cast<int>(this->Types->GetValue(cellId)));
  return static_cast<int>(this->Types->GetValue(cellId));
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGrid::GetCellSize(vtkIdType cellId)
{
  return this->Connectivity ? this->Connectivity->GetCellSize(cellId) : 0;
}

//------------------------------------------------------------------------------
vtkCell* vtkUnstructuredGrid::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->GenericCell);
  return this->GenericCell->GetRepresentativeCell();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  const int cellType = static_cast<int>(this->Types->GetValue(cellId));
  cell->SetCellType(cellType);

  this->Connectivity->GetCellAtId(cellId, cell->PointIds);
  this->Points->GetPoints(cell->PointIds, cell->Points);

  // Explicit face representation
  if (cell->RequiresExplicitFaceRepresentation())
  {
    this->GetPolyhedronFaces(cellId, cell->GetCellFaces());
  }

  // Some cells require special initialization to build data structures and such.
  if (cell->RequiresInitialization())
  {
    cell->Initialize();
  }
  this->SetCellOrderAndRationalWeights(cellId, cell);
}

//------------------------------------------------------------------------------
// Support GetCellBounds()
namespace
{ // anonymous
struct ComputeCellBoundsVisitor : public vtkCellArray::DispatchUtilities
{
  // vtkCellArray::Visit entry point:
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkPoints* points, vtkIdType cellId,
    double bounds[6]) const
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    const vtkIdType numPts = static_cast<vtkIdType>(endOffset - beginOffset);

    const auto pointIds = GetRange(conn).begin() + beginOffset;
    vtkBoundingBox::ComputeBounds(points, pointIds, numPts, bounds);
  }
};
} // anonymous

//------------------------------------------------------------------------------
// Faster implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkUnstructuredGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  this->Connectivity->Dispatch(ComputeCellBoundsVisitor{}, this->Points, cellId, bounds);
}

//------------------------------------------------------------------------------
// Return the number of points from the cell defined by the maximum number of
// points/
int vtkUnstructuredGrid::GetMaxCellSize()
{
  if (this->Connectivity)
  { // The internal implementation is threaded.
    return this->Connectivity->GetMaxCellSize();
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetMaxSpatialDimension()
{
  if (auto cellTypes = this->GetDistinctCellTypesArray())
  {
    int maxDim = 0;
    for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
    {
      maxDim = std::max(maxDim, vtkCellTypeUtilities::GetDimension(cellTypes->GetValue(i)));
    }
    return maxDim;
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetMinSpatialDimension()
{
  if (auto cellTypes = this->GetDistinctCellTypesArray())
  {
    int minDim = 3;
    for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
    {
      minDim = std::min(minDim, vtkCellTypeUtilities::GetDimension(cellTypes->GetValue(i)));
    }
    return minDim;
  }
  return 3;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGrid::GetNumberOfCells()
{
  vtkDebugMacro(<< "NUMBER OF CELLS = "
                << (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0));
  return (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0);
}

//------------------------------------------------------------------------------
// Insert/create cell in object by type and list of point ids defining
// cell topology. Using a special input format, this function also support
// polyhedron cells.
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(int type, vtkIdList* ptIds)
{
  if (type == VTK_POLYHEDRON)
  {
    // For polyhedron cell, input ptIds is of format:
    // (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
    vtkIdType* dataPtr = ptIds->GetPointer(0);
    return this->InsertNextCell(type, dataPtr[0], dataPtr + 1);
  }

  this->Connectivity->InsertNextCell(ptIds);

  // If faces have been created, we need to pad them (we are not creating
  // a polyhedral cell in this method)
  if (this->FaceLocations)
  {
    this->FaceLocations->InsertNextCell(0);
  }

  // insert cell type
  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//------------------------------------------------------------------------------
// Insert/create cell in object by type and list of point ids defining
// cell topology. Using a special input format, this function also support
// polyhedron cells.
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType ptIds[])
{
  if (type != VTK_POLYHEDRON)
  {
    // insert connectivity
    this->Connectivity->InsertNextCell(npts, ptIds);

    // If faces have been created, we need to pad them (we are not creating
    // a polyhedral cell in this method)
    if (this->FaceLocations)
    {
      this->FaceLocations->InsertNextCell(0);
    }
  }
  else
  {
    // For polyhedron, npts is actually number of faces, ptIds is of format:
    // (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
    vtkIdType realnpts;

    // We defer allocation for the faces because they are not commonly used and
    // we only want to allocate when necessary.
    if (!this->Faces)
    {
      this->Faces = vtkSmartPointer<vtkCellArray>::New();
      this->Faces->AllocateEstimate(this->Types->GetSize() * 6, 4); // assume 4 pts/face
      this->FaceLocations = vtkSmartPointer<vtkCellArray>::New();
      this->FaceLocations->AllocateEstimate(this->Types->GetSize(), 6); // assume 6 faces/cell
      // FaceLocations must be padded until the current position
      for (vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
      {
        this->FaceLocations->InsertNextCell(0);
      }
    }

    // insert cell connectivity and faces array
    vtkUnstructuredGrid::DecomposeAPolyhedronCell(
      npts, ptIds, realnpts, this->Connectivity, this->Faces, this->FaceLocations);
  }

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//------------------------------------------------------------------------------
// Insert/create cell in object by type and list of point and face ids
// defining cell topology. This method is meant for face-explicit cells (e.g.
// polyhedron).
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType pts[], vtkIdType nfaces, const vtkIdType faces[])
{
  if (type != VTK_POLYHEDRON)
  {
    return this->InsertNextCell(type, npts, pts);
  }
  // Insert connectivity (points that make up polyhedron)
  this->Connectivity->InsertNextCell(npts, pts);

  // Now insert faces; allocate storage if necessary.
  // We defer allocation for the faces because they are not commonly used and
  // we only want to allocate when necessary.
  if (!this->Faces)
  {
    this->Faces = vtkSmartPointer<vtkCellArray>::New();
    this->Faces->AllocateEstimate(this->Types->GetSize() * 6, 4); // assume 4 pts/face
    this->FaceLocations = vtkSmartPointer<vtkCellArray>::New();
    this->FaceLocations->AllocateEstimate(this->Types->GetSize(), 6); // assume 6 faces/cell
    // FaceLocations must be padded until the current position
    for (vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
    {
      this->FaceLocations->InsertNextCell(0);
    }
  }

  vtkIdType faceId = this->Faces->GetNumberOfCells();
  this->FaceLocations->InsertNextCell(nfaces);
  for (int faceNum = 0; faceNum < nfaces; ++faceNum)
  {
    this->FaceLocations->InsertCellPoint(faceId++);
  }

  for (int faceNum = 0; faceNum < nfaces; ++faceNum)
  {
    npts = faces[0];
    this->Faces->InsertNextCell(npts, &faces[1]);
    faces += npts + 1;
  } // for all faces

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//------------------------------------------------------------------------------
// Insert/create cell in object by type and list of point and face ids
// defining cell topology. This method is meant for face-explicit cells (e.g.
// polyhedron).
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType pts[], vtkCellArray* faces)
{
  if (type != VTK_POLYHEDRON)
  {
    return this->InsertNextCell(type, npts, pts);
  }
  // Insert connectivity (points that make up polyhedron)
  this->Connectivity->InsertNextCell(npts, pts);

  // Now insert faces; allocate storage if necessary.
  // We defer allocation for the faces because they are not commonly used and
  // we only want to allocate when necessary.
  if (!this->Faces)
  {
    this->Faces = vtkSmartPointer<vtkCellArray>::New();
    this->Faces->AllocateEstimate(this->Types->GetSize() * 6, 4); // assume 4 pts/face
    this->FaceLocations = vtkSmartPointer<vtkCellArray>::New();
    this->FaceLocations->AllocateEstimate(this->Types->GetSize(), 6); // assume 6 faces/cell
    // FaceLocations must be padded until the current position
    for (vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
    {
      this->FaceLocations->InsertNextCell(0);
    }
  }
  vtkIdType nfaces = faces->GetNumberOfCells();
  vtkIdType faceId = this->Faces->GetNumberOfCells();
  this->FaceLocations->InsertNextCell(nfaces);
  for (int faceNum = 0; faceNum < nfaces; ++faceNum)
  {
    this->FaceLocations->InsertCellPoint(faceId++);
  }
  this->Faces->Append(faces); // all faces

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::InitializeFacesRepresentation(vtkIdType numPrevCells)
{
  if (this->Faces || this->FaceLocations)
  {
    vtkErrorMacro("Face information already exist for this unstructured grid. "
                  "InitializeFacesRepresentation returned without execution.");
    return 0;
  }

  this->Faces = vtkSmartPointer<vtkCellArray>::New();
  this->Faces->AllocateEstimate(this->Types->GetSize() * 6, 4); // assume 4 pts/face
  this->FaceLocations = vtkSmartPointer<vtkCellArray>::New();
  this->FaceLocations->AllocateEstimate(this->Types->GetSize(), 6); // assume 6 faces/cell

  // FaceLocations must be padded until the current position
  for (vtkIdType i = 0; i < numPrevCells; i++)
  {
    this->FaceLocations->InsertNextCell(0);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkUnstructuredGrid::GetMeshMTime()
{
  vtkMTimeType time = vtkMath::Max(this->Points ? this->Points->GetMTime() : 0,
    this->Connectivity ? this->Connectivity->GetMTime() : 0);

  time = vtkMath::Max(this->GetGhostCellsTime(), time);

  return time;
}

//----------------------------------------------------------------------------
// Supporting functions for GetPolyhedronFaces()
namespace
{
struct GetPolyhedronNPts : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId,
    const vtkCellArray* faces, vtkIdType& npts)
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    const vtkIdType NumberOfFaces = static_cast<vtkIdType>(endOffset - beginOffset);
    const auto cellFaces = GetRange(conn).begin() + beginOffset;

    npts = 0;
    for (vtkIdType faceNum = 0; faceNum < NumberOfFaces; ++faceNum)
    {
      npts += faces->GetCellSize(static_cast<vtkIdType>(cellFaces[faceNum]));
    }
  }
};

template <typename PointTypeIter>
struct InsertNextCellPoints : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  vtkIdType operator()(
    OffsetsT* offsets, ConnectivityT* conn, const vtkIdType npts, const PointTypeIter pts)
  {
    using ValueType = GetAPIType<OffsetsT>;
    using OffsetsAccessorType = vtkDataArrayAccessor<OffsetsT>;
    using ConnectivityAccessorType = vtkDataArrayAccessor<ConnectivityT>;
    ConnectivityAccessorType connAccessor(conn);
    OffsetsAccessorType offsetsAccessor(offsets);

    const vtkIdType cellId = offsets->GetNumberOfValues() - 1;

    offsetsAccessor.InsertNext(static_cast<ValueType>(conn->GetNumberOfValues() + npts));

    for (vtkIdType i = 0; i < npts; ++i)
    {
      connAccessor.InsertNext(static_cast<ValueType>(pts[i]));
    }

    return cellId;
  }
};

template <typename FaceIdTypeIter>
struct CopyPolyhedronFaces : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType NumberOfFaces,
    const FaceIdTypeIter cellFaces, vtkCellArray* faces)
  {
    auto offsetsRange = GetRange(offsets);
    auto connRange = GetRange(conn);
    for (vtkIdType faceNum = 0; faceNum < NumberOfFaces; ++faceNum)
    {
      const auto& beginOffset = offsetsRange[cellFaces[faceNum]];
      const auto& endOffset = offsetsRange[cellFaces[faceNum] + 1];
      const vtkIdType NumberOfPoints = static_cast<vtkIdType>(endOffset - beginOffset);
      const auto cellPoints = connRange.begin() + beginOffset;
      using TInsertNextCellPoints = InsertNextCellPoints<decltype(cellPoints)>;

      faces->Dispatch(TInsertNextCellPoints{}, NumberOfPoints, cellPoints);
    }
  }
};

struct CopyPolyhedronCell : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId, vtkCellArray* src,
    vtkCellArray* tgt)
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    const vtkIdType NumberOfFaces = static_cast<vtkIdType>(endOffset - beginOffset);
    const auto cellFaces = GetRange(conn).begin() + beginOffset;
    using TCopyPolyhedronFaces = CopyPolyhedronFaces<decltype(cellFaces)>;

    src->Dispatch(TCopyPolyhedronFaces{}, NumberOfFaces, cellFaces, tgt);
  }
};

}
//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetPolyhedronFaces(vtkIdType cellId, vtkCellArray* faces)
{
  if (faces == nullptr)
  {
    return;
  }
  faces->Reset();
  // Get the locations of the face
  if (!this->Faces || cellId < 0 || cellId > this->FaceLocations->GetNumberOfCells() ||
    (this->FaceLocations->GetCellSize(cellId)) == 0)
  {
    return;
  }
  vtkIdType nfaces = 0;
  vtkIdType npts = 0;

  nfaces = this->FaceLocations->GetCellSize(cellId);
  this->FaceLocations->Dispatch(GetPolyhedronNPts{}, cellId, this->Faces, npts);
  faces->AllocateExact(nfaces, npts);
  this->FaceLocations->Dispatch(CopyPolyhedronCell{}, cellId, this->Faces, faces);
}

//------------------------------------------------------------------------------
vtkCellArray* vtkUnstructuredGrid::GetPolyhedronFaces()
{
  return this->Faces;
}

//------------------------------------------------------------------------------
vtkCellArray* vtkUnstructuredGrid::GetPolyhedronFaceLocations()
{
  return this->FaceLocations;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(int type, vtkCellArray* cells)
{
  vtkNew<vtkUnsignedCharArray> types;
  types->SetNumberOfComponents(1);
  types->SetNumberOfValues(cells->GetNumberOfCells());
  types->FillValue(static_cast<unsigned char>(type));

  this->SetCells(types, cells);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(int* types, vtkCellArray* cells)
{
  const vtkIdType ncells = cells->GetNumberOfCells();

  // Convert the types into a vtkUnsignedCharArray:
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(ncells);
  auto typeRange = vtk::DataArrayValueRange<1>(cellTypes);
  std::transform(types, types + ncells, typeRange.begin(),
    [](int t) -> unsigned char { return static_cast<unsigned char>(t); });

  this->SetCells(cellTypes, cells);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells)
{
  // check if cells contain any polyhedron cell
  const vtkIdType ncells = cells->GetNumberOfCells();
  const auto typeRange = vtk::DataArrayValueRange<1>(cellTypes);
  const bool containPolyhedron =
    std::find(typeRange.cbegin(), typeRange.cend(), VTK_POLYHEDRON) != typeRange.cend();

  if (!containPolyhedron)
  {
    this->SetPolyhedralCells(cellTypes, cells, nullptr, nullptr);
    return;
  }

  // If a polyhedron cell exists, its input cellArray is of special format.
  // [nCell0Faces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
  // We need to convert it into new cell connectivities of standard format,
  // Create faces and facelocations.
  vtkNew<vtkCellArray> newCells;
  newCells->AllocateExact(ncells, cells->GetNumberOfConnectivityIds());

  vtkNew<vtkCellArray> faces;
  faces->AllocateExact(ncells, cells->GetNumberOfConnectivityIds());

  vtkNew<vtkCellArray> faceLocations;
  faceLocations->AllocateExact(ncells, 4 * ncells);

  auto cellIter = vtkSmartPointer<vtkCellArrayIterator>::Take(cells->NewIterator());

  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellIter->GetCurrentCell(npts, pts);
    const vtkIdType cellId = cellIter->GetCurrentCellId();

    if (cellTypes->GetValue(cellId) != VTK_POLYHEDRON)
    {
      newCells->InsertNextCell(npts, pts);
      faceLocations->InsertNextCell(0);
    }
    else
    {
      vtkIdType realnpts;
      vtkIdType nfaces;
      vtkUnstructuredGrid::DecomposeAPolyhedronCell(
        pts, realnpts, nfaces, newCells, faces, faceLocations);
    }
  }

  this->SetPolyhedralCells(cellTypes, newCells, faceLocations, faces);
}

// VTK_DEPRECATED_IN_9_6_0()
//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells,
  vtkIdTypeArray* faceLocations, vtkIdTypeArray* faces)
{
  this->Connectivity = cells;
  this->Types = cellTypes;
  this->DistinctCellTypes = nullptr;
  this->DistinctCellTypesUpdateMTime = 0;
  this->Faces = nullptr;
  this->FaceLocations = nullptr;
  if (faceLocations != nullptr && faces != nullptr)
  {
    vtkIdType prepareSize = faceLocations->GetSize();
    vtkIdType faceId = 0;

    vtkNew<vtkCellArray> newFaces;
    newFaces->AllocateExact(prepareSize, faces->GetSize());

    vtkNew<vtkCellArray> newFaceLocations;
    newFaceLocations->AllocateExact(prepareSize, 4 * prepareSize);

    auto cellIter = vtkSmartPointer<vtkCellArrayIterator>::Take(cells->NewIterator());
    for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      const vtkIdType cellId = cellIter->GetCurrentCellId();
      if (cellTypes->GetValue(cellId) != VTK_POLYHEDRON)
      {
        newFaceLocations->InsertNextCell(0);
      }
      else
      {
        const vtkIdType loc = faceLocations->GetValue(cellId);
        vtkIdType* facePtr = faces->GetPointer(loc);
        vtkIdType nfaces = *facePtr++;
        vtkIdType len = 0;
        newFaceLocations->InsertNextCell(nfaces);
        for (vtkIdType i = 0; i < nfaces; ++i)
        {
          len += *(facePtr + len);
          len++;
          newFaceLocations->InsertCellPoint(faceId++);
        }
        newFaces->AppendLegacyFormat(facePtr, len, 0);
      }
    }
    this->Faces = newFaces;
    this->FaceLocations = newFaceLocations;
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::SetPolyhedralCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells,
  vtkCellArray* faceLocations, vtkCellArray* faces)
{
  this->Connectivity = cells;
  this->Types = cellTypes;
  this->DistinctCellTypes = nullptr;
  this->DistinctCellTypesUpdateMTime = 0;
  this->Faces = faces;
  this->FaceLocations = faceLocations;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::BuildLinks()
{
  if (!this->Points)
  {
    return;
  }
  // Create appropriate links. Currently, it's either a vtkCellLinks (when
  // the dataset is editable) or vtkStaticCellLinks (when the dataset is
  // not editable).
  if (!this->Links)
  {
    if (!this->Editable)
    {
      this->Links = vtkSmartPointer<vtkStaticCellLinks>::New();
    }
    else
    {
      this->Links = vtkSmartPointer<vtkCellLinks>::New();
      static_cast<vtkCellLinks*>(this->Links.Get())->Allocate(this->GetNumberOfPoints());
    }
    this->Links->SetDataSet(this);
  }
  else if (this->Points->GetMTime() > this->Links->GetMTime())
  {
    this->Links->SetDataSet(this);
  }
  this->Links->BuildLinks();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetPointCells(vtkIdType ptId, vtkIdType& ncells, vtkIdType*& cells)
{
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());

    ncells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());

    ncells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  this->Connectivity->GetCellAtId(cellId, ptIds);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetDistinctCellTypes(vtkCellTypes* types)
{
  this->GetDistinctCellTypesArray();
  types->DeepCopy(this->DistinctCellTypes);
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkUnstructuredGrid::GetDistinctCellTypesArray()
{
  if (this->Types == nullptr)
  {
    if (this->DistinctCellTypes == nullptr)
    {
      this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
    }
    return this->DistinctCellTypes->GetCellTypesArray();
  }

  if (this->DistinctCellTypes == nullptr ||
    this->Types->GetMTime() > this->DistinctCellTypesUpdateMTime)
  {
    if (this->DistinctCellTypes)
    {
      this->DistinctCellTypes->Reset();
    }
    else
    {
      this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
      this->DistinctCellTypes->Register(this);
      this->DistinctCellTypes->Delete();
    }
    vtkDataSet::GetDistinctCellTypes(this->DistinctCellTypes);

    this->DistinctCellTypesUpdateMTime = this->Types->GetMTime();
  }

  return this->DistinctCellTypes->GetCellTypesArray();
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkUnstructuredGrid::GetCellTypesArray()
{
  return this->Types;
}

//----------------------------------------------------------------------------
// Supporting functions for GetFaceStream()
namespace
{
template <typename FaceIdTypeIter>
struct GetFaceStreamVisitor : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType NumberOfFaces,
    const FaceIdTypeIter cellFaces, vtkIdList* faceStream)
  {
    auto offsetsRange = GetRange(offsets);
    auto connRange = GetRange(conn);
    for (vtkIdType faceNum = 0; faceNum < NumberOfFaces; ++faceNum)
    {
      const auto& beginOffset = offsetsRange[cellFaces[faceNum]];
      const auto& endOffset = offsetsRange[cellFaces[faceNum] + 1];
      const vtkIdType NumberOfPoints = static_cast<vtkIdType>(endOffset - beginOffset);
      const auto cellPoints = connRange.begin() + beginOffset;

      faceStream->InsertNextId(NumberOfPoints);
      for (vtkIdType ptIdx = 0; ptIdx < NumberOfPoints; ++ptIdx)
      {
        faceStream->InsertNextId(static_cast<vtkIdType>(cellPoints[ptIdx]));
      }
    }
  }
};

struct GetPolyFaceStreamVisitor : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId,
    vtkCellArray* faceArray, vtkIdList* faceStream)
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    const vtkIdType NumberOfFaces = static_cast<vtkIdType>(endOffset - beginOffset);

    if (NumberOfFaces == 0)
    {
      return;
    }
    const auto cellFaces = GetRange(conn).begin() + beginOffset;
    using TGetFaceStream = GetFaceStreamVisitor<decltype(cellFaces)>;
    faceArray->Dispatch(TGetFaceStream{}, NumberOfFaces, cellFaces, faceStream);
  }
};
}
//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetFaceStream(vtkIdType cellId, vtkIdList* ptIds)
{
  if (this->GetCellType(cellId) != VTK_POLYHEDRON)
  {
    this->GetCellPoints(cellId, ptIds);
    return;
  }

  ptIds->Reset();

  if (!this->Faces || !this->FaceLocations)
  {
    return;
  }

  // Get the locations of the face
  if (cellId < 0 || cellId > this->FaceLocations->GetNumberOfCells() ||
    (this->FaceLocations->GetCellSize(cellId) == 0))
  {
    return;
  }
  vtkIdType nfaces = this->FaceLocations->GetCellSize(cellId);
  ptIds->InsertNextId(nfaces);
  this->FaceLocations->Dispatch(GetPolyFaceStreamVisitor{}, cellId, this->Faces, ptIds);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  if (!this->Links)
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  vtkIdType numCells, *cells;
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }

  cellIds->SetNumberOfIds(numCells);
  for (auto i = 0; i < numCells; i++)
  {
    cellIds->SetId(i, cells[i]);
  }
}

//------------------------------------------------------------------------------
vtkCellIterator* vtkUnstructuredGrid::NewCellIterator()
{
  vtkUnstructuredGridCellIterator* iter(vtkUnstructuredGridCellIterator::New());
  iter->SetUnstructuredGrid(this);
  return iter;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::Reset()
{
  if (this->Connectivity)
  {
    this->Connectivity->Reset();
  }
  if (this->Links)
  {
    this->Links->Reset();
  }
  if (this->Types)
  {
    this->Types->Reset();
  }
  if (this->DistinctCellTypes)
  {
    this->DistinctCellTypes->Reset();
  }
  if (this->Faces)
  {
    this->Faces->Reset();
  }
  if (this->FaceLocations)
  {
    this->FaceLocations->Reset();
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::Squeeze()
{
  if (this->Connectivity)
  {
    this->Connectivity->Squeeze();
  }
  if (this->Links)
  {
    this->Links->Squeeze();
  }
  if (this->Types)
  {
    this->Types->Squeeze();
  }
  if (this->Faces)
  {
    this->Faces->Squeeze();
  }
  if (this->FaceLocations)
  {
    this->FaceLocations->Squeeze();
  }

  vtkPointSet::Squeeze();
}

//------------------------------------------------------------------------------
// Remove a reference to a cell in a particular point's link list. You may
// also consider using RemoveCellReference() to remove the references from
// all the cell's points to the cell. This operator does not reallocate
// memory; use the operator ResizeCellList() to do this if necessary. Note that
// dataset should be set to "Editable".
void vtkUnstructuredGrid::RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->RemoveCellReference(cellId, ptId);
}

//------------------------------------------------------------------------------
// Add a reference to a cell in a particular point's link list. (You may also
// consider using AddCellReference() to add the references from all the
// cell's points to the cell.) This operator does not realloc memory; use the
// operator ResizeCellList() to do this if necessary. Note that dataset
// should be set to "Editable".
void vtkUnstructuredGrid::AddReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->AddCellReference(cellId, ptId);
}

//------------------------------------------------------------------------------
// Resize the list of cells using a particular point. (This operator assumes
// that BuildLinks() has been called.) Note that dataset should be set to
// "Editable".
void vtkUnstructuredGrid::ResizeCellList(vtkIdType ptId, int size)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->ResizeCellList(ptId, size);
}

//------------------------------------------------------------------------------
// Replace the points defining cell "cellId" with a new set of points. This
// operator is (typically) used when links from points to cells have not been
// built (i.e., BuildLinks() has not been executed). Use the operator
// ReplaceLinkedCell() to replace a cell when cell structure has been built.
void vtkUnstructuredGrid::InternalReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[])
{
  this->Connectivity->ReplaceCellAtId(cellId, npts, pts);
}

//------------------------------------------------------------------------------
// Add a new cell to the cell data structure (after cell links have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.) Note that the dataset must
// be in "Editable" mode.
vtkIdType vtkUnstructuredGrid::InsertNextLinkedCell(int type, int npts, const vtkIdType pts[])
{
  vtkIdType i, id;

  id = this->InsertNextCell(type, npts, pts);

  vtkCellLinks* clinks = static_cast<vtkCellLinks*>(this->Links.Get());
  for (i = 0; i < npts; i++)
  {
    clinks->ResizeCellList(pts[i], 1);
    clinks->AddCellReference(id, pts[i]);
  }

  return id;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Links, "Links");
}

//------------------------------------------------------------------------------
unsigned long vtkUnstructuredGrid::GetActualMemorySize()
{
  unsigned long size = this->vtkPointSet::GetActualMemorySize();
  if (this->Connectivity)
  {
    size += this->Connectivity->GetActualMemorySize();
  }

  if (this->Links)
  {
    size += this->Links->GetActualMemorySize();
  }

  if (this->Types)
  {
    size += this->Types->GetActualMemorySize();
  }

  if (this->Faces)
  {
    size += this->Faces->GetActualMemorySize();
  }

  if (this->FaceLocations)
  {
    size += this->FaceLocations->GetActualMemorySize();
  }

  return size;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::ShallowCopy(vtkDataObject* dataObject)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(dataObject);
  if (this == grid)
  {
    return;
  }
  this->Superclass::ShallowCopy(dataObject);
  if (grid)
  {
    // I do not know if this is correct but.
    // ^ I really hope this comment lives for another 20 years.

    this->Connectivity = grid->Connectivity;
    this->Types = grid->Types;
    this->DistinctCellTypes = nullptr;
    this->DistinctCellTypesUpdateMTime = 0;
    this->Faces = grid->Faces;
    this->FaceLocations = grid->FaceLocations;

    if (grid->Links)
    {
      this->Links = vtkSmartPointer<vtkAbstractCellLinks>::Take(grid->Links->NewInstance());
      this->Links->SetDataSet(this);
      this->Links->ShallowCopy(grid->Links);
    }
    else
    {
      this->Links = nullptr;
    }
  }
  else if (vtkUnstructuredGridBase* ugb = vtkUnstructuredGridBase::SafeDownCast(dataObject))
  {
    bool isNewAlloc = false;
    if (!this->Connectivity || !this->Types)
    {
      this->AllocateEstimate(ugb->GetNumberOfCells(), ugb->GetMaxCellSize());
      isNewAlloc = true;
    }

    // The source object has vtkUnstructuredGrid topology, but a different
    // cell implementation. Deep copy the cells, and shallow copy the rest:
    auto cellIter = vtkSmartPointer<vtkCellIterator>::Take(ugb->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      this->InsertNextCell(cellIter->GetCellType(), cellIter->GetNumberOfPoints(),
        cellIter->GetPointIds()->GetPointer(0), cellIter->GetCellFaces());
    }

    if (isNewAlloc)
    {
      this->Squeeze();
    }
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DeepCopy(vtkDataObject* dataObject)
{
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(dataObject);

  if (grid)
  {
    // Skip the unstructured grid base implementation, as it uses a less
    // efficient method of copying cell data.
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
    this->vtkUnstructuredGridBase::Superclass::DeepCopy(grid);

    if (grid->Connectivity)
    {
      this->Connectivity = vtkSmartPointer<vtkCellArray>::New();
      this->Connectivity->DeepCopy(grid->Connectivity);
    }
    else
    {
      this->Connectivity = nullptr;
    }
    if (grid->Types)
    {
      this->Types = vtkSmartPointer<vtkUnsignedCharArray>::New();
      this->Types->DeepCopy(grid->Types);
    }
    else
    {
      this->Types = nullptr;
    }
    if (grid->DistinctCellTypes)
    {
      this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
      this->DistinctCellTypes->DeepCopy(grid->DistinctCellTypes);
    }
    else
    {
      this->DistinctCellTypes = nullptr;
    }
    if (grid->Faces)
    {
      this->Faces = vtkSmartPointer<vtkCellArray>::New();
      this->Faces->DeepCopy(grid->Faces);
    }
    else
    {
      this->Faces = nullptr;
    }
    if (grid->FaceLocations)
    {
      this->FaceLocations = vtkSmartPointer<vtkCellArray>::New();
      this->FaceLocations->DeepCopy(grid->FaceLocations);
    }
    else
    {
      this->FaceLocations = nullptr;
    }
    if (grid->Links)
    {
      this->Links = vtkSmartPointer<vtkAbstractCellLinks>::Take(grid->Links->NewInstance());
      this->Links->SetDataSet(this);
      this->Links->DeepCopy(grid->Links);
    }
    else
    {
      this->Links = nullptr;
    }
  }
  else
  {
    // Use the vtkUnstructuredGridBase deep copy implementation.
    this->Superclass::DeepCopy(dataObject);
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Pieces: " << this->GetNumberOfPieces() << endl;
  os << indent << "Piece: " << this->GetPiece() << endl;
  os << indent << "Ghost Level: " << this->GetGhostLevel() << endl;
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGrid::AllocateExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  if (numCells < 1)
  {
    numCells = 1024;
  }
  if (connectivitySize < 1)
  {
    connectivitySize = 1024;
  }

  this->DistinctCellTypesUpdateMTime = 0;
  this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
  this->Types = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Connectivity = vtkSmartPointer<vtkCellArray>::New();

  bool result = this->Connectivity->AllocateExact(numCells, connectivitySize);
  if (result)
  {
    result = this->Types->Allocate(numCells) != 0;
  }
  if (result)
  {
    result = this->DistinctCellTypes->Allocate(VTK_NUMBER_OF_CELL_TYPES) != 0;
  }

  return result;
}

//----------------------------------------------------------------------------
// Supporting functions for IsCellBoundary() and GetCellNeighbors(). These
// algorithms are approached as follows: Find the shortest cell links list
// of a point, and then see if any cells in that list use all the other points
// defining the boundary entity. If so, then the boundary entity is a boundary
// of the cell. If not, then the boundary entity is not a boundary of the cell.
namespace
{
// Determine whether the points provided define a boundary entity (i.e., used
// by only one cell), or whether the points define an interior entity (used
// by more than one cell).
template <class TLinks>
struct IsCellBoundaryImpl : public vtkCellArray::DispatchUtilities
{
  // vtkCellArray::Visit entry point:
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, TLinks* links, vtkIdType cellId,
    vtkIdType nPts, const vtkIdType* pts, vtkIdType& neighborCellId, bool& result) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    neighborCellId = -1;

    // Find the shortest linked list
    auto minPtId = pts[0];
    auto minNumCells = links->GetNcells(minPtId);
    vtkIdType numCells;
    for (vtkIdType i = 1; i < nPts; ++i)
    {
      const auto& ptId = pts[i];
      numCells = links->GetNcells(ptId);
      if (numCells < minNumCells)
      {
        minNumCells = numCells;
        minPtId = ptId;
      }
    }
    const auto minCells = links->GetCells(minPtId);

    // Now for each cell, see if it contains all the face points
    // in the facePts list. If so, then this is not a boundary face.
    const auto connRange = GetRange(conn);
    const auto offsetsRange = GetRange(offsets);
    bool match;
    vtkIdType j;
    ValueType k;
    for (vtkIdType i = 0; i < minNumCells; ++i)
    {
      const auto& minCellId = minCells[i];
      if (minCellId != cellId) // don't include current cell
      {
        // get cell points
        const ValueType nCellPts = offsetsRange[minCellId + 1] - offsetsRange[minCellId];
        const auto cellPts = connRange.begin() + offsetsRange[minCellId];
        match = true;
        for (j = 0; j < nPts && match; ++j) // for all pts in input boundary entity
        {
          const auto& ptId = pts[j];
          if (ptId != minPtId) // of course minPtId is contained by cell
          {
            match = false;
            for (k = 0; k < nCellPts; ++k) // for all points in candidate cell
            {
              if (ptId == cellPts[k])
              {
                match = true; // a match was found
                break;
              }
            } // for all points in current cell
          }   // if not guaranteed match
        }     // for all input points
        if (match)
        {
          neighborCellId = minCellId;
          result = false;
          return;
        }
      } // if not the reference cell
    }   // for each cell in minimum linked list
    result = true;
  }
};

// Identify the neighbors to the specified cell, where the neighbors
// use all the points in the points list (pts).
template <class TLinks>
struct GetCellNeighborsImpl : public vtkCellArray::DispatchUtilities
{
  // vtkCellArray::Visit entry point:
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, TLinks* links, vtkIdType cellId,
    vtkIdType nPts, const vtkIdType* pts, vtkIdList* cellIds) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    // Find the shortest linked list
    auto minPtId = pts[0];
    auto minNumCells = links->GetNcells(minPtId);
    vtkIdType numCells;
    for (vtkIdType i = 1; i < nPts; ++i)
    {
      const auto& ptId = pts[i];
      numCells = links->GetNcells(ptId);
      if (numCells < minNumCells)
      {
        minNumCells = numCells;
        minPtId = ptId;
      }
    }
    const auto minCells = links->GetCells(minPtId);

    // Now for each cell, see if it contains all the face points
    // in the facePts list. If so, then this is not a boundary face.
    const auto connRange = GetRange(conn);
    const auto offsetsRange = GetRange(offsets);
    bool match;
    vtkIdType j;
    ValueType k;
    for (vtkIdType i = 0; i < minNumCells; ++i)
    {
      const auto& minCellId = minCells[i];
      if (minCellId != cellId) // don't include current cell
      {
        // get cell points
        const ValueType nCellPts = offsetsRange[minCellId + 1] - offsetsRange[minCellId];
        const auto cellPts = connRange.begin() + offsetsRange[minCellId];
        match = true;
        for (j = 0; j < nPts && match; ++j) // for all pts in input boundary entity
        {
          const auto& ptId = pts[j];
          if (ptId != minPtId) // of course minPtId is contained by cell
          {
            match = false;
            for (k = 0; k < nCellPts; ++k) // for all points in candidate cell
            {
              if (ptId == cellPts[k])
              {
                match = true; // a match was found
                break;
              }
            } // for all points in current cell
          }   // if not guaranteed match
        }     // for all input points
        if (match)
        {
          cellIds->InsertNextId(minCellId);
        }
      } // if not the reference cell
    }   // for each cell in minimum linked list
  }
};
} // end anonymous namespace

//----------------------------------------------------------------------------
bool vtkUnstructuredGrid::IsCellBoundary(
  vtkIdType cellId, vtkIdType npts, const vtkIdType* pts, vtkIdType& neighborCellId)
{
  // Ensure that a valid neighborhood request is made.
  if (npts <= 0)
  {
    return false;
  }

  // Ensure that cell links are available.
  if (!this->Links)
  {
    this->BuildLinks();
  }

  // Get the links (cells that use each point) depending on the editable
  // state of this object.
  if (!this->Editable)
  {
    using CellLinksType = vtkStaticCellLinks;
    using TIsCellBoundary = IsCellBoundaryImpl<CellLinksType>;
    auto links = static_cast<CellLinksType*>(this->Links.Get());
    bool result;
    this->Connectivity->Dispatch(
      TIsCellBoundary{}, links, cellId, npts, pts, neighborCellId, result);
    return result;
  }
  else
  {
    using CellLinksType = vtkCellLinks;
    using TIsCellBoundary = IsCellBoundaryImpl<CellLinksType>;
    auto links = static_cast<CellLinksType*>(this->Links.Get());
    bool result;
    this->Connectivity->Dispatch(
      TIsCellBoundary{}, links, cellId, npts, pts, neighborCellId, result);
    return result;
  }
}

//----------------------------------------------------------------------------
// Return the cells that use all of the ptIds provided. This is a set
// (intersection) operation - it can have significant performance impacts on
// certain filters like vtkGeometryFilter.
void vtkUnstructuredGrid::GetCellNeighbors(
  vtkIdType cellId, vtkIdType npts, const vtkIdType* pts, vtkIdList* cellIds)
{
  // Empty the list
  cellIds->Reset();

  // Ensure that a proper neighborhood request is made.
  if (npts <= 0)
  {
    return;
  }

  // Ensure that links are built.
  if (!this->Links)
  {
    this->BuildLinks();
  }

  // Get the cell links based on the current state.
  if (!this->Editable)
  {
    using CellLinksType = vtkStaticCellLinks;
    using TGetCellNeighbors = GetCellNeighborsImpl<CellLinksType>;
    auto links = static_cast<CellLinksType*>(this->Links.Get());
    this->Connectivity->Dispatch(TGetCellNeighbors{}, links, cellId, npts, pts, cellIds);
  }
  else
  {
    using CellLinksType = vtkCellLinks;
    using TGetCellNeighbors = GetCellNeighborsImpl<CellLinksType>;
    auto links = static_cast<CellLinksType*>(this->Links.Get());
    this->Connectivity->Dispatch(TGetCellNeighbors{}, links, cellId, npts, pts, cellIds);
  }
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::GetCellNumberOfFaces(
  vtkIdType cellId, unsigned char& cellType, vtkGenericCell* cell)
{
  cellType = static_cast<unsigned char>(this->GetCellType(cellId));
  switch (cellType)
  {
    case VTK_EMPTY_CELL:
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
    case VTK_LINE:
    case VTK_POLY_LINE:
    case VTK_TRIANGLE:
    case VTK_TRIANGLE_STRIP:
    case VTK_POLYGON:
    case VTK_PIXEL:
    case VTK_QUAD:
    case VTK_QUADRATIC_EDGE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_CUBIC_LINE:
    case VTK_PARAMETRIC_CURVE:
    case VTK_PARAMETRIC_SURFACE:
    case VTK_PARAMETRIC_TRI_SURFACE:
    case VTK_PARAMETRIC_QUAD_SURFACE:
    case VTK_HIGHER_ORDER_EDGE:
    case VTK_HIGHER_ORDER_TRIANGLE:
    case VTK_HIGHER_ORDER_QUAD:
    case VTK_HIGHER_ORDER_POLYGON:
    case VTK_LAGRANGE_CURVE:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_CURVE:
    case VTK_BEZIER_TRIANGLE:
    case VTK_BEZIER_QUADRILATERAL:
      return 0;

    case VTK_TETRA:
    case VTK_QUADRATIC_TETRA:
    case VTK_PARAMETRIC_TETRA_REGION:
    case VTK_HIGHER_ORDER_TETRAHEDRON:
    case VTK_LAGRANGE_TETRAHEDRON:
    case VTK_BEZIER_TETRAHEDRON:
      return 4;

    case VTK_PYRAMID:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_TRIQUADRATIC_PYRAMID:
    case VTK_HIGHER_ORDER_PYRAMID:
    case VTK_WEDGE:
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_LINEAR_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_HIGHER_ORDER_WEDGE:
    case VTK_LAGRANGE_WEDGE:
    case VTK_BEZIER_WEDGE:
      return 5;

    case VTK_VOXEL:
    case VTK_HEXAHEDRON:
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_HIGHER_ORDER_HEXAHEDRON:
    case VTK_PARAMETRIC_HEX_REGION:
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
    case VTK_BEZIER_HEXAHEDRON:
      return 6;

    case VTK_PENTAGONAL_PRISM:
      return 7;

    case VTK_HEXAGONAL_PRISM:
      return 8;

    case VTK_POLYHEDRON:
      return this->FaceLocations->GetCellSize(cellId);

    case VTK_CONVEX_POINT_SET:
    default:
      this->GetCell(cellId, cell);
      return cell->GetNumberOfFaces();
  }
}

//------------------------------------------------------------------------------
int vtkUnstructuredGrid::IsHomogeneous()
{
  unsigned char type;
  if (this->Types && this->Types->GetMaxId() >= 0)
  {
    type = Types->GetValue(0);
    vtkIdType numCells = this->GetNumberOfCells();
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      if (this->Types->GetValue(cellId) != type)
      {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
// Fill container with indices of cells which match given type.
void vtkUnstructuredGrid::GetIdsOfCellsOfType(int type, vtkIdTypeArray* array)
{
  for (int cellId = 0; cellId < this->GetNumberOfCells(); cellId++)
  {
    if (static_cast<int>(Types->GetValue(cellId)) == type)
    {
      array->InsertNextValue(cellId);
    }
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::RemoveGhostCells()
{
  if (!this->GetNumberOfCells() || !this->CellData->GetGhostArray())
  {
    return;
  }
  vtkNew<vtkUnstructuredGrid> newGrid;

  vtkNew<vtkCellArray> newFaces;
  if (this->Faces)
  {
    this->Faces->IsStorage64Bit() ? newFaces->Use64BitStorage() : newFaces->Use32BitStorage();
  }
  vtkNew<vtkCellArray> newFaceLocations;
  if (this->FaceLocations)
  {
    this->FaceLocations->IsStorage64Bit() ? newFaceLocations->Use64BitStorage()
                                          : newFaceLocations->Use32BitStorage();
  }

  vtkSmartPointer<vtkDataArray> FacesOffset;
  vtkSmartPointer<vtkDataArray> FacesElements;
  vtkSmartPointer<vtkDataArray> FaceLocationsOffset;
  vtkSmartPointer<vtkDataArray> FaceLocationsElements;

  if (this->GetPolyhedronFaces())
  {
    FacesOffset = this->Faces->GetOffsetsArray();
    FacesElements = this->Faces->GetConnectivityArray();
    FaceLocationsOffset = this->FaceLocations->GetOffsetsArray();
    FaceLocationsElements = this->FaceLocations->GetConnectivityArray();
    //
    newFaces->AllocateEstimate(
      FacesOffset->GetNumberOfValues() - 1, FacesElements->GetNumberOfValues());
    newFaceLocations->AllocateEstimate(
      this->GetNumberOfCells(), FaceLocationsElements->GetNumberOfValues());
  }

  vtkNew<vtkCellArray> newCells;
  this->Connectivity->IsStorage64Bit() ? newCells->Use64BitStorage() : newCells->Use32BitStorage();

  using Dispatcher = vtkArrayDispatch::Dispatch3ByArray<vtkCellArray::StorageOffsetsArrays,
    vtkCellArray::StorageOffsetsArrays, vtkCellArray::StorageOffsetsArrays>;
  ::RemoveGhostCellsWorker worker;

  if (!Dispatcher::Execute(this->Connectivity->GetOffsetsArray(), FacesOffset.Get(),
        FaceLocationsOffset.Get(), worker, this->Connectivity->GetConnectivityArray(),
        FacesElements.Get(), FaceLocationsElements.Get(), this->Types,
        this->CellData->GetGhostArray(), this->GetNumberOfPoints(), newCells->GetOffsetsArray(),
        newCells->GetConnectivityArray(), newFaces->GetOffsetsArray(),
        newFaces->GetConnectivityArray(), newFaceLocations->GetOffsetsArray(),
        newFaceLocations->GetConnectivityArray()))
  {
    worker(this->Connectivity->GetOffsetsArray(), FacesOffset.Get(), FaceLocationsOffset.Get(),
      this->Connectivity->GetConnectivityArray(), FacesElements.Get(), FaceLocationsElements.Get(),
      this->Types, this->CellData->GetGhostArray(), this->GetNumberOfPoints(),
      newCells->GetOffsetsArray(), newCells->GetConnectivityArray(), newFaces->GetOffsetsArray(),
      newFaces->GetConnectivityArray(), newFaceLocations->GetOffsetsArray(),
      newFaceLocations->GetConnectivityArray());
  }

  vtkNew<vtkUnsignedCharArray> newTypes;
  newTypes->InsertTuplesStartingAt(0, worker.NewCellIdMap, this->Types);

  vtkNew<vtkPoints> newPoints;
  newPoints->SetDataType(this->GetPoints()->GetDataType());
  newPoints->GetData()->InsertTuplesStartingAt(0, worker.NewPointIdMap, this->Points->GetData());
  newGrid->SetPoints(newPoints);

  vtkCellData* outCD = newGrid->GetCellData();
  outCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  outCD->CopyAllocate(this->CellData);
  outCD->CopyData(this->CellData, worker.NewCellIdMap);

  vtkPointData* outPD = newGrid->GetPointData();
  outPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  outPD->CopyAllocate(this->PointData);
  outPD->CopyData(this->PointData, worker.NewPointIdMap);

  this->CopyStructure(newGrid);
  this->GetPointData()->ShallowCopy(newGrid->GetPointData());
  this->GetCellData()->ShallowCopy(newGrid->GetCellData());
  this->SetPolyhedralCells(newTypes, newCells, newFaceLocations, newFaces);

  this->Squeeze();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkCellArray* polyhedronCell,
  vtkIdType& numCellPts, vtkIdType& nCellfaces, vtkCellArray* cellArray, vtkIdTypeArray* faces)
{
  const vtkIdType* cellStream = nullptr;
  vtkIdType cellLength = 0;

  polyhedronCell->InitTraversal();
  polyhedronCell->GetNextCell(cellLength, cellStream);

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    cellStream, numCellPts, nCellfaces, cellArray, faces);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(const vtkIdType* cellStream,
  vtkIdType& numCellPts, vtkIdType& nCellFaces, vtkCellArray* cellArray, vtkIdTypeArray* faces)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
  {
    return;
  }

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    nCellFaces, cellStream + 1, numCellPts, cellArray, faces);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
  const vtkIdType cellStream[], vtkIdType& numCellPts, vtkCellArray* cellArray,
  vtkIdTypeArray* faces)
{
  std::set<vtkIdType> cellPointSet;
  std::set<vtkIdType>::iterator it;

  // insert number of faces into the face array
  faces->InsertNextValue(nCellFaces);

  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; fid++)
  {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    faces->InsertNextValue(npts);
    for (vtkIdType i = 0; i < npts; i++)
    {
      vtkIdType pid = *cellStream++;
      faces->InsertNextValue(pid);
      cellPointSet.insert(pid);
    }
  }

  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (it = cellPointSet.begin(); it != cellPointSet.end(); ++it)
  {
    cellArray->InsertCellPoint(*it);
  }

  // the real number of points in the polyhedron cell.
  numCellPts = static_cast<vtkIdType>(cellPointSet.size());
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkCellArray* polyhedronCell,
  vtkIdType& numCellPts, vtkIdType& nCellfaces, vtkCellArray* cellArray, vtkCellArray* faces)
{
  const vtkIdType* cellStream = nullptr;
  vtkIdType cellLength = 0;

  polyhedronCell->InitTraversal();
  polyhedronCell->GetNextCell(cellLength, cellStream);

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    cellStream, numCellPts, nCellfaces, cellArray, faces);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(const vtkIdType* cellStream,
  vtkIdType& numCellPts, vtkIdType& nCellFaces, vtkCellArray* cellArray, vtkCellArray* faces)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
  {
    return;
  }

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    nCellFaces, cellStream + 1, numCellPts, cellArray, faces);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(const vtkIdType* cellStream,
  vtkIdType& numCellPts, vtkIdType& nCellFaces, vtkCellArray* cellArray, vtkCellArray* faces,
  vtkCellArray* faceLocations)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
  {
    return;
  }

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    nCellFaces, cellStream + 1, numCellPts, cellArray, faces, faceLocations);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
  const vtkIdType cellStream[], vtkIdType& numCellPts, vtkCellArray* cellArray,
  vtkCellArray* facesArray)
{
  std::set<vtkIdType> cellPointSet;
  std::set<vtkIdType>::iterator it;

  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; fid++)
  {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    facesArray->InsertNextCell(static_cast<int>(npts));
    for (vtkIdType i = 0; i < npts; i++)
    {
      vtkIdType pid = *cellStream++;
      facesArray->InsertCellPoint(pid);
      cellPointSet.insert(pid);
    }
  }

  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (const auto& cellPoint : cellPointSet)
  {
    cellArray->InsertCellPoint(cellPoint);
  }

  // the real number of points in the polyhedron cell.
  numCellPts = static_cast<vtkIdType>(cellPointSet.size());
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
  const vtkIdType cellStream[], vtkIdType& numCellPts, vtkCellArray* cellArray, vtkCellArray* faces,
  vtkCellArray* faceLocations)
{
  std::set<vtkIdType> cellPointSet;
  std::set<vtkIdType>::iterator it;

  vtkIdType faceId = faces->GetNumberOfCells();
  faceLocations->InsertNextCell(static_cast<int>(nCellFaces));
  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; ++fid)
  {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    faces->InsertNextCell(static_cast<int>(npts));
    for (vtkIdType i = 0; i < npts; ++i)
    {
      vtkIdType pid = *cellStream++;
      faces->InsertCellPoint(pid);
      cellPointSet.insert(pid);
    }
    faceLocations->InsertCellPoint(faceId++);
  }

  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (const auto& cellPoint : cellPointSet)
  {
    cellArray->InsertCellPoint(cellPoint);
  }

  // the real number of points in the polyhedron cell.
  numCellPts = static_cast<vtkIdType>(cellPointSet.size());
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(vtkIdList* faceStream, vtkIdType* idMap)
{
  vtkIdType* idPtr = faceStream->GetPointer(0);
  vtkIdType nfaces = *idPtr++;
  for (vtkIdType i = 0; i < nfaces; i++)
  {
    vtkIdType npts = *idPtr++;
    for (vtkIdType j = 0; j < npts; j++)
    {
      *idPtr = idMap[*idPtr];
      idPtr++;
    }
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(
  vtkIdType nfaces, vtkIdType* faceStream, vtkIdType* idMap)
{
  vtkIdType* idPtr = faceStream;
  for (vtkIdType i = 0; i < nfaces; i++)
  {
    vtkIdType npts = *idPtr++;
    for (vtkIdType j = 0; j < npts; j++)
    {
      *idPtr = idMap[*idPtr];
      idPtr++;
    }
  }
}
//------------------------------------------------------------------------------
// Helper
namespace
{
struct ConvertVisitor : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* conn, vtkIdType* idMap)
  {
    using ValueType = GetAPIType<OffsetsT>;
    auto connRange = GetRange(conn);
    const vtkIdType nids = connRange.size();
    for (vtkIdType i = 0; i < nids; ++i)
    {
      ValueType tmp = connRange[i];
      connRange[i] = idMap[tmp];
    }
  }
};
}
//------------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(vtkCellArray* faces, vtkIdType* idMap)
{
  faces->Dispatch(ConvertVisitor{}, idMap);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGrid::GetData(vtkInformation* info)
{
  return info ? vtkUnstructuredGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkUnstructuredGrid::GetData(v->GetInformationObject(i));
}
VTK_ABI_NAMESPACE_END
