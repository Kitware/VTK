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
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoCamera.h"
#include "vtkGeoGlobeSource.h"
#include "vtkGeoGraphRepresentation.h"
#include "vtkGeoInteractorStyle.h"
#include "vtkGeoLineRepresentation.h"
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

vtkCxxRevisionMacro(vtkGeoView, "1.12");
vtkStandardNewMacro(vtkGeoView);
vtkCxxSetObjectMacro(vtkGeoView, Terrain, vtkGeoTerrain);
//----------------------------------------------------------------------------
vtkGeoView::vtkGeoView()
{
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
  this->LowResEarthActor        = vtkActor::New();
  this->LowResEarthSource       = NULL; // BuildLowResEarth tests if the source is null.
  this->BuildLowResEarth( cam->GetOrigin() ); // call once the mapper is set!
  this->LowResEarthActor->SetMapper(this->LowResEarthMapper);
  this->RenderWindow = 0;
  this->Terrain = 0;

  // Perform frustum selection by default
  this->SetSelectionModeToFrustum();

  // Add the assembly to the view.
  this->Assembly = vtkAssembly::New();
  this->Renderer->AddActor(this->Assembly);
}

//----------------------------------------------------------------------------
vtkGeoView::~vtkGeoView()
{
  this->LowResEarthSource->Delete();
  this->LowResEarthMapper->Delete();
  this->LowResEarthActor->Delete();
  this->Assembly->Delete();
  this->SetTerrain(0);

  if (this->RenderWindow)
    {
    this->RenderWindow->Delete();
    }
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
  

//----------------------------------------------------------------------------
void vtkGeoView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);

  if (win->GetRenderers()->GetFirstRenderer())
    {
    win->GetRenderers()->GetFirstRenderer()->AddActor(this->LowResEarthActor);
    }
  
  // We must keep a reference to the render window so we can call
  // ExitCleanup() before it gets deleted.
  if (win)
    {
    this->RenderWindow = win;
    this->RenderWindow->Register(this);
    }
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
    vtkGeoLineRepresentation* lineRep =
      vtkGeoLineRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (lineRep)
      {
      lineRep->PrepareForRendering();
      }
    vtkGeoGraphRepresentation* graphRep =
      vtkGeoGraphRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (graphRep)
      {
      graphRep->PrepareForRendering();
      }
    }

  if (this->Terrain)
    {
    this->Terrain->AddActors(this->Renderer, this->Assembly, imageReps);
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
void vtkGeoView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->GradientBackgroundOn();
}

//----------------------------------------------------------------------------
vtkGeoInteractorStyle* vtkGeoView::GetGeoInteractorStyle()
{
  return vtkGeoInteractorStyle::SafeDownCast(this->GetInteractorStyle());
}

//----------------------------------------------------------------------------
void vtkGeoView::SetGeoInteractorStyle(vtkGeoInteractorStyle* style)
{
  if(style && style != this->InteractorStyle)
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

