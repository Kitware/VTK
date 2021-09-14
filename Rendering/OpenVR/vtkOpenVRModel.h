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
* @class   vtkOpenVRModel
* @brief   OpenVR device model

* This internal class is used to load models
* such as for the trackers and controllers and to
* render them in the scene
*/

#ifndef vtkOpenVRModel_h
#define vtkOpenVRModel_h

#include "vtkNew.h"                   // for ivar
#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRModel.h"
#include <openvr.h> // for ivars

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRModel : public vtkVRModel
{
public:
  static vtkOpenVRModel* New();
  vtkTypeMacro(vtkOpenVRModel, vtkVRModel);

  vr::RenderModel_t* RawModel;

protected:
  vtkOpenVRModel();
  ~vtkOpenVRModel() override = default;

  void FillModelHelper() override;
  void SetPositionAndTCoords() override;
  void CreateTextureObject(vtkOpenGLRenderWindow* win) override;
  void LoadModelAndTexture(vtkOpenGLRenderWindow* win) override;
  vr::RenderModel_TextureMap_t* RawTexture;

private:
  vtkOpenVRModel(const vtkOpenVRModel&) = delete;
  void operator=(const vtkOpenVRModel&) = delete;
};

#endif
