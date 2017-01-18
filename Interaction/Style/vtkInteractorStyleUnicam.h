/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUnicam.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * This work (vtkInteractorStyleUnicam.h) was produced under a grant from
 * the Department of Energy to Brown University.  Neither Brown University
 * nor the authors assert any copyright with respect to this work and it may
 * be used, reproduced, and distributed without permission.
 */

/**
 * @class   vtkInteractorStyleUnicam
 * @brief   provides Unicam navigation style
 *
 * UniCam is a camera interactor.  Here, just the primary features of the
 * UniCam technique are implemented.  UniCam requires just one mouse button
 * and supports context sensitive dollying, panning, and rotation.  (In this
 * implementation, it uses the right mouse button, leaving the middle and
 * left available for other functions.) For more information, see the paper
 * at:
 *
 *    ftp://ftp.cs.brown.edu/pub/papers/graphics/research/unicam.pdf
 *
 * The following is a brief description of the UniCam Camera Controls.  You
 * can perform 3 operations on the camera: rotate, pan, and dolly the camera.
 * All operations are reached through the right mouse button & mouse
 * movements.
 *
 * IMPORTANT: UniCam assumes there is an axis that makes sense as a "up"
 * vector for the world.  By default, this axis is defined to be the
 * vector <0,0,1>.  You can set it explicitly for the data you are
 * viewing with the 'SetWorldUpVector(..)' method in C++, or similarly
 * in Tcl/Tk (or other interpreted languages).
 *
 * 1. ROTATE:
 *
 * Position the cursor over the point you wish to rotate around and press and
 * release the left mouse button.  A 'focus dot' appears indicating the
 * point that will be the center of rotation.  To rotate, press and hold the
 * left mouse button and drag the mouse.. release the button to complete the
 * rotation.
 *
 * Rotations can be done without placing a focus dot first by moving the
 * mouse cursor to within 10% of the window border & pressing and holding the
 * left button followed by dragging the mouse.  The last focus dot position
 * will be re-used.
 *
 * 2. PAN:
 *
 * Click and hold the left mouse button, and initially move the mouse left
 * or right.  The point under the initial pick will pick correlate w/ the
 * mouse tip-- (i.e., direct manipulation).
 *
 * 3. DOLLY (+ PAN):
 *
 * Click and hold the left mouse button, and initially move the mouse up or
 * down.  Moving the mouse down will dolly towards the picked point, and moving
 * the mouse up will dolly away from it.  Dollying occurs relative to the
 * picked point which simplifies the task of dollying towards a region of
 * interest. Left and right mouse movements will pan the camera left and right.
 *
 * @warning
 * (NOTE: This implementation of Unicam assumes a perspective camera.  It
 * could be modified relatively easily to also support an orthographic
 * projection.)
*/

#ifndef vtkInteractorStyleUnicam_h
#define vtkInteractorStyleUnicam_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

class vtkCamera;
class vtkWorldPointPicker;

//
// XXX - would have preferred to make these enumerations within the class,
//    enum { NONE=0, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT };
//    enum {CAM_INT_ROT, CAM_INT_CHOOSE, CAM_INT_PAN, CAM_INT_DOLLY};
// but vtkWrapTcl signaled a "syntax error" when it parsed the 'enum' line.
// So, am making them defines which is what the other classes that want
// to have constants appear to do.
//
// buttons pressed
#define VTK_UNICAM_NONE           0
#define VTK_UNICAM_BUTTON_LEFT    1
#define VTK_UNICAM_BUTTON_MIDDLE  2
#define VTK_UNICAM_BUTTON_RIGHT   3
//
// camera modes
#define VTK_UNICAM_CAM_INT_ROT    0
#define VTK_UNICAM_CAM_INT_CHOOSE 1
#define VTK_UNICAM_CAM_INT_PAN    2
#define VTK_UNICAM_CAM_INT_DOLLY  3

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleUnicam : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleUnicam *New();
  vtkTypeMacro(vtkInteractorStyleUnicam,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void SetWorldUpVector(double a[3]) {this->SetWorldUpVector(a[0],a[1],a[2]);}
  void SetWorldUpVector(double x, double y, double z);
  vtkGetVectorMacro(WorldUpVector, double, 3);

  //@{
  /**
   * Concrete implementation of event bindings
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  virtual void OnLeftButtonMove();
  //@}

  /**
   * OnTimer calls RotateCamera, RotateActor etc which should be overridden by
   * style subclasses.
   */
  void OnTimer() VTK_OVERRIDE;

protected:
  vtkInteractorStyleUnicam();
  ~vtkInteractorStyleUnicam() VTK_OVERRIDE;

  vtkWorldPointPicker *InteractionPicker;

  int      ButtonDown;   // which button is down
  double   DTime;        // time mouse button was pressed
  double   Dist;         // distance the mouse has moved since button press
  double    StartPix[2]; // pixel mouse movement started at
  double    LastPos[2];  // normalized position of mouse last frame
  double    LastPix[2];  // pixel position of mouse last frame
  double    DownPt[3];   // 3D point under cursor when mouse button pressed
  double    Center [3];   // center of camera rotation

  double    WorldUpVector[3]; // what the world thinks the 'up' vector is

  vtkActor    *FocusSphere; // geometry for indicating center of rotation
  int          IsDot;       // flag-- is the FocusSphere being displayed?
  vtkRenderer *FocusSphereRenderer;  // renderer for 'FocusSphere'

  int state;                 // which navigation mode was selected?

  void ChooseXY( int X, int Y );  // method for choosing type of navigation
  void RotateXY( int X, int Y );  // method for rotating
  void DollyXY( int X, int Y );  // method for dollying
  void PanXY( int X, int Y );  // method for panning

  // conveinence methods for translating & rotating the camera
  void  MyTranslateCamera(double v[3]);
  void  MyRotateCamera(double cx, double cy, double cz,
                       double ax, double ay, double az,
                       double angle);

  // Given a 3D point & a vtkCamera, compute the vectors that extend
  // from the projection of the center of projection to the center of
  // the right-edge and the center of the top-edge onto the plane
  // containing the 3D point & with normal parallel to the camera's
  // projection plane.
  void  GetRightVandUpV(double *p, vtkCamera *cam,
                        double *rightV, double *upV);

  // takes in pixels, returns normalized window coordinates
  void  NormalizeMouseXY(int X, int Y, double *NX, double *NY);

  // return the aspect ratio of the current window
  double WindowAspect();
private:
  vtkInteractorStyleUnicam(const vtkInteractorStyleUnicam&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleUnicam&) VTK_DELETE_FUNCTION;
};

#endif  // vtkInteractorStyleUnicam_h



