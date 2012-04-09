/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: Cone.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example creates a polygonal model of a cone, and then renders it to
// the screen. It will rotate the cone 360 degrees and then exit. The basic
// setup of source -> mapper -> actor -> renderer -> renderwindow is
// typical of most VTK programs.
//

// First include the required header files for the VTK classes we are using.
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellDataToPointData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkConeSource.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkMaskFields.h"
#include "vtkMergeFields.h"
#include "vtkProjectedAAHexahedraMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSplitField.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumeProperty.h"
#include "vtkProjectedTetrahedraMapper.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestProjectedHexahedra(int argc,
                           char *argv[])
{
  VTK_CREATE(vtkRenderer, ren1);
  ren1->SetBackground( 0.0, 0.0, 0.0 );

  //
  // Finally we create the render window which will show up on the screen.
  // We put our renderer into the render window using AddRenderer. We also
  // set the size to be 300 pixels by 300.
  //
  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer( ren1 );
  renWin->SetSize( 800, 800 );

  vtkSmartPointer<vtkRenderWindowInteractor> iren=
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Make sure we have a OpenGL context created before checking that
  // the volume mapper is supported by OpenGL.
  renWin->Render();

  // Create the reader for the data
  VTK_CREATE(vtkUnstructuredGridReader, reader);

  char *cfname=vtkTestUtilities::ExpandDataFileName(argc,argv,"Data/hexa.vtk");
  reader->SetFileName(cfname);
  delete [] cfname;

  // Create transfer mapping scalar value to opacity
  VTK_CREATE(vtkPiecewiseFunction, opacityTransferFunction);
  opacityTransferFunction->AddPoint(0.0, 0.0);

  opacityTransferFunction->AddPoint(8.0, 2.0);

  opacityTransferFunction->AddPoint(10.0, 1.5);

  opacityTransferFunction->AddPoint(13.0, 1.0);

  // Create transfer mapping scalar value to color
  VTK_CREATE(vtkColorTransferFunction, colorTransferFunction);
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);

  colorTransferFunction->AddRGBPoint(8.0, 1.0, 0.0, 0.0);

  colorTransferFunction->AddRGBPoint(10.0, 0.0, 0.0, 1.0);

  colorTransferFunction->AddRGBPoint(12.0, 0.0, 1.0, 0.0);

  // The property describes how the data will look
  VTK_CREATE(vtkVolumeProperty, volumeProperty);
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);

  // Make sure we have only tetrahedra.
  VTK_CREATE(vtkDataSetTriangleFilter, trifilter);
  trifilter->SetInputConnection(reader->GetOutputPort());

  // The mapper knows how to render the data
  VTK_CREATE(vtkProjectedTetrahedraMapper, volumeMapperTet);
  volumeMapperTet->SetInputConnection(trifilter->GetOutputPort());

  // The mapper knows how to render the data
  VTK_CREATE(vtkProjectedAAHexahedraMapper, volumeMapperHex);
  volumeMapperHex->SetInputConnection(reader->GetOutputPort());

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume
  VTK_CREATE(vtkVolume,volume);
#if 1
  volume->SetMapper(volumeMapperHex);
#else
  volume->SetMapper(volumeMapperTet);
#endif
  volume->SetProperty(volumeProperty);

  int valid=volumeMapperHex->IsRenderSupported(renWin);

  int retVal;
  if(valid)
    {
    iren->Initialize();
    ren1->AddVolume(volume);
    ren1->ResetCamera();
    renWin->Render();

    retVal = vtkTesting::Test(argc, argv, renWin, 75);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }
    }
  else
    {
    retVal=vtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
    }

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}


