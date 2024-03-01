// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderWindowInteractor3D
 * @brief   adds support for 3D events to vtkRenderWindowInteractor.
 *
 *
 * vtkRenderWindowInteractor3D provides a platform-independent interaction
 * support for 3D events including 3D clicks and 3D controller
 * orientations. It follows the same basic model as
 * vtkRenderWindowInteractor but adds methods to set and get 3D event
 * locations and orientations. VR systems will subclass this class to
 * provide the code to set these values based on events from their VR
 * controllers.
 */

#ifndef vtkRenderWindowInteractor3D_h
#define vtkRenderWindowInteractor3D_h

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingCoreModule.h" // For export macro

#include "vtkNew.h" // ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkCamera;
class vtkMatrix4x4;
enum class vtkEventDataDevice;
enum class vtkEventDataDeviceInput;

class VTKRENDERINGCORE_EXPORT vtkRenderWindowInteractor3D : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkRenderWindowInteractor3D* New();

  vtkTypeMacro(vtkRenderWindowInteractor3D, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  void Enable() override;
  void Disable() override;
  ///@}

  ///@{
  /**
   * With VR we know the world coordinate positions and orientations of events.
   * These methods support querying them instead of going through a display X,Y
   * coordinate approach as is standard for mouse/touch events
   */
  virtual double* GetWorldEventPosition(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->WorldEventPositions[pointerIndex];
  }
  virtual double* GetLastWorldEventPosition(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->LastWorldEventPositions[pointerIndex];
  }
  virtual double* GetWorldEventOrientation(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->WorldEventOrientations[pointerIndex];
  }
  virtual double* GetLastWorldEventOrientation(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->LastWorldEventOrientations[pointerIndex];
  }
  virtual void GetWorldEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  virtual void GetLastWorldEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  ///@}

  ///@{
  /**
   * With VR we know the physical/room coordinate positions
   * and orientations of events.
   * These methods support setting them.
   */
  virtual void SetPhysicalEventPosition(double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= VTKI_MAX_POINTERS)
    {
      return;
    }
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting PhysicalEventPosition to ("
                  << x << "," << y << "," << z << ") for pointerIndex number " << pointerIndex);
    if (this->PhysicalEventPositions[pointerIndex][0] != x ||
      this->PhysicalEventPositions[pointerIndex][1] != y ||
      this->PhysicalEventPositions[pointerIndex][2] != z ||
      this->LastPhysicalEventPositions[pointerIndex][0] != x ||
      this->LastPhysicalEventPositions[pointerIndex][1] != y ||
      this->LastPhysicalEventPositions[pointerIndex][2] != z)
    {
      this->LastPhysicalEventPositions[pointerIndex][0] =
        this->PhysicalEventPositions[pointerIndex][0];
      this->LastPhysicalEventPositions[pointerIndex][1] =
        this->PhysicalEventPositions[pointerIndex][1];
      this->LastPhysicalEventPositions[pointerIndex][2] =
        this->PhysicalEventPositions[pointerIndex][2];
      this->PhysicalEventPositions[pointerIndex][0] = x;
      this->PhysicalEventPositions[pointerIndex][1] = y;
      this->PhysicalEventPositions[pointerIndex][2] = z;
      this->Modified();
    }
  }
  virtual void SetPhysicalEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  ///@}

  ///@{
  /**
   * With VR we know the physical/room coordinate positions
   * and orientations of events.
   * These methods support getting them.
   */
  virtual void GetPhysicalEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  virtual void GetLastPhysicalEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  virtual void GetStartingPhysicalEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  ///@}

  /**
   * Return starting physical to world matrix.
   */
  virtual void GetStartingPhysicalToWorldMatrix(vtkMatrix4x4* startingPhysicalToWorldMatrix);

  /**
   * Set starting physical to world matrix.
   *
   * This method is intended to be used when defining a custom heuristic
   * for recognizing complex gestures.
   *
   * This method **does not** call `this->Modified()`.
   *
   * \sa vtkVRRenderWindowInteractor::HandleComplexGestureEvents()
   * \sa vtkVRRenderWindowInteractor::RecognizeComplexGesture()
   */
  virtual void SetStartingPhysicalToWorldMatrix(vtkMatrix4x4* startingPhysicalToWorldMatrix);

  /**
   * Set starting physical event pose.
   *
   * This method is intended to be used when defining a custom heuristic
   * for recognizing complex gestures.
   *
   * This method **does not** call `this->Modified()`.
   *
   * \sa vtkVRRenderWindowInteractor::HandleComplexGestureEvents()
   * \sa vtkVRRenderWindowInteractor::RecognizeComplexGesture()
   */
  virtual void SetStartingPhysicalEventPose(vtkMatrix4x4* poseMatrix, vtkEventDataDevice device);

  ///@{
  /**
   * With VR we know the world coordinate positions
   * and orientations of events. These methods
   * support setting them.
   */
  virtual void SetWorldEventPosition(double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= VTKI_MAX_POINTERS)
    {
      return;
    }
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WorldEventPosition to ("
                  << x << "," << y << "," << z << ") for pointerIndex number " << pointerIndex);
    if (this->WorldEventPositions[pointerIndex][0] != x ||
      this->WorldEventPositions[pointerIndex][1] != y ||
      this->WorldEventPositions[pointerIndex][2] != z ||
      this->LastWorldEventPositions[pointerIndex][0] != x ||
      this->LastWorldEventPositions[pointerIndex][1] != y ||
      this->LastWorldEventPositions[pointerIndex][2] != z)
    {
      this->LastWorldEventPositions[pointerIndex][0] = this->WorldEventPositions[pointerIndex][0];
      this->LastWorldEventPositions[pointerIndex][1] = this->WorldEventPositions[pointerIndex][1];
      this->LastWorldEventPositions[pointerIndex][2] = this->WorldEventPositions[pointerIndex][2];
      this->WorldEventPositions[pointerIndex][0] = x;
      this->WorldEventPositions[pointerIndex][1] = y;
      this->WorldEventPositions[pointerIndex][2] = z;
      this->Modified();
    }
  }
  virtual void SetWorldEventOrientation(double w, double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= VTKI_MAX_POINTERS)
    {
      return;
    }
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WorldEventOrientation to ("
                  << w << "," << x << "," << y << "," << z << ") for pointerIndex number "
                  << pointerIndex);
    if (this->WorldEventOrientations[pointerIndex][0] != w ||
      this->WorldEventOrientations[pointerIndex][1] != x ||
      this->WorldEventOrientations[pointerIndex][2] != y ||
      this->WorldEventOrientations[pointerIndex][3] != z ||
      this->LastWorldEventOrientations[pointerIndex][0] != w ||
      this->LastWorldEventOrientations[pointerIndex][1] != x ||
      this->LastWorldEventOrientations[pointerIndex][2] != y ||
      this->LastWorldEventOrientations[pointerIndex][3] != z)
    {
      this->LastWorldEventOrientations[pointerIndex][0] =
        this->WorldEventOrientations[pointerIndex][0];
      this->LastWorldEventOrientations[pointerIndex][1] =
        this->WorldEventOrientations[pointerIndex][1];
      this->LastWorldEventOrientations[pointerIndex][2] =
        this->WorldEventOrientations[pointerIndex][2];
      this->LastWorldEventOrientations[pointerIndex][3] =
        this->WorldEventOrientations[pointerIndex][3];
      this->WorldEventOrientations[pointerIndex][0] = w;
      this->WorldEventOrientations[pointerIndex][1] = x;
      this->WorldEventOrientations[pointerIndex][2] = y;
      this->WorldEventOrientations[pointerIndex][3] = z;
      this->Modified();
    }
  }
  virtual void SetWorldEventPose(vtkMatrix4x4* poseMatrix, int pointerIndex);
  ///@}

  ///@{
  /**
   * Override to set pointers down
   */
  void RightButtonPressEvent() override;
  void RightButtonReleaseEvent() override;
  ///@}

  ///@{
  /**
   * Override to set pointers down
   */
  void MiddleButtonPressEvent() override;
  void MiddleButtonReleaseEvent() override;
  ///@}

  ///@{
  /**
   * Get the latest touchpad or joystick position for a device
   */
  virtual void GetTouchPadPosition(vtkEventDataDevice, vtkEventDataDeviceInput, float[3]) {}
  ///@}

  ///@{
  /**
   * Set/get the direction of the physical coordinate system -Z axis in world coordinates.
   */
  virtual void SetPhysicalViewDirection(double, double, double) {}
  virtual double* GetPhysicalViewDirection() { return nullptr; }
  ///@}

  ///@{
  /**
   * Set/get the direction of the physical coordinate system +Y axis in world coordinates.
   */
  virtual void SetPhysicalViewUp(double, double, double) {}
  virtual double* GetPhysicalViewUp() { return nullptr; }
  ///@}

  ///@{
  /**
   * Set/get position of the physical coordinate system origin in world coordinates.
   */
  virtual void SetPhysicalTranslation(vtkCamera*, double, double, double) {}
  virtual double* GetPhysicalTranslation(vtkCamera*) { return nullptr; }
  ///@}

  ///@{
  /**
   * Set/get the physical scale (world / physical distance ratio)
   */
  virtual void SetPhysicalScale(double) {}
  virtual double GetPhysicalScale() { return 1.0; }
  ///@}

  ///@{
  /**
   * Set/get the translation for pan/swipe gestures, update LastTranslation
   */
  void SetTranslation3D(double val[3]);
  vtkGetVector3Macro(Translation3D, double);
  vtkGetVector3Macro(LastTranslation3D, double);
  ///@}

protected:
  vtkRenderWindowInteractor3D();
  ~vtkRenderWindowInteractor3D() override;

  int MouseInWindow;
  int StartedMessageLoop;
  double Translation3D[3];
  double LastTranslation3D[3];

  double WorldEventPositions[VTKI_MAX_POINTERS][3];
  double LastWorldEventPositions[VTKI_MAX_POINTERS][3];
  double PhysicalEventPositions[VTKI_MAX_POINTERS][3];
  double LastPhysicalEventPositions[VTKI_MAX_POINTERS][3];
  double StartingPhysicalEventPositions[VTKI_MAX_POINTERS][3];
  double WorldEventOrientations[VTKI_MAX_POINTERS][4];
  double LastWorldEventOrientations[VTKI_MAX_POINTERS][4];
  vtkNew<vtkMatrix4x4> WorldEventPoses[VTKI_MAX_POINTERS];
  vtkNew<vtkMatrix4x4> LastWorldEventPoses[VTKI_MAX_POINTERS];
  vtkNew<vtkMatrix4x4> PhysicalEventPoses[VTKI_MAX_POINTERS];
  vtkNew<vtkMatrix4x4> LastPhysicalEventPoses[VTKI_MAX_POINTERS];
  vtkNew<vtkMatrix4x4> StartingPhysicalEventPoses[VTKI_MAX_POINTERS];
  void RecognizeGesture(vtkCommand::EventIds) override;

  /**
   * Store physical to world matrix at the start of a complex gesture.
   */
  vtkNew<vtkMatrix4x4> StartingPhysicalToWorldMatrix;

private:
  vtkRenderWindowInteractor3D(const vtkRenderWindowInteractor3D&) = delete;
  void operator=(const vtkRenderWindowInteractor3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
