/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinearExtractor2D.cxx

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

#include <vtksys/ios/sstream>

// Reference value
vtkIdType cardSelectionLinearExtractor2D  = 20; 

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
  
  if ( nCells != cardSelectionLinearExtractor2D )
    {
    vtkGenericWarningMacro( "Incorrect cardinality: "
                           << nCells
                           << " != "
                           <<  cardSelectionLinearExtractor2D );
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
    fileNameSS << "./LinearExtraction2D-"
               << testIdx
               << ".vtk";
    vtkSmartPointer<vtkUnstructuredGridWriter> writer = vtkSmartPointer<vtkUnstructuredGridWriter>::New();
    writer->SetFileName( fileNameSS.str().c_str() );
    writer->SetInput( ugrid );
    writer->Write();
    cerr << "Wrote file "
         << fileNameSS.str()
         << endl;
    }

  return testStatus;
}

//----------------------------------------------------------------------------
int TestLinearExtractor2D( int argc, char * argv [] )
{
  // Initialize test value
  int testIntValue = 0;
  
  // Read 2D unstructured input mesh
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/SemiDisk/SemiDisk.vtk");
  vtkSmartPointer<vtkUnstructuredGridReader> reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName( fileName );
  reader->Update();

  // Create multi-block mesh for linear extractor
  vtkSmartPointer<vtkMultiBlockDataSet> mesh = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mesh->SetNumberOfBlocks( 1 );
  mesh->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Mesh" ); 
  mesh->SetBlock( 0, reader->GetOutput() );

  // *****************************************************************************
  // Selection along inner segment with endpoints (35.84,0,0) and (36.9,0.03,0)
  // *****************************************************************************

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearExtractor> le = vtkSmartPointer<vtkLinearExtractor>::New();
  le->SetInput( mesh );
  le->SetStartPoint( 35.84, .0, .0 );
  le->SetEndPoint( 36.9, .03, .0 );
  le->IncludeVerticesOff();
  le->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> es =  vtkSmartPointer<vtkExtractSelection>::New();
  es->SetInput( 0, mesh );
  es->SetInputConnection( 1, le->GetOutputPort() );
  es->Update();

  if ( 0 )
    {
    testIntValue += CheckExtractedUGrid( es, "Selection (35.84,0,0)-(36.9,0.03,0)", 0, true );
    }

  return testIntValue;
}
