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

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkAvatar.h"
#include "vtkNew.h" // for ivars

class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkOpenGLRenderer;

class VTKRENDERINGOPENVR_EXPORT vtkOpenGLAvatar : public vtkAvatar
{
public:
  static vtkOpenGLAvatar *New();
  vtkTypeMacro(vtkOpenGLAvatar, vtkAvatar);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual Avatar render method.
   */
  void Render(vtkRenderer *ren, vtkMapper *mapper) override;

  double *GetBounds() VTK_SIZEHINT(6) override;

protected:
  vtkOpenGLAvatar();
  ~vtkOpenGLAvatar() override;

  // move the torso and arms based on head/hand inputs.
  void CalcBody();

  vtkNew<vtkOpenGLPolyDataMapper> HeadMapper;
  vtkNew<vtkOpenGLActor> HeadActor;
  vtkNew<vtkOpenGLPolyDataMapper> LeftHandMapper;
  vtkNew<vtkOpenGLActor> LeftHandActor;
  vtkNew<vtkOpenGLPolyDataMapper> RightHandMapper;
  vtkNew<vtkOpenGLActor> RightHandActor;
  vtkNew<vtkOpenGLPolyDataMapper> BodyMapper[NUM_BODY];
  vtkNew<vtkOpenGLActor> BodyActor[NUM_BODY];

private:
  vtkOpenGLAvatar(const vtkOpenGLAvatar&) = delete;
  void operator=(const vtkOpenGLAvatar&) = delete;
};

#endif
