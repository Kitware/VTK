/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLActor
 * @brief   OpenGL actor
 *
 * vtkOpenGLActor is a concrete implementation of the abstract class vtkActor.
 * vtkOpenGLActor interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLActor_h
#define vtkOpenGLActor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkActor.h"

class vtkInformationIntegerKey;
class vtkOpenGLRenderer;
class vtkMatrix4x4;
class vtkMatrix3x3;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLActor : public vtkActor
{
public:
  static vtkOpenGLActor *New();
  vtkTypeMacro(vtkOpenGLActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Actual actor render method.
   */
  void Render(vtkRenderer *ren, vtkMapper *mapper);

  void GetKeyMatrices(vtkMatrix4x4 *&WCVCMatrix, vtkMatrix3x3 *&normalMatrix);

  /**
   * If this key is set in GetPropertyKeys(), the glDepthMask will be adjusted
   * prior to rendering translucent objects. This is useful for e.g. depth
   * peeling.

   * If GetIsOpaque() == true, the depth mask is always enabled, regardless of
   * this key. Otherwise, the depth mask is disabled for default alpha blending
   * unless this key is set.

   * If this key is set, the integer value has the following meanings:
   * 0: glDepthMask(GL_FALSE)
   * 1: glDepthMask(GL_TRUE)
   * Anything else: No change to depth mask.
   */
  static vtkInformationIntegerKey* GLDepthMaskOverride();

protected:
  vtkOpenGLActor();
  ~vtkOpenGLActor();

  vtkMatrix4x4 *MCWCMatrix;
  vtkMatrix3x3 *NormalMatrix;
  vtkTransform *NormalTransform;
  vtkTimeStamp KeyMatrixTime;

private:
  vtkOpenGLActor(const vtkOpenGLActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLActor&) VTK_DELETE_FUNCTION;
};

#endif
