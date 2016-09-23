/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacementMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkLabelPlacementMapper
// .SECTION Description
// this program tests vtkLabelPlacementMapper which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelHierarchy.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"


#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

int TestLabelPlacementMapper(int argc, char *argv[])
{
  int maxLevels = 5;
  int targetLabels = 32;
  double labelRatio = 0.05;
  char* fname = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/uniform-001371-5x5x5.vtp" );
  //int iteratorType = vtkLabelHierarchy::FULL_SORT;
  int iteratorType = vtkLabelHierarchy::QUEUE;
  //int iteratorType = vtkLabelHierarchy::DEPTH_FIRST;

  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> sphereActor =
    vtkSmartPointer<vtkActor>::New();

  sphere->SetRadius( 5.0 );
  sphereMapper->SetInputConnection( sphere->GetOutputPort() );
  sphereActor->SetMapper( sphereMapper );

  vtkSmartPointer<vtkLabelHierarchy> labelHierarchy =
    vtkSmartPointer<vtkLabelHierarchy>::New();
  vtkSmartPointer<vtkLabelPlacementMapper> labelPlacer =
    vtkSmartPointer<vtkLabelPlacementMapper>::New();
  vtkSmartPointer<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy =
    vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  vtkSmartPointer<vtkXMLPolyDataReader> xmlPolyDataReader =
    vtkSmartPointer<vtkXMLPolyDataReader>::New();

  vtkSmartPointer<vtkPolyDataMapper> polyDataMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkActor2D> textActor = vtkSmartPointer<vtkActor2D>::New();

  xmlPolyDataReader->SetFileName( fname );
  delete [] fname;

  vtkSmartPointer<vtkTextProperty> tprop = vtkSmartPointer<vtkTextProperty>::New();
  tprop->SetFontSize( 12 );
  tprop->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  tprop->SetColor( 0.0, 0.8, 0.2 );

  pointSetToLabelHierarchy->SetTextProperty( tprop );
  pointSetToLabelHierarchy->AddInputConnection( xmlPolyDataReader->GetOutputPort() );
  pointSetToLabelHierarchy->SetPriorityArrayName( "Priority" );
  pointSetToLabelHierarchy->SetLabelArrayName( "PlaceNames" );
  pointSetToLabelHierarchy->SetMaximumDepth( maxLevels );
  pointSetToLabelHierarchy->SetTargetLabelCount( targetLabels );

  labelPlacer->SetInputConnection( pointSetToLabelHierarchy->GetOutputPort() );
  labelPlacer->SetIteratorType( iteratorType );
  labelPlacer->SetMaximumLabelFraction( labelRatio );
  labelPlacer->UseDepthBufferOn();

  textActor->SetMapper( labelPlacer );

  renderer->AddActor( sphereActor );
  renderer->AddActor( textActor );

  renWin->SetSize( 300, 300 );
  renWin->AddRenderer( renderer );
  renderer->SetBackground( 0.0, 0.0, 0.0 );
    iren->SetRenderWindow( renWin );

  renWin->Render();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renderer->ResetCamera();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
  }

  return !retVal;
}
