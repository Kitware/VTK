/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataReader.h"

#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPolyDataReader);

//----------------------------------------------------------------------------
vtkPolyDataReader::vtkPolyDataReader()
{
  vtkPolyData *output = vtkPolyData::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkPolyDataReader::~vtkPolyDataReader()
{
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataReader::GetOutput(int idx)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkPolyDataReader::SetOutput(vtkPolyData *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}


//----------------------------------------------------------------------------
int vtkPolyDataReader::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return 1;
    }

  if (ghostLevel < 0)
    {
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int numPts=0;
  char line[256];
  int npts, size, ncells, i;
  int done=0;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int *tempArray;
  vtkIdType *idArray;

  vtkDebugMacro(<<"Reading vtk polygonal data...");

  if ( !(this->OpenVTKFile()) || !this->ReadHeader())
    {
    return 1;
    }
//
// Read polygonal data specific stuff
//
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 1;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if (!this->ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->CloseVTKFile ();
      return 1;
      }

    if ( strncmp(this->LowerCase(line),"polydata",8) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return 1;
      }
//
// Might find points, vertices, lines, polygons, or triangle strips
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
      else if ( ! strncmp(line, "points",6) )
        {
        if (!this->Read(&numPts))
          {
          vtkErrorMacro(<<"Cannot read number of points!");
          this->CloseVTKFile ();
          return 1;
          }

        this->ReadPoints(output, numPts);
        }

      else if ( ! strncmp(line,"vertices",8) )
        {
        vtkCellArray *verts = vtkCellArray::New();
        if (!(this->Read(&ncells) && this->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read vertices!");
          this->CloseVTKFile ();
          return 1;
          }

        tempArray = new int[size];
        idArray = verts->WritePointer(ncells, size);
        this->ReadCells(size, tempArray);
//        this->ReadCells(size, verts->WritePointer(ncells,size));
        for (i = 0; i < size; i++)
          {
          idArray[i] = tempArray[i];
          }
        output->SetVerts(verts);
        verts->Delete();
        delete [] tempArray;
        vtkDebugMacro(<<"Read " << ncells << " vertices");
        }

      else if ( ! strncmp(line,"lines",5) )
        {
        vtkCellArray *lines = vtkCellArray::New();
        if (!(this->Read(&ncells) && this->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read lines!");
          this->CloseVTKFile ();
          return 1;
          }
        tempArray = new int[size];
        idArray = lines->WritePointer(ncells, size);
        this->ReadCells(size, tempArray);
//        this->ReadCells(size, lines->WritePointer(ncells,size));
        for (i = 0; i < size; i++)
          {
          idArray[i] = tempArray[i];
          }

        output->SetLines(lines);
        lines->Delete();
        delete [] tempArray;
        vtkDebugMacro(<<"Read " << ncells << " lines");
        }

      else if ( ! strncmp(line,"polygons",8) )
        {
        vtkCellArray *polys = vtkCellArray::New();
        if (!(this->Read(&ncells) && this->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read polygons!");
          this->CloseVTKFile ();
          return 1;
          }

        tempArray = new int[size];
        idArray = polys->WritePointer(ncells, size);
        this->ReadCells(size, tempArray);
//        this->ReadCells(size, polys->WritePointer(ncells,size));
        for (i = 0; i < size; i++)
          {
          idArray[i] = tempArray[i];
          }
        output->SetPolys(polys);
        polys->Delete();
        delete [] tempArray;
        vtkDebugMacro(<<"Read " << ncells << " polygons");
        }

      else if ( ! strncmp(line,"triangle_strips",15) )
        {
        vtkCellArray *tris = vtkCellArray::New();
        if (!(this->Read(&ncells) && this->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read triangle strips!");
          this->CloseVTKFile ();
          return 1;
          }

        tempArray = new int[size];
        idArray = tris->WritePointer(ncells, size);
        this->ReadCells(size, tempArray);
//        this->ReadCells(size, tris->WritePointer(ncells,size));
        for (i = 0; i < size; i++)
          {
          idArray[i] = tempArray[i];
          }
        output->SetStrips(tris);
        tris->Delete();
        delete [] tempArray;
        vtkDebugMacro(<<"Read " << ncells << " triangle strips");
        }

      else if ( ! strncmp(line, "cell_data", 9) )
        {
        if (!this->Read(&ncells))
          {
          vtkErrorMacro(<<"Cannot read cell data!");
          this->CloseVTKFile ();
          return 1;
          }

        if ( ncells != output->GetNumberOfCells() )
          {
          vtkErrorMacro(<<"Number of cells don't match number data values!");
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
          vtkErrorMacro(<<"Number of points don't match number data values!");
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

      if ( ! output->GetPoints() ) vtkWarningMacro(<<"No points read!");
      if ( !(output->GetVerts() || output->GetLines() ||
      output->GetPolys() || output->GetStrips()) )
        vtkWarningMacro(<<"No topology read!");
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
    if (!this->Read(&numPts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->CloseVTKFile ();
      return 1;
      }

    this->ReadPointData(output, numPts);
    }

  else
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }
  this->CloseVTKFile ();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
