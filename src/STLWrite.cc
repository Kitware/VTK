/*=========================================================================

  Program:   Visualization Library
  Module:    STLWrite.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "STLWrite.hh"
#include "Polygon.hh"
#include "ByteSwap.hh"

vlSTLWriter::vlSTLWriter()
{
  this->Filename = NULL;
  this->WriteMode = STL_ASCII;
}

vlSTLWriter::~vlSTLWriter()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlSTLWriter::WriteData()
{
  vlPoints *pts;
  vlCellArray *polys;
  vlPolyData *input=(vlPolyData *)this->Input;

  input->Update();

  if ( (pts = input->GetPoints()) == NULL ||
  (polys = input->GetPolys()) == NULL )
    {
    vlErrorMacro(<<"No data to write!");
    return;
    }

  if ( this->Filename == NULL)
    {
    vlErrorMacro(<< "Please specify filename to write");
    return;
    }

  this->StartWrite(this->StartWriteArg);
  if ( this->WriteMode == STL_BINARY ) this->WriteBinarySTL(pts,polys);
  else this->WriteAsciiSTL(pts,polys);
  this->EndWrite(this->EndWriteArg);
}

static char header[]="Visualization Library generated SLA File                                        ";

void vlSTLWriter::WriteAsciiSTL(vlPoints *pts, vlCellArray *polys)
{
  FILE *fp;
  float n[3], *v1, *v2, *v3;
  int npts, *indx;
  vlPolygon poly;

  if ((fp = fopen(this->Filename, "w")) == NULL)
    {
    vlErrorMacro(<< "Couldn't open file: " << this->Filename);
    return;
    }
//
//  Write header
//
  vlDebugMacro("Writing ASCII sla file");
  fprintf (fp, "%80s\n", header);
//
//  Write out triangle polygons.  In not a triangle polygon, only first 
//  three vertices are written.
//
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    v1 = pts->GetPoint(indx[0]);
    v2 = pts->GetPoint(indx[1]);
    v3 = pts->GetPoint(indx[2]);

    poly.ComputeNormal(pts, npts, indx, n);

    fprintf (fp, " FACET NORMAL %.6g %.6g %.6g\n  OUTER LOOP\n",
            n[0], n[1], n[2]);

    fprintf (fp, "   VERTEX %.6g %.6g %.6g\n", v1[0], v1[1], v1[2]);
    fprintf (fp, "   VERTEX %.6g %.6g %.6g\n", v2[0], v2[1], v2[2]);
    fprintf (fp, "   VERTEX %.6g %.6g %.6g\n", v3[0], v3[1], v3[2]);

    fprintf (fp, "  ENDLOOP\n ENDFACET\n");
    }
  fprintf (fp, "ENDSOLID\n");
}

void vlSTLWriter::WriteBinarySTL(vlPoints *pts, vlCellArray *polys)
{
  FILE *fp;
  float n[3], *v1, *v2, *v3;
  int npts, *indx;
  vlPolygon poly;
  vlByteSwap swap;
  unsigned long ulint;
  unsigned short ibuff2=0;

  if ((fp = fopen(this->Filename, "wb")) == NULL)
    {
    vlErrorMacro(<< "Couldn't open file: " << this->Filename);
    return;
    }
//
//  Write header
//
  vlDebugMacro("Writing ASCII sla file");
  fwrite (header, 1, 80, fp);

  ulint = (unsigned long int) polys->GetNumberOfCells();
  swap.Swap4(&ulint);
  fwrite (&ulint, 1, 4, fp);
//
//  Write out triangle polygons.  In not a triangle polygon, only first 
//  three vertices are written.
//
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    v1 = pts->GetPoint(indx[0]);
    v2 = pts->GetPoint(indx[1]);
    v3 = pts->GetPoint(indx[2]);

    poly.ComputeNormal(pts, npts, indx, n);
    swap.Swap4(n); swap.Swap4(n+1); swap.Swap4(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = v1[0];  n[1] = v1[1];  n[2] = v1[2]; 
    swap.Swap4(n); swap.Swap4(n+1); swap.Swap4(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = v2[0];  n[1] = v2[1];  n[2] = v2[2]; 
    swap.Swap4(n); swap.Swap4(n+1); swap.Swap4(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = v3[0];  n[1] = v3[1];  n[2] = v3[2]; 
    swap.Swap4(n); swap.Swap4(n+1); swap.Swap4(n+2);
    fwrite (n, 4, 3, fp);

    fwrite (&ibuff2, 2, 1, fp);
    }
}

void vlSTLWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyFilter::_PrintSelf(os,indent);
  vlWriter::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}

