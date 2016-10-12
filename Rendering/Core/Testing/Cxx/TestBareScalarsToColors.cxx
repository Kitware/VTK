/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBareScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPointData.h"
#include "vtkImageMapper.h"
#include "vtkActor2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include <math.h>

int TestBareScalarsToColors(int argc, char *argv[])
{
  // Cases to check:
  // 1, 2, 3, 4 components to 1, 2, 3, 4, components
  // with scaling and without scaling
  // with alpha and without alpha
  // so 64 tests in total
  // on an 8x8 grid

  // Make the four sets of test scalars
  vtkSmartPointer<vtkUnsignedCharArray> inputs[4];
  for (int ncomp = 1; ncomp <= 4; ncomp++)
  {
    inputs[ncomp-1] = vtkSmartPointer<vtkUnsignedCharArray>::New();
    vtkUnsignedCharArray *arr = inputs[ncomp-1].GetPointer();

    arr->SetNumberOfComponents(ncomp);
    arr->SetNumberOfTuples(6400);

    // luminance conversion factors
    static float a = 0.30;
    static float b = 0.59;
    static float c = 0.11;
    static float d = 0.50;
    static int f = 85;

    unsigned char cval[4];
    vtkIdType i = 0;
    for (int j = 0; j < 16; j++)
    {
      for (int jj = 0; jj < 5; jj++)
      {
        for (int k = 0; k < 16; k++)
        {
          cval[0] = ((k >> 2) & 3)*f;
          cval[1] = (k & 3)*f;
          cval[2] = ((j >> 2) & 3)*f;
          cval[3] = (j & 3)*f;
          float l = cval[0]*a + cval[1]*b + cval[2]*c + d;
          unsigned char lc = static_cast<unsigned char>(l);
          cval[0] = (ncomp > 2 ? cval[0] : lc);
          cval[1] = (ncomp > 2 ? cval[1] : cval[3]);
          for (int kk = 0; kk < 5; kk++)
          {
            arr->SetTypedTuple(i++, cval);
          }
        }
      }
    }
  }

  vtkNew<vtkScalarsToColors> table2;
  vtkNew<vtkScalarsToColors> table;
  table->DeepCopy(table2.GetPointer()); // just for coverage

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->SetSize(640, 640);

  // Make the 64 sets of output scalars
  vtkSmartPointer<vtkUnsignedCharArray> outputs[64];
  for (int i = 0; i < 64; i++)
  {
    int j = (i & 7);
    int k = ((i >> 3) & 7);
    double alpha = 0.5*(2 - (j & 1));
    double range[2];
    range[0] = 63.75*(k & 1);
    range[1] = 255.0 - 63.75*(k & 1);
    int inputc = ((j >> 1) & 3) + 1;
    int outputc = ((k >> 1) & 3) + 1;

    table->SetRange(range);
    table->SetAlpha(alpha);

    if (inputc == 1 || inputc == 3)
    {
      table->SetVectorModeToMagnitude();
    }
    else if (inputc == 4)
    {
      table->SetVectorModeToRGBColors();
    }
    else
    {
      table->SetVectorModeToComponent();
    }

    // coverage
    unsigned char *color = table->MapValue(0.5*(range[0] + range[1]));
    if (color[0] != 128)
    {
      cout << "Expected greyscale 128: ";
      cout << color[0] << ", " << color[1] << ", " << color[2] << ", "
           << color[3] << std::endl;
    }

    outputs[i] = vtkSmartPointer<vtkUnsignedCharArray>::New();
    outputs[i]->SetNumberOfComponents(outputc);
    outputs[i]->SetNumberOfTuples(0);

    // test mapping with a count of zero
    vtkUnsignedCharArray *tmparray =
      table2->MapScalars(outputs[i], VTK_COLOR_MODE_DEFAULT, outputc);
    tmparray->Delete();

    table->MapVectorsThroughTable(
      inputs[inputc-1]->GetPointer(0),
      outputs[i]->WritePointer(0, 6400),
      VTK_UNSIGNED_CHAR, 0, inputc, outputc);

    // now the real thing
    outputs[i]->SetNumberOfTuples(6400);

    table->MapVectorsThroughTable(
      inputs[inputc-1]->GetPointer(0),
      outputs[i]->WritePointer(0, 6400),
      VTK_UNSIGNED_CHAR, 6400, inputc, outputc);

    vtkNew<vtkImageData> image;
    image->SetDimensions(80, 80, 1);
    vtkUnsignedCharArray *colors =
      table2->MapScalars(outputs[i], VTK_COLOR_MODE_DEFAULT, outputc);
    image->GetPointData()->SetScalars(colors);
    colors->Delete();

    int pos[2];
    pos[0] = j*80;
    pos[1] = k*80;

    vtkNew<vtkImageMapper> mapper;
    mapper->SetColorWindow(255.0);
    mapper->SetColorLevel(127.5);
    mapper->SetInputData(image.GetPointer());

    vtkNew<vtkActor2D> actor;
    actor->SetMapper(mapper.GetPointer());

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor.GetPointer());
    ren->SetViewport(pos[0]/640.0, pos[1]/640.0,
                    (pos[0] + 80)/640.0, (pos[1] + 80)/640.0);

    renWin->AddRenderer(ren.GetPointer());
  }

  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
