/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareSelectionPolyDataPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHardwareSelectionPolyDataPainter.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedIntArray.h"

vtkStandardNewMacro(vtkHardwareSelectionPolyDataPainter);
//-----------------------------------------------------------------------------
static inline int vtkHardwareSelectionPolyDataPainterGetTotalCells(vtkPolyData* pd,
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
//----------------------------------------------------------------------------
vtkHardwareSelectionPolyDataPainter::vtkHardwareSelectionPolyDataPainter()
{
  this->EnableSelection = 1;
  this->PointIdArrayName = NULL;
  this->CellIdArrayName = NULL;
  this->ProcessIdArrayName = NULL;
  this->CompositeIdArrayName = NULL;
}

//----------------------------------------------------------------------------
vtkHardwareSelectionPolyDataPainter::~vtkHardwareSelectionPolyDataPainter()
{
  this->SetPointIdArrayName(NULL);
  this->SetCellIdArrayName(NULL);
  this->SetProcessIdArrayName(NULL);
  this->SetCompositeIdArrayName(NULL);
}

//-----------------------------------------------------------------------------
void vtkHardwareSelectionPolyDataPainter::RenderInternal(
  vtkRenderer* renderer,
  vtkActor* vtkNotUsed(actor),
  unsigned long typeflags,
  bool vtkNotUsed(forceCompileOnly))
{
  if (typeflags == 0)
  {
    // No primitive to render.
    return;
  }

  vtkPainterDeviceAdapter* device =
    renderer->GetRenderWindow()->GetPainterDeviceAdapter();
  if (device == NULL)
  {
    vtkErrorMacro("Painter Device Adapter missing!");
    return;
  }

  vtkPolyData* pd = this->GetInputAsPolyData();
  this->TotalCells = vtkHardwareSelectionPolyDataPainterGetTotalCells(pd, typeflags);

  if (this->TotalCells == 0)
  {
    // skip empty polydatas.
    this->TimeToDraw = 0;
    return;
  }

  vtkHardwareSelector* selector = renderer->GetSelector();
  if (this->EnableSelection)
  {
    selector->BeginRenderProp();
    // We emphasis the vertex size to make sure they will be properly detected.
    if (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS)
    {
      device->MakeVertexEmphasis(true);
    }
  }

  this->Timer->StartTimer();
  vtkIdType startCell = 0;

  if (typeflags & vtkPainter::VERTS)
  {
    this->DrawCells(VTK_POLY_VERTEX, pd->GetVerts(), startCell, renderer);
  }

  startCell += pd->GetNumberOfVerts();
  if (typeflags & vtkPainter::LINES)
  {
    this->DrawCells(VTK_POLY_LINE, pd->GetLines(), startCell, renderer);
  }

  startCell += pd->GetNumberOfLines();
  if (typeflags & vtkPainter::POLYS)
  {
    this->DrawCells(VTK_POLYGON, pd->GetPolys(), startCell, renderer);
  }

  startCell += pd->GetNumberOfPolys();
  if (typeflags & vtkPainter::STRIPS)
  {
    this->DrawCells(VTK_TRIANGLE_STRIP, pd->GetStrips(), startCell, renderer);
  }
  if (this->EnableSelection)
  {
    selector->EndRenderProp();
    // We revert back our Vertex emphasis
    if (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS)
    {
      device->MakeVertexEmphasis(false);
    }
  }


  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
}

//-----------------------------------------------------------------------------
void vtkHardwareSelectionPolyDataPainter::DrawCells(
  int mode, vtkCellArray *connectivity, vtkIdType startCellId,
  vtkRenderer *renderer)
{
  vtkPolyData* pd = this->GetInputAsPolyData();
  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  vtkHardwareSelector* selector = renderer->GetSelector();
  int attributeMode = selector->GetFieldAssociation();
  // While looking at point selection we render only vertex so each pass
  // should fill the same pixels without risking of detecting vertex belonging
  // to other cells or block. BUT we do that after the ACTOR_PASS to make sure
  // we have the proper oclusion as we keep the Z-buffer arround. In that way
  // vertex that are hidden by some surface won't get selected.
  if (attributeMode == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
    selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS &&
    this->EnableSelection)
  {
    mode = VTK_POLY_VERTEX;
  }

  vtkPoints* p = pd->GetPoints();
  vtkIdType npts, *pts;
  vtkIdType cellId = startCellId;
  vtkUnsignedIntArray* procIdsArray = this->ProcessIdArrayName?
    vtkArrayDownCast<vtkUnsignedIntArray>(
      pd->GetPointData()->GetArray(this->ProcessIdArrayName)) : NULL;

  vtkIdTypeArray* pidArray = this->PointIdArrayName? vtkArrayDownCast<vtkIdTypeArray>(
    pd->GetPointData()->GetArray(this->PointIdArrayName)) : NULL;

  vtkIdTypeArray* cidArray = this->CellIdArrayName? vtkArrayDownCast<vtkIdTypeArray>(
    pd->GetCellData()->GetArray(this->CellIdArrayName)) : NULL;

  vtkUnsignedIntArray* compositeIdArray = this->CompositeIdArrayName?
    vtkArrayDownCast<vtkUnsignedIntArray>(
      pd->GetCellData()->GetArray(this->CompositeIdArrayName)) : NULL;

  int pointtype = p->GetDataType();
  void* voidpoints = p->GetVoidPointer(0);
  int count = 0;

  // Note that cell attributes are overridden by point attributes.
  for (connectivity->InitTraversal(); connectivity->GetNextCell(npts, pts); count++)
  {
    device->BeginPrimitive(mode);
    if (this->EnableSelection && compositeIdArray)
    {
      selector->RenderCompositeIndex(compositeIdArray->GetValue(cellId));
    }
    if (attributeMode == vtkDataObject::FIELD_ASSOCIATION_CELLS &&
      this->EnableSelection)
    {
      selector->RenderAttributeId(
        cidArray? cidArray->GetValue(cellId) : cellId);
    }
    for (vtkIdType cellpointi = 0; cellpointi < npts; cellpointi++)
    {
      vtkIdType pointId = pts[cellpointi];
      if (attributeMode == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        this->EnableSelection)
      {
        selector->RenderAttributeId(
          pidArray? pidArray->GetValue(pointId) : pointId);
      }
      if (this->EnableSelection && procIdsArray &&
        selector->GetUseProcessIdFromData())
      {
        selector->RenderProcessId(procIdsArray->GetPointer(0)[pointId]);
      }
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

//----------------------------------------------------------------------------
void vtkHardwareSelectionPolyDataPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EnableSelection: " << this->EnableSelection << endl;
  os << indent << "CellIdArrayName: " <<
    (this->CellIdArrayName? this->CellIdArrayName :  "(none)") << endl;
  os << indent << "PointIdArrayName: " <<
    (this->PointIdArrayName? this->PointIdArrayName: "(none)") << endl;
  os << indent << "CompositeIdArrayName: "
     << (this->CompositeIdArrayName? this->CompositeIdArrayName : "(none)")
     << endl;
}
