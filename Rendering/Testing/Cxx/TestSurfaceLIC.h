/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfaceLIC.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __TestSurfaceLIC_h
#define __TestSurfaceLIC_h

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkSurfaceLICPainter.h"
#include "vtkObjectFactory.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>
#include <vector>
#include <string>

#define VTK_CREATE_NEW(var, class) vtkSmartPointer<class> var = vtkSmartPointer<class>::New();

// This example demonstrates the use of vtkSurfaceLICPainter for rendering
// geometry with LIC on the surface.

enum { SURFACE_LIC_DEMO = 0, SURFACE_LIC_TEST = 1 };

int RenderingMode = SURFACE_LIC_TEST;

int SurfaceLIC( int argc, char * argv[] )
{
  std::string filename;
  int num_steps = 40;
  double step_size = 0.4;
  double lic_intensity = 0.8;
  //std::string color_by;
  std::string vectors;

  vtksys::CommandLineArguments arg;
  arg.StoreUnusedArguments(1);
  arg.Initialize(argc, argv);

  // Fill up accepted arguments.
  typedef vtksys::CommandLineArguments argT;

  arg.AddArgument("--data", argT::EQUAL_ARGUMENT, &filename,
    "(required) Enter dataset to load (currently only *.[vtk|vtp] files are supported");
  arg.AddArgument("--num-steps", argT::EQUAL_ARGUMENT, &num_steps,
    "(optional: default 40) Number of steps in each direction");
  arg.AddArgument("--step-size", argT::EQUAL_ARGUMENT, &step_size,
    "(optional: default 0.4) Step size in pixels");
  arg.AddArgument("--lic-intensity", argT::EQUAL_ARGUMENT, &lic_intensity,
    "(optional: default 0.8) Contribution of LIC in the final image [1.0 == max contribution]");
  //arg.AddArgument("--color-by", argT::EQUAL_ARGUMENT, &color_by,
  //  "(optional: default active scalars) Name of the array to color by");
  arg.AddArgument("--vectors", argT::EQUAL_ARGUMENT, &vectors,
    "(optional: default active point vectors) Name of the vector field array");

  if (!arg.Parse() || filename == "")
    {
    cerr << "Usage: " << endl;
    cerr << arg.GetHelp() << endl;
    return 1;
    }

  vtkSmartPointer<vtkPolyData> polydata;
  std::string ext = vtksys::SystemTools::GetFilenameExtension(filename);
  if (ext == ".vtk")
    {
    vtkGenericDataObjectReader* reader = vtkGenericDataObjectReader::New();
    reader->SetFileName(filename.c_str());
    
    vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
    surface->SetInputConnection(reader->GetOutputPort());
    surface->Update();

    polydata = surface->GetOutput();

    reader->Delete();
    surface->Delete();
    }
  else if (ext == ".vtp")
    {
    vtkXMLPolyDataReader* reader = vtkXMLPolyDataReader::New();
    reader->SetFileName(filename.c_str());
    reader->Update();
    polydata = reader->GetOutput();
    reader->Delete();
    }
  else
    {
    cerr << "Error: Unknown extension: '" << ext << "'"<< endl;
    return 1;
    }

  if (!polydata || polydata->GetNumberOfPoints() == 0)
    {
    cerr << "Error reading file: '" << filename.c_str() << "'" << endl;
    return 1;
    }

  // Set up the render window, renderer, interactor.
  VTK_CREATE_NEW(renWin, vtkRenderWindow);
  VTK_CREATE_NEW(renderer, vtkRenderer);
  VTK_CREATE_NEW(iren, vtkRenderWindowInteractor);
  renWin->SetReportGraphicErrors(1);
  renWin->AddRenderer(renderer);
  renWin->SetSize(300,300);
  iren->SetRenderWindow(renWin);
  renWin->Render();
  if (!vtkSurfaceLICPainter::IsSupported(renWin))
    {
    cout << "WARNING: The rendering context does not support required "
      "extensions." << endl;
    return 0;
    }

  // Create a mapper and insert the vtkSurfaceLICPainter painter into the
  // painter chain. This is essential since the entire logic of performin the
  // LIC is present in the vtkSurfaceLICPainter.
  VTK_CREATE_NEW(mapper, vtkPainterPolyDataMapper);
  VTK_CREATE_NEW(painter, vtkSurfaceLICPainter);
  painter->SetDelegatePainter(mapper->GetPainter());
  mapper->SetPainter(painter);

  // If user chose a vector field, select it.
  if (vectors != "")
    {
    painter->SetInputArrayToProcess(
      vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
      vectors.c_str());
    }
  else if (!polydata->GetPointData()->GetVectors() &&
    !polydata->GetCellData()->GetVectors())
    {
    cerr << "ERROR: No active vectors are available." << endl<<
            "       Please select the vectors array using '--vectors'" << endl;
    return 1;
    }

  // Pass parameters.
  painter->SetLICIntensity(lic_intensity);
  painter->SetNumberOfSteps(num_steps);
  painter->SetStepSize(step_size);

  // Set the mapper input
  mapper->SetInput(polydata);

  VTK_CREATE_NEW(actor, vtkActor);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->SetBackground(0.3, 0.3, 0.3);

  if ( RenderingMode )
    {
    // Code used for regression testing.
    renderer->GetActiveCamera()->SetFocalPoint(-1.88, -0.98, -1.04);
    renderer->GetActiveCamera()->SetPosition(13.64, 4.27, -31.59);
    renderer->GetActiveCamera()->SetViewAngle(30);
    renderer->GetActiveCamera()->SetViewUp(0.41, 0.83, 0.35);
    renderer->ResetCamera();
    renWin->Render();
    if (  painter->GetLICSuccess() == 0 ||
          painter->GetRenderingPreparationSuccess() == 0 )
      {
      return 0;
      }

    int retVal = vtkTesting::Test(argc, argv, renWin, 75);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }

    if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
      {
      return 0;
      }
    // failed.
    return 1;
    }
  else
    {
    renderer->ResetCamera();
    renWin->Render();
    if (  painter->GetLICSuccess() == 0 ||
          painter->GetRenderingPreparationSuccess() == 0 )
      {
      return 0;
      }
    iren->Start();
    }
  // failed.
  return 0;
}

#endif
