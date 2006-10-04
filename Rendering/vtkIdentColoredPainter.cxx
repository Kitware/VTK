/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentColoredPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIdentColoredPainter.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConfigure.h"
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
#include "vtkIdTypeArray.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkIdentColoredPainter, "1.9");
vtkStandardNewMacro(vtkIdentColoredPainter);

//-----------------------------------------------------------------------------
static inline int vtkIdentColoredPainterGetTotalCells(vtkPolyData* pd,
  unsigned long typeflags)
{
  int total_cells = 0;
  total_cells += (typeflags & vtkPainter::POLYS)? 
    pd->GetNumberOfPolys() : 0;
  total_cells += (typeflags & vtkPainter::STRIPS)? 
    pd->GetNumberOfStrips() : 0;
  return total_cells;
}

//-----------------------------------------------------------------------------
vtkIdentColoredPainter::vtkIdentColoredPainter()
{
  this->ColorMode = COLORBYIDENT;
  this->ResetCurrentId();

  this->ActorIds = NULL;
  this->PropAddrs = NULL;
}

//-----------------------------------------------------------------------------
vtkIdentColoredPainter::~vtkIdentColoredPainter()
{
  if (this->ActorIds != NULL)
    {
    this->ActorIds->Delete();
    this->ActorIds = NULL;
    delete[] this->PropAddrs;
    this->PropAddrs = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::SetToColorByConstant(unsigned int constant)
{
  this->ColorMode = COLORBYCONST;
  this->ResetCurrentId();
  this->CurrentIdPlane0 = constant;
}


//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::SetActorLookupTable(vtkProp **props, vtkIdTypeArray *ids)
{
  //free whatever we were given before this
  if (this->ActorIds != NULL)
    {
    this->ActorIds->Delete();
    this->ActorIds = NULL;
    delete[] this->PropAddrs;
    this->PropAddrs = NULL;
    }
  
  //sanity checking
  if (props == NULL || 
      ids == NULL || 
      (ids->GetNumberOfComponents() != 1) ||
      (ids->GetNumberOfTuples() == 0))
    {
    vtkWarningMacro("Invalid actor-id lookup table supplied.");
    return;
    }

  //copy over the new lookup table
  this->ActorIds = ids;
  this->ActorIds->Register(this);
  this->PropAddrs = new vtkProp*[ids->GetNumberOfTuples()];
  for (int i = 0; i < ids->GetNumberOfTuples(); i++)
    {
    this->PropAddrs[i] = props[i];
    }
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::SetToColorByActorId(vtkProp *actorAddr)
{
  this->ColorMode = COLORBYCONST;
  this->ResetCurrentId();

  vtkIdType maxId = 0;
  int numIds = 0;
  if (this->ActorIds != NULL)
    {
    numIds = this->ActorIds->GetNumberOfTuples();
    for (int i = 0; i< numIds; i++)
      {
      vtkIdType nextId = this->ActorIds->GetValue(i);
      if (actorAddr == this->PropAddrs[i])
        {
        this->CurrentIdPlane0 = nextId + 1;        
        return;
        }
      if (nextId > maxId)
        {
        maxId = nextId;
        }
      }
    }

  //we didn't find the actor in the table, make up an ID and add it
  //cerr << "ID not found for actor " << actorAddr 
  //     << " using " << maxId+1 << endl;
  vtkIdTypeArray *arr = vtkIdTypeArray::New();
  arr->SetNumberOfComponents(1);
  arr->SetNumberOfTuples(numIds+1);
  vtkProp **SaveProps = new vtkProp*[numIds+1];
  if (this->ActorIds != NULL)
    {
    for (int i = 0; i< numIds; i++)
      {
      arr->SetValue(i, this->ActorIds->GetValue(i));
      SaveProps[i] = this->PropAddrs[i];
      }
    }
  arr->SetValue(numIds, maxId+1);
  SaveProps[numIds] = actorAddr;
  this->SetActorLookupTable(SaveProps, arr);
  arr->Delete();

  this->CurrentIdPlane0 = maxId+1;
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::SetToColorByIncreasingIdent(unsigned int plane)
{
  this->ColorMode = COLORBYIDENT;
  this->Plane = (plane < 3)?plane:2;
  this->ResetCurrentId();
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::ResetCurrentId() 
{
  //do not use 0, it is reserved for miss
  this->CurrentIdPlane0 = 1;
  this->CurrentIdPlane1 = 1;
  this->CurrentIdPlane2 = 1;
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::IncrementCurrentId()
{
  if (this->ColorMode == COLORBYCONST)
    {
    return;
    }

  //the limits are set up assuming 24 bits total for each for RGB pixel
  //do not use A because the parallel compositing code does not support Alpha
  this->CurrentIdPlane0++;
  if (this->CurrentIdPlane0 >= 0x01000000)
    {
    this->CurrentIdPlane0 = 0x00000001;
    this->CurrentIdPlane1++;
    if (this->CurrentIdPlane1 >= 0x01000000)
      {
      this->CurrentIdPlane1 = 0x00000001;
      this->CurrentIdPlane2++;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::GetCurrentColor(unsigned char *RGB)
{
  unsigned int val = this->CurrentIdPlane0;
  if (this->ColorMode == COLORBYIDENT)
    {
    if (this->Plane == 1)
      {
      val = this->CurrentIdPlane1;
      }
    else if (this->Plane == 2)
      {
      val = this->CurrentIdPlane2;
      }
    }

  //cerr << "Curr Color is " 
  //     << this->ColorMode << " "
  //     << this->CurrentIdPlane2 << ":"
  //     << this->CurrentIdPlane1 << ":"
  //     << this->CurrentIdPlane0 << endl;

  RGB[0] = (val & 0x00FF0000)>>16;
  RGB[1] = (val & 0x0000FF00)>>8;
  RGB[2] = (val & 0x000000FF);
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::RenderInternal(vtkRenderer* renderer, 
                                            vtkActor* actor, 
                                            unsigned long typeflags)
{
  if (typeflags == 0)
    {
    // No primitive to render.
    return;
    }

  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  if (!device)
    {
    vtkErrorMacro("Painter Device Adapter missing!");
    return;
    }

  this->TotalCells = 
    vtkIdentColoredPainterGetTotalCells(this->PolyData, typeflags);

  this->Timer->StartTimer();

  //turn off antialising and lighting so that the colors we draw will be the
  //colors we read back
  int origMultisample = device->GetMultisampling();
  int origLighting = device->GetLighting();

  device->SetMultisampling(0);
  device->SetLighting(0);

  vtkIdType startCell = 0;
  startCell += this->PolyData->GetNumberOfVerts();
  startCell += this->PolyData->GetNumberOfLines();

  if (typeflags & vtkPainter::POLYS)
    {
#if defined(__APPLE__) && (defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA))
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      this->DrawCells(VTK_TETRA, this->PolyData->GetPolys(), startCell,
        renderer);
      }
    else
#endif
      {
      this->DrawCells(VTK_POLYGON, this->PolyData->GetPolys(), startCell,
        renderer);
      }
    }
 
  startCell += this->PolyData->GetNumberOfPolys();
  if (typeflags & vtkPainter::STRIPS)
    {
    this->DrawCells(VTK_TRIANGLE_STRIP, this->PolyData->GetStrips(), startCell,
      renderer);
    }

  //reset lighting back to the default
  device->SetLighting(origLighting);
  device->SetMultisampling(origMultisample);

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();

  // let the superclass pass on the request to delegate painter.
  // Ofcouse, more than likely, this call will never have a delegate,
  // but anyways.
  this->Superclass::RenderInternal(renderer, actor, typeflags);
}

//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::DrawCells(int mode, vtkCellArray *connectivity,
                                       vtkIdType startCellId, 
                                       vtkRenderer *renderer)
{
  if (!this->PolyData)
    {
    vtkWarningMacro("No polydata to render!");
    return;
    }

  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  vtkPoints* p = this->PolyData->GetPoints();
  vtkIdType npts, *pts;
  vtkIdType cellId = startCellId;

  int pointtype = p->GetDataType();
  void* voidpoints = p->GetVoidPointer(0);
  int count = 0;

  unsigned char color[3];
  for (connectivity->InitTraversal(); connectivity->GetNextCell(npts, pts); count++)
    {
    device->BeginPrimitive(mode);
    
    // fieldColors are same as cell colors except when rendering 
    // VTK_TRIANGLE_STRIP, when they represent triangle colors.
    this->GetCurrentColor(color);

    device->SendAttribute(vtkCellData::SCALARS, 3, VTK_UNSIGNED_CHAR, color);

    this->IncrementCurrentId();

    for (vtkIdType cellpointi = 0; cellpointi < npts; cellpointi++)
      {
      vtkIdType pointId = pts[cellpointi];
      // If using field colors, then we must send triangle colors, 
      // if rendering triangle strips.
      if (mode == VTK_TRIANGLE_STRIP && cellpointi > 2)
        {
        this->GetCurrentColor(color);

        device->SendAttribute(vtkCellData::SCALARS, 3, VTK_UNSIGNED_CHAR, color);

        this->IncrementCurrentId();
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
 
//-----------------------------------------------------------------------------
void vtkIdentColoredPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
