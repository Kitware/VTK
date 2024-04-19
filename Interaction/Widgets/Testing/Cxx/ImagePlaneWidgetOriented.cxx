// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageDataOutlineFilter.h"
#include "vtkImageMapToColors.h"
#include "vtkImageMapper3D.h"
#include "vtkImagePlaneWidget.h"
#include "vtkLogger.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume16Reader.h"

#include "vtkTestUtilities.h"

namespace helper
{
struct ImageInfo
{
  vtkIdType NbOfPoints;
  vtkIdType NbOfCells;
  double Origin[3];
  int Dimensions[3];
  int Extent[6];

  ImageInfo(vtkImageData* image)
  {
    this->NbOfPoints = image->GetNumberOfPoints();
    this->NbOfCells = image->GetNumberOfCells();
    image->GetOrigin(this->Origin);
    image->GetDimensions(this->Dimensions);
    image->GetExtent(this->Extent);
  }
};
};

//------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> loadImage(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkNew<vtkVolume16Reader> volumeReader;
  volumeReader->SetDataDimensions(64, 64);
  volumeReader->SetDataByteOrderToLittleEndian();
  volumeReader->SetImageRange(1, 93);
  volumeReader->SetDataSpacing(3.2, 3.2, 1.5);
  volumeReader->SetFilePrefix(fname);
  volumeReader->SetDataMask(0x7fff);
  volumeReader->Update();
  delete[] fname;

  return volumeReader->GetOutput();
}

//------------------------------------------------------------------------------
void addImageOutlineToRenderer(vtkImageData* image, vtkRenderer* renderer, double color[3])
{
  vtkNew<vtkImageDataOutlineFilter> outline;
  outline->SetInputData(image);
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);
  outlineActor->GetProperty()->SetColor(color);
  renderer->AddActor(outlineActor);
}

//------------------------------------------------------------------------------
void addResliceOutputToRenderer(vtkImagePlaneWidget* planeWidget, vtkRenderer* renderer)
{
  vtkNew<vtkImageMapToColors> colorMap;
  colorMap->PassAlphaToOutputOff();
  colorMap->SetActiveComponent(0);
  colorMap->SetOutputFormatToLuminance();
  colorMap->SetInputData(planeWidget->GetResliceOutput());
  colorMap->SetLookupTable(planeWidget->GetLookupTable());

  vtkNew<vtkImageActor> imageActor;
  imageActor->PickableOff();
  imageActor->GetMapper()->SetInputConnection(colorMap->GetOutputPort());

  renderer->AddActor(imageActor);
}

//------------------------------------------------------------------------------
void setupPlaneWidget(
  vtkImagePlaneWidget* planeWidget, vtkRenderWindowInteractor* iren, vtkImageData* volumeInput)
{
  vtkNew<vtkCellPicker> picker;
  picker->SetTolerance(0.005);

  planeWidget->SetInteractor(iren);
  planeWidget->SetPicker(picker);
  planeWidget->SetInputData(volumeInput);

  planeWidget->SetPlaneOrientationToXAxes();

  planeWidget->SetSliceIndex(42);
  planeWidget->On();
}

//------------------------------------------------------------------------------
void setupWindow(vtkRenderWindow* window, vtkRenderer* sceneRenderer,
  vtkRenderer* straightSliceRenderer, vtkRenderer* orientedSliceRenderer)
{
  window->SetMultiSamples(0);
  window->SetSize(600, 350);

  window->AddRenderer(sceneRenderer);
  window->AddRenderer(straightSliceRenderer);
  window->AddRenderer(orientedSliceRenderer);

  sceneRenderer->SetBackground(0.4, 0.4, 0.8);
  straightSliceRenderer->SetBackground(0.8, 0.4, 0.8);
  orientedSliceRenderer->SetBackground(0.4, 0.8, 0.8);

  sceneRenderer->SetViewport(0, 0, 0.5, 1);
  straightSliceRenderer->SetViewport(0.5, 0, 1, 0.5);
  orientedSliceRenderer->SetViewport(0.5, 0.5, 1, 1);

  window->Render();

  sceneRenderer->ResetCamera();
  straightSliceRenderer->ResetCamera();
  sceneRenderer->GetActiveCamera()->Elevation(110);
  sceneRenderer->GetActiveCamera()->SetViewUp(0, 0, -1);
  sceneRenderer->GetActiveCamera()->Azimuth(45);
  sceneRenderer->GetActiveCamera()->Dolly(1.15);
  sceneRenderer->ResetCameraClippingRange();
}

//------------------------------------------------------------------------------
void compareReslicePlane(vtkImageData* straightImage, vtkImageData* orientedImage,
  vtkImagePlaneWidget* straightWidget, vtkImagePlaneWidget* orientedWidget)
{
  double straightPoint[3];
  straightWidget->GetOrigin(straightPoint);
  double straightIndices[3];
  straightImage->TransformPhysicalPointToContinuousIndex(straightPoint, straightIndices);

  double orientedPoint[3];
  orientedWidget->GetOrigin(orientedPoint);
  double orientedIndices[3];
  orientedImage->TransformPhysicalPointToContinuousIndex(orientedPoint, orientedIndices);

  double indicesDelta = vtkMath::Distance2BetweenPoints(straightIndices, orientedIndices);
  vtkLogIf(ERROR, indicesDelta > 10e-6,
    "Reslice planes should have same Origin in image coordinates. Has "
      << orientedIndices[0] << " " << orientedIndices[1] << " " << orientedIndices[2]
      << " instead of " << straightIndices[0] << " " << straightIndices[1] << " "
      << straightIndices[2] << ". Error squared: " << indicesDelta);

  double straightNormal[3];
  straightWidget->GetNormal(straightNormal);
  double orientedNormal[3];
  orientedWidget->GetNormal(orientedNormal);

  straightImage->TransformPhysicalPointToContinuousIndex(straightNormal, straightIndices);
  orientedImage->TransformPhysicalPointToContinuousIndex(orientedNormal, orientedIndices);

  double angle = vtkMath::AngleBetweenVectors(straightIndices, orientedIndices);
  vtkLogIf(ERROR, !vtkMathUtilities::FuzzyCompare(angle, 0.),
    "Reslice planes should have same Normal in image coordinates. Has "
      << orientedIndices[0] << " " << orientedIndices[1] << " " << orientedIndices[2]
      << " instead of " << straightIndices[0] << " " << straightIndices[1] << " "
      << straightIndices[2] << ". Angle is: " << angle);
}

//------------------------------------------------------------------------------
void compareResliceImage(vtkImagePlaneWidget* straightWidget, vtkImagePlaneWidget* orientedWidget)
{
  auto straightSlice = straightWidget->GetResliceOutput();
  auto orientedSlice = orientedWidget->GetResliceOutput();

  auto straightInfo = helper::ImageInfo(straightSlice);
  auto orientedInfo = helper::ImageInfo(orientedSlice);

  vtkLogIf(ERROR, straightInfo.NbOfPoints != orientedInfo.NbOfPoints,
    "Number of points differs. Has " << orientedInfo.NbOfPoints << " instead of "
                                     << straightInfo.NbOfPoints);

  vtkLogIf(ERROR, straightInfo.NbOfCells != orientedInfo.NbOfCells,
    "Number of cells differs. Has " << orientedInfo.NbOfCells << " instead of "
                                    << straightInfo.NbOfCells);

  for (int axes = 0; axes < 3; axes++)
  {
    if (!vtkMathUtilities::NearlyEqual(straightInfo.Origin[axes], orientedInfo.Origin[axes], 10e-6))
    {
      vtkLog(ERROR,
        "Origin differs. Has " << orientedInfo.Origin[0] << " " << orientedInfo.Origin[1] << " "
                               << orientedInfo.Origin[2] << " "
                               << "instead of " << straightInfo.Origin[0] << " "
                               << straightInfo.Origin[1] << " " << straightInfo.Origin[2] << ".");
      break;
    }
  }

  for (int axes = 0; axes < 3; axes++)
  {
    if (straightInfo.Dimensions[axes] != orientedInfo.Dimensions[axes])
    {
      vtkLog(ERROR,
        "Dimensions differs. Has "
          << orientedInfo.Dimensions[0] << " " << orientedInfo.Dimensions[1] << " "
          << orientedInfo.Dimensions[2] << " "
          << "instead of " << straightInfo.Dimensions[0] << " " << straightInfo.Dimensions[1] << " "
          << straightInfo.Dimensions[2] << ".");
      break;
    }
  }

  for (int axes = 0; axes < 3; axes++)
  {
    if (straightInfo.Extent[2 * axes] != orientedInfo.Extent[2 * axes] ||
      straightInfo.Extent[2 * axes + 1] != orientedInfo.Extent[2 * axes + 1])
    {
      vtkLog(ERROR,
        "Extent differs. Has " << orientedInfo.Extent[0] << " " << orientedInfo.Extent[1] << " "
                               << orientedInfo.Extent[2] << " " << orientedInfo.Extent[3] << " "
                               << orientedInfo.Extent[4] << " " << orientedInfo.Extent[5] << " "
                               << "instead of " << straightInfo.Extent[0] << " "
                               << straightInfo.Extent[1] << " " << straightInfo.Extent[2] << "."
                               << straightInfo.Extent[3] << " " << straightInfo.Extent[4] << " "
                               << straightInfo.Extent[5] << ".");
      break;
    }
  }

  vtkLogIf(ERROR, false, "Slices are differents");
}

//------------------------------------------------------------------------------
int ImagePlaneWidgetOriented(int argc, char* argv[])
{
  vtkSmartPointer<vtkImageData> straightData = loadImage(argc, argv);

  vtkNew<vtkRenderer> sceneRenderer;
  double color[3] = { 0.5, 0.5, 0.5 };
  addImageOutlineToRenderer(straightData, sceneRenderer, color);

  // rotate original data by Pi/5
  vtkNew<vtkImageData> orientedData;
  orientedData->DeepCopy(straightData);
  const double ANGLE = vtkMath::Pi() / 5;
  const double COS = std::cos(ANGLE);
  const double SIN = std::sin(ANGLE);
  orientedData->SetDirectionMatrix(COS, -SIN, 0, SIN, COS, 0, 0, 0, 1);
  color[0] = 1;
  color[1] = 1;
  color[2] = 0;
  addImageOutlineToRenderer(orientedData, sceneRenderer, color);

  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkRenderer> orientedSliceRenderer;
  vtkNew<vtkRenderer> straightSliceRenderer;
  setupWindow(window, sceneRenderer, orientedSliceRenderer, straightSliceRenderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  vtkNew<vtkImagePlaneWidget> orientedPlaneWidget;
  setupPlaneWidget(orientedPlaneWidget, interactor, orientedData);
  addResliceOutputToRenderer(orientedPlaneWidget, orientedSliceRenderer);

  vtkNew<vtkImagePlaneWidget> straightPlaneWidget;
  setupPlaneWidget(straightPlaneWidget, interactor, straightData);
  addResliceOutputToRenderer(straightPlaneWidget, straightSliceRenderer);

  window->Render();
  sceneRenderer->ResetCamera();
  orientedSliceRenderer->ResetCamera();
  straightSliceRenderer->ResetCamera();

  compareReslicePlane(straightData, orientedData, straightPlaneWidget, orientedPlaneWidget);
  compareResliceImage(straightPlaneWidget, orientedPlaneWidget);

  interactor->Initialize();
  window->Render();
  interactor->Start();

  return EXIT_SUCCESS;
}
