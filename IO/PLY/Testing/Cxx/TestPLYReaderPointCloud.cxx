/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReaderPointCloud.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkPLYReader
// .SECTION Description
//

#include "vtkPLYReader.h"
#include "vtkSmartPointer.h"

#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>

#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestPLYReaderPointCloud( int argc, char *argv[] )
{
  // Read file name.
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/PointCloud.ply");

  // Create the reader.
  vtkSmartPointer<vtkPLYReader> reader =
    vtkSmartPointer<vtkPLYReader>::New();
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;

  // Create a mapper.
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Guess at a decent radius
  double bounds[6];
  reader->GetOutput()->GetBounds(bounds);

  double range[3];
  for (int i = 0; i < 3; ++i)
  {
    range[i] = bounds[2*i + 1] - bounds[2*i];
  }
  double radius = range[0] * .05;

  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetRadius(radius);

  vtkSmartPointer<vtkGlyph3D> glyph3D =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph3D->SetInputConnection(reader->GetOutputPort());
  glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
  glyph3D->ScalingOff();
  glyph3D->SetColorModeToColorByScalar();
  glyph3D->Update();

  vtkSmartPointer<vtkPolyDataMapper> glyph3DMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  glyph3DMapper->SetInputConnection(glyph3D->GetOutputPort());

  vtkSmartPointer<vtkActor> glyph3DActor =
    vtkSmartPointer<vtkActor>::New();
  glyph3DActor->SetMapper(glyph3DMapper);

  // Basic visualization.
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(ren);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->AddActor(glyph3DActor);
  ren->SetBackground(.4, .5, .7);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
