/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGPUVolumeRayCastMapper
 * @brief   Ray casting performed on the GPU.
 *
 * vtkGPUVolumeRayCastMapper is a volume mapper that performs ray casting on
 * the GPU using fragment programs.
 *
*/

#ifndef vtkGPUVolumeRayCastMapper_h
#define vtkGPUVolumeRayCastMapper_h

#include <vtkRenderingVolumeModule.h> // For export macro

#include "vtkVolumeMapper.h"

class vtkContourValues;
class vtkRenderWindow;
class vtkVolumeProperty;

//class vtkKWAMRVolumeMapper; // friend class.

class VTKRENDERINGVOLUME_EXPORT vtkGPUVolumeRayCastMapper : public vtkVolumeMapper
{
public:
  static vtkGPUVolumeRayCastMapper *New();
  vtkTypeMacro(vtkGPUVolumeRayCastMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

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
   * Compute the sample distance from the data spacing.  When the number of
   * voxels is 8, the sample distance will be roughly 1/200 the average voxel
   * size. The distance will grow proportionally to numVoxels^(1/3). Off by default.
   */
  vtkSetClampMacro( LockSampleDistanceToInputSpacing, int, 0, 1 );
  vtkGetMacro( LockSampleDistanceToInputSpacing, int );
  vtkBooleanMacro( LockSampleDistanceToInputSpacing, int );
  //@}

  //@{
  /**
   * If UseJittering is on, each ray traversal direction will be
   * perturbed slightly using a noise-texture to get rid of wood-grain
   * effect.
   */
  vtkSetClampMacro( UseJittering, int, 0, 1 );
  vtkGetMacro( UseJittering, int );
  vtkBooleanMacro( UseJittering, int );
  //@}

  //@{
  /**
   * If UseDepthPass is on, the mapper will use two passes. In the first
   * pass, an isocontour depth buffer will be utilized as starting point
   * for ray-casting hence eliminating traversal on voxels that are
   * not going to participate in final rendering. UseDepthPass requires
   * reasonable contour values to be set which can be set by calling
   * GetDepthPassContourValues() method and using vtkControurValues API.
   */
  vtkSetClampMacro( UseDepthPass, int, 0, 1 );
  vtkGetMacro( UseDepthPass, int );
  vtkBooleanMacro( UseDepthPass, int );
  //@}

  /**
   * Return handle to contour values container so
   * that values can be set by the application. Contour values
   * will be used only when UseDepthPass is on.
   */
  vtkContourValues* GetDepthPassContourValues();

  //@{
  /**
   * Set/Get the distance between samples used for rendering
   * when AutoAdjustSampleDistances is off, or when this mapper
   * has more than 1 second allocated to it for rendering.
   * Initial value is 1.0.
   */
  vtkSetMacro( SampleDistance, float );
  vtkGetMacro( SampleDistance, float );
  //@}

  //@{
  /**
   * Sampling distance in the XY image dimensions. Default value of 1 meaning
   * 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
   * set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels. This value
   * will be adjusted to meet a desired frame rate when AutoAdjustSampleDistances
   * is on.
   */
  vtkSetClampMacro( ImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( ImageSampleDistance, float );
  //@}

  //@{
  /**
   * This is the minimum image sample distance allow when the image
   * sample distance is being automatically adjusted.
   */
  vtkSetClampMacro( MinimumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MinimumImageSampleDistance, float );
  //@}

  //@{
  /**
   * This is the maximum image sample distance allow when the image
   * sample distance is being automatically adjusted.
   */
  vtkSetClampMacro( MaximumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MaximumImageSampleDistance, float );
  //@}


  //@{
  /**
   * Set/Get the window / level applied to the final color.
   * This allows brightness / contrast adjustments on the
   * final image.
   * window is the width of the window.
   * level is the center of the window.
   * Initial window value is 1.0
   * Initial level value is 0.5
   * window cannot be null but can be negative, this way
   * values will be reversed.
   * |window| can be larger than 1.0
   * level can be any real value.
   */
  vtkSetMacro( FinalColorWindow, float );
  vtkGetMacro( FinalColorWindow, float );
  vtkSetMacro( FinalColorLevel,  float );
  vtkGetMacro( FinalColorLevel,  float );
  //@}

  //@{
  /**
   * Maximum size of the 3D texture in GPU memory.
   * Will default to the size computed from the graphics
   * card. Can be adjusted by the user.
   */
  vtkSetMacro( MaxMemoryInBytes, vtkIdType );
  vtkGetMacro( MaxMemoryInBytes, vtkIdType );
  //@}

  //@{
  /**
   * Maximum fraction of the MaxMemoryInBytes that should
   * be used to hold the texture. Valid values are 0.1 to
   * 1.0.
   */
  vtkSetClampMacro( MaxMemoryFraction, float, 0.1f, 1.0f );
  vtkGetMacro( MaxMemoryFraction, float );
  //@}

  //@{
  /**
   * Tells if the mapper will report intermediate progress.
   * Initial value is true. As the progress works with a GL blocking
   * call (glFinish()), this can be useful for huge dataset but can
   * slow down rendering of small dataset. It should be set to true
   * for big dataset or complex shading and streaming but to false for
   * small datasets.
   */
  vtkSetMacro(ReportProgress,bool);
  vtkGetMacro(ReportProgress,bool);
  //@}

  /**
   * Based on hardware and properties, we may or may not be able to
   * render using 3D texture mapping. This indicates if 3D texture
   * mapping is supported by the hardware, and if the other extensions
   * necessary to support the specific properties are available.
   */
  virtual int IsRenderSupported(vtkRenderWindow *vtkNotUsed(window),
                                vtkVolumeProperty *vtkNotUsed(property))
  {
      return 0;
  }

  void CreateCanonicalView( vtkRenderer *ren,
                            vtkVolume *volume,
                            vtkImageData *image,
                            int blend_mode,
                            double viewDirection[3],
                            double viewUp[3] );

  //@{
  /**
   * Optionally, set a mask input. This mask may be a binary mask or a label
   * map. This must be specified via SetMaskType.

   * If the mask is a binary mask, the volume rendering is confined to regions
   * within the binary mask. The binary mask is assumed to have a datatype of
   * UCHAR and values of 255 (inside) and 0 (outside).

   * The mask may also be a label map. The label map is allowed to contain only
   * 3 labels (values of 0, 1 and 2) and must have a datatype of UCHAR. In voxels
   * with label value of 0, the color transfer function supplied by component
   * 0 is used.
   * In voxels with label value of 1, the color transfer function supplied by
   * component 1 is used and blended with the transfer function supplied by
   * component 0, with the blending weight being determined by
   * MaskBlendFactor.
   * In voxels with a label value of 2, the color transfer function supplied
   * by component 2 is used and blended with the transfer function supplied by
   * component 0, with the blending weight being determined by
   * MaskBlendFactor.
   */
  void SetMaskInput(vtkImageData *mask);
  vtkGetObjectMacro(MaskInput, vtkImageData);
  //@}

  enum { BinaryMaskType = 0, LabelMapMaskType };

  //@{
  /**
   * Set the mask type, if mask is to be used. See documentation for
   * SetMaskInput(). The default is a LabelMapMaskType.
   */
  vtkSetMacro( MaskType, int );
  vtkGetMacro( MaskType, int );
  void SetMaskTypeToBinary();
  void SetMaskTypeToLabelMap();
  //@}

  //@{
  /**
   * Tells how much mask color transfer function is used compared to the
   * standard color transfer function when the mask is true. This is relevant
   * only for the label map mask.
   * 0.0 means only standard color transfer function.
   * 1.0 means only mask color transfer function.
   * The default value is 1.0.
   */
  vtkSetClampMacro(MaskBlendFactor,float,0.0f,1.0f);
  vtkGetMacro(MaskBlendFactor,float);
  //@}

  //@{
  /**
   * Enable or disable setting output of volume rendering to be
   * color and depth textures. By default this is set to 0 (off).
   * It should be noted that it is possible that underlying API specific
   * mapper may not supoport RenderToImage mode.
   * \warning
   * \li This method ignores any other volumes / props in the scene.
   * \li This method does not respect the general attributes of the
   * scene i.e. background color, etc. It always produces a color
   * image that has a transparent white background outside the
   * bounds of the volume.

   * \sa GetDepthImage(), GetColorImage()
   */
  vtkSetMacro(RenderToImage, int);
  vtkGetMacro(RenderToImage, int);
  vtkBooleanMacro(RenderToImage, int);
  //@}

  //@{
  /**
   * Set/Get the scalar type of the depth texture in RenderToImage mode.
   * By default, the type if VTK_FLOAT.
   * \sa SetRenderToImage()
   */
  vtkSetMacro(DepthImageScalarType, int);
  vtkGetMacro(DepthImageScalarType, int);
  void SetDepthImageScalarTypeToUnsignedChar();
  void SetDepthImageScalarTypeToUnsignedShort();
  void SetDepthImageScalarTypeToFloat();
  //@}

  //@{
  /**
   * Enable or disable clamping the depth value of the fully
   * transparent voxel to the depth of the back-face of the
   * volume. This parameter is used when RenderToImage mode is
   * enabled. When ClampDepthToBackFace is false, the fully transparent
   * voxels will have a value of 1.0 in the depth image. When
   * this is true, the fully transparent voxels will have the
   * depth value of the face at which the ray exits the volume.
   * By default, this is set to 0 (off).
   * \sa SetRenderToImage(), GetDepthImage()
   */
  vtkSetMacro(ClampDepthToBackface, int);
  vtkGetMacro(ClampDepthToBackface, int);
  vtkBooleanMacro(ClampDepthToBackface, int);
  //@}

  /**
   * Low level API to export the depth texture as vtkImageData in
   * RenderToImage mode.
   * Should be implemented by the graphics API specific mapper (GL or other).
   * \sa SetRenderToImage()
   */
  virtual void GetDepthImage(vtkImageData*) {};

  /**
   * Low level API to export the color texture as vtkImageData in
   * RenderToImage mode.
   * Should be implemented by the graphics API specific mapper (GL or other).
   * \sa SetRenderToImage()
   */
  virtual void GetColorImage(vtkImageData*) {};

  /**
   * Initialize rendering for this volume.
   * \warning INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   */
  void Render( vtkRenderer *, vtkVolume * );

  /**
   * Handled in the subclass - the actual render method
   * \pre input is up-to-date.
   */
  virtual void GPURender( vtkRenderer *, vtkVolume *) {}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   * \warning INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   */
  void ReleaseGraphicsResources(vtkWindow *) {}

  /**
   * Return how much the dataset has to be reduced in each dimension to
   * fit on the GPU. If the value is 1.0, there is no need to reduce the
   * dataset.
   * \pre the calling thread has a current OpenGL context.
   * \pre mapper_supported: IsRenderSupported(renderer->GetRenderWindow(),0)
   * The computation is based on hardware limits (3D texture indexable size)
   * and MaxMemoryInBytes.
   * \post valid_i_ratio: ratio[0]>0 && ratio[0]<=1.0
   * \post valid_j_ratio: ratio[1]>0 && ratio[1]<=1.0
   * \post valid_k_ratio: ratio[2]>0 && ratio[2]<=1.0
   */
  virtual void GetReductionRatio(double ratio[3])=0;

protected:
  vtkGPUVolumeRayCastMapper();
  ~vtkGPUVolumeRayCastMapper();

  // Check to see that the render will be OK
  int ValidateRender( vtkRenderer *, vtkVolume * );


  // Special version of render called during the creation
  // of a canonical view.
  void CanonicalViewRender( vtkRenderer *, vtkVolume * );

  // Methods called by the AMR Volume Mapper.
  virtual void PreRender(vtkRenderer *ren,
                         vtkVolume *vol,
                         double datasetBounds[6],
                         double scalarRange[2],
                         int numberOfScalarComponents,
                         unsigned int numberOfLevels)=0;

  // \pre input is up-to-date
  virtual void RenderBlock(vtkRenderer *ren,
                           vtkVolume *vol,
                           unsigned int level)=0;

  virtual void PostRender(vtkRenderer *ren,
                          int numberOfScalarComponents)=0;

  /**
   * Called by the AMR Volume Mapper.
   * Set the flag that tells if the scalars are on point data (0) or
   * cell data (1).
   */
  void SetCellFlag(int cellFlag);

  int LockSampleDistanceToInputSpacing;
  int    AutoAdjustSampleDistances;
  float  ImageSampleDistance;
  float  MinimumImageSampleDistance;
  float  MaximumImageSampleDistance;

  // Render to texture mode flag
  int RenderToImage;

  // Depth image scalar type
  int DepthImageScalarType;

  // Clamp depth values to the depth of the face at which the ray
  // exits the volume
  int ClampDepthToBackface;

  // Enable / disable stochastic jittering
  int UseJittering;

  // Enable / disable two pass rendering
  int UseDepthPass;
  vtkContourValues* DepthPassContourValues;

  // The distance between sample points along the ray
  float  SampleDistance;

  int    SmallVolumeRender;
  double BigTimeToDraw;
  double SmallTimeToDraw;

  float FinalColorWindow;
  float FinalColorLevel;

  // 1 if we are generating the canonical image, 0 otherwise
  int   GeneratingCanonicalView;
  vtkImageData *CanonicalViewImageData;

  //@{
  /**
   * Set the mapper in AMR Mode or not. Initial value is false.
   * Called only by the vtkKWAMRVolumeMapper
   */
  vtkSetClampMacro(AMRMode,int,0,1);
  vtkGetMacro(AMRMode,int);
  vtkBooleanMacro(AMRMode,int);
  //@}

  vtkImageData * MaskInput;
  float          MaskBlendFactor;
  int            MaskType;

  int AMRMode;

  // Point data or cell data (or field data, not handled) ?
  int CellFlag;

  /**
   * Compute the cropping planes clipped by the bounds of the volume.
   * The result is put into this->ClippedCroppingRegionPlanes.
   * NOTE: IT WILL BE MOVED UP TO vtkVolumeMapper after bullet proof usage
   * in this mapper. Other subclasses will use the ClippedCroppingRegionsPlanes
   * members instead of CroppingRegionPlanes.
   * \pre volume_exists: this->GetInput()!=0
   * \pre valid_cropping: this->Cropping &&
   * this->CroppingRegionPlanes[0]<this->CroppingRegionPlanes[1] &&
   * this->CroppingRegionPlanes[2]<this->CroppingRegionPlanes[3] &&
   * this->CroppingRegionPlanes[4]<this->CroppingRegionPlanes[5])
   */
  virtual void ClipCroppingRegionPlanes();

  double         ClippedCroppingRegionPlanes[6];

  vtkIdType MaxMemoryInBytes;
  float MaxMemoryFraction;

  bool           ReportProgress;

  vtkImageData * TransformedInput;

  vtkGetObjectMacro(TransformedInput, vtkImageData);
  void SetTransformedInput(vtkImageData*);

  /**
   * This is needed only to check if the input data has been changed since the last
   * Render() call.
   */
  vtkImageData* LastInput;

private:
  vtkGPUVolumeRayCastMapper(const vtkGPUVolumeRayCastMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGPUVolumeRayCastMapper&) VTK_DELETE_FUNCTION;
};

#endif

