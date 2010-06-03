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
#include "vtkOpenGLProjectedTetrahedraMapper.h"
#include "vtkOpenGLProjectedAAHexahedraMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSplitField.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumeProperty.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestProjectedHexahedra(int argc, char* argv[])
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

  // Create the reader for the data
  vtkStdString filename;
  VTK_CREATE(vtkUnstructuredGridReader, reader);
  reader->SetFileName("/home/marchesi/VTKData/Data/hexa.vtk");

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
  VTK_CREATE(vtkOpenGLProjectedAAHexahedraMapper, volumeMapperHex);
  volumeMapperHex->SetInputConnection(reader->GetOutputPort());

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume
  vtkVolume* volume = vtkVolume::New();
#if 1
  volume->SetMapper(volumeMapperHex);
#else
  volume->SetMapper(volumeMapperTet);
#endif
  volume->SetProperty(volumeProperty);

  ren1->AddVolume(volume);
    
  while(true)
    {
    // render the image
    renWin->Render();
    // rotate the active camera by one degree
    ren1->GetActiveCamera()->Azimuth( 0.1 );
    }
  
  //
  // Free up any objects we created. All instances in VTK are deleted by
  // using the Delete() method.
  //
  ren1->Delete();
  renWin->Delete();

  return 0;
}


