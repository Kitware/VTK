/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLHardwareSupport.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLHardwareSupport
 * @brief   OpenGL rendering window
 *
 * vtkOpenGLHardwareSupport is an implementation of methods used
 * to query OpenGL and the hardware of what kind of graphics support
 * is available. When VTK supports more than one Graphics API an
 * abstract super class vtkHardwareSupport should be implemented
 * for this class to derive from.
*/

#ifndef vtkOpenGLHardwareSupport_h
#define vtkOpenGLHardwareSupport_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

class vtkOpenGLExtensionManager;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLHardwareSupport :
  public vtkObject //: public vtkHardwareSupport
{
public:
  vtkTypeMacro(vtkOpenGLHardwareSupport, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkOpenGLHardwareSupport *New();

  /**
   * Return the number of fixed-function texture units.
   */
  int GetNumberOfFixedTextureUnits();

  /**
   * Return the total number of texture image units accessible by a shader
   * program.
   */
  int GetNumberOfTextureUnits();

  /**
   * Test if MultiTexturing is supported.
   */
  bool GetSupportsMultiTexturing();

  //@{
  /**
   * Set/Get a reference to a vtkRenderWindow which is Required
   * for most methods of this class to work.
   */
  vtkGetObjectMacro(ExtensionManager, vtkOpenGLExtensionManager);
  void SetExtensionManager(vtkOpenGLExtensionManager* extensionManager);
  //@}

protected:
  vtkOpenGLHardwareSupport();
  ~vtkOpenGLHardwareSupport() VTK_OVERRIDE;

private:
  vtkOpenGLHardwareSupport(const vtkOpenGLHardwareSupport&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLHardwareSupport&) VTK_DELETE_FUNCTION;

  bool ExtensionManagerSet();

  vtkOpenGLExtensionManager* ExtensionManager;
};

#endif
