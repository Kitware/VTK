/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridReader.cxx
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
#include "vtkStructuredGridReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStructuredGridReader* vtkStructuredGridReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredGridReader");
  if(ret)
    {
    return (vtkStructuredGridReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredGridReader;
}




vtkStructuredGridReader::vtkStructuredGridReader()
{
  this->Reader = vtkDataReader::New();
  this->Reader->SetSource(this);
}

vtkStructuredGridReader::~vtkStructuredGridReader()
{
  this->Reader->Delete();
}

unsigned long int vtkStructuredGridReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader->GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Specify file name of vtk polygonal data file to read.
void vtkStructuredGridReader::SetFileName(char *name) 
{
  this->Reader->SetFileName(name);
}
char *vtkStructuredGridReader::GetFileName() 
{
  return this->Reader->GetFileName();
}

// Get the type of file (VTK_ASCII or VTK_BINARY)
int vtkStructuredGridReader::GetFileType() 
{
  return this->Reader->GetFileType();
}

// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkStructuredGridReader::SetScalarsName(char *name) 
{
  this->Reader->SetScalarsName(name);
}
char *vtkStructuredGridReader::GetScalarsName() 
{
  return this->Reader->GetScalarsName();
}

// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkStructuredGridReader::SetVectorsName(char *name) 
{
  this->Reader->SetVectorsName(name);
}
char *vtkStructuredGridReader::GetVectorsName() 
{
  return this->Reader->GetVectorsName();
}

// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkStructuredGridReader::SetTensorsName(char *name) 
{
  this->Reader->SetTensorsName(name);
}
char *vtkStructuredGridReader::GetTensorsName() 
{
  return this->Reader->GetTensorsName();
}

// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkStructuredGridReader::SetNormalsName(char *name) 
{
  this->Reader->SetNormalsName(name);
}
char *vtkStructuredGridReader::GetNormalsName() 
{
  return this->Reader->GetNormalsName();
}

// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkStructuredGridReader::SetTCoordsName(char *name) 
{
  this->Reader->SetTCoordsName(name);
}
char *vtkStructuredGridReader::GetTCoordsName() 
{
  return this->Reader->GetTCoordsName();
}

// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkStructuredGridReader::SetLookupTableName(char *name) 
{
  this->Reader->SetLookupTableName(name);
}
char *vtkStructuredGridReader::GetLookupTableName() 
{
  return this->Reader->GetLookupTableName();
}

// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkStructuredGridReader::SetFieldDataName(char *name) 
{
  this->Reader->SetFieldDataName(name);
}
char *vtkStructuredGridReader::GetFieldDataName() 
{
  return this->Reader->GetFieldDataName();
}

// We just need to read the dimensions
void vtkStructuredGridReader::ExecuteInformation()
{
  char line[256];
  int done=0;
  vtkStructuredGrid *output = this->GetOutput();
  
  if (!this->Reader->OpenVTKFile() || !this->Reader->ReadHeader())
    {
    return;
    }

  //
  // Read structured grid specific stuff
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

    if ( strncmp(this->Reader->LowerCase(line),"structured_grid",15) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader->CloseVTKFile ();
      return;
      }
    //
    // Read keyword and dimensions
    //
    while (!done)
      {
      if (!this->Reader->ReadString(line))
	{
	break;
	}

      if ( ! strncmp(this->Reader->LowerCase(line),"dimensions",10) )
        {
        int ext[6];
        if (!(this->Reader->Read(ext+1) && 
	      this->Reader->Read(ext+3) && 
	      this->Reader->Read(ext+5)))
          {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->Reader->CloseVTKFile ();
          return;
          }
	// read dimensions, change to extent;
	ext[0] = ext[2] = ext[4] = 0;
	--ext[1];
	--ext[3];
	--ext[5];
        output->SetWholeExtent(ext);
	// That is all we wanted !!!!!!!!!!!!!!!
	this->Reader->CloseVTKFile();
	return;
        }
      }
    }

  vtkErrorMacro("Could not read dimensions");
  this->Reader->CloseVTKFile ();
}

void vtkStructuredGridReader::Execute()
{
  int numPts=0, npts, numCells=0, ncells;
  char line[256];
  int dimsRead=0;
  int done=0;
  vtkStructuredGrid *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading vtk structured grid file...");
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
  // Read structured grid specific stuff
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

    if ( strncmp(this->Reader->LowerCase(line),"structured_grid",15) )
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

      else if ( ! strncmp(line,"points",6) )
        {
        if (!this->Reader->Read(&npts))
          {
          vtkErrorMacro(<<"Error reading points!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadPoints(output, npts);
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

        this->Reader->ReadCellData(output, npts);
        break; //out of this loop
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if (!this->Reader->Read(&numPts))
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
      if ( !output->GetPoints() ) vtkWarningMacro(<<"No points read.");
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
void vtkStructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  // the reader ivar's source will be this reader. 
  // We must do this to prevent infinite printing
  if (!recursing)
    { 
    vtkStructuredGridSource::PrintSelf(os,indent);
    recursing = 1;
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  recursing = 0;
}
