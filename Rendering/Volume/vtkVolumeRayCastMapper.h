/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkVolumeRayCastMapper - A slow but accurate mapper for rendering volumes
// .SECTION Description
// This is a software ray caster for rendering volumes in vtkImageData.

// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkVolumeRayCastMapper_h
#define __vtkVolumeRayCastMapper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeMapper.h"
#include "vtkVolumeRayCastFunction.h" // For vtkVolumeRayCastStaticInfo
                                      // and vtkVolumeRayCastDynamicInfo
#include "vtkFastNumericConversion.h" // for fast rounding and floor

class vtkEncodedGradientEstimator;
class vtkEncodedGradientShader;
class vtkMatrix4x4;
class vtkMultiThreader;
class vtkPlaneCollection;
class vtkRenderer;
class vtkTimerLog;
class vtkVolume;
class vtkVolumeRayCastFunction;
class vtkVolumeTransform;
class vtkTransform;
class vtkRayCastImageDisplayHelper;

//BTX
// Macro for floor of x

inline int vtkFloorFuncMacro(double x)
{
  return vtkFastNumericConversion::QuickFloor(x);
}


// Macro for rounding x (for x >= 0)
inline int vtkRoundFuncMacro(double x)
{
  return vtkFastNumericConversion::Round(x);
}
//ETX

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

// Forward declaration needed for use by friend declaration below.
VTK_THREAD_RETURN_TYPE VolumeRayCastMapper_CastRays( void *arg );

class VTKRENDERINGVOLUME_EXPORT vtkVolumeRayCastMapper : public vtkVolumeMapper
{
public:
  static vtkVolumeRayCastMapper *New();
  vtkTypeMacro(vtkVolumeRayCastMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/Get the distance between samples.  This variable is only
  // used for sampling ray casting methods.  Methods that compute
  // a ray value by stepping cell-by-cell are not affected by this
  // value.
  vtkSetMacro( SampleDistance, double );
  vtkGetMacro( SampleDistance, double );

  // Description:
  // Get / Set the volume ray cast function. This is used to process
  // values found along the ray to compute a final pixel value.
  virtual void SetVolumeRayCastFunction(vtkVolumeRayCastFunction*);
  vtkGetObjectMacro( VolumeRayCastFunction, vtkVolumeRayCastFunction );

  // Description:
  // Set / Get the gradient estimator used to estimate normals
  virtual void SetGradientEstimator(vtkEncodedGradientEstimator *gradest);
  vtkGetObjectMacro( GradientEstimator, vtkEncodedGradientEstimator );

  // Description:
  // Get the gradient shader.
  vtkGetObjectMacro( GradientShader, vtkEncodedGradientShader );

  // Description:
  // Sampling distance in the XY image dimensions. Default value of 1 meaning
  // 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
  // set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels.
  vtkSetClampMacro( ImageSampleDistance, double, 0.1, 100.0 );
  vtkGetMacro( ImageSampleDistance, double );

  // Description:
  // This is the minimum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MinimumImageSampleDistance, double, 0.1, 100.0 );
  vtkGetMacro( MinimumImageSampleDistance, double );

  // Description:
  // This is the maximum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MaximumImageSampleDistance, double, 0.1, 100.0 );
  vtkGetMacro( MaximumImageSampleDistance, double );

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
  int GetNumberOfThreads();

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
  virtual float GetGradientMagnitudeScale(int)
    {return this->GetGradientMagnitudeScale();};
  virtual float GetGradientMagnitudeBias(int)
    {return this->GetGradientMagnitudeBias();};

//ETX

protected:
  vtkVolumeRayCastMapper();
  ~vtkVolumeRayCastMapper();

  vtkVolumeRayCastFunction     *VolumeRayCastFunction;
  vtkEncodedGradientEstimator  *GradientEstimator;
  vtkEncodedGradientShader     *GradientShader;
  vtkRayCastImageDisplayHelper *ImageDisplayHelper;

  virtual void ReportReferences(vtkGarbageCollector*);

  // The distance between sample points along the ray
  double                       SampleDistance;
  double                       ImageSampleDistance;
  double                       MinimumImageSampleDistance;
  double                       MaximumImageSampleDistance;
  int                          AutoAdjustSampleDistances;

  double                       WorldSampleDistance;
  int                          ScalarDataType;
  void                         *ScalarDataPointer;

  void                         UpdateShadingTables( vtkRenderer *ren,
                                                    vtkVolume *vol );

  void ComputeMatrices( vtkImageData *data, vtkVolume *vol );
  int ComputeRowBounds( vtkVolume *vol, vtkRenderer *ren );

  friend VTK_THREAD_RETURN_TYPE VolumeRayCastMapper_CastRays( void *arg );

  vtkMultiThreader  *Threader;

  vtkMatrix4x4 *PerspectiveMatrix;
  vtkMatrix4x4 *ViewToWorldMatrix;
  vtkMatrix4x4 *ViewToVoxelsMatrix;
  vtkMatrix4x4 *VoxelsToViewMatrix;
  vtkMatrix4x4 *WorldToVoxelsMatrix;
  vtkMatrix4x4 *VoxelsToWorldMatrix;

  vtkMatrix4x4 *VolumeMatrix;

  vtkTransform *PerspectiveTransform;
  vtkTransform *VoxelsTransform;
  vtkTransform *VoxelsToViewTransform;

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

  int           ClipRayAgainstVolume( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                                      float bounds[6] );

  void          InitializeClippingPlanes( vtkVolumeRayCastStaticInfo *staticInfo,
                                          vtkPlaneCollection *planes );

  int           ClipRayAgainstClippingPlanes( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                                              vtkVolumeRayCastStaticInfo *staticInfo);

  // Get the ZBuffer value corresponding to location (x,y) where (x,y)
  // are indexing into the ImageInUse image. This must be converted to
  // the zbuffer image coordinates. Nearest neighbor value is returned.
  double         GetZBufferValue( int x, int y );

private:
  vtkVolumeRayCastMapper(const vtkVolumeRayCastMapper&);  // Not implemented.
  void operator=(const vtkVolumeRayCastMapper&);  // Not implemented.
};

#endif

