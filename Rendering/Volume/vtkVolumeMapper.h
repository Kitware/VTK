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
  void PrintSelf( ostream& os, vtkIndent indent );

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
   * Set/Get the blend mode. Currently this is only supported
   * by the vtkFixedPointVolumeRayCastMapper - other mappers
   * have different ways to set this (supplying a function
   * to a vtkVolumeRayCastMapper) or don't have any options
   * (vtkVolumeTextureMapper2D supports only compositing).
   * Additive blend mode adds scalars along the ray and multiply them by
   * their opacity mapping value.
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
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *) {}

  enum
  {
    COMPOSITE_BLEND,
    MAXIMUM_INTENSITY_BLEND,
    MINIMUM_INTENSITY_BLEND,
    AVERAGE_INTENSITY_BLEND,
    ADDITIVE_BLEND
  };

protected:
  vtkVolumeMapper();
  ~vtkVolumeMapper();

  // Compute a sample distance from the data spacing. When the number of
  // voxels is 8, the sample distance will be roughly 1/200 the average voxel
  // size. The distance will grow proportionally to numVoxels^(1/3).
  double SpacingAdjustedSampleDistance(double inputSpacing[3],
    int inputExtent[6]);

  int   BlendMode;

  // Threshold range for average intensity projection
  double AverageIPScalarRange[2];

  // Cropping variables, and a method for converting the world
  // coordinate cropping region planes to voxel coordinates
  int                  Cropping;
  double               CroppingRegionPlanes[6];
  double               VoxelCroppingRegionPlanes[6];
  int                  CroppingRegionFlags;
  void ConvertCroppingRegionPlanesToVoxels();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkVolumeMapper(const vtkVolumeMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeMapper&) VTK_DELETE_FUNCTION;
};


#endif


