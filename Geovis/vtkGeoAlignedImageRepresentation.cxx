/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageRepresentation.cxx

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

#include "vtkGeoAlignedImageRepresentation.h"

#include "vtkAssembly.h"
#include "vtkGeoAlignedImage.h"
#include "vtkGeoTerrain.h"
#include "vtkRenderView.h"
#include "vtkObjectFactory.h"
#include "vtkProp3DCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkView.h"

vtkCxxRevisionMacro(vtkGeoAlignedImageRepresentation, "1.1");
vtkStandardNewMacro(vtkGeoAlignedImageRepresentation);


//----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation::vtkGeoAlignedImageRepresentation() 
{
  this->Actor = vtkSmartPointer<vtkAssembly>::New();
  this->Terrain = 0;
  this->Image = 0;
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation::~vtkGeoAlignedImageRepresentation() 
{  
}

//-----------------------------------------------------------------------------
// This is to clean up actors, mappers, textures and other rendering object
// before the renderer and render window destruct.  It allows all graphics
// resources to be released cleanly.  Without this, the application 
// may crash on exit.
void vtkGeoAlignedImageRepresentation::ExitCleanup()
{
  this->Actor->GetParts()->RemoveAllItems();
  if (this->Image)
    {
    this->Image->ExitCleanup();
    }
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Actor: " << this->Actor << endl;
  os << indent << "Terain: " << this->Terrain << endl;
  os << indent << "Image: " << this->Image << endl;
}

//-----------------------------------------------------------------------------
// This constructs the best model possible given the data currently available.
// The request will be a separate non blocking call.
bool vtkGeoAlignedImageRepresentation::Update(vtkGeoCamera* cam)
{
  if (!cam)
    {
    return false;
    }
  bool changedFlag = 0;
  // If the terrain does not update, the image can still change to pick
  // tiles that better match the terrain.
  if (this->Terrain->Update(cam))
    {
    changedFlag = 1;
    }
  if (this->Image->Update(this->Terrain))
    {
    changedFlag = 1;
    }
  if (changedFlag)
    {
    // Now add the elements to the assembly.
    this->Actor->GetParts()->RemoveAllItems();
    this->Image->UpdateAssembly(this->Actor);
    }

  return changedFlag;
}

//-----------------------------------------------------------------------------
// Starting the representation API
bool vtkGeoAlignedImageRepresentation::AddToView(vtkView* view)
{
  vtkRenderView* gv = vtkRenderView::SafeDownCast(view);
  if (!gv)
    {
    return false;
    }
  gv->GetRenderer()->AddActor(this->Actor);
  return true;
}

//-----------------------------------------------------------------------------
// Starting the representation API
bool vtkGeoAlignedImageRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderView* gv = vtkRenderView::SafeDownCast(view);
  if (!gv)
    {
    return false;
    }
  gv->GetRenderer()->RemoveActor(this->Actor);
  return true;
}

  

