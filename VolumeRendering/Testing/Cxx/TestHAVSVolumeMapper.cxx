/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHAVSVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkHAVSVolumeMapper.h"

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
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

int TestHAVSVolumeMapper(int argc, char *argv[])
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
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  vtkStdString filename;
  filename = data_root;
  filename += "/Data/ironProt.vtk";

  vtkStructuredPointsReader *reader = vtkStructuredPointsReader::New();
  reader->SetFileName(filename.c_str());

  // Create a reader for the other data that will be contoured and
  // displayed as a polygonal mesh.
  filename = data_root;
  filename += "/Data/neghip.slc";

  vtkSLCReader *reader2 = vtkSLCReader::New();
  reader2->SetFileName(filename.c_str());

  // Convert from vtkImageData to vtkUnstructuredGrid.
  // Remove any cells where all values are below 80.
  vtkThreshold *thresh = vtkThreshold::New();
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(reader->GetOutputPort());

  // Make sure we have only tetrahedra.
  vtkDataSetTriangleFilter *trifilter = vtkDataSetTriangleFilter::New();
  trifilter->SetInputConnection(thresh->GetOutputPort());

  // Create transfer mapping scalar value to opacity.
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint(80.0,  0.0);
  opacityTransferFunction->AddPoint(120.0, 0.2);
  opacityTransferFunction->AddPoint(255.0, 0.2);

  // Create transfer mapping scalar value to color.
  vtkColorTransferFunction *colorTransferFunction
    = vtkColorTransferFunction::New();
  colorTransferFunction->AddRGBPoint(80.0,  0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look.
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  vtkHAVSVolumeMapper *volumeMapper
    = vtkHAVSVolumeMapper::New();
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());
  volumeMapper->SetLevelOfDetail(false);
  volumeMapper->SetGPUDataStructures(false);
  volumeMapper->SetKBufferSizeTo2();

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  vtkVolume *volume = vtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Contour the second dataset.
  vtkContourFilter *contour = vtkContourFilter::New();
  contour->SetValue(0, 80);
  contour->SetInputConnection(reader2->GetOutputPort());

  // Create a mapper for the polygonal data.
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Create an actor for the polygonal data.
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  // First test if mapper is supported
  renWin->SetSize(300, 300);
  renWin->Render();

  int supported = volumeMapper->SupportedByHardware(ren1);

  vtkTextActor *textActor = vtkTextActor::New();
  textActor->SetInput("Required Extensions\nNot Supported");
  textActor->SetDisplayPosition( 150, 150 );
  textActor->GetTextProperty()->SetJustificationToCentered();
  textActor->GetTextProperty()->SetFontSize(30);
  
  
  if ( !supported )
    {
    ren1->AddViewProp(textActor);
    }
  else
    {
    ren1->AddViewProp(actor);
    ren1->AddViewProp(volume);
    }
  

  ren1->ResetCamera();
  ren1->GetActiveCamera()->Azimuth(20.0);
  ren1->GetActiveCamera()->Elevation(10.0);
  ren1->GetActiveCamera()->Zoom(1.5);

  // Test default settings
  renWin->Render();

  // Test kbuffer size 6
  volumeMapper->SetKBufferSizeTo6();
  renWin->Render();

  // Test GPU Data structures
  volumeMapper->SetGPUDataStructures(true);
  renWin->Render();

  // Test Field Level of Detail
  volumeMapper->SetLevelOfDetail(true);
  volumeMapper->SetLevelOfDetailMethodField();
  renWin->Render();

  // Test Area Level of Detail
  volumeMapper->SetLevelOfDetailMethodArea();
  renWin->Render();

  // Return to default KBuffer size and Level of Detail
  volumeMapper->SetLevelOfDetail(false);
  volumeMapper->SetKBufferSizeTo2();
  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up.
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  reader2->Delete();
  thresh->Delete();
  trifilter->Delete();
  opacityTransferFunction->Delete();
  colorTransferFunction->Delete();
  volumeProperty->Delete();
  volumeMapper->Delete();
  volume->Delete();
  contour->Delete();
  mapper->Delete();
  actor->Delete();
  textActor->Delete();
  

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
