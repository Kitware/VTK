// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLAvatar
 * @brief   OpenGL Avatar
 *
 * vtkOpenGLAvatar is a concrete implementation of the abstract class vtkAvatar.
 * vtkOpenGLAvatar interfaces to the OpenGL rendering library.
 */

#ifndef vtkOpenGLAvatar_h
#define vtkOpenGLAvatar_h

#include "vtkAvatar.h"
#include "vtkNew.h"               // for ivars
#include "vtkRenderingVRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkOpenGLRenderer;
class vtkVRRay;
class vtkFlagpoleLabel;
class vtkTextProperty;

class VTKRENDERINGVR_EXPORT vtkOpenGLAvatar : public vtkAvatar
{
public:
  static vtkOpenGLAvatar* New();
  vtkTypeMacro(vtkOpenGLAvatar, vtkAvatar);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual Avatar render method.
   */
  int RenderOpaqueGeometry(vtkViewport* vp) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* vp) override;

  double* GetBounds() VTK_SIZEHINT(6) override;

  void SetUseLeftHand(bool val) override;
  void SetUseRightHand(bool val) override;
  void SetShowHandsOnly(bool val) override;

  // Set Ray parameters
  void SetLeftShowRay(bool v);
  void SetRightShowRay(bool v);
  void SetRayLength(double length);

  void SetLabel(const char* label);
  vtkTextProperty* GetLabelTextProperty();

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

  vtkNew<vtkVRRay> LeftRay;
  vtkNew<vtkVRRay> RightRay;

  vtkNew<vtkFlagpoleLabel> LabelActor;

private:
  vtkOpenGLAvatar(const vtkOpenGLAvatar&) = delete;
  void operator=(const vtkOpenGLAvatar&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
