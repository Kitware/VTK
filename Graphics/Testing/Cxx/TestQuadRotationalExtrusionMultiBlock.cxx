/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadRotationalExtrusionMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pébay, Kitware SAS 2011

#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkQuadRotationalExtrusionFilter.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

//----------------------------------------------------------------------------
int TestQuadRotationalExtrusionMultiBlock( int argc, char * argv [] )
{
  // Read block 0 of 2D polygonal input mesh
  char* fName0 = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/SemiDisk/SemiDisk-0.vtp");
  vtkNew<vtkXMLPolyDataReader> reader0;
  reader0->SetFileName( fName0 );
  reader0->Update();
  delete [] fName0;
 
  // Read block 1 of 2D polygonal input mesh
  char* fName1 = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/SemiDisk/SemiDisk-1.vtp");
  vtkNew<vtkXMLPolyDataReader> reader1;
  reader1->SetFileName( fName1 );
  reader1->Update();
  delete [] fName1;

  // Create multi-block data set for quad-based sweep
  vtkNew<vtkMultiBlockDataSet> inMesh;
  inMesh->SetNumberOfBlocks( 2 );
  inMesh->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Block 0" ); 
  inMesh->SetBlock( 0, reader0->GetOutput() );
  inMesh->GetMetaData( static_cast<unsigned>( 1 ) )->Set( vtkCompositeDataSet::NAME(), "Block 1" ); 
  inMesh->SetBlock( 1, reader1->GetOutput() );

  // Create 3/4 of a cylinder by rotational extrusion
  vtkNew<vtkQuadRotationalExtrusionFilter> sweeper;
  sweeper->SetResolution( 18 );
  sweeper->SetInput( inMesh.GetPointer() );
  sweeper->SetAxisToX();
  sweeper->SetDefaultAngle( 270 );
  sweeper->AddPerBlockAngle( 1, 90. );
  sweeper->AddPerBlockAngle( 2, 45.) ;

  // Turn composite output into single polydata
  vtkNew<vtkCompositeDataGeometryFilter> outMesh;
  outMesh->SetInputConnection( sweeper->GetOutputPort() );

  // Create normals for smooth rendering
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection( outMesh->GetOutputPort() );

  // Create mapper for surface representation of whole mesh
  vtkNew<vtkPolyDataMapper> outMeshMapper;
  outMeshMapper->SetInputConnection( normals->GetOutputPort() );
  outMeshMapper->SetResolveCoincidentTopologyPolygonOffsetParameters( 0., 1 );
  outMeshMapper->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for surface representation of whole mesh
  vtkNew<vtkActor> outMeshActor;
  outMeshActor->SetMapper( outMeshMapper.GetPointer() );
  outMeshActor->GetProperty()->SetRepresentationToSurface();
  outMeshActor->GetProperty()->SetInterpolationToGouraud();
  outMeshActor->GetProperty()->SetColor( .9, .9, .9 );

  // Retrieve polydata blocks output by sweeper
  sweeper->Update();
  vtkMultiBlockDataSet* outMeshMB = sweeper->GetOutput();
  vtkPolyData* outMesh0 = vtkPolyData::SafeDownCast( outMeshMB->GetBlock( 0 ) );
  vtkPolyData* outMesh1 = vtkPolyData::SafeDownCast( outMeshMB->GetBlock( 1 ) );
  
  // Create mapper for wireframe representation of block 0
  vtkNew<vtkPolyDataMapper> outBlockMapper0;
  outBlockMapper0->SetInput( outMesh0 );
  outBlockMapper0->SetResolveCoincidentTopologyPolygonOffsetParameters( 1., 1 );
  outBlockMapper0->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for wireframe representation of block 0
  vtkNew<vtkActor> outBlockActor0;
  outBlockActor0->SetMapper( outBlockMapper0.GetPointer() );
  outBlockActor0->GetProperty()->SetRepresentationToWireframe();
  outBlockActor0->GetProperty()->SetColor( .9, 0., 0.);
  outBlockActor0->GetProperty()->SetAmbient( 1. );
  outBlockActor0->GetProperty()->SetDiffuse( 0. );
  outBlockActor0->GetProperty()->SetSpecular( 0. );

  // Create mapper for wireframe representation of block 1
  vtkNew<vtkPolyDataMapper> outBlockMapper1;
  outBlockMapper1->SetInput( outMesh1 );
  outBlockMapper1->SetResolveCoincidentTopologyPolygonOffsetParameters( 1., 1 );
  outBlockMapper1->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for wireframe representation of block 1
  vtkNew<vtkActor> outBlockActor1;
  outBlockActor1->SetMapper( outBlockMapper1.GetPointer() );
  outBlockActor1->GetProperty()->SetRepresentationToWireframe();
  outBlockActor1->GetProperty()->SetColor( 0., .9, 0.);
  outBlockActor1->GetProperty()->SetAmbient( 1. );
  outBlockActor1->GetProperty()->SetDiffuse( 0. );
  outBlockActor1->GetProperty()->SetSpecular( 0. );

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> ren1;
  ren1->AddActor( outMeshActor.GetPointer() );
  ren1->AddActor( outBlockActor0.GetPointer() );
  ren1->AddActor( outBlockActor1.GetPointer() );
  ren1->SetBackground( 1., 1., 1. );

  // Create a renderWindow
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer( ren1.GetPointer() );
  renWin->SetSize( 400, 400 );
  renWin->SetMultiSamples( 0 );
  
  // Create a good view angle
  vtkNew<vtkCamera> camera;
  //camera->SetClippingRange( 0.576398, 28.8199 );
  camera->SetFocalPoint( 36.640094041788934, 0.3387609170199118, 1.2087523663629445 );
  camera->SetPosition( 37.77735939083618, 0.42739828159854326, 2.988046512725565 );
  camera->SetViewUp( -0.40432906992858864, 0.8891923825021084, 0.21413759621072337 );
  camera->SetViewAngle( 30. );
  ren1->SetActiveCamera( camera.GetPointer() );

  // Create interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow( renWin.GetPointer() );

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
