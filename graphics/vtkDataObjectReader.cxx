/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkDataObjectReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkDataObjectReader* vtkDataObjectReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataObjectReader");
  if(ret)
    {
    return (vtkDataObjectReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataObjectReader;
}




vtkDataObjectReader::vtkDataObjectReader()
{
  this->Reader = vtkDataReader::New();
  this->Reader->SetSource(this);
}

vtkDataObjectReader::~vtkDataObjectReader()
{
  this->Reader->Delete();
}

unsigned long int vtkDataObjectReader::GetMTime()
{
  unsigned long dtime = this->vtkDataObjectSource::GetMTime();
  unsigned long rtime = this->Reader->GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Specify file name of vtk field data file to read.
void vtkDataObjectReader::SetFileName(const char *name) 
{
  this->Reader->SetFileName(name);
}
char *vtkDataObjectReader::GetFileName() 
{
  return this->Reader->GetFileName();
}

// Get the type of file (ASCII or BINARY)
int vtkDataObjectReader::GetFileType() 
{
  return this->Reader->GetFileType();
}

void vtkDataObjectReader::Execute()
{
  char line[256];
  vtkFieldData *field=NULL;

  vtkDebugMacro(<<"Reading vtk field data...");

  if ( this->Debug )
    {
    this->Reader->DebugOn();
    }
  else
    {
    this->Reader->DebugOff();
    }

  if ( !(this->Reader->OpenVTKFile()) || !this->Reader->ReadHeader())
    {
    return;
    }

  // Read field data until end-of-file
  //
  while (this->Reader->ReadString(line) && !field )
    {
    if ( !strncmp(this->Reader->LowerCase(line),"field",(unsigned long)5) )
      {
      field = this->Reader->ReadFieldData(); //reads named field (or first found)
      if ( field != NULL )
	{
	this->GetOutput()->SetFieldData(field);
	field->Delete();
	}
      }

    else if ( !strncmp(this->Reader->LowerCase(line),"dataset",(unsigned long)7) )
      {
      vtkErrorMacro(<<"Field reader cannot read datasets");
      this->Reader->CloseVTKFile();
      return;
      }

    else 
      {
      vtkErrorMacro(<< "Unrecognized keyword: " << line);
      this->Reader->CloseVTKFile();
      return;
      }
    }
  //while field not read

  this->Reader->CloseVTKFile();
}

// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkDataObjectReader::SetFieldDataName(char *name) 
{
  this->Reader->SetFieldDataName(name);
}
char *vtkDataObjectReader::GetFieldDataName() 
{
  return this->Reader->GetFieldDataName();
}

void vtkDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObjectSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->Reader->GetFileName() ? this->Reader->GetFileName() : "(none)") << "\n";

  if ( this->Reader->GetFileType() == VTK_BINARY )
    {
    os << indent << "File Type: BINARY\n";
    }
  else
    {
    os << indent << "File Type: ASCII\n";
    }

  if ( this->Reader->GetFieldDataName() )
    {
    os << indent << "Field Data Name: " << this->Reader->GetFieldDataName() << "\n";
    }
  else
    {
    os << indent << "Field Data Name: (None)\n";
    }
}
