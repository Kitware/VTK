/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacerCoincidentPoints.cxx

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
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelHierarchy.h"
#include "vtkLabelPlacer.h"
#include "vtkLabelSizeCalculator.h"
#include "vtkMath.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"

#include "vtkSphereSource.h"


#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

/*
void prtbds( const char* msg, const double* bds )
{
  cout << msg << ":";
  for ( int i = 0; i < 3; ++ i )
    cout << " [ " << bds[2*i] << ", " << bds[2*i+1] << "]";
  cout << "\n";
}
*/

int TestLabelPlacerCoincidentPoints(int argc, char *argv[])
{
  int maxLevels = 5;
  int targetLabels = 7;
  double labelRatio = 1.0;
  int i = 0;
  int iteratorType = vtkLabelHierarchy::QUEUE;
  bool showBounds = true;

  vtkSmartPointer<vtkLabelSizeCalculator> labelSizeCalculator =
    vtkSmartPointer<vtkLabelSizeCalculator>::New();
  vtkSmartPointer<vtkLabelHierarchy> labelHierarchy =
    vtkSmartPointer<vtkLabelHierarchy>::New();
  vtkSmartPointer<vtkLabelPlacer> labelPlacer =
    vtkSmartPointer<vtkLabelPlacer>::New();
  vtkSmartPointer<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy =
    vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();

  vtkSmartPointer<vtkPolyDataMapper> polyDataMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkPolyDataMapper> polyDataMapper2 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> actor2 =
    vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0); // ensure to have the same test image everywhere

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkLabeledDataMapper> labeledMapper =
    vtkSmartPointer<vtkLabeledDataMapper>::New();
  vtkSmartPointer<vtkActor2D> textActor =
    vtkSmartPointer<vtkActor2D>::New();

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  //renderer->GetActiveCamera()->ParallelProjectionOn();

  vtkMath::RandomSeed(5678);

  for(i = 0; i < 29; i++)
  {
    //points->InsertPoint( i, vtkMath::Random(-1.0, 1.0), vtkMath::Random(-1.0, 1.0), 0. );
    points->InsertPoint( i, 0.0, 0.0, 0.0 );
  }
  points->InsertPoint( 29, 2.2, 2.2, 0.0 );

  vtkSmartPointer<vtkCellArray> cells =
    vtkSmartPointer<vtkCellArray>::New();

  cells->InsertNextCell(30);
  for(i = 0; i < 30; i++)
  {
    cells->InsertCellPoint(i);
  }

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetVerts(cells);

  vtkSmartPointer<vtkStringArray> stringData =
    vtkSmartPointer<vtkStringArray>::New();
  stringData->SetName("PlaceNames");
  stringData->InsertNextValue("Abu Dhabi");
  stringData->InsertNextValue("Amsterdam");
  stringData->InsertNextValue("Beijing");
  stringData->InsertNextValue("Berlin");
  stringData->InsertNextValue("Cairo");
  stringData->InsertNextValue("Caracas");
  stringData->InsertNextValue("Dublin");
  stringData->InsertNextValue("Georgetown");
  stringData->InsertNextValue("The Hague");
  stringData->InsertNextValue("Hanoi");
  stringData->InsertNextValue("Islamabad");
  stringData->InsertNextValue("Jakarta");
  stringData->InsertNextValue("Kiev");
  stringData->InsertNextValue("Kingston");
  stringData->InsertNextValue("Lima");
  stringData->InsertNextValue("London");
  stringData->InsertNextValue("Luxembourg City");
  stringData->InsertNextValue("Madrid");
  stringData->InsertNextValue("Moscow");
  stringData->InsertNextValue("Nairobi");
  stringData->InsertNextValue("New Delhi");
  stringData->InsertNextValue("Ottawa");
  stringData->InsertNextValue("Paris");
  stringData->InsertNextValue("Prague");
  stringData->InsertNextValue("Rome");
  stringData->InsertNextValue("Seoul");
  stringData->InsertNextValue("Tehran");
  stringData->InsertNextValue("Tokyo");
  stringData->InsertNextValue("Warsaw");
  stringData->InsertNextValue("Washington");

  polyData->GetPointData()->AddArray(stringData);

  labelSizeCalculator->SetInputData(polyData);
  labelSizeCalculator->GetFontProperty()->SetFontSize( 12 );
  labelSizeCalculator->GetFontProperty()->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  labelSizeCalculator->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames" );
  labelSizeCalculator->SetLabelSizeArrayName( "LabelSize" );

  pointSetToLabelHierarchy->AddInputConnection(labelSizeCalculator->GetOutputPort());
  pointSetToLabelHierarchy->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority" );
  pointSetToLabelHierarchy->SetInputArrayToProcess( 1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize" );
  pointSetToLabelHierarchy->SetInputArrayToProcess( 2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames" );
  pointSetToLabelHierarchy->SetMaximumDepth( maxLevels );
  pointSetToLabelHierarchy->SetTargetLabelCount( targetLabels );

  labelPlacer->SetInputConnection( pointSetToLabelHierarchy->GetOutputPort() );
  labelPlacer->SetIteratorType( iteratorType );
  labelPlacer->SetOutputTraversedBounds( showBounds );
  labelPlacer->SetRenderer( renderer );
  labelPlacer->SetMaximumLabelFraction( labelRatio );
  //labelPlacer->SetIteratorType(1); // Quadtree is only available type for 2-D.

  polyDataMapper->SetInputData(polyData);
  //polyDataMapper2->SetInputConnection(labelPlacer->GetOutputPort(2));

  actor->SetMapper(polyDataMapper);
  //actor2->SetMapper(polyDataMapper2);

  //labelPlacer->Update();

  labeledMapper->SetInputConnection(labelPlacer->GetOutputPort());
  labeledMapper->SetLabelTextProperty(labelSizeCalculator->GetFontProperty());
  labeledMapper->SetFieldDataName("LabelText");
  labeledMapper->SetLabelModeToLabelFieldData();
  labeledMapper->GetLabelTextProperty()->SetColor(0.0, 0.8, 0.2);
  textActor->SetMapper(labeledMapper);

  //renderer->AddActor(actor);
  //renderer->AddActor(actor2);
  renderer->AddActor(textActor);

  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  iren->SetRenderWindow(renWin);

  //labelPlacer->Update();
  //cout << "Pre-reset-camera bounds of...\n";
  //prtbds( "output 0", labelPlacer->GetOutput( 0 )->GetBounds() );
  //prtbds( "output 1", labelPlacer->GetOutput( 1 )->GetBounds() );
  //prtbds( "output 2", labelPlacer->GetOutput( 2 )->GetBounds() );
  /*
  renWin->Render();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renWin->Render();
  */
  //cout << "Post-reset-camera Bounds of...\n";
  //prtbds( "output 0", labelPlacer->GetOutput( 0 )->GetBounds() );
  //prtbds( "output 1", labelPlacer->GetOutput( 1 )->GetBounds() );
  //prtbds( "output 2", labelPlacer->GetOutput( 2 )->GetBounds() );
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
