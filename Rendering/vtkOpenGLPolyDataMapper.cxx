/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper.cxx
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
#include <string.h>
#include <math.h>

#include "vtkOpenGLPolyDataMapper.h"

#ifdef VTK_USE_QUARTZ
#include "vtkQuartzRenderWindow.h"
#else
#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkOpenGLRenderWindow.h"
#endif
#endif
#include "vtkOpenGLRenderer.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
//--------------------------------------------------------------------------
vtkOpenGLPolyDataMapper* vtkOpenGLPolyDataMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLPolyDataMapper");
  if(ret)
    {
    return (vtkOpenGLPolyDataMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLPolyDataMapper;
}
#endif

// Construct empty object.
vtkOpenGLPolyDataMapper::vtkOpenGLPolyDataMapper()
{
  this->ListId = 0;
}

// Destructor (don't call ReleaseGraphicsResources() since it is virtual
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }  
}

// Release the graphics resources used by this mapper.  In this case, release
// the display list if any.
void vtkOpenGLPolyDataMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->ListId && win)
    {
    win->MakeCurrent();
    glDeleteLists(this->ListId,1);
    this->ListId = 0;
    }
  this->LastWindow = NULL; 
}

// Get the lmcolor property, this is a pretty important little 
// function.  It determines how vertex colors will be handled  
// in gl.  When a PolyDataMapper has vertex colors it will use this 
// method to determine what lmcolor mode to set.               
GLenum vtkOpenGLPolyDataMapper::GetLmcolorMode(vtkProperty *prop)
{
  if (prop->GetAmbient() > prop->GetDiffuse())
    {
    return GL_AMBIENT;
    }
  else
    {
    return GL_DIFFUSE;
    }
}

//
// Receives from Actor -> maps data to primitives
//
void vtkOpenGLPolyDataMapper::RenderPiece(vtkRenderer *ren, vtkActor *act)
{
  vtkIdType numPts;
  vtkPolyData *input= this->GetInput();
  vtkPlaneCollection *clipPlanes;
  vtkPlane *plane;
  int i, numClipPlanes;
  double planeEquation[4];

  //
  // make sure that we've been properly initialized
  //
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }
  
  if ( input == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    input->Update();
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

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

// make sure our window is current
  ren->GetRenderWindow()->MakeCurrent();

  clipPlanes = this->ClippingPlanes;

  if (clipPlanes == NULL)
    {
    numClipPlanes = 0;
    }
  else
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL guarantees at most 6 additional clipping planes");
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
     glEnable((GLenum)(GL_CLIP_PLANE0+i));
    }

  if ( clipPlanes )
    {
    vtkMatrix4x4 *actorMatrix = vtkMatrix4x4::New();
    act->GetMatrix( actorMatrix );
    actorMatrix->Invert();
    
    float origin[4], normal[3], point[4];
    
    for (i = 0; i < numClipPlanes; i++)
      {    
      plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);
      
      plane->GetOrigin(origin);
      plane->GetNormal(normal);
      
      point[0] = origin[0] + normal[0];
      point[1] = origin[1] + normal[1];
      point[2] = origin[2] + normal[2];
      
      origin[3] = point[3] = 1.0;
      
      actorMatrix->MultiplyPoint( origin, origin );
      actorMatrix->MultiplyPoint( point, point );
      
      if ( origin[3] != 1.0 )
        {
        origin[0] /= origin[3];
        origin[1] /= origin[3];
        origin[2] /= origin[3];
        }
      
      if ( point[3] != 1.0 )
        {
        point[0] /= point[3];
        point[1] /= point[3];
        point[2] /= point[3];
        }
      
      normal[0] = point[0] - origin[0];
      normal[1] = point[1] - origin[1];
      normal[2] = point[2] - origin[2];
      
      planeEquation[0] = normal[0];
      planeEquation[1] = normal[1];
      planeEquation[2] = normal[2];
      planeEquation[3] = -(planeEquation[0]*origin[0]+
                           planeEquation[1]*origin[1]+
                           planeEquation[2]*origin[2]);
      glClipPlane((GLenum)(GL_CLIP_PLANE0+i),planeEquation);
      }
    
    actorMatrix->Delete();  
    }
  

  //
  // if something has changed regenerate colors and display lists
  // if required
  //
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime ||
       act->GetProperty()->GetMTime() > this->BuildTime ||
       ren->GetRenderWindow() != this->LastWindow)
    {
    // sets this->Colors as side effect
    this->MapScalars(act->GetProperty()->GetOpacity());

    if (!this->ImmediateModeRendering && 
        !this->GetGlobalImmediateModeRendering())
      {
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->LastWindow = ren->GetRenderWindow();
      
      // get a unique display list id
      this->ListId = glGenLists(1);
      glNewList(this->ListId,GL_COMPILE_AND_EXECUTE);

      // Time the actual drawing
      this->Timer->StartTimer();
      this->Draw(ren,act);
      this->Timer->StopTimer();      

      glEndList();
      }
    else
      {
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->LastWindow = ren->GetRenderWindow();
      }
    this->BuildTime.Modified();
    }
  // if nothing changed but we are using display lists, draw it
  else
    {
    if (!this->ImmediateModeRendering && 
        !this->GetGlobalImmediateModeRendering())
      {
      // Time the actual drawing
      this->Timer->StartTimer();
      glCallList(this->ListId);
      this->Timer->StopTimer();      
      }
    }
   
  // if we are in immediate mode rendering we always
  // want to draw the primitives here
  if (this->ImmediateModeRendering ||
      this->GetGlobalImmediateModeRendering())
    {
    this->MapScalars(act->GetProperty()->GetOpacity());
    // Time the actual drawing
    this->Timer->StartTimer();
    this->Draw(ren,act);
    this->Timer->StopTimer();      
    }

  this->TimeToDraw = (float)this->Timer->GetElapsedTime();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable((GLenum)(GL_CLIP_PLANE0+i));
    }
}

//
// Helper routine which starts a poly, triangle or quad based upon
// the number of points in the polygon and whether triangles or quads
// were the last thing being drawn (we can get better performance if we
// can draw several triangles within a single glBegin(GL_TRIANGLES) or
// several quads within a single glBegin(GL_QUADS). 
//
static void vtkOpenGLBeginPolyTriangleOrQuad(GLenum aGlFunction,
                                             GLenum &previousGlFunction,
                                             int npts)
{
  if (aGlFunction == GL_POLYGON)
    {
    switch (npts)
      {
      case 3:  // Need to draw a triangle.
        if (previousGlFunction != GL_TRIANGLES)
          {
          // we were not already drawing triangles, were we drawing quads?
          if (previousGlFunction == GL_QUADS)
            {
            // we were previously drawing quads, close down the quads.
            glEnd();
            }
          // start drawing triangles
          previousGlFunction = GL_TRIANGLES;
          glBegin(GL_TRIANGLES);
          }
          break;
      case 4:  // Need to draw a quad
        if (previousGlFunction != GL_QUADS)
          {
          // we were not already drawing quads, were we drawing triangles?
          if (previousGlFunction == GL_TRIANGLES)
            {
            // we were previously drawing triangles, close down the triangles.
            glEnd();
            }
          // start drawing quads
          previousGlFunction = GL_QUADS;
          glBegin(GL_QUADS);
          }
        break;
      default:
        // if we were supposed to be drawing polygons but were really
        // drawing triangles or quads, then we need to close down the
        // triangles or quads and begin a polygon
        if (previousGlFunction != GL_INVALID_VALUE
            && previousGlFunction != GL_POLYGON)
          {
          glEnd();
          }
        previousGlFunction = GL_POLYGON;
        glBegin(aGlFunction);
        break;
      }
    }
  else if (aGlFunction == GL_POINTS)
    {
    // we are supposed to be drawing points
    if (previousGlFunction != GL_POINTS)
      {
      // We were not drawing points before this, switch to points.
      // We don't need to worry about switching from triangles or quads
      // since draw all points before drawing any polygons (i.e. in the polys
      // case we switch to triangles and quads as an optimization, there is
      // nothing to switch to that is below points).
      previousGlFunction = GL_POINTS;
      glBegin(GL_POINTS);
      }
    }
  else
    {
    previousGlFunction = aGlFunction;
    glBegin(aGlFunction);
    }
}

static void vtkOpenGLDraw01(vtkCellArray *aPrim, GLenum aGlFunction,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *, vtkTCoords *, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );

    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES)
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawN013(vtkCellArray *aPrim, GLenum aGlFunction,
                              vtkIdType &, vtkPoints *p, vtkNormals *n, 
                              vtkUnsignedCharArray *, vtkTCoords *, 
                              vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;

  GLenum previousGlFunction=GL_INVALID_VALUE;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    {
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCN013(vtkCellArray *aPrim, GLenum aGlFunction,
                               vtkIdType &cellNum, vtkPoints *p, 
                               vtkNormals *n, vtkUnsignedCharArray *, 
                               vtkTCoords *, vtkOpenGLRenderer *ren, 
                               int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawS01(vtkCellArray *aPrim, GLenum aGlFunction,
                             vtkIdType &, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *c, vtkTCoords *, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawNS013(vtkCellArray *aPrim, GLenum aGlFunction,
                               vtkIdType &, vtkPoints *p, vtkNormals *n, 
                               vtkUnsignedCharArray *c, vtkTCoords *, 
                               vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCNS013(vtkCellArray *aPrim, GLenum aGlFunction,
                                vtkIdType &cellNum, vtkPoints *p, 
                                vtkNormals *n, vtkUnsignedCharArray *c, 
                                vtkTCoords *, vtkOpenGLRenderer *ren, 
                                int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawT01(vtkCellArray *aPrim, GLenum aGlFunction,
                             vtkIdType &, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *, vtkTCoords *t, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawNT013(vtkCellArray *aPrim, GLenum aGlFunction,
                               vtkIdType &, vtkPoints *p, vtkNormals *n, 
                               vtkUnsignedCharArray *, vtkTCoords *t, 
                               vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCNT013(vtkCellArray *aPrim, GLenum aGlFunction,
                                vtkIdType &cellNum, vtkPoints *p, 
                                vtkNormals *n, vtkUnsignedCharArray *, 
                                vtkTCoords *t, vtkOpenGLRenderer *ren, 
                                int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawST01(vtkCellArray *aPrim, GLenum aGlFunction,
                              vtkIdType &, vtkPoints *p, vtkNormals *, 
                              vtkUnsignedCharArray *c, vtkTCoords *t, 
                              vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawNST013(vtkCellArray *aPrim, GLenum aGlFunction,
                                vtkIdType &, vtkPoints *p, vtkNormals *n, 
                                vtkUnsignedCharArray *c, vtkTCoords *t, 
                                vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCNST013(vtkCellArray *aPrim, GLenum aGlFunction,
                                 vtkIdType &cellNum, vtkPoints *p, 
                                 vtkNormals *n, vtkUnsignedCharArray *c, 
                                 vtkTCoords *t, vtkOpenGLRenderer *ren,
                                 int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    {
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCS01(vtkCellArray *aPrim, GLenum aGlFunction,
                              vtkIdType &cellNum, vtkPoints *p, vtkNormals *, 
                              vtkUnsignedCharArray *c, vtkTCoords *, 
                              vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawNCS013(vtkCellArray *aPrim, GLenum aGlFunction,
                                vtkIdType &cellNum, vtkPoints *p, 
                                vtkNormals *n, vtkUnsignedCharArray *c, 
                                vtkTCoords *, vtkOpenGLRenderer *ren,
                                int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glColor4ubv(c->GetPointer(4*cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCNCS013(vtkCellArray *aPrim, GLenum aGlFunction,
                                 vtkIdType &cellNum, vtkPoints *p, 
                                 vtkNormals *n, vtkUnsignedCharArray *c, 
                                 vtkTCoords *, vtkOpenGLRenderer *ren,
                                 int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glColor4ubv(c->GetPointer(4*cellNum));
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCST01(vtkCellArray *aPrim, GLenum aGlFunction,
                               vtkIdType &cellNum, vtkPoints *p, vtkNormals *, 
                               vtkUnsignedCharArray *c, vtkTCoords *t, 
                               vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawNCST013(vtkCellArray *aPrim, GLenum aGlFunction,
                                 vtkIdType &cellNum, vtkPoints *p, 
                                 vtkNormals *n, vtkUnsignedCharArray *c, 
                                 vtkTCoords *t, vtkOpenGLRenderer *ren,
                                 int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCNCST013(vtkCellArray *aPrim, GLenum aGlFunction,
                                  vtkIdType &cellNum, vtkPoints *p, 
                                  vtkNormals *n, vtkUnsignedCharArray *c, 
                                  vtkTCoords *t, vtkOpenGLRenderer *ren, 
                                  int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glColor4ubv(c->GetPointer(4*cellNum));
    glNormal3fv(n->GetNormal(cellNum));
      
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}


static void vtkOpenGLDraw3(vtkCellArray *aPrim, GLenum aGlFunction,
                           vtkIdType &, vtkPoints *p, vtkNormals *, 
                           vtkUnsignedCharArray *, vtkTCoords *, 
                           vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    {
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawS3(vtkCellArray *aPrim, GLenum aGlFunction,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *c, vtkTCoords *, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawT3(vtkCellArray *aPrim, GLenum aGlFunction,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *, vtkTCoords *t, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawST3(vtkCellArray *aPrim, GLenum aGlFunction,
                             vtkIdType &, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *c, vtkTCoords *t, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES)
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCS3(vtkCellArray *aPrim, GLenum aGlFunction,
                             vtkIdType &cellNum, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *c, vtkTCoords *, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES)
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }

    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCST3(vtkCellArray *aPrim, GLenum aGlFunction,
                              vtkIdType &cellNum, vtkPoints *p, vtkNormals *, 
                              vtkUnsignedCharArray *c, vtkTCoords *t, 
                              vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}
  
static void vtkOpenGLDraw2(vtkCellArray *aPrim, GLenum aGlFunction,
                           vtkIdType &, vtkPoints *p, vtkNormals *, 
                           vtkUnsignedCharArray *, vtkTCoords *, 
                           vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  vtkIdType idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      if ( j > 2)
        {
        if (j % 2)
          {
          idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        else
          {
          idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        }
      else if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawS2(vtkCellArray *aPrim, GLenum aGlFunction,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *c, vtkTCoords *, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  vtkIdType idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      if ( j > 2)
        {
        if (j % 2)
          {
          idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        else
          {
          idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        }
      else if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }

    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawT2(vtkCellArray *aPrim, GLenum aGlFunction,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *, vtkTCoords *t, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType idx[3], *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      if ( j > 2)
        {
        if (j % 2)
          {
          idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        else
          {
          idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        }
      else if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawST2(vtkCellArray *aPrim, GLenum aGlFunction,
                             vtkIdType &, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *c, vtkTCoords *t, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType idx[3], *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      if ( j > 2)
        {
        if (j % 2)
          {
          idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        else
          {
          idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        }
      else if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCS2(vtkCellArray *aPrim, GLenum aGlFunction,
                             vtkIdType &cellNum, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *c, vtkTCoords *, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType idx[3], *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      if ( j > 2)
        {
        if (j % 2)
          {
          idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        else
          {
          idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        }
      else if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

static void vtkOpenGLDrawCST2(vtkCellArray *aPrim, GLenum aGlFunction,
                              vtkIdType &cellNum, vtkPoints *p, vtkNormals *, 
                              vtkUnsignedCharArray *c, vtkTCoords *t, 
                              vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType idx[3], *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkOpenGLBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetPointer(4*cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      if ( j > 2)
        {
        if (j % 2)
          {
          idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        else
          {
          idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
          vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
          }
        }
      else if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
        && (previousGlFunction != GL_QUADS)
        && (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
} 
  
static void vtkOpenGLDrawW(vtkCellArray *aPrim, GLenum,
                           vtkIdType &, vtkPoints *p, vtkNormals *, 
                           vtkUnsignedCharArray *, vtkTCoords *, 
                           vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType idx[3], *pts, npts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
      
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      if (j == 1)
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
  
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawNW(vtkCellArray *aPrim, GLenum,
                            vtkIdType &, vtkPoints *p, vtkNormals *n, 
                            vtkUnsignedCharArray *, vtkTCoords *, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawSW(vtkCellArray *aPrim, GLenum,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *c, vtkTCoords *, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, idx[3], npts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      if (j == 1)
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawNSW(vtkCellArray *aPrim, GLenum,
                             vtkIdType &, vtkPoints *p, vtkNormals *n, 
                             vtkUnsignedCharArray *c, vtkTCoords *, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawTW(vtkCellArray *aPrim, GLenum,
                            vtkIdType &, vtkPoints *p, vtkNormals *, 
                            vtkUnsignedCharArray *, vtkTCoords *t, 
                            vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, idx[3], npts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      if (j == 1)
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawNTW(vtkCellArray *aPrim, GLenum,
                             vtkIdType &, vtkPoints *p, vtkNormals *n, 
                             vtkUnsignedCharArray *, vtkTCoords *t, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawSTW(vtkCellArray *aPrim, GLenum,
                             vtkIdType &, vtkPoints *p, vtkNormals *, 
                             vtkUnsignedCharArray *c, vtkTCoords *t, 
                             vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, idx[3], npts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      if ( j == 0 )
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
      
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      if (j == 1)
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      else
        {
        idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
        vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
        }
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

static void vtkOpenGLDrawNSTW(vtkCellArray *aPrim, GLenum,
                              vtkIdType &, vtkPoints *p, vtkNormals *n, 
                              vtkUnsignedCharArray *c, vtkTCoords *t, 
                              vtkOpenGLRenderer *ren, int &noAbort)
{
  int j;
  vtkIdType *pts, npts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetPointer(4*pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
  
    // check for abort condition
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
        {
        noAbort = 0;
        }
      }
    }
}

// Draw method for OpenGL.
void vtkOpenGLPolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkOpenGLRenderer *ren = (vtkOpenGLRenderer *)aren;
  int rep, interpolation;
  float tran;
  GLenum glFunction[4], aGlFunction;
  vtkProperty *prop;
  vtkPoints *p;
  vtkCellArray *prims[4], *aPrim;
  vtkUnsignedCharArray *c=NULL;
  vtkNormals *n;
  vtkTCoords *t;
  int tDim;
  int noAbort = 1;
  vtkPolyData *input = this->GetInput();
  int cellScalars = 0;
  vtkIdType cellNum = 0;
  int cellNormals = 0;
  int resolve=0, zResolve=0;
  double zRes = 0.0;
  
  // get the property 
  prop = act->GetProperty();

  // get the transparency 
  tran = prop->GetOpacity();
  
  // if the primitives are invisable then get out of here 
  if (tran <= 0.0)
    {
    return;
    }

  // get the representation (e.g., surface / wireframe / points)
  rep = prop->GetRepresentation();

  switch (rep) 
    {
    case VTK_POINTS:
      glFunction[0]  = GL_POINTS;
      glFunction[1]  = GL_POINTS;
      glFunction[2]  = GL_POINTS;
      glFunction[3]  = GL_POINTS;
      break;
    case VTK_WIREFRAME:
      glFunction[0] = GL_POINTS;
      glFunction[1] = GL_LINE_STRIP;
      glFunction[2] = GL_LINE_STRIP;
      glFunction[3] = GL_LINE_LOOP;
      break;
    case VTK_SURFACE:
      glFunction[0] = GL_POINTS;
      glFunction[1] = GL_LINE_STRIP;
      glFunction[2] = GL_TRIANGLE_STRIP;
      glFunction[3] = GL_POLYGON;
      break;
    default: 
      vtkErrorMacro(<< "Bad representation sent\n");
      glFunction[0] = GL_POINTS;
      glFunction[1] = GL_LINE_STRIP;
      glFunction[2] = GL_TRIANGLE_STRIP;
      glFunction[3] = GL_POLYGON;
      break;
    }

  // get the shading interpolation 
  interpolation = prop->GetInterpolation();

  // and draw the display list
  p = input->GetPoints();
  
  // are they cell or point scalars
  if ( this->Colors )
    {
    c = this->Colors;
    if ( (this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          !input->GetPointData()->GetScalars() )
         && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
      {
      cellScalars = 1;
      }
    }
    
  prims[0] = input->GetVerts();
  prims[1] = input->GetLines();
  prims[2] = input->GetStrips();
  prims[3] = input->GetPolys();

  t = input->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetNumberOfComponents();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  n = input->GetPointData()->GetNormals();
  if (interpolation == VTK_FLAT)
    {
    n = 0;
    }
  
  cellNormals = 0;
  if (input->GetCellData()->GetNormals())
    {
    cellNormals = 1;
    n = input->GetCellData()->GetNormals();
    }
  
  // if we are doing vertex colors then set lmcolor to adjust 
  // the current materials ambient and diffuse values using   
  // vertex color commands otherwise tell it not to.          
  glDisable( GL_COLOR_MATERIAL );
  if (c)
    {
    glColorMaterial( GL_FRONT_AND_BACK, this->GetLmcolorMode(prop));
    glEnable( GL_COLOR_MATERIAL );
    }
  
  // how do we draw points
  void (*draw0)(vtkCellArray *, GLenum, vtkIdType &, vtkPoints *, vtkNormals *,
                vtkUnsignedCharArray *, vtkTCoords *, vtkOpenGLRenderer *, 
                int &);

  int idx;
  if (n && !cellNormals)
    {
    idx = 1;
    }
  else
    {
    idx = 0;
    }
  if (c)
    {
    idx += 2;
    }
  if (t)
    {
    idx += 4;
    }
  if (cellScalars)
    {
    idx += 8;
    }
  if (cellNormals)
    {
    idx += 16;
    }

  switch (idx) 
    {
    case 0: draw0 = vtkOpenGLDraw01; break;
    case 1: draw0 = vtkOpenGLDrawN013; break;
    case 2: draw0 = vtkOpenGLDrawS01; break;
    case 3: draw0 = vtkOpenGLDrawNS013; break;
    case 4: draw0 = vtkOpenGLDrawT01; break;
    case 5: draw0 = vtkOpenGLDrawNT013; break;
    case 6: draw0 = vtkOpenGLDrawST01; break;
    case 7: draw0 = vtkOpenGLDrawNST013; break;
    case 10: draw0 = vtkOpenGLDrawCS01; break;
    case 11: draw0 = vtkOpenGLDrawNCS013; break;
    case 14: draw0 = vtkOpenGLDrawCST01; break;
    case 15: draw0 = vtkOpenGLDrawNCST013; break;

    case 16: draw0 = vtkOpenGLDrawCN013; break;
    case 18: draw0 = vtkOpenGLDrawCNS013; break;
    case 20: draw0 = vtkOpenGLDrawCNT013; break;
    case 22: draw0 = vtkOpenGLDrawCNST013; break;
    case 26: draw0 = vtkOpenGLDrawCNCS013; break;
    case 30: draw0 = vtkOpenGLDrawCNCST013; break;
    default:
      // This case should never be reached.
      return;
    }

  // how do we draw lines
  void (*draw1)(vtkCellArray *, GLenum, vtkIdType &, vtkPoints *, vtkNormals *,
                vtkUnsignedCharArray *, vtkTCoords *, vtkOpenGLRenderer *, 
                int &);
  switch (idx) 
    {
    case 0: draw1 = vtkOpenGLDraw01; break;
    case 1: draw1 = vtkOpenGLDrawN013; break;
    case 2: draw1 = vtkOpenGLDrawS01; break;
    case 3: draw1 = vtkOpenGLDrawNS013; break;
    case 4: draw1 = vtkOpenGLDrawT01; break;
    case 5: draw1 = vtkOpenGLDrawNT013; break;
    case 6: draw1 = vtkOpenGLDrawST01; break;
    case 7: draw1 = vtkOpenGLDrawNST013; break;
    case 10: draw1 = vtkOpenGLDrawCS01; break;
    case 11: draw1 = vtkOpenGLDrawNCS013; break;
    case 14: draw1 = vtkOpenGLDrawCST01; break;
    case 15: draw1 = vtkOpenGLDrawNCST013; break;
    case 16: draw1 = vtkOpenGLDrawCN013; break;
    case 18: draw1 = vtkOpenGLDrawCNS013; break;
    case 20: draw1 = vtkOpenGLDrawCNT013; break;
    case 22: draw1 = vtkOpenGLDrawCNST013; break;
    case 26: draw1 = vtkOpenGLDrawCNCS013; break;
    case 30: draw1 = vtkOpenGLDrawCNCST013; break;
    default:
      // This case should never be reached.
      return;
    }

  // how do we draw tstrips
  void (*draw2)(vtkCellArray *, GLenum, vtkIdType &, vtkPoints *, vtkNormals *,
                vtkUnsignedCharArray *, vtkTCoords *, vtkOpenGLRenderer *, 
                int &);
  void (*draw2W)(vtkCellArray *, GLenum, vtkIdType &, vtkPoints *,
                 vtkNormals *, vtkUnsignedCharArray *, vtkTCoords *, 
                 vtkOpenGLRenderer *, int &);
  switch (idx) 
    {
    case 0: draw2 = vtkOpenGLDraw2; break;
    case 1: draw2 = vtkOpenGLDrawN013; break;
    case 2: draw2 = vtkOpenGLDrawS2; break;
    case 3: draw2 = vtkOpenGLDrawNS013; break;
    case 4: draw2 = vtkOpenGLDrawT2; break;
    case 5: draw2 = vtkOpenGLDrawNT013; break;
    case 6: draw2 = vtkOpenGLDrawST2; break;
    case 7: draw2 = vtkOpenGLDrawNST013; break;
    case 10: draw2 = vtkOpenGLDrawCS2; break;
    case 11: draw2 = vtkOpenGLDrawNCS013; break;
    case 14: draw2 = vtkOpenGLDrawCST2; break;
    case 15: draw2 = vtkOpenGLDrawNCST013; break;
    case 16: draw2 = vtkOpenGLDraw2; break;
    case 18: draw2 = vtkOpenGLDrawS2; break;
    case 20: draw2 = vtkOpenGLDrawT2; break;
    case 22: draw2 = vtkOpenGLDrawST2; break;
    case 26: draw2 = vtkOpenGLDrawCS2; break;
    case 30: draw2 = vtkOpenGLDrawCST2; break;
    default:
      // This case should never be reached.
      return;
    }
  switch (idx)
    {
    case 0: draw2W = vtkOpenGLDrawW; break;
    case 1: draw2W = vtkOpenGLDrawNW; break;
    case 2: draw2W = vtkOpenGLDrawSW; break;
    case 3: draw2W = vtkOpenGLDrawNSW; break;
    case 4: draw2W = vtkOpenGLDrawTW; break;
    case 5: draw2W = vtkOpenGLDrawNTW; break;
    case 6: draw2W = vtkOpenGLDrawSTW; break;
    case 7: draw2W = vtkOpenGLDrawNSTW; break;
    case 10: draw2W = vtkOpenGLDrawW; break;
    case 11: draw2W = vtkOpenGLDrawNW; break;
    case 14: draw2W = vtkOpenGLDrawTW; break;
    case 15: draw2W = vtkOpenGLDrawNTW; break;
    case 16: draw2W = vtkOpenGLDrawW; break;
    case 18: draw2W = vtkOpenGLDrawSW; break;
    case 20: draw2W = vtkOpenGLDrawTW; break;
    case 22: draw2W = vtkOpenGLDrawSTW; break;
    case 26: draw2W = vtkOpenGLDrawW; break;
    case 30: draw2W = vtkOpenGLDrawTW; break;
    default:
      // This case should never be reached.
      return;
    }
  
  // how do we draw polys
  void (*draw3)(vtkCellArray *, GLenum, vtkIdType &, vtkPoints *, vtkNormals *,
                vtkUnsignedCharArray *, vtkTCoords *, 
                vtkOpenGLRenderer *, int &);
  switch (idx) 
    {
    case 0: draw3 = vtkOpenGLDraw3; break;
    case 1: draw3 = vtkOpenGLDrawN013; break;
    case 2: draw3 = vtkOpenGLDrawS3; break;
    case 3: draw3 = vtkOpenGLDrawNS013; break;
    case 4: draw3 = vtkOpenGLDrawT3; break;
    case 5: draw3 = vtkOpenGLDrawNT013; break;
    case 6: draw3 = vtkOpenGLDrawST3; break;
    case 7: draw3 = vtkOpenGLDrawNST013; break;
    case 10: draw3 = vtkOpenGLDrawCS3; break;
    case 11: draw3 = vtkOpenGLDrawNCS013; break;
    case 14: draw3 = vtkOpenGLDrawCST3; break;
    case 15: draw3 = vtkOpenGLDrawNCST013; break;
    case 16: draw3 = vtkOpenGLDrawCN013; break;
    case 18: draw3 = vtkOpenGLDrawCNS013; break;
    case 20: draw3 = vtkOpenGLDrawCNT013; break;
    case 22: draw3 = vtkOpenGLDrawCNST013; break;
    case 26: draw3 = vtkOpenGLDrawCNCS013; break;
    case 30: draw3 = vtkOpenGLDrawCNCST013; break;
    default:
      // This case should never be reached.
      return;
    }

  if ( this->GetResolveCoincidentTopology() )
    {
    resolve = 1;
    if ( this->GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      zResolve = 1;
      zRes = this->GetResolveCoincidentTopologyZShift();
      }
    else
      {
#ifdef GL_VERSION_1_1
      float f, u;
      glEnable(GL_POLYGON_OFFSET_FILL);
      this->GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
      glPolygonOffset(f,u);
#endif      
      }
    }

  // do verts
  aPrim = prims[0];
  aGlFunction = glFunction[0];

  // For verts or lines that have no normals, disable shading.
  // This will fall back on the color set in the glColor4fv() 
  // call in vtkOpenGLProperty::Render() - the color returned
  // by vtkProperty::GetColor() with alpha set to 1.0.
  if (!n)
    {
    glDisable( GL_LIGHTING);
    }
  
  // draw all the elements
  draw0(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  
  // do lines
  if ( zResolve )
    {
    glDepthRange(zRes, 1.);
    }
  aPrim = prims[1];
  aGlFunction = glFunction[1];
  
  // draw all the elements
  draw1(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  
  // reset the lighting if we turned it off
  if (!n)
    {
    glEnable( GL_LIGHTING);
    }

  // disable shading if we are rendering points, but have no normals
  if (!n && rep == VTK_POINTS)
    {
    glDisable( GL_LIGHTING);
    }
  
  // do tstrips
  if ( zResolve )
    {
    glDepthRange(2*zRes, 1.);
    }
  aPrim = prims[2];
  aGlFunction = glFunction[2];
  draw2(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  if (rep == VTK_WIREFRAME)   
    {
    draw2W(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
    }

  // do polys
  aPrim = prims[3];
  aGlFunction = glFunction[3];
  draw3(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);

  // enable lighting again if necessary
  if (!n && rep == VTK_POINTS)
    {
    glEnable( GL_LIGHTING);
    }

  if (resolve)
    {
    if ( zResolve )
      {
      glDepthRange(0., 1.);
      }
    else
      {
#ifdef GL_VERSION_1_1
      glDisable(GL_POLYGON_OFFSET_FILL);
#endif
      }
    }
}
