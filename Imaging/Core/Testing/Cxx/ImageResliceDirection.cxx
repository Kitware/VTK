// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test the vtkImageReslice SetOutputDirection method
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkNew.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageReader2.h"
#include "vtkImageReslice.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyleImage.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkTestUtilities.h"

int ImageResliceDirection(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleImage> style;
  style->SetInteractionModeToImageSlicing();
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkNew<vtkImageReader2> reader;
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete[] fname;

  double range[2] = { 0.0, 4095.0 };

  // the columns of the direction matrix give the directions
  // that will map to the screen's horizontal (left->right),
  // vertical (bottom->top), and depth (into the screen)
  const double directions[4][9] = {
    // lower left, sagittal
    { 1.0, 0.0, 0.0,    // 1st
      0.0, 0.0, 1.0,    // 2nd
      0.0, -1.0, 0.0 }, // 3rd
    // lower right, oblique
    { 0.3610509009504489, 0.5641239080948949, 0.7425674805959468,    // 1st
      -0.8708194756386795, 0.48884072076035906, 0.05204027838960906, // 2nd
      -0.333640057204234, -0.6654314134771782, 0.6677464684942334 }, // 3rd
    // upper left, axial
    { 1.0, 0.0, 0.0,   // 1st
      0.0, 1.0, 0.0,   // 2nd
      0.0, 0.0, 1.0 }, // 3rd
    // upper right, coronal (matrix has a flip)
    { 0.0, 0.0, 1.0,    // 1st
      1.0, 0.0, 0.0,    // 2nd
      0.0, -1.0, 0.0 }, // 3rd
  };

  for (int i = 0; i < 4; i++)
  {
    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputConnection(reader->GetOutputPort());
    reslice->SetOutputSpacing(1.0, 1.0, 1.0);
    reslice->SetOutputDirection(directions[i]);
    reslice->SetInterpolationModeToLinear();

    vtkNew<vtkImageSliceMapper> imageMapper;
    imageMapper->SetInputConnection(reslice->GetOutputPort());
    imageMapper->SliceAtFocalPointOn();
    imageMapper->BorderOn();

    vtkNew<vtkImageSlice> image;
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    vtkNew<vtkRenderer> renderer;
    renderer->AddViewProp(image);
    renderer->SetBackground(0.2, 0.2, 0.2);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);

    // use center point to set camera
    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    double offset[3] = { 0.0, 0.0, 500.0 };
    vtkMatrix3x3::MultiplyPoint(directions[i], offset, offset);
    vtkMath::Add(point, offset, point);
    camera->SetPosition(point);
    double viewup[3] = { 0.0, 1.0, 0.0 };
    vtkMatrix3x3::MultiplyPoint(directions[i], viewup, viewup);
    camera->SetViewUp(viewup);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);
  }

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
