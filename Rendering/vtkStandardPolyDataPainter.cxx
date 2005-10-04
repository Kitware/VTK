/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStandardPolyDataPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkStandardPolyDataPainter.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkTriangle.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkStandardPolyDataPainter, "1.3");
vtkStandardNewMacro(vtkStandardPolyDataPainter);
//-----------------------------------------------------------------------------
static inline int vtkStandardPolyDataPainterGetTotalCells(vtkPolyData* pd,
  unsigned long typeflags)
{
  int total_cells = 0;
  total_cells += (typeflags & vtkPainter::VERTS)? 
    pd->GetNumberOfVerts() : 0;
  total_cells += (typeflags & vtkPainter::LINES)? 
    pd->GetNumberOfLines() : 0;
  total_cells += (typeflags & vtkPainter::POLYS)? 
    pd->GetNumberOfPolys() : 0;
  total_cells += (typeflags & vtkPainter::STRIPS)? 
    pd->GetNumberOfStrips() : 0;
  return total_cells;
}
//-----------------------------------------------------------------------------
vtkStandardPolyDataPainter::vtkStandardPolyDataPainter()
{
}

//-----------------------------------------------------------------------------
vtkStandardPolyDataPainter::~vtkStandardPolyDataPainter()
{
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
  unsigned long typeflags)
{
  if (typeflags == 0)
    {
    // No primitive to render.
    return;
    }

  if (!renderer->GetRenderWindow()->GetPainterDeviceAdapter())
    {
    vtkErrorMacro("Painter Device Adapter missing!");
    return;
    }
  this->TotalCells = vtkStandardPolyDataPainterGetTotalCells(this->PolyData,
    typeflags);
  this->Timer->StartTimer();
  vtkIdType startCell = 0;
  int interpolation = actor->GetProperty()->GetInterpolation();
  if (typeflags & vtkPainter::VERTS)
    {
    this->DrawCells(VTK_POLY_VERTEX, this->PolyData->GetVerts(), startCell,
      renderer, 0, interpolation);
    }

  startCell += this->PolyData->GetNumberOfVerts();
  if (typeflags & vtkPainter::LINES)
    {
    this->DrawCells(VTK_POLY_LINE, this->PolyData->GetLines(), startCell,
      renderer, 0, interpolation);
    }
  
  startCell += this->PolyData->GetNumberOfLines();
  if (typeflags & vtkPainter::POLYS)
    {
    this->DrawCells(VTK_POLYGON, this->PolyData->GetPolys(), startCell,
      renderer, this->BuildNormals, interpolation);
    }
 
  startCell += this->PolyData->GetNumberOfPolys();
  if (typeflags & vtkPainter::STRIPS)
    {
    this->DrawCells(VTK_TRIANGLE_STRIP, this->PolyData->GetStrips(), startCell,
      renderer, this->BuildNormals, interpolation);
    }

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();

  // let the superclass pass on the request to delegate painter.
  // Ofcouse, more than likely, this call will never have a delegate,
  // but anyways.
  this->Superclass::RenderInternal(renderer, actor, typeflags);
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::DrawCells(int mode, vtkCellArray *connectivity,
                                   vtkIdType startCellId, vtkRenderer *renderer,
                                   int buildnormals, int interpolation)
{
  if (!this->PolyData)
    {
    vtkWarningMacro("No polydata to render!");
    return;
    }

  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  vtkCellData* cellData = this->PolyData->GetCellData();
  vtkPointData* pointData = this->PolyData->GetPointData();
  vtkUnsignedCharArray* fieldColors = vtkUnsignedCharArray::SafeDownCast(
    this->PolyData->GetFieldData()->GetArray("Color"));
  
  vtkPoints* p = this->PolyData->GetPoints();
  vtkIdType npts, *pts;
  vtkIdType cellId = startCellId;
  vtkIdType fielddata_cellId = startCellId;
  
  int pointtype = p->GetDataType();
  void* voidpoints = p->GetVoidPointer(0);
  int count = 0;
  double polyNorm[3];
  vtkIdType normIdx[3];
  if (buildnormals)
    {
    // check is normals are present in data. if so, no need to build them.
    // Point normals can be used only when interpolation is not VTK_FLAT.
    // When interpolation == VTK_FLAT and cell normals are absent,
    // cell normals may be built (depending on this->BuildNormals).
    buildnormals = ((pointData->GetNormals() && interpolation != VTK_FLAT) || 
      cellData->GetNormals())? 0 : 1;
    }

  // Note that cell attributes are overridden by point attributes.
  for (connectivity->InitTraversal(); connectivity->GetNextCell(npts, pts); count++)
    {
    int attribii;

    device->BeginPrimitive(mode);

    for (attribii = 0; attribii < vtkCellData::NUM_ATTRIBUTES; attribii++)
      {
      vtkDataArray *a = cellData->GetAttribute(attribii);
      if (a == NULL || attribii == vtkCellData::VECTORS)
        {
        continue;
        }
      int numc = a->GetNumberOfComponents();
      device->SendAttribute(attribii, numc, a->GetDataType(), 
        a->GetVoidPointer(numc*cellId));
      }
    if (buildnormals)
      {
      if (mode == VTK_POLYGON)
        {
        vtkPolygon::ComputeNormal(p, npts, pts, polyNorm);
        }
      else // VTK_TRIANGLE_STRIP
        {
        vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
        }
      device->SendAttribute(vtkDataSetAttributes::NORMALS, 3, 
        VTK_DOUBLE, polyNorm);
      }

    if (fieldColors)
      {
      // fieldColors are same as cell colors except when rendering 
      // VTK_TRIANGLE_STRIP, when they represent triangle colors.
      int numc = fieldColors->GetNumberOfComponents();
      device->SendAttribute(vtkCellData::SCALARS, numc, VTK_UNSIGNED_CHAR,
        fieldColors->GetVoidPointer(numc * fielddata_cellId));
      fielddata_cellId++;
      }

    for (vtkIdType cellpointi = 0; cellpointi < npts; cellpointi++)
      {
      vtkIdType pointId = pts[cellpointi];
      // If using field colors, then we must send tringle colors, 
      // if rendering triangle strips.
      if (fieldColors && mode == VTK_TRIANGLE_STRIP && cellpointi > 2)
        {
        int numc = fieldColors->GetNumberOfComponents();
        device->SendAttribute(vtkCellData::SCALARS, numc, VTK_UNSIGNED_CHAR,
          fieldColors->GetVoidPointer(numc * fielddata_cellId));
        fielddata_cellId++; 
        }
      
      // Send point centered attributes.
      for (attribii = 0; attribii < vtkPointData::NUM_ATTRIBUTES; attribii++)
        {
        vtkDataArray *a = pointData->GetAttribute(attribii);
        if (!a || attribii == vtkPointData::VECTORS || 
          (interpolation == VTK_FLAT && attribii == vtkPointData::NORMALS))
          {
          // Point normals are skipped when interpolation is flat.
          // We may want to add an interpolation painter that does this.
          continue;
          }

        int numc = a->GetNumberOfComponents();
        device->SendAttribute(attribii, numc, a->GetDataType(),
          a->GetVoidPointer(numc*pointId));
        }

      if (buildnormals && mode == VTK_TRIANGLE_STRIP && cellpointi >=2)
        {
        // build the normal for each triangle in a tstrip.
        if (cellpointi % 2) 
          { 
          normIdx[0] = pts[cellpointi-2]; normIdx[1] = pts[cellpointi]; 
          normIdx[2] = pts[cellpointi-1]; 
          vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm); 
          } 
        else 
          { 
          normIdx[0] = pts[cellpointi-2]; normIdx[1] = pts[cellpointi-1]; 
          normIdx[2] = pts[cellpointi]; 
          vtkTriangle::ComputeNormal(p, 3, normIdx, polyNorm); 
          } 

        device->SendAttribute(vtkDataSetAttributes::NORMALS, 3, 
          VTK_DOUBLE, polyNorm);
        }

      // Send the point position as the last attribute.
      // TODO: how to mark that point position is being sent? since,
      // vtkDataSetAttributes doesn't define point positions and hence has
      // no enum for that. For now, vtkPointData::NUM_ATTRIBUTES marks
      // point positions.
      device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3, 
        pointtype, voidpoints, 3*pointId);
      }
    device->EndPrimitive();

    cellId++;

    if (count == 10000) 
      {
      count = 0;
      // report progress
      this->UpdateProgress(static_cast<double>(cellId - startCellId)/this->TotalCells);
      // Abort render.
      if (renderer->GetRenderWindow()->CheckAbortStatus())
        {
        return;
        }
      }
    }
}

 
