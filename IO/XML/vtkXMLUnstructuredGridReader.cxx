/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUnstructuredGridReader.h"

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUpdateCellsV8toV9.h"
#include "vtkXMLDataElement.h"

#include <cassert>

vtkStandardNewMacro(vtkXMLUnstructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLUnstructuredGridReader::vtkXMLUnstructuredGridReader()
{
  this->CellElements = nullptr;
  this->NumberOfCells = nullptr;
  this->CellsTimeStep = -1;
  this->CellsOffset = static_cast<unsigned long>(-1); // almost invalid state
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredGridReader::~vtkXMLUnstructuredGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLUnstructuredGridReader::GetOutput(int idx)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridReader::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  int i;
  this->TotalNumberOfCells = 0;
  for (i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfCells += this->NumberOfCells[i];
  }

  // Data reading will start at the beginning of the output.
  this->StartCell = 0;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfCells = new vtkIdType[numPieces];
  this->CellElements = new vtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->CellElements[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::DestroyPieces()
{
  delete[] this->CellElements;
  delete[] this->NumberOfCells;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredGridReader::GetNumberOfCellsInPiece(int piece)
{
  return this->NumberOfCells[piece];
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  cellTypes->FillValue(VTK_EMPTY_CELL);
  vtkNew<vtkCellArray> outCells;

  output->SetCells(cellTypes, outCells);
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }
  int i;

  if (!ePiece->GetScalarAttribute("NumberOfCells", this->NumberOfCells[this->Piece]))
  {
    vtkErrorMacro("Piece " << this->Piece << " is missing its NumberOfCells attribute.");
    this->NumberOfCells[this->Piece] = 0;
    return 0;
  }

  // Find the Cells element in the piece.
  this->CellElements[this->Piece] = nullptr;
  for (i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "Cells") == 0) && (eNested->GetNumberOfNestedElements() > 0))
    {
      this->CellElements[this->Piece] = eNested;
    }
  }

  if (!this->CellElements[this->Piece])
  {
    vtkErrorMacro("A piece is missing its Cells element.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  this->StartCell += this->NumberOfCells[this->Piece];
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data and point specifications (we read cell
  // specifications here).
  vtkIdType superclassPieceSize =
    ((this->NumberOfPointArrays + 1) * this->GetNumberOfPointsInPiece(this->Piece) +
      this->NumberOfCellArrays * this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from cell/face data arrays.
  // Three of them are for standard vtkUnstructuredGrid cell specification:
  // connectivities, offsets and types. Two optional arrays are for face
  // specification of polyhedron cells: faces and face offsets.
  // Note: We don't know exactly the array size of cell connectivities and
  // faces until we actually read the file. The following progress computation
  // assumes that each array cost the same time to read.
  vtkIdType totalPieceSize = superclassPieceSize + 5 * this->GetNumberOfCellsInPiece(this->Piece);
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.  The cell
  // specification reads two arrays, and then the cell types array is
  // one more.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[5] = { 0, float(superclassPieceSize) / totalPieceSize,
    ((float(superclassPieceSize) + 2 * this->GetNumberOfCellsInPiece(this->Piece)) /
      totalPieceSize),
    ((float(superclassPieceSize) + 3 * this->GetNumberOfCellsInPiece(this->Piece)) /
      totalPieceSize),
    1 };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Set the range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the Cells.
  vtkXMLDataElement* eCells = this->CellElements[this->Piece];
  if (!eCells)
  {
    vtkErrorMacro("Cannot find cell arrays in piece " << this->Piece);
    return 0;
  }

  //  int needToRead = this->CellsNeedToReadTimeStep(eNested,
  //    this->CellsTimeStep, this->CellsOffset);
  //  if( needToRead )
  {
    // Read the array.
    if (!this->ReadCellArray(
          this->NumberOfCells[this->Piece], this->TotalNumberOfCells, eCells, output->GetCells()))
    {
      return 0;
    }
  }

  // Set the range of progress for the cell types.
  this->SetProgressRange(progressRange, 2, fractions);

  // Read the corresponding cell types.
  vtkIdType numberOfCells = this->NumberOfCells[this->Piece];
  if (numberOfCells > 0)
  {
    vtkXMLDataElement* eTypes = this->FindDataArrayWithName(eCells, "types");
    if (!eTypes)
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be found.");
      return 0;
    }
    vtkAbstractArray* ac2 = this->CreateArray(eTypes);
    vtkDataArray* c2 = vtkArrayDownCast<vtkDataArray>(ac2);
    if (!c2 || (c2->GetNumberOfComponents() != 1))
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be created"
        << " with one component.");
      if (ac2)
      {
        ac2->Delete();
      }
      return 0;
    }
    c2->SetNumberOfTuples(numberOfCells);
    if (!this->ReadArrayValues(eTypes, 0, c2, 0, numberOfCells))
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array is not long enough.");
      return 0;
    }
    vtkUnsignedCharArray* cellTypes = this->ConvertToUnsignedCharArray(c2);
    if (!cellTypes)
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be converted"
        << " to a vtkUnsignedCharArray.");
      return 0;
    }

    // Copy the cell type data.
    memcpy(output->GetCellTypesArray()->GetPointer(this->StartCell), cellTypes->GetPointer(0),
      numberOfCells);

    // Permute node numbering on higher order hexahedra for legacy files (see
    // https://gitlab.kitware.com/vtk/vtk/-/merge_requests/6678 )
    if (this->GetFileMajorVersion() < 2 ||
      (this->GetFileMajorVersion() == 2 && this->GetFileMinorVersion() < 1))
    {
      vtkUpdateCellsV8toV9(output);
    }

    cellTypes->Delete();
  }

  // Set the range of progress for the faces.
  this->SetProgressRange(progressRange, 3, fractions);

  //
  // Read face array. Used for polyhedron mesh support. First need to
  // check if faces and faceoffsets arrays are available in this piece.
  if (!this->FindDataArrayWithName(eCells, "faces") ||
    !this->FindDataArrayWithName(eCells, "faceoffsets"))
  {
    if (output->GetFaces())
    {
      // This piece doesn't have any polyhedron but other pieces that
      // we've already processed do so we need to add in face information
      // for cells that don't have that by marking -1.
      for (vtkIdType c = 0; c < numberOfCells; c++)
      {
        output->GetFaceLocations()->InsertNextValue(-1);
      }
    }
    return 1;
  }

  // By default vtkUnstructuredGrid does not contain face information, which is
  // only used by polyhedron cells. If so far no polyhedron cells have been
  // added, the pointers to the arrays will be nullptr. In this case, we need to
  // initialize the arrays and assign values to the previous non-polyhedron cells.
  if (!output->GetFaces() || !output->GetFaceLocations())
  {
    output->InitializeFacesRepresentation(this->StartCell);
  }

  // Read face arrays.
  if (!this->ReadFaceArray(
        this->NumberOfCells[this->Piece], eCells, output->GetFaces(), output->GetFaceLocations()))
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadArrayForCells(
  vtkXMLDataElement* da, vtkAbstractArray* outArray)
{
  vtkIdType startCell = this->StartCell;
  vtkIdType numCells = this->NumberOfCells[this->Piece];
  vtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadArrayValues(da, startCell * components, outArray, 0, numCells * components);
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}
