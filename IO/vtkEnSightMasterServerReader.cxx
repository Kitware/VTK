/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightMasterServerReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnSightMasterServerReader.h"

#include "vtkObjectFactory.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkEnSightMasterServerReader, "1.11");
vtkStandardNewMacro(vtkEnSightMasterServerReader);

static int vtkEnSightMasterServerReaderStartsWith(const char* str1, const char* str2)
{
  if ( !str1 || !str2 || strlen(str1) < strlen(str2) )
    {
    return 0;
    }
  return !strncmp(str1, str2, strlen(str2));  
}

//----------------------------------------------------------------------------
vtkEnSightMasterServerReader::vtkEnSightMasterServerReader()
{
  this->PieceCaseFileName = 0;
  this->MaxNumberOfPieces = 0;
  this->CurrentPiece      = -1;
}

//----------------------------------------------------------------------------
vtkEnSightMasterServerReader::~vtkEnSightMasterServerReader()
{
  this->SetPieceCaseFileName(0);
}

//----------------------------------------------------------------------------
void vtkEnSightMasterServerReader::Execute()
{
  if ( !this->MaxNumberOfPieces )
    {
    vtkErrorMacro("No pieces to read");
    return;
    }

  if ( this->CurrentPiece < 0 || 
       this->CurrentPiece >= this->MaxNumberOfPieces )
    {
    vtkErrorMacro("Current piece has to be set before reading the file");
    return;
    }
  if ( this->DetermineFileName(this->CurrentPiece) != VTK_OK )
    {
    vtkErrorMacro("Cannot update piece: " << this->CurrentPiece);
    return;
    }
  if ( !this->Reader )
    {
    this->Reader = vtkGenericEnSightReader::New();
    }
  this->Reader->SetCaseFileName(this->PieceCaseFileName);
  if ( !this->Reader->GetFilePath() )
    {
    this->Reader->SetFilePath( this->GetFilePath() );
    }
  this->Superclass::Execute();
}

//----------------------------------------------------------------------------
void vtkEnSightMasterServerReader::ExecuteInformation()
{  
  if ( this->DetermineFileName(-1) != VTK_OK )
    {
    vtkErrorMacro("Problem parsing the case file");
    return;
    }  
}

//----------------------------------------------------------------------------
int vtkEnSightMasterServerReader::DetermineFileName(int piece)
{
  if (!this->CaseFileName)
    {
    vtkErrorMacro("A case file name must be specified.");
    return VTK_ERROR;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
      {
      sfilename += "/";
      }
    sfilename += this->CaseFileName;
    vtkDebugMacro("full path to case file: " << sfilename.c_str());
    }
  else
    {
    sfilename = this->CaseFileName;
    }
  
  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
    }

  char result[1024];

  int servers       = 0;
  int numberservers = 0;
  int currentserver = 0;

  while ( this->ReadNextDataLine(result) )
    {
    if ( strcmp(result, "FORMAT") == 0 )
      {
      // Format
      }
    else if ( strcmp(result, "SERVERS") == 0 )
      {
      servers = 1;
      }
    else if ( servers && 
              vtkEnSightMasterServerReaderStartsWith(result, "number of servers:") )
      {
      sscanf(result, "number of servers: %i", &numberservers);
      if ( !numberservers )
        {
        vtkErrorMacro("The case file is corrupted");
        break;
        }
      }
    else if ( servers && 
              vtkEnSightMasterServerReaderStartsWith(result, "casefile:") )
      {
      if ( currentserver == piece )
        {
        char filename[1024] = "";
        sscanf(result, "casefile: %s", filename);
        if ( filename[0] == 0 )
          {
          vtkErrorMacro("Problem parsing file name from: " << result);
          return VTK_ERROR;
          }
        this->SetPieceCaseFileName(filename);
        break;
        }
      currentserver ++;
      }
    }
  if ( piece == -1 && currentserver != numberservers )
    {
    //cout << "Number of servers (" << numberservers 
    //<< ") is not equal to the actual number of servers (" 
    //<< currentserver << ")" << endl;
    return VTK_ERROR;
    }

  this->MaxNumberOfPieces = numberservers;
  delete this->IS;
  this->IS = 0;
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkEnSightMasterServerReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Current piece: " << this->CurrentPiece << endl;
  os << indent << "Piece Case File name: " 
     << (this->PieceCaseFileName?this->PieceCaseFileName:"<none>") << endl;
  os << indent << "Maximum numbe of pieces: " << this->MaxNumberOfPieces 
     << endl;
}
