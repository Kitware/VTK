/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProbeFilterImageInput.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProbeFilter.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDelaunay3D.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkRegressionTestImage.h"

#include <iostream>

int TestProbeFilterImageInput(int argc, char* argv[])
{
  static const int dim = 48;
  double center[3];
  center[0] = center[1] = center[2] = static_cast<double>(dim)/2.0;
  int extent[6] = { 0, dim - 1, 0, dim - 1, 0, dim - 1 };

  vtkNew<vtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(extent[0], extent[1], extent[2], extent[3],
                              extent[4], extent[5]);
  imageSource->SetCenter(center);
  imageSource->Update();

  vtkImageData *img = imageSource->GetOutput();
  double range[2], origin[3], spacing[3];
  img->GetScalarRange(range);
  img->GetOrigin(origin);
  img->GetSpacing(spacing);


  // create an unstructured grid by generating a point cloud and
  // applying Delaunay triangulation on it.
  vtkMath::RandomSeed(0); // vtkPointSource internally uses vtkMath::Random()
  vtkNew<vtkPointSource> pointSource;
  pointSource->SetCenter(center);
  pointSource->SetRadius(center[0]);
  pointSource->SetNumberOfPoints(24 * 24 * 24);

  vtkNew<vtkDelaunay3D> delaunay3D;
  delaunay3D->SetInputConnection(pointSource->GetOutputPort());

  // probe into img using unstructured grif geometry
  vtkNew<vtkProbeFilter> probe1;
  probe1->SetSourceData(img);
  probe1->SetInputConnection(delaunay3D->GetOutputPort());

  // probe into the unstructured grid using ImageData geometry
  vtkNew<vtkImageData> outputData;
  outputData->SetExtent(extent);
  outputData->SetOrigin(origin);
  outputData->SetSpacing(spacing);
  vtkNew<vtkFloatArray> fa;
  fa->SetName("scalars");
  fa->Allocate(dim * dim * dim);
  outputData->GetPointData()->SetScalars(fa.GetPointer());

  vtkNew<vtkProbeFilter> probe2;
  probe2->SetSourceConnection(probe1->GetOutputPort());
  probe2->SetInputData(outputData.GetPointer());


  // render using ray-cast volume rendering
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  renWin->AddRenderer(ren.GetPointer());
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkSmartVolumeMapper> volumeMapper;
  volumeMapper->SetInputConnection(probe2->GetOutputPort());
  volumeMapper->SetRequestedRenderModeToRayCast();

  vtkNew<vtkColorTransferFunction> volumeColor;
  volumeColor->AddRGBPoint(range[0], 0.0, 0.0, 1.0);
  volumeColor->AddRGBPoint((range[0] + range[1]) * 0.5, 0.0, 1.0, 0.0);
  volumeColor->AddRGBPoint(range[1], 1.0, 0.0, 0.0);

  vtkNew<vtkPiecewiseFunction> volumeScalarOpacity;
  volumeScalarOpacity->AddPoint(range[0], 0.0);
  volumeScalarOpacity->AddPoint((range[0] + range[1]) * 0.5, 0.0);
  volumeScalarOpacity->AddPoint(range[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(volumeColor.GetPointer());
  volumeProperty->SetScalarOpacity(volumeScalarOpacity.GetPointer());
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOn();
  volumeProperty->SetAmbient(0.5);
  volumeProperty->SetDiffuse(0.8);
  volumeProperty->SetSpecular(0.2);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddViewProp(volume.GetPointer());
  ren->ResetCamera();
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
