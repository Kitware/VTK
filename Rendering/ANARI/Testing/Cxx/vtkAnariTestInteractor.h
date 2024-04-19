// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Description
// A common interactor style for the ANARI tests that understands
// the following key presses.
// c        => switch between ANARI and GL
// n        => focuses view on the next actor and hides all others
// 2/1      => increase/decrease the number of samples per pixel
// l        => turns on each light in the scene in turn
// I/i      => increase/decrease the global light intensity scale
// D/d      => increase/decrease the number of ambient occlusion samples
// t        => change renderer type: default, scivis, pathtracer
// b        => TODO: Change other parameters

#ifndef vtkAnariTestInteractor_h
#define vtkAnariTestInteractor_h

#include "vtkInteractorStyleTrackballCamera.h"

#include <string>
#include <vector>

class vtkCommand;
class vtkRenderer;
class vtkRenderPass;
class vtkRenderWindow;

// Define interaction style
class vtkAnariTestInteractor : public vtkInteractorStyleTrackballCamera
{
private:
  vtkRenderer* GLRenderer;
  vtkRenderPass* O;
  vtkRenderPass* G;
  int VisibleActor;
  int VisibleLight;
  vtkCommand* Looper;

public:
  static vtkAnariTestInteractor* New();
  vtkTypeMacro(vtkAnariTestInteractor, vtkInteractorStyleTrackballCamera);
  vtkAnariTestInteractor();
  ~vtkAnariTestInteractor();
  void SetPipelineControlPoints(vtkRenderer* g, vtkRenderPass* _O, vtkRenderPass* _G);
  virtual void OnKeyPress() override;

  static void AddName(const char* name);

  // access to a progressive rendering automator
  vtkCommand* GetLooper(vtkRenderWindow*);
};

#endif
