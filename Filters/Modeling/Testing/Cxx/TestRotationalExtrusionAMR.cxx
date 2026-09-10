// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkAOSDataArrayTemplate.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkFloatArray.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkNonOverlappingAMR.h>
#include <vtkOverlappingAMR.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRotationalExtrusionFilter.h>
#include <vtkTesting.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace
{

template <typename U>
void makeCellData(vtkCellData* cellData, const char* name, vtkIdType numCells)
{
  vtkNew<vtkAOSDataArrayTemplate<U>> array;
  array->SetName(name);
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(numCells);

  for (vtkIdType i = 0; i < numCells; ++i)
    array->SetValue(i, static_cast<U>(i));

  cellData->AddArray(array);
}

template <typename U>
void makePointData(vtkPointData* pointData, const char* name, vtkIdType numPts)
{
  vtkNew<vtkAOSDataArrayTemplate<U>> array;
  array->SetName(name);
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(numPts);

  for (vtkIdType i = 0; i < numPts; ++i)
    array->SetValue(i, static_cast<U>(i));

  pointData->AddArray(array);
}

vtkSmartPointer<vtkOverlappingAMR> GenerateOverlappingAMR()
{
  // generate AMR Data
  vtkNew<vtkOverlappingAMR> amr;

  std::vector<unsigned int> BlocksPerLevel{ 2, 1 };
  std::array<double, 3> globalOrigin{ 0.0, 0.0, 0.0 };

  amr->Initialize(BlocksPerLevel);
  amr->SetGridDescription(vtkStructuredData::VTK_STRUCTURED_XY_PLANE);
  amr->SetOrigin(globalOrigin.data());

  // Level 0: block 1 (2x1 cells, 3x2 points), origin (0,0,0), spacing 1.0
  std::array<int, 3> dims0{ 3, 2, 1 };
  std::array<double, 3> origin0{ 0.0, 0.0, 0.0 };
  std::array<double, 3> spacing0{ 1.0, 1.0, 1.0 };

  vtkNew<vtkUniformGrid> grid0;
  grid0->SetOrigin(origin0.data());
  grid0->SetSpacing(spacing0.data());
  grid0->SetDimensions(dims0.data());

  // Add Data

  makeCellData<int>(grid0->GetCellData(), "Int", grid0->GetNumberOfCells());
  makePointData<int>(grid0->GetPointData(), "Int", grid0->GetNumberOfPoints());

  makeCellData<float>(grid0->GetCellData(), "Float", grid0->GetNumberOfCells());
  makePointData<float>(grid0->GetPointData(), "Float", grid0->GetNumberOfPoints());

  makeCellData<double>(grid0->GetCellData(), "Double", grid0->GetNumberOfCells());
  makePointData<double>(grid0->GetPointData(), "Double", grid0->GetNumberOfPoints());

  makeCellData<double>(grid0->GetCellData(), "Double_Not_In_Both_Grids", grid0->GetNumberOfCells());
  makePointData<double>(
    grid0->GetPointData(), "Double_Not_In_Both_Grids", grid0->GetNumberOfPoints());

  vtkAMRBox box0(
    origin0.data(), dims0.data(), spacing0.data(), amr->GetOrigin(), amr->GetGridDescription());
  amr->SetSpacing(0, spacing0.data());
  amr->SetAMRBox(0, 0, box0);
  amr->SetDataSet(0, 0, grid0);

  // Level 0: block 2 (2x1 cells, 3x2 points), origin (0,0,0), spacing 1.0
  std::array<int, 3> dims1{ 2, 2, 1 };
  std::array<double, 3> origin1{ 2.0, 0.0, 0.0 };
  std::array<double, 3> spacing1{ 1.0, 1.0, 1.0 };

  vtkNew<vtkUniformGrid> grid1;
  grid1->SetOrigin(origin1.data());
  grid1->SetSpacing(spacing1.data());
  grid1->SetDimensions(dims1.data());

  // Add Data

  makeCellData<int>(grid1->GetCellData(), "Int", grid1->GetNumberOfCells());
  makePointData<int>(grid1->GetPointData(), "Int", grid1->GetNumberOfPoints());

  makeCellData<float>(grid1->GetCellData(), "Float", grid1->GetNumberOfCells());
  makePointData<float>(grid1->GetPointData(), "Float", grid1->GetNumberOfPoints());

  makeCellData<double>(grid1->GetCellData(), "Double", grid1->GetNumberOfCells());
  makePointData<double>(grid1->GetPointData(), "Double", grid1->GetNumberOfPoints());

  makeCellData<double>(grid1->GetCellData(), "Double_Not_In_Both_Grids", grid1->GetNumberOfCells());
  makePointData<double>(
    grid1->GetPointData(), "Double_Not_In_Both_Grids", grid1->GetNumberOfPoints());

  vtkAMRBox box1(
    origin1.data(), dims1.data(), spacing1.data(), amr->GetOrigin(), amr->GetGridDescription());
  amr->SetAMRBox(0, 1, box1);
  amr->SetDataSet(0, 1, grid1);

  // Level 1: one single block
  //
  // Block 1: 2x2 cells (3x3 points), origin (1,0,0), spacing 0.5
  std::array<int, 3> dims2{ 5, 3, 1 };
  std::array<double, 3> origin2{ 1.0, 0.0, 0.0 };
  std::array<double, 3> spacing2{ 0.5, 0.5, 0.5 };

  vtkNew<vtkUniformGrid> grid2;
  grid2->SetOrigin(origin2.data());
  grid2->SetSpacing(spacing2.data());
  grid2->SetDimensions(dims2.data());

  // Add Data

  makeCellData<int>(grid2->GetCellData(), "Int", grid2->GetNumberOfCells());
  makePointData<int>(grid2->GetPointData(), "Int", grid2->GetNumberOfPoints());

  makeCellData<float>(grid2->GetCellData(), "Float", grid2->GetNumberOfCells());
  makePointData<float>(grid2->GetPointData(), "Float", grid2->GetNumberOfPoints());

  makeCellData<double>(grid2->GetCellData(), "Double", grid2->GetNumberOfCells());
  makePointData<double>(grid2->GetPointData(), "Double", grid2->GetNumberOfPoints());

  vtkAMRBox box2(
    origin2.data(), dims2.data(), spacing2.data(), amr->GetOrigin(), amr->GetGridDescription());
  amr->SetSpacing(1, spacing2.data());
  amr->SetAMRBox(1, 0, box2);
  amr->SetDataSet(1, 0, grid2);

  if (!amr->CheckValidity())
  {
    vtkLog(ERROR, "TestRotationalExttrusionAMR: Constructed data did not pass validity check.");
    return nullptr;
  }
  return amr;
}

vtkSmartPointer<vtkNonOverlappingAMR> GenerateNonOverlappingAMR()
{
  vtkNew<vtkNonOverlappingAMR> amr;

  std::vector<unsigned int> BlocksPerLevel{ 1, 1 };

  amr->Initialize(BlocksPerLevel);
  amr->SetGridDescription(vtkStructuredData::VTK_STRUCTURED_XY_PLANE);

  // Level 0: one block (1x1 cells, 2x2 points), origin (0,0,0), spacing 1.0
  std::array<int, 3> dims0{ 4, 2, 1 };
  std::array<double, 3> origin0{ 0.0, 0.0, 0.0 };
  std::array<double, 3> spacing0{ 1.0, 1.0, 1.0 };

  vtkNew<vtkUniformGrid> grid0;
  grid0->SetOrigin(origin0.data());
  grid0->SetSpacing(spacing0.data());
  grid0->SetDimensions(dims0.data());

  // Add Data

  makeCellData<int>(grid0->GetCellData(), "Int", grid0->GetNumberOfCells());
  makePointData<int>(grid0->GetPointData(), "Int", grid0->GetNumberOfPoints());

  makeCellData<float>(grid0->GetCellData(), "Float", grid0->GetNumberOfCells());
  makePointData<float>(grid0->GetPointData(), "Float", grid0->GetNumberOfPoints());

  makeCellData<double>(grid0->GetCellData(), "Double", grid0->GetNumberOfCells());
  makePointData<double>(grid0->GetPointData(), "Double", grid0->GetNumberOfPoints());

  makeCellData<double>(grid0->GetCellData(), "Double_Not_In_Both_Grids", grid0->GetNumberOfCells());
  makePointData<double>(
    grid0->GetPointData(), "Double_Not_In_Both_Grids", grid0->GetNumberOfPoints());

  amr->SetDataSet(0, 0, grid0);

  // Level 1: one single block
  //
  // Block 1: 2x2 cells (3x3 points), origin (1,0,0), spacing 0.5
  std::array<int, 3> dims1{ 5, 3, 1 };
  std::array<double, 3> origin1{ 1.0, 0.0, 0.0 };
  std::array<double, 3> spacing1{ 0.5, 0.5, 0.5 };

  vtkNew<vtkUniformGrid> grid1;
  grid1->SetOrigin(origin1.data());
  grid1->SetSpacing(spacing1.data());
  grid1->SetDimensions(dims1.data());

  // Add Data

  makeCellData<int>(grid1->GetCellData(), "Int", grid1->GetNumberOfCells());
  makePointData<int>(grid1->GetPointData(), "Int", grid1->GetNumberOfPoints());

  makeCellData<float>(grid1->GetCellData(), "Float", grid1->GetNumberOfCells());
  makePointData<float>(grid1->GetPointData(), "Float", grid1->GetNumberOfPoints());

  makeCellData<double>(grid1->GetCellData(), "Double", grid1->GetNumberOfCells());
  makePointData<double>(grid1->GetPointData(), "Double", grid1->GetNumberOfPoints());

  amr->SetDataSet(1, 0, grid1);

  return amr;
}

template <typename T>
struct epsilon_value;

template <>
struct epsilon_value<float>
{
  static constexpr float value = 1.0e-6f;
};
template <>
struct epsilon_value<double>
{
  static constexpr double value = 1.0e-15;
};

template <typename T>
int Testing(vtkSmartPointer<vtkDataObject> outputDO, vtkIdType resolution, T extrusionAngle,
  vtkIdType inputType)
{

  std::array<double, 3> initCoords{};
  std::array<double, 3> lastCoords{};
  T angle{ 0 };
  T angleIncr{ vtkMath::RadiansFromDegrees(extrusionAngle) / (T)resolution };
  constexpr int baseNumPts{ 5 * 3 + 2 };
  constexpr int baseNumCells{ 1 + 4 + 4 };
  T epsilon{ epsilon_value<T>::value };
  bool capping{ false };

  // Testing OverlappingAMR
  // Testing output type
  if (outputDO->GetDataObjectType() != VTK_UNSTRUCTURED_GRID)
  {
    vtkLog(ERROR, "Output is not an UnstructuredGrid - Input type :" << inputType);
    return EXIT_FAILURE;
  }

  auto output = vtkUnstructuredGrid::SafeDownCast(outputDO);

  // Testing output points
  vtkSmartPointer<vtkPoints> points = output->GetPoints();

  vtkIdType numPts;
  // if capping
  if (extrusionAngle == 360)
  {
    numPts = resolution * (5 * 3 + 2);
    capping = true;
  }
  else
  {
    numPts = (resolution + 1) * (5 * 3 + 2);
  }

  if (points->GetNumberOfPoints() != numPts)
  {
    vtkLog(ERROR, "Unexpected number of points - Input type :" << inputType);
    return EXIT_FAILURE;
  }

  for (auto iRes{ 0 }; iRes < resolution; iRes++)
  {
    for (auto iPoint{ 0 }; iPoint < baseNumPts; iPoint++)
    {
      angle = iRes * angleIncr;
      points->GetPoint(iPoint, initCoords.data());
      points->GetPoint(iPoint + baseNumPts * iRes, lastCoords.data());
      // rotation about y axis
      // using:
      // x2 = z1*sin(theta) + z1*cos(theta)
      // y2 = y1
      // z2 = z1*cos(theta) - x1*sin(theta)

      if (std::abs((initCoords[2] * sin(angle) + initCoords[0] * cos(angle)) - lastCoords[0]) >
          epsilon ||
        std::abs(initCoords[1] - lastCoords[1]) > epsilon ||
        std::abs((initCoords[2] * cos(angle) - initCoords[0] * sin(angle)) - lastCoords[2]) >
          epsilon)
      {
        vtkLog(ERROR, "Points not at the right coordinates - Input type :" << inputType);
        return EXIT_FAILURE;
      }
    }
  }

  // Testing output point and cell data
  vtkSmartPointer<vtkPointData> pd{ output->GetPointData() };
  vtkSmartPointer<vtkCellData> cd{ output->GetCellData() };

  // data not present in all input grids should not be present in final grid
  if (pd->GetArray("Double_Not_In_Both_Grids") || cd->GetArray("Double_Not_In_Both_Grids"))
  {
    vtkLog(ERROR, "Point data not copied correctly - Input type :" << inputType);
    return EXIT_FAILURE;
  }

  for (auto iRes{ 0 }; iRes <= resolution; iRes++)
  {
    if (capping && (iRes == resolution))
    {
      break;
    }

    for (auto iPoint{ 0 }; iPoint < baseNumPts; iPoint++)
    {
      if (pd->GetArray("Int")->GetComponent(iPoint, 0) !=
        pd->GetArray("Int")->GetComponent(iPoint + baseNumPts * iRes, 0))
      {
        vtkLog(ERROR, "Point data not copied correctly - Input type :" << inputType);
        return EXIT_FAILURE;
      }
      if (pd->GetArray("Float")->GetComponent(iPoint, 0) !=
        pd->GetArray("Float")->GetComponent(iPoint + baseNumPts * iRes, 0))
      {
        vtkLog(ERROR, "Point data not copied correctly - Input type :" << inputType);
        return EXIT_FAILURE;
      }
      if (pd->GetArray("Double")->GetComponent(iPoint, 0) !=
        pd->GetArray("Double")->GetComponent(iPoint + baseNumPts * iRes, 0))
      {
        vtkLog(ERROR, "Point data not copied correctly - Input type :" << inputType);
        return EXIT_FAILURE;
      }
    }

    // Check cell data
    for (auto i{ 0 }; i < baseNumCells; i++)
    {
      // There is nCells * (resolution - 1) total cells, but nPts * resolution points, so we break
      // here.
      if (iRes == resolution)
      {
        break;
      }
      // cells are indexed differently (along the extrusion/resolution) from points (along the
      // planes/layers)
      int iCell = i * resolution;
      if (cd->GetArray("Int")->GetComponent(iCell, 0) !=
        cd->GetArray("Int")->GetComponent(iCell + iRes, 0))
      {
        vtkLog(ERROR, "Cell data not copied correctly - Input type :" << inputType);
      }
      if (cd->GetArray("Float")->GetComponent(iCell, 0) !=
        cd->GetArray("Float")->GetComponent(iCell + iRes, 0))
      {
        vtkLog(ERROR, "Cell data not copied correctly - Input type :" << inputType);
      }
      if (cd->GetArray("Double")->GetComponent(iCell, 0) !=
        cd->GetArray("Double")->GetComponent(iCell + iRes, 0))
      {
        vtkLog(ERROR, "Cell data not copied correctly - Input type :" << inputType);
      }
    }
  }
  return EXIT_SUCCESS;
}

}

int TestRotationalExtrusionAMR(int argc, char* argv[])
{

  // Testing OverlappingAMR
  auto overAMR = GenerateOverlappingAMR();

  vtkNew<vtkRotationalExtrusionFilter> filter;
  filter->SetInputDataObject(overAMR);
  filter->SetVolumetric(true);
  filter->SetAngle(180);
  filter->SetResolution(4);
  filter->SetAxisPosition(0, 0, 0);
  filter->SetRotationAxis(0, 1, 0);
  filter->SetOutputPointsPrecision(0); // Testing Single Precision
  filter->Update();
  if (Testing<float>(filter->GetOutputDataObject(0), filter->GetResolution(),
        static_cast<float>(filter->GetAngle()), overAMR->GetDataObjectType()) != EXIT_SUCCESS)
  {
    vtkLog(ERROR, "Testing<float> failed for overlapping AMR.");
    return EXIT_FAILURE;
  }

  filter->SetAngle(360);               // Testing Capping
  filter->SetOutputPointsPrecision(1); // Testing Double Precision
  filter->Update();
  if (Testing<double>(filter->GetOutputDataObject(0), filter->GetResolution(), filter->GetAngle(),
        overAMR->GetDataObjectType()) != EXIT_SUCCESS)
  {
    vtkLog(ERROR, "Testing<double> failed for overlapping AMR.");
    return EXIT_FAILURE;
  }

  // Testing NonOverlappingAMR
  auto nonOverAMR = GenerateNonOverlappingAMR();
  filter->SetInputDataObject(nonOverAMR);
  filter->Update();
  if (Testing<double>(filter->GetOutputDataObject(0), filter->GetResolution(), filter->GetAngle(),
        overAMR->GetDataObjectType()) != EXIT_SUCCESS)
  {
    vtkLog(ERROR, "Testing<double> failed for non-overlapping AMR.");
    return EXIT_FAILURE;
  }

  filter->SetInputDataObject(overAMR);
  filter->SetAngle(180);
  filter->Update();

  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputConnection(filter->GetOutputPort());

  // ----------- Setup render -------------
  // Create polydata mapper for wireframe representation
  vtkNew<vtkPolyDataMapper> mapperW;
  mapperW->SetInputConnection(surfaceFilter->GetOutputPort());

  // Create actor for wireframe representation
  vtkNew<vtkActor> actorW;
  actorW->SetMapper(mapperW);
  actorW->GetProperty()->SetRepresentationToWireframe();
  actorW->GetProperty()->SetColor(0., 0., 1.);
  actorW->GetProperty()->SetLineWidth(2);

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actorW);
  renderer->SetBackground(1., 1., 1.);

  // Create a good view angle
  vtkNew<vtkCamera> camera;
  camera->SetPosition(0., 5., 12.);
  camera->SetFocalPoint(0., 1., 0.);
  renderer->SetActiveCamera(camera);

  // Create a renderWindow
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal1 = vtkRegressionTestImage(renWin);
  if (retVal1 == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  filter->SetInputDataObject(nonOverAMR);
  filter->SetAngle(180);
  filter->Update();

  renWin->Modified();
  renWin->Render();

  int retVal2 = vtkRegressionTestImage(renWin);

  if (retVal2 == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return (retVal1 && retVal2) ? EXIT_SUCCESS : EXIT_FAILURE;
}
