/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32PolyDataMapper2D.cxx
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
#include "vtkWin32PolyDataMapper2D.h"
#include "vtkWin32ImageWindow.h"
#include "vtkObjectFactory.h"

#ifndef VTK_REMOVE_LEGACY_CODE
//-------------------------------------------------------------------------
vtkWin32PolyDataMapper2D* vtkWin32PolyDataMapper2D::New()
{
  vtkGenericWarningMacro(<<"Obsolete native imaging class: " 
                         <<"use OpenGL version instead");

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32PolyDataMapper2D");
  if(ret)
    {
    return (vtkWin32PolyDataMapper2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32PolyDataMapper2D;
}


void vtkWin32PolyDataMapper2D::RenderOverlay(vtkViewport* viewport, 
					     vtkActor2D* actor)
{
  int numPts;
  vtkPolyData *input= (vtkPolyData *)this->Input;
  int npts, j;
  vtkPoints *p, *displayPts;
  vtkCellArray *aPrim;
  vtkUnsignedCharArray *c=NULL;
  unsigned char *rgba;
  int *pts;
  float *ftmp;
  POINT *points = new POINT [1024];
  int currSize = 1024;
  HBRUSH brush, nbrush, oldBrush;
  int cellScalars = 0;
  int cellNum = 0;
  float tran;
  HPEN pen, npen, oldPen;

  vtkDebugMacro (<< "vtkWin32PolyDataMapper2D::Render");

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
  tran = actor->GetProperty()->GetOpacity();
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime || 
       this->LookupTable->GetMTime() > this->BuildTime ||
       actor->GetProperty()->GetMTime() > this->BuildTime)
    {
    // sets this->Colors as side effect
    this->MapScalars(tran);
    this->BuildTime.Modified();
    }

  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  HWND windowId = (HWND) window->GetGenericWindowId();

  // Get the device context from the window
  HDC hdc = (HDC) window->GetGenericContext();
 
  // Get the position of the text actor
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedLocalDisplayValue(viewport);

  // Set up the font color from the text actor
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  float*  actorColor = actor->GetProperty()->GetColor();
  red = (unsigned char) (actorColor[0] * 255.0);
  green = (unsigned char) (actorColor[1] * 255.0);
  blue = (unsigned char) (actorColor[2] * 255.0);

  // Set the compositing operator
  SetROP2(hdc, R2_COPYPEN);

  // Transform the points, if necessary
  p = input->GetPoints();
  if ( this->TransformCoordinate )
    {
    int *itmp, numPts = p->GetNumberOfPoints();
    displayPts = vtkPoints::New();
    displayPts->SetNumberOfPoints(numPts);
    for ( j=0; j < numPts; j++ )
      {
      this->TransformCoordinate->SetValue(p->GetPoint(j));
      itmp = this->TransformCoordinate->GetComputedDisplayValue(viewport);
      displayPts->SetPoint(j,itmp[0], itmp[1], itmp[2]);
      }
    p = displayPts;
    }

  // Set up the coloring
  if ( this->Colors )
    {
    c = this->Colors;
    if (!input->GetPointData()->GetScalars())
      {
      cellScalars = 1;
      }
    }

  // set the colors for the foreground
  brush = CreateSolidBrush(RGB(red,green,blue));
  oldBrush = (HBRUSH)SelectObject(hdc,brush);   

  // set the colors for the pen
  pen = (HPEN) CreatePen(PS_SOLID,0,RGB(red,green,blue));
  oldPen = (HPEN) SelectObject(hdc,pen);

  aPrim = input->GetPolys();
  
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    { 
    if (c) 
      {
      if (cellScalars) 
	{
	rgba = c->GetPointer(4*cellNum);
	}
      else
	{
	rgba = c->GetPointer(4*pts[j]);
	}
      npen = (HPEN) CreatePen(PS_SOLID,0,RGB(rgba[0],rgba[1],rgba[2]));
      pen = (HPEN) SelectObject(hdc,npen);
      DeleteObject(pen);
      pen = npen;
      nbrush = (HBRUSH)CreateSolidBrush(RGB(rgba[0],rgba[1],rgba[2]));
      brush = (HBRUSH)SelectObject(hdc,nbrush);   
      DeleteObject(brush);
      brush = nbrush;
      }
    if (npts > currSize)
      {
      delete [] points;
      points = new POINT [npts];
      currSize = npts;
      }
    for (j = 0; j < npts; j++) 
      {
      ftmp = p->GetPoint(pts[j]);
      points[j].x = actorPos[0] + ftmp[0];
      points[j].y = actorPos[1] - ftmp[1];
      }
    Polygon(hdc,points,npts);
    }

  aPrim = input->GetLines();
  
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    { 
    if (c) 
      {
      if (cellScalars) 
	{
	rgba = c->GetPointer(4*cellNum);
	}
      else
	{
	rgba = c->GetPointer(4*pts[j]);
	}
      npen = (HPEN) CreatePen(PS_SOLID,0,RGB(rgba[0],rgba[1],rgba[2]));
      pen =  (HPEN) SelectObject(hdc,npen);   
      DeleteObject(pen);
      pen = npen;
      }
    if (npts > currSize)
      {
      delete [] points;
      points = new POINT [npts];
      currSize = npts;
      }
    for (j = 0; j < npts; j++) 
      {
      ftmp = p->GetPoint(pts[j]);
      points[j].x = (short)(actorPos[0] + ftmp[0]);
      points[j].y = (short)(actorPos[1] - ftmp[1]);
      }
    Polyline(hdc,points,npts);
    }

  delete [] points;
  if ( this->TransformCoordinate )
    {
    p->Delete();
    }
  
  SelectObject(hdc, oldPen);
  DeleteObject(pen);

  SelectObject(hdc, oldBrush);
  DeleteObject(brush);
}

#endif
  
