/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageAlgorithmHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLImageAlgorithmHelper
 * @brief   Help image algorithms use the GPU
 *
 * Designed to make it easier to accelerate an image algorithm on the GPU
*/

#ifndef vtkOpenGLImageAlgorithmHelper_h
#define vtkOpenGLImageAlgorithmHelper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

#include "vtkOpenGLHelper.h" // used for ivars
#include "vtkSmartPointer.h" // for ivar

class vtkOpenGLRenderWindow;
class vtkRenderWindow;
class vtkImageData;
class vtkDataArray;

class vtkOpenGLImageAlgorithmCallback
{
public:
  virtual void InitializeShaderUniforms(vtkShaderProgram * /* program */) {};
  virtual void UpdateShaderUniforms(
    vtkShaderProgram * /* program */, int /* zExtent */) {};
  virtual ~vtkOpenGLImageAlgorithmCallback() {};
  vtkOpenGLImageAlgorithmCallback() {};
private:
  vtkOpenGLImageAlgorithmCallback(const vtkOpenGLImageAlgorithmCallback&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLImageAlgorithmCallback&) VTK_DELETE_FUNCTION;
};

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLImageAlgorithmHelper : public vtkObject
{
public:
  static vtkOpenGLImageAlgorithmHelper *New();
  vtkTypeMacro(vtkOpenGLImageAlgorithmHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Execute(
    vtkOpenGLImageAlgorithmCallback *cb,
    vtkImageData *inImage, vtkDataArray *inData,
    vtkImageData *outData, int outExt[6],
    const char *vertexCode,
    const char *fragmentCode,
    const char *geometryCode
    );

  /**
   * Set the render window to get the OpenGL resources from
   */
  void SetRenderWindow(vtkRenderWindow *renWin);

 protected:
  vtkOpenGLImageAlgorithmHelper();
  virtual ~vtkOpenGLImageAlgorithmHelper();

  vtkSmartPointer<vtkOpenGLRenderWindow> RenderWindow;
  vtkOpenGLHelper Quad;

 private:
  vtkOpenGLImageAlgorithmHelper(const vtkOpenGLImageAlgorithmHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLImageAlgorithmHelper&) VTK_DELETE_FUNCTION;
};

#endif

// VTK-HeaderTest-Exclude: vtkOpenGLImageAlgorithmHelper.h
