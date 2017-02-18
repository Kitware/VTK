/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageReader2.h"
#include "vtkOpenGLImageGradient.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestOpenGLImageGradient(int argc, char *argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleImage> style;
  style->SetInteractionModeToImageSlicing();
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin.Get());
  iren->SetInteractorStyle(style.Get());

  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkNew<vtkImageReader2> reader;
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0,63,0,63,1,93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete [] fname;

  vtkNew<vtkOpenGLImageGradient> filter;
  //  vtkNew<vtkImageGradient> filter;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->Update();
  // double *rnger = filter->GetOutput()->GetPointData()->GetScalars()->GetRange();

  vtkNew<vtkImageSliceMapper> imageMapper;
  imageMapper->SetInputConnection(filter->GetOutputPort());
  imageMapper->SetOrientation(2);
  imageMapper->SliceAtFocalPointOn();

  vtkNew<vtkImageSlice> image;
  image->SetMapper(imageMapper.Get());

  double range[2] = { -100, 100 };

  image->GetProperty()->SetColorWindow(range[1] - range[0]);
  image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));
  image->GetProperty()->SetInterpolationTypeToNearest();

  vtkNew<vtkRenderer> renderer;
  renderer->AddViewProp(image.Get());
  renderer->SetBackground(0.2,0.3,0.4);
  renWin->AddRenderer(renderer.Get());

  const double *bounds = imageMapper->GetBounds();
  double point[3];
  point[0] = 0.5*(bounds[0] + bounds[1]);
  point[1] = 0.5*(bounds[2] + bounds[3]);
  point[2] = 0.5*(bounds[4] + bounds[5]);

  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(point);
  point[imageMapper->GetOrientation()] += 500.0;
  camera->SetPosition(point);
  if (imageMapper->GetOrientation() == 2)
  {
    camera->SetViewUp(0.0, 1.0, 0.0);
  }
  else
  {
    camera->SetViewUp(0.0, 0.0, -1.0);
  }
  camera->ParallelProjectionOn();
  camera->SetParallelScale(0.8*128);

  renWin->SetSize(512,512);
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
