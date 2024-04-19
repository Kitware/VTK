// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVertexGlyphFilter.h"

#include "vtkCellArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVertexGlyphFilter);

//------------------------------------------------------------------------------

vtkVertexGlyphFilter::vtkVertexGlyphFilter() = default;

vtkVertexGlyphFilter::~vtkVertexGlyphFilter() = default;

void vtkVertexGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------

int vtkVertexGlyphFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------

int vtkVertexGlyphFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output.
  vtkPointSet* psInput = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph* graphInput = vtkGraph::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* points = nullptr;
  if (psInput)
  {
    points = psInput->GetPoints();
  }
  else
  {
    points = graphInput->GetPoints();
  }

  // If no points, then nothing to do.
  if (points == nullptr)
  {
    return 1;
  }

  output->SetPoints(points);
  vtkIdType numPoints = points->GetNumberOfPoints();

  if (psInput)
  {
    output->GetPointData()->PassData(psInput->GetPointData());
  }
  else
  {
    output->GetPointData()->PassData(graphInput->GetVertexData());
  }

  VTK_CREATE(vtkCellArray, cells);
  cells->AllocateEstimate(numPoints, 1);

  for (vtkIdType i = 0; i < numPoints; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    cells->InsertNextCell(1, &i);
  }
  output->SetVerts(cells);

  return 1;
}
VTK_ABI_NAMESPACE_END
