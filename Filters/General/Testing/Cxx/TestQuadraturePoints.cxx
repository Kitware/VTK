// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This example demonstrates the capabilities of vtkQuadraturePointInterpolator
// vtkQuadraturePointsGenerator and the class required to support their
// addition.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellDataToPointData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExtractGeometry.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkQuadraturePointInterpolator.h"
#include "vtkQuadraturePointsGenerator.h"
#include "vtkQuadratureSchemeDictionaryGenerator.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSetGet.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkWarpVector.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkSmartPointer.h"
#include <cstdlib>
#include <string>

// Generate a vector to warp by.
int GenerateWarpVector(vtkDataSet*);
// Generate a scalar to threshold by.
int GenerateThresholdScalar(vtkDataSet*);

int PipelineQuadraturePointsUG(vtkTesting* testHelper)
{
  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDir = testHelper->GetTempDirectory();
  std::string inputFileName = dataRoot + "/Data/Quadratic/CylinderQuadratic.vtk";
  std::string tempFile = tempDir + "/tmp.vtu";

  // Read, xml or legacy file.
  vtkDataSet* input = nullptr;
  vtkNew<vtkXMLUnstructuredGridReader> xusgr;
  xusgr->SetFileName(inputFileName.c_str());

  vtkNew<vtkUnstructuredGridReader> lusgr;
  lusgr->SetFileName(inputFileName.c_str());
  if (xusgr->CanReadFile(inputFileName.c_str()))
  {
    input = xusgr->GetOutput();
    xusgr->Update();
  }
  else if (lusgr->IsFileValid("unstructured_grid"))
  {
    lusgr->SetFileName(inputFileName.c_str());
    input = lusgr->GetOutput();
    lusgr->Update();
  }

  if (input == nullptr)
  {
    std::cerr << "Error: Could not read file " << inputFileName << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Add a couple arrays to be used in the demonstrations.
  int warpIdx = GenerateWarpVector(input);
  std::string warpName = input->GetPointData()->GetArray(warpIdx)->GetName();
  int threshIdx = GenerateThresholdScalar(input);
  std::string threshName = input->GetPointData()->GetArray(threshIdx)->GetName();

  // Add a quadrature scheme dictionary to the data set. This filter is
  // solely for our convenience. Typically we would expect that users
  // provide there own in XML format and use the readers or to generate
  // them on the fly.
  vtkNew<vtkQuadratureSchemeDictionaryGenerator> dictGen;
  dictGen->SetInputData(input);

  // Interpolate fields to the quadrature points. This generates new field data
  // arrays, but not a set of points.
  vtkNew<vtkQuadraturePointInterpolator> fieldInterp;
  fieldInterp->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  fieldInterp->SetInputConnection(dictGen->GetOutputPort());

  // Write the dataset as XML. This exercises the information writer.
  vtkNew<vtkXMLUnstructuredGridWriter> xusgw;
  xusgw->SetFileName(tempFile.c_str());
  xusgw->SetInputConnection(fieldInterp->GetOutputPort());
  xusgw->Write();

  // Read the data back in form disk. This exercises the information reader.
  xusgr->SetFileName(tempFile.c_str());
  xusgr->Update();

  input = xusgr->GetOutput();
  input->Register(nullptr);
  input->GetPointData()->SetActiveVectors(warpName.c_str());
  input->GetPointData()->SetActiveScalars(threshName.c_str());

  // Demonstrate warp by vector.
  vtkNew<vtkWarpVector> warper;
  warper->SetInputData(input);
  warper->SetScaleFactor(0.02);
  input->Delete();

  // Demonstrate clip functionality.
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.0, 0.0, 0.03);
  plane->SetNormal(0.0, 0.0, -1.0);
  vtkNew<vtkExtractGeometry> clip;
  clip->SetImplicitFunction(plane);
  clip->SetInputConnection(warper->GetOutputPort());

  // Demonstrate threshold functionality.
  vtkNew<vtkThreshold> thresholder;
  thresholder->SetInputConnection(clip->GetOutputPort());
  thresholder->SetThresholdFunction(vtkThreshold::THRESHOLD_BETWEEN);
  thresholder->SetLowerThreshold(0.0);
  thresholder->SetUpperThreshold(3.0);

  // Generate the quadrature point set using a specific array as point data.
  vtkNew<vtkQuadraturePointsGenerator> pointGen;
  pointGen->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  pointGen->SetInputConnection(thresholder->GetOutputPort());
  vtkPolyData* output = vtkPolyData::SafeDownCast(pointGen->GetOutput());
  pointGen->Update();
  const char* activeScalars = "pressure";
  output->GetPointData()->SetActiveScalars(activeScalars);

  // Glyph the point set.
  vtkNew<vtkSphereSource> ss;
  ss->SetRadius(0.0008);
  vtkNew<vtkGlyph3D> glyphs;
  glyphs->SetInputConnection(pointGen->GetOutputPort());
  glyphs->SetSourceConnection(ss->GetOutputPort());
  glyphs->ScalingOff();
  glyphs->SetColorModeToColorByScalar();
  // Map the glyphs.
  vtkNew<vtkPolyDataMapper> pdmQPts;
  pdmQPts->SetInputConnection(glyphs->GetOutputPort());
  pdmQPts->SetColorModeToMapScalars();
  pdmQPts->SetScalarModeToUsePointData();
  if (output->GetPointData()->GetArray(0) == nullptr)
  {
    vtkGenericWarningMacro(<< "no point data in output of vtkQuadraturePointsGenerator");
    return EXIT_FAILURE;
  }
  pdmQPts->SetScalarRange(output->GetPointData()->GetArray(activeScalars)->GetRange());
  vtkNew<vtkActor> outputActor;
  outputActor->SetMapper(pdmQPts);

  // Extract the surface of the warped input, for reference.
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(warper->GetOutputPort());
  // Map the warped surface.
  vtkNew<vtkPolyDataMapper> pdmWSurf;
  pdmWSurf->SetInputConnection(surface->GetOutputPort());
  pdmWSurf->ScalarVisibilityOff();
  vtkNew<vtkActor> surfaceActor;
  surfaceActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  surfaceActor->GetProperty()->SetRepresentationToSurface();
  surfaceActor->SetMapper(pdmWSurf);
  // Setup left render pane.
  vtkCamera* camera = nullptr;
  vtkNew<vtkRenderer> ren0;
  ren0->SetViewport(0.0, 0.0, 0.5, 1.0);
  ren0->AddActor(outputActor);
  ren0->SetBackground(0.328125, 0.347656, 0.425781);
  ren0->ResetCamera();
  camera = ren0->GetActiveCamera();
  camera->Elevation(95.0);
  camera->SetViewUp(0.0, 0.0, 1.0);
  camera->Azimuth(180.0);

  // Setup upper right pane.
  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0.5, 0.5, 1.0, 1.0);
  ren1->AddActor(outputActor);
  ren1->AddActor(surfaceActor);
  ren1->SetBackground(0.328125, 0.347656, 0.425781);
  ren1->ResetCamera();
  camera = ren1->GetActiveCamera();
  camera->Elevation(-85.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-5.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-10.0);
  camera->Azimuth(55.0);

  // Setup lower right pane.
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 0.5);
  ren2->AddActor(outputActor);
  ren2->SetBackground(0.328125, 0.347656, 0.425781);
  ren2->AddActor(surfaceActor);
  ren2->ResetCamera();

  // If interactive mode then we show wireframes for
  // reference.
  if (testHelper->IsInteractiveModeSpecified())
  {
    surfaceActor->GetProperty()->SetOpacity(1.0);
    surfaceActor->GetProperty()->SetRepresentationToWireframe();
  }
  // Render window
  vtkNew<vtkRenderWindow> renwin;
  renwin->AddRenderer(ren0);
  renwin->AddRenderer(ren1);
  renwin->AddRenderer(ren2);
  renwin->SetSize(800, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int PipelineQuadraturePointsImageData(vtkTesting* testHelper)
{
  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDir = testHelper->GetTempDirectory();
  std::string inputFileName = dataRoot + "/Data/2DScalar.vti";
  std::string tempFile = tempDir + "/tmp.vti";

  // Read, xml or legacy file.
  vtkDataSet* input = nullptr;
  vtkNew<vtkXMLImageDataReader> xImageReader;
  xImageReader->SetFileName(inputFileName.c_str());
  if (xImageReader->CanReadFile(inputFileName.c_str()))
  {
    input = xImageReader->GetOutput();
    xImageReader->Update();
  }

  if (input == nullptr)
  {
    std::cerr << "Error: Could not read file " << inputFileName << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Add a quadrature scheme dictionary to the data set. This filter is
  // solely for our convenience. Typically we would expect that users
  // provide there own in XML format and use the readers or to generate
  // them on the fly.
  vtkNew<vtkQuadratureSchemeDictionaryGenerator> dictGen;
  dictGen->SetInputData(input);

  // Interpolate fields to the quadrature points. This generates new field data
  // arrays, but not a set of points.
  vtkNew<vtkQuadraturePointInterpolator> fieldInterp;
  fieldInterp->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  fieldInterp->SetInputConnection(dictGen->GetOutputPort());

  // Write the dataset as XML. This exercises the information writer.
  vtkNew<vtkXMLImageDataWriter> xImageWriter;
  xImageWriter->SetFileName(tempFile.c_str());
  xImageWriter->SetInputConnection(fieldInterp->GetOutputPort());
  xImageWriter->Write();

  // Read the data back in form disk. This exercises the information reader.
  xImageReader->SetFileName(tempFile.c_str());
  xImageReader->Update();

  // Generate the quadrature point set using a specific array as point data.
  vtkNew<vtkQuadraturePointsGenerator> pointGen;
  pointGen->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  pointGen->SetInputConnection(xImageReader->GetOutputPort());
  pointGen->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(pointGen->GetOutput());

  if (output->GetPointData()->GetArray("values")->GetTuple1(0) != -0.22766)
  {
    std::cerr << "Read interpolated value failed." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestQuadraturePoints(int argc, char* argv[])
{
  vtkSmartPointer<vtkTesting> testHelper = vtkSmartPointer<vtkTesting>::New();
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  auto status = PipelineQuadraturePointsUG(testHelper);
  if (status != EXIT_SUCCESS)
  {
    vtkGenericWarningMacro(<< "Pipeline with UnstructuredGrid failed.");
    return status;
  }

  status = PipelineQuadraturePointsImageData(testHelper);
  if (status != EXIT_SUCCESS)
  {
    vtkGenericWarningMacro(<< "Pipeline with ImageData failed.");
    return status;
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int GenerateWarpVector(vtkDataSet* dataset)
{
  vtkDoubleArray* pts = vtkArrayDownCast<vtkDoubleArray>(dataset->GetPoints()->GetData());

  vtkIdType nTups = dataset->GetPointData()->GetArray(0)->GetNumberOfTuples();

  double ptsBounds[6];
  dataset->GetPoints()->GetBounds(ptsBounds);
  double zmax = ptsBounds[5];
  double zmin = ptsBounds[4];
  double zmid = (zmax + zmin) / 4.0;

  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  int idx = dataset->GetPointData()->AddArray(da); // note: returns the index.
  da->SetName("warp");
  da->SetNumberOfComponents(3);
  da->SetNumberOfTuples(nTups);
  double* pda = da->GetPointer(0);
  double* ppts = pts->GetPointer(0);
  for (vtkIdType i = 0; i < nTups; ++i)
  {
    double zs = (ppts[2] - zmid) / (zmax - zmid); // move z to -1 to 1
    double fzs = zs * zs * zs;                    // z**3
    double r[2];                                  // radial vector
    r[0] = ppts[0];
    r[1] = ppts[1];
    double modR = sqrt(r[0] * r[0] + r[1] * r[1]);
    r[0] /= modR; // unit radial vector
    r[0] *= fzs;  // scale by z**3 in -1 to 1
    r[1] /= modR;
    r[1] *= fzs;
    pda[0] = r[0]; // copy into result
    pda[1] = r[1];
    pda[2] = 0.0;
    pda += 3; // next
    ppts += 3;
  }
  return idx;
}
//------------------------------------------------------------------------------
int GenerateThresholdScalar(vtkDataSet* dataset)
{
  vtkDoubleArray* pts = vtkArrayDownCast<vtkDoubleArray>(dataset->GetPoints()->GetData());

  vtkIdType nTups = dataset->GetPointData()->GetArray(0)->GetNumberOfTuples();

  double ptsBounds[6];
  dataset->GetPoints()->GetBounds(ptsBounds);
  double zmax = ptsBounds[5];
  double zmin = ptsBounds[4];
  double zmid = (zmax + zmin) / 4.0;

  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  int idx = dataset->GetPointData()->AddArray(da); // note: returns the index.
  da->SetName("threshold");
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(nTups);
  double* pda = da->GetPointer(0);
  double* ppts = pts->GetPointer(0);
  for (vtkIdType i = 0; i < nTups; ++i)
  {
    double zs = (ppts[2] - zmid) / (zmax - zmid); // move z to -1 to 1
    double fzs = zs * zs * zs;                    // z**3
    double r[2];                                  // radial vector
    r[0] = ppts[0];
    r[1] = ppts[1];
    double modR = sqrt(r[0] * r[0] + r[1] * r[1]);
    r[1] /= modR; // scale by z**3 in -1 to 1
    r[1] *= fzs;
    pda[0] = r[1]; // copy into result
    pda += 1;      // next
    ppts += 3;
  }
  return idx;
}
