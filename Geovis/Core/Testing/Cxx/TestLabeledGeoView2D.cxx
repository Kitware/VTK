/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabeledGeoView2D.cxx

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
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoFileImageSource.h"
#include "vtkGeoFileTerrainSource.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoRandomGraphSource.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoView2D.h"
#include "vtkJPEGReader.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"

#include <vtksys/ios/sstream>

int TestLabeledGeoView2D(int argc, char* argv[])
{
  int proj = 40;
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  vtkStdString imageFile = fname;

  // Create the view
  vtkSmartPointer<vtkGeoView2D> view = vtkSmartPointer<vtkGeoView2D>::New();
  view->DisplayHoverTextOff();

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
  vtkSmartPointer<vtkJPEGReader> reader =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(imageFile.c_str());
  reader->Update();
  alignedSource->SetImage(reader->GetOutput());
  imageSource.TakeReference(alignedSource);
  imageSource->Initialize();
  imageRep->SetSource(imageSource);
  view->AddRepresentation(imageRep);
  view->SetLabelPlacementModeToNoOverlap();

  vtkSmartPointer<vtkGeoRandomGraphSource> graphSource =
    vtkSmartPointer<vtkGeoRandomGraphSource>::New();
  graphSource->SetNumberOfVertices(1000);
  graphSource->SetNumberOfEdges(0);
  vtkSmartPointer<vtkRenderedGraphRepresentation> graphRep =
    vtkSmartPointer<vtkRenderedGraphRepresentation>::New();
  graphRep->SetInputConnection(graphSource->GetOutputPort());
  graphRep->SetVertexLabelArrayName("latitude");
  graphRep->SetColorVerticesByArray(true);
  graphRep->SetVertexColorArrayName("longitude");
  graphRep->SetVertexLabelVisibility(true);
  graphRep->SetLayoutStrategyToAssignCoordinates("longitude", "latitude");

  view->AddRepresentation(graphRep);

  // Set up the viewport
  view->GetRenderWindow()->SetSize(600, 600);
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

  view->ResetCamera();
  view->GetRenderer()->GetActiveCamera()->SetParallelScale(scale);

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

