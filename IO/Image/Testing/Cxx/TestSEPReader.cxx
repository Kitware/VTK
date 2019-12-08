/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSEPReader.cxx

  Copyright (c) GeometryFactory
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSEPReader
// .SECTION Description
// Load a SEP file, check the grid and render it.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSEPReader.h"
#include "vtkTestUtilities.h"

int TestSEPReader(int argc, char* argv[])
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/small.H");

  vtkNew<vtkSEPReader> SEPReader;

  // Check the image can be read
  if (!SEPReader->CanReadFile(filename))
  {
    std::cerr << "CanReadFile failed for " << filename << "\n";
    delete filename;
    return EXIT_FAILURE;
  }

  // Read the input image
  SEPReader->SetFileName(filename);
  delete filename;
  SEPReader->Update();

  // Check the image properties
  int* extents = SEPReader->GetDataExtent();
  if (extents[0] != 0 || extents[1] != 4 || extents[2] != 0 || extents[3] != 4 || extents[4] != 0 ||
    extents[5] != 3)
  {
    std::cerr << "Unexpected data extents!" << std::endl;
    return EXIT_FAILURE;
  }

  double* origin = SEPReader->GetDataOrigin();
  if (origin[0] != 0. || origin[1] != 0. || origin[2] != 0.)
  {
    std::cerr << "Unexpected data origin!" << std::endl;
    return EXIT_FAILURE;
  }

  double* spacing = SEPReader->GetDataSpacing();
  if (spacing[0] != 1. || spacing[1] != 1. || spacing[2] != 1.)
  {
    std::cerr << "Unexpected data spacing!" << std::endl;
    return EXIT_FAILURE;
  }

  // Visualize the grid
  double scalarRange[2];
  SEPReader->GetOutput()->GetScalarRange(scalarRange);

  vtkNew<vtkLookupTable> table;
  table->SetRampToLinear();
  table->SetRange(scalarRange[0], scalarRange[1]);
  table->SetValueRange(0.0, 1.0);
  table->SetSaturationRange(0.0, 0.0);
  table->SetAlphaRange(1.0, 1.0);
  table->Build();

  vtkNew<vtkImageMapToColors> colors;
  colors->SetInputConnection(SEPReader->GetOutputPort());
  colors->SetLookupTable(table);

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(colors->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("scalars");
  mapper->SetColorModeToMapScalars();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0, 0, 0);
  ren->AddActor(actor);
  ren->ResetCamera();
  double z1 = ren->GetActiveCamera()->GetPosition()[2];
  ren->GetActiveCamera()->SetPosition(0.25 * z1, 0.25 * z1, 0.5 * z1);
  ren->GetActiveCamera()->SetFocalPoint(0., 0., 0.);
  ren->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Start();

  return EXIT_SUCCESS;
}
