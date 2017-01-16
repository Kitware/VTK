/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLCamera
 * @brief   OpenGL camera
 *
 * vtkOpenGLCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkOpenGLCamera interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLCamera_h
#define vtkOpenGLCamera_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkCamera.h"

class vtkOpenGLRenderer;
class vtkMatrix3x3;
class vtkMatrix4x4;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLCamera : public vtkCamera
{
public:
  static vtkOpenGLCamera *New();
  vtkTypeMacro(vtkOpenGLCamera, vtkCamera);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer *ren) VTK_OVERRIDE;

  void UpdateViewport(vtkRenderer *ren) VTK_OVERRIDE;

  virtual void GetKeyMatrices(vtkRenderer *ren, vtkMatrix4x4 *&WCVCMatrix,
    vtkMatrix3x3 *&normalMatrix, vtkMatrix4x4 *&VCDCMatrix, vtkMatrix4x4 *&WCDCMatrix);

protected:
  vtkOpenGLCamera();
  ~vtkOpenGLCamera() VTK_OVERRIDE;

  vtkMatrix4x4 *WCDCMatrix;
  vtkMatrix4x4 *WCVCMatrix;
  vtkMatrix3x3 *NormalMatrix;
  vtkMatrix4x4 *VCDCMatrix;
  vtkTimeStamp KeyMatrixTime;
  vtkRenderer *LastRenderer;

private:
  vtkOpenGLCamera(const vtkOpenGLCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLCamera&) VTK_DELETE_FUNCTION;
};

#endif
