// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This test covers the rendering of surface and wireframe normals at
// medium, large, and small scales (ordered from left to right).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestPolyDataMapperNormals(int argc, char* argv[])
{
  // Cases to check:
  // 1a) surface normals, scale of 1.0
  // 1b) surface normals, scale of 1e20
  // 1c) surface normals, scale of 1e-20
  // 2a) wireframe normals, scale of 1.0
  // 2b) wireframe normals, scale of 1e20
  // 2c) wireframe normals, scale of 1e-20

  // define a hexahedron, do not define normals for it
  constexpr float coords[] = {
    -0.52, -0.52, -0.5, // 0
    0.52, -0.52, -0.5,  // 1
    0.52, 0.52, -0.5,   // 2
    -0.52, 0.52, -0.5,  // 3
    -0.48, -0.48, 0.5,  // 4
    0.48, -0.48, 0.5,   // 5
    0.48, 0.48, 0.5,    // 6
    -0.48, 0.48, 0.5,   // 7
  };
  constexpr vtkIdType polys[] = {
    0, 1, 5, 4, // face 0
    0, 4, 7, 3, // face 1
    4, 5, 6, 7, // face 2
    3, 7, 6, 2, // face 3
    1, 2, 6, 5, // face 4
    0, 3, 2, 1, // face 5
  };

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->SetSize(600, 400);

  // test very large and very small dimensions
  constexpr float scales[3] = {
    1.0,  // normal-sized
    1e20, // huge dimensions
    1e-9  // tiny dimensions
  };

  for (int i = 0; i < 6; i++)
  {
    // use one of the three scales
    float scale = scales[i % 3];
    // if true, use wireframe
    bool wireframe = (i / 3 == 0);

    // scale the hexahedron
    vtkIdType numPoints = vtkIdType(sizeof(coords) / (3 * sizeof(*coords)));
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(numPoints);
    for (vtkIdType ptId = 0; ptId < numPoints; ptId++)
    {
      float scaledPoint[3];
      vtkMath::Assign(&coords[ptId * 3], scaledPoint);
      vtkMath::MultiplyScalar(scaledPoint, scale);
      points->SetPoint(ptId, scaledPoint);
    }

    vtkIdType numCells = vtkIdType(sizeof(polys) / (4 * sizeof(*polys)));
    vtkNew<vtkCellArray> cells;
    for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
      cells->InsertNextCell(4, &polys[cellId * 4]);
    }

    vtkNew<vtkPolyData> data;
    data->SetPoints(points);
    data->SetPolys(cells);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    if (wireframe)
    {
      actor->GetProperty()->SetRepresentationToWireframe();
    }
    else
    {
      actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    }

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor);

    // set up one of the six viewports
    ren->SetViewport(
      (i % 3) / 3.0, (wireframe ? 0.5 : 0.0), ((i % 3) + 1) / 3.0, (wireframe ? 1.0 : 0.5));

    // camera positioning corresponds to scale
    vtkCamera* camera = ren->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(scale, -0.5 * scale, 3.0 * scale);
    ren->ResetCameraClippingRange();

    renWin->AddRenderer(ren);
  }

  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
