/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridReader.cxx
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
#include "vtkRectilinearGridReader.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkRectilinearGridReader, "1.27");
vtkStandardNewMacro(vtkRectilinearGridReader);

//----------------------------------------------------------------------------
vtkRectilinearGridReader::vtkRectilinearGridReader()
{
  this->vtkSource::SetNthOutput(0,vtkRectilinearGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkRectilinearGridReader::~vtkRectilinearGridReader()
{
}

//----------------------------------------------------------------------------
vtkRectilinearGrid *vtkRectilinearGridReader::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkRectilinearGrid *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridReader::SetOutput(vtkRectilinearGrid *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
void vtkRectilinearGridReader::ExecuteInformation()
{
  char line[256];
  vtkRectilinearGrid *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading vtk rectilinear grid file info...");

  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return;
    }

  // Read rectilinear grid specific stuff
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

    if ( strncmp(this->LowerCase(line),"rectilinear_grid",16) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return;
      }

    // Read keyword and number of points
    //
    while (1)
      {
      if (!this->ReadString(line))
        {
        break;
        }

      if ( ! strncmp(this->LowerCase(line),"dimensions",10) )
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

        output->SetWholeExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
        // We got what we want.  Now return.
        this->CloseVTKFile ();
        return;
        }
      }
    }

  this->CloseVTKFile ();
}

//----------------------------------------------------------------------------

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
    this->DebugOn();
    }
  else
    {
    this->DebugOff();
    }

  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
      return;
    }

  // Read rectilinear grid specific stuff
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

    if ( strncmp(this->LowerCase(line),"rectilinear_grid",16) )
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
      else if ( ! strncmp(line,"dimensions",10) )
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

      else if ( ! strncmp(line,"x_coordinate",12) )
        {
        if (!this->Read(&ncoords))
          {
          vtkErrorMacro(<<"Error reading x coordinates!");
          this->CloseVTKFile ();
          return;
          }

        this->ReadCoordinates(output, 0, ncoords);
        }

      else if ( ! strncmp(line,"y_coordinate",12) )
        {
        if (!this->Read(&ncoords))
          {
          vtkErrorMacro(<<"Error reading y coordinates!");
          this->CloseVTKFile ();
          return;
          }

        this->ReadCoordinates(output, 1, ncoords);
        }

      else if ( ! strncmp(line,"z_coordinate",12) )
        {
        if (!this->Read(&ncoords))
          {
          vtkErrorMacro(<<"Error reading z coordinates!");
          this->CloseVTKFile ();
          return;
          }

        this->ReadCoordinates(output, 2, ncoords);
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
        if (!this->Read(&npts))
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
      if ( !output->GetXCoordinates() || 
      output->GetXCoordinates()->GetNumberOfTuples() < 1 )
        {
        vtkWarningMacro(<<"No x coordinatess read.");
        }
      if ( !output->GetYCoordinates() || 
      output->GetYCoordinates()->GetNumberOfTuples() < 1 )
        {
        vtkWarningMacro(<<"No y coordinates read.");
        }
      if ( !output->GetZCoordinates() || 
      output->GetZCoordinates()->GetNumberOfTuples() < 1 )
        {
        vtkWarningMacro(<<"No z coordinates read.");
        }
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

//----------------------------------------------------------------------------
void vtkRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
