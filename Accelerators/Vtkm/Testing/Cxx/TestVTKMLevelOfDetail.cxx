/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Uncomment this to directly compare serial and TBB versions
// #define FORCE_VTKM_DEVICE

// TODO: Make a way to force the VTK-m device without actually loading VTK-m
// headers (and all subsequent dependent headers).

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkMaskPoints.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkQuadricClustering.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkTriangleFilter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkmLevelOfDetail.h"

#ifdef FORCE_VTKM_DEVICE

#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#endif // FORCE_VTKM_DEVICE

#include <iomanip>
#include <sstream>

/*
 * This test has benchmarking code as well as a unit test.
 *
 * To run the benchmarks, add a "Benchmark" argument when invoking this test.
 *
 * By default, a wavelet is generated and used to time the filter's execution.
 * By setting the LUCY_PATH define below to the path to lucy.ply (or any other
 * ply file), other datasets can be used during benchmarking.
 *
 * The benchmark will print out timing information comparing vtkmLevelOfDetail
 * to vtkQuadricClustering, and also generate side-by-side renderings of each
 * algorithm for various grid dimensions. These images are written to the
 * working directory can be combined into a summary image by running
 * imagemagick's convert utility:
 *
 * convert LOD_0* -append summary.png
 */

//#define LUCY_PATH "/prm/lucy.ply"

namespace
{

const static int NUM_SAMPLES = 1;
const static int FONT_SIZE = 30;

struct VTKmFilterGenerator
{
  using FilterType = vtkmLevelOfDetail;

  int GridSize;

  VTKmFilterGenerator(int gridSize)
    : GridSize(gridSize)
  {
  }

  FilterType* operator()() const
  {
    FilterType* filter = FilterType::New();
    filter->SetNumberOfDivisions(this->GridSize, this->GridSize, this->GridSize);
    return filter;
  }

  vtkSmartPointer<vtkPolyData> Result;
};

struct VTKFilterGenerator
{
  using FilterType = vtkQuadricClustering;

  int GridSize;
  bool UseInputPoints;
  vtkSmartPointer<vtkPolyData> Result;

  VTKFilterGenerator(int gridSize, bool useInputPoints)
    : GridSize(gridSize)
    , UseInputPoints(useInputPoints)
  {
  }

  FilterType* operator()() const
  {
    FilterType* filter = FilterType::New();
    filter->SetNumberOfDivisions(this->GridSize, this->GridSize, this->GridSize);

    // Mimic PV's GeometeryRepresentation decimator settings:
    filter->SetAutoAdjustNumberOfDivisions(0);
    filter->SetUseInternalTriangles(0);
    filter->SetCopyCellData(1);
    filter->SetUseInputPoints(this->UseInputPoints ? 1 : 0);

    return filter;
  }
};

template <typename FilterGenerator>
double BenchmarkFilter(FilterGenerator& filterGen, vtkPolyData* input)
{
  using FilterType = typename FilterGenerator::FilterType;

  vtkNew<vtkTimerLog> timer;
  double result = 0.f;

  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    FilterType* filter = filterGen();
    filter->SetInputData(input);

    timer->StartTimer();
    filter->Update();
    timer->StopTimer();

    result += timer->GetElapsedTime();
    filterGen.Result = filter->GetOutput();
    filter->Delete();
  }

  return result / static_cast<double>(NUM_SAMPLES);
}

void RenderResults(int gridSize, vtkPolyData* input, double vtkmTime, vtkPolyData* vtkmData,
  double vtkTime, vtkPolyData* vtkData)
{
  double modelColor[3] = { 1., 1., 1. };
  double bgColor[3] = { .75, .75, .75 };
  double textColor[3] = { 0., 0., 0. };

  vtkNew<vtkRenderer> vtkRen;
  {
    vtkRen->SetViewport(0., 0., 0.5, 1.);
    vtkRen->SetBackground(bgColor);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(vtkData);
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->SetColor(modelColor);
    vtkRen->AddActor(actor);

    std::ostringstream tmp;
    tmp << "VTK: " << std::setprecision(3) << vtkTime << "s\n"
        << "NumPts: " << vtkData->GetNumberOfPoints() << "\n"
        << "NumTri: " << vtkData->GetNumberOfCells() << "\n";

    vtkNew<vtkTextActor> timeText;
    timeText->SetInput(tmp.str().c_str());
    timeText->GetTextProperty()->SetJustificationToCentered();
    timeText->GetTextProperty()->SetColor(textColor);
    timeText->GetTextProperty()->SetFontSize(FONT_SIZE);
    timeText->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    timeText->GetPositionCoordinate()->SetValue(0.5, 0.01);
    vtkRen->AddActor(timeText);
  }

  vtkNew<vtkRenderer> vtkmRen;
  {
    vtkmRen->SetViewport(0.5, 0., 1., 1.);
    vtkmRen->SetBackground(bgColor);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(vtkmData);
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->SetColor(modelColor);
    vtkmRen->AddActor(actor);

    std::ostringstream tmp;
    tmp << "VTK-m: " << std::setprecision(3) << vtkmTime << "s\n"
        << "NumPts: " << vtkmData->GetNumberOfPoints() << "\n"
        << "NumTri: " << vtkmData->GetNumberOfCells() << "\n";

    vtkNew<vtkTextActor> timeText;
    timeText->SetInput(tmp.str().c_str());
    timeText->GetTextProperty()->SetJustificationToCentered();
    timeText->GetTextProperty()->SetColor(textColor);
    timeText->GetTextProperty()->SetFontSize(FONT_SIZE);
    timeText->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    timeText->GetPositionCoordinate()->SetValue(0.5, 0.01);
    vtkmRen->AddActor(timeText);
  }

  vtkNew<vtkRenderer> metaRen;
  {
    metaRen->SetPreserveColorBuffer(1);

    std::ostringstream tmp;
    tmp << gridSize << "x" << gridSize << "x" << gridSize << "\n"
        << "InPts: " << input->GetNumberOfPoints() << "\n"
        << "InTri: " << input->GetNumberOfCells() << "\n";

    vtkNew<vtkTextActor> gridText;
    gridText->SetInput(tmp.str().c_str());
    gridText->GetTextProperty()->SetJustificationToCentered();
    gridText->GetTextProperty()->SetVerticalJustificationToTop();
    gridText->GetTextProperty()->SetColor(textColor);
    gridText->GetTextProperty()->SetFontSize(FONT_SIZE);
    gridText->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    gridText->GetPositionCoordinate()->SetValue(0.5, 0.95);
    metaRen->AddActor(gridText);
  }

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(800, 400);
  renWin->AddRenderer(vtkRen);
  renWin->AddRenderer(vtkmRen);
  renWin->AddRenderer(metaRen);

  renWin->Render();

#ifdef LUCY_PATH
  vtkRen->GetActiveCamera()->SetPosition(0, 1, 0);
  vtkRen->GetActiveCamera()->SetViewUp(0, 0, 1);
  vtkRen->GetActiveCamera()->SetFocalPoint(0, 0, 0);
#endif

  vtkRen->ResetCamera();
  vtkRen->GetActiveCamera()->Zoom(2.0);
  vtkmRen->SetActiveCamera(vtkRen->GetActiveCamera());
  renWin->Render();

  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(renWin);

  std::ostringstream tmp;
  tmp << "LOD_" << std::setw(4) << std::setfill('0') << std::right << gridSize << ".png";

  vtkNew<vtkPNGWriter> png;
  png->SetInputConnection(w2i->GetOutputPort());
  png->SetFileName(tmp.str().c_str());
  png->Write();
}

void RunBenchmark(int gridSize)
{
  // Prepare input dataset:
  static vtkSmartPointer<vtkPolyData> input;
  if (!input)
  {
#ifndef LUCY_PATH
    vtkNew<vtkRTAnalyticSource> wavelet;
    wavelet->SetXFreq(60);
    wavelet->SetYFreq(30);
    wavelet->SetZFreq(40);
    wavelet->SetXMag(10);
    wavelet->SetYMag(18);
    wavelet->SetZMag(5);
    wavelet->SetWholeExtent(-255, 256, -255, 256, -127, 128);
    vtkNew<vtkContourFilter> contour;
    contour->SetInputConnection(wavelet->GetOutputPort());
    contour->SetNumberOfContours(1);
    contour->SetValue(0, 157.);
    contour->Update();
    input = contour->GetOutput();
#else
    vtkNew<vtkPLYReader> reader;
    reader->SetFileName(LUCY_PATH);
    reader->Update();
    input = reader->GetOutput();
#endif
  }

#ifdef FORCE_VTKM_DEVICE

  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetRuntimeDeviceTracker();

  // Run VTKm
  vtkSmartPointer<vtkPolyData> vtkmResultSerial;
  double vtkmTimeSerial = 0.;
  {
    tracker.ForceDevice(vtkm::cont::DeviceAdapterTagSerial());
    VTKmFilterGenerator generator(gridSize);
    vtkmTimeSerial = BenchmarkFilter(generator, input);
    vtkmResultSerial = generator.Result;
    tracker.Reset();
  }

#ifdef VTKM_ENABLE_TBB
  vtkSmartPointer<vtkPolyData> vtkmResultTBB;
  double vtkmTimeTBB = 0.;
  bool tbbDeviceValid = tracker.CanRunOn(vtkm::cont::DeviceAdapterTagTBB());
  if (tbbDeviceValid)
  {
    tracker.ForceDevice(vtkm::cont::DeviceAdapterTagTBB());
    VTKmFilterGenerator generator(gridSize);
    vtkmTimeTBB = BenchmarkFilter(generator, input);
    vtkmResultTBB = generator.Result;
    tracker.Reset();
  }
#endif // VTKM_ENABLE_TBB

#else // !FORCE_VTKM_DEVICE

  // Run VTKm
  vtkSmartPointer<vtkPolyData> vtkmResult;
  double vtkmTime = 0.;
  {
    VTKmFilterGenerator generator(gridSize);
    vtkmTime = BenchmarkFilter(generator, input);
    vtkmResult = generator.Result;
  }

#endif

  // Run VTK -- average clustered points
  vtkSmartPointer<vtkPolyData> vtkResultAvePts;
  double vtkTimeAvePts = 0.;
  {
    VTKFilterGenerator generator(gridSize, false);
    vtkTimeAvePts = BenchmarkFilter(generator, input);
    vtkResultAvePts = generator.Result;
  }

  // Run VTK -- reuse input points
  vtkSmartPointer<vtkPolyData> vtkResult;
  double vtkTime = 0.;
  {
    VTKFilterGenerator generator(gridSize, true);
    vtkTime = BenchmarkFilter(generator, input);
    vtkResult = generator.Result;
  }

  std::cerr << "Results for a " << gridSize << "x" << gridSize << "x" << gridSize << " grid.\n"
            << "Input dataset has " << input->GetNumberOfPoints()
            << " points "
               "and "
            << input->GetNumberOfCells() << " cells.\n";

#ifdef FORCE_VTKM_DEVICE

  std::cerr << "vtkmLevelOfDetail (serial, average clustered points): " << vtkmTimeSerial
            << " seconds, " << vtkmResultSerial->GetNumberOfPoints() << " points, "
            << vtkmResultSerial->GetNumberOfCells() << " cells.\n";

#ifdef VTKM_ENABLE_TBB
  if (tbbDeviceValid)
  {
    std::cerr << "vtkmLevelOfDetail (tbb, average clustered points): " << vtkmTimeTBB
              << " seconds, " << vtkmResultTBB->GetNumberOfPoints() << " points, "
              << vtkmResultTBB->GetNumberOfCells() << " cells.\n";
  }
#endif // VTKM_ENABLE_TBB

#else // !FORCE_VTKM_DEVICE

  std::cerr << "vtkmLevelOfDetail (average clustered points): " << vtkmTime << " seconds, "
            << vtkmResult->GetNumberOfPoints() << " points, " << vtkmResult->GetNumberOfCells()
            << " cells.\n";

#endif // !FORCE_VTKM_DEVICE

  std::cerr << "vtkQuadricClustering (average clustered points): " << vtkTimeAvePts << " seconds, "
            << vtkResultAvePts->GetNumberOfPoints() << " points, "
            << vtkResultAvePts->GetNumberOfCells() << " cells.\n"
            << "vtkQuadricClustering (reuse input points): " << vtkTime << " seconds, "
            << vtkResult->GetNumberOfPoints() << " points, " << vtkResult->GetNumberOfCells()
            << " cells.\n";

#ifdef FORCE_VTKM_DEVICE
#ifdef VTKM_ENABLE_TBB
  RenderResults(gridSize, input, vtkmTimeTBB, vtkmResultTBB, vtkTime, vtkResult);
#endif // VTKM_ENABLE_TBB
#else  // !FORCE_VTKM_DEVICE
  RenderResults(gridSize, input, vtkmTime, vtkmResult, vtkTime, vtkResult);
#endif // !FORCE_VTKM_DEVICE
}

void RunBenchmarks()
{
  RunBenchmark(32);
  RunBenchmark(64);
  RunBenchmark(128);
  RunBenchmark(256);
  RunBenchmark(512);
}

} // end anon namespace

int TestVTKMLevelOfDetail(int argc, char* argv[])
{
  bool doBenchmarks = false;

  for (int i = 1; i < argc; ++i)
  {
    if (std::string("Benchmark") == argv[i])
    {
      doBenchmarks = true;
      break;
    }
  }

  if (doBenchmarks)
  {
    RunBenchmarks();
    return 0;
  }

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  //---------------------------------------------------
  // Load file and make only triangles
  //---------------------------------------------------
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTriangleFilter> clean;
  clean->SetInputConnection(reader->GetOutputPort());
  clean->Update();

  //---------------------------------------------------
  // Test LOD filter 4 times
  // We will setup 4 instances of the filter at different
  // levels of subdivision to make sure it is working properly
  //---------------------------------------------------

  std::vector<vtkNew<vtkmLevelOfDetail> > levelOfDetails(4);
  std::vector<vtkNew<vtkDataSetSurfaceFilter> > surfaces(4);
  std::vector<vtkNew<vtkPolyDataMapper> > mappers(4);
  std::vector<vtkNew<vtkActor> > actors(4);

  for (int i = 0; i < 4; ++i)
  {
    levelOfDetails[i]->SetInputConnection(clean->GetOutputPort());
    // subdivision levels of 16, 32, 48, 64
    levelOfDetails[i]->SetNumberOfXDivisions(((i + 1) * 16));
    levelOfDetails[i]->SetNumberOfYDivisions(((i + 1) * 16));
    levelOfDetails[i]->SetNumberOfZDivisions(((i + 1) * 16));

    surfaces[i]->SetInputConnection(levelOfDetails[i]->GetOutputPort());

    mappers[i]->SetInputConnection(surfaces[i]->GetOutputPort());

    actors[i]->SetMapper(mappers[i]);
    actors[i]->SetPosition((i % 2) * 10, -(i / 2) * 10, 0);

    ren->AddActor(actors[i]);
  }

  ren->SetBackground(0.1, 0.2, 0.4);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.3);
  renWin->SetSize(600, 600);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}
