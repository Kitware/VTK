/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedRepresentation.cxx

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

#include "vtkRenderedRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkRenderedRepresentation, "1.3");
vtkStandardNewMacro(vtkRenderedRepresentation);

class vtkRenderedRepresentation::Internals
{
public:
  // Convenience vectors for storing props to add/remove until the next render,
  // where they are added/removed by PrepareForRendering().
  vtkstd::vector<vtkSmartPointer<vtkProp> > PropsToAdd;
  vtkstd::vector<vtkSmartPointer<vtkProp> > PropsToRemove;
};


vtkRenderedRepresentation::vtkRenderedRepresentation()
{
  this->Implementation = new Internals();
  this->LabelRenderMode = vtkRenderView::FREETYPE;
}

vtkRenderedRepresentation::~vtkRenderedRepresentation()
{
  delete this->Implementation;
}

void vtkRenderedRepresentation::AddPropOnNextRender(vtkProp* p)
{
  this->Implementation->PropsToAdd.push_back(p);
}

void vtkRenderedRepresentation::RemovePropOnNextRender(vtkProp* p)
{
  this->Implementation->PropsToRemove.push_back(p);
}

void vtkRenderedRepresentation::PrepareForRendering(vtkRenderView* view)
{
  // Add props scheduled to be added on next render.
  for (size_t i = 0; i < this->Implementation->PropsToAdd.size(); ++i)
    {
    view->GetRenderer()->AddViewProp(this->Implementation->PropsToAdd[i]);
    }
  this->Implementation->PropsToAdd.clear();

  // Remove props scheduled to be removed on next render.
  for (size_t i = 0; i < this->Implementation->PropsToRemove.size(); ++i)
    {
    view->GetRenderer()->RemoveViewProp(this->Implementation->PropsToRemove[i]);
    }
  this->Implementation->PropsToRemove.clear();
}

void vtkRenderedRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LabelRenderMode: " << this->LabelRenderMode << endl;
}
