/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRRay.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkVRRay
* @brief   VR device model

* Represents a ray shooting from a VR controller, used for pointing or picking.
*/

#ifndef vtkVRRay_h
#define vtkVRRay_h

#include "vtkNew.h" // for ivar
#include "vtkObject.h"
#include "vtkOpenGLHelper.h"      // ivar
#include "vtkRenderingVRModule.h" // For export macro

class vtkOpenGLRenderWindow;
class vtkRenderWindow;
class vtkOpenGLVertexBufferObject;
class vtkMatrix4x4;

class VTKRENDERINGVR_EXPORT vtkVRRay : public vtkObject
{
public:
  static vtkVRRay* New();
  vtkTypeMacro(vtkVRRay, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Build(vtkOpenGLRenderWindow* win);
  void Render(vtkOpenGLRenderWindow* win, vtkMatrix4x4* poseMatrix);

  // show the model
  vtkSetMacro(Show, bool);
  vtkGetMacro(Show, bool);

  vtkSetMacro(Length, float);

  vtkSetVector3Macro(Color, float);

  void ReleaseGraphicsResources(vtkRenderWindow* win);

protected:
  vtkVRRay();
  ~vtkVRRay() override;

  bool Show;
  bool Loaded;

  vtkOpenGLHelper RayHelper;
  vtkOpenGLVertexBufferObject* RayVBO;
  vtkNew<vtkMatrix4x4> PoseMatrix;

  float Length;
  float Color[3];

private:
  vtkVRRay(const vtkVRRay&) = delete;
  void operator=(const vtkVRRay&) = delete;
};

#endif
