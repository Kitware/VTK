/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.cxx
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
#include "vtkPolyDataReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPolyDataReader* vtkPolyDataReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyDataReader");
  if(ret)
    {
    return (vtkPolyDataReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyDataReader;
}




vtkPolyDataReader::vtkPolyDataReader()
{
  this->Reader = vtkDataReader::New();
  this->Reader->SetSource(this);
}

vtkPolyDataReader::~vtkPolyDataReader()
{
  this->Reader->Delete();
  this->Reader = NULL;
}

unsigned long int vtkPolyDataReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader->GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Specify file name of vtk polygonal data file to read.
void vtkPolyDataReader::SetFileName(const char *name) 
{
  this->Reader->SetFileName(name);
}
char *vtkPolyDataReader::GetFileName() 
{
  return this->Reader->GetFileName();
}

// Get the type of file (ASCII or BINARY)
int vtkPolyDataReader::GetFileType() 
{
  return this->Reader->GetFileType();
}

// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkPolyDataReader::SetScalarsName(char *name) 
{
  this->Reader->SetScalarsName(name);
}
char *vtkPolyDataReader::GetScalarsName() 
{
  return this->Reader->GetScalarsName();
}

// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkPolyDataReader::SetVectorsName(char *name) 
{
  this->Reader->SetVectorsName(name);
}
char *vtkPolyDataReader::GetVectorsName() 
{
  return this->Reader->GetVectorsName();
}

// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkPolyDataReader::SetTensorsName(char *name) 
{
  this->Reader->SetTensorsName(name);
}
char *vtkPolyDataReader::GetTensorsName() 
{
  return this->Reader->GetTensorsName();
}

// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkPolyDataReader::SetNormalsName(char *name) 
{
  this->Reader->SetNormalsName(name);
}
char *vtkPolyDataReader::GetNormalsName() 
{
  return this->Reader->GetNormalsName();
}

// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkPolyDataReader::SetTCoordsName(char *name) 
{
  this->Reader->SetTCoordsName(name);
}
char *vtkPolyDataReader::GetTCoordsName() 
{
  return this->Reader->GetTCoordsName();
}

// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkPolyDataReader::SetLookupTableName(char *name) 
{
  this->Reader->SetLookupTableName(name);
}
char *vtkPolyDataReader::GetLookupTableName() 
{
  return this->Reader->GetLookupTableName();
}

// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkPolyDataReader::SetFieldDataName(char *name) 
{
  this->Reader->SetFieldDataName(name);
}
char *vtkPolyDataReader::GetFieldDataName() 
{
  return this->Reader->GetFieldDataName();
}

void vtkPolyDataReader::Execute()
{
  int numPts=0;
  char line[256];
  int npts, size, ncells;
  int done=0;
  vtkPolyData *output = this->GetOutput();

  vtkDebugMacro(<<"Reading vtk polygonal data...");

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
//
// Read polygonal data specific stuff
//
  if (!this->Reader->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->Reader->CloseVTKFile ();
    return;
    }

  if ( !strncmp(this->Reader->LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if (!this->Reader->ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->Reader->CloseVTKFile ();
      return;
      } 

    if ( strncmp(this->Reader->LowerCase(line),"polydata",8) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader->CloseVTKFile ();
      return;
      }
//
// Might find points, vertices, lines, polygons, or triangle strips
//
    while (!done)
      {
      if (!this->Reader->ReadString(line))
	{
	break;
	}

      if (! strncmp(this->Reader->LowerCase(line), "field", 5))
	{
	vtkFieldData* fd = this->Reader->ReadFieldData();
	output->SetFieldData(fd);
	fd->Delete(); // ?
	}
      else if ( ! strncmp(line, "points",6) )
        {
        if (!this->Reader->Read(&numPts))
          {
          vtkErrorMacro(<<"Cannot read number of points!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadPoints(output, numPts);
        }

      else if ( ! strncmp(line,"vertices",8) )
        {
        vtkCellArray *verts = vtkCellArray::New();
        if (!(this->Reader->Read(&ncells) && this->Reader->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read vertices!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCells(size, verts->WritePointer(ncells,size));
        output->SetVerts(verts);
        verts->Delete();
        vtkDebugMacro(<<"Read " << ncells << " vertices");
        }

      else if ( ! strncmp(line,"lines",5) )
        {
        vtkCellArray *lines = vtkCellArray::New();
        if (!(this->Reader->Read(&ncells) && this->Reader->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read lines!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCells(size, lines->WritePointer(ncells,size));
        output->SetLines(lines);
        lines->Delete();
        vtkDebugMacro(<<"Read " << ncells << " lines");
        }

      else if ( ! strncmp(line,"polygons",8) )
        {
        vtkCellArray *polys = vtkCellArray::New();
        if (!(this->Reader->Read(&ncells) && this->Reader->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read polygons!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCells(size, polys->WritePointer(ncells,size));
        output->SetPolys(polys);
        polys->Delete();
        vtkDebugMacro(<<"Read " << ncells << " polygons");
        }

      else if ( ! strncmp(line,"triangle_strips",15) )
        {
        vtkCellArray *tris = vtkCellArray::New();
        if (!(this->Reader->Read(&ncells) && this->Reader->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read triangle strips!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCells(size, tris->WritePointer(ncells,size));
        output->SetStrips(tris);
        tris->Delete();
        vtkDebugMacro(<<"Read " << ncells << " triangle strips");
        }

      else if ( ! strncmp(line, "cell_data", 9) )
        {
        if (!this->Reader->Read(&ncells))
          {
          vtkErrorMacro(<<"Cannot read cell data!");
          this->Reader->CloseVTKFile ();
          return;
          }
        
        if ( ncells != output->GetNumberOfCells() )
          {
          vtkErrorMacro(<<"Number of cells don't match number data values!");
          return;
          }

        this->Reader->ReadCellData(output, ncells);
        break; //out of this loop
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if (!this->Reader->Read(&npts))
          {
          vtkErrorMacro(<<"Cannot read point data!");
          this->Reader->CloseVTKFile ();
          return;
          }
        
        if ( npts != numPts )
          {
          vtkErrorMacro(<<"Number of points don't match number data values!");
          return;
          }

        this->Reader->ReadPointData(output, npts);
        break; //out of this loop
        }

      else
        {
        vtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->Reader->CloseVTKFile ();
        return;
        }
      }

      if ( ! output->GetPoints() ) vtkWarningMacro(<<"No points read!");
      if ( !(output->GetVerts() || output->GetLines() || 
      output->GetPolys() || output->GetStrips()) ) 
        vtkWarningMacro(<<"No topology read!");
    }

  else if ( !strncmp(line, "cell_data", 9) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader->Read(&ncells))
      {
      vtkErrorMacro(<<"Cannot read cell data!");
      this->Reader->CloseVTKFile ();
      return;
      }

    this->Reader->ReadCellData(output, ncells);
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader->Read(&numPts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->Reader->CloseVTKFile ();
      return;
      }

    this->Reader->ReadPointData(output, numPts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }
  this->Reader->CloseVTKFile ();
}

static int recursing = 0;
void vtkPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  // the reader ivar's source will be this reader. we must do this to prevent infinite printing
  if (!recursing)
    { 
    vtkPolyDataSource::PrintSelf(os,indent);
    recursing = 1;
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  recursing = 0;
}
