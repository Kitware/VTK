/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLPolyDataMapper.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkTimerLog.h"
#include "vtkTriangle.h"
#include "vtkOpenGLRenderWindow.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
  #if defined(__APPLE__) && (defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA))
    #include <OpenGL/gl.h>
  #else
    #include <GL/gl.h>
  #endif
#endif

#include <math.h>


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLPolyDataMapper, "1.89");
vtkStandardNewMacro(vtkOpenGLPolyDataMapper);
#endif

// some definitions for what the polydata has in it
#define VTK_PDM_NORMALS            0x01
#define VTK_PDM_COLORS             0x02
#define VTK_PDM_TCOORDS            0x04
#define VTK_PDM_CELL_COLORS        0x08
#define VTK_PDM_CELL_NORMALS       0x10
#define VTK_PDM_POINT_TYPE_FLOAT   0x20
#define VTK_PDM_POINT_TYPE_DOUBLE  0x40
#define VTK_PDM_NORMAL_TYPE_FLOAT  0x80
#define VTK_PDM_NORMAL_TYPE_DOUBLE 0x100
#define VTK_PDM_TCOORD_TYPE_FLOAT  0x200
#define VTK_PDM_TCOORD_TYPE_DOUBLE 0x400
#define VTK_PDM_OPAQUE_COLORS      0x800

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
    
    double origin[4], normal[3], point[4];
    
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
  int noAbort=1;
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
      glNewList(this->ListId,GL_COMPILE);

      noAbort = this->Draw(ren,act);
      glEndList();

      // Time the actual drawing
      this->Timer->StartTimer();
      glCallList(this->ListId);
      this->Timer->StopTimer();      
      }
    else
      {
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->LastWindow = ren->GetRenderWindow();
      }
    if (noAbort)
      {
      this->BuildTime.Modified();
      }
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


#define vtkDrawPointsMacro(ptype,ntype,glPFunc,glNFunc,glCFunc,glNInit) \
{ \
  vtkIdType nPts = 0; int count = 0; \
  ptype *points = (ptype *)voidPoints; \
  glNInit \
  glBegin(GL_POINTS); \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    while (nPts > 0) \
      { \
      glCFunc \
      glNFunc \
      glPFunc \
      ++ptIds; \
      --nPts; \
      } \
    if (count == 100) \
      { \
      cellNum += 100; \
      count = 0; \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        noAbort = 0; \
        break; \
        } \
      } \
    ++count; \
    } \
  cellNum += count; \
  glEnd(); \
}

#define vtkDrawPrimsMacro(ptype,ntype,prim,glPFunc,glNFunc,glCFunc,glNInit) \
{ \
  vtkIdType nPts = 0; int count = 0; \
  ptype *points = (ptype *)voidPoints; \
  glNInit \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    glBegin(prim); \
    while (nPts > 0) \
      { \
      glCFunc \
      glNFunc \
      glPFunc \
      ++ptIds; \
      --nPts; \
      } \
    glEnd(); \
    if (count == 100) \
      { \
      cellNum += 100; \
      count = 0; \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        noAbort = 0; \
        break; \
        } \
      } \
    ++count; \
    } \
  cellNum += count; \
}

#define vtkDrawPolysMacro(ptype,ntype,ttype,prim,glPFunc,glNFunc,glCFunc,glTFunc,glFlatNFunc,glNInit,glTInit) \
{ \
  vtkIdType nPts = 0; int count = 0; \
  ptype *points = (ptype *)voidPoints; \
  GLenum previousGlFunction=GL_INVALID_VALUE; \
  glNInit \
  glTInit \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousGlFunction, nPts ); \
    glFlatNFunc \
    while (nPts > 0) \
      { \
      glTFunc \
      glCFunc \
      glNFunc \
      glPFunc \
      ++ptIds; \
      --nPts; \
      } \
    if (count == 0) \
      { \
      cellNum += 255; \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        noAbort = 0; \
        break; \
        } \
      } \
    if ((previousGlFunction != GL_TRIANGLES)  \
        && (previousGlFunction != GL_QUADS)   \
        && (previousGlFunction != GL_POINTS)) \
      {  \
      glEnd(); \
      } \
    } \
  cellNum += count; \
  if ((previousGlFunction == GL_TRIANGLES)  \
      || (previousGlFunction == GL_QUADS)   \
      || (previousGlFunction == GL_POINTS)) \
    { \
    glEnd(); \
    } \
}

#define vtkDrawStripLinesMacro(ptype,ntype,ttype,prim,glPFunc,glNFunc,glCFunc,glTFunc,glFlatNFunc,glNInit,glTInit) \
{ \
  vtkIdType nPts = 0; \
  ptype *points = (ptype *)voidPoints; \
  vtkIdType *savedPtIds = ptIds; \
  glNInit \
  glTInit \
  while (ptIds < endPtIds) \
    { \
    glBegin(prim); \
    nPts = *ptIds; \
    ++ptIds; \
    glFlatNFunc \
    while (nPts > 0) \
      { \
      glTFunc \
      glCFunc \
      glNFunc \
      glPFunc \
      ptIds += 2; \
      nPts -= 2; \
      } \
    glEnd(); \
    ptIds += nPts; /* nPts could be 0 or -1 here */ \
    } \
  ptIds = savedPtIds; \
  while (ptIds < endPtIds) \
    { \
    glBegin(prim); \
    nPts = *ptIds; \
    ++ptIds; \
    glFlatNFunc \
    ++ptIds; \
    --nPts; \
    while (nPts > 0) \
      { \
      glTFunc \
      glCFunc \
      glNFunc \
      glPFunc \
      ptIds += 2; \
      nPts -= 2; \
      } \
    glEnd(); \
    ptIds += nPts; /* nPts could be 0 or -1 here */ \
    } \
}

void vtkOpenGLPolyDataMapperDrawPoints(int idx,
                                       vtkPoints *p, 
                                       vtkDataArray *n,
                                       vtkUnsignedCharArray *c,
                                       vtkDataArray *t,
                                       vtkIdType &cellNum,
                                       int &noAbort,
                                       vtkCellArray *ca,
                                       vtkRenderer *ren)
{
  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  unsigned char *colors = 0;
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    }
  
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();
    
  // draw all the elements, use fast path if available
  switch (idx)
    {
    case VTK_PDM_POINT_TYPE_FLOAT:
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,;,;,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPointsMacro(double, float, glVertex3dv(points + 3**ptIds);,;,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,
                         glNormal3fv(normals + 3**ptIds);, ;, 
                         float *normals = (float *)voidNormals;);

      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,;,
                         glColor4ubv(colors + 4**ptIds);,;);
      
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,;,
                         glColor3ubv(colors + 4**ptIds);,;);      
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,
                         glNormal3fv(normals + 3**ptIds);,
                         glColor4ubv(colors + 4**ptIds);,
                         float *normals = (float *)voidNormals;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,
                         glNormal3fv(normals + 3**ptIds);,
                         glColor3ubv(colors + 4**ptIds);,
                         float *normals = (float *)voidNormals;);
      break;
    default:
    {
    int j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    int count = 0;
    glBegin(GL_POINTS);
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(npts,pts); 
         count++)
      { 
      for (j = 0; j < npts; j++) 
        {
        if (c)
          {
          if (idx & 0x08)
            {
            glColor4ubv(c->GetPointer(cellNum << 2));
            }
          else
            {
            glColor4ubv(c->GetPointer(pts[j]<< 2));
            }
          }
        if (t)
          {
          glTexCoord2dv(t->GetTuple(pts[j]));
          }
        if (n)
          {
          if (idx & 0x10)
            {
            glNormal3dv(n->GetTuple(cellNum));
            }
          else
            {
            glNormal3dv(n->GetTuple(pts[j]));
            }
          }
        glVertex3dv(p->GetPoint(pts[j]));
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
      ++cellNum;
      }
    glEnd();
    }  
    }
}


void vtkOpenGLPolyDataMapperDrawLines(int idx,
                                      vtkPoints *p, 
                                      vtkDataArray *n,
                                      vtkUnsignedCharArray *c,
                                      vtkDataArray *t,
                                      vtkIdType &cellNum,
                                      int &noAbort,
                                      vtkCellArray *ca,
                                      vtkRenderer *ren)
{
  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  unsigned char *colors = 0;
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();
  
  // draw all the elements, use fast path if available
  switch (idx)
    {
    case VTK_PDM_POINT_TYPE_FLOAT:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP, 
                        glVertex3fv(points + 3**ptIds);,;,;,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPrimsMacro(double, float, GL_LINE_STRIP,
                        glVertex3dv(points + 3**ptIds);,;,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals + 3**ptIds);,;,
                        float *normals = (float *)voidNormals;
        );
      
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glVertex3fv(points + 3**ptIds);,;,
                        glColor4ubv(colors + 4**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glVertex3fv(points + 3**ptIds);,;,
                        glColor3ubv(colors + 4**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals + 3**ptIds);,
                        glColor4ubv(colors + 4**ptIds);,
                        float *normals = (float *)voidNormals;
        );
    break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals + 3**ptIds);,
                        glColor3ubv(colors + 4**ptIds);,
                        float *normals = (float *)voidNormals;
        );
    break;
    default:
    {
    int j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    int count = 0;
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(npts,pts); 
         count++)
      { 
      glBegin(GL_LINE_STRIP);
      for (j = 0; j < npts; j++) 
        {
        if (c)
          {
          if (idx & 0x08)
            {
            glColor4ubv(c->GetPointer(cellNum << 2));
            }
          else
            {
            glColor4ubv(c->GetPointer(pts[j] << 2));
            }
          }
        if (t)
          {
          glTexCoord2dv(t->GetTuple(pts[j]));
          }
        if (n)
          {
          if (idx & 0x10)
            {
            glNormal3dv(n->GetTuple(cellNum));
            }
          else
            {
            glNormal3dv(n->GetTuple(pts[j]));
            }
          }
        glVertex3dv(p->GetPoint(pts[j]));
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
      ++cellNum;
      }
    }  
    }
}









#define PolyNormal \
{ double polyNorm[3]; vtkPolygon::ComputeNormal(p,nPts,ptIds,polyNorm); glNormal3dv(polyNorm); }

void vtkOpenGLPolyDataMapperDrawPolygons(int idx,
                                         vtkPoints *p, 
                                         vtkDataArray *n,
                                         vtkUnsignedCharArray *c,
                                         vtkDataArray *t,
                                         vtkIdType &cellNum,
                                         int &noAbort,
                                         GLenum rep,
                                         vtkCellArray *ca,
                                         vtkRenderer *ren)
{
  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  void *voidTCoords = 0;
  unsigned char *colors = 0;
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    }
  if (t)
    {
    voidTCoords = t->GetVoidPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();

  // draw all the elements, use fast path if available
  switch (idx)
    {
    case VTK_PDM_POINT_TYPE_FLOAT:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);,;,;,;, 
                        PolyNormal,;,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPolysMacro(double, float, float, rep, 
                        glVertex3dv(points + 3**ptIds);,;,;,;, 
                        PolyNormal,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);,;,;,;,
                        float *normals = (float *)voidNormals;,;
        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);,;, 
                        glColor4ubv(colors + 4**ptIds);,;, 
                        PolyNormal,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);,;, 
                        glColor3ubv(colors + 4**ptIds);,;, 
                        PolyNormal,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);, 
                        glColor4ubv(colors + 4**ptIds);,;,;,
                        float *normals = (float *)voidNormals;,;
        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);, 
                        glColor3ubv(colors + 4**ptIds);,;,;,
                        float *normals = (float *)voidNormals;,;
        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
        VTK_PDM_NORMALS | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);,;,
                        glTexCoord2fv(tcoords + 2**ptIds);,;,
                        float *normals = (float *)voidNormals;,
                        float *tcoords = (float *)voidTCoords;);
      break;
    default:
    {
    int j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    int count = 0;
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(npts,pts); 
         count++)
      { 
      glBegin(rep);
      if (!n)
        { 
        double polyNorm[3]; 
        vtkPolygon::ComputeNormal(p,npts,pts,polyNorm); 
        glNormal3dv(polyNorm);
        }
      for (j = 0; j < npts; j++) 
        {
        if (c)
          {
          if (idx & VTK_PDM_CELL_COLORS)
            {
            glColor4ubv(c->GetPointer(cellNum << 2));
            }
          else
            {
            glColor4ubv(c->GetPointer(pts[j] << 2));
            }
          }
        if (t)
          {
          glTexCoord2dv(t->GetTuple(pts[j]));
          }
        if (n)
          {
          if (idx & VTK_PDM_CELL_NORMALS)
            {
            glNormal3dv(n->GetTuple(cellNum));
            }
          else
            {
            glNormal3dv(n->GetTuple(pts[j]));
            }
          }
        glVertex3dv(p->GetPoint(pts[j]));
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
      ++cellNum;
      }
    }  
    }
}

// fix refs here
#define TStripNormal \
if ( vcount > 2) \
{ \
  if (vcount % 2) \
    { \
    normIdx[0] = ptIds[-2]; normIdx[1] = ptIds[0]; normIdx[2] = ptIds[-1]; \
    vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm); \
    } \
  else \
    { \
    normIdx[0] = ptIds[-2]; normIdx[1] = ptIds[-1]; normIdx[2] = ptIds[0]; \
    vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm); \
    } \
  glNormal3dv(polyNorm); \
} \
vcount++; 

#define TStripNormalStart \
  vtkTriangle::ComputeNormal(p, 3, ptIds, polyNorm); \
  glNormal3dv(polyNorm); int vcount = 0;

void vtkOpenGLPolyDataMapperDrawTStrips(int idx,
                                        vtkPoints *p, 
                                        vtkDataArray *n,
                                        vtkUnsignedCharArray *c,
                                        vtkDataArray *t,
                                        vtkIdType &cellNum,
                                        int &noAbort,
                                        GLenum rep,
                                        vtkCellArray *ca,
                                        vtkRenderer *ren)
{
  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  void *voidTCoords = 0;
  unsigned char *colors = 0;
  double polyNorm[3];
  vtkIdType normIdx[3];
  
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    }
  if (t)
    {
    voidTCoords = t->GetVoidPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();

  // draw all the elements, use fast path if available
  switch (idx)
    {
    case VTK_PDM_POINT_TYPE_FLOAT:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, TStripNormal,;,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPolysMacro(double, float, float, rep, 
                        glVertex3dv(points + 3**ptIds);, TStripNormal,;,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);,;,;,;,
                        float *normals = (float *)voidNormals;,;
                        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, TStripNormal, 
                        glColor4ubv(colors + (*ptIds << 2));,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, TStripNormal, 
                        glColor3ubv(colors + (*ptIds << 2));,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);, 
                        glColor4ubv(colors + (*ptIds << 2));,;,;,
                        float *normals = (float *)voidNormals;,;
                        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);, 
                        glColor3ubv(colors + (*ptIds << 2));,;,;,
                        float *normals = (float *)voidNormals;,;
                        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
        VTK_PDM_NORMALS | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, 
                        glNormal3fv(normals + 3**ptIds);,;,
                        glTexCoord2fv(tcoords + 2**ptIds);,;,
                        float *normals = (float *)voidNormals;,
                        float *tcoords = (float *)voidTCoords;);
      break;
    default:
    {
    int j;
    vtkIdType nPts = 0;
    int count = 0;
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(nPts,ptIds); 
         count++)
      { 
      glBegin(rep);
      vtkTriangle::ComputeNormal(p, 3, ptIds, polyNorm);
      glNormal3dv(polyNorm);
      for (j = 0; j < nPts; j++) 
        {
        if (c)
          {
          if (idx & VTK_PDM_CELL_COLORS)
            {
            glColor4ubv(c->GetPointer(cellNum << 2));
            }
          else
            {
            glColor4ubv(c->GetPointer(ptIds[j] << 2));
            }
          }
        if (t)
          {
          glTexCoord2dv(t->GetTuple(ptIds[j]));
          }
        if (n)
          {
          if (idx & VTK_PDM_CELL_NORMALS)
            {
            glNormal3dv(n->GetTuple(cellNum));
            }
          else
            {
            glNormal3dv(n->GetTuple(ptIds[j]));
            }
          }
        else
          {
          if (j >= 2) 
            { 
            if (j % 2) 
              { 
              normIdx[0] = ptIds[j-2]; normIdx[1] = ptIds[j]; 
              normIdx[2] = ptIds[j-1]; 
              vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm); 
              } 
            else 
              { 
              normIdx[0] = ptIds[j-2]; normIdx[1] = ptIds[j-1]; 
              normIdx[2] = ptIds[j]; 
              vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm); 
              } 
            } 
          glNormal3dv(polyNorm);
          }
        glVertex3dv(p->GetPoint(ptIds[j]));
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
      ++cellNum;
      }
    }  
    }
}

void vtkOpenGLPolyDataMapperDrawTStripLines(int idx,
                                            vtkPoints *p, 
                                            vtkDataArray *n,
                                            vtkUnsignedCharArray *c,
                                            vtkDataArray *t,
                                            vtkIdType &cellNum,
                                            int &noAbort,
                                            GLenum rep,
                                            vtkCellArray *ca,
                                            vtkRenderer *ren)
{
  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  void *voidTCoords = 0;
  unsigned char *colors = 0;
  double polyNorm[3];
  vtkIdType normIdx[3];
  
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    }
  if (t)
    {
    voidTCoords = t->GetVoidPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();

  // draw all the elements, use fast path if available
  switch (idx)
    {
    case VTK_PDM_POINT_TYPE_FLOAT:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, TStripNormal,;,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawStripLinesMacro(double, float, float, rep, 
                        glVertex3dv(points + 3**ptIds);, TStripNormal,;,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                             glVertex3fv(points + 3**ptIds);, 
                             glNormal3fv(normals + 3**ptIds);,;,;,;,
                             float *normals = (float *)voidNormals;,;
                             );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, TStripNormal, 
                        glColor4ubv(colors + 4**ptIds);,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                        glVertex3fv(points + 3**ptIds);, TStripNormal, 
                        glColor3ubv(colors + 4**ptIds);,;, 
                        TStripNormalStart,;,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                             glVertex3fv(points + 3**ptIds);, 
                             glNormal3fv(normals + 3**ptIds);, 
                             glColor4ubv(colors + 4**ptIds);,;,;,
                             float *normals = (float *)voidNormals;,;
        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
      VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                             glVertex3fv(points + 3**ptIds);, 
                             glNormal3fv(normals + 3**ptIds);, 
                             glColor3ubv(colors + 4**ptIds);,;,;,
                             float *normals = (float *)voidNormals;,;
        );
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | 
        VTK_PDM_NORMALS | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawStripLinesMacro(float, float, float, rep, 
                             glVertex3fv(points + 3**ptIds);, 
                             glNormal3fv(normals + 3**ptIds);,;,
                             glTexCoord2fv(tcoords + 2**ptIds);,;,
                             float *normals = (float *)voidNormals;,
                             float *tcoords = (float *)voidTCoords;);
      break;
    default:
    {
    int j;
    vtkIdType nPts = 0;
    int count = 0;
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(nPts,ptIds); 
         count++)
      { 
      glBegin(rep);
      for (j = 0; j < nPts; j += 2) 
        {
        if (c)
          {
          if (idx & VTK_PDM_CELL_COLORS)
            {
            glColor4ubv(c->GetPointer(cellNum << 2));
            }
          else
            {
            glColor4ubv(c->GetPointer(ptIds[j] << 2));
            }
          }
        if (t)
          {
          glTexCoord2dv(t->GetTuple(ptIds[j]));
          }
        if (n)
          {
          if (idx & VTK_PDM_CELL_NORMALS)
            {
            glNormal3dv(n->GetTuple(cellNum));
            }
          else
            {
            glNormal3dv(n->GetTuple(ptIds[j]));
            }
          }
        else
          {
          if ( j == 0 )
            {
            vtkTriangle::ComputeNormal(p, 3, ptIds, polyNorm);
            }
          else
            {
            normIdx[0] = ptIds[j-2]; normIdx[1] = ptIds[j-1]; 
            normIdx[2] = ptIds[j]; 
            vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm);
            }
          glNormal3dv(polyNorm);
          }
        glVertex3dv(p->GetPoint(ptIds[j]));
        }
      glEnd();
      
      glBegin(rep);
      for (j = 1; j < nPts; j += 2) 
        {
        if (c)
          {
          if (idx & VTK_PDM_CELL_COLORS)
            {
            glColor4ubv(c->GetPointer(cellNum << 2));
            }
          else
            {
            glColor4ubv(c->GetPointer(ptIds[j] << 2));
            }
          }
        if (t)
          {
          glTexCoord2dv(t->GetTuple(ptIds[j]));
          }
        if (n)
          {
          if (idx & VTK_PDM_CELL_NORMALS)
            {
            glNormal3dv(n->GetTuple(cellNum));
            }
          else
            {
            glNormal3dv(n->GetTuple(ptIds[j]));
            }
          }
        else
          {
          if (j == 1)
            {
            vtkTriangle::ComputeNormal(p, 3, ptIds, polyNorm);
            }
          else
            {
            normIdx[0] = ptIds[j-2]; normIdx[1] = ptIds[j]; 
            normIdx[2] = ptIds[j-1]; 
            vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm);
            }
          glNormal3dv(polyNorm);
          }
        glVertex3dv(p->GetPoint(ptIds[j]));
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
      ++cellNum;
      }
    }  
    }
}

// Draw method for OpenGL.
int vtkOpenGLPolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkOpenGLRenderer *ren = (vtkOpenGLRenderer *)aren;
  int rep, interpolation;
  float tran;
  vtkProperty *prop;
  vtkPoints *p;
  vtkUnsignedCharArray *c=NULL;
  vtkDataArray *n;
  vtkDataArray *t;
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
    return noAbort;
    }

  // get the representation (e.g., surface / wireframe / points)
  rep = prop->GetRepresentation();

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
    GLenum lmcolorMode;
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT)
      {
      if (prop->GetAmbient() > prop->GetDiffuse())
        {
        lmcolorMode = GL_AMBIENT;
        }
      else
        {
        lmcolorMode = GL_DIFFUSE;
        }
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE)
      {
      lmcolorMode = GL_AMBIENT_AND_DIFFUSE;
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT)
      {
      lmcolorMode = GL_AMBIENT;
      }
    else // if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE)
      {
      lmcolorMode = GL_DIFFUSE;
      } 
    glColorMaterial( GL_FRONT_AND_BACK, lmcolorMode);
    glEnable( GL_COLOR_MATERIAL );
    }
  
  unsigned long idx = 0;
  if (n && !cellNormals)
    {
    idx |= VTK_PDM_NORMALS;
    }
  if (c)
    {
    idx |= VTK_PDM_COLORS;
    if (c->GetName())
      { // In the future, I will look at the number of components.
      // All paths will have to handle 3 componet colors.
      idx |= VTK_PDM_OPAQUE_COLORS;
      }
    }
  if (t)
    {
    idx |= VTK_PDM_TCOORDS;
    }
  if (cellScalars)
    {
    idx |= VTK_PDM_CELL_COLORS;
    }
  if (cellNormals)
    {
    idx |= VTK_PDM_CELL_NORMALS;
    }
  
  // store the types in the index
  if (p->GetDataType() == VTK_FLOAT)
    {
    idx |= VTK_PDM_POINT_TYPE_FLOAT;
    }
  else if (p->GetDataType() == VTK_DOUBLE)
    {
    idx |= VTK_PDM_POINT_TYPE_DOUBLE;
    }
  if (n)
    {
    if (n->GetDataType() == VTK_FLOAT)
      {
      idx |= VTK_PDM_NORMAL_TYPE_FLOAT;
      }
    else if (n->GetDataType() == VTK_DOUBLE)
      {
      idx |= VTK_PDM_NORMAL_TYPE_DOUBLE;
      }
    }
  if (t)
    {
    if (t->GetDataType() == VTK_FLOAT)
      {
      idx |= VTK_PDM_TCOORD_TYPE_FLOAT;
      }
    else if (t->GetDataType() == VTK_DOUBLE)
      {
      idx |= VTK_PDM_TCOORD_TYPE_DOUBLE;
      }
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
      double f, u;
      glEnable(GL_POLYGON_OFFSET_FILL);
      this->GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
      glPolygonOffset(f,u);
#endif      
      }
    }

  // For verts or lines that have no normals, disable shading.
  // This will fall back on the color set in the glColor4fv() 
  // call in vtkOpenGLProperty::Render() - the color returned
  // by vtkProperty::GetColor() with alpha set to 1.0.
  if (!n)
    {
    glDisable( GL_LIGHTING);
    }

  vtkOpenGLPolyDataMapperDrawPoints(idx,p,n,c,t,cellNum,noAbort,
                                    input->GetVerts(), ren);
  
  // do lines
  if ( zResolve )
    {
    glDepthRange(zRes, 1.);
    }
  if (rep == VTK_POINTS)
    {
    vtkOpenGLPolyDataMapperDrawPoints(idx,p,n,c,t,cellNum, noAbort,
                                      input->GetLines(), ren);
    }
  else
    {
    vtkOpenGLPolyDataMapperDrawLines(idx,p,n,c,t,cellNum, noAbort,
                                     input->GetLines(), ren);
    }
  
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
  
  // do polys
  if (rep == VTK_POINTS)
    {
    vtkOpenGLPolyDataMapperDrawPoints(idx,p,n,c,t,cellNum, noAbort,
                                      input->GetPolys(), ren);
    }
  else if (rep == VTK_WIREFRAME)
    {
    vtkOpenGLPolyDataMapperDrawPolygons(idx,p,n,c,t,cellNum, noAbort, 
                                        GL_LINE_LOOP, input->GetPolys(), ren);
    }
  else
    {
    vtkOpenGLPolyDataMapperDrawPolygons(idx,p,n,c,t,cellNum, noAbort,
                                        GL_POLYGON, input->GetPolys(), ren);
    }
  

  // do tstrips
  if ( zResolve )
    {
    glDepthRange(2*zRes, 1.);
    }
  if (rep == VTK_POINTS)
    {
    vtkOpenGLPolyDataMapperDrawPoints(idx,p,n,c,t,cellNum, noAbort, 
                                      input->GetStrips(), ren);
    }
  else if (rep == VTK_WIREFRAME)
    {
    vtkIdType oldCellNum = cellNum;
    vtkOpenGLPolyDataMapperDrawTStrips(idx,p,n,c,t,cellNum, noAbort,
                                       GL_LINE_STRIP, input->GetStrips(), 
                                       ren);
    vtkOpenGLPolyDataMapperDrawTStripLines(idx,p,n,c,t,oldCellNum, noAbort,
                                           GL_LINE_STRIP, input->GetStrips(), 
                                           ren);
    }
  else
    {
    vtkOpenGLPolyDataMapperDrawTStrips(idx,p,n,c,t,cellNum, noAbort,
                                       GL_TRIANGLE_STRIP, input->GetStrips(), 
                                       ren);
    }

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

  return noAbort;
}

void vtkOpenGLPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

  











