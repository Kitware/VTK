/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BYURead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BYURead.hh"
#include "FPoints.hh"
#include "FVectors.hh"
#include "FScalars.hh"
#include "FTCoords.hh"

vtkBYUReader::vtkBYUReader()
{
  this->GeometryFilename = NULL;
  this->DisplacementFilename = NULL;
  this->ScalarFilename = NULL;
  this->TextureFilename = NULL;

  this->ReadDisplacement = 1;
  this->ReadScalar = 1;
  this->ReadTexture = 1;

  this->PartNumber = 0;
}

vtkBYUReader::~vtkBYUReader()
{
  if ( this->GeometryFilename ) delete [] this->GeometryFilename;
  if ( this->DisplacementFilename ) delete [] this->DisplacementFilename;
  if ( this->ScalarFilename ) delete [] this->ScalarFilename;
  if ( this->TextureFilename ) delete [] this->TextureFilename;
}

void vtkBYUReader::Execute()
{
  FILE *geomFp;
  int numPts;
//
// Initialize
//
  this->Initialize();

  if ((geomFp = fopen(this->GeometryFilename, "r")) == NULL)
    {
    vtkErrorMacro(<< "Geometry file: " << this->GeometryFilename << " not found");
    return;
    }
  else
    {
    this->ReadGeometryFile(geomFp,numPts);
    }

  this->ReadDisplacementFile(numPts);
  this->ReadScalarFile(numPts);
  this->ReadTextureFile(numPts);
}

void vtkBYUReader::ReadGeometryFile(FILE *geomFile, int &numPts)
{
  int numParts, numPolys, numEdges;
  int partStart, partEnd;
  int i;
  vtkFloatPoints *newPts;
  vtkCellArray *newPolys;
  float x[3];
  int npts, pts[MAX_CELL_SIZE];
  int id, polyId;
//
// Read header (not using fixed format! - potential problem in some files.)
//
  fscanf (geomFile, "%d %d %d %d", &numParts, &numPts, &numPolys, &numEdges);

  if ( this->PartNumber > numParts )
    {
    vtkWarningMacro(<<"Specified part number > number of parts");
    this->PartNumber = 0;
    }

  if ( this->PartNumber > 0 ) // read just part specified
    {
    vtkDebugMacro(<<"Reading part number: " << this->PartNumber);
    for (i=0; i < (this->PartNumber-1); i++) fscanf (geomFile, "%*d %*d");
    fscanf (geomFile, "%d %d", &partStart, &partEnd);
    for (i=this->PartNumber; i < numParts; i++) fscanf (geomFile, "%*d %*d");
    }
  else // read all parts
    {
    vtkDebugMacro(<<"Reading all parts.");
    for (i=0; i < numParts; i++) fscanf (geomFile, "%*d %*d");
    partStart = 1;
    partEnd = LARGE_INTEGER;
    }

  if ( numParts < 1 || numPts < 1 || numPolys < 1 )
    {
    vtkErrorMacro(<<"Bad MOVIE.BYU file");
    return;
    }
//
// Allocate data objects
//
  newPts = new vtkFloatPoints(numPts);
  newPolys = new vtkCellArray(numPolys+numEdges);
//
// Read data
//
  // read point coordinates
  for (i=0; i<numPts; i++)
    {
    fscanf(geomFile, "%e %e %e", x, x+1, x+2);
    newPts->InsertPoint(i,x);
    }

  // read poly data. Have to fix 1-offset. Only reading part number specified.
  for ( polyId=1; polyId <= numPolys; polyId++ )
    {
    // read this polygon
    for (npts=0; npts < MAX_CELL_SIZE; npts++)
      {
      fscanf (geomFile, "%d", pts+npts);
      if ( pts[npts] <= 0 ) break;
      }

    if ( pts[npts] <= 0 ) //terminated based on negative value
      {
      pts[npts] = -pts[npts];
      }
    else //terminated based on exceeding number of points
      {
      for ( id=1; id >= 0; ) fscanf (geomFile, "%d", &id);
      }
    npts++;

    // Insert polygon (if in selected part)
    if ( partStart <= polyId && polyId <= partEnd )
      {
      for ( i=0; i < npts; i++ ) pts[i] -= 1; //fix one-offset
      newPolys->InsertNextCell(npts,pts);
      }
    }

  vtkDebugMacro(<<"Reading:" << numPts << " points, "
                 << numPolys << " polygons.");

  this->SetPoints(newPts);
  newPts->Delete();

  this->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkBYUReader::ReadDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  float v[3];
  vtkFloatVectors *newVectors;

  if ( this->ReadDisplacement && this->DisplacementFilename )
    {
    if ( !(dispFp = fopen(this->DisplacementFilename, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open displacement file");
      return;
      }
    }
  else return;
//
// Allocate and read data
//
  newVectors = new vtkFloatVectors(numPts);

  for (i=0; i<numPts; i++)
    {
    fscanf(dispFp, "%e %e %e", v, v+1, v+2);
    newVectors->SetVector(i,v);
    }

  vtkDebugMacro(<<"Read " << numPts << " displacements");

  this->PointData.SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBYUReader::ReadScalarFile(int numPts)
{
  FILE *scalarFp;
  int i;
  float s;
  vtkFloatScalars *newScalars;

  if ( this->ReadScalar && this->ScalarFilename )
    {
    if ( !(scalarFp = fopen(this->ScalarFilename, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open scalar file");
      return;
      }
    }
  else return;
//
// Allocate and read data
//
  newScalars = new vtkFloatScalars(numPts);

  for (i=0; i<numPts; i++)
    {
    fscanf(scalarFp, "%e", &s);
    newScalars->SetScalar(i,s);
    }

  vtkDebugMacro(<<"Read " << numPts << " scalars");

  this->PointData.SetScalars(newScalars);
  newScalars->Delete();
}

void vtkBYUReader::ReadTextureFile(int numPts)
{
  FILE *textureFp;
  int i;
  float t[2];
  vtkFloatTCoords *newTCoords;

  if ( this->ReadTexture && this->TextureFilename )
    {
    if ( !(textureFp = fopen(this->TextureFilename, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open texture file");
      return;
      }
    }
  else return;
//
// Allocate and read data
//
  newTCoords = new vtkFloatTCoords(numPts,2);

  for (i=0; i<numPts; i++)
    {
    fscanf(textureFp, "%e %e", t, t+1);
    newTCoords->SetTCoord(i,t);
    }

  vtkDebugMacro(<<"Read " << numPts << " texture coordinates");

  this->PointData.SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkBYUReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Geometry Filename: " << this->GeometryFilename << "\n";
  os << indent << "Read Displacement: " << (this->ReadDisplacement ? "On\n" : "Off\n");
  os << indent << "Displacement Filename: " << this->DisplacementFilename << "\n";
  os << indent << "Read Scalar: " << (this->ReadScalar ? "On\n" : "Off\n");
  os << indent << "Scalar Filename: " << this->ScalarFilename << "\n";
  os << indent << "Read Texture: " << (this->ReadTexture ? "On\n" : "Off\n");
  os << indent << "Texture Filename: " << this->TextureFilename << "\n";
}

