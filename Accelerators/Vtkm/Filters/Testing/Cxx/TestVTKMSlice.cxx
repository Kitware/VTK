// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataSetTriangleFilter.h"
#include "vtkIdentityTransform.h"
#include "vtkImageDataToPointSet.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointDataToCellData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygonBuilder.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkTransformFilter.h"
#include "vtkmSlice.h"

namespace
{

bool TestStructured(int type)
{
  auto imageSource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2, 2, -2, 2, -2, 2);

  auto toStructuredGrid = vtkSmartPointer<vtkImageDataToPointSet>::New();
  auto changePointsPrecision = vtkSmartPointer<vtkTransformFilter>::New();

  vtkAlgorithm* filter = imageSource;
  if (type == 1) // StructuredGrid
  {
    toStructuredGrid->SetInputConnection(imageSource->GetOutputPort());

    // vtkImageDataToPointSet always generates double precision output points
    // but vtkmSlice currently only supports single precision.
    // We can use vtkTransformFilter's `SetOutputPointsPrecision` feature to
    // change the precision.
    vtkNew<vtkIdentityTransform> identity;
    changePointsPrecision->SetTransform(identity);
    changePointsPrecision->SetInputConnection(toStructuredGrid->GetOutputPort());
    changePointsPrecision->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

    filter = changePointsPrecision;
  }

  auto cutter = vtkSmartPointer<vtkmSlice>::New();
  auto p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(-1.5, -1.5, -1.5);
  p3d->SetNormal(1, 1, 1);

  cutter->SetCutFunction(p3d);
  cutter->SetInputConnection(0, filter->GetOutputPort());
  cutter->SetGenerateTriangles(1);
  cutter->Update();
  auto output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 7 || output->CheckAttributes())
  {
    return false;
  }
  return true;
}

bool TestUnstructured()
{
  auto imageSource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2, 2, -2, 2, -2, 2);

  auto dataFilter = vtkSmartPointer<vtkPointDataToCellData>::New();
  dataFilter->SetInputConnection(imageSource->GetOutputPort());

  auto tetraFilter = vtkSmartPointer<vtkDataSetTriangleFilter>::New();
  tetraFilter->SetInputConnection(dataFilter->GetOutputPort());

  auto cutter = vtkSmartPointer<vtkmSlice>::New();
  auto p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(-1.5, -1.5, -1.5);
  p3d->SetNormal(1, 1, 1);

  cutter->SetCutFunction(p3d);
  cutter->SetInputConnection(0, tetraFilter->GetOutputPort());
  cutter->SetGenerateTriangles(1);
  cutter->Update();
  auto output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 10)
  {
    return false;
  }
  return true;
}

} // anonymous namespace

int TestVTKMSlice(int, char*[])
{
  for (int type = 0; type < 2; type++)
  {
    if (!TestStructured(type))
    {
      cerr << "Cutting Structured failed" << endl;
      return EXIT_FAILURE;
    }
  }

  if (!TestUnstructured())
  {
    cerr << "Cutting Unstructured failed" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
