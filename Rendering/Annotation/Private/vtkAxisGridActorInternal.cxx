// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisGridActorInternal.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAxisGridActorInternal);

//----------------------------------------------------------------------------
vtkAxisGridActorInternal::vtkAxisGridActorInternal()
{
  this->GridMapper->SetInputData(this->PolyData);
  this->SetMapper(this->GridMapper);
}

//----------------------------------------------------------------------------
vtkAxisGridActorInternal::~vtkAxisGridActorInternal() = default;

//----------------------------------------------------------------------------
void vtkAxisGridActorInternal::SetHorizontalLinesLeftPoints(vtkPoints* points)
{
  this->XTicksStart = points;
}

//----------------------------------------------------------------------------
void vtkAxisGridActorInternal::SetHorizontalLinesRightPoints(vtkPoints* points)
{
  this->XTicksEnd = points;
}

//----------------------------------------------------------------------------
void vtkAxisGridActorInternal::SetVerticalLinesTopPoints(vtkPoints* points)
{
  this->YTicksStart = points;
}

//----------------------------------------------------------------------------
void vtkAxisGridActorInternal::SetVerticalLinesBottomPoints(vtkPoints* points)
{
  this->YTicksEnd = points;
}

//----------------------------------------------------------------------------
int vtkAxisGridActorInternal::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildGrid();
  return this->Superclass::RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------------
void vtkAxisGridActorInternal::BuildGrid()
{
  this->PolyData->Initialize();

  if (!this->XTicksStart || !this->XTicksEnd || !this->YTicksStart || !this->YTicksEnd)
  {
    vtkErrorMacro(<< "Unspecified tick positions");
    return;
  }
  if (this->XTicksStart->GetNumberOfPoints() != this->XTicksEnd->GetNumberOfPoints())
  {
    vtkErrorMacro(<< "Number of ticks for X axis don't match");
    return;
  }
  if (this->YTicksStart->GetNumberOfPoints() != this->YTicksEnd->GetNumberOfPoints())
  {
    vtkErrorMacro(<< "Number of ticks for Y axis don't match");
    return;
  }

  int nbXLines = this->XTicksStart->GetNumberOfPoints();
  int nbYLines = this->YTicksStart->GetNumberOfPoints();
  int totalLines = nbXLines + nbYLines;

  // Init data structures
  this->PolyDataPoints->Allocate(totalLines * 2);
  this->PolyDataPoints->SetDataType(VTK_DOUBLE);
  this->PolyDataLines->Allocate(
    this->PolyDataLines->EstimateSize(static_cast<vtkIdType>(totalLines), 2));

  // Create horizontal lines
  for (int i = 0; i < nbXLines; i++)
  {
    vtkIdType pids[2];
    pids[0] = this->PolyDataPoints->InsertNextPoint(this->XTicksStart->GetPoint(i));
    pids[1] = this->PolyDataPoints->InsertNextPoint(this->XTicksEnd->GetPoint(nbXLines - i - 1));
    this->PolyDataLines->InsertNextCell(2, pids);
  }

  // Create vertical lines
  for (int i = 0; i < nbYLines; i++)
  {
    vtkIdType pids[2];
    pids[0] = this->PolyDataPoints->InsertNextPoint(this->YTicksStart->GetPoint(i));
    pids[1] = this->PolyDataPoints->InsertNextPoint(this->YTicksEnd->GetPoint(nbYLines - i - 1));
    this->PolyDataLines->InsertNextCell(2, pids);
  }

  // Build PolyData
  this->SetMapper(this->GridMapper);
  this->PolyData->SetPoints(this->PolyDataPoints.GetPointer());
  this->PolyData->SetLines(this->PolyDataLines.GetPointer());
  this->PolyDataPoints->Modified();
  this->PolyDataLines->Modified();
  this->PolyData->Modified();
}

//----------------------------------------------------------------------------
void vtkAxisGridActorInternal::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Horizontal lines left: \n" << this->XTicksStart << "\n";
  this->XTicksStart->PrintSelf(os, indent.GetNextIndent());
  os << indent << "\nHorizontal lines right: \n";
  this->XTicksEnd->PrintSelf(os, indent.GetNextIndent());

  os << indent << "\nVertical lines top: \n" << this->YTicksStart << "\n";
  this->YTicksStart->PrintSelf(os, indent.GetNextIndent());
  os << indent << "\nVertical lines end: \n" << this->YTicksEnd << "\n";
  this->YTicksEnd->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
