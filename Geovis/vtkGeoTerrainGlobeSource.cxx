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

vtkCxxRevisionMacro(vtkGeoTerrainGlobeSource, "1.2");
vtkStandardNewMacro(vtkGeoTerrainGlobeSource);


//----------------------------------------------------------------------------
vtkGeoTerrainGlobeSource::vtkGeoTerrainGlobeSource() 
{
  this->Globe = vtkSmartPointer<vtkGlobeSource>::New();
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

  this->Globe->SetOrigin(this->Origin);

  range = node->GetLongitudeRange();
  this->Globe->SetStartLongitude(range[0]);
  this->Globe->SetEndLongitude(range[1]);

  range = node->GetLatitudeRange();
  this->Globe->SetStartLatitude(range[0]);
  this->Globe->SetEndLatitude(range[1]);

  this->Globe->SetLongitudeResolution(16);
  this->Globe->SetLatitudeResolution(16);
  
  this->Globe->SetCurtainHeight(20000.0);
  this->Globe->Update();

  vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
  model->ShallowCopy(this->Globe->GetOutput());

  node->SetModel(model);
  node->UpdateBoundingSphere();
}


