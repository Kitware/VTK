/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.cxx
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
#include "vtkPolyDataReader.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPolyDataReader, "1.23");
vtkStandardNewMacro(vtkPolyDataReader);

vtkPolyDataReader::vtkPolyDataReader()
{
  this->vtkSource::SetNthOutput(0, vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  this->ExecutePiece = this->ExecuteNumberOfPieces = 0;
  this->ExecuteGhostLevel = 0;
}

vtkPolyDataReader::~vtkPolyDataReader()
{
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataReader::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkPolyDataReader::SetOutput(vtkPolyData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}


//----------------------------------------------------------------------------
void vtkPolyDataReader::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces, ghostLevel;
  vtkPolyData *output = (vtkPolyData *)data;
  int idx;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);
  
  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return;
    }
  
  if (ghostLevel < 0)
    {
    return;
    }
  
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->SetUpdateExtent(piece, numPieces, ghostLevel);
      }
    }
  
  // Save the piece so execute can use this information.
  this->ExecutePiece = piece;
  this->ExecuteNumberOfPieces = numPieces;
  
  this->ExecuteGhostLevel = ghostLevel;
}

void vtkPolyDataReader::Execute()
{
  int numPts=0;
  char line[256];
  int npts, size, ncells, i;
  int done=0;
  vtkPolyData *output = this->GetOutput();
  int *tempArray;
  vtkIdType *idArray;
  
  vtkDebugMacro(<<"Reading vtk polygonal data...");

  if ( !(this->OpenVTKFile()) || !this->ReadHeader())
    {
    return;
    }
//
// Read polygonal data specific stuff
//
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return;
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
      return;
      } 

    if ( strncmp(this->LowerCase(line),"polydata",8) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile ();
      return;
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
          return;
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
          return;
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
          return;
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
          return;
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
          return;
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
          return;
          }
        
        if ( ncells != output->GetNumberOfCells() )
          {
          vtkErrorMacro(<<"Number of cells don't match number data values!");
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
          vtkErrorMacro(<<"Number of points don't match number data values!");
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
      return;
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
      return;
      }

    this->ReadPointData(output, numPts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }
  this->CloseVTKFile ();
}

void vtkPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
