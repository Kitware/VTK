/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCaster.h
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
// .NAME vtkRayCaster - A helper object for renderer that controls ray casting

// .SECTION Description
// vtkRayCaster is an automatically created object within vtkRenderer. It is
// used for ray casting operations. It stores variables such as the view
// rays, and information on multiresolution image rendering which are queried
// by the specific ray casters.

// .SECTION see also
// vtkRenderer vtkViewRays

#ifndef __vtkRayCaster_h
#define __vtkRayCaster_h

#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkViewRays.h"
#include "vtkVolumeMapper.h"
#include "vtkMultiThreader.h"

#define VTK_MAX_VIEW_RAYS_LEVEL 4

class VTK_EXPORT vtkRayCaster : public vtkObject
{
public:
  static vtkRayCaster *New();
  vtkTypeMacro(vtkRayCaster,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method for a vtkVolumeMapper to retrieve the view rays 
  // for a perspective projection
  float *GetPerspectiveViewRays();

  // Description:
  // Get the size in pixels of the view rays for the selected scale index
  void GetViewRaysSize( int size[2] );


  // Description:
  // For a parallel projection, get the starting position of a ray in
  // the lower left hand corder of the viewport.
  float *GetParallelStartPosition( void );

  // Description:
  // For a parallel projection, get the (x,y,z) world increments to move one
  // pixel along the image plane x and the image plane y axes.
  float *GetParallelIncrements( void );

  
  // Description:
  // Set/Get the scale factor for a given level. This is used during multi-
  // resolution interactive rendering
  void SetImageScale(int level, float scale); 
  float GetImageScale(int level); 

  // Description:
  // Get the number of levels of resolution.
  int GetImageScaleCount( void ) { return VTK_MAX_VIEW_RAYS_LEVEL; };

  // Description:
  // During multi-resolution rendering, this indicated the selected level
  // of resolution
  vtkSetClampMacro(SelectedImageScaleIndex, int, 0, VTK_MAX_VIEW_RAYS_LEVEL-1);
  vtkGetMacro( SelectedImageScaleIndex, int );

  // Description:
  // For each level of resolution, set the step size associated with that
  // level. This may be used by the vtkVolumeMapper.
  void SetViewRaysStepSize(int level, float scale); 
  float GetViewRaysStepSize(int level); 

  // Description:
  // Get the value of AutomaticScaleAdjustment. 0 = off, 1 = on
  vtkGetMacro( AutomaticScaleAdjustment, int );
  
  // Description:
  // Turn the automatic scale adjustment on
  void AutomaticScaleAdjustmentOn( void );

  // Description:
  // Turn the automatic scale adjustment off
  void AutomaticScaleAdjustmentOff( void );

  // Description:
  // Set / Get the lower limit for scaling an image. This will define the
  // worst resolution allowed during multiresolution rendering. The default
  // value is 0.15.
  vtkSetClampMacro( AutomaticScaleLowerLimit, float, 0.0, 1.0 );
  vtkGetMacro( AutomaticScaleLowerLimit, float );

  // Description:
  // Set/Get the value of bilinear image zooming.
  vtkSetMacro( BilinearImageZoom, int );
  vtkGetMacro( BilinearImageZoom, int );
  vtkBooleanMacro( BilinearImageZoom, int );

  // Description:
  // Get the total time required for ray casting.
  vtkGetMacro( TotalRenderTime, float );
 
  // Description:
  // Set / Get the number of threads used during ray casting
  vtkSetMacro( NumberOfThreads, int );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Get the number of samples taken during the last image rendered.
  // This is the number of samples for ray cast images only - any
  // samples taken from other types of mapper will be reported in that
  // mapper directly
  int GetNumberOfSamplesTaken();

  int **RowBounds;
  int *RowBoundsSize;
  
  vtkProp **SoftwareProps;
  vtkProp **RayCastProps;

//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // This method allows the ray caster to know about the renderer with which 
  // it is associated
  vtkSetObjectMacro(Renderer,vtkRenderer);
  vtkGetObjectMacro(Renderer,vtkRenderer);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render all prop that require ray casting or that render into
  // an image buffer. Merge the results with the image generated from
  // updating the geometry, and place it on the screen. 
  void Render(vtkRenderer *, int, vtkProp **, int, vtkProp **);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // This method returns the scale that should be applied to the viewport
  // for geometric rendering, and for the image in volume rendering. It 
  // is either explicitly set (if AutomaticScaleAdjustment is off) or
  // is adjusted automatically to get the desired frame rate.
  //
  // Note: IMPORTANT!!!! This should only be called once per render!!!
  //
  float GetViewportScaleFactor( vtkRenderer *ren );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Get the step size that should be used 
  float GetViewportStepSize( );
  

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float *GetCurrentZBuffer()
  {
    if ( this->FirstBlend )
      {
      return NULL;
      }
    else
      {
      return this->ZImage;
      }
  };

//ETX

protected:
  vtkRayCaster();
  ~vtkRayCaster();
  vtkRayCaster(const vtkRayCaster&);
  void operator=(const vtkRayCaster&);

  // Description:
  // Zoom the small image up the full size using nearest neighbor 
  // interpolation
  void NearestNeighborZoom(float *smallImage, float *largeImage,
			   int smallDims[2], int largeDims[2] );

  // Description:
  // Zoom the small image up the full size using bilinear interpolation
  void BilinearZoom(float *smallImage, float *largeImage,
			   int smallDims[2], int largeDims[2] );


  // Description:
  // Rescale the image from the small size to the full size using one of
  // the two interpolation methods above - NearestNeighborZoom or
  // BilinearZoom.
  void RescaleImage( );

  void RenderFrameBufferVolumes( vtkRenderer *ren );
  void InitializeRenderBuffers( vtkRenderer *ren );
  void InitializeRayCasting( vtkRenderer *ren );

//BTX
  void ComputeRowBounds( vtkRenderer *ren, vtkProp *prop, int index );
//ETX

  friend VTK_THREAD_RETURN_TYPE RayCast_RenderImage( void *arg );

  vtkMultiThreader *Threader;

  int              NumberOfThreads;

  int              NumberOfSamplesTaken[VTK_MAX_THREADS];

  vtkRenderer   *Renderer;

  // These are the variables necessary for adjusting the image
  // size during ray casting to achieve a desired update rate
  vtkViewRays   *ViewRays[VTK_MAX_VIEW_RAYS_LEVEL+1];
  float         *SelectedViewRays;
  float         ImageScale[VTK_MAX_VIEW_RAYS_LEVEL+1];
  int           ImageSize[2];
  int           FullImageSize[2];
  int		BilinearImageZoom;
  int           SelectedImageScaleIndex;
  int           StableImageScaleCounter;
  float         PreviousAllocatedTime;
  int           AutomaticScaleAdjustment;
  float         AutomaticScaleLowerLimit;
  float         ImageRenderTime[2];
  float         ViewRaysStepSize[VTK_MAX_VIEW_RAYS_LEVEL];
  float         TotalRenderTime;

  // The working color and depth image
  float         *RGBAImage;
  float         *ZImage;


  // Hang on to a pointer to each volume with a ray cast mapper.
  // We'll also need some information for each of these volumes
  vtkVolume     **RayCastVolumes;
  //BTX
  struct VolumeRayCastVolumeInfoStruct *VolumeInfo;
  //ETX
  int           RayCastPropCount;
  int           SoftwareBufferPropCount;
  vtkTransform  *ViewToWorldTransform;
  float         CameraClippingRange[2];
  float         ViewToWorldMatrix[4][4];
  int           FirstBlend;
  float         CameraInverse22;
  float         CameraInverse23;
  float         CameraInverse32;
  float         CameraInverse33;
  float         CameraPosition[3];
  float         *ParallelStartPosition;
  float         *ParallelIncrements;
  int           ParallelProjection;
  int           NeedBackgroundBlend;
  float         Background[3];
};

#endif


