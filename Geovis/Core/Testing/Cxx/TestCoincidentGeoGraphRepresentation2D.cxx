/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCoincidentGeoGraphRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .SECTION Description

#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoFileImageSource.h"
#include "vtkGeoFileTerrainSource.h"
#include "vtkGeoProjection.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoTransform.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoView2D.h"
#include "vtkGraphLayoutView.h"
#include "vtkIdTypeArray.h"
#include "vtkJPEGReader.h"
#include "vtkMath.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"

#include <sstream>

int TestCoincidentGeoGraphRepresentation2D(int argc, char* argv[])
{
  int projNum = 26; // eqc : Equidistant Cylindrical (Plate Caree)
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  vtkStdString imageFile = fname;

  // Create the view
  vtkSmartPointer<vtkGeoView2D> view = vtkSmartPointer<vtkGeoView2D>::New();
  view->DisplayHoverTextOff();
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere

  // Create the terrain
  vtkSmartPointer<vtkGeoTerrain2D> terrain =
    vtkSmartPointer<vtkGeoTerrain2D>::New();
  vtkSmartPointer<vtkGeoSource> terrainSource;
  vtkGeoProjectionSource* projSource = vtkGeoProjectionSource::New();
  projSource->SetProjection(projNum);
  projSource->Initialize();
  vtkSmartPointer<vtkGeoTransform> transform =
    vtkSmartPointer<vtkGeoTransform>::New();
  vtkSmartPointer<vtkGeoProjection> proj =
    vtkSmartPointer<vtkGeoProjection>::New();
  proj->SetName(vtkGeoProjection::GetProjectionName(projNum));
  transform->SetDestinationProjection(proj);
  terrainSource.TakeReference(projSource);
  terrain->SetSource(terrainSource);
  view->SetSurface(terrain);

  // Create background image
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> imageRep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  vtkSmartPointer<vtkGeoSource> imageSource;
  vtkGeoAlignedImageSource* alignedSource = vtkGeoAlignedImageSource::New();
  vtkSmartPointer<vtkJPEGReader> reader =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(imageFile.c_str());
  reader->Update();
  alignedSource->SetImage(reader->GetOutput());
  imageSource.TakeReference(alignedSource);
  imageSource->Initialize();
  imageRep->SetSource(imageSource);
  view->AddRepresentation(imageRep);

  // Add a graph representation
  vtkSmartPointer<vtkMutableUndirectedGraph> graph =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkSmartPointer<vtkDoubleArray> latArr =
    vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkDoubleArray> lonArr =
    vtkSmartPointer<vtkDoubleArray>::New();
  latArr->SetNumberOfTuples(128);
  lonArr->SetNumberOfTuples(128);
  latArr->SetName("latitude");
  lonArr->SetName("longitude");

  vtkSmartPointer<vtkIdTypeArray> colorScalars =
    vtkSmartPointer<vtkIdTypeArray>::New();
  colorScalars->SetName("stuff");

  vtkIdType v;

  for (v = 0; v < 20; ++v)
  {
    latArr->SetValue(v, 0.0);
    lonArr->SetValue(v, 0.0);
    graph->AddVertex();
  }

  for (v = 20; v < 40; ++v)
  {
    latArr->SetValue(v, 42);
    lonArr->SetValue(v, -73);
    graph->AddVertex();
  }

  for (v = 40; v < 49; ++v)
  {
    latArr->SetValue(v, 35);
    lonArr->SetValue(v, -106);
    graph->AddVertex();
  }

  for (v = 49; v < 66; ++v)
  {
    latArr->SetValue(v, 39);
    lonArr->SetValue(v, 116);
    graph->AddVertex();
  }

   for (v = 66; v < 80; ++v)
   {
    latArr->SetValue(v, -31);
    lonArr->SetValue(v, 115);
    graph->AddVertex();
   }

  for (v = 80; v < 105; ++v)
  {
    latArr->SetValue(v, 48.87);
    lonArr->SetValue(v, 2.29);
    graph->AddVertex();
  }

  for (v = 105; v < 122; ++v)
  {
    latArr->SetValue(v, -34.44);
    lonArr->SetValue(v, -59.20);
    graph->AddVertex();
  }

  // SANTAREM
  latArr->SetValue(122, -2.26);
  lonArr->SetValue(122, -54.41);
  graph->AddVertex();

  // CAIRO
  latArr->SetValue(123, 30.03);
  lonArr->SetValue(123, 31.15);
  graph->AddVertex();

  // TEHRAN
  latArr->SetValue(124, 35.40);
  lonArr->SetValue(124, 51.26);
  graph->AddVertex();

  // MOSCOW
  latArr->SetValue(125, 55.45);
  lonArr->SetValue(125, 37.42);
  graph->AddVertex();

  // CALCUTTA
  latArr->SetValue(126, 22.30);
  lonArr->SetValue(126, 88.20);
  graph->AddVertex();

  // JAKARTA
  latArr->SetValue(127, -6.08);
  lonArr->SetValue(127, 106.45);
  graph->AddVertex();

  graph->GetVertexData()->AddArray(latArr);
  graph->GetVertexData()->AddArray(lonArr);

  for (v = 1; v < 20; ++v)
  {
    graph->AddEdge(0, v);
  }

  for (v = 21; v < 40; ++v)
  {
    graph->AddEdge(20, v);
  }

  for (v = 41; v < 49; ++v)
  {
    graph->AddEdge(40, v);
  }

  for (v = 50; v < 66; ++v)
  {
    graph->AddEdge(49, v);
  }

  for (v = 67; v < 80; ++v)
  {
    graph->AddEdge(66, v);
  }

  for (v = 81; v < 105; ++v)
  {
    graph->AddEdge(80, v);
  }

  for (v = 106; v < 122; ++v)
  {
    graph->AddEdge(105, v);
  }

  graph->AddEdge(122, 123);
  graph->AddEdge(122, 20);
  graph->AddEdge(20, 40);
  graph->AddEdge(122, 105);
  graph->AddEdge(123, 124);
  graph->AddEdge(123, 0);
  graph->AddEdge(124, 125);
  graph->AddEdge(125, 80);
  graph->AddEdge(124, 126);
  graph->AddEdge(126, 49);
  graph->AddEdge(126, 127);
  graph->AddEdge(127, 66);

  vtkMath::RandomSeed(123456);
  for(int i = 0; i < 128; i++)
  {
    colorScalars->InsertNextValue(
      static_cast<vtkIdType>(vtkMath::Random(0, 1024)));
  }
  graph->GetVertexData()->AddArray(colorScalars);

  vtkSmartPointer<vtkRenderedGraphRepresentation> graphRep =
    vtkSmartPointer<vtkRenderedGraphRepresentation>::New();
  graphRep->SetInputData(graph);
  graphRep->SetVertexColorArrayName("stuff");
  graphRep->SetColorVerticesByArray(true);
  graphRep->SetLayoutStrategyToAssignCoordinates("longitude", "latitude");

  view->AddRepresentation(graphRep);

  terrain->SetSource(terrainSource);
  imageRep->SetSource(imageSource);

  // Set up the viewport
  view->GetRenderWindow()->SetSize(900, 600);
  view->Render();
  view->ResetCamera();
  view->GetRenderer()->GetActiveCamera()->Zoom(2.1);
  view->Render();
  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }

  terrainSource->ShutDown();
  imageSource->ShutDown();

  delete [] fname;
  return !retVal;
}

