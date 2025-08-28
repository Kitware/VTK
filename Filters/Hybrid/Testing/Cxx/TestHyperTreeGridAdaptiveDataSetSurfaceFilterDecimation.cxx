// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

namespace
{
const char* HTG_1D_DESCRIPTOR = "R.|R.|..";
const char* HTG_1D_MASK = "11|11|10";

const char* HTG_2D_DESCRIPTOR =
  "RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
  "...R ..R. .... .R.. R...|.... .... .R.. ....|....";
const char* HTG_2D_MASK =
  "111111|0000 1111 1111 1111 1111|1111 0001 0111 0101 1011 1111 0111|1111 0111 "
  "1111 1111 1111 1111|1111 1111 1111 1111|1111";

const char* HTG_3D_DESCRIPTOR =
  "RRR .R. .RR ..R ..R .R.|R.......................... ........................... "
  "........................... .............R............. ....RR.RR........R......... "
  ".....RRRR.....R.RR......... ........................... ........................... "
  "...........................|........................... ........................... "
  "........................... ...RR.RR.......RR.......... ........................... "
  "RR......................... ........................... ........................... "
  "........................... ........................... ........................... "
  "........................... ........................... "
  "............RRR............|........................... ........................... "
  ".......RR.................. ........................... ........................... "
  "........................... ........................... ........................... "
  "........................... ........................... "
  "...........................|........................... ...........................";

const char* HTG_3D_MASK =
  "111 011 011 111 011 110|111111111111111111111111111 111111111111111111111111111 "
  "000000000100110111111111111 111111111111111111111111111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
  "000110011100000100100010100|000001011011111111111111111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111001111111101111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111111111111111111 "
  "111111111111111111111111111|000000000111100100111100100 000000000111001001111001001 "
  "000000111100100111111111111 000000111001001111111111111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
  "111111111111111111111111111 111111111111111111111111111 "
  "110110110100111110111000000|111111111111111111111111111 111111111111111111111111111";

/**
 * Set the camera position and focal point, render, and check the number of cells in `pd` against
 * `expected`.
 * Return true if the number of cells in `pd` is equal to `expected`, and false otherwise.
 */
bool RenderAndCheckVisibleCells(vtkCamera* camera, vtkRenderWindow* renWin, double point[3],
  double position[3], vtkAdaptiveDataSetSurfaceFilter* surface, vtkPolyData* pd, vtkIdType expected)
{
  camera->SetFocalPoint(point);
  camera->SetPosition(position);
  surface->Modified();
  renWin->Render();
  if (pd->GetNumberOfCells() != expected)
  {
    vtkLogF(ERROR,
      "Incorrect number of visible cells. Expected %" VTK_ID_TYPE_PRId " but got %" VTK_ID_TYPE_PRId
      ".",
      expected, pd->GetNumberOfCells());
    return false;
  }
  return true;
}

/**
 * Return a new `vtkHyperTreeGridSource` from the given parameters
 */
vtkSmartPointer<vtkHyperTreeGridSource> CreateHTGSource(int maxDepth, double dimensions[3],
  double scaleZ, int branchFactor, const char* descriptor, const char* mask, bool useMask)
{
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(maxDepth);
  htGrid->SetDimensions(dimensions[0], dimensions[1], dimensions[2]);
  htGrid->SetGridScale(1.5, 1., scaleZ);
  htGrid->SetBranchFactor(branchFactor);
  htGrid->SetDescriptor(descriptor);
  htGrid->SetUseMask(useMask);
  htGrid->SetMask(mask);
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));
  return htGrid;
}

/**
 * Create HTG and rendering pipeline, and setup `vtkAdaptiveDataSetSurfaceFilter` with them.
 * Update the filter and return its output.
 */
vtkPolyData* UpdateSurface(vtkHyperTreeGridSource* htGrid, vtkRenderer* renderer, vtkCamera* camera,
  vtkAdaptiveDataSetSurfaceFilter* surface, vtkRenderWindow* renWin)
{
  surface->SetRenderer(renderer);
  surface->SetInputConnection(htGrid->GetOutputPort());
  surface->SetViewPointDepend(false);
  surface->Update();
  vtkPolyData* pd = surface->GetOutput();
  double* range = pd->GetCellData()->GetArray("Depth")->GetRange();

  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetScalarRange(range);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  camera->SetClippingRange(1., 100.);

  renderer->AddActor(actor);
  renderer->SetBackground(1., 1., 1.);
  renderer->SetActiveCamera(camera);

  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  surface->SetViewPointDepend(true);
  surface->Update();

  return pd;
}

vtkSmartPointer<vtkHyperTreeGridSource> CreateHTGSource1D(bool useMask)
{
  double dimensions[3] = { 3, 1, 1 };
  return CreateHTGSource(3, dimensions, 1.0, 2, HTG_1D_DESCRIPTOR, HTG_1D_MASK, useMask);
}

vtkSmartPointer<vtkHyperTreeGridSource> CreateHTGSource2D(bool useMask)
{
  double dimensions[3] = { 3, 4, 1 };
  return CreateHTGSource(6, dimensions, 10.0, 2, HTG_2D_DESCRIPTOR, HTG_2D_MASK, useMask);
}

vtkSmartPointer<vtkHyperTreeGridSource> CreateHTGSource3D(bool useMask)
{
  double dimensions[3] = { 4, 4, 3 };
  return CreateHTGSource(5, dimensions, 0.7, 3, HTG_3D_DESCRIPTOR, HTG_3D_MASK, useMask);
}

/**
 * Helper struct to store test parameters
 */
struct TestParams
{
  double focalOffSetX;
  double focalOffSetY;
  double positionOffSetX;
  double positionOffSetY;
  vtkIdType expectedCells;
};

/**
 * Perform decimation tests on the hyper tree grid with optional masking.
 * The function iterates over a set of test parameters, adjusting the camera's focal point
 * and position, rendering the scene, and verifying the number of visible cells.
 */
bool TestDecimation(vtkHyperTreeGridSource* htGrid, std::vector<TestParams> testParams,
  double focalOffSetZ, double positionOffSetZ)
{
  vtkNew<vtkAdaptiveDataSetSurfaceFilter> surface;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkCamera> camera;
  vtkNew<vtkRenderWindow> renWin;

  vtkPolyData* pd = UpdateSurface(htGrid, renderer, camera, surface, renWin);

  for (const auto& testParam : testParams)
  {
    double focalPoint[3] = { testParam.focalOffSetX, testParam.focalOffSetY, focalOffSetZ };
    double cameraPos[3] = { testParam.positionOffSetX, testParam.positionOffSetY, positionOffSetZ };
    if (!RenderAndCheckVisibleCells(
          camera, renWin, focalPoint, cameraPos, surface, pd, testParam.expectedCells))
    {
      return false;
    }
  }
  return true;
}

bool TestSimpleDecimation()
{
  // 1D
  double point[2] = { 1.5, 0 };
  double position[2] = { point[0], point[1] };
  std::vector<TestParams> testParams = {
    { point[0], point[1], position[0], position[1], 4 },
    { point[0] - 3, point[1], position[0] - 3, position[1], 3 },
  };

  if (!TestDecimation(CreateHTGSource1D(false), testParams, 0, 10))
  {
    return false;
  }

  // 2D
  point[0] = 0.75;
  point[1] = 1.5;
  position[0] = point[0];
  position[1] = point[1];
  testParams = {
    { point[0], point[1], position[0], position[1], 75 },
    { point[0] - 1, point[1], position[0] - 1, position[1], 70 },
    { point[0] - 2, point[1], position[0] - 2, position[1], 57 },
    { point[0] + 3, point[1], position[0] + 3, position[1], 35 },
    { point[0] + 3, point[1] + 3, position[0] + 3, position[1] + 3, 7 },
    { point[0] + 3, point[1] - 2, position[0] + 3, position[1] - 2, 34 },
    { point[0] + 7, point[1] - 2, position[0] + 7, position[1] - 2, 0 },
  };

  if (!TestDecimation(CreateHTGSource2D(false), testParams, 0, 10))
  {
    return false;
  }

  // 3D
  point[0] = 2.25;
  point[1] = 1.5;
  position[0] = -3.6;
  position[1] = 6.3;
  testParams = {
    { point[0], point[1], position[0], position[1], 466 },
    { point[0] - 1.5, point[1], position[0] - 1.5, position[1], 446 },
    { point[0] - 3, point[1], position[0] - 3, position[1], 267 },
    { point[0] + 3, point[1], position[0] + 3, position[1], 389 },
    { point[0] + 3, point[1] + 1, position[0] + 3, position[1] + 1, 382 },
    { point[0] + 3, point[1] - 2, position[0] + 3, position[1] - 2, 227 },
  };

  return TestDecimation(CreateHTGSource3D(false), testParams, 0.7, -6.72);
}

bool TestMaskedDecimation()
{
  // 1D
  double point[2] = { 1.5, 0 };
  double position[2] = { point[0], point[1] };
  std::vector<TestParams> testParams = {
    { point[0], point[1], position[0], position[1], 3 },
    { point[0] + 2, point[1], position[0] + 2, position[1], 2 },
  };

  if (!TestDecimation(CreateHTGSource1D(true), testParams, 0, 10))
  {
    return false;
  }

  // 2D
  point[0] = 0.75;
  point[1] = 1.5;
  position[0] = point[0];
  position[1] = point[1];
  testParams = {
    { point[0], point[1], position[0], position[1], 62 },
    { point[0] + 3, point[1], position[0] + 3, position[1], 31 },
  };
  if (!TestDecimation(CreateHTGSource2D(true), testParams, 0, 10))
  {
    return false;
  }

  // 3D
  point[0] = 2.25;
  point[1] = 1.5;
  position[0] = -3.6;
  position[1] = 6.3;
  testParams = {
    { point[0], point[1], position[0], position[1], 664 },
    { point[0] - 3, point[1], position[0] - 3, position[1], 414 },
  };
  return TestDecimation(CreateHTGSource3D(true), testParams, 0.7, -6.72);
}
}

int TestHyperTreeGridAdaptiveDataSetSurfaceFilterDecimation(
  int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool status = true;
  status &= TestSimpleDecimation();
  status &= TestMaskedDecimation();
  return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
