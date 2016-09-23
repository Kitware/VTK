// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SLACParticleReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSLACParticleReader.h"
#include "vtkSLACReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <sstream>

int SLACParticleReader(int argc, char *argv[])
{
  char *meshFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                             "Data/SLAC/pic-example/mesh.ncdf");
  char *modeFileNamePattern = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                         "Data/SLAC/pic-example/fields_%d.mod");
  char *particleFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                      "Data/SLAC/pic-example/particles_5.ncdf");

  // Set up mesh reader.
  VTK_CREATE(vtkSLACReader, meshReader);
  meshReader->SetMeshFileName(meshFileName);
  delete[] meshFileName;

  char *modeFileName = new char[strlen(modeFileNamePattern) + 10];
  for (int i = 0; i < 9; i++)
  {
    sprintf(modeFileName, modeFileNamePattern, i);
    meshReader->AddModeFileName(modeFileName);
  }
  delete[] modeFileName;
  delete[] modeFileNamePattern;

  meshReader->ReadInternalVolumeOn();
  meshReader->ReadExternalSurfaceOff();
  meshReader->ReadMidpointsOff();

  // Extract geometry that we can render.
  VTK_CREATE(vtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(
                       meshReader->GetOutputPort(vtkSLACReader::VOLUME_OUTPUT));

  // Set up particle reader.
  VTK_CREATE(vtkSLACParticleReader, particleReader);
  particleReader->SetFileName(particleFileName);
  delete[] particleFileName;

  // Set up rendering stuff.
  VTK_CREATE(vtkPolyDataMapper, meshMapper);
  meshMapper->SetInputConnection(geometry->GetOutputPort());
  meshMapper->SetScalarModeToUsePointFieldData();
  meshMapper->ColorByArrayComponent("efield", 2);
  meshMapper->UseLookupTableScalarRangeOff();
  meshMapper->SetScalarRange(1.0, 1e+05);

  VTK_CREATE(vtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  lut->SetScaleToLog10();
  meshMapper->SetLookupTable(lut);

  VTK_CREATE(vtkActor, meshActor);
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->FrontfaceCullingOn();

  VTK_CREATE(vtkPolyDataMapper, particleMapper);
  particleMapper->SetInputConnection(particleReader->GetOutputPort());
  particleMapper->ScalarVisibilityOff();

  VTK_CREATE(vtkActor, particleActor);
  particleActor->SetMapper(particleMapper);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(meshActor);
  renderer->AddActor(particleActor);
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.2, 0.05, 0.0);
  camera->SetFocalPoint(0.0, 0.05, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(300, 200);
  renwin->AddRenderer(renderer);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();

  double time
    = particleReader->GetOutput()->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  cout << "Time in particle reader: " << time << endl;

  // Change the time to test the time step field load and to have the field
  // match the particles in time.
  geometry->UpdateInformation();
  geometry->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
    time);
  renwin->Render();

  // Do the test comparison.
  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
