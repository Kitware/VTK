/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCaster.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

// .NAME vtkVolumeRayCaster - Abstract class for mappers that use Depth 
// Buffer PARC
// .SECTION Description
// This is the abstract class for mappers that use Depth Buffer PARC for
// acceleration. Some common methods such as the distance between samples
// and the grouping of voxels are defined in this class.  
// 

// .SECTION see also
// vtkVolumeMapper vtkMIPDPARCMapper

#ifndef __vtkVolumeRayCaster_h
#define __vtkVolumeRayCaster_h

#include "vtkRenderer.h"
#include "vtkVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkTransform.h"
#include "vtkMultiThreader.h"
#include "vtkRayBounder.h"

// Macro for ceiling of x
#define vtkCeilingFuncMacro(x) (((x) == (int)(x))?((int)(x)):((int)(x+1.0)))

// Macro for floor of x
#define vtkFloorFuncMacro(x)   (((x) < 0.0)?((int)((x)-1.0)):((int)(x)))

// Macro for rounding x
#define vtkRoundFuncMacro(x)   (int)((x)+0.5)

// Macro for trilinear interpolation - do four linear interpolations on
// edges, two linear interpolations between pairs of edges, then a final
// interpolation between faces
#define vtkTrilinFuncMacro(v,x,y,z,a,b,c,d,e,f,g,h)         \
        t00 =   a + (x)*(b-a);      \
        t01 =   c + (x)*(d-c);      \
        t10 =   e + (x)*(f-e);      \
        t11 =   g + (x)*(h-g);      \
        t0  = t00 + (y)*(t01-t00);  \
        t1  = t10 + (y)*(t11-t10);  \
        v   =  t0 + (z)*(t1-t0);

class VTK_EXPORT vtkVolumeRayCaster : public vtkVolumeMapper
{
public:
  vtkVolumeRayCaster();
  ~vtkVolumeRayCaster();
  const char *GetClassName() {return "vtkVolumeRayCaster";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Set/Get the distance between samples.  This variable is only
  // used for sampling ray casting methods.  Methods that compute
  // a ray value by stepping cell-by-cell are not affected by this
  // value.  Note: this distance is in volume coordinates.  This 
  // means that if you set the scale to 4 in the vtkVolume, you
  // will NOT have 4 times as many samples.
  vtkSetMacro( SampleDistance, float );
  vtkGetMacro( SampleDistance, float );

  // Description:
  // Get the size of the volume in voxels
  vtkGetVectorMacro( ScalarDataSize, int, 3 );

  // Description:
  // Set/Get the interpolation type.  GetInterpolationType returns one
  // of the following strings: "NearestNeighbor", "Trilinear".
  void SetInterpolationTypeToNearestNeighbor(void) 
    {this->InterpolationType = 0;};
  void SetInterpolationTypeToTrilinear(void)
    {this->InterpolationType = 1;};
  char *GetInterpolationType( void );

  // Description:
  // Get the total steps taken during the computation of the last image.
  // For a ray stepping algorithm, this is the number of steps taken along
  // the ray.  For a cell-by-cell stepping algorithm, this is the number
  // of cells considered along the ray.
  vtkGetMacro( TotalStepsTaken, int );

  // Description:
  // Get the total number of rays cast during the computation of the
  // last image.  Rays which do not intersect the volume, or have been
  // determined to have zero contribution to the image according to the
  // Parc approximation are not counted.
  vtkGetMacro( TotalRaysCast, int );

  // Description:
  // Get the time spent drawing the Parc approximation for the last image.
  // This time, plus the casting time is the total time required to compute
  // the image for this volume.
  vtkGetMacro( DrawTime, float );
 
  // Description:
  // Get the time spent casting rays for the last image.  This is the 
  // software portion of the algorithm, and the draw time measures the 
  // hardware part of the method.
  vtkGetMacro( CastTime, float );

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( ThreadCount, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( ThreadCount, int );

  vtkSetObjectMacro( RayBounder, vtkRayBounder );
  vtkGetObjectMacro( RayBounder, vtkRayBounder );


  // Description:
  // Get a pointer to the depth or RGBA buffer created during the previous
  // render.  These methods are used by the vtkVolumeRenderer
  float *GetZbufferData( void )   { return this->ZImage; };
  float *GetRGBAPixelData( void ) { return this->RGBAImage; };

  // Description:
  // Required methods for volume mappers.  The Parc algorithm destroys
  // whatever is in the hardware buffer, and returns the image in
  // software (through the GetZBufferData and GetRGBAPixelData calls)
  int DestroyHardwareBuffer( void ) { return 1; };
  int ImageLocatedInHardware( void ) { return 0; };

  // Description:
  // Render this volume.
  virtual void Render( vtkRenderer *ren, vtkVolume *vol );

  // Description:
  // Return the scalar value below which all opacities are zero
  virtual float GetZeroOpacityThreshold( void ) = 0;

  // These variables should be protected but are being
  // made public to be accessible to the templated function.
  // We used to have the templated function as a friend, but
  // this does not work with all compilers

  int                        DataIncrement[3];

  // The interpolation type (nearest neighbor = 0 or trilinear = 1)
  int                        InterpolationType;

protected:
  // The RGBA and Z (depth) images that are computed during a Render call.
  float                      *RGBAImage;
  float                      *ZImage;

  // The distance between sample points along the ray
  float                      SampleDistance;

  // The number of threads to use when ray casting
  int                        ThreadCount;

  // The view rays, their size, and a transformation that will bring them
  // into the volume's coordinate system. These rays are only valid for 
  // perspective viewing transforms
  float                      *ViewRays;
  int                        ViewRaysSize[2];
  vtkTransform               ViewRaysTransform;

  // Accounting information
  int                        TotalStepsTaken;
  int                        TotalRaysCast;
  float                      DrawTime;
  float                      CastTime;

  // Accounting information for steps taken must first be
  // computed per thread id to ensure mutual exclusion. Then these
  // numbers are added to get the TotalStepsTaken
  int                        TotalStepsTakenPerId[VTK_MAX_THREADS];
  int                        TotalRaysCastPerId[VTK_MAX_THREADS];


  // These are variables used by the initialize routines to pass
  // information to the render routines. The initialize routines are
  // vtkInitializeParallelImage and vtkInitializePerspectiveImage. The
  // render routines are vtkRenderParallelImage and 
  // vtkRenderPerspectiveImage. The initialize routines are single
  // threaded and therefore don't have to worry about only calling
  // reentrant functions.  The render routines are multi-threaded
  // and cannot call non-reentrant code, and cannot modify instance
  // parameters unless sure of mutual exclusion.
  int                        ScalarDataSize[3];
  float                      VolumeScaleFactor;
  float                      LocalRayScale; // delete this
  float                      WorldSampleDistance;
  float                      CameraClippingRange[2];
  float                      LocalRayDirection[3];
  float                      LocalRayOrigin[3];
  float                      LocalUnitRayDirection[3];
  float                      LocalRayStart[3];
  float                      XOriginIncrement[3];
  float                      YOriginIncrement[3];
  float                      *DepthRangeBufferPointer;
  int                        ScalarDataType;
  void                       *ScalarDataPointer;
  float                      ParallelZScale;
  float                      ParallelZBias;
  float                      ZNumerator;
  float                      ZDenomMult;
  float                      ZDenomAdd;
  float                      *RenderZData;

  int                        ClipRayAgainstVolume( float ray_info[12],
						   float bounds_info[12] );

  void                       GeneralImageInitialization( vtkRenderer *ren, 
							 vtkVolume *vol );

  // Initialize parallel (orthographic) rendering
  // This includes building the transformation matrix from camera to
  // volume space, and setting up all the info needed from other
  // objects. This leaves the RenderParallelImage code reentrant since
  // it relies only on values stored in the depth parc mapper itself.
  void                       InitializeParallelImage( vtkRenderer *ren, 
						      vtkVolume *vol );

  // This is the routine that casts rays for a parallel image - 
  // it may run in several simultaneous threads of execution so it
  // must be reentrant. It is a friend function instead of a member
  // function since it must be of void type to be used as an argument
  // in sproc or pthread_create
  friend VTK_THREAD_RETURN_TYPE  RenderParallelImage(void *arg);
  
  // Initialize perspective rendering
  // This includes building the transformation matrix from camera to
  // volume space, and setting up all the info needed from other
  // objects. This leaves the RenderPerspectiveImage code reentrant since
  // it relies only on values stored in the depth parc mapper itself.
  void  InitializePerspectiveImage( vtkRenderer *ren, vtkVolume *vol );

  // This is the routine that casts rays for a perspective image - 
  // it may run in several simultaneous threads of execution so it
  // must be reentrant. It is a friend function instead of a member
  // function since it must be of void type to be used as an argument
  // in sproc or pthread_create
  friend VTK_THREAD_RETURN_TYPE RenderPerspectiveImage(void *arg );

  // Description:
  // Give a ray type (0 = unsigned char, 1 = unsigned short,
  // 2 = short) cast a ray through the scalar data starting
  // at ray_position and taking num_steps of ray_increment size.
  // Return the final compositing value in pixel_value where
  // pixel_value[0] = red, pixel_value[1] = green, 
  // pixel_value[2] = blue, pixel_value[3] = alpha,
  // pixel_value[4] = depth and pixel_value[5] = number of samples
  // This method is in the specific volume mapper class since it must
  // call a templated function that is only defined in that class
  virtual void               CastARay( int ray_type, void *data_ptr,
				       float ray_position[3], 
				       float ray_increment[3],
				       int num_steps, 
				       float pixel_value[6] ) = 0;

  // Description:
  // This is called from the Render method.  Its purpose is to
  // give the concrete depth parc mapper a chance to do any specific
  // updating that it must do. 
  virtual void               CasterUpdate( vtkRenderer *ren, 
					   vtkVolume *vol ) = 0;

  vtkMultiThreader           Threader;

  vtkRayBounder              *RayBounder;
};

// Description:
// Return the correct interpolation type string based on the
// InterpolationType variable value.
inline char *vtkVolumeRayCaster::GetInterpolationType( void )
{
  if ( this->InterpolationType == 0 )
    return "NearestNeighbor";
  else if ( this->InterpolationType == 1 )
    return "Trilinear";

  return NULL;
}
#endif

