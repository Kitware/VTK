/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkStructuredGridReader.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
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
  this->vtkSource::SetNthOutput(0, vtkStructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  this->ExecuteExtent[0] = this->ExecuteExtent[2] = this->ExecuteExtent[4] = 0;
  this->ExecuteExtent[1] = this->ExecuteExtent[3] = this->ExecuteExtent[5] = 0;
}

vtkStructuredGridReader::~vtkStructuredGridReader()
{
}

//----------------------------------------------------------------------------
vtkStructuredGrid *vtkStructuredGridReader::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredGrid *)(this->Outputs[0]);
}


//----------------------------------------------------------------------------
void vtkStructuredGridReader::SetOutput(vtkStructuredGrid *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

// We just need to read the dimensions
void vtkStructuredGridReader::ExecuteInformation()
{
  char line[256];
  int done=0;
  vtkStructuredGrid *output = this->GetOutput();
  
  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return;
    }

  // Read structured grid specific stuff
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->CloseVTKFile ();
      return;
      } 

    if ( strncmp(this->LowerCase(line),"structured_grid",15) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return;
      }

    // Read keyword and dimensions
    //
    while (!done)
      {
      if (!this->ReadString(line))
        {
        break;
        }

      if ( ! strncmp(this->LowerCase(line),"dimensions",10) )
        {
        int ext[6];
        if (!(this->Read(ext+1) && 
              this->Read(ext+3) && 
              this->Read(ext+5)))
          {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->CloseVTKFile ();
          return;
          }
        // read dimensions, change to extent;
        ext[0] = ext[2] = ext[4] = 0;
        --ext[1];
        --ext[3];
        --ext[5];
        output->SetWholeExtent(ext);
        // That is all we wanted !!!!!!!!!!!!!!!
        this->CloseVTKFile();
        return;
        }
      }
    }

  vtkErrorMacro("Could not read dimensions");
  this->CloseVTKFile ();
}

void vtkStructuredGridReader::Execute()
{
  int numPts=0, npts, numCells=0, ncells;
  char line[256];
  int dimsRead=0;
  int done=0;
  vtkStructuredGrid *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading vtk structured grid file...");

  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return;
    }

  // Read structured grid specific stuff
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->CloseVTKFile ();
      return;
      } 

    if ( strncmp(this->LowerCase(line),"structured_grid",15) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return;
      }

    // Read keyword and number of points
    //
    while (!done)
      {
      if (!this->ReadString(line))
        {
        break;
        }

      if (! strncmp(this->LowerCase(line), "field", 5))
        {
        vtkFieldData* fd = this->ReadFieldData();
        output->SetFieldData(fd);
        fd->Delete(); // ?
        }
      else if ( ! strncmp(line, "dimensions",10) )
        {
        int dim[3];
        if (!(this->Read(dim) && 
              this->Read(dim+1) && 
              this->Read(dim+2)))
          {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->CloseVTKFile ();
          return;
          }

        numPts = dim[0] * dim[1] * dim[2];
        output->SetDimensions(dim);
        numCells = output->GetNumberOfCells();
        dimsRead = 1;
        }

      else if ( ! strncmp(line,"blanking",8) )
        {
        if (!this->Read(&npts))
          {
          vtkErrorMacro(<<"Error reading blanking!");
          this->CloseVTKFile ();
          return;
          }

        if (!this->ReadString(line)) 
          {
          vtkErrorMacro(<<"Cannot read blank type!" );
          this->CloseVTKFile ();
          return;
          }

        vtkUnsignedCharArray *data = vtkUnsignedCharArray::SafeDownCast(
                                        this->ReadArray(line, numPts, 1));

        if ( data != NULL )
          {
          output->BlankingOn();
          output->SetPointVisibility(data);
          data->Delete();
          }
        }

      else if ( ! strncmp(line,"points",6) )
        {
        if (!this->Read(&npts))
          {
          vtkErrorMacro(<<"Error reading points!");
          this->CloseVTKFile ();
          return;
          }

        this->ReadPoints(output, npts);
        }

      else if ( ! strncmp(line, "cell_data", 9) )
        {
        if (!this->Read(&ncells))
          {
          vtkErrorMacro(<<"Cannot read cell data!");
          this->CloseVTKFile ();
          return;
          }
        
        if ( ncells != numCells )
          {
          vtkErrorMacro(<<"Number of cells don't match!");
          this->CloseVTKFile ();
          return;
          }

        this->ReadCellData(output, ncells);
        break; //out of this loop
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if (!this->Read(&numPts))
          {
          vtkErrorMacro(<<"Cannot read point data!");
          this->CloseVTKFile ();
          return;
          }
        
        if ( npts != numPts )
          {
          vtkErrorMacro(<<"Number of points don't match!");
          this->CloseVTKFile ();
          return;
          }

        this->ReadPointData(output, npts);
        break; //out of this loop
        }

      else
        {
        vtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->CloseVTKFile ();
        return;
        }
      }

      if ( !dimsRead ) vtkWarningMacro(<<"No dimensions read.");
      if ( !output->GetPoints() ) vtkWarningMacro(<<"No points read.");
    }

  else if ( !strncmp(line, "cell_data", 9) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Read(&ncells))
      {
      vtkErrorMacro(<<"Cannot read cell data!");
      this->CloseVTKFile ();
      return;
      }
    this->ReadCellData(output, ncells);
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Read(&npts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->CloseVTKFile ();
      return;
      }
    this->ReadPointData(output, npts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }
    this->CloseVTKFile ();
}

void vtkStructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataReader::PrintSelf(os,indent);
}
