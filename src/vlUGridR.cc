/*=========================================================================

  Program:   Visualization Library
  Module:    vlUGridR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlUGridR.hh"

vlUnstructuredGridReader::vlUnstructuredGridReader()
{
  this->Filename = NULL;
}

vlUnstructuredGridReader::~vlUnstructuredGridReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

unsigned long int vlUnstructuredGridReader::GetMTime()
{
  unsigned long dtime = this->vlUnstructuredGridSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

void vlUnstructuredGridReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts, size, ncells;
  vlCellArray *cells=NULL;
  int *types=NULL;

  vlDebugMacro(<<"Reading vl unsstructured grid...");
  this->Initialize();

  if ( !(fp=this->Reader.OpenVLFile(this->Filename, this->Debug)) ||
  ! this->Reader.ReadHeader(fp,this->Debug) )
      return;
//
// Read unstructured grid specific stuff
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

    if ( strncmp(this->Reader.LowerCase(line),"unstructured_grid",17) )
      {
      vlErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Might find points, cells, and cell types
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

      else if ( ! strncmp(line,"cells",5) )
        {
        if ( (retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2 ) 
          goto PREMATURE;

        cells = new vlCellArray;
        this->Reader.ReadCells(fp, size, cells->WritePtr(ncells,size));
        cells->WrotePtr();
        if ( cells && types ) this->SetCells(types, cells);
        }

      else if ( ! strncmp(line,"cell_types",5) )
        {
        if ( (retStat=fscanf(fp,"%d", &ncells)) == EOF || retStat < 1 ) 
          goto PREMATURE;

        types = new int[ncells];
        if ( this->Reader.GetFileType() == BINARY )
          {
          if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
          if ( fread(types,sizeof(int),ncells,fp) != ncells ) goto PREMATURE;
          }
        else //ascii
          {
          for (int i=0; i<size; i++)
            {
            if ((retStat=fscanf(fp,"%d",types[i])) == EOF || retStat < 1) 
              goto PREMATURE;
            }
          }
        if ( cells && types ) this->SetCells(types, cells);
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
  if ( types ) delete [] types;
  return;

  PREMATURE:
    vlErrorMacro(<< "Premature EOF");
    return;

  UNRECOGNIZED:
    vlErrorMacro(<< "Unrecognized keyord: " << line);
    return;
}

void vlUnstructuredGridReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlUnstructuredGridSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
  this->Reader.PrintSelf(os,indent.GetNextIndent());
}
