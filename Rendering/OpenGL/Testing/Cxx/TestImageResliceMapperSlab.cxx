/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperSlab.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the slab modes of vtkImageResliceMapper
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
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageReader2.h"
#include "vtkLookupTable.h"
#include "vtkPlane.h"

int TestImageResliceMapperSlab(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetSize(400,400);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkImageReader2 *reader = vtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetDataOrigin(-100.8, -100.9, -69.0);
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;

  for (int i = 0; i < 4; i++)
    {
    vtkRenderer *renderer = vtkRenderer::New();
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    vtkImageResliceMapper *imageMapper = vtkImageResliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SetSlabThickness(20);
    imageMapper->SliceFacesCameraOn();

    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    image->GetProperty()->SetInterpolationTypeToLinear();
    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);
    renderer->AddViewProp(image);

    if (i < 3)
      {
      if (i == 0)
        {
        imageMapper->SetSlabTypeToMin();
        }
      if (i == 1)
        {
        imageMapper->SetSlabTypeToMax();
        }
      if (i == 2)
        {
        imageMapper->SetSlabTypeToMean();
        }
      if (i != 2)
        {
        camera->Azimuth(90);
        camera->Roll(85);
        camera->Azimuth(40);
        camera->Elevation(30);
        }
      }
    else
      {
      imageMapper->ResampleToScreenPixelsOff();
      imageMapper->SetSlabTypeToSum();
      imageMapper->SetSlabThickness(100);
      image->GetProperty()->SetColorWindow(2000*100);
      image->GetProperty()->SetColorLevel(1000*100);
      camera->Azimuth(91);
      camera->Roll(90);
      }

    image->Delete();
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(120.0);
    }

  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }
  iren->Delete();

  reader->Delete();

  return !retVal;
}
