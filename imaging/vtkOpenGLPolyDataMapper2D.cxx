/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.cxx
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
#include "vtkOpenGLPolyDataMapper2D.h"
#include <GL/gl.h>

void vtkOpenGLPolyDataMapper2D::RenderOpaqueGeometry(vtkViewport* viewport,
						     vtkActor2D* actor)
{
  int numPts;
  vtkPolyData *input= (vtkPolyData *)this->Input;
  int npts, idx[3], rep, j;
  float fclr[4];
  short clr[4];
  vtkPoints *p;
  vtkCellArray *aPrim;
  vtkScalars *c=NULL;
  unsigned char *rgba;
	unsigned char color[4];
  int *pts;
  float *ftmp;
  int cellScalars = 0;
  int cellNum = 0;

  vtkDebugMacro (<< "vtkOpenGLPolyDataMapper2D::Render");

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

  // Get the position of the actor
  int *size = viewport->GetSize();
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedDisplayValue(viewport);

  // Set up the font color from the text actor
  float*  actorColor = actor->GetProperty()->GetColor();
  color[0] = (unsigned char) (actorColor[0] * 255.0);
  color[1] = (unsigned char) (actorColor[1] * 255.0);
  color[2] = (unsigned char) (actorColor[2] * 255.0);
  color[3] = (unsigned char) (255.0*actor->GetProperty()->GetOpacity());

  // Calculate the size of the bounding rectangle
  // and draw the display list
  p = input->GetPoints();
  if ( this->Colors )
    {
    c = this->Colors;
    c->InitColorTraversal(actor->GetProperty()->GetOpacity(), 
			  this->LookupTable, this->ColorMode);
    if (!input->GetPointData()->GetScalars())
      {
      cellScalars = 1;
      }
    }
  vtkDebugMacro(<< c);
  vtkDebugMacro(<< cellScalars);

  // set the colors for the foreground
  glColor4ubv(color);

  // push a 2D matrix on the stack
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glDisable( GL_DEPTH_TEST );
  glDisable( GL_LIGHTING);
  glOrtho(-actorPos[0],-actorPos[0] + size[0] - 1,
	  -actorPos[1], -actorPos[1] +size[1] -1, 0, 1);
  
  aPrim = input->GetPolys();
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    { 
    glBegin(GL_POLYGON);
    for (j = 0; j < npts; j++) 
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
	glColor4ubv(rgba);
	}
      glVertex2fv(p->GetPoint(pts[j]));
      }
    glEnd();
    }

  aPrim = input->GetLines();
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    { 
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j++) 
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
	glColor4ubv(rgba);
	}
      glVertex2fv(p->GetPoint(pts[j]));
      }
    glEnd();
    }


  // push a 2D matrix on the stack
  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_DEPTH_TEST );
  glEnable( GL_LIGHTING);
}


  
