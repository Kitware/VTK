/*=========================================================================

  Program:   Visualization Library
  Module:    vlPolyR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlPolyR.hh"

vlPolyReader::vlPolyReader()
{
}

vlPolyReader::~vlPolyReader()
{
}

unsigned long int vlPolyReader::GetMTime()
{
  unsigned long dtime = this->vlPolySource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vl polygonal data file to read.
void vlPolyReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vlPolyReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vlPolyReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vlPolyReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vlPolyReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vlPolyReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vlPolyReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vlPolyReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vlPolyReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vlPolyReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vlPolyReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vlPolyReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vlPolyReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vlPolyReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vlPolyReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vlPolyReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts, size, ncells;

  vlDebugMacro(<<"Reading vl polygonal data...");
  this->Initialize();
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if ( !(fp=this->Reader.OpenVLFile()) ||
  ! this->Reader.ReadHeader(fp) )
      return;
//
// Read polygonal data specific stuff
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    {
    vlErrorMacro(<<"Data file ends prematurely!");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      {
      vlErrorMacro(<<"Data file ends prematurely!");
      return;
      } 

    if ( strncmp(this->Reader.LowerCase(line),"polydata",8) )
      {
      vlErrorMacro(<< "Cannot read dataset type: " << line);
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
          vlErrorMacro(<<"Cannot read number of points!");
          return;
          }

        this->Reader.ReadPoints(fp, (vlPointSet *)this, numPts);
        }

      else if ( ! strncmp(line,"vertices",8) )
        {
        vlCellArray *verts = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vlErrorMacro(<<"Cannot read vertices!");
          return;
          }

        this->Reader.ReadCells(fp, size, verts->WritePtr(ncells,size));
        verts->WrotePtr();
        this->SetVerts(verts);
        vlDebugMacro(<<"Read " << ncells << " vertices\n");
        }

      else if ( ! strncmp(line,"lines",5) )
        {
        vlCellArray *lines = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vlErrorMacro(<<"Cannot read lines!");
          return;
          }

        this->Reader.ReadCells(fp, size, lines->WritePtr(ncells,size));
        lines->WrotePtr();
        this->SetLines(lines);
        vlDebugMacro(<<"Read " << ncells << " lines\n");
        }

      else if ( ! strncmp(line,"polygons",8) )
        {
        vlCellArray *polys = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vlErrorMacro(<<"Cannot read polygons!");
          return;
          }

        this->Reader.ReadCells(fp, size, polys->WritePtr(ncells,size));
        polys->WrotePtr();
        this->SetPolys(polys);
        vlDebugMacro(<<"Read " << ncells << " polygons\n");
        }

      else if ( ! strncmp(line,"triangle_strips",15) )
        {
        vlCellArray *tris = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          {
          vlErrorMacro(<<"Cannot read triangle strips!");
          return;
          }

        this->Reader.ReadCells(fp, size, tris->WritePtr(ncells,size));
        tris->WrotePtr();
        this->SetStrips(tris);
        vlDebugMacro(<<"Read " << ncells << " triangle strips\n");
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          {
          vlErrorMacro(<<"Cannot read point data!");
          return;
          }
        
        if ( npts != numPts )
          {
          vlErrorMacro(<<"Number of points don't match number data values!");
          return;
          }

        this->Reader.ReadPointData(fp, (vlDataSet *)this, npts);
        break; //out of this loop
        }

      else
        {
        vlErrorMacro(<< "Unrecognized keyord: " << line);
        return;
        }
      }

      if ( ! this->GetPoints() ) vlWarningMacro(<<"No points read!");
      if ( !(this->GetVerts() || this->GetLines() || 
      this->GetPolys() || this->GetStrips()) ) 
        vlWarningMacro(<<"No topology read!");
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vlWarningMacro(<<"No geometry defined in data file!");
    if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
      {
      vlErrorMacro(<<"Cannot read point data!");
      return;
      }

    this->Reader.ReadPointData(fp, (vlDataSet *)this, numPts);
    }

  else 
    {
    vlErrorMacro(<< "Unrecognized keyord: " << line);
    }
}

void vlPolyReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolySource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
