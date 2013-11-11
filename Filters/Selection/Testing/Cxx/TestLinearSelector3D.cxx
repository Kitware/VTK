/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinearSelector3D.cxx

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
#include "vtkLinearSelector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGridWriter.h"

#include <vtksys/ios/sstream>

// Reference values
const vtkIdType cardSelection[] =
{
  54,
  54,
  108,
  45,
};

// ------------------------------------------------------------------------------------------------
static int CheckExtractedUGrid( vtkExtractSelection* extract,
                                const char* tag,
                                int testIdx,
                                bool writeGrid )
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

  // If requested, write mesh
  if ( writeGrid )
    {
    vtksys_ios::ostringstream fileNameSS;
    fileNameSS << "./LinearExtraction3D-"
               << testIdx
               << ".vtk";
    vtkSmartPointer<vtkUnstructuredGridWriter> writer = vtkSmartPointer<vtkUnstructuredGridWriter>::New();
    writer->SetFileName( fileNameSS.str().c_str() );
    writer->SetInputData( ugrid );
    writer->Write();
    cerr << "Wrote file "
         << fileNameSS.str()
         << endl;
    }

  return testStatus;
}

//----------------------------------------------------------------------------
int TestLinearSelector3D( int argc, char * argv [] )
{
  // Initialize test value
  int testIntValue = 0;

  // Read 3D unstructured input mesh
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/AngularSector.vtk");
  vtkSmartPointer<vtkUnstructuredGridReader> reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName( fileName );
  reader->Update();
  delete [] fileName;

  // Create multi-block mesh for linear selector
  vtkSmartPointer<vtkMultiBlockDataSet> mesh = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mesh->SetNumberOfBlocks( 1 );
  mesh->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Mesh" );
  mesh->SetBlock( 0, reader->GetOutput() );

  // *****************************************************************************
  // 0. Selection along inner segment with endpoints (0,0,0) and (.23, 04,.04)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearSelector> ls0 = vtkSmartPointer<vtkLinearSelector>::New();
  ls0->SetInputData( mesh );
  ls0->SetStartPoint( .0, .0, .0 );
  ls0->SetEndPoint( .23, .04, .04 );
  ls0->IncludeVerticesOff();
  ls0->SetVertexEliminationTolerance( 1.e-12 );
  ls0->SetTolerance(1.e-12);

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es0 =  vtkSmartPointer<vtkExtractSelection>::New();
  es0->SetInputData( 0, mesh );
  es0->SetInputConnection( 1, ls0->GetOutputPort() );
  es0->Update();

  testIntValue += CheckExtractedUGrid( es0, "Selection (0,0,0)-(0.23,0.04,0.04)", 0, true );

  // *****************************************************************************
  // 1. Selection along boundary segment with endpoints (0,0,0) and (.23,0,0)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearSelector> ls1 = vtkSmartPointer<vtkLinearSelector>::New();
  ls1->SetInputData( mesh );
  ls1->SetStartPoint( .0, .0, .0 );
  ls1->SetEndPoint( .23, .0, .0 );
  ls1->IncludeVerticesOff();
  ls1->SetVertexEliminationTolerance( 1.e-12 );
  ls1->SetTolerance(1.e-12);

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es1 =  vtkSmartPointer<vtkExtractSelection>::New();
  es1->SetInputData( 0, mesh );
  es1->SetInputConnection( 1, ls1->GetOutputPort() );
  es1->Update();

  testIntValue += CheckExtractedUGrid( es1, "Selection (0,0,0)-(0.23,0,0)", 1, true );

  // *****************************************************************************
  // 2. Selection along broken line through (.23,0,0), (0,0,0), (.23,.04,.04)
  // *****************************************************************************

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points2 = vtkSmartPointer<vtkPoints>::New();
  points2->InsertNextPoint( .23, .0, .0 );
  points2->InsertNextPoint( .0, .0, .0 );
  points2->InsertNextPoint( .23, .04, .04 );

  // Create selection along this broken line
  vtkSmartPointer<vtkLinearSelector> ls2 = vtkSmartPointer<vtkLinearSelector>::New();
  ls2->SetInputData( mesh );
  ls2->SetPoints( points2 );
  ls2->IncludeVerticesOff();
  ls2->SetVertexEliminationTolerance( 1.e-12 );
  ls2->SetTolerance(1.e-12);
  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es2 =  vtkSmartPointer<vtkExtractSelection>::New();
  es2->SetInputData( 0, mesh );
  es2->SetInputConnection( 1, ls2->GetOutputPort() );
  es2->Update();

  testIntValue += CheckExtractedUGrid( es2, "Selection (0.23,0,0)-(0,0,0)-(0.23,0.04,0.04)", 2, true );

  // *****************************************************************************
  // 3. Selection along broken line through (.23,0,0), (.1,0,0), (.23,.01,.0033)
  // *****************************************************************************

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points3 = vtkSmartPointer<vtkPoints>::New();
  points3->InsertNextPoint( .23, .0, .0 );
  points3->InsertNextPoint( .1, .0, .0 );
  points3->InsertNextPoint( .23, .01, .0033 );

  // Create selection along this broken line
  vtkSmartPointer<vtkLinearSelector> ls3 = vtkSmartPointer<vtkLinearSelector>::New();
  ls3->SetInputData( mesh );
  ls3->SetPoints( points3 );
  ls3->IncludeVerticesOff();
  ls3->SetVertexEliminationTolerance( 1.e-12 );
  ls3->SetTolerance(1.e-12);

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es3 =  vtkSmartPointer<vtkExtractSelection>::New();
  es3->SetInputData( 0, mesh );
  es3->SetInputConnection( 1, ls3->GetOutputPort() );
  es3->Update();

  testIntValue += CheckExtractedUGrid( es3, "Selection (0.23,0,0)-(0.1,0,0)-(0.23,0.01,0.0033)", 3, true );

  return testIntValue;
}
