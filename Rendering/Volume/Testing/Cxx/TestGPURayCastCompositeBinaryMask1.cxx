/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCompositeBinaryMask1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test masks a rectangular volume to a cylindrical shape and tests that
// the mask is persistent with changing volume property parameters

#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastCompositeBinaryMask1(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Dimensions of object
  const int cx = 128;
  const int cy = 128;
  const int cz = 512;

  // Full scale value for data
  const double fullScale = 100.0;

  // Create the image data and mask objects
  vtkNew<vtkImageData> imageData;
  imageData->SetDimensions(cx, cy, cz);
  imageData->AllocateScalars(VTK_UNSIGNED_SHORT, 1);

  vtkNew<vtkImageData> imageMask;
  imageMask->SetDimensions(cx, cy, cz);
  imageMask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  // Get pointers to the image and mask data's scalar arrays
  unsigned short *image =
    static_cast<unsigned short*>(imageData->GetScalarPointer());
  unsigned char *mask =
    static_cast<unsigned char*>(imageMask->GetScalarPointer());

  // Initialize image and mask with data
  int index = 0;
  for (int z = 0; z < cz; ++z)
    {
    for( int y=0; y < cy; ++y)
      {
      for( int x=0; x < cx; ++x)
        {
        // Data will increase from 0 to full scale in the z direction
        image[index] = static_cast<unsigned short>( fullScale * z / cz);

        // Inside the mask? Radius of cylinder mask is 1/2 cx which should
        // equal cy
        const double radius = cx / 2.0;
        const double xCenter = cx / 2.0;
        const double yCenter = cy / 2.0;
        const double distance = sqrt(pow(x-xCenter, 2.0) + pow(y-yCenter, 2.0));
        const bool inside = distance < radius;
        mask[index] = (inside) ? 255 : 0;
        index++;
        }
      }
    }

  // Create a volume mapper and add image data and mask
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputData(imageData.GetPointer());
  mapper->SetMaskInput(imageMask.GetPointer());
  mapper->SetMaskTypeToBinary();

  // Create color and opacity nodes (red and blue)
  vtkNew<vtkColorTransferFunction> colors;
  colors->AddHSVPoint(0.0*fullScale, 0.0, 0.5, 0.5);
  colors->AddHSVPoint(1.0*fullScale, 2.0/3.0, 0.5, 0.5);

  vtkNew<vtkPiecewiseFunction> opacities;
  opacities->AddPoint(0.0*fullScale, 0.6);
  opacities->AddPoint(1.0*fullScale, 0.6);

  // Create color property
  vtkNew<vtkVolumeProperty> colorProperty;
  colorProperty->SetColor(colors.GetPointer());
  colorProperty->SetScalarOpacity(opacities.GetPointer());

  // Create volume
  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(colorProperty.GetPointer());

  // Render
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->Render();
  int valid = mapper->IsRenderSupported(renWin.GetPointer(),
                                        colorProperty.GetPointer());
  if (!valid)
  {
    cout << "Required extensions not supported." << endl;
    return EXIT_SUCCESS;
  }

  ren->AddVolume(volume.GetPointer());
  renWin->Render();
  double values[6];
  colors->GetNodeValue(0, values);
  values[2] = 0.5; values[3] = 0.5;
  colors->SetNodeValue(0, values);
  ren->ResetCamera();
  renWin->Render();
  return vtkTesting::InteractorEventLoop( argc, argv, iren.GetPointer() );
}
