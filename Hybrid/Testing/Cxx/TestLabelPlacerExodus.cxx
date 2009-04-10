/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacerExodus.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkLabelPlacer
// .SECTION Description
// this program tests vtkLabelPlacer which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkColorTransferFunction.h"
#include "vtkExodusReader.h"
#include "vtkGeometryFilter.h"
#include "vtkIntArray.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelHierarchy.h"
#include "vtkLabelPlacer.h"
#include "vtkLabelSizeCalculator.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkTIFFWriter.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkWindowToImageFilter.h"


#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

int TestLabelPlacerExodus(int argc, char *argv[])
  {
  int maxLevels = 5;
  int targetLabels = 32;
  double labelRatio = 0.05;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref.ex2");
  //int iteratorType = vtkLabelHierarchy::FULL_SORT;
  int iteratorType = vtkLabelHierarchy::QUEUE;
  //int iteratorType = vtkLabelHierarchy::DEPTH_FIRST;
  bool showBounds = false;

  vtkSmartPointer<vtkLabelSizeCalculator> labelSizeCalculator = 
    vtkSmartPointer<vtkLabelSizeCalculator>::New();
  vtkSmartPointer<vtkLabelHierarchy> labelHierarchy = 
    vtkSmartPointer<vtkLabelHierarchy>::New();
  vtkSmartPointer<vtkLabelPlacer> labelPlacer = 
    vtkSmartPointer<vtkLabelPlacer>::New();
  vtkSmartPointer<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy = 
    vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  vtkSmartPointer<vtkExodusReader> exoReader =
    vtkSmartPointer<vtkExodusReader>::New();

  vtkSmartPointer<vtkPlane> plane1 = vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkPlane> plane2 = vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkClipDataSet> clip1 = vtkSmartPointer<vtkClipDataSet>::New();
  vtkSmartPointer<vtkClipDataSet> clip2 = vtkSmartPointer<vtkClipDataSet>::New();

  plane1->SetNormal(0.874613683283037, 0.0, -0.484820487411659);
  plane2->SetNormal(-0.483077342911335, 0.875577684026794, 0.0);

  vtkSmartPointer<vtkContourFilter> contour = vtkSmartPointer<vtkContourFilter>::New();

  vtkSmartPointer<vtkColorTransferFunction> contourXfer = vtkSmartPointer<vtkColorTransferFunction>::New();
  vtkSmartPointer<vtkColorTransferFunction> modelXfer = vtkSmartPointer<vtkColorTransferFunction>::New();

  vtkSmartPointer<vtkPolyDataMapper> modelMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkPolyDataMapper> contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> modelActor = vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkActor> contourActor = vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkLabeledDataMapper> labeledMapper = 
    vtkSmartPointer<vtkLabeledDataMapper>::New();
  vtkSmartPointer<vtkActor2D> textActor = vtkSmartPointer<vtkActor2D>::New();
  vtkSmartPointer<vtkCellCenters> cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  vtkSmartPointer<vtkGeometryFilter> geometry1 = vtkSmartPointer<vtkGeometryFilter>::New();
  vtkSmartPointer<vtkGeometryFilter> geometry2 = vtkSmartPointer<vtkGeometryFilter>::New();

  vtkSmartPointer<vtkPolyDataNormals> normals1 = vtkSmartPointer<vtkPolyDataNormals>::New();
  vtkSmartPointer<vtkPolyDataNormals> normals2 = vtkSmartPointer<vtkPolyDataNormals>::New();


  normals1->SplittingOn();
  normals1->ConsistencyOn();
  normals1->NonManifoldTraversalOn();
  normals2->SplittingOn();
  normals2->ConsistencyOn();
  normals2->NonManifoldTraversalOn();


  //xmlPolyDataReader->SetFileName( fname );
  exoReader->SetFileName( fname );
  //exoReader->SetPointArrayStatus("Temp", 1);
  exoReader->SetAllPointArrayStatus(1);
  delete [] fname;

  contour->SetInputConnection(exoReader->GetOutputPort());
  contour->ComputeNormalsOn();
  contour->ComputeGradientsOn();
  contour->ComputeScalarsOn();
  contour->SetValue(0, 362);
  contour->SetValue(1, 500);
  contour->SetValue(2, 638);
  contour->SetValue(3, 775);
  contour->SetValue(4, 844);
  contour->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Temp");

  clip1->SetInputConnection(exoReader->GetOutputPort());
  clip1->SetClipFunction(plane1);
  clip1->InsideOutOn();
  geometry1->SetInputConnection(clip1->GetOutputPort());

  clip2->SetInputConnection(contour->GetOutputPort());
  clip2->SetClipFunction(plane2);
  geometry2->SetInputConnection(clip2->GetOutputPort());

  cellCenters->SetInputConnection(clip2->GetOutputPort());
  cellCenters->Update();
  //cellCenters->GetOutput()->Print(cout);

  //geometry2->Update();
  //geometry2->GetOutput()->Print(cout);

  int numTemps = cellCenters->GetOutput()->GetNumberOfPoints();

  vtkSmartPointer<vtkIntArray> priority = vtkSmartPointer<vtkIntArray>::New();
  priority->SetName("Priority");
  priority->SetNumberOfComponents(1);
  priority->SetNumberOfValues(numTemps);

  for(int i = 0; i < numTemps; i++)
    {
    priority->SetValue(i, static_cast<int>(vtkMath::Random(0.0, 5.0)));
    }

  geometry2->Update();
  vtkSmartPointer<vtkPolyData> temp = vtkSmartPointer<vtkPolyData>::New();
  temp->ShallowCopy(cellCenters->GetOutput());
  temp->GetPointData()->AddArray(priority);
  //temp->GetPointData()->AddArray(geometry2->GetOutput()->GetPointData()->GetArray("Temp"));
  //temp-
  //temp->Print(cout);

  /// Label ///

  labelSizeCalculator->SetInput(temp);
  labelSizeCalculator->GetFontProperty()->SetFontSize( 14 );
  labelSizeCalculator->GetFontProperty()->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  labelSizeCalculator->GetFontProperty()->ShadowOn();
  labelSizeCalculator->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PedigreeElementId" );

  pointSetToLabelHierarchy->AddInputConnection(labelSizeCalculator->GetOutputPort());
  pointSetToLabelHierarchy->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority" );
  pointSetToLabelHierarchy->SetInputArrayToProcess( 1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize" );
  pointSetToLabelHierarchy->SetInputArrayToProcess( 2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PedigreeElementId" );
  pointSetToLabelHierarchy->SetMaximumDepth( maxLevels );
  pointSetToLabelHierarchy->SetTargetLabelCount( targetLabels );

  labelPlacer->SetInputConnection( pointSetToLabelHierarchy->GetOutputPort() );
  labelPlacer->SetIteratorType( iteratorType );
  labelPlacer->SetOutputTraversedBounds( showBounds );
  labelPlacer->SetRenderer( renderer );
  labelPlacer->SetMaximumLabelFraction( labelRatio );
  labelPlacer->UseDepthBufferOn();

  labeledMapper->SetInputConnection(labelPlacer->GetOutputPort());
  labeledMapper->SetLabelTextProperty(labelSizeCalculator->GetFontProperty());
  labeledMapper->SetFieldDataName("LabelText");
  labeledMapper->SetLabelModeToLabelFieldData();
  labeledMapper->GetLabelTextProperty()->SetColor(1.0, 1.0, 1.0);
  textActor->SetMapper(labeledMapper);

  /// End Label ///

  normals1->SetInputConnection(geometry1->GetOutputPort());

  modelXfer->SetColorSpaceToDiverging();
  modelXfer->AddRGBPoint(0.08, 0.138094, 0.241093, 0.709102);
  modelXfer->AddRGBPoint(0.18, 0.672801, 0.140795, 0.126604);
  modelXfer->SetScaleToLinear();
  modelXfer->Build();

  modelMapper->SetInputConnection(normals1->GetOutputPort());
  modelMapper->SelectColorArray("AsH3");
  modelMapper->ScalarVisibilityOn();
  modelMapper->SetLookupTable(modelXfer);
  modelMapper->UseLookupTableScalarRangeOn();
  modelMapper->SetColorModeToMapScalars();
  modelMapper->ScalarVisibilityOn();
  modelMapper->SetScalarModeToUsePointFieldData();

  modelActor->SetMapper(modelMapper);

  contourXfer->SetColorSpaceToRGB();
  contourXfer->AddRGBPoint(293.0, 0.0, 0.666667, 0.0);
  contourXfer->AddRGBPoint(913.5, 0.67451, 0.443137, 0.113725);
  contourXfer->SetScaleToLinear();
  contourXfer->Build();

  normals2->SetInputConnection(geometry2->GetOutputPort());

  contourMapper->SetInputConnection(normals2->GetOutputPort());
  contourMapper->SelectColorArray("Temp");
  contourMapper->ScalarVisibilityOn();
  contourMapper->SetLookupTable(contourXfer);
  contourMapper->UseLookupTableScalarRangeOn();
  contourMapper->SetColorModeToMapScalars();
  contourMapper->ScalarVisibilityOn();
  contourMapper->SetScalarModeToUsePointFieldData();

  contourActor->SetMapper(contourMapper);
  modelActor->SetPosition(0.05, -0.05, 0.0);

  renderer->AddActor(contourActor);
  renderer->AddActor(modelActor);
  renderer->AddActor(textActor);


  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(1.0, 1.0, 1.0);
  iren->SetRenderWindow(renWin);

  renderer->GetActiveCamera()->SetFocalPoint(-9.25157, 7.70629, 3.69546);
  renderer->GetActiveCamera()->SetPosition(24.9979, -27.946, -4.03877);
  renderer->GetActiveCamera()->SetViewAngle(30);
  renderer->GetActiveCamera()->SetViewUp(0.248261, 0.427108, -8.869451);

  renWin->Render();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renderer->ResetCamera();

  vtkSmartPointer<vtkWindowToImageFilter> capture = vtkSmartPointer<vtkWindowToImageFilter>::New();

  //capture->SetInput(renWin);
  //char buffer[1024];

  //vtkSmartPointer<vtkTIFFWriter> tiffWriter = vtkSmartPointer<vtkTIFFWriter>::New();
  //tiffWriter->SetInputConnection(capture->GetOutputPort());
  //tiffWriter->SetCompressionToNoCompression();

  int j = 0;

  /*for(j = 0; j < 20; j++)
  {
  renderer->GetActiveCamera()->Azimuth(-0.5);
  renWin->Render();

  capture->Modified();
  sprintf(buffer, "%04d.tiff", j );

  tiffWriter->SetFileName(buffer);
  tiffWriter->Write();
  }*/

  for(; j < 80; j++)
    {
    renderer->GetActiveCamera()->Zoom(1.01);
    renWin->Render();

    //capture->Modified();
    //sprintf(buffer, "%04d.tiff", j );

    //tiffWriter->SetFileName(buffer);
    //tiffWriter->Write();
    }

  for(; j < 400; j++)
    {
    renderer->GetActiveCamera()->Azimuth(-0.25);
    renWin->Render();

    //capture->Modified();
    //sprintf(buffer, "%04d.tiff", j );

    //tiffWriter->SetFileName(buffer);
    //tiffWriter->Write();
    }

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
  }
