/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldDataReader.cxx
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
#include "vtkFieldDataReader.h"

vtkFieldDataReader::vtkFieldDataReader()
{
  this->Reader.SetSource(this);
}

unsigned long int vtkFieldDataReader::GetMTime()
{
  unsigned long dtime = this->vtkFieldDataSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vtk field data file to read.
void vtkFieldDataReader::SetFileName(char *name) 
{
  this->Reader.SetFileName(name);
}
char *vtkFieldDataReader::GetFileName() 
{
  return this->Reader.GetFileName();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vtkFieldDataReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

void vtkFieldDataReader::Execute()
{
  int numPts=0;
  char line[256];
  int npts, size, ncells;
  vtkFieldData *output=(vtkFieldData *)this->Output;

  vtkDebugMacro(<<"Reading vtk field data...");

  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if ( !(this->Reader.OpenVTKFile()) || !this->Reader.ReadHeader())
      return;
  //
  // Read field specific stuff
  //
  if (!this->Reader.ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->Reader.CloseVTKFile ();
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"field",(unsigned long)5) )
    {
    vtkFieldData *f = this->Reader.ReadFieldData(0);
    this->Output->GetFieldData()->ShallowCopy(*f);
    f->Delete();
    }

  else if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
    vtkErrorMacro(<<"Field reader cannot read datasets");
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyord: " << line);
    }
  
  this->Reader.CloseVTKFile();
}

// Description:
// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkFieldDataReader::SetFieldDataName(char *name) 
{
  this->Reader.SetFieldDataName(name);
}
char *vtkFieldDataReader::GetFieldDataName() 
{
  return this->Reader.GetFieldDataName();
}

void vtkFieldDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFieldDataSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->Reader.GetFileName() ? this->Reader.GetFileName() : "(none)") << "\n";

  if ( this->Reader.GetFileType() == VTK_BINARY )
    os << indent << "File Type: BINARY\n";
  else
    os << indent << "File Type: ASCII\n";

  if ( this->Reader.GetFieldDataName() )
    os << indent << "Field Data Name: " << this->Reader.GetFieldDataName() << "\n";
  else
    os << indent << "Field Data Name: (None)\n";
}
