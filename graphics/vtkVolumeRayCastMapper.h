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

// .NAME vtkVolumeRayCastMapper - Abstract class for ray casting mappers
// .SECTION Description
// This is the abstract class for mappers that use volumetric ray casting

// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkVolumeRayCastMapper_h
#define __vtkVolumeRayCastMapper_h

#include "vtkVolumeMapper.h"
#include "vtkMultiThreader.h"
#include "vtkVolumeRayCastFunction.h"
#include "vtkRayBounder.h"
#include "vtkEncodedGradientShader.h"
#include "vtkEncodedGradientEstimator.h"

class vtkRenderer;
class vtkVolume;
class vtkVolumeTransform;


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

  int GetMapperType() { return VTK_RAYCAST_VOLUME_MAPPER; };

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
  // Get / Set the ray bounder. This is used to clip the rays during
  // ray casting.
  vtkSetObjectMacro( RayBounder, vtkRayBounder );
  vtkGetObjectMacro( RayBounder, vtkRayBounder );

  // Description:
  // Get / Set the volume ray cast function. This is used to process
  // values found along the ray to compute a final pixel value.
  vtkSetObjectMacro( VolumeRayCastFunction, vtkVolumeRayCastFunction );
  vtkGetObjectMacro( VolumeRayCastFunction, vtkVolumeRayCastFunction );

  // Description:
  // Initialize rendering for this volume.
  void Render( vtkRenderer *ren, vtkVolume *vol ) {};
//BTX
  void InitializeRender( vtkRenderer *ren, vtkVolume *vol,
			 struct VolumeRayCastVolumeInfoStruct *volumeInfo );
//ETX

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter RenderWindow could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkRenderWindow *);


  // Description:
  // Return the scalar value below which all opacities are zero
  float GetZeroOpacityThreshold( vtkVolume *vol );

//BTX - this is for interal use by the ray cast function, not for 
//      general use
  // Description:
  // Provided for the ray cast function to query the data increments
  vtkGetVectorMacro( DataIncrement, int, 3 );
//ETX

  // Description:
  // Set / Get the gradient estimator used to estimate normals
  void SetGradientEstimator( vtkEncodedGradientEstimator *gradest );
  vtkGetObjectMacro( GradientEstimator, vtkEncodedGradientEstimator );

  // Description:
  // Get the gradient shader.
  vtkGetObjectMacro( GradientShader, vtkEncodedGradientShader );


//BTX
  void CastViewRay( struct VolumeRayCastRayInfoStruct *rayInfo,
		    struct VolumeRayCastVolumeInfoStruct *volumeInfo );
//ETX

protected:

  int                          DataIncrement[3];

  vtkVolumeRayCastFunction     *VolumeRayCastFunction;

  vtkEncodedGradientEstimator  *GradientEstimator;

  vtkEncodedGradientShader     *GradientShader;

  // The distance between sample points along the ray
  float                        SampleDistance;

  float                        WorldSampleDistance;
  int                          ScalarDataType;
  void                         *ScalarDataPointer;
  float                        *DepthRangeBufferPointer;

//BTX
  int                          ClipRayAgainstVolume( struct VolumeRayCastRayInfoStruct *rayInfo );
//ETX
  void                         GeneralImageInitialization( vtkRenderer *ren, 
							   vtkVolume *vol );

  void                         UpdateShadingTables( vtkRenderer *ren, 
						    vtkVolume *vol );

  vtkRayBounder                *RayBounder;

  float                        RayStart[3];
  float                        RayEnd[3];
  int                          RayPixel[2];
  float                        RayColor[4];
  float                        VolumeBounds[6];
  float                        WorldToVolumeMatrix[16];
  float                        ViewToVolumeMatrix[16];
};

#endif

