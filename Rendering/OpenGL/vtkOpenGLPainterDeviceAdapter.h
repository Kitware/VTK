/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPainterDeviceAdapter.h

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
 * @class   vtkOpenGLPainterDeviceAdapter
 * @brief   An adapter between a vtkPainter and a rendering device.
 *
 *
 *
 * An adapter between vtkPainter and the OpenGL rendering system.  Only a
 * handful of attributes with special meaning are supported.  The OpenGL
 * attribute used for each attribute is given below.
 *
 * \verbatim
 * vtkDataSetAttributes::NORMALS          glNormal
 * vtkDataSetAttributes:::SCALARS         glColor
 * vtkDataSetAttributes::TCOORDS          glTexCoord
 * vtkDataSetAttributes::NUM_ATTRIBUTES   glVertex
 * \endverbatim
 *
*/

#ifndef vtkOpenGLPainterDeviceAdapter_h
#define vtkOpenGLPainterDeviceAdapter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPainterDeviceAdapter.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLPainterDeviceAdapter :
  public vtkPainterDeviceAdapter
{
public:
  vtkTypeMacro(vtkOpenGLPainterDeviceAdapter, vtkPainterDeviceAdapter);
  static vtkOpenGLPainterDeviceAdapter *New();
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Converts mode from VTK_* to GL_* and calls glBegin.
   */
  void BeginPrimitive(int mode) override;

  /**
   * Calls glEnd.
   */
  void EndPrimitive() override;

  /**
   * Returns if the given attribute type is supported by the device.
   * Returns 1 is supported, 0 otherwise.
   */
  int IsAttributesSupported(int attribute) override;

  /**
   * Calls one of glVertex*, glNormal*, glColor*, or glTexCoord*.
   */
  void SendAttribute(int index, int components, int type,
                             const void *attribute, vtkIdType offset=0) override;

  /**
   * Calls glMultiTex
   */
  void SendMultiTextureCoords(int numcomp, int type, const void *attribute,
                                      int idx, vtkIdType offset) override;

  /**
   * Calls one of glVertexPointer, glNormalPointer, glColorPointer, or
   * glTexCoordPointer.
   */
  void SetAttributePointer(int index, int numcomponents, int type,
                                   int stride, const void *pointer) override;

  //@{
  /**
   * Calls glEnableClientState or glDisableClientState.
   */
  void EnableAttributeArray(int index) override;
  void DisableAttributeArray(int index) override;
  //@}

  /**
   * Calls glDrawArrays.  Mode is converted from VTK_* to GL_*.
   */
  void DrawArrays(int mode, vtkIdType first, vtkIdType count) override;

  /**
   * Calls glDrawElements.  Mode and type are converted from VTK_* to GL_*.
   */
  void DrawElements(int mode, vtkIdType count, int type, void *indices) override;

  /**
   * Returns true if renderer is a vtkOpenGLRenderer.
   */
  int Compatible(vtkRenderer *renderer) override;

  /**
   * Turns emphasis of vertices on or off for vertex selection.
   * When emphasized verts are drawn nearer to the camera and are drawn
   * larger than normal to make selection of them more reliable.
   */
  void MakeVertexEmphasis(bool mode) override;

  //@{
  /**
   * Control use of the stencil buffer (for vertex selection).
   */
  void Stencil(int on) override;
  void WriteStencil(vtkIdType value) override;
  void TestStencil(vtkIdType value) override;
  //@}

protected:
  vtkOpenGLPainterDeviceAdapter();
  ~vtkOpenGLPainterDeviceAdapter() override;

  double PointSize;
  double RangeNear;
  double RangeFar;
  int MaxStencil;
  bool Initialized;
private:
  vtkOpenGLPainterDeviceAdapter(const vtkOpenGLPainterDeviceAdapter &) = delete;
  void operator=(const vtkOpenGLPainterDeviceAdapter &) = delete;
};

#endif
