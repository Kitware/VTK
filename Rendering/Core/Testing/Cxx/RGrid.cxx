/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

int RGrid( int argc, char *argv[] )
{
  int i;
  static float x[47]={-1.22396f, -1.17188f, -1.11979f, -1.06771f, -1.01562f, -0.963542f,
                      -0.911458f, -0.859375f, -0.807292f, -0.755208f, -0.703125f, -0.651042f,
                      -0.598958f, -0.546875f, -0.494792f, -0.442708f, -0.390625f, -0.338542f,
                      -0.286458f, -0.234375f, -0.182292f, -0.130209f, -0.078125f, -0.026042f,
                       0.0260415f, 0.078125f, 0.130208f, 0.182291f, 0.234375f, 0.286458f,
                       0.338542f, 0.390625f, 0.442708f, 0.494792f, 0.546875f, 0.598958f,
                       0.651042f, 0.703125f, 0.755208f, 0.807292f, 0.859375f, 0.911458f,
                       0.963542f, 1.01562f, 1.06771f, 1.11979f, 1.17188f};
  static float y[33]={-1.25f, -1.17188f, -1.09375f, -1.01562f, -0.9375f, -0.859375f,
                      -0.78125f, -0.703125f, -0.625f, -0.546875f, -0.46875f, -0.390625f,
                      -0.3125f, -0.234375f, -0.15625f, -0.078125f, 0.0f, 0.078125f,
                       0.15625f, 0.234375f, 0.3125f, 0.390625f, 0.46875f, 0.546875f,
                       0.625f, 0.703125f, 0.78125f, 0.859375f, 0.9375f, 1.01562f,
                       1.09375f, 1.17188f, 1.25f};
  static float z[44]={0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
                      0.6f, 0.7f, 0.75f, 0.8f, 0.9f, 1.0f,
                      1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f,
                      1.7f, 1.75f, 1.8f, 1.9f, 2.0f, 2.1f,
                      2.2f, 2.3f, 2.4f, 2.5f, 2.6f, 2.7f,
                      2.75f, 2.8f, 2.9f, 3.0f, 3.1f, 3.2f,
                      3.3f, 3.4f, 3.5f, 3.6f, 3.7f, 3.75f,
                      3.8f, 3.9f};

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

#ifdef VTK_OPENGL1
  if (strcmp(renWin->GetRenderingBackend(),"OpenGL1"))
  {
    return vtkTesting::FAILED;
  }
#endif

#ifdef VTK_OPENGL2
  if (strcmp(renWin->GetRenderingBackend(),"OpenGL2"))
  {
    return vtkTesting::FAILED;
  }
#endif

  vtkFloatArray *xCoords = vtkFloatArray::New();
  for (i=0; i<47; i++) xCoords->InsertNextValue(x[i]);

  vtkFloatArray *yCoords = vtkFloatArray::New();
  for (i=0; i<33; i++) yCoords->InsertNextValue(y[i]);

  vtkFloatArray *zCoords = vtkFloatArray::New();
  for (i=0; i<44; i++) zCoords->InsertNextValue(z[i]);

  vtkRectilinearGrid *rgrid = vtkRectilinearGrid::New();
    rgrid->SetDimensions(47,33,44);
    rgrid->SetXCoordinates(xCoords);
    rgrid->SetYCoordinates(yCoords);
    rgrid->SetZCoordinates(zCoords);

  vtkRectilinearGridGeometryFilter *plane = vtkRectilinearGridGeometryFilter::New();
    plane->SetInputData(rgrid);
    plane->SetExtent(0,46, 16,16, 0,43);

  vtkPolyDataMapper *rgridMapper = vtkPolyDataMapper::New();
      rgridMapper->SetInputConnection(plane->GetOutputPort());

  vtkActor *wireActor = vtkActor::New();
      wireActor->SetMapper(rgridMapper);
      wireActor->GetProperty()->SetRepresentationToWireframe();
      wireActor->GetProperty()->SetColor(0,0,0);

  renderer->AddActor(wireActor);
  renderer->SetBackground(1,1,1);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(60.0);
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(1.0);

  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  xCoords->Delete();
  yCoords->Delete();
  zCoords->Delete();
  rgrid->Delete();
  rgridMapper->Delete();
  plane->Delete();
  wireActor->Delete();

  return !retVal;

}
