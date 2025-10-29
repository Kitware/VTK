// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

namespace
{

vtkSmartPointer<vtkImageData> Create1PixelImage(
  unsigned char red, unsigned char green, unsigned char blue)
{
  vtkNew<vtkImageData> image;
  vtkNew<vtkUnsignedCharArray> imagePixel;
  imagePixel->SetNumberOfComponents(3);
  imagePixel->SetNumberOfTuples(1);
  imagePixel->FillComponent(0, red);
  imagePixel->FillComponent(1, green);
  imagePixel->FillComponent(2, blue);
  image->SetExtent(0, 1, 0, 1, 0, 0);
  image->GetPointData()->SetScalars(imagePixel);
  return image;
}

}

int TestCompositePolyDataMapperBlockTextures(int argc, char* argv[])
{
  vtkNew<vtkPlaneSource> plane1;
  plane1->SetOrigin(-1.1, -0.5, 0.0);
  plane1->SetPoint1(-0.1, -0.5, 0.0);
  plane1->SetPoint2(-1.1, 0.5, 0.0);

  vtkNew<vtkPlaneSource> plane2;
  plane2->SetOrigin(0.1, -0.5, 0.0);
  plane2->SetPoint1(1.1, -0.5, 0.0);
  plane2->SetPoint2(0.1, 0.5, 0.0);

  vtkNew<vtkGroupDataSetsFilter> groupDataSet;
  groupDataSet->AddInputConnection(plane1->GetOutputPort());
  groupDataSet->AddInputConnection(plane2->GetOutputPort());
  groupDataSet->SetOutputTypeToMultiBlockDataSet();
  groupDataSet->Update();

  vtkSmartPointer<vtkCompositePolyDataMapper> compositeMapper =
    vtkSmartPointer<vtkCompositePolyDataMapper>::New();
  compositeMapper->SetInputConnection(groupDataSet->GetOutputPort());
  vtkNew<vtkCompositeDataDisplayAttributes> compositeDataDisplayAttribute;
  compositeMapper->SetCompositeDataDisplayAttributes(compositeDataDisplayAttribute);
  // We create a 1x1 dummy image to use it as a texture. We then assign them to different blocks and
  // check the result.
  vtkSmartPointer<vtkImageData> plane1Texture = ::Create1PixelImage(255, 0, 0);
  vtkSmartPointer<vtkImageData> plane2Texture = ::Create1PixelImage(0, 0, 255);
  compositeMapper->SetBlockTextureImage(1, plane1Texture);
  compositeMapper->SetBlockTextureImage(2, plane2Texture);

  vtkNew<vtkActor> compositeActor;
  compositeActor->SetMapper(compositeMapper);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(compositeActor);

  vtkSmartPointer<vtkRenderWindow> window = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  window->AddRenderer(renderer);
  window->SetInteractor(iren);
  window->Render();

  int retVal = vtkRegressionTestImageThreshold(window.GetPointer(), 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
