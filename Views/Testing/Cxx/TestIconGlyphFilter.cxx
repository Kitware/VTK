
/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include <vtkIconGlyphFilter.h>

#include <vtkDoubleArray.h>
#include <vtkFollower.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphMapper.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPointSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPNGReader.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTexture.h>
#include <vtkImageData.h>

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

  
int TestIconGlyphFilter( int argc, char *argv[])
{
 // vtkRegressionTester::Result result = vtkRegressionTester::Passed;
 // vtkRegressionTester *test = new vtkRegressionTester("IconGlyphFilter");

  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/TangoIcons.png");

  int imageDims[3];

  vtkPNGReader * imageReader = vtkPNGReader::New();

  imageReader->SetFileName(fname);
  imageReader->Update();

  imageReader->GetOutput()->GetDimensions(imageDims);

  vtkMutableUndirectedGraph * pointSet = vtkMutableUndirectedGraph::New();
  vtkPoints * points = vtkPoints::New();
  vtkDoubleArray * pointData = vtkDoubleArray::New();
  pointData->SetNumberOfComponents(3);
  points->SetData((vtkDataArray *)pointData);
  pointSet->SetPoints(points);

  vtkIntArray * iconIndex = vtkIntArray::New();
  iconIndex->SetNumberOfComponents(1);

  pointSet->GetVertexData()->SetScalars(iconIndex);

  pointSet->AddVertex();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(2.0, 0.0, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(2.0, 3.50, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(0.0, -5.0, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(4.0, -5.0, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(-3.0, 5.0, 0.0);
  pointSet->AddVertex();
  points->InsertNextPoint(5.0, 0.0, 0.0);

  pointSet->AddEdge(0, 1);
  pointSet->AddEdge(1, 2);
  pointSet->AddEdge(2, 3);
  pointSet->AddEdge(3, 4);
  pointSet->AddEdge(4, 5);
  pointSet->AddEdge(5, 6);
  pointSet->AddEdge(6, 7);
  pointSet->AddEdge(7, 0);
 
 /* iconIndex->InsertNextTuple1(1);
  iconIndex->InsertNextTuple1(4);
  iconIndex->InsertNextTuple1(26);
  iconIndex->InsertNextTuple1(17);
  iconIndex->InsertNextTuple1(0);
  iconIndex->InsertNextTuple1(5);
  iconIndex->InsertNextTuple1(1);
  iconIndex->InsertNextTuple1(29);*/

  iconIndex->InsertNextTuple1(0);
  iconIndex->InsertNextTuple1(0);
  iconIndex->InsertNextTuple1(1);
  iconIndex->InsertNextTuple1(1);
  iconIndex->InsertNextTuple1(2);
  iconIndex->InsertNextTuple1(2);
  iconIndex->InsertNextTuple1(3);
  iconIndex->InsertNextTuple1(3);


  vtkGraphLayoutView *view = vtkGraphLayoutView::New();
  view->AddRepresentationFromInput(pointSet);
  
  vtkTexture * texture =  vtkTexture::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  view->SetIconTexture(texture);
  int size[] = {24, 24};
  view->SetIconSize(size);
  view->SetLayoutStrategyToPassThrough();

  vtkRenderWindow * renWin = vtkRenderWindow::New();
  renWin->SetSize(500, 500);
  view->SetupRenderWindow(renWin);



  renWin->GetInteractor()->Initialize();

  int retVal = vtkRegressionTestImageThreshold(renWin,18);
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renWin->GetInteractor()->Start();
    }

  //renWin->GetInteractor()->Start();

  imageReader->Delete();
  iconIndex->Delete();
  pointSet->Delete();
  points->Delete();
  pointData->Delete();
  view->Delete();
  renWin->Delete();
  texture->Delete();

  return retVal;
}

