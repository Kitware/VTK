// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAvatar
 * @brief Renders head and hands for a user in VR
 *
 * Set position and orientation for the head and two hands,
 * shows an observer where the avatar is looking and pointing.
 */

#ifndef vtkAvatar_h
#define vtkAvatar_h

#include "vtkActor.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT vtkAvatar : public vtkActor
{
public:
  static vtkAvatar* New();
  vtkTypeMacro(vtkAvatar, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set/Get the head and hand transforms.
   */
  vtkGetVector3Macro(HeadPosition, double);
  vtkSetVector3Macro(HeadPosition, double);
  vtkGetVector3Macro(HeadOrientation, double);
  vtkSetVector3Macro(HeadOrientation, double);

  vtkGetVector3Macro(LeftHandPosition, double);
  vtkSetVector3Macro(LeftHandPosition, double);
  vtkGetVector3Macro(LeftHandOrientation, double);
  vtkSetVector3Macro(LeftHandOrientation, double);

  vtkGetVector3Macro(RightHandPosition, double);
  vtkSetVector3Macro(RightHandPosition, double);
  vtkGetVector3Macro(RightHandOrientation, double);
  vtkSetVector3Macro(RightHandOrientation, double);

  /**
   * Up vector, in world coords. Must be normalized.
   */
  vtkGetVector3Macro(UpVector, double);
  vtkSetVector3Macro(UpVector, double);

  ///@{
  /**
   * Normally, hand position/orientation is set explicitly.
   * If set to false, hand and arm will follow the torso
   * in a neutral position.
   */
  vtkSetMacro(UseLeftHand, bool);
  vtkGetMacro(UseLeftHand, bool);
  vtkBooleanMacro(UseLeftHand, bool);
  vtkSetMacro(UseRightHand, bool);
  vtkGetMacro(UseRightHand, bool);
  vtkBooleanMacro(UseRightHand, bool);
  ///@}

  ///@{
  /**
   * Show just the hands. Default false.
   */
  vtkSetMacro(ShowHandsOnly, bool);
  vtkGetMacro(ShowHandsOnly, bool);
  vtkBooleanMacro(ShowHandsOnly, bool);
  ///@}

protected:
  vtkAvatar();
  ~vtkAvatar() override;

  double HeadPosition[3];
  double HeadOrientation[3];
  double LeftHandPosition[3];
  double LeftHandOrientation[3];
  double RightHandPosition[3];
  double RightHandOrientation[3];
  enum
  {
    TORSO,
    LEFT_FORE,
    RIGHT_FORE,
    LEFT_UPPER,
    RIGHT_UPPER,
    NUM_BODY,
  };
  double BodyPosition[NUM_BODY][3];
  double BodyOrientation[NUM_BODY][3];

  double UpVector[3];

  bool UseLeftHand;
  bool UseRightHand;
  bool ShowHandsOnly;

private:
  vtkAvatar(const vtkAvatar&) = delete;
  void operator=(const vtkAvatar&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAvatar_h
