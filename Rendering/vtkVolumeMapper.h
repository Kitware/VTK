/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeMapper is the abstract definition of a volume mapper.  Several
// basic types of volume mappers are supported. There are ray casters, which
// produce an image that must be merged with geometry, there are hardware
// methods that blend with geometry, and some combinations of these.

// .SECTION see also
// vtkVolumeRayCastMapper

#ifndef __vtkVolumeMapper_h
#define __vtkVolumeMapper_h

#include "vtkAbstractMapper3D.h"
#include "vtkImageData.h"

class vtkRenderer;
class vtkVolume;

#define VTK_CROP_SUBVOLUME              0x0002000
#define VTK_CROP_FENCE                  0x2ebfeba
#define VTK_CROP_INVERTED_FENCE         0x5140145
#define VTK_CROP_CROSS                  0x0417410
#define VTK_CROP_INVERTED_CROSS         0x7be8bef

class vtkWindow;
class vtkImageClip;

class VTK_RENDERING_EXPORT vtkVolumeMapper : public vtkAbstractMapper3D
{
public:
  vtkTypeRevisionMacro(vtkVolumeMapper,vtkAbstractMapper3D);
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Update the volume rendering pipeline by updating the scalar input
  virtual void Update();

  // Description:
  // Set/Get the input data
  void SetInput( vtkImageData * );
  vtkImageData *GetInput();

  // Description:
  // Does the data have independent components, or do some define color 
  // only? If IndependentComponents is On (the default) then each component 
  // will be independently passed through a lookup table to determine RGBA, 
  // shaded, and then combined with the other components using the 
  // ComponentBlendMode. Some volume Mappers can handle 1 to 4 component 
  // unsigned char or unsigned short data (see each mapper header file to
  // determine functionality). If IndependentComponents is Off, then you 
  // must have either 2 or 4 componenet data. For 2 component data, the 
  // first is passed through the first color tranfser function and the 
  // second component is passed through the first opacity transfer function. 
  // Normals will be generated off of the second component. For 4 component 
  // data, the first three will directly represent RGB (no lookup table). 
  // The fourth component will be passed through the first scalar opacity 
  // transfer function for opacity. Normals will be generated from the fourth 
  // component.
  vtkSetClampMacro( IndependentComponents, int, 0, 1 );
  vtkGetMacro( IndependentComponents, int );
  vtkBooleanMacro( IndependentComponents, int );

  // Description:
  // If we have more than 1 independent components, how will the resulting
  // RGBA values be combined?
  //
  // Add:  R = R1 + R2, G = G1 + G2, B = B1 + B2, A = A1 + A2
  //
  // MaxOpacity:  A1 >= A2 then R = R1, G = G1, B = B1, A = A1
  //              A2 >  A1 then R = R2, G = G2, B = B2, A = A2
  //
  vtkSetClampMacro( ComponentBlendMode, int, 
                    vtkVolumeMapper::ComponentBlendAdd,
                    vtkVolumeMapper::ComponentBlendMaxOpacity );
  vtkGetMacro( ComponentBlendMode, int );
  void SetComponentBlendModeToAdd() 
    {this->SetComponentBlendMode(vtkVolumeMapper::ComponentBlendAdd);};
  void SetComponentBlendModeToMaxOpacity() 
    {this->SetComponentBlendMode(vtkVolumeMapper::ComponentBlendMaxOpacity);};
  
      
  
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
  vtkSetVector6Macro( CroppingRegionPlanes, float );
  vtkGetVectorMacro(  CroppingRegionPlanes, float, 6 );

  // Description:
  // Get the cropping region planes in voxels. Only valid during the 
  // rendering process
  vtkGetVectorMacro( VoxelCroppingRegionPlanes, float, 6 );
  
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

  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual float *GetBounds();
  virtual void GetBounds(float bounds[6])
    { this->vtkAbstractMapper3D::GetBounds(bounds); };


//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  virtual float GetGradientMagnitudeScale() {return 1.0;};
  virtual float GetGradientMagnitudeBias()  {return 0.0;};
  virtual float GetGradientMagnitudeScale(int) {return 1.0;};
  virtual float GetGradientMagnitudeBias(int)  {return 0.0;};
  

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
  
  enum ComponentBlendModes {
    ComponentBlendAdd = 0,
    ComponentBlendMaxOpacity
  };
  
//ETX


protected:
  vtkVolumeMapper();
  ~vtkVolumeMapper();

  // Cropping variables, and a method for converting the world
  // coordinate cropping region planes to voxel coordinates
  int                  Cropping;
  float                CroppingRegionPlanes[6];
  float                VoxelCroppingRegionPlanes[6];
  int                  CroppingRegionFlags;
  void ConvertCroppingRegionPlanesToVoxels();
  
  // Flag for independent or dependent components
  int                  IndependentComponents;
  
  // How should we combine the components
  int                  ComponentBlendMode;
  
  vtkTimeStamp         BuildTime;

  // Clipper used on input to ensure it is the right size
  vtkImageClip        *ImageClipper;
  
  
private:
  vtkVolumeMapper(const vtkVolumeMapper&);  // Not implemented.
  void operator=(const vtkVolumeMapper&);  // Not implemented.
};


#endif


