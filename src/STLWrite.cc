/*=========================================================================

  Program:   Visualization Toolkit
  Module:    STLWrite.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "STLWrite.hh"
#include "Polygon.hh"
#include "ByteSwap.hh"

vtkSTLWriter::vtkSTLWriter()
{
  this->Filename = NULL;
  this->WriteMode = STL_ASCII;
}

vtkSTLWriter::~vtkSTLWriter()
{
  if ( this->Filename ) delete [] this->Filename;
}

// Description:
// Specify the input data or filter.
void vtkSTLWriter::SetInput(vtkPolyData *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

void vtkSTLWriter::WriteData()
{
  vtkPoints *pts;
  vtkCellArray *polys;
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( (pts = input->GetPoints()) == NULL ||
  (polys = input->GetPolys()) == NULL )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }

  if ( this->Filename == NULL)
    {
    vtkErrorMacro(<< "Please specify filename to write");
    return;
    }

  this->StartWrite(this->StartWriteArg);
  if ( this->WriteMode == STL_BINARY ) this->WriteBinarySTL(pts,polys);
  else this->WriteAsciiSTL(pts,polys);
  this->EndWrite(this->EndWriteArg);
}

static char header[]="Visualization Toolkit generated SLA File                                        ";

void vtkSTLWriter::WriteAsciiSTL(vtkPoints *pts, vtkCellArray *polys)
{
  FILE *fp;
  float n[3], *v1, *v2, *v3;
  int npts, *indx;
  vtkPolygon poly;

  if ((fp = fopen(this->Filename, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->Filename);
    return;
    }
//
//  Write header
//
  vtkDebugMacro("Writing ASCII sla file");
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

void vtkSTLWriter::WriteBinarySTL(vtkPoints *pts, vtkCellArray *polys)
{
  FILE *fp;
  float n[3], *v1, *v2, *v3;
  int npts, *indx;
  vtkPolygon poly;
  vtkByteSwap swap;
  unsigned long ulint;
  unsigned short ibuff2=0;

  if ((fp = fopen(this->Filename, "wb")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->Filename);
    return;
    }
//
//  Write header
//
  vtkDebugMacro("Writing ASCII sla file");
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

void vtkSTLWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);
 
  os << indent << "Filename: " << this->Filename << "\n";

  if ( this->WriteMode == STL_ASCII  )
    os << indent << "Write Mode: ASCII\n";
  else
    os << indent << "Write Mode: BINARY\n";

}

