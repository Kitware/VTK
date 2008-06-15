/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkObjectFactory.h"
#include "vtkGeoTerrain.h"
#include "vtkGeoTerrainCache.h"
#include "vtkGeoTerrainGlobeSource.h"

vtkCxxRevisionMacro(vtkGeoTerrain, "1.2");
vtkStandardNewMacro(vtkGeoTerrain);


//----------------------------------------------------------------------------
vtkGeoTerrain::vtkGeoTerrain() 
{
  // It is OK to have a default, 
  // but the use should be able to change the cache.
  this->Cache = vtkSmartPointer<vtkGeoTerrainCache>::New();
  vtkSmartPointer<vtkGeoTerrainSource> source;
  source = vtkSmartPointer<vtkGeoTerrainGlobeSource>::New();
  this->Cache->SetTerrainSource(source);
}

//-----------------------------------------------------------------------------
vtkGeoTerrain::~vtkGeoTerrain() 
{  
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::SetCache(vtkGeoTerrainCache* cache)
{
  this->Cache = cache;
}

//-----------------------------------------------------------------------------
vtkGeoTerrainCache* vtkGeoTerrain::GetCache()
{
  return this->Cache;
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::StartEdit()
{
  this->NewNodes.clear();
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::AddNode(vtkGeoTerrainNode* node)
{
  this->NewNodes.push_back(node);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::FinishEdit()
{
  this->Nodes = this->NewNodes;
  this->NewNodes.clear();
}

//-----------------------------------------------------------------------------
int vtkGeoTerrain::GetNumberOfNodes()
{
  return static_cast<int>(this->Nodes.size());
}

//-----------------------------------------------------------------------------
vtkGeoTerrainNode* vtkGeoTerrain::GetNode(int idx)
{
  return this->Nodes[idx];
}

//-----------------------------------------------------------------------------
bool vtkGeoTerrain::Update(vtkGeoCamera* camera)
{
  bool returnValue = this->Cache->Update(this, camera);
  // I am putting the request second so that it will not block the Update.
  this->Cache->Request(camera);
  
  return returnValue;


}
