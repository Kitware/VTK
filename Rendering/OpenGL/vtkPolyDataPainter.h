/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPainter.h

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
 * @class   vtkPolyDataPainter
 * @brief   Abstract class for drawing poly data.
 *
 *
 * vtkPolyDataPainter encapsulates a method of drawing poly data.  This is a subset
 * of what a mapper does.  The painter does no maintenance of the rendering
 * state (camera, lights, etc.).  It is solely responsible for issuing
 * rendering commands that build graphics primitives.
 *
 * To simplify coding, an implementation of vtkPolyDataPainter is allowed to support
 * only certain types of poly data or certain types of primitives.
 *
 * @sa
 * vtkDefaultPainter
 * vtkStandardPainter
 * vtkPainterDeviceAdapter
 *
*/

#ifndef vtkPolyDataPainter_h
#define vtkPolyDataPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPainter.h"

class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkPolyData;

class VTKRENDERINGOPENGL_EXPORT vtkPolyDataPainter : public vtkPainter
{
public:
  vtkTypeMacro(vtkPolyDataPainter, vtkPainter);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get/set the poly data to render.
   */
  vtkPolyData* GetInputAsPolyData();

  /**
   * Get the output polydata from this Painter. The default
   * implementation forwards the input polydata as the output.
   */
  vtkPolyData* GetOutputAsPolyData();

  /**
   * Keys used to specify control the behaviour of the painter.
   * When on, the painter creates normals when none are available in the
   * polydata. On by default.
   */
  static vtkInformationIntegerKey* BUILD_NORMALS();

  /**
   * Key added to disable any scalar coloring for the current pass.
   */
  static vtkInformationIntegerKey* DISABLE_SCALAR_COLOR();

  // Set the mapping between vtkPointData (vtkCellData) arrays and
  // generic vertex attributes.
  static vtkInformationObjectBaseKey* DATA_ARRAY_TO_VERTEX_ATTRIBUTE();

  /**
   * Key to store the shader device adaptor.
   */
  static vtkInformationObjectBaseKey* SHADER_DEVICE_ADAPTOR();

  /**
   * Overridden to stop the render call if input polydata is not set,
   * since PolyDataPainter cannot paint without any polydata input.
   */
  void Render(vtkRenderer* renderer, vtkActor* actor,
                      unsigned long typeflags, bool forceCompileOnly) VTK_OVERRIDE;

protected:
  vtkPolyDataPainter();
  ~vtkPolyDataPainter() VTK_OVERRIDE;

  int BuildNormals; // ivar synchornized with this->Information before
    // RenderInternal() is called. The ivar are purposefully protected,
    // the only way to affecting these from outside should be using
    // the information object.
  vtkSetMacro(BuildNormals, int);

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called.
   */
  void ProcessInformation(vtkInformation*) VTK_OVERRIDE;

private:
  vtkPolyDataPainter(const vtkPolyDataPainter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataPainter &) VTK_DELETE_FUNCTION;
};

#endif //_vtkPolyDataPainter_h
