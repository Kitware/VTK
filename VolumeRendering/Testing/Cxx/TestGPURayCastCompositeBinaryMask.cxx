/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCompositeMask.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkVolume16Reader.h"
#include "vtkImageData.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"

int TestGPURayCastCompositeBinaryMask(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> reader =
    vtkSmartPointer<vtkVolume16Reader>::New();
  reader->SetDataDimensions( 64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange( 1, 93);
  reader->SetDataSpacing( 3.2, 3.2, 1.5);
  reader->SetFilePrefix( fname );
  reader->SetDataMask( 0x7fff);
  reader->Update();

  delete[] fname;

  vtkImageData *input=reader->GetOutput();

  int dim[3];
  double spacing[3];
  input->GetSpacing(spacing);
  input->GetDimensions(dim);

  vtkSmartPointer< vtkGPUVolumeRayCastMapper > mapper
    = vtkSmartPointer< vtkGPUVolumeRayCastMapper >::New();
  vtkSmartPointer< vtkVolume > volume
    = vtkSmartPointer< vtkVolume >::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetMaskTypeToBinary();
  mapper->SetAutoAdjustSampleDistances(0);

   // assume the scalar field is a set of samples taken from a
  // contiguous band-limited volumetric field.
  // assume max frequency is present:
  // min spacing divided by 2. Nyquist-Shannon theorem says so.
  // sample distance could be bigger if we compute the actual max frequency
  // in the data.
  double distance=spacing[0];
  if(distance>spacing[1])
    {
    distance=spacing[1];
    }
  if(distance>spacing[2])
    {
    distance=spacing[2];
    }
  distance=distance/2.0;

  // This does not take the screen size of a cell into account.
  // distance has to be smaller: min(nyquis,screensize)

  mapper->SetSampleDistance(static_cast<float>(distance));

  vtkSmartPointer< vtkColorTransferFunction > colorFun
    = vtkSmartPointer< vtkColorTransferFunction >::New();
  vtkSmartPointer< vtkPiecewiseFunction > opacityFun
    = vtkSmartPointer< vtkPiecewiseFunction >::New();

  // Create the property and attach the transfer functions
  vtkSmartPointer< vtkVolumeProperty > property
    = vtkSmartPointer< vtkVolumeProperty >::New();
  property->SetIndependentComponents(true);
  property->SetColor(colorFun);
  property->SetScalarOpacity(opacityFun);
  property->SetInterpolationTypeToLinear();

  // connect up the volume to the property and the mapper
  volume->SetProperty(property);
  volume->SetMapper(mapper);

  colorFun->AddRGBPoint(     0.0, 0.5 , 0.0 , 0.0);
  colorFun->AddRGBPoint(   600.0, 1.0 , 0.5 , 0.5);
  colorFun->AddRGBPoint(  1280.0, 0.9 , 0.2 , 0.3);
  colorFun->AddRGBPoint(  1960.0, 0.81, 0.27, 0.1);
  colorFun->AddRGBPoint(  4095.0, 0.5 , 0.5 , 0.5);

  opacityFun->AddPoint(   70.0,  0.0);
  opacityFun->AddPoint(  599.0,  0);
  opacityFun->AddPoint(  600.0, 0);
  opacityFun->AddPoint(  1195.0, 0);
  opacityFun->AddPoint(  1200, .2);
  opacityFun->AddPoint(  1300, .3);
  opacityFun->AddPoint(  2000, .3);
  opacityFun->AddPoint(  4095.0,  1.0);

  mapper->SetBlendModeToComposite();
  property->ShadeOn();


  // Make the mask
  vtkSmartPointer< vtkImageData > mask
    = vtkSmartPointer< vtkImageData >::New();
  mask->SetExtent(input->GetExtent());
  mask->SetSpacing(input->GetSpacing());
  mask->SetScalarTypeToUnsignedChar();
  mask->SetOrigin(input->GetOrigin());
  mask->AllocateScalars();

  // Create a simple mask that's split along the X axis
  unsigned char *ptr = static_cast< unsigned char * >(mask->GetScalarPointer());
  for (int z = 0; z < dim[2]; z++)
    {
    for (int y = 0; y < dim[1]; y++)
      {
      for (int x = 0; x < dim[0]; x++)
        {
        *ptr = (x < dim[0]/2 ? 255 : 0);
        ++ptr;
        }
      }
    }

  mapper->SetMaskInput(mask);


  vtkSmartPointer< vtkRenderWindowInteractor > iren
    = vtkSmartPointer< vtkRenderWindowInteractor >::New();
  vtkSmartPointer< vtkRenderWindow > renWin
    = vtkSmartPointer< vtkRenderWindow >::New();
  renWin->SetSize(300,300);
  iren->SetRenderWindow(renWin);

  vtkSmartPointer< vtkRenderer > ren =
    vtkSmartPointer< vtkRenderer >::New();
  renWin->AddRenderer(ren);
  renWin->Render();

  int valid = mapper->IsRenderSupported(renWin,property);
  if (!valid)
    {
    cout << "Required extensions not supported." << endl;
    return EXIT_SUCCESS;
    }

  ren->AddViewProp(volume);
  iren->Initialize();
  ren->GetActiveCamera()->SetPosition(77.5144, 712.092, 83.5837);
  ren->GetActiveCamera()->SetViewUp(-0.0359422, 0.0224666, -0.999101);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  return vtkTesting::InteractorEventLoop( argc, argv, iren );
}
