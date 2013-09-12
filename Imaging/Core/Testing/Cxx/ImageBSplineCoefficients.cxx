/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageBSplineCoefficients.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the computation of b-spline coefficients for image interpolation
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkSmartPointer.h"

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
#include "vtkImageBSplineInterpolator.h"
#include "vtkImageBSplineCoefficients.h"

#include "vtkTestUtilities.h"

int ImageBSplineCoefficients(int argc, char *argv[])
{
  int retVal = EXIT_SUCCESS;
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyle> style =
    vtkSmartPointer<vtkInteractorStyle>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  vtkSmartPointer<vtkPNGReader> reader =
    vtkSmartPointer<vtkPNGReader>::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  reader->SetDataSpacing(0.8, 0.8, 1.5);
  delete[] fname;

  double range[2], spacing[3], origin[3];
  reader->Update();
  reader->GetOutput()->GetScalarRange(range);
  reader->GetOutput()->GetOrigin(origin);
  reader->GetOutput()->GetSpacing(spacing);

  vtkSmartPointer<vtkImageBSplineCoefficients> coeffs =
    vtkSmartPointer<vtkImageBSplineCoefficients>::New();
  coeffs->SetInputConnection(reader->GetOutputPort());
  coeffs->Update();

  vtkSmartPointer<vtkImageBSplineInterpolator> interp =
    vtkSmartPointer<vtkImageBSplineInterpolator>::New();

  double points[20][3] = {
    { 84.75451, 130.78060, 0.0 },
    { 186.06953, 154.55128, 0.0 },
    { 154.13078, 197.45840, 0.0 },
    { 145.61198, 36.34792, 0.0 },
    { 40.33874, 30.06436, 0.0 },
    { 21.36589, 188.13579, 0.0 },
    { 200.00869, 13.00973, 0.0 },
    { 203.80943, 84.01201, 0.0 },
    { 106.21129, 46.93066, 0.0 },
    { 63.68103, 27.69037, 0.0 },
    { 39.02579, 25.66588, 0.0 },
    { 68.09259, 199.75126, 0.0 },
    { 49.33922, 184.93931, 0.0 },
    { 109.41784, 129.55180, 0.0 },
    { 200.92542, 20.52690, 0.0 },
    { 103.22520, 57.90443, 0.0 },
    { 76.80291, 156.51090, 0.0 },
    { 170.93426, 133.77648, 0.0 },
    { 149.54594, 5.60610, 0.0 },
    { 6.08451, 185.60837, 0.0 },
  };

  int modes[3] = {
    VTK_IMAGE_BORDER_CLAMP,
    VTK_IMAGE_BORDER_REPEAT,
    VTK_IMAGE_BORDER_MIRROR
  };

  int m = VTK_IMAGE_BSPLINE_DEGREE_MAX;

  for (int j = 0; j <= m; j++)
    {
    for (int jj = 0; jj < 3; jj++)
      {
      int mode = modes[jj];

      coeffs->SetSplineDegree(j);
      coeffs->SetBorderMode(mode);
      coeffs->Update();

      interp->SetSplineDegree(j);
      interp->SetBorderMode(mode);
      interp->SetTolerance(256);
      interp->Initialize(coeffs->GetOutput());

      for (int k = 0; k < 20; k++)
        {
        double x = points[k][0];
        double y = points[k][1];
        double z = points[k][2];

        // directly use the Thevenaz interpolation code
        double v0 = coeffs->Evaluate(x, y, z);

        // fiddle with points to test border modes
        if (mode == VTK_IMAGE_BORDER_REPEAT)
          {
          x += 256*spacing[0];
          y -= 256*spacing[1];
          }
        else if (mode == VTK_IMAGE_BORDER_MIRROR)
          {
          x = origin[0] - x;
          y = 2*(origin[1] + 255*spacing[1]) - y;
          }
        // use interpolator
        double v1 = interp->Interpolate(x, y, z, 0);

        double tol = 1e-6;
        double e = (v0 - v1)/(range[1] - range[0]);
        if (fabs(e) > tol)
          {
          cerr << "Bad interpolation, error is " << e << " k = " << k
               << " degree = " << j << " mode = " << mode << "\n";
          cerr << v0 << " " << v1 << "\n";
          retVal = EXIT_FAILURE;
          }
        }
      }

    interp->ReleaseData();
    }

  for (int i = 0; i < 2; i++)
    {
    vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.0,0.0,0.0);
    renderer->SetViewport(0.5*(i&1), 0.0,
                          0.5 + 0.5*(i&1), 1.0);
    renWin->AddRenderer(renderer);

    vtkSmartPointer<vtkImageSliceMapper> imageMapper =
      vtkSmartPointer<vtkImageSliceMapper>::New();
    if (i == 0)
      {
      imageMapper->SetInputConnection(reader->GetOutputPort());
      }
    else
      {
      coeffs->SetSplineDegree(3);
      imageMapper->SetInputConnection(coeffs->GetOutputPort());
      }

    double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);

    vtkSmartPointer<vtkImageSlice> image =
      vtkSmartPointer<vtkImageSlice>::New();
    image->SetMapper(imageMapper);
    renderer->AddViewProp(image);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));

    }

  renWin->SetSize(512,256);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return retVal;
}
