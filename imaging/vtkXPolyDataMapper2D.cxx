/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXPolyDataMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <stdlib.h>
#include <math.h>
#include "vtkXPolyDataMapper2D.h"
#include "vtkXImageWindow.h"

int vtkXPolyDataMapper2D::GetCompositingMode(vtkActor2D* actor)
{

  vtkProperty2D* tempProp = actor->GetProperty();
  int compositeMode = tempProp->GetCompositingOperator();

  switch (compositeMode)
  {
  case VTK_BLACK:
	  return GXclear;
  case VTK_NOT_DEST:
	  return GXinvert;
  case VTK_SRC_AND_DEST:
	  return GXand;
  case VTK_SRC_OR_DEST:
	  return  GXor;
  case VTK_NOT_SRC:
	  return GXcopyInverted;
  case VTK_SRC_XOR_DEST:
	  return GXxor;
  case VTK_SRC_AND_notDEST:
	  return GXandReverse;
  case VTK_SRC:
	  return GXcopy;
  case VTK_WHITE:
	  return GXset;
  default:
	  return GXcopy;
  }

}


void vtkXPolyDataMapper2D::Render(vtkViewport* viewport, vtkActor2D* actor)
{
  int numPts;
  vtkPolyData *input= (vtkPolyData *)this->Input;
  int npts, j;
  vtkPoints *p;
  vtkCellArray *aPrim;
  vtkScalars *c=NULL;
  unsigned char *rgba;
  int *pts;
  float *ftmp;
  int currSize = 1024;
  int cellScalars = 0;
  int cellNum = 0;
  float tran;
  XPoint *points = new XPoint [1024];
  
  vtkDebugMacro (<< "vtkXPolyDataMapper2D::Render");

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
  
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

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

  // Set the compositing mode for the actor
  int compositeMode = this->GetCompositingMode(actor);
  XSetFunction(displayId, gc, compositeMode);

  // Calculate the size of the bounding rectangle
  // and draw the display list
  p = input->GetPoints();
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
      if (cellScalars) 
	{
	rgba = c->GetColor(cellNum);
	}
      else
	{
	rgba = c->GetColor(pts[j]);
	}
      aColor.red = (unsigned short) (rgba[0] * 256);
      aColor.green = (unsigned short) (actorColor[1] * 256);
      aColor.blue = (unsigned short) (actorColor[2] * 256);
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
    if (c) 
      {
      if (cellScalars) 
	{
	rgba = c->GetColor(cellNum);
	}
      else
	{
	rgba = c->GetColor(pts[j]);
	}
      aColor.red = (unsigned short) (rgba[0] * 256);
      aColor.green = (unsigned short) (actorColor[1] * 256);
      aColor.blue = (unsigned short) (actorColor[2] * 256);
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
    XDrawLines(displayId, drawable, gc, points, npts, CoordModeOrigin);
    }

  // Flush the X queue
  XFlush(displayId);
  XSync(displayId, False);

  delete [] points;
}


  
