/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinearExtractor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "vtkExtractSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLinearExtractor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

// ------------------------------------------------------------------------------------------------
static int CheckExtractedUGrid( vtkExtractSelection* extract, const char* tag )
{
  vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::SafeDownCast( extract->GetOutput() );
  if ( ! outputMB )
    {
    vtkGenericWarningMacro("Cannot downcast extracted selection to multiblock dataset.");

    return 1;
    }

  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast( outputMB->GetBlock( 0 ) );
  if ( ! ugrid )
    {
    vtkGenericWarningMacro("Cannot downcast extracted selection to unstructured grid.");

    return 1;
    }

  vtkIdType nCells = ugrid->GetNumberOfCells();
  cout << tag 
       << " contains " 
       << nCells
       << " cells."
       << endl;

  return 0;
}

//----------------------------------------------------------------------------
int TestLinearExtractor3D( int argc, char * argv [] )
{
  // Initialize test value
  int testIntValue = 0;
  
  // Read 3D unstructured input mesh
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/AngularSector.vtk");
  vtkSmartPointer<vtkUnstructuredGridReader> reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName( fileName );
  reader->Update();

  // Create multi-block mesh for linear extractor
  vtkSmartPointer<vtkMultiBlockDataSet> mesh = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mesh->SetNumberOfBlocks( 1 );
  mesh->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Mesh" ); 
  mesh->SetBlock( 0, reader->GetOutput() );

  // *****************************************************************************
  // 1. Selection along inner segment with endpoints (0,0,0) and (.23, 04,.04)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearExtractor> le1 = vtkSmartPointer<vtkLinearExtractor>::New();
  le1->SetInput( mesh );
  le1->SetStartPoint( .0, .0, .0 );
  le1->SetEndPoint( .23, .04, .04 );
  le1->IncludeVerticesOff();
  le1->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es1 =  vtkSmartPointer<vtkExtractSelection>::New();
  es1->SetInput( 0, mesh );
  es1->SetInputConnection( 1, le1->GetOutputPort() );
  es1->Update();

  testIntValue += CheckExtractedUGrid( es1, "Selection (0,0,0)-(0.23,0.04,0.04)" );

  // *****************************************************************************
  // 2. Selection along boundary segment with endpoints (0,0,0) and (.23,0,0)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearExtractor> le2 = vtkSmartPointer<vtkLinearExtractor>::New();
  le2->SetInput( mesh );
  le2->SetStartPoint( .0, .0, .0 );
  le2->SetEndPoint( .23, .0, .0 );
  le2->IncludeVerticesOff();
  le2->SetVertexEliminationTolerance( 1.e-12 );
  
  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es2 =  vtkSmartPointer<vtkExtractSelection>::New();
  es2->SetInput( 0, mesh );
  es2->SetInputConnection( 1, le2->GetOutputPort() );
  es2->Update();

  testIntValue += CheckExtractedUGrid( es2, "Selection (0,0,0)-(0.23,0,0)" );

  // *****************************************************************************
  // 3. Selection along broken line through (.23,0,0), (0,0,0), (.23,.04,.04)
  // *****************************************************************************

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points3 = vtkSmartPointer<vtkPoints>::New();
  points3->InsertNextPoint( .23, .0, .0 );
  points3->InsertNextPoint( .0, .0, .0 );
  points3->InsertNextPoint( .23, .04, .04 );

  // Create selection along this broken line
  vtkSmartPointer<vtkLinearExtractor> le3 = vtkSmartPointer<vtkLinearExtractor>::New();
  le3->SetInput( mesh );
  le3->SetPoints( points3 );
  le3->IncludeVerticesOff();
  le3->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es3 =  vtkSmartPointer<vtkExtractSelection>::New();
  es3->SetInput( 0, mesh );
  es3->SetInputConnection( 1, le3->GetOutputPort() );
  es3->Update();

  testIntValue += CheckExtractedUGrid( es3, "Selection (0.23,0,0)-(0,0,0)-(0.23,0.04,0.04)" );

  // *****************************************************************************
  // 4. Selection along broken line through (.23,0,0), (.1,0,0), (.23,.01,.0033)
  // *****************************************************************************

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points4 = vtkSmartPointer<vtkPoints>::New();
  points4->InsertNextPoint( .23, .0, .0 );
  points4->InsertNextPoint( .1, .0, .0 );
  points4->InsertNextPoint( .23, .01, .0033 );

  // Create selection along this broken line
  vtkSmartPointer<vtkLinearExtractor> le4 = vtkSmartPointer<vtkLinearExtractor>::New();
  le4->SetInput( mesh );
  le4->SetPoints( points4 );
  le4->IncludeVerticesOff();
  le4->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es4 =  vtkSmartPointer<vtkExtractSelection>::New();
  es4->SetInput( 0, mesh );
  es4->SetInputConnection( 1, le4->GetOutputPort() );
  es4->Update();

  testIntValue += CheckExtractedUGrid( es4, "Selection (0.23,0,0)-(0.1,0,0)-(0.23,0.01,0.0033)" );

  return testIntValue;
}
