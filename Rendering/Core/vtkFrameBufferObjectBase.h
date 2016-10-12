/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObjectBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFrameBufferObjectBase
 * @brief   abstract interface to OpenGL FBOs
 *
 * API for classes that encapsulate an OpenGL Frame Buffer Object.
*/

#ifndef vtkFrameBufferObjectBase_h
#define vtkFrameBufferObjectBase_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderer;
class vtkProp;
class vtkInformation;

class VTKRENDERINGCORE_EXPORT vtkFrameBufferObjectBase : public vtkObject
{
 public:
  vtkTypeMacro(vtkFrameBufferObjectBase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Dimensions in pixels of the framebuffer.
   */
  virtual int *GetLastSize() = 0;
  virtual void GetLastSize (int &_arg1, int &_arg2) = 0;
  virtual void GetLastSize (int _arg[2]) = 0;
  //@}

protected:
  vtkFrameBufferObjectBase(); // no default constructor.
  ~vtkFrameBufferObjectBase();

private:
  vtkFrameBufferObjectBase(const vtkFrameBufferObjectBase &) VTK_DELETE_FUNCTION;
  void operator=(const vtkFrameBufferObjectBase &) VTK_DELETE_FUNCTION;
};

#endif
