/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MergeF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MergeF.hh"
#include "PolyData.hh"

vtkMergeFilter::vtkMergeFilter()
{
  // prevents dangling reference to DataSet
  this->Geometry = new vtkPolyData;

  this->Scalars = NULL;
  this->Vectors = NULL;
  this->Normals = NULL;
  this->TCoords = NULL;
  this->Tensors = NULL;
  this->UserDefined = NULL;
}

vtkMergeFilter::~vtkMergeFilter()
{
}

void vtkMergeFilter::Update()
{
  unsigned long int mtime=0, dsMtime;

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Geometry->Update();
  if ( this->Scalars ) 
    {
    this->Scalars->Update();
    dsMtime = this->Scalars->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->Vectors )
    {
    this->Vectors->Update();
    dsMtime = this->Vectors->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->Normals )
    {
    this->Normals->Update();
    dsMtime = this->Normals->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->TCoords )
    {
    this->TCoords->Update();
    dsMtime = this->TCoords->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->Tensors )
    {
    this->Tensors->Update();
    dsMtime = this->Tensors->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->UserDefined )
    {
    this->UserDefined->Update();
    dsMtime = this->UserDefined->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  if (mtime > this->GetMTime() || this->GetMTime() > this->ExecuteTime ||
  this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
  
  if ( this->Geometry->ShouldIReleaseData() ) this->Geometry->ReleaseData();

  if ( this->Scalars && this->Scalars->ShouldIReleaseData() ) 
    this->Scalars->ReleaseData();

  if ( this->Vectors && this->Vectors->ShouldIReleaseData() ) 
    this->Vectors->ReleaseData();

  if ( this->Normals && this->Normals->ShouldIReleaseData() ) 
    this->Normals->ReleaseData();

  if ( this->TCoords && this->TCoords->ShouldIReleaseData() ) 
    this->TCoords->ReleaseData();

  if ( this->Tensors && this->Tensors->ShouldIReleaseData() ) 
    this->Tensors->ReleaseData();

  if ( this->UserDefined && this->UserDefined->ShouldIReleaseData() ) 
    this->UserDefined->ReleaseData();
}

void vtkMergeFilter::Initialize()
{
  if ( this->Geometry )
    {
    // copies input geometry to internal data set
    this->Geometry->Delete();
    this->Geometry = this->Geometry->MakeObject(); 
    }
  else
    {
    return;
    }
}

int vtkMergeFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkMergeFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

// Merge it all together
void vtkMergeFilter::Execute()
{
  int numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  int numTensors=0, numUserDefined=0;
  vtkPointData *pd;
  vtkScalars *scalars;
  vtkVectors *vectors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkTensors *tensors;
  vtkUserDefined *ud;

  vtkDebugMacro(<<"Merging data!");

  // geometry is created
  this->Initialize();

  if ( (numPts = this->Geometry->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"Nothing to merge!");
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

void vtkMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFilter::_PrintSelf(os,indent);
  vtkDataSet::PrintSelf(os,indent);

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

