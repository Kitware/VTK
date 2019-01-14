/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPDataReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataReader.h"

#include <cassert>
#include <sstream>

//----------------------------------------------------------------------------
vtkXMLPDataReader::vtkXMLPDataReader()
{
  this->GhostLevel = 0;
  this->PieceReaders = nullptr;
}

//----------------------------------------------------------------------------
vtkXMLPDataReader::~vtkXMLPDataReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLPDataReader::GetPieceInputAsDataSet(int piece)
{
  vtkXMLDataReader* reader = this->PieceReaders[piece];
  if (!reader)
  {
    return nullptr;
  }
  if (reader->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }
  return static_cast<vtkDataSet*>(reader->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Setup the output arrays.
  vtkXMLDataElement* ePointData = this->PPointDataElement;
  vtkXMLDataElement* eCellData = this->PCellDataElement;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  vtkPointData* pointData = output->GetPointData();
  vtkCellData* cellData = output->GetCellData();

  // Get the size of the output arrays.
  unsigned long pointTuples = this->GetNumberOfPoints();
  unsigned long cellTuples = this->GetNumberOfCells();

  // Allocate data in the arrays.
  int i;
  if (ePointData)
  {
    for (i = 0; i < ePointData->GetNumberOfNestedElements(); ++i)
    {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if (this->PointDataArrayIsEnabled(eNested))
      {
        vtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
        {
          array->SetNumberOfTuples(pointTuples);
          pointData->AddArray(array);
          array->Delete();
        }
        else
        {
          this->DataError = 1;
        }
      }
    }
  }

  if (eCellData)
  {
    for (i = 0; i < eCellData->GetNumberOfNestedElements(); i++)
    {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if (this->CellDataArrayIsEnabled(eNested))
      {
        vtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
        {
          array->SetNumberOfTuples(cellTuples);
          cellData->AddArray(array);
          array->Delete();
        }
        else
        {
          this->DataError = 1;
        }
      }
    }
  }

  // Setup attribute indices for the point data and cell data.
  this->ReadAttributeIndices(ePointData, pointData);
  this->ReadAttributeIndices(eCellData, cellData);
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLPDataReader::SetupOutputInformation(vtkInformation* outInfo)
{
  if (this->InformationError)
  {
    vtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
  }

  // Initialize DataArraySelections to anable all that are present
  this->SetDataArraySelections(this->PPointDataElement, this->PointDataArraySelection);
  this->SetDataArraySelections(this->PCellDataElement, this->CellDataArraySelection);

  // Setup the Field Information for PointData.  We only need the
  // information from one piece because all pieces have the same set of arrays.
  vtkInformationVector* infoVector = nullptr;
  if (!this->SetFieldDataInfo(this->PPointDataElement, vtkDataObject::FIELD_ASSOCIATION_POINTS,
        this->GetNumberOfPoints(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    outInfo->Set(vtkDataObject::POINT_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }

  // now the Cell data
  infoVector = nullptr;
  if (!this->SetFieldDataInfo(this->PCellDataElement, vtkDataObject::FIELD_ASSOCIATION_CELLS,
        this->GetNumberOfCells(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    outInfo->Set(vtkDataObject::CELL_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  vtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);
  if (localInfo->Has(vtkDataObject::POINT_DATA_VECTOR()))
  {
    outInfo->CopyEntry(localInfo, vtkDataObject::POINT_DATA_VECTOR());
  }
  if (localInfo->Has(vtkDataObject::CELL_DATA_VECTOR()))
  {
    outInfo->CopyEntry(localInfo, vtkDataObject::CELL_DATA_VECTOR());
  }
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }
  // Read information about the data.
  if (!ePrimary->GetScalarAttribute("GhostLevel", this->GhostLevel))
  {
    this->GhostLevel = 0;
  }

  // Read information about the pieces.
  this->PPointDataElement = nullptr;
  this->PCellDataElement = nullptr;
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      ++numPieces;
    }
    else if (strcmp(eNested->GetName(), "PPointData") == 0)
    {
      this->PPointDataElement = eNested;
    }
    else if (strcmp(eNested->GetName(), "PCellData") == 0)
    {
      this->PCellDataElement = eNested;
    }
    else if (strcmp(eNested->GetName(), "FieldData") == 0)
    {
      this->FieldDataElement = eNested;
    }
  }
  this->SetupPieces(numPieces);
  int piece = 0;
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      if (!this->ReadPiece(eNested, piece++))
      {
        return 0;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);

  this->PieceReaders = new vtkXMLDataReader*[this->NumberOfPieces];

  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    this->PieceReaders[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::DestroyPieces()
{
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->PieceReaders[i]->RemoveObserver(this->PieceProgressObserver);
      this->PieceReaders[i]->Delete();
    }
  }

  delete[] this->PieceReaders;
  this->PieceReaders = nullptr;

  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  this->PieceElements[this->Piece] = ePiece;

  const char* fileName = ePiece->GetAttribute("Source");
  if (!fileName)
  {
    vtkErrorMacro("Piece " << this->Piece << " has no Source attribute.");
    return 0;
  }

  // The file name is relative to the summary file.  Convert it to
  // something we can use.
  char* pieceFileName = this->CreatePieceFileName(fileName);

  vtkXMLDataReader* reader = this->CreatePieceReader();
  this->PieceReaders[this->Piece] = reader;
  this->PieceReaders[this->Piece]->AddObserver(
    vtkCommand::ProgressEvent, this->PieceProgressObserver);
  reader->SetFileName(pieceFileName);

  delete[] pieceFileName;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPieceData(int index)
{
  this->Piece = index;

  // We need data, make sure the piece can be read.
  if (!this->CanReadPiece(this->Piece))
  {
    vtkErrorMacro("File for piece " << this->Piece << " cannot be read.");
    return 0;
  }

  // Actually read the data.
  this->PieceReaders[this->Piece]->SetAbortExecute(0);
  vtkDataArraySelection* pds = this->PieceReaders[this->Piece]->GetPointDataArraySelection();
  vtkDataArraySelection* cds = this->PieceReaders[this->Piece]->GetCellDataArraySelection();
  pds->CopySelections(this->PointDataArraySelection);
  cds->CopySelections(this->CellDataArraySelection);
  return this->ReadPieceData();
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPieceData()
{
  vtkDataSet* input = this->GetPieceInputAsDataSet(this->Piece);
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());

  // copy any field data
  if (input->GetFieldData())
  {
    for (int i = 0; i < input->GetFieldData()->GetNumberOfArrays(); i++)
    {
      output->GetFieldData()->AddArray(input->GetFieldData()->GetAbstractArray(i));
    }
  }

  // Copy point data and cell data for this piece.
  for (int i = 0; i < output->GetPointData()->GetNumberOfArrays(); ++i)
  {
    this->CopyArrayForPoints(
      input->GetPointData()->GetArray(i), output->GetPointData()->GetArray(i));
  }
  for (int i = 0; i < output->GetCellData()->GetNumberOfArrays(); ++i)
  {
    this->CopyArrayForCells(input->GetCellData()->GetArray(i), output->GetCellData()->GetArray(i));
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::CanReadPiece(int index)
{
  // If necessary, test whether the piece can be read.
  vtkXMLDataReader* reader = this->PieceReaders[index];
  if (reader && !this->CanReadPieceFlag[index])
  {
    if (reader->CanReadFile(reader->GetFileName()))
    {
      // We can read the piece.  Save result to avoid later repeat of
      // test.
      this->CanReadPieceFlag[index] = 1;
    }
    else
    {
      // We cannot read the piece.  Destroy the reader to avoid later
      // repeat of test.
      this->PieceReaders[index] = nullptr;
      reader->Delete();
    }
  }

  return (this->PieceReaders[index] ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::PieceProgressCallback()
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float pieceProgress = this->PieceReaders[this->Piece]->GetProgress();
  float progress = this->ProgressRange[0] + pieceProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    this->PieceReaders[this->Piece]->SetAbortExecute(1);
  }
}
