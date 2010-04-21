/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTStripsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTStripsPainter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"

#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkTStripsPainter);
//-----------------------------------------------------------------------------
vtkTStripsPainter::vtkTStripsPainter()
{
  this->SetSupportedPrimitive(vtkPainter::STRIPS);
}

//-----------------------------------------------------------------------------
vtkTStripsPainter::~vtkTStripsPainter()
{
}

//-----------------------------------------------------------------------------
#define vtkDrawStripLinesMacro(prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; \
  vtkIdType *savedPtIds = ptIds; \
  glInitFuncs \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    device->BeginPrimitive(prim); \
    glCellFuncs \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ptIds += 2; \
      nPts -= 2; \
      } \
    device->EndPrimitive(); \
    ptIds += nPts; /* nPts could be 0 or -1 here */ \
    } \
  ptIds = savedPtIds; \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    device->BeginPrimitive(prim); \
    glCellFuncs \
    ++ptIds; \
    --nPts; \
    while (nPts > 0) \
      { \
      glVertFuncs \
      ptIds += 2; \
      nPts -= 2; \
      } \
    device->EndPrimitive(); \
    ptIds += nPts; /* nPts could be 0 or -1 here */ \
    } \
}

//-----------------------------------------------------------------------------
#define vtkDrawPolysMacro(prim,glVertFuncs,glCellFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  glInitFuncs \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    device->BeginPrimitive(prim);\
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
      this->UpdateProgress(static_cast<double>(cellNum-cellNumStart)/totalCells); \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        break; \
        } \
      } \
      device->EndPrimitive(); \
    } \
  cellNum += count; \
}

//-----------------------------------------------------------------------------
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
  device->SendAttribute(vtkPointData::NORMALS, 3,\
    VTK_DOUBLE, polyNorm); \
} \
vcount++; 

//-----------------------------------------------------------------------------
#define TStripNormalStart \
  vtkTriangle::ComputeNormal(p, 3, ptIds, polyNorm); \
  device->SendAttribute(vtkPointData::NORMALS, 3,\
    VTK_DOUBLE, polyNorm); int vcount = 0;

//-----------------------------------------------------------------------------
int vtkTStripsPainter::RenderPrimitive(unsigned long idx, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren)
{
  vtkPolyData* pd = this->GetInputAsPolyData();
  vtkPoints* p = pd->GetPoints();
  vtkCellArray* ca = pd->GetStrips();
  vtkIdType cellNum = pd->GetNumberOfVerts() + 
    pd->GetNumberOfLines() + pd->GetNumberOfPolys();
  vtkIdType cellNumStart = cellNum;
  vtkIdType totalCells = ca->GetNumberOfCells();

  vtkPainterDeviceAdapter* device = ren->GetRenderWindow()->
    GetPainterDeviceAdapter();
  void *points = p->GetVoidPointer(0);
  void *normals = 0;
  void *tcoords = 0;
  unsigned char *colors = 0;
  double polyNorm[3];
  vtkIdType normIdx[3];

  int rep = VTK_TRIANGLE_STRIP;
  
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
    }
  if (t)
    {
    tcoords = t->GetVoidPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();

  int ptype = p->GetDataType();
  int ntype = (n)? n->GetDataType() : 0;
  int ttype = (t)? t->GetDataType() : 0;
  int tcomps = (t)? t->GetNumberOfComponents() : 0;

  // Ignore edge flags.
  idx &= (~VTK_PDM_EDGEFLAGS);

  // draw all the elements, use fast path if available
  switch (idx)
    {
  case 0:
    if (this->BuildNormals)
      {
      vtkDrawPolysMacro(rep, 
        TStripNormal; 
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        TStripNormalStart,;);
      }
    else
      {
      vtkDrawPolysMacro(rep, 
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        ;,;);
      }
    break;
  case VTK_PDM_NORMALS:
    vtkDrawPolysMacro(rep,
      device->SendAttribute(vtkPointData::NORMALS, 3,
        ntype, normals, 3**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;,;);
    break;
  case VTK_PDM_COLORS:
    if (this->BuildNormals)
      {
      vtkDrawPolysMacro(rep, 
        TStripNormal
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        TStripNormalStart,;);
      }
    else
      {
      vtkDrawPolysMacro(rep, 
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        ;,;);
      }
    break;
  case VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
    if (this->BuildNormals)
      {

      vtkDrawPolysMacro(rep, 
        TStripNormal 
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        TStripNormalStart,;);

      }
    else
      {
      vtkDrawPolysMacro(rep, 
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        ;,;);
      }
    break;
  case VTK_PDM_NORMALS | VTK_PDM_COLORS:
    vtkDrawPolysMacro(rep,
      device->SendAttribute(vtkPointData::NORMALS, 3,
        ntype, normals, 3**ptIds);
      device->SendAttribute(vtkPointData::SCALARS, 4,
        VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;,;);
    break;
  case VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
    vtkDrawPolysMacro(rep, 
      device->SendAttribute(vtkPointData::NORMALS, 3,
        ntype, normals, 3**ptIds);
      device->SendAttribute(vtkPointData::SCALARS, 3,
        VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;,;);
    break;
  case VTK_PDM_NORMALS | VTK_PDM_TCOORDS:
    vtkDrawPolysMacro(rep,
      device->SendAttribute(vtkPointData::NORMALS, 3,
        ntype, normals, 3**ptIds);
      device->SendAttribute(vtkPointData::TCOORDS, tcomps,
        ttype, tcoords, tcomps**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;,;);
    break;
  case VTK_PDM_COLORS | VTK_PDM_TCOORDS:
    if (this->BuildNormals)
      {
      vtkDrawPolysMacro(rep,
        TStripNormal
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        TStripNormalStart,;);
      }
    else
      {
      vtkDrawPolysMacro(rep,
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        ;,;);
      }
    break;
  case VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS | VTK_PDM_TCOORDS:
    if (this->BuildNormals)
      {
      vtkDrawPolysMacro(rep,
        TStripNormal
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        TStripNormalStart,;);
      }
    else
      {
      vtkDrawPolysMacro(rep,
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,
        ;,;);
      }
    break;
  case VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_TCOORDS:
    vtkDrawPolysMacro(rep,
      device->SendAttribute(vtkPointData::NORMALS, 3,
        ntype, normals, 3**ptIds);
      device->SendAttribute(vtkPointData::SCALARS, 4,
        VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
      device->SendAttribute(vtkPointData::TCOORDS, tcomps,
        ttype, tcoords, tcomps**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;,;);
    break;
  case VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS |
    VTK_PDM_TCOORDS:
    vtkDrawPolysMacro(rep,
      device->SendAttribute(vtkPointData::NORMALS, 3,
        ntype, normals, 3**ptIds);
      device->SendAttribute(vtkPointData::SCALARS, 3,
        VTK_UNSIGNED_CHAR, colors + (*ptIds<<2));
      device->SendAttribute(vtkPointData::TCOORDS, tcomps,
        ttype, tcoords, tcomps**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;,;);
    break;
  default:
    return 0; // let delegate painter process this render.
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkTStripsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
