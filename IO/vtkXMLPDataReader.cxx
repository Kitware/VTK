/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPDataReader.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataReader.h"

vtkCxxRevisionMacro(vtkXMLPDataReader, "1.3");

//----------------------------------------------------------------------------
vtkXMLPDataReader::vtkXMLPDataReader()
{
  this->GhostLevel = 0;
  
  this->NumberOfPieces = 0;
  
  this->PieceElements = 0;
  this->PieceReaders = 0;
  this->CanReadPieceFlag = 0;
  
  this->PathName = 0;  
}

//----------------------------------------------------------------------------
vtkXMLPDataReader::~vtkXMLPDataReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }  
  if(this->PathName) { delete [] this->PathName; }
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
  if(!reader) { return 0; }
  if(reader->GetNumberOfOutputs() < 1) { return 0; }
  return static_cast<vtkDataSet*>(reader->GetOutputs()[0]);
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();
  int i;
  
  // Setup the output arrays.
  vtkXMLDataElement* ePointData = this->PPointDataElement;
  vtkXMLDataElement* eCellData = this->PCellDataElement;
  vtkPointData* pointData = this->GetOutputAsDataSet()->GetPointData();
  vtkCellData* cellData = this->GetOutputAsDataSet()->GetCellData();  
  
  // Setup the point and cell data arrays without allocation.
  this->SetDataArraySelections(ePointData, this->PointDataArraySelection);
  if(ePointData)
    {
    for(i=0;i < ePointData->GetNumberOfNestedElements();++i)
      {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if(this->PointDataArrayIsEnabled(eNested))
        {
        vtkDataArray* array = this->CreateDataArray(eNested);
        pointData->AddArray(array);
        array->Delete();
        }
      }
    }
  this->SetDataArraySelections(eCellData, this->CellDataArraySelection);
  if(eCellData)
    {
    for(i=0;i < eCellData->GetNumberOfNestedElements();++i)
      {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if(this->CellDataArrayIsEnabled(eNested))
        {
        vtkDataArray* array = this->CreateDataArray(eNested);
        cellData->AddArray(array);
        array->Delete();
        }
      }
    }
  
  // Setup attribute indices for the point data and cell data.
  this->ReadAttributeIndices(ePointData, pointData);
  this->ReadAttributeIndices(eCellData, cellData);
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  vtkDataSet* output = this->GetOutputAsDataSet();
  vtkXMLDataElement* ePointData = this->PPointDataElement;
  vtkXMLDataElement* eCellData = this->PCellDataElement;
  vtkPointData* pointData = output->GetPointData();
  vtkCellData* cellData = output->GetCellData();
  
  // Get the size of the output arrays.
  unsigned long pointTuples = this->GetNumberOfPoints();
  unsigned long cellTuples = this->GetNumberOfCells();  
  
  // Allocate data in the arrays.
  int i;
  if(ePointData)
    {
    int a=0;
    for(i=0;i < ePointData->GetNumberOfNestedElements();++i)
      {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if(this->PointDataArrayIsEnabled(eNested))
        {
        pointData->GetArray(a++)->SetNumberOfTuples(pointTuples);
        }
      }
    }
  if(eCellData)
    {
    int a=0;
    for(i=0;i < eCellData->GetNumberOfNestedElements();++i)
      {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if(this->CellDataArrayIsEnabled(eNested))
        {
        cellData->GetArray(a++)->SetNumberOfTuples(cellTuples);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::ReadXMLInformation()
{
  // First setup the filename components.
  this->SplitFileName();
  
  // Now proceed with reading the information.
  this->Superclass::ReadXMLInformation();
}

//----------------------------------------------------------------------------
int vtkXMLPDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
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
    if(strcmp(eNested->GetName(), "Piece") == 0) { ++numPieces; }
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
      if(!this->ReadPiece(eNested, piece++)) { return 0; }
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SetupPieces(int numPieces)
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
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
    if(this->PieceReaders[i]) { this->PieceReaders[i]->Delete(); }
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
  vtkDataSet* output = this->GetOutputAsDataSet();
  //vtkXMLDataElement* ePointData = this->PPointDataElement;
  //vtkXMLDataElement* eCellData = this->PCellDataElement;
  
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
  ostrstream fn;
  if(this->PathName) { fn << this->PathName; }
  fn << fileName << ends;
  return fn.str();
}

//----------------------------------------------------------------------------
void vtkXMLPDataReader::SplitFileName()
{
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
  if(this->PathName) { delete [] this->PathName; this->PathName = 0; }
  char* rbegin = end-1;
  char* rend = begin-1;
  for(s=rbegin;s != rend;--s) { if(*s == '/') { break; } }
  if(s >= begin)
    {
    length = (s-begin)+1;
    this->PathName = new char[length+1];
    strncpy(this->PathName, this->FileName, length);
    this->PathName[length] = '\0';
    begin = s+1;
    }
  
  // Cleanup temporary name.
  delete [] fileName;
}
