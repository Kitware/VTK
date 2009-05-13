/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadraturePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates the capabilities of vtkQuadraturePointInterpolator
// vtkQuadraturePointsGenerator and the class required to suppport their
// addition.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkQuadratureSchemeDictionaryGenerator.h"
#include "vtkQuadraturePointInterpolator.h"
#include "vtkQuadraturePointsGenerator.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkGlyph3D.h"
#include "vtkSphereSource.h"
#include "vtkDoubleArray.h"
#include "vtkWarpVector.h"
#include "vtkExtractGeometry.h"
#include "vtkThreshold.h"
#include "vtkPlane.h"
#include "vtkDataObject.h"
#include "vtkProperty.h"
#include "vtkPNGWriter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkDataSetSurfaceFilter.h"

#include "vtkstd/string"
#include "vtkSmartPointer.h"

// Generate a vector to warp by.
int GenerateWarpVector(vtkUnstructuredGrid *usg);
// Generate a scalar to threshold by.
int GenerateThresholdScalar(vtkUnstructuredGrid *usg);

int TestQuadraturePoints(int argc,char *argv[])
{
  vtkSmartPointer<vtkTesting> testHelper = vtkSmartPointer<vtkTesting>::New();
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  if (!testHelper->IsFlagSpecified("-D"))
    {
    cerr << "Error: -D /path/to/data was not specified.";
    return 1;
    }
  vtkstd::string dataRoot=testHelper->GetDataRoot();
  vtkstd::string tempDir=testHelper->GetTempDirectory();
  vtkstd::string inputFileName=dataRoot+"/Data/Quadratic/CylinderQuadratic.vtk";
  vtkstd::string tempFile=tempDir+"/tmp.vtu";
  vtkstd::string tempBaseline=tempDir+"/TestQuadraturePoints.png";

  // Raed, xml or legacy file.
  vtkUnstructuredGrid *input=0;
  vtkSmartPointer<vtkXMLUnstructuredGridReader> xusgr = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  xusgr->SetFileName(inputFileName.c_str());
  vtkSmartPointer<vtkUnstructuredGridReader> lusgr = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  lusgr->SetFileName(inputFileName.c_str());
  if (xusgr->CanReadFile(inputFileName.c_str()))
    {
    input=xusgr->GetOutput();
    input->Update();
    }
  else if (lusgr->IsFileValid("unstructured_grid"))
    {
    lusgr->SetFileName(inputFileName.c_str());
    input=lusgr->GetOutput();
    input->Update();
    }
  if (input==0)
    {
    cerr << "Error: Could not read file " << inputFileName << "." << endl;
    return 1;
    }

  // Add a couple arrays to be used in the demonstrations.
  int warpIdx=GenerateWarpVector(input);
  vtkstd::string warpName=input->GetPointData()->GetArray(warpIdx)->GetName();
  int threshIdx=GenerateThresholdScalar(input);
  vtkstd::string threshName=input->GetPointData()->GetArray(threshIdx)->GetName();

  // Add a quadrature scheme dictionary to the data set. This filter is
  // solely for our convinience. Typically we would expect that users
  // provide there own in XML format and use the readers or to generate
  // them on the fly.
  vtkSmartPointer<vtkQuadratureSchemeDictionaryGenerator> dictGen =
    vtkSmartPointer<vtkQuadratureSchemeDictionaryGenerator>::New();
  dictGen->SetInput(input);

  // Interpolate fields to the quadrature points. This generates new field data
  // arrays, but not a set of points.
  vtkSmartPointer<vtkQuadraturePointInterpolator> fieldInterp = 
    vtkSmartPointer<vtkQuadraturePointInterpolator>::New();
  fieldInterp->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  fieldInterp->SetInputConnection(dictGen->GetOutputPort());

  // Write the dataset as XML. This excercises the information writer.
  /*vtkXMLUnstructuredGridWriter *xusgw=vtkXMLUnstructuredGridWriter::New();
  xusgw->SetFileName(tempFile.c_str());
  xusgw->SetInput(fieldInterp->GetOutput());
  fieldInterp->Delete();
  xusgw->Write();
  xusgw->Delete();

  // Read the data back in form disk. This excercises the information reader.
  xusgr=vtkXMLUnstructuredGridReader::New();
  xusgr->SetFileName(tempFile.c_str());
  input=xusgr->GetOutput();*/
  input = vtkUnstructuredGrid::SafeDownCast(fieldInterp->GetOutput());
  input->Update();
  input->GetPointData()->SetActiveVectors(warpName.c_str());
  input->GetPointData()->SetActiveScalars(threshName.c_str());
 // Demonstrate warp by vector.
  vtkSmartPointer<vtkWarpVector> warper = vtkSmartPointer<vtkWarpVector>::New();
  warper->SetInput(input);
  warper->SetScaleFactor(0.02);

  // Demonstrate clip functionality.
  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0,0.0,0.03);
  plane->SetNormal(0.0,0.0,-1.0);
  vtkSmartPointer<vtkExtractGeometry> clip = vtkSmartPointer<vtkExtractGeometry>::New();
  clip->SetImplicitFunction(plane);
  clip->SetInputConnection(warper->GetOutputPort());

  // Demonstrate threshold functionality.
  vtkSmartPointer<vtkThreshold> thresholder = vtkSmartPointer<vtkThreshold>::New();
  thresholder->SetInputConnection(clip->GetOutputPort());
  thresholder->ThresholdBetween(0.0,3.0);

  // Generate the quadrature point set using a specific array as point data.
  vtkSmartPointer<vtkQuadraturePointsGenerator> pointGen = vtkSmartPointer<vtkQuadraturePointsGenerator>::New();
  pointGen->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  pointGen->SetInputConnection(thresholder->GetOutputPort());
  vtkPolyData *output=vtkPolyData::SafeDownCast(pointGen->GetOutput());
  output->Update();
  const char* activeScalars = "pressure";
  output->GetPointData()->SetActiveScalars(activeScalars);

  // Glyph the point set.
  vtkSmartPointer<vtkSphereSource> ss = vtkSmartPointer<vtkSphereSource>::New();
  ss->SetRadius(0.0008);
  vtkSmartPointer<vtkGlyph3D> glyphs = vtkSmartPointer<vtkGlyph3D>::New();
  glyphs->SetInput(output);
  glyphs->SetSource(ss->GetOutput());
  glyphs->ScalingOff();
  glyphs->SetColorModeToColorByScalar();
  // Map the glyphs.
  vtkSmartPointer<vtkPolyDataMapper> pdmQPts = vtkSmartPointer<vtkPolyDataMapper>::New();
  pdmQPts->SetInputConnection(glyphs->GetOutputPort());
  pdmQPts->SetColorModeToMapScalars();
  pdmQPts->SetScalarModeToUsePointData();
  if(output->GetPointData()->GetArray(0) == NULL)
    {
    vtkGenericWarningMacro( << "no point data in output of vtkQuadraturePointsGenerator" );
    return 0;
    }
  pdmQPts->SetScalarRange(output->GetPointData()->GetArray(activeScalars)->GetRange());
  vtkSmartPointer<vtkActor> outputActor = vtkSmartPointer<vtkActor>::New();
  outputActor->SetMapper(pdmQPts);

  // Extract the surface of the warped input, for reference.
  vtkSmartPointer<vtkDataSetSurfaceFilter> surface = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surface->SetInputConnection(warper->GetOutputPort());
  // Map the warped surface.
  vtkSmartPointer<vtkPolyDataMapper> pdmWSurf = vtkSmartPointer<vtkPolyDataMapper>::New();
  pdmWSurf->SetInputConnection(surface->GetOutputPort());
  pdmWSurf->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> surfaceActor = vtkSmartPointer<vtkActor>::New();
  surfaceActor->GetProperty()->SetColor(1.0,1.0,1.0);
  surfaceActor->GetProperty()->SetRepresentationToSurface();
  surfaceActor->SetMapper(pdmWSurf);
  // Setup left render pane.
  vtkCamera *camera=0;
  vtkSmartPointer<vtkRenderer> ren0 = vtkSmartPointer<vtkRenderer>::New();
  ren0->SetViewport(0.0,0.0,0.5,1.0);
  ren0->AddActor(outputActor);
  ren0->SetBackground(0.328125, 0.347656, 0.425781);
  ren0->ResetCamera();
  camera = ren0->GetActiveCamera();
  camera->Elevation(95.0);
  camera->SetViewUp(0.0,0.0,1.0);
  camera->Azimuth(180.0);
  camera=0;
  // Setup upper right pane.
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  ren1->SetViewport(0.5,0.5,1.0,1.0);
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
  camera=0;
  // Setup lower right pane.
  vtkSmartPointer<vtkRenderer> ren2 = vtkSmartPointer<vtkRenderer>::New();
  ren2->SetViewport(0.5,0.0,1.0,0.5);
  ren2->AddActor(outputActor);
  ren2->SetBackground(0.328125, 0.347656, 0.425781);
  ren2->AddActor(surfaceActor);
  ren2->ResetCamera();
  camera=0;
  // If interactive mode then we show wireframes for
  // reference.
  if (testHelper->IsInteractiveModeSpecified())
    {
    surfaceActor->GetProperty()->SetOpacity(1.0);
    surfaceActor->GetProperty()->SetRepresentationToWireframe();
    }
  // Render window
  vtkSmartPointer<vtkRenderWindow> renwin = vtkSmartPointer<vtkRenderWindow>::New();
  renwin->AddRenderer(ren0);
  renwin->AddRenderer(ren1);
  renwin->AddRenderer(ren2);
  renwin->SetSize(800,600);

  // Perform the regression test.
  int failFlag=1;
  // Perform test.
  failFlag=vtkTesting::Test(argc, argv, renwin, 5.0);
  if (failFlag==vtkTesting::DO_INTERACTOR)
    {
    // Not testing, interact with scene.
    vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    }
  else
    {
    // Save a baseline
    vtkSmartPointer<vtkWindowToImageFilter> baselineImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
    baselineImage->SetInput(renwin);
    vtkSmartPointer<vtkPNGWriter> baselineWriter = vtkSmartPointer<vtkPNGWriter>::New();
    baselineWriter->SetFileName(tempBaseline.c_str());
    baselineWriter->SetInputConnection(baselineImage->GetOutputPort());
    baselineWriter->Write();
    }
  failFlag=failFlag==vtkTesting::PASSED?0:1;

  return failFlag;
}

//-----------------------------------------------------------------------------
int GenerateWarpVector(vtkUnstructuredGrid *usg)
{
  vtkDoubleArray *pts
    = vtkDoubleArray::SafeDownCast(usg->GetPoints()->GetData());

  vtkIdType nTups
    = usg->GetPointData()->GetArray(0)->GetNumberOfTuples();

  double ptsBounds[6];
  usg->GetPoints()->GetBounds(ptsBounds);
  double zmax=ptsBounds[5];
  double zmin=ptsBounds[4];
  double zmid=(zmax+zmin)/4.0;

  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  int idx=usg->GetPointData()->AddArray(da); // note: returns the index.
  da->SetName("warp");
  da->SetNumberOfComponents(3);
  da->SetNumberOfTuples(nTups);
  double *pda=da->GetPointer(0);
  double *ppts=pts->GetPointer(0);
  for (vtkIdType i=0; i<nTups; ++i)
    {
    double zs=(ppts[2]-zmid)/(zmax-zmid); // move z to -1 to 1
    double fzs=zs*zs*zs;                  // z**3
    double r[2];                          // radial vector
    r[0]=ppts[0];
    r[1]=ppts[1];
    double modR=sqrt(r[0]*r[0]+r[1]*r[1]);
    r[0]/=modR;                           // unit radial vector
    r[0]*=fzs;                            // scale by z**3 in -1 to 1
    r[1]/=modR;
    r[1]*=fzs;
    pda[0]=r[0];                          // copy into result
    pda[1]=r[1];
    pda[2]=0.0;
    pda+=3;                               // next
    ppts+=3;
    }
  return idx;
}
//-----------------------------------------------------------------------------
int GenerateThresholdScalar(vtkUnstructuredGrid *usg)
{
  vtkDoubleArray *pts
    = vtkDoubleArray::SafeDownCast(usg->GetPoints()->GetData());

  vtkIdType nTups
    = usg->GetPointData()->GetArray(0)->GetNumberOfTuples();

  double ptsBounds[6];
  usg->GetPoints()->GetBounds(ptsBounds);
  double zmax=ptsBounds[5];
  double zmin=ptsBounds[4];
  double zmid=(zmax+zmin)/4.0;

  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  int idx=usg->GetPointData()->AddArray(da); // note: returns the index.
  da->SetName("threshold");
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(nTups);
  double *pda=da->GetPointer(0);
  double *ppts=pts->GetPointer(0);
  for (vtkIdType i=0; i<nTups; ++i)
    {
    double zs=(ppts[2]-zmid)/(zmax-zmid); // move z to -1 to 1
    double fzs=zs*zs*zs;                  // z**3
    double r[2];                          // radial vector
    r[0]=ppts[0];
    r[1]=ppts[1];
    double modR=sqrt(r[0]*r[0]+r[1]*r[1]);
    r[1]/=modR;                           // scale by z**3 in -1 to 1
    r[1]*=fzs;
    pda[0]=r[1];                          // copy into result
    pda+=1;                               // next
    ppts+=3;
    }
  return idx;
}



