/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoView.cxx

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

#include "vtkGeoView.h"

#include "vtkActor.h"
#include "vtkAssembly.h"
#include "vtkCullerCollection.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoCamera.h"
#include "vtkGeoGlobeSource.h"
#include "vtkGeoInteractorStyle.h"
#include "vtkGeoMath.h"
#include "vtkGeoSphereTransform.h"
#include "vtkGeoTerrain.h"
#include "vtkGlobeSource.h"
#include "vtkLight.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkViewTheme.h"

vtkStandardNewMacro(vtkGeoView);
vtkCxxSetObjectMacro(vtkGeoView, Terrain, vtkGeoTerrain);
//----------------------------------------------------------------------------
vtkGeoView::vtkGeoView()
{
  this->Terrain = 0;

  // Replace the interactor style
  vtkGeoInteractorStyle* style = vtkGeoInteractorStyle::New();
  this->SetInteractorStyle(style);
  style->SetCurrentRenderer(this->Renderer);
  style->ResetCamera();
  style->Delete();

  // Make a light that is ambient only
  vtkLight* light = vtkLight::New();
  light->SetAmbientColor(1.0, 1.0, 1.0);
  light->SetDiffuseColor(0.0, 0.0, 0.0);
  this->Renderer->RemoveAllLights();
  this->Renderer->AddLight(light);
  light->Delete();

  // Set the camera
  vtkGeoCamera* cam = style->GetGeoCamera();
  this->Renderer->SetActiveCamera(cam->GetVTKCamera());

  // Make an actor that is a low resolution earth.
  // This is simply to hide geometry on the other side of the earth when picking.
  // The actor in vtkGeoBackgroundImageRepresentation is not rendered during
  // visible cell selection because it is a vtkAssembly.
  this->LowResEarthMapper       = vtkPolyDataMapper::New();
//  this->LowResEarthMapper->SetResolveCoincidentTopologyToPolygonOffset();
//  this->LowResEarthMapper->SetResolveCoincidentTopologyPolygonOffsetParameters(1.0, 1000.0);

  this->LowResEarthActor        = vtkActor::New();
  this->LowResEarthSource       = NULL; // BuildLowResEarth tests if the source is null.
  this->BuildLowResEarth( cam->GetOrigin() ); // call once the mapper is set!
  this->LowResEarthActor->SetMapper(this->LowResEarthMapper);
  this->Renderer->AddActor(this->LowResEarthActor);

  // Add the assembly to the view.
  this->Assembly = vtkAssembly::New();
  this->Renderer->AddActor(this->Assembly);

  vtkGeoSphereTransform* t = vtkGeoSphereTransform::New();
//  t->SetBaseAltitude(vtkGeoMath::EarthRadiusMeters()*0.0001);
  t->SetBaseAltitude(0.0);
  this->SetTransform(t);
  t->Delete();

  this->UsingMesaDrivers = -1;
}

//----------------------------------------------------------------------------
vtkGeoView::~vtkGeoView()
{
  this->LowResEarthSource->Delete();
  this->LowResEarthMapper->Delete();
  this->LowResEarthActor->Delete();
  this->Assembly->Delete();
  this->SetTerrain(0);
}

//----------------------------------------------------------------------------
void vtkGeoView::BuildLowResEarth( double origin[3] )
{
  if (this->LowResEarthSource)
  {
    this->LowResEarthSource->Delete();
  }
  this->LowResEarthSource       = vtkGlobeSource::New();
  this->LowResEarthSource->SetOrigin( origin );
  // Make it slightly smaller than the earth so it is not visible
  double radius = this->LowResEarthSource->GetRadius();
  this->LowResEarthSource->SetRadius(0.95*radius);
  this->LowResEarthSource->SetStartLatitude(-90.0);
  this->LowResEarthSource->SetEndLatitude(90.0);
  this->LowResEarthSource->SetStartLongitude(-180.0);
  this->LowResEarthSource->SetEndLongitude(180.0);
  this->LowResEarthSource->SetLongitudeResolution(15);
  this->LowResEarthMapper->SetInputConnection(this->LowResEarthSource->GetOutputPort());
  //this->LowResEarthActor->VisibilityOff();
}

//-----------------------------------------------------------------------------
void vtkGeoView::SetLockHeading(bool lock)
{
  vtkGeoInteractorStyle* style = vtkGeoInteractorStyle::SafeDownCast(
    this->GetInteractorStyle());
  style->SetLockHeading(lock);
}

//-----------------------------------------------------------------------------
bool vtkGeoView::GetLockHeading()
{
  vtkGeoInteractorStyle* style = vtkGeoInteractorStyle::SafeDownCast(
    this->GetInteractorStyle());
  return style->GetLockHeading();
}

//-----------------------------------------------------------------------------
// This is a placeholder for a timer polling of the terrain source.
// This just checks every render.
// Prepares the view for rendering.
void vtkGeoView::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkSmartPointer<vtkCollection> imageReps =
    vtkSmartPointer<vtkCollection>::New();
  for (int i = 0; i < this->GetNumberOfRepresentations(); i++)
  {
    vtkGeoAlignedImageRepresentation* imageRep =
      vtkGeoAlignedImageRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (imageRep)
    {
      imageReps->AddItem(imageRep);
    }
  }

  if (this->Terrain)
  {
    this->Terrain->AddActors(this->Renderer, this->Assembly, imageReps);
  }
}

//----------------------------------------------------------------------------
void vtkGeoView::Render()
{
  // If this is the first time, render an extra time to get things
  // initialized for the first PrepareForRendering pass.
  this->RenderWindow->MakeCurrent();
  if(!this->RenderWindow->IsCurrent())
  {

    // Note: For some reason this needs to be called even thoguh it does not
    // make much difference logically.
    this->Superclass::Render();
    return;
  }

  this->Update();
  this->PrepareForRendering();

  // Since ZShift or polygon offsets are global, here we would like to
  // use polygon offset to push the polygons (of globe) away so that
  // other polygons or lines can be drawn on top of it.
  double zShift = 0.0;
  double factor = 0.0;
  double units = 0.0;

  // Save the depth offset state.
  if(vtkMapper::GetResolveCoincidentTopology() ==
     VTK_RESOLVE_POLYGON_OFFSET)
  {
    vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(
        factor, units);
  }
  else if(vtkMapper::GetResolveCoincidentTopology() ==
          VTK_RESOLVE_SHIFT_ZBUFFER)
  {
    zShift = vtkMapper::GetResolveCoincidentTopologyZShift();
  }

  vtkMapper::SetResolveCoincidentTopologyZShift(0.0);
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();

  // Apply minimum offset (using factor and units == 1.0).
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(1.0, 1.0);

  this->Renderer->GetCullers()->RemoveAllItems();
  this->RenderWindow->Render();

  // Restore the depth offset state.
  if(vtkMapper::GetResolveCoincidentTopology() ==
     VTK_RESOLVE_POLYGON_OFFSET)
  {
    vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
    vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(
        factor, units);
  }
  else if(vtkMapper::GetResolveCoincidentTopology() ==
          VTK_RESOLVE_SHIFT_ZBUFFER)
  {
    vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
    vtkMapper::SetResolveCoincidentTopologyZShift(zShift);
  }
  else
  {
    vtkMapper::SetResolveCoincidentTopologyToOff();
  }
}

//----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation* vtkGeoView::AddDefaultImageRepresentation(vtkImageData* image)
{
  // Add default terrain
  vtkSmartPointer<vtkGeoGlobeSource> terrainSource =
    vtkSmartPointer<vtkGeoGlobeSource>::New();
  vtkSmartPointer<vtkGeoTerrain> terrain =
    vtkSmartPointer<vtkGeoTerrain>::New();
  terrain->SetSource(terrainSource);
  this->SetTerrain(terrain);

  // Add image representation
  vtkSmartPointer<vtkGeoAlignedImageSource> imageSource =
    vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  imageSource->SetImage(image);
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  rep->SetSource(imageSource);
  this->AddRepresentation(rep);

  return rep;
}

//----------------------------------------------------------------------------
vtkGeoInteractorStyle* vtkGeoView::GetGeoInteractorStyle()
{
  return vtkGeoInteractorStyle::SafeDownCast(this->GetInteractorStyle());
}

//----------------------------------------------------------------------------
void vtkGeoView::SetGeoInteractorStyle(vtkGeoInteractorStyle* style)
{
  if(style && style != this->GetInteractorStyle())
  {
    this->SetInteractorStyle(style);
    style->SetCurrentRenderer(this->Renderer);
    style->ResetCamera();

    // Set the camera
    vtkGeoCamera* cam = style->GetGeoCamera();
    this->Renderer->SetActiveCamera(cam->GetVTKCamera());
    this->RenderWindow->GetInteractor()->SetInteractorStyle(style);
  }
}

//----------------------------------------------------------------------------
void vtkGeoView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "Terrain: " << (this->Terrain ? "" : "(none)") << endl;
  if (this->Terrain)
  {
    this->Terrain->PrintSelf(os, indent.GetNextIndent());
  }
}

