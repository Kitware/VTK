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

template <typename FaceIdType>
struct FetchFacesVisitor
{
  template <typename CellStateT>
  void operator()(CellStateT& state, const vtkIdType NumberOfFaces, const FaceIdType* cellFaces,
    vtkIdList* faceStream)
  {
    vtkIdType loc = 0;
    faceStream->SetId(0, NumberOfFaces);
    for (vtkIdType faceNum = 0; faceNum < NumberOfFaces; ++faceNum)
    {
      const vtkIdType beginOffset = state.GetBeginOffset(cellFaces[faceNum]);
      const vtkIdType endOffset = state.GetEndOffset(cellFaces[faceNum]);
      const vtkIdType NumberOfPoints = endOffset - beginOffset;
      const auto cellPoints = state.GetConnectivity()->GetPointer(beginOffset);

      faceStream->SetId(++loc, NumberOfPoints);
      for (vtkIdType ptIdx = 0; ptIdx < NumberOfPoints; ++ptIdx)
      {
        faceStream->SetId(++loc, static_cast<vtkIdType>(cellPoints[ptIdx]));
      }
    }
  }
};

struct FetchPolyFacesVisitor
{
  template <typename CellStateT>
  void operator()(
    CellStateT& state, const vtkIdType cellId, vtkCellArray* faceArray, vtkIdList* faceStream)
  {

    using ValueType = typename CellStateT::ValueType;
    using TFetchFaces = FetchFacesVisitor<ValueType>;

    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    const vtkIdType NumberOfFaces = endOffset - beginOffset;

    if (NumberOfFaces == 0)
    {
      return;
    }
    const auto cellFaces = state.GetConnectivity()->GetPointer(beginOffset);
    faceArray->Visit(TFetchFaces{}, NumberOfFaces, cellFaces, faceStream);
  }
};
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchFaces()
{
  if (this->PolyFaceLocs)
  {
    const vtkIdType cellId = this->Cells->GetCurrentCellId();
    vtkIdType nfaces = 0;
    vtkIdType npts = 0;
    vtkIdType facesSize = 1;
    nfaces = this->PolyFaceLocs->GetCellSize(cellId);
    npts = this->PolyFaceLocs->Visit(GetPolyhedronNPts{}, cellId, this->PolyFaceConn);
    facesSize += nfaces;
    facesSize += npts;
    this->Faces->SetNumberOfIds(facesSize);
    this->PolyFaceLocs->Visit(FetchPolyFacesVisitor{}, cellId, this->PolyFaceConn, this->Faces);
  }
  else
  {
    this->Faces->SetNumberOfIds(0);
  }
}
VTK_ABI_NAMESPACE_END
