// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor2D.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageMapper.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShortArray.h"
#include "vtkStringScanner.h"
#include "vtkType.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>

// Run the test with args <arrayType> <numComponents> <colorLevel> <colorWindow>
// where arrayType is one of: double, short, uchar
// numComponents is 1,2,3,4
// colorLevel and colorWindow are doubles in the range of [0.0,255.0]

namespace TestImageMapperNS
{
const char* VALID_ARRAY_TYPES[] = { "double", "short", "uchar" };

template <typename T, typename ValueType = typename T::ValueType>
vtkSmartPointer<T> CreateArray(
  int numComponents, int numTuples, ValueType minValue, ValueType maxValue)
{
  vtkSmartPointer<T> array = vtkSmartPointer<T>::New();
  array->SetNumberOfComponents(numComponents);
  array->SetNumberOfTuples(numTuples);
  ValueType range = maxValue - minValue;
  for (int i = 0; i < numTuples; ++i)
  {
    int x = i % static_cast<int>(std::sqrt(numTuples));
    int y = i / static_cast<int>(std::sqrt(numTuples));
    // Create a color gradient with some pattern
    ValueType r = minValue + static_cast<ValueType>(range * x / (std::sqrt(numTuples) - 1));
    ValueType g = minValue + static_cast<ValueType>(range * y / (std::sqrt(numTuples) - 1));
    ValueType b = minValue + static_cast<ValueType>(range * (x ^ y) / (std::sqrt(numTuples) - 1));
    if (numComponents >= 3)
    {
      array->SetComponent(i, 0, r);
      array->SetComponent(i, 1, g);
      array->SetComponent(i, 2, b);
    }
    else if (numComponents == 2)
    {
      array->SetComponent(i, 0, r);
      array->SetComponent(i, 1, g);
    }
    else
    {
      array->SetComponent(i, 0, r);
    }
  }
  if (numComponents > 3)
  {
    array->FillComponent(3, 255);
  }
  return array;
}

vtkSmartPointer<vtkImageData> CreateImageData(vtkDataArray* scalars, int width, int height)
{
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(width, height, 1);
  image->AllocateScalars(scalars->GetDataType(), scalars->GetNumberOfComponents());
  image->GetPointData()->SetScalars(scalars);
  return image;
}
} // namespace TestImageMapperNS

int TestImageMapper(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(480, 500);
  iren->SetRenderWindow(renWin);

  int numComponents = 1;
  for (int i = 0; i < argc; ++i)
  {
    if (strcmp(argv[i], "--components") == 0 && i + 1 < argc)
    {
      VTK_FROM_CHARS_IF_ERROR_RETURN(argv[i + 1], numComponents, EXIT_FAILURE);
      break;
    }
  }

  const int width = 280;
  const int height = 320;
  const int numTuples = width * height;

  auto doubleScalars =
    TestImageMapperNS::CreateArray<vtkDoubleArray>(numComponents, numTuples, 0.0, 255.0);
  auto ucharScalars =
    TestImageMapperNS::CreateArray<vtkUnsignedCharArray>(numComponents, numTuples, 0u, 255u);
  auto shortScalars =
    TestImageMapperNS::CreateArray<vtkShortArray>(numComponents, numTuples, 0, 255);

  struct ViewportData
  {
    bool EnableColorShiftScale;
    int VTKType;
    std::array<double, 4> Viewport;
    vtkSmartPointer<vtkDataArray> Scalars;
  };
  // shift-scale: double, uchar, short
  // no-shift-scale: double, uchar, short
  ViewportData viewports[] = {
    { true, VTK_DOUBLE, { 0.0, 0.0, 0.3, 0.5 }, doubleScalars },
    { true, VTK_UNSIGNED_CHAR, { 0.3, 0.0, 0.6, 0.5 }, ucharScalars },
    { true, VTK_UNSIGNED_SHORT, { 0.6, 0.0, 1.0, 0.5 }, shortScalars },
    { false, VTK_DOUBLE, { 0.0, 0.5, 0.3, 1.0 }, doubleScalars },
    { false, VTK_UNSIGNED_CHAR, { 0.3, 0.5, 0.6, 1.0 }, ucharScalars },
    { false, VTK_UNSIGNED_SHORT, { 0.6, 0.5, 1.0, 1.0 }, shortScalars },
  };
  for (const auto& vp : viewports)
  {
    auto image = TestImageMapperNS::CreateImageData(vp.Scalars, width, height);
    vtkNew<vtkImageMapper> mapper;
    mapper->SetInputData(image);
    if (vp.EnableColorShiftScale)
    {
      mapper->SetColorLevel(127.5);
      mapper->SetColorWindow(255.0);
    }
    else
    {
      mapper->SetColorLevel(63.75);
      mapper->SetColorWindow(127.5);
    }
    vtkNew<vtkActor2D> actor;
    actor->SetMapper(mapper);
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(vp.Viewport.data());
    renderer->AddActor(actor);
    renWin->AddRenderer(renderer);
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return retVal == vtkRegressionTester::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
}
