/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BYUWrite.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BYUWrite.hh"

// Description:
// Create object so that it writes displacement, scalar, and texture files
// (if data is available).
vtkBYUWriter::vtkBYUWriter()
{
  this->GeometryFilename = NULL;
  this->DisplacementFilename = NULL;
  this->ScalarFilename = NULL;
  this->TextureFilename = NULL;

  this->WriteDisplacement = 1;
  this->WriteScalar = 1;
  this->WriteTexture = 1;
}

vtkBYUWriter::~vtkBYUWriter()
{
  if ( this->GeometryFilename ) delete [] this->GeometryFilename;
  if ( this->DisplacementFilename ) delete [] this->DisplacementFilename;
  if ( this->ScalarFilename ) delete [] this->ScalarFilename;
  if ( this->TextureFilename ) delete [] this->TextureFilename;
}

// Description:
// Specify the input data or filter.
void vtkBYUWriter::SetInput(vtkPolyData *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

// Description:
// Write out data in MOVIE.BYU format.
void vtkBYUWriter::WriteData()
{
  FILE *geomFp;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  int numPts=input->GetNumberOfPoints();

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }

  if ((geomFp = fopen(this->GeometryFilename, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open geometry file: " << this->GeometryFilename);
    return;
    }
  else
    {
    this->WriteGeometryFile(geomFp,numPts);
    }

  this->WriteDisplacementFile(numPts);
  this->WriteScalarFile(numPts);
  this->WriteTextureFile(numPts);
}

void vtkBYUWriter::WriteGeometryFile(FILE *geomFile, int numPts)
{
  int numPolys, numEdges;
  int i;
  float *x;
  int npts, *pts;
  vtkPoints *inPts;
  vtkCellArray *inPolys;
  vtkPolyData *input=(vtkPolyData *)this->Input;
//
// Check input
//
  if ( (inPts=input->GetPoints()) == NULL ||
  (inPolys=input->GetPolys()) == NULL )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }
//
// Write header (not using fixed format! - potential problem in some files.)
//
  numPolys = input->GetPolys()->GetNumberOfCells();
  for (numEdges=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    numEdges += npts;
    }

  fprintf (geomFile, "%d %d %d %d\n", 1, numPts, numPolys, numEdges);
  fprintf (geomFile, "%d %d", 1, numPolys);
//
// Write data
//
  // write point coordinates
  for (i=0; i < numPts; i++)
    {
    x = inPts->GetPoint(i);
    fprintf(geomFile, "%e %e %e", x[0], x[1], x[2]);
    if ( (i % 2) ) fprintf(geomFile, "\n");
    }
  if ( (numPts % 2) ) fprintf(geomFile, "\n");

  // write poly data. Remember 1-offset.
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    // write this polygon
    for (i=0; i < (npts-1); i++) fprintf (geomFile, "%d ", pts[i]+1);
    fprintf (geomFile, "%d\n", -(pts[npts-1]+1));
    }

  vtkDebugMacro(<<"Wrote " << numPts << " points, " << numPolys << " polygons");
}

void vtkBYUWriter::WriteDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  float *v;
  vtkVectors *inVectors;
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( this->WriteDisplacement && this->DisplacementFilename &&
  (inVectors = input->GetPointData()->GetVectors()) != NULL )
    {
    if ( !(dispFp = fopen(this->DisplacementFilename, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open displacement file");
      return;
      }
    }
  else return;
//
// Write data
//
  for (i=0; i < numPts; i++)
    {
    v = inVectors->GetVector(i);
    fprintf(dispFp, "%e %e %e", v[0], v[1], v[2]);
    if ( (i % 2) ) fprintf (dispFp, "\n");
    }

  vtkDebugMacro(<<"Wrote " << numPts << " displacements");
}

void vtkBYUWriter::WriteScalarFile(int numPts)
{
  FILE *scalarFp;
  int i;
  float s;
  vtkScalars *inScalars;
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( this->WriteScalar && this->ScalarFilename &&
  (inScalars = input->GetPointData()->GetScalars()) != NULL )
    {
    if ( !(scalarFp = fopen(this->ScalarFilename, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open scalar file");
      return;
      }
    }
  else return;
//
// Write data
//
  for (i=0; i < numPts; i++)
    {
    s = inScalars->GetScalar(i);
    fprintf(scalarFp, "%e", s);
    if ( i != 0 && !(i % 6) ) fprintf (scalarFp, "\n");
    }

  vtkDebugMacro(<<"Wrote " << numPts << " scalars");
}

void vtkBYUWriter::WriteTextureFile(int numPts)
{
  FILE *textureFp;
  int i;
  float *t;
  vtkTCoords *inTCoords;
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( this->WriteTexture && this->TextureFilename &&
  (inTCoords = input->GetPointData()->GetTCoords()) != NULL )
    {
    if ( !(textureFp = fopen(this->TextureFilename, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open texture file");
      return;
      }
    }
  else return;
//
// Write data
//
  for (i=0; i < numPts; i++)
    {
    if ( i != 0 && !(i % 3) ) fprintf (textureFp, "\n");
    t = inTCoords->GetTCoord(i);
    fprintf(textureFp, "%e %e", t[0], t[1]);
    }

  vtkDebugMacro(<<"Wrote " << numPts << " texture coordinates");
}

void vtkBYUWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

  os << indent << "Geometry Filename: " << this->GeometryFilename << "\n";
  os << indent << "Write Displacement: " << (this->WriteDisplacement ? "On\n" : "Off\n");
  os << indent << "Displacement Filename: " << this->DisplacementFilename << "\n";
  os << indent << "Write Scalar: " << (this->WriteScalar ? "On\n" : "Off\n");
  os << indent << "Scalar Filename: " << this->ScalarFilename << "\n";
  os << indent << "Write Texture: " << (this->WriteTexture ? "On\n" : "Off\n");
  os << indent << "Texture Filename: " << this->TextureFilename << "\n";
}

