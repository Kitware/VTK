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
#include "vtkGeoGraphRepresentation2D.h"
#include "vtkGeoTerrain2D.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkViewTheme.h"

vtkCxxRevisionMacro(vtkGeoView2D, "1.3");
vtkStandardNewMacro(vtkGeoView2D);
vtkCxxSetObjectMacro(vtkGeoView2D, Surface, vtkGeoTerrain2D);

vtkGeoView2D::vtkGeoView2D()
{
  this->Surface = 0;
  vtkSmartPointer<vtkInteractorStyleRubberBand2D> style =
    vtkSmartPointer<vtkInteractorStyleRubberBand2D>::New();
  this->SetInteractorStyle(style);
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
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

void vtkGeoView2D::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Assembly: " << this->Assembly << "\n";
  os << indent << "Surface: " << this->Surface << "\n";
}

void vtkGeoView2D::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->GradientBackgroundOn();
}

void vtkGeoView2D::PrepareForRendering()
{
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

void vtkGeoView2D::AddRepresentationInternal(vtkDataRepresentation* rep)
{
  vtkGeoGraphRepresentation2D* graphRep = vtkGeoGraphRepresentation2D::SafeDownCast(rep);
  // Make sure the transform of the graph representation matches that of the view.
  if (graphRep && this->Surface && this->Surface->GetTransform())
    {
    graphRep->SetTransform(this->Surface->GetTransform());
    }
}

