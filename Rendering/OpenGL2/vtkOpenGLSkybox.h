/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLSkybox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLSkybox
 * @brief   OpenGL Skybox
 *
 * vtkOpenGLSkybox is a concrete implementation of the abstract class vtkSkybox.
 * vtkOpenGLSkybox interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLSkybox_h
#define vtkOpenGLSkybox_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSkybox.h"
#include "vtkNew.h" // for ivars

class vtkOpenGLPolyDataMapper;

class vtkInformationIntegerKey;
class vtkOpenGLRenderer;
class vtkMatrix4x4;
class vtkMatrix3x3;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLSkybox : public vtkSkybox
{
public:
  static vtkOpenGLSkybox *New();
  vtkTypeMacro(vtkOpenGLSkybox, vtkSkybox);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual Skybox render method.
   */
  void Render(vtkRenderer *ren, vtkMapper *mapper) override;

  void GetKeyMatrices(vtkMatrix4x4 *&WCVCMatrix, vtkMatrix3x3 *&normalMatrix);

protected:
  vtkOpenGLSkybox();
  ~vtkOpenGLSkybox() override;

  vtkMatrix4x4 *MCWCMatrix;
  vtkMatrix3x3 *NormalMatrix;
  vtkTransform *NormalTransform;
  vtkTimeStamp KeyMatrixTime;

  vtkNew<vtkOpenGLPolyDataMapper> CubeMapper;

private:
  vtkOpenGLSkybox(const vtkOpenGLSkybox&) = delete;
  void operator=(const vtkOpenGLSkybox&) = delete;
};

#endif
