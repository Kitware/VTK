/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLAvatar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLAvatar
 * @brief   OpenGL Avatar
 *
 * vtkOpenGLAvatar is a concrete implementation of the abstract class vtkAvatar.
 * vtkOpenGLAvatar interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLAvatar_h
#define vtkOpenGLAvatar_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkAvatar.h"
#include "vtkNew.h" // for ivars

class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLAvatar : public vtkAvatar
{
public:
  static vtkOpenGLAvatar *New();
  vtkTypeMacro(vtkOpenGLAvatar, vtkAvatar);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual Avatar render method.
   */
  void Render(vtkRenderer *ren, vtkMapper *mapper) override;

protected:
  vtkOpenGLAvatar();
  ~vtkOpenGLAvatar() override;

  vtkNew<vtkOpenGLPolyDataMapper> HeadMapper;
  vtkNew<vtkOpenGLActor> HeadActor;
  vtkNew<vtkOpenGLPolyDataMapper> LeftHandMapper;
  vtkNew<vtkOpenGLActor> LeftHandActor;
  vtkNew<vtkOpenGLPolyDataMapper> RightHandMapper;
  vtkNew<vtkOpenGLActor> RightHandActor;

private:
  vtkOpenGLAvatar(const vtkOpenGLAvatar&) = delete;
  void operator=(const vtkOpenGLAvatar&) = delete;
};

#endif
