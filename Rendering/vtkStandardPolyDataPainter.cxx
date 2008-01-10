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
#include "vtkConfigure.h"
#include "vtkDataArray.h"
#include "vtkGenericVertexAttributeMapping.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShaderDeviceAdapter.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkTriangle.h"

#include <vtkstd/vector>

class vtkStandardPolyDataPainter::vtkInternal
{
public:
  struct vtkInfo
    {
    unsigned int MappingsIndex;
    vtkDataArray* Array;
    };
  typedef vtkstd::vector<vtkInfo> InfoVector;
  InfoVector CellAttributesCache;
  InfoVector PointAttributesCache;
  vtkSmartPointer<vtkGenericVertexAttributeMapping> Mappings;
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkStandardPolyDataPainter, "1.8");
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
  this->Internal = new vtkInternal;
}

//-----------------------------------------------------------------------------
vtkStandardPolyDataPainter::~vtkStandardPolyDataPainter()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::ProcessInformation(vtkInformation* info)
{
  this->Internal->Mappings = 0;

  if( info->Has(DATA_ARRAY_TO_VERTEX_ATTRIBUTE()) )
    {
    vtkGenericVertexAttributeMapping* collection = 
      vtkGenericVertexAttributeMapping::SafeDownCast(
      info->Get(DATA_ARRAY_TO_VERTEX_ATTRIBUTE()));
    this->Internal->Mappings = collection;
    }
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::UpdateGenericAttributesCache()
{
  if (this->Internal->Mappings)
    {
    vtkPolyData* pd = this->GetInputAsPolyData();
    unsigned int max = this->Internal->Mappings->GetNumberOfMappings();
    for (unsigned int cc=0; cc < max; cc++)
      {
      int field = this->Internal->Mappings->GetFieldAssociation(cc);
      const char *dataArrayName = this->Internal->Mappings->GetArrayName(cc);
      const char *vertexAttributeName = 
        this->Internal->Mappings->GetAttributeName(cc);

      if (dataArrayName == NULL)
        {
        continue;
        }

      if (vertexAttributeName == NULL)
        {
        continue;
        }

      vtkDataArray *inArray = NULL;
      vtkInternal::InfoVector* dest=0;
  
      if (field == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
        inArray = pd->GetPointData()->GetArray(dataArrayName);
        dest = &this->Internal->PointAttributesCache;
        }
      else if (field == vtkDataObject::FIELD_ASSOCIATION_CELLS)
        {
        inArray = pd->GetCellData()->GetArray(dataArrayName);
        dest = &this->Internal->CellAttributesCache;
        }

      if (inArray && dest)
        {
        vtkInternal::vtkInfo info;
        info.MappingsIndex = cc;
        info.Array = inArray;
        dest->push_back(info);
        }
      }
    }
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
  vtkPolyData* pd = this->GetInputAsPolyData();
  this->TotalCells = vtkStandardPolyDataPainterGetTotalCells(pd, typeflags);
  this->Timer->StartTimer();
  vtkProperty* property = actor->GetProperty();
  vtkIdType startCell = 0;
  int interpolation = property->GetInterpolation();
  vtkShaderDeviceAdapter* shaderDevice=0;

  this->Internal->PointAttributesCache.clear();
  this->Internal->CellAttributesCache.clear();
  if (property->GetShading() && property->GetShaderProgram())
    {
    // Preprocess the generic vertex attributes that we need to pass to the
    // shader.
    this->UpdateGenericAttributesCache();
    shaderDevice = property->GetShaderProgram()->GetShaderDeviceAdapter();
    }
  if (shaderDevice)
    {
    shaderDevice->PrepareForRender();
    }

  if (typeflags & vtkPainter::VERTS)
    {
    this->DrawCells(VTK_POLY_VERTEX, pd->GetVerts(), startCell,
      shaderDevice, renderer, 0, interpolation);
    }

  startCell += pd->GetNumberOfVerts();
  if (typeflags & vtkPainter::LINES)
    {
    this->DrawCells(VTK_POLY_LINE, pd->GetLines(), startCell,
      shaderDevice, renderer, 0, interpolation);
    }
  
  startCell += pd->GetNumberOfLines();
  if (typeflags & vtkPainter::POLYS)
    {
#if defined(__APPLE__) && (defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA))
    if (property->GetRepresentation() == VTK_WIREFRAME)
      {
      this->DrawCells(VTK_TETRA, pd->GetPolys(), startCell,
        shaderDevice, renderer, this->BuildNormals, interpolation);
      }
    else
#endif
      {
      this->DrawCells(VTK_POLYGON, pd->GetPolys(), startCell,
        shaderDevice, renderer, this->BuildNormals, interpolation);
      }
    }
 
  startCell += pd->GetNumberOfPolys();
  if (typeflags & vtkPainter::STRIPS)
    {
    this->DrawCells(VTK_TRIANGLE_STRIP, pd->GetStrips(), startCell,
      shaderDevice, renderer, this->BuildNormals, interpolation);
    }

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();

  // let the superclass pass on the request to delegate painter.
  // Ofcouse, more than likely, this call will never have a delegate,
  // but anyways.
  this->Superclass::RenderInternal(renderer, actor, typeflags);


  this->Internal->PointAttributesCache.clear();
  this->Internal->CellAttributesCache.clear();
}

//-----------------------------------------------------------------------------
void vtkStandardPolyDataPainter::DrawCells(int mode, vtkCellArray *connectivity,
                                   vtkIdType startCellId,
                                   vtkShaderDeviceAdapter* shaderDevice,
                                   vtkRenderer *renderer,
                                   int buildnormals, int interpolation)
{
  vtkPolyData* pd = this->GetInputAsPolyData();

  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  vtkCellData* cellData = pd->GetCellData();
  vtkPointData* pointData = pd->GetPointData();
  vtkUnsignedCharArray* fieldColors = vtkUnsignedCharArray::SafeDownCast(
    pd->GetFieldData()->GetArray("Color"));
 
  int disable_scalar_color = 0;
  if (this->Information->Has(DISABLE_SCALAR_COLOR()) &&
    this->Information->Get(DISABLE_SCALAR_COLOR())==1)
    {
    disable_scalar_color = 1;
    }
  if (disable_scalar_color)
    {
    fieldColors = 0;
    }

  vtkPoints* p = pd->GetPoints();
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

  // skip scalars if disable_scalar_color is true.
  int start_attribute = (disable_scalar_color? 1 : 0);
  // Note that cell attributes are overridden by point attributes.
  for (connectivity->InitTraversal(); connectivity->GetNextCell(npts, pts); count++)
    {
    int attribii;

    device->BeginPrimitive(mode);

    // SEND CELL ATTRIBUTES
    for (attribii = start_attribute; attribii < vtkCellData::NUM_ATTRIBUTES; attribii++)
      {
      if (!device->IsAttributesSupported(attribii))
        {
        // skip non-renderable attributes.
        continue;
        }
      vtkDataArray *a = cellData->GetAttribute(attribii);
      if (a == NULL)
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

    // Send generic attributes associated with the cell.
    vtkInternal::InfoVector::iterator gaIter = this->Internal->CellAttributesCache.begin();
    for (; shaderDevice && gaIter != this->Internal->CellAttributesCache.end(); ++gaIter)
      {
      vtkDataArray* a = gaIter->Array; 
      unsigned int mappingsIndex = gaIter->MappingsIndex;
      int numc = a->GetNumberOfComponents();
      int siComp = this->Internal->Mappings->GetComponent(mappingsIndex);
      // if siComp==-1, then all components of the array are sent,
      // otherwise only the choosen component is sent.
      shaderDevice->SendAttribute(
        this->Internal->Mappings->GetAttributeName(mappingsIndex),
        (siComp>=0)? 1: numc,
        a->GetDataType(),
        (siComp>=0) ? a->GetVoidPointer(numc*cellId+siComp) :
        a->GetVoidPointer(numc*cellId));
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
      
      // SEND POINT ATTRIBUTES.
      // Send point centered attributes.
      for (attribii = start_attribute; attribii < vtkPointData::NUM_ATTRIBUTES; attribii++)
        {
        if (!device->IsAttributesSupported(attribii))
          {
          // skip non-renderable attributes.
          continue;
          }
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

      // Send generic attributes associated with the point.
      gaIter = this->Internal->PointAttributesCache.begin();
      for (; shaderDevice && gaIter != 
        this->Internal->PointAttributesCache.end(); ++gaIter)
        {
        vtkDataArray* a = gaIter->Array; 
        unsigned int mappingsIndex = gaIter->MappingsIndex;
        int numc = a->GetNumberOfComponents();
        int siComp = this->Internal->Mappings->GetComponent(mappingsIndex);
        // if siComp==-1, then all components of the array are sent,
        // otherwise only the choosen component is sent.
        shaderDevice->SendAttribute(
          this->Internal->Mappings->GetAttributeName(mappingsIndex),
          (siComp>=0)? 1: numc,
          a->GetDataType(),
          (siComp>=0) ? a->GetVoidPointer(numc*pointId+siComp) :
          a->GetVoidPointer(numc*pointId));
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

 
