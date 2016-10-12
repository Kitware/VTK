/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStandardPolyDataPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   vtkStandardPolyDataPainter
 * @brief   A standard implementation of vtkPolyDataPainter.
 *
 *
 * vtkStandardPolyDataPainter is a catch-all painter.  It should work with pretty
 * much any vtkPolyData, and attributes, and vtkPolyDataPainterDeviceAdapter.  On
 * the flip side, the vtkStandardPolyDataPainter will be slower than the more
 * special purpose painters.
 * @sa
 * vtkDefaultPainter
 * @par Thanks:
 * Support for generic vertex attributes in VTK was contributed in
 * collaboration with Stephane Ploix at EDF.
*/

#ifndef vtkStandardPolyDataPainter_h
#define vtkStandardPolyDataPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataPainter.h"

class vtkCellArray;
class vtkDataArray;
class vtkPointData;
class vtkPoints;
class vtkShaderDeviceAdapter2;

class VTKRENDERINGOPENGL_EXPORT vtkStandardPolyDataPainter : public vtkPolyDataPainter
{
public:
  vtkTypeMacro(vtkStandardPolyDataPainter, vtkPolyDataPainter);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkStandardPolyDataPainter *New();

  void AddMultiTextureCoordsArray(vtkDataArray * array);

protected:
  vtkStandardPolyDataPainter();
  ~vtkStandardPolyDataPainter();

  /**
   * Generates rendering primitives of appropriate type(s). Multiple types
   * of preimitives can be requested by or-ring the primitive flags.
   */
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);

  void DrawCells(int mode, vtkCellArray *connectivity,
                 vtkIdType startCellId,
                 vtkShaderDeviceAdapter2 *shaderDevice2,
                 vtkRenderer *renderer,
                 int buildnormals, int interpolation);

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called.
   */
  virtual void ProcessInformation(vtkInformation*);

  void UpdateGenericAttributesCache(vtkShaderDeviceAdapter2 *shaderDevice2);

  vtkIdType TotalCells;
private:
  vtkStandardPolyDataPainter(const vtkStandardPolyDataPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStandardPolyDataPainter&) VTK_DELETE_FUNCTION;

  class vtkInternal;
  vtkInternal* Internal;

};

#endif //vtkStandardPolyDataPainter_h
