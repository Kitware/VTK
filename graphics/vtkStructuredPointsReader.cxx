/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsReader.cxx
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
#include "vtkStructuredPointsReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStructuredPointsReader* vtkStructuredPointsReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredPointsReader");
  if(ret)
    {
    return (vtkStructuredPointsReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredPointsReader;
}




vtkStructuredPointsReader::vtkStructuredPointsReader()
{
  this->Reader = vtkDataReader::New();
  this->Reader->SetSource(this);
}

vtkStructuredPointsReader::~vtkStructuredPointsReader()
{
  this->Reader->Delete();
  this->Reader = NULL;
}

unsigned long int vtkStructuredPointsReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader->GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Specify file name of vtk structured points data file to read.
void vtkStructuredPointsReader::SetFileName(char *name) 
{
  this->Reader->SetFileName(name);
}
char *vtkStructuredPointsReader::GetFileName() 
{
  return this->Reader->GetFileName();
}

// Get the type of file (VTK_ASCII or VTK_BINARY).
int vtkStructuredPointsReader::GetFileType() 
{
  return this->Reader->GetFileType();
}

// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkStructuredPointsReader::SetScalarsName(char *name) 
{
  this->Reader->SetScalarsName(name);
}
char *vtkStructuredPointsReader::GetScalarsName() 
{
  return this->Reader->GetScalarsName();
}

// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkStructuredPointsReader::SetVectorsName(char *name) 
{
  this->Reader->SetVectorsName(name);
}
char *vtkStructuredPointsReader::GetVectorsName() 
{
  return this->Reader->GetVectorsName();
}

// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkStructuredPointsReader::SetTensorsName(char *name) 
{
  this->Reader->SetTensorsName(name);
}
char *vtkStructuredPointsReader::GetTensorsName() 
{
  return this->Reader->GetTensorsName();
}

// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkStructuredPointsReader::SetNormalsName(char *name) 
{
  this->Reader->SetNormalsName(name);
}
char *vtkStructuredPointsReader::GetNormalsName() 
{
  return this->Reader->GetNormalsName();
}

// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkStructuredPointsReader::SetTCoordsName(char *name) 
{
  this->Reader->SetTCoordsName(name);
}
char *vtkStructuredPointsReader::GetTCoordsName() 
{
  return this->Reader->GetTCoordsName();
}

// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkStructuredPointsReader::SetLookupTableName(char *name) 
{
  this->Reader->SetLookupTableName(name);
}
char *vtkStructuredPointsReader::GetLookupTableName() 
{
  return this->Reader->GetLookupTableName();
}

// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkStructuredPointsReader::SetFieldDataName(char *name) 
{
  this->Reader->SetFieldDataName(name);
}
char *vtkStructuredPointsReader::GetFieldDataName() 
{
  return this->Reader->GetFieldDataName();
}

void vtkStructuredPointsReader::Execute()
{
  int numPts=0, numCells=0;
  char line[256];
  int npts, ncells;
  int dimsRead=0, arRead=0, originRead=0;
  int done=0;
  vtkStructuredPoints *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading vtk structured points file...");
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
// Read structured points specific stuff
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

    if ( strncmp(this->Reader->LowerCase(line),"structured_points",17) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader->CloseVTKFile ();
      return;
      }
//
// Read keyword and number of points
//
    numPts = output->GetNumberOfPoints(); // get default
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

      else if ( !strncmp(line,"aspect_ratio",12) || !strncmp(line,"spacing",7) )
        {
        float ar[3];
        if (!(this->Reader->Read(ar) && 
	      this->Reader->Read(ar+1) && 
	      this->Reader->Read(ar+2)))
          {
          vtkErrorMacro(<<"Error reading spacing!");
          this->Reader->CloseVTKFile ();
          return;
          }

        output->SetSpacing(ar);
        arRead = 1;
        }

      else if ( ! strncmp(line,"origin",6) )
        {
        float origin[3];
        if (!(this->Reader->Read(origin) && 
	      this->Reader->Read(origin+1) && 
	      this->Reader->Read(origin+2)))
          {
          vtkErrorMacro(<<"Error reading origin!");
          this->Reader->CloseVTKFile ();
          return;
          }

        output->SetOrigin(origin);
        originRead = 1;
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
          vtkErrorMacro(<<"Number of cells don't match data values!");
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
          vtkErrorMacro(<<"Number of points don't match data values!");
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
      if ( !arRead ) vtkWarningMacro(<<"No spacing read.");
      if ( !originRead ) vtkWarningMacro(<<"No origin read.");
    }

  else if ( !strncmp(line,"cell_data",9) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader->Read(&ncells))
      {
      vtkErrorMacro(<<"Cannot read cell data!");
      this->Reader->CloseVTKFile ();
      return;
      }
    this->Reader->ReadCellData(output, numCells);
    }

  else if ( !strncmp(line,"point_data",10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader->Read(&npts))
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
void vtkStructuredPointsReader::PrintSelf(ostream& os, vtkIndent indent)
{
  // the reader ivar's source will be this reader. we must do this to prevent infinite printing
  if (!recursing)
    { 
    vtkStructuredPointsSource::PrintSelf(os,indent);
    recursing = 1;
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  recursing = 0;
}
