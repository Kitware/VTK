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
#include "vtkFloatArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkTimerLog.h"
#include "vtkTriangle.h"
#include "vtkOpenGLTexture.h"
#include "vtkImageData.h"
#include "vtkWindow.h"
#include "vtkRenderWindow.h"

#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"

#include <math.h>

vtkStandardNewMacro(vtkOpenGLPolyDataMapper);

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
#define VTK_PDM_TCOORD_1D          0x800
#define VTK_PDM_OPAQUE_COLORS      0x1000
#define VTK_PDM_USE_FIELD_DATA     0x2000

// Construct empty object.
vtkOpenGLPolyDataMapper::vtkOpenGLPolyDataMapper()
{
  this->ListId = 0;
  this->TotalCells = 0;
  this->InternalColorTexture = 0;
}

// Destructor (don't call ReleaseGraphicsResources() since it is virtual
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }
  if (this->InternalColorTexture)
    { // Resources released previously.
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
    }
}

// Release the graphics resources used by this mapper.  In this case, release
// the display list if any.
void vtkOpenGLPolyDataMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->ListId && win && win->GetMapped())
    {
    win->MakeCurrent();
    glDeleteLists(this->ListId, 1);
    vtkOpenGLCheckErrorMacro("failed after glDeleteLists");
    }
  this->ListId = 0;
  this->LastWindow = NULL;
  // We may not want to do this here.
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->ReleaseGraphicsResources(win);
    }
}

//
// Receives from Actor -> maps data to primitives
//
void vtkOpenGLPolyDataMapper::RenderPiece(vtkRenderer *ren, vtkActor *act)
{
  vtkOpenGLClearErrorMacro();

  vtkPolyData *input= this->GetInput();

  // make sure that we've been properly initialized
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  if (!input)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    if (!this->Static)
      {
      this->GetInputAlgorithm()->Update();
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    vtkIdType numPts = input->GetNumberOfPoints();
    if (numPts == 0)
      {
      vtkDebugMacro(<< "No points!");
      return;
      }
    }

  if (!this->LookupTable)
    {
    this->CreateDefaultLookupTable();
    }

  // make sure our window is current
  ren->GetRenderWindow()->MakeCurrent();

  // add all the clipping planes
  int numClipPlanes = this->GetNumberOfClippingPlanes();
  if (numClipPlanes > 6)
    {
    vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
    numClipPlanes = 6;
    }

  for (int i = 0; i < numClipPlanes; i++)
    {
    double planeEquation[4];
    this->GetClippingPlaneInDataCoords(act->GetMatrix(), i, planeEquation);
    GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0 + i);
    glEnable(clipPlaneId);
    glClipPlane(clipPlaneId, planeEquation);
    }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(act->GetProperty()->GetOpacity());
  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
    {
    if (this->InternalColorTexture == 0)
      {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
      }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
    // Keep color from interacting with texture.
    float info[4] = {1.f, 1.f, 1.f, 1.f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, info);
    }

  //
  // if something has changed regenerate colors and display lists
  // if required
  //
  int noAbort = 1;
  if ( this->GetMTime() > this->BuildTime ||
       input->GetMTime() > this->BuildTime ||
       act->GetProperty()->GetMTime() > this->BuildTime ||
       ren->GetRenderWindow() != this->LastWindow)
    {
    if (!this->ImmediateModeRendering &&
        !this->GetGlobalImmediateModeRendering())
      {
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->LastWindow = ren->GetRenderWindow();

      // If we are coloring by texture, then load the texture map.
      // Use Map as indicator, because texture hangs around.
      if (this->ColorTextureMap)
        {
        this->InternalColorTexture->Load(ren);
        }

      // get a unique display list id
      this->ListId = glGenLists(1);
      glNewList(this->ListId, GL_COMPILE);

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
      // If we are coloring by texture, then load the texture map.
      // Use Map as indicator, because texture hangs around.
      if (this->ColorTextureMap)
        {
        this->InternalColorTexture->Load(ren);
        }

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
    // If we are coloring by texture, then load the texture map.
    // Use Map as indicator, because texture hangs around.
    if (this->ColorTextureMap)
      {
      this->InternalColorTexture->Load(ren);
      }
    // Time the actual drawing
    this->Timer->StartTimer();
    this->Draw(ren,act);
    this->Timer->StopTimer();
    }

  this->TimeToDraw = this->Timer->GetElapsedTime();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
    {
    this->TimeToDraw = 0.0001;
    }

  for (int c = 0; c < numClipPlanes; c++)
    {
    GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0 + c);
    glDisable(clipPlaneId);
    }

  vtkOpenGLCheckErrorMacro("failed after RenderPiece");
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

//-----------------------------------------

#define vtkDrawPointsMacro(ptype,ntype,glVertFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  ptype *points = static_cast<ptype *>(voidPoints);     \
  glInitFuncs \
  glBegin(GL_POINTS); \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ++ptIds; \
      --nPts; \
      } \
    if (++count == 10000) \
      { \
      cellNum += 10000; \
      count = 0; \
      this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells); \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        noAbort = 0; \
        break; \
        } \
      } \
    } \
  cellNum += count; \
  glEnd(); \
}

#define vtkDrawPrimsMacro(ptype,ntype,prim,glVertFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  ptype *points = static_cast<ptype *>(voidPoints);    \
  glInitFuncs \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    glBegin(prim); \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ++ptIds; \
      --nPts; \
      } \
    glEnd(); \
    if (++count == 10000) \
      { \
      cellNum += 10000; \
      count = 0; \
      this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells); \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        noAbort = 0; \
        break; \
        } \
      } \
    } \
  cellNum += count; \
}

#define vtkDrawPolysMacro(ptype,ntype,ttype,prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  ptype *points = static_cast<ptype *>(voidPoints);    \
  GLenum previousGlFunction=GL_INVALID_VALUE; \
  glInitFuncs \
while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousGlFunction, nPts ); \
    glCellFuncs \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ++ptIds; \
      --nPts; \
      } \
    if (++count == 10000) \
      { \
      cellNum += 10000; \
      count = 0; \
      this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells); \
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

#define vtkDrawPolysMacro4Tri(ptype,ntype,ttype,prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  ptype *points = static_cast<ptype *>(voidPoints);     \
  GLenum previousGlFunction=GL_INVALID_VALUE; \
  glInitFuncs \
  \
  double quad_center[3] = {0, 0, 0}; \
  double quad_center_col[4] = {0, 0, 0, 0}; \
  double quad_points[4][3]; \
  double quad_points_col[4][4]; \
  double dist_center[4] = {0, 0, 0, 0}; \
  \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
  ++ptIds; \
  /* If we don't want to draw a QUAD (ex : a triangle nPts = 3) */ \
  if (nPts != 4) { \
  /* Classic method */ \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousGlFunction, nPts ); \
    glCellFuncs \
    while (nPts > 0) \
    { \
    glVertFuncs \
    ++ptIds; \
    --nPts; \
    } \
  } \
  /* If we want to draw a QUAD */ \
  else { \
  /* We launch glBegin(GL_TRIANGLES) mode in order to draw 4 triangles */ \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousGlFunction, 3 ); \
    glCellFuncs \
  /* We keep pointer on the first point of the first triangle */ \
  /* ptIdsFirstPtQuad will be used for center calculation and for 2nd point of 4th triangle */ \
    vtkIdType *ptIdsFirstPtQuad; \
    ptIdsFirstPtQuad = ptIds; \
  /* QUAD Center calculation */ \
  /* We save the 4 QUAD points and their color */ \
    GLfloat *vpt; \
    GLubyte *vcol; \
    for (int i=0; i<4; i++) { \
      /* Position : */ \
      vpt = points + 3**ptIds; \
      quad_points[i][0] = vpt[0]; \
      quad_points[i][1] = vpt[1]; \
      quad_points[i][2] = vpt[2]; \
      /* Color : */ \
      vcol = colors + 4**ptIds; \
      quad_points_col[i][0] = vcol[0]; \
      quad_points_col[i][1] = vcol[1]; \
      quad_points_col[i][2] = vcol[2]; \
      quad_points_col[i][3] = vcol[3]; \
      ++ptIds; \
    } \
  /* Actual calculation of QUAD center with the 4 summits */ \
    quad_center[0] = (quad_points[0][0] + quad_points[1][0] + quad_points[2][0] + quad_points[3][0])/4; \
    quad_center[1] = (quad_points[0][1] + quad_points[1][1] + quad_points[2][1] + quad_points[3][1])/4; \
    quad_center[2] = (quad_points[0][2] + quad_points[1][2] + quad_points[2][2] + quad_points[3][2])/4; \
  /* Color center calculation  (Interpolation on each component of RGB vector) */ \
  /* Calculation of distances between center and summits */ \
    for (int i=0; i<4; i++) { \
      dist_center[i] = sqrt((quad_points[i][0] - quad_center[0])*(quad_points[i][0] - quad_center[0]) + \
                (quad_points[i][1] - quad_center[1])*(quad_points[i][1] - quad_center[1]) + \
                (quad_points[i][2] - quad_center[2])*(quad_points[i][2] - quad_center[2])); \
    } \
  /* Color interpolation (3 for RGB and 1 for Alpha transparency) */ \
    for (int i=0; i<4; i++) { \
      quad_center_col[i] = ((dist_center[3]*quad_points_col[1][i] + dist_center[1]*quad_points_col[3][i])/(dist_center[1] + dist_center[3]) + \
                (dist_center[2]*quad_points_col[0][i] + dist_center[0]*quad_points_col[2][i])/(dist_center[2] + dist_center[0]) \
                )/2; \
    } \
  /* We take pointer on the first QUAD point */ \
    ptIds = ptIdsFirstPtQuad; \
  /* Actual drawing of 4 triangles */ \
    for (int i=0; i<4; i++) { \
      /* 1st point */ \
      glVertFuncs \
      ++ptIds; \
      /* 2nd point */ \
      if (i >= 3) { /* If it is the last triangle */ \
        /* this 2nd point = the 1st point of 1st triangle */ \
        glColor3ubv(colors + 4**ptIdsFirstPtQuad); \
        glVertex3fv(static_cast<float*>(points) + 3**ptIdsFirstPtQuad); \
      } \
      else { \
        /* Else 2nd point = next point */ \
        glVertFuncs \
      } \
      /* 3rd point */ \
      glColor4f(quad_center_col[0],quad_center_col[1],quad_center_col[2],quad_center_col[3]); \
      glVertex3f(quad_center[0],quad_center[1],quad_center[2]); \
    } \
  } /* End of if (nPts == 4) */ \
    if (++count == 10000) \
      { \
      cellNum += 10000; \
      count = 0; \
      this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells); \
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

#define vtkDrawPolysMacro4TriTex(ptype,ntype,ttype,prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  ptype *points = static_cast<ptype *>(voidPoints);     \
  GLenum previousGlFunction=GL_INVALID_VALUE; \
  glInitFuncs \
  \
double quad_center[3] = {0, 0, 0}; \
double quad_center_tex = 0; \
double quad_points[4][3]; \
double quad_points_tex[4]; \
double dist_center[4] = {0, 0, 0, 0}; \
  \
while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
  ++ptIds; \
  /* If we don't want to draw a QUAD (ex : a triangle nPts = 3) */ \
  if (nPts != 4) { \
  /* Classic method */ \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousGlFunction, nPts ); \
    glCellFuncs \
    while (nPts > 0) \
    { \
    glVertFuncs \
    ++ptIds; \
    --nPts; \
    } \
  } \
  /* If we want to draw a QUAD */ \
  else { \
    /* We launch glBegin(GL_TRIANGLES) mode in order to draw 4 triangles */ \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousGlFunction, 3 ); \
    glCellFuncs \
    /* We keep pointer on the first point of the first triangle */ \
    /* ptIdsFirstPtQuad will be used for center calculation and for 2nd point of 4th triangle */ \
    vtkIdType *ptIdsFirstPtQuad; \
    ptIdsFirstPtQuad = ptIds; \
  /* QUAD Center calculation */ \
  /* We save the 4 QUAD points and their texture value */ \
    GLfloat *vpt; \
    GLfloat *vtex; \
    for (int i=0; i<4; i++) { \
      /* Position : */ \
      vpt = points + 3**ptIds; \
      quad_points[i][0] = vpt[0]; \
      quad_points[i][1] = vpt[1]; \
      quad_points[i][2] = vpt[2]; \
      /* Texture : */ \
      vtex = tcoords + *ptIds; \
      quad_points_tex[i] = vtex[0]; \
      ++ptIds; \
    } \
  /* Actual calculation of QUAD center with the 4 summits */ \
    quad_center[0] = (quad_points[0][0] + quad_points[1][0] + quad_points[2][0] + quad_points[3][0])/4; \
    quad_center[1] = (quad_points[0][1] + quad_points[1][1] + quad_points[2][1] + quad_points[3][1])/4; \
    quad_center[2] = (quad_points[0][2] + quad_points[1][2] + quad_points[2][2] + quad_points[3][2])/4; \
  /* Texture center calculation  (Interpolation on each component of RGB vector) */ \
  /* Calculation of distances between center and summits */ \
    for (int i=0; i<4; i++) { \
      dist_center[i] = sqrt((quad_points[i][0] - quad_center[0])*(quad_points[i][0] - quad_center[0]) + \
                (quad_points[i][1] - quad_center[1])*(quad_points[i][1] - quad_center[1]) + \
                (quad_points[i][2] - quad_center[2])*(quad_points[i][2] - quad_center[2])); \
    } \
  /* Texture interpolation */ \
    quad_center_tex = ((dist_center[3]*quad_points_tex[1] + dist_center[1]*quad_points_tex[3])/(dist_center[1] + dist_center[3]) + \
              (dist_center[2]*quad_points_tex[0] + dist_center[0]*quad_points_tex[2])/(dist_center[2] + dist_center[0]) \
              )/2; \
  /* We take pointer on the first QUAD point */ \
    ptIds = ptIdsFirstPtQuad; \
  /* Actual drawing of 4 triangles */ \
    for (int i=0; i<4; i++) { \
      /* 1st point */ \
      glVertFuncs \
      ++ptIds; \
      /* 2nd point */ \
      if (i >= 3) { /* If it is the last triangle */ \
        /* this 2nd point = the 1st point of 1st triangle */ \
        glTexCoord1fv(tcoords + *ptIdsFirstPtQuad); \
        glVertex3fv(points + 3**ptIdsFirstPtQuad); \
      } \
      else { \
        /* Else 2nd point = next point */ \
        glVertFuncs \
      } \
      /* 3rd point */ \
      glTexCoord1f(quad_center_tex); \
      glVertex3f(quad_center[0],quad_center[1],quad_center[2]); \
    } \
  } /* End of if (nPts == 4) */ \
    if (++count == 10000) \
      { \
      cellNum += 10000; \
      count = 0; \
      this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells); \
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

#define vtkDrawStripLinesMacro(ptype,ntype,ttype,prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; \
  ptype *points = static_cast<ptype *>(voidPoints);     \
  vtkIdType *savedPtIds = ptIds; \
  glInitFuncs \
  while (ptIds < endPtIds) \
    { \
    glBegin(prim); \
    nPts = *ptIds; \
    ++ptIds; \
    glCellFuncs \
    while (nPts > 0) \
      { \
      glVertFuncs \
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
    glCellFuncs \
    ++ptIds; \
    --nPts; \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ptIds += 2; \
      nPts -= 2; \
      } \
    glEnd(); \
    ptIds += nPts; /* nPts could be 0 or -1 here */ \
    } \
}

void vtkOpenGLPolyDataMapper::DrawPoints(int idx,
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
  if (ca->GetNumberOfCells() == 0)
    {
    return;
    }
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
      vtkDrawPointsMacro(float, float, glVertex3fv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPointsMacro(double, float, glVertex3dv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPointsMacro(float, float,
                         glNormal3fv(normals + 3**ptIds);
                         glVertex3fv(points + 3**ptIds);,
                         float *normals = static_cast<float *>(voidNormals););

      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPointsMacro(float, float,
                         glColor4ubv(colors + 4**ptIds);
                         glVertex3fv(points + 3**ptIds);,;);

      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPointsMacro(float, float,
                         glColor3ubv(colors + 4**ptIds);
                         glVertex3fv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPointsMacro(float, float,
                         glNormal3fv(normals + 3**ptIds);
                         glColor4ubv(colors + 4**ptIds);
                         glVertex3fv(points + 3**ptIds);,
                         float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPointsMacro(float, float,
                         glNormal3fv(normals + 3**ptIds);
                         glColor3ubv(colors + 4**ptIds);
                         glVertex3fv(points + 3**ptIds);,
                         float *normals = static_cast<float *>(voidNormals););
      break;
    default:
    {
    int j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    unsigned short count = 0;
    glBegin(GL_POINTS);
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(npts,pts);
         count++)
      {
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
            glColor4ubv(c->GetPointer(pts[j]<< 2));
            }
          }
        if (t)
          {
          if (idx & VTK_PDM_TCOORD_1D)
            {
            glTexCoord1dv(t->GetTuple(pts[j]));
            }
          else
            {
            glTexCoord2dv(t->GetTuple(pts[j]));
            }
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

      // check for abort condition
      if (count == 10000)
        {
        count = 0;
        // report progress
        this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells);
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


void vtkOpenGLPolyDataMapper::DrawLines(int idx,
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
  void *voidTCoords = 0;
  unsigned char *colors = 0;
  if (ca->GetNumberOfCells() == 0)
    {
    return;
    }
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (t)
    {
    voidTCoords = t->GetVoidPointer(0);
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
                        glVertex3fv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPrimsMacro(double, float, GL_LINE_STRIP,
                        glVertex3dv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glNormal3fv(normals + 3**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        float *normals = static_cast<float *>(voidNormals););

      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glColor4ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glColor3ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glNormal3fv(normals + 3**ptIds);
                        glColor4ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        float *normals = static_cast<float *>(voidNormals);
        );
    break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glNormal3fv(normals + 3**ptIds);
                        glColor3ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        float *normals = static_cast<float *>(voidNormals);
        );
    break;
    case VTK_PDM_POINT_TYPE_FLOAT |
      VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORD_1D | VTK_PDM_TCOORDS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glTexCoord1fv(tcoords + *ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        float *tcoords = static_cast<float *>(voidTCoords);
        );
    break;
    case VTK_PDM_POINT_TYPE_FLOAT |
      VTK_PDM_NORMAL_TYPE_FLOAT | VTK_PDM_NORMALS |
      VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORD_1D | VTK_PDM_TCOORDS:
      vtkDrawPrimsMacro(float, float, GL_LINE_STRIP,
                        glNormal3fv(normals + 3**ptIds);
                        glTexCoord1fv(tcoords + *ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        float *tcoords = static_cast<float *>(voidTCoords);
                        float *normals = static_cast<float *>(voidNormals);
        );
    break;
    default:
    {
    int j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    unsigned short count = 0;
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(npts,pts);
         count++)
      {
      glBegin(GL_LINE_STRIP);
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
          if (idx & VTK_PDM_TCOORD_1D)
            {
            glTexCoord1dv(t->GetTuple(pts[j]));
            }
          else
            {
            glTexCoord2dv(t->GetTuple(pts[j]));
            }
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
      if (count == 10000)
        {
        count = 0;
        // report progress
        this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells);
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

void vtkOpenGLPolyDataMapper::DrawPolygons(int idx,
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
  vtkOpenGLClearErrorMacro();

  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  void *voidTCoords = 0;
  unsigned char *colors = 0;
  if (ca->GetNumberOfCells() == 0)
    {
    return;
    }
  if (n)
    {
    voidNormals = n->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    // if these are cell colors then advance to the first cell
    if (idx & VTK_PDM_CELL_COLORS)
      {
      colors = colors + cellNum*4;
      }
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
                        glVertex3fv(points + 3**ptIds);,
                        PolyNormal,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPolysMacro(double, float, float, rep,
                        glVertex3dv(points + 3**ptIds);,
                        PolyNormal,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glColor4ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        PolyNormal,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro4Tri(float, float, float, rep,
                        glColor3ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        PolyNormal,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glColor4ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glColor3ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | VTK_PDM_NORMALS |
         VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORD_1D | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glTexCoord1fv(tcoords + *ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals);
                        float *tcoords = static_cast<float *>(voidTCoords););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | VTK_PDM_CELL_NORMALS |
         VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORD_1D | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glTexCoord1fv(tcoords + *ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals); normals += 3;,
                        float *tcoords = static_cast<float *>(voidTCoords);
                        float *normals = static_cast<float *>(voidNormals);
                        normals += cellNum*3;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT |
         VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORD_1D | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro4TriTex(float, float, float, rep,
                        glTexCoord1fv(tcoords + *ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        PolyNormal;,
                               float *tcoords = static_cast<float *>(voidTCoords););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
        VTK_PDM_NORMALS | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glTexCoord2fv(tcoords + 2**ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals);
                        float *tcoords = static_cast<float *>(voidTCoords););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|
      VTK_PDM_CELL_NORMALS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals); normals += 3;,
                        float *normals = static_cast<float *>(voidNormals);
                        normals += cellNum*3;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glColor4ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals); normals += 3;,
                        float *normals = static_cast<float *>(voidNormals);
                        normals += cellNum*3;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glColor3ubv(colors + 4**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals); normals += 3;,
                        float *normals = static_cast<float *>(voidNormals);
                        normals += cellNum*3;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        glColor4ubv(colors); colors += 4;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS |
      VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glVertex3fv(points + 3**ptIds);,
                        glColor3ubv(colors); colors += 4;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals); normals += 3;
                        glColor4ubv(colors); colors += 4;,
                        float *normals = static_cast<float *>(voidNormals);
                        normals += cellNum*3;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS |
      VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glVertex3fv(points + 3**ptIds);,
                        glNormal3fv(normals); normals += 3;
                        glColor3ubv(colors); colors += 4;,
                        float *normals = static_cast<float *>(voidNormals);
                        normals += cellNum*3;);
      break;
    default:
    {
    int j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    unsigned short count = 0;
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
          if (idx & VTK_PDM_TCOORD_1D)
            {
            glTexCoord1dv(t->GetTuple(pts[j]));
            }
          else
            {
            glTexCoord2dv(t->GetTuple(pts[j]));
            }
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
      if (count == 10000)
        {
        count = 0;
        // report progress
        this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells);
        if (ren->GetRenderWindow()->CheckAbortStatus())
          {
          noAbort = 0;
          }
        }
      ++cellNum;
      }
    }
    }
  vtkOpenGLCheckErrorMacro("failed after DrawPolygons");
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

void vtkOpenGLPolyDataMapper::DrawTStrips(int idx,
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
  vtkOpenGLClearErrorMacro();
  void *voidPoints = p->GetVoidPointer(0);
  void *voidNormals = 0;
  void *voidTCoords = 0;
  unsigned char *colors = 0;
  double polyNorm[3];
  vtkIdType normIdx[3];

  if (ca->GetNumberOfCells() == 0)
    {
    return;
    }
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
                        TStripNormal glVertex3fv(points + 3**ptIds);,
                        TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawPolysMacro(double, float, float, rep,
                        TStripNormal glVertex3dv(points + 3**ptIds);,
                        TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        TStripNormal
                        glColor4ubv(colors + (*ptIds << 2));
                        glVertex3fv(points + 3**ptIds);,
                        TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        TStripNormal
                        glColor3ubv(colors + (*ptIds << 2));
                        glVertex3fv(points + 3**ptIds);,
                        TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glColor4ubv(colors + (*ptIds << 2));
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glColor3ubv(colors + (*ptIds << 2));
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | VTK_PDM_NORMALS |
        VTK_PDM_TCOORD_1D | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glTexCoord1fv(tcoords + *ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals);
                        float *tcoords = static_cast<float *>(voidTCoords););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
        VTK_PDM_NORMALS | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(float, float, float, rep,
                        glNormal3fv(normals + 3**ptIds);
                        glTexCoord2fv(tcoords + 2**ptIds);
                        glVertex3fv(points + 3**ptIds);,;,
                        float *normals = static_cast<float *>(voidNormals);
                        float *tcoords = static_cast<float *>(voidTCoords););
      break;
    default:
    {
    int j;
    vtkIdType nPts = 0;
    unsigned short count = 0;
    unsigned long coloroffset = cellNum;
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
          if ( (idx & VTK_PDM_USE_FIELD_DATA) && j>=2 )
            {
            glColor4ubv(c->GetPointer(coloroffset << 2));
            coloroffset++;
            }
          else if (idx & VTK_PDM_CELL_COLORS)
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
          if (idx & VTK_PDM_TCOORD_1D)
            {
            glTexCoord1dv(t->GetTuple(ptIds[j]));
            }
          else
            {
            glTexCoord2dv(t->GetTuple(ptIds[j]));
            }
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
      if (count == 10000)
        {
        count = 0;
        // report progress
        this->UpdateProgress(static_cast<double>(cellNum)/this->TotalCells);
        if (ren->GetRenderWindow()->CheckAbortStatus())
          {
          noAbort = 0;
          }
        }
      ++cellNum;
      }
    }
    }
  vtkOpenGLCheckErrorMacro("failed after DrawTStrips");
}

static void vtkOpenGLPolyDataMapperDrawTStripLines(int idx,
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
  vtkOpenGLClearErrorMacro();
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
                             TStripNormal; glVertex3fv(points + 3**ptIds);,
                             TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_DOUBLE:
      vtkDrawStripLinesMacro(double, float, float, rep,
                        TStripNormal glVertex3dv(points + 3**ptIds);,
                        TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT|VTK_PDM_NORMAL_TYPE_FLOAT|VTK_PDM_NORMALS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             glNormal3fv(normals + 3**ptIds);
                             glVertex3fv(points + 3**ptIds);,;,
                             float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             TStripNormal;
                             glColor4ubv(colors + 4**ptIds);
                             glVertex3fv(points + 3**ptIds);,
                             TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             TStripNormal;
                             glColor3ubv(colors + 4**ptIds);
                             glVertex3fv(points + 3**ptIds);,
                             TStripNormalStart,;);
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             glNormal3fv(normals + 3**ptIds);
                             glColor4ubv(colors + 4**ptIds);
                             glVertex3fv(points + 3**ptIds);,;,
                             float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
      VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             glNormal3fv(normals + 3**ptIds);
                             glColor3ubv(colors + 4**ptIds);
                             glVertex3fv(points + 3**ptIds);,;,
                             float *normals = static_cast<float *>(voidNormals););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT | VTK_PDM_NORMALS |
      VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORD_1D | VTK_PDM_TCOORDS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             glNormal3fv(normals + 3**ptIds);
                             glTexCoord1fv(tcoords + *ptIds);
                             glVertex3fv(points + 3**ptIds);,;,
                             float *normals = static_cast<float *>(voidNormals);
                             float *tcoords = static_cast<float *>(voidTCoords););
      break;
    case VTK_PDM_POINT_TYPE_FLOAT | VTK_PDM_NORMAL_TYPE_FLOAT |
        VTK_PDM_NORMALS | VTK_PDM_TCOORD_TYPE_FLOAT | VTK_PDM_TCOORDS:
      vtkDrawStripLinesMacro(float, float, float, rep,
                             glNormal3fv(normals + 3**ptIds);
                             glTexCoord2fv(tcoords + 2**ptIds);
                             glVertex3fv(points + 3**ptIds);,;,
                             float *normals = static_cast<float *>(voidNormals);
                             float *tcoords = static_cast<float *>(voidTCoords););
      break;
    default:
    {
    int j;
    vtkIdType nPts = 0;
    int count = 0;
    unsigned long coloroffset = cellNum;
    for (ca->InitTraversal(); noAbort && ca->GetNextCell(nPts,ptIds);
         count++)
      {
      glBegin(rep);
      for (j = 0; j < nPts; j += 2)
        {
        if (c)
          {
          if ( (idx & VTK_PDM_USE_FIELD_DATA) && j >= 2)
            {
            glColor4ubv(c->GetPointer((coloroffset+j) << 2));
            }
          else if (idx & VTK_PDM_CELL_COLORS)
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
          if (idx & VTK_PDM_TCOORD_1D)
            {
            glTexCoord1dv(t->GetTuple(ptIds[j]));
            }
          else
            {
            glTexCoord2dv(t->GetTuple(ptIds[j]));
            }
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
          if ( (idx & VTK_PDM_USE_FIELD_DATA) && j >= 2)
            {
            glColor4ubv(c->GetPointer((coloroffset+j) << 2));
            }
          else if (idx & VTK_PDM_CELL_COLORS)
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
          if (idx & VTK_PDM_TCOORD_1D)
            {
            glTexCoord1dv(t->GetTuple(ptIds[j]));
            }
          else
            {
            glTexCoord2dv(t->GetTuple(ptIds[j]));
            }
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
      coloroffset += (nPts >= 2)? (nPts - 2) : 0;
      }
    }
    }
  vtkOpenGLStaticCheckErrorMacro("failed after DrawTStripLines");
}

// Draw method for OpenGL.
int vtkOpenGLPolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkOpenGLClearErrorMacro();
  vtkOpenGLRenderer *ren = static_cast<vtkOpenGLRenderer *>(aren);
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
  int cellNormals;
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
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !input->GetPointData()->GetScalars() )
         && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
      {
      cellScalars = 1;
      }
    }

  n = input->GetPointData()->GetNormals();
  if (interpolation == VTK_FLAT)
    {
    n = 0;
    }

  cellNormals = 0;
  if (n == 0 && input->GetCellData()->GetNormals())
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
  if (cellScalars)
    {
    idx |= VTK_PDM_CELL_COLORS;
    }
  if (cellNormals)
    {
    idx |= VTK_PDM_CELL_NORMALS;
    }
  if (this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
    {
    idx |= VTK_PDM_USE_FIELD_DATA;
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

  // Texture and color by texture
  t = input->GetPointData()->GetTCoords();
  if ( t )
    {
    tDim = t->GetNumberOfComponents();
    if (tDim > 2)
      {
      vtkDebugMacro(<< "Currently only 1d and 2d textures are supported.\n");
      t = NULL;
      }
    }
  // Set the texture if we are going to use texture
  // for coloring with a point attribute.
  // fixme ... make the existence of the coordinate array the signal.
  if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates &&
      ! (idx & VTK_PDM_CELL_COLORS))
    {
    t = this->ColorCoordinates;
    }
  // Set the flags
  if (t)
    {
    idx |= VTK_PDM_TCOORDS;
    if (t->GetDataType() == VTK_FLOAT)
      {
      idx |= VTK_PDM_TCOORD_TYPE_FLOAT;
      }
    else if (t->GetDataType() == VTK_DOUBLE)
      {
      idx |= VTK_PDM_TCOORD_TYPE_DOUBLE;
      }
    if (t->GetNumberOfComponents() == 1)
      {
      idx |= VTK_PDM_TCOORD_1D;
      }
    // Not 1D assumes 2D texture coordinates.
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

  // we need to know the total number of cells so that we can report progress
  this->TotalCells =
    input->GetVerts()->GetNumberOfCells() +
    input->GetLines()->GetNumberOfCells() +
    input->GetPolys()->GetNumberOfCells() +
    input->GetStrips()->GetNumberOfCells();

  // For verts or lines that have no normals, disable shading.
  // This will fall back on the color set in the glColor4fv()
  // call in vtkOpenGLProperty::Render() - the color returned
  // by vtkProperty::GetColor() with alpha set to 1.0.
  if (!n)
    {
    glDisable( GL_LIGHTING);
    }

  this->DrawPoints(idx,p,n,c,t,cellNum,noAbort,input->GetVerts(), ren);

  // do lines
  if ( zResolve )
    {
    glDepthRange(zRes, 1.);
    }
  if (rep == VTK_POINTS)
    {
    this->DrawPoints(idx,p,n,c,t,cellNum, noAbort,input->GetLines(), ren);
    }
  else
    {
    this->DrawLines(idx,p,n,c,t,cellNum, noAbort, input->GetLines(), ren);
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
  if (rep == VTK_POINTS && !prop->GetBackfaceCulling() && !prop->GetFrontfaceCulling() )
    {
    this->DrawPoints(idx,p,n,c,t,cellNum, noAbort, input->GetPolys(), ren);
    }
  else if (rep == VTK_WIREFRAME && !prop->GetBackfaceCulling() && !prop->GetFrontfaceCulling())
    {
    this->DrawPolygons(idx,p,n,c,t,cellNum, noAbort,
                       GL_LINE_LOOP, input->GetPolys(), ren);
    }
  else
    {
    this->DrawPolygons(idx,p,n,c,t,cellNum, noAbort,
                       GL_POLYGON, input->GetPolys(), ren);
    }


  // do tstrips
  if ( zResolve )
    {
    glDepthRange(2*zRes, 1.);
    }
  if (rep == VTK_POINTS && !prop->GetBackfaceCulling() && !prop->GetFrontfaceCulling() )
    {
    this->DrawPoints(idx,p,n,c,t,cellNum, noAbort, input->GetStrips(), ren);
    }
  else if (rep == VTK_WIREFRAME && !prop->GetBackfaceCulling() && !prop->GetFrontfaceCulling())
    {
    vtkIdType oldCellNum = cellNum;
    this->DrawTStrips(idx,p,n,c,t,cellNum, noAbort,
                      GL_LINE_STRIP, input->GetStrips(), ren);
    vtkOpenGLPolyDataMapperDrawTStripLines(idx,p,n,c,t,oldCellNum, noAbort,
                                           GL_LINE_STRIP, input->GetStrips(),
                                           ren);
    }
  else
    {
    this->DrawTStrips(idx,p,n,c,t,cellNum, noAbort,
                      GL_TRIANGLE_STRIP, input->GetStrips(), ren);
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

  vtkOpenGLCheckErrorMacro("failed after Draw");
  this->UpdateProgress(1.0);
  return noAbort;
}

void vtkOpenGLPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
