/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectReader.cxx
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
#include "vtkDataObjectReader.h"

#include "vtkObjectFactory.h"
#include "vtkFieldData.h"

vtkCxxRevisionMacro(vtkDataObjectReader, "1.16");
vtkStandardNewMacro(vtkDataObjectReader);

vtkDataObjectReader::vtkDataObjectReader()
{
  this->SetOutput(vtkDataObject::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

vtkDataObjectReader::~vtkDataObjectReader()
{
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectReader::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkDataObjectReader::SetOutput(vtkDataObject *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

void vtkDataObjectReader::Execute()
{
  char line[256];
  vtkFieldData *field=NULL;

  vtkDebugMacro(<<"Reading vtk field data...");

  if ( !(this->OpenVTKFile()) || !this->ReadHeader())
    {
    return;
    }

  // Read field data until end-of-file
  //
  while (this->ReadString(line) && !field )
    {
    if ( !strncmp(this->LowerCase(line),"field",(unsigned long)5) )
      {
      field = this->ReadFieldData(); //reads named field (or first found)
      if ( field != NULL )
        {
        this->GetOutput()->SetFieldData(field);
        field->Delete();
        }
      }

    else if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
      {
      vtkErrorMacro(<<"Field reader cannot read datasets");
      this->CloseVTKFile();
      return;
      }

    else 
      {
      vtkErrorMacro(<< "Unrecognized keyword: " << line);
      this->CloseVTKFile();
      return;
      }
    }
  //while field not read

  this->CloseVTKFile();
}

void vtkDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
