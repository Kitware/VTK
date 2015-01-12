/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainterDeviceAdapter.h

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

// .NAME vtkPainterDeviceAdapter - An adapter between a vtkPainter and a rendering device.
//
// .SECTION Description
//
// This class is an adapter between a vtkPainter and a rendering device (such
// as an OpenGL machine).  Having an abstract adapter allows vtkPainters
// to be re-used for any rendering system.
//
// Although VTK really only uses OpenGL right now, there are reasons to
// swap out the rendering functions.  Sometimes MESA with mangled names
// is used.  Also, different shader extensions use different functions.
// Furthermore, Cg also has its own interface.
//
// The interface for this class should be familier to anyone experienced
// with OpenGL.
//
// .SECTION See Also
// vtkPainter
//

#ifndef vtkPainterDeviceAdapter_h
#define vtkPainterDeviceAdapter_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkDataArray.h" // needed for inline functions.
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkPainterDeviceAdapter : public vtkObject
{
public:
  static vtkPainterDeviceAdapter* New();
  vtkTypeMacro(vtkPainterDeviceAdapter, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Signals the start of sending a primitive to the graphics card.  The
  // mode is one of VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
  // VTK_TRIANGLE, VTK_TRIANGLE_STRIP, VTK_POLYGON, or VTK_QUAD.  The
  // primitive is defined by the attributes sent between the calls to
  // BeginPrimitive and EndPrimitive.  You do not need to call
  // EndPrimitive/BeginPrimitive between primitives that have a constant
  // number of points (i.e. VTK_VERTEX, VTK_LINE, VTK_TRIANGLE, and
  // VTK_QUAD).
  virtual void BeginPrimitive(int mode) = 0;

  // Description:
  // Signals the end of sending a primitive to the graphics card.
  virtual void EndPrimitive() = 0;

  // Description:
  // Returns if the given attribute type is supported by the device.
  // Returns 1 is supported, 0 otherwise.
  virtual int IsAttributesSupported(int attribute)=0;

  // Description:
  // Calls glMultiTex
  virtual void SendMultiTextureCoords(int numcomp, int type, const void *attribute,
                                      int idx, vtkIdType offset) = 0;

  // Description:
  // Sends a single attribute to the graphics card.  The index parameter
  // identifies the attribute.  Some indices have special meaning (see
  // vtkPainter for details).  The components parameter gives the number of
  // components in the attribute.  In general, components must be between
  // 1-4, but a rendering system may impose even more constraints.  The
  // type parameter is a VTK type enumeration (VTK_FLOAT, VTK_INT, etc.).
  // Again, a rendering system may not support all types for all
  // attributes.  The attribute parameter is the actual data for the
  // attribute.
  // If offset is specified, it is added to attribute pointer after
  // it has been casted to the proper type.
  virtual void SendAttribute(int index, int components, int type,
                             const void *attribute, vtkIdType offset=0) = 0;

  // Description:
  // Sets an array of attributes.  This allows you to send all the data for
  // a particular attribute with one call, thus greatly reducing function
  // call overhead.  Once set, the array is enabled with
  // EnableAttributeArray, and the data is sent with a call to DrawArrays
  // DrawElements.
  void SetAttributePointer(int index, vtkDataArray *attributeArray);

  // Description:
  // Sets an array of attributes.  This allows you to send all the data for
  // a particular attribute with one call, thus greatly reducing function
  // call overhead.  Once set, the array is enabled with
  // EnableAttributeArray, and the data is sent with a call to DrawArrays
  // DrawElements.
  //
  // \arg \c index the index of the attribute.
  // \arg \c numcomponents the number of components in each attribute entry.
  // \arg \c type the data type (VTK_FLOAT, VTK_UNSIGNED_CHAR, etc.).
  // \arg \c stride the byte offset between entries in the array (0 for
  //    tightly packed).
  // \arg \c pointer the array holding the data.
  virtual void SetAttributePointer(int index, int numcomponents, int type,
                                   int stride, const void *pointer) = 0;

  // Description:
  // Enable/disable the attribute array set with SetAttributePointer.
  virtual void EnableAttributeArray(int index) = 0;
  virtual void DisableAttributeArray(int index) = 0;

  // Description:
  // Send a section of the enabled attribute pointers to the graphics card
  // to define a primitive.  The mode is one of VTK_VERTEX,
  // VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE, VTK_TRIANGLE,
  // VTK_TRIANGLE_STRIP, VTK_POLYGON, or VTK_QUAD.  It identifies which
  // type of primitive the attribute data is defining.  The parameters
  // first and count identify what part of the attribute arrays define the
  // given primitive.  If mode is a primitive that has a constant number of
  // points (i.e. VTK_VERTEX, VTK_LINE, VTK_TRIANGLE, and VTK_QUAD), you
  // may draw multiple primitives with one call to DrawArrays.
  virtual void DrawArrays(int mode, vtkIdType first, vtkIdType count) = 0;

  // Description:
  // Send items in the attribute pointers to the graphics card to define a
  // primitive.  The mode is one of VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE,
  // VTK_POLY_LINE, VTK_TRIANGLE, VTK_TRIANGLE_STRIP, VTK_POLYGON, or
  // VTK_QUAD.  It identifies which type of primitive the attribute data is
  // defining.  The indices array holds the list of attribute elements that
  // define the primitive.  The count and type parameters give the number
  // and data type of the indices array.  The type parameter is a VTK type
  // enumeration (VTK_UNSIGNED_INT, ...).  The type should be an integer
  // type (for obvious reasons).  If mode is a primitive that has a
  // constant number of points (i.e. VTK_VERTEX, VTK_LINE, VTK_TRIANGLE,
  // and VTK_QUAD), you may draw multiple primitives with one call to
  // DrawArrays.
  virtual void DrawElements(int mode, vtkIdType count, int type,
                            void *indices) = 0;

  // Description:
  // Returns true if this device adapter is compatible with the given
  // vtkRenderer.
  virtual int Compatible(vtkRenderer *renderer) = 0;

#ifndef VTK_LEGACY_REMOVE
  // Description:
  // @deprecated code that needs access directly to OpenGL state should
  // manage it locally.
  // Turns lighting on and off.
  virtual void MakeLighting(int mode) = 0;

  // Description:
  // @deprecated code that needs access directly to OpenGL state should
  // manage it locally.
  // Returns current lighting setting.
  virtual int QueryLighting() = 0;

  // Description:
  // @deprecated code that needs access directly to OpenGL state should
  // manage it locally.
  // Turns antialiasing on and off.
  virtual void MakeMultisampling(int mode) = 0;

  // Description:
  // @deprecated code that needs access directly to OpenGL state should
  // manage it locally.
  // Returns current antialiasing setting.
  virtual int QueryMultisampling() = 0;

  // Description:
  // @deprecated code that needs access directly to OpenGL state should
  // manage it locally.
  // Turns blending on and off.
  virtual void MakeBlending(int mode) = 0;

  // Description:
  // @deprecated code that needs access directly to OpenGL state should
  // manage it locally.
  // Returns current blending setting.
  virtual int QueryBlending() = 0;
#endif

  // Description:
  // Turns emphasis of vertices on or off for vertex selection.
  virtual void MakeVertexEmphasis(bool mode) = 0;

  // Description:
  // Control use of the stencil buffer (for vertex selection).
  virtual void Stencil(int on) = 0;
  virtual void WriteStencil(vtkIdType value) = 0;
  virtual void TestStencil(vtkIdType value) = 0;

protected:
  vtkPainterDeviceAdapter();
  ~vtkPainterDeviceAdapter();

private:
  vtkPainterDeviceAdapter(const vtkPainterDeviceAdapter &);  // Not implemented.
  void operator=(const vtkPainterDeviceAdapter &);  // Not implemented.
};

inline void vtkPainterDeviceAdapter::SetAttributePointer(int index,
                                                   vtkDataArray *attributeArray)
{
  this->SetAttributePointer(index, attributeArray->GetNumberOfComponents(),
                            attributeArray->GetDataType(), 0,
                            attributeArray->GetVoidPointer(0));
}

#endif //_vtkPainterDeviceAdapter_h
