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
static void PrintSelectionNodes( vtkSelection* sel, const char* tag = NULL)
{
  if( tag )
    {
    cout << tag 
         << endl;
    }

  // Iterate over nodes
  vtkIdType numNodes = sel->GetNumberOfNodes();
  for( int iNode = 0; iNode < numNodes; ++ iNode )
    {
    cout << ( tag ? "\t" : "" )
         << "Node: " 
         << iNode 
         << endl;

    // Iterate over selection list for this node
    vtkIdType listSize = sel->GetNode( iNode )->GetSelectionList()->GetNumberOfTuples();
    for( int iVal = 0; iVal < listSize; ++ iVal )
      {
      cout << ( tag ? "\t" : "" )
           << "\t" 
           << iVal 
           << "\t" 
           << sel->GetNode( iNode )->GetSelectionList()->GetVariantValue( iVal )
           << endl;
      } // for iVal
    } // for iNode
}

//----------------------------------------------------------------------------
int TestLinearExtractor3D( int argc, char * argv [] )
{
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
  le1->Update();

  vtkSelection* s1 = le1->GetOutput();
  PrintSelectionNodes( s1 , "Selection (0,0,0)-(0.23,0.04,0.04)" );

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
  le2->Update();

  vtkSelection* s2 = le2->GetOutput();
  PrintSelectionNodes( s2 , "Selection (0,0,0)-(0.23,0,0)" );
  
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
  le3->Update();

  vtkSelection* s3 = le3->GetOutput();
  PrintSelectionNodes( s3 , "Selection (0.23,0,0)-(0,0,0)-(0.23,0.04,0.04)" );

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
  le4->Update();

  vtkSelection* s4 = le4->GetOutput();
  PrintSelectionNodes( s4 , "Selection (0.23,0,0)-(0.1,0,0)-(0.23,0.01,0.0033)" );


  int retVal = 1;

  return !retVal;
}
