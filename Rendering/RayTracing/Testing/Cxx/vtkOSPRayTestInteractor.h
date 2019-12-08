/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayTestInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
// A common interactor style for the ospray tests that understands
// the following key presses.
// c        => switch between OSPRay and GL
// s        => turn shadows on and off
// n        => focuses view on the next actor and hides all others
// 2/1      => increase/decrease the number of samples per pixel
// P/p      => increase/decrease the number of OSPRay rendering passes
// l        => turns on each light in the scene in turn
// I/i      => increase/decrease the global light intensity scale
// D/d      => increase/decrease the number of ambient occlusion samples
// t        => change renderer type: scivis, pathtracer
// N        => toggle use of openimage denoiser, if applicable

#ifndef vtkOSPRayTestInteractor_h
#define vtkOSPRayTestInteractor_h

#include "vtkInteractorStyleTrackballCamera.h"

#include <string>
#include <vector>

class vtkCommand;
class vtkRenderer;
class vtkRenderPass;
class vtkRenderWindow;

// Define interaction style
class vtkOSPRayTestInteractor : public vtkInteractorStyleTrackballCamera
{
private:
  vtkRenderer* GLRenderer;
  vtkRenderPass* O;
  vtkRenderPass* G;
  int VisibleActor;
  int VisibleLight;
  vtkCommand* Looper;

public:
  static vtkOSPRayTestInteractor* New();
  vtkTypeMacro(vtkOSPRayTestInteractor, vtkInteractorStyleTrackballCamera);
  vtkOSPRayTestInteractor();
  ~vtkOSPRayTestInteractor();
  void SetPipelineControlPoints(vtkRenderer* g, vtkRenderPass* _O, vtkRenderPass* _G);
  virtual void OnKeyPress() override;

  static void AddName(const char* name);

  // access to a progressive rendering automator
  vtkCommand* GetLooper(vtkRenderWindow*);
};

#endif
