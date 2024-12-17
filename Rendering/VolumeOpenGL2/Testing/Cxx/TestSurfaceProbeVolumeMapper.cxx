// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkNew.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkOpenGLSurfaceProbeVolumeMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyLine.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSplineFilter.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkVolume.h"
#include "vtkVolume16Reader.h"
#include "vtkVolumeProperty.h"

namespace
{

vtkSmartPointer<vtkPolyData> CreateCurvedPlane(double planeWidth, double& curveLength)
{
  // Create centerline polydata
  vtkNew<vtkPoints> linePoints;
  linePoints->InsertNextPoint(70.0, 105.0, 70.0);
  linePoints->InsertNextPoint(95.0, 165.0, 70.0);
  linePoints->InsertNextPoint(125.0, 105.0, 70.0);

  vtkNew<vtkPolyLine> line;
  line->GetPointIds()->SetNumberOfIds(linePoints->GetNumberOfPoints());
  for (int i = 0; i < linePoints->GetNumberOfPoints(); i++)
  {
    line->GetPointIds()->SetId(i, i);
  }

  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(line);

  vtkNew<vtkPolyData> linePolydata;
  linePolydata->SetPoints(linePoints);
  linePolydata->SetLines(lines);

  // Create spline from centerline polydata
  vtkNew<vtkSplineFilter> spline;
  spline->SetNumberOfSubdivisions(50);
  spline->SetInputData(linePolydata);
  spline->Update();

  vtkPolyData* splinePolydata = spline->GetOutput();

  // Extrude spline to create a curved plane
  vtkNew<vtkLinearExtrusionFilter> lineExtrusion;
  lineExtrusion->SetInputData(splinePolydata);
  lineExtrusion->SetExtrusionTypeToVectorExtrusion();
  lineExtrusion->SetVector(0, 0, planeWidth);

  // Compute normals for blend mode
  vtkNew<vtkPolyDataNormals> normalsFilter;
  normalsFilter->SetInputConnection(lineExtrusion->GetOutputPort());
  normalsFilter->ConsistencyOn();
  normalsFilter->SplittingOff();

  normalsFilter->Update();
  vtkSmartPointer<vtkPolyData> probeSurface = normalsFilter->GetOutput();

  curveLength = 0.0;
  // Compute centerline length for texture coordinates
  for (int i = 0; i < splinePolydata->GetNumberOfPoints(); i++)
  {
    double p1[3] = {};
    double p2[3] = {};
    splinePolydata->GetPoint(i, p1);
    splinePolydata->GetPoint(i > 0 ? i - 1 : i, p2);

    curveLength += std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  }

  // Compute texture coordinates
  vtkNew<vtkDoubleArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(2 * probeSurface->GetNumberOfPoints());

  double currentLineDistance = 0.0;
  for (int i = 0; i < splinePolydata->GetNumberOfPoints(); i++)
  {
    double p1[3] = {};
    double p2[3] = {};
    splinePolydata->GetPoint(i, p1);
    splinePolydata->GetPoint(i > 0 ? i - 1 : i, p2);

    currentLineDistance += std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2));

    double xCoord = currentLineDistance / curveLength;

    tcoords->SetTuple2(i, xCoord, 1.0);
    tcoords->SetTuple2(i + splinePolydata->GetNumberOfPoints(), xCoord, 0.0);
  }
  probeSurface->GetPointData()->SetTCoords(tcoords);

  return probeSurface;
}

}

//------------------------------------------------------------------------------
int TestSurfaceProbeVolumeMapper(int argc, char* argv[])
{
  // Load image
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkNew<vtkVolume16Reader> volumeReader;
  volumeReader->SetDataDimensions(64, 64);
  volumeReader->SetDataByteOrderToLittleEndian();
  volumeReader->SetImageRange(1, 93);
  volumeReader->SetDataSpacing(3.2, 3.2, 1.5);
  volumeReader->SetFilePrefix(fname);
  volumeReader->Update();
  delete[] fname;
  vtkImageData* volumeData = volumeReader->GetOutput();

  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(1, 1, 1);
  renderer->SetViewport(0, 0.3, 1, 1);

  vtkNew<vtkRenderer> renderer2;
  renderWindow->AddRenderer(renderer2);
  renderer2->SetBackground(1, 1, 1);
  renderer2->SetViewport(0, 0, 1, 0.3);

  // Test mixing surface probe mapper with volume rendering
  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(volumeData);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);

  vtkNew<vtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBPoint(0, 0.0, 0.0, 0.0);
  colorFunction->AddRGBPoint(3900, 1.0, 1.0, 1.0);
  vtkNew<vtkPiecewiseFunction> scalarOpacityFunction;
  scalarOpacityFunction->AddPoint(1000, 0.0);
  scalarOpacityFunction->AddPoint(3900, 0.15);
  volume->GetProperty()->SetScalarOpacity(scalarOpacityFunction);
  volume->GetProperty()->SetColor(colorFunction);
  volume->GetProperty()->SetInterpolationTypeToLinear();

  renderer->AddVolume(volume);

  // Create probe surface
  double planeWidth = 30.0;
  double lineDistance = 0.0;
  vtkSmartPointer<vtkPolyData> probeSurface = CreateCurvedPlane(planeWidth, lineDistance);

  // Test probe mapper without probe input. The input data is used for probing and rendering.
  vtkNew<vtkOpenGLSurfaceProbeVolumeMapper> probeMapper;
  probeMapper->SetInputData(probeSurface);
  probeMapper->SetSourceData(volumeData);
  probeMapper->SetBlendModeToAverageIntensity();
  probeMapper->SetBlendWidth(10);
  probeMapper->SetWindow(2000);
  probeMapper->SetLevel(2000);

  vtkNew<vtkActor> probeActor;
  probeActor->SetMapper(probeMapper);

  // Test transforms applied to probe surface
  double* c = probeActor->GetCenter();
  vtkNew<vtkTransform> transform;
  transform->Translate(c);
  transform->RotateX(30);
  transform->Translate(-c[0], -c[1], -c[2]);

  probeActor->SetUserTransform(transform);

  renderer->AddActor(probeActor);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(70);
  renderer->GetActiveCamera()->Dolly(1.5);
  renderer->ResetCameraClippingRange();

  // Test probe mapper on straightened plane.
  // The probe input is projected on the input data
  vtkNew<vtkPlaneSource> planeSource;
  planeSource->SetOrigin(0, 0, 0);
  planeSource->SetPoint1(0, lineDistance, 0);
  planeSource->SetPoint2(planeWidth, 0, 0);

  vtkNew<vtkOpenGLSurfaceProbeVolumeMapper> probeMapper2;
  probeMapper2->SetInputConnection(planeSource->GetOutputPort());
  probeMapper2->SetProbeInputData(probeSurface);
  probeMapper2->SetSourceData(volumeData);
  probeMapper2->SetBlendModeToAverageIntensity();
  probeMapper2->SetBlendWidth(10);
  probeMapper2->SetWindow(2000);
  probeMapper2->SetLevel(2000);

  vtkNew<vtkActor> probeActor2;
  probeActor2->SetMapper(probeMapper2);
  probeActor2->SetUserTransform(transform);

  renderer2->AddActor(probeActor2);

  renderer2->GetActiveCamera()->SetViewUp(1, 0, 0);
  renderer2->GetActiveCamera()->Yaw(210);
  renderer2->ResetCamera();
  renderer2->GetActiveCamera()->Dolly(3);
  renderer2->ResetCameraClippingRange();

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
