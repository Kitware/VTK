/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPDataObjectReader.h"

#include "vtkCallbackCommand.h"
#include "vtkXMLDataElement.h"

#include <cassert>
#include <sstream>

//----------------------------------------------------------------------------
vtkXMLPDataObjectReader::vtkXMLPDataObjectReader()
{
  this->NumberOfPieces = 0;

  this->PieceElements = nullptr;
  this->CanReadPieceFlag = nullptr;

  this->PathName = nullptr;

  // Setup a callback for the internal serial readers to report
  // progress.
  this->PieceProgressObserver = vtkCallbackCommand::New();
  this->PieceProgressObserver->SetCallback(&vtkXMLPDataObjectReader::PieceProgressCallbackFunction);
  this->PieceProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkXMLPDataObjectReader::~vtkXMLPDataObjectReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  delete[] this->PathName;
  this->PieceProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
}

//----------------------------------------------------------------------------
char* vtkXMLPDataObjectReader::CreatePieceFileName(const char* fileName)
{
  assert(fileName);

  std::ostringstream fileNameStream;

  // only prepend the path if the given file name is not
  // absolute (i.e. doesn't start with '/')
  if (this->PathName && fileName[0] != '/')
  {
    fileNameStream << this->PathName;
  }
  fileNameStream << fileName;

  size_t len = fileNameStream.str().length();
  char* buffer = new char[len + 1];
  strncpy(buffer, fileNameStream.str().c_str(), len);
  buffer[len] = '\0';

  return buffer;
}
//----------------------------------------------------------------------------
void vtkXMLPDataObjectReader::SplitFileName()
{
  if (!this->FileName)
  {
    vtkErrorMacro(<< "Need to specify a filename");
    return;
  }

  // Pull the PathName component out of the FileName.
  size_t length = strlen(this->FileName);
  char* fileName = new char[length + 1];
  strcpy(fileName, this->FileName);
  char* begin = fileName;
  char* end = fileName + length;
  char* s;

#if defined(_WIN32)
  // Convert to UNIX-style slashes.
  for (s = begin; s != end; ++s)
  {
    if (*s == '\\')
    {
      *s = '/';
    }
  }
#endif

  // Extract the path name up to the last '/'.
  delete[] this->PathName;
  this->PathName = nullptr;
  char* rbegin = end - 1;
  char* rend = begin - 1;
  for (s = rbegin; s != rend; --s)
  {
    if (*s == '/')
    {
      break;
    }
  }
  if (s >= begin)
  {
    length = (s - begin) + 1;
    this->PathName = new char[length + 1];
    strncpy(this->PathName, this->FileName, length);
    this->PathName[length] = '\0';
  }

  // Cleanup temporary name.
  delete[] fileName;
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectReader::PieceProgressCallbackFunction(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkXMLPDataObjectReader*>(clientdata)->PieceProgressCallback();
}

//----------------------------------------------------------------------------
int vtkXMLPDataObjectReader::ReadXMLInformation()
{
  // First setup the filename components.
  this->SplitFileName();

  // Now proceed with reading the information.
  return this->Superclass::ReadXMLInformation();
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectReader::SetupPieces(int numPieces)
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }

  this->NumberOfPieces = numPieces;
  this->PieceElements = new vtkXMLDataElement*[this->NumberOfPieces];
  this->CanReadPieceFlag = new int[this->NumberOfPieces];

  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    this->PieceElements[i] = nullptr;
    this->CanReadPieceFlag[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectReader::DestroyPieces()
{
  delete[] this->PieceElements;
  delete[] this->CanReadPieceFlag;
  this->PieceElements = nullptr;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
int vtkXMLPDataObjectReader::ReadPiece(vtkXMLDataElement* ePiece, int index)
{
  this->Piece = index;
  return this->ReadPiece(ePiece);
}
