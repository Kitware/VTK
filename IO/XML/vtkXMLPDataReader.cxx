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
#include "vtkPointData.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataReader.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>
#include <sstream>


//----------------------------------------------------------------------------
vtkXMLPDataReader::vtkXMLPDataReader()
{
  this->GhostLevel = 0;

  this->NumberOfPieces = 0;

  this->PieceElements = 0;
  this->PieceReaders = 0;
  this->CanReadPieceFlag = 0;

  this->PathName = 0;

  // Setup a callback for the internal serial readers to report
  // progress.
  this->PieceProgressObserver = vtkCallbackCommand::New();
  this->PieceProgressObserver->SetCallback(&vtkXMLPDataReader::PieceProgressCallbackFunction);
  this->PieceProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkXMLPDataReader::~vtkXMLPDataReader()
{
  if(this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  delete [] this->PathName;
  this->PieceProgressObserver->Delete();
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
  if(!reader)
  {
    return 0;
  }
  if(reader->GetNumberOfOutputPorts() < 1)
  {
    return 0;
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
  if(ePointData)
  {
    for(i=0;i < ePointData->GetNumberOfNestedElements();++i)
    {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if(this->PointDataArrayIsEnabled(eNested))
      {
        vtkAbstractArray* array = this->CreateArray(eNested);
        if(array)
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

  if(eCellData)
  {
    for(i = 0; i < eCellData->GetNumberOfNestedElements(); i++)
    {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if(this->CellDataArrayIsEnabled(eNested))
      {
        vtkAbstractArray* array = this->CreateArray(eNested);
        if(array)
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
int vtkXMLPDataReader::ReadXMLInformation()
{
  // First setup the filename components.
  this->SplitFileName();

  // Now proceed with reading the information.
  return this->Superclass::ReadXMLInformation();
}


//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLPDataReader::SetupOutputInformation(vtkInformation *outInfo)
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
  vtkInformationVector *infoVector = NULL;
  if (!this->SetFieldDataInfo(this->PPointDataElement,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, this->GetNumberOfPoints(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    outInfo->Set(vtkDataObject::POINT_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }

  // now the Cell data
  infoVector = NULL;
  if (!this->SetFieldDataInfo(this->PCellDataElement,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, this->GetNumberOfCells(), infoVector))
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
void vtkXMLPDataReader::CopyOutputInformation(vtkInformation *outInfo,
                                              int port)
{
  vtkInformation *localInfo =
    this->GetExecutive()->GetOutputInformation( port );
  if ( localInfo->Has(vtkDataObject::POINT_DATA_VECTOR()) )
  {
    outInfo->CopyEntry( localInfo, vtkDataObject::POINT_DATA_VECTOR() );
  }
  if ( localInfo->Has(vtkDataObject::CELL_DATA_VECTOR()) )
  {
    outInfo->CopyEntry( localInfo, vtkDataObject::CELL_DATA_VECTOR() );
  }
}


//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }
  // Read information about the data.
  if(!ePrimary->GetScalarAttribute("GhostLevel", this->GhostLevel))
  {
    this->GhostLevel = 0;
  }

  // Read information about the pieces.
  this->PPointDataElement = 0;
  this->PCellDataElement = 0;
  int i;
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  for(i=0;i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "Piece") == 0)
    {
      ++numPieces;
    }
    else if(strcmp(eNested->GetName(), "PPointData") == 0)
    {
      this->PPointDataElement = eNested;
    }
    else if(strcmp(eNested->GetName(), "PCellData") == 0)
    {
      this->PCellDataElement = eNested;
    }
  }
  this->SetupPieces(numPieces);
  int piece = 0;
  for(i=0;i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "Piece") == 0)
    {
      if(!this->ReadPiece(eNested, piece++))
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
  if(this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->NumberOfPieces = numPieces;
  this->PieceElements = new vtkXMLDataElement*[this->NumberOfPieces];
  this->PieceReaders = new vtkXMLDataReader*[this->NumberOfPieces];
  this->CanReadPieceFlag = new int[this->NumberOfPieces];
  int i;
  for(i=0;i < this->NumberOfPieces;++i)
  {
    this->PieceElements[i] = 0;
    this->PieceReaders[i] = 0;
    this->CanReadPieceFlag[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::DestroyPieces()
{
  int i;
  for(i=0;i < this->NumberOfPieces;++i)
  {
    if(this->PieceReaders[i])
    {
      this->PieceReaders[i]->RemoveObserver(this->PieceProgressObserver);
      this->PieceReaders[i]->Delete();
    }
  }
  delete [] this->PieceElements;
  delete [] this->CanReadPieceFlag;
  delete [] this->PieceReaders;
  this->PieceElements = 0;
  this->PieceReaders = 0;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPiece(vtkXMLDataElement* ePiece, int index)
{
  this->Piece = index;
  return this->ReadPiece(ePiece);
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  this->PieceElements[this->Piece] = ePiece;

  const char* fileName = ePiece->GetAttribute("Source");
  if(!fileName)
  {
    vtkErrorMacro("Piece " << this->Piece << " has no Source attribute.");
    return 0;
  }

  // The file name is relative to the summary file.  Convert it to
  // something we can use.
  char *pieceFileName = this->CreatePieceFileName(fileName);

  vtkXMLDataReader* reader = this->CreatePieceReader();
  this->PieceReaders[this->Piece] = reader;
  this->PieceReaders[this->Piece]->AddObserver(vtkCommand::ProgressEvent,
                                               this->PieceProgressObserver);
  reader->SetFileName(pieceFileName);

  delete [] pieceFileName;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPieceData(int index)
{
  this->Piece = index;

  // We need data, make sure the piece can be read.
  if(!this->CanReadPiece(this->Piece))
  {
    vtkErrorMacro("File for piece " << this->Piece << " cannot be read.");
    return 0;
  }

  // Actually read the data.
  this->PieceReaders[this->Piece]->SetAbortExecute(0);
  vtkDataArraySelection* pds =
    this->PieceReaders[this->Piece]->GetPointDataArraySelection();
  vtkDataArraySelection* cds =
    this->PieceReaders[this->Piece]->GetCellDataArraySelection();
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
    int i;
    for (i = 0; i < input->GetFieldData()->GetNumberOfArrays(); i++)
    {
      output->GetFieldData()->AddArray( input->GetFieldData()->GetArray(i) );
    }
  }

  // Copy point data and cell data for this piece.
  int i;
  for(i=0;i < output->GetPointData()->GetNumberOfArrays();++i)
  {
    this->CopyArrayForPoints(input->GetPointData()->GetArray(i),
                             output->GetPointData()->GetArray(i));
  }
  for(i=0;i < output->GetCellData()->GetNumberOfArrays();++i)
  {
    this->CopyArrayForCells(input->GetCellData()->GetArray(i),
                            output->GetCellData()->GetArray(i));
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::CanReadPiece(int index)
{
  // If necessary, test whether the piece can be read.
  vtkXMLDataReader* reader = this->PieceReaders[index];
  if(reader && !this->CanReadPieceFlag[index])
  {
    if(reader->CanReadFile(reader->GetFileName()))
    {
      // We can read the piece.  Save result to avoid later repeat of
      // test.
      this->CanReadPieceFlag[index] = 1;
    }
    else
    {
      // We cannot read the piece.  Destroy the reader to avoid later
      // repeat of test.
      this->PieceReaders[index] = 0;
      reader->Delete();
    }
  }

  return (this->PieceReaders[index]? 1:0);
}

//----------------------------------------------------------------------------
char* vtkXMLPDataReader::CreatePieceFileName(const char* fileName)
{
  assert(fileName);

  std::ostringstream fn_with_warning_C4701;

  // only prepend the path if the given file name is not
  // absolute (i.e. doesn't start with '/')
  if(this->PathName && fileName[0] != '/')
  {
    fn_with_warning_C4701 << this->PathName;
  }
  fn_with_warning_C4701 << fileName;

  size_t len = fn_with_warning_C4701.str().length();
  char *buffer = new char[len + 1];
  strncpy(buffer, fn_with_warning_C4701.str().c_str(), len);
  buffer[len] = '\0';

  return buffer;
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SplitFileName()
{
  if(!this->FileName)
  {
    vtkErrorMacro( << "Need to specify a filename" );
    return;
  }

  // Pull the PathName component out of the FileName.
  size_t length = strlen(this->FileName);
  char* fileName = new char[length+1];
  strcpy(fileName, this->FileName);
  char* begin = fileName;
  char* end = fileName + length;
  char* s;

#if defined(_WIN32)
  // Convert to UNIX-style slashes.
  for(s=begin;s != end;++s) { if(*s == '\\') { *s = '/'; } }
#endif

  // Extract the path name up to the last '/'.
  delete [] this->PathName;
  this->PathName = 0;
  char* rbegin = end-1;
  char* rend = begin-1;
  for(s=rbegin;s != rend;--s)
  {
    if(*s == '/')
    {
      break;
    }
  }
  if(s >= begin)
  {
    length = (s-begin)+1;
    this->PathName = new char[length+1];
    strncpy(this->PathName, this->FileName, length);
    this->PathName[length] = '\0';
  }

  // Cleanup temporary name.
  delete [] fileName;
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::PieceProgressCallbackFunction(vtkObject*, unsigned long,
                                                      void* clientdata, void*)
{
  reinterpret_cast<vtkXMLPDataReader*>(clientdata)->PieceProgressCallback();
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::PieceProgressCallback()
{
  float width = this->ProgressRange[1]-this->ProgressRange[0];
  float pieceProgress = this->PieceReaders[this->Piece]->GetProgress();
  float progress = this->ProgressRange[0] + pieceProgress*width;
  this->UpdateProgressDiscrete(progress);
  if(this->AbortExecute)
  {
    this->PieceReaders[this->Piece]->SetAbortExecute(1);
  }
}
