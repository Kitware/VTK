/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Timingtests.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
To add a test you must define a subclass of vtkRTTest and implement the
pure virtual functions. Then in the main section at the bottom of this
file add your test to the tests to be run and rebuild. See some of the
existing tests to get an idea of what to do.
*/

#include "vtkRenderTimings.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkNew.h"

/*=========================================================================
Define a test for simple triangle mesh surfaces
=========================================================================*/
#include "vtkParametricBoy.h"
#include "vtkParametricTorus.h"
#include "vtkParametricFunctionSource.h"

class surfaceTest : public vtkRTTest
{
public:
  surfaceTest(const char *name, bool withColors, bool withNormals) : vtkRTTest(name)
  {
    this->WithColors = withColors;
    this->WithNormals = withNormals;
  }

  const char *GetSummaryResultName() { return "Mtris/sec"; }

  const char *GetSecondSummaryResultName() { return "triangles"; }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats, int /*argc*/, char * /* argv */[])
    {
    int ures, vres;
    ats->GetSequenceNumbers(ures,vres);

    // ------------------------------------------------------------
    // Create surface
    // ------------------------------------------------------------
    //  vtkNew<vtkParametricBoy> PB;
    vtkNew<vtkParametricTorus> PB;
    vtkNew<vtkParametricFunctionSource> PFS;
    PFS->SetParametricFunction(PB.Get());
    if (this->WithColors)
      {
      PFS->SetScalarModeToPhase();
      }
    else
      {
      PFS->SetScalarModeToNone();
      }
    if (this->WithNormals == false)
      {
      PFS->GenerateNormalsOff();
      }
    PFS->SetUResolution(ures * 50);
    PFS->SetVResolution(vres * 100);
    PFS->Update();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(PFS->GetOutputPort());
    mapper->SetScalarRange(0.0, 360.0);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.Get());
    ren1->AddActor(actor.Get());

    // set the size/color of our window
    renWindow->SetSize(500, 500);
    ren1->SetBackground(0.2, 0.3, 0.5);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;
    ren1->GetActiveCamera()->Azimuth(90);
    ren1->ResetCameraClippingRange();

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(1);
      ren1->GetActiveCamera()->Elevation(1);
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime) >
          this->TargetTime * 1.5)
        {
        frameCount = i + 1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime() - startTime -
                                  firstFrameTime) / frameCount;
    double numTris = PFS->GetOutput()->GetPolys()->GetNumberOfCells();

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["Mtris"] = 1.0e-6 * numTris;
    result.Results["Mtris/sec"] = 1.0e-6 * numTris / subsequentFrameTime;
    result.Results["triangles"] = numTris;

    return result;
    }

protected:
  bool WithNormals;
  bool WithColors;
};

/*=========================================================================
Define a test for glyphing
=========================================================================*/
#include "vtkGlyph3DMapper.h"
#include "vtkPlaneSource.h"
#include "vtkElevationFilter.h"
#include "vtkSphereSource.h"

class glyphTest : public vtkRTTest
{
public:
  glyphTest(const char *name) : vtkRTTest(name)
  {
  }

  const char *GetSummaryResultName() { return "Mtris/sec"; }

  const char *GetSecondSummaryResultName() { return "triangles"; }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats, int /*argc*/, char * /* argv */[])
    {
    int res1, res2, res3, res4;
    ats->GetSequenceNumbers(res1, res2, res3, res4);

    // create
    vtkNew<vtkPlaneSource> plane;
    plane->SetResolution(res1 * 10, res2 * 10);
    plane->SetOrigin(-res1 * 5.0, -res2 * 5.0, 0.0);
    plane->SetPoint1(res1 * 5.0,-res2 * 5.0, 0.0);
    plane->SetPoint2(-res1 * 5.0,res2 * 5.0, 0.0);
    vtkNew<vtkElevationFilter> colors;
    colors->SetInputConnection(plane->GetOutputPort());
    colors->SetLowPoint(plane->GetOrigin());
    colors->SetHighPoint(res1 * 5.0, res2 * 5.0, 0.0);

    // create simple poly data so we can apply glyph
    vtkNew<vtkSphereSource> sphere;
    sphere->SetPhiResolution(5 * res3 + 2);
    sphere->SetThetaResolution(10 * res4);
    sphere->SetRadius(0.7);

    vtkNew<vtkGlyph3DMapper> mapper;
    mapper->SetInputConnection(colors->GetOutputPort());
    mapper->SetSourceConnection(sphere->GetOutputPort());
    mapper->SetScalarRange(0.0, 2.0);

    // vtkNew<vtkPolyDataMapper> mapper;
    // mapper->SetInputConnection(colors->GetOutputPort());
    // mapper->SetScalarRange(0.0,2.0);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.Get());
    ren1->AddActor(actor.Get());

    // set the size/color of our window
    renWindow->SetSize(600, 600);
    ren1->SetBackground(0.2, 0.3, 0.5);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(0.5);
      ren1->GetActiveCamera()->Elevation(0.5);
      ren1->GetActiveCamera()->Zoom(1.01);
      ren1->ResetCameraClippingRange();
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime) >
          this->TargetTime * 1.5)
        {
        frameCount = i + 1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime() - startTime -
                                  firstFrameTime)/frameCount;
    double numTris = 100.0 * res1 * res2 *
                     sphere->GetOutput()->GetPolys()->GetNumberOfCells();

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["Mtris"] = 1.0e-6 * numTris;
    result.Results["Mtris/sec"] = 1.0e-6 * numTris/subsequentFrameTime;
    result.Results["triangles"] = numTris;

    return result;
    }

protected:
  bool WithNormals;
  bool WithColors;
};

/*=========================================================================
The main entry point
=========================================================================*/
int main( int argc, char *argv[] )
{
  // create the timing framework
  vtkRenderTimings a;

  // add the tests
  a.TestsToRun.push_back(new surfaceTest("Surface", false, false));
  a.TestsToRun.push_back(new surfaceTest("SurfaceColored", true, false));
  a.TestsToRun.push_back(new surfaceTest("SurfaceWithNormals", false, true));
  a.TestsToRun.push_back(new surfaceTest("SurfaceColoredWithNormals", true, true));

  a.TestsToRun.push_back(new glyphTest("Glyphing"));

  // process them
  return a.ParseCommandLineArguments(argc, argv);
}
