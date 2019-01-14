/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellDistanceSelector3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2012

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExtractSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkCellDistanceSelector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGridWriter.h"

#include <sstream>

// Reference values
const vtkIdType cardCellDistanceSelection3D[] =
{
  125,
  16,
  20,
  73,
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

  if ( nCells != cardCellDistanceSelection3D[testIdx] )
  {
    vtkGenericWarningMacro( "Incorrect cardinality: "
                           << nCells
                           << " != "
                           << cardCellDistanceSelection3D[testIdx] );
    testStatus = 1;
  }

  // Verify selection cells
  cerr << "Original cell Ids: ";
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
    std::ostringstream fileNameSS;
    fileNameSS << "./CellDistanceExtraction3D-"
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
int TestCellDistanceSelector3D( int argc, char * argv [] )
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
  // 0. Selection within distance of 2 from cell 7010
  // *****************************************************************************

  // Create a selection, sel0, of cell with index 7010
  vtkSmartPointer<vtkIdTypeArray> selArr0 = vtkSmartPointer<vtkIdTypeArray>::New();
  selArr0->InsertNextValue( 7010 );
  vtkSmartPointer<vtkSelectionNode> selNode0 = vtkSmartPointer<vtkSelectionNode>::New();
  selNode0->SetContentType( vtkSelectionNode::INDICES );
  selNode0->SetFieldType( vtkSelectionNode::CELL );
  selNode0->GetProperties()->Set( vtkSelectionNode::COMPOSITE_INDEX(), 1 );
  selNode0->SetSelectionList( selArr0 );
  vtkSmartPointer<vtkSelection> sel0 = vtkSmartPointer<vtkSelection>::New();
  sel0->AddNode( selNode0 );

  // Create selection up to topological distance of 2
  vtkSmartPointer<vtkCellDistanceSelector> ls0 = vtkSmartPointer<vtkCellDistanceSelector>::New();
  ls0->SetInputMesh( mesh );
  ls0->SetInputSelection( sel0 );
  ls0->SetDistance( 2 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es0 =  vtkSmartPointer<vtkExtractSelection>::New();
  es0->SetInputData( 0, mesh );
  es0->SetInputConnection( 1, ls0->GetOutputPort() );
  es0->Update();
  testIntValue += CheckExtractedUGrid( es0, "Selection d({7010})<3", 0, true );

  // *****************************************************************************
  // 1. Selection at distance of 1 from ridge 7643-7499-7355-7211, excluding it
  // *****************************************************************************

  // Create a selection, sel1, of cells with indices 7643-7499-7355-7211
  vtkSmartPointer<vtkIdTypeArray> selArr1 = vtkSmartPointer<vtkIdTypeArray>::New();
  selArr1->InsertNextValue( 7643 );
  selArr1->InsertNextValue( 7499 );
  selArr1->InsertNextValue( 7355 );
  selArr1->InsertNextValue( 7211 );
  vtkSmartPointer<vtkSelectionNode> selNode1 = vtkSmartPointer<vtkSelectionNode>::New();
  selNode1->SetContentType( vtkSelectionNode::INDICES );
  selNode1->SetFieldType( vtkSelectionNode::CELL );
  selNode1->GetProperties()->Set( vtkSelectionNode::COMPOSITE_INDEX(), 1 );
  selNode1->SetSelectionList( selArr1 );
  vtkSmartPointer<vtkSelection> sel1 = vtkSmartPointer<vtkSelection>::New();
  sel1->AddNode( selNode1 );

  // Create selection at distance of 1
  vtkSmartPointer<vtkCellDistanceSelector> ls1 = vtkSmartPointer<vtkCellDistanceSelector>::New();
  ls1->SetInputMesh( mesh );
  ls1->SetInputSelection( sel1 );
  ls1->SetDistance( 1 );
  ls1->IncludeSeedOff();

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es1 =  vtkSmartPointer<vtkExtractSelection>::New();
  es1->SetInputData( 0, mesh );
  es1->SetInputConnection( 1, ls1->GetOutputPort() );
  es1->Update();
  testIntValue += CheckExtractedUGrid( es1, "Selection d({7643-7499-7355-7211})=1", 1, true );

  // *****************************************************************************
  // 2. Selection at distance of 2 from corner 7632, retaining seed
  // *****************************************************************************

  // Create a selection, sel2, of cell with index 7632
  vtkSmartPointer<vtkIdTypeArray> selArr2 = vtkSmartPointer<vtkIdTypeArray>::New();
  selArr2->InsertNextValue( 7632 );
  vtkSmartPointer<vtkSelectionNode> selNode2 = vtkSmartPointer<vtkSelectionNode>::New();
  selNode2->SetContentType( vtkSelectionNode::INDICES );
  selNode2->SetFieldType( vtkSelectionNode::CELL );
  selNode2->GetProperties()->Set( vtkSelectionNode::COMPOSITE_INDEX(), 1 );
  selNode2->SetSelectionList( selArr2 );
  vtkSmartPointer<vtkSelection> sel2 = vtkSmartPointer<vtkSelection>::New();
  sel2->AddNode( selNode2 );

  // Create selection at distance of 2
  vtkSmartPointer<vtkCellDistanceSelector> ls2 = vtkSmartPointer<vtkCellDistanceSelector>::New();
  ls2->SetInputMesh( mesh );
  ls2->SetInputSelection( sel2 );
  ls2->SetDistance( 2 );
  ls2->AddIntermediateOff();

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es2 =  vtkSmartPointer<vtkExtractSelection>::New();
  es2->SetInputData( 0, mesh );
  es2->SetInputConnection( 1, ls2->GetOutputPort() );
  es2->Update();
  testIntValue += CheckExtractedUGrid( es2, "Selection d({7632})=0|2", 2, true );

  // *****************************************************************************
  // 3. Selection within distance of 1 from cells 6413, 7268, and 7399
  // *****************************************************************************

  // Create a selection, sel3, of cells with indices 6413, 7268, and 7399
  vtkSmartPointer<vtkIdTypeArray> selArr3 = vtkSmartPointer<vtkIdTypeArray>::New();
  selArr3->InsertNextValue( 6413 );
  selArr3->InsertNextValue( 7268 );
  selArr3->InsertNextValue( 7399 );
  vtkSmartPointer<vtkSelectionNode> selNode3 = vtkSmartPointer<vtkSelectionNode>::New();
  selNode3->SetContentType( vtkSelectionNode::INDICES );
  selNode3->SetFieldType( vtkSelectionNode::CELL );
  selNode3->GetProperties()->Set( vtkSelectionNode::COMPOSITE_INDEX(), 1 );
  selNode3->SetSelectionList( selArr3 );
  vtkSmartPointer<vtkSelection> sel3 = vtkSmartPointer<vtkSelection>::New();
  sel3->AddNode( selNode3 );

  // Create selection within distance of 1
  vtkSmartPointer<vtkCellDistanceSelector> ls3 = vtkSmartPointer<vtkCellDistanceSelector>::New();
  ls3->SetInputMesh( mesh );
  ls3->SetInputSelection( sel3 );
  ls3->SetDistance( 1 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es3 =  vtkSmartPointer<vtkExtractSelection>::New();
  es3->SetInputData( 0, mesh );
  es3->SetInputConnection( 1, ls3->GetOutputPort() );
  es3->Update();
  testIntValue += CheckExtractedUGrid( es3, "Selection d({6413,7268,7399})<2", 3, true );

  return testIntValue;
}
