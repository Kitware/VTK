/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPointsPainter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkPointsPainter);
//-----------------------------------------------------------------------------
vtkPointsPainter::vtkPointsPainter()
{
  this->SetSupportedPrimitive(vtkPainter::VERTS);
}

//-----------------------------------------------------------------------------
vtkPointsPainter::~vtkPointsPainter()
{
}

//-----------------------------------------------------------------------------
#define vtkDrawPointsMacro(glVertFuncs, glInitFuncs) \
{ \
  vtkIdType nPts; \
  unsigned short count = 0; \
  glInitFuncs \
  device->BeginPrimitive(VTK_POLY_VERTEX);\
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
      this->UpdateProgress(static_cast<double>(cellNum)/totalCells);    \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        break; \
        } \
      } \
    } \
  cellNum += count; \
  device->EndPrimitive();\
}

//-----------------------------------------------------------------------------
int vtkPointsPainter::RenderPrimitive(unsigned long idx, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* vtkNotUsed(t), vtkRenderer* ren)
{
  vtkPoints* p = this->GetInputAsPolyData()->GetPoints();
  vtkCellArray* ca = this->GetInputAsPolyData()->GetVerts();
  vtkIdType cellNum = 0;
  vtkIdType totalCells = ca->GetNumberOfCells();

  vtkPainterDeviceAdapter* device = ren->GetRenderWindow()->
    GetPainterDeviceAdapter();
  void *points = p->GetVoidPointer(0);
  void *normals = 0;
  unsigned char *colors = 0;
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
  
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();
  int ptype = p->GetDataType();
  int ntype = (n)? n->GetDataType() : 0;
  // draw all the elements, use fast path if available
  
  // since this painter does not deal with field colors specially,
  // we just ignore the flag.
  idx &= (~VTK_PDM_FIELD_COLORS);

  // Also ignore edge flags.
  idx &= (~VTK_PDM_EDGEFLAGS);

  switch (idx)
    {
  case 0://no cell/point attribs are present. 
    vtkDrawPointsMacro(
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3, 
        ptype, points, 3**ptIds);,;);
    break;

  case VTK_PDM_NORMALS:
    vtkDrawPointsMacro(
      device->SendAttribute(vtkPointData::NORMALS, 3, ntype, 
        normals, 3**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3, 
        ptype, points, 3**ptIds);,;);
    break;

  case VTK_PDM_COLORS:
    vtkDrawPointsMacro( 
      device->SendAttribute(vtkPointData::SCALARS, 4, 
        VTK_UNSIGNED_CHAR, colors + 4**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;);
    break;
  case VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
    vtkDrawPointsMacro(
      device->SendAttribute(vtkPointData::SCALARS, 3,
        VTK_UNSIGNED_CHAR, colors + 4**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;);
    break;

  case VTK_PDM_NORMALS | VTK_PDM_COLORS:
    vtkDrawPointsMacro(
      device->SendAttribute(vtkPointData::NORMALS, 3, ntype, 
        normals, 3**ptIds);
      device->SendAttribute(vtkPointData::SCALARS, 4, 
        VTK_UNSIGNED_CHAR, colors + 4**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;);
    break;

  case VTK_PDM_NORMALS | VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
    vtkDrawPointsMacro(
      device->SendAttribute(vtkPointData::NORMALS, 3, ntype, 
        normals, 3**ptIds);
      device->SendAttribute(vtkPointData::SCALARS, 3,
        VTK_UNSIGNED_CHAR, colors + 4**ptIds);
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
        ptype, points, 3**ptIds);,;);
    break;
  default:
    return 0; // let the delegate painter handle it. 
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPointsPainter::PrintSelf(ostream& os ,vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
