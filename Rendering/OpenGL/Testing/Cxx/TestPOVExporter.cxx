/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPOVExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestPOVExpoter - Tests vtkPOVExporter.
// .SECTION Description
// Creates a scene and uses POVExporter to generate a pov file. Test passes
// if the file exists and has non zero length.

#include "vtkSphereSource.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkLODActor.h"
#include "vtkTextMapper.h"
#include "vtkProperty.h"
#include "vtkActor2D.h"
#include "vtkCellPicker.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkCamera.h"
#include "vtkPOVExporter.h"
#include "vtksys/SystemTools.hxx"

int TestPOVExporter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int err = 0;
  int exists = 0;
  unsigned long length = 0;

  // create a sphere source, mapper, and actor
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereMapper->GlobalImmediateModeRenderingOn();
  vtkActor *sphereActor = vtkLODActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereActor->GetProperty()->SetDiffuseColor(0.8900, 0.8100, 0.3400);
  sphereActor->GetProperty()->SetSpecular(0.4);
  sphereActor->GetProperty()->SetSpecularPower(20);

  // create the spikes by glyphing the sphere with a cone.  Create the
  // mapper and actor for the glyphs.
  vtkConeSource *cone = vtkConeSource::New();
  cone->SetResolution(20);
  vtkGlyph3D *glyph = vtkGlyph3D::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  vtkPolyDataMapper *spikeMapper = vtkPolyDataMapper::New();
  spikeMapper->SetInputConnection(glyph->GetOutputPort());
  vtkLODActor *spikeActor = vtkLODActor::New();
  spikeActor->SetMapper(spikeMapper);
  spikeActor->GetProperty()->SetDiffuseColor(1.0000, 0.3882, 0.2784);
  spikeActor->GetProperty()->SetSpecular(0.4);
  spikeActor->GetProperty()->SetSpecularPower(20);

  // Create the Renderer, RenderWindow, etc. and set the Picker.
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);

  ren->AddActor(sphereActor);
  ren->AddActor(spikeActor);
  ren->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300,300);

  ren->ResetCamera();
  vtkCamera *cam1 = ren->GetActiveCamera();
  cam1->Zoom(1.4);

  renWin->Render();

  // Instead of letting renderer to render the scene, we use
  // an exportor to save it to a file.
  vtkPOVExporter *povexp = vtkPOVExporter::New();
  povexp->SetRenderWindow(renWin);
  povexp->SetFileName("TestPOVExporter.pov");
  cout << "Writing file TestPOVExporter.pov..." << endl;

  povexp->Write();
  cout << "Done writing file TestPOVExporter.pov..." << endl;

  povexp->Delete();

  exists =
    static_cast<int>(vtksys::SystemTools::FileExists("TestPOVExporter.pov"));
  length = vtksys::SystemTools::FileLength("TestPOVExporter.pov");
  cout << "TestPOVExporter.pov file exists: " << exists << endl;
  cout << "TestPOVExporter.pov file length: " << length << endl;
  if (!exists)
    {
    err = 1;
    cerr << "ERROR: 1 - Test failing because TestPOVExporter.pov file doesn't exist..." << endl;
    }
  else
    {
    vtksys::SystemTools::RemoveFile("TestPOVExporter.pov");
    }

  if (0==length)
    {
    err = 2;
    cerr << "ERROR: 2 - Test failing because TestPOVExporter.pov file has zero length..." << endl;
    }

  renWin->Delete();
  ren->Delete();
  spikeActor->Delete();
  spikeMapper->Delete();
  glyph->Delete();
  cone->Delete();
  sphereActor->Delete();
  sphereMapper->Delete();
  sphere->Delete();

  return err;
}
