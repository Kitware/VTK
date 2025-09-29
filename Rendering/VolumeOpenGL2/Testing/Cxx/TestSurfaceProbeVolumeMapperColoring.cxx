// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
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
#include "vtkRendererCollection.h"
#include "vtkSplineFilter.h"
#include "vtkTestUtilities.h"
#include "vtkVolume16Reader.h"

namespace
{

vtkSmartPointer<vtkPolyData> CreateCurvedPlane(double planeWidth, double& curveLength)
{
  // Create centerline polydata
  vtkNew<vtkPoints> linePoints;
  linePoints->InsertNextPoint(70.0, 105.0, 60.0);
  linePoints->InsertNextPoint(95.0, 165.0, 60.0);
  linePoints->InsertNextPoint(125.0, 105.0, 60.0);

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

//------------------------------------------------------------------------------
void TestTranslucentColorTransferFunction(
  vtkRenderer* renderer, vtkPolyData* input, vtkPolyData* probe, vtkImageData* volumeData)
{
  double* rng = volumeData->GetScalarRange();
  double window = 0.5 * (rng[1] - rng[0]);
  double level = rng[0] + 0.25 * (rng[1] + rng[0]);

  vtkNew<vtkDiscretizableColorTransferFunction> colorFunction;
  colorFunction->AddRGBPoint(rng[0], 0.0, 0.0, 1.0);
  colorFunction->AddRGBPoint(rng[1], 1.0, 0.0, 0.0);
  vtkNew<vtkPiecewiseFunction> scalarOpacityFunction;
  scalarOpacityFunction->AddPoint(rng[0], 0.0);
  scalarOpacityFunction->AddPoint(rng[1], 1.0);
  colorFunction->SetScalarOpacityFunction(scalarOpacityFunction);
  colorFunction->EnableOpacityMappingOn();

  vtkNew<vtkOpenGLSurfaceProbeVolumeMapper> probeMapper;
  probeMapper->SetInputData(input);
  probeMapper->SetProbeInputData(probe);
  probeMapper->SetSourceData(volumeData);
  probeMapper->SetWindow(window);
  probeMapper->SetLevel(level);
  probeMapper->SetLookupTable(colorFunction);

  vtkNew<vtkActor> probeActor;
  probeActor->SetMapper(probeMapper);

  renderer->AddActor(probeActor);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Dolly(3);
  renderer->ResetCameraClippingRange();
}

//------------------------------------------------------------------------------
void TestLUTRange(
  vtkRenderer* renderer, vtkPolyData* input, vtkPolyData* probe, vtkImageData* volumeData)
{
  double* rng = volumeData->GetScalarRange();
  double window = 0.5 * (rng[1] - rng[0]);
  double level = rng[0] + 0.25 * (rng[1] + rng[0]);

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(255);
  lut->SetRange(level - 0.5 * window, level + 0.5 * window);
  lut->Build();

  vtkNew<vtkOpenGLSurfaceProbeVolumeMapper> probeMapper;
  probeMapper->SetInputData(input);
  probeMapper->SetProbeInputData(probe);
  probeMapper->SetSourceData(volumeData);
  probeMapper->SetLookupTable(lut);
  probeMapper->UseLookupTableScalarRangeOn();

  vtkNew<vtkActor> probeActor;
  probeActor->SetMapper(probeMapper);

  renderer->AddActor(probeActor);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Dolly(3);
  renderer->ResetCameraClippingRange();
}

//------------------------------------------------------------------------------
void TestRGBSource(
  vtkRenderer* renderer, vtkPolyData* input, vtkPolyData* probe, vtkImageData* volumeData)
{
  double* rng = volumeData->GetScalarRange();

  vtkNew<vtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBPoint(rng[0], 1.0, 1.0, 0.0);
  colorFunction->AddRGBPoint(rng[1], 0.0, 1.0, 0.0);

  vtkNew<vtkImageMapToColors> imageMap;
  imageMap->SetInputData(volumeData);
  imageMap->SetOutputFormatToRGB();
  imageMap->SetLookupTable(colorFunction);
  imageMap->SetEnableSMP(false);
  imageMap->Update();

  vtkNew<vtkOpenGLSurfaceProbeVolumeMapper> probeMapper;
  probeMapper->SetInputData(input);
  probeMapper->SetProbeInputData(probe);
  probeMapper->SetSourceData(imageMap->GetOutput());

  vtkNew<vtkActor> probeActor;
  probeActor->SetMapper(probeMapper);

  renderer->AddActor(probeActor);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Dolly(3);
  renderer->ResetCameraClippingRange();
}

}

//------------------------------------------------------------------------------
int TestSurfaceProbeVolumeMapperColoring(int argc, char* argv[])
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

  // Create probe surface
  double planeWidth = 30.0;
  double lineDistance = 0.0;
  vtkSmartPointer<vtkPolyData> probeSurface = CreateCurvedPlane(planeWidth, lineDistance);

  // Create input
  vtkNew<vtkPlaneSource> planeSource;
  planeSource->SetOrigin(0, 0, 0);
  planeSource->SetPoint1(0, lineDistance, 0);
  planeSource->SetPoint2(planeWidth, 0, 0);
  planeSource->Update();

  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  int nbRenderers = 3;
  for (int i = 0; i < nbRenderers; i++)
  {
    double renSizeY = 1.0 / nbRenderers;
    vtkNew<vtkRenderer> renderer;
    renderWindow->AddRenderer(renderer);
    renderer->SetBackground(1, 1, 1);
    renderer->SetViewport(0.0, i * renSizeY, 1, (i + 1) * renSizeY);
    renderer->GetActiveCamera()->SetViewUp(1, 0, 0);
  }

  // 0. Translucent ColorTransferFunction with window/level
  vtkRenderer* ren0 = vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(0));
  TestTranslucentColorTransferFunction(ren0, planeSource->GetOutput(), probeSurface, volumeData);

  // 1. LUT range
  vtkRenderer* ren1 = vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(1));
  TestLUTRange(ren1, planeSource->GetOutput(), probeSurface, volumeData);

  // 2. RGB data
  vtkRenderer* ren2 = vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(2));
  TestRGBSource(ren2, planeSource->GetOutput(), probeSurface, volumeData);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
