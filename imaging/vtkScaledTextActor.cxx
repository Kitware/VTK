/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkScaledTextActor.h"

vtkScaledTextActor::vtkScaledTextActor()
{
  this->Position2Coordinate = vtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.6, 0.1);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);
  
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.2,0.85);

  this->TextActor = vtkActor2D::New();
  this->LastOrigin[0] = 0;
  this->LastOrigin[1] = 0;
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;
}

vtkScaledTextActor::~vtkScaledTextActor()
{
  this->TextActor->Delete();
  this->Position2Coordinate->Delete();
  this->Position2Coordinate = NULL;
}

void vtkScaledTextActor::SetWidth(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(w,pos[1]);
}

void vtkScaledTextActor::SetHeight(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(pos[0],w);
}
    
float vtkScaledTextActor::GetWidth()
{
  return this->Position2Coordinate->GetValue()[0];
}
float vtkScaledTextActor::GetHeight()
{
  return this->Position2Coordinate->GetValue()[1];
}

int vtkScaledTextActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;
  
  // Everything is built, just have to render
  this->TextActor->RenderOverlay(viewport);
  return 1;
}

int vtkScaledTextActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;
  int i;
  int size[2];
  int stringHeight, stringWidth;
  int fontSize;
  
  if ( ! this->TextActor->GetMapper() )
    {
    vtkWarningMacro(<<"Need a text mapper to render");
    return 0;
    }

  // Check to see whether we have to rebuild everything
  if (viewport->GetMTime() > this->BuildTime ||
      ( viewport->GetVTKWindow() && 
        viewport->GetVTKWindow()->GetMTime() > this->BuildTime ) )
    {
    // if the viewport has changed we may - or may not need
    // to rebuild, it depends on if the projected coords chage
    int *textOrigin, textWidth, textHeight;
    textOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      textOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      textOrigin[1];
    if (this->LastSize[0] != size[0] || this->LastSize[1] != size[1] ||
	this->LastOrigin[0] != textOrigin[0] || 
	this->LastOrigin[1] != textOrigin[1])
      {
      this->Modified();
      }
    }
  
  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding text");
    
    // get the viewport size in display coordinates
    int *textOrigin, textWidth, textHeight;
    textOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      textOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      textOrigin[1];
    this->LastOrigin[0] = textOrigin[0];
    this->LastOrigin[1] = textOrigin[1];
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
    
    // Update all the composing objects
    // find the best size for the font
    int tempi[2];
    // use the last size as a first guess
    vtkTextMapper *tMapper = (vtkTextMapper *)this->TextActor->GetMapper();
    fontSize = tMapper->GetFontSize();
    tMapper->GetSize(viewport,tempi);
    // while the size is too small increase it
    while (tempi[1] < size[1] && tempi[0] < size[0] && fontSize < 100)
      {
      fontSize++;
      tMapper->SetFontSize(fontSize);
      tMapper->GetSize(viewport,tempi);
      }
    // while the size is too large decrease it
    while ((tempi[1] > size[1] || tempi[0] > size[0])&& fontSize > 0)
      {
      fontSize--;
      tMapper->SetFontSize(fontSize);
      tMapper->GetSize(viewport,tempi);
      }
    
    // now set the position of the TextActor
    int fpos[2];
    switch (tMapper->GetJustification())
      {
      case VTK_TEXT_LEFT:
        fpos[0] = textOrigin[0];
        break;
      case VTK_TEXT_CENTERED:
        fpos[0] = textOrigin[0] + size[0]/2;
        break;
      case VTK_TEXT_RIGHT:
        fpos[0] = textOrigin[0]+size[0];        
        break;
      }
    switch (tMapper->GetVerticalJustification())
      {
      case VTK_TEXT_TOP:
        fpos[1] = textOrigin[1] + size[1];
        break;
      case VTK_TEXT_CENTERED:
        fpos[1] = textOrigin[1] + size[1]/2;
        break;
      case VTK_TEXT_BOTTOM:
        fpos[1] = textOrigin[1];        
        break;
      }
        
    this->TextActor->SetPosition(fpos[0],fpos[1]);
    this->TextActor->SetProperty(this->GetProperty());
    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  this->TextActor->RenderOpaqueGeometry(viewport);
  return 1;
}

void vtkScaledTextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

}

vtkCoordinate *vtkScaledTextActor::GetPosition2Coordinate() 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Position2Coordinate address " << this->Position2Coordinate ); 
  return this->Position2Coordinate; 
} 
void vtkScaledTextActor::SetPosition2(float x[2]) 
{
  this->SetPosition2(x[0],x[1]);
} 
void vtkScaledTextActor::SetPosition2(float x, float y) 
{ 
  this->Position2Coordinate->SetCoordinateSystem(VTK_VIEWPORT); 
  this->Position2Coordinate->SetValue(x,y); 
} 
float *vtkScaledTextActor::GetPosition2() 
{ 
  return this->Position2Coordinate->GetValue(); 
}

void vtkScaledTextActor::SetMapper(vtkTextMapper *mapper)
{
  this->TextActor->SetMapper(mapper);
}
