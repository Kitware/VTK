/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMapper.h
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
class vtkPlaneCollection;

// Macro for floor of x
#define vtkFloorFuncMacro(x)   (((x) < 0.0)?((int)((x)-1.0)):((int)(x)))

// Macro for rounding x
#define vtkRoundFuncMacro(x)   (int)((x)+0.5)

// Macro for tri-linear interpolation - do four linear interpolations on
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
  static vtkVolumeRayCastMapper *New();
  vtkTypeMacro(vtkVolumeRayCastMapper,vtkVolumeMapper);
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
  // Set / Get the gradient estimator used to estimate normals
  void SetGradientEstimator( vtkEncodedGradientEstimator *gradest );
  vtkGetObjectMacro( GradientEstimator, vtkEncodedGradientEstimator );

  // Description:
  // Get the gradient shader.
  vtkGetObjectMacro( GradientShader, vtkEncodedGradientShader );

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Initialize rendering for this volume.
  void Render( vtkRenderer *, vtkVolume * ) {};

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  void InitializeRender( vtkRenderer *ren, vtkVolume *vol,
			 VTKRayCastVolumeInfo *volumeInfo );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Return the scalar value below which all opacities are zero
  float GetZeroOpacityThreshold( vtkVolume *vol );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  void CastViewRay( VTKRayCastRayInfo *rayInfo,
		    VTKRayCastVolumeInfo *volumeInfo );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  int GetMapperType() { return VTK_RAYCAST_VOLUME_MAPPER; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // This is a ray cast mapper.
  virtual int IsARayCastMapper() {return 1;};

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Values needed by the volume
  virtual float GetGradientMagnitudeScale();
  virtual float GetGradientMagnitudeBias();
  
//ETX

protected:
  vtkVolumeRayCastMapper();
  ~vtkVolumeRayCastMapper();
  vtkVolumeRayCastMapper(const vtkVolumeRayCastMapper&);
  void operator=(const vtkVolumeRayCastMapper&);

  vtkVolumeRayCastFunction     *VolumeRayCastFunction;
  vtkEncodedGradientEstimator  *GradientEstimator;
  vtkEncodedGradientShader     *GradientShader;

  // The distance between sample points along the ray
  float                        SampleDistance;

  float                        WorldSampleDistance;
  int                          ScalarDataType;
  void                         *ScalarDataPointer;
  float                        *DepthRangeBufferPointer;

  int                          ClipRayAgainstVolume( 
					  VTKRayCastRayInfo *rayInfo,
					  VTKRayCastVolumeInfo *volumeInfo,
					  float bounds[6] );

  int                          ClipRayAgainstClippingPlanes( 
					  VTKRayCastRayInfo *rayInfo,
					  VTKRayCastVolumeInfo *volumeInfo,
					  vtkPlaneCollection *planes );

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
  float                        VolumeToWorldMatrix[16];
  float                        ViewToVolumeMatrix[16];
};

#endif

