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
#include "vtkGeoAlignedImage.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageCache.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoCamera.h"
#include "vtkGeoGraphRepresentation.h"
#include "vtkGeoInteractorStyle.h"
#include "vtkGeoLineRepresentation.h"
#include "vtkGlobeSource.h"
#include "vtkLight.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkViewTheme.h"
#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New();

vtkCxxRevisionMacro(vtkGeoView, "1.1");
vtkStandardNewMacro(vtkGeoView);
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

  // Make an actor that is a low resolution earth.  This is simply to hide
  // geometry on the other side of the earth when picking.  The actor in
  // vtkGeoAlignedImageRepresentation is not rendered during visible cell
  // selection because it is a vtkAssembly.
  this->LowResEarthSource       = vtkGlobeSource::New();
  this->LowResEarthMapper       = vtkPolyDataMapper::New();
  this->LowResEarthActor        = vtkActor::New();
  this->LowResEarthSource->SetStartLatitude(-90.0);
  this->LowResEarthSource->SetEndLatitude(90.0);
  this->LowResEarthSource->SetStartLongitude(-180.0);
  this->LowResEarthSource->SetEndLongitude(180.0);
  this->LowResEarthMapper->SetInputConnection(this->LowResEarthSource->GetOutputPort());
  this->LowResEarthActor->SetMapper(this->LowResEarthMapper);
  // Make it slightly smaller than the earth so it is not visible
  this->LowResEarthActor->SetScale(0.95); 
  
  this->RenderWindow = 0;
}

//----------------------------------------------------------------------------
vtkGeoView::~vtkGeoView()
{
  for (int i = 0; i < this->GetNumberOfRepresentations(); i++)
    {
    vtkGeoAlignedImageRepresentation* imageRep = 
      vtkGeoAlignedImageRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (imageRep)
      {
      imageRep->ExitCleanup();
      }
    }
  
  this->LowResEarthSource->Delete();
  this->LowResEarthMapper->Delete();
  this->LowResEarthActor->Delete();

  // Important: Delete the render window AFTER calling ExitCleanup on all
  // image representations.
  if (this->RenderWindow)
    {
    this->RenderWindow->Delete();
    }
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
  vtkGeoInteractorStyle* style = vtkGeoInteractorStyle::SafeDownCast(
    this->GetInteractorStyle());
  vtkGeoCamera* cam = style->GetGeoCamera();
  int* rendererSize = this->Renderer->GetSize();
  cam->InitializeNodeAnalysis(rendererSize);
  
  for (int i = 0; i < this->GetNumberOfRepresentations(); i++)
    {
    vtkGeoAlignedImageRepresentation* imageRep = 
      vtkGeoAlignedImageRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (imageRep)
      {
      imageRep->Update(cam);
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
}

//----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation* vtkGeoView::AddDefaultImageRepresentation(const char* filename)
{
  VTK_CREATE(vtkGeoTerrain, terrain);
  VTK_CREATE(vtkGeoAlignedImage, image);
  VTK_CREATE(vtkGeoAlignedImageSource, imageSource);
  
  imageSource->LoadImage(filename);
  
  VTK_CREATE(vtkGeoAlignedImageCache, imageCache);
  imageCache->SetSource(imageSource);
  image->SetCache(imageCache);
  
  VTK_CREATE(vtkGeoAlignedImageRepresentation, rep);
  rep->SetImage(image);
  rep->SetTerrain(terrain);
  rep->Update(0);
  
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
void vtkGeoView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

