/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

// .NAME vtkVolumeRayCastMapper - Abstract class for mappers that use Depth 
// Buffer PARC
// .SECTION Description
// This is the abstract class for mappers that use Depth Buffer PARC for
// acceleration. Some common methods such as the distance between samples
// and the grouping of voxels are defined in this class.  
// 

// .SECTION see also
// vtkVolumeMapper vtkMIPDPARCMapper

#ifndef __vtkVolumeRayCastMapper_h
#define __vtkVolumeRayCastMapper_h

#include "vtkVolumeMapper.h"
#include "vtkMultiThreader.h"
#include "vtkNormalEncoder.h"
#include "vtkTransform.h"
#include "vtkRayBounder.h"

class vtkVolumeRayCastFunction;
class vtkRenderer;
class vtkVolume;

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

class VTK_EXPORT vtkVolumeRayCastMapper : public vtkVolumeMapper
{
public:
  vtkVolumeRayCastMapper();
  ~vtkVolumeRayCastMapper();
  static vtkVolumeRayCastMapper *New() {return new vtkVolumeRayCastMapper;};
  const char *GetClassName() {return "vtkVolumeRayCastMapper";};
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

  vtkSetObjectMacro( VolumeRayCastFunction, vtkVolumeRayCastFunction );
  vtkGetObjectMacro( VolumeRayCastFunction, vtkVolumeRayCastFunction );


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
  float GetZeroOpacityThreshold( vtkVolume *vol );

  vtkGetVectorMacro( DataIncrement, int, 3 );

  vtkNormalEncoder *GetNormalEncoder(void) { return &(this->NormalEncoder); }

protected:
  // The RGBA and Z (depth) images that are computed during a Render call.
  float                      *RGBAImage;
  float                      *ZImage;

  int                        DataIncrement[3];

  vtkVolumeRayCastFunction   *VolumeRayCastFunction;

  // The normal encoder for creating/storing gradients and 
  // gradient magnitudes
  vtkNormalEncoder            NormalEncoder;

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


  // The rgb transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the rgb transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                       *RGBTFArray;
  vtkTimeStamp                RGBTFArrayMTime;

  // The gray transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the gray transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                       *GrayTFArray;
  vtkTimeStamp                GrayTFArrayMTime;

  // The opacity transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a samples at each scalar value of the opacity transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                       *OpacityTFArray;
  vtkTimeStamp                OpacityTFArrayMTime;

  // The corrected opacity transfer function array - this is identical
  // to the opacity transfer function array when the step size is 1.
  // In other cases, it is corrected to reflect the new material thickness
  // modelled by a step size different than 1.
  float                       *CorrectedOpacityTFArray;

  // CorrectedStepSize is the step size corrently modelled by
  // CorrectedTFArray.  It is used to determine when the 
  // CorrectedTFArray needs to be updated to match SampleDistance
  // in the volume mapper.
  float                       CorrectedStepSize;

  // CorrectedOTFArrayMTime - compared with OpacityTFArrayMTime for update
  vtkTimeStamp                CorrectedOTFArrayMTime;

  // Number of elements in the transfer function arrays
  int                         TFArraySize;

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
  void                       InitializeParallelImage( vtkRenderer *ren ); 

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
  void  InitializePerspectiveImage( vtkRenderer *ren );

  // This is the routine that casts rays for a perspective image - 
  // it may run in several simultaneous threads of execution so it
  // must be reentrant. It is a friend function instead of a member
  // function since it must be of void type to be used as an argument
  // in sproc or pthread_create
  friend VTK_THREAD_RETURN_TYPE RenderPerspectiveImage(void *arg );

  // Description:
  // This is called from the Render method.  Its purpose is to
  // give the concrete depth parc mapper a chance to do any specific
  // updating that it must do. 
  void                       CasterUpdate( vtkRenderer *ren, 
					   vtkVolume *vol );
  void                       UpdateShadingTables( vtkRenderer *ren, 
						  vtkVolume *vol );
  void                       UpdateTransferFunctions( vtkRenderer *ren, 
						      vtkVolume *vol );


  void UpdateOpacityTFforSampleSize(vtkRenderer *ren, vtkVolume *vol);

  vtkMultiThreader           Threader;

  vtkRayBounder              *RayBounder;
};

#endif

