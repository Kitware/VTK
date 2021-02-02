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
* @class   vtkOpenXRRay
* @brief   OpenXR device model

* Represents a ray shooting from a VR controller, used for pointing or picking.
*/

#ifndef vtkOpenXRRay_h
#define vtkOpenXRRay_h

#include "vtkMatrix4x4.h"
#include "vtkNew.h" // for ivar
#include "vtkObject.h"
#include "vtkOpenGLHelper.h" // ivar
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

class vtkOpenGLRenderWindow;
class vtkRenderWindow;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRRay : public vtkObject
{
public:
  static vtkOpenXRRay* New();
  vtkTypeMacro(vtkOpenXRRay, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Build(vtkOpenGLRenderWindow* win);
  void Render(vtkOpenGLRenderWindow* win, vtkMatrix4x4* poseMatrix = nullptr);

  // show the model
  vtkSetMacro(Show, bool);
  vtkGetMacro(Show, bool);

  vtkSetMacro(Length, float);
  vtkSetVector3Macro(Color, float);
  vtkSetObjectMacro(PoseMatrix, vtkMatrix4x4);

  vtkSetMacro(Name, std::string);
  vtkGetMacro(Name, std::string);

  void ReleaseGraphicsResources(vtkRenderWindow* win);

protected:
  vtkOpenXRRay() = default;
  ~vtkOpenXRRay() override = default;

  // To imitate the vtkOpenVRModel class
  // as an OpenXRModel does not exist yet,
  // Store some infos about the model
  std::string Name;

  bool Show = false;
  bool Loaded = false;

  float Length = 1.0f;
  float Color[3] = { 1.f, 0.f, 0.f };

  vtkNew<vtkOpenGLVertexBufferObject> RayVBO;
  vtkOpenGLHelper RayHelper;
  vtkMatrix4x4* PoseMatrix;

private:
  vtkOpenXRRay(const vtkOpenXRRay&) = delete;
  void operator=(const vtkOpenXRRay&) = delete;
};

#endif
