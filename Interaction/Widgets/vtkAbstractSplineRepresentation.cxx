// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractSplineRepresentation.h"

#include "vtkDoubleArray.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractSplineRepresentation::vtkAbstractSplineRepresentation()
{
  // Initialize pipeline configuration
  this->ParametricFunctionSource->SetScalarModeToNone();
  this->ParametricFunctionSource->GenerateTextureCoordinatesOff();
  this->ParametricFunctionSource->SetUResolution(this->Resolution);

  this->LineMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LineActor->SetMapper(this->LineMapper);
}

//------------------------------------------------------------------------------
vtkAbstractSplineRepresentation::~vtkAbstractSplineRepresentation()
{
  this->SetParametricSplineInternal(nullptr);
}

//------------------------------------------------------------------------------
void vtkAbstractSplineRepresentation::CleanRepresentation()
{
  this->LineMapper->SetInputConnection(nullptr);
  this->SetParametricSplineInternal(nullptr);
}

//------------------------------------------------------------------------------
void vtkAbstractSplineRepresentation::SetParametricSplineInternal(vtkParametricSpline* spline)
{
  if (this->ParametricSpline != spline)
  {
    // to avoid destructor recursion
    vtkParametricSpline* temp = this->ParametricSpline;
    this->ParametricSpline = spline;
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    if (this->ParametricSpline != nullptr)
    {
      this->ParametricSpline->Register(this);
      this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkAbstractSplineRepresentation::SetParametricSpline(vtkParametricSpline* spline)
{
  this->SetParametricSplineInternal(spline);
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkAbstractSplineRepresentation::GetHandlePositions()
{
  return vtkArrayDownCast<vtkDoubleArray>(this->ParametricSpline->GetPoints()->GetData());
}

//------------------------------------------------------------------------------
void vtkAbstractSplineRepresentation::SetResolution(int resolution)
{
  if (this->Resolution == resolution || resolution < (this->NumberOfHandles - 1))
  {
    return;
  }

  this->Resolution = resolution;
  this->ParametricFunctionSource->SetUResolution(this->Resolution);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkAbstractSplineRepresentation::GetPolyData(vtkPolyData* pd)
{
  if (!pd)
  {
    vtkErrorMacro(<< "ERROR: Invalid or nullptr polydata\n");
    return;
  }
  this->ParametricFunctionSource->Update();
  pd->ShallowCopy(this->ParametricFunctionSource->GetOutput());
}

//------------------------------------------------------------------------------
double vtkAbstractSplineRepresentation::GetSummedLength()
{
  vtkPoints* points = this->ParametricFunctionSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if (npts < 2)
  {
    return 0.0;
  }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);

  // check in which case we are: even or odd number of points
  // (else the last while loop iteration would go too far for an even number)
  int imax = (npts % 2 == 0) ? npts - 2 : npts - 1;

  while (i < imax)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  if (npts % 2 == 0)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  return sum;
}

//------------------------------------------------------------------------------
void vtkAbstractSplineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParametricSpline: (" << this->ParametricSpline << "\n";
  if (this->ParametricSpline)
  {
    this->ParametricSpline->PrintSelf(os, indent.GetNextIndent());
    os << indent << ")\n";
  }
  else
  {
    os << "none)\n";
  }

  os << indent << "Resolution: " << this->Resolution << "\n";
}
VTK_ABI_NAMESPACE_END
