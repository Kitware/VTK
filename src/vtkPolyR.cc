/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkPolyR.hh"

vtkPolyReader::vtkPolyReader()
{
}

vtkPolyReader::~vtkPolyReader()
{
}

unsigned long int vtkPolyReader::GetMTime()
{
  unsigned long dtime = this->vtkPolySource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vtk polygonal data file to read.
void vtkPolyReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vtkPolyReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vtkPolyReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkPolyReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vtkPolyReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkPolyReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vtkPolyReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkPolyReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vtkPolyReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkPolyReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vtkPolyReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkPolyReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vtkPolyReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkPolyReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vtkPolyReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vtkPolyReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts, size, ncells;

  vtkDebugMacro(<<"Reading vtk polygonal data...");
  this->Initialize();
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if ( !(fp=this->Reader.OpenVTKFile()) ||
  ! this->Reader.ReadHeader(fp) )
      return;
//
// Read polygonal data specific stuff
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      return;
      } 

    if ( strncmp(this->Reader.LowerCase(line),"polydata",8) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Might find points, vertices, lines, polygons, or triangle strips
//
    while (1)
      {
      if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) break;

      if ( ! strncmp(this->Reader.LowerCase(line),"points",6) )
        {
        if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
          {
          vtkErrorMacro(<<"Cannot read number of points!");
          return;
          }

        this->Reader.ReadPoints(fp, (vtkPointSet *)this, numPts);
        }

      else if ( ! strncmp(line,"vertices",8) )
        {
        vtkCellArray *verts = new vtkCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vtkErrorMacro(<<"Cannot read vertices!");
          return;
          }

        this->Reader.ReadCells(fp, size, verts->WritePtr(ncells,size));
        verts->WrotePtr();
        this->SetVerts(verts);
        verts->Delete();
        vtkDebugMacro(<<"Read " << ncells << " vertices");
        }

      else if ( ! strncmp(line,"lines",5) )
        {
        vtkCellArray *lines = new vtkCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vtkErrorMacro(<<"Cannot read lines!");
          return;
          }

        this->Reader.ReadCells(fp, size, lines->WritePtr(ncells,size));
        lines->WrotePtr();
        this->SetLines(lines);
        lines->Delete();
        vtkDebugMacro(<<"Read " << ncells << " lines");
        }

      else if ( ! strncmp(line,"polygons",8) )
        {
        vtkCellArray *polys = new vtkCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vtkErrorMacro(<<"Cannot read polygons!");
          return;
          }

        this->Reader.ReadCells(fp, size, polys->WritePtr(ncells,size));
        polys->WrotePtr();
        this->SetPolys(polys);
        polys->Delete();
        vtkDebugMacro(<<"Read " << ncells << " polygons");
        }

      else if ( ! strncmp(line,"triangle_strips",15) )
        {
        vtkCellArray *tris = new vtkCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vtkErrorMacro(<<"Cannot read triangle strips!");
          return;
          }

        this->Reader.ReadCells(fp, size, tris->WritePtr(ncells,size));
        tris->WrotePtr();
        this->SetStrips(tris);
        tris->Delete();
        vtkDebugMacro(<<"Read " << ncells << " triangle strips");
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          {
          vtkErrorMacro(<<"Cannot read point data!");
          return;
          }
        
        if ( npts != numPts )
          {
          vtkErrorMacro(<<"Number of points don't match number data values!");
          return;
          }

        this->Reader.ReadPointData(fp, (vtkDataSet *)this, npts);
        break; //out of this loop
        }

      else
        {
        vtkErrorMacro(<< "Unrecognized keyord: " << line);
        return;
        }
      }

      if ( ! this->GetPoints() ) vtkWarningMacro(<<"No points read!");
      if ( !(this->GetVerts() || this->GetLines() || 
      this->GetPolys() || this->GetStrips()) ) 
        vtkWarningMacro(<<"No topology read!");
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
      {
      vtkErrorMacro(<<"Cannot read point data!");
      return;
      }

    this->Reader.ReadPointData(fp, (vtkDataSet *)this, numPts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyord: " << line);
    }
}

void vtkPolyReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
