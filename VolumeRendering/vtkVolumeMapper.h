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
// .NAME vtkVolumeMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeMapper is the abstract definition of a volume mapper for regular
// rectilinear data (vtkImageData).  Several  basic types of volume mappers 
// are supported. 

// .SECTION see also
// vtkVolumeRayCastMapper vtkVolumeTextureMapper2D

#ifndef __vtkVolumeMapper_h
#define __vtkVolumeMapper_h

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

class VTK_VOLUMERENDERING_EXPORT vtkVolumeMapper : public vtkAbstractVolumeMapper
{
public:
  vtkTypeMacro(vtkVolumeMapper,vtkAbstractVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/Get the input data
  virtual void SetInput( vtkImageData * );
  virtual void SetInput( vtkDataSet * );
  vtkImageData *GetInput();

  // Description:
  // Set/Get the blend mode. Currently this is only supported
  // by the vtkFixedPointVolumeRayCastMapper - other mappers
  // have different ways to set this (supplying a function
  // to a vtkVolumeRayCastMapper) or don't have any options
  // (vtkVolumeTextureMapper2D supports only compositing).
  // Additive blend mode adds scalars along the ray and multiply them by
  // their opacity mapping value.
  vtkSetMacro( BlendMode, int );
  void SetBlendModeToComposite()
    { this->SetBlendMode( vtkVolumeMapper::COMPOSITE_BLEND ); }
  void SetBlendModeToMaximumIntensity()
    { this->SetBlendMode( vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND ); }
  void SetBlendModeToMinimumIntensity()
    { this->SetBlendMode( vtkVolumeMapper::MINIMUM_INTENSITY_BLEND ); }
  void SetBlendModeToAdditive()
    { this->SetBlendMode( vtkVolumeMapper::ADDITIVE_BLEND ); }
  vtkGetMacro( BlendMode, int );
  
  // Description:
  // Turn On/Off orthogonal cropping. (Clipping planes are
  // perpendicular to the coordinate axes.)
  vtkSetClampMacro(Cropping,int,0,1);
  vtkGetMacro(Cropping,int);
  vtkBooleanMacro(Cropping,int);

  // Description:
  // Set/Get the Cropping Region Planes ( xmin, xmax, ymin, ymax, zmin, zmax )
  // These planes are defined in volume coordinates - spacing and origin are
  // considered.
  vtkSetVector6Macro( CroppingRegionPlanes, double );
  vtkGetVectorMacro(  CroppingRegionPlanes, double, 6 );

  // Description:
  // Get the cropping region planes in voxels. Only valid during the 
  // rendering process
  vtkGetVectorMacro( VoxelCroppingRegionPlanes, double, 6 );
  
  // Description:
  // Set the flags for the cropping regions. The clipping planes divide the
  // volume into 27 regions - there is one bit for each region. The regions 
  // start from the one containing voxel (0,0,0), moving along the x axis 
  // fastest, the y axis next, and the z axis slowest. These are represented 
  // from the lowest bit to bit number 27 in the integer containing the 
  // flags. There are several convenience functions to set some common 
  // configurations - subvolume (the default), fence (between any of the 
  // clip plane pairs), inverted fence, cross (between any two of the 
  // clip plane pairs) and inverted cross.
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

//BTX  

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};
  
  enum 
  {
    COMPOSITE_BLEND,
    MAXIMUM_INTENSITY_BLEND,
    MINIMUM_INTENSITY_BLEND,
    ADDITIVE_BLEND
  };
//ETX

protected:
  vtkVolumeMapper();
  ~vtkVolumeMapper();

  int   BlendMode;

  // Cropping variables, and a method for converting the world
  // coordinate cropping region planes to voxel coordinates
  int                  Cropping;
  double               CroppingRegionPlanes[6];
  double               VoxelCroppingRegionPlanes[6];
  int                  CroppingRegionFlags;
  void ConvertCroppingRegionPlanesToVoxels();
  
  virtual int FillInputPortInformation(int, vtkInformation*);
  
private:
  vtkVolumeMapper(const vtkVolumeMapper&);  // Not implemented.
  void operator=(const vtkVolumeMapper&);  // Not implemented.
};


#endif


