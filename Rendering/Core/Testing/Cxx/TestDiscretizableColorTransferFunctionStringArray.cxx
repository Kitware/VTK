/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiscretizableColorTransferFunctionStringArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>
#include <vtkStringArray.h>
#include <vtkUnsignedCharArray.h>


int TestDiscretizableColorTransferFunctionStringArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkStringArray> sArray = vtkSmartPointer<vtkStringArray>::New();
  const int numStrings = 6;
  sArray->SetNumberOfValues(numStrings);
  sArray->SetName("TestArray");
  vtkVariant category1("Category1");
  vtkVariant category2("Category2");
  vtkVariant category3("Category3");
  sArray->SetValue(0, category1.ToString());
  sArray->SetValue(1, category2.ToString());
  sArray->SetValue(2, category3.ToString());
  sArray->SetValue(3, category2.ToString());
  sArray->SetValue(4, category3.ToString());
  sArray->SetValue(5, category1.ToString());

  for (int i = 0; i < sArray->GetNumberOfValues(); ++i)
  {
    std::cout << sArray->GetValue(i) << "\n";
  }

  vtkSmartPointer<vtkDiscretizableColorTransferFunction> tfer =
    vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
  tfer->IndexedLookupOn();

  tfer->SetNumberOfIndexedColors(3);
  tfer->SetIndexedColor(0, 0.0, 0.0, 0.0);
  tfer->SetIndexedColor(1, 1.0, 0.0, 0.0);
  tfer->SetIndexedColor(2, 1.0, 0.0, 1.0);
  tfer->SetAnnotation(category1, "Annotation1");
  tfer->SetAnnotation(category2, "Annotation2");
  tfer->SetAnnotation(category3, "Annotation3");

  vtkUnsignedCharArray* colors = tfer->MapScalars(sArray, VTK_RGBA, -1);

  unsigned char expectedColors[numStrings][4] = {
    {0, 0, 0, 255},
    {255, 0, 0, 255},
    {255, 0, 255, 255},
    {255, 0, 0, 255},
    {255, 0, 255, 255},
    {0, 0, 0, 255}};

  for (int i = 0; i < sArray->GetNumberOfValues(); ++i)
  {
    unsigned char color[4];
    colors->GetTypedTuple(i, color);
    if (expectedColors[i][0] != color[0] ||
        expectedColors[i][1] != color[1] ||
        expectedColors[i][2] != color[2] ||
        expectedColors[i][3] != color[3])
    {
      std::cerr << "Color for string " << i << " ("
                << static_cast<int>(color[0]) << ", "
                << static_cast<int>(color[1]) << ", "
                << static_cast<int>(color[2]) << ", "
                << static_cast<int>(color[3])
                << ") does not match expected color ("
                << static_cast<int>(expectedColors[i][0]) << ", "
                << static_cast<int>(expectedColors[i][1]) << ", "
                << static_cast<int>(expectedColors[i][2]) << ", "
                << static_cast<int>(expectedColors[i][3]) << std::endl;
      return EXIT_FAILURE;
    }
  }

  colors->Delete();

  return EXIT_SUCCESS;
}
