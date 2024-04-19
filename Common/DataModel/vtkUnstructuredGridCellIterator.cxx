// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnstructuredGridCellIterator.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>

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
struct GetPolyhedronNPts
{
  // Insert full cell
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& state, const vtkIdType cellId, const vtkCellArray* faces)
  {
    vtkIdType npts = 0;

    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    const vtkIdType NumberOfFaces = endOffset - beginOffset;
    const auto cellFaces = state.GetConnectivity()->GetPointer(beginOffset);

    for (vtkIdType faceNum = 0; faceNum < NumberOfFaces; ++faceNum)
    {
      npts += faces->GetCellSize(static_cast<vtkIdType>(cellFaces[faceNum]));
    }
    return npts;
  }
};

template <typename PointType>
struct InsertNextCellPoints
{
  // Insert full cell
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& state, const vtkIdType npts, const PointType pts[])
  {
    using ValueType = typename CellStateT::ValueType;
    auto* conn = state.GetConnectivity();
    auto* offsets = state.GetOffsets();

    const vtkIdType cellId = offsets->GetNumberOfValues() - 1;

    offsets->InsertNextValue(static_cast<ValueType>(conn->GetNumberOfValues() + npts));

    for (vtkIdType i = 0; i < npts; ++i)
    {
      conn->InsertNextValue(static_cast<ValueType>(pts[i]));
    }

    return cellId;
  }
};

template <typename FaceIdType>
struct CopyPolyhedronFaces
{
  template <typename CellStateT>
  void operator()(CellStateT& state, const vtkIdType NumberOfFaces, const FaceIdType* cellFaces,
    vtkCellArray* faces)
  {
    using ValueType = typename CellStateT::ValueType;
    using TInsertNextCellPoints = InsertNextCellPoints<ValueType>;
    for (vtkIdType faceNum = 0; faceNum < NumberOfFaces; ++faceNum)
    {
      const vtkIdType beginOffset = state.GetBeginOffset(cellFaces[faceNum]);
      const vtkIdType endOffset = state.GetEndOffset(cellFaces[faceNum]);
      const vtkIdType NumberOfPoints = endOffset - beginOffset;
      const auto cellPoints = state.GetConnectivity()->GetPointer(beginOffset);

      faces->Visit(TInsertNextCellPoints{}, NumberOfPoints, cellPoints);
    }
  }
};

struct CopyPolyhedronCell
{
  // Insert full cell
  template <typename CellStateT>
  void operator()(CellStateT& state, const vtkIdType cellId, vtkCellArray* src, vtkCellArray* tgt)
  {
    using ValueType = typename CellStateT::ValueType;
    using TCopyPolyhedronFaces = CopyPolyhedronFaces<ValueType>;
    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    const vtkIdType NumberOfFaces = endOffset - beginOffset;
    const auto cellFaces = state.GetConnectivity()->GetPointer(beginOffset);

    src->Visit(TCopyPolyhedronFaces{}, NumberOfFaces, cellFaces, tgt);
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
    npts = this->PolyFaceLocs->Visit(GetPolyhedronNPts{}, cellId, this->PolyFaceConn);
    this->Faces->AllocateExact(nfaces, npts);
    this->PolyFaceLocs->Visit(CopyPolyhedronCell{}, cellId, this->PolyFaceConn, this->Faces);
  }
  else
  {
    this->Faces->Reset();
  }
}
VTK_ABI_NAMESPACE_END
