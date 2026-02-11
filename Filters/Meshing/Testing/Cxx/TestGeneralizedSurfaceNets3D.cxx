// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates a set of random spheres in a 3D box, each sphere
// assigned an associated region id. Then random points are produced, and
// depending on what sphere they fall into, they are assigned the
// corresponding sphere's region id. (Points not falling into any sphere are
// assigned an outside region id.) This creates separate ares of points with
// the same region id, and the surface net represents boundaries between the
// regions.

#include "vtkActor.h"
#include "vtkGeneralizedSurfaceNets3D.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkOutlineFilter.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#include <iostream>

namespace
{

struct Sphere
{
  int RegionId;
  double Center[3];
  double Radius;
  Sphere(int rid, double x, double y, double z, double r)
    : RegionId(rid)
    , Center{ x, y, z }
    , Radius(r)
  {
  }
};

void ProduceRandomPoints(int numSpheres, int numPts, vtkPolyData* randomPts)
{
  double xRange[2] = { -2, 2 };
  double yRange[2] = { -4, 4 };
  double zRange[2] = { -6, 6 };
  double rRange[2] = { 0.5, 2.0 };

  // Provision the vtkPolyData
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  pts->SetNumberOfPoints(numPts);

  vtkNew<vtkIntArray> regionIds;
  regionIds->SetNumberOfTuples(numPts);

  randomPts->SetPoints(pts);
  randomPts->GetPointData()->SetScalars(regionIds);

  // Generate the spheres
  std::vector<Sphere> spheres;
  for (int rid = 0; rid < numSpheres; ++rid)
  {
    double x = vtkMath::Random(xRange[0], xRange[1]);
    double y = vtkMath::Random(yRange[0], yRange[1]);
    double z = vtkMath::Random(zRange[0], zRange[1]);
    double r = vtkMath::Random(rRange[0], rRange[1]);
    spheres.emplace_back(rid, x, y, z, r);
  }

  // Generate the random points
  for (int pid = 0; pid < numPts; ++pid)
  {
    double x[3];
    x[0] = vtkMath::Random(xRange[0], xRange[1]);
    x[1] = vtkMath::Random(yRange[0], yRange[1]);
    x[2] = vtkMath::Random(zRange[0], zRange[1]);

    // Locate the sphere that the pointis contained by
    int rid = (100); // arbitrary background value
    for (auto& sItr : spheres)
    {
      if (sqrt(vtkMath::Distance2BetweenPoints(x, sItr.Center)) < sItr.Radius)
      {
        rid = sItr.RegionId;
        break;
      }
    }

    // Populate points and region ids
    pts->SetPoint(pid, x);
    regionIds->SetValue(pid, rid);

  } // for all points
}

} // anonymous

int TestGeneralizedSurfaceNets3D(int argc, char* argv[])
{
  int numSpheres = 5;
  int numPts = 1000000;

  vtkNew<vtkPolyData> randomPoints;

  ProduceRandomPoints(numSpheres, numPts, randomPoints);

  // Surface net
  vtkNew<vtkGeneralizedSurfaceNets3D> surfaceNets;
  surfaceNets->SetInputData(randomPoints);
  for (int i = 0; i < numSpheres; ++i)
  {
    surfaceNets->SetLabel(i, i);
  }
  surfaceNets->BoundaryCappingOn();
  surfaceNets->SmoothingOn();
  surfaceNets->SetNumberOfIterations(50);
  surfaceNets->SetConstraintDistance(1);
  surfaceNets->GenerateSmoothingStencilsOff();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaceNets->GetOutputPort());
  mapper->SetScalarModeToUseCellData();
  mapper->SelectColorArray("Surface Net Scalars");
  mapper->SetScalarRange(0, numSpheres - 1);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Bounding box
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(surfaceNets->GetOutputPort());

  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);
  ren->AddActor(outlineActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(399, 401);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
