// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates multiple random circles (in an xy-image) with different
// region ids, and then produces sampled points from the image. Using the
// vtkVoronoiFlower2D class, a Voronoi tessellation is then produced.

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkFeatureEdges.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLabeledImagePointSampler.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataWriter.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMPTools.h"
#include "vtkSurfaceNets3D.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVoronoiFlower2D.h"

#include <iostream>

namespace
{

struct Circle
{
  int RegionId;
  double Center[3];
  double Radius;
  Circle(int rid, double x, double y, double r)
    : RegionId(rid)
    , Center{ x, y, 0.0 }
    , Radius(r)
  {
  }

  // Label pixels within circle
  void LabelPixels(vtkImageData* image)
  {
    int dims[3];
    image->GetDimensions(dims);
    vtkIdType numPts = image->GetNumberOfPoints();
    assert(numPts == (dims[0] * dims[1] * dims[2]));

    vtkDataArray* scalars = image->GetPointData()->GetScalars();
    int* sPtr = vtkIntArray::FastDownCast(scalars)->GetPointer(0);

    double R2 = (this->Radius * this->Radius);
    // Make the circle sampling a little faster
    vtkSMPTools::For(0, numPts,
      [this, &image, &R2, &sPtr](vtkIdType ptId, vtkIdType endPtId)
      {
        double d2, x[3];
        int* s = sPtr + ptId;
        for (; ptId < endPtId; ++ptId, ++s)
        {
          image->GetPoint(ptId, x);
          d2 = vtkMath::Distance2BetweenPoints(x, this->Center);
          if (d2 <= R2)
          {
            *s = this->RegionId;
          }
        }
      }); // end lambda
  }       // Label pixels
};        // Circle

// This does the actual work: updates the vtkPline implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkPickFollowerCallback : public vtkCommand
{
public:
  static vtkPickFollowerCallback* New() { return new vtkPickFollowerCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    //      vtkPropPicker *picker = reinterpret_cast<vtkPropPicker*>(caller);
    vtkCellPicker* picker = reinterpret_cast<vtkCellPicker*>(caller);
    if (picker->GetViewProp() != nullptr)
    {
      std::cout << "Picked: " << picker->GetCellId() << "\n";
    }
  }
  vtkPickFollowerCallback() = default;
};

} // anonymous

int TestLabeledImagePointSampler3(int argc, char* argv[])
{
  // Create an xy image
  int numCircles = 50;
  const int DIM = 1001;
  int dims[3] = { 2 * DIM + 1, DIM, 1 };

  double xRange[2] = { -4, 4 };
  double yRange[2] = { -2, 2 };
  double zRange[2] = { 0, 0 };
  double rRange[2] = { 0.2, 0.75 };

  double spacing[3];
  spacing[0] = (xRange[1] - xRange[0]) / static_cast<double>(dims[0] - 1);
  spacing[1] = (yRange[1] - yRange[0]) / static_cast<double>(dims[1] - 1);
  spacing[2] = 0.0;

  vtkNew<vtkImageData> image;
  image->SetDimensions(dims);
  image->SetOrigin(xRange[0], yRange[0], zRange[0]);
  image->SetSpacing(spacing);
  image->AllocateScalars(VTK_INT, 1);
  vtkIntArray* scalars = vtkIntArray::FastDownCast(image->GetPointData()->GetScalars());
  int* sPtr = scalars->GetPointer(0);
  vtkSMPTools::Fill(sPtr, sPtr + image->GetNumberOfPoints(), 0); // fill with background

  // Generate the circles
  std::vector<Circle> circles;
  for (int rid = 0; rid < numCircles; ++rid)
  {
    double x = vtkMath::Random(xRange[0], xRange[1]);
    double y = vtkMath::Random(yRange[0], yRange[1]);
    double r = vtkMath::Random(rRange[0], rRange[1]);
    circles.emplace_back(rid + 1, x, y, r);
  }

  // Populate the image. This sets region ids within the circles.
  for (auto& sItr : circles)
  {
    sItr.LabelPixels(image);
  }

  // Time processing
  vtkNew<vtkTimerLog> timer;

  // Sampled points
  vtkNew<vtkLabeledImagePointSampler> sampler;
  sampler->SetInputData(image);
  sampler->GenerateLabels(numCircles, 1, numCircles);
  sampler->SetDensityDistributionToExponential();
  //  sampler->SetDensityDistributionToLinear();
  sampler->SetN(2);
  sampler->SetOutputTypeToLabeledPoints();
  sampler->SetOutputTypeToBackgroundPoints();
  sampler->SetOutputTypeToAllPoints();
  sampler->BackgroundPointMappingOn();
  sampler->SetBackgroundPointLabel(-100);
  sampler->GenerateVertsOn();
  sampler->RandomizeOn();
  sampler->JoggleOn();
  sampler->JoggleRadiusIsAbsoluteOn();
  sampler->SetJoggleRadius(0.001);

  timer->StartTimer();
  sampler->Update();
  timer->StopTimer();
  auto time = timer->GetElapsedTime();
  std::cout << "Time to sample data: " << time << "\n";

  // Voronoi tessellation
  vtkNew<vtkVoronoiFlower2D> vor;
  vor->SetInputConnection(sampler->GetOutputPort());
  vor->SetGenerateCellScalarsToRandom();
  vor->SetGenerateCellScalarsToRegionIds();
  vor->SetOutputTypeToVoronoi();

  timer->StartTimer();
  vor->Update();
  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Time to Voronoi data: " << time << "\n";
  std::cout << "\tNumber of threads used: " << vor->GetNumberOfThreads() << "\n";

  // View the resulting Voronoi tessellation
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(vor->GetOutputPort());
  mapper->SetScalarRange(0, numCircles + 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  actor->GetProperty()->SetColor(1, 1, 1);
  actor->GetProperty()->SetPointSize(2);

  // View the resulting point sampling
  vtkNew<vtkPolyDataMapper> pointsMapper;
  pointsMapper->SetInputConnection(sampler->GetOutputPort());
  pointsMapper->SetScalarRange(-100, numCircles + 1);
  pointsMapper->SetScalarRange(-100, 1);
  pointsMapper->ScalarVisibilityOn();

  vtkNew<vtkActor> pointsActor;
  pointsActor->SetMapper(pointsMapper);
  pointsActor->GetProperty()->SetColor(1, 1, 1);
  pointsActor->GetProperty()->SetPointSize(2);

  // Report some stats
  sampler->Update();
  std::cout << "\tNumber of input Voroni points: " << sampler->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output Voronoi points: " << vor->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output Voronoi cells: " << vor->GetOutput()->GetNumberOfCells() << "\n";

  // Bounding box
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(vor->GetOutputPort());

  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);
  ren->AddActor(outlineActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->AddRenderer(ren);

  // Picking callback
  vtkNew<vtkPickFollowerCallback> myCallback;

  vtkNew<vtkCellPicker> picker;
  picker->AddObserver(vtkCommand::EndPickEvent, myCallback);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->SetPicker(picker);

  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
