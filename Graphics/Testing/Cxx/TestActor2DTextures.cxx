
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

  
int TestActor2DTextures( int argc, char *argv[])
{
 // vtkRegressionTester::Result result = vtkRegressionTester::Passed;
 // vtkRegressionTester *test = new vtkRegressionTester("IconGlyphFilter");

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
    for(double j = 1.0; j < 8; j++)
      {
      points->InsertNextPoint(i * 26.0, j * 26.0, 0.0);
      }
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

  vtkPolyDataMapper2D * mapper = vtkPolyDataMapper2D::New();
  mapper->SetInputConnection(iconFilter->GetOutputPort());

  vtkTexturedActor2D * iconActor = vtkTexturedActor2D::New();
  iconActor->SetMapper(mapper);
  
  vtkTexture * texture =  vtkTexture::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  iconActor->SetTexture(texture);

  vtkRenderer * renderer = vtkRenderer::New();
  vtkRenderWindow * renWin = vtkRenderWindow::New();
  renWin->SetSize(208, 208);
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

  return !retVal;
}

