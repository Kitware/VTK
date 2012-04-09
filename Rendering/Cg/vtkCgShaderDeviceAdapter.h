/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShaderDeviceAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCgShaderDeviceAdapter - adapter to pass generic vertex attributes 
// to the rendering pipeline to be used in a Cg shader.
// .SECTION Description
// vtkShaderDeviceAdapter subclass for Cg.
// .SECTION Thanks
// Support for generic vertex attributes in VTK was contributed in
// collaboration with Stephane Ploix at EDF.

#ifndef __vtkCgShaderDeviceAdapter_h
#define __vtkCgShaderDeviceAdapter_h

#include "vtkShaderDeviceAdapter.h"

class VTK_RENDERING_EXPORT vtkCgShaderDeviceAdapter : public vtkShaderDeviceAdapter
{
public:
  static vtkCgShaderDeviceAdapter* New();
  vtkTypeMacro(vtkCgShaderDeviceAdapter, vtkShaderDeviceAdapter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Descrition:
  // This method is called before rendering. This gives the shader device
  // adapter an opportunity to collect information, such as attribute indices
  // that it will need while rendering.
  virtual void PrepareForRender(); 

  // Description:
  // Sends a single attribute to the graphics card.  
  // The attrname parameter identifies the name of attribute.  
  // The components parameter gives the number of
  // components in the attribute.  In general, components must be between
  // 1-4, but a rendering system may impose even more constraints.  The
  // type parameter is a VTK type enumeration (VTK_FLOAT, VTK_INT, etc.).
  // Again, a rendering system may not support all types for all
  // attributes.  The attribute parameter is the actual data for the
  // attribute.
  // If offset is specified, it is added to attribute pointer \c after
  // it has been casted to the proper type.
  virtual void SendAttribute(const char* attrname,
    int components, int type, 
    const void* attribute, unsigned long offset=0);

//BTX
  void SendAttributeInternal(const char* attrname, int components, const double*);
  void SendAttributeInternal(const char* attrname, int components, const float*);
protected:
  vtkCgShaderDeviceAdapter();
  ~vtkCgShaderDeviceAdapter();

private:
  vtkCgShaderDeviceAdapter(const vtkCgShaderDeviceAdapter&); // Not implemented.
  void operator=(const vtkCgShaderDeviceAdapter&); // Not implemented.

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif


