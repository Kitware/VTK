
/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include <vtkIconGlyphFilter.h>

#include <vtkTexturedActor2D.h>
#include <vtkAppendPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPointSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPNGReader.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTexture.h>
#include <vtkImageData.h>

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

  
int TestIconGlyphFilterGravity( int argc, char *argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Tango/TangoIcons.png");

  int imageDims[3];

  vtkPNGReader * imageReader = vtkPNGReader::New();

  imageReader->SetFileName(fname);
  delete[] fname;
  imageReader->Update();

  imageReader->GetOutput()->GetDimensions(imageDims);

  vtkPolyData * pointSet = vtkPolyData::New();
  vtkPoints * points = vtkPoints::New();
  vtkDoubleArray * pointData = vtkDoubleArray::New();
  pointData->SetNumberOfComponents(3);
  points->SetData(pointData);
  pointSet->SetPoints(points);

  vtkIntArray * iconIndex = vtkIntArray::New();
  iconIndex->SetNumberOfComponents(1);

  pointSet->GetPointData()->SetScalars(iconIndex);

  for(double i = 1.0; i < 8; i++)
    {
    points->InsertNextPoint(i * 26.0, 26.0, 0.0);
    }

  for(int i = 0; i < points->GetNumberOfPoints(); i++)
    {
    iconIndex->InsertNextTuple1(i);
    }

  int size[] = {24, 24};
  
  vtkIconGlyphFilter * iconFilter = vtkIconGlyphFilter::New();

  iconFilter->SetInput(pointSet);
  iconFilter->SetIconSize(size);
  iconFilter->SetUseIconSize(true);
  iconFilter->SetIconSheetSize(imageDims);
  iconFilter->SetGravityToBottomLeft();

  vtkPolyData * pointSet2 = vtkPolyData::New();
  vtkPoints * points2 = vtkPoints::New();
  vtkDoubleArray * pointData2 = vtkDoubleArray::New();
  pointData2->SetNumberOfComponents(3);
  points2->SetData(pointData2);
  pointSet2->SetPoints(points2);

  vtkIntArray * iconIndex2 = vtkIntArray::New();
  iconIndex2->SetNumberOfComponents(1);

  pointSet2->GetPointData()->SetScalars(iconIndex2);

  for(double i = 1.0; i < 8; i++)
    {
    points2->InsertNextPoint(i * 26.0, 52.0, 0.0);
    }

  for(int i = 0; i < points2->GetNumberOfPoints(); i++)
    {
    iconIndex2->InsertNextTuple1(i + 8);
    }
  
  vtkIconGlyphFilter * iconFilter2 = vtkIconGlyphFilter::New();

  iconFilter2->SetInput(pointSet2);
  iconFilter2->SetIconSize(size);
  iconFilter2->SetUseIconSize(true);
  iconFilter2->SetIconSheetSize(imageDims);
  iconFilter2->SetGravityToBottomCenter();

  vtkPolyData * pointSet3 = vtkPolyData::New();
  vtkPoints * points3 = vtkPoints::New();
  vtkDoubleArray * pointData3 = vtkDoubleArray::New();
  pointData3->SetNumberOfComponents(3);
  points3->SetData(pointData3);
  pointSet3->SetPoints(points3);

  vtkIntArray * iconIndex3 = vtkIntArray::New();
  iconIndex3->SetNumberOfComponents(1);

  pointSet3->GetPointData()->SetScalars(iconIndex3);

  for(double i = 1.0; i < 8; i++)
    {
    points3->InsertNextPoint(i * 26.0, 78.0, 0.0);
    }

  for(int i = 0; i < points3->GetNumberOfPoints(); i++)
    {
    iconIndex3->InsertNextTuple1(i + 16);
    }
  
  vtkIconGlyphFilter * iconFilter3 = vtkIconGlyphFilter::New();

  iconFilter3->SetInput(pointSet3);
  iconFilter3->SetIconSize(size);
  iconFilter3->SetUseIconSize(true);
  iconFilter3->SetIconSheetSize(imageDims);
  iconFilter3->SetGravityToBottomRight();

  vtkPolyData * pointSet4 = vtkPolyData::New();
  vtkPoints * points4 = vtkPoints::New();
  vtkDoubleArray * pointData4 = vtkDoubleArray::New();
  pointData4->SetNumberOfComponents(3);
  points4->SetData(pointData4);
  pointSet4->SetPoints(points4);

  vtkIntArray * iconIndex4 = vtkIntArray::New();
  iconIndex4->SetNumberOfComponents(1);

  pointSet4->GetPointData()->SetScalars(iconIndex4);

  for(double i = 1.0; i < 8; i++)
    {
    points4->InsertNextPoint(i * 26.0, 104.0, 0.0);
    }

  for(int i = 0; i < points4->GetNumberOfPoints(); i++)
    {
    iconIndex4->InsertNextTuple1(i + 24);
    }
  
  vtkIconGlyphFilter * iconFilter4 = vtkIconGlyphFilter::New();

  iconFilter4->SetInput(pointSet4);
  iconFilter4->SetIconSize(size);
  iconFilter4->SetUseIconSize(true);
  iconFilter4->SetIconSheetSize(imageDims);
  iconFilter4->SetGravityToCenterLeft();

  vtkPolyData * pointSet5 = vtkPolyData::New();
  vtkPoints * points5 = vtkPoints::New();
  vtkDoubleArray * pointData5 = vtkDoubleArray::New();
  pointData5->SetNumberOfComponents(3);
  points5->SetData(pointData5);
  pointSet5->SetPoints(points5);

  vtkIntArray * iconIndex5 = vtkIntArray::New();
  iconIndex5->SetNumberOfComponents(1);

  pointSet5->GetPointData()->SetScalars(iconIndex5);

  for(double i = 1.0; i < 8; i++)
    {
    points5->InsertNextPoint(i * 26.0, 130.0, 0.0);
    }

  for(int i = 0; i < points5->GetNumberOfPoints(); i++)
    {
    iconIndex5->InsertNextTuple1(i + 32);
    }
  
  vtkIconGlyphFilter * iconFilter5 = vtkIconGlyphFilter::New();

  iconFilter5->SetInput(pointSet5);
  iconFilter5->SetIconSize(size);
  iconFilter5->SetUseIconSize(true);
  iconFilter5->SetIconSheetSize(imageDims);
  iconFilter5->SetGravityToCenterCenter();

  vtkPolyData * pointSet6 = vtkPolyData::New();
  vtkPoints * points6 = vtkPoints::New();
  vtkDoubleArray * pointData6 = vtkDoubleArray::New();
  pointData6->SetNumberOfComponents(3);
  points6->SetData(pointData6);
  pointSet6->SetPoints(points6);

  vtkIntArray * iconIndex6 = vtkIntArray::New();
  iconIndex6->SetNumberOfComponents(1);

  pointSet6->GetPointData()->SetScalars(iconIndex6);

  for(double i = 1.0; i < 8; i++)
    {
    points6->InsertNextPoint(i * 26.0, 156.0, 0.0);
    }

  for(int i = 0; i < points6->GetNumberOfPoints(); i++)
    {
    iconIndex6->InsertNextTuple1(i + 40);
    }
  
  vtkIconGlyphFilter * iconFilter6 = vtkIconGlyphFilter::New();

  iconFilter6->SetInput(pointSet6);
  iconFilter6->SetIconSize(size);
  iconFilter6->SetUseIconSize(true);
  iconFilter6->SetIconSheetSize(imageDims);
  iconFilter6->SetGravityToCenterRight();

  vtkPolyData * pointSet7 = vtkPolyData::New();
  vtkPoints * points7 = vtkPoints::New();
  vtkDoubleArray * pointData7 = vtkDoubleArray::New();
  pointData7->SetNumberOfComponents(3);
  points7->SetData(pointData7);
  pointSet7->SetPoints(points7);

  vtkIntArray * iconIndex7 = vtkIntArray::New();
  iconIndex7->SetNumberOfComponents(1);

  pointSet7->GetPointData()->SetScalars(iconIndex7);

  for(double i = 1.0; i < 8; i++)
    {
    points7->InsertNextPoint(i * 26.0, 182.0, 0.0);
    }

  for(int i = 0; i < points7->GetNumberOfPoints(); i++)
    {
    iconIndex7->InsertNextTuple1(i + 48);
    }
  
  vtkIconGlyphFilter * iconFilter7 = vtkIconGlyphFilter::New();

  iconFilter7->SetInput(pointSet7);
  iconFilter7->SetIconSize(size);
  iconFilter7->SetUseIconSize(true);
  iconFilter7->SetIconSheetSize(imageDims);
  iconFilter7->SetGravityToTopLeft();

  vtkPolyData * pointSet8 = vtkPolyData::New();
  vtkPoints * points8 = vtkPoints::New();
  vtkDoubleArray * pointData8 = vtkDoubleArray::New();
  pointData8->SetNumberOfComponents(3);
  points8->SetData(pointData8);
  pointSet8->SetPoints(points8);

  vtkIntArray * iconIndex8 = vtkIntArray::New();
  iconIndex8->SetNumberOfComponents(1);

  pointSet8->GetPointData()->SetScalars(iconIndex8);

  for(double i = 1.0; i < 8; i++)
    {
    points8->InsertNextPoint(i * 26.0, 208.0, 0.0);
    }

  for(int i = 0; i < points8->GetNumberOfPoints(); i++)
    {
    iconIndex8->InsertNextTuple1(i + 56);
    }
  
  vtkIconGlyphFilter * iconFilter8 = vtkIconGlyphFilter::New();

  iconFilter8->SetInput(pointSet8);
  iconFilter8->SetIconSize(size);
  iconFilter8->SetUseIconSize(true);
  iconFilter8->SetIconSheetSize(imageDims);
  iconFilter8->SetGravityToTopCenter();

  vtkPolyData * pointSet9 = vtkPolyData::New();
  vtkPoints * points9 = vtkPoints::New();
  vtkDoubleArray * pointData9 = vtkDoubleArray::New();
  pointData9->SetNumberOfComponents(3);
  points9->SetData(pointData9);
  pointSet9->SetPoints(points9);

  vtkIntArray * iconIndex9 = vtkIntArray::New();
  iconIndex9->SetNumberOfComponents(1);

  pointSet9->GetPointData()->SetScalars(iconIndex9);

  for(double i = 1.0; i < 8; i++)
    {
    points9->InsertNextPoint(i * 26.0, 234.0, 0.0);
    }

  for(int i = 0; i < points9->GetNumberOfPoints(); i++)
    {
    iconIndex9->InsertNextTuple1(i + 64);
    }
  
  vtkIconGlyphFilter * iconFilter9 = vtkIconGlyphFilter::New();

  iconFilter9->SetInput(pointSet9);
  iconFilter9->SetIconSize(size);
  iconFilter9->SetUseIconSize(true);
  iconFilter9->SetIconSheetSize(imageDims);
  iconFilter9->SetGravityToTopRight();

  vtkAppendPolyData * append = vtkAppendPolyData::New();
  append->AddInputConnection(iconFilter->GetOutputPort());
  append->AddInputConnection(iconFilter2->GetOutputPort());
  append->AddInputConnection(iconFilter3->GetOutputPort());
  append->AddInputConnection(iconFilter4->GetOutputPort());
  append->AddInputConnection(iconFilter5->GetOutputPort());
  append->AddInputConnection(iconFilter6->GetOutputPort());
  append->AddInputConnection(iconFilter7->GetOutputPort());
  append->AddInputConnection(iconFilter8->GetOutputPort());
  append->AddInputConnection(iconFilter9->GetOutputPort());

  vtkPolyDataMapper2D * mapper = vtkPolyDataMapper2D::New();
  mapper->SetInputConnection(append->GetOutputPort());

  vtkTexturedActor2D * iconActor = vtkTexturedActor2D::New();
  iconActor->SetMapper(mapper);
  
  vtkTexture * texture =  vtkTexture::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  iconActor->SetTexture(texture);

  vtkRenderer * renderer = vtkRenderer::New();
  vtkRenderWindow * renWin = vtkRenderWindow::New();
  renWin->SetSize(208, 260);
  renWin->AddRenderer(renderer);
  
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  renderer->AddActor(iconActor);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  mapper->Delete();
  texture->Delete();
  imageReader->Delete();
  iconIndex->Delete();
  iconFilter->Delete();
  iconActor->Delete();
  pointSet->Delete();
  points->Delete();
  pointData->Delete();
  iconIndex2->Delete();
  iconFilter2->Delete();
  pointSet2->Delete();
  points2->Delete();
  pointData2->Delete();
  iconIndex3->Delete();
  iconFilter3->Delete();
  pointSet3->Delete();
  points3->Delete();
  pointData3->Delete();
  iconIndex4->Delete();
  iconFilter4->Delete();
  pointSet4->Delete();
  points4->Delete();
  pointData4->Delete();
  iconIndex5->Delete();
  iconFilter5->Delete();
  pointSet5->Delete();
  points5->Delete();
  pointData5->Delete();
  iconIndex6->Delete();
  iconFilter6->Delete();
  pointSet6->Delete();
  points6->Delete();
  pointData6->Delete();
  iconIndex7->Delete();
  iconFilter7->Delete();
  pointSet7->Delete();
  points7->Delete();
  pointData7->Delete();
  iconIndex8->Delete();
  iconFilter8->Delete();
  pointSet8->Delete();
  points8->Delete();
  pointData8->Delete();
  iconIndex9->Delete();
  iconFilter9->Delete();
  pointSet9->Delete();
  points9->Delete();
  pointData9->Delete();
  append->Delete();

  return !retVal;
}

