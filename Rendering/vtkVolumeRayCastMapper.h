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

// .NAME vtkVolumeRayCastMapper - A slow but accurate mapper for rendering volumes
// .SECTION Description
// This is a software ray caster for rendering volumes in vtkImageData. 

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

class  vtkRenderer;
class  vtkVolume;
class  vtkVolumeTransform;
class  vtkPlaneCollection;
class  vtkPoints;
class  vtkCellArray;
class  vtkTCoords;
class  vtkPolyData;
class  vtkTexture;
class  vtkActor;
class  vtkPolyDataMapper;
class  vtkMultiThreader;
class  vtkTimerLog;

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
  // value. 
  vtkSetMacro( SampleDistance, float );
  vtkGetMacro( SampleDistance, float );

  // Description:
  // Get / Set the ray bounder. This is used to clip the rays during
  // ray casting.
  void SetRayBounder( vtkRayBounder *bounder )
    {
      VTK_LEGACY_METHOD(SetRayBounder,"4.0");
      if (bounder!=this->RayBounder)
        {
        if ( this->RayBounder )
          {
          this->RayBounder->UnRegister(this);
          }
        this->RayBounder = bounder;
        bounder->Register(this);
        }
    };
  
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

  // Description:
  // Sampling distance in the XY image dimensions. Default value of 1 meaning
  // 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
  // set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels.
  vtkSetClampMacro( ImageSampleDistance, float, 0.1, 100.0 );
  vtkGetMacro( ImageSampleDistance, float );

  // Description:
  // This is the minimum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MinimumImageSampleDistance, float, 0.1, 100.0 );
  vtkGetMacro( MinimumImageSampleDistance, float );

  // Description:
  // This is the maximum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MaximumImageSampleDistance, float, 0.1, 100.0 );
  vtkGetMacro( MaximumImageSampleDistance, float );

  // Description:
  // If AutoAdjustSampleDistances is on, the the ImageSampleDistance
  // will be varied to achieve the allocated render time of this 
  // prop (controlled by the desired update rate and any culling in
  // use). 
  vtkSetClampMacro( AutoAdjustSampleDistances, int, 0, 1 );
  vtkGetMacro( AutoAdjustSampleDistances, int );
  vtkBooleanMacro( AutoAdjustSampleDistances, int );
  
  // Description:
  // Set/Get the number of threads to use. This by default is equal to
  // the number of available processors detected.
  void SetNumberOfThreads( int num );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // If IntermixIntersectingGeometry is turned on, the zbuffer will be
  // captured and used to limit the traversal of the rays.
  vtkSetClampMacro( IntermixIntersectingGeometry, int, 0, 1 );
  vtkGetMacro( IntermixIntersectingGeometry, int );
  vtkBooleanMacro( IntermixIntersectingGeometry, int );
  
//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Initialize rendering for this volume.
  void Render( vtkRenderer *, vtkVolume * );

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
  float                        ImageSampleDistance;
  float                        MinimumImageSampleDistance;
  float                        MaximumImageSampleDistance;
  int                          AutoAdjustSampleDistances;
  
  float                        WorldSampleDistance;
  int                          ScalarDataType;
  void                         *ScalarDataPointer;

  void                         UpdateShadingTables( vtkRenderer *ren, 
						    vtkVolume *vol );

  vtkRayBounder                *RayBounder;

  void ComputeMatrices( vtkImageData *data, vtkVolume *vol );
  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren)=0;
  int ComputeRowBounds( vtkVolume *vol, vtkRenderer *ren );
  
  friend VTK_THREAD_RETURN_TYPE VolumeRayCastMapper_CastRays( void *arg );

  vtkMultiThreader  *Threader;
  int               NumberOfThreads;

  vtkMatrix4x4 *PerspectiveMatrix;
  vtkMatrix4x4 *ViewToWorldMatrix;
  vtkMatrix4x4 *ViewToVoxelsMatrix;
  vtkMatrix4x4 *VoxelsToViewMatrix;
  vtkMatrix4x4 *WorldToVoxelsMatrix;
  vtkMatrix4x4 *VoxelsToWorldMatrix;
  
  // This is how big the image would be if it covered the entire viewport
  int            ImageViewportSize[2];
  
  // This is how big the allocated memory for image is. This may be bigger
  // or smaller than ImageFullSize - it will be bigger if necessary to 
  // ensure a power of 2, it will be smaller if the volume only covers a
  // small region of the viewport
  int            ImageMemorySize[2];
  
  // This is the size of subregion in ImageSize image that we are using for
  // the current image. Since ImageSize is a power of 2, there is likely
  // wasted space in it. This number will be used for things such as clearing
  // the image if necessary.
  int            ImageInUseSize[2];
  
  // This is the location in ImageFullSize image where our ImageSize image
  // is located.
  int            ImageOrigin[2];
  
  // This is the allocated image
  unsigned char *Image;
  
  int  *RowBounds;
  int  *OldRowBounds;

  float        *RenderTimeTable;
  vtkVolume   **RenderVolumeTable;
  vtkRenderer **RenderRendererTable;
  int           RenderTableSize;
  int           RenderTableEntries;

  void StoreRenderTime( vtkRenderer *ren, vtkVolume *vol, float t );
  float RetrieveRenderTime( vtkRenderer *ren, vtkVolume *vol );

  int           IntermixIntersectingGeometry;

  float        *ZBuffer;
  int           ZBufferSize[2];
  int           ZBufferOrigin[2];

  float         MinimumViewDistance;
  
  int           ClipRayAgainstVolume( VTKVRCDynamicInfo *dynamicInfo,
                                      float bounds[6] );

  void          InitializeClippingPlanes( VTKVRCStaticInfo *staticInfo,
                                          vtkPlaneCollection *planes );

  int           ClipRayAgainstClippingPlanes( VTKVRCDynamicInfo *dynamicInfo,
                                              VTKVRCStaticInfo *staticInfo);
  
  // Get the ZBuffer value corresponding to location (x,y) where (x,y)
  // are indexing into the ImageInUse image. This must be converted to
  // the zbuffer image coordinates. Nearest neighbor value is returned.
  float         GetZBufferValue( int x, int y );

};

#endif

