/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProMapper.h
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
// .NAME vtkVolumeProMapper - Superclass for VolumePRO volume rendering mappers
//
// .SECTION Description
// vtkVolumeProMapper is the superclass for VolumePRO volume rendering mappers.
// Any functionality that is general across all VolumePRO implementations is
// placed here in this class. Subclasses of this class are for the specific
// board implementations. Subclasses of that are for underlying graphics 
// languages. Users should not create subclasses directly - 
// a vtkVolumeProMapper will automatically create the object of the right 
// type.
//
// If you do not have the VolumePRO libraries when building this object, then
// the New method will create a default renderer that will not render.
// You can check the NumberOfBoards ivar to see if it is a real rendering class.
// To build with the VolumePRO board see vtkVolumeProVG500Mapper.h or
// vtkVolumeProVP1000Mapper.h for instructions.
//
// For more information on the VolumePRO hardware, please see:
//
//   http://www.terarecon.com/3d_products.shtml
//
// If you encounter any problems with this class, please inform Kitware, Inc.
// at kitware@kitware.com.
//
//
// .SECTION See Also
// vtkVolumeMapper vtkVolumeProVG500Mapper vtkOpenGLVolumeProVG500Mapper
// vtkVolumeProVP1000Mapper vtkOpenGLVolumeProVP1000Mapper
//


#ifndef __vtkVolumeProMapper_h
#define __vtkVolumeProMapper_h

#include "vtkVolumeMapper.h"
#include "vtkToolkits.h"
#include "vtkVersion.h"

//BTX
#if defined (VTK_HAVE_VP1000) || defined (VTK_FORCE_COMPILE_VP1000)
namespace vli3 {
#endif
class VLIContext;
class VLIVolume;
class VLILookupTable;
class VLILight;
class VLICutPlane;
#if defined (VTK_HAVE_VP1000) || defined (VTK_FORCE_COMPILE_VP1000)
}
using namespace vli3;
#endif
//ETX

#define VTK_BLEND_MODE_COMPOSITE        0
#define VTK_BLEND_MODE_MAX_INTENSITY    1
#define VTK_BLEND_MODE_MIN_INTENSITY    2

#define VTK_CURSOR_TYPE_CROSSHAIR       0
#define VTK_CURSOR_TYPE_PLANE           1

#define VTK_VOLUME_8BIT                 0
#define VTK_VOLUME_12BIT_UPPER          1
#define VTK_VOLUME_12BIT_LOWER          2
//BTX
#if ((VTK_MAJOR_VERSION == 3)&&(VTK_MINOR_VERSION == 2))
class VTK_EXPORT vtkVolumeProMapper : public vtkVolumeMapper
#else
//ETX
class VTK_RENDERING_EXPORT vtkVolumeProMapper : public vtkVolumeMapper
#endif
{
public:
  vtkTypeRevisionMacro(vtkVolumeProMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Automatically create the proper subclass
  static vtkVolumeProMapper *New();

  // Description:
  // Render the image using the hardware and place it in the frame buffer
  virtual void Render( vtkRenderer *, vtkVolume * ) {}
  
  // Description:
  // Set the blend mode
  vtkSetClampMacro( BlendMode, int,
        VTK_BLEND_MODE_COMPOSITE, VTK_BLEND_MODE_MIN_INTENSITY );
  vtkGetMacro(BlendMode,int);
  void SetBlendModeToComposite() 
        {this->SetBlendMode(VTK_BLEND_MODE_COMPOSITE);};
  void SetBlendModeToMaximumIntensity() 
        {this->SetBlendMode(VTK_BLEND_MODE_MAX_INTENSITY);};
  void SetBlendModeToMinimumIntensity() 
        {this->SetBlendMode(VTK_BLEND_MODE_MIN_INTENSITY);};
  const char *GetBlendModeAsString(void);

  // Description:
  // Set the subvolume
  vtkSetVector6Macro( SubVolume, int );
  vtkGetVectorMacro( SubVolume, int, 6 );

  // Description:
  // Turn the cursor on / off
  vtkSetClampMacro( Cursor, int, 0, 1 );
  vtkGetMacro( Cursor, int );
  vtkBooleanMacro( Cursor, int );

  // Description:
  // Set the type of the cursor
  vtkSetClampMacro( CursorType, int, 
                    VTK_CURSOR_TYPE_CROSSHAIR, VTK_CURSOR_TYPE_PLANE );
  vtkGetMacro( CursorType, int );
  void SetCursorTypeToCrossHair()
    { this->SetCursorType( VTK_CURSOR_TYPE_CROSSHAIR ); };
  void SetCursorTypeToPlane()
    { this->SetCursorType( VTK_CURSOR_TYPE_PLANE ); };
  const char *GetCursorTypeAsString( void );

  // Description:
  // Set/Get the cursor position
  vtkSetVector3Macro( CursorPosition, double );
  vtkGetVectorMacro( CursorPosition, double, 3 );

  // Description:
  // Set/Get the cursor color
  vtkSetVector3Macro( CursorXAxisColor, double );
  vtkGetVectorMacro(  CursorXAxisColor, double, 3 );
  vtkSetVector3Macro( CursorYAxisColor, double );
  vtkGetVectorMacro(  CursorYAxisColor, double, 3 );
  vtkSetVector3Macro( CursorZAxisColor, double );
  vtkGetVectorMacro(  CursorZAxisColor, double, 3 );

  // Description:
  // Turn supersampling on/off
  vtkSetClampMacro( SuperSampling, int, 0, 1 );
  vtkGetMacro( SuperSampling, int );
  vtkBooleanMacro( SuperSampling, int );

  // Description:
  // Set the supersampling factors
  void SetSuperSamplingFactor( double x, double y, double z );
  void SetSuperSamplingFactor( double f[3] )
    { this->SetSuperSamplingFactor( f[0], f[1], f[2] ); };
  vtkGetVectorMacro( SuperSamplingFactor, double, 3 );

  // Description:
  // Turn on / off the cut plane
  vtkSetClampMacro( CutPlane, int, 0, 1 );
  vtkGetMacro( CutPlane, int );
  vtkBooleanMacro( CutPlane, int );

  // Description:
  // Set/Get the cut plane equation
  vtkSetVector4Macro( CutPlaneEquation, double );
  vtkGetVectorMacro( CutPlaneEquation, double, 4 );
  
  // Description:
  // Set / Get the cut plane thickness
  vtkSetClampMacro( CutPlaneThickness, double, 0.0, 9.99e10 );
  vtkGetMacro( CutPlaneThickness, double );

  // Description:
  // Set / Get the cut plane falloff value for intensities
  vtkSetClampMacro( CutPlaneFallOffDistance, int, 0, 16 );
  vtkGetMacro( CutPlaneFallOffDistance, int );

  // Description:
  // Set/Get the gradient magnitude opacity modulation 
  vtkSetClampMacro( GradientOpacityModulation, int, 0, 1 );
  vtkGetMacro( GradientOpacityModulation, int );
  vtkBooleanMacro( GradientOpacityModulation, int );

  // Description:
  // Set/Get the gradient magnitude diffuse modulation 
  vtkSetClampMacro( GradientDiffuseModulation, int, 0, 1 );
  vtkGetMacro( GradientDiffuseModulation, int );
  vtkBooleanMacro( GradientDiffuseModulation, int );

  // Description:
  // Set/Get the gradient magnitude specular modulation 
  vtkSetClampMacro( GradientSpecularModulation, int, 0, 1 );
  vtkGetMacro( GradientSpecularModulation, int );
  vtkBooleanMacro( GradientSpecularModulation, int );

  // Description:
  // Conveniece methods for debugging
  vtkGetMacro( NoHardware, int );
  vtkGetMacro( WrongVLIVersion, int );
  
  // Description:
  // Access methods for some board info
  vtkGetMacro( NumberOfBoards, int );
  vtkGetMacro( MajorBoardVersion, int );
  vtkGetMacro( MinorBoardVersion, int );
  virtual int GetAvailableBoardMemory() { return 0; }
  virtual void GetLockSizesForBoardMemory( unsigned int vtkNotUsed(type),
                                           unsigned int * vtkNotUsed(xSize),
                                           unsigned int * vtkNotUsed(ySize),
                                           unsigned int * vtkNotUsed(zSize)) {};

  // Description:
  // Specify whether any geometry intersects the volume.
  // Does nothing with VG500
  vtkSetClampMacro(IntermixIntersectingGeometry, int, 0, 1);
  vtkGetMacro(IntermixIntersectingGeometry, int);
  vtkBooleanMacro(IntermixIntersectingGeometry, int);
  
protected:
  vtkVolumeProMapper();
  ~vtkVolumeProMapper();
 // Make sure everything is OK for rendering
  int StatusOK();

  // The volume context - create it once and keep it around
  VLIContext           *Context;

  // The Volume, and the Input that was used to build the volume
  // and the time at which it was last built.
  VLIVolume            *Volume;
  vtkImageData         *VolumeInput;
  vtkTimeStamp         *VolumeBuildTime;

  // The type of data in the volume - 8bit, 12bit upper, or 12bit lower
  int                  VolumeDataType;

  // The lookup table for RGBA - create it once then modify it as 
  // necessary
  VLILookupTable       *LookupTable;

  // The blending mode to use
  int                  BlendMode;

  // The lights, and how many of them there are. Not all of them
  // are turned on or used.
  VLILight             **Lights;
  int                  NumberOfLights;

  // The subvolume extent (xmin, xmax, ymin, ymax, zmin, zmax)
  int                  SubVolume[6];

  // The cursor parameters
  int                  Cursor;
  int                  CursorType;
  double               CursorPosition[3];
  double               CursorXAxisColor[3];
  double               CursorYAxisColor[3];
  double               CursorZAxisColor[3];
  
  // The cut plane parameters
  int                  CutPlane;
  VLICutPlane          *Cut;
  double               CutPlaneEquation[4];
  double               CutPlaneThickness;
  int                  CutPlaneFallOffDistance;

  // The supersampling parameters
  int                  SuperSampling;
  double               SuperSamplingFactor[3];

  // The gradient modulation flags
  int                  GradientOpacityModulation;
  int                  GradientDiffuseModulation;
  int                  GradientSpecularModulation;

  // Some board properties
  int                  NumberOfBoards;
  int                  MajorBoardVersion;
  int                  MinorBoardVersion;
  int                  GradientTableSize;

  // Some error conditions that may occur during initialization
  int                  NoHardware;
  int                  WrongVLIVersion;
  int                  DisplayedMessage;

  // The embedded geometry flag
  int IntermixIntersectingGeometry;
  
//BTX
#if ((VTK_MAJOR_VERSION == 3)&&(VTK_MINOR_VERSION == 2))
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE
  virtual int GetMapperType() {return VTK_FRAMEBUFFER_VOLUME_MAPPER;};
#endif
//ETX
  
private:
  vtkVolumeProMapper(const vtkVolumeProMapper&);  // Not implemented.
  void operator=(const vtkVolumeProMapper&);  // Not implemented.
};

// Description:
// Get the blending mode as a descriptive string
inline const char *vtkVolumeProMapper::GetBlendModeAsString()
{
  switch ( this->BlendMode )
    {
    case VTK_BLEND_MODE_COMPOSITE:
      return "Composite";
    case VTK_BLEND_MODE_MAX_INTENSITY:
      return "Maximum Intensity";
    case VTK_BLEND_MODE_MIN_INTENSITY:
      return "Minimum Intensity";
    default:
      return "Unknown Blend Mode";
    }
}

// Description:
// Get the cursor type as a descriptive string
inline const char *vtkVolumeProMapper::GetCursorTypeAsString()
{
  switch ( this->CursorType )
    {
    case VTK_CURSOR_TYPE_CROSSHAIR:
      return "Crosshair";
    case VTK_CURSOR_TYPE_PLANE:
      return "Plane";
    default:
      return "Unknown Cursor Type";
    }
}

#endif

