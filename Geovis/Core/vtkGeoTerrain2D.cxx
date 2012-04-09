/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain2D.cxx

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

#include "vtkGeoTerrain2D.h"

#include "vtkCamera.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoSource.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkGeoTerrain2D);
//----------------------------------------------------------------------------
vtkGeoTerrain2D::vtkGeoTerrain2D()
{
  this->LocationTolerance = 50.0;
  this->TextureTolerance = 1.0;
  this->CameraBounds[0] = 0;
  this->CameraBounds[1] = 1;
  this->CameraBounds[2] = 0;
  this->CameraBounds[3] = 1;
  this->PixelSize = 1;
}

//----------------------------------------------------------------------------
vtkGeoTerrain2D::~vtkGeoTerrain2D()
{
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::InitializeNodeAnalysis(vtkRenderer* ren)
{
  // Determine the 2D camera bounds
  vtkCamera* cam = ren->GetActiveCamera();
  double scale = cam->GetParallelScale();
  double* pos = cam->GetPosition();
  int* size = ren->GetSize();
  this->PixelSize = 2.0 * scale / size[1];
  this->CameraBounds[0] = pos[0] - size[0]*this->PixelSize/2.0;
  this->CameraBounds[1] = pos[0] + size[0]*this->PixelSize/2.0;
  this->CameraBounds[2] = pos[1] - size[1]*this->PixelSize/2.0;
  this->CameraBounds[3] = pos[1] + size[1]*this->PixelSize/2.0;
}

//----------------------------------------------------------------------------
bool vtkGeoTerrain2D::NodeInViewport(vtkGeoTerrainNode* cur)
{
  double bounds[4];
  cur->GetProjectionBounds(bounds);
  return bounds[1] > this->CameraBounds[0] && bounds[0] < this->CameraBounds[1] &&
         bounds[3] > this->CameraBounds[2] && bounds[2] < this->CameraBounds[3];
}

//----------------------------------------------------------------------------
int vtkGeoTerrain2D::EvaluateNode(vtkGeoTerrainNode* cur)
{
  double bounds[4];
  cur->GetProjectionBounds(bounds);

  // Determine the maximum allowable location error
  double maxLocationError = this->LocationTolerance*this->PixelSize;
  bool locationErrorOk = cur->GetError() < maxLocationError;

  // Determine the maximum allowable patch size
  double maxPatchSize = 300.0*this->TextureTolerance*this->PixelSize;
  double patchX = bounds[1] - bounds[0];
  double patchY = bounds[3] - bounds[2];
  double patchSize = (patchX > patchY) ? patchX : patchY;
  bool textureErrorOk = patchSize < maxPatchSize;

  if (!locationErrorOk || !textureErrorOk)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "LocationTolerance: " << this->LocationTolerance << "\n";
  os << indent << "TextureTolerance: " << this->TextureTolerance << "\n";
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkGeoTerrain2D::GetTransform()
{ 
  if(this->GeoSource != NULL)
    {
    return this->GeoSource->GetTransform(); 
    }
  return NULL;
}

