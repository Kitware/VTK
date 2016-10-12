/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRendererDelegate
 * @brief   Render the props of a vtkRenderer
 *
 * vtkRendererDelegate is an abstract class with a pure virtual method Render.
 * This method replaces the Render method of vtkRenderer to allow custom
 * rendering from an external project. A RendererDelegate is connected to
 * a vtkRenderer with method SetDelegate(). An external project just
 * has to provide a concrete implementation of vtkRendererDelegate.
 *
 * @sa
 * vtkRenderer
*/

#ifndef vtkRendererDelegate_h
#define vtkRendererDelegate_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkRendererDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkRendererDelegate,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Render the props of vtkRenderer if Used is on.
   */
  virtual void Render(vtkRenderer *r)=0;

  //@{
  /**
   * Tells if the delegate has to be used by the renderer or not.
   * Initial value is off.
   */
  vtkSetMacro(Used, bool);
  vtkGetMacro(Used, bool);
  vtkBooleanMacro(Used, bool);
  //@}

protected:
  vtkRendererDelegate();
  virtual ~vtkRendererDelegate();

  bool Used;

private:
  vtkRendererDelegate(const vtkRendererDelegate&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRendererDelegate&) VTK_DELETE_FUNCTION;
};

#endif
