/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeMapper
 * @brief   Abstract class for a volume mapper
 *
 *
 * vtkVolumeMapper is the abstract definition of a volume mapper for regular
 * rectilinear data (vtkImageData).  Several  basic types of volume mappers
 * are supported.
 *
 * @sa
 * vtkVolumeRayCastMapper vtkVolumeTextureMapper2D
*/

#ifndef vtkVolumeMapper_h
#define vtkVolumeMapper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkAbstractVolumeMapper.h"

class vtkRenderer;
class vtkVolume;
class vtkImageData;

#define VTK_CROP_SUBVOLUME              0x0002000
#define VTK_CROP_FENCE                  0x2ebfeba
#define VTK_CROP_INVERTED_FENCE         0x5140145
#define VTK_CROP_CROSS                  0x0417410
#define VTK_CROP_INVERTED_CROSS         0x7be8bef

class vtkWindow;

class VTKRENDERINGVOLUME_EXPORT vtkVolumeMapper : public vtkAbstractVolumeMapper
{
public:
  vtkTypeMacro(vtkVolumeMapper,vtkAbstractVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the input data
   */
  virtual void SetInputData( vtkImageData * );
  virtual void SetInputData( vtkDataSet * );
  vtkImageData *GetInput();
  //@}

  //@{
  /**
   * Set/Get the blend mode.
   * The default mode is Composite where the scalar values are sampled through
   * the volume and composited in a front-to-back scheme through alpha blending.
   * The final color and opacity is determined using the color and opacity
   * transfer functions.
   *
   * Maximum and minimum intensity blend modes use the maximum and minimum
   * scalar values, respectively,  along the sampling ray. The final color and
   * opacity is determined by passing the resultant value through the color and
   * opacity transfer functions.
   *
   * Additive blend mode accumulates scalar values by passing each value through
   * the opacity transfer function and then adding up the product of the value
   * and its opacity. In other words, the scalar values are scaled using the
   * opacity transfer function and summed to derive the final color. Note that
   * the resulting image is always grayscale i.e. aggregated values are not
   * passed through the color transfer function. This is because the final
   * value is a derived value and not a real data value along the sampling ray.
   *
   * Average intensity blend mode works similar to the additive blend mode where
   * the scalar values are multiplied by opacity calculated from the opacity
   * transfer function and then added. The additional step here is to
   * divide the sum by the number of samples taken through the volume.
   * One can control the scalar range by setting the AverageIPScalarRange ivar
   * to disregard scalar values, not in the range of interest, from the average
   * computation.
   * As is the case with the additive intensity projection, the final
   * image will always be grayscale i.e. the aggregated values are not
   * passed through the color transfer function. This is because the
   * resultant value is a derived value and not a real data value along
   * the sampling ray.
   *
   * \note vtkVolumeMapper::AVERAGE_INTENSITY_BLEND is only supported by the
   * vtkGPUVolumeRayCastMapper with the OpenGL2 backend.
   * \sa SetAverageIPScalarRange()
   */
  vtkSetMacro( BlendMode, int );
  void SetBlendModeToComposite()
    { this->SetBlendMode( vtkVolumeMapper::COMPOSITE_BLEND ); }
  void SetBlendModeToMaximumIntensity()
    { this->SetBlendMode( vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND ); }
  void SetBlendModeToMinimumIntensity()
    { this->SetBlendMode( vtkVolumeMapper::MINIMUM_INTENSITY_BLEND ); }
  void SetBlendModeToAverageIntensity()
    { this->SetBlendMode( vtkVolumeMapper::AVERAGE_INTENSITY_BLEND ); }
  void SetBlendModeToAdditive()
    { this->SetBlendMode( vtkVolumeMapper::ADDITIVE_BLEND ); }
  vtkGetMacro( BlendMode, int );
  //@}

  //@{
  /**
   * Set/Get the scalar range to be considered for average intensity projection
   * blend mode. Only scalar values between this range will be averaged during
   * ray casting. This can be useful when volume rendering CT datasets where the
   * areas occupied by air would deviate the final rendering. By default, the
   * range is set to (VTK_DOUBLE_MIN, VTK_DOUBLE_MAX).
   * \sa SetBlendModeToAverageIntensity()
   */
  vtkSetVector2Macro(AverageIPScalarRange, double);
  vtkGetVectorMacro(AverageIPScalarRange, double, 2);
  //@}

  //@{
  /**
   * Turn On/Off orthogonal cropping. (Clipping planes are
   * perpendicular to the coordinate axes.)
   */
  vtkSetClampMacro(Cropping,int,0,1);
  vtkGetMacro(Cropping,int);
  vtkBooleanMacro(Cropping,int);
  //@}

  //@{
  /**
   * Set/Get the Cropping Region Planes ( xmin, xmax, ymin, ymax, zmin, zmax )
   * These planes are defined in volume coordinates - spacing and origin are
   * considered.
   */
  vtkSetVector6Macro( CroppingRegionPlanes, double );
  vtkGetVectorMacro(  CroppingRegionPlanes, double, 6 );
  //@}

  //@{
  /**
   * Get the cropping region planes in voxels. Only valid during the
   * rendering process
   */
  vtkGetVectorMacro( VoxelCroppingRegionPlanes, double, 6 );
  //@}

  //@{
  /**
   * Set the flags for the cropping regions. The clipping planes divide the
   * volume into 27 regions - there is one bit for each region. The regions
   * start from the one containing voxel (0,0,0), moving along the x axis
   * fastest, the y axis next, and the z axis slowest. These are represented
   * from the lowest bit to bit number 27 in the integer containing the
   * flags. There are several convenience functions to set some common
   * configurations - subvolume (the default), fence (between any of the
   * clip plane pairs), inverted fence, cross (between any two of the
   * clip plane pairs) and inverted cross.
   */
  vtkSetClampMacro( CroppingRegionFlags, int, 0x0, 0x7ffffff );
  vtkGetMacro( CroppingRegionFlags, int );
  void SetCroppingRegionFlagsToSubVolume()
    {this->SetCroppingRegionFlags( VTK_CROP_SUBVOLUME );};
  void SetCroppingRegionFlagsToFence()
    {this->SetCroppingRegionFlags( VTK_CROP_FENCE );};
  void SetCroppingRegionFlagsToInvertedFence()
    {this->SetCroppingRegionFlags( VTK_CROP_INVERTED_FENCE );};
  void SetCroppingRegionFlagsToCross()
    {this->SetCroppingRegionFlags( VTK_CROP_CROSS );};
  void SetCroppingRegionFlagsToInvertedCross()
    {this->SetCroppingRegionFlags( VTK_CROP_INVERTED_CROSS );};
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(vtkRenderer *ren, vtkVolume *vol) VTK_OVERRIDE =0;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE {}

  /**
   * Blend modes.
   * The default mode is Composite where the scalar values are sampled through
   * the volume and composited in a front-to-back scheme through alpha blending.
   * The final color and opacity is determined using the color and opacity
   * transfer functions.
   *
   * Maximum and minimum intensity blend modes use the maximum and minimum
   * scalar values, respectively,  along the sampling ray. The final color and
   * opacity is determined by passing the resultant value through the color and
   * opacity transfer functions.
   *
   * Additive blend mode accumulates scalar values by passing each value through
   * the opacity transfer function and then adding up the product of the value
   * and its opacity. In other words, the scalar values are scaled using the
   * opacity transfer function and summed to derive the final color. Note that
   * the resulting image is always grayscale i.e. aggregated values are not
   * passed through the color transfer function. This is because the final
   * value is a derived value and not a real data value along the sampling ray.
   *
   * Average intensity blend mode works similar to the additive blend mode where
   * the scalar values are multiplied by opacity calculated from the opacity
   * transfer function and then added. The additional step here is to
   * divide the sum by the number of samples taken through the volume.
   * As is the case with the additive intensity projection, the final
   * image will always be grayscale i.e. the aggregated values are not
   * passed through the color transfer function. This is because the
   * resultant value is a derived value and not a real data value along
   * the sampling ray.
   *
   * \note vtkVolumeMapper::AVERAGE_INTENSITY_BLEND is only supported by the
   * vtkGPUVolumeRayCastMapper with the OpenGL2 backend.
   */
  enum BlendModes
  {
    COMPOSITE_BLEND,
    MAXIMUM_INTENSITY_BLEND,
    MINIMUM_INTENSITY_BLEND,
    AVERAGE_INTENSITY_BLEND,
    ADDITIVE_BLEND
  };

protected:
  vtkVolumeMapper();
  ~vtkVolumeMapper() VTK_OVERRIDE;

  /**
   * Compute a sample distance from the data spacing. When the number of
   * voxels is 8, the sample distance will be roughly 1/200 the average voxel
   * size. The distance will grow proportionally to numVoxels^(1/3).
   */
  double SpacingAdjustedSampleDistance(double inputSpacing[3],
                                       int inputExtent[6]);

  int   BlendMode;

  /**
   * Threshold range for average intensity projection
   */
  double AverageIPScalarRange[2];

  //@{
  /**
   * Cropping variables, and a method for converting the world
   * coordinate cropping region planes to voxel coordinates
   */
  int                  Cropping;
  double               CroppingRegionPlanes[6];
  double               VoxelCroppingRegionPlanes[6];
  int                  CroppingRegionFlags;
  void ConvertCroppingRegionPlanesToVoxels();
  //@}

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

private:
  vtkVolumeMapper(const vtkVolumeMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeMapper&) VTK_DELETE_FUNCTION;
};


#endif


