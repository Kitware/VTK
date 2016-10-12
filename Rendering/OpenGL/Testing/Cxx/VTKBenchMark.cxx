/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VTKBenchMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCutter.h"
#include "vtkImageData.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphere.h"
#include "vtkStripper.h"
#include "vtkTimerLog.h"
#include "vtkTriangleFilter.h"

class VTKBenchmark
{
public:

  // Description:
  // The main entry point for the benchmark
  int Run();

  VTKBenchmark();

private:

  double BuildTheFractal();
  double DrawTheFractal();

  vtkSmartPointer<vtkTimerLog> Timer;
  vtkSmartPointer<vtkImageMandelbrotSource> Mandelbrot;
  vtkSmartPointer<vtkImageGaussianSmooth> GaussianSmooth;
  vtkSmartPointer<vtkCutter> Cutter;
  vtkSmartPointer<vtkTriangleFilter> TriFilter;
  vtkSmartPointer<vtkStripper> Stripper;
  vtkSmartPointer<vtkPolyDataNormals> Normals;

  int ImmediateMode;
  int ScalarColoring;
  int UseNormals;

  // all times are in seconds
  double DataBuildTime;
};

VTKBenchmark::VTKBenchmark()
{
  this->Timer = vtkSmartPointer<vtkTimerLog>::New();
  this->Mandelbrot = vtkSmartPointer<vtkImageMandelbrotSource>::New();
  this->GaussianSmooth = vtkSmartPointer<vtkImageGaussianSmooth>::New();
  this->Cutter = vtkSmartPointer<vtkCutter>::New();
  this->TriFilter = vtkSmartPointer<vtkTriangleFilter>::New();
  this->Stripper = vtkSmartPointer<vtkStripper>::New();
  this->Normals = vtkSmartPointer<vtkPolyDataNormals>::New();

  this->ImmediateMode = 1;
  this->ScalarColoring = 0;
  this->UseNormals = 0;
  this->DataBuildTime=0.0;
}

double VTKBenchmark::BuildTheFractal()
{
  cerr << "Building Fractal ... (this may take a minute or two)\n";

  // time the data creation
  this->Timer->StartTimer();

  // first we want to create some data, a 256 cubed Mandelbrot src
  this->Mandelbrot->SetWholeExtent(0,255,0,255,0,255);
  this->Mandelbrot->SetOriginCX(-1.75,-1.25,-1,0);
  this->Mandelbrot->Update();

  cerr << "Smoothing...\n";
  this->GaussianSmooth->SetInputConnection(this->Mandelbrot->GetOutputPort());
  this->GaussianSmooth->Update();

  // extract a sphere from the fractal volume
  vtkSmartPointer<vtkSphere> sphere =
    vtkSmartPointer<vtkSphere>::New();

  // add two contours
  cerr << "Cutting...\n";
  this->Cutter->SetInputConnection(this->GaussianSmooth->GetOutputPort());
  this->Cutter->SetCutFunction(sphere);
  this->Cutter->Update();

  // convert it to all triangles
  cerr << "Converting to Triangles...\n";
  this->TriFilter->SetInputConnection(this->Cutter->GetOutputPort());
  this->TriFilter->Update();

  // generate Normals
  cerr << "Computing Normals...\n";
  this->Normals->SetInputConnection(this->TriFilter->GetOutputPort());
  this->Normals->Update();

  // and then strip them
  cerr << "Creating Strips...\n";
  this->Stripper->SetInputConnection(this->Normals->GetOutputPort());
  this->Stripper->Update();


  cerr << "Number Of Triangles: " <<
    this->TriFilter->GetOutput()->GetNumberOfPolys() << "\n";
  cerr << "Average Strip Length: " <<
    static_cast<double>(this->TriFilter->GetOutput()->GetNumberOfPolys())/
    static_cast<double>(this->Stripper->GetOutput()->GetNumberOfStrips()) << "\n";

  this->Timer->StopTimer();

  return this->Timer->GetElapsedTime();
}


int VTKBenchmark::Run()
{
  this->DataBuildTime = this->BuildTheFractal();

  cerr << "Build Rate: " << 1.0/this->DataBuildTime << "\n";

  for (this->ImmediateMode = 0; this->ImmediateMode < 2;
       this->ImmediateMode++)
  {
    for (this->ScalarColoring = 0; this->ScalarColoring < 2;
         this->ScalarColoring++)
    {
      for (this->UseNormals = 0; this->UseNormals < 2;
           this->UseNormals++)
      {
        cerr << "Render Rate: "
             << (this->ImmediateMode ? "IMED " : "     ")
             << (this->ScalarColoring ? "SCAL " : "     ")
             << (this->UseNormals ? "NORM " : "     ")
             << this->DrawTheFractal() << " MegaTriangles/Second\n";
      }
    }
  }

  return 0;
}


double VTKBenchmark::DrawTheFractal()
{
  // create a rendering window and both renderers
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWindow->AddRenderer(ren1);

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

  if (this->UseNormals)
  {
    this->Stripper->SetInputConnection(this->Normals->GetOutputPort());
  }
  else
  {
    this->Stripper->SetInputConnection(this->TriFilter->GetOutputPort());
  }
  mapper->SetInputConnection(this->Stripper->GetOutputPort());
  mapper->SetImmediateModeRendering(this->ImmediateMode);
  mapper->SetScalarVisibility(this->ScalarColoring);
  mapper->SetScalarRange(5,30);

  actor->SetMapper(mapper);
  ren1->AddActor(actor);

  // set the size of our window
  renWindow->SetSize(500,500);

  // set the viewports and background of the renderers
  ren1->SetBackground(0.2,0.3,0.5);

  // draw the resulting scene
  renWindow->Render();

  this->Timer->StartTimer();

  // do a azimuth of the cameras 50 degrees per iteration
  ren1->GetActiveCamera()->Azimuth(50);
  renWindow->Render();
  ren1->GetActiveCamera()->Azimuth(50);
  renWindow->Render();
  ren1->GetActiveCamera()->Azimuth(50);
  renWindow->Render();
  ren1->GetActiveCamera()->Zoom(3.0);
  ren1->GetActiveCamera()->Azimuth(50);
  renWindow->Render();
  ren1->GetActiveCamera()->Elevation(50);
  renWindow->Render();
  ren1->GetActiveCamera()->Elevation(50);
  renWindow->Render();

  this->Timer->StopTimer();

  // compute the M triangles per second
  double numTris =
    static_cast<double>(this->TriFilter->GetOutput()->GetNumberOfPolys());

  return 6.0e-6*numTris/this->Timer->GetElapsedTime();
}



int main( int argc, char *argv[] )
{
  VTKBenchmark a;
  if (argc > 1)
  {
    cerr << argv[0] << " takes no arguments\n";
  }
  return a.Run();
}

