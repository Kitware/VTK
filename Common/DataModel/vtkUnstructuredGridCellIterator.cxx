// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnstructuredGridCellIterator.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnstructuredGridCellIterator);

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Cells)
  {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Cells: (none)" << endl;
  }

  if (this->Types)
  {
    os << indent << "Types:\n";
    this->Types->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Types: (none)" << endl;
  }

  if (this->PolyFaceConn)
  {
    os << indent << "FaceConn:\n";
    this->PolyFaceConn->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FaceConn: (none)" << endl;
  }

  if (this->PolyFaceLocs)
  {
    os << indent << "FaceLocs:\n";
    this->PolyFaceLocs->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FaceLocs: (none)" << endl;
  }

  if (this->Coords)
  {
    os << indent << "Coords:\n";
    this->Coords->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Coords: (none)" << endl;
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::SetUnstructuredGrid(vtkUnstructuredGrid* ug)
{
  // If the unstructured grid has not been initialized yet, these may not exist:
  vtkUnsignedCharArray* cellTypeArray = ug ? ug->GetCellTypesArray() : nullptr;
  vtkCellArray* cellArray = ug ? ug->GetCells() : nullptr;
  vtkPoints* points = ug ? ug->GetPoints() : nullptr;

  if (points)
  {
    this->Points->SetDataType(points->GetDataType());
  }

  if (ug && cellTypeArray && cellArray && points)
  {
    this->Cells = vtk::TakeSmartPointer(cellArray->NewIterator());
    this->Cells->GoToFirstCell();

    this->Types = cellTypeArray;
    this->PolyFaceConn = ug->GetPolyhedronFaces();
    this->PolyFaceLocs = ug->GetPolyhedronFaceLocations();
    this->Coords = points;
  }
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGridCellIterator::IsDoneWithTraversal()
{
  return this->Cells ? this->Cells->IsDoneWithTraversal() : true;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridCellIterator::GetCellId()
{
  return this->Cells->GetCurrentCellId();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::IncrementToNextCell()
{
  this->Cells->GoToNextCell();
}

//------------------------------------------------------------------------------
vtkUnstructuredGridCellIterator::vtkUnstructuredGridCellIterator() = default;

//------------------------------------------------------------------------------
vtkUnstructuredGridCellIterator::~vtkUnstructuredGridCellIterator() = default;

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::ResetToFirstCell()
{
  if (this->Cells)
  {
    this->Cells->GoToFirstCell();
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchCellType()
{
  const vtkIdType cellId = this->Cells->GetCurrentCellId();
  this->CellType = this->Types->GetValue(cellId);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchPointIds()
{
  this->Cells->GetCurrentCell(this->PointIds);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchPoints()
{
  this->Coords->GetPoints(this->GetPointIds(), this->Points);
}

//----------------------------------------------------------------------------
// Supporting functions for FetchFaces()
namespace
{
struct GetPolyhedronNPts : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId,
    const vtkCellArray* faces, vtkIdType& npts) const
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
    using AccessorType = vtkDataArrayAccessor<OffsetsT>;
    AccessorType connAccessor(conn);
    AccessorType offsetsAccessor(offsets);

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

} // end anon namespace

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchFaces()
{
  if (this->PolyFaceLocs)
  {
    const vtkIdType cellId = this->Cells->GetCurrentCellId();
    vtkIdType nfaces = 0;
    vtkIdType npts = 0;
    nfaces = this->PolyFaceLocs->GetCellSize(cellId);
    this->PolyFaceLocs->Dispatch(GetPolyhedronNPts{}, cellId, this->PolyFaceConn, npts);
    this->Faces->AllocateExact(nfaces, npts);
    this->PolyFaceLocs->Dispatch(CopyPolyhedronCell{}, cellId, this->PolyFaceConn, this->Faces);
  }
  else
  {
    this->Faces->Reset();
  }
}
VTK_ABI_NAMESPACE_END
