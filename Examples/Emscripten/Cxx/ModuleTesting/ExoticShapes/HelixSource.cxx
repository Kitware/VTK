// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HelixSource.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

const double PI_TWICE = 2.0 * vtkMath::Pi();

vtkStandardNewMacro(HelixSource);

HelixSource::HelixSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

HelixSource::~HelixSource() = default;

void HelixSource::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "Radius: " << this->Radius << '\n' << "Pitch: " << this->Pitch << '\n';
  this->Superclass::PrintSelf(os, indent);
}

int HelixSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  auto* outInfo = outputVector->GetInformationObject(0);

  // get the output
  auto* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  const auto n = this->ResolutionPerTurn * this->NumberOfTurns;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(n);

  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(n);
  const auto& a = this->Radius;
  const auto& b = this->Pitch / PI_TWICE;

  for (vtkIdType i = 0; i < n; ++i)
  {
    const double t = this->NumberOfTurns * PI_TWICE * static_cast<double>(i) / (n - 1);
    // x = a*cos(t), y = a*sin(t), z = b * t
    points->SetPoint(i, a * std::cos(t), a * std::sin(t), b * t);
    lines->InsertCellPoint(i);
  }

  output->SetPoints(points);
  output->SetLines(lines);
  return 1;
}

VTK_ABI_NAMESPACE_END
