/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridReader.cxx
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
#include "vtkRectilinearGridReader.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkRectilinearGridReader* vtkRectilinearGridReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRectilinearGridReader");
  if(ret)
    {
    return (vtkRectilinearGridReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRectilinearGridReader;
}




//----------------------------------------------------------------------------
vtkRectilinearGridReader::vtkRectilinearGridReader()
{
  this->Reader = vtkDataReader::New();
  this->Reader->SetSource(this);
}

//----------------------------------------------------------------------------
vtkRectilinearGridReader::~vtkRectilinearGridReader()
{
  this->Reader->Delete();
}

//----------------------------------------------------------------------------
unsigned long int vtkRectilinearGridReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader->GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

//----------------------------------------------------------------------------
// Specify file name of vtk polygonal data file to read.
void vtkRectilinearGridReader::SetFileName(char *name) 
{
  this->Reader->SetFileName(name);
}
//----------------------------------------------------------------------------
char *vtkRectilinearGridReader::GetFileName() 
{
  return this->Reader->GetFileName();
}

//----------------------------------------------------------------------------
// Get the type of file (VTK_ASCII or VTK_BINARY)
int vtkRectilinearGridReader::GetFileType() 
{
  return this->Reader->GetFileType();
}

//----------------------------------------------------------------------------
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkRectilinearGridReader::SetScalarsName(char *name) 
{
  this->Reader->SetScalarsName(name);
}
char *vtkRectilinearGridReader::GetScalarsName() 
{
  return this->Reader->GetScalarsName();
}

//----------------------------------------------------------------------------
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkRectilinearGridReader::SetVectorsName(char *name) 
{
  this->Reader->SetVectorsName(name);
}
char *vtkRectilinearGridReader::GetVectorsName() 
{
  return this->Reader->GetVectorsName();
}

//----------------------------------------------------------------------------
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkRectilinearGridReader::SetTensorsName(char *name) 
{
  this->Reader->SetTensorsName(name);
}
char *vtkRectilinearGridReader::GetTensorsName() 
{
  return this->Reader->GetTensorsName();
}

//----------------------------------------------------------------------------
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkRectilinearGridReader::SetNormalsName(char *name) 
{
  this->Reader->SetNormalsName(name);
}
//----------------------------------------------------------------------------
char *vtkRectilinearGridReader::GetNormalsName() 
{
  return this->Reader->GetNormalsName();
}

//----------------------------------------------------------------------------
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkRectilinearGridReader::SetTCoordsName(char *name) 
{
  this->Reader->SetTCoordsName(name);
}
//----------------------------------------------------------------------------
char *vtkRectilinearGridReader::GetTCoordsName() 
{
  return this->Reader->GetTCoordsName();
}

//----------------------------------------------------------------------------
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkRectilinearGridReader::SetLookupTableName(char *name) 
{
  this->Reader->SetLookupTableName(name);
}
//----------------------------------------------------------------------------
char *vtkRectilinearGridReader::GetLookupTableName() 
{
  return this->Reader->GetLookupTableName();
}

//----------------------------------------------------------------------------
// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkRectilinearGridReader::SetFieldDataName(char *name) 
{
  this->Reader->SetFieldDataName(name);
}
char *vtkRectilinearGridReader::GetFieldDataName() 
{
  return this->Reader->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkRectilinearGridReader::ExecuteInformation()
{
  char line[256];
  vtkRectilinearGrid *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading vtk rectilinear grid file info...");

  if (!this->Reader->OpenVTKFile() || !this->Reader->ReadHeader())
    {
    return;
    }
  //
  // Read rectilinear grid specific stuff
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

    if ( strncmp(this->Reader->LowerCase(line),"rectilinear_grid",16) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader->CloseVTKFile ();
      return;
      }
    //
    // Read keyword and number of points
    //
    while (1)
      {
      if (!this->Reader->ReadString(line))
	{
	break;
	}

      if ( ! strncmp(this->Reader->LowerCase(line),"dimensions",10) )
        {
        int dim[3];
        if (!(this->Reader->Read(dim) && 
	      this->Reader->Read(dim+1) && 
	      this->Reader->Read(dim+2)))
          {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->Reader->CloseVTKFile ();
          return;
          }

	output->SetWholeExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
	// We got what we want.  Now return.
	this->Reader->CloseVTKFile ();
	return;
        }
      }
    }

  this->Reader->CloseVTKFile ();
}

//----------------------------------------------------------------------------
// This used to be the execute method.
void vtkRectilinearGridReader::Execute()
{
  int numPts=0, npts, ncoords, numCells=0, ncells;
  char line[256];
  int dimsRead=0;
  int done=0;
  vtkRectilinearGrid *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading vtk rectilinear grid file...");
  if ( this->Debug )
    {
    this->Reader->DebugOn();
    }
  else
    {
    this->Reader->DebugOff();
    }

  if (!this->Reader->OpenVTKFile() || !this->Reader->ReadHeader())
    {
      return;
    }
  //
  // Read rectilinear grid specific stuff
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

    if ( strncmp(this->Reader->LowerCase(line),"rectilinear_grid",16) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader->CloseVTKFile ();
      return;
      }
    //
    // Read keyword and number of points
    //
    while (!done)
      {
      if (!this->Reader->ReadString(line))
	{
	break;
	}

      if ( ! strncmp(this->Reader->LowerCase(line),"dimensions",10) )
        {
        int dim[3];
        if (!(this->Reader->Read(dim) && 
	      this->Reader->Read(dim+1) && 
	      this->Reader->Read(dim+2)))
          {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->Reader->CloseVTKFile ();
          return;
          }

        numPts = dim[0] * dim[1] * dim[2];
        output->SetDimensions(dim);
	numCells = output->GetNumberOfCells();
        dimsRead = 1;
        }

      else if ( ! strncmp(line,"x_coordinate",12) )
        {
        if (!this->Reader->Read(&ncoords))
          {
          vtkErrorMacro(<<"Error reading x coordinates!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCoordinates(output, 0, ncoords);
        }

      else if ( ! strncmp(line,"y_coordinate",12) )
        {
        if (!this->Reader->Read(&ncoords))
          {
          vtkErrorMacro(<<"Error reading y coordinates!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCoordinates(output, 1, ncoords);
        }

      else if ( ! strncmp(line,"z_coordinate",12) )
        {
        if (!this->Reader->Read(&ncoords))
          {
          vtkErrorMacro(<<"Error reading z coordinates!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCoordinates(output, 2, ncoords);
        }

      else if ( ! strncmp(line, "cell_data", 9) )
        {
        if (!this->Reader->Read(&ncells))
          {
          vtkErrorMacro(<<"Cannot read cell data!");
          this->Reader->CloseVTKFile ();
          return;
          }
        
        if ( ncells != numCells )
          {
          vtkErrorMacro(<<"Number of cells don't match!");
          this->Reader->CloseVTKFile ();
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
          vtkErrorMacro(<<"Number of points don't match!");
          this->Reader->CloseVTKFile ();
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

      if ( !dimsRead ) vtkWarningMacro(<<"No dimensions read.");
      if ( !output->GetXCoordinates() || 
      output->GetXCoordinates()->GetNumberOfScalars() < 1 )
        {
        vtkWarningMacro(<<"No x coordinatess read.");
        }
      if ( !output->GetYCoordinates() || 
      output->GetYCoordinates()->GetNumberOfScalars() < 1 )
        {
        vtkWarningMacro(<<"No y coordinates read.");
        }
      if ( !output->GetZCoordinates() || 
      output->GetZCoordinates()->GetNumberOfScalars() < 1 )
        {
        vtkWarningMacro(<<"No z coordinates read.");
        }
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
    if (!this->Reader->Read(&npts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->Reader->CloseVTKFile ();
      return;
      }
    this->Reader->ReadPointData(output, npts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }

  this->Reader->CloseVTKFile ();
}

static int recursing = 0;
//----------------------------------------------------------------------------
void vtkRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  // the reader ivar's source will be this Reader. 
  // we must do this to prevent infinite printing
  if (!recursing)
    { 
    vtkRectilinearGridSource::PrintSelf(os,indent);
    recursing = 1;
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  recursing = 0;
}
