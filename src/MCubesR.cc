/*=========================================================================

  Program:   Visualization Library
  Module:    MCubesR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <sys/stat.h>
#include "MCubesR.hh"
#include "MergePts.hh"

// Description:
// Construct object with FlipNormals and Normals set to true.
vlMCubesReader::vlMCubesReader()
{
  this->Filename = NULL;
  this->LimitsFilename = NULL;

  this->Locator = NULL;
  this->SelfCreatedLocator = 0;

  this->FlipNormals = 1;
  this->Normals = 1;
}

vlMCubesReader::~vlMCubesReader()
{
  if (this->Filename) delete [] this->Filename;
  if (this->LimitsFilename) delete [] this->LimitsFilename;
  if (this->SelfCreatedLocator) delete this->Locator;
}

void vlMCubesReader::Execute()
{
  FILE *fp, *limitp;
  vlFloatPoints *newPts;
  vlCellArray *newPolys;
  vlFloatNormals *newNormals;
  float bounds[6];
  int i, j, k, numPts, numTris;
  typedef struct {
    float x[3], n[3];
  } pointType;
  struct  stat buf;
  pointType point;
  int nodes[3];
  float direction, n[3], dummy[2];

  vlDebugMacro(<<"Reading marching cubes file");
//
// Initialize
//
  this->Initialize();

  if ( this->Filename == NULL )
    {
    vlErrorMacro(<< "Please specify input filename");
    return;
    }
  if ( (fp = fopen(this->Filename, "r")) == NULL)
    {
    vlErrorMacro(<< "File " << this->Filename << " not found");
    return;
    }

  // Try to read limits file to get bounds. Otherwise, read data.
  if ( this->LimitsFilename != NULL && 
  (limitp = fopen (this->LimitsFilename, "r")) != NULL &&
  stat (this->Filename, &buf) == 0 )
    {
    // skip first three pairs
    fread (dummy, sizeof(float), 2, limitp);
    fread (dummy, sizeof(float), 2, limitp);
    fread (dummy, sizeof(float), 2, limitp);

    // next three pairs are x, y, z limits
    for (i = 0; i < 6; i++) fread (&bounds[i], sizeof (float), 1, limitp);

    fclose (limitp);

    // calculate the number of triangles and vertices from file size

    numTris = buf.st_size / (18*sizeof(float)); //3 points + normals
    numPts = numTris * 3;	    
    }
  else // read data to get bounds
    {
    bounds[0] = bounds[2] = bounds[4] = LARGE_FLOAT;
    bounds[1] = bounds[3] = bounds[5] = -LARGE_FLOAT;
    for (i=0; fread(&point, sizeof(pointType), 1, fp); i++) 
      {
      for (j=0; j<3; j++) 
        {
        bounds[2*j] = (bounds[2*j] < point.x[j] ? bounds[2*j] : point.x[j]);
        bounds[2*j+1] = (bounds[2*j+1] > point.x[j] ? bounds[2*j+1] : point.x[j]);
        }

      if ( i && ((i % 10000) == 0) )
        {
        vlDebugMacro(<<"Triangle vertices #" << i);
        }
      }
    numTris = i / 3;
    numPts = i;
    }
//
// Now re-read and merge
//
  rewind (fp);
  newPts = new vlFloatPoints(numPts);
  newPolys = new vlCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numTris,3));

  if ( this->Normals ) newNormals = new vlFloatNormals(numPts);
  
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPts, bounds);

  direction = this->FlipNormals ? 1.0 : -1.0;

  for ( i=0; i<numTris; i++) 
    {
    for (j=0; j<3; j++) 
      {
      fread (&point, sizeof(pointType), 1, fp);
      nodes[j] = this->Locator->InsertPoint(point.x);
      if ( this->Normals )
        {
        for (k=0; k<3; k++) n[k] = point.n[k] * direction;
        newNormals->InsertNormal(nodes[j],n);
        }
      }
    if ( nodes[0] != nodes[1] && nodes[0] != nodes[2] && 
    nodes[1] != nodes[2] )
      newPolys->InsertNextCell(3,nodes);
    }
  vlDebugMacro(<< "Read: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newPolys->GetNumberOfCells() << " triangles");

  fclose(fp);
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->SetPolys(newPolys);
  if (this->Normals) this->GetPointData()->SetNormals(newNormals);
  this->Squeeze(); // might have merged stuff

  if (this->Locator) this->Locator->Initialize(); //free storage
}

// Description:
// Specify a spatial locator for merging points. By
// default an instance of vlMergePoints is used.
void vlMCubesReader::SetLocator(vlLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) delete this->Locator;
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vlMCubesReader::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) delete this->Locator;
  this->Locator = new vlMergePoints;
  this->SelfCreatedLocator = 1;
}

void vlMCubesReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolySource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
  os << indent << "Limits Filename: " << this->LimitsFilename << "\n";
  os << indent << "Normals: " << (this->Normals ? "On\n" : "Off\n");
  os << indent << "FlipNormals: " << (this->FlipNormals ? "On\n" : "Off\n");
}
