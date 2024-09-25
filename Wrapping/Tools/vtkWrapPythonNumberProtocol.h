// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonNumberProtocol_h
#define vtkWrapPythonNumberProtocol_h

#include "vtkParseData.h"

#include <stdio.h>

/** Overrides __rshift__ operator for vtkAlgorithm and vtkDataObject so that a pipeline
 *  can be built like:
 *  vtkSphereSource() >> vtkElevationFilter()
 *   (or)
 *  vtkImageData() >> vtkElevationFilter()
 */
int vtkWrapPython_GenerateNumberProtocolDefintions(FILE* fp, const ClassInfo* classInfo);

#endif /* vtkWrapPythonNumberProtocol_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonNumberProtocol.h */
