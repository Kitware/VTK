// Description
// This test creates multiple random circles (in an xy-image) with different
// region ids, and then produces sampled points from the image. Using the
// vtkVoronoiFlower2D class, a Voronoi tessellation is then produced, as
// well as a 2D surface net.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkExtractEdges.h"
#include "vtkFeatureEdges.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLabeledImagePointSampler.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPNGWriter.h"
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
#include "vtkTestUtilities.h"
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

} // anonymous

int TestVoronoi2DSurfaceNets(int argc, char* argv[])
{
  int numCircles = 17;
  const int DIM = 151;
  int dims[3] = { 2 * DIM + 1, DIM, 1 };

  double xRange[2] = { -1, 1 };
  double yRange[2] = { -1, 1 };
  double zRange[2] = { 0, 0 };
  double rRange[2] = { 0.1, 0.35 };

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
  vtkSMPTools::Fill(sPtr, sPtr + image->GetNumberOfPoints(), -1); // fill with background

  // Generate the circles
  std::vector<Circle> circles;
  for (int rid = 0; rid < numCircles; ++rid)
  {
    double x = vtkMath::Random(xRange[0], xRange[1]);
    double y = vtkMath::Random(yRange[0], yRange[1]);
    double r = vtkMath::Random(rRange[0], rRange[1]);
    circles.emplace_back(rid, x, y, r);
  }

  for (auto& sItr : circles)
  {
    sItr.LabelPixels(image);
  }

  // Time processing
  vtkNew<vtkTimerLog> timer;
  double time = 0.0;

  // Sampled points
  vtkNew<vtkLabeledImagePointSampler> sampler;
  sampler->SetInputData(image);
  sampler->GenerateLabels(numCircles, 0, numCircles - 1);
  sampler->SetDensityDistributionToExponential();
  sampler->SetN(2);
  sampler->SetOutputTypeToLabeledPoints();
  sampler->SetOutputTypeToBackgroundPoints();
  sampler->SetOutputTypeToAllPoints();
  sampler->BackgroundPointMappingOn();
  sampler->SetBackgroundPointLabel(-1);
  sampler->GenerateVertsOn();
  sampler->RandomizeOn();
  sampler->SetRandomProbabilityRange(0, 0.8);
  sampler->JoggleOn();
  sampler->JoggleRadiusIsAbsoluteOn();
  sampler->SetJoggleRadius(0.001);
  sampler->ConstrainJoggleOn();
  sampler->SetJoggleConstraint(0.75);

  timer->StartTimer();
  sampler->Update();
  timer->StopTimer();
  std::cout << "Time to sample data: " << time << "\n";

  // View the resulting point sampling
  vtkNew<vtkPolyDataMapper> pointsMapper;
  pointsMapper->SetInputConnection(sampler->GetOutputPort());
  pointsMapper->SetScalarRange(-1, numCircles - 1);
  pointsMapper->ScalarVisibilityOn();

  vtkNew<vtkActor> pointsActor;
  pointsActor->SetMapper(pointsMapper);
  pointsActor->GetProperty()->SetColor(1, 1, 1);
  pointsActor->GetProperty()->SetPointSize(3);

  // Voronoi tessellation
  vtkNew<vtkVoronoiFlower2D> vor;
  vor->SetInputConnection(sampler->GetOutputPort());
  vor->SetGenerateCellScalarsToRegionIds();
  vor->SetOutputTypeToVoronoi();

  timer->StartTimer();
  vor->Update();
  timer->StopTimer();
  time = timer->GetElapsedTime();
  // Report some stats
  std::cout << "Time to Voronoi data: " << time << "\n";
  std::cout << "\tNumber of threads used: " << vor->GetNumberOfThreads() << "\n";
  std::cout << "\tNumber of input Voroni points: " << sampler->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output Voronoi points: " << vor->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output Voronoi cells: " << vor->GetOutput()->GetNumberOfCells() << "\n";

  // View the resulting Voronoi tessellation
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(vor->GetOutputPort());
  mapper->SetScalarRange(-1, numCircles - 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Extract edges and display them, work around OpenGL bug (as compared to
  // enabling EdgeVisibility which causes issues on some systems).
  vtkNew<vtkExtractEdges> extract;
  extract->SetInputConnection(vor->GetOutputPort());

  vtkNew<vtkPolyDataMapper> extractMapper;
  extractMapper->SetInputConnection(extract->GetOutputPort());
  extractMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> edgeActor;
  edgeActor->SetMapper(extractMapper);
  edgeActor->GetProperty()->SetColor(0, 0, 0);
  edgeActor->GetProperty()->RenderLinesAsTubesOn();
  edgeActor->GetProperty()->SetLineWidth(1);

  // Surface net
  vtkNew<vtkVoronoiFlower2D> sn;
  sn->SetInputConnection(sampler->GetOutputPort());
  sn->SetOutputTypeToSurfaceNet();

  timer->StartTimer();
  sn->Update();
  timer->StopTimer();
  time = timer->GetElapsedTime();
  // Report some stats
  std::cout << "Time to SurfaceNet data: " << time << "\n";
  std::cout << "\tNumber of threads used: " << vor->GetNumberOfThreads() << "\n";
  std::cout << "\tNumber of input Voroni points: " << sampler->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output SurfaceNet points: " << sn->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output SurfaceNet lines: " << sn->GetOutput()->GetNumberOfCells()
            << "\n";

  vtkNew<vtkPolyDataMapper> snMapper;
  snMapper->SetInputConnection(sn->GetOutputPort());
  snMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> snActor;
  snActor->SetMapper(snMapper);
  snActor->GetProperty()->SetColor(0, 1, 0);
  snActor->GetProperty()->RenderLinesAsTubesOn();
  snActor->GetProperty()->SetLineWidth(4);

  vtkNew<vtkConstrainedSmoothingFilter> smooth;
  smooth->SetInputConnection(sn->GetOutputPort());
  smooth->SetConstraintDistance(1);
  smooth->SetRelaxationFactor(0.25);
  smooth->SetNumberOfIterations(40);

  vtkNew<vtkPolyDataMapper> smoothMapper;
  smoothMapper->SetInputConnection(smooth->GetOutputPort());
  smoothMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> smoothActor;
  smoothActor->SetMapper(smoothMapper);
  smoothActor->GetProperty()->SetColor(0, 1, 0);
  smoothActor->GetProperty()->RenderLinesAsTubesOn();
  smoothActor->GetProperty()->SetLineWidth(4);

  vtkNew<vtkRenderer> ren0;
  ren0->SetViewport(0, 0.5, 0.5, 1);
  ren0->SetBackground(1, 1, 1);
  ren0->SetBackground(0, 0, 0);
  ren0->AddActor(pointsActor);

  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0.5, 0.5, 1, 1);
  ren1->SetBackground(1, 1, 1);
  ren1->SetBackground(0, 0, 0);
  ren1->AddActor(actor);
  ren1->AddActor(edgeActor);

  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0, 0, 0.5, 0.5);
  ren2->SetBackground(1, 1, 1);
  ren2->SetBackground(0, 0, 0);
  ren2->AddActor(snActor);

  vtkNew<vtkRenderer> ren3;
  ren3->SetViewport(0.5, 0, 1, 0.5);
  ren3->SetBackground(1, 1, 1);
  ren3->SetBackground(0, 0, 0);
  ren3->AddActor(smoothActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(1200, 1000);
  renWin->AddRenderer(ren0);
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  renWin->AddRenderer(ren3);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren1->ResetCamera();
  renWin->Render();
  ren1->GetActiveCamera()->Zoom(1.4);
  ren0->SetActiveCamera(ren1->GetActiveCamera());
  ren2->SetActiveCamera(ren1->GetActiveCamera());
  ren3->SetActiveCamera(ren1->GetActiveCamera());

  static_cast<vtkLookupTable*>(pointsMapper->GetLookupTable())->SetHueRange(0, 0.667);
  pointsMapper->Modified();
  static_cast<vtkLookupTable*>(pointsMapper->GetLookupTable())->Build();
  static_cast<vtkLookupTable*>(pointsMapper->GetLookupTable())->SetTableValue(0, 1, 0, 0, 1);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
