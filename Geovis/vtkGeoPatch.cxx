/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoPatch.cxx

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
#include "vtkGeoPatch.h"
#include "vtkProperty.h"

//----------------------------------------------------------------------------
vtkGeoPatch::vtkGeoPatch() 
{
  this->Filter = vtkSmartPointer<vtkGeoComputeTextureCoordinates>::New();
  this->Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->GetProperty()->SetAmbientColor(1.0, 1.0, 1.0);  
  this->Actor->GetProperty()->SetAmbient(1.0);
  this->Texture = vtkSmartPointer<vtkTexture>::New();
  
  this->Mapper->SetInput(this->Filter->GetOutput());
  this->Mapper->ImmediateModeRenderingOn();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetTexture(this->Texture);
}

//-----------------------------------------------------------------------------
vtkGeoPatch::~vtkGeoPatch() 
{  
}

//-----------------------------------------------------------------------------
void vtkGeoPatch::SetTerrainNode(vtkGeoTerrainNode* node)
{
  this->TerrainNode = node;
  this->Valid = false;
}


//-----------------------------------------------------------------------------
void vtkGeoPatch::SetImageNode(vtkGeoImageNode* node)
{  
  this->ImageNode = node;
  this->Valid = false;
}


//-----------------------------------------------------------------------------
vtkActor* vtkGeoPatch::GetActor()
{
  if ( ! this->Valid)
    {
    // Let the user know they have to call update.
    return 0;
    }
  
  return this->Actor;
}

  
  
//-----------------------------------------------------------------------------
void vtkGeoPatch::Update()
{
  if (this->Valid)
    {
    return;
    }

  if (this->TerrainNode == 0 && this->TerrainNode->GetModel() == 0)
    {
    vtkGenericWarningMacro("No terrain.");
    return;
    }
  if (this->ImageNode == 0 && this->ImageNode->GetImage() == 0)
    { // Display the terrain with no texture.
    this->Actor->SetTexture(0);
    this->Mapper->SetInput(this->TerrainNode->GetModel());
    return;
    }
  this->Filter->SetInput(this->TerrainNode->GetModel());
  double longitudeLatitudeExtent[4];
  double* range;
  range = this->ImageNode->GetLongitudeRange();
  longitudeLatitudeExtent[0] = range[0];
  longitudeLatitudeExtent[1] = range[1];
  range = this->ImageNode->GetLatitudeRange();
  longitudeLatitudeExtent[2] = range[0];
  longitudeLatitudeExtent[3] = range[1];
  this->Filter->SetImageLongitudeLatitudeExtent(longitudeLatitudeExtent);
  this->Filter->Update();

  this->Texture->SetInput(this->ImageNode->GetImage());
  this->Texture->InterpolateOff();

  this->Valid = 1;
}

