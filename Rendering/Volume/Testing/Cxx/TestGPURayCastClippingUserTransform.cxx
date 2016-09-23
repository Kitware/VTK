/*=========================================================================
Program:   Visualization Toolkit
Module:    TestGPURayCastClippingUserTransform.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Description
// This test creates a vtkImageData with two components.
// The data is volume rendered considering the two components as independent.
#include <iostream>
#include <fstream>
using namespace std;

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkUnsignedShortArray.h"
#include "vtkImageReader2.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkInteractorStyleImage.h"
#include "vtkCommand.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkMatrix4x4.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"


static double* ComputeNormal(double* reference, bool flipSign)
{
  double* normal = new double[3];
  if (flipSign)
  {
    normal[0] = -reference[0];
    normal[1] = -reference[1];
    normal[2] = -reference[2];
  }
  else
  {
    normal[0] = reference[0];
    normal[1] = reference[1];
    normal[2] = reference[2];
  }

  return normal;
}


static double* ComputeOrigin(double* focalPoint, double* reference,
                             double distance, bool flipSign)
{

  double* origin = new double[3];
  if (flipSign)
  {
    origin[0] = focalPoint[0] - distance * reference[0];
    origin[1] = focalPoint[1] - distance * reference[1];
    origin[2] = focalPoint[2] - distance * reference[2];
  }
  else
  {
    origin[0] = focalPoint[0] + distance * reference[0];
    origin[1] = focalPoint[1] + distance * reference[1];
    origin[2] = focalPoint[2] + distance * reference[2];
  }

  return origin;
}


static void UpdateFrontClippingPlane(vtkPlane* frontClippingPlane,
                                     double* normal, double* focalPoint,
                                     double slabThickness)
{
  // The front plane is the start of ray cast
  // The front normal should be in the same direction as the camera
  // direction (opposite to the plane facing direction)
  double* frontNormal = ComputeNormal(normal, true);

  // Get the origin of the front clipping plane
  double halfSlabThickness = slabThickness / 2;
  double* frontOrigin = ComputeOrigin(focalPoint, normal, halfSlabThickness, false);

  // Set the normal and origin of the front clipping plane
  frontClippingPlane->SetNormal(frontNormal);
  frontClippingPlane->SetOrigin(frontOrigin);
}


static void UpdateRearClippingPlane(vtkPlane* rearClippingPlane, double* normal,
                                    double* focalPoint, double slabThickness)
{

  // The rear normal is the end of ray cast
  // The rear normal should be in the opposite direction to the
  // camera direction (same as the plane facing direction)
  double* rearNormal = ComputeNormal(normal, false);

  // Get the origin of the rear clipping plane
  double halfSlabThickness = slabThickness / 2;
  double* rearOrigin = ComputeOrigin(focalPoint, normal, halfSlabThickness, true);

  // Set the normal and origin of the rear clipping plane
  rearClippingPlane->SetNormal(rearNormal);
  rearClippingPlane->SetOrigin(rearOrigin);
}


class vtkInteractorStyleCallback : public vtkCommand
{
public:
  static vtkInteractorStyleCallback *New()
  {
    return new vtkInteractorStyleCallback;
  }

  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkInteractorStyle* style = reinterpret_cast<vtkInteractorStyle*>(caller);

    vtkCamera * camera = style->GetCurrentRenderer()->GetActiveCamera();
    //vtkCamera *camera = reinterpret_cast<vtkCamera*>(caller);

    // Get the normal and focal point of the camera
    double* normal = camera->GetViewPlaneNormal();
    double* focalPoint = camera->GetFocalPoint();

    // Fixed slab thickness
    slabThickness = 3.0;
    UpdateFrontClippingPlane(frontClippingPlane, normal, focalPoint, slabThickness);
    UpdateRearClippingPlane(rearClippingPlane, normal, focalPoint, slabThickness);
  }

  vtkInteractorStyleCallback(){}

  void SetFrontClippingPlane(vtkPlane* fcPlane)
  {
    this->frontClippingPlane = fcPlane;
  }

  void SetRearClippingPlane(vtkPlane* rcPlane)
  {
    this->rearClippingPlane = rcPlane;
  }

  double slabThickness;
  vtkPlane* frontClippingPlane;
  vtkPlane* rearClippingPlane;
};


int TestGPURayCastClippingUserTransform(int argc, char *argv[])
{
  int width = 256;
  int height = 256;
  int depth = 148;
  double spacing[3] = { 1.4844, 1.4844, 1.2 };

  // Read the image
  streampos size;
  char * memblock;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                     "Data/MagnitudeImage_256x256x148");

  ifstream file(fname, ios::in | ios::binary | ios::ate);
  if (file.is_open())
  {
    size = file.tellg();
    memblock = new char[size];
    file.seekg(0, ios::beg);
    file.read(memblock, size);
    file.close();
  }
  else
  {
    cout << "Unable to open file";
    return 1;
  }

  // Convert to short
  unsigned short* shortData = new unsigned short[size / 2];
  int idx = 0;
  int idx2 = 0;
  for (idx = 0; idx < size / 2; idx ++) {
    idx2 = idx * 2;
    shortData[idx] = (short)(((memblock[idx2] & 0xFF) << 8) | (memblock[idx2+1] & 0xFF));
  }

  //
  int volumeSizeInSlice = width * height * depth;
  vtkNew<vtkUnsignedShortArray> dataArrayMag;
  dataArrayMag->Allocate(volumeSizeInSlice, 0);
  dataArrayMag->SetNumberOfComponents(1);
  dataArrayMag->SetNumberOfTuples(volumeSizeInSlice);
  dataArrayMag->SetArray(shortData, volumeSizeInSlice, 1);

  vtkNew<vtkImageData> imageData;
  imageData->SetDimensions(width, height, depth);
  imageData->SetSpacing(spacing);
  imageData->GetPointData()->SetScalars(dataArrayMag.GetPointer());

  // Create a clipping plane
  vtkNew<vtkPlane> frontClippingPlane;
  vtkNew<vtkPlane> rearClippingPlane;

  // Create a clipping plane collection
  vtkNew<vtkPlaneCollection> clippingPlaneCollection;
  clippingPlaneCollection->AddItem(frontClippingPlane.GetPointer());
  clippingPlaneCollection->AddItem(rearClippingPlane.GetPointer());

  // Create a mapper
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  //volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetInputData(imageData.GetPointer());
  volumeMapper->SetBlendModeToMaximumIntensity();
  volumeMapper->AutoAdjustSampleDistancesOff();
  volumeMapper->SetSampleDistance(1.0);
  volumeMapper->SetImageSampleDistance(1.0);
  volumeMapper->SetClippingPlanes(clippingPlaneCollection.GetPointer());

  // Create volume scale opacity
  vtkNew<vtkPiecewiseFunction> volumeScalarOpacity;
  volumeScalarOpacity->AddPoint(0, 0.0);
  volumeScalarOpacity->AddPoint(32767, 1.0);
  volumeScalarOpacity->ClampingOn();

  // Create a property
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOff();
  volumeProperty->SetAmbient(1.0);
  volumeProperty->SetDiffuse(0.0);
  volumeProperty->SetSpecular(0.0);
  volumeProperty->IndependentComponentsOn();
  volumeProperty->SetScalarOpacity(volumeScalarOpacity.GetPointer());
  volumeProperty->SetColor(volumeScalarOpacity.GetPointer());

  // Create a volume
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  volume->PickableOff();

  // Rotate the blue props
  double rowVector[3] = { 0.0, 0.0, -1.0 };
  double columnVector[3] = { 1.0, 0.0, 0.0 };
  double normalVector[3];
  vtkMath::Cross(rowVector, columnVector, normalVector);
  double position[3] = { 0.0, 0.0, 0.0 };

  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->Identity();
  matrix->SetElement(0, 0, rowVector[0]);
  matrix->SetElement(0, 1, rowVector[1]);
  matrix->SetElement(0, 2, rowVector[2]);
  matrix->SetElement(0, 3, position[0]);
  matrix->SetElement(1, 0, columnVector[0]);
  matrix->SetElement(1, 1, columnVector[1]);
  matrix->SetElement(1, 2, columnVector[2]);
  matrix->SetElement(1, 3, position[1]);
  matrix->SetElement(2, 0, normalVector[0]);
  matrix->SetElement(2, 1, normalVector[1]);
  matrix->SetElement(2, 2, normalVector[2]);
  matrix->SetElement(2, 3, position[2]);

  volume->SetUserMatrix(matrix);

  // Create a outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputData(imageData.GetPointer());

  // Create an outline mapper
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());

  vtkNew<vtkActor> outline;
  outline->SetMapper(outlineMapper.GetPointer());
  outline->PickableOff();

  // Create a renderer
  vtkNew<vtkRenderer> ren;
  ren->AddViewProp(volume.GetPointer());
  ren->AddViewProp(outline.GetPointer());

  // Get the center of volume
  double* center = volume->GetCenter();
  double cameraFocal[3];
  cameraFocal[0] = center[0];
  cameraFocal[1] = center[1];
  cameraFocal[2] = center[2];

  double cameraViewUp[3] = { 0.00, -1.00, 0.00 };
  double cameraNormal[3] = { 0.00, 0.00, -1.00 };
  double cameraDistance = 1000.0;

  double cameraPosition[3];
  cameraPosition[0] = cameraFocal[0] + cameraDistance * cameraNormal[0];
  cameraPosition[1] = cameraFocal[1] + cameraDistance * cameraNormal[1];
  cameraPosition[2] = cameraFocal[2] + cameraDistance * cameraNormal[2];

  // Update clipping planes
  UpdateFrontClippingPlane(frontClippingPlane.GetPointer(), cameraNormal, cameraFocal, 3.0);
  UpdateRearClippingPlane(rearClippingPlane.GetPointer(), cameraNormal, cameraFocal, 3.0);

  // Get the active camera
  vtkCamera* camera = ren->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(250);
  camera->SetPosition(cameraPosition);
  camera->SetFocalPoint(cameraFocal);
  camera->SetViewUp(cameraViewUp);

  // Create a render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(500, 500);
  renWin->AddRenderer(ren.GetPointer());

  // Create a style
  vtkNew<vtkInteractorStyleImage> style;
  style->SetInteractionModeToImage3D();

  // Create a interactor style callback
  vtkNew<vtkInteractorStyleCallback> interactorStyleCallback;
  interactorStyleCallback->frontClippingPlane = frontClippingPlane.GetPointer();
  interactorStyleCallback->rearClippingPlane = rearClippingPlane.GetPointer();
  style->AddObserver(vtkCommand::InteractionEvent, interactorStyleCallback.GetPointer());

  // Create an interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetInteractorStyle(style.GetPointer());
  iren->SetRenderWindow(renWin.GetPointer());

  // Start
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin.GetPointer(), 70);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }


  delete[] memblock;
  delete[] shortData;
  delete[] fname;

  return !retVal;
}
