/*=========================================================================

  Program:   Visualization Library
  Module:    MergeF.cc
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
#include "MergeF.hh"
#include "PolyData.hh"

vlMergeFilter::vlMergeFilter()
{
  // prevents dangling reference to DataSet
  this->Geometry = new vlPolyData;
  this->Geometry->Register(this);
}

vlMergeFilter::~vlMergeFilter()
{
  this->Geometry->UnRegister(this);

  if ( this->Scalars ) this->Scalars->UnRegister(this);
  if ( this->Vectors ) this->Vectors->UnRegister(this);
  if ( this->Normals ) this->Normals->UnRegister(this);
  if ( this->TCoords ) this->TCoords->UnRegister(this);
}

void vlMergeFilter::Update()
{
  this->Geometry->Update();
  if ( this->Scalars ) this->Scalars->Update();
  if ( this->Vectors ) this->Vectors->Update();
  if ( this->Normals ) this->Normals->Update();
  if ( this->TCoords ) this->TCoords->Update();
}

void vlMergeFilter::Initialize()
{
  if ( this->Geometry )
    {
    this->Geometry->UnRegister(this);
    // copies input geometry to internal data set
    this->Geometry = this->Geometry->MakeObject(); 
    this->Geometry->Register(this);
    }
  else
    {
    return;
    }
}

void vlMergeFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlMergeFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance

    vlFilter::PrintSelf(os,indent);
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

    this->PrintWatchOff(); // stop worrying about it now
   }
}

// Merge it all together
void vlMergeFilter::Execute()
{
  int numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  vlPointData *pd;
  vlScalars *scalars;
  vlVectors *vectors;
  vlNormals *normals;
  vlTCoords *tcoords;

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

  // merge data only if it is consistent
  if ( numPts == numScalars )
    this->PointData.SetScalars(scalars);

  if ( numPts == numVectors )
    this->PointData.SetVectors(vectors);
    
  if ( numPts == numNormals )
    this->PointData.SetNormals(normals);

  if ( numPts == numTCoords )
    this->PointData.SetTCoords(tcoords);
}
