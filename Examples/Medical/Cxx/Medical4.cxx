/*=========================================================================

Program:   Visualization Toolkit
Module:    Medical4.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//
// This example reads a volume dataset and displays it via volume rendering.
//

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume16Reader.h>
#include <vtkVolume.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkCamera.h>

int main (int argc, char *argv[])
{
  if (argc < 2)
  {
    cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return EXIT_FAILURE;
  }

  // Create the renderer, the render window, and the interactor. The renderer
  // draws into the render window, the interactor enables mouse- and
  // keyboard-based interaction with the scene.
  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The following reader is used to read a series of 2D slices (images)
  // that compose the volume. The slice dimensions are set, and the
  // pixel spacing. The data Endianness must also be specified. The reader
  // uses the FilePrefix in combination with the slice number to construct
  // filenames using the format FilePrefix.%d. (In this case the FilePrefix
  // is the root name of the file: quarter.)
  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetImageRange(1, 93);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetFilePrefix(argv[1]);
  v16->SetDataSpacing(3.2, 3.2, 1.5);

  // The volume will be displayed by ray-cast alpha compositing.
  // A ray-cast mapper is needed to do the ray-casting.
  vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper =
    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
  volumeMapper->SetInputConnection(v16->GetOutputPort());

  // The color transfer function maps voxel intensities to colors.
  // It is modality-specific, and often anatomy-specific as well.
  // The goal is to one color for flesh (between 500 and 1000)
  // and another color for bone (1150 and over).
  vtkSmartPointer<vtkColorTransferFunction>volumeColor =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  volumeColor->AddRGBPoint(0,    0.0, 0.0, 0.0);
  volumeColor->AddRGBPoint(500,  1.0, 0.5, 0.3);
  volumeColor->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  volumeColor->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  // The opacity transfer function is used to control the opacity
  // of different tissue types.
  vtkSmartPointer<vtkPiecewiseFunction> volumeScalarOpacity =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  volumeScalarOpacity->AddPoint(0,    0.00);
  volumeScalarOpacity->AddPoint(500,  0.15);
  volumeScalarOpacity->AddPoint(1000, 0.15);
  volumeScalarOpacity->AddPoint(1150, 0.85);

  // The gradient opacity function is used to decrease the opacity
  // in the "flat" regions of the volume while maintaining the opacity
  // at the boundaries between tissue types.  The gradient is measured
  // as the amount by which the intensity changes over unit distance.
  // For most medical data, the unit distance is 1mm.
  vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  volumeGradientOpacity->AddPoint(0,   0.0);
  volumeGradientOpacity->AddPoint(90,  0.5);
  volumeGradientOpacity->AddPoint(100, 1.0);

  // The VolumeProperty attaches the color and opacity functions to the
  // volume, and sets other volume properties.  The interpolation should
  // be set to linear to do a high-quality rendering.  The ShadeOn option
  // turns on directional lighting, which will usually enhance the
  // appearance of the volume and make it look more "3D".  However,
  // the quality of the shading depends on how accurately the gradient
  // of the volume can be calculated, and for noisy data the gradient
  // estimation will be very poor.  The impact of the shading can be
  // decreased by increasing the Ambient coefficient while decreasing
  // the Diffuse and Specular coefficient.  To increase the impact
  // of shading, decrease the Ambient and increase the Diffuse and Specular.
  vtkSmartPointer<vtkVolumeProperty> volumeProperty =
    vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor(volumeColor);
  volumeProperty->SetScalarOpacity(volumeScalarOpacity);
  volumeProperty->SetGradientOpacity(volumeGradientOpacity);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOn();
  volumeProperty->SetAmbient(0.4);
  volumeProperty->SetDiffuse(0.6);
  volumeProperty->SetSpecular(0.2);

  // The vtkVolume is a vtkProp3D (like a vtkActor) and controls the position
  // and orientation of the volume in world coordinates.
  vtkSmartPointer<vtkVolume> volume =
    vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Finally, add the volume to the renderer
  ren->AddViewProp(volume);

  // Set up an initial view of the volume.  The focal point will be the
  // center of the volume, and the camera position will be 400mm to the
  // patient's left (which is our right).
  vtkCamera *camera = ren->GetActiveCamera();
  double *c = volume->GetCenter();
  camera->SetFocalPoint(c[0], c[1], c[2]);
  camera->SetPosition(c[0] + 400, c[1], c[2]);
  camera->SetViewUp(0, 0, -1);

  // Increase the size of the render window
  renWin->SetSize(640, 480);

  // Interact with the data.
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
