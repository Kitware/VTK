/*=========================================================================

  Program:   Visualization Library
  Module:    BYURead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BYURead.hh"
#include "FPoints.hh"
#include "FVectors.hh"
#include "FScalars.hh"
#include "FTCoords.hh"

vlBYUReader::vlBYUReader()
{
  this->GeometryFilename = NULL;
  this->DisplacementFilename = NULL;
  this->ScalarFilename = NULL;
  this->TextureFilename = NULL;

  this->ReadDisplacement = 1;
  this->ReadScalar = 1;
  this->ReadTexture = 1;

  this->PartNumber = 1;
}

vlBYUReader::~vlBYUReader()
{
  if ( this->GeometryFilename ) delete [] this->GeometryFilename;
  if ( this->DisplacementFilename ) delete [] this->DisplacementFilename;
  if ( this->ScalarFilename ) delete [] this->ScalarFilename;
  if ( this->TextureFilename ) delete [] this->TextureFilename;
}

void vlBYUReader::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlBYUReader::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

    os << indent << "Geometry Filename: " << this->GeometryFilename << "\n";
    os << indent << "Read Displacement: " << (this->ReadDisplacement ? "On\n" : "Off\n");
    os << indent << "Displacement Filename: " << this->DisplacementFilename << "\n";
    os << indent << "Read Scalar: " << (this->ReadScalar ? "On\n" : "Off\n");
    os << indent << "Scalar Filename: " << this->ScalarFilename << "\n";
    os << indent << "Read Texture: " << (this->ReadTexture ? "On\n" : "Off\n");
    os << indent << "Texture Filename: " << this->TextureFilename << "\n";
    }
}

void vlBYUReader::Execute()
{
  FILE *geomFp;
  int numPts;
//
// Initialize
//
  this->Initialize();

  if ((geomFp = fopen(this->GeometryFilename, "r")) == NULL)
    {
    vlErrorMacro(<< "Geometry file: " << this->GeometryFilename << " not found");
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

void vlBYUReader::ReadGeometryFile(FILE *geomFile, int &numPts)
{
  int numParts, numPolys, numEdges;
  int partStart, partEnd;
  int i;
  vlFloatPoints *newPts;
  vlCellArray *newPolys;
  float x[3];
  int npts, pts[MAX_CELL_SIZE];
  int id, polyId;
//
// Read header (not using fixed format! - potential problem in some files.)
//
  fscanf (geomFile, "%d %d %d %d", &numParts, &numPts, &numPolys, &numEdges);

  // skip over unwanted parts
  for (i=0; i < (this->PartNumber-1); i++) fscanf (geomFile, "%*d %*d");
  fscanf (geomFile, "%d %d", &partStart, &partEnd);
  for (i=this->PartNumber; i < numParts; i++) fscanf (geomFile, "%*d %*d");

  if ( numParts < 1 || numPts < 1 || numPolys < 1 )
    {
    vlErrorMacro(<<"Bad MOVIE.BYU file");
    return;
    }
//
// Allocate data objects
//
  newPts = new vlFloatPoints(numPts);
  newPolys = new vlCellArray(numPolys+numEdges);
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
      while ( id >= 0 ) fscanf (geomFile, "%d", &id);
      }
    npts++;

    // Insert polygon (if in selected part)
    if ( partStart <= polyId && polyId <= partEnd )
      {
      for ( i=0; i < npts; i++ ) pts[i] -= 1; //fix one-offset
      newPolys->InsertNextCell(npts,pts);
      }
    }

  vlDebugMacro(<<"Read " << numPts << " points, " << newPolys->GetNumberOfCells() 
               << " polygons");

  this->SetPoints(newPts);
  this->SetPolys(newPolys);
}

void vlBYUReader::ReadDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  float v[3];
  vlFloatVectors *newVectors;

  if ( this->ReadDisplacement && this->DisplacementFilename )
    {
    if ( !(dispFp = fopen(this->DisplacementFilename, "r")) )
      {
      vlErrorMacro (<<"Couldn't open displacement file");
      return;
      }
    }
  else return;
//
// Allocate and read data
//
  newVectors = new vlFloatVectors(numPts);

  for (i=0; i<numPts; i++)
    {
    fscanf(dispFp, "%e %e %e", v, v+1, v+2);
    newVectors->SetVector(i,v);
    }

  vlDebugMacro(<<"Read " << numPts << " displacements");

  this->PointData.SetVectors(newVectors);
}

void vlBYUReader::ReadScalarFile(int numPts)
{
  FILE *scalarFp;
  int i;
  float s;
  vlFloatScalars *newScalars;

  if ( this->ReadScalar && this->ScalarFilename )
    {
    if ( !(scalarFp = fopen(this->ScalarFilename, "r")) )
      {
      vlErrorMacro (<<"Couldn't open scalar file");
      return;
      }
    }
  else return;
//
// Allocate and read data
//
  newScalars = new vlFloatScalars(numPts);

  for (i=0; i<numPts; i++)
    {
    fscanf(scalarFp, "%e", &s);
    newScalars->SetScalar(i,s);
    }

  vlDebugMacro(<<"Read " << numPts << " scalars");

  this->PointData.SetScalars(newScalars);
}

void vlBYUReader::ReadTextureFile(int numPts)
{
  FILE *textureFp;
  int i;
  float t[2];
  vlFloatTCoords *newTCoords;

  if ( this->ReadTexture && this->TextureFilename )
    {
    if ( !(textureFp = fopen(this->TextureFilename, "r")) )
      {
      vlErrorMacro (<<"Couldn't open texture file");
      return;
      }
    }
  else return;
//
// Allocate and read data
//
  newTCoords = new vlFloatTCoords(numPts,2);

  for (i=0; i<numPts; i++)
    {
    fscanf(textureFp, "%e %e", t, t+1);
    newTCoords->SetTCoord(i,t);
    }

  vlDebugMacro(<<"Read " << numPts << " texture coordinates");

  this->PointData.SetTCoords(newTCoords);
}
