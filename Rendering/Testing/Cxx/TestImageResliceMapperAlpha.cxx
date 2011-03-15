/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperAlpha.cxx

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
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageReader2.h"
#include "vtkImageGridSource.h"

int TestImageResliceMapperAlpha(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkImageReader2 *reader = vtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;

  vtkImageGridSource *grid = vtkImageGridSource::New();
  grid->SetDataExtent(0, 60, 0, 60, 1, 93);
  grid->SetDataSpacing(3.2, 3.2, 1.5);
  grid->SetDataOrigin(0, 0, 0);
  grid->SetDataScalarTypeToUnsignedChar();
  grid->SetLineValue(255);

  vtkLookupTable *table = vtkLookupTable::New();
  table->SetRampToLinear();
  table->SetRange(0.0, 255.0);
  table->SetValueRange(1.0, 1.0);
  table->SetSaturationRange(0.0, 0.0);
  table->SetAlphaRange(0.0, 1.0);
  table->Build();

  vtkLookupTable *table2 = vtkLookupTable::New();
  table2->SetRampToLinear();
  table2->SetRange(0.0, 255.0);
  table2->SetValueRange(1.0, 1.0);
  table2->SetHueRange(0.2, 0.4);
  table2->SetSaturationRange(1.0, 1.0);
  table2->SetAlphaRange(0.5, 1.0);
  table2->Build();

  vtkImageMapToColors *colors = vtkImageMapToColors::New();
  colors->SetInputConnection(grid->GetOutputPort());
  colors->SetLookupTable(table);
  colors->PassAlphaToOutputOn();
  colors->SetOutputFormatToLuminanceAlpha();

  vtkImageMapToColors *colors2 = vtkImageMapToColors::New();
  colors2->SetInputConnection(grid->GetOutputPort());
  colors2->SetLookupTable(table2);
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

    vtkImageResliceMapper *imageMapper = vtkImageResliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceFacesCameraOn();
    imageMapper->SliceAtFocalPointOn();
    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(2000.0);
    image->GetProperty()->SetColorLevel(1000.0);
    imageMapper->Delete();

    vtkImageResliceMapper *imageMapper2 = vtkImageResliceMapper::New();
    imageMapper2->SliceFacesCameraOn();
    imageMapper2->SliceAtFocalPointOn();
    vtkImageSlice *image2 = vtkImageSlice::New();
    image2->SetMapper(imageMapper2);
    imageMapper2->Delete();

    if (i == 0)
      {
      imageMapper2->SetInputConnection(grid->GetOutputPort());
      image2->GetProperty()->SetOpacity(0.5);
      }
    else if (i == 1)
      {
      imageMapper2->SetInputConnection(colors->GetOutputPort());
      camera->Elevation(30);
      }
    else if (i == 2)
      {
      imageMapper2->SetInputConnection(colors2->GetOutputPort());
      image2->GetProperty()->SetOpacity(0.5);
      }
    else
      {
      imageMapper2->SetInputConnection(grid->GetOutputPort());
      image2->GetProperty()->SetLookupTable(table2);
      image2->GetProperty()->SetOpacity(0.9);
      image->RotateWXYZ(30, 1, 0.5, 0);
      }

    renderer->AddViewProp(image);
    renderer->AddViewProp(image2);
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(110.0);

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
  grid->Delete();

  return !retVal;
}
