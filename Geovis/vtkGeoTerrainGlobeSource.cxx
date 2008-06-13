/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrainGlobeSource.cxx

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
#include "vtkGeoTerrainGlobeSource.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGlobeSource.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkGeoTerrainGlobeSource, "1.1");
vtkStandardNewMacro(vtkGeoTerrainGlobeSource);


//----------------------------------------------------------------------------
vtkGeoTerrainGlobeSource::vtkGeoTerrainGlobeSource() 
{
}

//-----------------------------------------------------------------------------
vtkGeoTerrainGlobeSource::~vtkGeoTerrainGlobeSource() 
{  
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainGlobeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainGlobeSource::GenerateTerrainForNode(vtkGeoTerrainNode* node)
{
  double* range;
  vtkSmartPointer<vtkGlobeSource> globe = vtkSmartPointer<vtkGlobeSource>::New();

  range = node->GetLongitudeRange();
  globe->SetStartLongitude(range[0]);
  globe->SetEndLongitude(range[1]);

  range = node->GetLatitudeRange();
  globe->SetStartLatitude(range[0]);
  globe->SetEndLatitude(range[1]);

  globe->SetLongitudeResolution(16);
  globe->SetLatitudeResolution(16);
  
  globe->SetCurtainHeight(20000.0);
  globe->Update();

  vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
  model->ShallowCopy(globe->GetOutput());

  node->SetModel(model);
  node->UpdateBoundingSphere();
}


