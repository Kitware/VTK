// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "uses_vtk.h"

int main(int argc, char* argv[])
{
  if (vtkObject_class_name() == "vtkObject")
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
