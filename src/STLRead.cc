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
#include "MergePts.hh"

#define ASCII 0
#define BINARY 1

// Description:
// Construct object with merging set to true.
vlSTLReader::vlSTLReader()
{
  this->Filename = NULL;
  this->Merging = 1;
  this->Locator = NULL;
  this->SelfCreatedLocator = 0;
}

vlSTLReader::~vlSTLReader()
{
  if (this->Filename) delete [] this->Filename;
  if (this->SelfCreatedLocator) delete this->Locator;
}

void vlSTLReader::Execute()
{
  FILE *fp;
  vlFloatPoints *newPts, *mergedPts;
  vlCellArray *newPolys, *mergedPolys;
//
// Initialize
//
  this->Initialize();

  if ((fp = fopen(this->Filename, "r")) == NULL)
    {
    vlErrorMacro(<< "File " << this->Filename << " not found");
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

  vlDebugMacro(<< "Read: " 
               << newPts->GetNumberOfPoints() << " points, "
               << newPolys->GetNumberOfCells() << " triangles");

  fclose(fp);
//
// If merging is on, create hash table and merge points/triangles.
//
  if ( this->Merging )
    {
    int npts, *pts, i, nodes[3];

    mergedPts = new vlFloatPoints(newPts->GetNumberOfPoints()/2);
    mergedPolys = new vlCellArray(newPolys->GetSize());

    if ( this->Locator == NULL ) this->CreateDefaultLocator();
    this->Locator->InitPointInsertion (mergedPts, newPts->GetBounds());

    for (newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); )
      {
      for (i=0; i < 3; i++) 
        nodes[i] = this->Locator->InsertPoint(newPts->GetPoint(pts[i]));

      if ( nodes[0] != nodes[1] && nodes[0] != nodes[2] && 
      nodes[1] != nodes[2] )
        mergedPolys->InsertNextCell(3,nodes);
      }

      delete newPts;
      delete newPolys;

      vlDebugMacro(<< "Merged to: " 
                   << mergedPts->GetNumberOfPoints() << " points, " 
                   << mergedPolys->GetNumberOfCells() << " triangles");
    }
  else
    {
    mergedPts = newPts;
    mergedPolys = newPolys;
    }
//
// Since we sized the dynamic arrays arbitrarily to begin with 
// need to resize them to fit data
//
  mergedPts->Squeeze();
  mergedPolys->Squeeze();
//
// Update ourselves
//
  this->SetPoints(mergedPts);
  this->SetPolys(mergedPolys);

  if (this->Locator) this->Locator->Initialize(); //free storage
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

  vlDebugMacro(<< " Reading BINARY STL file");
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
    vlDebugMacro(<< "Bad binary count: attempting to correct (" << numTris << ")");
    }

  for ( i=0; fread(&facet,48,1,fp) > 0; i++ )
    {
    fread(&ibuff2,2,1,fp); //read extra junk

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

    if ( (i % 5000) == 0 && i != 0 )
      vlDebugMacro(<< "triangle# " << i);
    }

  return 0;
}

int vlSTLReader::ReadASCIISTL(FILE *fp, vlFloatPoints *newPts, vlCellArray *newPolys)
{
  char line[256];
  float x[3];
  int pts[3];

  vlDebugMacro(<< " Reading ASCII STL file");
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

    if ( (newPolys->GetNumberOfCells() % 5000) == 0 )
      vlDebugMacro(<< "triangle# " << newPolys->GetNumberOfCells());
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

// Description:
// Specify a spatial locator for merging points. By
// default an instance of vlMergePoints is used.
void vlSTLReader::SetLocator(vlLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) delete this->Locator;
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vlSTLReader::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) delete this->Locator;
  this->Locator = new vlMergePoints;
  this->SelfCreatedLocator = 1;
}

void vlSTLReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolySource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}
