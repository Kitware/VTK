/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextActor.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

vtkCxxRevisionMacro(vtkTextActor, "1.10");
vtkStandardNewMacro(vtkTextActor);

vtkCxxSetObjectMacro(vtkTextActor,TextProperty,vtkTextProperty);

// ----------------------------------------------------------------------------
vtkTextActor::vtkTextActor()
{
  // To remain compatible with code using vtkActor2D, we must set
  // position coord to Viewport, not Normalized Viewport
  // so...compute equivalent coords for initial position
  this->PositionCoordinate->SetCoordinateSystemToViewport();
  
  this->AdjustedPositionCoordinate = vtkCoordinate::New();
  this->AdjustedPositionCoordinate->SetCoordinateSystemToNormalizedViewport();

  // Create default text mapper
  vtkTextMapper *mapper = vtkTextMapper::New();
  this->SetMapper(mapper);
  mapper->Delete();
  
  this->TextProperty = vtkTextProperty::New();

  this->LastOrigin[0]     = 0;
  this->LastOrigin[1]     = 0;

  this->LastSize[0]       = 0;
  this->LastSize[1]       = 0;

  this->MinimumSize[0]    = 10;
  this->MinimumSize[1]    = 10;

  this->MaximumLineHeight = 1.0;
  this->ScaledText        = 0;
  this->AlignmentPoint    = 0;
}

// ----------------------------------------------------------------------------
vtkTextActor::~vtkTextActor()
{
  this->AdjustedPositionCoordinate->Delete();
  this->SetTextProperty(NULL);
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetMapper(vtkTextMapper *mapper)
{
  this->vtkActor2D::SetMapper( mapper );
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetMapper(vtkMapper2D *mapper)
{
  if (mapper->IsA("vtkTextMapper"))
    {
    this->SetMapper( (vtkTextMapper *)mapper );
    }
  else
    {
    vtkErrorMacro("Must use vtkTextMapper for this class");
    }
  }

// ----------------------------------------------------------------------------
void vtkTextActor::SetInput(const char* input)
{
  vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
  if (!mapper) 
    { 
    vtkErrorMacro("Actor has not vtkTextMapper");
    }
  mapper->SetInput(input);
}

// ----------------------------------------------------------------------------
char* vtkTextActor::GetInput()
{
  vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
  if (!mapper) 
    { 
    vtkErrorMacro("Actor has not vtkTextMapper");
    }
  return mapper->GetInput();
}

// ----------------------------------------------------------------------------
void vtkTextActor::ShallowCopy(vtkProp *prop)
{
  vtkTextActor *a = vtkTextActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetPosition2(a->GetPosition2());
    this->SetMinimumSize(a->GetMinimumSize());
    this->SetMaximumLineHeight(a->GetMaximumLineHeight());
    this->SetScaledText(a->GetScaledText());
    this->SetAlignmentPoint(a->GetAlignmentPoint());
    this->SetTextProperty(a->GetTextProperty());
    }
  // Now do superclass (mapper is handled by it as well).
  this->vtkActor2D::ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkTextActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->vtkActor2D::ReleaseGraphicsResources(win);
}

// ----------------------------------------------------------------------------
int vtkTextActor::RenderOverlay(vtkViewport *viewport)
{
  // Everything is built in RenderOpaqueGeometry, just have to render
  return this->vtkActor2D::RenderOverlay(viewport);
}

// ----------------------------------------------------------------------------
int vtkTextActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int size[2];

  vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
  if (!mapper)
    {
    return 0;
    }

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render text actor");
    return 0;
    }

  // we don't need to do anything additional, just pass the call
  // right through to the actor
  if (!this->ScaledText)
    {
    int *point1, *point2;
    float u, v;
    point1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    point2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    size[0] = point2[0] - point1[0];
    size[1] = point2[1] - point1[1];
    switch (this->AlignmentPoint)
      {
      case 0:
        u = point1[0];
        v = point1[1];
        break;
      case 1:
        u = point1[0] + size[0]/2;
        v = point1[1];
        break;
      case 2:
        u = point2[0];
        v = point1[1];
        break;
      case 3:
        u = point1[0];
        v = point1[1] + size[1]/2;
        break;
      case 4:
        u = point1[0] + size[0]/2;
        v = point1[1] + size[1]/2;
        break;
      case 5:
        u = point2[0];
        v = point1[1] + size[1]/2;
        break;
      case 6:
        u = point1[0];
        v = point2[1];
        break;
      case 7:
        u = point1[0] + size[0]/2;
        v = point2[1];
        break;
      case 8:
        u = point2[0];
        v = point2[1];
        break;
      }

    viewport->ViewportToNormalizedViewport(u, v);
    this->AdjustedPositionCoordinate->SetValue(u,v);
    return this->vtkActor2D::RenderOpaqueGeometry(viewport);
    }

  // Check to see whether we have to rebuild everything
  if (viewport->GetMTime() > this->BuildTime ||
      (viewport->GetVTKWindow() &&
       viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
    {
    // if the viewport has changed we may - or may not need
    // to rebuild, it depends on if the projected coords change
    int *point1, *point2;
    point1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    point2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    size[0] = point2[0] - point1[0];
    size[1] = point2[1] - point1[1];
    if (this->LastSize[0] != size[0]     || this->LastSize[1] != size[1] ||
        this->LastOrigin[0] != point1[0] || this->LastOrigin[1] != point1[1])
      {
      this->Modified();
      }
    }
  
  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime ||
      mapper->GetMTime() > this->BuildTime ||
      tprop->GetMTime() > this->BuildTime)
    {
    if (tprop->GetMTime() > this->BuildTime)
      {
      // Shallow copy here so that the size of the text prop is not affected
      // by the automatic adjustment of its text mapper's size (i.e. its
      // mapper's text property is identical except for the font size
      // which will be modified later). This allows text actors to
      // share the same text property.
      vtkTextProperty *tproptemp = mapper->GetTextProperty();
      if (tproptemp)
        {
        tproptemp->ShallowCopy(tprop);
        }
      }
    vtkDebugMacro(<<"Rebuilding text");

    // get the viewport size in display coordinates
    int *point1, *point2;
    point1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    point2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    size[0] = point2[0] - point1[0];
    size[1] = point2[1] - point1[1];
    this->LastOrigin[0] = point1[0];
    this->LastOrigin[1] = point1[1];

    //  Lets try to minimize the number of times we change the font size.
    //  If the width of the font box has not changed by more than a pixel
    // (numerical issues)  do not recompute font size.
    if (this->LastSize[0] < size[0]-1 || this->LastSize[1] < size[1]-1 ||
        this->LastSize[0] > size[0]+1 || this->LastSize[1] > size[1]+1)
      {
      this->LastSize[0] = size[0];
      this->LastSize[1] = size[1];
      
      // limit by minimum size
      if (this->MinimumSize[0] > size[0])
        {
        size[0] = this->MinimumSize[0];
        }
      if (this->MinimumSize[1] > size[1])
        {
        size[1] = this->MinimumSize[1];
        }    
      int max_height = (int)(this->MaximumLineHeight * (float)size[1]);

      mapper->SetConstrainedFontSize(
        viewport, 
        size[0], 
        (size[1] < max_height ? size[1] : max_height));
      }
    
    // now set the position of the Text
    int fpos[2];
    switch (tprop->GetJustification())
      {
      case VTK_TEXT_LEFT:
        fpos[0] = point1[0];
        break;
      case VTK_TEXT_CENTERED:
        fpos[0] = point1[0] + size[0]/2;
        break;
      case VTK_TEXT_RIGHT:
        fpos[0] = point1[0]+size[0];
        break;
      }
    switch (tprop->GetVerticalJustification())
      {
      case VTK_TEXT_TOP:
        fpos[1] = point1[1] + size[1];
        break;
      case VTK_TEXT_CENTERED:
        fpos[1] = point1[1] + size[1]/2;
        break;
      case VTK_TEXT_BOTTOM:
        fpos[1] = point1[1];
        break;
      }

    float u = fpos[0], v = fpos[1];
    viewport->ViewportToNormalizedViewport(u, v);
    this->AdjustedPositionCoordinate->SetValue(u,v);
    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  return this->vtkActor2D::RenderOpaqueGeometry(viewport);
}

// ----------------------------------------------------------------------------
void vtkTextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TextProperty)
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }

  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
  os << indent << "MinimumSize: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
  os << indent << "ScaledText: " << this->ScaledText << endl;
  os << indent << "AlignmentPoint: " << this->AlignmentPoint << endl;
}
