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

vtkCxxRevisionMacro(vtkTextActor, "1.3");
vtkStandardNewMacro(vtkTextActor);
// ----------------------------------------------------------------------------
vtkTextActor::vtkTextActor()
{
  // to remain compatible with code using vtkActor2D, we must set
  // position coord to Viewport, not Normalized Viewport
  // so...compute equivalent coords for initial position
  this->PositionCoordinate->SetCoordinateSystemToViewport();
  //
  this->AdjustedPositionCoordinate = vtkCoordinate::New();
  this->AdjustedPositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  // create default text mapper
  vtkTextMapper *mapper = vtkTextMapper::New();
  this->SetMapper(mapper);
  mapper->Delete();
  //
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
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetMapper(vtkTextMapper *mapper)
{ // this is the public method
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
  this->vtkActor2D::RenderOverlay(viewport);
  return 1;
}
// ----------------------------------------------------------------------------
int vtkTextActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int size[2];
  int fontSize=0, oldfontsize=0;

  vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
  if (!mapper)
    {
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
    this->vtkActor2D::RenderOpaqueGeometry(viewport);
    return 1;
    }

  // Check to see whether we have to rebuild everything
  if (viewport->GetMTime() > this->BuildTime ||
      ( viewport->GetVTKWindow() &&
        viewport->GetVTKWindow()->GetMTime() > this->BuildTime ) )
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
  if ( this->GetMTime() > this->BuildTime)
    {
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
      // Update all the composing objects
      // find the best size for the font
      int tempi[2];
      // use the last size as a first guess
      fontSize = mapper->GetFontSize();
      oldfontsize = fontSize;
      mapper->GetSize(viewport,tempi);
      int lineMax = (int)(size[1]*this->MaximumLineHeight
                          * mapper->GetNumberOfLines());
      
      // while the size is too small increase it
      while (tempi[1] < size[1] &&
             tempi[0] < size[0] && 
             tempi[1] < lineMax &&
             fontSize < 100)
        {
        fontSize++;
        mapper->SetFontSize(fontSize);
        mapper->GetSize(viewport,tempi);
        }
      // while the size is too large decrease it
      while ((tempi[1] > size[1] || tempi[0] > size[0] || tempi[1] > lineMax)
             && fontSize > 0)
        {
        fontSize--;
        mapper->SetFontSize(fontSize);
        mapper->GetSize(viewport,tempi);
        }

      }

      if (oldfontsize!=fontSize)
        { // don't do this after this->BuildTime.Modified(); !!!!
        this->Modified();
        }

    // now set the position of the Text
    int fpos[2];
    switch (mapper->GetJustification())
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
    switch (mapper->GetVerticalJustification())
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
  this->vtkActor2D::RenderOpaqueGeometry(viewport);
  return 1;
}
// ----------------------------------------------------------------------------
void vtkTextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
  os << indent << "MinimumSize: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
}
// ----------------------------------------------------------------------------
#define SetStuffMacro(function,param)          \
    vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper(); \
    if (!mapper) { return; }                   \
    unsigned long mtime1 = mapper->GetMTime(); \
    mapper->Set##function(param);              \
    unsigned long mtime2 = mapper->GetMTime(); \
    if (mtime1 != mtime2)                      \
      {                                        \
      this->Modified();                        \
      }

#define GetStuffMacro(function,default)        \
    vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper(); \
    if (!mapper) { return default; }           \
    return mapper->Get##function();            \

void vtkTextActor::SetInput(const char *inputString)
{
    SetStuffMacro(Input,inputString);
}
// ----------------------------------------------------------------------------
char *vtkTextActor::GetInput(void)
{
    GetStuffMacro(Input,NULL);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetFontSize(int size)
{
    SetStuffMacro(FontSize,size);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetFontSize(void)
{
    GetStuffMacro(FontSize,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetBold(int val)
{
    SetStuffMacro(Bold,val);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetBold(void)
{
    GetStuffMacro(Bold,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetItalic(int val)
{
    SetStuffMacro(Italic,val);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetItalic(void)
{
    GetStuffMacro(Italic,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetShadow(int val)
{
    SetStuffMacro(Shadow,val);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetShadow(void)
{
    GetStuffMacro(Shadow,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetFontFamily(int val)
{
    SetStuffMacro(FontFamily,val);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetFontFamily(void)
{
    GetStuffMacro(FontFamily,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetJustification(int val)
{
    SetStuffMacro(Justification,val);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetJustification(void)
{
    GetStuffMacro(Justification,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetVerticalJustification(int val)
{
    SetStuffMacro(VerticalJustification,val);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetVerticalJustification(void)
{
    GetStuffMacro(VerticalJustification,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetLineOffset(float val)
{
    SetStuffMacro(LineOffset,val);
}
// ----------------------------------------------------------------------------
float vtkTextActor::GetLineOffset(void)
{
    GetStuffMacro(LineOffset,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::SetLineSpacing(float val)
{
    SetStuffMacro(LineSpacing,val);
}
// ----------------------------------------------------------------------------
float vtkTextActor::GetLineSpacing(void)
{
    GetStuffMacro(LineSpacing,0);
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetNumberOfLines(const char *input)
{
    vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
    if (mapper)
      {
      return mapper->GetNumberOfLines(input);
      }
    else
      {
      return 0;
      }
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetNumberOfLines(void)
{
    GetStuffMacro(NumberOfLines,0);
}
// ----------------------------------------------------------------------------
void vtkTextActor::GetSize(vtkViewport *v, int size[2])
{
    vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
    if (mapper)
      {
      mapper->GetSize(v, size);
      }
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetWidth(vtkViewport*v)
{
    vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
    if (mapper)
      {
      return mapper->GetWidth(v);
      }
    else
      {
      return 0;
      }
}
// ----------------------------------------------------------------------------
int vtkTextActor::GetHeight(vtkViewport*v)
{
    vtkTextMapper *mapper = (vtkTextMapper *)this->GetMapper();
    if (mapper)
      {
      return mapper->GetHeight(v);
      }
    else
      {
      return 0;
      }
}

