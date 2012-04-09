/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectReader.h"

#include "vtkObjectFactory.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFieldData.h"
#include "vtkDataObject.h"

vtkStandardNewMacro(vtkDataObjectReader);

vtkDataObjectReader::vtkDataObjectReader()
{
  vtkDataObject *output = vtkDataObject::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  output->ReleaseData();
  output->Delete();
}

vtkDataObjectReader::~vtkDataObjectReader()
{
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObjectReader::GetOutput(int port)
{
  return vtkDataObject::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkDataObjectReader::SetOutput(vtkDataObject *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

int vtkDataObjectReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  
  char line[256];
  vtkFieldData *field=NULL;

  vtkDebugMacro(<<"Reading vtk field data...");

  if ( !(this->OpenVTKFile()) || !this->ReadHeader())
    {
    return 1;
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
        output->SetFieldData(field);
        field->Delete();
        }
      }

    else if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
      {
      vtkErrorMacro(<<"Field reader cannot read datasets");
      this->CloseVTKFile();
      return 1;
      }

    else 
      {
      vtkErrorMacro(<< "Unrecognized keyword: " << line);
      this->CloseVTKFile();
      return 1;
      }
    }
  //while field not read

  this->CloseVTKFile();

  return 1;
}

int vtkDataObjectReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

void vtkDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
