/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGlobeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGeoGlobeSource.h"

#include "vtkGeoTerrainNode.h"
#include "vtkGlobeSource.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkGeoGlobeSource);
//----------------------------------------------------------------------------
vtkGeoGlobeSource::vtkGeoGlobeSource()
{
}

//----------------------------------------------------------------------------
vtkGeoGlobeSource::~vtkGeoGlobeSource()
{
}

//----------------------------------------------------------------------------
bool vtkGeoGlobeSource::FetchRoot(vtkGeoTreeNode* r)
{
  vtkGeoTerrainNode* root = 0;
  if (!(root = vtkGeoTerrainNode::SafeDownCast(r)))
  {
    vtkErrorMacro(<< "Can only fetch surface nodes from this source.");
  }

  vtkSmartPointer<vtkGlobeSource> source =
    vtkSmartPointer<vtkGlobeSource>::New();
  source->SetStartLatitude(-90.0);
  source->SetEndLatitude(90.0);
  source->SetStartLongitude(-180.0);
  source->SetEndLongitude(180.0);
  source->SetLatitudeResolution(20);
  source->SetLongitudeResolution(20);
  source->SetCurtainHeight(2000);
  source->Update();
  root->GetModel()->ShallowCopy(source->GetOutput());
  root->SetLatitudeRange(-90, 90);
  root->SetLongitudeRange(-180, 180);
  root->UpdateBoundingSphere();

  // Make sure bounds are up to date so we don't have threading issues
  // when we hand this off to the main thread.
  root->GetModel()->ComputeBounds();

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoGlobeSource::FetchChild(vtkGeoTreeNode* p, int index, vtkGeoTreeNode* c)
{
  vtkGeoTerrainNode* parent = 0;
  if (!(parent = vtkGeoTerrainNode::SafeDownCast(p)))
  {
    vtkErrorMacro(<< "Can only fetch surface nodes from this source.");
  }
  vtkGeoTerrainNode* child = 0;
  if (!(child = vtkGeoTerrainNode::SafeDownCast(c)))
  {
    vtkErrorMacro(<< "Can only fetch surface nodes from this source.");
  }

  double lonRange[2];
  double latRange[2];
  double center[2];
  parent->GetLongitudeRange(lonRange);
  parent->GetLatitudeRange(latRange);
  center[0] = (lonRange[1] + lonRange[0])/2.0;
  center[1] = (latRange[1] + latRange[0])/2.0;

  int level = parent->GetLevel() + 1;
  child->SetLevel(level);
  if (index / 2)
  {
    child->SetLatitudeRange(center[1], latRange[1]);
  }
  else
  {
    child->SetLatitudeRange(latRange[0], center[1]);
  }
  if (index % 2)
  {
    child->SetLongitudeRange(center[0], lonRange[1]);
  }
  else
  {
    child->SetLongitudeRange(lonRange[0], center[0]);
  }

  int id = 0;
  id = parent->GetId() | (index << (2*level - 2));

  child->SetId(id);
  vtkSmartPointer<vtkGlobeSource> source =
    vtkSmartPointer<vtkGlobeSource>::New();
  source->SetStartLatitude(child->GetLatitudeRange()[0]);
  source->SetEndLatitude(child->GetLatitudeRange()[1]);
  source->SetStartLongitude(child->GetLongitudeRange()[0]);
  source->SetEndLongitude(child->GetLongitudeRange()[1]);
  source->SetCurtainHeight(2000);
  source->Update();
  child->GetModel()->ShallowCopy(source->GetOutput());
  child->UpdateBoundingSphere();

  // Make sure bounds are up to date so we don't have threading issues
  // when we hand this off to the main thread.
  child->GetModel()->ComputeBounds();

  return true;
}

//----------------------------------------------------------------------------
void vtkGeoGlobeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
