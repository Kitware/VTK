// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

namespace
{

inline vtkSmartPointer<vtkTable> CreateDummyData()
{
  vtkNew<vtkTable> table;

  vtkNew<vtkDoubleArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  vtkNew<vtkDoubleArray> arrC;
  arrC->SetName("cosine");
  table->AddColumn(arrC);
  vtkNew<vtkDoubleArray> arrS;
  arrS->SetName("sine");
  table->AddColumn(arrS);
  vtkNew<vtkDoubleArray> arrS2;
  arrS2->SetName("tangent");
  table->AddColumn(arrS2);

  int numPoints = 200;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, tan(i * inc) + 0.5);
  }

  return table;
}

}
