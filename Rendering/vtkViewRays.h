/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewRays.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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

// .NAME vtkViewRays - provides view ray information for efficiently casting rays
// .SECTION Description
// The vtkViewRays class calculates and stores the relevant information 
// necessary to efficiently render perspective or parallel viewing rays. 
// View rays are typically used by a ray caster where a view ray is
// cast per pixel in the requested image. 
//
// The number of view rays requested in both X and Y is specified by
// the SetSize() method. The camera model used to create the view ray 
// information is specified by the SetRenderer() method. Both the SetSize()
// and SetRenderer() method must be called before viewing ray information 
// can be requested. vtkViewRays will recompute view ray information should
// the critical information in the camera, the renderer or number of rays 
// change.
//
// vtkViewRays contains view ray information in view coordinates for either 
// parallel or perspective viewing transformations. This reduces the
// computational burden on a ray caster since the computation of perspective,
// normalized viewing ray vectors is pre-computed once rather than during each
// rendering.
// 
// When the camera viewing transform is perspective the start of each 
// viewing vector (eye point) is fixed, but the direction of each ray varies. 
// Therefore, vtkViewRays contains a 2D array of 3D unit vectors each 
// representing the direction of a view ray with respect to the viewing 
// coordinate system. A pointer to this 2D array of vectors is obtained by 
// calling the GetPerspectiveViewRays() method.
//
// When the camera viewing transform is parallel the start of each viewing 
// vector varies across the viewing plane but the direction remains constant. 
// Since the starting point of each view ray can be captured by regularly 
// sampling the parallel viewing plane, a 2D array of starting view ray 
// positions is NOT used. Instead, the first view ray position (bottom left
// corner of view) is returned by the GetParallelStartPosition() method and
// the X and Y distance increments to the next view ray starting position 
// are returned by the GetParallelIncrements() method.

#ifndef __vtkViewRays_h
#define __vtkViewRays_h
#include "vtkObject.h"
#include "vtkMatrix4x4.h"
class vtkRenderer;

class VTK_EXPORT vtkViewRays :public vtkObject
  {
public:
  static vtkViewRays *New();
    vtkTypeMacro(vtkViewRays,vtkObject);
  void PrintSelf(ostream& os,vtkIndent indent);

  // Description:
  // Specify the vtkRenderer whose camera will be used to to calculate 
  // the view rays. The type of camera (parallel, perspective) will
  // determine which type of view ray information is calculated.
  // Note: this does not increase the reference count of the renderer.
  void SetRenderer(vtkRenderer *ren);
  vtkGetObjectMacro(Renderer,vtkRenderer);

  // Description:
  // Set the image size for the view rays.
  vtkSetVector2Macro(Size,int);
  vtkGetVector2Macro(Size,int);

  // Description:
  // Retrieve the 2D array of normalized view ray vectors formatted as
  // 3 floats per vector (dx,dy,dz).  This method is only valid after the 
  // SetSize() and SetRenderer() methods have been called.
  float *GetPerspectiveViewRays(void);

  // Description:
  // Retrieve the position of the bottom left ray
  float *GetParallelStartPosition(void);

  // Description:
  // Retrieve the distance to the next ray starting point along the X and
  // Y direction.
  float *GetParallelIncrements(void);

protected:
  vtkViewRays(void);
  ~vtkViewRays(void);
  vtkViewRays(const vtkViewRays&);
  void operator=(const vtkViewRays&);

  vtkRenderer     *Renderer;		// Renderer contains a camera
  int             Size[2];		// Number of view rays
  unsigned long   ViewRaysCamMtime;	// Camera modified time
  unsigned long   ViewRaysMTime;	// View rays modified time

  // Parallel camera information
  void            ComputeParallelInfo( int size[2] );
  float           StartPosition[3];	// Position of lower left ray
  float           Increments[2];	// Distance to move 1 ray over in X,Y

  // Perspective camera information
  void            ComputePerspectiveInfo(float *vr_ptr,int size[2]);
  float           *ViewRays;
  };
#endif



