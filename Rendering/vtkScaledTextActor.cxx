/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkScaledTextActor.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkScaledTextActor* vtkScaledTextActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkScaledTextActor");
  if(ret)
    {
    return (vtkScaledTextActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkScaledTextActor;
}

vtkScaledTextActor::vtkScaledTextActor()
{
  this->Position2Coordinate->SetValue(0.6, 0.1);
  
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.2,0.85);

  this->TextActor = vtkActor2D::New();
  this->LastOrigin[0] = 0;
  this->LastOrigin[1] = 0;
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;

  this->MinimumSize[0] = 10;
  this->MinimumSize[1] = 10;  

  this->MaximumLineHeight = 1.0;
}

vtkScaledTextActor::~vtkScaledTextActor()
{
  this->TextActor->Delete();
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkScaledTextActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->vtkActor2D::ReleaseGraphicsResources(win);
  this->TextActor->ReleaseGraphicsResources(win);
}

int vtkScaledTextActor::RenderOverlay(vtkViewport *viewport)
{
  // Everything is built, just have to render
  this->TextActor->RenderOverlay(viewport);
  return 1;
}

int vtkScaledTextActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int size[2];
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
    int *textOrigin;
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
  
  vtkTextMapper *tMapper = (vtkTextMapper *)this->TextActor->GetMapper();
  
  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime ||
       this->TextActor->GetMapper()->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding text");
    
    // get the viewport size in display coordinates
    int *textOrigin;
    textOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      textOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      textOrigin[1];
    this->LastOrigin[0] = textOrigin[0];
    this->LastOrigin[1] = textOrigin[1];

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
      fontSize = tMapper->GetFontSize();
      tMapper->GetSize(viewport,tempi);
      int lineMax = (int)(size[1]*this->MaximumLineHeight
			  * tMapper->GetNumberOfLines());
      
      // while the size is too small increase it
      while (tempi[1] < size[1] && 
	     tempi[0] < size[0] && 
	     tempi[1] < lineMax &&
	     fontSize < 100)
	{
	fontSize++;
	tMapper->SetFontSize(fontSize);
	tMapper->GetSize(viewport,tempi);
	}
      // while the size is too large decrease it
      while ((tempi[1] > size[1] || tempi[0] > size[0] || tempi[1] > lineMax)
	     && fontSize > 0)
	{
	fontSize--;
	tMapper->SetFontSize(fontSize);
	tMapper->GetSize(viewport,tempi);
	}
      
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

  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
  os << indent << "MinimumSize: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
}

void vtkScaledTextActor::SetMapper(vtkTextMapper *mapper)
{
  this->TextActor->SetMapper(mapper);
}

void vtkScaledTextActor::ShallowCopy(vtkProp *prop)
{
  vtkScaledTextActor *a = vtkScaledTextActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetPosition2(a->GetPosition2());
    this->SetMinimumSize(a->GetMinimumSize());
    this->SetMaximumLineHeight(a->GetMaximumLineHeight());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
