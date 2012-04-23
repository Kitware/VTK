/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the Nearest, Linear, and Cubic interpolation.
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
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkPNGReader.h"
#include "vtkLookupTable.h"

int TestImageSliceMapperInterpolation(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetSize(400,400);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkPNGReader *reader = vtkPNGReader::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
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

    vtkImageSliceMapper *imageMapper = vtkImageSliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());

    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);

    if (i < 3)
      {
      image->GetProperty()->SetColorWindow(1000);
      image->GetProperty()->SetColorLevel(1500);
      image->GetProperty()->SetInterpolationType(i);
      }
    else
      {
      vtkLookupTable *table = vtkLookupTable::New();
      table->Build();
      table->SetRange(1000, 2000);
      image->GetProperty()->SetLookupTable(table);
      image->GetProperty()->UseLookupTableScalarRangeOn();
      table->Delete();
      }

    image->Delete();
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(10.0);
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
