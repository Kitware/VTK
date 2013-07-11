/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLPolyDataMapper2D.h"

#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkgluPickMatrix.h"
#include "vtkOpenGLError.h"
#include <math.h>

vtkStandardNewMacro(vtkOpenGLPolyDataMapper2D);

void vtkOpenGLPolyDataMapper2D::RenderOverlay(vtkViewport* viewport,
                                              vtkActor2D* actor)
{
  vtkOpenGLClearErrorMacro();
  int            numPts;
  vtkPolyData    *input=static_cast<vtkPolyData *>(this->GetInput());
  int            j;
  vtkPoints      *p, *displayPts;
  vtkCellArray   *aPrim;
  vtkUnsignedCharArray *c=NULL;
  unsigned char  *rgba;
  unsigned char  color[4];
  vtkIdType      *pts = 0;
  vtkIdType      npts = 0;
  int            cellScalars = 0;
  int            cellNum = 0;
  vtkPlaneCollection *clipPlanes;
  vtkPlane           *plane;
  int                 i,numClipPlanes;
  double              planeEquation[4];
  vtkDataArray*  t = 0;

  vtkDebugMacro (<< "vtkOpenGLPolyDataMapper2D::Render");

  if ( input == NULL )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->GetInputAlgorithm()->Update();
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

  // Texture and color by texture
  t = input->GetPointData()->GetTCoords();
  if ( t )
    {
    int tDim = t->GetNumberOfComponents();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = 0;
      }
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
  int size[2];
  size[0] = viewport->GetSize()[0];
  size[1] = viewport->GetSize()[1];

  double *vport = viewport->GetViewport();
  int* actorPos =
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  // get window info
  double *tileViewPort = viewport->GetVTKWindow()->GetTileViewport();
  double visVP[4];
  visVP[0] = (vport[0] >= tileViewPort[0]) ? vport[0] : tileViewPort[0];
  visVP[1] = (vport[1] >= tileViewPort[1]) ? vport[1] : tileViewPort[1];
  visVP[2] = (vport[2] <= tileViewPort[2]) ? vport[2] : tileViewPort[2];
  visVP[3] = (vport[3] <= tileViewPort[3]) ? vport[3] : tileViewPort[3];
  if (visVP[0] >= visVP[2])
    {
    return;
    }
  if (visVP[1] >= visVP[3])
    {
    return;
    }
  size[0] =
    vtkMath::Round(size[0]*(visVP[2] - visVP[0])/(vport[2] - vport[0]));
  size[1] =
    vtkMath::Round(size[1]*(visVP[3] - visVP[1])/(vport[3] - vport[1]));

  // Set up the font color from the text actor
  double* actorColor = actor->GetProperty()->GetColor();
  color[0] = static_cast<unsigned char>(actorColor[0] * 255.0);
  color[1] = static_cast<unsigned char>(actorColor[1] * 255.0);
  color[2] = static_cast<unsigned char>(actorColor[2] * 255.0);
  color[3] = static_cast<unsigned char>(
    255.0*actor->GetProperty()->GetOpacity());

  // Transform the points, if necessary
  p = input->GetPoints();
  if ( this->TransformCoordinate )
    {
    numPts = p->GetNumberOfPoints();
    displayPts = vtkPoints::New();
    displayPts->SetNumberOfPoints(numPts);
    for ( j=0; j < numPts; j++ )
      {
      this->TransformCoordinate->SetValue(p->GetPoint(j));
      if (this->TransformCoordinateUseDouble)
        {
        double* dtmp = this->TransformCoordinate->GetComputedDoubleViewportValue(viewport);
        displayPts->SetPoint(j,dtmp[0], dtmp[1], 0.0);
        }
      else
        {
        int* itmp = this->TransformCoordinate->GetComputedViewportValue(viewport);
        displayPts->SetPoint(j,itmp[0], itmp[1], 0.0);
        }
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
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  if(viewport->GetIsPicking())
    {
    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
                     viewport->GetPickWidth(),
                     viewport->GetPickHeight(),
                     viewport->GetOrigin(), viewport->GetSize());
    }

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  if (!t)
    {
    glDisable(GL_TEXTURE_2D);
    }
  glDisable(GL_LIGHTING);


  // Assume we want to do Zbuffering for now.
  // we may turn this off later
  glDepthMask(GL_TRUE);

  int *winSize = viewport->GetVTKWindow()->GetSize();

  int xoff = static_cast<int>(actorPos[0] - (visVP[0] - vport[0])*
                              winSize[0]);
  int yoff = static_cast<int>(actorPos[1] - (visVP[1] - vport[1])*
                              winSize[1]);

  // set ortho projection
  float left = -xoff;
  float right = -xoff + size[0];
  float bottom = -yoff;
  float top = -yoff + size[1];

  // it's an error to call glOrtho with
  // either left==right or top==bottom
  if (left==right)
    {
    right = left + 1.0;
    }
  if (bottom==top)
    {
    top = bottom + 1.0;
    }

  if (actor->GetProperty()->GetDisplayLocation() ==
       VTK_FOREGROUND_LOCATION)
    {
    glOrtho(left, right, bottom, top, 0, 1);
    }
  else
    {
    glOrtho(left, right, bottom, top, -1, 0);
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
    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    plane = static_cast<vtkPlane *>(clipPlanes->GetItemAsObject(i));

    planeEquation[0] = plane->GetNormal()[0];
    planeEquation[1] = plane->GetNormal()[1];
    planeEquation[2] = plane->GetNormal()[2];
    planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
                         planeEquation[1]*plane->GetOrigin()[1]+
                         planeEquation[2]*plane->GetOrigin()[2]);
    glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0+i),planeEquation);
    }

  // Set the PointSize
  glPointSize(actor->GetProperty()->GetPointSize());
  vtkOpenGLGL2PSHelper::SetPointSize(actor->GetProperty()->GetPointSize());

  double *dptr;
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
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0],dptr[1],0);
      }
    }
  glEnd();

  // Set the LineWidth
  glLineWidth(actor->GetProperty()->GetLineWidth());
  vtkOpenGLGL2PSHelper::SetLineWidth(actor->GetProperty()->GetLineWidth());

  // Set the LineStipple
  if (actor->GetProperty()->GetLineStipplePattern() != 0xFFFF)
    {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(actor->GetProperty()->GetLineStippleRepeatFactor(),
                  actor->GetProperty()->GetLineStipplePattern());
    vtkOpenGLGL2PSHelper::EnableStipple();
    }
  else
    {
    glDisable(GL_LINE_STIPPLE);
    vtkOpenGLGL2PSHelper::DisableStipple();
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
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0], dptr[1], 0);
      }
    glEnd();
    }

  aPrim = input->GetStrips();
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    {
    glBegin(GL_TRIANGLE_STRIP);
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
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0],dptr[1],0);
      }
    glEnd();
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
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0],dptr[1],0);
      }
    glEnd();
    }

  if ( this->TransformCoordinate )
    {
    p->Delete();
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glEnable(GL_LIGHTING);

  // Turn it back on in case we've turned it off
  glDepthMask(GL_TRUE);
  glDisable(GL_TEXTURE_2D);
  vtkOpenGLCheckErrorMacro("failed after RenderOverlay");
}

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
