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
 * @class   vtkOpenGLPointGaussianMapper
 * @brief   draw PointGaussians using imposters
 *
 * An OpenGL mapper that uses imposters to draw PointGaussians. Supports
 * transparency and picking as well.
*/

#ifndef vtkOpenGLPointGaussianMapper_h
#define vtkOpenGLPointGaussianMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkPointGaussianMapper.h"

class vtkOpenGLPointGaussianMapperHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLPointGaussianMapper : public vtkPointGaussianMapper
{
public:
  static vtkOpenGLPointGaussianMapper* New();
  vtkTypeMacro(vtkOpenGLPointGaussianMapper, vtkPointGaussianMapper)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * Is this mapper opqaue? currently always false.
   */
  bool GetIsOpaque() VTK_OVERRIDE;

protected:
  vtkOpenGLPointGaussianMapper();
  ~vtkOpenGLPointGaussianMapper() VTK_OVERRIDE;

  void RenderPiece(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  vtkOpenGLPointGaussianMapperHelper *Helper;
  vtkTimeStamp HelperUpdateTime;

private:
  vtkOpenGLPointGaussianMapper(const vtkOpenGLPointGaussianMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLPointGaussianMapper&) VTK_DELETE_FUNCTION;
};

#endif
