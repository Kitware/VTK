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
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <assert.h>

vtkStandardNewMacro(vtkXMLUnstructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLUnstructuredGridReader::vtkXMLUnstructuredGridReader()
{
  this->CellElements = 0;
  this->NumberOfCells = 0;
  this->CellsTimeStep = -1;
  this->CellsOffset   = static_cast<unsigned long>(-1); // almost invalid state
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredGridReader::~vtkXMLUnstructuredGridReader()
{
  if(this->NumberOfPieces)
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
  return vtkUnstructuredGrid::SafeDownCast( this->GetOutputDataObject(idx) );
}

//----------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridReader::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::GetOutputUpdateExtent(int& piece,
                                                         int& numberOfPieces,
                                                         int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  int i;
  this->TotalNumberOfCells = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
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
  for(int i=0;i < numPieces; ++i)
    {
    this->CellElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::DestroyPieces()
{
  delete [] this->CellElements;
  delete [] this->NumberOfCells;
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

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
      this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  vtkCellArray* outCells = vtkCellArray::New();

  vtkIdTypeArray* locations = vtkIdTypeArray::New();
  locations->SetNumberOfTuples(this->GetNumberOfCells());

  output->SetCells(cellTypes, locations, outCells);

  locations->Delete();
  outCells->Delete();
  cellTypes->Delete();
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece))
    {
    return 0;
    }
  int i;

  if(!ePiece->GetScalarAttribute("NumberOfCells",
                                 this->NumberOfCells[this->Piece]))
    {
    vtkErrorMacro("Piece " << this->Piece
                  << " is missing its NumberOfCells attribute.");
    this->NumberOfCells[this->Piece] = 0;
    return 0;
    }

  // Find the Cells element in the piece.
  this->CellElements[this->Piece] = 0;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Cells") == 0)
       && (eNested->GetNumberOfNestedElements() > 0))
      {
      this->CellElements[this->Piece] = eNested;
      }
    }

  if(!this->CellElements[this->Piece])
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
    ((this->NumberOfPointArrays+1)*this->GetNumberOfPointsInPiece(this->Piece)+
     this->NumberOfCellArrays*this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point/cell specifications themselves (cell
  // specifications for vtkUnstructuredGrid take three data arrays).
  vtkIdType totalPieceSize =
    superclassPieceSize + 3*this->GetNumberOfCellsInPiece(this->Piece);
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.  The cell
  // specification reads two arrays, and then the cell types array is
  // one more.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4] =
    {
      0,
      float(superclassPieceSize) / totalPieceSize,
      ((float(superclassPieceSize) +
        2*this->GetNumberOfCellsInPiece(this->Piece)) / totalPieceSize),
      1
    };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if(!this->Superclass::ReadPieceData())
    {
    return 0;
    }

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
      this->GetCurrentOutput());

  // Save the start location where the new cell connectivity will be
  // appended.
  vtkIdType startLoc = 0;
  if(output->GetCells()->GetData())
    {
    startLoc = output->GetCells()->GetData()->GetNumberOfTuples();
    }

  // Set the range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the Cells.
  vtkXMLDataElement* eCells = this->CellElements[this->Piece];
  if(eCells)
    {
//    int needToRead = this->CellsNeedToReadTimeStep(eNested,
//      this->CellsTimeStep, this->CellsOffset);
//    if( needToRead )
      {
      // Read the array.
      if(!this->ReadCellArray(this->NumberOfCells[this->Piece],
                              this->TotalNumberOfCells,
                              eCells,
                              output->GetCells()))
        {
        return 0;
        }
      }
    }

  // Construct the cell locations.
  vtkIdTypeArray* locations = output->GetCellLocationsArray();
  vtkIdType* locs = locations->GetPointer(this->StartCell);
  vtkIdType* begin = output->GetCells()->GetData()->GetPointer(startLoc);
  vtkIdType* cur = begin;
  vtkIdType i;
  for(i=0; i < this->NumberOfCells[this->Piece]; ++i)
    {
    locs[i] = startLoc + cur - begin;
    cur += *cur + 1;
    }

  // Set the range of progress for the cell types.
  this->SetProgressRange(progressRange, 2, fractions);

  // Read the corresponding cell types.
  vtkIdType numberOfCells = this->NumberOfCells[this->Piece];
  vtkXMLDataElement* eTypes = this->FindDataArrayWithName(eCells, "types");
  if(!eTypes)
    {
    vtkErrorMacro("Cannot read cell types from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"types\" array could not be found.");
    return 0;
    }
  vtkAbstractArray* ac2 = this->CreateArray(eTypes);
  vtkDataArray* c2 = vtkDataArray::SafeDownCast(ac2);
  if(!c2 || (c2->GetNumberOfComponents() != 1))
    {
    vtkErrorMacro("Cannot read cell types from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"types\" array could not be created"
                  << " with one component.");
    if (ac2) { ac2->Delete(); }
    return 0;
    }
  c2->SetNumberOfTuples(numberOfCells);
  if(!this->ReadArrayValues(eTypes, 0, c2, 0, numberOfCells))
    {
    vtkErrorMacro("Cannot read cell types from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"types\" array is not long enough.");
    return 0;
    }
  vtkUnsignedCharArray* cellTypes = this->ConvertToUnsignedCharArray(c2);
  if(!cellTypes)
    {
    vtkErrorMacro("Cannot read cell types from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"types\" array could not be converted"
                  << " to a vtkUnsignedCharArray.");
    return 0;
    }

  // Copy the cell type data.
  memcpy(output->GetCellTypesArray()->GetPointer(this->StartCell),
         cellTypes->GetPointer(0), numberOfCells);

  cellTypes->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadArrayForCells(vtkXMLDataElement* da,
                                                    vtkAbstractArray* outArray)
{
  vtkIdType startCell = this->StartCell;
  vtkIdType numCells = this->NumberOfCells[this->Piece];
  vtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadArrayValues(da, startCell*components, outArray,
    0, numCells*components);
}


//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}


