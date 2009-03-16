/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtLabelSizeCalculator.cxx

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
#include "vtkCellArray.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelHierarchy.h"
#include "vtkLabelPlacer.h"
#include "vtkLabelSizeCalculator.h"
#include "vtkQtLabelSizeCalculator.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTexturedActor2D.h"
#include "vtkImageData.h"
#include "vtkQtLabelSurface.h"
#include "vtkQtInitialization.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

int TestQtLabelSizeCalculator(int argc, char *argv[])
{
  int maxLevels = 5;
  int targetLabels = 7;
  double labelRatio = 1.0;
  int i = 0;
  int iteratorType = vtkLabelHierarchy::FULL_SORT;
  bool showBounds = true;

  VTK_CREATE(vtkQtInitialization, init);

  VTK_CREATE( vtkQtLabelSizeCalculator, qtLabelSizeCalculator );
  VTK_CREATE( vtkLabelSizeCalculator, labelSizeCalculator );
  VTK_CREATE( vtkLabelHierarchy, labelHierarchy );
  VTK_CREATE( vtkLabelPlacer, labelPlacer2 );
  VTK_CREATE( vtkPointSetToLabelHierarchy, pointSetToLabelHierarchy );
  VTK_CREATE( vtkPolyDataMapper, polyDataMapper );
  VTK_CREATE( vtkActor, actor );
  VTK_CREATE( vtkRenderer, renderer );
  VTK_CREATE( vtkRenderWindow, renWin );
  renWin->SetMultiSamples(0); // ensure to have the same test image everywhere
  
  VTK_CREATE( vtkRenderWindowInteractor, iren );
  VTK_CREATE( vtkQtLabelSurface, qtLabelPlacer );
  VTK_CREATE( vtkLabeledDataMapper, labeledMapper );
  VTK_CREATE( vtkActor2D, textActor2 );
  VTK_CREATE( vtkPoints, points );

  for(i = 0; i < 29; i++)
    {
    points->InsertPoint( i, 0.0, 0.0, 0.0 );
    }
  points->InsertPoint( 29, 5.0, 5.0, 0.0 );

  VTK_CREATE( vtkCellArray, cells );
  cells->InsertNextCell(30);
  for(i = 0; i < 30; i++)
    {
    cells->InsertCellPoint(i);
    }

  VTK_CREATE( vtkPolyData, polyData );
  polyData->SetPoints(points);
  polyData->SetVerts(cells);

  VTK_CREATE( vtkStringArray, stringData );
  stringData->SetName("PlaceNames");
  stringData->InsertNextValue("\302\242 \302\245 Abu Dhabi");
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
  stringData->InsertNextValue("Luxembourg <i>City</i>");
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

  qtLabelSizeCalculator->SetInput(polyData);
  qtLabelSizeCalculator->DebugOn();
  qtLabelSizeCalculator->GetFontProperty()->SetFontSize( 12 );
  qtLabelSizeCalculator->GetFontProperty()->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  qtLabelSizeCalculator->GetFontProperty()->SetColor(0., 0., 1.);
  qtLabelSizeCalculator->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames" );
  qtLabelSizeCalculator->SetLabelSizeArrayName( "LabelSize" );

  labelSizeCalculator->SetInput(polyData);
  labelSizeCalculator->DebugOn();
  labelSizeCalculator->GetFontProperty()->SetFontSize( 12 );
  labelSizeCalculator->GetFontProperty()->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  labelSizeCalculator->GetFontProperty()->SetColor(0., 0., 1.);
  labelSizeCalculator->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames" );
  labelSizeCalculator->SetLabelSizeArrayName( "LabelSize" );
  labelSizeCalculator->Update();

  pointSetToLabelHierarchy->AddInputConnection(qtLabelSizeCalculator->GetOutputPort());
  pointSetToLabelHierarchy->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority" );
  pointSetToLabelHierarchy->SetInputArrayToProcess( 1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize" );
  pointSetToLabelHierarchy->SetInputArrayToProcess( 2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames" );
  pointSetToLabelHierarchy->SetMaximumDepth( maxLevels );
  pointSetToLabelHierarchy->SetTargetLabelCount( targetLabels );

  labelPlacer2->SetInputConnection( pointSetToLabelHierarchy->GetOutputPort() );
  labelPlacer2->SetIteratorType( iteratorType );
  labelPlacer2->SetOutputTraversedBounds( showBounds );
  labelPlacer2->SetRenderer( renderer );
  labelPlacer2->SetMaximumLabelFraction( labelRatio );
  labelPlacer2->SetOutputCoordinateSystem( vtkLabelPlacer::DISPLAY );

  qtLabelPlacer->SetInputConnection(labelPlacer2->GetOutputPort());
  qtLabelPlacer->SetRenderer( renderer );
  qtLabelPlacer->SetLabelTextProperty(qtLabelSizeCalculator->GetFontProperty());
  qtLabelPlacer->SetFieldDataName("LabelText");
//  cairoLabelSizeCalculator->SetRenderWindow( qtLabelPlacer->GetCairoRenderWindow() );

  VTK_CREATE( vtkPolyDataMapper2D, polyDataMapper2 ); 
  polyDataMapper2->SetInputConnection( qtLabelPlacer->GetOutputPort(1) );
  VTK_CREATE( vtkTexturedActor2D, actor2 ); 
  actor2->SetMapper( polyDataMapper2 );
  qtLabelPlacer->Update();
  VTK_CREATE( vtkTexture, texture );
  texture->SetInput( qtLabelPlacer->GetOutput() );
  actor2->SetTexture( texture );

  renderer->AddActor(actor2);

  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.5, 0.5, 0.5);
  //renderer->SetBackground(1.0, 1.0, 1.0);
    iren->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
//  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
