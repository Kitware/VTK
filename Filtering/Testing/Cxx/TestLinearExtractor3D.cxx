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

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExtractSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLinearExtractor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGridWriter.h"

// Reference values
vtkIdType cardSelection[] = 
{
  53,
  53,
  106,
  44,
};

// ------------------------------------------------------------------------------------------------
static int CheckExtractedUGrid( vtkExtractSelection* extract, 
                                      const char* tag,
                                      int testIdx )
{
  // Output must be a multiblock dataset
  vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::SafeDownCast( extract->GetOutput() );
  if ( ! outputMB )
    {
    vtkGenericWarningMacro("Cannot downcast extracted selection to multiblock dataset.");

    return 1;
    }

  // First block must be an unstructured grid
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast( outputMB->GetBlock( 0 ) );
  if ( ! ugrid )
    {
    vtkGenericWarningMacro("Cannot downcast extracted selection to unstructured grid.");

    return 1;
    }

  // Initialize test status
  int testStatus = 0;
  cerr << endl;

  // Verify selection cardinality
  vtkIdType nCells = ugrid->GetNumberOfCells();
  cout << tag 
       << " contains " 
       << nCells
       << " cells."
       << endl;
  
  if ( nCells != cardSelection[testIdx] )
    {
    vtkGenericWarningMacro( "Incorrect cardinality: "
                           << nCells
                           << " != "
                           << cardSelection[testIdx] );
    testStatus = 1;
    }

  // Verify selection cells
  cerr << "Original cell Ids (types): ";
  ugrid->GetCellData()->SetActiveScalars( "vtkOriginalCellIds" );
  vtkDataArray* oCellIds = ugrid->GetCellData()->GetScalars();
  for ( vtkIdType i = 0; i < oCellIds->GetNumberOfTuples(); ++ i )
    {
    cerr << oCellIds->GetTuple1( i )
         << " ";
    }
  cerr << endl;

  return testStatus;
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
  // 0. Selection along inner segment with endpoints (0,0,0) and (.23, 04,.04)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearExtractor> le0 = vtkSmartPointer<vtkLinearExtractor>::New();
  le0->SetInput( mesh );
  le0->SetStartPoint( .0, .0, .0 );
  le0->SetEndPoint( .23, .04, .04 );
  le0->IncludeVerticesOff();
  le0->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es0 =  vtkSmartPointer<vtkExtractSelection>::New();
  es0->SetInput( 0, mesh );
  es0->SetInputConnection( 1, le0->GetOutputPort() );
  es0->Update();

  vtkMultiBlockDataSet* outMB = vtkMultiBlockDataSet::SafeDownCast( es0->GetOutput() );
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast( outMB->GetBlock( 0 ) );
  vtkSmartPointer<vtkUnstructuredGridWriter> writer = vtkSmartPointer<vtkUnstructuredGridWriter>::New();
  writer->SetFileName( "/home/philippe/Test0.vtk" );
  writer->SetInput( grid );
  writer->Write();

  testIntValue += CheckExtractedUGrid( es0, "Selection (0,0,0)-(0.23,0.04,0.04)", 0 );

  // *****************************************************************************
  // 1. Selection along boundary segment with endpoints (0,0,0) and (.23,0,0)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearExtractor> le1 = vtkSmartPointer<vtkLinearExtractor>::New();
  le1->SetInput( mesh );
  le1->SetStartPoint( .0, .0, .0 );
  le1->SetEndPoint( .23, .0, .0 );
  le1->IncludeVerticesOff();
  le1->SetVertexEliminationTolerance( 1.e-12 );
  
  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es1 =  vtkSmartPointer<vtkExtractSelection>::New();
  es1->SetInput( 0, mesh );
  es1->SetInputConnection( 1, le1->GetOutputPort() );
  es1->Update();

  testIntValue += CheckExtractedUGrid( es1, "Selection (0,0,0)-(0.23,0,0)", 1 );

  // *****************************************************************************
  // 2. Selection along broken line through (.23,0,0), (0,0,0), (.23,.04,.04)
  // *****************************************************************************

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points2 = vtkSmartPointer<vtkPoints>::New();
  points2->InsertNextPoint( .23, .0, .0 );
  points2->InsertNextPoint( .0, .0, .0 );
  points2->InsertNextPoint( .23, .04, .04 );

  // Create selection along this broken line
  vtkSmartPointer<vtkLinearExtractor> le2 = vtkSmartPointer<vtkLinearExtractor>::New();
  le2->SetInput( mesh );
  le2->SetPoints( points2 );
  le2->IncludeVerticesOff();
  le2->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es2 =  vtkSmartPointer<vtkExtractSelection>::New();
  es2->SetInput( 0, mesh );
  es2->SetInputConnection( 1, le2->GetOutputPort() );
  es2->Update();

  testIntValue += CheckExtractedUGrid( es2, "Selection (0.23,0,0)-(0,0,0)-(0.23,0.04,0.04)", 2 );

  // *****************************************************************************
  // 3. Selection along broken line through (.23,0,0), (.1,0,0), (.23,.01,.0033)
  // *****************************************************************************

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points3 = vtkSmartPointer<vtkPoints>::New();
  points3->InsertNextPoint( .23, .0, .0 );
  points3->InsertNextPoint( .1, .0, .0 );
  points3->InsertNextPoint( .23, .01, .0033 );

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

  testIntValue += CheckExtractedUGrid( es3, "Selection (0.23,0,0)-(0.1,0,0)-(0.23,0.01,0.0033)", 3 );

  return testIntValue;
}
