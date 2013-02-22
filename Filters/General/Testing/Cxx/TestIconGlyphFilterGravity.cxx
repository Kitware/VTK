
/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "vtkSmartPointer.h"
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

int TestIconGlyphFilterGravity( int argc, char *argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Tango/TangoIcons.png");

  int imageDims[3];

  vtkSmartPointer<vtkPNGReader>  imageReader =
    vtkSmartPointer<vtkPNGReader>::New();

  imageReader->SetFileName(fname);
  delete[] fname;
  imageReader->Update();

  imageReader->GetOutput()->GetDimensions(imageDims);

  vtkSmartPointer<vtkPolyData>  pointSet =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData->SetNumberOfComponents(3);
  points->SetData(pointData);
  pointSet->SetPoints(points);

  vtkSmartPointer<vtkIntArray>  iconIndex =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter->SetInputData(pointSet);
  iconFilter->SetIconSize(size);
  iconFilter->SetUseIconSize(true);
  iconFilter->SetIconSheetSize(imageDims);
  iconFilter->SetGravityToBottomLeft();

  vtkSmartPointer<vtkPolyData>  pointSet2 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points2 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData2 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData2->SetNumberOfComponents(3);
  points2->SetData(pointData2);
  pointSet2->SetPoints(points2);

  vtkSmartPointer<vtkIntArray>  iconIndex2 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter2 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter2->SetInputData(pointSet2);
  iconFilter2->SetIconSize(size);
  iconFilter2->SetUseIconSize(true);
  iconFilter2->SetIconSheetSize(imageDims);
  iconFilter2->SetGravityToBottomCenter();

  vtkSmartPointer<vtkPolyData>  pointSet3 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points3 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData3 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData3->SetNumberOfComponents(3);
  points3->SetData(pointData3);
  pointSet3->SetPoints(points3);

  vtkSmartPointer<vtkIntArray>  iconIndex3 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter3 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter3->SetInputData(pointSet3);
  iconFilter3->SetIconSize(size);
  iconFilter3->SetUseIconSize(true);
  iconFilter3->SetIconSheetSize(imageDims);
  iconFilter3->SetGravityToBottomRight();

  vtkSmartPointer<vtkPolyData>  pointSet4 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points4 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData4 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData4->SetNumberOfComponents(3);
  points4->SetData(pointData4);
  pointSet4->SetPoints(points4);

  vtkSmartPointer<vtkIntArray>  iconIndex4 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter4 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter4->SetInputData(pointSet4);
  iconFilter4->SetIconSize(size);
  iconFilter4->SetUseIconSize(true);
  iconFilter4->SetIconSheetSize(imageDims);
  iconFilter4->SetGravityToCenterLeft();

  vtkSmartPointer<vtkPolyData>  pointSet5 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points5 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData5 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData5->SetNumberOfComponents(3);
  points5->SetData(pointData5);
  pointSet5->SetPoints(points5);

  vtkSmartPointer<vtkIntArray>  iconIndex5 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter5 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter5->SetInputData(pointSet5);
  iconFilter5->SetIconSize(size);
  iconFilter5->SetUseIconSize(true);
  iconFilter5->SetIconSheetSize(imageDims);
  iconFilter5->SetGravityToCenterCenter();

  vtkSmartPointer<vtkPolyData>  pointSet6 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points6 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData6 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData6->SetNumberOfComponents(3);
  points6->SetData(pointData6);
  pointSet6->SetPoints(points6);

  vtkSmartPointer<vtkIntArray>  iconIndex6 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter6 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter6->SetInputData(pointSet6);
  iconFilter6->SetIconSize(size);
  iconFilter6->SetUseIconSize(true);
  iconFilter6->SetIconSheetSize(imageDims);
  iconFilter6->SetGravityToCenterRight();

  vtkSmartPointer<vtkPolyData>  pointSet7 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points7 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData7 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData7->SetNumberOfComponents(3);
  points7->SetData(pointData7);
  pointSet7->SetPoints(points7);

  vtkSmartPointer<vtkIntArray>  iconIndex7 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter7 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter7->SetInputData(pointSet7);
  iconFilter7->SetIconSize(size);
  iconFilter7->SetUseIconSize(true);
  iconFilter7->SetIconSheetSize(imageDims);
  iconFilter7->SetGravityToTopLeft();

  vtkSmartPointer<vtkPolyData>  pointSet8 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points8 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData8 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData8->SetNumberOfComponents(3);
  points8->SetData(pointData8);
  pointSet8->SetPoints(points8);

  vtkSmartPointer<vtkIntArray>  iconIndex8 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter8 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter8->SetInputData(pointSet8);
  iconFilter8->SetIconSize(size);
  iconFilter8->SetUseIconSize(true);
  iconFilter8->SetIconSheetSize(imageDims);
  iconFilter8->SetGravityToTopCenter();

  vtkSmartPointer<vtkPolyData>  pointSet9 =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints>  points9 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray>  pointData9 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointData9->SetNumberOfComponents(3);
  points9->SetData(pointData9);
  pointSet9->SetPoints(points9);

  vtkSmartPointer<vtkIntArray>  iconIndex9 =
    vtkSmartPointer<vtkIntArray>::New();
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

  vtkSmartPointer<vtkIconGlyphFilter>  iconFilter9 =
    vtkSmartPointer<vtkIconGlyphFilter>::New();

  iconFilter9->SetInputData(pointSet9);
  iconFilter9->SetIconSize(size);
  iconFilter9->SetUseIconSize(true);
  iconFilter9->SetIconSheetSize(imageDims);
  iconFilter9->SetGravityToTopRight();

  vtkSmartPointer<vtkAppendPolyData>  append =
    vtkSmartPointer<vtkAppendPolyData>::New();
  append->AddInputConnection(iconFilter->GetOutputPort());
  append->AddInputConnection(iconFilter2->GetOutputPort());
  append->AddInputConnection(iconFilter3->GetOutputPort());
  append->AddInputConnection(iconFilter4->GetOutputPort());
  append->AddInputConnection(iconFilter5->GetOutputPort());
  append->AddInputConnection(iconFilter6->GetOutputPort());
  append->AddInputConnection(iconFilter7->GetOutputPort());
  append->AddInputConnection(iconFilter8->GetOutputPort());
  append->AddInputConnection(iconFilter9->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper2D>  mapper =
    vtkSmartPointer<vtkPolyDataMapper2D>::New();
  mapper->SetInputConnection(append->GetOutputPort());

  vtkSmartPointer<vtkTexturedActor2D>  iconActor =
    vtkSmartPointer<vtkTexturedActor2D>::New();
  iconActor->SetMapper(mapper);

  vtkSmartPointer<vtkTexture>  texture =
    vtkSmartPointer<vtkTexture>::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  iconActor->SetTexture(texture);

  vtkSmartPointer<vtkRenderer>  renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow>  renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(208, 260);
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
    iren->SetRenderWindow(renWin);

  renderer->AddActor(iconActor);
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
