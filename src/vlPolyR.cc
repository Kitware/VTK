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
  this->Filename = NULL;
}

vlPolyReader::~vlPolyReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

unsigned long int vlPolyReader::GetMTime()
{
  unsigned long dtime = this->vlPolySource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
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

  if ( !(fp=this->Reader.OpenVLFile(this->Filename, this->Debug)) ||
  ! this->Reader.ReadHeader(fp,this->Debug) )
      return;
//
// Read polygonal data specific stuff
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    goto PREMATURE;

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      goto PREMATURE;

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
      if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
        goto PREMATURE;

      if ( ! strncmp(this->Reader.LowerCase(line),"points",6) )
        {
        if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
          goto PREMATURE;

        this->Reader.ReadPoints(fp, (vlPointSet *)this, numPts);
        }

      else if ( ! strncmp(line,"vertices",8) )
        {
        vlCellArray *verts = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          goto PREMATURE;

        this->Reader.ReadCells(fp, size, verts->WritePtr(ncells,size));
        verts->WrotePtr();
        this->SetVerts(verts);
        }

      else if ( ! strncmp(line,"lines",5) )
        {
        vlCellArray *lines = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          goto PREMATURE;

        this->Reader.ReadCells(fp, size, lines->WritePtr(ncells,size));
        lines->WrotePtr();
        this->SetLines(lines);
        }

      else if ( ! strncmp(line,"polygons",8) )
        {
        vlCellArray *polys = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          goto PREMATURE;

        this->Reader.ReadCells(fp, size, polys->WritePtr(ncells,size));
        polys->WrotePtr();
        this->SetPolys(polys);
        }

      else if ( ! strncmp(line,"triangle_strips",15) )
        {
        vlCellArray *tris = new vlCellArray;
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          goto PREMATURE;

        this->Reader.ReadCells(fp, size, tris->WritePtr(ncells,size));
        tris->WrotePtr();
        this->SetStrips(tris);
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          goto PREMATURE;
        
        if ( npts != numPts )
          {
          vlErrorMacro(<<"Number of points don't match!");
          return;
          }

        break; //out of this loop
        }

      else
        goto UNRECOGNIZED;

      }
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
      goto PREMATURE;

    numPts = 0;
    vlWarningMacro(<<"Not reading any dataset geometry...");
    }

  else 
    goto UNRECOGNIZED;

//
// Now read the point data
//
  this->Reader.ReadPointData(fp, (vlDataSet *)this, numPts, this->Debug);
  return;

  PREMATURE:
    vlErrorMacro(<< "Premature EOF");
    return;

  UNRECOGNIZED:
    vlErrorMacro(<< "Unrecognized keyord: " << line);
    return;
}

void vlPolyReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolySource::PrintSelf(os,indent);
  os << indent << "Filename: " << this->Filename << "\n";
  this->Reader.PrintSelf(os,indent.GetNextIndent());
}
