// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHoverWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkHoverWidgetTest1(int, char*[])
{
  vtkSmartPointer<vtkHoverWidget> node1 = vtkSmartPointer<vtkHoverWidget>::New();

  EXERCISE_BASIC_HOVER_METHODS(node1);

  return EXIT_SUCCESS;
}
