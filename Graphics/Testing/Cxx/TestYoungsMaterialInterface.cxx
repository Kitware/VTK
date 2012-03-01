/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadRotationalExtrusion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pébay, Kitware SAS 2012

#include "vtkAVSucdReader.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSetMapper.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"
#include "vtkYoungsMaterialInterface.h"

//----------------------------------------------------------------------------
int TestYoungsMaterialInterface( int argc, char * argv [] )
{
  // Create renderer and add actors to it
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground( .8, .8 ,.8 );

  // Create render window;
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer( renderer.GetPointer() );
  window->SetSize( 500, 200 );
  window->SetMultiSamples( 0 );

  // Create interactor;
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow( window.GetPointer() );

  // Read from AVS UCD data in binary form;
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/UCD2D/UCD_00005.inp");
  vtkNew<vtkAVSucdReader> reader;
  reader->SetFileName( fileName );
  delete [] fileName;

  // Update reader and get mesh cell data;
  reader->Update();
  vtkUnstructuredGrid* mesh = reader->GetOutput();
  vtkCellData* cellData = mesh->GetCellData();

  // Create normal vectors;
  cellData->SetActiveScalars( "norme[0]" );
  vtkDataArray* normX = cellData->GetScalars();
  cellData->SetActiveScalars( "norme[1]" );
  vtkDataArray* normY = cellData->GetScalars();
  vtkIdType n = normX->GetNumberOfTuples();
  vtkNew<vtkDoubleArray> norm;
  norm->SetNumberOfComponents( 3 );
  norm->SetNumberOfTuples( n );
  norm->SetName( "norme" );
  for ( int i = 0; i < n; ++ i )
    {
    norm->SetTuple3( i, normX->GetTuple1( i ), normY->GetTuple1( i ), 0. );
    }
  cellData->SetVectors( norm.GetPointer() );

  // Extract submesh corresponding with cells containing material 2
  cellData->SetActiveScalars( "Material Id" );
  vtkNew<vtkThreshold> threshold2;
  threshold2->SetInput( mesh );
  threshold2->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS );
  threshold2->ThresholdByLower( 2 );
  threshold2->Update();
  vtkUnstructuredGrid* meshMat2 = threshold2->GetOutput();

  // Extract submesh corresponding with cells containing material 3
  vtkNew<vtkThreshold> threshold3;
  threshold3->SetInput( mesh );
  threshold3->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS );
  threshold3->ThresholdByUpper( 3 );
  threshold3->Update();
  vtkUnstructuredGrid* meshMat3 = threshold3->GetOutput();

  // Make multiblock from extracted submeshes;
  vtkNew<vtkMultiBlockDataSet> meshMB;
  meshMB->SetNumberOfBlocks( 2 );
  meshMB->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Material 2" );
  meshMB->SetBlock( 0, meshMat2 );
  meshMB->GetMetaData( static_cast<unsigned>( 1 ) )->Set( vtkCompositeDataSet::NAME(), "Material 3" );
  meshMB->SetBlock( 1, meshMat3 );

  // Create mapper for submesh corresponding to material 2;
  double* matRange = cellData->GetScalars()->GetRange();
  vtkNew<vtkDataSetMapper> meshMapper;
  meshMapper->SetInput( meshMat2 );
  meshMapper->SetScalarRange( matRange );
  meshMapper->SetScalarModeToUseCellData();
  meshMapper->SetColorModeToMapScalars();
  meshMapper->ScalarVisibilityOn();
  meshMapper->SetResolveCoincidentTopologyPolygonOffsetParameters( 0, 1 );
  meshMapper->SetResolveCoincidentTopologyToPolygonOffset();

  // Create wireframe actor for entire mesh
  vtkNew<vtkActor> meshActor;
  meshActor->SetMapper( meshMapper.GetPointer() );
  meshActor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddViewProp( meshActor.GetPointer() );

  cellData->SetActiveScalars("frac_pres[1]");
  // Reconstruct Youngs material interface
  vtkNew<vtkYoungsMaterialInterface> youngs;
  youngs->SetInput( meshMB.GetPointer() );
  youngs->SetNumberOfMaterials( 2 );
  youngs->SetMaterialVolumeFractionArray( 0, "frac_pres[1]" );
  youngs->SetMaterialVolumeFractionArray( 1, "frac_pres[2]" );
  youngs->SetMaterialNormalArray( 0, "norme" );
  youngs->SetMaterialNormalArray( 1, "norme" );
  youngs->SetVolumeFractionRange( .001, .999 );
  youngs->FillMaterialOn();
  youngs->RemoveAllMaterialBlockMappings();
  youngs->AddMaterialBlockMapping( -1 );
  youngs->AddMaterialBlockMapping( 1 );
  youngs->AddMaterialBlockMapping( -2 );
  youngs->AddMaterialBlockMapping( 2 );
  youngs->UseAllBlocksOff();
  youngs->Update();

  // Create mappers and actors for surface rendering of all reconstructed interfaces;
  vtkNew<vtkCompositeDataIterator> interfaceIterator;
  interfaceIterator->SetDataSet( youngs->GetOutput() );
  interfaceIterator->VisitOnlyLeavesOn();
  interfaceIterator->SkipEmptyNodesOn();
  interfaceIterator->InitTraversal();
  interfaceIterator->GoToFirstItem();
  while ( interfaceIterator->IsDoneWithTraversal() == 0 )
    {
    // Select blue component of leaf mesh
    double bComp = interfaceIterator->GetCurrentFlatIndex() == 2 ? 0 : 1;
    
    // Fetch interface object and downcast to data set
    vtkDataObject* interfaceDO = interfaceIterator->GetCurrentDataObject();
    vtkDataSet* interface = vtkDataSet::SafeDownCast( interfaceDO );

    // Create mapper for interface
    vtkNew<vtkDataSetMapper> interfaceMapper;
    interfaceMapper->SetInput( interface );
    interfaceIterator->GoToNextItem();
    interfaceMapper->ScalarVisibilityOff();
    interfaceMapper->SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
    interfaceMapper->SetResolveCoincidentTopologyToPolygonOffset();

    // Create surface actor and add it to view
    vtkNew<vtkActor> interfaceActor;
    interfaceActor->SetMapper( interfaceMapper.GetPointer() );
    interfaceActor->GetProperty()->SetColor( 0., 1 - bComp, bComp );
    interfaceActor->GetProperty()->SetRepresentationToSurface();
    renderer->AddViewProp( interfaceActor.GetPointer() );
    }
  
  // Render and test;
  window->Render();
  
  int retVal = vtkRegressionTestImage( window.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    interactor->Start();
    }
  
  return !retVal;
}
