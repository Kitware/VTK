/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPainterDeviceAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMesaPainterDeviceAdapter - An adapter between a vtkPainter and a 
// rendering device.
// .SECTION Description
// An adapter between vtkPainter and the Mesa rendering system.  Only a
// handful of attributes with special meaning are supported.  The Mesa
// attribute used for each attribute is given below.
//
// \verbatim
// vtkDataSetAttributes::NORMALS          glNormal
// vtkDataSetAttributes:::SCALARS         glColor
// vtkDataSetAttributes::TCOORDS          glTexCoord
// vtkDataSetAttributes::NUM_ATTRIBUTES   glVertex
// \endverbatim
//

#ifndef __vtkMesaPainterDeviceAdapter_h
#define __vtkMesaPainterDeviceAdapter_h

#include "vtkPainterDeviceAdapter.h"

class VTK_RENDERING_EXPORT vtkMesaPainterDeviceAdapter : 
  public vtkPainterDeviceAdapter
{
public:
  vtkTypeMacro(vtkMesaPainterDeviceAdapter, vtkPainterDeviceAdapter);
  static vtkMesaPainterDeviceAdapter *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Converts mode from VTK_* to GL_* and calls glBegin.
  virtual void BeginPrimitive(int mode);

  // Description:
  // Calls glEnd.
  virtual void EndPrimitive();

  // Description:
  // Calls one of glVertex*, glNormal*, glColor*, or glTexCoord*.
  virtual void SendAttribute(int index, int components, int type,
                             const void *attribute, unsigned long offset=0);

  // Description:
  // Calls one of glVertexPointer, glNormalPointer, glColorPointer, or
  // glTexCoordPointer.
  virtual void SetAttributePointer(int index, int numcomponents, int type,
                                   int stride, const void *pointer);

  // Description:
  // Calls glEnableClientState or glDisableClientState.
  virtual void EnableAttributeArray(int index);
  virtual void DisableAttributeArray(int index);

  // Description:
  // Calls glDrawArrays.  Mode is converted from VTK_* to GL_*.
  virtual void DrawArrays(int mode, vtkIdType first, vtkIdType count);

  // Description:
  // Calls glDrawElements.  Mode and type are converted from VTK_* to GL_*.
  virtual void DrawElements(int mode, vtkIdType count, int type, void *indices);

  // Description:
  // Returns true if renderer is a vtkMesaRenderer.
  virtual int Compatible(vtkRenderer *renderer);

  // Description:
  // Turns lighting on and off.
  virtual void MakeLighting(int mode);

  // Description:
  // Returns current lighting setting.
  virtual int QueryLighting();
  
  // Description:
  // Turns antialiasing on and off.
  virtual void MakeMultisampling(int mode);

  // Description:
  // Returns current antialiasing setting.
  virtual int QueryMultisampling();

  // Description:
  // Turns blending on and off.
  virtual void MakeBlending(int mode);
 
  // Description:
  // Returns current blending setting.
  virtual int QueryBlending();

protected:
  vtkMesaPainterDeviceAdapter();
  ~vtkMesaPainterDeviceAdapter();

private:
  vtkMesaPainterDeviceAdapter(const vtkMesaPainterDeviceAdapter &);  // Not implemented.
  void operator=(const vtkMesaPainterDeviceAdapter &);  // Not implemented.
};

#endif //_vtkMesaPainterDeviceAdapter_h
