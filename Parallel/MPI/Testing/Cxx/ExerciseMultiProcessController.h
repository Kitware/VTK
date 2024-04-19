// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkABINamespace.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
VTK_ABI_NAMESPACE_END

// Runs the given multi process controller through the ropes.  Returns
// value is 0 on success (so that it may be passed back from the main
// application.
int ExerciseMultiProcessController(vtkMultiProcessController* controller);
