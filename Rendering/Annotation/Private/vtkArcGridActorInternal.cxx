// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArcGridActorInternal.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkViewport.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkArcGridActorInternal);

//----------------------------------------------------------------------------
vtkArcGridActorInternal::vtkArcGridActorInternal()
{
  this->GridMapper->SetInputData(this->PolyData);
  this->SetMapper(this->GridMapper);
}

//----------------------------------------------------------------------------
vtkArcGridActorInternal::~vtkArcGridActorInternal() = default;

//----------------------------------------------------------------------------
int vtkArcGridActorInternal::RenderOverlay(vtkViewport* viewport)
{
  this->PolyData->Initialize();

  if (!this->HasData())
  {
    return 0;
  }

  this->BuildGrid(viewport);
  return this->Superclass::RenderOverlay(viewport);
}

//----------------------------------------------------------------------------
bool vtkArcGridActorInternal::HasData()
{
  if (!this->TicksStart)
  {
    vtkErrorMacro(<< "Unspecified tick positions");
    return false;
  }

  int nbArcs = this->TicksStart->GetNumberOfPoints();
  if (nbArcs < 1 || this->Resolution < 2)
  {
    vtkWarningMacro(<< "Nothing arcs to draw");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkArcGridActorInternal::BuildGrid(vtkViewport* viewport)
{
  vtkNew<vtkPoints> polyDataPoints;
  polyDataPoints->SetDataType(VTK_DOUBLE);
  vtkNew<vtkCellArray> polyDataLines;

  this->PolyData->SetPoints(polyDataPoints);
  this->PolyData->SetLines(polyDataLines);

  int* viewportSize = viewport->GetSize();
  double centerViewportCoordinates[3] = { this->Center[0] * viewportSize[0],
    this->Center[1] * viewportSize[1], 0 };

  int nbArcs = this->TicksStart->GetNumberOfPoints();
  double startAxes[3];
  this->TicksStart->GetPoint(nbArcs - 1, startAxes);
  vtkMath::Subtract(startAxes, centerViewportCoordinates, startAxes);

  double origin[3] = { 1, 0, 0 };
  double zAxis[3] = { 0, 0, 1 };
  const double startAngle = vtkMath::SignedAngleBetweenVectors(origin, startAxes, zAxis);
  const double maxRadius = vtkMath::Norm(startAxes);

  const double totalAngle = vtkMath::RadiansFromDegrees(this->Angle);

  std::vector<vtkIdType> pids(this->Resolution);
  for (int arc = 0; arc < nbArcs; arc++)
  {
    pids[0] = polyDataPoints->InsertNextPoint(this->TicksStart->GetPoint(arc));
    double newPoint[3] = { 0., 0., 0. };
    double arcRadius = maxRadius * (arc + 1) / (nbArcs);
    for (int intermediate = 0; intermediate < this->Resolution; intermediate++)
    {
      const double newAngle =
        startAngle + totalAngle * intermediate / static_cast<double>(this->Resolution - 1);
      newPoint[0] = std::cos(newAngle) * arcRadius + centerViewportCoordinates[0];
      newPoint[1] = std::sin(newAngle) * arcRadius + centerViewportCoordinates[1];
      pids[intermediate] = polyDataPoints->InsertNextPoint(newPoint);
    }

    polyDataLines->InsertNextCell(this->Resolution, pids.data());
  }
}

//----------------------------------------------------------------------------
void vtkArcGridActorInternal::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Arcs start: \n" << this->TicksStart << "\n";
  this->TicksStart->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Center: " << this->Center[0] << " " << this->Center[1] << "\n";
  os << indent << "Resolution: " << this->Resolution << "\n";
}
VTK_ABI_NAMESPACE_END
