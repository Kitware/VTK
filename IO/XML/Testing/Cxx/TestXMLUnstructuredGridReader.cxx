/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLUnstructuredGridReader.h"

#include "vtkNew.h"
#include <string>

int TestXMLUnstructuredGridReader(int argc, char *argv[])
{
  int i;
  // Need to get the data root.
  const char *data_root = NULL;
  for (i = 0; i < argc-1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i+1];
      break;
    }
  }
  if (!data_root)
  {
    cout << "Need to specify the directory to VTK_DATA_ROOT with -D <dir>." << endl;
    return 1;
  }

  // Test that the right number of time steps can be read from a .vtu file.
  std::string filename;
  filename = data_root;
  filename += "/Data/many_time_steps.vtu";
  cout << "Loading " << filename.c_str() << endl;
  vtkNew<vtkXMLUnstructuredGridReader> reader1;
  reader1->SetFileName(filename.c_str());
  reader1->Update();

  int tsResult = 0;
  if (reader1->GetNumberOfTimeSteps() != 4100)
  {
    std::cerr << "Expected to read 4100 timesteps, got "
      << reader1->GetNumberOfTimeSteps() << " instead." << std::endl;
    tsResult = 1;
  }

  // Create the reader for the data (.vtu) with multiple pieces,
  // and each piece contains a pyramid cell and a polyhedron cell.
  filename = data_root;
  filename += "/Data/polyhedron2pieces.vtu";
  cout << "Loading " << filename.c_str() << endl;
  vtkNew<vtkXMLUnstructuredGridReader> reader2;
  reader2->SetFileName(filename.c_str());

  vtkNew<vtkDataSetSurfaceFilter> surfaces;
  surfaces->SetInputConnection(reader2->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaces->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());
  renderer->SetBackground(0,0,0);

  vtkNew<vtkRenderWindow> renwin;
  renwin->SetMultiSamples(0);
  renwin->AddRenderer(renderer.GetPointer());
  renwin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin.GetPointer());
  iren->Initialize();

  renderer->ResetCamera();
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->Elevation(-90.0);
  camera->SetViewUp(0.0, 0.0, 1.0);
  camera->Azimuth(125.0);

  // interact with data
  renwin->Render();

  int rtResult = vtkRegressionTestImage( renwin.GetPointer() );
  if ( rtResult == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return tsResult + !rtResult;
}
