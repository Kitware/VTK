/*=========================================================================

  Program:   Visualization Library
  Module:    MergeF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MergeF.hh"
#include "PolyData.hh"

vlMergeFilter::vlMergeFilter()
{
  // prevents dangling reference to DataSet
  this->Geometry = new vlPolyData;

  this->Scalars = NULL;
  this->Vectors = NULL;
  this->Normals = NULL;
  this->TCoords = NULL;
  this->Tensors = NULL;
  this->UserDefined = NULL;
}

vlMergeFilter::~vlMergeFilter()
{
}

void vlMergeFilter::Update()
{
  this->Geometry->Update();
  if ( this->Scalars ) this->Scalars->Update();
  if ( this->Vectors ) this->Vectors->Update();
  if ( this->Normals ) this->Normals->Update();
  if ( this->TCoords ) this->TCoords->Update();
  if ( this->Tensors ) this->Tensors->Update();
  if ( this->UserDefined ) this->UserDefined->Update();
}

void vlMergeFilter::Initialize()
{
  if ( this->Geometry )
    {
    delete this->Geometry;
    // copies input geometry to internal data set
    this->Geometry = this->Geometry->MakeObject(); 
    }
  else
    {
    return;
    }
}

void vlMergeFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlFilter::_PrintSelf(os,indent);
  vlDataSet::PrintSelf(os,indent);

  os << indent << "Geometry: (" << this->Geometry << ")\n";
  os << indent << "Geometry type: " << this->Geometry->GetClassName() << "\n";

  if ( this->Scalars )
    os << indent << "Scalars: (" << this->Scalars << ")\n";
  else
    os << indent << "Scalars: (none)\n";

  if ( this->Vectors )
    os << indent << "Vectors: (" << this->Vectors << ")\n";
  else
    os << indent << "Vectors: (none)\n";

  if ( this->Normals )
    os << indent << "Normals: (" << this->Normals << ")\n";
  else
    os << indent << "Normals: (none)\n";

  if ( this->TCoords )
    os << indent << "TCoords: (" << this->TCoords << ")\n";
  else
    os << indent << "TCoords: (none)\n";

  if ( this->Tensors )
    os << indent << "Tensors: (" << this->Tensors << ")\n";
  else
    os << indent << "Tensors: (none)\n";

  if ( this->UserDefined )
    os << indent << "UserDefined: (" << this->UserDefined << ")\n";
  else
    os << indent << "UserDefined: (none)\n";
}

// Merge it all together
void vlMergeFilter::Execute()
{
  int numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  int numTensors=0, numUserDefined=0;
  vlPointData *pd;
  vlScalars *scalars;
  vlVectors *vectors;
  vlNormals *normals;
  vlTCoords *tcoords;
  vlTensors *tensors;
  vlUserDefined *ud;

  vlDebugMacro(<<"Merging data!");

  // geometry is created
  this->Initialize();

  if ( (numPts = this->Geometry->GetNumberOfPoints()) < 1 )
    {
    vlErrorMacro(<<"Nothing to merge!");
    return;
    }
  
  if ( this->Scalars ) 
    {
    pd = this->Scalars->GetPointData();
    scalars = pd->GetScalars();
    if ( scalars != NULL ) numScalars= scalars->GetNumberOfScalars();
    }

  if ( this->Vectors ) 
    {
    pd = this->Vectors->GetPointData();
    vectors = pd->GetVectors();
    if ( vectors != NULL ) numVectors= vectors->GetNumberOfVectors();
    }

  if ( this->Normals ) 
    {
    pd = this->Normals->GetPointData();
    normals = pd->GetNormals();
    if ( normals != NULL ) numNormals= normals->GetNumberOfNormals();
    }

  if ( this->TCoords ) 
    {
    pd = this->TCoords->GetPointData();
    tcoords = pd->GetTCoords();
    if ( tcoords != NULL ) numTCoords= tcoords->GetNumberOfTCoords();
    }

  if ( this->Tensors ) 
    {
    pd = this->Tensors->GetPointData();
    tensors = pd->GetTensors();
    if ( tensors != NULL ) numTensors = tensors->GetNumberOfTensors();
    }

  if ( this->UserDefined ) 
    {
    pd = this->UserDefined->GetPointData();
    ud = pd->GetUserDefined();
    if ( ud != NULL ) numUserDefined = ud->GetNumberOfUserDefined();
    }

  // merge data only if it is consistent
  if ( numPts == numScalars )
    this->PointData.SetScalars(scalars);

  if ( numPts == numVectors )
    this->PointData.SetVectors(vectors);
    
  if ( numPts == numNormals )
    this->PointData.SetNormals(normals);

  if ( numPts == numTCoords )
    this->PointData.SetTCoords(tcoords);

  if ( numPts == numTensors )
    this->PointData.SetTensors(tensors);

  if ( numPts == numUserDefined )
    this->PointData.SetUserDefined(ud);
}
