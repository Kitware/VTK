/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDataObjectReader.h"

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
void vtkDataObjectReader::SetFileName(char *name) 
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
	this->Output->SetFieldData(field);
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
