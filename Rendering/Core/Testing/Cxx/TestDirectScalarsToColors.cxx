/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDirectScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor2D.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkImageMapper.h"
#include "vtkLongArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <math.h>

namespace {
template<typename T>
void UCharToColor(unsigned char src, T* dest)
{
  *dest = src;
}

template<> inline
void UCharToColor(unsigned char src, double* dest)
{
  *dest = ((static_cast<double>(src) / 255.0));
}

template<> inline
void UCharToColor(unsigned char src, float* dest)
{
  *dest = (static_cast<float>(src) / 255.0);
}
};


template<typename T, typename BaseT>
void addViews (vtkRenderWindow* renWin, int typeIndex)
{
  vtkNew<vtkScalarsToColors> map;
  // Make the four sets of test scalars
  vtkSmartPointer<T> inputs[4];
  for (int ncomp = 1; ncomp <= 4; ncomp++)
    {
    int posX = ((ncomp - 1) & 1);
    int posY = ((ncomp - 1) >> 1);
    inputs[ncomp-1] = vtkSmartPointer<T>::New();
    T *arr = inputs[ncomp-1].GetPointer();

    arr->SetNumberOfComponents(ncomp);
    arr->SetNumberOfTuples(6400);

    // luminance conversion factors
    static float a = 0.30;
    static float b = 0.59;
    static float c = 0.11;
    static float d = 0.50;
    static int f = 85;

    BaseT cval[4];
    vtkIdType i = 0;
    for (int j = 0; j < 16; j++)
      {
      for (int jj = 0; jj < 5; jj++)
        {
        for (int k = 0; k < 16; k++)
          {
          cval[0] = (((k >> 2) & 3)*f);
          cval[1] = ((k & 3)*f);
          cval[2] = (((j >> 2) & 3)*f);
          cval[3] = ((j & 3)*f);
          float l = cval[0]*a + cval[1]*b + cval[2]*c + d;
          unsigned char lc = static_cast<unsigned char>(l);
          cval[0] = ((ncomp > 2 ? cval[0] : lc));
          cval[1] = ((ncomp > 2 ? cval[1] : cval[3]));
          // store values between 0 and 1 for floating point colors.
          for (int index = 0; index < 4; ++index)
            {
            UCharToColor (cval[index], &cval[index]);
            }
          for (int kk = 0; kk < 5; kk++)
            {
            arr->SetTupleValue(i++, cval);
            }
          }
        }
      }

    vtkNew<vtkImageData> image;
    image->SetDimensions(80, 80, 1);
    vtkUnsignedCharArray *colors =
      map->MapScalars(arr, VTK_COLOR_MODE_DIRECT_SCALARS, -1);
    if (colors == NULL)
    {
    continue;
    }
    image->GetPointData()->SetScalars(colors);
    colors->Delete();

    int pos[2];
    pos[0] = (((typeIndex & 3) << 1) + posX) * 80;
    pos[1] = ((((typeIndex >> 2) & 3) << 1) + posY) * 80;

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
}


// Modified from TestBareScalarsToColors
int TestDirectScalarsToColors(int argc, char *argv[])
{
  // Cases to check:
  // 1, 2, 3, 4 components

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->SetSize(640, 640);

  int i = -1;
  addViews<vtkUnsignedCharArray, unsigned char> (renWin.GetPointer(), ++i);
  // This line generates an expected ERROR message.
  // addViews<vtkCharArray, char>(renWin.GetPointer(), ++i);
  addViews<vtkUnsignedShortArray, unsigned short> (renWin.GetPointer(), ++i);
  addViews<vtkShortArray, short>(renWin.GetPointer(), ++i);
  addViews<vtkUnsignedIntArray, unsigned int> (renWin.GetPointer(), ++i);
  addViews<vtkIntArray, int>(renWin.GetPointer(), ++i);
  addViews<vtkUnsignedLongArray, unsigned long> (renWin.GetPointer(), ++i);
  addViews<vtkLongArray, long>(renWin.GetPointer(), ++i);
  addViews<vtkFloatArray, float> (renWin.GetPointer(), ++i);
  addViews<vtkDoubleArray, double>(renWin.GetPointer(), ++i);
  // Mac-Lion-64-gcc-4.2.1 (kamino) does not clear the render window
  // unless we create renderers for the whole window.
  for (++i; i < 16; ++i)
  {
  int pos[2];
  pos[0] = (i & 3) * 160;
  pos[1] = ((i>>2) & 3) * 160;
  vtkNew<vtkRenderer> ren;
  ren->SetViewport(pos[0]/640.0, pos[1]/640.0,
                   (pos[0] + 160)/640.0, (pos[1] + 160)/640.0);
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
