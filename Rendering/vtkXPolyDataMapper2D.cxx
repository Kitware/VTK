/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXPolyDataMapper2D.cxx
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
#include <stdlib.h>
#include <math.h>
#include "vtkXPolyDataMapper2D.h"
#include "vtkXImageWindow.h"
#include "vtkObjectFactory.h"


#ifndef VTK_REMOVE_LEGACY_CODE

//------------------------------------------------------------------------------
vtkXPolyDataMapper2D* vtkXPolyDataMapper2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXPolyDataMapper2D");
  if(ret)
    {
    return (vtkXPolyDataMapper2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXPolyDataMapper2D;
}




void vtkXPolyDataMapper2D::RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
{
  int numPts;
  vtkPolyData *input= (vtkPolyData *)this->Input;
  int npts, j;
  vtkPoints *p, *displayPts;
  vtkCellArray *aPrim;
  vtkScalars *c=NULL;
  unsigned char *rgba;
  int *pts;
  float *ftmp;
  int cellScalars = 0;
  int cellNum = 0;
  float tran;
  int lastX, lastY, X, Y; 
  XPoint *points = new XPoint [1024];
  int currSize = 1024;
 
  vtkDebugMacro (<< "vtkXPolyDataMapper2D::RenderOverlay");

  if ( input == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    input->Update();
    numPts = input->GetNumberOfPoints();
    } 

  if (numPts == 0)
    {
    vtkDebugMacro(<< "No points!");
    return;
    }
  
  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
    }

  //
  // if something has changed regenrate colors and display lists
  // if required
  //
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime || 
       this->LookupTable->GetMTime() > this->BuildTime ||
       actor->GetProperty()->GetMTime() > this->BuildTime)
    {
    // sets this->Colors as side effect
    this->GetColors();
    this->BuildTime.Modified();
    }

  // Get the window info
  vtkWindow*  window = viewport->GetVTKWindow();
  Display* displayId = (Display*) window->GetGenericDisplayId();
  GC gc = (GC) window->GetGenericContext();
  Window windowId = (Window) window->GetGenericWindowId();

  // Get the drawable to draw into
  Drawable drawable = (Drawable) window->GetGenericDrawable();
  if (!drawable) vtkErrorMacro(<<"Window returned NULL drawable!");
  
  // Get the position of the text actor
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedLocalDisplayValue(viewport);

  // Set up the forground color
  XWindowAttributes attr;
  XGetWindowAttributes(displayId,windowId,&attr);
  XColor aColor;
  float* actorColor = actor->GetProperty()->GetColor();
  aColor.red = (unsigned short) (actorColor[0] * 65535.0);
  aColor.green = (unsigned short) (actorColor[1] * 65535.0);
  aColor.blue = (unsigned short) (actorColor[2] * 65535.0);
  XAllocColor(displayId, attr.colormap, &aColor);
  XSetForeground(displayId, gc, aColor.pixel);
  XSetFillStyle(displayId, gc, FillSolid);
  
  tran = actor->GetProperty()->GetOpacity();

  // Transform the points, if necessary
  p = input->GetPoints();
  if ( this->TransformCoordinate )
    {
    int *itmp;
    numPts = p->GetNumberOfPoints();
    displayPts = vtkPoints::New();
    displayPts->SetNumberOfPoints(numPts);
    for ( j=0; j < numPts; j++ )
      {
      this->TransformCoordinate->SetValue(p->GetPoint(j));
      itmp = this->TransformCoordinate->GetComputedDisplayValue(viewport);
      displayPts->SetPoint(j, itmp[0], itmp[1], 0.0);
      }
    p = displayPts;
    }

  // Get colors
  if ( this->Colors )
    {
    c = this->Colors;
    c->InitColorTraversal(tran, this->LookupTable, this->ColorMode);
    if (!input->GetPointData()->GetScalars())
      {
      cellScalars = 1;
      }
    }

  aPrim = input->GetPolys();
  
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    { 
    if (c) 
      {
      // if cell scalars are present, color poly with those, otherwise
      // use the color of the first point
      if (cellScalars) 
	{
	rgba = c->GetColor(cellNum);
	}
      else
	{
	rgba = c->GetColor(pts[0]);
	}
      aColor.red = (unsigned short) (rgba[0] * 256);
      aColor.green = (unsigned short) (rgba[1] * 256);
      aColor.blue = (unsigned short) (rgba[2] * 256);
      XAllocColor(displayId, attr.colormap, &aColor);
      XSetForeground(displayId, gc, aColor.pixel);
      }
    if (npts > currSize)
      {
      delete [] points;
      points = new XPoint [npts];
      currSize = npts;
      }
    for (j = 0; j < npts; j++) 
      {
      ftmp = p->GetPoint(pts[j]);
      points[j].x = (short)(actorPos[0] + ftmp[0]);
      points[j].y = (short)(actorPos[1] - ftmp[1]);
      }
    XFillPolygon(displayId, drawable, gc, points, npts, 
		 Complex, CoordModeOrigin);
    }

  aPrim = input->GetLines();
  
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    { 
    if (c && cellScalars) 
      {
      rgba = c->GetColor(cellNum);
      aColor.red = (unsigned short) (rgba[0] * 256);
      aColor.green = (unsigned short) (rgba[1] * 256);
      aColor.blue = (unsigned short) (rgba[2] * 256);
      XAllocColor(displayId, attr.colormap, &aColor);
      XSetForeground(displayId, gc, aColor.pixel);
      }
    ftmp = p->GetPoint(pts[0]);

    lastX = (int)(actorPos[0] + ftmp[0]);
    lastY = (int)(actorPos[1] - ftmp[1]);

    for (j = 1; j < npts; j++) 
      {
      ftmp = p->GetPoint(pts[j]);
      if (c && !cellScalars)
	{
	rgba = c->GetColor(pts[j]);
	aColor.red = (unsigned short) (rgba[0] * 256);
	aColor.green = (unsigned short) (rgba[1] * 256);
	aColor.blue = (unsigned short) (rgba[2] * 256);
	XAllocColor(displayId, attr.colormap, &aColor);
	XSetForeground(displayId, gc, aColor.pixel);
	}
      X = (int)(actorPos[0] + ftmp[0]);
      Y = (int)(actorPos[1] - ftmp[1]);
      XDrawLine(displayId, drawable, gc, lastX, lastY, X, Y);
      lastX = X;
      lastY = Y;
      }
    }

  // Flush the X queue
  XFlush(displayId);
  XSync(displayId, False);

  delete [] points;
  if ( this->TransformCoordinate )
    {
    p->Delete();
    }
}
#endif
