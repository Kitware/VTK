/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectWriter.cxx
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
#include "vtkDataObjectWriter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDataObjectWriter, "1.13");
vtkStandardNewMacro(vtkDataObjectWriter);

vtkDataObjectWriter::vtkDataObjectWriter()
{
  this->Writer = vtkDataWriter::New();
}

vtkDataObjectWriter::~vtkDataObjectWriter()
{
  this->Writer->Delete();
}

//----------------------------------------------------------------------------
void vtkDataObjectWriter::SetInput(vtkDataObject *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Inputs[0]);
}

// Write FieldData data to file
void vtkDataObjectWriter::WriteData()
{
  ostream *fp;
  vtkFieldData *f=this->GetInput()->GetFieldData();

  vtkDebugMacro(<<"Writing vtk FieldData data...");

  if ( !(fp=this->Writer->OpenVTKFile()) || !this->Writer->WriteHeader(fp) )
    {
    return;
    }
  //
  // Write FieldData data specific stuff
  //
  this->Writer->WriteFieldData(fp, f);
  
  this->Writer->CloseVTKFile(fp);  
}

void vtkDataObjectWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "File Name: " 
     << (this->Writer->GetFileName() ? this->Writer->GetFileName() : "(none)") << "\n";

  if ( this->Writer->GetFileType() == VTK_BINARY )
    {
    os << indent << "File Type: BINARY\n";
    }
  else
    {
    os << indent << "File Type: ASCII\n";
    }

  if ( this->Writer->GetHeader() )
    {
    os << indent << "Header: " << this->Writer->GetHeader() << "\n";
    }
  else
    {
    os << indent << "Header: (None)\n";
    }

  if ( this->Writer->GetFieldDataName() )
    {
    os << indent << "Field Data Name: " << this->Writer->GetFieldDataName() << "\n";
    }
  else
    {
    os << indent << "Field Data Name: (None)\n";
    }

}

