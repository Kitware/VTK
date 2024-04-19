// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Label map validation tests
 *
 * Test label mapping with a various scenarios of mismatch between mask labels and properties
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

namespace TestGPURayCastLabelMapValidityNS
{
void CreateTransferFunctions(vtkSmartPointer<vtkVolumeProperty> vprop)
{
  // Main color map
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.3, 0.3, 0.3);   // grey
  ctf->AddRGBPoint(49, 0.3, 0.3, 0.3);  // grey
  ctf->AddRGBPoint(50, 1.0, 0.65, 0.0); // orange
  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.1);
  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.4);
  vprop->SetColor(ctf);
  vprop->SetScalarOpacity(pf);
  vprop->SetGradientOpacity(gf);
  vprop->SetDisableGradientOpacity(0);
  vprop->SetColor(ctf);
  vprop->SetScalarOpacity(pf);

  // setup mask 1 colors
  vtkNew<vtkColorTransferFunction> ctf1;
  vtkNew<vtkPiecewiseFunction> gradientOpacityFun1;
  vtkNew<vtkPiecewiseFunction> opacityFunc1;

  ctf1->AddRGBPoint(0, 1.0, 0.0, 0.0); // red
  gradientOpacityFun1->AddPoint(0, 0.6);
  opacityFunc1->AddPoint(0, 1.0);

  // setup mask 2 colors
  vtkNew<vtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0, 0.0, 1.0, 0.0); // green

  // setup mask 3 colors
  vtkNew<vtkColorTransferFunction> ctf3;
  ctf3->AddRGBPoint(0, 0.0, 0.0, 1.0); // blue

  vprop->SetLabelColor(1, ctf1);
  vprop->SetLabelScalarOpacity(1, opacityFunc1);
  vprop->SetLabelGradientOpacity(1, gradientOpacityFun1);

  // Test re-using values
  vprop->SetLabelColor(2, ctf2);
  vprop->SetLabelScalarOpacity(2, opacityFunc1);
  vprop->SetLabelGradientOpacity(2, gradientOpacityFun1);

  vtkNew<vtkPiecewiseFunction> gradientOpacityFun3;
  gradientOpacityFun3->AddPoint(0, 0.7);
  vtkNew<vtkPiecewiseFunction> opacityFunc3;
  opacityFunc3->AddPoint(0, 0.9);
  vprop->SetLabelColor(3, ctf3);
  vprop->SetLabelScalarOpacity(3, opacityFunc3);
  vprop->SetLabelGradientOpacity(3, gradientOpacityFun3);
}

void CreateVolumePipeline(vtkSmartPointer<vtkImageData> image, vtkSmartPointer<vtkImageData> mask,
  vtkSmartPointer<vtkVolume> volume, vtkSmartPointer<vtkVolumeProperty> volumeProperty,
  vtkSmartPointer<vtkGPUVolumeRayCastMapper> mapper)
{
  volume->SetProperty(volumeProperty);
  volume->SetMapper(mapper);
  mapper->SetInputData(image);
  mapper->SetMaskInput(mask);
  mapper->SetMaskTypeToLabelMap();
}

void CreateImageData(vtkSmartPointer<vtkImageData> imData)
{
  int dims[3] = { 60, 10, 60 };
  imData->SetOrigin(0, 0, 0);
  imData->SetSpacing(1, 1, 1);
  imData->SetDimensions(dims);
  imData->AllocateScalars(VTK_UNSIGNED_SHORT, 1);

  unsigned short* ptr = static_cast<unsigned short*>(imData->GetScalarPointer(0, 0, 0));
  for (int k = 0; k < dims[2]; ++k)
  {
    for (int j = 0; j < dims[1]; ++j)
    {
      for (int i = 0; i < dims[0]; ++i)
      {
        *ptr++ = static_cast<unsigned short>(j * 2);
      }
    }
  }
}

void CreateMaskData(vtkSmartPointer<vtkImageData> mask, int testcase)
{
  int dims[3] = { 60, 10, 60 };
  mask->SetOrigin(0, 0, 0);
  mask->SetSpacing(1, 1, 1);
  mask->SetDimensions(dims);
  mask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  int cubeSize = 15;
  int spacer = 3;
  for (int k = 0; k < dims[2]; ++k)
  {
    for (int j = 0; j < dims[1]; ++j)
    {
      for (int i = 0; i < dims[0]; ++i)
      {
        unsigned char* ptr = static_cast<unsigned char*>(mask->GetScalarPointer(i, j, k));
        if (k > cubeSize * 0 + spacer && k < cubeSize * 1 + spacer)
        {
          if (i > spacer && i < cubeSize * 1 + spacer)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 1; // label 1 mask
                break;
              case 2:
                *ptr = 5; // label 5 mask (undefined label)
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
          if (i > cubeSize * 1 + spacer * 2 && i < cubeSize * 2 + spacer * 2)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 2; // label 2 mask
                break;
              case 2:
                *ptr = 2; // label 2 mask
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
          if (i > cubeSize * 2 + spacer * 3 && i < cubeSize * 3 + spacer * 3)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 3; // label 3 mask
                break;
              case 2:
                *ptr = 0; // no label
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
        }

        // row 2 - 3 cubes
        if (k > cubeSize * 1 + spacer * 2 && k < cubeSize * 2 + spacer * 2)
        {
          if (i > spacer && i < cubeSize * 1 + spacer)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 1; // label 1 mask
                break;
              case 2:
                *ptr = 1; // label 1 mask
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
          if (i > cubeSize * 1 + spacer * 2 && i < cubeSize * 2 + spacer * 2)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 2; // label 2 mask
                break;
              case 2:
                *ptr = 4; // label 4 mask (undefined label)
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
          if (i > cubeSize * 2 + spacer * 3 && i < cubeSize * 3 + spacer * 3)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 3; // label 3 mask
                break;
              case 2:
                *ptr = 3; // label 3 mask
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
        }

        // row 3 - 3 cubes
        if (k > cubeSize * 2 + spacer * 3 && k < cubeSize * 3 + spacer * 3)
        {
          if (i > spacer && i < cubeSize * 1 + spacer)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 1; // label 1 mask
                break;
              case 2:
                *ptr = 1; // label 1 mask
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
          if (i > cubeSize * 1 + spacer * 2 && i < cubeSize * 2 + spacer * 2)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 2; // label 2 mask
                break;
              case 2:
                *ptr = 2; // label 2 mask
                break;
              case 3:
                *ptr = 5; // label 5 mask (undefined label)
                break;
            }
          }
          if (i > cubeSize * 2 + spacer * 3 && i < cubeSize * 3 + spacer * 3)
          {
            switch (testcase)
            {
              case 0:
              default:
                *ptr = 1; // label 1 mask
                break;
              case 1:
                *ptr = 3; // label 3 mask
                break;
              case 2:
                *ptr = 3; // label 3 mask
                break;
              case 3:
                *ptr = 2; // label 5 mask (undefined label)
                break;
            }
          }
        }
      }
    }
  }
}
} // end of namespace TestGPURayCastValidityNS

int TestGPURayCastLabelMapValidity(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren[4];
  vtkNew<vtkGPUVolumeRayCastMapper> mapper[4];
  vtkNew<vtkVolume> volume[4];
  vtkNew<vtkVolumeProperty> volumeProperty;
  TestGPURayCastLabelMapValidityNS::CreateTransferFunctions(volumeProperty);
  vtkNew<vtkImageData> image;
  TestGPURayCastLabelMapValidityNS::CreateImageData(image);
  for (int i = 0; i < 4; ++i)
  {
    ren[i]->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    double vp[4];
    ren[i]->GetViewport(vp);
    ren[i]->AddVolume(volume[i]);
    renWin->AddRenderer(ren[i]);
    vtkNew<vtkImageData> mask;
    TestGPURayCastLabelMapValidityNS::CreateMaskData(mask, (3 - i));
    TestGPURayCastLabelMapValidityNS::CreateVolumePipeline(
      image, mask, volume[i], volumeProperty, mapper[i]);
    vtkCamera* c = ren[i]->GetActiveCamera();
    c->SetFocalPoint(0, 0, 0);
    c->SetPosition(0, 1, 0); // view along the Y axis
    c->SetViewUp(0, 0, -1);  // look down on the Z axis
    c->SetParallelProjection(true);
    ren[i]->ResetCamera();
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  int retVal = vtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
