// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSideData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridSideMap.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef vtkXMLOffsetsManager_DoNotInclude

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
// The side numbering of the input when it was supplied, which has to be
// written for the side data to read back against it. A derived numbering is
// recomputed on reading.
vtkUnstructuredGridSideMap* AuthoredSides(vtkUnstructuredGridBase* input)
{
  auto* grid = vtkUnstructuredGrid::SafeDownCast(input);
  return grid && grid->HasSideMap() && grid->GetSideMap()->GetAuthored() ? grid->GetSideMap()
                                                                         : nullptr;
}

// The side data of the input when it has any arrays.
vtkSideData* SideArrays(vtkUnstructuredGridBase* input)
{
  auto* grid = vtkUnstructuredGrid::SafeDownCast(input);
  return grid && grid->GetSideData()->GetNumberOfArrays() > 0 ? grid->GetSideData() : nullptr;
}
}

vtkStandardNewMacro(vtkXMLUnstructuredGridWriter);

//------------------------------------------------------------------------------
vtkXMLUnstructuredGridWriter::vtkXMLUnstructuredGridWriter()
{
  this->CellsOM = new OffsetsManagerArray;
  this->SideDataOM = new OffsetsManagerArray;
  this->SideIdsOM = new OffsetsManagerArray;
}

//------------------------------------------------------------------------------
vtkXMLUnstructuredGridWriter::~vtkXMLUnstructuredGridWriter()
{
  delete this->CellsOM;
  delete this->SideDataOM;
  delete this->SideIdsOM;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkUnstructuredGridBase* vtkXMLUnstructuredGridWriter::GetInput()
{
  return static_cast<vtkUnstructuredGridBase*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridWriter::GetDataSetName()
{
  return "UnstructuredGrid";
}

//------------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "vtu";
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteInlinePieceAttributes()
{
  this->Superclass::WriteInlinePieceAttributes();
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  vtkUnstructuredGridBase* input = this->GetInput();
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkUnstructuredGridBase* input = this->GetInput();

  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell specifications.
  if (vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(input))
  {
    // This is a bit more efficient and avoids iteration over all cells.
    this->WritePolyCellsInline("Cells", grid->GetCells(), grid->GetCellTypes(),
      grid->GetOrientedPolyhedronFaces(), grid->GetOrientedPolyhedronFaceLocations(), indent);
  }
  else
  {
    auto cellIter = vtk::TakeSmartPointer(input->NewCellIterator());
    this->WriteCellsInline(
      "Cells", cellIter, input->GetNumberOfCells(), input->GetMaxCellSize(), indent);
  }

  if (vtkUnstructuredGridSideMap* sides = AuthoredSides(input))
  {
    ostream& os = *(this->Stream);
    os << indent << "<SideIds";
    this->WriteScalarAttribute("NumberOfSides", sides->GetNumberOfSides());
    this->WriteScalarAttribute("NumberOfHalfSides", sides->GetNumberOfHalfSides());
    os << ">\n";
    this->WriteArrayInline(sides->GetHalfSideToSide(), indent.GetNextIndent(), "HalfSideToSide");
    os << indent << "</SideIds>\n";
  }
  if (vtkSideData* sideData = SideArrays(input))
  {
    this->WriteDataSetAttributesInline(sideData, "SideData", indent, 1);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();

  this->NumberOfCellsPositions = new vtkTypeInt64[this->NumberOfPieces];
  this->CellsOM->Allocate(this->NumberOfPieces, 7, this->NumberOfTimeSteps);
  this->SideDataOM->Allocate(this->NumberOfPieces);
  this->SideIdsOM->Allocate(this->NumberOfPieces, 1, this->NumberOfTimeSteps);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();

  delete[] this->NumberOfCellsPositions;
  this->NumberOfCellsPositions = nullptr;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPieceAttributes(int index)
{
  this->Superclass::WriteAppendedPieceAttributes(index);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->NumberOfCellsPositions[index] = this->ReserveAttributeSpace("NumberOfCells");
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPiece(int index, vtkIndent indent)
{
  vtkUnstructuredGridBase* input = this->GetInput();
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  if (vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(input))
  {
    this->ConvertCells(grid->GetCells());
    this->WritePolyCellsAppended("Cells", grid->GetCellTypes(), grid->GetOrientedPolyhedronFaces(),
      grid->GetOrientedPolyhedronFaceLocations(), indent, &this->CellsOM->GetPiece(index));
  }
  else
  {
    vtkCellIterator* cellIter = input->NewCellIterator();
    this->WriteCellsAppended(
      "Cells", cellIter, input->GetNumberOfCells(), indent, &this->CellsOM->GetPiece(index));
    cellIter->Delete();
  }

  if (vtkUnstructuredGridSideMap* sides = AuthoredSides(input))
  {
    ostream& os = *(this->Stream);
    os << indent << "<SideIds";
    this->WriteScalarAttribute("NumberOfSides", sides->GetNumberOfSides());
    this->WriteScalarAttribute("NumberOfHalfSides", sides->GetNumberOfHalfSides());
    os << ">\n";
    OffsetsManager& offsets = this->SideIdsOM->GetPiece(index).GetElement(0);
    for (int t = 0; t < this->NumberOfTimeSteps; ++t)
    {
      this->WriteArrayAppended(
        sides->GetHalfSideToSide(), indent.GetNextIndent(), offsets, "HalfSideToSide", 0, t);
    }
    os << indent << "</SideIds>\n";
  }
  if (vtkSideData* sideData = SideArrays(input))
  {
    this->WriteDataSetAttributesAppended(
      sideData, "SideData", indent, &this->SideDataOM->GetPiece(index), 1);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkUnstructuredGridBase* input = this->GetInput();

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->NumberOfCellsPositions[index]));
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell specification arrays.
  if (vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(input))
  {
    this->WritePolyCellsAppendedData(grid->GetCells(), grid->GetCellTypes(),
      grid->GetOrientedPolyhedronFaces(), grid->GetOrientedPolyhedronFaceLocations(),
      this->CurrentTimeIndex, &this->CellsOM->GetPiece(index));
  }
  else
  {
    vtkCellIterator* cellIter = input->NewCellIterator();
    this->WriteCellsAppendedData(cellIter, input->GetNumberOfCells(), input->GetMaxCellSize(),
      this->CurrentTimeIndex, &this->CellsOM->GetPiece(index));
    cellIter->Delete();
  }

  if (vtkUnstructuredGridSideMap* sides = AuthoredSides(input))
  {
    OffsetsManager& offsets = this->SideIdsOM->GetPiece(index).GetElement(0);
    this->WriteArrayAppendedData(sides->GetHalfSideToSide(),
      offsets.GetPosition(this->CurrentTimeIndex), offsets.GetOffsetValue(this->CurrentTimeIndex));
  }
  if (vtkSideData* sideData = SideArrays(input))
  {
    this->WriteDataSetAttributesAppendedData(
      sideData, this->CurrentTimeIndex, &this->SideDataOM->GetPiece(index));
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredGridWriter::GetNumberOfInputCells()
{
  return this->GetInput()->GetNumberOfCells();
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::CalculateSuperclassFraction(float* fractions)
{
  vtkUnstructuredGridBase* input = this->GetInput();

  // The superclass will write point/cell data and point specifications.
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays * this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays * this->GetNumberOfInputCells();
  vtkIdType pointsSize = this->GetNumberOfInputPoints();

  // This class will write cell specifications.
  vtkIdType connectSize = 0;
  if (vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(input))
  {
    if (grid->GetCells() == nullptr)
    {
      connectSize = 0;
    }
    else
    {
      connectSize = grid->GetCells()->GetNumberOfConnectivityIds();
    }
  }
  else
  {
    vtkCellIterator* cellIter = input->NewCellIterator();
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      connectSize += cellIter->GetNumberOfPoints();
    }
    cellIter->Delete();
  }

  vtkIdType offsetSize = input->GetNumberOfCells();
  vtkIdType typesSize = input->GetNumberOfCells();

  int total = (pdSize + cdSize + pointsSize + connectSize + offsetSize + typesSize);
  if (total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(pdSize + cdSize + pointsSize) / total;
  fractions[2] = 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  return 1;
}
VTK_ABI_NAMESPACE_END
