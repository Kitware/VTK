// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConvertToPointCloud.h"

#include "vtkInformation.h"
#include "vtkPointData.h"

#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkConvertToPointCloud);

//------------------------------------------------------------------------------
int vtkConvertToPointCloud::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Recover input and output
  vtkDataSet* dataset = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  // Copy the point and field data
  output->GetPointData()->ShallowCopy(dataset->GetPointData());
  output->GetFieldData()->ShallowCopy(dataset->GetFieldData());

  // Copy the points
  vtkPointSet* pointSet = vtkPointSet::SafeDownCast(dataset);
  if (pointSet)
  {
    // Input is a vtkPointSet, copy the points
    vtkNew<vtkPoints> points;
    points->ShallowCopy(pointSet->GetPoints());
    output->SetPoints(points);
  }
  else
  {
    // Not a vtkPointSet, use a slower approach
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(dataset->GetNumberOfPoints());
    for (vtkIdType i = 0; i < dataset->GetNumberOfPoints(); i++)
    {
      double x[3];
      dataset->GetPoint(i, x);
      points->SetPoint(i, x);
    }
    output->SetPoints(points);
  }

  switch (this->CellGenerationMode)
  {
    case (vtkConvertToPointCloud::POLYVERTEX_CELL):
    {
      // Create a polyvertex cell
      std::vector<vtkIdType> polyVertex(dataset->GetNumberOfPoints());
      std::iota(polyVertex.begin(), polyVertex.end(), 0);
      vtkNew<vtkCellArray> verts;
      verts->InsertNextCell(dataset->GetNumberOfPoints(), polyVertex.data());
      output->SetVerts(verts);
    }
    break;
    case (vtkConvertToPointCloud::VERTEX_CELLS):
    {
      // Create as many vertex cells as they are points
      // Note: A faster implementation could be done using SetVoidArray
      vtkNew<vtkCellArray> verts;
      verts->SetNumberOfCells(dataset->GetNumberOfPoints());
      for (vtkIdType i = 0; i < dataset->GetNumberOfPoints(); i++)
      {
        verts->InsertNextCell(1, &i);
      }
      output->SetVerts(verts);
    }
    break;
    case (vtkConvertToPointCloud::NO_CELLS):
    default:
      break;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertToPointCloud::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkConvertToPointCloud::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Cell Generation Mode: " << this->CellGenerationMode << endl;
}
VTK_ABI_NAMESPACE_END
