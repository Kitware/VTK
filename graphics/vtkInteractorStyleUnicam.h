/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUnicam.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    contributed by A. Forsberg, Brown University, December 2000


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkInteractorStyleUnicam - provides Unicam navigation style
// .SECTION Description
// UniCam is a camera interactor.  Here, just the primary features of the
// UniCam technique are implemented.  UniCam requires just one mouse button
// and supports context sensitive zooming, dollying, and rotation.  (In this
// implementation, it uses the right mouse button, leaving the middle and
// left available for other functions.) For more information, see the paper
// at:
// 
//    ftp://ftp.cs.brown.edu/pub/papers/graphics/research/unicam.pdf
// 
// The following is a brief description of the UniCam Camera Controls.  You
// can perform 3 operations on the camera: rotate, pan the camera, and zoom.
// All operations are reached through the right mouse button & mouse
// movements.
// 
// IMPORTANT: UniCam assumes there is an axis that makes sense as a "up"
// vector for the world.  By default, this axis is defined to be the
// vector <0,0,1>.  You can set it explicitly for the data you are
// viewing with the 'SetWorldUpVector(..)' method in C++, or similarly
// in Tcl/Tk (or other interpreted languages).
// 
// 1. ROTATE:
// 
// Position the cursor over the point you wish to rotate around and press and
// release the right mouse button.  A 'focus dot' appears indicating the
// point that will be the center of rotation.  To rotate, press and hold the
// right mouse button and drag the mouse.. release the button to complete the
// rotation.
// 
// Rotations can be done without placing a focus dot first by moving the
// mouse cursor to within 10% of the window border & pressing and holding the
// right button followed by dragging the mouse.  The last focus dot position
// will be re-used.
// 
// 2. PAN:
// 
// Click and hold the right mouse button, and initially move the mouse left
// or right.  The point under the initial pick will pick correlate w/ the
// mouse tip-- (i.e., direct manipulation).
// 
// 3. ZOOM (+ PAN):
// 
// Click and hold the right mouse button, and initially move the mouse up or
// down.  Moving the mouse down will zoom in on the picked point, and moving
// the mouse up will zoom out.  Zooming occurs relative to the picked point
// which simplifies the task of zooming in on a region of interest.  Left and
// right mouse movements will pan the camera left and right.

// .SECTION Caveats
// (NOTE: This implementation of Unicam assumes a perspective camera.  It
// could be modified relatively easily to also support an orthographic
// projection.)


#ifndef __vtkInteractorStyleUnicam_h
#define __vtkInteractorStyleUnicam_h

#include "vtkInteractorStyle.h"
#include "vtkRenderer.h"
class vtkWorldPointPicker;

// define 'TheTime()' function-- returns time in elapsed seconds
#if defined(_WIN32) || defined(WIN32)
#include <winbase.h>

inline double TheTime() 
  {return double(GetTickCount())/1000.0;}
#else
#include <sys/time.h>

inline double TheTime() 
{
  struct timeval ts; struct timezone tz;
  gettimeofday(&ts, &tz);
  return (double)(ts.tv_sec + ts.tv_usec/1e6);
}
#endif


// 
// XXX - would have preferred to make these enumerations within the class,
//    enum { NONE=0, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT };
//    enum {CAM_INT_ROT, CAM_INT_CHOOSE, CAM_INT_PAN, CAM_INT_ZOOM};
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
#define VTK_UNICAM_CAM_INT_ZOOM   3

class VTK_EXPORT vtkInteractorStyleUnicam : public vtkInteractorStyle 
{
public:
  static vtkInteractorStyleUnicam *New();
  vtkTypeMacro(vtkInteractorStyleUnicam,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Concrete implementation of event bindings
  virtual   void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  virtual   void OnRightButtonUp  (int ctrl, int shift, int X, int Y);
  virtual   void OnMouseMove(int ctrl, int shift, int X, int Y);
  
  virtual   void OnLeftButtonMove  (int ctrl, int shift, int X, int Y) {}
  virtual   void OnMiddleButtonMove(int ctrl, int shift, int X, int Y) {}
  virtual   void OnRightButtonMove (int ctrl, int shift, int X, int Y);

  // Description:
  // OnTimer calls RotateCamera, RotateActor etc which should be overridden by
  // style subclasses.
  virtual void OnTimer(void);

  void SetWorldUpVector(double a[3]) {this->SetWorldUpVector(a[0],a[1],a[2]);}
  void SetWorldUpVector(float  a[3]) {this->SetWorldUpVector(a[0],a[1],a[2]);}
  void SetWorldUpVector(float x, float y, float z);
  vtkGetVectorMacro(WorldUpVector, float, 3);

protected:
  vtkInteractorStyleUnicam();
  virtual ~vtkInteractorStyleUnicam();
  vtkInteractorStyleUnicam(const vtkInteractorStyleUnicam&) {};
  void operator=(const vtkInteractorStyleUnicam&) {};

  vtkWorldPointPicker *InteractionPicker;
  
  int      ButtonDown;   // which button is down
  double   DTime;        // time mouse button was pressed
  double   Dist;         // distance the mouse has moved since button press
  float    StartPix[2]; // pixel mouse movement started at
  float    LastPos[2];  // normalized position of mouse last frame
  float    LastPix[2];  // pixel position of mouse last frame
  float    DownPt[3];   // 3D point under cursor when mouse button pressed
  float    Center [3];   // center of camera rotation

  float    WorldUpVector[3]; // what the world thinks the 'up' vector is

  vtkActor    *FocusSphere; // geometry for indicating center of rotation
  int          IsDot;       // flag-- is the FocusSphere being displayed?
  vtkRenderer *FocusSphereRenderer;  // renderer for 'FocusSphere'

  int state;                 // which navigation mode was selected?

  void choose( int X, int Y );  // method for choosing type of navigation
  void rot   ( int X, int Y );  // method for rotating
  void zoom  ( int X, int Y );  // method for zooming
  void pan   ( int X, int Y );  // method for panning

  // conveinence methods for translating & rotating the camera
  void  MyTranslateCamera(float v[3]);
  void  MyRotateCamera(float cx, float cy, float cz,
		       float ax, float ay, float az,
		       float angle);

  // convert a vector from camera to world space
  void  CameraToWorld(float *in, float *out);

  // takes in pixels, returns normalized window coordinates
  void  NormalizeMouseXY(int X, int Y, float *NX, float *NY);

  // return the aspect ratio of the current window
  float WindowAspect();
};

#endif  // __vtkInteractorStyleUnicam_h



