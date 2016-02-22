/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayTestInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
// .SECTION Description
// A common interactor style for the ospray tests that understands
// the following key presses.
// c        => switch between OSPRay and GL
// s        => turn shadows on and off
// n        => focusses view on the next actor and hides all others
// 2/1      => increase/decrease the number of samples per pixel
// P/p      => increase/decrease the number of OSPRay rendering passes

#ifndef vtkOsprayActorNode_h
#define vtkOsprayActorNode_h

#include "vtkInteractorStyleTrackballCamera.h"

#include <vector>
#include <string>

class vtkOpenGLRenderer;
class vtkRenderPass;

// Define interaction style
class vtkOsprayTestInteractor : public vtkInteractorStyleTrackballCamera
{
private:
  vtkOpenGLRenderer *GLRenderer;
  vtkRenderPass *O;
  vtkRenderPass *G;
  int VisibleActor;

public:
  static vtkOsprayTestInteractor* New();
  vtkTypeMacro(vtkOsprayTestInteractor, vtkInteractorStyleTrackballCamera);
  vtkOsprayTestInteractor();
  void SetPipelineControlPoints(vtkOpenGLRenderer *g,
                                vtkRenderPass *_O,
                                vtkRenderPass *_G);
  virtual void OnKeyPress();

  static void AddName(const char *name);
};

#endif
