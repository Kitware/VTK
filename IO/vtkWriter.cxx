/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWriter.cxx
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
#include "vtkWriter.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkWriter, "1.35");

// Construct with no start and end write methods or arguments.
vtkWriter::vtkWriter()
{
}

vtkWriter::~vtkWriter()
{
}

vtkDataObject *vtkWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return this->Inputs[0];
}


// Write data to output. Method executes subclasses WriteData() method, as 
// well as StartMethod() and EndMethod() methods.
void vtkWriter::Write()
{
  vtkDataObject *input = this->GetInput();

  // make sure input is available
  if ( !input )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  //
  input->Update();
  //
  if (input->GetUpdateTime() < this->WriteTime &&
      this->GetMTime() < this->WriteTime)
  {
    // we are up to date
    return;
  }
  //
  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  this->WriteData();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  if ( input->ShouldIReleaseData() )
    {
    input->ReleaseData();
    }
  this->WriteTime.Modified();
}

// Convenient alias for Write() method.
void vtkWriter::Update()
{
  this->Write();
}

void vtkWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

void vtkWriter::EncodeArrayName(char* resname, const char* name)
{
  if ( !name || !resname )
    {   
    return;
    }
  int cc = 0;
  ostrstream str;

  char buffer[10];

  while( name[cc] )
    {
    if ( name[cc] < '0' || 
         name[cc] > '9' && name[cc] < 'A' ||
         name[cc] > 'Z' && name[cc] < 'a' ||
         name[cc] > 'z' )
      {
      sprintf(buffer, "%2X", name[cc]);
      str << "%%" << buffer; // Two % because it goes through printf format
      }
    else
      {
      str << name[cc];
      }
    cc++;
    }
  str << ends;
  strcpy(resname, str.str());
  str.rdbuf()->freeze(0);
}
