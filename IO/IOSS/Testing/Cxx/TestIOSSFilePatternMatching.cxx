// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Test vtkIOSSReader::GetRelatedFiles(...).
 */
#include "vtkIOSSReader.h"
#include "vtkObject.h"

int TestIOSSFilePatternMatching(int, char*[])
{
  return vtkIOSSReader::DoTestFilePatternMatching() ? EXIT_SUCCESS : EXIT_FAILURE;
}
