/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewRays.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
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
  vtkViewRays(void);
  ~vtkViewRays(void);
  static vtkViewRays *New() {return new vtkViewRays;};
  char *GetClassName() {return "vtkViewRays";};
  void PrintSelf(ostream& os,vtkIndent indent);

  // Description:
  // Specify the vtkRenderer whose camera will be used to to calculate 
  // the view rays. The type of camera (parallel, perspective) will
  // determine which type of view ray information is calculated.
  vtkGetObjectMacro(Renderer,vtkRenderer);
  vtkSetObjectMacro(Renderer,vtkRenderer);

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
  vtkRenderer     *Renderer;		// Renderer contains a camera
  int             Size[2];		// Number of view rays
  unsigned long   ViewRaysCamMtime;	// Camera modified time
  unsigned long   ViewRaysMTime;	// View rays modified time

  // Parallel camera information
  void            CalculateParallelInfo( int size[2] );
  float           StartPosition[3];	// Position of lower left ray
  float           Increments[2];	// Distance to move 1 ray over in X,Y

  // Perspective camera information
  void            CalculatePerspectiveInfo(float *vr_ptr,int size[2]);
  float           *ViewRays;
  };
#endif



