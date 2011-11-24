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
  vtkIdType numNodes = sel->GetNumberOfNodes();

  if(tag)
    {
    cout << tag << endl;
    }

  for(int iNode=0; iNode < numNodes; iNode++)
    {
    if(tag) cout << "\t";
    cout << "Node: " << iNode << endl;
    vtkIdType listSize = sel->GetNode(iNode)->GetSelectionList()->GetNumberOfTuples();
    for(int iVal=0; iVal < listSize; iVal++)
      {
      if(tag) cout << "\t";
      cout << "\t" << iVal << "\t" << sel->GetNode(iNode)->GetSelectionList()->GetVariantValue(iVal) << endl;
      }
    }
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

  // Create selection along one line segment
  vtkSmartPointer<vtkLinearExtractor> extractor = vtkSmartPointer<vtkLinearExtractor>::New();
  extractor->SetInput( mesh );
  extractor->SetStartPoint( 0., 0., 0. );
  extractor->SetEndPoint( .23, .04, .04 );
  extractor->Update();

  vtkSelection* sel = extractor->GetOutput();
  PrintSelectionNodes( sel , "selection" );
  
  int retVal = 1;

  return !retVal;
}
