/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightMasterServerReader.cxx
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
#include "vtkEnSightMasterServerReader.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"

vtkCxxRevisionMacro(vtkEnSightMasterServerReader, "1.1");
vtkStandardNewMacro(vtkEnSightMasterServerReader);

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
    vtkErrorMacro("No pices to read");
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
  //cout << "Updating piece: " << this->CurrentPiece 
  //     << " which is in the file: " 
  //     << this->PieceCaseFileName << endl;
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
  char line[1024];

  if (!this->CaseFileName)
    {
    vtkErrorMacro("A case file name must be specified.");
    return VTK_ERROR;
    }
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, this->CaseFileName);
    vtkDebugMacro("full path to case file: " << line);
    }
  else
    {
    strcpy(line, this->CaseFileName);
    }

  //cout << "ExecuteInformation" << endl;
  this->IS = new ifstream(line, ios::in);;
  if ( this->IS->fail() )
    {
    //cout << "Cannot open file: " << line << endl;
    return VTK_ERROR;
    }
  char result[1024];

  int servers       = 0;
  int numberservers = 0;
  int currentserver = 0;

  while ( this->ReadNextDataLine(result) )
    {
    if ( vtkString::Equals(result, "FORMAT") )
      {
      // Format
      }
    else if ( vtkString::Equals(result, "SERVERS") )
      {
      servers = 1;
      }
    else if ( servers && vtkString::StartsWith(result, "number of servers:") )
      {
      sscanf(result, "number of servers: %i", &numberservers);
      if ( !numberservers )
        {
        vtkErrorMacro("The case file is corrupted");
        break;
        }
      }
    else if ( servers && vtkString::StartsWith(result, "casefile:") )
      {
      if ( currentserver == piece )
        {
        char filename[1024] = "";
        sscanf(result, "casefile: %s", filename);
        if ( filename[0] == 0 )
          {
          //cout << "Problem parsing file name from: " << result << endl;
          return VTK_ERROR;
          }
        this->SetPieceCaseFileName(filename);
        }
      else
        {
        currentserver ++;
        }
      }
    //cout << "Read: " << result << endl;
    }
  if ( piece == -1 && currentserver != numberservers )
    {
    //cout << "Number of servers (" << numberservers 
    //<< ") is not equal to the actual number of servers (" 
    //<< currentserver << ")" << endl;
    return VTK_ERROR;
    }
  //cout << "Number of servers is: " << numberservers << endl;
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
