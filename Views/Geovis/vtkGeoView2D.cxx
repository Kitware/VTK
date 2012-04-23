/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoView2D.cxx

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
#include "vtkGeoView2D.h"

#include "vtkAssembly.h"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoTerrain2D.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkViewTheme.h"

vtkStandardNewMacro(vtkGeoView2D);
vtkCxxSetObjectMacro(vtkGeoView2D, Surface, vtkGeoTerrain2D);

vtkGeoView2D::vtkGeoView2D()
{
  this->Surface = 0;
  this->SetInteractionModeTo2D();
  this->Assembly = vtkAssembly::New();
  this->Renderer->AddActor(this->Assembly);
  this->SetSelectionModeToFrustum();
}

vtkGeoView2D::~vtkGeoView2D()
{
  this->SetSurface(0);
  if (this->Assembly)
    {
    this->Assembly->Delete();
    }
}

vtkAbstractTransform* vtkGeoView2D::GetTransform()
{
  if (this->Surface)
    {
    return this->Surface->GetTransform();
    }
  return 0;
}

void vtkGeoView2D::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Assembly: " << this->Assembly << "\n";
  os << indent << "Surface: " << this->Surface << "\n";
}

void vtkGeoView2D::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->GradientBackgroundOn();
}

void vtkGeoView2D::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  if (!this->Surface)
    {
    return;
    }
  vtkSmartPointer<vtkCollection> collection =
    vtkSmartPointer<vtkCollection>::New();
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkDataRepresentation* r = this->GetRepresentation(i);
    vtkGeoAlignedImageRepresentation* img = vtkGeoAlignedImageRepresentation::SafeDownCast(r);
    if (img)
      {
      collection->AddItem(img);
      }
    }
  if (collection->GetNumberOfItems() > 0)
    {
    this->Surface->AddActors(this->Renderer, this->Assembly, collection);
    }
}

void vtkGeoView2D::Render()
{
  // If this is the first time, render an extra time to get things
  // initialized for the first PrepareForRendering pass.
  this->RenderWindow->MakeCurrent();
  if (!this->RenderWindow->IsCurrent())
    {
    this->Update();
    this->PrepareForRendering();
    this->RenderWindow->Render();
    }
  this->Superclass::Render();
}
