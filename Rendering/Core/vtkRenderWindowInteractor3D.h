/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkRenderWindowInteractor.h"

class vtkCamera;

class VTKRENDERINGCORE_EXPORT vtkRenderWindowInteractor3D : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkRenderWindowInteractor3D *New();

  vtkTypeMacro(vtkRenderWindowInteractor3D,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  void Enable() VTK_OVERRIDE;
  void Disable() VTK_OVERRIDE;
  //@}

  /**
   * OpenVR specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate the application. An application can Specify
   * ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
   */
  void TerminateApp(void) VTK_OVERRIDE;

  /**
   * Create default picker. Used to create one when none is specified.
   * Default is an instance of vtkPropPicker.
   */
  vtkAbstractPropPicker *CreateDefaultPicker() VTK_OVERRIDE;

  //@{
  /**
   * With VR we know the world coordinate positions
   * and orientations of events. These methods
   * support querying them instead of going through
   * a display X,Y coordinate approach as is standard
   * for mouse/touch events
   */
  virtual double *GetWorldEventPosition(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return NULL;
    }
    return this->WorldEventPositions[pointerIndex];
  }
  virtual double *GetLastWorldEventPosition(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return NULL;
    }
    return this->LastWorldEventPositions[pointerIndex];
  }
  virtual double *GetWorldEventOrientation(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return NULL;
    }
    return this->WorldEventOrientations[pointerIndex];
  }
  virtual double *GetLastWorldEventOrientation(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return NULL;
    }
    return this->LastWorldEventOrientations[pointerIndex];
  }
  //@}

  //@{
  /**
   * With VR we know the physical/room coordinate positions
   * and orientations of events. These methods
   * support setting them.
   */
  virtual void SetPhysicalEventPosition(double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= VTKI_MAX_POINTERS)
    {
      return;
    }
    vtkDebugMacro(
      << this->GetClassName() << " (" << this
      << "): setting PhysicalEventPosition to ("
      << x << "," << y << "," << z
      << ") for pointerIndex number " << pointerIndex);
    if (this->PhysicalEventPositions[pointerIndex][0] != x ||
        this->PhysicalEventPositions[pointerIndex][1] != y ||
        this->PhysicalEventPositions[pointerIndex][2] != z ||
        this->LastPhysicalEventPositions[pointerIndex][0] != x ||
        this->LastPhysicalEventPositions[pointerIndex][1] != y ||
        this->LastPhysicalEventPositions[pointerIndex][2] != z)
    {
      this->LastPhysicalEventPositions[pointerIndex][0] = this->PhysicalEventPositions[pointerIndex][0];
      this->LastPhysicalEventPositions[pointerIndex][1] = this->PhysicalEventPositions[pointerIndex][1];
      this->LastPhysicalEventPositions[pointerIndex][2] = this->PhysicalEventPositions[pointerIndex][2];
      this->PhysicalEventPositions[pointerIndex][0] = x;
      this->PhysicalEventPositions[pointerIndex][1] = y;
      this->PhysicalEventPositions[pointerIndex][2] = z;
      this->Modified();
    }
  }
  //@}

  //@{
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
    vtkDebugMacro(
      << this->GetClassName() << " (" << this
      << "): setting WorldEventPosition to ("
      << x << "," << y << "," << z
      << ") for pointerIndex number " << pointerIndex);
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
    vtkDebugMacro(
      << this->GetClassName() << " (" << this
      << "): setting WorldEventOrientation to ("
      << w << "," << x << "," << y << "," << z
      << ") for pointerIndex number " << pointerIndex);
    if (this->WorldEventOrientations[pointerIndex][0] != w ||
        this->WorldEventOrientations[pointerIndex][1] != x ||
        this->WorldEventOrientations[pointerIndex][2] != y ||
        this->WorldEventOrientations[pointerIndex][3] != z ||
        this->LastWorldEventOrientations[pointerIndex][0] != w ||
        this->LastWorldEventOrientations[pointerIndex][1] != x ||
        this->LastWorldEventOrientations[pointerIndex][2] != y ||
        this->LastWorldEventOrientations[pointerIndex][3] != z)
    {
      this->LastWorldEventOrientations[pointerIndex][0] = this->WorldEventOrientations[pointerIndex][0];
      this->LastWorldEventOrientations[pointerIndex][1] = this->WorldEventOrientations[pointerIndex][1];
      this->LastWorldEventOrientations[pointerIndex][2] = this->WorldEventOrientations[pointerIndex][2];
      this->LastWorldEventOrientations[pointerIndex][3] = this->WorldEventOrientations[pointerIndex][3];
      this->WorldEventOrientations[pointerIndex][0] = w;
      this->WorldEventOrientations[pointerIndex][1] = x;
      this->WorldEventOrientations[pointerIndex][2] = y;
      this->WorldEventOrientations[pointerIndex][3] = z;
      this->Modified();
    }
  }
  //@}

  //@{
  /**
   * Override to set pointers down
   */
  void RightButtonPressEvent() VTK_OVERRIDE;
  void RightButtonReleaseEvent() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Override to set pointers down
   */
  void MiddleButtonPressEvent() VTK_OVERRIDE;
  void MiddleButtonReleaseEvent() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the latest touchpad position
   */
  vtkSetVector2Macro(TouchPadPosition,float);
  vtkGetVector2Macro(TouchPadPosition,float);
  //@}

  //@{
  /**
   * Set/Get the optional translation to map world coordinates into the
   * 3D physical space (meters, 0,0,0).
   */
  virtual void SetPhysicalTranslation(vtkCamera *, double, double, double) {};
  virtual double *GetPhysicalTranslation(vtkCamera *) { return NULL; };
  //@}

  //@{
  /**
   * Set/get the tranlation for pan/swipe gestures, update LastTranslation
   */
  void SetTranslation3D(double val[3]);
  vtkGetVector3Macro(Translation3D, double);
  vtkGetVector3Macro(LastTranslation3D, double);
  //@}

  /**
   * Is the interactor loop done
   */
  vtkGetMacro(Done, bool);

protected:
  vtkRenderWindowInteractor3D();
  ~vtkRenderWindowInteractor3D() VTK_OVERRIDE;

  int     MouseInWindow;
  int     StartedMessageLoop;
  float TouchPadPosition[2];
  double Translation3D[3];
  double LastTranslation3D[3];

  bool Done;  // is the event loop done running

  double   WorldEventPositions[VTKI_MAX_POINTERS][3];
  double   LastWorldEventPositions[VTKI_MAX_POINTERS][3];
  double   PhysicalEventPositions[VTKI_MAX_POINTERS][3];
  double   LastPhysicalEventPositions[VTKI_MAX_POINTERS][3];
  double   StartingPhysicalEventPositions[VTKI_MAX_POINTERS][3];
  double   WorldEventOrientations[VTKI_MAX_POINTERS][4];
  double   LastWorldEventOrientations[VTKI_MAX_POINTERS][4];
  void RecognizeGesture(vtkCommand::EventIds) VTK_OVERRIDE;

private:
  vtkRenderWindowInteractor3D(const vtkRenderWindowInteractor3D&) VTK_DELETE_FUNCTION;  // Not implemented.
  void operator=(const vtkRenderWindowInteractor3D&) VTK_DELETE_FUNCTION;  // Not implemented.
};

#endif
