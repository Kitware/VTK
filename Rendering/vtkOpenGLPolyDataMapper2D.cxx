/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.cxx
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
#include <stdlib.h>
#include <math.h>
#include "vtkOpenGLPolyDataMapper2D.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"
#include "vtkgluPickMatrix.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLPolyDataMapper2D, "1.32");
vtkStandardNewMacro(vtkOpenGLPolyDataMapper2D);
#endif



void vtkOpenGLPolyDataMapper2D::RenderOpaqueGeometry(vtkViewport* viewport,
                                                     vtkActor2D* actor)
{
  int            numPts;
  vtkPolyData    *input= (vtkPolyData *)this->Input;
  int            j;
  vtkPoints      *p, *displayPts;
  vtkCellArray   *aPrim;
  vtkUnsignedCharArray *c=NULL;
  unsigned char  *rgba;
  unsigned char  color[4];
  vtkIdType      *pts, npts;
  int            cellScalars = 0;
  int            cellNum = 0;
  vtkPlaneCollection *clipPlanes;
  vtkPlane           *plane;
  int                 i,numClipPlanes;
  double              planeEquation[4];

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

  // if something has changed regenrate colors and display lists
  // if required
  //
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime || 
       this->LookupTable->GetMTime() > this->BuildTime ||
       actor->GetProperty()->GetMTime() > this->BuildTime)
    {
    // sets this->Colors as side effect
    this->MapScalars(actor->GetProperty()->GetOpacity());
    this->BuildTime.Modified();
    }

  // Get the position of the actor
  int *size = viewport->GetSize();
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  // get window info
  float *tileViewPort = viewport->GetVTKWindow()->GetTileViewport();
  int tileScale = viewport->GetVTKWindow()->GetTileScale();

  // Set up the font color from the text actor
  float*  actorColor = actor->GetProperty()->GetColor();
  color[0] = (unsigned char) (actorColor[0] * 255.0);
  color[1] = (unsigned char) (actorColor[1] * 255.0);
  color[2] = (unsigned char) (actorColor[2] * 255.0);
  color[3] = (unsigned char) (255.0*actor->GetProperty()->GetOpacity());

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
      itmp = this->TransformCoordinate->GetComputedViewportValue(viewport);
      displayPts->SetPoint(j,itmp[0], itmp[1], 0.0);
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
  vtkDebugMacro(<< c);
  vtkDebugMacro(<< cellScalars);

  // set the colors for the foreground
  glColor4ubv(color);

  // push a 2D matrix on the stack
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  if(viewport->GetIsPicking())
    {
    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
                     1, 1, viewport->GetOrigin(), viewport->GetSize());
    }
  
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_TEXTURE_2D);
  glDisable( GL_LIGHTING);

  int xoff = static_cast<int>(actorPos[0] - size[0]*tileViewPort[0]);
  int yoff = static_cast<int>(actorPos[1] - size[1]*tileViewPort[1]);

  if ( actor->GetProperty()->GetDisplayLocation() == 
       VTK_FOREGROUND_LOCATION )
    {
    glOrtho(-xoff,-xoff + (float)size[0]/tileScale,
            -yoff, -yoff +(float)size[1]/tileScale, 0, 1);
    }  
  else
    {
    glOrtho(-xoff,-xoff + (float)size[0]/tileScale,
            -yoff, -yoff + (float)size[1]/tileScale, -1, 0);
    }
    
  // Clipping plane stuff

  clipPlanes = this->ClippingPlanes;

  if (clipPlanes == NULL)
    {
    numClipPlanes = 0;
    }
  else
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 4)
      {
      vtkErrorMacro(<< "Only 4 clipping planes are used with 2D mappers");
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glEnable((GLenum)(GL_CLIP_PLANE0+i));
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);

    planeEquation[0] = plane->GetNormal()[0];
    planeEquation[1] = plane->GetNormal()[1];
    planeEquation[2] = plane->GetNormal()[2];
    planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
                         planeEquation[1]*plane->GetOrigin()[1]+
                         planeEquation[2]*plane->GetOrigin()[2]);
    glClipPlane((GLenum)(GL_CLIP_PLANE0+i),planeEquation);
    }

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
          rgba = c->GetPointer(4*cellNum);
          }
        else
          {
          rgba = c->GetPointer(4*pts[j]);
          }
        glColor4ubv(rgba);
        }
      glVertex2fv(p->GetPoint(pts[j]));
      }
    glEnd();
    }

  // Set the LineWidth
  glLineWidth(actor->GetProperty()->GetLineWidth());

  // Set the LineStipple
  if (actor->GetProperty()->GetLineStipplePattern() != 0xFFFF)
    {
    glEnable (GL_LINE_STIPPLE);
    glLineStipple (actor->GetProperty()->GetLineStippleRepeatFactor(), 
                   actor->GetProperty()->GetLineStipplePattern());
    }
  else
    {
    glDisable (GL_LINE_STIPPLE);
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
          rgba = c->GetPointer(4*cellNum);
          }
        else
          {
          rgba = c->GetPointer(4*pts[j]);
          }
        glColor4ubv(rgba);
        }
      glVertex2fv(p->GetPoint(pts[j]));
      }
    glEnd();
    }

  // Set the PointSize
  glPointSize(actor->GetProperty()->GetPointSize());

  aPrim = input->GetVerts();
  glBegin(GL_POINTS);
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    {
    for (j = 0; j < npts; j++)
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
        glColor4ubv(rgba);
        }
      glVertex2fv(p->GetPoint(pts[j]));
      }
    }
  glEnd();

  if ( this->TransformCoordinate )
    {
    p->Delete();
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable((GLenum)(GL_CLIP_PLANE0+i));
    }

  // push a 2D matrix on the stack
  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}


  
