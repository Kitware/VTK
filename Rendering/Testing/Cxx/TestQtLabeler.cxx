/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtLabeler.cxx

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
#include "vtkPNGWriter.h"
#include "vtkTexture.h"
#include "vtkQtInitialization.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

int TestQtLabeler(int argc, char *argv[])
{
  int maxLevels = 5;
  int targetLabels = 7;
  double labelRatio = 1.0;
  int i = 0;
  int iteratorType = vtkLabelHierarchy::FULL_SORT;
  bool showBounds = true;

  vtkSmartPointer<vtkLabelSizeCalculator> labelSizeCalculator = 
    vtkSmartPointer<vtkLabelSizeCalculator>::New();
  vtkSmartPointer<vtkLabelPlacer> labelPlacer = 
    vtkSmartPointer<vtkLabelPlacer>::New();
  vtkSmartPointer<vtkLabelPlacer> labelPlacer2 = 
    vtkSmartPointer<vtkLabelPlacer>::New();
  vtkSmartPointer<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy = 
    vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();

  vtkSmartPointer<vtkPolyDataMapper> polyDataMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> actor = 
    vtkSmartPointer<vtkActor>::New();

  vtkSmartPointer<vtkRenderer> renderer = 
    vtkSmartPointer<vtkRenderer>::New();
  
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0); // ensure to have the same test image everywhere
  
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkQtLabelSurface> cairoLabelPlacer = 
    vtkSmartPointer<vtkQtLabelSurface>::New();
  vtkSmartPointer<vtkLabeledDataMapper> labeledMapper = 
    vtkSmartPointer<vtkLabeledDataMapper>::New();
  vtkSmartPointer<vtkActor2D> textActor2 = 
    vtkSmartPointer<vtkActor2D>::New();

  vtkSmartPointer<vtkPoints> points = 
    vtkSmartPointer<vtkPoints>::New();

  for(i = 0; i < 29; i++)
    {
    points->InsertPoint( i, 0.0, 0.0, 0.0 );
    }
  points->InsertPoint( 29, 5.0, 5.0, 0.0 );

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
  stringData->InsertNextValue("<span>\302\242 \302\245 Abu Dhabi</span>");
  stringData->InsertNextValue("<span>3&#x3A3;(x-x<sub>c</sub>)<sup>2</sup></span>");
  stringData->InsertNextValue("<tt>Beijing</tt>");
  stringData->InsertNextValue("B<sup>erlin</sup>");
  stringData->InsertNextValue("<big>\xE0\xA7\xA0 Cairo</big>");
  stringData->InsertNextValue("<b>Caracas</b>");
  stringData->InsertNextValue("<small>Dublin</small>");
  stringData->InsertNextValue("<s>Georgetown</s>");
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
  
  labelSizeCalculator->SetInput(polyData);
  labelSizeCalculator->GetFontProperty()->SetFontSize( 12 );
  labelSizeCalculator->GetFontProperty()->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  labelSizeCalculator->GetFontProperty()->SetColor(1., 0., 0.);
  labelSizeCalculator->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames" );
  labelSizeCalculator->SetLabelSizeArrayName( "LabelSize" );

  //create a new text property with the same values as the font properties
  // in the labelSizeCalculator so that we can change colors for comparisons
  // between the two labelers...
  VTK_CREATE( vtkTextProperty, cairoTextProperty );
  cairoTextProperty->SetFontSize(12);
  cairoTextProperty->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  cairoTextProperty->SetColor( 1,1,1 );
  cairoTextProperty->SetShadow(1);

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

  labelPlacer2->SetInputConnection( pointSetToLabelHierarchy->GetOutputPort() );
  labelPlacer2->SetIteratorType( iteratorType );
  labelPlacer2->SetOutputTraversedBounds( showBounds );
  labelPlacer2->SetRenderer( renderer );
  labelPlacer2->SetMaximumLabelFraction( labelRatio );
  labelPlacer2->SetOutputCoordinateSystem( vtkLabelPlacer::DISPLAY );

  polyDataMapper->SetInputConnection(labelPlacer->GetOutputPort());

  actor->SetMapper(polyDataMapper);

  labelPlacer->Update();

  cairoLabelPlacer->SetInputConnection(labelPlacer2->GetOutputPort());
  cairoLabelPlacer->SetRenderer( renderer );
  cairoLabelPlacer->SetLabelTextProperty(cairoTextProperty);
  cairoLabelPlacer->SetFieldDataName("LabelText");

  labeledMapper->SetInputConnection(labelPlacer->GetOutputPort());
  labeledMapper->SetLabelTextProperty(labelSizeCalculator->GetFontProperty());
  labeledMapper->SetFieldDataName("LabelText");
  labeledMapper->SetLabelModeToLabelFieldData();
  textActor2->SetMapper(labeledMapper);

  VTK_CREATE( vtkPolyDataMapper2D, polyDataMapper2 ); 
  polyDataMapper2->SetInputConnection( cairoLabelPlacer->GetOutputPort(1) );
  VTK_CREATE( vtkTexturedActor2D, actor2 ); 
  actor2->SetMapper( polyDataMapper2 );
  cairoLabelPlacer->Update();
  VTK_CREATE( vtkTexture, texture );
  texture->SetInput( cairoLabelPlacer->GetOutput() );
  texture->SetBlendingMode( vtkTexture::VTK_TEXTURE_BLENDING_MODE_NONE );
  actor2->SetTexture( texture );
  VTK_CREATE( vtkPNGWriter, pngw );
  pngw->SetFilePrefix( "blar" );
  pngw->SetFilePattern( "%s.%d.png" );
  pngw->SetInputConnection( cairoLabelPlacer->GetOutputPort() );

  renderer->AddActor(actor);
  renderer->AddActor(textActor2);
  renderer->AddActor(actor2);

  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.5, 0.5, 0.5);
//  renderer->SetBackground(1.0, 1.0, 1.0);
//  renderer->SetBackground(0, 0, 0);
    iren->SetRenderWindow(renWin);

  labelPlacer->Update();
  renWin->Render();
  pngw->Write();

  int retVal = vtkRegressionTestImage( renWin );
//  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
