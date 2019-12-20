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
* @class   vtkOpenVRRay
* @brief   OpenVR device model

* Represents a ray shooting from a VR controller, used for pointing or picking.
*/

#ifndef vtkOpenVRRay_h
#define vtkOpenVRRay_h

#include "vtkNew.h" // for ivar
#include "vtkObject.h"
#include "vtkOpenGLHelper.h"          // ivar
#include "vtkRenderingOpenVRModule.h" // For export macro
#include <openvr.h>                   // for ivars

class vtkOpenGLRenderWindow;
class vtkRenderWindow;
class vtkOpenGLVertexBufferObject;
class vtkMatrix4x4;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRay : public vtkObject
{
public:
  static vtkOpenVRRay* New();
  vtkTypeMacro(vtkOpenVRRay, vtkObject);
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
  vtkOpenVRRay();
  ~vtkOpenVRRay() override;

  bool Show;
  bool Loaded;

  vtkOpenGLHelper RayHelper;
  vtkOpenGLVertexBufferObject* RayVBO;
  vtkNew<vtkMatrix4x4> PoseMatrix;

  float Length;
  float Color[3];

private:
  vtkOpenVRRay(const vtkOpenVRRay&) = delete;
  void operator=(const vtkOpenVRRay&) = delete;
};

#endif
