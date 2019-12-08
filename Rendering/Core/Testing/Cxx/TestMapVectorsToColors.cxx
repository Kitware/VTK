/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMapVectorsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor2D.h"
#include "vtkImageData.h"
#include "vtkImageMapper.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

#include <cmath>

int TestMapVectorsToColors(int argc, char* argv[])
{
  // Cases to check:
  // 1 component and 3 component inputs
  // vector sizes 1, 2, 3 and default
  // vector components 1, 2, 3 plus default
  // magnitude mapping and component mapping
  // so 64 tests in total
  // on an 8x8 grid

  // Make the four sets of test scalars
  vtkSmartPointer<vtkUnsignedCharArray> inputs[4];
  for (int ncomp = 1; ncomp <= 4; ncomp++)
  {
    inputs[ncomp - 1] = vtkSmartPointer<vtkUnsignedCharArray>::New();
    vtkUnsignedCharArray* arr = inputs[ncomp - 1];

    arr->SetNumberOfComponents(ncomp);
    arr->SetNumberOfTuples(6400);

    unsigned char cval[4];
    vtkIdType i = 0;
    for (int j = 0; j < 16; j++)
    {
      for (int jj = 0; jj < 5; jj++)
      {
        for (int k = 0; k < 16; k++)
        {
          static const int f = 85;
          cval[0] = ((k >> 2) & 3) * f;
          cval[1] = (k & 3) * f;
          cval[2] = ((j >> 2) & 3) * f;
          cval[3] = (j & 3) * f;
          for (int kk = 0; kk < 5; kk++)
          {
            arr->SetTypedTuple(i++, cval);
          }
        }
      }
    }
  }

  vtkNew<vtkLookupTable> table;
  table->Build();

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->SetSize(640, 640);

  // Make the 64 sets of output scalars
  vtkSmartPointer<vtkUnsignedCharArray> outputs[64];
  for (int i = 0; i < 64; i++)
  {
    int j = (i & 7);
    int k = ((i >> 3) & 7);
    int inputc = 3 - 2 * (j & 1);
    bool useMagnitude = ((k & 1) == 1);
    int vectorComponent = ((j >> 1) & 3) - 1;
    int vectorSize = ((k >> 1) & 3);
    vectorSize -= (vectorSize == 0);

    table->SetRange(0, 255);

    if (useMagnitude)
    {
      table->SetVectorModeToMagnitude();
    }
    else
    {
      table->SetVectorModeToComponent();
    }

    outputs[i] = vtkSmartPointer<vtkUnsignedCharArray>::New();
    outputs[i]->SetNumberOfComponents(4);
    outputs[i]->SetNumberOfTuples(0);

    // test mapping with a count of zero
    vtkUnsignedCharArray* tmparray =
      table->MapScalars(outputs[i], VTK_COLOR_MODE_DEFAULT, VTK_RGBA);
    tmparray->Delete();

    table->MapVectorsThroughTable(inputs[inputc - 1]->GetPointer(0),
      outputs[i]->WritePointer(0, 6400), VTK_UNSIGNED_CHAR, 0, inputc, VTK_RGBA, vectorComponent,
      vectorSize);

    // now the real thing
    outputs[i]->SetNumberOfTuples(6400);

    table->MapVectorsThroughTable(inputs[inputc - 1]->GetPointer(0),
      outputs[i]->WritePointer(0, 6400), VTK_UNSIGNED_CHAR, 6400, inputc, VTK_RGBA, vectorComponent,
      vectorSize);

    vtkNew<vtkImageData> image;
    image->SetDimensions(80, 80, 1);
    image->GetPointData()->SetScalars(outputs[i]);

    int pos[2];
    pos[0] = j * 80;
    pos[1] = k * 80;

    vtkNew<vtkImageMapper> mapper;
    mapper->SetColorWindow(255.0);
    mapper->SetColorLevel(127.5);
    mapper->SetInputData(image);

    vtkNew<vtkActor2D> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor);
    ren->SetViewport(pos[0] / 640.0, pos[1] / 640.0, (pos[0] + 80) / 640.0, (pos[1] + 80) / 640.0);

    renWin->AddRenderer(ren);
  }

  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
