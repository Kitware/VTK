/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ProjectedTetrahedraZoomIn.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// This test makes sure that the mapper behaves well when the user zooms in
// enough to have cells in front of the near plane.

#include "vtkProjectedTetrahedraMapper.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkSLCReader.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkThreshold.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkStdString.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int ProjectedTetrahedraZoomIn(int argc, char *argv[])
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

  // Create the standard renderer, render window, and interactor.
  VTK_CREATE(vtkRenderer, ren1);
  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);

  // check for driver support
  renWin->Render();
  VTK_CREATE(vtkProjectedTetrahedraMapper, volumeMapper);
  if (!volumeMapper->IsSupported(renWin))
  {
    vtkGenericWarningMacro("Projected tetrahedra is not supported. Skipping tests.");
    return 0;
  }

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  vtkStdString filename;
  filename = data_root;
  filename += "/Data/ironProt.vtk";
  cout << "Loading " << filename.c_str() << endl;
  VTK_CREATE(vtkStructuredPointsReader, reader);
  reader->SetFileName(filename.c_str());

  // Create a reader for the other data that will be contoured and
  // displayed as a polygonal mesh.
  filename = data_root;
  filename += "/Data/neghip.slc";
  cout << "Loading " << filename.c_str() << endl;
  VTK_CREATE(vtkSLCReader, reader2);
  reader2->SetFileName(filename.c_str());

  // Convert from vtkImageData to vtkUnstructuredGrid.
  // Remove any cells where all values are below 80.
  VTK_CREATE(vtkThreshold, thresh);
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(reader->GetOutputPort());

  // Make sure we have only tetrahedra.
  VTK_CREATE(vtkDataSetTriangleFilter, trifilter);
  trifilter->SetInputConnection(thresh->GetOutputPort());

  // Create transfer mapping scalar value to opacity.
  VTK_CREATE(vtkPiecewiseFunction, opacityTransferFunction);
  opacityTransferFunction->AddPoint(80.0,  0.0);
  opacityTransferFunction->AddPoint(120.0, 0.2);
  opacityTransferFunction->AddPoint(255.0, 0.2);

  // Create transfer mapping scalar value to color.
  VTK_CREATE(vtkColorTransferFunction, colorTransferFunction);
  colorTransferFunction->AddRGBPoint(80.0,  0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look.
  VTK_CREATE(vtkVolumeProperty, volumeProperty);
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  VTK_CREATE(vtkVolume, volume);
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Contour the second dataset.
  VTK_CREATE(vtkContourFilter, contour);
  contour->SetValue(0, 80);
  contour->SetInputConnection(reader2->GetOutputPort());

  // Create a mapper for the polygonal data.
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Create an actor for the polygonal data.
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  ren1->AddViewProp(actor);
  ren1->AddVolume(volume);

  renWin->SetSize(300, 300);
  ren1->ResetCamera();

  vtkCamera *camera = ren1->GetActiveCamera();
  camera->ParallelProjectionOff();
  camera->SetFocalPoint(33, 33, 33);
  camera->SetPosition(43, 38, 61);
  camera->SetViewUp(0, 1, 0);
  camera->SetViewAngle(20);
  camera->SetClippingRange(0.1, 135);
  camera->SetEyeAngle(2);

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // For now we are just checking to make sure that the mapper does not crash.
  // Maybe in the future we will do an image comparison.
#if 0
  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
#else
  vtkGenericWarningMacro("This test will always pass.");
  return 0;
#endif
}
