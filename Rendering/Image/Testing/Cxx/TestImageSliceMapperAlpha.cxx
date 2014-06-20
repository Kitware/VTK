/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperAlpha.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test alpha blending RGBA, LA, Opacity<1.0, lookup table
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkImageMapToColors.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkPNGReader.h"

int TestImageSliceMapperAlpha(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkPNGReader *reader = vtkPNGReader::New();
  vtkPNGReader *reader2 = vtkPNGReader::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");
  char* fname2 = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/alphachannel.png");
  reader->SetFileName(fname);
  reader2->SetFileName(fname2);
  delete[] fname;
  delete[] fname2;

  vtkLookupTable *table = vtkLookupTable::New();
  table->SetRampToLinear();
  table->SetRange(0.0, 255.0);
  table->SetValueRange(0.0, 1.0);
  table->SetSaturationRange(0.0, 0.0);
  table->SetVectorModeToRGBColors();
  table->Build();

  vtkLookupTable *table2 = vtkLookupTable::New();
  table2->SetRampToLinear();
  table2->SetRange(0, 255);
  table2->SetHueRange(0.3, 0.3);
  table2->SetValueRange(0.0, 1.0);
  table2->SetSaturationRange(1.0, 1.0);
  table2->SetAlphaRange(0.0, 1.0);
  table2->SetVectorModeToComponent();
  table2->SetVectorComponent(3);
  table2->Build();

  vtkImageMapToColors *colors = vtkImageMapToColors::New();
  colors->SetInputConnection(reader2->GetOutputPort());
  colors->SetLookupTable(table);
  colors->PassAlphaToOutputOn();
  colors->SetOutputFormatToLuminanceAlpha();

  vtkImageMapToColors *colors2 = vtkImageMapToColors::New();
  colors2->SetInputConnection(reader2->GetOutputPort());
  colors2->SetLookupTable(table);
  colors2->SetOutputFormatToRGB();

  for (int i = 0; i < 4; i++)
    {
    vtkRenderer *renderer = vtkRenderer::New();
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1,0.2,0.4);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    vtkImageSliceMapper *imageMapper = vtkImageSliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(2000.0);
    image->GetProperty()->SetColorLevel(1000.0);
    imageMapper->Delete();

    vtkImageSliceMapper *imageMapper2 = vtkImageSliceMapper::New();
    vtkImageSlice *image2 = vtkImageSlice::New();
    image2->SetMapper(imageMapper2);
    imageMapper2->Delete();

    if (i == 0)
      {
      imageMapper2->SetInputConnection(reader2->GetOutputPort());
      }
    else if (i == 1)
      {
      imageMapper2->SetInputConnection(colors->GetOutputPort());
      }
    else if (i == 2)
      {
      imageMapper2->SetInputConnection(colors2->GetOutputPort());
      image2->GetProperty()->SetOpacity(0.5);
      }
    else
      {
      imageMapper2->SetInputConnection(reader2->GetOutputPort());
      image2->GetProperty()->SetLookupTable(table2);
      image2->GetProperty()->SetOpacity(0.9);
      }

    renderer->AddViewProp(image);
    renderer->AddViewProp(image2);
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(200.0);

    image->Delete();
    image2->Delete();
    }

  colors->Delete();
  colors2->Delete();
  table->Delete();
  table2->Delete();

  renWin->SetSize(400,400);

  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }
  iren->Delete();

  reader->Delete();
  reader2->Delete();

  return !retVal;
}
