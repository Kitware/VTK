/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedButtonRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTexturedButtonRepresentation2D.h"
#include "vtkBalloonRepresentation.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "vtkCoordinate.h"
#include <map>

vtkStandardNewMacro(vtkTexturedButtonRepresentation2D);

vtkCxxSetObjectMacro(vtkTexturedButtonRepresentation2D,Property,vtkProperty2D);
vtkCxxSetObjectMacro(vtkTexturedButtonRepresentation2D,HoveringProperty,vtkProperty2D);
vtkCxxSetObjectMacro(vtkTexturedButtonRepresentation2D,SelectingProperty,vtkProperty2D);

// Map of textures
class vtkTextureArray : public std::map<int,vtkSmartPointer<vtkImageData> > {};
typedef std::map<int,vtkSmartPointer<vtkImageData> >::iterator vtkTextureArrayIterator;


//----------------------------------------------------------------------
vtkTexturedButtonRepresentation2D::vtkTexturedButtonRepresentation2D()
{
  // Configure the balloon
  this->Balloon = vtkBalloonRepresentation::New();
  this->Balloon->SetOffset(0,0);

  // Set up the initial properties
  this->CreateDefaultProperties();

  // List of textures
  this->TextureArray = new vtkTextureArray;

  // Anchor point assuming that the button is anchored in 3D
  // If NULL, then the placement occurs in display space
  this->Anchor = NULL;

}

//----------------------------------------------------------------------
vtkTexturedButtonRepresentation2D::~vtkTexturedButtonRepresentation2D()
{
  this->Balloon->Delete();

  if ( this->Property )
  {
    this->Property->Delete();
    this->Property = NULL;
  }

  if ( this->HoveringProperty )
  {
    this->HoveringProperty->Delete();
    this->HoveringProperty = NULL;
  }

  if ( this->SelectingProperty )
  {
    this->SelectingProperty->Delete();
    this->SelectingProperty = NULL;
  }

  delete this->TextureArray;

  if ( this->Anchor )
  {
    this->Anchor->Delete();
  }

}

//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::
SetButtonTexture(int i, vtkImageData *image)
{
  if ( i < 0 )
  {
    i = 0;
  }
  if ( i >= this->NumberOfStates )
  {
    i = this->NumberOfStates - 1;
  }

  (*this->TextureArray)[i] = image;
}


//-------------------------------------------------------------------------
vtkImageData *vtkTexturedButtonRepresentation2D::
GetButtonTexture(int i)
{
  if ( i < 0 )
  {
    i = 0;
  }
  if ( i >= this->NumberOfStates )
  {
    i = this->NumberOfStates - 1;
  }

  vtkTextureArrayIterator iter = this->TextureArray->find(i);
  if ( iter != this->TextureArray->end() )
  {
    return (*iter).second;
  }
  else
  {
    return NULL;
  }
}

//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);
  for (i=0; i<6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  if ( this->Anchor )
  {//no longer in world space
    this->Anchor->Delete();
    this->Anchor = NULL;
  }

  double e[2];
  e[0] = static_cast<double>(bounds[0]);
  e[1] = static_cast<double>(bounds[2]);
  this->Balloon->StartWidgetInteraction(e);
  this->Balloon->SetImageSize(static_cast<int>(bounds[1]-bounds[0]),
                              static_cast<int>(bounds[3]-bounds[2]));
}

//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::PlaceWidget(double anchor[3], int size[2])
{
  if ( ! this->Anchor )
  {
    this->Anchor = vtkCoordinate::New();
    this->Anchor->SetCoordinateSystemToWorld();
  }

  this->Anchor->SetValue(anchor);

  double e[2]; e[0] = e[1] = 0.0;
  if ( this->Renderer )
  {
    double *p = this->Anchor->GetComputedDoubleDisplayValue(this->Renderer);
    this->Balloon->SetRenderer(this->Renderer);
    this->Balloon->StartWidgetInteraction(p);
    e[0] = static_cast<double>(p[0]);
    e[1] = static_cast<double>(p[1]);
  }
  else
  {
    this->Balloon->StartWidgetInteraction(e);
  }

  this->Balloon->SetImageSize(size);

  this->InitialBounds[0] = e[0];
  this->InitialBounds[1] = e[0]+size[0];
  this->InitialBounds[2] = e[1];
  this->InitialBounds[3] = e[1]+size[1];
  this->InitialBounds[4] = this->InitialBounds[5] = 0.0;

  double *bounds = this->InitialBounds;
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
}


//-------------------------------------------------------------------------
int vtkTexturedButtonRepresentation2D
::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->Balloon->SetRenderer(this->GetRenderer());
  if ( this->Balloon->ComputeInteractionState(X,Y) == vtkBalloonRepresentation::OnImage )
  {
    this->InteractionState = vtkButtonRepresentation::Inside;
  }
  else
  {
    this->InteractionState = vtkButtonRepresentation::Outside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::Highlight(int highlight)
{
  this->Superclass::Highlight(highlight);

  vtkProperty2D *initialProperty = this->Balloon->GetImageProperty();
  vtkProperty2D *selectedProperty;

  if ( highlight == vtkButtonRepresentation::HighlightHovering )
  {
    this->Balloon->SetImageProperty(this->HoveringProperty);
    selectedProperty = this->HoveringProperty;
  }
  else if ( highlight == vtkButtonRepresentation::HighlightSelecting )
  {
    this->Balloon->SetImageProperty(this->SelectingProperty);
    selectedProperty = this->SelectingProperty;
  }
  else //if ( highlight == vtkButtonRepresentation::HighlightNormal )
  {
    this->Balloon->SetImageProperty(this->Property);
    selectedProperty = this->Property;
  }

  if ( selectedProperty != initialProperty )
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::CreateDefaultProperties()
{
  this->Property = vtkProperty2D::New();
  this->Property->SetColor(0.9,0.9,0.9);

  this->HoveringProperty = vtkProperty2D::New();
  this->HoveringProperty->SetColor(1,1,1);

  this->SelectingProperty = vtkProperty2D::New();
  this->SelectingProperty->SetColor(0.5,0.5,0.5);
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer &&
        this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime) ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
  {
    this->Balloon->SetRenderer(this->Renderer);

    // Setup the texture
    vtkTextureArrayIterator iter = this->TextureArray->find(this->State);
    if ( iter != this->TextureArray->end() )
    {
      this->Balloon->SetBalloonImage((*iter).second);
    }
    else
    {
      this->Balloon->SetBalloonImage(NULL);
    }

    // Update the position if anchored in world coordinates
    if ( this->Anchor )
    {
      double *p = this->Anchor->GetComputedDoubleDisplayValue(this->Renderer);
      this->Balloon->StartWidgetInteraction(p);
      this->Balloon->Modified();
    }

    this->BuildTime.Modified();
  }
}


//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::ShallowCopy(vtkProp *prop)
{
  vtkTexturedButtonRepresentation2D *rep =
    vtkTexturedButtonRepresentation2D::SafeDownCast(prop);
  if ( rep )
  {
    this->Property->DeepCopy(rep->Property);
    this->HoveringProperty->DeepCopy(rep->HoveringProperty);
    this->SelectingProperty->DeepCopy(rep->SelectingProperty);

    vtkTextureArrayIterator iter;
    for ( iter=rep->TextureArray->begin();
          iter != rep->TextureArray->end(); ++iter )
    {
      (*this->TextureArray)[(*iter).first] = (*iter).second;
    }
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::
ReleaseGraphicsResources(vtkWindow *win)
{
  this->Balloon->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkTexturedButtonRepresentation2D::
RenderOverlay(vtkViewport *viewport)
{
  this->BuildRepresentation();

  return this->Balloon->RenderOverlay(viewport);
}

//-----------------------------------------------------------------------------
int vtkTexturedButtonRepresentation2D::
HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  return this->Balloon->HasTranslucentPolygonalGeometry();
}


//----------------------------------------------------------------------
double *vtkTexturedButtonRepresentation2D::GetBounds()
{
  return NULL;
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->Balloon->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  if ( this->Property )
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if ( this->HoveringProperty )
  {
    os << indent << "Hovering Property: " << this->HoveringProperty << "\n";
  }
  else
  {
    os << indent << "Hovering Property: (none)\n";
  }

  if ( this->SelectingProperty )
  {
    os << indent << "Selecting Property: " << this->SelectingProperty << "\n";
  }
  else
  {
    os << indent << "Selecting Property: (none)\n";
  }
}
