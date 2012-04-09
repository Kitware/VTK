/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ZsweepConcavities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2006 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkProjectedTetrahedraMapper.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGridVolumeZSweepMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkRegressionTestImage.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New();

int ZsweepConcavities(int argc, char *argv[])
{
  VTK_CREATE(vtkRTAnalyticSource, input);
  input->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  input->SetCenter(0.0, 0.0, 0.0);
  input->SetMaximum(255.0);
  input->SetXFreq(60.0);
  input->SetYFreq(30.0);
  input->SetZFreq(40.0);
  input->SetXMag(10.0);
  input->SetYMag(18.0);
  input->SetZMag(5.0);
  input->SetStandardDeviation(0.5);
  input->SetSubsampleRate(1);

  VTK_CREATE(vtkThreshold, threshold);
  threshold->SetInputConnection(input->GetOutputPort());
  threshold->ThresholdByLower(130.0);

  VTK_CREATE(vtkDataSetTriangleFilter, tetra);
  tetra->SetInputConnection(threshold->GetOutputPort());

  VTK_CREATE(vtkUnstructuredGridVolumeZSweepMapper, zsweep);
  zsweep->SetInputConnection(tetra->GetOutputPort());

  VTK_CREATE(vtkVolume, volume);
  volume->SetMapper(zsweep);

  VTK_CREATE(vtkColorTransferFunction, color);
  color->SetColorSpaceToHSV();
  color->HSVWrapOn();
  color->AddHSVPoint(0.0, 0.0, 0.0, 0.0);
  VTK_CREATE(vtkPiecewiseFunction, opacity);
  opacity->AddPoint(0.0, 0.25);
  volume->GetProperty()->SetColor(color);
  volume->GetProperty()->SetScalarOpacity(opacity);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddVolume(volume);
  renderer->SetBackground(1, 1, 1);

  renderer->ResetCamera();
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->Azimuth(40.0);
  camera->Elevation(40.0);

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(300, 300);
  renwin->AddRenderer(renderer);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  return (retVal != vtkRegressionTester::PASSED);
}
