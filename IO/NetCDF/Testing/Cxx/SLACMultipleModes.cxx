// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SLACReaderLinear.cxx

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
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSLACReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"

int SLACMultipleModes(int argc, char *argv[])
{
  // Set up reader.
  vtkNew<vtkSLACReader> reader;

  char *meshFileName = vtkTestUtilities::ExpandDataFileName(
                         argc,
                         argv,
                         "Data/SLAC/pillbox/Pillbox3TenDSlice.ncdf");
  char *modeFileName0 =
      vtkTestUtilities::ExpandDataFileName(
        argc,
        argv,
        "Data/SLAC/pillbox/omega3p.l0.m0000.1.3138186e+09.mod");
  char *modeFileName1 =
      vtkTestUtilities::ExpandDataFileName(
        argc,
        argv,
        "Data/SLAC/pillbox/omega3p.l0.m0001.1.3138187e+09.mod");
  char *modeFileName2 =
      vtkTestUtilities::ExpandDataFileName(
        argc,
        argv,
        "Data/SLAC/pillbox/omega3p.l0.m0002.1.3138189e+09.mod");
  reader->SetMeshFileName(meshFileName);
  delete[] meshFileName;
  reader->AddModeFileName(modeFileName0);
  delete[] modeFileName0;
  reader->AddModeFileName(modeFileName1);
  delete[] modeFileName1;
  reader->AddModeFileName(modeFileName2);
  delete[] modeFileName2;

  reader->ReadInternalVolumeOff();
  reader->ReadExternalSurfaceOn();
  reader->ReadMidpointsOff();

  reader->UpdateInformation();
  double period =
      reader->GetExecutive()->GetOutputInformation(
        vtkSLACReader::SURFACE_OUTPUT)->Get(
        vtkStreamingDemandDrivenPipeline::TIME_RANGE())[1];

  reader->ResetPhaseShifts();
  reader->SetPhaseShift(1, 0.5*period);
  reader->SetPhaseShift(2, 0.5*period);

  reader->ResetFrequencyScales();
  reader->SetFrequencyScale(0, 0.75);
  reader->SetFrequencyScale(1, 1.5);

  // Extract geometry that we can render.
  vtkNew<vtkCompositeDataGeometryFilter> geometry;
  geometry->SetInputConnection(
        reader->GetOutputPort(vtkSLACReader::SURFACE_OUTPUT));

  // Set up rendering stuff.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ColorByArrayComponent("efield", 2);
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(-240, 240);

  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.66667, 0.0);
  mapper->SetLookupTable(lut.GetPointer());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.75, 0.0, 0.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  vtkNew<vtkRenderWindow> renwin;
  renwin->SetSize(600, 150);
  renwin->AddRenderer(renderer.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin.GetPointer());
  renwin->Render();

  // Change the time to offset the phase.
  vtkStreamingDemandDrivenPipeline *sdd =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(geometry->GetExecutive());
  sdd->SetUpdateTimeStep(0, 0.5*period);

  // Do the test comparison.
  int retVal = vtkRegressionTestImage(renwin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
