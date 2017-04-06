/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridReader.h"

#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkRectilinearGridReader);

//----------------------------------------------------------------------------
vtkRectilinearGridReader::vtkRectilinearGridReader()
{
  vtkRectilinearGrid *output = vtkRectilinearGrid::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkRectilinearGridReader::~vtkRectilinearGridReader()
{
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGridReader::GetOutput(int idx)
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkRectilinearGridReader::SetOutput(vtkRectilinearGrid *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int vtkRectilinearGridReader::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  return this->ReadMetaData(outInfo);
}

//----------------------------------------------------------------------------
int vtkRectilinearGridReader::ReadMetaData(vtkInformation *outInfo)
{
  char line[256];
  bool dimsRead=0;

  vtkDebugMacro(<<"Reading vtk rectilinear grid file info...");

  if (!this->OpenVTKFile() || !this->ReadHeader())
  {
    return 1;
  }

  // Read rectilinear grid specific stuff
  //
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 1;
  }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
  {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->CloseVTKFile ();
      return 1;
    }

    if ( strncmp(this->LowerCase(line),"rectilinear_grid",16) )
    {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return 1;
    }

    // Read keyword and number of points
    //
    while (1)
    {
      if (!this->ReadString(line))
      {
        break;
      }

      if ( ! strncmp(this->LowerCase(line), "dimensions",10) && !dimsRead )
      {
        int dim[3];
        if (!(this->Read(dim) &&
              this->Read(dim+1) &&
              this->Read(dim+2)))
        {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->CloseVTKFile ();
          this->SetErrorCode( vtkErrorCode::FileFormatError );
          return 1;
        }
        outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                     0,dim[0]-1,0,dim[1]-1,0,dim[2]-1);
        dimsRead = 1;
      }

      else if ( ! strncmp(line, "extent", 6) && !dimsRead )
      {
        int extent[6];
        if (!(this->Read(extent) &&
              this->Read(extent+1) &&
              this->Read(extent+2) &&
              this->Read(extent+3) &&
              this->Read(extent+4) &&
              this->Read(extent+5)))
        {
          vtkErrorMacro(<<"Error reading extent!");
          this->CloseVTKFile ();
          this->SetErrorCode( vtkErrorCode::FileFormatError );
          return 1;
        }

        outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                     extent[0], extent[1], extent[2], extent[3],
                     extent[4], extent[5]);

        dimsRead = 1;
      }
    }
  }

  if ( !dimsRead)
  {
    vtkWarningMacro(<<"Could not read dimensions or extents from the file.");
  }
  this->CloseVTKFile ();
  return 1;
}

//----------------------------------------------------------------------------
int vtkRectilinearGridReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkIdType numPts=0, npts, ncoords, numCells=0, ncells;
  char line[256];
  int dimsRead=0;
  vtkRectilinearGrid *output = vtkRectilinearGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
    return 1;
  }

  // Read rectilinear grid specific stuff
  //
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 1;
  }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
  {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->CloseVTKFile ();
      return 1;
    }

    if ( strncmp(this->LowerCase(line),"rectilinear_grid",16) )
    {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return 1;
    }

    // Read keyword and number of points
    //
    while (true)
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

      else if ( ! strncmp(line, "extent", 6) && !dimsRead )
      {
        int extent[6];
        if (!(this->Read(extent) &&
              this->Read(extent+1) &&
              this->Read(extent+2) &&
              this->Read(extent+3) &&
              this->Read(extent+4) &&
              this->Read(extent+5)))
        {
          vtkErrorMacro(<<"Error reading extent!");
          this->CloseVTKFile ();
          this->SetErrorCode( vtkErrorCode::FileFormatError );
          return 1;
        }

        output->SetExtent(extent);
        numPts = output->GetNumberOfPoints();
        numCells = output->GetNumberOfCells();
        dimsRead = 1;
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
          this->SetErrorCode( vtkErrorCode::FileFormatError );
          return 1;
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
          return 1;
        }

        this->ReadCoordinates(output, 0, ncoords);
      }

      else if ( ! strncmp(line,"y_coordinate",12) )
      {
        if (!this->Read(&ncoords))
        {
          vtkErrorMacro(<<"Error reading y coordinates!");
          this->CloseVTKFile ();
          return 1;
        }

        this->ReadCoordinates(output, 1, ncoords);
      }

      else if ( ! strncmp(line,"z_coordinate",12) )
      {
        if (!this->Read(&ncoords))
        {
          vtkErrorMacro(<<"Error reading z coordinates!");
          this->CloseVTKFile ();
          return 1;
        }

        this->ReadCoordinates(output, 2, ncoords);
      }

      else if ( ! strncmp(line, "cell_data", 9) )
      {
        if (!this->Read(&ncells))
        {
          vtkErrorMacro(<<"Cannot read cell data!");
          this->CloseVTKFile ();
          return 1;
        }

        if ( ncells != numCells )
        {
          vtkErrorMacro(<<"Number of cells don't match!");
          this->CloseVTKFile ();
          return 1;
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
          return 1;
        }

        if ( npts != numPts )
        {
          vtkErrorMacro(<<"Number of points don't match!");
          this->CloseVTKFile ();
          return 1;
        }

        this->ReadPointData(output, npts);
        break; //out of this loop
      }

      else
      {
        vtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->CloseVTKFile ();
        return 1;
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
      return 1;
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
      return 1;
    }
    this->ReadPointData(output, npts);
  }

  else
  {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
  }

  this->CloseVTKFile ();

  return 1;
}

//----------------------------------------------------------------------------
int vtkRectilinearGridReader::FillOutputPortInformation(int,
                                                        vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
