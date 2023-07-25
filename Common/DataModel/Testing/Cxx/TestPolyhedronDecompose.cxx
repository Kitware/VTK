// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include <vtkActor.h>
#include <vtkBitArray.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkContourFilter.h>
#include <vtkDoubleArray.h>
#include <vtkGeometryFilter.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyhedron.h>
#include <vtkPolyhedronUtilities.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkUnstructuredGrid.h>

#include <cmath>
#include <limits>

namespace
{
vtkSmartPointer<vtkPolyhedron> MakePolyhedron1();
vtkSmartPointer<vtkPolyhedron> MakePolyhedron2();

template <typename T>
bool testValue(const std::string& value, T gotVal, T expectedVal);
}

//------------------------------------------------------------------------------
int TestPolyhedronDecompose(int argc, char* argv[])
{
  ////////// Setup data objects //////////

  // Create neighboring polyhedrons
  auto polyhedron1 = ::MakePolyhedron1();
  auto polyhedron2 = ::MakePolyhedron2();

  // Add some cell data
  vtkNew<vtkDoubleArray> cellArray;
  cellArray->SetNumberOfValues(2);
  cellArray->SetName("Cell array");
  cellArray->SetValue(0, 1.5);
  cellArray->SetValue(1, 1.5);

  vtkNew<vtkCellData> cellData;
  cellData->AddArray(cellArray);

  // Add some point data
  constexpr std::array<double, 16> doubleValues = { 2, 5, 2, 2, 2, 3, 2, 3, 2, 5, 2, 2, 2, 3, 2,
    3 };
  const std::array<std::string, 16> stringValues = { "A", "A", "A", "A", "A", "A", "A", "A", "A",
    "A", "A", "A", "A", "A", "A", "A" };
  constexpr std::array<int, 16> bitValues = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  vtkNew<vtkDoubleArray> pointArrayDouble; // Will be dispatched
  pointArrayDouble->SetNumberOfValues(16);
  pointArrayDouble->SetName("Doubles");

  vtkNew<vtkStringArray> pointArrayString; // Will not
  pointArrayString->SetNumberOfValues(16);
  pointArrayString->SetName("Strings");

  vtkNew<vtkBitArray> pointArrayBits; // Will not
  pointArrayBits->SetNumberOfValues(16);
  pointArrayBits->SetName("Bits");

  for (int i = 0; i < 16; i++)
  {
    pointArrayDouble->SetValue(i, doubleValues[i]);
    pointArrayString->SetValue(i, stringValues[i]);
    pointArrayBits->SetValue(i, bitValues[i]);
  }

  vtkNew<vtkPointData> pointData;
  pointData->AddArray(pointArrayDouble);
  pointData->AddArray(pointArrayString);
  pointData->AddArray(pointArrayBits);

  // Decompose polyhedra
  auto decomposedUG1 = vtkPolyhedronUtilities::Decompose(polyhedron1, pointData, 0, cellData);
  auto decomposedUG2 = vtkPolyhedronUtilities::Decompose(polyhedron2, pointData, 1, cellData);

  std::array<vtkSmartPointer<vtkUnstructuredGrid>, 2> decomposedUGs = { decomposedUG1,
    decomposedUG2 };
  for (auto const& decomposedUG : decomposedUGs)
  {
    ////////// Test geometry //////////

    // New number of pts = original pts + face barycenters + cell barycenter
    auto numberOfPts = decomposedUG->GetNumberOfPoints();
    if (!testValue("number of points", numberOfPts, vtkIdType(8 + 6 + 1)))
    {
      return EXIT_FAILURE;
    }

    // New number of cells = original nb of faces * 4
    auto numberOfCells = decomposedUG->GetNumberOfCells();
    if (!testValue("number of cells", numberOfCells, vtkIdType(6 * 4)))
    {
      return EXIT_FAILURE;
    }

    ////////// Test data //////////

    auto pointDataDec = decomposedUG->GetPointData();
    auto cellDataDec = decomposedUG->GetCellData();

    // Test barycenters point data
    // Face barycenter: mean value of face point data
    // Cell barycenter (last one): mean value of face barycenters point data
    vtkDoubleArray* doubleArray =
      vtkDoubleArray::SafeDownCast(pointDataDec->GetAbstractArray("Doubles"));
    if (!doubleArray)
    {
      std::cerr << "Unable to retrieve \"Doubles\" point data." << std::endl;
    }

    if (!testValue("point data (\"Doubles\") nb of tuples", doubleArray->GetNumberOfTuples(),
          decomposedUG->GetNumberOfPoints()))
    {
      return EXIT_FAILURE;
    }

    constexpr double expectedValues[7] = { 2.75, 3, 2, 3.25, 2.25, 2.5, 2.625 };
    for (int i = 0; i < 7; i++)
    {
      if (!testValue("point data (\"Doubles\") at index " + std::to_string(i),
            doubleArray->GetValue(i + 8), expectedValues[i]))
      {
        return EXIT_FAILURE;
      }
    }

    // vtkStringArray is not dispatched, check that the fallback initialized
    // the values as an empty string
    vtkStringArray* stringArray =
      vtkStringArray::SafeDownCast(pointDataDec->GetAbstractArray("Strings"));
    if (!stringArray)
    {
      std::cerr << "Unable to retrieve \"Strings\" point data." << std::endl;
    }

    if (!testValue("point data (\"Strings\") nb of tuples", stringArray->GetNumberOfTuples(),
          decomposedUG->GetNumberOfPoints()))
    {
      return EXIT_FAILURE;
    }

    for (int i = 0; i < 7; i++)
    {
      if (!testValue("point data (\"Strings\") at index " + std::to_string(i),
            stringArray->GetValue(i + 8), vtkStdString()))
      {
        return EXIT_FAILURE;
      }
    }

    // vtkBitArray is not dispatched, check that the fallback initialized
    // the values with 0
    vtkBitArray* bitArray = vtkBitArray::SafeDownCast(pointDataDec->GetAbstractArray("Bits"));
    if (!bitArray)
    {
      std::cerr << "Unable to retrieve \"Bits\" point data." << std::endl;
    }

    if (!testValue("point data (\"Bits\") nb of tuples", bitArray->GetNumberOfTuples(),
          decomposedUG->GetNumberOfPoints()))
    {
      return EXIT_FAILURE;
    }

    for (int i = 0; i < 7; i++)
    {
      if (!testValue(
            "point data (\"Bits\") at index " + std::to_string(i), bitArray->GetValue(i + 8), 0))
      {
        return EXIT_FAILURE;
      }
    }

    // Cell data should be copied to all cells
    vtkDoubleArray* doubleArrayCells =
      vtkDoubleArray::SafeDownCast(cellDataDec->GetAbstractArray("Cell array"));
    if (!doubleArrayCells)
    {
      std::cerr << "Unable to retrieve \"Cell array\" cell data." << std::endl;
    }

    for (vtkIdType cellId = 0; cellId < decomposedUG->GetNumberOfCells(); cellId++)
    {
      if (!testValue("Cell array", doubleArrayCells->GetValue(cellId), 1.5))
      {
        return EXIT_FAILURE;
      }
    }
  }

  ////////// Test contour //////////

  // Extract contours from decomposed ugs
  vtkNew<vtkContourFilter> contourFilter;
  contourFilter->SetInputData(decomposedUG1);
  contourFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Doubles");
  contourFilter->SetNumberOfContours(1);
  contourFilter->SetValue(0, 3.5);
  contourFilter->Update();

  vtkNew<vtkPolyData> contour1;
  contour1->DeepCopy(vtkPolyData::SafeDownCast(contourFilter->GetOutputDataObject(0)));

  contourFilter->SetInputData(decomposedUG2);
  contourFilter->Update();

  vtkNew<vtkPolyData> contour2;
  contour2->DeepCopy(vtkPolyData::SafeDownCast(contourFilter->GetOutputDataObject(0)));

  // Extract surface from decomposed ugs for rendering
  vtkNew<vtkGeometryFilter> filter;
  filter->SetInputDataObject(decomposedUG1);
  filter->Update();

  vtkNew<vtkPolyData> ugSurface1;
  ugSurface1->DeepCopy(vtkPolyData::SafeDownCast(filter->GetOutputDataObject(0)));

  filter->SetInputDataObject(decomposedUG2);
  filter->Update();

  vtkNew<vtkPolyData> ugSurface2;
  ugSurface2->DeepCopy(vtkPolyData::SafeDownCast(filter->GetOutputDataObject(0)));
  ugSurface2->DeepCopy(vtkPolyData::SafeDownCast(filter->GetOutputDataObject(0)));

  // Mappers
  vtkNew<vtkPolyDataMapper> ugMapper1;
  ugMapper1->SetInputData(ugSurface1);
  vtkNew<vtkPolyDataMapper> ugMapper2;
  ugMapper2->SetInputData(ugSurface2);
  vtkNew<vtkPolyDataMapper> contourMapper1;
  contourMapper1->SetInputData(contour1);
  vtkNew<vtkPolyDataMapper> contourMapper2;
  contourMapper2->SetInputData(contour2);

  // Actors
  vtkNew<vtkActor> ugActor1;
  ugActor1->SetMapper(ugMapper1);
  ugActor1->GetProperty()->SetOpacity(0.1);
  vtkNew<vtkActor> ugActor2;
  ugActor2->SetMapper(ugMapper2);
  ugActor2->GetProperty()->SetOpacity(0.1);
  vtkNew<vtkActor> contourActor1;
  contourActor1->SetMapper(contourMapper1);
  vtkNew<vtkActor> contourActor2;
  contourActor2->SetMapper(contourMapper2);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(ugActor1);
  renderer->AddActor(ugActor2);
  renderer->AddActor(contourActor1);
  renderer->AddActor(contourActor2);

  // Camera
  renderer->GetActiveCamera()->Azimuth(135);
  renderer->ResetCamera();

  // Render window
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(300, 300);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Regression image testing
  renderWindow->Render();
  int retVal = vtkRegressionTestImage(renderWindow);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}

namespace
{
//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyhedron> MakePolyhedron1()
{
  vtkSmartPointer<vtkPolyhedron> polyhedron = vtkSmartPointer<vtkPolyhedron>::New();

  // Point Ids
  for (int i = 0; i < 8; ++i)
  {
    polyhedron->GetPointIds()->InsertNextId(i);
  }

  // Points
  polyhedron->GetPoints()->InsertNextPoint(2.5, -7.5, 2.5);
  polyhedron->GetPoints()->InsertNextPoint(5.31, -5.31, 4.68);
  polyhedron->GetPoints()->InsertNextPoint(2.5, -2.5, 2.5);
  polyhedron->GetPoints()->InsertNextPoint(7.5, -2.5, 2.5);
  polyhedron->GetPoints()->InsertNextPoint(2.5, -7.5, 7.5);
  polyhedron->GetPoints()->InsertNextPoint(6.25, -6.25, 6.25);
  polyhedron->GetPoints()->InsertNextPoint(2.5, -2.5, 7.5);
  polyhedron->GetPoints()->InsertNextPoint(6.25, -3.75, 6.25);

  // Faces
  vtkIdType faces[31] = { 6, 4, 0, 1, 3, 2, 4, 0, 4, 5, 1, 4, 0, 2, 6, 4, 4, 1, 5, 7, 3, 4, 3, 7, 6,
    2, 4, 4, 6, 7, 5 };

  polyhedron->SetFaces(faces);
  polyhedron->Initialize();

  return polyhedron;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyhedron> MakePolyhedron2()
{
  vtkSmartPointer<vtkPolyhedron> polyhedron = vtkSmartPointer<vtkPolyhedron>::New();

  // Point Ids
  for (int i = 8; i < 16; ++i)
  {
    polyhedron->GetPointIds()->InsertNextId(i);
  }

  // Points
  polyhedron->GetPoints()->InsertNextPoint(2.5, -7.5, 2.5);
  polyhedron->GetPoints()->InsertNextPoint(5.31, -5.31, 4.68);
  polyhedron->GetPoints()->InsertNextPoint(2.5, -12.5, 2.5);
  polyhedron->GetPoints()->InsertNextPoint(7.5, -12.5, 2.5);
  polyhedron->GetPoints()->InsertNextPoint(2.5, -7.5, 7.5);
  polyhedron->GetPoints()->InsertNextPoint(6.25, -6.25, 6.25);
  polyhedron->GetPoints()->InsertNextPoint(2.5, -12.5, 7.5);
  polyhedron->GetPoints()->InsertNextPoint(6.25, -13.75, 6.25);

  // Faces
  vtkIdType faces[31] = { 6, 4, 10, 11, 9, 8, 4, 9, 13, 12, 8, 4, 12, 14, 10, 8, 4, 11, 15, 13, 9,
    4, 10, 14, 15, 11, 4, 13, 15, 14, 12 };

  polyhedron->SetFaces(faces);
  polyhedron->Initialize();

  return polyhedron;
}

//------------------------------------------------------------------------------
template <typename T>
bool testValue(const std::string& value, T gotVal, T expectedVal)
{
  if (gotVal != expectedVal)
  {
    std::cerr << "Wrong " << value << ". Got " << gotVal << ", expected is : " << expectedVal
              << std::endl;
    return false;
  }
  return true;
}
}
