/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkUnstructuredGridVolumeRayCastMapper
 * @brief   A software mapper for unstructured volumes
 *
 * This is a software ray caster for rendering volumes in vtkUnstructuredGrid.
 *
 * @sa
 * vtkVolumeMapper
*/

#ifndef vtkUnstructuredGridVolumeRayCastMapper_h
#define vtkUnstructuredGridVolumeRayCastMapper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeMapper.h"

class vtkDoubleArray;
class vtkIdList;
class vtkMultiThreader;
class vtkRayCastImageDisplayHelper;
class vtkRenderer;
class vtkTimerLog;
class vtkUnstructuredGridVolumeRayCastFunction;
class vtkUnstructuredGridVolumeRayCastIterator;
class vtkUnstructuredGridVolumeRayIntegrator;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridVolumeRayCastMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  static vtkUnstructuredGridVolumeRayCastMapper *New();
  vtkTypeMacro(vtkUnstructuredGridVolumeRayCastMapper,vtkUnstructuredGridVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  //@{
  /**
   * Sampling distance in the XY image dimensions. Default value of 1 meaning
   * 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
   * set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels.
   */
  vtkSetClampMacro( ImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( ImageSampleDistance, float );
  //@}

  //@{
  /**
   * This is the minimum image sample distance allow when the image
   * sample distance is being automatically adjusted
   */
  vtkSetClampMacro( MinimumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MinimumImageSampleDistance, float );
  //@}

  //@{
  /**
   * This is the maximum image sample distance allow when the image
   * sample distance is being automatically adjusted
   */
  vtkSetClampMacro( MaximumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MaximumImageSampleDistance, float );
  //@}

  //@{
  /**
   * If AutoAdjustSampleDistances is on, the the ImageSampleDistance
   * will be varied to achieve the allocated render time of this
   * prop (controlled by the desired update rate and any culling in
   * use).
   */
  vtkSetClampMacro( AutoAdjustSampleDistances, int, 0, 1 );
  vtkGetMacro( AutoAdjustSampleDistances, int );
  vtkBooleanMacro( AutoAdjustSampleDistances, int );
  //@}

  //@{
  /**
   * Set/Get the number of threads to use. This by default is equal to
   * the number of available processors detected.
   */
  vtkSetMacro( NumberOfThreads, int );
  vtkGetMacro( NumberOfThreads, int );
  //@}

  //@{
  /**
   * If IntermixIntersectingGeometry is turned on, the zbuffer will be
   * captured and used to limit the traversal of the rays.
   */
  vtkSetClampMacro( IntermixIntersectingGeometry, int, 0, 1 );
  vtkGetMacro( IntermixIntersectingGeometry, int );
  vtkBooleanMacro( IntermixIntersectingGeometry, int );
  //@}

  //@{
  /**
   * Set/Get the helper class for casting rays.
   */
  virtual void SetRayCastFunction(vtkUnstructuredGridVolumeRayCastFunction *f);
  vtkGetObjectMacro(RayCastFunction, vtkUnstructuredGridVolumeRayCastFunction);
  //@}

  //@{
  /**
   * Set/Get the helper class for integrating rays.  If set to NULL, a
   * default integrator will be assigned.
   */
  virtual void SetRayIntegrator(vtkUnstructuredGridVolumeRayIntegrator *ri);
  vtkGetObjectMacro(RayIntegrator, vtkUnstructuredGridVolumeRayIntegrator);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Initialize rendering for this volume.
   */
  void Render( vtkRenderer *, vtkVolume * ) VTK_OVERRIDE;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  vtkGetVectorMacro( ImageInUseSize, int, 2 );
  vtkGetVectorMacro( ImageOrigin, int, 2 );
  vtkGetVectorMacro( ImageViewportSize, int , 2 );

  void CastRays( int threadID, int threadCount );

protected:
  vtkUnstructuredGridVolumeRayCastMapper();
  ~vtkUnstructuredGridVolumeRayCastMapper() VTK_OVERRIDE;

  float                        ImageSampleDistance;
  float                        MinimumImageSampleDistance;
  float                        MaximumImageSampleDistance;
  int                          AutoAdjustSampleDistances;

  vtkMultiThreader  *Threader;
  int               NumberOfThreads;

  vtkRayCastImageDisplayHelper *ImageDisplayHelper;

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

  // Get the ZBuffer value corresponding to location (x,y) where (x,y)
  // are indexing into the ImageInUse image. This must be converted to
  // the zbuffer image coordinates. Nearest neighbor value is returned.
  double         GetZBufferValue( int x, int y );

  double         GetMinimumBoundsDepth( vtkRenderer *ren,
                                       vtkVolume   *vol );

  vtkUnstructuredGridVolumeRayCastFunction  *RayCastFunction;
  vtkUnstructuredGridVolumeRayCastIterator **RayCastIterators;
  vtkUnstructuredGridVolumeRayIntegrator    *RayIntegrator;
  vtkUnstructuredGridVolumeRayIntegrator    *RealRayIntegrator;

  vtkIdList      **IntersectedCellsBuffer;
  vtkDoubleArray **IntersectionLengthsBuffer;
  vtkDataArray   **NearIntersectionsBuffer;
  vtkDataArray   **FarIntersectionsBuffer;

  vtkVolume     *CurrentVolume;
  vtkRenderer   *CurrentRenderer;

  vtkDataArray *Scalars;
  int           CellScalars;

private:
  vtkUnstructuredGridVolumeRayCastMapper(const vtkUnstructuredGridVolumeRayCastMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridVolumeRayCastMapper&) VTK_DELETE_FUNCTION;
};

#endif

