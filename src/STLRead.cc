/*=========================================================================

  Program:   Visualization Library
  Module:    STLRead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <ctype.h>
#include "STLRead.hh"
#include "ByteSwap.hh"

#define ASCII 0
#define BINARY 1

void vlSTLReader::Execute()
{
  FILE *fp;
  vlFloatPoints *newPts;
  vlCellArray *newPolys;
//
// Initialize
//
  this->Initialize();

  if ((fp = fopen(this->Filename, "r")) == NULL)
    {
    vlErrorMacro(<< "File " << this->Filename << " not found\n");
    return;
    }

  newPts = new vlFloatPoints(5000,10000);
  newPolys = new vlCellArray(10000,20000);
  
//
// Depending upon file type, read differently
//
  if ( this->GetSTLFileType(fp) == ASCII )
    {
    if ( this->ReadASCIISTL(fp,newPts,newPolys) ) return;
    }
  else
    {
    if ( this->ReadBinarySTL(fp,newPts,newPolys) ) return;
    }

  vlDebugMacro(<< "Read " << newPts->NumberOfPoints() << " points\n");
  vlDebugMacro(<< "Read " << newPolys->GetNumberOfCells() << " triangles\n");
//
// Since we sized the dynamic arrays arbitrarily to begin with 
// need to resize them to fit data
//
  newPts->Squeeze();
  newPolys->Squeeze();
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->SetPolys(newPolys);
}

int vlSTLReader::ReadBinarySTL(FILE *fp, vlFloatPoints *newPts, vlCellArray *newPolys)
{
  int i, numTris, pts[3];
  unsigned long   ulint;
  unsigned short  ibuff2;
  char    header[81];
  vlByteSwap swap;
  typedef struct  {float  n[3], v1[3], v2[3], v3[3];} facet_t;
  facet_t facet;

  vlDebugMacro(<< " Reading BINARY STL file\n");
//
//  File is read to obtain raw information as well as bounding box
//
  fread (header, 1, 80, fp);
  fread (&ulint, 1, 4, fp);
  swap.Swap4 (&ulint);
//
// Many .stl files contain bogus count.  Hence we will ignore and read 
//   until end of file.
//
  if ( (numTris = (int) ulint) <= 0 )
    {
    vlDebugMacro(<< "Bad binary count (" << numTris << ")\n");
    }

  while ( fread(&facet,48,1,fp) > 0 )
    {
    fread(&ibuff2,2,1,fp); /* read extra junk */

    swap.Swap4 (facet.n);
    swap.Swap4 (facet.n+1);
    swap.Swap4 (facet.n+2);

    swap.Swap4 (facet.v1);
    swap.Swap4 (facet.v1+1);
    swap.Swap4 (facet.v1+2);
    pts[0] = newPts->InsertNextPoint(facet.v1);

    swap.Swap4 (facet.v2);
    swap.Swap4 (facet.v2+1);
    swap.Swap4 (facet.v2+2);
    pts[1] = newPts->InsertNextPoint(facet.v2);

    swap.Swap4 (facet.v3);
    swap.Swap4 (facet.v3+1);
    swap.Swap4 (facet.v3+2);
    pts[2] = newPts->InsertNextPoint(facet.v3);

    newPolys->InsertNextCell(3,pts);

    if (this->Debug && (i % 5000) == 0 && i != 0 )
      fprintf (stderr,"%s: triangle #%d\n", this->GetClassName(), i);
    }

  return 0;
}

int vlSTLReader::ReadASCIISTL(FILE *fp, vlFloatPoints *newPts, vlCellArray *newPolys)
{
  char line[256];
  int i;
  float x[3];
  int pts[3];

  vlDebugMacro(<< " Reading ASCII STL file\n");
//
//  Ingest header and junk to get to first vertex
//
  fgets (line, 255, fp);
/*
 *  Go into loop, reading  facet normal and vertices
 */
  while (fscanf(fp,"%*s %*s %f %f %f\n", x, x+1, x+2)!=EOF) 
    {
    fgets (line, 255, fp);
    fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2);
    pts[0] = newPts->InsertNextPoint(x);
    fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2);
    pts[1] = newPts->InsertNextPoint(x);
    fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2);
    pts[2] = newPts->InsertNextPoint(x);
    fgets (line, 255, fp); /* end loop */
    fgets (line, 255, fp); /* end facet */

    newPolys->InsertNextCell(3,pts);

    if (this->Debug && ((newPolys->GetNumberOfCells() % 5000) == 0) )
      fprintf (stderr,"%s: triangle #%d\n", this->GetClassName(), newPolys->GetNumberOfCells());
    }

  return 0;
}

int vlSTLReader::GetSTLFileType(FILE *fp)
{
  char header[256];
  int type, i;
//
//  Read a little from the file to figure what type it is.
//
  fgets (header, 255, fp); /* first line is always ascii */
  fgets (header, 18, fp); 
  for (i=0, type=ASCII; i<17 && type == ASCII; i++) // don't test \0
    if ( ! isprint(header[i]) )
      type = BINARY;
//
// Reset file for reading
//
  rewind (fp);
  return type;
}

void vlSTLReader::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSTLReader::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

    os << indent << "Filename: " << this->Filename << "\n";
    }
}
