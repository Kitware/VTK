/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolygonsPainter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharArray.h"
vtkStandardNewMacro(vtkPolygonsPainter);

#define VTK_PP_INVALID_TYPE -1
//-----------------------------------------------------------------------------
vtkPolygonsPainter::vtkPolygonsPainter()
{
  this->SetSupportedPrimitive(vtkPainter::POLYS);
}

//-----------------------------------------------------------------------------
vtkPolygonsPainter::~vtkPolygonsPainter()
{
}

//-----------------------------------------------------------------------------
//
// Helper routine which starts a poly, triangle or quad based upon
// the number of points in the polygon and whether triangles or quads
// were the last thing being drawn (we can get better performance if we
// can draw several triangles within a single glBegin(GL_TRIANGLES) or
// several quads within a single glBegin(GL_QUADS).
//
static inline void vtkOpenGLBeginPolyTriangleOrQuad(int aPrimitive,
                                             int &previousPrimitive,
                                             int npts,
                                             vtkPainterDeviceAdapter* device)
{
  if (aPrimitive == VTK_POLYGON)
    {
    switch (npts)
      {
      case 3:  // Need to draw a triangle.
        if (previousPrimitive != VTK_TRIANGLE)
          {
          // we were not already drawing triangles, were we drawing quads?
          if (previousPrimitive == VTK_QUAD)
            {
            // we were previously drawing quads, close down the quads.
            device->EndPrimitive();
            }
          // start drawing triangles
          previousPrimitive = VTK_TRIANGLE;
          device->BeginPrimitive(VTK_TRIANGLE);
          }
          break;
      case 4:  // Need to draw a quad
        if (previousPrimitive != VTK_QUAD)
          {
          // we were not already drawing quads, were we drawing triangles?
          if (previousPrimitive == VTK_TRIANGLE)
            {
            // we were previously drawing triangles, close down the triangles.
            device->EndPrimitive();
            }
          // start drawing quads
          previousPrimitive = VTK_QUAD;
          device->BeginPrimitive(VTK_QUAD);
          }
        break;
      default:
        // if we were supposed to be drawing polygons but were really
        // drawing triangles or quads, then we need to close down the
        // triangles or quads and begin a polygon
        if (previousPrimitive != VTK_PP_INVALID_TYPE
            && previousPrimitive != VTK_POLYGON)
          {
          device->EndPrimitive();
          }
        previousPrimitive = VTK_POLYGON;
        device->BeginPrimitive(VTK_POLYGON);
        break;
      }
    }
  else if (aPrimitive == VTK_VERTEX || aPrimitive == VTK_POLY_VERTEX)
    {
    // we are supposed to be drawing points
    if (previousPrimitive != VTK_VERTEX && previousPrimitive != VTK_POLY_VERTEX)
      {
      // We were not drawing points before this, switch to points.
      // We don't need to worry about switching from triangles or quads
      // since draw all points before drawing any polygons (i.e. in the polys
      // case we switch to triangles and quads as an optimization, there is
      // nothing to switch to that is below points).
      previousPrimitive = VTK_VERTEX;
      device->BeginPrimitive(VTK_VERTEX);
      }
    }
  else
    {
    previousPrimitive = aPrimitive;
    device->BeginPrimitive(aPrimitive);
    }
}

//-----------------------------------------------------------------------------
#define vtkDrawPolysMacro(prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  int previousPrimitive = VTK_PP_INVALID_TYPE; \
  glInitFuncs \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    vtkOpenGLBeginPolyTriangleOrQuad( prim, previousPrimitive, nPts, device); \
    glCellFuncs \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ++ptIds; \
      --nPts; \
      } \
    cellNum++;\
    if (++count == 10000) \
      { \
      count = 0; \
      this->UpdateProgress(static_cast<double>(cellNum-cellNumStart)/totalCells);\
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        break; \
        } \
      } \
    if ((previousPrimitive != VTK_TRIANGLE)  \
        && (previousPrimitive != VTK_QUAD)   \
        && (previousPrimitive != VTK_VERTEX)) \
      {  \
      device->EndPrimitive(); \
      } \
    } \
  if ((previousPrimitive == VTK_TRIANGLE)  \
      || (previousPrimitive == VTK_QUAD)   \
      || (previousPrimitive == VTK_VERTEX)) \
    { \
    device->EndPrimitive(); \
    } \
}

//-----------------------------------------------------------------------------
// used to build normals when normals are missing.
#define PolyNormal \
{ double polyNorm[3]; vtkPolygon::ComputeNormal(p,nPts,ptIds,polyNorm); \
  device->SendAttribute(vtkPointData::NORMALS, 3, VTK_DOUBLE, polyNorm); }

//-----------------------------------------------------------------------------
int vtkPolygonsPainter::RenderPrimitive(unsigned long idx, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren)
{
  vtkPolyData* pd = this->GetInputAsPolyData();
  vtkPoints* p = pd->GetPoints();
  vtkCellArray* ca = pd->GetPolys();
  vtkIdType cellNum = pd->GetVerts()->GetNumberOfCells() +
    pd->GetLines()->GetNumberOfCells();
  vtkIdType cellNumStart = cellNum;
  vtkIdType totalCells = ca->GetNumberOfCells();
  vtkUnsignedCharArray *ef = vtkUnsignedCharArray::SafeDownCast(
              pd->GetPointData()->GetAttribute(vtkDataSetAttributes::EDGEFLAG));

  vtkPainterDeviceAdapter* device = ren->GetRenderWindow()->
    GetPainterDeviceAdapter();
  void *points = p->GetVoidPointer(0);
  void *normals = 0;
  void *tcoords = 0;
  unsigned char *colors = 0;
  unsigned char *edgeflags = 0;
  int primitive = VTK_POLYGON;

  if (ca->GetNumberOfCells() == 0)
    {
    return 1;
    }
  if (n)
    {
    normals = n->GetVoidPointer(0);
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
    tcoords = t->GetVoidPointer(0);
    }
  if (ef)
    {
    edgeflags = ef->GetPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();
  int ptype = p->GetDataType();
  int ntype = (n)? n->GetDataType() : 0;
  int ttype = (t)? t->GetDataType() : 0;
  int tcomps = (t)? t->GetNumberOfComponents() : 0;
  int eftype = (ef)? ef->GetDataType() : 0;
  int celloffset = 0;

  // since this painter does not deal with field colors specially,
  // we just ignore the flag.
  idx &= (~VTK_PDM_FIELD_COLORS);

  // draw all the elements, use fast path if available
  switch (idx)
    {
    case 0:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
            ptype, points, 3**ptIds);, PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
            ptype, points, 3**ptIds);, ;,;);
        }
      break;

    case VTK_PDM_NORMALS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;,;);
      break;

    case VTK_PDM_COLORS:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
          device->SendAttribute(vtkPointData::SCALARS, 4,
            VTK_UNSIGNED_CHAR, colors + 4**ptIds);
          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
            ptype, points, 3**ptIds);,
          PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
          device->SendAttribute(vtkPointData::SCALARS, 4,
            VTK_UNSIGNED_CHAR, colors + 4**ptIds);
          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
            ptype, points, 3**ptIds);,
          ;,;);
        }
      break;
    case VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
          device->SendAttribute(vtkPointData::SCALARS, 3,
            VTK_UNSIGNED_CHAR, colors + 4**ptIds);
          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
            ptype, points, 3**ptIds);,
          PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
          device->SendAttribute(vtkPointData::SCALARS, 3,
            VTK_UNSIGNED_CHAR, colors + 4**ptIds);
          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
            ptype, points, 3**ptIds);,
          ;,;);
        }
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;,;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;,;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;,;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_TCOORDS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3*celloffset); celloffset++;,
        celloffset = cellNum;);
      break;
    case VTK_PDM_TCOORDS:
         if (this->BuildNormals)
           {
           vtkDrawPolysMacro(primitive,
             device->SendAttribute(vtkPointData::TCOORDS, tcomps,
               ttype, tcoords, tcomps**ptIds);
             device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
               ptype, points, 3**ptIds);,
             PolyNormal,;);
           }
         else
           {
           vtkDrawPolysMacro(primitive,
             device->SendAttribute(vtkPointData::TCOORDS, 1,
               ttype, tcoords, *ptIds);
             device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
               ptype, points, 3**ptIds);,
             ;,;);
           }
      break;
    case VTK_PDM_CELL_NORMALS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3*celloffset); celloffset++;,
        celloffset = cellNum;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3*celloffset); celloffset++;,
        celloffset = cellNum;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3*celloffset); celloffset++;,
        celloffset = cellNum;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors); colors += 4;,;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS | VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors); colors += 4;,;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3*celloffset); celloffset++;
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors); colors += 4;,
        celloffset = cellNum;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS | VTK_PDM_CELL_COLORS:
      vtkDrawPolysMacro(primitive,
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3*celloffset); celloffset++;
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors); colors += 4;,
        celloffset = cellNum;);
      break;

    case VTK_PDM_EDGEFLAGS:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);, ;,;);
        }
      break;

    case VTK_PDM_NORMALS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,;,;);
      break;

    case VTK_PDM_COLORS | VTK_PDM_EDGEFLAGS:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::SCALARS, 4,
                                                VTK_UNSIGNED_CHAR,
                                                colors + 4**ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::SCALARS, 4,
                                                VTK_UNSIGNED_CHAR,
                                                colors + 4**ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          ;,;);
        }
      break;
    case VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS | VTK_PDM_EDGEFLAGS:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::SCALARS, 3,
                                                VTK_UNSIGNED_CHAR,
                                                colors + 4**ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::SCALARS, 3,
                                                VTK_UNSIGNED_CHAR,
                                                colors + 4**ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          ;,;);
        }
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3**ptIds);
                        device->SendAttribute(vtkPointData::SCALARS, 4,
                                              VTK_UNSIGNED_CHAR,
                                              colors + 4**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,;,;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3**ptIds);
                        device->SendAttribute(vtkPointData::SCALARS, 3,
                                              VTK_UNSIGNED_CHAR,
                                              colors + 4**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,;,;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_TCOORDS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3**ptIds);
                        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
                                              ttype, tcoords, tcomps**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,;,;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_TCOORDS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
                                              ttype, tcoords, tcomps**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3*celloffset);
                        celloffset++;,
                        celloffset = cellNum;);
      break;
    case VTK_PDM_TCOORDS | VTK_PDM_EDGEFLAGS:
      if (this->BuildNormals)
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::TCOORDS, tcomps,
                                                ttype, tcoords, tcomps**ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          PolyNormal,;);
        }
      else
        {
        vtkDrawPolysMacro(primitive,
                          device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                                eftype, edgeflags, *ptIds);
                          device->SendAttribute(vtkPointData::TCOORDS, 1,
                                                ttype, tcoords, *ptIds);
                          device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                                ptype, points, 3**ptIds);,
                          ;,;);
        }
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3*celloffset);
                        celloffset++;,
                        celloffset = cellNum;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::SCALARS, 4,
                                              VTK_UNSIGNED_CHAR,
                                              colors + 4**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3*celloffset);
                        celloffset++;,
                        celloffset = cellNum;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::SCALARS, 3,
                                              VTK_UNSIGNED_CHAR,
                                              colors + 4**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3*celloffset);
                        celloffset++;,
                        celloffset = cellNum;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_CELL_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::SCALARS, 4,
                                              VTK_UNSIGNED_CHAR, colors);
                        colors += 4;,;);
      break;
    case VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS | VTK_PDM_CELL_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3**ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::SCALARS, 3,
                                              VTK_UNSIGNED_CHAR, colors);
                        colors += 4;,;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS | VTK_PDM_CELL_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3*celloffset);
                        celloffset++;
                        device->SendAttribute(vtkPointData::SCALARS, 4,
                                              VTK_UNSIGNED_CHAR, colors);
                        colors += 4;,
                        celloffset = cellNum;);
      break;
    case VTK_PDM_CELL_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS | VTK_PDM_CELL_COLORS | VTK_PDM_EDGEFLAGS:
      vtkDrawPolysMacro(primitive,
                        device->SendAttribute(vtkPointData::EDGEFLAG, 1,
                                              eftype, edgeflags, *ptIds);
                        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                                              ptype, points, 3**ptIds);,
                        device->SendAttribute(vtkPointData::NORMALS, 3,
                                              ntype, normals, 3*celloffset);
                        celloffset++;
                        device->SendAttribute(vtkPointData::SCALARS, 3,
                                              VTK_UNSIGNED_CHAR, colors);
                        colors += 4;,
                        celloffset = cellNum;);
      break;

    default:
      return 0; // let the delegate painter handle it.
    }

  if (idx & VTK_PDM_EDGEFLAGS)
    {
    // Reset the edge flag to 1 so that if the next thing rendered does not
    // have an edge flag, it will have all edges on.
    unsigned char edgeflag = 1;
    device->SendAttribute(vtkPointData::EDGEFLAG, 1, VTK_UNSIGNED_CHAR,
                          &edgeflag, 0);
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPolygonsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
