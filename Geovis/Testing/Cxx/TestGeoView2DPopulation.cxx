/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView2DPopulation.cxx

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
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoFileImageSource.h"
#include "vtkGeoFileTerrainSource.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoView2D.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkJPEGReader.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"
#include "vtkTimerLog.h"
#include "vtkViewTheme.h"

#include "vtkAbstractArray.h"
#include "vtkTable.h"
#include "vtkDelimitedTextReader.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTableToPolyData.h"
#include "vtkTextProperty.h"

#include <vtksys/ios/sstream>

int TestGeoView2DPopulation(int argc, char* argv[])
{
  int proj = 33;
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  vtkStdString imageFile = fname;

  char* dbName = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/CityPopulationsUTF8.txt");
  vtkStdString databaseFile = dbName;

  // Create the view
  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkGeoView2D> view = vtkSmartPointer<vtkGeoView2D>::New();
  view->SetLabelRenderModeToQt();
  //view->SetupRenderWindow(win);

  // Create the terrain
  vtkSmartPointer<vtkGeoTerrain2D> terrain =
    vtkSmartPointer<vtkGeoTerrain2D>::New();
  vtkSmartPointer<vtkGeoSource> terrainSource;
  vtkGeoProjectionSource* projSource = vtkGeoProjectionSource::New();
  projSource->SetProjection(proj);
  terrainSource.TakeReference(projSource);
  terrainSource->Initialize();
  terrain->SetSource(terrainSource);
  view->SetSurface(terrain);

  // Create background image
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> imageRep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  vtkSmartPointer<vtkGeoSource> imageSource;
  vtkGeoAlignedImageSource* alignedSource = vtkGeoAlignedImageSource::New();
  vtkSmartPointer<vtkJPEGReader> reader = vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(imageFile.c_str());
  reader->Update();
  alignedSource->SetImage(reader->GetOutput());
  imageSource.TakeReference(alignedSource);
  imageSource->Initialize();
  imageRep->SetSource(imageSource);
  view->AddRepresentation(imageRep);
  view->SetLabelPlacementModeToLabelPlacer();

  // Read in Database of Cities with Population databaseFile
  vtkSmartPointer<vtkDelimitedTextReader> textReader =
    vtkSmartPointer<vtkDelimitedTextReader>::New();
  textReader->SetFileName(databaseFile);
  textReader->SetHaveHeaders(true);
  textReader->SetDetectNumericColumns(true);
  textReader->SetFieldDelimiterCharacters("\t");

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  textReader->Update();
  timer->StopTimer();
  vtkTable * table = textReader->GetOutput();
  vtkIntArray * priority;

  cout << "Reading time: " << timer->GetElapsedTime() << endl;
  cout << "Number of Columns: " << table->GetNumberOfColumns() << endl;
  cout << "Number of Rows: " << table->GetNumberOfRows() << endl;
  cout << "Column1 Name: " << table->GetColumnName(0) << "." << endl;

  int numRows = table->GetNumberOfRows();
  int i = 0;

  vtkSmartPointer<vtkDoubleArray> colorArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  colorArray->SetName("Colors");
  colorArray->SetNumberOfTuples(numRows);

  priority = vtkIntArray::SafeDownCast(table->GetColumnByName("Priority"));
  
  for(i = 0; i < numRows; i++)
    {
    colorArray->SetValue(i, log(static_cast<double>(priority->GetValue(i)) + 1.0)); 
    }

  timer->StartTimer();


  vtkSmartPointer<vtkMutableUndirectedGraph > graph = 
    vtkSmartPointer<vtkMutableUndirectedGraph >::New();
  graph->GetVertexData()->PassData(table->GetRowData());
  graph->GetVertexData()->AddArray(colorArray);

  for(i = 0; i < numRows; i++)
    {
    graph->AddVertex();
    }

  vtkSmartPointer<vtkRenderedGraphRepresentation> graphRep = 
    vtkSmartPointer<vtkRenderedGraphRepresentation>::New();
  graphRep->SetInput(graph);
  graphRep->SetVertexLabelArrayName("LabelText1");
  graphRep->SetVertexLabelPriorityArrayName("Priority");
  graphRep->SetVertexLabelVisibility(true);
  graphRep->SetColorVerticesByArray(true);
  graphRep->SetVertexColorArrayName("Colors");
  graphRep->SetEdgeVisibility(false);
  graphRep->SetLayoutStrategyToAssignCoordinates("Longitude", "Latitude");
  graphRep->GetVertexLabelTextProperty()->ShadowOn();
  
  view->AddRepresentation(graphRep);

  timer->StopTimer();
  cout << "GraphCreation time: " << timer->GetElapsedTime() << endl;

  vtkSmartPointer<vtkViewTheme> theme = vtkSmartPointer<vtkViewTheme>::New();
  view->ApplyViewTheme(theme);


  // Set up the viewport
  win->SetSize(600, 600);
  vtkSmartPointer<vtkGeoTerrainNode> root =
    vtkSmartPointer<vtkGeoTerrainNode>::New();
  terrainSource->FetchRoot(root);
  double bounds[6];
  root->GetModel()->GetBounds(bounds);
  bounds[0] = bounds[0] - (bounds[1] - bounds[0])*0.01;
  bounds[1] = bounds[1] + (bounds[1] - bounds[0])*0.01;
  bounds[2] = bounds[2] - (bounds[3] - bounds[2])*0.01;
  bounds[3] = bounds[3] + (bounds[3] - bounds[2])*0.01;
  double scalex = (bounds[1] - bounds[0])/2.0;
  double scaley = (bounds[3] - bounds[2])/2.0;
  double scale = (scalex > scaley) ? scalex : scaley;
  
  view->GetRenderer()->ResetCamera();
  view->GetRenderer()->GetActiveCamera()->SetParallelScale(scale);

  view->Update();
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();
    }

  terrainSource->ShutDown();
  imageSource->ShutDown();

  delete [] fname;
  return !retVal;
}

