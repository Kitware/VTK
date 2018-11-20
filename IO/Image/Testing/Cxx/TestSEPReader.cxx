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
//Load a SEP file, reads some information from the .H file and print them,
//then render the image. If the SEP file is not `small.H`, this test will fail.


#include "vtkSmartPointer.h"

#include "vtkSEPReader.h"

#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkImageMapToColors.h"
#include "vtkPolyDataMapper.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkActor.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkProperty.h"
#include "vtkLookupTable.h"


int TestSEPReader(int argc, char *argv[])
{
  if ( argc <= 1 )
  {
    cout << "Usage: " << argv[0] << " <SEP file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkNew<vtkSEPReader> SEPReader;

  // Check the image can be read
  if (!SEPReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  SEPReader->SetFileName(filename.c_str());
  SEPReader->Update();

  // Read and display the image properties
  int* extents= SEPReader->GetDataExtent();
  cout << "extent:  " << extents[0] << " - " << extents[1] << ", "
      << extents[2] <<" - "<< extents[3] << ", "
         << extents[4] << " - " << extents[5] << endl;
  if( extents[0] != 0 || extents[1] != 4
     || extents[2] != 0 || extents[3] != 4
     || extents[4] != 0 || extents[5] != 3
     )
  {
    return EXIT_FAILURE;
  }
  double* origin= SEPReader->GetDataOrigin();
  cout << "origin:  " << origin[0] << ", " << origin[1] << ", " << origin[2] << endl;
  if( origin[0] != 0
     || origin[1] != 0
     || origin[2] != 0
     )
  {
    return EXIT_FAILURE;
  }
  double* spacing= SEPReader->GetDataSpacing();
  cout << "spacing:  " << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << endl;
  if( spacing[0] != 1
     || spacing[1] != 1
     || spacing[2] != 1
     )
  {
    return EXIT_FAILURE;
  }

  // Visualize
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
  colors->PassAlphaToOutputOn();
  colors->SetOutputFormatToLuminanceAlpha();
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0,0,0);
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->SetSize(300,300);
  vtkNew<vtkDataSetSurfaceFilter> extract_surface;
  extract_surface->SetInputConnection(colors->GetOutputPort());
  extract_surface->SetNonlinearSubdivisionLevel(4);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(extract_surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("scalars");
  mapper->SetColorModeToMapScalars();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  actor->GetProperty()->SetEdgeVisibility(1);

  ren->ResetCamera();
  double z1 = ren->GetActiveCamera()->GetPosition()[2];
  double x2(0.25*z1), y2(0.25*z1), z2(0.5*z1);
  ren->GetActiveCamera()->SetPosition(x2,y2,z2);
  ren->GetActiveCamera()->SetFocalPoint(0,0,0);
  ren->ResetCamera();
  iren->Start();
  return EXIT_SUCCESS;
}
