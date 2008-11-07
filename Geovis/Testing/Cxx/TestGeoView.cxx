/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView.cxx

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

#include "vtkBMPReader.h"
#include "vtkCamera.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoGraphRepresentation.h"
#include "vtkGeoGraphRepresentation2D.h"
#include "vtkGeoProjection.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoRandomGraphSource.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTransform.h"
#include "vtkGeoView.h"
#include "vtkGeoView2D.h"
#include "vtkGraphLayoutView.h"
#include "vtkJPEGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"
#include "vtkViewTheme.h"
#include "vtkViewUpdater.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New();

int TestGeoView(int argc, char* argv[])
{
  // Create the geo view.
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkGeoView, view);
  view->SetupRenderWindow(win);
  win->SetSize(400,400);

  char* fname = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/masonry-wide.jpg");
  //char* fname = vtkTestUtilities::ExpandDataFileName(
  //    argc, argv, "Data/Clouds.tif");
  VTK_CREATE(vtkJPEGReader, reader);
  reader->SetFileName(fname);
  reader->Update();
  view->AddDefaultImageRepresentation(reader->GetOutput());

  // Add image representation
  char* fname2 = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  //char* fname2 = vtkTestUtilities::ExpandDataFileName(
  //    argc, argv, "Data/world.topo.bathy.200407.3x21600x10800.jpg");
  //char* fname2 = vtkTestUtilities::ExpandDataFileName(
  //    argc, argv, "Data/world.topo.bathy.200407.3x10000x5000.jpg");
  VTK_CREATE(vtkJPEGReader, reader2);
  reader2->SetFileName(fname2);
  reader2->Update();
  vtkSmartPointer<vtkGeoAlignedImageSource> imageSource =
    vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  imageSource->SetImage(reader2->GetOutput());
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  rep->SetSource(imageSource);
  view->AddRepresentation(rep);

  vtkSmartPointer<vtkGeoRandomGraphSource> source =
    vtkSmartPointer<vtkGeoRandomGraphSource>::New();
  source->SetNumberOfVertices(100);
  source->StartWithTreeOn();
  source->SetNumberOfEdges(0);
  
  vtkSmartPointer<vtkGeoGraphRepresentation> graphRep =
    vtkSmartPointer<vtkGeoGraphRepresentation>::New();
  graphRep->SetInputConnection(source->GetOutputPort());
  view->AddRepresentation(graphRep);

  view->Update();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    // If we are interactive, make some other views to link with.

    // Create a 2D geo view.
    int projIndex = 40;
    vtkSmartPointer<vtkRenderWindow> win2D =
      vtkSmartPointer<vtkRenderWindow>::New();
    vtkSmartPointer<vtkGeoView2D> view2D =
      vtkSmartPointer<vtkGeoView2D>::New();
    view2D->SetupRenderWindow(win2D);
    win2D->SetSize(400,400);

    vtkSmartPointer<vtkGeoAlignedImageSource> imageSource2D =
      vtkSmartPointer<vtkGeoAlignedImageSource>::New();
    imageSource2D->SetImage(reader2->GetOutput());
    vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep2D =
      vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
    rep2D->SetSource(imageSource2D);
    view2D->AddRepresentation(rep2D);

    vtkSmartPointer<vtkGeoProjectionSource> grat =
      vtkSmartPointer<vtkGeoProjectionSource>::New();
    grat->SetProjection(projIndex);

    // Set up the viewport
    vtkSmartPointer<vtkGeoTerrainNode> root = vtkSmartPointer<vtkGeoTerrainNode>::New();
    grat->FetchRoot(root);
    double bounds[4];
    root->GetProjectionBounds(bounds);
    view2D->GetRenderer()->GetActiveCamera()->SetParallelScale((bounds[3] - bounds[2]) / 2.0);

    vtkSmartPointer<vtkGeoTerrain2D> surf = vtkSmartPointer<vtkGeoTerrain2D>::New();
    surf->SetSource(grat);
    view2D->SetSurface(surf);

    vtkSmartPointer<vtkGeoGraphRepresentation2D> graphRep2D =
      vtkSmartPointer<vtkGeoGraphRepresentation2D>::New();
    graphRep2D->SetInputConnection(source->GetOutputPort());
    vtkSmartPointer<vtkGeoProjection> projection =
      vtkSmartPointer<vtkGeoProjection>::New();
    projection->SetName(projection->GetProjectionName(projIndex));
    vtkSmartPointer<vtkGeoTransform> transform =
      vtkSmartPointer<vtkGeoTransform>::New();
    transform->SetDestinationProjection(projection);
    graphRep2D->SetTransform(transform);
    view2D->AddRepresentation(graphRep2D);

    // Set up a graph layout view
    vtkSmartPointer<vtkGraphLayoutView> graphView =
      vtkSmartPointer<vtkGraphLayoutView>::New();
    vtkSmartPointer<vtkDataRepresentation> graphRep2 =
      vtkSmartPointer<vtkDataRepresentation>::New();
    graphRep2->SetInputConnection(source->GetOutputPort());
    graphView->AddRepresentation(graphRep2);
    vtkSmartPointer<vtkRenderWindow> graphWin =
      vtkSmartPointer<vtkRenderWindow>::New();
    graphWin->SetSize(400, 400);
    graphView->SetupRenderWindow(graphWin);

    graphRep2->SetSelectionLink(graphRep->GetSelectionLink());
    graphRep2D->SetSelectionLink(graphRep->GetSelectionLink());
    vtkSmartPointer<vtkViewUpdater> updater =
      vtkSmartPointer<vtkViewUpdater>::New();
    updater->AddView(view);
    updater->AddView(view2D);
    updater->AddView(graphView);

    graphView->GetRenderer()->ResetCamera();
    graphView->Update();

    // Make all the themes the same.
    vtkSmartPointer<vtkViewTheme> theme;
    theme.TakeReference(vtkViewTheme::CreateMellowTheme());
    theme->SetSelectedCellColor(1.0, 0.0, 1.0);
    theme->SetSelectedPointColor(1.0, 0.0, 1.0);
    graphView->ApplyViewTheme(theme);
    view->ApplyViewTheme(theme);
    graphRep->ApplyViewTheme(theme);
    graphRep2D->ApplyViewTheme(theme);

    // Interact with data.
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

  delete [] fname;
  delete [] fname2;
  return !retVal;
}

