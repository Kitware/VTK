/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GLBenchmarking.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAutoInit.h"
//VTK_MODULE_INIT(vtkRenderingFreeType)
#ifdef VTK_OPENGL2
//VTK_MODULE_INIT(vtkRenderingOpenGL2)
//VTK_MODULE_INIT(vtkRenderingContextOpenGL2)
#else
//VTK_MODULE_INIT(vtkRenderingOpenGL)
//VTK_MODULE_INIT(vtkRenderingContextOpenGL)
#endif

#include "vtkActor.h"
#include "vtkAxis.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkChartLegend.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDelimitedTextWriter.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariant.h"
#include "vtkVector.h"

#include "vtkParametricBoy.h"
#include "vtkParametricTorus.h"
#include "vtkParametricFunctionSource.h"

#include <vtksys/CommandLineArguments.hxx>

namespace vtk
{
class BenchmarkTest
{
public:
  BenchmarkTest() { ; }
  virtual ~BenchmarkTest() { ; }

  virtual vtkIdType Build(vtkRenderer *, const vtkVector2i &)
  {
    return 0;
  }
};

class SurfaceTest : public BenchmarkTest
{
public:
  SurfaceTest()
  {
  }

  virtual ~SurfaceTest()
  {
  }

  virtual vtkIdType Build(vtkRenderer *renderer, const vtkVector2i &res)
  {
    //vtkVector2i res(20, 50);
    vtkNew<vtkParametricBoy> parametricShape;
    vtkNew<vtkParametricFunctionSource> parametricSource;
    parametricSource->SetParametricFunction(parametricShape.Get());
    parametricSource->SetUResolution(res[0] * 50);
    parametricSource->SetVResolution(res[1] * 100);
    parametricSource->Update();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(parametricSource->GetOutputPort());
    mapper->SetScalarRange(0, 360);
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());
    renderer->AddActor(actor.Get());

    return parametricSource->GetOutput()->GetPolys()->GetNumberOfCells();
  }
};

vtkVector2i GenerateSequenceNumbers(int sequenceCount)
{
  const int seqX[] = { 1, 2, 3, 5, 5, 5, 6, 10 };
  const int seqY[] = { 1, 1, 1, 1, 2, 4, 5,  5 };
  vtkVector2i val(1, 1);
  while (sequenceCount >= 8)
    {
    val[0] *= 10;
    val[1] *= 10;
    sequenceCount -= 8;
    }
  val[0] *= seqX[sequenceCount];
  val[1] *= seqY[sequenceCount];
  return val;
}

} // End namespace

bool runTest(vtkRenderer *renderer, vtkTable *results, int seq, int row,
             double timeout = 0.5)
{
  vtk::SurfaceTest surfaceTest;
  vtkIdType triangles = surfaceTest.Build(renderer,
                                          vtk::GenerateSequenceNumbers(seq));

  double startTime = vtkTimerLog::GetUniversalTime();
  vtkRenderWindow *window = renderer->GetRenderWindow();
  renderer->ResetCamera();
  window->Render();

  double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;

  renderer->GetActiveCamera()->Azimuth(90);
  renderer->ResetCameraClippingRange();

  int frameCount = 50;
  for (int i = 0; i < frameCount; ++i)
    {
    window->Render();
    renderer->GetActiveCamera()->Azimuth(3);
    renderer->GetActiveCamera()->Elevation(1);
    }
  double subsequentFrameTime = (vtkTimerLog::GetUniversalTime() - startTime -
                                firstFrameTime) / frameCount;

  results->SetValue(row, 0, triangles);
  results->SetValue(row, 1, firstFrameTime);
  results->SetValue(row, 2, subsequentFrameTime);
  results->SetValue(row, 3, triangles / subsequentFrameTime * 1e-6);
  results->Modified();

  cout << "First frame:\t" << firstFrameTime
       << "\nAverage frame:\t" << subsequentFrameTime
       << "\nTriangles (M):\t" << triangles * 1e-6
       << "\nMtris/sec:\t" << triangles / subsequentFrameTime * 1e-6
       << "\nRow:\t" << row
       << endl;

  return subsequentFrameTime <= timeout;
}

class Arguments
{
public:
  Arguments(int argc, char *argv[]) : Start(0), End(16), Timeout(1.0),
    FileName("results.csv"), DisplayHelp(false)
  {
    typedef vtksys::CommandLineArguments arg;
    this->Args.Initialize(argc, argv);
    this->Args.AddArgument("--start", arg::SPACE_ARGUMENT,
                           &this->Start,
                           "Start of the test sequence sizes");
    this->Args.AddArgument("--end", arg::SPACE_ARGUMENT,
                           &this->End,
                           "End of the test sequence sizes");
    this->Args.AddArgument("--timeout", arg::SPACE_ARGUMENT,
                           &this->Timeout,
                           "Maximum average frame time before test termination");
    this->Args.AddArgument("--file", arg::SPACE_ARGUMENT,
                           &this->FileName,
                           "File to save results to");
    this->Args.AddBooleanArgument("--help",
                                  &this->DisplayHelp,
                                  "Provide a listing of command line options");

    if (!this->Args.Parse())
      {
      cerr << "Problem parsing arguments" << endl;
      }

    if (this->DisplayHelp)
      {
      cout << "Usage" << endl << endl << this->Args.GetHelp() << endl;
      }
  }

  vtksys::CommandLineArguments Args;
  int Start;
  int End;
  double Timeout;
  std::string FileName;
  bool DisplayHelp;
};

int main(int argc, char *argv[])
{
  Arguments args(argc, argv);
  if (args.DisplayHelp)
    {
    return 0;
    }

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer.Get());
  window->SetSize(800, 600);
  renderer->SetBackground(0.2, 0.3, 0.4);
  vtkNew<vtkCamera> refCamera;
  refCamera->DeepCopy(renderer->GetActiveCamera());

  // Set up our results table, this will be used for our timings etc.
  vtkNew<vtkTable> results;
  vtkNew<vtkIntArray> tris;
  tris->SetName("Triangles");
  vtkNew<vtkDoubleArray> firstFrame;
  firstFrame->SetName("First Frame");
  vtkNew<vtkDoubleArray> averageFrame;
  averageFrame->SetName("Average Frame");
  vtkNew<vtkDoubleArray> triRate;
  triRate->SetName("Mtris/sec");
  results->AddColumn(tris.Get());
  results->AddColumn(firstFrame.Get());
  results->AddColumn(averageFrame.Get());
  results->AddColumn(triRate.Get());

  // Set up a chart to show the data being generated in real time.
  vtkNew<vtkContextView> chartView;
  chartView->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkChartXY> chart;
  chartView->GetScene()->AddItem(chart.Get());
  vtkPlot *plot = chart->AddPlot(vtkChart::LINE);
  plot->SetInputData(results.Get(), 0, 3);
  plot = chart->AddPlot(vtkChart::LINE);
  plot->SetInputData(results.Get(), 0, 1);
  chart->SetPlotCorner(plot, 1);
  plot = chart->AddPlot(vtkChart::LINE);
  plot->SetInputData(results.Get(), 0, 2);
  chart->SetPlotCorner(plot, 1);
  chart->GetAxis(vtkAxis::LEFT)->SetTitle("Mtris/sec");
  chart->GetAxis(vtkAxis::BOTTOM)->SetTitle("triangles");
  chart->GetAxis(vtkAxis::RIGHT)->SetTitle("time (sec)");
  chart->SetShowLegend(true);
  chart->GetLegend()->SetHorizontalAlignment(vtkChartLegend::LEFT);

  int startSeq = args.Start;
  int endSeq = args.End;
  results->SetNumberOfRows(endSeq - startSeq + 1);
  int row = 0;
  for (int i = startSeq; i <= endSeq; ++i)
    {
    cout << "Running sequence point " << i << endl;
    results->SetNumberOfRows(i - startSeq + 1);
    window->Render();
    renderer->RemoveAllViewProps();
    renderer->GetActiveCamera()->DeepCopy(refCamera.Get());
    if (!runTest(renderer.Get(), results.Get(), i, row++, args.Timeout))
      {
      break;
      }
    if (results->GetNumberOfRows() > 1)
      {
      chart->RecalculateBounds();
      chartView->Render();
      }
    }

  vtkNew<vtkDelimitedTextWriter> writer;
  writer->SetInputData(results.Get());
  writer->SetFileName(args.FileName.c_str());
  writer->Update();
  writer->Write();

  return 0;
}
